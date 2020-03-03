/**@file
 * This header file is part of the I/O library; it contains the timer queue
 * declarations.
 *
 * A timer queue allows multiple non-periodic wait operations with different
 * expiration times to use the same I/O timer. This is much more efficient than
 * creating a separate I/O timer for each concurrent wait.
 *
 * @copyright 2015-2020 Lely Industries N.V.
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

#ifndef LELY_IO2_TQUEUE_H_
#define LELY_IO2_TQUEUE_H_

#include <lely/io2/timer.h>
#include <lely/util/pheap.h>

/// A timer queue.
typedef struct io_tqueue io_tqueue_t;

/// A wait operation suitable for use with a timer queue.
struct io_tqueue_wait {
	/// The absolute expiration time.
	struct timespec value;
	/**
	 * The task (to be) submitted upon completion (or cancellation) of the
	 * wait operation.
	 */
	struct ev_task task;
	/**
	 * The error number, obtained as if by get_errc(), if an error occurred
	 * or the operation was canceled.
	 */
	int errc;
	struct pnode _node;
};

/// The static initializer for #io_tqueue_wait.
#define IO_TQUEUE_WAIT_INIT(sec, nsec, exec, func) \
	{ \
		{ (sec), (nsec) }, EV_TASK_INIT(exec, func), 0, PNODE_INIT \
	}

#ifdef __cplusplus
extern "C" {
#endif

void *io_tqueue_alloc(void);
void io_tqueue_free(void *ptr);
io_tqueue_t *io_tqueue_init(
		io_tqueue_t *tq, io_timer_t *timer, ev_exec_t *exec);
void io_tqueue_fini(io_tqueue_t *tq);

/**
 * Creates a new timer queue.
 *
 * @param timer a pointer to the I/O timer to be used for the queue. During the
 *              lifetime of the timer queue, io_timer_settime() MUST NOT be
 *              invoked.
 * @param exec  a pointer to the executor used to execute asynchronous tasks. If
 *              <b>exec</b> is NULL, the executor of the I/O timer is used.
 *
 * @returns a pointer to a new timer queue, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
io_tqueue_t *io_tqueue_create(io_timer_t *timer, ev_exec_t *exec);

/// Destroys a timer queue. @see io_tqueue_create()
void io_tqueue_destroy(io_tqueue_t *tq);

/// Returns a pointer to the abstract I/O device representing the timer queue.
io_dev_t *io_tqueue_get_dev(const io_tqueue_t *tq);

/// Returns a pointer to the I/O timer used by the timer queue.
io_timer_t *io_tqueue_get_timer(const io_tqueue_t *tq);

/**
 * Submits a wait operation to a timer queue. The completion task is submitted
 * for execution once the timeout specified in the <b>value</b> member of the
 * wait operation expires.
 */
void io_tqueue_submit_wait(io_tqueue_t *tq, struct io_tqueue_wait *wait);

/**
 * Cancels the specified timer queue wait operation if it is pending. The
 * completion task is submitted for execution with <b>errc</b> =
 * #errnum2c(#ERRNUM_CANCELED).
 *
 * @returns 1 if the operation was canceled, and 0 if it was not pending.
 *
 * @see io_dev_cancel()
 */
size_t io_tqueue_cancel_wait(io_tqueue_t *tq, struct io_tqueue_wait *wait);

/**
 * Aborts the specified timer queue wait operation if it is pending. If aborted,
 * the completion task is _not_ submitted for execution.
 *
 * @returns 1 if the operation was aborted, and 0 if it was not pending.
 *
 * @see io_dev_abort()
 */
size_t io_tqueue_abort_wait(io_tqueue_t *tq, struct io_tqueue_wait *wait);

/**
 * Submits an asynchronous wait operation to a timer queue and creates a future
 * which becomes ready once the wait operation completes (or is canceled). The
 * result of the future is an `int` containing the error number.
 *
 * @param tq    a pointer to a timer queue.
 * @param exec  a pointer to the executor used to execute the completion
 *              function of the wait operation. If NULL, the default executor of
 *              the timer queue is used.
 * @param value a pointer to the absolute expiration time.
 * @param pwait the address at which to store a pointer to the wait operation
 *              (can be NULL).
 *
 * @returns a pointer to a future, or NULL on error. In the latter case, the
 * error number can be obtained with get_errc().
 */
ev_future_t *io_tqueue_async_wait(io_tqueue_t *tq, ev_exec_t *exec,
		const struct timespec *value, struct io_tqueue_wait **pwait);

/**
 * Obtains a pointer to a timer queue wait operation from a pointer to its
 * completion task.
 */
struct io_tqueue_wait *io_tqueue_wait_from_task(struct ev_task *task);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_TQUEUE_H_
