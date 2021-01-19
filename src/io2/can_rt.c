/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * CAN frame router.
 *
 * @see lely/io2/can_rt.h
 *
 * @copyright 2015-2021 Lely Industries N.V.
 *
 * @author J. S. Seldenthuis <jseldenthuis@lely.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "io2.h"

#if !LELY_NO_MALLOC

#if !LELY_NO_THREADS
#include <lely/libc/threads.h>
#endif
#include <lely/ev/exec.h>
#include <lely/ev/strand.h>
#include <lely/io2/can_rt.h>
#include <lely/io2/ctx.h>
#include <lely/util/errnum.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdlib.h>

static io_ctx_t *io_can_rt_dev_get_ctx(const io_dev_t *dev);
static ev_exec_t *io_can_rt_dev_get_exec(const io_dev_t *dev);
static size_t io_can_rt_dev_cancel(io_dev_t *dev, struct ev_task *task);
static size_t io_can_rt_dev_abort(io_dev_t *dev, struct ev_task *task);

// clang-format off
static const struct io_dev_vtbl io_can_rt_dev_vtbl = {
	&io_can_rt_dev_get_ctx,
	&io_can_rt_dev_get_exec,
	&io_can_rt_dev_cancel,
	&io_can_rt_dev_abort
};
// clang-format on

static void io_can_rt_svc_shutdown(struct io_svc *svc);

// clang-format off
static const struct io_svc_vtbl io_can_rt_svc_vtbl = {
	NULL,
	&io_can_rt_svc_shutdown
};
// clang-format on

struct io_can_rt {
	const struct io_dev_vtbl *dev_vptr;
	struct io_svc svc;
	io_ctx_t *ctx;
	io_can_chan_t *chan;
	ev_exec_t *exec;
	struct can_msg msg;
	struct can_err err;
	struct io_can_chan_read read;
	struct ev_task task;
#if !LELY_NO_THREADS
	mtx_t mtx;
#endif
	unsigned shutdown : 1;
	unsigned submitted : 1;
	struct rbtree msg_queue;
	struct sllist err_queue;
};

static void io_can_rt_read_func(struct ev_task *task);
static void io_can_rt_task_func(struct ev_task *task);

static inline io_can_rt_t *io_can_rt_from_dev(const io_dev_t *dev);
static inline io_can_rt_t *io_can_rt_from_svc(const struct io_svc *svc);

static void io_can_rt_do_pop(io_can_rt_t *rt, struct sllist *msg_queue,
		struct sllist *err_queue, struct ev_task *task);
static void io_can_rt_do_pop_read_msg(io_can_rt_t *rt, struct sllist *queue,
		struct io_can_rt_read_msg *read_msg);
static void io_can_rt_do_pop_read_err(io_can_rt_t *rt, struct sllist *queue,
		struct io_can_rt_read_err *read_err);

static void io_can_rt_read_msg_post(struct io_can_rt_read_msg *read_msg,
		const struct can_msg *msg, int errc);
static size_t io_can_rt_read_msg_queue_post(
		struct sllist *queue, const struct can_msg *msg, int errc);

static void io_can_rt_read_err_post(struct io_can_rt_read_err *read_err,
		const struct can_err *err, int errc);
static size_t io_can_rt_read_err_queue_post(
		struct sllist *queue, const struct can_err *err, int errc);

static int io_can_rt_read_msg_cmp(const void *p1, const void *p2);

struct io_can_rt_async_read_msg {
	ev_promise_t *promise;
	struct io_can_rt_read_msg read_msg;
	struct can_msg msg;
};

static void io_can_rt_async_read_msg_func(struct ev_task *task);

struct io_can_rt_async_read_err {
	ev_promise_t *promise;
	struct io_can_rt_read_err read_err;
	struct can_err err;
};

static void io_can_rt_async_read_err_func(struct ev_task *task);

struct io_can_rt_async_shutdown {
	ev_promise_t *promise;
	struct ev_task task;
};

static void io_can_rt_async_shutdown_func(struct ev_task *task);

