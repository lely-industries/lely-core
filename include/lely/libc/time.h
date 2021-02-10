/**@file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<time.h>` and defines any missing functionality.
 *
 * @copyright 2013-2018 Lely Industries N.V.
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

#ifndef LELY_LIBC_TIME_H_
#define LELY_LIBC_TIME_H_

#include <lely/libc/sys/types.h>

#include <time.h>

#ifndef LELY_HAVE_ITIMERSPEC
#if defined(_POSIX_C_SOURCE) || defined(_POSIX_TIMERS) \
		|| defined(_TIMESPEC_DEFINED)
#define LELY_HAVE_ITIMERSPEC 1
#endif
#endif

#ifndef LELY_HAVE_TIMESPEC
#if __STDC_VERSION__ >= 201112L || _MSC_VER >= 1900 \
		|| _POSIX_C_SOURCE >= 199309L || defined(__CYGWIN__) \
		|| defined(_TIMESPEC_DEFINED) || defined(__timespec_defined)
#define LELY_HAVE_TIMESPEC 1
#endif
#endif

#ifndef LELY_HAVE_TIMESPEC_GET
#if (__STDC_VERSION__ >= 201112L || __USE_ISOC11 || _MSC_VER >= 1900) \
		&& defined(TIME_UTC)
#define LELY_HAVE_TIMESPEC_GET 1
#endif
#endif

#if !defined(_POSIX_C_SOURCE) && !defined(_POSIX_TIMERS) \
		&& !defined(__MINGW32__)

#ifndef CLOCK_REALTIME
/// The identifier of the system-wide clock measuring real time.
#define CLOCK_REALTIME 0
#endif

/**
 * The identifier for the system-wide monotonic clock, which is defined as a
 * clock measuring real time, whose value cannot be set via `clock_settime()`
 * and which cannot have negative clock jumps.
 */
#define CLOCK_MONOTONIC 1

/**
 * The identifier of the CPU-time clock associated with the process making a
 * `clock_*()` function call (not supported on Windows).
 */
#define CLOCK_PROCESS_CPUTIME_ID 2

/**
 * The identifier of the CPU-time clock associated with the thread making a
 * `clock_*()` function call (not supported on Windows).
 */
#define CLOCK_THREAD_CPUTIME_ID 3

#ifndef TIMER_ABSTIME
/// Flag indicating time is absolute.
#define TIMER_ABSTIME 1
#endif

#endif // !_POSIX_C_SOURCE && !_POSIX_TIMERS && !__MINGW32__

#if !LELY_HAVE_TIMESPEC

/// A time type with nanosecond resolution.
struct timespec {
	/// Whole seconds (>= 0).
	time_t tv_sec;
	/// Nanoseconds [0, 999999999].
	long tv_nsec;
};

#endif // !LELY_HAVE_TIMESPEC

#if !LELY_HAVE_ITIMERSPEC

/// A struct specifying an interval and initial value for a timer.
struct itimerspec {
	/// The timer period.
	struct timespec it_interval;
	/// The timer expiration.
	struct timespec it_value;
};

#endif // !LELY_HAVE_ITIMERSPEC

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_POSIX_TIMERS) && !defined(__MINGW32__)

/**
 * Obtains the resolution of a clock. Clock resolutions are
 * implementation-defined and cannot be set by a process.
 *
 * @param clock_id the identifier of the clock (one of #CLOCK_REALTIME,
 *                 #CLOCK_MONOTONIC, #CLOCK_PROCESS_CPUTIME_ID or
 *                 #CLOCK_THREAD_CPUTIME_ID).
 * @param res      the address at which to store the resolution of the specified
 *                 clock (can be NULL).
 *
 * @returns 0 on success, or -1 on error. In the latter case, `errno` is set to
 * indicate the error.
 *
 * @see clock_gettime(), clock_settime()
 */
int clock_getres(clockid_t clock_id, struct timespec *res);

