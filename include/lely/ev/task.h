/**@file
 * This header file is part of the event library; it contains the task
 * declarations.
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

#ifndef LELY_EV_TASK_H_
#define LELY_EV_TASK_H_

#include <lely/ev/ev.h>
#include <lely/util/sllist.h>

#ifdef __cplusplus
extern "C" {
#endif

/// The type of function invoked by an executor when a task is run.
typedef void ev_task_func_t(struct ev_task *task);

/**
 * An executable task. Tasks are used to submit arbitrary functions to an
 * executor. Additional data can be associated with a task by embedding it in a
 * struct and using structof() from the task fuction to obtain a pointer to the
 * struct.
 */
struct ev_task {
	/// A pointer to the executor to which the task is (to be) submitted.
	ev_exec_t *exec;
	/// The function to be invoked when the task is run.
	ev_task_func_t *func;
	// The node of this task in a queue.
	struct slnode _node;
	// A pointer used to store additional data while processing a task.
	void *_data;
};

/// The static initializer for #ev_task.
#define EV_TASK_INIT(exec, func) \
	{ \
		(exec), (func), SLNODE_INIT, NULL \
	}

/**
 * Converts a pointer to a node in a queue to the address of the task containing
 * the node.
 *
 * @returns a pointer to the task, or NULL if <b>node</b> is NULL.
 */
struct ev_task *ev_task_from_node(struct slnode *node);

/**
 * Post the tasks in <b>queue</b> to their respective executors and invokes
 * ev_exec_on_task_fini() for each of them.
 *
 * @returns the number of tasks in <b>queue</b>.
 */
size_t ev_task_queue_post(struct sllist *queue);

/**
 * Aborts the tasks in <b>queue</b> by invoking ev_exec_on_task_fini() for each
 * of them.
 *
 * @returns the number of tasks in <b>queue</b>.
 */
size_t ev_task_queue_abort(struct sllist *queue);

#ifdef __cplusplus
}
#endif

#endif // !LELY_EV_TASK_H_
