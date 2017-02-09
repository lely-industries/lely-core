/*!\file
 * This file is part of the C11 and POSIX compatibility library.
 *
 * \see lely/libc/time.h
 *
 * \copyright 2017 Lely Industries N.V.
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

#include "libc.h"

#ifndef LELY_NO_RT

#ifdef _WIN32

#include <lely/libc/threads.h>
#include "timespec.h"

#include <stdlib.h>

#include <process.h>

//! The magic number used to check the validity of a timer ("LELY").
#define TIMER_MAGIC	0x594c454c

//! The timer struct.
struct timer {
	/*!
	 * The magic number used to check the validity of the timer (MUST equal
	 * #TIMER_MAGIC).
	 */
	unsigned int magic;
	//! The notification type (#SIGEV_NONE or #SIGEV_THREAD).
	int sigev_notify;
	//! The signal value.
	union sigval sigev_value;
	//! The notification function.
	void (__cdecl *sigev_notify_function)(union sigval);
	//! The waitable timer object.
	HANDLE hTimer;
	//! The mutex protecting #expire, #period, #armed and #overrun.
	mtx_t mtx;
	//! The absolute expiration time.
	struct timespec expire;
	//! The period.
	struct timespec period;
	//! The expiration time passed to `SetWaitableTimer()`.
	LARGE_INTEGER DueTime;
	//! The period (in milliseconds) passed to `SetWaitableTimer()`.
	LONG lPeriod;
	//! A flag indicating whether the timer is armed.
	int armed;
	//! The overrun counter.
	int overrun;
	//! A pointer to the next timer in #timer_list.
	struct timer *next;
};

//! Initializes #timer_mtx and #timer_exit and starts the timer thread.
static void __cdecl timer_init(void);
//! Signals the timer thread to exit.
static void __cdecl timer_fini(void);
//! The function running in the timer thread.
static void __cdecl timer_start(void *arglist);

//! The asynchronous procedure used to arm queued timers.
static void __stdcall timer_apc_set(ULONG_PTR dwParam);
//! The asynchronous procedure invoked when a timer is triggered.
static void __stdcall timer_apc_proc(LPVOID lpArgToCompletionRoutine,
		DWORD dwTimerLowValue, DWORD dwTimerHighValue);

//! The flag ensuring timer_init() is only called once.
static once_flag timer_once = ONCE_FLAG_INIT;
//! The mutex protecting #timer_list and the \a next field in the #timer struct.
static mtx_t timer_mtx;
//! The list of timers waiting to be armed.
static struct timer *timer_list;
//! The event used by time_fini() to signal the timer thread to exit.
static HANDLE timer_exit;
//! The handle of the timer thread.
static uintptr_t timer_thr = -1;

