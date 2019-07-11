/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * user-defined CAN channel.
 *
 * @see lely/io2/user/can.h
 *
 * @copyright 2015-2019 Lely Industries N.V.
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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or useried.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../can.h"
#if !LELY_NO_THREADS
#include <lely/libc/threads.h>
#endif
#include <lely/io2/ctx.h>
#include <lely/io2/user/can.h>
#include <lely/util/errnum.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdlib.h>

#include "../cbuf.h"

#ifndef LELY_IO_USER_CAN_RXLEN
/**
 * The default receive queue length (in number of CAN frames) of the
 * user-defined CAN channel.
 */
#define LELY_IO_USER_CAN_RXLEN 1024
#endif

struct io_user_can_frame {
	int is_err;
	union {
		struct can_msg msg;
		struct can_err err;
	} u;
	struct timespec ts;
};

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
LELY_IO_DEFINE_CBUF(io_user_can_buf, struct io_user_can_frame)
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

static io_ctx_t *io_user_can_chan_dev_get_ctx(const io_dev_t *dev);
static ev_exec_t *io_user_can_chan_dev_get_exec(const io_dev_t *dev);
static size_t io_user_can_chan_dev_cancel(io_dev_t *dev, struct ev_task *task);
static size_t io_user_can_chan_dev_abort(io_dev_t *dev, struct ev_task *task);

// clang-format off
static const struct io_dev_vtbl io_user_can_chan_dev_vtbl = {
	&io_user_can_chan_dev_get_ctx,
	&io_user_can_chan_dev_get_exec,
	&io_user_can_chan_dev_cancel,
	&io_user_can_chan_dev_abort
};
// clang-format on

static io_dev_t *io_user_can_chan_get_dev(const io_can_chan_t *chan);
static int io_user_can_chan_get_flags(const io_can_chan_t *chan);
static int io_user_can_chan_read(io_can_chan_t *chan, struct can_msg *msg,
		struct can_err *err, struct timespec *tp, int timeout);
static void io_user_can_chan_submit_read(
		io_can_chan_t *chan, struct io_can_chan_read *read);
static int io_user_can_chan_write(
		io_can_chan_t *chan, const struct can_msg *msg, int timeout);
static void io_user_can_chan_submit_write(
		io_can_chan_t *chan, struct io_can_chan_write *write);

// clang-format off
static const struct io_can_chan_vtbl io_user_can_chan_vtbl = {
	&io_user_can_chan_get_dev,
	&io_user_can_chan_get_flags,
	&io_user_can_chan_read,
	&io_user_can_chan_submit_read,
	&io_user_can_chan_write,
	&io_user_can_chan_submit_write
};
// clang-format on

static void io_user_can_chan_svc_shutdown(struct io_svc *svc);

// clang-format off
static const struct io_svc_vtbl io_user_can_chan_svc_vtbl = {
	NULL,
	&io_user_can_chan_svc_shutdown
};
// clang-format on

struct io_user_can_chan {
	const struct io_dev_vtbl *dev_vptr;
	const struct io_can_chan_vtbl *chan_vptr;
	struct io_svc svc;
	io_ctx_t *ctx;
	ev_exec_t *exec;
	int flags;
	int txtimeo;
	io_user_can_chan_write_t *func;
	void *arg;
	struct ev_task write_task;
#if !LELY_NO_THREADS
	mtx_t mtx;
#endif
	unsigned shutdown : 1;
	unsigned write_posted : 1;
	struct sllist read_queue;
	struct sllist write_queue;
	struct io_user_can_buf rxbuf;
	struct ev_task *current_task;
};

static void io_user_can_chan_write_task_func(struct ev_task *task);

static inline struct io_user_can_chan *io_user_can_chan_from_dev(
		const io_dev_t *dev);
static inline struct io_user_can_chan *io_user_can_chan_from_chan(
		const io_can_chan_t *chan);
static inline struct io_user_can_chan *io_user_can_chan_from_svc(
		const struct io_svc *svc);

static void io_user_can_chan_do_pop(struct io_user_can_chan *user,
		struct sllist *read_queue, struct sllist *write_queue,
		struct ev_task *task);

void *
io_user_can_chan_alloc(void)
{
	struct io_user_can_chan *user = malloc(sizeof(*user));
	if (!user)
		set_errc(errno2c(errno));
	return user ? &user->chan_vptr : NULL;
}