void *
io_can_rt_alloc(void)
{
	void *ptr = malloc(sizeof(io_can_rt_t));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
io_can_rt_free(void *ptr)
{
	free(ptr);
}

io_can_rt_t *
io_can_rt_init(io_can_rt_t *rt, io_can_chan_t *chan, ev_exec_t *exec)
{
	assert(rt);
	assert(chan);
	io_dev_t *dev = io_can_chan_get_dev(chan);
	assert(dev);
	io_ctx_t *ctx = io_dev_get_ctx(dev);
	assert(ctx);
	if (!exec)
		exec = io_dev_get_exec(dev);
	assert(exec);

	int errc = 0;

	rt->dev_vptr = &io_can_rt_dev_vtbl;

	rt->svc = (struct io_svc)IO_SVC_INIT(&io_can_rt_svc_vtbl);
	rt->ctx = ctx;

	rt->chan = chan;

	rt->exec = ev_strand_create(exec);
	if (!rt->exec) {
		errc = get_errc();
		goto error_create_strand;
	}

	rt->msg = (struct can_msg)CAN_MSG_INIT;
	rt->err = (struct can_err)CAN_ERR_INIT;
	rt->read = (struct io_can_chan_read)IO_CAN_CHAN_READ_INIT(&rt->msg,
			&rt->err, NULL, rt->exec, &io_can_rt_read_func);

	rt->task = (struct ev_task)EV_TASK_INIT(rt->exec, &io_can_rt_task_func);

#if !LELY_NO_THREADS
	if (mtx_init(&rt->mtx, mtx_plain) != thrd_success) {
		errc = get_errc();
		goto error_init_mtx;
	}
#endif

	rt->shutdown = 0;
	rt->submitted = 0;

	rbtree_init(&rt->msg_queue, &io_can_rt_read_msg_cmp);
	sllist_init(&rt->err_queue);

	io_ctx_insert(rt->ctx, &rt->svc);

	return rt;

#if !LELY_NO_THREADS
	// mtx_destroy(&rt->mtx);
error_init_mtx:
#endif
	ev_strand_destroy(rt->exec);
error_create_strand:
	set_errc(errc);
	return NULL;
}

void
io_can_rt_fini(io_can_rt_t *rt)
{
	assert(rt);

	io_ctx_remove(rt->ctx, &rt->svc);

#if !LELY_NO_THREADS
	mtx_destroy(&rt->mtx);
#endif

	ev_strand_destroy(rt->exec);
}

io_can_rt_t *
io_can_rt_create(io_can_chan_t *chan, ev_exec_t *exec)
{
	int errc = 0;

	io_can_rt_t *rt = io_can_rt_alloc();
	if (!rt) {
		errc = get_errc();
		goto error_alloc;
	}

	io_can_rt_t *tmp = io_can_rt_init(rt, chan, exec);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	rt = tmp;

	return rt;

error_init:
	io_can_rt_free(rt);
error_alloc:
	set_errc(errc);
	return NULL;
}

void
io_can_rt_destroy(io_can_rt_t *rt)
{
	if (rt) {
		io_can_rt_fini(rt);
		io_can_rt_free(rt);
	}
}

io_dev_t *
io_can_rt_get_dev(const io_can_rt_t *rt)
{
	assert(rt);

	return &rt->dev_vptr;
}

io_can_chan_t *
io_can_rt_get_chan(const io_can_rt_t *rt)
{
	assert(rt);

	return rt->chan;
}

void
io_can_rt_submit_read_msg(io_can_rt_t *rt, struct io_can_rt_read_msg *read_msg)
{
	assert(rt);
	assert(read_msg);
	struct ev_task *task = &read_msg->task;

	task->exec = rt->exec;
	ev_exec_on_task_init(task->exec);

#if !LELY_NO_THREADS
	mtx_lock(&rt->mtx);
#endif
	if (rt->shutdown) {
#if !LELY_NO_THREADS
		mtx_unlock(&rt->mtx);
#endif
		io_can_rt_read_msg_post(
				read_msg, NULL, errnum2c(ERRNUM_CANCELED));
	} else {
		task->_data = &rt->msg_queue;
		struct rbnode *node = rbtree_find(&rt->msg_queue, read_msg);
		if (node) {
			read_msg = structof(
					node, struct io_can_rt_read_msg, _node);
			sllist_push_back(&read_msg->_queue, &task->_node);
		} else {
			rbnode_init(&read_msg->_node, read_msg);
			sllist_init(&read_msg->_queue);
			rbtree_insert(&rt->msg_queue, &read_msg->_node);
		}
		int submit = !rt->submitted;
		if (submit)
			rt->submitted = 1;
#if !LELY_NO_THREADS
		mtx_unlock(&rt->mtx);
#endif
		// cppcheck-suppress duplicateCondition
		if (submit)
			io_can_chan_submit_read(rt->chan, &rt->read);
	}
}

size_t
io_can_rt_cancel_read_msg(io_can_rt_t *rt, struct io_can_rt_read_msg *read_msg)
{
	assert(rt);
	assert(read_msg);
	struct ev_task *task = &read_msg->task;

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&rt->mtx);
#endif
	if (task->_data == &rt->msg_queue)
		io_can_rt_do_pop_read_msg(rt, &queue, read_msg);
#if !LELY_NO_THREADS
	mtx_unlock(&rt->mtx);
#endif

	return io_can_rt_read_msg_queue_post(
			&queue, NULL, errnum2c(ERRNUM_CANCELED));
}

