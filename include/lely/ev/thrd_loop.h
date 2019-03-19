/**@file
 * This header file is part of the event library; it contains the thread-local
 * event loop declarations.
 *
 * The thread-local event loop maintains a task queue for each thread of
 * execution. Although the corresponding executor is shared between all threads,
 * tasks are always submitted to the thread-local queue and executed when
 * ev_thrd_loop_run() or ev_thrd_loop_run_one() is invoked from the thread on
 * which they were submitted.
 *
 * If not explicitly stopped, ev_thrd_loop_run() and ev_thrd_loop_run_one() will
 * execute pending tasks as long as the thread has outstanding work. In this
 * context, outstanding work is defined as the sum of all pending and currently
 * executing tasks, plus the number of calls to ev_exec_on_task_init() by this
 * thread, minus the number of calls to ev_exec_on_task_fini(). If, at any time,
 * the outstanding work falls to 0, the thread-local event loop is stopped as if
 * by ev_thrd_loop_stop().
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

#ifndef LELY_EV_THRD_LOOP_H_
#define LELY_EV_THRD_LOOP_H_

#include <lely/ev/ev.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns a pointer to the executor corresponding to the thread-local event
 * loop. This pointer is shared between all threads.
 */
ev_exec_t *ev_thrd_loop_get_exec(void);

/**
 * Stops the thread-local event loop. Subsequent calls to ev_thrd_loop_run() and
 * ev_thrd_loop_run_one() will return 0 immediately.
 *
 * @post ev_thrd_loop_stopped() returns 1.
 */
void ev_thrd_loop_stop(void);

/// Returns 1 if the thread-local event loop is stopped, and 0 if not.
int ev_thrd_loop_stopped(void);

/// Restarts a thread-local event loop. @post ev_thrd_loop_stopped() returns 0.
void ev_thrd_loop_restart(void);

/**
 * If the thread-local event loop is not stopped, run all available tasks. If,
 * afterwards, no outstanding work remains, performs ev_thrd_loop_stop().
 *
 * @returns the number of executed tasks.
 */
size_t ev_thrd_loop_run(void);

/**
 * If the thread-local event loop is not stopped, executes the first task
 * submitted to it, if available. If, afterwards, no outstanding work remains,
 * performs ev_thrd_loop_stop().
 *
 * @returns 1 if a task was executed, and 0 if not.
 */
size_t ev_thrd_loop_run_one(void);

#ifdef __cplusplus
}
#endif

#endif // !LELY_EV_THRD_LOOP_H_