void
io_user_can_chan_free(void *ptr)
{
	if (ptr)
		free(io_user_can_chan_from_chan(ptr));
}

io_can_chan_t *
io_user_can_chan_init(io_can_chan_t *chan, io_ctx_t *ctx, ev_exec_t *exec,
		int flags, size_t rxlen, int txtimeo,
		io_user_can_chan_write_t *func, void *arg)
{
	struct io_user_can_chan *user = io_user_can_chan_from_chan(chan);
	assert(ctx);
	assert(exec);

	if (flags & ~IO_CAN_BUS_FLAG_MASK) {
		set_errnum(ERRNUM_INVAL);
		return NULL;
	}

	if (!rxlen)
		rxlen = LELY_IO_USER_CAN_RXLEN;

	if (!txtimeo)
		txtimeo = LELY_IO_TX_TIMEOUT;

	int errsv = 0;

	user->dev_vptr = &io_user_can_chan_dev_vtbl;
	user->chan_vptr = &io_user_can_chan_vtbl;

	user->svc = (struct io_svc)IO_SVC_INIT(&io_user_can_chan_svc_vtbl);
	user->ctx = ctx;

	user->exec = exec;

	user->flags = flags;
	user->txtimeo = txtimeo;

	user->func = func;
	user->arg = arg;

	user->write_task = (struct ev_task)EV_TASK_INIT(
			user->exec, &io_user_can_chan_write_task_func);

#if !LELY_NO_THREADS
	if (mtx_init(&user->mtx, mtx_plain) != thrd_success) {
		errsv = get_errc();
		goto error_init_mtx;
	}
#endif

	user->shutdown = 0;
	user->write_posted = 0;

	sllist_init(&user->read_queue);
	sllist_init(&user->write_queue);

	if (io_user_can_buf_init(&user->rxbuf, rxlen) == -1) {
		errsv = get_errc();
		goto error_init_rxbuf;
	}

	io_ctx_insert(user->ctx, &user->svc);

	user->current_task = NULL;

	return chan;

	io_user_can_buf_fini(&user->rxbuf);
error_init_rxbuf:
#if !LELY_NO_THREADS
	mtx_destroy(&user->mtx);
error_init_mtx:
#endif
	set_errc(errsv);
	return NULL;
}

void
io_user_can_chan_fini(io_can_chan_t *chan)
{
	struct io_user_can_chan *user = io_user_can_chan_from_chan(chan);

	io_ctx_remove(user->ctx, &user->svc);
	// Cancel all pending tasks.
	io_user_can_chan_svc_shutdown(&user->svc);

#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
	// If necessary, busy-wait until io_user_can_chan_write_task_func()
	// completes.
	while (user->write_posted) {
		mtx_unlock(&user->mtx);
		thrd_yield();
		mtx_lock(&user->mtx);
	}
	mtx_unlock(&user->mtx);
#endif

	io_user_can_buf_fini(&user->rxbuf);

#if !LELY_NO_THREADS
	mtx_destroy(&user->mtx);
#endif
}

io_can_chan_t *
io_user_can_chan_create(io_ctx_t *ctx, ev_exec_t *exec, int flags, size_t rxlen,
		int txtimeo, io_user_can_chan_write_t *func, void *arg)
{
	int errc = 0;

	io_can_chan_t *chan = io_user_can_chan_alloc();
	if (!chan) {
		errc = get_errc();
		goto error_alloc;
	}

	io_can_chan_t *tmp = io_user_can_chan_init(
			chan, ctx, exec, flags, rxlen, txtimeo, func, arg);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	chan = tmp;

	return chan;

error_init:
	io_user_can_chan_free((void *)chan);
error_alloc:
	set_errc(errc);
	return NULL;
}

void
io_user_can_chan_destroy(io_can_chan_t *chan)
{
	if (chan) {
		io_user_can_chan_fini(chan);
		io_user_can_chan_free((void *)chan);
	}
}

