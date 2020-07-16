/**@file
 * This header file is part of the utilities library; it contains the time
 * function declarations.
 *
 * @copyright 2013-2020 Lely Industries N.V.
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

#ifndef LELY_UTIL_TIME_H_
#define LELY_UTIL_TIME_H_

#include <lely/libc/time.h>

#include <assert.h>
#include <stdint.h>

#ifndef LELY_UTIL_TIME_INLINE
#define LELY_UTIL_TIME_INLINE static inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// Adds the time interval *<b>inc</b> to the time at <b>tp</b>.
LELY_UTIL_TIME_INLINE void timespec_add(
		struct timespec *tp, const struct timespec *inc);

/// Adds <b>sec</b> seconds to the time at <b>tp</b>.
LELY_UTIL_TIME_INLINE void timespec_add_sec(
		struct timespec *tp, uint_least64_t sec);

/// Adds <b>msec</b> milliseconds to the time at <b>tp</b>.
LELY_UTIL_TIME_INLINE void timespec_add_msec(
		struct timespec *tp, uint_least64_t msec);

/// Adds <b>usec</b> microseconds to the time at <b>tp</b>.
LELY_UTIL_TIME_INLINE void timespec_add_usec(
		struct timespec *tp, uint_least64_t usec);

/// Adds <b>nsec</b> nanoseconds to the time at <b>tp</b>.
LELY_UTIL_TIME_INLINE void timespec_add_nsec(
		struct timespec *tp, uint_least64_t nsec);

/// Subtracts the time interval *<b>dec</b> from the time at <b>tp</b>.
LELY_UTIL_TIME_INLINE void timespec_sub(
		struct timespec *tp, const struct timespec *dec);

/// Subtracts <b>sec</b> seconds from the time at <b>tp</b>.
LELY_UTIL_TIME_INLINE void timespec_sub_sec(
		struct timespec *tp, uint_least64_t sec);

/// Subtracts <b>msec</b> milliseconds from the time at <b>tp</b>.
LELY_UTIL_TIME_INLINE void timespec_sub_msec(
		struct timespec *tp, uint_least64_t msec);

/// Subtracts <b>usec</b> microseconds from the time at <b>tp</b>.
LELY_UTIL_TIME_INLINE void timespec_sub_usec(
		struct timespec *tp, uint_least64_t usec);

/// Subtracts <b>nsec</b> nanoseconds from the time at <b>tp</b>.
LELY_UTIL_TIME_INLINE void timespec_sub_nsec(
		struct timespec *tp, uint_least64_t nsec);

/**
 * Returns the time difference (in seconds) between *<b>t1</b> and *<b>t2</b>.
 * Difference is rounded to full seconds towards zero (with decimals truncated).
 */
LELY_UTIL_TIME_INLINE int_least64_t timespec_diff_sec(
		const struct timespec *t1, const struct timespec *t2);

/**
 * Returns the time difference (in milliseconds) between *<b>t1</b> and
 * *<b>t2</b>. Difference is rounded to full miliseconds towards zero
 * (with decimals truncated).
 */
LELY_UTIL_TIME_INLINE int_least64_t timespec_diff_msec(
		const struct timespec *t1, const struct timespec *t2);

/**
 * Returns the time difference (in microseconds) between *<b>t1</b> and
 * *<b>t2</b>. Difference is rounded to full microseconds towards zero
 * (with decimals truncated).
 */
LELY_UTIL_TIME_INLINE int_least64_t timespec_diff_usec(
		const struct timespec *t1, const struct timespec *t2);

/**
 * Returns the time difference (in nanoseconds) between *<b>t1</b> and
 * *<b>t2</b>.
 */
LELY_UTIL_TIME_INLINE int_least64_t timespec_diff_nsec(
		const struct timespec *t1, const struct timespec *t2);

/**
 * Compares two times.
 *
 * @returns an integer greater than, equal to, or less than 0 if the time stored
 * at <b>p1</b> is greater than, equal to, or less than the time stored at
 * <b>p2</b>.
 */
LELY_UTIL_TIME_INLINE int timespec_cmp(const void *p1, const void *p2);