size_t
io_can_rt_abort_read_msg(io_can_rt_t *rt, struct io_can_rt_read_msg *read_msg)
{
	assert(rt);
	assert(read_msg);
	struct ev_task *task = &read_msg->task;

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&rt->mtx);
#endif
	if (task->_data == &rt->msg_queue)
		io_can_rt_do_pop_read_msg(rt, &queue, read_msg);
#if !LELY_NO_THREADS
	mtx_unlock(&rt->mtx);
#endif

	return ev_task_queue_abort(&queue);
}

ev_future_t *
io_can_rt_async_read_msg(io_can_rt_t *rt, uint_least32_t id,
		uint_least8_t flags, struct io_can_rt_read_msg **pread_msg)
{
	ev_promise_t *promise = ev_promise_create(
			sizeof(struct io_can_rt_async_read_msg), NULL);
	if (!promise)
		return NULL;
	ev_future_t *future = ev_promise_get_future(promise);

	struct io_can_rt_async_read_msg *async_read_msg =
			ev_promise_data(promise);
	async_read_msg->promise = promise;
	async_read_msg->read_msg =
			(struct io_can_rt_read_msg)IO_CAN_RT_READ_MSG_INIT(id,
					flags, &io_can_rt_async_read_msg_func);
	async_read_msg->msg = (struct can_msg)CAN_MSG_INIT;

	io_can_rt_submit_read_msg(rt, &async_read_msg->read_msg);

	if (pread_msg)
		*pread_msg = &async_read_msg->read_msg;

	return future;
}

void
io_can_rt_submit_read_err(io_can_rt_t *rt, struct io_can_rt_read_err *read_err)
{
	assert(rt);
	assert(read_err);
	struct ev_task *task = &read_err->task;

	task->exec = rt->exec;
	ev_exec_on_task_init(task->exec);

#if !LELY_NO_THREADS
	mtx_lock(&rt->mtx);
#endif
	if (rt->shutdown) {
#if !LELY_NO_THREADS
		mtx_unlock(&rt->mtx);
#endif
		io_can_rt_read_err_post(
				read_err, NULL, errnum2c(ERRNUM_CANCELED));
	} else {
		task->_data = &rt->err_queue;
		sllist_push_back(&rt->err_queue, &task->_node);
		int submit = !rt->submitted;
		if (submit)
			rt->submitted = 1;
#if !LELY_NO_THREADS
		mtx_unlock(&rt->mtx);
#endif
		// cppcheck-suppress duplicateCondition
		if (submit)
			io_can_chan_submit_read(rt->chan, &rt->read);
	}
}

