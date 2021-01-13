/**@file
 * This header file is part of the event library; it contains the polling event
 * loop declarations.
 *
 * The polling event loop is an event loop suitable for use with asynchronous
 * I/O operations. It is typically used in a thread pool to execute the tasks
 * submitted to it through its associated executor. If no tasks are pending, the
 * event loop can optionally poll for external events (through the abstract
 * #ev_poll_t polling interface), such as I/O completion or readiness
 * notifications.
 *
 * The event loop does not create its own threads. It depends on the user to
 * execute one of the run functions (ev_loop_wait(), ev_loop_wait_until(),
 * ev_loop_wait_one(), ev_loop_wait_one_until(), ev_loop_run(),
 * ev_loop_run_until(), ev_loop_run_one(), ev_loop_run_one_until(),
 * ev_loop_poll() and ev_loop_poll_one()). If not explicitly stopped, these
 * functions will execute pending tasks as long as the event loop has
 * outstanding work. In this context, outstanding work is defined as the sum of
 * all pending and currently executing tasks, plus the number of calls to
 * ev_exec_on_task_init(), minus the number of calls to ev_exec_on_task_fini().
 * If, at any time, the outstanding work falls to 0, the event loop is stopped
 * as if by ev_loop_stop().
 *
 * @copyright 2018-2020 Lely Industries N.V.
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

#ifndef LELY_EV_LOOP_H_
#define LELY_EV_LOOP_H_

#include <lely/ev/future.h>
#include <lely/ev/poll.h>
#include <lely/libc/time.h>

#include <stddef.h>

#ifndef LELY_EV_LOOP_INLINE
#define LELY_EV_LOOP_INLINE static inline
#endif

/// A polling event loop.
typedef struct ev_loop ev_loop_t;

#ifdef __cplusplus
extern "C" {
#endif

void *ev_loop_alloc(void);
void ev_loop_free(void *ptr);
ev_loop_t *ev_loop_init(
		ev_loop_t *loop, ev_poll_t *poll, size_t npoll, int poll_task);
void ev_loop_fini(ev_loop_t *loop);

/**
 * Creates a new polling event loop.
 *
 * @param poll      a pointer to the polling instance to be used to poll for
 *                  events. If <b>poll</b> is NULL, the event loop does not
 *                  poll.
 * @param npoll     the maximum number of threads allowed to poll (i.e., invoke
 *                  `ev_poll_wait(poll, ...)` concurrently. If <b>npoll</b> is
 *                  0, there is no limit. If <b>poll</b> is NULL, this parameter
 *                  is ignored. On platforms supporting I/O completion ports
 *                  (Windows) it is generally most efficient to allow all
 *                  threads to poll (i.e., `npoll == 0`), while on platforms
 *                  using the reactor pattern (POSIX) it is best to allow a
 *                  single polling thread (`npoll == 1`).
 * @param poll_task if <b>poll_task</b> is non-zero, a polling task is queued to
 *                  the event loop and requeued when executed. This can prevent
 *                  starvation, since the event loop otherwise only polls if no
 *                  tasks are pending, which may never happen in CPU-bound
 *                  situations. Under light loads, however, the extra polling
 *                  overhead may harm performance. If <b>poll</b> is NULL, this
 *                  parameter is ignored.
 *
 * @returns a pointer to the new event loop, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
ev_loop_t *ev_loop_create(ev_poll_t *poll, size_t npoll, int poll_task);

/// Destroys a polling event loop. @see ev_loop_create()
void ev_loop_destroy(ev_loop_t *loop);

/**
 * Returns a pointer to the polling instance used by the event loop, or NULL if
 * the loop does not poll.
 */
ev_poll_t *ev_loop_get_poll(const ev_loop_t *loop);

/// Returns a pointer to the executor corresponding to the event loop.
ev_exec_t *ev_loop_get_exec(const ev_loop_t *loop);

/**
 * Stops the event loop. Ongoing calls to ev_loop_run(), ev_loop_run_until(),
 * ev_loop_run_one() and ev_loop_run_one_until() will terminate and future calls
 * will return 0 immediately.
 *
 * If this function is invoked explicitly, the loop MAY still contain
 * outstanding work and it MAY NOT be safe to destroy submitted, but not
 * completed, tasks. Invoke ev_loop_poll(), preceded by ev_loop_restart() if
 * necessary, to ensure no outstanding work remains.
 *
 * @post ev_loop_stopped() returns 1.
 */
void ev_loop_stop(ev_loop_t *loop);

/// Returns 1 if the event loop is stopped, and 0 if not.
int ev_loop_stopped(const ev_loop_t *loop);

/// Restarts an event loop. @post ev_loop_stopped() returns 0.
void ev_loop_restart(ev_loop_t *loop);

