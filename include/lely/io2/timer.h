/**@file
 * This header file is part of the I/O library; it contains the abstract timer
 * interface.
 *
 * The timer interface is modeled after the POSIX timer_getoverrun(),
 * timer_gettime() and timer_settime() functions.
 *
 * @copyright 2014-2019 Lely Industries N.V.
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

#ifndef LELY_IO2_TIMER_H_
#define LELY_IO2_TIMER_H_

#include <lely/ev/future.h>
#include <lely/ev/task.h>
#include <lely/io2/clock.h>
#include <lely/io2/dev.h>

#ifndef LELY_IO_TIMER_INLINE
#define LELY_IO_TIMER_INLINE static inline
#endif

/// An abstract timer.
typedef const struct io_timer_vtbl *const io_timer_t;

/// The result of an I/O timer wait operation.
struct io_timer_wait_result {
	/**
	 * The result of the wait operation: the expiration overrun count (see
	 * io_timer_getoverrun()) on success, or -1 on error (or if the
	 * operation is canceled). In the latter case, the error number is
	 * stored in #errc.
	 */
	int result;
	/// The error number, obtained as if by get_errc(), if #result is -1.
	int errc;
};

/// A wait operation suitable for use with an I/O timer.
struct io_timer_wait {
	/**
	 * The task (to be) submitted upon completion (or cancellation) of the
	 * wait operation.
	 */
	struct ev_task task;
	/// The result of the wait operation.
	struct io_timer_wait_result r;
};

/// The static initializer for #io_timer_wait.
#define IO_TIMER_WAIT_INIT(exec, func) \
	{ \
		EV_TASK_INIT(exec, func), { 0, 0 } \
	}

