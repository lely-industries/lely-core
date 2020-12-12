/**@file
 * This file is part of the event library; it contains the implementation of the
 * standard executor functions.
 *
 * @see lely/ev/std_exec.h
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

#include "ev.h"

#if !LELY_NO_MALLOC

#include <lely/ev/exec.h>
#include <lely/ev/std_exec.h>
#include <lely/ev/task.h>
#include <lely/util/errnum.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

static void ev_std_exec_on_task_init(ev_exec_t *exec);
static void ev_std_exec_on_task_fini(ev_exec_t *exec);
static int ev_std_exec_dispatch(ev_exec_t *exec, struct ev_task *task);
static void ev_std_exec_post(ev_exec_t *exec, struct ev_task *task);
static void ev_std_exec_defer(ev_exec_t *exec, struct ev_task *task);
static size_t ev_std_exec_abort(ev_exec_t *exec, struct ev_task *task);
static void ev_std_exec_run(ev_exec_t *exec, struct ev_task *task);

// clang-format off
static const struct ev_exec_vtbl ev_std_exec_vtbl = {
	&ev_std_exec_on_task_init,
	&ev_std_exec_on_task_fini,
	&ev_std_exec_dispatch,
	&ev_std_exec_post,
	&ev_std_exec_defer,
	&ev_std_exec_abort,
	&ev_std_exec_run
};
// clang-format on

static inline struct ev_std_exec *ev_std_exec_from_exec(const ev_exec_t *exec);

struct ev_task_node {
	struct ev_task_node *next;
	struct sllist *queue;
};

struct ev_exec_node {
	struct ev_exec_node *next;
	ev_exec_t *exec;
	struct ev_task_node *queue;
};

#if LELY_NO_THREADS
static struct ev_exec_node *ev_exec_list;
#else
static _Thread_local struct ev_exec_node *ev_exec_list;
#endif

static struct ev_exec_node **ev_exec_find(
		const ev_exec_t *exec, struct ev_exec_node **pnode);

void *
ev_std_exec_alloc(void)
{
	struct ev_std_exec *exec = malloc(sizeof(*exec));
#if !LELY_NO_ERRNO
	if (!exec)
		set_errc(errno2c(errno));
#endif
	// cppcheck-suppress memleak symbolName=exec
	return exec ? &exec->exec_vptr : NULL;
}

void
ev_std_exec_free(void *ptr)
{
	if (ptr)
		free(ev_std_exec_from_exec(ptr));
}

ev_exec_t *
ev_std_exec_init(ev_exec_t *exec_, ev_std_exec_impl_t *impl)
{
	struct ev_std_exec *exec = ev_std_exec_from_exec(exec_);
	assert(impl);

	exec->exec_vptr = &ev_std_exec_vtbl;

	exec->impl = impl;

	return exec_;
}

void
ev_std_exec_fini(ev_exec_t *exec)
{
	(void)exec;
}

ev_exec_t *
ev_std_exec_create(ev_std_exec_impl_t *impl)
{
	int errc = 0;

	ev_exec_t *exec = ev_std_exec_alloc();
	if (!exec) {
		errc = get_errc();
		goto error_alloc;
	}

	ev_exec_t *tmp = ev_std_exec_init(exec, impl);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	exec = tmp;

	return exec;

error_init:
	ev_std_exec_free((void *)exec);
error_alloc:
	set_errc(errc);
	return NULL;
}

void
ev_std_exec_destroy(ev_exec_t *exec)
{
	if (exec) {
		ev_std_exec_fini(exec);
		ev_std_exec_free((void *)exec);
	}
}

static void
ev_std_exec_on_task_init(ev_exec_t *exec_)
{
	struct ev_std_exec *exec = ev_std_exec_from_exec(exec_);
	assert(exec->impl);
	assert((*exec->impl)->on_task_init);

	(*exec->impl)->on_task_init(exec->impl);
}

static void
ev_std_exec_on_task_fini(ev_exec_t *exec_)
{
	struct ev_std_exec *exec = ev_std_exec_from_exec(exec_);
	assert(exec->impl);
	assert((*exec->impl)->on_task_fini);

	(*exec->impl)->on_task_fini(exec->impl);
}

static int
ev_std_exec_dispatch(ev_exec_t *exec, struct ev_task *task)
{
	assert(task);
	assert(!task->exec || task->exec == exec);

	struct ev_exec_node **pnode = ev_exec_find(exec, &ev_exec_list);
	if (!*pnode) {
		ev_std_exec_post(exec, task);
		return 0;
	}

	if (!task->exec)
		task->exec = exec;

	if (task->func)
		task->func(task);

	return 1;
}

static void
ev_std_exec_post(ev_exec_t *exec_, struct ev_task *task)
{
	struct ev_std_exec *exec = ev_std_exec_from_exec(exec_);
	assert(exec->impl);
	assert((*exec->impl)->post);
	assert(task);
	assert(!task->exec || task->exec == exec_);

	if (!task->exec)
		task->exec = exec_;

	(*exec->impl)->post(exec->impl, task);
}

static void
ev_std_exec_defer(ev_exec_t *exec, struct ev_task *task)
{
	assert(exec);
	assert(task);
	assert(!task->exec || task->exec == exec);

	struct ev_exec_node **pnode = ev_exec_find(exec, &ev_exec_list);
	if (!*pnode) {
		ev_std_exec_post(exec, task);
		return;
	}

	if (!task->exec)
		task->exec = exec;

	ev_std_exec_on_task_init(exec);
	assert((*pnode)->queue);
	assert((*pnode)->queue->queue);
	sllist_push_back((*pnode)->queue->queue, &task->_node);
}

static size_t
ev_std_exec_abort(ev_exec_t *exec_, struct ev_task *task)
{
	struct ev_std_exec *exec = ev_std_exec_from_exec(exec_);
	assert(exec->impl);
	assert((*exec->impl)->abort);

	size_t nnode = 0;
	size_t nimpl = 0;

	struct ev_exec_node **pnode = ev_exec_find(exec_, &ev_exec_list);
	if (*pnode) {
		assert((*pnode)->queue);
		struct sllist *queue = (*pnode)->queue->queue;
		assert(queue);
		if (!task) {
			while (sllist_pop_front(queue)) {
				ev_std_exec_on_task_fini(exec_);
				nnode += nnode < SIZE_MAX;
			}
		} else if (sllist_remove(queue, &task->_node)) {
			ev_std_exec_on_task_fini(exec_);
			nnode++;
		}
	}

	if (!task || !nnode)
		nimpl = (*exec->impl)->abort(exec->impl, task);

	return nimpl < SIZE_MAX - nnode ? nnode + nimpl : SIZE_MAX;
}

static void
ev_std_exec_run(ev_exec_t *exec_, struct ev_task *task)
{
	struct ev_std_exec *exec = ev_std_exec_from_exec(exec_);
	assert(exec->impl);
	assert((*exec->impl)->post);
	assert(task);
	assert(!task->exec || task->exec == exec_);

	if (!task->exec)
		task->exec = exec_;

	if (task->func) {
		struct ev_exec_node **plist = &ev_exec_list;

		struct sllist queue;
		sllist_init(&queue);
		struct ev_task_node queue_node = { NULL, &queue };
		struct ev_exec_node exec_node = { NULL, exec_, &queue_node };

		struct ev_exec_node **pnode = ev_exec_find(exec_, plist);
		if (*pnode) {
			queue_node.next = (*pnode)->queue;
			(*pnode)->queue = &queue_node;
		} else {
			exec_node.next = *plist;
			*plist = &exec_node;
		}

		task->func(task);

		pnode = ev_exec_find(exec_, plist);
		assert(*pnode);
		assert((*pnode)->queue == &queue_node);
		if ((*pnode)->queue->next) {
			(*pnode)->queue = (*pnode)->queue->next;
		} else {
			*pnode = (*pnode)->next;
		}

		while ((task = ev_task_from_node(sllist_pop_front(&queue)))) {
			assert(task->exec == exec_);
			(*exec->impl)->post(exec->impl, task);
			ev_std_exec_on_task_fini(exec_);
		}
	}
}

static inline struct ev_std_exec *
ev_std_exec_from_exec(const ev_exec_t *exec)
{
	assert(exec);

	return structof(exec, struct ev_std_exec, exec_vptr);
}

static struct ev_exec_node **
ev_exec_find(const ev_exec_t *exec, struct ev_exec_node **pnode)
{
	while (*pnode && (*pnode)->exec != exec)
		pnode = &(*pnode)->next;
	return pnode;
}

#endif // !LELY_NO_MALLOC
