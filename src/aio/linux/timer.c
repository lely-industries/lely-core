/*!\file
 * This file is part of the asynchronous I/O library; it contains ...
 *
 * \see lely/aio/system/timer.h
 *
 * \copyright 2018 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#include "../aio.h"

#ifdef __linux__

#if !LELY_NO_THREADS
#include <lely/libc/threads.h>
#endif
#include <lely/aio/context.h>
#include <lely/aio/queue.h>
#include <lely/aio/reactor.h>
#include <lely/aio/timer.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/timerfd.h>

static aio_clock_t *aio_timer_impl_get_clock(const aio_timer_t *timer);
static int aio_timer_impl_getoverrun(const aio_timer_t *timer);
static int aio_timer_impl_gettime(const aio_timer_t *timer,
		struct itimerspec *value);
static int aio_timer_impl_settime(aio_timer_t *timer, int flags,
		const struct itimerspec *value, struct itimerspec *ovalue);
static aio_exec_t *aio_timer_impl_get_exec(const aio_timer_t *timer);
static void aio_timer_impl_submit_wait(aio_timer_t *timer,
		struct aio_task *task);
static size_t aio_timer_impl_cancel(aio_timer_t *timer, struct aio_task *task);

static const struct aio_timer_vtbl aio_timer_impl_vtbl = {
	&aio_timer_impl_get_clock,
	&aio_timer_impl_getoverrun,
	&aio_timer_impl_gettime,
	&aio_timer_impl_settime,
	&aio_timer_impl_get_exec,
	&aio_timer_impl_submit_wait,
	&aio_timer_impl_cancel
};

static int aio_timer_impl_service_notify_fork(struct aio_service *srv,
		enum aio_fork_event e);
static void aio_timer_impl_service_shutdown(struct aio_service *srv);

static const struct aio_service_vtbl aio_timer_impl_service_vtbl = {
	&aio_timer_impl_service_notify_fork,
	&aio_timer_impl_service_shutdown
};

struct aio_timer_impl {
	const struct aio_timer_vtbl *timer_vptr;
	clockid_t clockid;
	aio_exec_t *exec;
	aio_reactor_t *reactor;
	struct aio_service srv;
	aio_context_t *ctx;
	int tfd;
	struct aio_watch watch;
#if !LELY_NO_THREADS
	mtx_t mtx;
#endif
	int shutdown;
	int overrun;
	struct aio_queue queue;
};

static int aio_timer_impl_func(struct aio_watch *watch, int events);

static inline struct aio_timer_impl *aio_impl_from_timer(
		const aio_timer_t *timer);
static inline struct aio_timer_impl *aio_impl_from_service(
		const struct aio_service *srv);

static int aio_timer_impl_open(struct aio_timer_impl *impl);
static int aio_timer_impl_close(struct aio_timer_impl *impl);

LELY_AIO_EXPORT void *
aio_timer_alloc(void)
{
	struct aio_timer_impl *impl = malloc(sizeof(*impl));
	return impl ? &impl->timer_vptr : NULL;
}

LELY_AIO_EXPORT void
aio_timer_free(void *ptr)
{
	if (ptr)
		free(aio_impl_from_timer(ptr));
}

LELY_AIO_EXPORT aio_timer_t *
aio_timer_init(aio_timer_t *timer, clockid_t clockid, aio_exec_t *exec,
		aio_reactor_t *reactor)
{
	struct aio_timer_impl *impl = aio_impl_from_timer(timer);
	assert(exec);
	assert(reactor);
	aio_context_t *ctx = aio_reactor_get_context(reactor);
	assert(ctx);

	int errc = 0;

	impl->timer_vptr = &aio_timer_impl_vtbl;

	impl->clockid = clockid;

	impl->exec = exec;

	impl->reactor = reactor;

	impl->srv = (struct aio_service)AIO_SERVICE_INIT(
			&aio_timer_impl_service_vtbl);
	impl->ctx = ctx;

	impl->watch = (struct aio_watch)AIO_WATCH_INIT(&aio_timer_impl_func);
	impl->tfd = -1;
	if (aio_timer_impl_open(impl) == -1) {
		errc = errno;
		goto error_open;
	}

#if !LELY_NO_THREADS
	if (mtx_init(&impl->mtx, mtx_plain) != thrd_success) {
		errc = errno;
		goto error_init_mtx;
	}
#endif

	impl->shutdown = 0;

	impl->overrun = 0;
	aio_queue_init(&impl->queue);

	aio_context_insert(impl->ctx, &impl->srv);

	return timer;

#if !LELY_NO_THREADS
	mtx_destroy(&impl->mtx);
error_init_mtx:
#endif
	aio_timer_impl_close(impl);
error_open:
	errno = errc;
	return NULL;
}

LELY_AIO_EXPORT void
aio_timer_fini(aio_timer_t *timer)
{
	struct aio_timer_impl *impl = aio_impl_from_timer(timer);

	aio_context_remove(impl->ctx, &impl->srv);

#if !LELY_NO_THREADS
	mtx_destroy(&impl->mtx);
#endif

	aio_timer_impl_close(impl);
}

LELY_AIO_EXPORT aio_timer_t *
aio_timer_create(clockid_t clockid, aio_exec_t *exec, aio_reactor_t *reactor)
{
	int errc = 0;

	aio_timer_t *timer = aio_timer_alloc();
	if (!timer) {
		errc = errno;
		goto error_alloc;
	}

	aio_timer_t *tmp = aio_timer_init(timer, clockid, exec, reactor);
	if (!tmp) {
		errc = errno;
		goto error_init;
	}
	timer = tmp;

	return timer;

error_init:
	aio_timer_free((void *)timer);
error_alloc:
	errno = errc;
	return NULL;
}

LELY_AIO_EXPORT void
aio_timer_destroy(aio_timer_t *timer)
{
	if (timer) {
		aio_timer_fini(timer);
		aio_timer_free((void *)timer);
	}
}

static aio_clock_t *
aio_timer_impl_get_clock(const aio_timer_t *timer)
{
	const struct aio_timer_impl *impl = aio_impl_from_timer(timer);

	switch (impl->clockid) {
	case CLOCK_REALTIME:
		return AIO_CLOCK_REALTIME;
	case CLOCK_MONOTONIC:
		return AIO_CLOCK_MONOTONIC;
	default:
		return NULL;
	}
}

static int
aio_timer_impl_getoverrun(const aio_timer_t *timer)
{
	const struct aio_timer_impl *impl = aio_impl_from_timer(timer);

#if !LELY_NO_THREADS
	mtx_lock((mtx_t *)&impl->mtx);
#endif
	int overrun = impl->overrun;
#if !LELY_NO_THREADS
	mtx_unlock((mtx_t *)&impl->mtx);
#endif

	return overrun;
}

static int
aio_timer_impl_gettime(const aio_timer_t *timer, struct itimerspec *value)
{
	const struct aio_timer_impl *impl = aio_impl_from_timer(timer);

	return timerfd_gettime(impl->tfd, value);
}

static int
aio_timer_impl_settime(aio_timer_t *timer, int flags_,
		const struct itimerspec *value, struct itimerspec *ovalue)
{
	struct aio_timer_impl *impl = aio_impl_from_timer(timer);

	int flags = 0;
	if (flags_ & TIMER_ABSTIME)
		flags |= TFD_TIMER_ABSTIME;

	return timerfd_settime(impl->tfd, flags, value, ovalue);
}

static aio_exec_t *
aio_timer_impl_get_exec(const aio_timer_t *timer)
{
	const struct aio_timer_impl *impl = aio_impl_from_timer(timer);

	return impl->exec;
}

static void
aio_timer_impl_submit_wait(aio_timer_t *timer, struct aio_task *task)
{
	struct aio_timer_impl *impl = aio_impl_from_timer(timer);
	assert(task);

	if (!task->exec)
		task->exec = aio_timer_get_exec(timer);
	task->errc = task->func ? EINPROGRESS : 0;

	if (task->func) {
		aio_exec_on_task_started(task->exec);
#if !LELY_NO_THREADS
		mtx_lock(&impl->mtx);
#endif
		if (impl->shutdown) {
			task->errc = ECANCELED;
			aio_exec_post(task->exec, task);
			aio_exec_on_task_finished(task->exec);
		} else {
			aio_queue_push(&impl->queue, task);
		}
#if !LELY_NO_THREADS
		mtx_unlock(&impl->mtx);
#endif
	}
}

static size_t
aio_timer_impl_cancel(aio_timer_t *timer, struct aio_task *task)
{
	struct aio_timer_impl *impl = aio_impl_from_timer(timer);

	struct aio_queue queue;
	aio_queue_init(&queue);

#if! LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif
	aio_queue_move(&queue, &impl->queue, task);
#if! LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	return aio_queue_cancel(&queue, ECANCELED);
}

static int
aio_timer_impl_service_notify_fork(struct aio_service *srv,
		enum aio_fork_event e)
{
	struct aio_timer_impl *impl = aio_impl_from_service(srv);

	if (e != AIO_FORK_CHILD || impl->shutdown)
		return 0;

	int result = -1;
	int errc = errno;

	struct itimerspec value = { { 0, 0 }, { 0, 0 } };
	if (timerfd_gettime(impl->tfd, &value) == -1 && !result) {
		errc = errno;
		result = -1;
	}

	if (aio_timer_impl_close(impl) == -1 && !result) {
		errc = errno;
		result = -1;
	}

	if (aio_timer_impl_open(impl) == -1 && !result) {
		errc = errno;
		result = -1;
	}

	if (timerfd_settime(impl->tfd, 0, &value, NULL) == -1) {
		errc = errno;
		result = -1;
	}

	errno = errc;
	return result;
}

static void
aio_timer_impl_service_shutdown(struct aio_service *srv)
{
	struct aio_timer_impl *impl = aio_impl_from_service(srv);

	struct aio_queue queue;
	aio_queue_init(&queue);

#if! LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif

	assert(!impl->shutdown);
	impl->shutdown = 1;

	aio_queue_move(&queue, &impl->queue, NULL);

	aio_reactor_watch(impl->reactor, &impl->watch, impl->tfd, 0);

#if! LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	aio_queue_cancel(&queue, ECANCELED);
}

static int
aio_timer_impl_func(struct aio_watch *watch, int events)
{
	assert(watch);
	struct aio_timer_impl *impl =
			structof(watch, struct aio_timer_impl, watch);
	__unused_var(events);

	int overrun = -1;

	int errc = errno;
	ssize_t result;
	do {
		uint64_t value = 0;
		errno = 0;
		result = read(impl->tfd, &value, sizeof(value));

		if (result == sizeof(value)) {
			if (value > (uint64_t)INT_MAX + 1)
				value = (uint64_t)INT_MAX + 1;
			if (overrun > (int)(INT_MAX - value))
				overrun = INT_MAX;
			else
				overrun += value;
		}
	} while (result == sizeof(uint64_t)
			|| (result == -1 && errno == EINTR));

	if (result == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
		overrun = 0;
		errc = errno;
	} else {
		errno = errc;
		errc = 0;
	}

	if (overrun >= 0) {
		struct aio_queue queue;
		aio_queue_init(&queue);

#if !LELY_NO_THREADS
		mtx_lock(&impl->mtx);
#endif
		impl->overrun = overrun;
		aio_queue_move(&queue, &impl->queue, NULL);
#if !LELY_NO_THREADS
		mtx_unlock(&impl->mtx);
#endif

		aio_queue_cancel(&queue, errc);
	}

	return AIO_WATCH_READ;
}

static inline struct aio_timer_impl *
aio_impl_from_timer(const aio_timer_t *timer)
{
	assert(timer);

	return structof(timer, struct aio_timer_impl, timer_vptr);
}

static inline struct aio_timer_impl *
aio_impl_from_service(const struct aio_service *srv)
{
	assert(srv);

	return structof(srv, struct aio_timer_impl, srv);
}

static int
aio_timer_impl_open(struct aio_timer_impl *impl)
{
	assert(impl);

	int errc = 0;

	if (aio_timer_impl_close(impl) == -1) {
		errc = errno;
		goto error_close;
	}

	impl->tfd = timerfd_create(impl->clockid, TFD_NONBLOCK | TFD_CLOEXEC);
	if (impl->tfd == -1) {
		errc = errno;
		goto error_timerfd_create;
	}

	if (aio_reactor_watch(impl->reactor, &impl->watch, impl->tfd,
			AIO_WATCH_READ) == -1) {
		errc = errno;
		goto error_reactor_watch;
	}

	return 0;

error_reactor_watch:
	close(impl->tfd);
error_timerfd_create:
error_close:
	errno = errc;
	return -1;
}

static int
aio_timer_impl_close(struct aio_timer_impl *impl)
{
	assert(impl);

	int tfd = impl->tfd;
	if (tfd == -1)
		return 0;
	impl->tfd = -1;

	int result = 0;
	int errc = errno;

	if (!impl->shutdown && aio_reactor_watch(impl->reactor, &impl->watch,
			tfd, 0) == -1 && !result) {
		errc = errno;
		result = -1;
	}

	if (close(tfd) == -1 && !result) {
		errc = errno;
		result = -1;
	}

	errno = errc;
	return result;
}

#endif // __linux__