size_t
io_can_rt_cancel_read_err(io_can_rt_t *rt, struct io_can_rt_read_err *read_err)
{
	assert(rt);
	assert(read_err);
	struct ev_task *task = &read_err->task;

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&rt->mtx);
#endif
	if (task->_data == &rt->err_queue)
		io_can_rt_do_pop_read_err(rt, &queue, read_err);
#if !LELY_NO_THREADS
	mtx_unlock(&rt->mtx);
#endif

	return io_can_rt_read_err_queue_post(
			&queue, NULL, errnum2c(ERRNUM_CANCELED));
}

size_t
io_can_rt_abort_read_err(io_can_rt_t *rt, struct io_can_rt_read_err *read_err)
{
	assert(rt);
	assert(read_err);
	struct ev_task *task = &read_err->task;

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&rt->mtx);
#endif
	if (task->_data == &rt->err_queue)
		io_can_rt_do_pop_read_err(rt, &queue, read_err);
#if !LELY_NO_THREADS
	mtx_unlock(&rt->mtx);
#endif

	return ev_task_queue_abort(&queue);
}

ev_future_t *
io_can_rt_async_read_err(io_can_rt_t *rt, struct io_can_rt_read_err **pread_err)
{
	ev_promise_t *promise = ev_promise_create(
			sizeof(struct io_can_rt_async_read_err), NULL);
	if (!promise)
		return NULL;
	ev_future_t *future = ev_promise_get_future(promise);

	struct io_can_rt_async_read_err *async_read_err =
			ev_promise_data(promise);
	async_read_err->promise = promise;
	async_read_err->read_err =
			(struct io_can_rt_read_err)IO_CAN_RT_READ_ERR_INIT(
					&io_can_rt_async_read_err_func);
	async_read_err->err = (struct can_err)CAN_ERR_INIT;

	io_can_rt_submit_read_err(rt, &async_read_err->read_err);

	if (pread_err)
		*pread_err = &async_read_err->read_err;

	return future;
}

ev_future_t *
io_can_rt_async_shutdown(io_can_rt_t *rt)
{
	ev_promise_t *promise = ev_promise_create(
			sizeof(struct io_can_rt_async_shutdown), NULL);
	if (!promise)
		return NULL;
	ev_future_t *future = ev_promise_get_future(promise);

	struct io_can_rt_async_shutdown *async_shutdown =
			ev_promise_data(promise);
	async_shutdown->promise = promise;
	async_shutdown->task = (struct ev_task)EV_TASK_INIT(
			rt->exec, &io_can_rt_async_shutdown_func);

	io_can_rt_svc_shutdown(&rt->svc);
	ev_exec_post(async_shutdown->task.exec, &async_shutdown->task);

	return future;
}

struct io_can_rt_read_msg *
io_can_rt_read_msg_from_task(struct ev_task *task)
{
	return task ? structof(task, struct io_can_rt_read_msg, task) : NULL;
}

struct io_can_rt_read_err *
io_can_rt_read_err_from_task(struct ev_task *task)
{
	return task ? structof(task, struct io_can_rt_read_err, task) : NULL;
}

static io_ctx_t *
io_can_rt_dev_get_ctx(const io_dev_t *dev)
{
	const io_can_rt_t *rt = io_can_rt_from_dev(dev);

	return rt->ctx;
}

static ev_exec_t *
io_can_rt_dev_get_exec(const io_dev_t *dev)
{
	const io_can_rt_t *rt = io_can_rt_from_dev(dev);

	return rt->exec;
}

static size_t
io_can_rt_dev_cancel(io_dev_t *dev, struct ev_task *task)
{
	io_can_rt_t *rt = io_can_rt_from_dev(dev);

	struct sllist msg_queue, err_queue;
	sllist_init(&msg_queue);
	sllist_init(&err_queue);

#if !LELY_NO_THREADS
	mtx_lock(&rt->mtx);
#endif
	io_can_rt_do_pop(rt, &msg_queue, &err_queue, task);
#if !LELY_NO_THREADS
	mtx_unlock(&rt->mtx);
#endif
	size_t nmsg = io_can_rt_read_msg_queue_post(
			&msg_queue, NULL, errnum2c(ERRNUM_CANCELED));
	size_t nerr = io_can_rt_read_err_queue_post(
			&err_queue, NULL, errnum2c(ERRNUM_CANCELED));
	return nerr < SIZE_MAX - nmsg ? nmsg + nerr : SIZE_MAX;
}

