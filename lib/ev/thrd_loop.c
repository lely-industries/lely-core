/**@file
 * This file is part of the event library; it contains the implementation of the
 * thread-local event loop functions.
 *
 * @see lely/ev/thrd_loop.h
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
#include <lely/ev/exec.h>
#include <lely/ev/task.h>
#include <lely/ev/thrd_loop.h>

#include <assert.h>
#include <stdint.h>

static void ev_thrd_loop_exec_on_task_init(ev_exec_t *exec);
static void ev_thrd_loop_exec_on_task_fini(ev_exec_t *exec);
static int ev_thrd_loop_exec_dispatch(ev_exec_t *exec, struct ev_task *task);
static void ev_thrd_loop_exec_defer(ev_exec_t *exec, struct ev_task *task);
static size_t ev_thrd_loop_exec_abort(ev_exec_t *exec, struct ev_task *task);
static void ev_thrd_loop_exec_run(ev_exec_t *exec, struct ev_task *task);

// clang-format off
static const struct ev_exec_vtbl ev_thrd_loop_exec_vtbl = {
	&ev_thrd_loop_exec_on_task_init,
	&ev_thrd_loop_exec_on_task_fini,
	&ev_thrd_loop_exec_dispatch,
	&ev_thrd_loop_exec_defer,
	&ev_thrd_loop_exec_defer,
	&ev_thrd_loop_exec_abort,
	&ev_thrd_loop_exec_run
};
// clang-format on

/// A thread-local event loop.
struct ev_thrd_loop {
	/// The queue of pending tasks.
	struct sllist queue;
	/**
	 * The number of pending tasks. This equals the number tasks in #queue
	 * plus the number of calls to ev_exec_on_task_init() minus those to
	 * ev_exec_on_task_init(). ev_thrd_loop_stop() is called once this value
	 * reaches 0.
	 */
	size_t ntasks;
	/// A flag specifying whether the event loop is stopped.
	int stopped;
	/// A flag specifying whether ev_exec_run() is running on this thread.
	int running;
};

/// Returns a pointer to the thread-local event loop.
static struct ev_thrd_loop *ev_thrd_loop(void);

static void ev_thrd_loop_on_task_init(struct ev_thrd_loop *loop);
static void ev_thrd_loop_on_task_fini(struct ev_thrd_loop *loop);

ev_exec_t *
ev_thrd_loop_get_exec(void)
{
	static ev_exec_t ev_thrd_loop_exec = &ev_thrd_loop_exec_vtbl;

	return &ev_thrd_loop_exec;
}

void
ev_thrd_loop_stop(void)
{
	ev_thrd_loop()->stopped = 1;
}

int
ev_thrd_loop_stopped(void)
{
	return ev_thrd_loop()->stopped;
}

void
ev_thrd_loop_restart(void)
{
	ev_thrd_loop()->stopped = 0;
}

size_t
ev_thrd_loop_run(void)
{
	size_t n = 0;
	while (ev_thrd_loop_run_one())
		n += n < SIZE_MAX;
	return n;
}

size_t
ev_thrd_loop_run_one(void)
{
	struct ev_thrd_loop *loop = ev_thrd_loop();

	if (loop->stopped)
		return 0;

	struct ev_task *task =
			ev_task_from_node(sllist_pop_front(&loop->queue));
	if (!task) {
		if (!loop->ntasks)
			loop->stopped = 1;
		return 0;
	}

	assert(task->exec);
	ev_exec_run(task->exec, task);

	ev_thrd_loop_on_task_fini(ev_thrd_loop());

	return 1;
}

static void
ev_thrd_loop_exec_on_task_init(ev_exec_t *exec)
{
	assert(exec == ev_thrd_loop_get_exec());
	(void)exec;

	ev_thrd_loop_on_task_init(ev_thrd_loop());
}

static void
ev_thrd_loop_exec_on_task_fini(ev_exec_t *exec)
{
	assert(exec == ev_thrd_loop_get_exec());
	(void)exec;

	ev_thrd_loop_on_task_fini(ev_thrd_loop());
}

static int
ev_thrd_loop_exec_dispatch(ev_exec_t *exec, struct ev_task *task)
{
	assert(exec == ev_thrd_loop_get_exec());
	assert(task);

	if (!task->exec)
		task->exec = exec;
	assert(task->exec == exec);

	struct ev_thrd_loop *loop = ev_thrd_loop();
	ev_thrd_loop_on_task_init(loop);
	if (loop->running) {
		ev_thrd_loop_exec_run(exec, task);
		ev_thrd_loop_on_task_fini(loop);
		return 1;
	} else {
		sllist_push_back(&loop->queue, &task->_node);
		return 0;
	}
}

static void
ev_thrd_loop_exec_defer(ev_exec_t *exec, struct ev_task *task)
{
	assert(exec == ev_thrd_loop_get_exec());
	assert(task);

	if (!task->exec)
		task->exec = exec;
	assert(task->exec == exec);

	struct ev_thrd_loop *loop = ev_thrd_loop();
	ev_thrd_loop_on_task_init(loop);
	sllist_push_back(&loop->queue, &task->_node);
}

static size_t
ev_thrd_loop_exec_abort(ev_exec_t *exec, struct ev_task *task)
{
	assert(exec == ev_thrd_loop_get_exec());
	(void)exec;

	struct ev_thrd_loop *loop = ev_thrd_loop();

	size_t n = 0;
	if (!task) {
		while (sllist_pop_front(&loop->queue)) {
			ev_thrd_loop_on_task_fini(loop);
			n += n < SIZE_MAX;
		}
	} else if (sllist_remove(&loop->queue, &task->_node)) {
		ev_thrd_loop_on_task_fini(loop);
		n++;
	}
	return n;
}

static void
ev_thrd_loop_exec_run(ev_exec_t *exec, struct ev_task *task)
{
	assert(exec == ev_thrd_loop_get_exec());
	assert(task);

	if (!task->exec)
		task->exec = exec;
	assert(task->exec == exec);

	struct ev_thrd_loop *loop = ev_thrd_loop();

	if (task->func) {
		int running = loop->running;
		loop->running = 1;

		task->func(task);

		// cppcheck-suppress redundantAssignment
		loop->running = running;
	}
}

static struct ev_thrd_loop *
ev_thrd_loop(void)
{
#if LELY_NO_THREADS
	static struct ev_thrd_loop *loop;
#else
	static _Thread_local struct ev_thrd_loop *loop;
#endif
	if (!loop) {
#if LELY_NO_THREADS
		static struct ev_thrd_loop loop_;
#else
		static _Thread_local struct ev_thrd_loop loop_;
#endif
		sllist_init(&loop_.queue);
		loop_.ntasks = 0;
		loop_.stopped = 0;
		loop_.running = 0;
		loop = &loop_;
	}
	return loop;
}

static void
ev_thrd_loop_on_task_init(struct ev_thrd_loop *loop)
{
	assert(loop);

	loop->ntasks++;
}

static void
ev_thrd_loop_on_task_fini(struct ev_thrd_loop *loop)
{
	assert(loop);
	assert(loop->ntasks);

	if (!--loop->ntasks)
		loop->stopped = 1;
}
