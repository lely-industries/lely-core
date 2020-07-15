/**@file
 * This header file is part of the event library; it contains the abstract task
 * executor interface.
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

#ifndef LELY_EV_EXEC_H_
#define LELY_EV_EXEC_H_

#include <lely/ev/ev.h>

#include <stddef.h>

#ifndef LELY_EV_EXEC_INLINE
#define LELY_EV_EXEC_INLINE static inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct ev_exec_vtbl {
	void (*on_task_init)(ev_exec_t *exec);
	void (*on_task_fini)(ev_exec_t *exec);
	int (*dispatch)(ev_exec_t *exec, struct ev_task *task);
	void (*post)(ev_exec_t *exec, struct ev_task *task);
	void (*defer)(ev_exec_t *exec, struct ev_task *task);
	size_t (*abort)(ev_exec_t *exec, struct ev_task *task);
	void (*run)(ev_exec_t *exec, struct ev_task *task);
};

/**
 * Indicates to the specified executor that a task will be submitted for
 * execution in the future. This is typically used to prevent an event loop from
 * terminating early.
 */
LELY_EV_EXEC_INLINE void ev_exec_on_task_init(ev_exec_t *exec);

/// Undoes the effect of a previous call to ev_exec_on_task_init().
LELY_EV_EXEC_INLINE void ev_exec_on_task_fini(ev_exec_t *exec);

/**
 * Submits <b>*task</b> to <b>*exec</b> for execution. The task function is
 * invoked at most once. The executor MAY block pending the completion of the
 * task. This typically happens when this function is invoked from the execution
 * context of <b>*exec</b> (i.e., by a task currently being executed).
 *
 * @returns 1 if this function blocked and the task has completed, and 0 if not.
 *
 * @see ev_exec_post(), ev_exec_defer()
 */
LELY_EV_EXEC_INLINE int ev_exec_dispatch(ev_exec_t *exec, struct ev_task *task);

/**
 * Submits <b>*task</b> to <b>*exec</b> for execution. The task function is
 * invoked at most once. The executor SHALL NOT block pending the completion of
 * the task, but MAY begin executing the task before this function returns.
 *
 * @see ev_exec_dispatch(), ev_exec_defer()
 */
LELY_EV_EXEC_INLINE void ev_exec_post(ev_exec_t *exec, struct ev_task *task);

/**
 * Submits <b>*task</b> to <b>*exec</b> for execution. The task function is
 * invoked at most once. The executor SHALL NOT block pending the completion of
 * the task, and, if invoked from a running task, SHALL NOT begin executing the
 * task before the current task completes. This function is typically used to
 * indicate that* <b>*task</b> is a continuation of the current call context.
 *
 * @see ev_exec_dispatch(), ev_exec_post()
 */
LELY_EV_EXEC_INLINE void ev_exec_defer(ev_exec_t *exec, struct ev_task *task);

/**
 * Aborts the specified task submitted to <b>*exec</b>, if it has not yet begun
 * executing, or all pending tasks if <b>task</b> is NULL.
 *
 * @returns the number of aborted tasks.
 */
LELY_EV_EXEC_INLINE size_t ev_exec_abort(ev_exec_t *exec, struct ev_task *task);

/**
 * Invokes the task function in <b>*task</b> as if the task is being executed by
 * <b>*exec</b>. This function typically sets up an execution context in which
 * ev_exec_dispatch() and ev_exec_defer() behave differently than if the task
 * function is invoked directly.
 */
LELY_EV_EXEC_INLINE void ev_exec_run(ev_exec_t *exec, struct ev_task *task);

inline void
ev_exec_on_task_init(ev_exec_t *exec)
{
	(*exec)->on_task_init(exec);
}

inline void
ev_exec_on_task_fini(ev_exec_t *exec)
{
	(*exec)->on_task_fini(exec);
}

inline int
ev_exec_dispatch(ev_exec_t *exec, struct ev_task *task)
{
	return (*exec)->dispatch(exec, task);
}

inline void
ev_exec_post(ev_exec_t *exec, struct ev_task *task)
{
	(*exec)->post(exec, task);
}

inline void
ev_exec_defer(ev_exec_t *exec, struct ev_task *task)
{
	(*exec)->defer(exec, task);
}

inline size_t
ev_exec_abort(ev_exec_t *exec, struct ev_task *task)
{
	return (*exec)->abort(exec, task);
}

inline void
ev_exec_run(ev_exec_t *exec, struct ev_task *task)
{
	(*exec)->run(exec, task);
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_EV_EXEC_H_
