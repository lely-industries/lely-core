/**@file
 * This file is part of the asynchronous I/O library; it contains ...
 *
 * @see lely/aio/queue.h
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

#include "aio.h"
#include <lely/util/util.h>
#define LELY_AIO_QUEUE_INLINE extern inline LELY_DLL_EXPORT
#include <lely/aio/queue.h>

#include <assert.h>
#include <stdint.h>

struct aio_task *
aio_queue_back(const struct aio_queue *queue)
{
	assert(queue);

	return queue->plast == &queue->first
			? structof(queue->plast, struct aio_task, _next)
			: NULL;
}

struct aio_task *
aio_queue_remove(struct aio_queue *queue, struct aio_task *task)
{
	assert(queue);

	struct aio_task **ptask = &queue->first;
	while (*ptask && *ptask != task)
		ptask = &(*ptask)->_next;
	if ((task = *ptask)) {
		if (!(*ptask = task->_next))
			queue->plast = ptask;
		task->_next = NULL;
	}
	return task;
}

struct aio_queue *
aio_queue_move(struct aio_queue *dst, struct aio_queue *src,
		struct aio_task *task)
{
	assert(dst);
	assert(src);

	if (task) {
		if (aio_queue_remove(src, task))
			aio_queue_push(dst, task);
	} else if (!aio_queue_empty(src)) {
		*dst->plast = src->first;
		dst->plast = src->plast;
		aio_queue_init(src);
	}

	return dst;
}

size_t
aio_queue_post(struct aio_queue *queue)
{
	assert(queue);

	size_t n = 0;

	struct aio_task *task;
	while ((task = aio_queue_pop(queue))) {
		aio_exec_post(task->exec, task);
		aio_exec_on_task_finished(task->exec);
		n += n < SIZE_MAX;
	}

	return n;
}

size_t
aio_queue_cancel(struct aio_queue *queue, int errc)
{
	assert(queue);

	size_t n = 0;

	struct aio_task *task;
	while ((task = aio_queue_pop(queue))) {
		task->errc = errc;
		aio_exec_post(task->exec, task);
		aio_exec_on_task_finished(task->exec);
		n += n < SIZE_MAX;
	}

	return n;
}
