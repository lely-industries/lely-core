/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * timer queue.
 *
 * @see lely/io2/tqueue.h
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
#include <lely/io2/ctx.h>
#include <lely/io2/tqueue.h>
#include <lely/util/errnum.h>
#include <lely/util/time.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdlib.h>

static io_ctx_t *io_tqueue_dev_get_ctx(const io_dev_t *dev);
static ev_exec_t *io_tqueue_dev_get_exec(const io_dev_t *dev);
static size_t io_tqueue_dev_cancel(io_dev_t *dev, struct ev_task *task);
static size_t io_tqueue_dev_abort(io_dev_t *dev, struct ev_task *task);

// clang-format off
static const struct io_dev_vtbl io_tqueue_dev_vtbl = {
	&io_tqueue_dev_get_ctx,
	&io_tqueue_dev_get_exec,
	&io_tqueue_dev_cancel,
	&io_tqueue_dev_abort
};
// clang-format on

static void io_tqueue_svc_shutdown(struct io_svc *svc);

// clang-format off
static const struct io_svc_vtbl io_tqueue_svc_vtbl = {
	NULL,
	&io_tqueue_svc_shutdown
};
// clang-format on

struct io_tqueue {
	const struct io_dev_vtbl *dev_vptr;
	struct io_svc svc;
	io_ctx_t *ctx;
	io_timer_t *timer;
	ev_exec_t *exec;
	struct io_timer_wait wait;
#if !LELY_NO_THREADS
	mtx_t mtx;
#endif
	unsigned shutdown : 1;
	unsigned submitted : 1;
	struct timespec next;
	struct pheap queue;
};

static void io_tqueue_wait_func(struct ev_task *task);

static inline io_tqueue_t *io_tqueue_from_dev(const io_dev_t *dev);
static inline io_tqueue_t *io_tqueue_from_svc(const struct io_svc *svc);

static void io_tqueue_pop(
		io_tqueue_t *tq, struct sllist *queue, struct ev_task *task);
static void io_tqueue_do_pop_wait(io_tqueue_t *tq, struct sllist *queue,
		struct io_tqueue_wait *wait);

static inline void io_tqueue_wait_post(struct io_tqueue_wait *wait, int errc);
static inline size_t io_tqueue_wait_queue_post(struct sllist *queue, int errc);

struct io_tqueue_async_wait {
	ev_promise_t *promise;
	struct io_tqueue_wait wait;
};

static void io_tqueue_async_wait_func(struct ev_task *task);

