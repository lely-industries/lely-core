/**@file
 * This header file is part of the event library; it contains the fiber
 * executor, mutex and condition variable declarations.
 *
 * The fiber executor ensures that each task runs in a fiber, or stackful
 * coroutine. Since it is platform-dependent whether fibers can be migrated
 * between threads, the (inner) executor from which the fibers are resumed MUST
 * be single-threaded.
 *
 * To limit the creation overhead, fibers are reused once they finish executing
 * a task. The implementation maintains a list of unused fibers, up to a
 * user-defined limit.
 *
 * The fiber mutex and condition variable are similar to the standard C11 mutex
 * and condition variable, except that they suspend the currently running fiber
 * instead of the thread.
 *
 * @see lely/util/fiber.h
 *
 * @copyright 2019-2022 Lely Industries N.V.
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

#ifndef LELY_EV_FIBER_EXEC_H_
#define LELY_EV_FIBER_EXEC_H_

#include <lely/ev/future.h>
#include <lely/util/fiber.h>

enum {
	/// Indicates that the requested operation succeeded.
	ev_fiber_success,
	/// Indicates that the requested operation failed.
	ev_fiber_error,
	/**
	 * Indicates that the time specified in the call was reached without
	 * acquiring the requested resource.
	 */
	ev_fiber_timedout,
	/**
	 * Indicates that the requested operation failed because a resource
	 * requested by a test and return function is already in use.
	 */
	ev_fiber_busy,
	/**
	 * Indicates that the requested operation failed because it was unable
	 * to allocate memory.
	 */
	ev_fiber_nomem
};

enum {
	/**
	 * A fiber mutex type that supports neither timeout nor recursive
	 * locking.
	 */
	ev_fiber_mtx_plain,
	/// A fiber mutex type that supports timeout (currently not supported).
	ev_fiber_mtx_timed,
	/// A fiber mutex type that supports recursive locking.
	ev_fiber_mtx_recursive
};

/**
 * A synchronization primitive (similar to the standard C11 mutex) that can be
 * used to protect shared data from being simultaneously accessed by multiple
 * fibers. This mutex offers exclusive, non-recursive ownership semantics.
 */
typedef struct {
	void *_impl;
} ev_fiber_mtx_t;

/**
 * A synchronization primitive (similar to the standard C11 condition variable)
 * that can be used to block one or more fibers until another fiber or thread
 * both modifies the shared variable (the _condition_), and notifies the
 * condition variable.
 */
