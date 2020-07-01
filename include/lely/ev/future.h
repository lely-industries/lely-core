/**@file
 * This header file is part of the event library; it contains the
 * <a href="https://en.wikipedia.org/wiki/Futures_and_promises">futures and
 * promises</a> declarations.
 *
 * Unlike the futures and promises in C++11, this implementation provides
 * non-blocking semantics; instead of waiting for a future to become ready, the
 * user can submit a task which is executed once the future is ready.
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

#ifndef LELY_EV_FUTURE_H_
#define LELY_EV_FUTURE_H_

#include <lely/ev/ev.h>

#include <stdarg.h>
#include <stddef.h>

/**
 * An object providing a facility to store a value that is later acquired
 * asynchronously via an #ev_future_t object created by this object. A promise
 * is similar to a single-shot event and meant to be used only once.
 */
typedef struct ev_promise ev_promise_t;

/// An object providing access to the result of an asynchronous operation.
typedef struct ev_future ev_future_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of the function used to destroy (but not free) the shared state of a
 * promise once the last reference is released.
 *
 * @param ptr a pointer to the shared state to be destroyed.
 */
typedef void ev_promise_dtor_t(void *ptr);

/**
 * Constructs a new promise with an optional empty shared state. The promise is
 * destroyed once the last reference to it is released.
 *
 * @param size the size (in bytes) of the shared state (can be 0).
 * @param dtor a pointer to the function used to destroy the shared state (can
 *             be NULL).
 *
 * @returns a reference to the new promise, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see ev_promise_release()
 */
ev_promise_t *ev_promise_create(size_t size, ev_promise_dtor_t *dtor);

/**
 * Acquires a reference to a promise. If <b>promise</b> is NULL, this function
 * has no effect.
 *
 * @returns <b>promise</b>.
 *
 * @see ev_promise_release()
 */
ev_promise_t *ev_promise_acquire(ev_promise_t *promise);

/**
 * Releases a reference to a promise. If this is the last reference to the
 * promise, the promise is destroyed. If no references remain to its associated
 * future, the shared state is destroyed by the destructor provided to
 * ev_promise_create(). The object pointed to by <b>promise</b> MUST NOT be
 * referenced after this function had been invoked.
 *
 * @see ev_promise_acquire()
 */
void ev_promise_release(ev_promise_t *promise);

/**
 * Returns 1 if <b>promise</b> is the only reference to the promise and no
 * references to its associated future are held. Note that, although the
 * reference count itself is not reliable in multi-threaded environments,
 * optimizations based of whether a reference is unique are reliable.
 */
int ev_promise_is_unique(const ev_promise_t *promise);

/// Returns a pointer to the shared state of a promise.
void *ev_promise_data(const ev_promise_t *promise);

/**
 * Satiesfies a promise, if it was not aready satisfied, and stores the
 * specified value for retrieval by ev_future_get().
 *
 * This function is equivalent to
 * ```{.c}
 * int result = ev_promise_set_acquire(promise);
 * if (result)
 *         ev_promise_set_release(promise, value);
 * return result;
 * ```
 *
 * @returns 1 if the promise is successfully satisfied, and 0 if it was already
 * satisfied or is in the process of being satisfied.
 */
int ev_promise_set(ev_promise_t *promise, void *value);

/**
 * Checks if the specified promise can be satisfied by the caller and, if so,
 * prevents others from satisfying the promise.
 *
 * @returns 1 if the promise can be satisfed by the caller, and 0 if it was
 * already satisfied or is in the process of being satisfied.
 */
int ev_promise_set_acquire(ev_promise_t *promise);

/**
 * Satisfies a promise prepared by ev_promise_set_acquire(), and stores the
 * specified value for retrieval by ev_future_get(). It is common for the value
 * to be stored in the shared state (in which case <b>value</b> equals
 * `ev_promise_data(promise)`), but this is not required.
 *
 * @pre the preceding call ev_promise_set_acquire() returned 1.
 */
void ev_promise_set_release(ev_promise_t *promise, void *value);

/// Returns (a reference to) a future associated with the specified promise.
ev_future_t *ev_promise_get_future(ev_promise_t *promise);

/**
 * Acquires a reference to a future. If <b>future</b> is NULL, this function
 * has no effect.
 *
 * @returns <b>future</b>.
 *
 * @see ev_future_release()
 */
ev_future_t *ev_future_acquire(ev_future_t *future);

/**
 * Releases a reference to a future. If this is the last reference to the
 * future, the future is destroyed. If no references remain to its associated
 * promise, the shared state is destroyed. The object pointed to by
 * <b>future</b> MUST NOT be referenced after this function had been invoked.
 *
 * @see ev_future_acquire()
 */
void ev_future_release(ev_future_t *future);

/**
 * Returns 1 if <b>future</b> is the only reference to the future and no
 * references to its associated promise are held. Note that, although the
 * reference count itself is not reliable in multi-threaded environments,
 * optimizations based of whether a reference is unique are reliable.
 */
int ev_future_is_unique(const ev_future_t *future);