static size_t
io_can_rt_dev_abort(io_dev_t *dev, struct ev_task *task)
{
	io_can_rt_t *rt = io_can_rt_from_dev(dev);

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&rt->mtx);
#endif
	io_can_rt_do_pop(rt, &queue, &queue, task);
#if !LELY_NO_THREADS
	mtx_unlock(&rt->mtx);
#endif
	return ev_task_queue_abort(&queue);
}

static void
io_can_rt_svc_shutdown(struct io_svc *svc)
{
	io_can_rt_t *rt = io_can_rt_from_svc(svc);
	io_dev_t *dev = &rt->dev_vptr;

#if !LELY_NO_THREADS
	mtx_lock(&rt->mtx);
#endif
	int shutdown = !rt->shutdown;
	rt->shutdown = 1;
	// Abort io_can_rt_read_func().
	if (shutdown && rt->submitted
			&& io_can_chan_abort_read(rt->chan, &rt->read))
		rt->submitted = 0;
#if !LELY_NO_THREADS
	mtx_unlock(&rt->mtx);
#endif

	if (shutdown)
		// Cancel all pending operations.
		io_can_rt_dev_cancel(dev, NULL);
}

static void
io_can_rt_read_func(struct ev_task *task)
{
	assert(task);
	struct io_can_chan_read *read = io_can_chan_read_from_task(task);
	io_can_rt_t *rt = structof(read, io_can_rt_t, read);

	if (rt->read.r.result > 0) {
		struct sllist queue;
		sllist_init(&queue);
		struct io_can_rt_read_msg key;
		key.id = rt->read.msg->id;
		key.flags = rt->read.msg->flags;
#if !LELY_NO_THREADS
		mtx_lock(&rt->mtx);
#endif
		struct rbnode *node = rbtree_find(&rt->msg_queue, &key);
		if (node) {
			rbtree_remove(&rt->msg_queue, node);
			struct io_can_rt_read_msg *read_msg = structof(
					node, struct io_can_rt_read_msg, _node);
			sllist_push_back(&queue, &read_msg->task._node);
			sllist_append(&queue, &read_msg->_queue);
		}
#if !LELY_NO_THREADS
		mtx_unlock(&rt->mtx);
#endif
		io_can_rt_read_msg_queue_post(&queue, rt->read.msg, 0);
	} else if (!rt->read.r.result) {
		struct sllist queue;
		sllist_init(&queue);
#if !LELY_NO_THREADS
		mtx_lock(&rt->mtx);
#endif
		sllist_append(&queue, &rt->err_queue);
#if !LELY_NO_THREADS
		mtx_unlock(&rt->mtx);
#endif
		io_can_rt_read_err_queue_post(&queue, rt->read.err, 0);
	} else if (rt->read.r.errc) {
		struct sllist msg_queue, err_queue;
		sllist_init(&msg_queue);
		sllist_init(&err_queue);
#if !LELY_NO_THREADS
		mtx_lock(&rt->mtx);
#endif
		io_can_rt_do_pop(rt, &msg_queue, &err_queue, NULL);
#if !LELY_NO_THREADS
		mtx_unlock(&rt->mtx);
#endif
		io_can_rt_read_msg_queue_post(
				&msg_queue, NULL, rt->read.r.errc);
		io_can_rt_read_err_queue_post(
				&err_queue, NULL, rt->read.r.errc);
	}

	ev_exec_post(rt->task.exec, &rt->task);
}