LELY_LIBC_EXPORT int __cdecl
timer_create(clockid_t clockid, struct sigevent *evp, timer_t *timerid)
{
	switch (clockid) {
	case CLOCK_REALTIME:
		break;
	case CLOCK_MONOTONIC:
	case CLOCK_PROCESS_CPUTIME_ID:
	case CLOCK_THREAD_CPUTIME_ID:
		errno = ENOTSUP;
		return -1;
	default:
		errno = EINVAL;
		return -1;
	}

	int sigev_notify = SIGEV_SIGNAL;
	if (evp)
		sigev_notify = evp->sigev_notify;
	switch (sigev_notify) {
	case SIGEV_SIGNAL:
		errno = ENOTSUP;
		return -1;
	case SIGEV_NONE:
	case SIGEV_THREAD:
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	int errsv = 0;

	struct timer *timer = malloc(sizeof(*timer));
	if (__unlikely(!timer)) {
		errsv = errno;
		goto error_malloc_timer;
	}

	timer->magic = TIMER_MAGIC;
	timer->sigev_notify = evp->sigev_notify;
	timer->sigev_value = evp->sigev_value;
	timer->sigev_notify_function = evp->sigev_notify_function;

	timer->hTimer = NULL;
	if (timer->sigev_notify == SIGEV_THREAD) {
		timer->hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
		if (__unlikely(!timer->hTimer)) {
			errsv = EAGAIN;
			goto error_CreateWaitableTimer;
		}
	}

	mtx_init(&timer->mtx, mtx_plain);

	timer->expire = (struct timespec){ 0, 0 };
	timer->period = (struct timespec){ 0, 0 };
	timer->DueTime.QuadPart = 0;
	timer->lPeriod = 0;
	timer->armed = 0;
	timer->overrun = 0;
	timer->next = NULL;

	*timerid = timer;

	return 0;

error_CreateWaitableTimer:
	free(timer);
error_malloc_timer:
	errno = errsv;
	return -1;
}

LELY_LIBC_EXPORT int __cdecl
timer_delete(timer_t timerid)
{
	struct timer *timer = timerid;
	if (__unlikely(!timer || timer->magic != TIMER_MAGIC)) {
		errno = EINVAL;
		return -1;
	}

	timer_settime(timerid, 0, &(struct itimerspec){ { 0, 0 }, { 0, 0 } },
			NULL);

	if (timer->sigev_notify == SIGEV_THREAD)
		CloseHandle(timer->hTimer);
	mtx_destroy(&timer->mtx);
	timer->magic = 0;

	free(timer);

	return 0;
}

LELY_LIBC_EXPORT int __cdecl
timer_getoverrun(timer_t timerid)
{
	struct timer *timer = timerid;
	if (__unlikely(!timer || timer->magic != TIMER_MAGIC)) {
		errno = EINVAL;
		return -1;
	}

	mtx_lock(&timer->mtx);
	int overrun = timer->overrun;
	mtx_unlock(&timer->mtx);

	return overrun;
}

LELY_LIBC_EXPORT int __cdecl
timer_gettime(timer_t timerid, struct itimerspec *value)
{
	struct timer *timer = timerid;
	if (__unlikely(!timer || timer->magic != TIMER_MAGIC)) {
		errno = EINVAL;
		return -1;
	}

	mtx_lock(&timer->mtx);
	struct timespec expire = timer->expire;
	struct timespec period = timer->period;
	mtx_unlock(&timer->mtx);

	if (expire.tv_sec || expire.tv_nsec) {
		struct timespec now = { 0, 0 };
		if (__unlikely(clock_gettime(CLOCK_REALTIME, &now) == -1))
			return -1;
		timespec_sub(&expire, &now);
	}

	value->it_interval = period;
	value->it_value = expire;

	return 0;
}

LELY_LIBC_EXPORT int __cdecl
timer_settime(timer_t timerid, int flags, const struct itimerspec *value,
		struct itimerspec *ovalue)
{
	struct timer *timer = timerid;
	if (__unlikely(!timer || timer->magic != TIMER_MAGIC)) {
		errno = EINVAL;
		return -1;
	}

	struct timespec period = value->it_interval;
	struct timespec expire = value->it_value;

	int arm = expire.tv_sec != 0 || expire.tv_nsec != 0;

	if (__unlikely(arm && (period.tv_nsec < 0
			|| period.tv_nsec >= 1000000000l))) {
		errno = EINVAL;
		return -1;
	}
	if (!arm || period.tv_sec < 0)
		period = (struct timespec){ 0, 0 };

	LONG lPeriod = 0;
	if (arm && timer->sigev_notify == SIGEV_THREAD) {
		if (__unlikely(period.tv_sec > (LONG_MAX - 1000) / 1000)) {
			errno = EINVAL;
			return -1;
		}
		// Round the period up to the nearest millisecond.
		lPeriod = (LONG)(period.tv_sec * 1000)
				+ (period.tv_nsec + 999999l) / 1000000l;
		period = (struct timespec) {
			lPeriod / 1000,
			(lPeriod % 1000) * 1000000l
		};
	}

	struct timespec now = { 0, 0 };
	if (__unlikely(clock_gettime(CLOCK_REALTIME, &now) == -1))
		return -1;

	LARGE_INTEGER DueTime = { { 0, 0 } };
	if (arm && timer->sigev_notify == SIGEV_THREAD)
		DueTime.QuadPart = expire.tv_sec * 10000000l
				+ expire.tv_nsec / 100;
	if (arm && !(flags & TIMER_ABSTIME)) {
		// A relative expiration time is indicated with a negative value
		// in the call to SwtWaitableTimer().
		if (timer->sigev_notify == SIGEV_THREAD)
			DueTime.QuadPart *= -1;
		// Compute the absolute expiration time.
		timespec_add(&expire, &now);
	}

	if (timer->sigev_notify == SIGEV_THREAD) {
		// Start the timer thread, if necessary.
		call_once(&timer_once, &timer_init);

		mtx_lock(&timer_mtx);

		// Remove the timer from the queue, if present.
		struct timer **ptimer = &timer_list;
		for (; *ptimer && *ptimer != timer; ptimer = &(*ptimer)->next);
		if (*ptimer)
			*ptimer = (*ptimer)->next;
	}

	mtx_lock(&timer->mtx);

	if (ovalue) {
		if (timer->armed) {
			ovalue->it_interval = timer->period;
			ovalue->it_value = timer->expire;
			timespec_sub(&ovalue->it_value, &now);
		} else {
			*ovalue = (struct itimerspec){ { 0, 0 }, { 0, 0 } };
		}
	}
	timer->expire = expire;
	timer->period = period;

	if (timer->sigev_notify == SIGEV_THREAD) {
		timer->DueTime = DueTime;
		timer->lPeriod = lPeriod;

		if (timer->armed)
			CancelWaitableTimer(timer->hTimer);
		// timer_apc_wait() will arm the timer.
		timer->armed = 0;
		timer->overrun = 0;

		timer->next = NULL;
		// Append the timer to the queue.
		struct timer **ptimer = &timer_list;
		for (; *ptimer; ptimer = &(*ptimer)->next);
		*ptimer = timer;
	} else {
		timer->armed = arm;
	}

	mtx_unlock(&timer->mtx);

	if (timer->sigev_notify == SIGEV_THREAD) {
		mtx_unlock(&timer_mtx);
		if (arm)
			QueueUserAPC(&timer_apc_set, (HANDLE)timer_thr, 0);
	}

	return 0;
}

static void __cdecl
timer_init(void)
{
	mtx_init(&timer_mtx, mtx_plain);

	timer_exit = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (__unlikely(!timer_exit))
		goto error_create_timer_exit;

	timer_thr = _beginthread(&timer_start, 0, NULL);
	if (__unlikely(timer_thr == (uintptr_t)-1))
		goto error__beginthread;

	if (__unlikely(atexit(&timer_fini)))
		goto error_atexit;

	return;

error_atexit:
	SetEvent(timer_exit);
	timer_thr = -1;
	return;
error__beginthread:
	CloseHandle(timer_exit);
error_create_timer_exit:
	mtx_destroy(&timer_mtx);
}

static void __cdecl
timer_fini(void)
{
	SetEvent(timer_exit);
}

static void __cdecl
timer_start(void *arglist)
{
	__unused_var(arglist);

	// Wait until the #timer_exit event is signaled. We use an alertable
	// wait to allow timer_apc_set() and timer_apc_proc() to run on this
	// thread.
	while (WaitForSingleObjectEx(timer_exit, INFINITE, TRUE)
			== WAIT_IO_COMPLETION);
	// Finalize the objects initialized by timer_init().
	CloseHandle(timer_exit);

	mtx_destroy(&timer_mtx);
}

static void __stdcall
timer_apc_set(ULONG_PTR dwParam)
{
	__unused_var(dwParam);

	// Arm all queued timers.
	mtx_lock(&timer_mtx);
	while (timer_list) {
		struct timer *timer = timer_list;
		timer_list = timer_list->next;

		mtx_lock(&timer->mtx);
		SetWaitableTimer(timer->hTimer, &timer->DueTime,
				timer->lPeriod, &timer_apc_proc, timer, TRUE);
		timer->armed = 1;
		mtx_unlock(&timer->mtx);
	}
	mtx_unlock(&timer_mtx);
}

static void __stdcall
timer_apc_proc(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue,
		DWORD dwTimerHighValue)
{
	struct timer *timer = lpArgToCompletionRoutine;

	union sigval sigev_value;
	void (__cdecl *sigev_notify_function)(union sigval) = NULL;

	mtx_lock(&timer->mtx);
	if (timer->armed) {
		sigev_value = timer->sigev_value;
		sigev_notify_function = timer->sigev_notify_function;

		if (timer->period.tv_sec || timer->period.tv_nsec) {
			// If the timer is periodic, calculate the overrun
			// counter.
			FILETIME ft = { 0, 0 };
			tp2ft(&timer->expire, &ft);
			ULARGE_INTEGER expire = { {
				ft.dwLowDateTime,
				ft.dwHighDateTime
			} };
			ft = (FILETIME){ dwTimerLowValue, dwTimerHighValue };
			ULARGE_INTEGER now = { {
				ft.dwLowDateTime,
				ft.dwHighDateTime
			} };
			LONGLONG overrun = (now.QuadPart - expire.QuadPart)
					/ 10000 / timer->lPeriod;
			// A negative overrun indicates an overflow.
			if (__unlikely(overrun < 0 || overrun > INT_MAX))
				overrun = INT_MAX;
			timer->overrun = (int)overrun;
			// Update the expiration time.
			ft2tp(&ft, &timer->expire);
			timespec_add(&timer->expire, &timer->period);
		} else {
			// Reset the timer if it is non-periodic.
			timer->expire = (struct timespec){ 0, 0 };
			timer->period = (struct timespec){ 0, 0 };
			timer->DueTime.QuadPart = 0;
			timer->lPeriod = 0;
			timer->armed = 0;
			timer->overrun = 0;
		}
	}
	mtx_unlock(&timer->mtx);

	// Call the notification function without holding any locks. This allows
	// the function to reset its own timer.
	if (sigev_notify_function)
		sigev_notify_function(sigev_value);
}

#endif // _WIN32

#endif // !LELY_NO_RT