void *
io_tqueue_alloc(void)
{
	void *ptr = malloc(sizeof(io_tqueue_t));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
io_tqueue_free(void *ptr)
{
	free(ptr);
}

io_tqueue_t *
io_tqueue_init(io_tqueue_t *tq, io_timer_t *timer, ev_exec_t *exec)
{
	assert(tq);
	assert(timer);
	io_dev_t *dev = io_timer_get_dev(timer);
	assert(dev);
	io_ctx_t *ctx = io_dev_get_ctx(dev);
	assert(ctx);
	if (!exec)
		exec = io_dev_get_exec(dev);
	assert(exec);

	tq->dev_vptr = &io_tqueue_dev_vtbl;

	tq->svc = (struct io_svc)IO_SVC_INIT(&io_tqueue_svc_vtbl);
	tq->ctx = ctx;

	tq->timer = timer;

	tq->exec = exec;

	tq->wait = (struct io_timer_wait)IO_TIMER_WAIT_INIT(
			tq->exec, &io_tqueue_wait_func);

#if !LELY_NO_THREADS
	if (mtx_init(&tq->mtx, mtx_plain) != thrd_success)
		return NULL;
#endif

	tq->shutdown = 0;
	tq->submitted = 0;

	tq->next = (struct timespec){ 0, 0 };

	pheap_init(&tq->queue, &timespec_cmp);

	io_ctx_insert(tq->ctx, &tq->svc);

	return tq;
}

void
io_tqueue_fini(io_tqueue_t *tq)
{
	assert(tq);

	io_ctx_remove(tq->ctx, &tq->svc);
	// Cancel all pending operations.
	io_tqueue_svc_shutdown(&tq->svc);

#if !LELY_NO_THREADS
	mtx_lock(&tq->mtx);
	// If necessary, busy-wait until If io_tqueue_wait_func() completes.
	while (tq->submitted
			&& !ev_exec_abort(tq->wait.task.exec, &tq->wait.task)) {
		mtx_unlock(&tq->mtx);
		thrd_yield();
		mtx_lock(&tq->mtx);
	}
	mtx_unlock(&tq->mtx);

	mtx_destroy(&tq->mtx);
#endif
}

io_tqueue_t *
io_tqueue_create(io_timer_t *timer, ev_exec_t *exec)
{
	int errc = 0;

	io_tqueue_t *tq = io_tqueue_alloc();
	if (!tq) {
		errc = get_errc();
		goto error_alloc;
	}

	io_tqueue_t *tmp = io_tqueue_init(tq, timer, exec);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	tq = tmp;

	return tq;

error_init:
	io_tqueue_free(tq);
error_alloc:
	set_errc(errc);
	return NULL;
}

void
io_tqueue_destroy(io_tqueue_t *tq)
{
	if (tq) {
		io_tqueue_fini(tq);
		io_tqueue_free(tq);
	}
}

io_dev_t *
io_tqueue_get_dev(const io_tqueue_t *tq)
{
	assert(tq);

	return &tq->dev_vptr;
}

io_timer_t *
io_tqueue_get_timer(const io_tqueue_t *tq)
{
	assert(tq);

	return tq->timer;
}

void
io_tqueue_submit_wait(io_tqueue_t *tq, struct io_tqueue_wait *wait)
{
	assert(tq);
	assert(wait);
	struct ev_task *task = &wait->task;

	if (!task->exec)
		task->exec = tq->exec;
	ev_exec_on_task_init(task->exec);

#if !LELY_NO_THREADS
	mtx_lock(&tq->mtx);
#endif
	if (tq->shutdown) {
#if !LELY_NO_THREADS
		mtx_unlock(&tq->mtx);
#endif
		io_tqueue_wait_post(wait, errnum2c(ERRNUM_CANCELED));
	} else {
		int errsv = get_errc();
		int errc = 0;
		struct sllist queue;
		sllist_init(&queue);

		task->_data = &tq->queue;
		pnode_init(&wait->_node, &wait->value);
		pheap_insert(&tq->queue, &wait->_node);

		if ((!tq->next.tv_sec && !tq->next.tv_nsec)
				|| timespec_cmp(&wait->value, &tq->next) < 0) {
			tq->next = wait->value;
			struct itimerspec value = { { 0, 0 }, tq->next };
			// clang-format off
			if (io_timer_settime(tq->timer, TIMER_ABSTIME, &value,
					NULL) == -1) {
				// clang-format on
				tq->next = (struct timespec){ 0, 0 };
				errc = get_errc();
				io_tqueue_do_pop_wait(tq, &queue, NULL);
			}
		}

		int submit = !tq->submitted && !pheap_empty(&tq->queue);
		if (submit)
			tq->submitted = 1;
#if !LELY_NO_THREADS
		mtx_unlock(&tq->mtx);
#endif
		// cppcheck-suppress duplicateCondition
		if (submit)
			io_timer_submit_wait(tq->timer, &tq->wait);

		io_tqueue_wait_queue_post(&queue, errc);
		set_errc(errsv);
	}
}

size_t
io_tqueue_cancel_wait(io_tqueue_t *tq, struct io_tqueue_wait *wait)
{
	assert(tq);

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&tq->mtx);
#endif
	io_tqueue_do_pop_wait(tq, &queue, wait);
#if !LELY_NO_THREADS
	mtx_unlock(&tq->mtx);
#endif

	return io_tqueue_wait_queue_post(&queue, errnum2c(ERRNUM_CANCELED));
}

size_t
io_tqueue_abort_wait(io_tqueue_t *tq, struct io_tqueue_wait *wait)
{
	assert(tq);

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&tq->mtx);
#endif
	io_tqueue_do_pop_wait(tq, &queue, wait);
#if !LELY_NO_THREADS
	mtx_unlock(&tq->mtx);
