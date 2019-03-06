/**@file
 * This file is part of the asynchronous I/O library; it contains ...
 *
 * @see lely/aio/exec.h
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
#include <lely/util/errnum.h>
#define LELY_AIO_EXEC_INLINE extern inline LELY_DLL_EXPORT
#include <lely/aio/exec.h>
#include <lely/aio/loop.h>
#include <lely/aio/queue.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdlib.h>

static void aio_exec_impl_run(aio_exec_t *exec, struct aio_task *task);
static void aio_exec_impl_dispatch(aio_exec_t *exec, struct aio_task *task);
static void aio_exec_impl_post(aio_exec_t *exec, struct aio_task *task);
static void aio_exec_impl_defer(aio_exec_t *exec, struct aio_task *task);
static void aio_exec_impl_on_task_started(aio_exec_t *exec);
static void aio_exec_impl_on_task_finished(aio_exec_t *exec);

// clang-format off
static const struct aio_exec_vtbl aio_exec_impl_vtbl = {
	&aio_exec_impl_run,
	&aio_exec_impl_dispatch,
	&aio_exec_impl_post,
	&aio_exec_impl_defer,
	&aio_exec_impl_on_task_started,
	&aio_exec_impl_on_task_finished
};
// clang-format on

struct aio_exec_impl {
	const struct aio_exec_vtbl *exec_vptr;
	aio_loop_t *loop;
};

static inline struct aio_exec_impl *aio_impl_from_exec(const aio_exec_t *exec);

struct aio_queue_node {
	struct aio_queue_node *next;
	struct aio_queue *queue;
};

struct aio_exec_node {
	struct aio_exec_node *next;
	aio_exec_t *exec;
	struct aio_queue_node *queue;
};

#if LELY_NO_THREADS
static struct aio_exec_node *aio_exec_list;
#else
static _Thread_local struct aio_exec_node *aio_exec_list;
#endif

static struct aio_exec_node **aio_exec_find(
		const aio_exec_t *loop, struct aio_exec_node **pnode);

void *
aio_exec_alloc(void)
{
	struct aio_exec_impl *impl = malloc(sizeof(*impl));
	if (!impl)
		set_errc(errno2c(errno));
	return impl ? &impl->exec_vptr : NULL;
}

void
aio_exec_free(void *ptr)
{
	if (ptr)
		free(aio_impl_from_exec(ptr));
}

aio_exec_t *
aio_exec_init(aio_exec_t *exec, aio_loop_t *loop)
{
	struct aio_exec_impl *impl = aio_impl_from_exec(exec);
	assert(loop);

	impl->exec_vptr = &aio_exec_impl_vtbl;

	impl->loop = loop;

	return exec;
}

void
aio_exec_fini(aio_exec_t *exec)
{
	(void)exec;
}

aio_exec_t *
aio_exec_create(aio_loop_t *loop)
{
	int errc = 0;

	aio_exec_t *exec = aio_exec_alloc();
	if (!exec) {
		errc = get_errc();
		goto error_alloc;
	}

	aio_exec_t *tmp = aio_exec_init(exec, loop);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	exec = tmp;

	return exec;

error_init:
	aio_exec_free((void *)exec);
error_alloc:
	set_errc(errc);
	return NULL;
}

void
aio_exec_destroy(aio_exec_t *exec)
{
	if (exec) {
		aio_exec_fini(exec);
		aio_exec_free((void *)exec);
	}
}

static void
aio_exec_impl_run(aio_exec_t *exec, struct aio_task *task)
{
	struct aio_exec_impl *impl = aio_impl_from_exec(exec);
	assert(task);
	assert(!task->exec || task->exec == exec);

	if (!task->exec)
		task->exec = exec;

	if (task->func) {
		struct aio_exec_node **plist = &aio_exec_list;

		struct aio_queue queue;
		aio_queue_init(&queue);
		struct aio_queue_node queue_node = { NULL, &queue };
		struct aio_exec_node exec_node = { NULL, exec, &queue_node };

		struct aio_exec_node **pnode = aio_exec_find(exec, plist);
		if (*pnode) {
			queue_node.next = (*pnode)->queue;
			(*pnode)->queue = &queue_node;
		} else {
			exec_node.next = *plist;
			*plist = &exec_node;
		}

		task->func(task);

		pnode = aio_exec_find(exec, plist);
		assert(*pnode);
		assert((*pnode)->queue == &queue_node);
		if ((*pnode)->queue->next) {
			(*pnode)->queue = (*pnode)->queue->next;
		} else {
			*pnode = (*pnode)->next;
		}

		while ((task = aio_queue_pop(&queue))) {
			assert(task->exec == exec);
			aio_loop_post(impl->loop, task);
			aio_exec_impl_on_task_finished(exec);
		}
	}

	aio_exec_impl_on_task_finished(exec);
}

static void
aio_exec_impl_dispatch(aio_exec_t *exec, struct aio_task *task)
{
	assert(task);
	assert(!task->exec || task->exec == exec);

	struct aio_exec_node **pnode = aio_exec_find(exec, &aio_exec_list);
	if (!*pnode) {
		aio_exec_impl_post(exec, task);
		return;
	}

	if (!task->exec)
		task->exec = exec;

	if (!task->func)
		return;

	task->func(task);
}

static void
aio_exec_impl_post(aio_exec_t *exec, struct aio_task *task)
{
	struct aio_exec_impl *impl = aio_impl_from_exec(exec);
	assert(task);
	assert(!task->exec || task->exec == exec);

	if (!task->exec)
		task->exec = exec;

	if (!task->func)
		return;

	aio_loop_post(impl->loop, task);
}

static void
aio_exec_impl_defer(aio_exec_t *exec, struct aio_task *task)
{
	assert(task);
	assert(!task->exec || task->exec == exec);

	struct aio_exec_node **pnode = aio_exec_find(exec, &aio_exec_list);
	if (!*pnode) {
		aio_exec_impl_post(exec, task);
		return;
	}

	if (!task->exec)
		task->exec = exec;

	if (!task->func)
		return;

	aio_exec_impl_on_task_started(exec);
	assert((*pnode)->queue);
	assert((*pnode)->queue->queue);
	aio_queue_push((*pnode)->queue->queue, task);
}

static void
aio_exec_impl_on_task_started(aio_exec_t *exec)
{
	struct aio_exec_impl *impl = aio_impl_from_exec(exec);

	aio_loop_on_task_started(impl->loop);
}

static void
aio_exec_impl_on_task_finished(aio_exec_t *exec)
{
	struct aio_exec_impl *impl = aio_impl_from_exec(exec);

	aio_loop_on_task_finished(impl->loop);
}

static inline struct aio_exec_impl *
aio_impl_from_exec(const aio_exec_t *exec)
{
	assert(exec);

	return structof(exec, struct aio_exec_impl, exec_vptr);
}

static struct aio_exec_node **
aio_exec_find(const aio_exec_t *exec, struct aio_exec_node **pnode)
{
	while (*pnode && (*pnode)->exec != exec)
		pnode = &(*pnode)->next;
	return pnode;
}
