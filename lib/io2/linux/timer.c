/**@file
 * This file is part of the I/O library; it contains the I/O system timer
 * implementation for Linux.
 *
 * @see lely/io2/sys/timer.h
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

#include "io.h"

#if !LELY_NO_STDIO && defined(__linux__)

#include "../timer.h"
#include <lely/io2/ctx.h>
#include <lely/io2/posix/poll.h>
#include <lely/io2/sys/clock.h>
#include <lely/io2/sys/timer.h>
#include <lely/util/util.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#if !LELY_NO_THREADS
#include <pthread.h>
#include <sched.h>
#endif
#include <unistd.h>

#include <sys/timerfd.h>

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

static int io_timer_impl_svc_notify_fork(
		struct io_svc *svc, enum io_fork_event e);
static void io_timer_impl_svc_shutdown(struct io_svc *svc);

// clang-format off
static const struct io_svc_vtbl io_timer_impl_svc_vtbl = {
	&io_timer_impl_svc_notify_fork,
	&io_timer_impl_svc_shutdown
};
// clang-format on

struct io_timer_impl {
	const struct io_dev_vtbl *dev_vptr;
	const struct io_timer_vtbl *timer_vptr;
	io_poll_t *poll;
	struct io_svc svc;
	io_ctx_t *ctx;
	ev_exec_t *exec;
	clockid_t clockid;
	struct io_poll_watch watch;
	int tfd;
	struct ev_task wait_task;
#if !LELY_NO_THREADS
	pthread_mutex_t mtx;
#endif
	unsigned shutdown : 1;
	unsigned wait_posted : 1;
	struct sllist wait_queue;
	int overrun;
};

static void io_timer_impl_watch_func(struct io_poll_watch *watch, int events);
static void io_timer_impl_wait_task_func(struct ev_task *task);

static inline struct io_timer_impl *io_timer_impl_from_dev(const io_dev_t *dev);
static inline struct io_timer_impl *io_timer_impl_from_timer(
		const io_timer_t *timer);
static inline struct io_timer_impl *io_timer_impl_from_svc(
		const struct io_svc *svc);

static void io_timer_impl_pop(struct io_timer_impl *impl, struct sllist *queue,
		struct ev_task *task);

static int io_timer_impl_open(struct io_timer_impl *impl);
static int io_timer_impl_close(struct io_timer_impl *impl);

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

	impl->poll = poll;

	impl->svc = (struct io_svc)IO_SVC_INIT(&io_timer_impl_svc_vtbl);
	impl->ctx = ctx;

	impl->exec = exec;

	impl->clockid = clockid;

	impl->watch = (struct io_poll_watch)IO_POLL_WATCH_INIT(
			&io_timer_impl_watch_func);
	impl->tfd = -1;

	impl->wait_task = (struct ev_task)EV_TASK_INIT(
			impl->exec, &io_timer_impl_wait_task_func);

#if !LELY_NO_THREADS
	if ((errsv = pthread_mutex_init(&impl->mtx, NULL)))
		goto error_init_mtx;
#endif

	impl->shutdown = 0;
	impl->wait_posted = 0;

	sllist_init(&impl->wait_queue);

	impl->overrun = 0;

	if (io_timer_impl_open(impl) == -1) {
		errsv = errno;
		goto error_open;
	}

	io_ctx_insert(impl->ctx, &impl->svc);

	return timer;

	// io_timer_impl_close(impl);
error_open:
#if !LELY_NO_THREADS
	pthread_mutex_destroy(&impl->mtx);
error_init_mtx:
#endif
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

#if !LELY_NO_THREADS
	pthread_mutex_lock(&impl->mtx);
	// If necessary, busy-wait until io_timer_impl_wait_task_func()
	// completes.
	while (impl->wait_posted) {
		// Try to abort io_timer_impl_wait_task_func().
		if (ev_exec_abort(impl->wait_task.exec, &impl->wait_task))
			break;
		pthread_mutex_unlock(&impl->mtx);
		sched_yield();
		pthread_mutex_lock(&impl->mtx);
	}
	pthread_mutex_unlock(&impl->mtx);
#endif

	// Disarm the timer.
	io_timer_impl_close(impl);

#if !LELY_NO_THREADS
	pthread_mutex_destroy(&impl->mtx);
#endif
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

#if !LELY_NO_THREADS
	pthread_mutex_lock((pthread_mutex_t *)&impl->mtx);
#endif
	int overrun = impl->overrun;
#if !LELY_NO_THREADS
	pthread_mutex_unlock((pthread_mutex_t *)&impl->mtx);
#endif

	return overrun;
}

static int
io_timer_impl_gettime(const io_timer_t *timer, struct itimerspec *value)
{
	const struct io_timer_impl *impl = io_timer_impl_from_timer(timer);

	return timerfd_gettime(impl->tfd, value);
}

static int
io_timer_impl_settime(io_timer_t *timer, int flags,
		const struct itimerspec *value, struct itimerspec *ovalue)
{
	struct io_timer_impl *impl = io_timer_impl_from_timer(timer);

	int flags_ = 0;
	if (flags & TIMER_ABSTIME)
		flags_ |= TFD_TIMER_ABSTIME;

	return timerfd_settime(impl->tfd, flags_, value, ovalue);
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

static int
io_timer_impl_svc_notify_fork(struct io_svc *svc, enum io_fork_event e)
{
	struct io_timer_impl *impl = io_timer_impl_from_svc(svc);

	if (e != IO_FORK_CHILD || impl->shutdown)
		return 0;

	int result = 0;
	int errsv = errno;

	struct itimerspec value = { { 0, 0 }, { 0, 0 } };
	if (timerfd_gettime(impl->tfd, &value) == -1) {
		errsv = errno;
		result = -1;
	}

	if (io_timer_impl_close(impl) == -1 && !result) {
		errsv = errno;
		result = -1;
	}

	if (io_timer_impl_open(impl) == -1 && !result) {
		errsv = errno;
		result = -1;
	}

	if (timerfd_settime(impl->tfd, 0, &value, NULL) == -1 && !result) {
		errsv = errno;
		result = -1;
	}

	errno = errsv;
	return result;
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
	if (shutdown) {
		// Stop monitoring the timer file descriptor.
		io_poll_watch(impl->poll, impl->tfd, 0, &impl->watch);
		// Try to abort io_timer_impl_wait_task_func().
		// clang-format off
		if (impl->wait_posted && ev_exec_abort(impl->wait_task.exec,
				&impl->wait_task))
			// clang-format on
			impl->wait_posted = 0;
	}
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->mtx);
#endif
	// cppcheck-suppress duplicateCondition
	if (shutdown)
		// Cancel all pending operations.
		io_timer_impl_dev_cancel(dev, NULL);
}

static void
io_timer_impl_watch_func(struct io_poll_watch *watch, int events)
{
	assert(watch);
	struct io_timer_impl *impl =
			structof(watch, struct io_timer_impl, watch);
	(void)events;

#if !LELY_NO_THREADS
	pthread_mutex_lock(&impl->mtx);
#endif
	int post_wait = !impl->wait_posted && !impl->shutdown;
	if (post_wait)
		impl->wait_posted = 1;
#if !LELY_NO_THREADS
	pthread_mutex_unlock(&impl->mtx);
#endif
	// cppcheck-suppress duplicateCondition
	if (post_wait)
		ev_exec_post(impl->wait_task.exec, &impl->wait_task);
}

static void
io_timer_impl_wait_task_func(struct ev_task *task)
{
	assert(task);
	struct io_timer_impl *impl =
			structof(task, struct io_timer_impl, wait_task);

	int errsv = errno;
	int overrun = -1;
	int events = 0;

	ssize_t result;
	for (;;) {
		uintmax_t value = 0;
		do {
			errno = 0;
			result = read(impl->tfd, &value, sizeof(value));
		} while (result == -1 && errno == EINTR);
		if (result != sizeof(value))
			break;

		if (value > (uintmax_t)INT_MAX + 1)
			value = (uintmax_t)INT_MAX + 1;
		if (overrun > (int)(INT_MAX - value))
			overrun = INT_MAX;
		else
			overrun += value;
	}
	if (result == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		errno = 0;
		events |= IO_EVENT_IN;
	}

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	pthread_mutex_lock(&impl->mtx);
#endif
	if (overrun >= 0)
		impl->overrun = overrun;
	if (overrun >= 0 || errno)
		sllist_append(&queue, &impl->wait_queue);

	if (events && !impl->shutdown)
		io_poll_watch(impl->poll, impl->tfd, events, &impl->watch);
	impl->wait_posted = 0;
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

static int
io_timer_impl_open(struct io_timer_impl *impl)
{
	assert(impl);

	int errsv = 0;

	if (io_timer_impl_close(impl) == -1) {
		errsv = errno;
		goto error_close;
	}

	impl->tfd = timerfd_create(impl->clockid, TFD_NONBLOCK | TFD_CLOEXEC);
	if (impl->tfd == -1) {
		errsv = errno;
		goto error_timerfd_create;
	}

	if (io_poll_watch(impl->poll, impl->tfd, IO_EVENT_IN, &impl->watch)
			== -1) {
		errsv = errno;
		goto error_poll_watch;
	}

	return 0;

error_poll_watch:
	close(impl->tfd);
	impl->tfd = -1;
error_timerfd_create:
error_close:
	errno = errsv;
	return -1;
}

static int
io_timer_impl_close(struct io_timer_impl *impl)
{
	assert(impl);

	int tfd = impl->tfd;
	if (tfd == -1)
		return 0;
	impl->tfd = -1;

	int result = 0;
	int errsv = errno;

	if (!impl->shutdown
			&& io_poll_watch(impl->poll, tfd, 0, &impl->watch)
					== -1) {
		errsv = errno;
		result = -1;
	}

	if (close(tfd) == -1 && !result) {
		errsv = errno;
		result = -1;
	}

	errno = errsv;
	return result;
}

#endif // !LELY_NO_STDIO && __linux__
