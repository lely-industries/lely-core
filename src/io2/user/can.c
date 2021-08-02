/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * user-defined CAN channel.
 *
 * @see lely/io2/user/can.h
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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or useried.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../can.h"

#if !LELY_NO_MALLOC

#if !LELY_NO_THREADS
#include <lely/libc/threads.h>
#endif
#include <lely/io2/ctx.h>
#include <lely/io2/user/can.h>
#include <lely/util/diag.h>
#include <lely/util/errnum.h>
#include <lely/util/spscring.h>
#include <lely/util/time.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdlib.h>

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

/// The implementation of a user-defined CAN channel.
struct io_user_can_chan {
	/// A pointer to the virtual table for the I/O device interface.
	const struct io_dev_vtbl *dev_vptr;
	/// A pointer to the virtual table for the CAN channel interface.
	const struct io_can_chan_vtbl *chan_vptr;
	/// The I/O service representing the channel.
	struct io_svc svc;
	/// A pointer to the I/O context with which the channel is registered.
	io_ctx_t *ctx;
	/// A pointer to the executor used to execute all I/O tasks.
	ev_exec_t *exec;
	/// The flags specifying which CAN bus features are enabled.
	int flags;
	/**
	 * The timeout (in milliseconds) when writing a CAN frame
	 * asynchronously.
	 */
	int txtimeo;
	/**
	 * A pointer to the function to be invoked when a CAN frame needs to be
	 * written.
	 */
	io_user_can_chan_write_t *func;
	/**
	 * The user-specific value to be passed as the second argument to #func.
	 */
	void *arg;
	/// The task responsible for initiating read operations.
	struct ev_task read_task;
	/// The task responsible for initiating write operations.
	struct ev_task write_task;
#if !LELY_NO_THREADS
	/// The mutex protecting the receive queue producer.
	mtx_t p_mtx;
	/// The condition variable used to wake up the receive queue producer.
	cnd_t p_cond;
	/// The mutex protecting the receive queue consumer.
	mtx_t c_mtx;
	/// The condition variable used to wake up the receive queue consumer.
	cnd_t c_cond;
#endif
	/// The ring buffer used to control the receive queue.
	struct spscring rxring;
	/// The receive queue.
	struct io_user_can_frame *rxbuf;
#if !LELY_NO_THREADS
	/**
	 * The mutex protecting the channel and the queues of pending
	 * operations.
	 */
	mtx_t mtx;
#endif
	/// A flag indicating whether the I/O service has been shut down.
	unsigned shutdown : 1;
	/// A flag indicating whether #read_task has been posted to #exec.
	unsigned read_posted : 1;
	/// A flag indicating whether #write_task has been posted to #exec.
	unsigned write_posted : 1;
	/// The queue containing pending read operations.
	struct sllist read_queue;
	/// The queue containing pending write operations.
	struct sllist write_queue;
	/// The read operation currently being executed.
	struct ev_task *current_read;
	/// The write operation currently being executed.
	struct ev_task *current_write;
};

static void io_user_can_chan_read_task_func(struct ev_task *task);
static void io_user_can_chan_write_task_func(struct ev_task *task);

static inline struct io_user_can_chan *io_user_can_chan_from_dev(
		const io_dev_t *dev);
static inline struct io_user_can_chan *io_user_can_chan_from_chan(
		const io_can_chan_t *chan);
static inline struct io_user_can_chan *io_user_can_chan_from_svc(
		const struct io_svc *svc);

static int io_user_can_chan_on_frame(struct io_user_can_chan *user,
		const struct io_user_can_frame *frame, int timeout);

static void io_user_can_chan_p_signal(struct spscring *ring, void *arg);
static void io_user_can_chan_c_signal(struct spscring *ring, void *arg);

static void io_user_can_chan_do_pop(struct io_user_can_chan *user,
		struct sllist *read_queue, struct sllist *write_queue,
		struct ev_task *task);

static size_t io_user_can_chan_do_abort_tasks(struct io_user_can_chan *user);

