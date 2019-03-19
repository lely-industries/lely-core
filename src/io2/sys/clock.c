/**@file
 * This file is part of the I/O library; it contains the standard system clock
 * implementation.
 *
 * @see lely/io2/sys/clock.h
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

#include "../io2.h"
#include <lely/io2/sys/clock.h>

#ifdef CLOCK_REALTIME

static int io_clock_realtime_getres(
		const io_clock_t *clock, struct timespec *res);
static int io_clock_realtime_gettime(
		const io_clock_t *clock, struct timespec *tp);
static int io_clock_realtime_settime(
		io_clock_t *clock, const struct timespec *tp);

// clang-format off
static const struct io_clock_vtbl io_clock_realtime_vtbl = {
	&io_clock_realtime_getres,
	&io_clock_realtime_gettime,
	&io_clock_realtime_settime
};
// clang-format on

const struct io_clock_realtime io_clock_realtime = { &io_clock_realtime_vtbl };

#endif // CLOCK_REALTIME

#ifdef CLOCK_MONOTONIC

static int io_clock_monotonic_getres(
		const io_clock_t *clock, struct timespec *res);
static int io_clock_monotonic_gettime(
		const io_clock_t *clock, struct timespec *tp);
static int io_clock_monotonic_settime(
		io_clock_t *clock, const struct timespec *tp);

// clang-format off
static const struct io_clock_vtbl io_clock_monotonic_vtbl = {
	&io_clock_monotonic_getres,
	&io_clock_monotonic_gettime,
	&io_clock_monotonic_settime
};
// clang-format on

const struct io_clock_monotonic io_clock_monotonic = {
	&io_clock_monotonic_vtbl
};

#endif // CLOCK_MONOTONIC

#ifdef CLOCK_REALTIME

static int
io_clock_realtime_getres(const io_clock_t *clock, struct timespec *res)
{
	(void)clock;

	return clock_getres(CLOCK_REALTIME, res);
}

static int
io_clock_realtime_gettime(const io_clock_t *clock, struct timespec *tp)
{
	(void)clock;

	return clock_gettime(CLOCK_REALTIME, tp);
}

static int
io_clock_realtime_settime(io_clock_t *clock, const struct timespec *tp)
{
	(void)clock;

	return clock_settime(CLOCK_REALTIME, tp);
}

#endif // CLOCK_REALTIME

#ifdef CLOCK_MONOTONIC

static int
io_clock_monotonic_getres(const io_clock_t *clock, struct timespec *res)
{
	(void)clock;

	return clock_getres(CLOCK_MONOTONIC, res);
}

static int
io_clock_monotonic_gettime(const io_clock_t *clock, struct timespec *tp)
{
	(void)clock;

	return clock_gettime(CLOCK_MONOTONIC, tp);
}

static int
io_clock_monotonic_settime(io_clock_t *clock, const struct timespec *tp)
{
	(void)clock;

	return clock_settime(CLOCK_MONOTONIC, tp);
}

#endif // CLOCK_MONOTONIC
