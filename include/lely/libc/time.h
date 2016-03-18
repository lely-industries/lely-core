/*!\file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<time.h>` and defines any missing functionality.
 *
 * \copyright 2016 Lely Industries N.V.
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

#include <lely/libc/libc.h>

#include <time.h>

#if !(__STDC_VERSION__ >= 201112L) \
		&& !(_POSIX_C_SOURCE >= 199309L || defined(__CYGWIN__) \
		|| defined(_TIMESPEC_DEFINED) || defined(__timespec_defined))
//! A time type with nanosecond resolution.
struct timespec {
	//! Whole seconds (>= 0).
	time_t tv_sec;
	//! Nanoseconds [0, 999999999].
	long tv_nsec;
};
#endif

#if !defined(_POSIX_C_SOURCE) && !defined(_POSIX_TIMERS)

#ifdef __cplusplus
extern "C" {
#endif

LELY_LIBC_EXTERN int __cdecl nanosleep(const struct timespec *rqtp,
		struct timespec *rmtp);

#ifdef __cplusplus
}
#endif

#endif // !_POSIX_C_SOURCE && !_POSIX_TIMERS

#if !(__STDC_VERSION__ >= 201112L) && !defined(__USE_ISOC11)

#ifndef TIME_UTC
//! An integer constant greater than 0 that designates the UTC time base.
#define TIME_UTC	1
#endif

#ifdef __cplusplus
extern "C" {
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

#ifdef __cplusplus
}
#endif

#endif // !(__STDC_VERSION__ >= 201112L) && !__USE_ISOC11

#endif

