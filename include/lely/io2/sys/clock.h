/**@file
 * This header file is part of the I/O library; it contains the standard system
 * clock definitions.
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

#ifndef LELY_IO2_SYS_CLOCK_H_
#define LELY_IO2_SYS_CLOCK_H_

#include <lely/io2/clock.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct io_clock_realtime {
	const struct io_clock_vtbl *vptr;
} io_clock_realtime;

/// The #io_clock_t pointer representing the POSIX realtime clock.
#define IO_CLOCK_REALTIME (&io_clock_realtime.vptr)

extern const struct io_clock_monotonic {
	const struct io_clock_vtbl *vptr;
} io_clock_monotonic;

/// The #io_clock_t pointer representing the POSIX monotonic clock.
#define IO_CLOCK_MONOTONIC (&io_clock_monotonic.vptr)

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_SYS_CLOCK_H_