void *
io_user_can_chan_alloc(void)
{
	struct io_user_can_chan *user = malloc(sizeof(*user));
	if (!user) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return NULL;
	}
	// Suppress a GCC maybe-uninitialized warning.
	user->chan_vptr = NULL;
	// cppcheck-suppress memleak symbolName=user
	return &user->chan_vptr;
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

	int errc = 0;

	user->dev_vptr = &io_user_can_chan_dev_vtbl;
	user->chan_vptr = &io_user_can_chan_vtbl;

	user->svc = (struct io_svc)IO_SVC_INIT(&io_user_can_chan_svc_vtbl);
	user->ctx = ctx;

	user->exec = exec;

	user->flags = flags;
	user->txtimeo = txtimeo;

	user->func = func;
	user->arg = arg;

	user->read_task = (struct ev_task)EV_TASK_INIT(
			user->exec, &io_user_can_chan_read_task_func);
	user->write_task = (struct ev_task)EV_TASK_INIT(
			user->exec, &io_user_can_chan_write_task_func);

#if !LELY_NO_THREADS
	if (mtx_init(&user->p_mtx, mtx_plain) != thrd_success) {
		errc = get_errc();
		goto error_init_p_mtx;
	}

	if (cnd_init(&user->p_cond) != thrd_success) {
		errc = get_errc();
		goto error_init_p_cond;
	}

	if (mtx_init(&user->c_mtx, mtx_plain) != thrd_success) {
		errc = get_errc();
		goto error_init_c_mtx;
	}

	if (cnd_init(&user->c_cond) != thrd_success) {
		errc = get_errc();
		goto error_init_c_cond;
	}
#endif

	spscring_init(&user->rxring, rxlen);
	user->rxbuf = calloc(rxlen, sizeof(struct io_user_can_frame));
	if (!user->rxbuf) {
#if !LELY_NO_ERRNO
		errc = errno2c(errno);
#endif
		goto error_alloc_rxbuf;
	}

#if !LELY_NO_THREADS
	if (mtx_init(&user->mtx, mtx_plain) != thrd_success) {
		errc = get_errc();
		goto error_init_mtx;
	}
#endif

	user->shutdown = 0;
	user->read_posted = 0;
	user->write_posted = 0;

	sllist_init(&user->read_queue);
	sllist_init(&user->write_queue);
	user->current_read = NULL;
	user->current_write = NULL;

	io_ctx_insert(user->ctx, &user->svc);

	return chan;

#if !LELY_NO_THREADS
	// mtx_destroy(&user->mtx);
error_init_mtx:
#endif
	free(user->rxbuf);
error_alloc_rxbuf:
#if !LELY_NO_THREADS
	cnd_destroy(&user->c_cond);
error_init_c_cond:
	mtx_destroy(&user->c_mtx);
error_init_c_mtx:
	cnd_destroy(&user->p_cond);
error_init_p_cond:
	mtx_destroy(&user->p_mtx);
error_init_p_mtx:
#endif
	set_errc(errc);
	return NULL;
}

void
io_user_can_chan_fini(io_can_chan_t *chan)
{
	struct io_user_can_chan *user = io_user_can_chan_from_chan(chan);

	io_ctx_remove(user->ctx, &user->svc);
	// Cancel all pending tasks.
	io_user_can_chan_svc_shutdown(&user->svc);

	// Abort any consumer wait operation running in a task. Producer wait
	// operations are only initiated by io_user_can_chan_on_msg() and
	// io_user_can_chan_on_err(), and should have terminated before this
	// function is called.
	spscring_c_abort_wait(&user->rxring);

#if !LELY_NO_THREADS
	int warning = 0;
	mtx_lock(&user->mtx);
	// If necessary, busy-wait until io_user_can_chan_read_task_func() and
	// io_user_can_chan_write_task_func() complete.
	while (user->read_posted || user->write_posted) {
		if (io_user_can_chan_do_abort_tasks(user))
			continue;
		mtx_unlock(&user->mtx);
		if (!warning) {
			warning = 1;
			diag(DIAG_WARNING, 0,
					"io_user_can_chan_fini() invoked with pending operations");
		}
		thrd_yield();
		mtx_lock(&user->mtx);
	}
	mtx_unlock(&user->mtx);

	mtx_destroy(&user->mtx);
#endif

	free(user->rxbuf);

#if !LELY_NO_THREADS
	cnd_destroy(&user->c_cond);
	mtx_destroy(&user->c_mtx);
	cnd_destroy(&user->p_cond);
	mtx_destroy(&user->p_mtx);
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
		const struct timespec *tp, int timeout)
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

	struct io_user_can_frame frame = { .is_err = 0,
		.u.msg = *msg,
		.ts = tp ? *tp : (struct timespec){ 0, 0 } };
	return io_user_can_chan_on_frame(user, &frame, timeout);
}