#endif

	return ev_task_queue_abort(&queue);
}

ev_future_t *
io_tqueue_async_wait(io_tqueue_t *tq, ev_exec_t *exec,
		const struct timespec *value, struct io_tqueue_wait **pwait)
{
	assert(value);

	ev_promise_t *promise = ev_promise_create(
			sizeof(struct io_tqueue_async_wait), NULL);
	if (!promise)
		return NULL;
	ev_future_t *future = ev_promise_get_future(promise);

	struct io_tqueue_async_wait *async_wait = ev_promise_data(promise);
	async_wait->promise = promise;
	async_wait->wait = (struct io_tqueue_wait)IO_TQUEUE_WAIT_INIT(
			value->tv_sec, value->tv_nsec, exec,
			&io_tqueue_async_wait_func);

	io_tqueue_submit_wait(tq, &async_wait->wait);

	if (pwait)
		*pwait = &async_wait->wait;

	return future;
}

struct io_tqueue_wait *
io_tqueue_wait_from_task(struct ev_task *task)
{
	return task ? structof(task, struct io_tqueue_wait, task) : NULL;
}

static io_ctx_t *
io_tqueue_dev_get_ctx(const io_dev_t *dev)
{
	const io_tqueue_t *tq = io_tqueue_from_dev(dev);

	return tq->ctx;
}

static ev_exec_t *
io_tqueue_dev_get_exec(const io_dev_t *dev)
{
	const io_tqueue_t *tq = io_tqueue_from_dev(dev);

	return tq->exec;
}

static size_t
io_tqueue_dev_cancel(io_dev_t *dev, struct ev_task *task)
{
	io_tqueue_t *tq = io_tqueue_from_dev(dev);

	struct sllist queue;
	sllist_init(&queue);

	io_tqueue_pop(tq, &queue, task);

	return io_tqueue_wait_queue_post(&queue, errnum2c(ERRNUM_CANCELED));
}

static size_t
io_tqueue_dev_abort(io_dev_t *dev, struct ev_task *task)
{
	io_tqueue_t *tq = io_tqueue_from_dev(dev);

	struct sllist queue;
	sllist_init(&queue);

	io_tqueue_pop(tq, &queue, task);

	return ev_task_queue_abort(&queue);
}

static void
io_tqueue_svc_shutdown(struct io_svc *svc)
{
	io_tqueue_t *tq = io_tqueue_from_svc(svc);
	io_dev_t *dev = &tq->dev_vptr;

#if !LELY_NO_THREADS
	mtx_lock(&tq->mtx);
#endif
	int shutdown = !tq->shutdown;
	tq->shutdown = 1;
	// Abort io_tqueue_wait_func().
	if (shutdown && tq->submitted
			&& io_timer_abort_wait(tq->timer, &tq->wait))
		tq->submitted = 0;
#if !LELY_NO_THREADS
	mtx_unlock(&tq->mtx);
#endif

	if (shutdown)
		// Cancel all pending operations.
		io_tqueue_dev_cancel(dev, NULL);
}

