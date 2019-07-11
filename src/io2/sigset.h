/**@file
 * This is the internal header file of the signal wait operation queue
 * functions.
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

#ifndef LELY_IO2_INTERN_SIGSET_H_
#define LELY_IO2_INTERN_SIGSET_H_

#include "io2.h"
#include <lely/ev/exec.h>
#include <lely/io2/sigset.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static void io_sigset_wait_post(struct io_sigset_wait *wait, int signo);
static size_t io_sigset_wait_queue_post(struct sllist *queue, int signo);

static inline void
io_sigset_wait_post(struct io_sigset_wait *wait, int signo)
{
	wait->signo = signo;

	ev_exec_t *exec = wait->task.exec;
	ev_exec_post(exec, &wait->task);
	ev_exec_on_task_fini(exec);
}

static inline size_t
io_sigset_wait_queue_post(struct sllist *queue, int signo)
{
	size_t n = 0;

	struct slnode *node;
	while ((node = sllist_pop_front(queue))) {
		struct ev_task *task = ev_task_from_node(node);
		struct io_sigset_wait *wait = io_sigset_wait_from_task(task);
		io_sigset_wait_post(wait, signo);
		n += n < SIZE_MAX;
	}

	return n;
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_INTERN_SIGSET_H_