int
io_user_can_chan_on_msg(io_can_chan_t *chan, const struct can_msg *msg,
		const struct timespec *tp)
{
	struct io_user_can_chan *user = io_user_can_chan_from_chan(chan);
	assert(msg);

#if !LELY_NO_CANFD
	int flags = 0;
	if (msg->flags & CAN_FLAG_FDF)
		flags |= IO_CAN_BUS_FLAG_FDF;
	if (msg->flags & CAN_FLAG_BRS)
		flags |= IO_CAN_BUS_FLAG_BRS;
	if ((flags & user->flags) != flags)
		return 0;
#endif

#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
#endif
	struct ev_task *task =
			ev_task_from_node(sllist_pop_front(&user->read_queue));
	if (!task && io_user_can_buf_capacity(&user->rxbuf)) {
		// If there is no pending read operation and the receive buffer
		// is not full, push the frame to the buffer.
		*io_user_can_buf_push(&user->rxbuf) =
				(struct io_user_can_frame){ .is_err = 0,
					.u.msg = *msg };
#if !LELY_NO_THREADS
		mtx_unlock(&user->mtx);
#endif
		return 0;
	}
#if !LELY_NO_THREADS
	mtx_unlock(&user->mtx);
#endif

	struct io_can_chan_read *read = io_can_chan_read_from_task(task);
	if (read->msg)
		*read->msg = *msg;
	if (read->tp)
		*read->tp = tp ? *tp : (struct timespec){ 0, 0 };
	io_can_chan_read_post(read, 1, 0);

	return 1;
}

int
io_user_can_chan_on_err(io_can_chan_t *chan, const struct can_err *err,
		const struct timespec *tp)
{
	struct io_user_can_chan *user = io_user_can_chan_from_chan(chan);
	assert(err);

	if (!(user->flags & IO_CAN_BUS_FLAG_ERR))
		return 0;

#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
#endif
	struct ev_task *task =
			ev_task_from_node(sllist_pop_front(&user->read_queue));
	if (!task && io_user_can_buf_capacity(&user->rxbuf)) {
		// If there is no pending read operation and the receive buffer
		// is not full, push the error frame to the buffer.
		*io_user_can_buf_push(&user->rxbuf) =
				(struct io_user_can_frame){ .is_err = 1,
					.u.err = *err };
#if !LELY_NO_THREADS
		mtx_unlock(&user->mtx);
#endif
		return 0;
	}
#if !LELY_NO_THREADS
	mtx_unlock(&user->mtx);
#endif

	struct io_can_chan_read *read = io_can_chan_read_from_task(task);
	if (read->err)
		*read->err = *err;
	if (read->tp)
		*read->tp = tp ? *tp : (struct timespec){ 0, 0 };
	io_can_chan_read_post(read, 0, 0);

	return 1;
}

static io_ctx_t *
io_user_can_chan_dev_get_ctx(const io_dev_t *dev)
{
	const struct io_user_can_chan *user = io_user_can_chan_from_dev(dev);

	return user->ctx;
}

static ev_exec_t *
io_user_can_chan_dev_get_exec(const io_dev_t *dev)
{
	const struct io_user_can_chan *user = io_user_can_chan_from_dev(dev);

	return user->exec;
}

static size_t
io_user_can_chan_dev_cancel(io_dev_t *dev, struct ev_task *task)
{
	struct io_user_can_chan *user = io_user_can_chan_from_dev(dev);

	size_t n = 0;

	struct sllist read_queue, write_queue;
	sllist_init(&read_queue);
	sllist_init(&write_queue);

#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
#endif
	if (user->current_task && (!task || task == user->current_task)) {
		user->current_task = NULL;
		n++;
	}
	io_user_can_chan_do_pop(user, &read_queue, &write_queue, task);
#if !LELY_NO_THREADS
	mtx_unlock(&user->mtx);
#endif

	size_t nread = io_can_chan_read_queue_post(
			&read_queue, -1, errnum2c(ERRNUM_CANCELED));
	n = n < SIZE_MAX - nread ? n + nread : SIZE_MAX;
	size_t nwrite = io_can_chan_write_queue_post(
			&write_queue, errnum2c(ERRNUM_CANCELED));
	n = n < SIZE_MAX - nwrite ? n + nwrite : SIZE_MAX;

	return n;
}

static size_t
io_user_can_chan_dev_abort(io_dev_t *dev, struct ev_task *task)
{
	struct io_user_can_chan *user = io_user_can_chan_from_dev(dev);

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
#endif
	io_user_can_chan_do_pop(user, &queue, &queue, task);
#if !LELY_NO_THREADS
	mtx_unlock(&user->mtx);
#endif

	return ev_task_queue_abort(&queue);
}