static void
io_tqueue_wait_func(struct ev_task *task)
{
	struct io_timer_wait *wait = io_timer_wait_from_task(task);
	assert(wait);
	io_tqueue_t *tq = structof(wait, io_tqueue_t, wait);

	int errsv = get_errc();
	int errc = 0;
	struct sllist queue;
	sllist_init(&queue);

	struct timespec now = { 0, 0 };
	if (io_clock_gettime(io_timer_get_clock(tq->timer), &now) == -1) {
		errc = get_errc();
#if !LELY_NO_THREADS
		mtx_lock(&tq->mtx);
#endif
		assert(tq->submitted);
		tq->submitted = 0;
		tq->next = (struct timespec){ 0, 0 };

		io_tqueue_do_pop_wait(tq, &queue, NULL);
#if !LELY_NO_THREADS
		mtx_unlock(&tq->mtx);
#endif
	} else {
#if !LELY_NO_THREADS
		mtx_lock(&tq->mtx);
#endif
		assert(tq->submitted);
		tq->submitted = 0;
		tq->next = (struct timespec){ 0, 0 };

		struct pnode *node;
		while ((node = pheap_first(&tq->queue))) {
			struct io_tqueue_wait *wait = structof(
					node, struct io_tqueue_wait, _node);
			if (timespec_cmp(&now, &wait->value) < 0)
				break;
			pheap_remove(&tq->queue, node);
			wait->task._data = NULL;
			sllist_push_back(&queue, &wait->task._node);
		}

		if (node) {
			struct io_tqueue_wait *wait = structof(
					node, struct io_tqueue_wait, _node);
			tq->next = wait->value;
			struct itimerspec value = { { 0, 0 }, tq->next };
			// clang-format off
			if (io_timer_settime(tq->timer, TIMER_ABSTIME, &value,
					NULL) == -1) {
				// clang-format on
				tq->next = (struct timespec){ 0, 0 };
				errc = get_errc();
				io_tqueue_do_pop_wait(tq, &queue, NULL);
			}
		}

		int submit = tq->submitted =
				!pheap_empty(&tq->queue) && !tq->shutdown;
#if !LELY_NO_THREADS
		mtx_unlock(&tq->mtx);
#endif
		if (submit)
			io_timer_submit_wait(tq->timer, &tq->wait);
	}

	io_tqueue_wait_queue_post(&queue, errc);

	set_errc(errsv);
}

static inline io_tqueue_t *
io_tqueue_from_dev(const io_dev_t *dev)
{
	assert(dev);

	return structof(dev, io_tqueue_t, dev_vptr);
}

static inline io_tqueue_t *
io_tqueue_from_svc(const struct io_svc *svc)
{
	assert(svc);

	return structof(svc, io_tqueue_t, svc);
}

static void
io_tqueue_pop(io_tqueue_t *tq, struct sllist *queue, struct ev_task *task)
{
	assert(tq);
	assert(queue);

#if !LELY_NO_THREADS
	mtx_lock(&tq->mtx);
#endif
	io_tqueue_do_pop_wait(tq, queue, io_tqueue_wait_from_task(task));
#if !LELY_NO_THREADS
	mtx_unlock(&tq->mtx);
#endif
}

static void
io_tqueue_do_pop_wait(io_tqueue_t *tq, struct sllist *queue,
		struct io_tqueue_wait *wait)
{
	assert(tq);
	assert(queue);

	if (!wait) {
		struct pnode *node;
		while ((node = pheap_first(&tq->queue))) {
			pheap_remove(&tq->queue, node);
			struct io_tqueue_wait *wait = structof(
					node, struct io_tqueue_wait, _node);
			wait->task._data = NULL;
			sllist_push_back(queue, &wait->task._node);
		}
	} else if (wait->task._data == &tq->queue) {
		pheap_remove(&tq->queue, &wait->_node);
		wait->task._data = NULL;
		sllist_push_back(queue, &wait->task._node);
	}

	if (pheap_empty(&tq->queue) && tq->submitted)
		io_timer_cancel_wait(tq->timer, &tq->wait);
}

static inline void
io_tqueue_wait_post(struct io_tqueue_wait *wait, int errc)
{
	wait->task._data = NULL;
	wait->errc = errc;

	ev_exec_t *exec = wait->task.exec;
	ev_exec_post(exec, &wait->task);
	ev_exec_on_task_fini(exec);
}

static inline size_t
io_tqueue_wait_queue_post(struct sllist *queue, int errc)
{
	size_t n = 0;

	struct slnode *node;
	while ((node = sllist_pop_front(queue))) {
		struct ev_task *task = ev_task_from_node(node);
		struct io_tqueue_wait *wait = io_tqueue_wait_from_task(task);
		io_tqueue_wait_post(wait, errc);
		n += n < SIZE_MAX;
	}

	return n;
}

static void
io_tqueue_async_wait_func(struct ev_task *task)
{
	assert(task);
	struct io_tqueue_wait *wait = io_tqueue_wait_from_task(task);
	struct io_tqueue_async_wait *async_wait =
			structof(wait, struct io_tqueue_async_wait, wait);

	ev_promise_set(async_wait->promise, &wait->errc);
	ev_promise_release(async_wait->promise);
}

#endif // !LELY_NO_MALLOC