LELY_UTIL_TIME_INLINE void
timespec_add(struct timespec *tp, const struct timespec *inc)
{
	assert(tp);
	assert(inc);

	tp->tv_sec += inc->tv_sec;
	tp->tv_nsec += inc->tv_nsec;

	if (tp->tv_nsec >= 1000000000l) {
		tp->tv_sec++;
		tp->tv_nsec -= 1000000000l;
	}
}

LELY_UTIL_TIME_INLINE void
timespec_add_sec(struct timespec *tp, uint_least64_t sec)
{
	assert(tp);

	tp->tv_sec += sec;
}

LELY_UTIL_TIME_INLINE void
timespec_add_msec(struct timespec *tp, uint_least64_t msec)
{
	struct timespec inc = { (time_t)(msec / 1000),
		(long)((msec % 1000) * 1000000l) };
	timespec_add(tp, &inc);
}

LELY_UTIL_TIME_INLINE void
timespec_add_usec(struct timespec *tp, uint_least64_t usec)
{
	struct timespec inc = { (time_t)(usec / 1000000l),
		(long)((usec % 1000000l) * 1000) };
	timespec_add(tp, &inc);
}

LELY_UTIL_TIME_INLINE void
timespec_add_nsec(struct timespec *tp, uint_least64_t nsec)
{
	struct timespec inc = { (time_t)(nsec / 1000000000l),
		(long)(nsec % 1000000000l) };
	timespec_add(tp, &inc);
}

LELY_UTIL_TIME_INLINE void
timespec_sub(struct timespec *tp, const struct timespec *dec)
{
	assert(tp);
	assert(dec);

	tp->tv_sec -= dec->tv_sec;
	tp->tv_nsec -= dec->tv_nsec;

	if (tp->tv_nsec < 0) {
		tp->tv_sec--;
		tp->tv_nsec += 1000000000l;
	}
}

LELY_UTIL_TIME_INLINE void
timespec_sub_sec(struct timespec *tp, uint_least64_t sec)
{
	assert(tp);

	tp->tv_sec -= sec;
}

LELY_UTIL_TIME_INLINE void
timespec_sub_msec(struct timespec *tp, uint_least64_t msec)
{
	struct timespec dec = { (time_t)(msec / 1000),
		(long)((msec % 1000) * 1000000l) };
	timespec_sub(tp, &dec);
}

LELY_UTIL_TIME_INLINE void
timespec_sub_usec(struct timespec *tp, uint_least64_t usec)
{
	struct timespec dec = { (time_t)(usec / 1000000l),
		(long)((usec % 1000000l) * 1000) };
	timespec_sub(tp, &dec);
}

LELY_UTIL_TIME_INLINE void
timespec_sub_nsec(struct timespec *tp, uint_least64_t nsec)
{
	struct timespec dec = { (time_t)(nsec / 1000000000l),
		(long)(nsec % 1000000000l) };
	timespec_sub(tp, &dec);
}

LELY_UTIL_TIME_INLINE int_least64_t
timespec_diff_sec(const struct timespec *t1, const struct timespec *t2)
{
	assert(t1);
	assert(t2);

	return t1->tv_sec - t2->tv_sec;
}

LELY_UTIL_TIME_INLINE int_least64_t
timespec_diff_msec(const struct timespec *t1, const struct timespec *t2)
{
	assert(t1);
	assert(t2);

	return (int_least64_t)(t1->tv_sec - t2->tv_sec) * 1000
			+ (t1->tv_nsec - t2->tv_nsec) / 1000000l;
}

LELY_UTIL_TIME_INLINE int_least64_t
timespec_diff_usec(const struct timespec *t1, const struct timespec *t2)
{
	assert(t1);
	assert(t2);

	return (int_least64_t)(t1->tv_sec - t2->tv_sec) * 1000000l
			+ (t1->tv_nsec - t2->tv_nsec) / 1000;
}

LELY_UTIL_TIME_INLINE int_least64_t
timespec_diff_nsec(const struct timespec *t1, const struct timespec *t2)
{
	assert(t1);
	assert(t2);

	return (int_least64_t)(t1->tv_sec - t2->tv_sec) * 1000000000l
			+ t1->tv_nsec - t2->tv_nsec;
}

LELY_UTIL_TIME_INLINE int
timespec_cmp(const void *p1, const void *p2)
{
	if (p1 == p2)
		return 0;

	if (!p1)
		return -1;
	if (!p2)
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

#endif // !LELY_UTIL_TIME_H_
