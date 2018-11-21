/**@file
 * This file is part of the C11 and POSIX compatibility library.
 *
 * @see lely/libc/time.h
 *
 * @copyright 2014-2018 Lely Industries N.V.
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

#include "libc.h"

#if !LELY_NO_RT

#if _WIN32 && !defined(__MINGW32__)

#include <lely/libc/time.h>

#include <errno.h>

#include <windows.h>

/**
 * The difference between Windows file time (seconds since 00:00:00 UTC on
 * January 1, 1601) and the Unix epoch (seconds since 00:00:00 UTC on January 1,
 * 1970) is 369 years and 89 leap days.
 */
#define FILETIME_EPOCH ((LONGLONG)(369 * 365 + 89) * 24 * 60 * 60)

#ifdef _USE_32BIT_TIME_T
/// The lower limit for a value of type `time_t`.
#define TIME_T_MIN LONG_MIN
/// The upper limit for a value of type `time_t`.
#define TIME_T_MAX LONG_MAX
#else
/// The lower limit for a value of type `time_t`.
#define TIME_T_MIN _I64_MIN
/// The upper limit for a value of type `time_t`.
#define TIME_T_MAX _I64_MAX
#endif

int
clock_getres(clockid_t clock_id, struct timespec *res)
{
	if (clock_id == CLOCK_MONOTONIC) {
		// On Windows XP or later, this will always succeed.
		LARGE_INTEGER liFrequency;
		QueryPerformanceFrequency(&liFrequency);
		LONGLONG pf = liFrequency.QuadPart;

		if (res)
			*res = (struct timespec){ 0,
				(long)((1000000000l + pf / 2) / pf) };

		return 0;
	}

	switch (clock_id) {
	case CLOCK_REALTIME:
	case CLOCK_PROCESS_CPUTIME_ID:
	case CLOCK_THREAD_CPUTIME_ID: break;
	default: errno = EINVAL; return -1;
	}

	DWORD dwTimeAdjustment;
	DWORD dwTimeIncrement;
	BOOL bTimeAdjustmentDisabled;
	// clang-format off
	if (!GetSystemTimeAdjustment(&dwTimeAdjustment, &dwTimeIncrement,
			&bTimeAdjustmentDisabled))
		// clang-format on
		return -1;

	if (res)
		*res = (struct timespec){ 0, dwTimeIncrement * 100 };

	return 0;
}

int
clock_gettime(clockid_t clock_id, struct timespec *tp)
{
	if (clock_id == CLOCK_MONOTONIC) {
		// On Windows XP or later, this will always succeed.
		LARGE_INTEGER liFrequency;
		QueryPerformanceFrequency(&liFrequency);
		LONGLONG pf = liFrequency.QuadPart;
		LARGE_INTEGER liPerformanceCount;
		QueryPerformanceCounter(&liPerformanceCount);
		LONGLONG pc = liPerformanceCount.QuadPart;
		if (pc / pf > TIME_T_MAX) {
			errno = EOVERFLOW;
			return -1;
		}

		if (tp) {
			tp->tv_sec = pc / pf;
			tp->tv_nsec = (long)(((pc % pf) * (1000000000l + pf / 2))
					/ pf);
		}

		return 0;
	}

	LONGLONG ft;
	switch (clock_id) {
	case CLOCK_REALTIME: {
		FILETIME SystemTimeAsFileTime;
#if defined(NTDDI_WIN8) && NTDDI_VERSION >= NTDDI_WIN8
		GetSystemTimePreciseAsFileTime(&SystemTimeAsFileTime);
#else
		GetSystemTimeAsFileTime(&SystemTimeAsFileTime);
#endif
		ULARGE_INTEGER st = {
			.LowPart = SystemTimeAsFileTime.dwLowDateTime,
			.HighPart = SystemTimeAsFileTime.dwHighDateTime
		};
		ft = st.QuadPart - (ULONGLONG)FILETIME_EPOCH * 10000000ul;
		break;
	}
	case CLOCK_PROCESS_CPUTIME_ID: {
		FILETIME CreationTime, ExitTime, KernelTime, UserTime;
		// clang-format off
		if (!GetProcessTimes(GetCurrentProcess(), &CreationTime,
				&ExitTime, &KernelTime, &UserTime))
			// clang-format on
			return -1;
		// Add the time spent in kernel mode and the time spent in user
		// mode to obtain the total process time.
		ULARGE_INTEGER kt = { .LowPart = KernelTime.dwLowDateTime,
			.HighPart = KernelTime.dwHighDateTime };
		ULARGE_INTEGER ut = { .LowPart = UserTime.dwLowDateTime,
			.HighPart = UserTime.dwHighDateTime };
		if (kt.QuadPart + ut.QuadPart > _I64_MAX) {
			errno = EOVERFLOW;
			return -1;
		}
		ft = kt.QuadPart + ut.QuadPart;
		break;
	}
	case CLOCK_THREAD_CPUTIME_ID: {
		FILETIME CreationTime, ExitTime, KernelTime, UserTime;
		// clang-format off
		if (!GetProcessTimes(GetCurrentThread(), &CreationTime,
				&ExitTime, &KernelTime, &UserTime))
			// clang-format on
			return -1;
		// Add the time spent in kernel mode and the time spent in user
		// mode to obtain the total thread time.
		ULARGE_INTEGER kt = { .LowPart = KernelTime.dwLowDateTime,
			.HighPart = KernelTime.dwHighDateTime };
		ULARGE_INTEGER ut = { .LowPart = UserTime.dwLowDateTime,
			.HighPart = UserTime.dwHighDateTime };
		if (kt.QuadPart + ut.QuadPart > _I64_MAX) {
			errno = EOVERFLOW;
			return -1;
		}
		ft = kt.QuadPart + ut.QuadPart;
		break;
	}
	default: errno = EINVAL; return -1;
	}

	LONGLONG sec = ft / 10000000l;
	if (sec < TIME_T_MIN || sec > TIME_T_MAX) {
		errno = EOVERFLOW;
		return -1;
	}

	if (tp) {
		tp->tv_sec = sec;
		tp->tv_nsec = (ft % 10000000l) * 100;
	}

	return 0;
}

