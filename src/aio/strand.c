/**@file
 * This file is part of the asynchronous I/O library; it contains ...
 *
 * @see lely/aio/strand.h
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
#if !LELY_NO_THREADS
#include <lely/libc/threads.h>
#endif
#include <lely/aio/queue.h>
#include <lely/aio/strand.h>
#include <lely/util/errnum.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdlib.h>

static void aio_strand_exec_dispatch(aio_exec_t *exec, struct aio_task *task);
static void aio_strand_exec_post(aio_exec_t *exec, struct aio_task *task);
static void aio_strand_exec_defer(aio_exec_t *exec, struct aio_task *task);
static void aio_strand_on_task_started(aio_exec_t *exec);
static void aio_strand_on_task_finished(aio_exec_t *exec);

// clang-format off
static const struct aio_exec_vtbl aio_strand_exec_vtbl = {
	NULL,
	&aio_strand_exec_dispatch,
	&aio_strand_exec_post,
	&aio_strand_exec_defer,
	&aio_strand_on_task_started,
	&aio_strand_on_task_finished
};
// clang-format on

struct aio_strand {
	const struct aio_exec_vtbl *exec_vptr;
	aio_exec_t *inner_exec;
	struct aio_task task;
#if !LELY_NO_THREADS
	mtx_t mtx;
#endif
	int running;
	const int *thread;
	struct aio_queue queue;
};

static void aio_strand_func(struct aio_task *task);

static inline struct aio_strand *aio_strand_from_exec(const aio_exec_t *exec);

#if LELY_NO_THREADS
static const int aio_strand_thread;
#else
static _Thread_local const int aio_strand_thread;
#endif

void *
aio_strand_alloc(void)
{
	struct aio_strand *strand = malloc(sizeof(*strand));
	if (!strand)
		set_errc(errno2c(errno));
	return strand ? &strand->exec_vptr : NULL;
}

void
aio_strand_free(void *ptr)
{
	if (ptr)
		free(aio_strand_from_exec(ptr));
}

aio_exec_t *
aio_strand_init(aio_exec_t *exec, aio_exec_t *inner_exec)
{
	struct aio_strand *strand = aio_strand_from_exec(exec);
	assert(inner_exec);

	strand->exec_vptr = &aio_strand_exec_vtbl;

	strand->inner_exec = inner_exec;
	strand->task = (struct aio_task)AIO_TASK_INIT(
			strand->inner_exec, &aio_strand_func);

#if !LELY_NO_THREADS
	if (mtx_init(&strand->mtx, mtx_plain) != thrd_success)
		return NULL;
#endif

	strand->running = 0;
	strand->thread = NULL;
	aio_queue_init(&strand->queue);

	return exec;
}

void
aio_strand_fini(aio_exec_t *exec)
{
	struct aio_strand *strand = aio_strand_from_exec(exec);

#if LELY_NO_THREADS
	(void)strand;
#else
	mtx_destroy(&strand->mtx);
#endif
}

aio_exec_t *
aio_strand_create(aio_exec_t *inner_exec)
{
	int errc = 0;

	aio_exec_t *exec = aio_strand_alloc();
	if (!exec) {
		errc = get_errc();
		goto error_alloc;
	}

	aio_exec_t *tmp = aio_strand_init(exec, inner_exec);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	exec = tmp;

	return exec;

error_alloc:
	aio_strand_free((void *)exec);
error_init:
	set_errc(errc);
	return NULL;
}

void
aio_strand_destroy(aio_exec_t *exec)
{
	if (exec) {
		aio_strand_fini(exec);
		aio_strand_free((void *)exec);
	}
}

aio_exec_t *
aio_strand_get_inner_exec(const aio_exec_t *exec)
{
	const struct aio_strand *strand = aio_strand_from_exec(exec);

	return strand->inner_exec;
}

static void
aio_strand_exec_dispatch(aio_exec_t *exec, struct aio_task *task)
{
	struct aio_strand *strand = aio_strand_from_exec(exec);
	assert(task);
	assert(!task->exec || task->exec == exec);

	if (!task->exec)
		task->exec = exec;
	aio_strand_on_task_started(exec);

#if !LELY_NO_THREADS
	mtx_lock(&strand->mtx);
#endif
	if (strand->thread == &aio_strand_thread) {
#if !LELY_NO_THREADS
		mtx_unlock(&strand->mtx);
#endif
		if (task->func)
			task->func(task);
		aio_strand_on_task_finished(exec);
	} else {
		aio_queue_push(&strand->queue, task);
		if (!strand->running) {
			strand->running = 1;
			aio_exec_dispatch(strand->inner_exec, &strand->task);
		}
#if !LELY_NO_THREADS
		mtx_unlock(&strand->mtx);
#endif
	}
}

static void
aio_strand_exec_post(aio_exec_t *exec, struct aio_task *task)
{
	struct aio_strand *strand = aio_strand_from_exec(exec);
	assert(task);
	assert(!task->exec || task->exec == exec);

	if (!task->exec)
		task->exec = exec;
	aio_strand_on_task_started(exec);

#if !LELY_NO_THREADS
	mtx_lock(&strand->mtx);
#endif
	aio_queue_push(&strand->queue, task);
	if (!strand->running) {
		strand->running = 1;
		aio_exec_post(strand->inner_exec, &strand->task);
	}
#if !LELY_NO_THREADS
	mtx_unlock(&strand->mtx);
#endif
}

static void
aio_strand_exec_defer(aio_exec_t *exec, struct aio_task *task)
{
	struct aio_strand *strand = aio_strand_from_exec(exec);
	assert(task);
	assert(!task->exec || task->exec == exec);

	if (!task->exec)
		task->exec = exec;
	aio_strand_on_task_started(exec);

#if !LELY_NO_THREADS
	mtx_lock(&strand->mtx);
#endif
	aio_queue_push(&strand->queue, task);
	if (!strand->running) {
		strand->running = 1;
		aio_exec_defer(strand->inner_exec, &strand->task);
	}
#if !LELY_NO_THREADS
	mtx_unlock(&strand->mtx);
#endif
}

static void
aio_strand_on_task_started(aio_exec_t *exec)
{
	aio_exec_on_task_started(aio_strand_get_inner_exec(exec));
}

static void
aio_strand_on_task_finished(aio_exec_t *exec)
{
	aio_exec_on_task_finished(aio_strand_get_inner_exec(exec));
}

static void
aio_strand_func(struct aio_task *task)
{
	assert(task);
	struct aio_strand *strand = structof(task, struct aio_strand, task);
	aio_exec_t *exec = &strand->exec_vptr;

#if !LELY_NO_THREADS
	mtx_lock(&strand->mtx);
#endif
	task = aio_queue_pop(&strand->queue);
	assert(!strand->thread);
	strand->thread = &aio_strand_thread;
#if !LELY_NO_THREADS
	mtx_unlock(&strand->mtx);
#endif

	assert(task);
	assert(task->exec == exec);
	if (task->func)
		task->func(task);
	aio_strand_on_task_finished(exec);

#if !LELY_NO_THREADS
	mtx_lock(&strand->mtx);
#endif
	assert(strand->thread == &aio_strand_thread);
	strand->thread = NULL;
	assert(strand->running == 1);
	if (aio_queue_empty(&strand->queue))
		strand->running = 0;
	else
		aio_exec_defer(strand->inner_exec, &strand->task);
#if !LELY_NO_THREADS
	mtx_unlock(&strand->mtx);
#endif
}

static inline struct aio_strand *
aio_strand_from_exec(const aio_exec_t *exec)
{
	assert(exec);

	return structof(exec, struct aio_strand, exec_vptr);
}
