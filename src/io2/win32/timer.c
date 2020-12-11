/**@file
 * This file is part of the I/O library; it contains the I/O system timer
 * implementation for Windows.
 *
 * @see lely/io2/sys/timer.h
 *
 * @copyright 2018-2019 Lely Industries N.V.
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

#if !LELY_NO_STDIO && _WIN32

#include "../timer.h"
#include <lely/io2/ctx.h>
#include <lely/io2/sys/clock.h>
#include <lely/io2/sys/timer.h>
#include <lely/io2/win32/poll.h>
#include <lely/util/errnum.h>
#include <lely/util/time.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdlib.h>

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
#if !LELY_NO_THREADS
	CRITICAL_SECTION CriticalSection1;
#endif
	int shutdown;
	struct sllist queue;
#if !LELY_NO_THREADS
	CRITICAL_SECTION CriticalSection2;
#endif
	struct itimerspec value;
	int overrun;
#if !LELY_NO_THREADS
	CRITICAL_SECTION CriticalSection3;
#endif
	HANDLE Timer;
};

static void CALLBACK io_timer_impl_func(
		PVOID lpParameter, BOOLEAN TimerOrWaitFired);

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
	if (!impl) {
		set_errc(errno2c(errno));
		return NULL;
	}
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

	impl->dev_vptr = &io_timer_impl_dev_vtbl;
	impl->timer_vptr = &io_timer_impl_vtbl;

	impl->svc = (struct io_svc)IO_SVC_INIT(&io_timer_impl_svc_vtbl);
	impl->ctx = ctx;

	impl->exec = exec;

	impl->clockid = clockid;

#if !LELY_NO_THREADS
	InitializeCriticalSection(&impl->CriticalSection1);
#endif
	impl->shutdown = 0;
	sllist_init(&impl->queue);

#if !LELY_NO_THREADS
	InitializeCriticalSection(&impl->CriticalSection2);
#endif
	impl->value = (struct itimerspec){ { 0, 0 }, { 0, 0 } };
	impl->overrun = 0;

#if !LELY_NO_THREADS
	InitializeCriticalSection(&impl->CriticalSection3);
#endif
	impl->Timer = NULL;

	io_ctx_insert(impl->ctx, &impl->svc);

	return timer;
}

void
io_timer_fini(io_timer_t *timer)
{
	struct io_timer_impl *impl = io_timer_impl_from_timer(timer);

	io_ctx_remove(impl->ctx, &impl->svc);
	// Cancel all pending tasks.
	io_timer_impl_svc_shutdown(&impl->svc);

	if (impl->Timer)
		DeleteTimerQueueTimer(NULL, impl->Timer, INVALID_HANDLE_VALUE);

#if !LELY_NO_THREADS
	DeleteCriticalSection(&impl->CriticalSection3);
	DeleteCriticalSection(&impl->CriticalSection2);
	DeleteCriticalSection(&impl->CriticalSection1);
#endif
}

io_timer_t *
io_timer_create(io_poll_t *poll, ev_exec_t *exec, clockid_t clockid)
{
	DWORD dwErrCode = 0;

	io_timer_t *timer = io_timer_alloc();
	if (!timer) {
		dwErrCode = GetLastError();
		goto error_alloc;
	}

	io_timer_t *tmp = io_timer_init(timer, poll, exec, clockid);
	if (!tmp) {
		dwErrCode = GetLastError();
		goto error_init;
	}
	timer = tmp;

	return timer;

error_init:
	io_timer_free((void *)timer);
error_alloc:
	SetLastError(dwErrCode);
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

	return io_timer_wait_queue_post(&queue, -1, ERROR_OPERATION_ABORTED);
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
	EnterCriticalSection((LPCRITICAL_SECTION)&impl->CriticalSection2);
#endif
	int overrun = impl->overrun;
#if !LELY_NO_THREADS
	LeaveCriticalSection((LPCRITICAL_SECTION)&impl->CriticalSection2);
#endif

	return overrun;
}

static int
io_timer_impl_gettime(const io_timer_t *timer, struct itimerspec *value)
{
	const struct io_timer_impl *impl = io_timer_impl_from_timer(timer);
	assert(value);

#if !LELY_NO_THREADS
	EnterCriticalSection((LPCRITICAL_SECTION)&impl->CriticalSection2);
#endif
	struct itimerspec value_ = impl->value;
#if !LELY_NO_THREADS
	LeaveCriticalSection((LPCRITICAL_SECTION)&impl->CriticalSection2);
#endif

	if (value_.it_value.tv_sec || value_.it_value.tv_nsec) {
		struct timespec now = { 0, 0 };
		if (clock_gettime(impl->clockid, &now) == -1) {
			set_errc(errno2c(errno));
			return -1;
		}
		timespec_sub(&value_.it_value, &now);
	}

	if (value)
		*value = value_;

	return 0;
}

static int
io_timer_impl_settime(io_timer_t *timer, int flags,
		const struct itimerspec *value, struct itimerspec *ovalue)
{
	struct io_timer_impl *impl = io_timer_impl_from_timer(timer);
	assert(value);

	struct timespec now = { 0, 0 };
	if (clock_gettime(impl->clockid, &now) == -1) {
		set_errc(errno2c(errno));
		return -1;
	}

	ULONGLONG DueTime = 0;
	ULONGLONG Period = 0;

	struct timespec period = value->it_interval;
	struct timespec expiry = value->it_value;
	int arm = expiry.tv_sec || expiry.tv_nsec;
	if (arm) {
		if (period.tv_nsec < 0 || period.tv_nsec >= 1000000000l) {
			SetLastError(ERROR_INVALID_PARAMETER);
			return -1;
		}
		if (period.tv_sec < 0)
			period = (struct timespec){ 0, 0 };
		Period = (ULONGLONG)period.tv_sec * 1000
				+ (period.tv_nsec + 999999l) / 1000000l;
		period.tv_nsec = (Period % 1000) * 1000000l;
		if (Period > ULONG_MAX) {
			SetLastError(ERROR_INVALID_PARAMETER);
			return -1;
		}

		if (expiry.tv_nsec < 0 || expiry.tv_nsec >= 1000000000l) {
			SetLastError(ERROR_INVALID_PARAMETER);
			return -1;
		}
		if (flags & TIMER_ABSTIME)
			timespec_sub(&expiry, &now);
		if (expiry.tv_sec < 0)
			expiry = (struct timespec){ 0, 0 };
		DueTime = (ULONGLONG)expiry.tv_sec * 1000
				+ (expiry.tv_nsec + 999999l) / 1000000l;
		if (DueTime > ULONG_MAX) {
			SetLastError(ERROR_INVALID_PARAMETER);
			return -1;
		}

		timespec_add(&expiry, &now);
	} else {
		period = (struct timespec){ 0, 0 };
	}

	int result = 0;
	DWORD dwErrCode = GetLastError();

#if !LELY_NO_THREADS
	EnterCriticalSection(&impl->CriticalSection3);
#endif

	if (impl->Timer) {
		DeleteTimerQueueTimer(NULL, impl->Timer, INVALID_HANDLE_VALUE);
		impl->Timer = NULL;
	}

#if !LELY_NO_THREADS
	EnterCriticalSection(&impl->CriticalSection2);
#endif

	struct itimerspec ovalue_ = impl->value;
	if (ovalue_.it_value.tv_sec || ovalue_.it_value.tv_nsec)
		timespec_sub(&ovalue_.it_value, &now);
	impl->value = (struct itimerspec){ period, expiry };
	impl->overrun = 0;

	// TODO: Check if WT_EXECUTEDEFAULT is the best option.
	ULONG Flags = WT_EXECUTEDEFAULT;
	if (!Period)
		Flags |= WT_EXECUTEONLYONCE;

	// clang-format off
	if (arm && !CreateTimerQueueTimer(&impl->Timer, NULL,
			&io_timer_impl_func, impl, DueTime, Period, Flags)) {
		// clang-format on
		result = -1;
		dwErrCode = GetLastError();

		impl->value = (struct itimerspec){ { 0, 0 }, { 0, 0 } };
	}

#if !LELY_NO_THREADS
	LeaveCriticalSection(&impl->CriticalSection2);
	LeaveCriticalSection(&impl->CriticalSection3);
#endif

	if (ovalue)
		*ovalue = ovalue_;

	SetLastError(dwErrCode);
	return result;
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
	EnterCriticalSection(&impl->CriticalSection1);
#endif
	if (impl->shutdown) {
#if !LELY_NO_THREADS
		LeaveCriticalSection(&impl->CriticalSection1);
#endif
		io_timer_wait_post(wait, -1, ERROR_OPERATION_ABORTED);
	} else {
		sllist_push_back(&impl->queue, &task->_node);
#if !LELY_NO_THREADS
		LeaveCriticalSection(&impl->CriticalSection1);
#endif
	}
}

static void
io_timer_impl_svc_shutdown(struct io_svc *svc)
{
	struct io_timer_impl *impl = io_timer_impl_from_svc(svc);
	io_dev_t *dev = &impl->dev_vptr;

#if !LELY_NO_THREADS
	EnterCriticalSection(&impl->CriticalSection1);
#endif
	int shutdown = !impl->shutdown;
	impl->shutdown = 1;
#if !LELY_NO_THREADS
	LeaveCriticalSection(&impl->CriticalSection1);
#endif

	if (shutdown)
		io_timer_impl_dev_cancel(dev, NULL);
}

static void CALLBACK
io_timer_impl_func(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	struct io_timer_impl *impl = lpParameter;
	assert(impl);
	(void)TimerOrWaitFired;

	int errsv = errno;
	int overrun = 0;
	int errc = 0;

	struct timespec now = { 0, 0 };
	if (clock_gettime(impl->clockid, &now) == -1) {
		errc = errno2c(errno);
		overrun = -1;
	}

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	EnterCriticalSection(&impl->CriticalSection2);
#endif

	if (overrun >= 0) {
		struct timespec *period = &impl->value.it_interval;
		struct timespec *expiry = &impl->value.it_value;
		if (period->tv_sec || period->tv_nsec) {
			assert(!(period->tv_nsec % 1000000l));
			ULONG Period = period->tv_sec * 1000
					+ period->tv_nsec / 1000000l;
			LONGLONG overrun = timespec_diff_msec(&now, expiry)
					/ Period;
			timespec_add_msec(expiry, (overrun + 1) * Period);
			impl->overrun = MIN(MAX(overrun, INT_MIN), INT_MAX);
		} else {
			*expiry = (struct timespec){ 0, 0 };
			impl->overrun = 0;
		}
		overrun = impl->overrun;
	}

	if (overrun >= 0 || errc)
		sllist_append(&queue, &impl->queue);

#if !LELY_NO_THREADS
	LeaveCriticalSection(&impl->CriticalSection2);
#endif

	io_timer_wait_queue_post(&queue, overrun, errc);
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
	EnterCriticalSection(&impl->CriticalSection1);
#endif
	if (!task)
		sllist_append(queue, &impl->queue);
	else if (sllist_remove(&impl->queue, &task->_node))
		sllist_push_back(queue, &task->_node);
#if !LELY_NO_THREADS
	LeaveCriticalSection(&impl->CriticalSection1);
#endif
}

#endif // !LELY_NO_STDIO && _WIN32