int
clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *rqtp,
		struct timespec *rmtp)
{
	switch (clock_id) {
	case CLOCK_REALTIME:
	case CLOCK_MONOTONIC: break;
	case CLOCK_PROCESS_CPUTIME_ID: return ENOTSUP;
	case CLOCK_THREAD_CPUTIME_ID:
	default: return EINVAL;
	}

	if (rqtp->tv_nsec < 0 || rqtp->tv_nsec >= 1000000000l)
		return EINVAL;

	int errsv = errno;

	struct timespec now = { 0, 0 };
	if (clock_gettime(clock_id, &now) == -1) {
		int result = errno;
		errno = errsv;
		return result;
	}

	// Compute the absolute timeout while avoiding integer overflow.
	struct timespec tp = *rqtp;
	if (!(flags & TIMER_ABSTIME)) {
		if (tp.tv_sec < 0 || (!tp.tv_sec && !tp.tv_nsec))
			return 0;
		if (now.tv_sec > TIME_T_MAX - tp.tv_sec)
			return EINVAL;
		tp.tv_sec += now.tv_sec;
		tp.tv_nsec += now.tv_nsec;
		if (tp.tv_nsec >= 1000000000l) {
			if (tp.tv_sec == TIME_T_MAX)
				return EINVAL;
			tp.tv_sec++;
			tp.tv_nsec -= 1000000000l;
		}
	}

	// clang-format off
	while (now.tv_sec < tp.tv_sec || (now.tv_sec == tp.tv_sec
			&& now.tv_nsec < tp.tv_nsec)) {
		// clang-format on
		// Round up to the nearest number of milliseconds, to make sure
		// we don't wake up too early.
		LONGLONG llMilliseconds =
				(LONGLONG)(tp.tv_sec - now.tv_sec) * 1000
				+ (tp.tv_nsec - now.tv_nsec + 999999l)
						/ 1000000l;
		DWORD dwMilliseconds = llMilliseconds <= MAX_SLEEP_MS
				? (DWORD)llMilliseconds
				: MAX_SLEEP_MS;
		DWORD dwResult = SleepEx(dwMilliseconds, TRUE);
		if (clock_gettime(clock_id, &now) == -1) {
			int result = errno;
			errno = errsv;
			return result;
		}
		if (dwResult) {
			// In case of an interrupted relative sleep, compute the
			// remaining time.
			if (!(flags & TIMER_ABSTIME) && rmtp) {
				rmtp->tv_sec = tp.tv_sec - now.tv_sec;
				rmtp->tv_nsec = tp.tv_nsec - now.tv_nsec;
				if (rmtp->tv_nsec < 0) {
					rmtp->tv_sec--;
					rmtp->tv_nsec += 1000000000l;
				}
			}
			return EINTR;
		}
	}

	return 0;
}

int
clock_settime(clockid_t clock_id, const struct timespec *tp)
{
	if (clock_id != CLOCK_REALTIME) {
		errno = EINVAL;
		return -1;
	}

	if (tp->tv_nsec < 0 || tp->tv_nsec >= 1000000000l) {
		errno = EINVAL;
		return -1;
	}

	if (tp->tv_sec + FILETIME_EPOCH < 0) {
		errno = EINVAL;
		return -1;
	}
	ULARGE_INTEGER li = { .QuadPart = tp->tv_sec + FILETIME_EPOCH };
	if (li.QuadPart > _UI64_MAX / 10000000ul) {
		errno = EINVAL;
		return -1;
	}
	li.QuadPart *= 10000000ul;
	if (li.QuadPart > _UI64_MAX - tp->tv_nsec / 100) {
		errno = EINVAL;
		return -1;
	}
	li.QuadPart += tp->tv_nsec / 100;

	FILETIME ft = { li.LowPart, li.HighPart };
	SYSTEMTIME st;
	if (!FileTimeToSystemTime(&ft, &st)) {
		errno = EINVAL;
		return -1;
	}
	if (!SetSystemTime(&st)) {
		errno = EPERM;
		return -1;
	}

	return 0;
}

#endif // _WIN32 && !__MINGW32__

#endif // !LELY_NO_RT