/**
 * Equivalent to
 * ```{.c}
 * size_t n = 0;
 * while (ev_loop_wait_one(loop, future))
 *         n += n < SIZE_MAX;
 * return n;
 * ```
 */
size_t ev_loop_wait(ev_loop_t *loop, ev_future_t *future);

/**
 * Equivalent to
 * ```{.c}
 * size_t n = 0;
 * while (ev_loop_wait_one_until(loop, future, abs_time))
 *         n += n < SIZE_MAX;
 * return n;
 * ```
 */
size_t ev_loop_wait_until(ev_loop_t *loop, ev_future_t *future,
		const struct timespec *abs_time);

/**
 * If the event loop has pending tasks, runs a single task. Otherwise, blocks
 * while the loop has outstanding work or until the loop is stopped or the
 * specified future (if not NULL) becomes ready. If the loop has no outstanding
 * work, ev_loop_stop() is invoked. Note that more than one task MAY be executed
 * if the task function invokes ev_exec_dispatch().
 *
 * @returns 1 if a task was executed, and 0 otherwise.
 */
size_t ev_loop_wait_one(ev_loop_t *loop, ev_future_t *future);

/**
 * If the event loop has pending tasks, runs a single task. Otherwise, blocks
 * while the loop has outstanding work or until the loop is stopped or the
 * specified future (if not NULL) becomes ready or the absolute timeout expires.
 * If <b>abs_time</b> is NULL, this function will not block. If the loop has no
 * outstanding work, ev_loop_stop() is invoked. Note that more than one task MAY
 * be executed if the task function invokes ev_exec_dispatch().
 *
 * Note that <b>abs_time</b> MUST be specified relative to the C11 `TIME_UTC`
 * time base. Depending on the platform, this base MAY be subject to clock
 * changes while this function is executing.
 *
 * @returns 1 if a task was executed, and 0 otherwise. If no task was executed
 * because of a timeout, get_errnum() returns #ERRNUM_TIMEDOUT.
 */
size_t ev_loop_wait_one_until(ev_loop_t *loop, ev_future_t *future,
		const struct timespec *abs_time);

/// Equivalent to `ev_loop_wait(loop, NULL)`.
LELY_EV_LOOP_INLINE size_t ev_loop_run(ev_loop_t *loop);

/// Equivalent to `ev_loop_wait_until(loop, NULL, abs_time)`.
LELY_EV_LOOP_INLINE size_t ev_loop_run_until(
		ev_loop_t *loop, const struct timespec *abs_time);

/// Equivalent to `ev_loop_wait_one(loop, NULL)`.
LELY_EV_LOOP_INLINE size_t ev_loop_run_one(ev_loop_t *loop);

/// Equivalent to `ev_loop_wait_one_until(loop, NULL, abs_time)`.
LELY_EV_LOOP_INLINE size_t ev_loop_run_one_until(
		ev_loop_t *loop, const struct timespec *abs_time);

/// Equivalent to `ev_loop_run_until(loop, NULL)`.
LELY_EV_LOOP_INLINE size_t ev_loop_poll(ev_loop_t *loop);

/// Equivalent to `ev_loop_run_one_until(loop, NULL)`.
LELY_EV_LOOP_INLINE size_t ev_loop_poll_one(ev_loop_t *loop);

/**
 * Returns the identifier of the calling thread. This identifier can be used to
 * interrupt a call to one of the run funtions from another thread with
 * ev_loop_kill().
 */
void *ev_loop_self(void);

/**
 * Interrupts an event loop running on the specified thread. In case of a nested
 * event loop, this function only interrupts the inner loop.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error code can
 * be obtained with get_errc().
 */
int ev_loop_kill(ev_loop_t *loop, void *thr);

inline size_t
ev_loop_run(ev_loop_t *loop)
{
	return ev_loop_wait(loop, NULL);
}

inline size_t
ev_loop_run_until(ev_loop_t *loop, const struct timespec *abs_time)
{
	return ev_loop_wait_until(loop, NULL, abs_time);
}

inline size_t
ev_loop_run_one(ev_loop_t *loop)
{
	return ev_loop_wait_one(loop, NULL);
}

inline size_t
ev_loop_run_one_until(ev_loop_t *loop, const struct timespec *abs_time)
{
	return ev_loop_wait_one_until(loop, NULL, abs_time);
}

inline size_t
ev_loop_poll(ev_loop_t *loop)
{
	return ev_loop_run_until(loop, NULL);
}

inline size_t
ev_loop_poll_one(ev_loop_t *loop)
{
	return ev_loop_run_one_until(loop, NULL);
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_EV_LOOP_H_
