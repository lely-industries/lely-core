/**@file
 * This header file is part of the event library; it contains the fiber executor
 * declarations.
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
 * @see lely/util/fiber.h
 *
 * @copyright 2019 Lely Industries N.V.
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

#ifndef LELY_EV_CO_FIBER_H_
#define LELY_EV_CO_FIBER_H_

#include <lely/ev/future.h>
#include <lely/util/fiber.h>

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

#ifdef __cplusplus
}
#endif

#endif // !LELY_EV_CO_FIBER_H_
