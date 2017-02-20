/*!\file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<time.h>` and defines any missing functionality.
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

#ifndef LELY_LIBC_TIME_H
#define LELY_LIBC_TIME_H

#include <lely/libc/signal.h>

#include <time.h>

#ifndef LELY_HAVE_ITIMERSPEC
#if defined(_POSIX_C_SOURCE) || defined(_POSIX_TIMERS) \
		|| defined(_TIMESPEC_DEFINED)
#define LELY_HAVE_ITIMERSPEC	1
#endif
#endif

#ifndef LELY_HAVE_TIMESPEC
#if __STDC_VERSION__ >= 201112L || _MSC_VER >= 1900 \
		|| _POSIX_C_SOURCE >= 199309L || defined(__CYGWIN__) \
		|| defined(_TIMESPEC_DEFINED) || defined(__timespec_defined)
#define LELY_HAVE_TIMESPEC	1
#endif
#endif

#ifndef LELY_HAVE_TIMESPEC_GET
#if (__STDC_VERSION__ >= 201112L || __USE_ISOC11 || _MSC_VER >= 1900) \
		&& defined(TIME_UTC)
#define LELY_HAVE_TIMESPEC_GET	1
#endif
#endif

#if !defined(_POSIX_C_SOURCE) && !defined(_POSIX_TIMERS) \
		&& !defined(__MINGW32__)

//! The identifier of the system-wide clock measuring real time.
#define CLOCK_REALTIME	0

/*!
 * The identifier for the system-wide monotonic clock, which is defined as a
 * clock measuring real time, whose value cannot be set via `clock_settime()`
 * and which cannot have negative clock jumps.
 */
#define CLOCK_MONOTONIC	1

/*!
 * The identifier of the CPU-time clock associated with the process making a
 * `clock_*()` function call (not supported on Windows).
 */
#define CLOCK_PROCESS_CPUTIME_ID	2

/*!
 * The identifier of the CPU-time clock associated with the thread making a
 * `clock_*()` function call (not supported on Windows).
 */
#define CLOCK_THREAD_CPUTIME_ID	3

//! Flag indicating time is absolute.
#define TIMER_ABSTIME	1

#endif // !_POSIX_C_SOURCE && !_POSIX_TIMERS && !__MINGW32__

#ifndef LELY_HAVE_TIMESPEC
//! A time type with nanosecond resolution.
struct timespec {
	//! Whole seconds (>= 0).
	time_t tv_sec;
	//! Nanoseconds [0, 999999999].
	long tv_nsec;
};
#endif

#ifndef LELY_HAVE_ITIMERSPEC
//! A struct specifying an interval and initial value for a timer.
struct itimerspec {
	//! The timer period.
	struct timespec it_interval;
	//! The timer expiration.
	struct timespec it_value;
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_POSIX_C_SOURCE) && !defined(_POSIX_TIMERS) \
		&& !defined(__MINGW32__)

/*!
 * Obtains the resolution of a clock. Clock resolutions are
 * implementation-defined and cannot be set by a process.
 *
 * \param clock_id the identifier of the clock (one of #CLOCK_REALTIME,
 *                 #CLOCK_MONOTONIC, #CLOCK_PROCESS_CPUTIME_ID or
 *                 #CLOCK_THREAD_CPUTIME_ID).
 * \param res      the address at which to store the resolution of the specified
 *                 clock (can be NULL).
 *
 * \returns 0 on success, or -1 on error. In the latter case, `errno` is set to
 * indicate the error.
 *
 * \see clock_gettime(), clock_settime()
 */
LELY_LIBC_EXTERN int __cdecl clock_getres(clockid_t clock_id,
		struct timespec *res);

/*!
 * Obtains the current value of a clock.
 *
 * \param clock_id the identifier of the clock (one of #CLOCK_REALTIME,
 *                 #CLOCK_MONOTONIC, #CLOCK_PROCESS_CPUTIME_ID or
 *                 #CLOCK_THREAD_CPUTIME_ID).
 * \param tp       the address at which to store the value of the specified
 *                 clock (can be NULL).
 *
 * \returns 0 on success, or -1 on error. In the latter case, `errno` is set to
 * indicate the error.
 *
 * \see clock_getres(), clock_settime()
 */
