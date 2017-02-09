/*!\file
 * This is the internal header file of the time functions.
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

#ifndef LELY_LIBC_INTERN_TIMESPEC_H
#define LELY_LIBC_INTERN_TIMESPEC_H

#include "libc.h"
#include <lely/libc/time.h>

#include <errno.h>

#ifdef _WIN32
#ifdef _USE_32BIT_TIME_T
//! The lower limit for a value of type `time_t`.
#define TIME_T_MIN	LONG_MIN
//! The upper limit for a value of type `time_t`.
#define TIME_T_MAX	LONG_MAX
#else
//! The lower limit for a value of type `time_t`.
#define TIME_T_MIN	_I64_MIN
//! The upper limit for a value of type `time_t`.
#define TIME_T_MAX	_I64_MAX
#endif
/*!
 * The difference between Windows file time (seconds since 00:00:00 UTC on
 * January 1, 1601) and the Unix epoch (seconds since 00:00:00 UTC on January 1,
 * 1970) is 369 years and 89 leap days.
 */
#define UNIX_FILETIME	((LONGLONG)(369 * 365 + 89) * 24 * 60 * 60)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

//! Converts Windows file time to the Unix epoch.
static inline int ft2tp(const FILETIME *ft, struct timespec *tp);

//! Converts the Unix epoch to Windows file time.
static inline int tp2ft(const struct timespec *tp, FILETIME *ft);

#endif

//! Adds the time interval *\a inc to the time at \a tp.
static inline void timespec_add(struct timespec *tp,
		const struct timespec *inc);

//! Subtracts the time interval *\a dec from the time at \a tp.
static inline void timespec_sub(struct timespec *tp,
		const struct timespec *dec);

#ifdef _WIN32

static inline int
ft2tp(const FILETIME *ft, struct timespec *tp)
{
	ULARGE_INTEGER li = { { ft->dwLowDateTime, ft->dwHighDateTime } };
	LONGLONG sec = li.QuadPart / 10000000ul - UNIX_FILETIME;
	if (__unlikely(sec < TIME_T_MIN || sec > TIME_T_MAX)) {
		errno = EOVERFLOW;
		return -1;
	}
	tp->tv_sec = sec;
	tp->tv_nsec = (li.QuadPart % 10000000ul) * 100;

	return 0;
}

static inline int
tp2ft(const struct timespec *tp, FILETIME *ft)
{
	if (__unlikely(tp->tv_sec + UNIX_FILETIME < 0))
		goto error;
	ULARGE_INTEGER li = { .QuadPart = tp->tv_sec + UNIX_FILETIME };
	if (__unlikely(li.QuadPart > _UI64_MAX / 10000000ul))
		goto error;
	li.QuadPart *= 10000000ul;
	if (__unlikely(li.QuadPart > _UI64_MAX - tp->tv_nsec / 100))
		goto error;
	li.QuadPart += tp->tv_nsec / 100;
	ft->dwLowDateTime = li.LowPart;
	ft->dwHighDateTime = li.HighPart;

	return 0;

error:
	errno = EINVAL;
	return -1;
}

#endif // _WIN32

static inline void
timespec_add(struct timespec *tp, const struct timespec *inc)
{
	tp->tv_sec += inc->tv_sec;
	tp->tv_nsec += inc->tv_nsec;
	if (tp->tv_nsec >= 1000000000l) {
		tp->tv_sec++;
		tp->tv_nsec -= 1000000000l;
	}
}

static inline void
timespec_sub(struct timespec *tp, const struct timespec *dec)
{
	tp->tv_sec -= dec->tv_sec;
	tp->tv_nsec -= dec->tv_nsec;
	if (tp->tv_nsec < 0) {
		tp->tv_sec--;
		tp->tv_nsec += 1000000000l;
	}
}

#ifdef __cplusplus
}
#endif

#endif