/**
 * Obtains the current value of a clock.
 *
 * @param clock_id the identifier of the clock (one of #CLOCK_REALTIME,
 *                 #CLOCK_MONOTONIC, #CLOCK_PROCESS_CPUTIME_ID or
 *                 #CLOCK_THREAD_CPUTIME_ID).
 * @param tp       the address at which to store the value of the specified
 *                 clock (can be NULL).
 *
 * @returns 0 on success, or -1 on error. In the latter case, `errno` is set to
 * indicate the error.
 *
 * @see clock_getres(), clock_settime()
 */
int clock_gettime(clockid_t clock_id, struct timespec *tp);

/**
 * Sleeps until a time interval or absolute time has elapsed on a clock. If the
 * sleep is interrupted (by a signal, an I/O completion callback function or an
 * asynchronous procedure call), `EINTR` is returned. In the case of a relative
 * sleep, the remaining time is then stored at <b>rmtp</b>.
 *
 * Absolute sleep MAY be implemented on some platforms with a relative sleep
 * with respect to the current value of `clock_gettime()` for the specified
 * clock. Unfortunately, this implementation is susceptible to a race condition
 * if the process is preempted between the call to `clock_gettime()` and the
 * start of the relative sleep.
 *
 * Whether changing the value of the #CLOCK_REALTIME clock affects the wakeup
 * time of any threads in an absolute sleep is implementation-defined.
 *
 * @param clock_id the identifier of the clock (one of #CLOCK_REALTIME,
 *                 #CLOCK_MONOTONIC or #CLOCK_PROCESS_CPUTIME_ID).
 * @param flags    if #TIMER_ABSTIME is set in <b>flags</b>, this function shall
 *                 sleep until the value of the clock specified by
 *                 <b>clock_id</b> reaches the absolute time specified by
 *                 <b>rqtp</b>. If #TIMER_ABSTIME is not set, this function
 *                 shall sleep until the time interval specified by <b>rqtp</b>
 *                 has elapsed.
 * @param rqtp     a pointer to a time interval or absolute time, depending on
 *                 the value of <b>flags</b>.
 * @param rmtp     the address at which to store the remaining time if the
 *                 relative sleep was interrupted (can be NULL).
 *
 * @returns 0 on success, or an error number on error.
 */
int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *rqtp,
		struct timespec *rmtp);

/**
 * Sets the value of a clock.
 *
 * @param clock_id the identifier of the clock (one of #CLOCK_REALTIME,
 *                 #CLOCK_PROCESS_CPUTIME_ID or #CLOCK_THREAD_CPUTIME_ID).
 * @param tp       a pointer to the new value of the specified clock.
 *
 * @returns 0 on success, or -1 on error. In the latter case, `errno` is set to
 * indicate the error.
 *
 * @see clock_getres(), clock_gettime()
 */
int clock_settime(clockid_t clock_id, const struct timespec *tp);

/**
 * Equivalent to `clock_nanosleep(CLOCK_REALTIME, 0, rqtp, rmtp)`, except that
 * on error, it returns -1 and sets `errno` to indicate the error.
 */
int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);

#endif // !_POSIX_TIMERS && !__MINGW32__

#if !LELY_HAVE_TIMESPEC_GET

#ifndef TIME_UTC
/// An integer constant greater than 0 that designates the UTC time base.
#define TIME_UTC 1
#endif

/**
 * Sets the interval at <b>ts</b> to hold the current calendar time based on the
 * specified time base.
 *
 * If <b>base</b> is `TIME_UTC`, the <b>tv_sec</b> member is set to the number
 * of seconds since an implementation defined epoch, truncated to a whole value
 * and the <b>tv_nsec</b> member is set to the integral number of nanoseconds,
 * rounded to the resolution of the system clock.
 *
 * @returns the nonzero value <b>base</b> on success; otherwise, it returns
 * zero.
 */
int timespec_get(struct timespec *ts, int base);

#endif // !LELY_HAVE_TIMESPEC_GET

#ifdef __cplusplus
}
#endif

#endif // !LELY_LIBC_TIME_H_