LELY_LIBC_EXTERN int __cdecl clock_gettime(clockid_t clock_id,
		struct timespec *tp);

/*!
 * Sleeps until a time interval or absolute time has elapsed on a clock. If the
 * sleep is interrupted (by a signal, an I/O completion callback function or an
 * asynchronous procedure call), `EINTR` is returned. In the case of a relative
 * sleep, the remaining time is then stored at \a rmtp.
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
 * \param clock_id the identifier of the clock (one of #CLOCK_REALTIME,
 *                 #CLOCK_MONOTONIC or #CLOCK_PROCESS_CPUTIME_ID).
 * \param flags    if the #TIMER_ABSTIME is set in \a flags, this function shall
 *                 sleep until the value of the clock specified by \a clock_id
 *                 reaches the absolute time specified by \a rqtp. If
 *                 #TIMER_ABSTIME is not set, this function shall sleep until
 *                 the time interval specified by \a rqtp has elapsed.
 * \param rqtp     a pointer to a time interval or absolute time, depending on
 *                 the value of \a flags.
 * \param rmtp     the address at which to store the remaining time if the
 *                 relative sleep was interrupted (can be NULL).
 *
 * \returns 0 on success, or an error number on error.
 */
LELY_LIBC_EXTERN int __cdecl clock_nanosleep(clockid_t clock_id,
		int flags, const struct timespec *rqtp, struct timespec *rmtp);

/*!
 * Sets the value of a clock.
 *
 * \param clock_id the identifier of the clock (one of #CLOCK_REALTIME,
 *                 #CLOCK_PROCESS_CPUTIME_ID or #CLOCK_THREAD_CPUTIME_ID).
 * \param tp       a pointer to the new value of the specified clock.
 *
 * \returns 0 on success, or -1 on error. In the latter case, `errno` is set to
 * indicate the error.
 *
 * \see clock_getres(), clock_gettime()
 */
LELY_LIBC_EXTERN int __cdecl clock_settime(clockid_t clock_id,
		const struct timespec *tp);

/*!
 * Equivalent to `clock_nanosleep(CLOCK_REALTIME, 0, rqtp, rmtp)`, except that
 * on error, it returns -1 and sets `errno` to indicate the error.
 */
LELY_LIBC_EXTERN int __cdecl nanosleep(const struct timespec *rqtp,
		struct timespec *rmtp);

#endif // !_POSIX_C_SOURCE && !_POSIX_TIMERS && !__MINGW32__

#if defined(_WIN32) || (!defined(_POSIX_C_SOURCE) && !defined(_POSIX_TIMERS))

/*!
 * Creates a per-process timer using the specified clock as the timing base.
 *
 * \param clockid the identifier of the clock (one of #CLOCK_REALTIME,
 *                #CLOCK_MONOTONIC, #CLOCK_PROCESS_CPUTIME_ID or
 *                #CLOCK_THREAD_CPUTIME_ID). On Windows, only #CLOCK_REALTIME is
 *                supported.
 * \param evp     a pointer to a #sigevent struct defining the asynchronous
 *                notification to occur when the timer expires. If \a evp is
 *                NULL, a default #sigevent struct is used where the
 *                \a sigev_notify member has the value #SIGEV_SIGNAL, the
 *                \a sigev_signal member equals the default signal number, and
 *                the \a sigev_value member contains the timer ID. On Windows,
 *                only #SIGEV_NONE and #SIGEV_THREAD are supported for
 *                \a sigev_notify.
 * \param timerid the address at which to store the timer ID. The timer ID is
 *                guaranteed to be unique within the calling process until the
 *                timer is deleted.
 *
 * \returns 0 on success, or -1 on error. In the latter case, `errno` is set to
 * indicate the error.
 *
 * \see timer_delete()
 */