static void
io_can_rt_task_func(struct ev_task *task)
{
	assert(task);
	io_can_rt_t *rt = structof(task, io_can_rt_t, task);

#if !LELY_NO_THREADS
	mtx_lock(&rt->mtx);
#endif
	assert(rt->submitted);
	int submit = rt->submitted =
			(!rbtree_empty(&rt->msg_queue)
					|| !sllist_empty(&rt->err_queue))
			&& !rt->shutdown;
#if !LELY_NO_THREADS
	mtx_unlock(&rt->mtx);
#endif

	if (submit)
		io_can_chan_submit_read(rt->chan, &rt->read);
}

static inline io_can_rt_t *
io_can_rt_from_dev(const io_dev_t *dev)
{
	assert(dev);

	return structof(dev, io_can_rt_t, dev_vptr);
}

static inline io_can_rt_t *
io_can_rt_from_svc(const struct io_svc *svc)
{
	assert(svc);

	return structof(svc, io_can_rt_t, svc);
}

static void
io_can_rt_do_pop(io_can_rt_t *rt, struct sllist *msg_queue,
		struct sllist *err_queue, struct ev_task *task)
{
	assert(rt);
	assert(msg_queue);
	assert(err_queue);

	if (!task) {
		rbtree_foreach (&rt->msg_queue, node) {
			rbtree_remove(&rt->msg_queue, node);
			struct io_can_rt_read_msg *read_msg = structof(
					node, struct io_can_rt_read_msg, _node);
			sllist_push_back(msg_queue, &read_msg->task._node);
			sllist_append(msg_queue, &read_msg->_queue);
		}
		sllist_append(err_queue, &rt->err_queue);
		if (rt->submitted)
			io_can_chan_cancel_read(rt->chan, &rt->read);
	} else if (task->_data == &rt->msg_queue) {
		io_can_rt_do_pop_read_msg(rt, msg_queue,
				io_can_rt_read_msg_from_task(task));
	} else if (task->_data == &rt->err_queue) {
		io_can_rt_do_pop_read_err(rt, err_queue,
				io_can_rt_read_err_from_task(task));
	}
}

static void
io_can_rt_do_pop_read_msg(io_can_rt_t *rt, struct sllist *queue,
		struct io_can_rt_read_msg *read_msg)
{
	assert(rt);
	assert(queue);
	assert(read_msg);
	struct ev_task *task = &read_msg->task;
	assert(task->_data == &rt->msg_queue);

	struct rbnode *node = rbtree_find(&rt->msg_queue, read_msg);
	if (node == &read_msg->_node) {
		rbtree_remove(&rt->msg_queue, &read_msg->_node);
		if (!sllist_empty(&read_msg->_queue)) {
			struct sllist queue = read_msg->_queue;
			read_msg = io_can_rt_read_msg_from_task(
					ev_task_from_node(sllist_pop_front(
							&queue)));
			rbnode_init(&read_msg->_node, read_msg);
			sllist_init(&read_msg->_queue);
			sllist_append(&read_msg->_queue, &queue);
			rbtree_insert(&rt->msg_queue, &read_msg->_node);
		} else if (rt->submitted && rbtree_empty(&rt->msg_queue)
				&& sllist_empty(&rt->err_queue)) {
			io_can_chan_cancel_read(rt->chan, &rt->read);
		}
		task->_data = NULL;
		sllist_push_back(queue, &task->_node);
	} else if (node) {
		read_msg = structof(node, struct io_can_rt_read_msg, _node);
		if (sllist_remove(&read_msg->_queue, &task->_node)) {
			task->_data = NULL;
			sllist_push_back(queue, &task->_node);
		}
	}
}

static void
io_can_rt_do_pop_read_err(io_can_rt_t *rt, struct sllist *queue,
		struct io_can_rt_read_err *read_err)
{
	assert(rt);
	assert(queue);
	assert(read_err);
	struct ev_task *task = &read_err->task;
	assert(task->_data == &rt->err_queue);

	if (sllist_remove(&rt->err_queue, &task->_node)) {
		if (rt->submitted && rbtree_empty(&rt->msg_queue)
				&& sllist_empty(&rt->err_queue))
			io_can_chan_cancel_read(rt->chan, &rt->read);
		task->_data = NULL;
		sllist_push_back(queue, &task->_node);
	}
}

