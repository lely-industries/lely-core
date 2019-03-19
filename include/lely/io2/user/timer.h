/**@file
 * This header file is part of the I/O library; it contains the user-defined
 * timer declarations.
 *
 * The user-defined timer is a passive timer; it does not actively monitor a
 * system clock, but provides a clock which requires the user to periodically
 * update the time with io_clock_settime(). These updates then trigger timer
 * expirations. The timer can optionally notify the user of expiration time
 * updates. These updates can be used by an application to determine when to
 * invoke io_clock_settime().
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

#ifndef LELY_IO2_USER_TIMER_H_
#define LELY_IO2_USER_TIMER_H_

#include <lely/io2/timer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of function invoked by a user-defined timer when the expiration time
 * is updated with io_timer_settime() or when a periodic timer expires.
 *
 * @param tp  a pointer to the next expiration time.
 * @param arg the user-specific value submitted to io_user_timer_create().
 */
typedef void io_user_timer_setnext_t(const struct timespec *tp, void *arg);

void *io_user_timer_alloc(void);
void io_user_timer_free(void *ptr);
io_timer_t *io_user_timer_init(io_timer_t *timer, io_ctx_t *ctx,
		ev_exec_t *exec, io_user_timer_setnext_t *func, void *arg);
void io_user_timer_fini(io_timer_t *timer);

/**
 * Creates a new user-defined timer.
 *
 * @param ctx  a pointer to the I/O context with which the timer should be
 *             registered.
 * @param exec a pointer to the executor used to execute asynchronous tasks.
 * @param func a pointer to the function to be invoked when the expiration time
 *             is updated (can be NULL).
 * @param arg  the user-specific value to be passed as the second argument to
 *             <b>func</b>.
 *
 * @returns a pointer to a new timer, or NULL on error. In the latter case, the
 * error number can be obtained with get_errc().
 */
io_timer_t *io_user_timer_create(io_ctx_t *ctx, ev_exec_t *exec,
		io_user_timer_setnext_t *func, void *arg);

/// Destroys a user-defined timer. @see io_user_timer_create()
void io_user_timer_destroy(io_timer_t *timer);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_USER_TIMER_H_