static io_dev_t *
io_user_can_chan_get_dev(const io_can_chan_t *chan)
{
	const struct io_user_can_chan *user = io_user_can_chan_from_chan(chan);

	return &user->dev_vptr;
}

static int
io_user_can_chan_get_flags(const io_can_chan_t *chan)
{
	const struct io_user_can_chan *user = io_user_can_chan_from_chan(chan);

	return user->flags;
}

static int
io_user_can_chan_read(io_can_chan_t *chan, struct can_msg *msg,
		struct can_err *err, struct timespec *tp, int timeout)
{
	struct io_user_can_chan *user = io_user_can_chan_from_chan(chan);
	(void)timeout;

#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
#endif
	struct io_user_can_frame *frame = io_user_can_buf_pop(&user->rxbuf);
#if !LELY_NO_THREADS
	mtx_unlock(&user->mtx);
#endif
	if (!frame) {
		set_errnum(ERRNUM_AGAIN);
		return -1;
	}

	if (!frame->is_err && msg)
		*msg = frame->u.msg;
	else if (frame->is_err && err)
		*err = frame->u.err;
	if (tp)
		*tp = frame->ts;
	return !frame->is_err;
}

static void
io_user_can_chan_submit_read(io_can_chan_t *chan, struct io_can_chan_read *read)
{
	struct io_user_can_chan *user = io_user_can_chan_from_chan(chan);
	assert(read);
	struct ev_task *task = &read->task;

	if (!task->exec)
		task->exec = user->exec;
	ev_exec_on_task_init(task->exec);

	struct io_user_can_frame *frame;
#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
#endif
	if (user->shutdown) {
#if !LELY_NO_THREADS
		mtx_unlock(&user->mtx);
#endif
		io_can_chan_read_post(read, -1, errnum2c(ERRNUM_CANCELED));
	} else if ((frame = io_user_can_buf_pop(&user->rxbuf))) {
#if !LELY_NO_THREADS
		mtx_unlock(&user->mtx);
#endif
		if (!frame->is_err && read->msg)
			*read->msg = frame->u.msg;
		else if (frame->is_err && read->err)
			*read->err = frame->u.err;
		if (read->tp)
			*read->tp = frame->ts;
		io_can_chan_read_post(read, !frame->is_err, 0);
	} else {
		sllist_push_back(&user->read_queue, &task->_node);
#if !LELY_NO_THREADS
		mtx_unlock(&user->mtx);
#endif
	}
}

static int
io_user_can_chan_write(
		io_can_chan_t *chan, const struct can_msg *msg, int timeout)
{
	struct io_user_can_chan *user = io_user_can_chan_from_chan(chan);
	assert(msg);

#if !LELY_NO_CANFD
	int flags = 0;
	if (msg->flags & CAN_FLAG_FDF)
		flags |= IO_CAN_BUS_FLAG_FDF;
	if (msg->flags & CAN_FLAG_BRS)
		flags |= IO_CAN_BUS_FLAG_BRS;
	if ((flags & user->flags) != flags) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
#endif

	if (!user->func) {
		set_errnum(ERRNUM_NOSYS);
		return -1;
	}

	return user->func(msg, timeout, user->arg);
}

static void
io_user_can_chan_submit_write(
		io_can_chan_t *chan, struct io_can_chan_write *write)
{
	struct io_user_can_chan *user = io_user_can_chan_from_chan(chan);
	assert(write);
	struct ev_task *task = &write->task;

#if !LELY_NO_CANFD
	int flags = 0;
	if (write->msg->flags & CAN_FLAG_FDF)
		flags |= IO_CAN_BUS_FLAG_FDF;
	if (write->msg->flags & CAN_FLAG_BRS)
		flags |= IO_CAN_BUS_FLAG_BRS;
#endif

	if (!task->exec)
		task->exec = user->exec;
	ev_exec_on_task_init(task->exec);

#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
#endif
	if (user->shutdown) {
#if !LELY_NO_THREADS
		mtx_unlock(&user->mtx);
#endif
		io_can_chan_write_post(write, errnum2c(ERRNUM_CANCELED));
#if !LELY_NO_CANFD
	} else if ((flags & user->flags) != flags) {
#if !LELY_NO_THREADS
		mtx_unlock(&user->mtx);
#endif
		io_can_chan_write_post(write, errnum2c(ERRNUM_INVAL));
#endif
	} else if (!user->func) {
#if !LELY_NO_THREADS
		mtx_unlock(&user->mtx);
#endif
		io_can_chan_write_post(write, errnum2c(ERRNUM_NOSYS));
	} else {
		int post_write = !user->write_posted
				&& sllist_empty(&user->write_queue);
		sllist_push_back(&user->write_queue, &task->_node);
		if (post_write)
			user->write_posted = 1;
#if !LELY_NO_THREADS
		mtx_unlock(&user->mtx);
#endif
		if (post_write)
			ev_exec_post(user->write_task.exec, &user->write_task);
	}
}