#ifdef __cplusplus
extern "C" {
#endif

struct io_timer_vtbl {
	io_dev_t *(*get_dev)(const io_timer_t *timer);
	io_clock_t *(*get_clock)(const io_timer_t *timer);
	int (*getoverrun)(const io_timer_t *timer);
	int (*gettime)(const io_timer_t *timer, struct itimerspec *value);
	int (*settime)(io_timer_t *timer, int flags,
			const struct itimerspec *value,
			struct itimerspec *ovalue);
	void (*submit_wait)(io_timer_t *timer, struct io_timer_wait *wait);
};

/// @see io_dev_get_ctx()
static inline io_ctx_t *io_timer_get_ctx(const io_timer_t *timer);

/// @see io_dev_get_exec()
static inline ev_exec_t *io_timer_get_exec(const io_timer_t *timer);

/// @see io_dev_cancel()
static inline size_t io_timer_cancel(io_timer_t *timer, struct ev_task *task);

/// @see io_dev_abort()
static inline size_t io_timer_abort(io_timer_t *timer, struct ev_task *task);

/// Returns a pointer to the abstract I/O device representing the timer.
LELY_IO_TIMER_INLINE io_dev_t *io_timer_get_dev(const io_timer_t *timer);

/// Returns a pointer to the clock used by the timer.
LELY_IO_TIMER_INLINE io_clock_t *io_timer_get_clock(const io_timer_t *timer);

/**
 * Obtains the I/O timer expiration overrun count of the last successfully
 * processed expiration. When a periodic timer expires but the event is not
 * processed before the next expiration, a timer overrun occurs.
 *
 * @returns the overrun counter, or -1 on error. In the latter case, the error
 * number can be obtained with get_errc().
 */
LELY_IO_TIMER_INLINE int io_timer_getoverrun(const io_timer_t *timer);

/**
 * Obtains the amount of time until the specified I/O timer expires and the
 * reload value of the timer.
 *
 * @param timer a pointer to an I/O timer.
 * @param value the address at which to store the timer values. On success, the
 *              the <b>it_value</b> member shall contain the time interval until
 *              the next expiration, or zero if the timer is disarmed. The
 *              <b>it_interval</b> member shall contain the reload value last
 *              set by io_timer_settime().
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
LELY_IO_TIMER_INLINE int io_timer_gettime(
		const io_timer_t *timer, struct itimerspec *value);

/**
 * Arms or disarms an I/O timer.
 *
 * @param timer  a pointer to an I/O timer.
 * @param flags  if `TIMER_ABSTIME` is set in <b>flags</b>, the <b>it_value</b>
 *               member if *<b>value</b> contains the absolute time of the first
 *               expiration. If `TIMER_ABSTIME` is not set, the <b>it_value</b>
 *               member contains the time interval until the first expiration.
 * @param value  a pointer to a struct containing the period of the timer in the
 *               <b>it_interval</b> member and the expiration in the
 *               <b>it_value</b> member. If the <b>it_value</b> member is zero,
 *               the timer is disarmed. If the <b>it_interval</b> member is
 *               non-zero, a periodic (or repetitive) timer is specified. The
 *               period MAY be rounded up to the nearest multiple of the clock
 *               resolution.
 * @param ovalue if not NULL, the address at which to store the previous amount
 *               of time until the timer would have expired, together with the
 *               previous reload value, as if by _atomically_ calling
 *               io_timer_gettime() before this function.
 */
LELY_IO_TIMER_INLINE int io_timer_settime(io_timer_t *timer, int flags,
		const struct itimerspec *value, struct itimerspec *ovalue);

/**
 * Submits a wait operation to an I/O timer. The completion task is submitted
 * for execution once the timer expires.
 */
LELY_IO_TIMER_INLINE void io_timer_submit_wait(
		io_timer_t *timer, struct io_timer_wait *wait);

/**
 * Cancels the specified I/O timer wait operation if it is pending. The
 * completion task is submitted for execution with <b>result</b> = -1 and
 * <b>errc</b> = #errnum2c(#ERRNUM_CANCELED).
 *
 * @returns 1 if the operation was canceled, and 0 if it was not pending.
 *
 * @see io_dev_cancel()
 */
static inline size_t io_timer_cancel_wait(
		io_timer_t *timer, struct io_timer_wait *wait);

/**
 * Aborts the specified I/O timer wait operation if it is pending. If aborted,
 * the completion task is _not_ submitted for execution.
 *
 * @returns 1 if the operation was aborted, and 0 if it was not pending.
 *
 * @see io_dev_abort()
 */
static inline size_t io_timer_abort_wait(
		io_timer_t *timer, struct io_timer_wait *wait);

/**
 * Submits an asynchronous wait operation to an I/O timer and creates a future
 * which becomes ready once the wait operation completes (or is canceled). The
 * result of the future has type #io_timer_wait_result.
 *
 * @param timer a pointer to an I/O timer.
 * @param exec  a pointer to the executor used to execute the completion
 *              function of the wait operation. If NULL, the default executor of
 *              the I/O timer is used.
 * @param pwait the address at which to store a pointer to the wait operation
 *              (can be NULL).
 *
 * @returns a pointer to a future, or NULL on error. In the latter case, the
 * error number can be obtained with get_errc().
 */
ev_future_t *io_timer_async_wait(io_timer_t *timer, ev_exec_t *exec,
		struct io_timer_wait **pwait);

/**
 * Obtains a pointer to an I/O timer wait operation from a pointer to its
 * completion task.
 */
struct io_timer_wait *io_timer_wait_from_task(struct ev_task *task);

static inline io_ctx_t *
io_timer_get_ctx(const io_timer_t *timer)
{
	return io_dev_get_ctx(io_timer_get_dev(timer));
}

static inline ev_exec_t *
io_timer_get_exec(const io_timer_t *timer)
{
	return io_dev_get_exec(io_timer_get_dev(timer));
}

static inline size_t
io_timer_cancel(io_timer_t *timer, struct ev_task *task)
{
	return io_dev_cancel(io_timer_get_dev(timer), task);
}

static inline size_t
io_timer_abort(io_timer_t *timer, struct ev_task *task)
{
	return io_dev_abort(io_timer_get_dev(timer), task);
}

inline io_dev_t *
io_timer_get_dev(const io_timer_t *timer)
{
	return (*timer)->get_dev(timer);
}

inline io_clock_t *
io_timer_get_clock(const io_timer_t *timer)
{
	return (*timer)->get_clock(timer);
}

inline int
io_timer_getoverrun(const io_timer_t *timer)
{
	return (*timer)->getoverrun(timer);
}

inline int
io_timer_gettime(const io_timer_t *timer, struct itimerspec *value)
{
	return (*timer)->gettime(timer, value);
}

inline int
io_timer_settime(io_timer_t *timer, int flags, const struct itimerspec *value,
		struct itimerspec *ovalue)
{
	return (*timer)->settime(timer, flags, value, ovalue);
}

inline void
io_timer_submit_wait(io_timer_t *timer, struct io_timer_wait *wait)
{
	(*timer)->submit_wait(timer, wait);
}

static inline size_t
io_timer_cancel_wait(io_timer_t *timer, struct io_timer_wait *wait)
{
	return io_timer_cancel(timer, &wait->task);
}

static inline size_t
io_timer_abort_wait(io_timer_t *timer, struct io_timer_wait *wait)
{
	return io_timer_abort(timer, &wait->task);
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_TIMER_H_