typedef struct {
	void *_impl;
} ev_fiber_cnd_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the calling thread for use by fiber executors. This function MUST
 * be invoked at least once in each thread that will resume a suspended fiber.
 * This function can be invoked more than once by the same thread. Only the
 * first invocation initializes the thread.
 *
 * @param flags      the flags used to initialize the internal fiber of the
 *                   calling thread (see fiber_thrd_init()) and any subsequent
 *                   fibers created by a fiber executor. <b>flags</b> can be any
 *                   supported combination of #FIBER_SAVE_MASK,
 *                   #FIBER_SAVE_FENV, #FIBER_SAVE_ERROR and #FIBER_GUARD_STACK.
 * @param stack_size the size (in bytes) of the stack frame to be allocated for
 *                   the fibers. If 0, the default size (#LELY_FIBER_STKSZ) is
 *                   used. The size of the allocated stack is always at least
 *                   #LELY_FIBER_MINSTKSZ bytes.
 * @param max_unused the maximum number of unused fibers kept alive for future
 *                   tasks. If 0, the default number (#LELY_EV_FIBER_MAX_UNUSED)
 *                   used.
 *
 * @returns 1 if the calling thread is already initialized, 0 if it has been
 * successfully initialized, or -1 on error. In the latter case, the error
 * number can be obtained with get_errc().
 */
int ev_fiber_thrd_init(int flags, size_t stack_size, size_t max_unused);

/**
 * Finalizes the calling thread and prevents further use by fiber executors.
 * This function MUST be called once for each successful call to
 * ev_fiber_thrd_init(). Only the last invocation finalizes the thread.
 */
void ev_fiber_thrd_fini(void);

void *ev_fiber_exec_alloc(void);
void ev_fiber_exec_free(void *ptr);
ev_exec_t *ev_fiber_exec_init(ev_exec_t *exec, ev_exec_t *inner_exec);
void ev_fiber_exec_fini(ev_exec_t *exec);

/**
 * Creates a fiber executor. This function MUST be called from the thread used
 * by the inner executor.
 *
 * @param inner_exec a pointer to the inner executor used to resume suspended
 *                   fibers (which execute tasks). This MUST be a
 *                   single-threaded executor.
 *
 * @returns a pointer to a new fiber executor, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
ev_exec_t *ev_fiber_exec_create(ev_exec_t *inner_exec);

/**
 * Destroys a fiber executor. This function MUST be called from the thread on
 * which the executor was created.
 *
 * @see ev_fiber_exec_create()
 */
void ev_fiber_exec_destroy(ev_exec_t *exec);

/// Returns a pointer to the inner executor of a fiber executor.
ev_exec_t *ev_fiber_exec_get_inner_exec(const ev_exec_t *exec);

/**
 * Suspends a currently running fiber until the specified future becomes ready
 * (or is cancelled). If <b>future</b> is NULL, the fiber is suspended and
 * immediately resubmitted to the inner executor.
 *
 * This function MUST only be invoked from tasks submitted to a fiber executor.
 */
void ev_fiber_await(ev_future_t *future);

/**
 * Creates a fiber mutex object with properties indicated by <b>type</b>, which
 * must have one of the four values:
 * - `#ev_fiber_mtx_plain` for a simple non-recursive mutex,
 * - `#ev_fiber_mtx_timed` for a non-recursive mutex that supports timeout,
 * - `#ev_fiber_mtx_plain | #ev_fiber_mtx_recursive` for a simple recursive
 *    mutex, or
 * - `#ev_fiber_mtx_timed | #ev_fiber_mtx_recursive` for a recursive mutex that
 *    supports timeout.
 * Note that `#ev_fiber_mtx_timed` is currently not supported.
 *
 * If this function succeeds, it sets the mutex at <b>mtx</b> to a value that
 * uniquely identifies the newly created mutex.
 *
 * @returns #ev_fiber_success on success, or #ev_fiber_nomem if no memory could
 * be allocated for the newly created mutex, or #ev_fiber_error if the request
 * could not be honored.
 */
int ev_fiber_mtx_init(ev_fiber_mtx_t *mtx, int type);

/**
 * Releases any resources used by the fiber mutex at <b>mtx</b>. No fibers can
 * be blocked waiting for the mutex at <b>mtx</b>.
 */
void ev_fiber_mtx_destroy(ev_fiber_mtx_t *mtx);

/**
 * Suspends the currently running fiber until it locks the fiber mutex at
 * <b>mtx</b>. If the mutex is non-recursive, it SHALL not be locked by the
 * calling fiber. Prior calls to ev_fiber_mtx_unlock() on the same mutex shall
 * synchronize with this operation.
 *
 * This function MUST be called from a task running on a fiber executor.
 *
 * @returns #ev_fiber_success on success, or #ev_fiber_error if the request
 * could not be honored.
 */
int ev_fiber_mtx_lock(ev_fiber_mtx_t *mtx);

/**
 * Endeavors to lock the fiber mutex at <b>mtx</b>. If the mutex is already
 * locked, the function returns without blocking. If the operation succeeds,
 * prior calls to ev_fiber_mtx_unlock() on the same mutex shall synchronize with
 * this operation.
 *
 * This function MUST be called from a task running on a fiber executor.
 *
 * @returns #ev_fiber_success on success, or #ev_fiber_busy if the resource
 * requested is already in use, or #ev_fiber_error if the request could not be
 * honored.
 */
int ev_fiber_mtx_trylock(ev_fiber_mtx_t *mtx);

/**
 * Unlocks the fiber mutex at <b>mtx</b>. The mutex at <b>mtx</b> SHALL be
 * locked by the calling fiber.
 *
 * This function MUST be called from a task running on a fiber executor.
 *
 * @returns #ev_fiber_success on success, or #ev_fiber_error if the request
 * could not be honored.
 */
int ev_fiber_mtx_unlock(ev_fiber_mtx_t *mtx);

/**
 * Creates a fiber condition variable. If it succeeds it sets the variable at
 * <b>cond</b> to a value that uniquely identifies the newly created condition
 * variable. A fiber that calls ev_fiber_cnd_wait() on a newly created condition
 * variable will block.
 *
 * @returns #ev_fiber_success on success, or #ev_fiber_nomem if no memory could
 * be allocated for the newly created condition, or #ev_fiber_error if the
 * request could not be honored.
 */
int ev_fiber_cnd_init(ev_fiber_cnd_t *cond);

/**
 * Releases all resources used by the fiber condition variable at <b>cond</b>.
 * This function requires that no fibers be blocked waiting for the condition
 * variable at <b>cond</b>.
 */
void ev_fiber_cnd_destroy(ev_fiber_cnd_t *cond);

/**
 * Unblocks one of the fibers that are blocked on the fiber condition variable
 * at <b>cond</b> at the time of the call. If no fibers are blocked on the
 * condition variable at the time of the call, the function does nothing.
 *
 * @returns #ev_fiber_success.
 */
int ev_fiber_cnd_signal(ev_fiber_cnd_t *cond);

/**
 * Unblocks all of the fibers that are blocked on the fiber condition variable
 * at <b>cond</b> at the time of the call. If no fibers are blocked on the
 * condition variable at <b>cond</b> at the time of the call, the function does
 * nothing.
 *
 * @returns #ev_fiber_success.
 */
int ev_fiber_cnd_broadcast(ev_fiber_cnd_t *cond);

/**
 * Atomically unlocks the fiber mutex at <b>mtx</b> and endeavors to block until
 * the fiber condition variable at <b>cond</b> is signaled by a call to
 * ev_fiber_cnd_signal() or to ev_fiber_cnd_broadcast(). When the calling fiber
 * becomes unblocked it locks the mutex at <b>mtx</b> before it returns. This
 * function requires that the mutex at <b>mtx</b> be locked by the calling
 * fiber.
 *
 * This function MUST be called from a task running on a fiber executor.
 *
 * @returns #ev_fiber_success on success, or #ev_fiber_error if the request
 * could not be honored.
 */
int ev_fiber_cnd_wait(ev_fiber_cnd_t *cond, ev_fiber_mtx_t *mtx);

#ifdef __cplusplus
}
#endif

#endif // !LELY_EV_FIBER_EXEC_H_
