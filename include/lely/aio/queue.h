/*!\file
 * This header file is part of the asynchronous I/O library; it contains ...
 *
 * \copyright 2018 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#ifndef LELY_AIO_QUEUE_H_
#define LELY_AIO_QUEUE_H_

#include <lely/aio/exec.h>

#include <stddef.h>

#ifndef LELY_AIO_QUEUE_INLINE
#define LELY_AIO_QUEUE_INLINE	inline
#endif

struct aio_queue {
	struct aio_task *first;
	struct aio_task **plast;
};

#ifdef __cplusplus
extern "C" {
#endif

LELY_AIO_QUEUE_INLINE void aio_queue_init(struct aio_queue *queue);
LELY_AIO_QUEUE_INLINE struct aio_task *aio_queue_front(
		const struct aio_queue *queue);
LELY_AIO_EXTERN struct aio_task *aio_queue_back(const struct aio_queue *queue);
LELY_AIO_QUEUE_INLINE int aio_queue_empty(const struct aio_queue *queue);
LELY_AIO_EXTERN struct aio_task *aio_queue_remove(struct aio_queue *queue,
		struct aio_task *task);
LELY_AIO_QUEUE_INLINE void aio_queue_push(struct aio_queue *queue,
		struct aio_task *task);
LELY_AIO_QUEUE_INLINE struct aio_task *aio_queue_pop(struct aio_queue *queue);

LELY_AIO_EXTERN struct aio_queue *aio_queue_move(struct aio_queue *dst,
		struct aio_queue *src, struct aio_task *task);

LELY_AIO_EXTERN size_t aio_queue_post(struct aio_queue *queue);
LELY_AIO_EXTERN size_t aio_queue_cancel(struct aio_queue *queue, int errc);

LELY_AIO_QUEUE_INLINE void
aio_queue_init(struct aio_queue *queue)
{
	*(queue->plast = &queue->first) = NULL;
}

LELY_AIO_QUEUE_INLINE struct aio_task *
aio_queue_front(const struct aio_queue *queue)
{
	return queue->first;
}

LELY_AIO_QUEUE_INLINE int
aio_queue_empty(const struct aio_queue *queue)
{
	return !queue->first;
}

LELY_AIO_QUEUE_INLINE void
aio_queue_push(struct aio_queue *queue, struct aio_task *task)
{
	*queue->plast = task;
	queue->plast = &(*queue->plast)->_next;
	*queue->plast = NULL;
}

LELY_AIO_QUEUE_INLINE struct aio_task *
aio_queue_pop(struct aio_queue *queue)
{
	struct aio_task *task = queue->first;
	if (task) {
		if (!(queue->first = task->_next))
			queue->plast = &queue->first;
		task->_next = NULL;
	}
	return task;
}

#ifdef __cplusplus
}
#endif

#endif // LELY_AIO_QUEUE_H_
