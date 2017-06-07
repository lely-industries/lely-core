/*!\file
 * This header file is part of the utilities library; it contains the external
 * clock and timer declarations.
 *
 * The external timer API mimics the POSIX clock and timer functions. It is
 * 'external' in that the user must periodically invoke `xclock_settime()` to
 * advance the clock.
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

#ifndef LELY_UTIL_XTIME_H
#define LELY_UTIL_XTIME_H

#include <lely/libc/time.h>
#include <lely/util/util.h>

struct __xclock;
#ifndef __cplusplus
//! An opaque external clock type.
typedef struct __xclock xclock_t;
#endif

struct __xtimer;
#ifndef __cplusplus
//! An opaque external timer type.
typedef struct __xtimer xtimer_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

LELY_UTIL_EXTERN void *__xclock_alloc(void);
LELY_UTIL_EXTERN void __xclock_free(void *ptr);
LELY_UTIL_EXTERN struct __xclock *__xclock_init(struct __xclock *clock);
LELY_UTIL_EXTERN void __xclock_fini(struct __xclock *clock);

/*!
 * Creates an external clock.
 *
 * \returns a pointer to a new clock, or NULL on error. In the latter case, the
 * error number can be obtained with `get_errnum()`.
 *
 * \see xclock_destroy()
 */
LELY_UTIL_EXTERN xclock_t *xclock_create(void);

//! Destroys an external clock. \see xclock_create()
LELY_UTIL_EXTERN void xclock_destroy(xclock_t *clock);

/*!
 * Obtains the resolution of an external clock. The resolution is defined as the
 * interval between the last two updates of `xclock_settime()`.
 *
 * \param clock a pointer to a clock.
 * \param res  the address at which to store the resolution  (can be NULL).
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see xclock_gettime(), xclock_settime()
 */
LELY_UTIL_EXTERN int xclock_getres(xclock_t *clock, struct timespec *res);

/*!
 * Obtains the current value of an external clock.
 *
 * \param clock a pointer to a clock.
 * \param tp    the address at which to store the value of the clock (can be
 *              NULL).
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see xclock_getres(), xclock_settime()
 */
LELY_UTIL_EXTERN int xclock_gettime(xclock_t *clock, struct timespec *tp);

/*!
 * Sleeps until a time interval or absolute time has elapsed on an external
 * clock.
 *
 * \param clock a pointer to a clock.
 * \param flags if the `TIMER_ABSTIME` is set in \a flags, this function shall
 *              sleep until the value of the specified clock reaches the
 *              absolute time specified by \a rqtp. If `TIMER_ABSTIME` is not
 *              set, this function shall sleep until the time interval specified
 *              by \a rqtp has elapsed.
 * \param rqtp  a pointer to a time interval or absolute time, depending on
 *              the value of \a flags.
 * \param rmtp  the address at which to store the remaining time if the an error
 *              occurred during a relative sleep (can be NULL).
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_UTIL_EXTERN int xclock_nanosleep(xclock_t *clock, int flags,
		const struct timespec *rqtp, struct timespec *rmtp);

/*!
 * Sets the value of a clock. This function MAY wake up threads waiting on
 * `xclock_nanosleep()` with the specified clock and MAY trigger any timers
 * attached to the clock.
 *
 * \param clock a pointer to a clock.
 * \param tp    a pointer to the new value of the clock.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see xclock_getres(), xclock_gettime()
 */
LELY_UTIL_EXTERN int xclock_settime(xclock_t *clock, const struct timespec *tp);

LELY_UTIL_EXTERN void *__xtimer_alloc(void);
LELY_UTIL_EXTERN void __xtimer_free(void *ptr);
LELY_UTIL_EXTERN struct __xtimer *__xtimer_init(struct __xtimer *timer,
		xclock_t *clock, const struct sigevent *evp);
LELY_UTIL_EXTERN void __xtimer_fini(struct __xtimer *timer);

/*!
 * Creates an external timer. The timer is triggered by `xclock_settime()`.
 *
 * \param clock a pointer to an external clock.
 * \param evp   a pointer to a `sigevent` struct defining the asynchronous
 *              notification to occur when the timer expires. Only `SIGEV_NONE`
 *              and `SIGEV_THREAD` are supported for the \a sigev_notify member.
 *
 * \returns a pointer to a new timer, or NULL on error. In the latter case, the
 * error number can be obtained with `get_errnum()`.
 *
 * \see xtimer_destroy()
 */
LELY_UTIL_EXTERN xtimer_t *xtimer_create(xclock_t *clock,
		const struct sigevent *evp);

//! Destroys an external timer. \see xtimer_create()
LELY_UTIL_EXTERN void xtimer_destroy(xtimer_t *timer);

/*!
 * Returns the timer expiration overrun count for the specified external timer.
 * Only a single notification shall be issued for a given timer at any point in
 * time. When a timer expires with a pending notification, no notification is
 * issued, and a timer overrun shall occur. The overrun count contains the
 * number of extra timer expirations that occurred between the time the
 * notification was issued and when it was delivered.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_UTIL_EXTERN int xtimer_getoverrun(xtimer_t *timer);

/*!
 * Obtains the amount of time until an external timer expires and the reload
 * value of the timer.
 *
 * \param timer a pointer to the timer.
 * \param value the address of an `itimerspec` struct. On success, the
 *              \a it_value member shall contain the time until the next
 *              expiration, or zero if the timer is disarmed. The \a it_interval
 *              member shall contain the reload value last set by
 *              `xtimer_settime()`.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_UTIL_EXTERN int xtimer_gettime(xtimer_t *timer, struct itimerspec *value);

/*!
 * Arms or disarms an external timer. If the specified timer was already armed,
 * the expiration time shall be reset to the specified value.
 *
 * \param timer  a pointer to the timer.
 * \param flags  if the `TIMER_ABSTIME` is set in \a flags, the \a it_value
 *               member of *\a value contains the absolute time of the first
 *               expiration. If `TIMER_ABSTIME` is not set, the \a it_value
 *               member contains the time interval until the first expiration.
 * \param value  a pointer to an `itimerspec` struct containing the period and
 *               expiration of the timer. If the \a it_value member is zero, the
 *               timer shall be disarmed. If the \a it_interval member is
 *               non-zero, a periodic (or repetitive) timer is specified.
 * \param ovalue if not NULL, the address at which to store  previous amount of
 *               time before the timer would have expired, or zero if the timer
 *               was disarmed, together with the previous timer reload value.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_UTIL_EXTERN int xtimer_settime(xtimer_t *timer, int flags,
		const struct itimerspec *value, struct itimerspec *ovalue);

#ifdef __cplusplus
}
#endif

#endif