int
io_user_can_chan_on_err(io_can_chan_t *chan, const struct can_err *err,
		const struct timespec *tp, int timeout)
{
	struct io_user_can_chan *user = io_user_can_chan_from_chan(chan);
	assert(err);

	if (!(user->flags & IO_CAN_BUS_FLAG_ERR)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	struct io_user_can_frame frame = { .is_err = 1,
		.u.err = *err,
		.ts = tp ? *tp : (struct timespec){ 0, 0 } };
	return io_user_can_chan_on_frame(user, &frame, timeout);
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
	if (user->current_read && (!task || task == user->current_read)) {
		user->current_read = NULL;
		n++;
	}
	if (user->current_write && (!task || task == user->current_write)) {
		user->current_write = NULL;
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

#if !LELY_NO_THREADS
	// Compute the absolute timeout for cnd_timedwait().
	struct timespec ts = { 0, 0 };
	if (timeout > 0) {
#if LELY_NO_TIMEOUT
		timeout = 0;
		(void)ts;
#else
		if (!timespec_get(&ts, TIME_UTC))
			return -1;
		timespec_add_msec(&ts, timeout);
#endif
	}
#endif

#if !LELY_NO_THREADS
	mtx_lock(&user->c_mtx);
#endif
	size_t i = 0;
	for (;;) {
		// Check if a frame is available in the receive queue.
		size_t n = 1;
		i = spscring_c_alloc(&user->rxring, &n);
		if (n)
			break;
		if (!timeout) {
#if !LELY_NO_THREADS
			mtx_unlock(&user->c_mtx);
#endif
			set_errnum(ERRNUM_AGAIN);
			return -1;
		}
		// Submit a wait operation for a single frame.
		// clang-format off
		if (!spscring_c_submit_wait(&user->rxring, 1,
				&io_user_can_chan_c_signal, user))
			// clang-format on
			// If the wait condition was already satisfied, try
			// again.
			continue;
			// Wait for the buffer to signal that a frame is
			// available, or time out if that takes too long.
#if !LELY_NO_THREADS
		int result;
#if !LELY_NO_TIMEOUT
		if (timeout > 0)
			result = cnd_timedwait(
					&user->c_cond, &user->c_mtx, &ts);
		else
#endif
			result = cnd_wait(&user->c_cond, &user->c_mtx);
		if (result != thrd_success) {
			mtx_unlock(&user->c_mtx);
#if !LELY_NO_TIMEOUT
			if (result == thrd_timedout)
				set_errnum(ERRNUM_AGAIN);
#endif
			return -1;
		}
#endif
	}
	// Copy the frame from the buffer.
	struct io_user_can_frame *frame = &user->rxbuf[i];
	int is_err = frame->is_err;
	if (!is_err && msg)
		*msg = frame->u.msg;
	else if (is_err && err)
		*err = frame->u.err;
	if (tp)
		*tp = frame->ts;
	spscring_c_commit(&user->rxring, 1);
#if !LELY_NO_THREADS
	mtx_unlock(&user->c_mtx);
#endif

	return !is_err;
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

#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
#endif
	if (user->shutdown) {
#if !LELY_NO_THREADS
		mtx_unlock(&user->mtx);
#endif
		io_can_chan_read_post(read, -1, errnum2c(ERRNUM_CANCELED));
	} else {
		int post_read = !user->read_posted
				&& sllist_empty(&user->read_queue);
		sllist_push_back(&user->read_queue, &task->_node);
		if (post_read)
			user->read_posted = 1;
#if !LELY_NO_THREADS
		mtx_unlock(&user->mtx);
#endif
		// cppcheck-suppress duplicateCondition
		if (post_read)
			ev_exec_post(user->read_task.exec, &user->read_task);
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
		// cppcheck-suppress duplicateCondition
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
	if (shutdown)
		// Try to abort io_user_can_chan_read_task_func() and
		// io_user_can_chan_write_task_func().
		io_user_can_chan_do_abort_tasks(user);
#if !LELY_NO_THREADS
	mtx_unlock(&user->mtx);
#endif
	// cppcheck-suppress duplicateCondition
	if (shutdown)
		// Cancel all pending operations.
		io_user_can_chan_dev_cancel(dev, NULL);
}

static void
io_user_can_chan_read_task_func(struct ev_task *task)
{
	assert(task);
	struct io_user_can_chan *user =
			structof(task, struct io_user_can_chan, read_task);
	io_can_chan_t *chan = &user->chan_vptr;

	int errsv = get_errc();

	int wouldblock = 0;

#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
#endif
	// Try to process all pending read operations at once.
	while ((task = user->current_read = ev_task_from_node(
				sllist_pop_front(&user->read_queue)))) {
#if !LELY_NO_THREADS
		mtx_unlock(&user->mtx);
#endif
		struct io_can_chan_read *read =
				io_can_chan_read_from_task(task);
		int result = io_user_can_chan_read(
				chan, read->msg, read->err, read->tp, 0);
		int errc = result >= 0 ? 0 : get_errc();
		wouldblock = errc == errnum2c(ERRNUM_AGAIN)
				|| errc == errnum2c(ERRNUM_WOULDBLOCK);
		if (!wouldblock)
			// The operation succeeded or failed immediately.
			io_can_chan_read_post(read, result, errc);
#if !LELY_NO_THREADS
		mtx_lock(&user->mtx);
#endif
		if (task == user->current_read) {
			// Put the read operation back on the queue if it would
			// block, unless it was canceled.
			if (wouldblock) {
				sllist_push_front(&user->read_queue,
						&task->_node);
				task = NULL;
			}
			user->current_read = NULL;
		}
		assert(!user->current_read);
		// Stop if the operation did or would block.
		if (wouldblock)
			break;
	}
	// Repost this task if any read operations remain in the queue.
	int post_read = !sllist_empty(&user->read_queue) && !user->shutdown;
	// Register a wait operation if the receive queue is empty.
	if (post_read && wouldblock) {
#if !LELY_NO_THREADS
		mtx_lock(&user->c_mtx);
#endif
		// Do not repost this task unless the wait condition can be
		// satisfied immediately.
		post_read = !spscring_c_submit_wait(&user->rxring, 1,
				io_user_can_chan_c_signal, user);
#if !LELY_NO_THREADS
		mtx_unlock(&user->c_mtx);
#endif
	}
	user->read_posted = post_read;
#if !LELY_NO_THREADS
	mtx_unlock(&user->mtx);
#endif

	if (task && wouldblock)
		// The operation would block but was canceled before it could be
		// requeued.
		io_can_chan_read_post(io_can_chan_read_from_task(task), -1,
				errnum2c(ERRNUM_CANCELED));

	if (post_read)
		ev_exec_post(user->read_task.exec, &user->read_task);

	set_errc(errsv);
}

static void
io_user_can_chan_write_task_func(struct ev_task *task)
{
	assert(task);
	struct io_user_can_chan *user =
			structof(task, struct io_user_can_chan, write_task);
	io_can_chan_t *chan = &user->chan_vptr;

	int errsv = get_errc();

	int wouldblock = 0;

#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
#endif
	// clang-format off
	if ((task = user->current_write = ev_task_from_node(
			sllist_pop_front(&user->write_queue)))) {
		// clang-format on
#if !LELY_NO_THREADS
		mtx_unlock(&user->mtx);
#endif
		struct io_can_chan_write *write =
				io_can_chan_write_from_task(task);
		int result = io_user_can_chan_write(
				chan, write->msg, user->txtimeo);
		int errc = !result ? 0 : get_errc();
		wouldblock = errc == errnum2c(ERRNUM_AGAIN)
				|| errc == errnum2c(ERRNUM_WOULDBLOCK);
		if (!wouldblock)
			// The operation succeeded or failed immediately.
			io_can_chan_write_post(write, errc);
#if !LELY_NO_THREADS
		mtx_lock(&user->mtx);
#endif
		if (task == user->current_write) {
			// Put the read operation back on the queue if it would
			// block, unless it was canceled.
			if (wouldblock) {
				sllist_push_front(&user->write_queue,
						&task->_node);
				task = NULL;
			}
			user->current_write = NULL;
		}
		assert(!user->current_write);
	}
	int post_write = user->write_posted =
			!sllist_empty(&user->write_queue) && !user->shutdown;
#if !LELY_NO_THREADS
	mtx_unlock(&user->mtx);
#endif

	if (task && wouldblock)
		// The operation would block but was canceled before it could be
		// requeued.
		io_can_chan_write_post(io_can_chan_write_from_task(task),
				errnum2c(ERRNUM_CANCELED));

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

static int
io_user_can_chan_on_frame(struct io_user_can_chan *user,
		const struct io_user_can_frame *frame, int timeout)
{
	assert(user);
	assert(frame);

#if !LELY_NO_THREADS
	// Compute the absolute timeout for cnd_timedwait().
	struct timespec ts = { 0, 0 };
	if (timeout > 0) {
#if LELY_NO_TIMEOUT
		timeout = 0;
		(void)ts;
#else
		if (!timespec_get(&ts, TIME_UTC))
			return -1;
		timespec_add_msec(&ts, timeout);
#endif
	}
#endif

#if !LELY_NO_THREADS
	mtx_lock(&user->p_mtx);
#endif
	size_t i = 0;
	for (;;) {
		// Check if a slot is available in the receive queue.
		size_t n = 1;
		i = spscring_p_alloc(&user->rxring, &n);
		if (n)
			break;
		if (!timeout) {
#if !LELY_NO_THREADS
			mtx_unlock(&user->p_mtx);
#endif
			set_errnum(ERRNUM_AGAIN);
			return -1;
		}
		// clang-format off
		if (!spscring_p_submit_wait(&user->rxring, 1,
				&io_user_can_chan_p_signal, user))
			// clang-format on
			// If the wait condition was already satisfied, try
			// again.
			continue;
#if !LELY_NO_THREADS
		int result;
#if !LELY_NO_TIMEOUT
		// Wait for the buffer to signal that a slot is available, or
		// time out if that takes too long.
		if (timeout > 0)
			result = cnd_timedwait(
					&user->p_cond, &user->p_mtx, &ts);
		else
#endif
			result = cnd_wait(&user->p_cond, &user->p_mtx);
		if (result != thrd_success) {
			mtx_unlock(&user->p_mtx);
#if !LELY_NO_TIMEOUT
			if (result == thrd_timedout)
				set_errnum(ERRNUM_AGAIN);
#endif
			return -1;
		}
#endif
	}
	// Copy the frame to the buffer.
	user->rxbuf[i] = *frame;
	spscring_p_commit(&user->rxring, 1);
#if !LELY_NO_THREADS
	mtx_unlock(&user->p_mtx);
#endif

	return 0;
}

static void
io_user_can_chan_p_signal(struct spscring *ring, void *arg)
{
	(void)ring;

#if LELY_NO_THREADS
	(void)arg;
#else
	struct io_user_can_chan *user = arg;
	assert(user);

	mtx_lock(&user->p_mtx);
	cnd_broadcast(&user->p_cond);
	mtx_unlock(&user->p_mtx);
#endif
}

static void
io_user_can_chan_c_signal(struct spscring *ring, void *arg)
{
	(void)ring;
	struct io_user_can_chan *user = arg;
	assert(user);

#if !LELY_NO_THREADS
	mtx_lock(&user->c_mtx);
	cnd_broadcast(&user->c_cond);
	mtx_unlock(&user->c_mtx);
#endif

#if !LELY_NO_THREADS
	mtx_lock(&user->mtx);
#endif
	int post_read = !user->read_posted && !sllist_empty(&user->read_queue)
			&& !user->shutdown;
	if (post_read)
		user->read_posted = 1;
#if !LELY_NO_THREADS
	mtx_unlock(&user->mtx);
#endif
	// cppcheck-suppress duplicateCondition
	if (post_read)
		ev_exec_post(user->read_task.exec, &user->read_task);
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

static size_t
io_user_can_chan_do_abort_tasks(struct io_user_can_chan *user)
{
	size_t n = 0;

	// Try to abort io_user_can_chan_read_task_func().
	// clang-format off
	if (user->read_posted && ev_exec_abort(user->read_task.exec,
			&user->read_task)) {
		// clang-format on
		user->read_posted = 0;
		n++;
	}

	// Try to abort io_user_can_chan_write_task_func().
	// clang-format off
	if (user->write_posted && ev_exec_abort(user->write_task.exec,
			&user->write_task)) {
		// clang-format on
		user->write_posted = 0;
		n++;
	}

	return n;
}

#endif // !LELY_NO_MALLOC