static void
io_can_rt_read_msg_post(struct io_can_rt_read_msg *read_msg,
		const struct can_msg *msg, int errc)
{
	read_msg->r.msg = msg;
	read_msg->r.errc = errc;

	ev_exec_t *exec = read_msg->task.exec;
	ev_exec_post(exec, &read_msg->task);
	ev_exec_on_task_fini(exec);
}

static size_t
io_can_rt_read_msg_queue_post(
		struct sllist *queue, const struct can_msg *msg, int errc)
{
	size_t n = 0;

	struct slnode *node;
	while ((node = sllist_pop_front(queue))) {
		struct ev_task *task = ev_task_from_node(node);
		struct io_can_rt_read_msg *read_msg =
				io_can_rt_read_msg_from_task(task);
		io_can_rt_read_msg_post(read_msg, msg, errc);
		n += n < SIZE_MAX;
	}

	return n;
}

static void
io_can_rt_read_err_post(struct io_can_rt_read_err *read_err,
		const struct can_err *err, int errc)
{
	read_err->r.err = err;
	read_err->r.errc = errc;

	ev_exec_t *exec = read_err->task.exec;
	ev_exec_post(exec, &read_err->task);
	ev_exec_on_task_fini(exec);
}

static size_t
io_can_rt_read_err_queue_post(
		struct sllist *queue, const struct can_err *err, int errc)
{
	size_t n = 0;

	struct slnode *node;
	while ((node = sllist_pop_front(queue))) {
		struct ev_task *task = ev_task_from_node(node);
		struct io_can_rt_read_err *read_err =
				io_can_rt_read_err_from_task(task);
		io_can_rt_read_err_post(read_err, err, errc);
		n += n < SIZE_MAX;
	}

	return n;
}

static int
io_can_rt_read_msg_cmp(const void *p1, const void *p2)
{
	const struct io_can_rt_read_msg *r1 = p1;
	assert(r1);
	const struct io_can_rt_read_msg *r2 = p2;
	assert(r2);

	int cmp = (r2->id < r1->id) - (r1->id < r2->id);
	if (!cmp)
		cmp = (r2->flags < r1->flags) - (r1->flags < r2->flags);
	return cmp;
}

static void
io_can_rt_async_read_msg_func(struct ev_task *task)
{
	assert(task);
	struct io_can_rt_read_msg *read_msg =
			io_can_rt_read_msg_from_task(task);
	struct io_can_rt_async_read_msg *async_read_msg = structof(
			read_msg, struct io_can_rt_async_read_msg, read_msg);

	if (ev_promise_set_acquire(async_read_msg->promise)) {
		if (read_msg->r.msg) {
			async_read_msg->msg = *read_msg->r.msg;
			read_msg->r.msg = &async_read_msg->msg;
		}
		ev_promise_set_release(async_read_msg->promise, &read_msg->r);
	}
	ev_promise_release(async_read_msg->promise);
}

static void
io_can_rt_async_read_err_func(struct ev_task *task)
{
	assert(task);
	struct io_can_rt_read_err *read_err =
			io_can_rt_read_err_from_task(task);
	struct io_can_rt_async_read_err *async_read_err = structof(
			read_err, struct io_can_rt_async_read_err, read_err);

	if (ev_promise_set_acquire(async_read_err->promise)) {
		if (read_err->r.err) {
			async_read_err->err = *read_err->r.err;
			read_err->r.err = &async_read_err->err;
		}
		ev_promise_set_release(async_read_err->promise, &read_err->r);
	}
	ev_promise_release(async_read_err->promise);
}

static void
io_can_rt_async_shutdown_func(struct ev_task *task)
{
	assert(task);
	struct io_can_rt_async_shutdown *async_shutdown =
			structof(task, struct io_can_rt_async_shutdown, task);

	ev_promise_set(async_shutdown->promise, NULL);
	ev_promise_release(async_shutdown->promise);
}

#endif // !LELY_NO_MALLOC