LELY_LIBC_EXTERN int __cdecl timer_create(clockid_t clockid,
		struct sigevent *evp, timer_t *timerid);

/*!
 * Deletes the specified timer created with timer_create(). If the timer is
 * armed when timer_delete() is called, the behavior shall be as if the timer is
 * automatically disarmed before removal.
 *
 * \returns 0 on success, or -1 on error. In the latter case, `errno` is set to
 * indicate the error.
 */
LELY_LIBC_EXTERN int __cdecl timer_delete(timer_t timerid);

/*!
 * Returns the timer expiration overrun count for the specified timer. Only a
 * single notification shall be issued for a given timer at any point in time.
 * When a timer expires with a pending notification, no notification is issued,
 * and a timer overrun shall occur. The overrun count contains the number of
 * extra timer expirations that occurred between the time the notification was
 * issued and when it was delivered.
 *
 * \returns 0 on success, or -1 on error. In the latter case, `errno` is set to
 * indicate the error.
 */
LELY_LIBC_EXTERN int __cdecl timer_getoverrun(timer_t timerid);

/*!
 * Obtains the amount of time until the specified timer expires and the reload
 * value of the timer.
 *
 * \param timerid a timer ID created with `timer_create()`.
 * \param value   the address of an #itimerspec struct. On success, the
 *                \a it_value member shall contain the time until the next
 *                expiration, or zero if the timer is disarmed. The
 *                \a it_interval member shall contain the reload value last set
 *                by `timer_settime()`.
 *
 * \returns 0 on success, or -1 on error. In the latter case, `errno` is set to
 * indicate the error.
 */
LELY_LIBC_EXTERN int __cdecl timer_gettime(timer_t timerid,
		struct itimerspec *value);

/*!
 * Arms or disarms a timer. If the specified timer was already armed, the
 * expiration time shall be reset to the specified value.
 *
 * \param timerid a timer ID created with `timer_create()`.
 * \param flags   if the #TIMER_ABSTIME is set in \a flags, the \a it_value
 *                member of *\a value contains the absolute time of the first
 *                expiration. If #TIMER_ABSTIME is not set, the \a it_value
 *                member contains the time interval until the first expiration.
 * \param value   a pointer to an #itimerspec struct containing the period and
 *                expiration of the timer. If the \a it_value member is zero,
 *                the timer shall be disarmed. If the \a it_interval member is
 *                non-zero, a periodic (or repetitive) timer is specified. The
 *                period is rounded up to the nearest multiple of the clock
 *                resolution.
 * \param ovalue  if not NULL, the address at which to store  previous amount of
 *                time before the timer would have expired, or zero if the timer
 *                was disarmed, together with the previous timer reload value.
 *
 * \returns 0 on success, or -1 on error. In the latter case, `errno` is set to
 * indicate the error.
 */
LELY_LIBC_EXTERN int __cdecl timer_settime(timer_t timerid, int flags,
		const struct itimerspec *value, struct itimerspec *ovalue);

#endif // _WIN32 || (!_POSIX_C_SOURCE && !_POSIX_TIMERS)

#ifndef LELY_HAVE_TIMESPEC_GET

#ifndef TIME_UTC
//! An integer constant greater than 0 that designates the UTC time base.
#define TIME_UTC	1
#endif

/*!
 * Sets the interval at \a ts to hold the current calendar time based on the
 * specified time base.
 *
 * If base is `TIME_UTC`, the \a tv_sec member is set to the number of seconds
 * since an implementation defined epoch, truncated to a whole value and the
 * \a tv_nsec member is set to the integral number of nanoseconds, rounded to
 * the resolution of the system clock.
 *
 * \returns the nonzero value \a base on success; otherwise, it returns zero.
 */
LELY_LIBC_EXTERN int __cdecl timespec_get(struct timespec *ts, int base);

#endif // !LELY_HAVE_TIMESPEC_GET

#ifdef __cplusplus
}
#endif

#endif

