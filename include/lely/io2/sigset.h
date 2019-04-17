/**@file
 * This header file is part of the I/O library; it contains the abstract signal
 * handler interface.
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

#ifndef LELY_IO2_SIGSET_H_
#define LELY_IO2_SIGSET_H_

#include <lely/ev/future.h>
#include <lely/ev/task.h>
#include <lely/io2/dev.h>

#include <signal.h>

#if _WIN32 && !defined(SIGHUP)
#define SIGHUP 1
#endif

#ifndef LELY_IO_SIGSET_INLINE
#define LELY_IO_SIGSET_INLINE static inline
#endif

/// An abstract signal handler.
typedef const struct io_sigset_vtbl *const io_sigset_t;

/// A wait operation suitable for use with a signal handler.
struct io_sigset_wait {
	/**
	 * The task (to be) submitted upon completion (or cancellation) of the
	 * wait operation.
	 */
	struct ev_task task;
	/// The signal number, or 0 if the wait operation was canceled.
	int signo;
};

/// The static initializer for #io_sigset_wait.
#define IO_SIGSET_WAIT_INIT(exec, func) \
	{ \
		EV_TASK_INIT(exec, func), 0 \
	}

#ifdef __cplusplus
extern "C" {
#endif

struct io_sigset_vtbl {
	io_dev_t *(*get_dev)(const io_sigset_t *sigset);
	int (*clear)(io_sigset_t *sigset);
	int (*insert)(io_sigset_t *sigset, int signo);
	int (*remove)(io_sigset_t *sigset, int signo);
	void (*submit_wait)(io_sigset_t *sigset, struct io_sigset_wait *wait);
};

/// @see io_dev_get_ctx()
static inline io_ctx_t *io_sigset_get_ctx(const io_sigset_t *sigset);

/// @see io_dev_get_exec()
static inline ev_exec_t *io_sigset_get_exec(const io_sigset_t *sigset);

/// @see io_dev_cancel()
static inline size_t io_sigset_cancel(
		io_sigset_t *sigset, struct ev_task *task);

/// @see io_dev_abort()
static inline size_t io_sigset_abort(io_sigset_t *sigset, struct ev_task *task);

/**
 * Returns a pointer to the abstract I/O device representing the signal handler.
 */
LELY_IO_SIGSET_INLINE io_dev_t *io_sigset_get_dev(const io_sigset_t *sigset);

/**
 * Clears the set of signals being monitored by a signal handler.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sigset_insert(), io_sigset_remove()
 */
LELY_IO_SIGSET_INLINE int io_sigset_clear(io_sigset_t *sigset);

/**
 * Insert the specified signal number into the set of signals being monitored by
 * a signal handler.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sigset_clear(), io_sigset_remove()
 */
LELY_IO_SIGSET_INLINE int io_sigset_insert(io_sigset_t *sigset, int signo);

/**
 * Removes the specified signal number from the set of signals being monitored
 * by a signal handler.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_sigset_clear(), io_sigset_insert()
 */
LELY_IO_SIGSET_INLINE int io_sigset_remove(io_sigset_t *sigset, int signo);

/**
 * Submits a wait operation to a signal handler. The completion task is
 * submitted for execution once a signal is caught.
 */
LELY_IO_SIGSET_INLINE void io_sigset_submit_wait(
		io_sigset_t *sigset, struct io_sigset_wait *wait);

/**
 * Cancels the specified signal wait operation if it is pending. The completion
 * task is submitted for execution with <b>signo</b> = 0.
 *
 * @returns 1 if the operation was canceled, and 0 if it was not pending.
 *
 * @see io_dev_cancel()
 */
static inline size_t io_sigset_cancel_wait(
		io_sigset_t *sigset, struct io_sigset_wait *wait);

/**
 * Aborts the specified signal wait operation if it is pending. If aborted, the
 * completion task is _not_ submitted for execution.
 *
 * @returns 1 if the operation was aborted, and 0 if it was not pending.
 *
 * @see io_dev_abort()
 */
static inline size_t io_sigset_abort_wait(
		io_sigset_t *sigset, struct io_sigset_wait *wait);

/**
 * Submits an asynchronous wait operation to a signal handler and creates a
 * future which becomes ready once the wait operation completes (or is
 * canceled). The result of the future is an `int` containing the signal number,
 * or 0 if the wait operation was canceled.
 *
 * @param sigset a pointer to a signal handler.
 * @param exec   a pointer to the executor used to execute the completion
 *               function of the wait operation. If NULL, the default executor
 *               of the signal handler is used.
 * @param pwait  the address at which to store a pointer to the wait operation
 *               (can be NULL).
 *
 * @returns a pointer to a future, or NULL on error. In the latter case, the
 * error number can be obtained with get_errc().
 */
ev_future_t *io_sigset_async_wait(io_sigset_t *sigset, ev_exec_t *exec,
		struct io_sigset_wait **pwait);
/**
 * Obtains a pointer to a signal wait operation from a pointer to its completion
 * task.
 */
struct io_sigset_wait *io_sigset_wait_from_task(struct ev_task *task);

static inline io_ctx_t *
io_sigset_get_ctx(const io_sigset_t *sigset)
{
	return io_dev_get_ctx(io_sigset_get_dev(sigset));
}

static inline ev_exec_t *
io_sigset_get_exec(const io_sigset_t *sigset)
{
	return io_dev_get_exec(io_sigset_get_dev(sigset));
}

static inline size_t
io_sigset_cancel(io_sigset_t *sigset, struct ev_task *task)
{
	return io_dev_cancel(io_sigset_get_dev(sigset), task);
}

static inline size_t
io_sigset_abort(io_sigset_t *sigset, struct ev_task *task)
{
	return io_dev_abort(io_sigset_get_dev(sigset), task);
}

inline io_dev_t *
io_sigset_get_dev(const io_sigset_t *sigset)
{
	return (*sigset)->get_dev(sigset);
}

inline int
io_sigset_clear(io_sigset_t *sigset)
{
	return (*sigset)->clear(sigset);
}

inline int
io_sigset_insert(io_sigset_t *sigset, int signo)
{
	return (*sigset)->insert(sigset, signo);
}

inline int
io_sigset_remove(io_sigset_t *sigset, int signo)
{
	return (*sigset)->remove(sigset, signo);
}

inline void
io_sigset_submit_wait(io_sigset_t *sigset, struct io_sigset_wait *wait)
{
	(*sigset)->submit_wait(sigset, wait);
}

static inline size_t
io_sigset_cancel_wait(io_sigset_t *sigset, struct io_sigset_wait *wait)
{
	return io_sigset_cancel(sigset, &wait->task);
}

static inline size_t
io_sigset_abort_wait(io_sigset_t *sigset, struct io_sigset_wait *wait)
{
	return io_sigset_abort(sigset, &wait->task);
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_SIGSET_H_
