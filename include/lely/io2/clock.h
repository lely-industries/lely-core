/**@file
 * This header file is part of the I/O library; it contains the abstract clock
 * interface.
 *
 * The clock interface is modeled after the POSIX clock_getres(),
 * clock_gettime() and clock_settime() functions.
 *
 * @copyright 2018-2019 Lely Industries N.V.
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

#ifndef LELY_IO2_CLOCK_H_
#define LELY_IO2_CLOCK_H_

#include <lely/io2/io2.h>
#include <lely/libc/time.h>

#ifndef LELY_IO_CLOCK_INLINE
#define LELY_IO_CLOCK_INLINE static inline
#endif

/// An abstract clock.
typedef const struct io_clock_vtbl *const io_clock_t;

#ifdef __cplusplus
extern "C" {
#endif

struct io_clock_vtbl {
	int (*getres)(const io_clock_t *clock, struct timespec *res);
	int (*gettime)(const io_clock_t *clock, struct timespec *tp);
	int (*settime)(io_clock_t *clock, const struct timespec *tp);
};

/**
 * Obtains the resolution of the specified clock. Note that the resolution MAY
 * NOT be constant for user-defined clocks (i.e., it MAY be the interval between
 * the last two clock updates).
 *
 * @param clock a pointer to a clock.
 * @param res   the address at which to store the resolution (can be NULL).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
LELY_IO_CLOCK_INLINE int io_clock_getres(
		const io_clock_t *clock, struct timespec *res);

/**
 * Obtains the current time value of the specified clock.
 *
 * @param clock a pointer to a clock.
 * @param tp    the address at which to store the time value.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
LELY_IO_CLOCK_INLINE int io_clock_gettime(
		const io_clock_t *clock, struct timespec *tp);

/**
 * Sets the time value of the specified clock. This operation MAY require
 * elevated privileges in the calling process.
 *
 * @param clock a pointer to a clock.
 * @param tp    a pointer to the time value. This value MAY be rounded to the
 *              nearest multiple of the clock resolution given by
 *              io_clock_getres().
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
LELY_IO_CLOCK_INLINE int io_clock_settime(
		io_clock_t *clock, const struct timespec *tp);

inline int
io_clock_getres(const io_clock_t *clock, struct timespec *res)
{
	return (*clock)->getres(clock, res);
}

inline int
io_clock_gettime(const io_clock_t *clock, struct timespec *tp)
{
	return (*clock)->gettime(clock, tp);
}

inline int
io_clock_settime(io_clock_t *clock, const struct timespec *tp)
{
	return (*clock)->settime(clock, tp);
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_CLOCK_H_