/**
 * Checks if the specified future is ready, i.e., its associated promise has
 * been satisfied.
 *
 * @returns 1 if the future is ready, and 0 if not.
 *
 * @see ev_promise_set(), ev_future_get()
 */
int ev_future_is_ready(const ev_future_t *future);

/**
 * Returns the result of a ready future. Calling this function when the
 * associated promise is not yet satisfied is undefined behavior.
 *
 * @returns the value provided to ev_promise_set().
 *
 * @pre ev_future_is_ready() returns 1.
 *
 * @see ev_future_is_ready()
 */
void *ev_future_get(const ev_future_t *future);

/**
 * Submits a task to be executed once the specified future is ready. This
 * function SHALL NOT block waiting for the future to become ready or the task
 * to be completed. If this function is called with a ready future, the task
 * MAY begin executing before this function returns, depending on the behavior
 * of ev_exec_post() of its associated executor.
 */
void ev_future_submit(ev_future_t *future, struct ev_task *task);

/**
 * Cancels the specified task submitted with ev_future_submit(), if it has not
 * yet been scheduled for execution, or all tasks if <b>task</b> is NULL.
 * Canceled tasks are submitted for execution to their associated executor with
 * ev_exec_post(), whether the future is ready or not.
 *
 * @returns the number of canceled tasks.
 *
 * @see ev_future_abort()
 */
size_t ev_future_cancel(ev_future_t *future, struct ev_task *task);

/**
 * Aborts the specified task submitted with ev_future_submit(), if it has not
 * yet been scheduled for execution, or all tasks if <b>task</b> is NULL.
 * Aborted tasks are _not_ scheduled for execution.
 *
 * @returns the number of aborted tasks.
 *
 * @see ev_future_cancel()
 */
size_t ev_future_abort(ev_future_t *future, struct ev_task *task);

/**
 * Equivalent to ev_future_when_all_n(), except that it accepts a variable
 * number of arguments instead of a list of futures. The last argument MUST be a
 * NULL pointer.
 */
ev_future_t *ev_future_when_all(ev_exec_t *exec, ev_future_t *future, ...);

/**
 * Equivalent to ev_future_when_all(), except that it accepts a `va_list`
 * instead of a variable number of arguments.
 */
ev_future_t *ev_future_when_all_v(
		ev_exec_t *exec, ev_future_t *future, va_list ap);

/**
 * Creates a future that becomes ready when all of the input futures become
 * ready or one of the input futures is abandoned before becoming ready. The
 * result of the created future, once it becomes ready, is a `size_t` value
 * containing the number of ready futures or the index of the first abandoned
 * future. If no input futures are specified, a ready future is returned with an
 * empty result (ev_future_get() returns NULL).
 *
 * This function acquires references to each of the input futures and submits
 * tasks waiting for them to become ready. It is the responsibility of the
 * caller to ensure that these tasks are not aborted with ev_future_abort(), as
 * that would lead to a resource leak.
 *
 * @param exec    a pointer the executor used for the waiting tasks.
 * @param n       the number of (pointers to) futures in <b>futures</b>.
 * @param futures an array of pointers to futures (can be NULL if <b>n</b> is
 *                0).
 *
 * @returns a pointer to a new future, or NULL on error. In the latter case, the
 * error number can be obtained with get_errc().
 */
ev_future_t *ev_future_when_all_n(
		ev_exec_t *exec, size_t n, ev_future_t *const *futures);

/**
 * Equivalent to ev_future_when_any_n(), except that it accepts a variable
 * number of arguments instead of a list of futures. The last argument MUST be a
 * NULL pointer.
 */
ev_future_t *ev_future_when_any(ev_exec_t *exec, ev_future_t *future, ...);

/**
 * Equivalent to ev_future_when_any(), except that it accepts a `va_list`
 * instead of a variable number of arguments.
 */
ev_future_t *ev_future_when_any_v(
		ev_exec_t *exec, ev_future_t *future, va_list ap);

/**
 * Creates a future that becomes ready when at least one of the input futures
 * becomes ready or is abandoned. The result of the created future, once it
 * becomes ready, is a `size_t` value containing the index of the first ready
 * (or abandoned) future. If no input futures are specified, a ready future is
 * returned with an empty result (ev_future_get() returns NULL).
 *
 * This function acquires references to each of the input futures and submits
 * tasks waiting for them to become ready. It is the responsibility of the
 * caller to ensure that these tasks are not aborted with ev_future_abort(), as
 * that would lead to a resource leak.
 *
 * @param exec    a pointer the executor used for the waiting tasks.
 * @param n       the number of (pointers to) futures in <b>futures</b>.
 * @param futures an array of pointers to futures (can be NULL if <b>n</b> is
 *                0).
 *
 * @returns a pointer to a new future, or NULL on error. In the latter case, the
 * error number can be obtained with get_errc().
 */
ev_future_t *ev_future_when_any_n(
		ev_exec_t *exec, size_t n, ev_future_t *const *futures);

#ifdef __cplusplus
}
#endif

#endif // !LELY_EV_FUTURE_H_
