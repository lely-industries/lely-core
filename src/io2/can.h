/**@file
 * This is the internal header file of the CAN bus operation queue functions.
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

#ifndef LELY_IO2_INTERN_CAN_H_
#define LELY_IO2_INTERN_CAN_H_

#include "io2.h"
#include <lely/ev/exec.h>
#include <lely/io2/can.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static void io_can_chan_read_post(
		struct io_can_chan_read *read, int result, int errc);
static size_t io_can_chan_read_queue_post(
		struct sllist *queue, int result, int errc);

static void io_can_chan_write_post(struct io_can_chan_write *write, int errc);
static size_t io_can_chan_write_queue_post(struct sllist *queue, int errc);

static inline void
io_can_chan_read_post(struct io_can_chan_read *read, int result, int errc)
{
	read->r.result = result;
	read->r.errc = errc;

	ev_exec_t *exec = read->task.exec;
	ev_exec_post(exec, &read->task);
	ev_exec_on_task_fini(exec);
}

static inline size_t
io_can_chan_read_queue_post(struct sllist *queue, int result, int errc)
{
	size_t n = 0;

	struct slnode *node;
	while ((node = sllist_pop_front(queue))) {
		struct ev_task *task = ev_task_from_node(node);
		struct io_can_chan_read *read =
				io_can_chan_read_from_task(task);
		io_can_chan_read_post(read, result, errc);
		n += n < SIZE_MAX;
	}

	return n;
}

static inline void
io_can_chan_write_post(struct io_can_chan_write *write, int errc)
{
	write->errc = errc;

	ev_exec_t *exec = write->task.exec;
	ev_exec_post(exec, &write->task);
	ev_exec_on_task_fini(exec);
}

static inline size_t
io_can_chan_write_queue_post(struct sllist *queue, int errc)
{
	size_t n = 0;

	struct slnode *node;
	while ((node = sllist_pop_front(queue))) {
		struct ev_task *task = ev_task_from_node(node);
		struct io_can_chan_write *write =
				io_can_chan_write_from_task(task);
		io_can_chan_write_post(write, errc);
		n += n < SIZE_MAX;
	}

	return n;
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_INTERN_CAN_H_