static void
io_user_can_chan_svc_shutdown(struct io_svc *svc)
{
	struct io_user_can_chan *user = io_user_can_chan_from_svc(svc);
	io_dev_t *dev = &user->dev_vptr;

#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
#endif
	int shutdown = !user->shutdown;
	user->shutdown = 1;
	// Abort io_user_can_chan_write_task_func().
	// clang-format off
	if (shutdown && user->write_posted
			&& ev_exec_abort(user->write_task.exec,
					&user->write_task))
		// clang-format on
		user->write_posted = 0;
#if !LELY_NO_THREADS
	mtx_unlock(&user->mtx);
#endif

	if (shutdown)
		// Cancel all pending tasks.
		io_user_can_chan_dev_cancel(dev, NULL);
}

static void
io_user_can_chan_write_task_func(struct ev_task *task)
{
	assert(task);
	struct io_user_can_chan *user =
			structof(task, struct io_user_can_chan, write_task);

	int errsv = get_errc();

#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
#endif
	user->write_posted = 0;
	task = user->current_task =
			ev_task_from_node(sllist_pop_front(&user->write_queue));
#if !LELY_NO_THREADS
	mtx_unlock(&user->mtx);
#endif

	int result = 0;
	int errc = 0;
	if (task) {
		struct io_can_chan_write *write =
				io_can_chan_write_from_task(task);
		assert(write->msg);
		set_errc(0);
		assert(user->func);
		result = user->func(write->msg, user->txtimeo, user->arg);
		if (result < 0)
			errc = get_errc();
	}

	// clang-format off
	int wouldblock = result < 0 && (errc2num(errc) == ERRNUM_AGAIN
			|| errc2num(errc) == ERRNUM_WOULDBLOCK);
	// clang-format on

#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
#endif
	if (wouldblock && task == user->current_task) {
		// Put the write operation back on the queue if it would block,
		// unless it was canceled.
		sllist_push_front(&user->write_queue, &task->_node);
		task = NULL;
	}
	user->current_task = NULL;

	int post_write = !user->write_posted
			&& !sllist_empty(&user->write_queue);
	if (post_write)
		user->write_posted = 1;
#if !LELY_NO_THREADS
	mtx_unlock(&user->mtx);
#endif

	if (task) {
		if (wouldblock)
			errc = errnum2c(ERRNUM_CANCELED);
		struct io_can_chan_write *write =
				io_can_chan_write_from_task(task);
		io_can_chan_write_post(write, errc);
	}

	if (post_write)
		ev_exec_post(user->write_task.exec, &user->write_task);

	set_errc(errsv);
}

static inline struct io_user_can_chan *
io_user_can_chan_from_dev(const io_dev_t *dev)
{
	assert(dev);

	return structof(dev, struct io_user_can_chan, dev_vptr);
}

static inline struct io_user_can_chan *
io_user_can_chan_from_chan(const io_can_chan_t *chan)
{
	assert(chan);

	return structof(chan, struct io_user_can_chan, chan_vptr);
}

static inline struct io_user_can_chan *
io_user_can_chan_from_svc(const struct io_svc *svc)
{
	assert(svc);

	return structof(svc, struct io_user_can_chan, svc);
}

static void
io_user_can_chan_do_pop(struct io_user_can_chan *user,
		struct sllist *read_queue, struct sllist *write_queue,
		struct ev_task *task)
{
	assert(user);
	assert(read_queue);
	assert(write_queue);

	if (!task) {
		sllist_append(read_queue, &user->read_queue);
		sllist_append(write_queue, &user->write_queue);
	} else if (sllist_remove(&user->read_queue, &task->_node)) {
		sllist_push_back(read_queue, &task->_node);
	} else if (sllist_remove(&user->write_queue, &task->_node)) {
		sllist_push_back(write_queue, &task->_node);
	}
}
