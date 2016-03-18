/*!\file
 * This file is part of the C11 and POSIX compatibility library.
 *
 * \see lely/libc/time.h
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

#include "libc.h"
#ifdef _WIN32
#include <lely/libc/stdint.h>
#endif
#include <lely/libc/time.h>

#ifdef _WIN32

LELY_LIBC_EXPORT int __cdecl
nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
	int64_t msec = rqtp->tv_sec * 1000 + rqtp->tv_nsec / 1000000;
	if (__likely(msec > 0))
		Sleep(msec);
	// Since Sleep() cannot be interrupted, there is no remaining time.
	if (rmtp)
		*rmtp = (struct timespec){ 0, 0 };
	return 0;
}

#endif

#if !(__STDC_VERSION__ >= 201112L) && !defined(__USE_ISOC11)

#ifdef _WIN32

LELY_LIBC_EXPORT int __cdecl
timespec_get(struct timespec *ts, int base)
{
	if (__unlikely(base != TIME_UTC))
		return 0;

	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	uint64_t nsec = 100 * ((uint64_t)ft.dwHighDateTime << 32
			| (uint64_t)ft.dwLowDateTime);
	ts->tv_sec = nsec / 1000000000l;
	ts->tv_nsec = nsec % 1000000000l;
	// Convert file time (seconds since 00:00:00 UTC on January 1, 1601), to
	// the Unix epoch (seconds since 00:00:00 UTC on January 1, 1970). This
	// is a difference of 369 years and 89 leap days.
	ts->tv_sec -= (int64_t)(369 * 365 + 89) * 24 * 60 * 60;

	return base;
}

#elif defined(_POSIX_TIMERS)

LELY_LIBC_EXPORT int __cdecl
timespec_get(struct timespec *ts, int base)
{
	if (__unlikely(base != TIME_UTC))
		return 0;
	if (__unlikely(clock_gettime(CLOCK_REALTIME, ts) == -1))
		return 0;
	return base;
}

#endif

#endif // !(__STDC_VERSION__ >= 201112L) && !defined(__USE_ISOC11)

