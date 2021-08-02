/**@file
 * This file is part of the I/O library; it contains the I/O system timer
 * implementation for POSIX platforms.
 *
 * @see lely/io2/sys/timer.h
 *
 * @copyright 2014-2021 Lely Industries N.V.
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

#include "io.h"

#if !LELY_NO_STDIO && _POSIX_C_SOURCE >= 199309L

#include "../timer.h"
#include <lely/io2/ctx.h>
#include <lely/io2/posix/poll.h>
#include <lely/io2/sys/clock.h>
#include <lely/io2/sys/timer.h>
#include <lely/util/util.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#if !LELY_NO_THREADS
#include <pthread.h>
#endif
#include <signal.h>

static io_ctx_t *io_timer_impl_dev_get_ctx(const io_dev_t *dev);
static ev_exec_t *io_timer_impl_dev_get_exec(const io_dev_t *dev);
static size_t io_timer_impl_dev_cancel(io_dev_t *dev, struct ev_task *task);
static size_t io_timer_impl_dev_abort(io_dev_t *dev, struct ev_task *task);

// clang-format off
static const struct io_dev_vtbl io_timer_impl_dev_vtbl = {
	&io_timer_impl_dev_get_ctx,
	&io_timer_impl_dev_get_exec,
	&io_timer_impl_dev_cancel,
	&io_timer_impl_dev_abort
};
// clang-format on

static io_dev_t *io_timer_impl_get_dev(const io_timer_t *timer);
static io_clock_t *io_timer_impl_get_clock(const io_timer_t *timer);
static int io_timer_impl_getoverrun(const io_timer_t *timer);
static int io_timer_impl_gettime(
		const io_timer_t *timer, struct itimerspec *value);
static int io_timer_impl_settime(io_timer_t *timer, int flags,
		const struct itimerspec *value, struct itimerspec *ovalue);
static void io_timer_impl_submit_wait(
		io_timer_t *timer, struct io_timer_wait *wait);

// clang-format off
static const struct io_timer_vtbl io_timer_impl_vtbl = {
	&io_timer_impl_get_dev,
	&io_timer_impl_get_clock,
	&io_timer_impl_getoverrun,
	&io_timer_impl_gettime,
	&io_timer_impl_settime,
	&io_timer_impl_submit_wait
};
// clang-format on

static void io_timer_impl_svc_shutdown(struct io_svc *svc);

// clang-format off
static const struct io_svc_vtbl io_timer_impl_svc_vtbl = {
	NULL,
	&io_timer_impl_svc_shutdown
};
// clang-format on

struct io_timer_impl {
	const struct io_dev_vtbl *dev_vptr;
	const struct io_timer_vtbl *timer_vptr;
	struct io_svc svc;
	io_ctx_t *ctx;
	ev_exec_t *exec;
	clockid_t clockid;
	timer_t timerid;
#if !LELY_NO_THREADS
	pthread_mutex_t mtx;
#endif
	int shutdown;
	struct sllist wait_queue;
};

static void io_timer_impl_notify_function(union sigval val);

static inline struct io_timer_impl *io_timer_impl_from_dev(const io_dev_t *dev);
static inline struct io_timer_impl *io_timer_impl_from_timer(
		const io_timer_t *timer);
static inline struct io_timer_impl *io_timer_impl_from_svc(
		const struct io_svc *svc);

static void io_timer_impl_pop(struct io_timer_impl *impl, struct sllist *queue,
		struct ev_task *task);

void *
io_timer_alloc(void)
{
	struct io_timer_impl *impl = malloc(sizeof(*impl));
	if (!impl)
		return NULL;
	// Suppress a GCC maybe-uninitialized warning.
	impl->timer_vptr = NULL;
	// cppcheck-suppress memleak symbolName=impl
	return &impl->timer_vptr;
}

void
io_timer_free(void *ptr)
{
	if (ptr)
		free(io_timer_impl_from_timer(ptr));
}

io_timer_t *
io_timer_init(io_timer_t *timer, io_poll_t *poll, ev_exec_t *exec,
		clockid_t clockid)
{
	struct io_timer_impl *impl = io_timer_impl_from_timer(timer);
	assert(poll);
	assert(exec);
	io_ctx_t *ctx = io_poll_get_ctx(poll);
	assert(ctx);

	int errsv = 0;

	impl->dev_vptr = &io_timer_impl_dev_vtbl;
	impl->timer_vptr = &io_timer_impl_vtbl;

	impl->svc = (struct io_svc)IO_SVC_INIT(&io_timer_impl_svc_vtbl);
	impl->ctx = ctx;

	impl->exec = exec;

	impl->clockid = clockid;

	struct sigevent ev;
	ev.sigev_notify = SIGEV_THREAD;
	ev.sigev_signo = 0;
	ev.sigev_value.sival_ptr = impl;
	ev.sigev_notify_function = &io_timer_impl_notify_function;
	ev.sigev_notify_attributes = NULL;
	if (timer_create(clockid, &ev, &impl->timerid) == -1) {
		errsv = errno;
		goto error_create_timer;
	}

#if !LELY_NO_THREADS
	if ((errsv = pthread_mutex_init(&impl->mtx, NULL)))
		goto error_init_mtx;
#endif

	impl->shutdown = 0;

	sllist_init(&impl->wait_queue);

	io_ctx_insert(impl->ctx, &impl->svc);

	return timer;

#if !LELY_NO_THREADS
	// pthread_mutex_destroy(&impl->mtx);
error_init_mtx:
#endif
	timer_delete(impl->timerid);
error_create_timer:
	errno = errsv;
	return NULL;
}

void
io_timer_fini(io_timer_t *timer)
{
	struct io_timer_impl *impl = io_timer_impl_from_timer(timer);

	io_ctx_remove(impl->ctx, &impl->svc);
	// Cancel all pending tasks.
	io_timer_impl_svc_shutdown(&impl->svc);

	// Disarm the timer.
	struct itimerspec value = { { 0, 0 }, { 0, 0 } };
	timer_settime(impl->timerid, 0, &value, NULL);

	// TODO: Find a reliable way to wait for io_timer_impl_notify_function()
	// to complete.

#if !LELY_NO_THREADS
	pthread_mutex_destroy(&impl->mtx);
#endif

	timer_delete(impl->timerid);
}

io_timer_t *
io_timer_create(io_poll_t *poll, ev_exec_t *exec, clockid_t clockid)
{
	int errsv = 0;

	io_timer_t *timer = io_timer_alloc();
	if (!timer) {
		errsv = errno;
		goto error_alloc;
	}

	io_timer_t *tmp = io_timer_init(timer, poll, exec, clockid);
	if (!tmp) {
		errsv = errno;
		goto error_init;
	}
	timer = tmp;

	return timer;

error_init:
	io_timer_free((void *)timer);
error_alloc:
	errno = errsv;
	return NULL;
}

void
io_timer_destroy(io_timer_t *timer)
{
	if (timer) {
		io_timer_fini(timer);
		io_timer_free((void *)timer);
	}
}

static io_ctx_t *
io_timer_impl_dev_get_ctx(const io_dev_t *dev)
{
	const struct io_timer_impl *impl = io_timer_impl_from_dev(dev);

	return impl->ctx;
}

static ev_exec_t *
io_timer_impl_dev_get_exec(const io_dev_t *dev)
{
	const struct io_timer_impl *impl = io_timer_impl_from_dev(dev);

	return impl->exec;
}

static size_t
io_timer_impl_dev_cancel(io_dev_t *dev, struct ev_task *task)
{
	struct io_timer_impl *impl = io_timer_impl_from_dev(dev);

	struct sllist queue;
	sllist_init(&queue);

	io_timer_impl_pop(impl, &queue, task);

	return io_timer_wait_queue_post(&queue, -1, ECANCELED);
}

static size_t
io_timer_impl_dev_abort(io_dev_t *dev, struct ev_task *task)
{
	struct io_timer_impl *impl = io_timer_impl_from_dev(dev);

	struct sllist queue;
	sllist_init(&queue);

	io_timer_impl_pop(impl, &queue, task);

	return ev_task_queue_abort(&queue);
}

static io_dev_t *
io_timer_impl_get_dev(const io_timer_t *timer)
{
	const struct io_timer_impl *impl = io_timer_impl_from_timer(timer);

	return &impl->dev_vptr;
}

static io_clock_t *
io_timer_impl_get_clock(const io_timer_t *timer)
{
	const struct io_timer_impl *impl = io_timer_impl_from_timer(timer);
	assert(impl->clockid == CLOCK_REALTIME
			|| impl->clockid == CLOCK_MONOTONIC);

	switch (impl->clockid) {
	case CLOCK_REALTIME: return IO_CLOCK_REALTIME;
	case CLOCK_MONOTONIC: return IO_CLOCK_MONOTONIC;
	default: return NULL;
	}
}

static int
io_timer_impl_getoverrun(const io_timer_t *timer)
{
	const struct io_timer_impl *impl = io_timer_impl_from_timer(timer);

	return timer_getoverrun(impl->timerid);
}

static int
io_timer_impl_gettime(const io_timer_t *timer, struct itimerspec *value)
{
	const struct io_timer_impl *impl = io_timer_impl_from_timer(timer);

	return timer_gettime(impl->timerid, value);
}

static int
io_timer_impl_settime(io_timer_t *timer, int flags,
		const struct itimerspec *value, struct itimerspec *ovalue)
{
	struct io_timer_impl *impl = io_timer_impl_from_timer(timer);

	return timer_settime(impl->timerid, flags, value, ovalue);
}

static void
io_timer_impl_submit_wait(io_timer_t *timer, struct io_timer_wait *wait)
{
	struct io_timer_impl *impl = io_timer_impl_from_timer(timer);
	assert(wait);
	struct ev_task *task = &wait->task;

	if (!task->exec)
		task->exec = impl->exec;
	ev_exec_on_task_init(task->exec);

#if !LELY_NO_THREADS
	pthread_mutex_lock(&impl->mtx);
#endif
	if (impl->shutdown) {
#if !LELY_NO_THREADS
		pthread_mutex_unlock(&impl->mtx);
#endif
		io_timer_wait_post(wait, -1, ECANCELED);
	} else {
		sllist_push_back(&impl->wait_queue, &task->_node);
#if !LELY_NO_THREADS
		pthread_mutex_unlock(&impl->mtx);
#endif
	}
}

static void
io_timer_impl_svc_shutdown(struct io_svc *svc)
{
	struct io_timer_impl *impl = io_timer_impl_from_svc(svc);
	io_dev_t *dev = &impl->dev_vptr;

#if !LELY_NO_THREADS
	pthread_mutex_lock(&impl->mtx);
#endif
	int shutdown = !impl->shutdown;
	impl->shutdown = 1;
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->mtx);
#endif

	if (shutdown)
		// Cancel all pending operations.
		io_timer_impl_dev_cancel(dev, NULL);
}

static void
io_timer_impl_notify_function(union sigval val)
{
	struct io_timer_impl *impl = val.sival_ptr;
	assert(impl);

	int errsv = errno;
	errno = 0;
	int overrun = timer_getoverrun(impl->timerid);

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	pthread_mutex_lock(&impl->mtx);
#endif
	sllist_append(&queue, &impl->wait_queue);
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->mtx);
#endif

	io_timer_wait_queue_post(&queue, overrun, errno);
	errno = errsv;
}

static inline struct io_timer_impl *
io_timer_impl_from_dev(const io_dev_t *dev)
{
	assert(dev);

	return structof(dev, struct io_timer_impl, dev_vptr);
}

static inline struct io_timer_impl *
io_timer_impl_from_timer(const io_timer_t *timer)
{
	assert(timer);

	return structof(timer, struct io_timer_impl, timer_vptr);
}

static inline struct io_timer_impl *
io_timer_impl_from_svc(const struct io_svc *svc)
{
	assert(svc);

	return structof(svc, struct io_timer_impl, svc);
}

static void
io_timer_impl_pop(struct io_timer_impl *impl, struct sllist *queue,
		struct ev_task *task)
{
	assert(impl);
	assert(queue);

#if !LELY_NO_THREADS
	pthread_mutex_lock(&impl->mtx);
#endif
	if (!task)
		sllist_append(queue, &impl->wait_queue);
	else if (sllist_remove(&impl->wait_queue, &task->_node))
		sllist_push_back(queue, &task->_node);
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->mtx);
#endif
}

#endif // !LELY_NO_STDIO && _POSIX_C_SOURCE >= 199309L
