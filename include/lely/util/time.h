/*!\file
 * This header file is part of the utilities library; it contains the time
 * functions.
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

#ifndef LELY_UTIL_TIME_H
#define LELY_UTIL_TIME_H

#include <lely/libc/time.h>
#include <lely/util/util.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Adds the time interval *\a inc to the time at \a tp.
static inline void timespec_add(struct timespec *tp,
		const struct timespec *inc);

//! Adds \a sec seconds to the time at \a tp.
static inline void timespec_add_sec(struct timespec *tp, int64_t sec);

//! Adds \a msec milliseconds to the time at \a tp.
static inline void timespec_add_msec(struct timespec *tp, int64_t msec);

//! Adds \a usec microseconds to the time at \a tp.
static inline void timespec_add_usec(struct timespec *tp, int64_t usec);

//! Adds \a nsec nanoseconds to the time at \a tp.
static inline void timespec_add_nsec(struct timespec *tp, int64_t nsec);

//! Returns the time difference (in seconds) between *\a t1 and *\a t2.
static inline int64_t timespec_diff_sec(const struct timespec *t1,
		const struct timespec *t2);

//! Returns the time difference (in milliseconds) between *\a t1 and *\a t2.
static inline int64_t timespec_diff_msec(const struct timespec *t1,
		const struct timespec *t2);

//! Returns the time difference (in microseconds) between *\a t1 and *\a t2.
static inline int64_t timespec_diff_usec(const struct timespec *t1,
		const struct timespec *t2);

//! Returns the time difference (in nanoseconds) between *\a t1 and *\a t2.
static inline int64_t timespec_diff_nsec(const struct timespec *t1,
		const struct timespec *t2);

/*!
 * Compares two times. This function has the signature of #cmp_t.
 *
 * \returns an integer greater than, equal to, or less than 0 if *\a p1 is
 * greater than, equal to, or less than *\a p2.
 */
static inline int __cdecl timespec_cmp(const void *p1, const void *p2);

static inline void
timespec_add(struct timespec *tp, const struct timespec *inc)
{
	tp->tv_sec += inc->tv_sec;
	tp->tv_nsec += inc->tv_nsec;

	if (tp->tv_nsec < 0) {
		tp->tv_sec--;
		tp->tv_nsec += 1000000000l;
	} else if (tp->tv_nsec >= 1000000000l) {
		tp->tv_sec++;
		tp->tv_nsec -= 1000000000l;
	}
}

static inline void
timespec_add_sec(struct timespec *tp, int64_t sec)
{
	tp->tv_sec += sec;
}

static inline void
timespec_add_msec(struct timespec *tp, int64_t msec)
{
	struct timespec inc = {
		(time_t)(msec / 1000),
		(long)((msec % 1000) * 1000000)
	};
	timespec_add(tp, &inc);
}

static inline void
timespec_add_usec(struct timespec *tp, int64_t usec)
{
	struct timespec inc = {
		(time_t)(usec / 1000000),
		(long)((usec % 1000000) * 1000)
	};
	timespec_add(tp, &inc);
}

static inline void
timespec_add_nsec(struct timespec *tp, int64_t nsec)
{
	struct timespec inc = {
		(time_t)(nsec / 1000000000l),
		(long)(nsec % 1000000000l)
	};
	timespec_add(tp, &inc);
}

static inline int64_t
timespec_diff_sec(const struct timespec *t1, const struct timespec *t2)
{
	return ((int64_t)t1->tv_sec - (int64_t)t2->tv_sec)
			+ ((int64_t)t1->tv_nsec - (int64_t)t2->tv_nsec)
			/ 1000000000l;
}

static inline int64_t
timespec_diff_msec(const struct timespec *t1, const struct timespec *t2)
{
	return ((int64_t)t1->tv_sec - (int64_t)t2->tv_sec) * 1000
			+ ((int64_t)t1->tv_nsec - (int64_t)t2->tv_nsec)
			/ 1000000;
}

static inline int64_t
timespec_diff_usec(const struct timespec *t1, const struct timespec *t2)
{
	return ((int64_t)t1->tv_sec - (int64_t)t2->tv_sec) * 1000000
			+ ((int64_t)t1->tv_nsec - (int64_t)t2->tv_nsec) / 1000;
}

static inline int64_t
timespec_diff_nsec(const struct timespec *t1, const struct timespec *t2)
{
	return ((int64_t)t1->tv_sec - (int64_t)t2->tv_sec) * 1000000000l
			+ ((int64_t)t1->tv_nsec - (int64_t)t2->tv_nsec);
}

static inline int __cdecl
timespec_cmp(const void *p1, const void *p2)
{
	if (p1 == p2)
		return 0;

	if (__unlikely(!p1))
		return -1;
	if (__unlikely(!p2))
		return 1;

	const struct timespec *t1 = (const struct timespec *)p1;
	const struct timespec *t2 = (const struct timespec *)p2;

	int cmp = (t2->tv_sec < t1->tv_sec) - (t1->tv_sec < t2->tv_sec);
	if (!cmp)
		cmp = (t2->tv_nsec < t1->tv_nsec) - (t1->tv_nsec < t2->tv_nsec);
	return cmp;
}

#ifdef __cplusplus
}
#endif

#endif

