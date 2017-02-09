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

#include "timespec.h"

#if defined(_WIN32) && !defined(__MINGW32__)

LELY_LIBC_EXPORT int __cdecl
clock_getres(clockid_t clock_id, struct timespec *res)
{
	switch (clock_id) {
	case CLOCK_REALTIME: {
		if (res)
			*res = (struct timespec){ 0, 100 };
		return 0;
	}
	case CLOCK_MONOTONIC: {
		if (res) {
			// On Windows XP or later, this will always succeed.
			LARGE_INTEGER Frequency;
			QueryPerformanceFrequency(&Frequency);
			*res = (struct timespec){
				0,
				(long)(1000000000l / Frequency.QuadPart)
			};
		}
		return 0;
	}
	default:
		errno = EINVAL;
		return -1;
	}
}

LELY_LIBC_EXPORT int __cdecl
clock_gettime(clockid_t clock_id, struct timespec *tp)
{
	switch (clock_id) {
	case CLOCK_REALTIME: {
		if (tp) {
			FILETIME ft;
#if defined(NTDDI_WIN8) && NTDDI_VERSION >= NTDDI_WIN8
			GetSystemTimePreciseAsFileTime(&ft);
#else
			GetSystemTimeAsFileTime(&ft);
#endif
			if (__unlikely(ft2tp(&ft, tp) == -1))
				return -1;
		}
		return 0;
	}
	case CLOCK_MONOTONIC: {
		if (tp) {
			// On Windows XP or later, this will always succeed.
			LARGE_INTEGER Frequency;
			QueryPerformanceFrequency(&Frequency);
			LARGE_INTEGER PerformanceCount;
			QueryPerformanceCounter(&PerformanceCount);
			if (__unlikely(PerformanceCount.QuadPart
					/ Frequency.QuadPart > TIME_T_MAX)) {
				errno = EOVERFLOW;
				return -1;
			}
			tp->tv_sec = PerformanceCount.QuadPart
					/ Frequency.QuadPart;
			tp->tv_nsec = (long)((PerformanceCount.QuadPart
					% Frequency.QuadPart)
					* 1000000000l / Frequency.QuadPart);
		}
		return 0;
	}
	default:
		errno = EINVAL;
		return -1;
	}
}

LELY_LIBC_EXPORT int __cdecl
clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *rqtp,
		struct timespec *rmtp)
{
	switch (clock_id) {
	case CLOCK_REALTIME:
	case CLOCK_MONOTONIC:
		break;
	case CLOCK_PROCESS_CPUTIME_ID:
		return ENOTSUP;
	default:
		return EINVAL;
	}

	if (__unlikely(rqtp->tv_nsec < 0 || rqtp->tv_nsec >= 1000000000l))
		return EINVAL;

	int errsv = errno;

	struct timespec before = { 0, 0 };
	if (__unlikely(clock_gettime(clock_id, &before) == -1)) {
		int result = errno;
		errno = errsv;
		return result;
	}

	struct timespec tp = *rqtp;
	if (flags & TIMER_ABSTIME)
		timespec_sub(&tp, &before);
	if (tp.tv_sec < 0)
		return 0;

	LONGLONG Milliseconds = (LONGLONG)tp.tv_sec * 1000
			+ (tp.tv_nsec + 999999l) / 1000000l;
	if (__unlikely(Milliseconds > ULONG_MAX))
		return EINVAL;

	if (SleepEx((DWORD)Milliseconds, TRUE)) {
		if (!(flags & TIMER_ABSTIME) && rmtp) {
			struct timespec after = { 0, 0 };
			if (__unlikely(clock_gettime(clock_id, &after) == -1)) {
				int result = errno;
				errno = errsv;
				return result;
			}
			timespec_sub(&after, &before);
			*rmtp = tp;
			timespec_sub(rmtp, &after);
		}
		return EINTR;
	}

	return 0;
}

LELY_LIBC_EXPORT int __cdecl
clock_settime(clockid_t clock_id, const struct timespec *tp)
{
	if (__unlikely(clock_id != CLOCK_REALTIME)) {
		errno = EINVAL;
		return -1;
	}

	if (__unlikely(tp->tv_nsec < 0 || tp->tv_nsec >= 1000000000l)) {
		errno = EINVAL;
		return -1;
	}

	FILETIME ft;
	if (__unlikely(tp2ft(tp, &ft) == -1))
		return -1;
	SYSTEMTIME st;
	if (__unlikely(!FileTimeToSystemTime(&ft, &st))) {
		errno = EINVAL;
		return -1;
	}
	if (__unlikely(!SetSystemTime(&st))) {
		errno = EPERM;
		return -1;
	}

	return 0;
}

#endif // _WIN32 && !__MINGW32__

#endif // !LELY_NO_RT

