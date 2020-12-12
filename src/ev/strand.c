/**@file
 * This file is part of the event library; it contains the implementation of the
 * strand executor functions.
 *
 * @see lely/ev/strand.h
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

#if !LELY_NO_THREADS
#include <lely/libc/stdatomic.h>
#include <lely/libc/threads.h>
#endif
#include <lely/ev/exec.h>
#include <lely/ev/strand.h>
#include <lely/ev/task.h>
#include <lely/util/errnum.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

static void ev_strand_exec_on_task_init(ev_exec_t *exec);
static void ev_strand_exec_on_task_fini(ev_exec_t *exec);
static int ev_strand_exec_dispatch(ev_exec_t *exec, struct ev_task *task);
static void ev_strand_exec_defer(ev_exec_t *exec, struct ev_task *task);
static size_t ev_strand_exec_abort(ev_exec_t *exec, struct ev_task *task);

// clang-format off
static const struct ev_exec_vtbl ev_strand_exec_vtbl = {
	&ev_strand_exec_on_task_init,
	&ev_strand_exec_on_task_fini,
	&ev_strand_exec_dispatch,
	&ev_strand_exec_defer,
	&ev_strand_exec_defer,
	&ev_strand_exec_abort,
	NULL
};
// clang-format on

struct ev_strand {
	const struct ev_exec_vtbl *exec_vptr;
	ev_exec_t *inner_exec;
	struct ev_task task;
#if !LELY_NO_THREADS
	mtx_t mtx;
#endif
	int posted;
	struct sllist queue;
	const int *thr;
};

static void ev_strand_func(struct ev_task *task);

static inline struct ev_strand *ev_strand_from_exec(const ev_exec_t *exec);

#if LELY_NO_THREADS
static const int ev_strand_thrd;
#else
static _Thread_local const int ev_strand_thrd;
#endif

void *
ev_strand_alloc(void)
{
	struct ev_strand *strand = malloc(sizeof(*strand));
#if !LELY_NO_ERRNO
	if (!strand)
		set_errc(errno2c(errno));
#endif
	// cppcheck-suppress memleak symbolName=strand
	return strand ? &strand->exec_vptr : NULL;
}

void
ev_strand_free(void *ptr)
{
	free(ptr);
}

ev_exec_t *
ev_strand_init(ev_exec_t *exec, ev_exec_t *inner_exec)
{
	struct ev_strand *strand = ev_strand_from_exec(exec);
	assert(inner_exec);

	strand->exec_vptr = &ev_strand_exec_vtbl;

	strand->inner_exec = inner_exec;
	strand->task = (struct ev_task)EV_TASK_INIT(
			strand->inner_exec, &ev_strand_func);

#if !LELY_NO_THREADS
	if (mtx_init(&strand->mtx, mtx_plain) != thrd_success)
		return NULL;
#endif

	strand->posted = 0;

	sllist_init(&strand->queue);

	strand->thr = NULL;

	return exec;
}

void
ev_strand_fini(ev_exec_t *exec)
{
	struct ev_strand *strand = ev_strand_from_exec(exec);

	ev_strand_exec_abort(exec, NULL);

#if !LELY_NO_THREADS
	mtx_lock(&strand->mtx);
#endif
	// Abort ev_strand_func().
	if (strand->posted && ev_exec_abort(strand->task.exec, &strand->task))
		strand->posted = 0;
#if !LELY_NO_THREADS
	// If necessary, busy-wait until ev_strand_func() completes.
	while (strand->posted) {
		mtx_unlock(&strand->mtx);
		thrd_yield();
		mtx_lock(&strand->mtx);
	}
	mtx_unlock(&strand->mtx);

	mtx_destroy(&strand->mtx);
#endif
}

ev_exec_t *
ev_strand_create(ev_exec_t *inner_exec)
{
	int errc = 0;

	ev_exec_t *exec = ev_strand_alloc();
	if (!exec) {
		errc = get_errc();
		goto error_alloc;
	}

	ev_exec_t *tmp = ev_strand_init(exec, inner_exec);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	exec = tmp;

	return exec;

error_alloc:
	ev_strand_free((void *)exec);
error_init:
	set_errc(errc);
	return NULL;
}

void
ev_strand_destroy(ev_exec_t *exec)
{
	if (exec) {
		ev_strand_fini(exec);
		ev_strand_free((void *)exec);
	}
}

ev_exec_t *
ev_strand_get_inner_exec(const ev_exec_t *exec)
{
	struct ev_strand *strand = ev_strand_from_exec(exec);

	return strand->inner_exec;
}

static void
ev_strand_exec_on_task_init(ev_exec_t *exec)
{
	struct ev_strand *strand = ev_strand_from_exec(exec);

	ev_exec_on_task_init(strand->inner_exec);
}

static void
ev_strand_exec_on_task_fini(ev_exec_t *exec)
{
	struct ev_strand *strand = ev_strand_from_exec(exec);

	ev_exec_on_task_fini(strand->inner_exec);
}

static int
ev_strand_exec_dispatch(ev_exec_t *exec, struct ev_task *task)
{
	struct ev_strand *strand = ev_strand_from_exec(exec);
	assert(task);
	assert(!task->exec || task->exec == exec);

	if (!task->exec)
		task->exec = exec;
	ev_strand_exec_on_task_init(exec);

#if !LELY_NO_THREADS
	mtx_lock(&strand->mtx);
#endif
	if (strand->thr == &ev_strand_thrd) {
#if !LELY_NO_THREADS
		mtx_unlock(&strand->mtx);
#endif
		if (task->func)
			task->func(task);
		ev_strand_exec_on_task_fini(exec);
		return 1;
	} else {
		sllist_push_back(&strand->queue, &task->_node);
		int post = !strand->posted;
		strand->posted = 1;
#if !LELY_NO_THREADS
		mtx_unlock(&strand->mtx);
#endif
		if (post)
			ev_exec_post(strand->task.exec, &strand->task);
		return 0;
	}
}

static void
ev_strand_exec_defer(ev_exec_t *exec, struct ev_task *task)
{
	struct ev_strand *strand = ev_strand_from_exec(exec);
	assert(task);
	assert(!task->exec || task->exec == exec);

	if (!task->exec)
		task->exec = exec;
	ev_strand_exec_on_task_init(exec);

#if !LELY_NO_THREADS
	mtx_lock(&strand->mtx);
#endif
	sllist_push_back(&strand->queue, &task->_node);
	int post = !strand->posted;
	strand->posted = 1;
#if !LELY_NO_THREADS
	mtx_unlock(&strand->mtx);
#endif
	if (post)
		ev_exec_post(strand->task.exec, &strand->task);
}

static size_t
ev_strand_exec_abort(ev_exec_t *exec, struct ev_task *task)
{
	struct ev_strand *strand = ev_strand_from_exec(exec);

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&strand->mtx);
#endif
	if (!task)
		sllist_append(&queue, &strand->queue);
	else if (sllist_remove(&strand->queue, &task->_node))
		sllist_push_back(&queue, &task->_node);
#if !LELY_NO_THREADS
	mtx_unlock(&strand->mtx);
#endif

	size_t n = 0;
	while (sllist_pop_front(&queue)) {
		ev_strand_exec_on_task_fini(exec);
		n += n < SIZE_MAX;
	}
	return n;
}

static void
ev_strand_func(struct ev_task *task)
{
	assert(task);
	struct ev_strand *strand = structof(task, struct ev_strand, task);
	ev_exec_t *exec = &strand->exec_vptr;

#if !LELY_NO_THREADS
	mtx_lock(&strand->mtx);
#endif
	task = ev_task_from_node(sllist_pop_front(&strand->queue));
	if (task) {
		assert(!strand->thr);
		strand->thr = &ev_strand_thrd;
#if !LELY_NO_THREADS
		mtx_unlock(&strand->mtx);
#endif
		assert(task->exec == exec);
		if (task->func)
			task->func(task);
		ev_strand_exec_on_task_fini(exec);
#if !LELY_NO_THREADS
		mtx_lock(&strand->mtx);
#endif
		assert(strand->thr == &ev_strand_thrd);
		strand->thr = NULL;
	}
	assert(strand->posted == 1);
	int post = strand->posted = !sllist_empty(&strand->queue);
#if !LELY_NO_THREADS
	mtx_unlock(&strand->mtx);
#endif
	if (post)
		ev_exec_post(strand->task.exec, &strand->task);
}

static inline struct ev_strand *
ev_strand_from_exec(const ev_exec_t *exec)
{
	assert(exec);

	return structof(exec, struct ev_strand, exec_vptr);
}

#endif // !LELY_NO_MALLOC
