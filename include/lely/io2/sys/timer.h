/**@file
 * This header file is part of the I/O library; it contains the I/O system timer
 * declarations.
 *
 * @copyright 2015-2019 Lely Industries N.V.
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

#ifndef LELY_IO2_SYS_TIMER_H_
#define LELY_IO2_SYS_TIMER_H_

#include <lely/io2/sys/io.h>
#include <lely/io2/timer.h>

#ifdef __cplusplus
extern "C" {
#endif

void *io_timer_alloc(void);
void io_timer_free(void *ptr);
io_timer_t *io_timer_init(io_timer_t *timer, io_poll_t *poll, ev_exec_t *exec,
		clockid_t clockid);
void io_timer_fini(io_timer_t *timer);

/**
 * Creates a new I/O system timer.
 *
 * @param poll    a pointer to the I/O polling instance used to monitor timer
 *                events.
 * @param exec    a pointer to the executor used to execute asynchronous tasks.
 * @param clockid the POSIX identifier of the clock to use (`CLOCK_REALTIME` or
 *                `CLOCK_MONOTONIC`).
 *
 * @returns a pointer to a new timer, or NULL on error. In the latter case, the
 * error number can be obtained with get_errc().
 */
io_timer_t *io_timer_create(
		io_poll_t *poll, ev_exec_t *exec, clockid_t clockid);

/// Destroys an I/O system timer. @see io_timer_create()
void io_timer_destroy(io_timer_t *timer);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_SYS_TIMER_H_
