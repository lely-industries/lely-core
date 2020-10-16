/**@file
 * This header file is part of the utilities library; it contains the stop token
 * declarations.
 *
 * The stop token API is based on the C++20 stop token interface.
 *
 * @copyright 2020 Lely Industries N.V.
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

#ifndef LELY_UTIL_STOP_H_
#define LELY_UTIL_STOP_H_

#include <lely/util/sllist.h>

/**
 * An object providing the means to check if a stop request has been made for
 * its associated #stop_source_t object. It is essentially a thread-safe "view"
 * of the associated stop-state.
 */
typedef struct stop_token stop_token_t;

/**
 * An object providing the means to issue a stop request. A stop request is
 * visible to all associated #stop_token_t objects.
 */
typedef struct stop_source stop_source_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An object providing the means to register a callback function with an
 * #stop_token_t object. The callback function will be invoked when the
 * #stop_source_t object associated with the stop token is requested to stop.
 */
struct stop_func {
	/// The function to be invoked when a stop request is issued.
	void (*func)(struct stop_func *func);
	// The node of this callback in a queue.
	struct slnode _node;
};

/// The static initializer for #stop_func.
#define STOP_FUNC_INIT(func) \
	{ \
		(func), SLNODE_INIT \
	}

/**
 * Acquires a reference to a stop token. If <b>token</b> is NULL, this function
 * has no effect.
 *
 * @returns <b>token</b>.
 *
 * @see stop_token_release()
 */
stop_token_t *stop_token_acquire(stop_token_t *token);

/**
 * Releases a reference to a stop token. If this is the last reference to the
 * token, the token is destroyed. If no references remain to its associated
 * stop source, the stop-state is destroyed. The object pointed to by
 * <b>token</b> MUST NOT be referenced after this function has been invoked.
 *
 * @see stop_token_acquire()
 */
void stop_token_release(stop_token_t *token);

/**
 * Checks if the stop-state associated with the specified stop token has
 * received a stop request.
 *
 * @returns 1 if a stop request has been received, and 0 if not.
 *
 * @see stop_token_stop_requested()
 */
int stop_token_stop_requested(const stop_token_t *token);

/**
 * Checks if the stop-state associated with the specified stop token has
 * received a stop request, or if it has an associated stop source that can
 * still issue such an request.
 *
 * @returns 1 if a stop request has been received or still can be received, and
 * 0 if not.
 *
 * @see stop_token_stop_requested()
 */
int stop_token_stop_possible(const stop_token_t *token);

/**
 * Registers a callback function with the specified stop token. If a stop
 * request has already been issued for the associated stop source (i.e.,
 * stop_token_stop_requested() returns 1), the callback function is invoked in
 * the calling thread before this function returns.
 *
 * @returns 1 if the callback function was invoked, and 0 if not.
 *
 * @see stop_token_remove()
 */
int stop_token_insert(stop_token_t *token, struct stop_func *func);

/**
 * Deregisters a callback function from the specified stop token. If the
 * callback function is being invoked concurrently on another thread, this
 * function does not return until the callback function invocation is complete.
 * If the callback function is being invoked on the calling thread, this
 * function does not wait until the invocation is complete. Hence it is safe to
 * invoke this function from the callback function.
 *
 * @see stop_token_insert()
 */
void stop_token_remove(stop_token_t *token, struct stop_func *func);

/**
 * Creates a stop source with a new stop-state. The stop source is destroyed
 * once the last reference to it is released.
 *
 * @returns a reference to the new stop source on succes, or NULL on error. In
 * the latter case, the error number can be obtained with get_errc().
 *
 * @see stop_source_release()
 */
stop_source_t *stop_source_create(void);

/**
 * Acquires a reference to a stop source. If <b>source</b> is NULL, this
 * function has no effect.
 *
 * @returns <b>source</b>.
 *
 * @see stop_source_release()
 */
stop_source_t *stop_source_acquire(stop_source_t *source);

/**
 * Releases a reference to a stop source. If this is the last reference to the
 * source, the source is destroyed. If no references remain to its associated
 * stop token, the stop-state is destroyed. The object pointed to by
 * <b>source</b> MUST NOT be referenced after this function has been invoked.
 *
 * @see stop_source_acquire()
 */
void stop_source_release(stop_source_t *source);

/**
 * Issues a stop request to the stop-state associated with the specified stop
 * source, if it has not already received a stop request. Once a stop is
 * requested, it cannot be withdrawn. If a stop request is issued, any
 * #stop_func callbacks registered with the associated stop token are invoked
 * synchronously on the calling thread.
 *
 * @returns 1 if a stop request was issued, and 0 if not. In the latter case,
 * another thread MAY still be in the middle of invoking a #stop_func callback.
 *
 * @post stop_source_stop_requested() returns 1.
 */
int stop_source_request_stop(stop_source_t *source);

/**
 * Checks if the stop-state associated with the specified stop source has
 * received a stop request.
 *
 * @returns 1 if a stop request has been received, and 0 if not.
 *
 * @see stop_source_request_stop()
 */
int stop_source_stop_requested(const stop_source_t *source);

/**
 * Returns (a reference to) a stop token associated with the specified stop
 * source's stop-state.
 */
stop_token_t *stop_source_get_token(stop_source_t *source);

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_STOP_H_
