/**@file
 * This file is part of the event library; it contains the implementation of the
 * polling event loop functions.
 *
 * @see lely/ev/loop.h
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
#define LELY_EV_LOOP_INLINE extern inline
#include <lely/ev/loop.h>
#include <lely/ev/std_exec.h>
#include <lely/ev/task.h>
#include <lely/util/dllist.h>
#include <lely/util/errnum.h>
#include <lely/util/time.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef LELY_EV_LOOP_CTX_MAX_UNUSED
/// The maximum number of unused contexts per event loop.
#define LELY_EV_LOOP_CTX_MAX_UNUSED 16
#endif

/// An event loop context.
struct ev_loop_ctx {
	/**
	 * The number of references to this context. Once the reference count
	 * reaches zero, this struct is reclaimed.
	 */
	size_t refcnt;
	/// A pointer to the event loop managing this context.
	ev_loop_t *loop;
	/// The future on which the loop is waiting.
	ev_future_t *future;
	/// The task to be executed once the future is ready.
	struct ev_task task;
	/// The address of the stopped flag of the thread.
	int *pstopped;
#if !LELY_NO_THREADS
	/**
	 * The condition variable used by threads to wait for a task to be
	 * submitted to the event loop or for the event loop to be stopped or
	 * interrupted.
	 */
	cnd_t cond;
	/// A flag indicating if a thread is waiting on #cond.
	unsigned waiting : 1;
#endif
	/// A flag indicating if #future is ready.
	unsigned ready : 1;
	/// A flag indicating if a thread is polling.
	unsigned polling : 1;
	/// The thread identifier of the polling instance.
	void *thr;
	/// The node of this context in the list of waiting or polling contexts.
	struct dlnode node;
	/**
	 * A pointer to the next context in the list of running or unused
	 * contexts.
	 */
	struct ev_loop_ctx *next;
};

static void ev_loop_ctx_task_func(struct ev_task *task);

static struct ev_loop_ctx *ev_loop_ctx_alloc(void);
static void ev_loop_ctx_free(struct ev_loop_ctx *ctx);

static void ev_loop_ctx_release(struct ev_loop_ctx *ctx);

static struct ev_loop_ctx *ev_loop_ctx_create(
		ev_loop_t *loop, ev_future_t *future);
static void ev_loop_ctx_destroy(struct ev_loop_ctx *ctx);

static size_t ev_loop_ctx_wait_one(struct ev_loop_ctx **pctx, ev_loop_t *loop,
		ev_future_t *future);
static size_t ev_loop_ctx_wait_one_until(struct ev_loop_ctx **pctx,
		ev_loop_t *loop, ev_future_t *future,
		const struct timespec *abs_time);

static int ev_loop_ctx_kill(struct ev_loop_ctx *ctx, int stop);

/// An event loop thread.
struct ev_loop_thrd {
	/// A flag used to interrupt the event loop on this thread.
	int stopped;
	/// A pointer to the event loop context for this thread.
	struct ev_loop_ctx *ctx;
};

#if LELY_NO_THREADS
static struct ev_loop_thrd ev_loop_thrd = { 0, NULL };
#else
static _Thread_local struct ev_loop_thrd ev_loop_thrd = { 0, NULL };
#endif

static void ev_loop_std_exec_impl_on_task_init(ev_std_exec_impl_t *impl);
static void ev_loop_std_exec_impl_on_task_fini(ev_std_exec_impl_t *impl);
static void ev_loop_std_exec_impl_post(
		ev_std_exec_impl_t *impl, struct ev_task *task);
static size_t ev_loop_std_exec_impl_abort(
		ev_std_exec_impl_t *impl, struct ev_task *task);

// clang-format off
static const struct ev_std_exec_impl_vtbl ev_loop_std_exec_impl_vtbl = {
	&ev_loop_std_exec_impl_on_task_init,
	&ev_loop_std_exec_impl_on_task_fini,
	&ev_loop_std_exec_impl_post,
	&ev_loop_std_exec_impl_abort
};
// clang-format on

/// A polling event loop.
struct ev_loop {
	/// A pointer to the interface used to poll for events (can be `NULL`).
	ev_poll_t *poll;
	/**
	 * The number of threads allowed to poll simultaneously. If <b>npoll</b>
	 * is 0, there is no limit.
	 */
	size_t npoll;
	/**
	 * A pointer to the virtual table containing the interface used by the
	 * standard executor (#exec).
	 */
	const struct ev_std_exec_impl_vtbl *impl_vptr;
	/// The executor corresponding to the event loop.
	struct ev_std_exec exec;
#if !LELY_NO_THREADS
	/// The mutex protecting the task queue.
	mtx_t mtx;
#endif
	/// The queue of pending tasks.
	struct sllist queue;
	/// The task used to trigger polling.
	struct ev_task task;
	/**
	 * The number of pending tasks. This equals the number tasks in #queue
	 * plus the number of calls to ev_exec_on_task_init() minus those to
	 * ev_exec_on_task_fini(). ev_loop_stop() is called once this value
	 * reaches 0.
	 */
#if LELY_NO_THREADS || (LELY_NO_ATOMICS && (!_WIN32 || defined(__MINGW32__)))
	size_t ntasks;
#elif _WIN64 && !defined(__MINGW32__)
	volatile LONGLONG ntasks;
#elif _WIN32 && !defined(__MINGW32__)
	volatile LONG ntasks;
#else
	atomic_size_t ntasks;
#endif
	/// A flag specifying whether the event loop is stopped.
	int stopped;
#if !LELY_NO_THREADS
	/// The list of waiting contexts.
	struct dllist waiting;
#endif
	/// The list of polling contexts.
	struct dllist polling;
	/// The number of polling contexts.
	size_t npolling;
	/// The list of unused contexts.
	struct ev_loop_ctx *unused;
	/**
	 * The number of unused contexts. This WILL NOT exceed
	 * #LELY_EV_LOOP_CTX_MAX_UNUSED.
	 */
	size_t nunused;
};

static inline ev_loop_t *ev_loop_from_impl(const ev_std_exec_impl_t *impl);

static int ev_loop_empty(const ev_loop_t *loop);
static size_t ev_loop_ntasks(const ev_loop_t *loop);

static void ev_loop_do_stop(ev_loop_t *loop);

static int ev_loop_kill_any(ev_loop_t *loop, int polling);

void *
ev_loop_alloc(void)
{
	void *ptr = malloc(sizeof(ev_loop_t));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
ev_loop_free(void *ptr)
{
	free(ptr);
}

ev_loop_t *
ev_loop_init(ev_loop_t *loop, ev_poll_t *poll, size_t npoll, int poll_task)
{
	assert(loop);

	loop->poll = poll;
	loop->npoll = npoll;

	loop->impl_vptr = &ev_loop_std_exec_impl_vtbl;
	ev_std_exec_init(ev_loop_get_exec(loop), &loop->impl_vptr);

#if !LELY_NO_THREADS
	if (mtx_init(&loop->mtx, mtx_plain) != thrd_success)
		return NULL;
#endif

	sllist_init(&loop->queue);

	loop->task = (struct ev_task)EV_TASK_INIT(NULL, NULL);
	if (loop->poll && poll_task)
		sllist_push_back(&loop->queue, &loop->task._node);

#if LELY_NO_THREADS || LELY_NO_ATOMICS || (_WIN32 && !defined(__MINGW32__))
	loop->ntasks = 0;
#else
	atomic_init(&loop->ntasks, 0);
#endif

	loop->stopped = 0;

#if !LELY_NO_THREADS
	dllist_init(&loop->waiting);
#endif
	dllist_init(&loop->polling);
	loop->npolling = 0;

	loop->unused = NULL;
	loop->nunused = 0;

	return loop;
}

void
ev_loop_fini(ev_loop_t *loop)
{
	assert(loop);

	while (loop->unused) {
		struct ev_loop_ctx *ctx = loop->unused;
		loop->unused = ctx->next;
		ev_loop_ctx_free(ctx);
	}

	assert(!loop->npolling);
	assert(dllist_empty(&loop->polling));
#if !LELY_NO_THREADS
	assert(dllist_empty(&loop->waiting));
#endif

#if !LELY_NO_THREADS
	mtx_destroy(&loop->mtx);
#endif

	ev_std_exec_fini(ev_loop_get_exec(loop));
}

ev_loop_t *
ev_loop_create(ev_poll_t *poll, size_t npoll, int poll_task)
{
	int errc = 0;

	ev_loop_t *loop = ev_loop_alloc();
	if (!loop) {
		errc = get_errc();
		goto error_alloc;
	}

	ev_loop_t *tmp = ev_loop_init(loop, poll, npoll, poll_task);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	loop = tmp;

	return loop;

error_init:
	ev_loop_free(loop);
error_alloc:
	set_errc(errc);
	return NULL;
}

void
ev_loop_destroy(ev_loop_t *loop)
{
	if (loop) {
		ev_loop_fini(loop);
		ev_loop_free(loop);
	}
}

ev_poll_t *
ev_loop_get_poll(const ev_loop_t *loop)
{
	assert(loop);

	return loop->poll;
}

ev_exec_t *
ev_loop_get_exec(const ev_loop_t *loop)
{
	assert(loop);

	return &loop->exec.exec_vptr;
}

void
ev_loop_stop(ev_loop_t *loop)
{
	assert(loop);

#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	ev_loop_do_stop(loop);
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif
}

int
ev_loop_stopped(const ev_loop_t *loop)
{
	assert(loop);

#if !LELY_NO_THREADS
	mtx_lock((mtx_t *)&loop->mtx);
#endif
	int stopped = loop->stopped;
#if !LELY_NO_THREADS
	mtx_unlock((mtx_t *)&loop->mtx);
#endif
	return stopped;
}

void
ev_loop_restart(ev_loop_t *loop)
{
	assert(loop);

#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	loop->stopped = 0;
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif
}

size_t
ev_loop_wait(ev_loop_t *loop, ev_future_t *future)
{
	size_t n = 0;
	struct ev_loop_ctx *ctx = NULL;
	while (ev_loop_ctx_wait_one(&ctx, loop, future))
		n += n < SIZE_MAX;
	ev_loop_ctx_destroy(ctx);
	return n;
}

size_t
ev_loop_wait_until(ev_loop_t *loop, ev_future_t *future,
		const struct timespec *abs_time)
{
	size_t n = 0;
	struct ev_loop_ctx *ctx = NULL;
	while (ev_loop_ctx_wait_one_until(&ctx, loop, future, abs_time))
		n += n < SIZE_MAX;
	ev_loop_ctx_destroy(ctx);
	return n;
}

size_t
ev_loop_wait_one(ev_loop_t *loop, ev_future_t *future)
{
	struct ev_loop_ctx *ctx = NULL;
	size_t n = ev_loop_ctx_wait_one(&ctx, loop, future);
	ev_loop_ctx_destroy(ctx);
	return n;
}

size_t
ev_loop_wait_one_until(ev_loop_t *loop, ev_future_t *future,
		const struct timespec *abs_time)
{
	struct ev_loop_ctx *ctx = NULL;
	size_t n = ev_loop_ctx_wait_one_until(&ctx, loop, future, abs_time);
	ev_loop_ctx_destroy(ctx);
	return n;
}

void *
ev_loop_self(void)
{
	return &ev_loop_thrd;
}

int
ev_loop_kill(ev_loop_t *loop, void *thr_)
{
#if LELY_NO_THREADS
	(void)loop;
#else
	assert(loop);
#endif
	struct ev_loop_thrd *thr = thr_;
	assert(thr);

	int result = 0;
	int errc = get_errc();
#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	if (!thr->stopped) {
		if (thr->ctx) {
			if ((result = ev_loop_ctx_kill(thr->ctx, 1)) == -1)
				errc = get_errc();
		} else {
			thr->stopped = 1;
		}
	}
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif
	set_errc(errc);
	return result;
}

static void
ev_loop_ctx_task_func(struct ev_task *task)
{
	assert(task);
	struct ev_loop_ctx *ctx = structof(task, struct ev_loop_ctx, task);
	ev_loop_t *loop = ctx->loop;
	assert(loop);
	assert(ctx->future);

#if LELY_NO_THREADS
	(void)loop;
#else
	mtx_lock(&loop->mtx);
#endif
	if (ctx->refcnt > 1 && !*ctx->pstopped && !ctx->ready) {
		ctx->ready = 1;
		ev_loop_ctx_kill(ctx, 0);
	}
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif
	ev_loop_ctx_release(ctx);
}

static struct ev_loop_ctx *
ev_loop_ctx_alloc(void)
{
	int errc = 0;

	struct ev_loop_ctx *ctx = malloc(sizeof(*ctx));
	if (!ctx) {
#if !LELY_NO_ERRNO
		errc = errno2c(errno);
#endif
		goto error_malloc_ctx;
	}

	ctx->refcnt = 0;

	ctx->loop = NULL;

	ctx->future = NULL;
	ctx->task = (struct ev_task)EV_TASK_INIT(NULL, &ev_loop_ctx_task_func);

	ctx->pstopped = NULL;
#if !LELY_NO_THREADS
	if (cnd_init(&ctx->cond) != thrd_success) {
		errc = get_errc();
		goto error_init_cond;
	}
	ctx->waiting = 0;
#endif
	ctx->ready = 0;
	ctx->polling = 0;
	ctx->thr = NULL;

	dlnode_init(&ctx->node);
	ctx->next = NULL;

	return ctx;

#if !LELY_NO_THREADS
	// cnd_destroy(&ctx->cond);
error_init_cond:
#endif
	free(ctx);
error_malloc_ctx:
	set_errc(errc);
	return NULL;
}

static void
ev_loop_ctx_free(struct ev_loop_ctx *ctx)
{
	if (ctx) {
#if !LELY_NO_THREADS
		cnd_destroy(&ctx->cond);
#endif
		free(ctx);
	}
}

static void
ev_loop_ctx_release(struct ev_loop_ctx *ctx)
{
	assert(ctx);
	ev_loop_t *loop = ctx->loop;
	assert(loop);

#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	if (--ctx->refcnt) {
#if !LELY_NO_THREADS
		mtx_unlock(&loop->mtx);
#endif
		return;
	}
	ev_future_t *future = ctx->future;
	ctx->future = NULL;
#if !LELY_NO_THREADS
	assert(!ctx->waiting);
#endif
	assert(!ctx->polling);
	if (loop->nunused < LELY_EV_LOOP_CTX_MAX_UNUSED) {
		ctx->next = loop->unused;
		loop->unused = ctx;
		loop->nunused++;
#if !LELY_NO_THREADS
		mtx_unlock(&loop->mtx);
#endif
		ev_future_release(future);
	} else {
#if !LELY_NO_THREADS
		mtx_unlock(&loop->mtx);
#endif
		ev_future_release(future);
		ev_loop_ctx_free(ctx);
	}
}

static struct ev_loop_ctx *
ev_loop_ctx_create(ev_loop_t *loop, ev_future_t *future)
{
	assert(loop);

#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	struct ev_loop_ctx *ctx = loop->unused;
	if (ctx) {
		loop->unused = ctx->next;
		assert(loop->nunused);
		loop->nunused--;
#if !LELY_NO_THREADS
		mtx_unlock(&loop->mtx);
#endif
	} else {
#if !LELY_NO_THREADS
		mtx_unlock(&loop->mtx);
#endif
		ctx = ev_loop_ctx_alloc();
		if (!ctx)
			return NULL;
	}

	assert(!ctx->refcnt);
	ctx->refcnt++;

	ctx->loop = loop;

	ctx->future = ev_future_acquire(future);
	ctx->task = (struct ev_task)EV_TASK_INIT(
			ev_loop_get_exec(loop), &ev_loop_ctx_task_func);

	ctx->pstopped = &ev_loop_thrd.stopped;

#if !LELY_NO_THREADS
	assert(!ctx->waiting);
#endif
	ctx->ready = 0;
	assert(!ctx->polling);
	ctx->thr = loop->poll ? ev_poll_self(loop->poll) : NULL;

	dlnode_init(&ctx->node);

	ctx->next = ev_loop_thrd.ctx;
	ev_loop_thrd.ctx = ctx;

	if (ctx->future) {
		ctx->refcnt++;
		ev_future_submit(ctx->future, &ctx->task);
	}

	return ctx;
}

static void
ev_loop_ctx_destroy(struct ev_loop_ctx *ctx)
{
	if (ctx) {
		if (ctx->future)
			ev_future_cancel(ctx->future, &ctx->task);

		assert(ev_loop_thrd.ctx == ctx);
		ev_loop_thrd.ctx = ctx->next;

		ev_loop_ctx_release(ctx);
	}
}

static inline int
ev_loop_can_poll(ev_loop_t *loop)
{
	return loop->poll && (!loop->npoll || loop->npolling < loop->npoll);
}

static size_t
ev_loop_ctx_wait_one(
		struct ev_loop_ctx **pctx, ev_loop_t *loop, ev_future_t *future)
{
	assert(pctx);
	struct ev_loop_ctx *ctx = *pctx;
	assert(loop);
	assert(!ctx || ctx->loop == loop);

	size_t n = 0;
#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	int poll_task = 0;
	while (!loop->stopped && (!ctx || (!*ctx->pstopped && !ctx->ready))) {
		// Stop the event loop if no more tasks remain or have been
		// announced with ev_exec_on_task_init(), unless we should be
		// waiting on a future but have not created the event loop
		// context yet.
		if (ev_loop_empty(loop) && !ev_loop_ntasks(loop)
				&& (!future || ctx)) {
			ev_loop_do_stop(loop);
			continue;
		}
		struct ev_task *task = ev_task_from_node(
				sllist_pop_front(&loop->queue));
		if (task && task == &loop->task) {
			// The polling task is not a real task, but is part of
			// the task queue for scheduling purposes.
			poll_task = 1;
			task = NULL;
		}
		if (task) {
			// If a real task is available, execute it and return.
#if !LELY_NO_THREADS
			mtx_unlock(&loop->mtx);
#endif
			assert(task->exec);
			ev_exec_run(task->exec, task);
			n++;
#if !LELY_NO_THREADS
			mtx_lock(&loop->mtx);
#endif
			break;
		}
		if (!ctx) {
			// Only create an event loop context when we have to
			// poll or wait.
#if !LELY_NO_THREADS
			mtx_unlock(&loop->mtx);
#endif
			ctx = *pctx = ev_loop_ctx_create(loop, future);
			if (!ctx) {
#if !LELY_NO_THREADS
				mtx_lock(&loop->mtx);
#endif
				break;
			}
#if !LELY_NO_THREADS
			mtx_lock(&loop->mtx);
#endif
			// We released the lock, so a task may have been queued.
			continue;
		}
		if (ev_loop_can_poll(loop)) {
			ctx->polling = 1;
			// Wake polling threads in LIFO order.
			dllist_push_front(&loop->polling, &ctx->node);
			loop->npolling++;
			int empty = sllist_empty(&loop->queue);
#if !LELY_NO_THREADS
			mtx_unlock(&loop->mtx);
#endif
			int result = ev_poll_wait(loop->poll, empty ? -1 : 0);
#if !LELY_NO_THREADS
			mtx_lock(&loop->mtx);
#endif
			loop->npolling--;
			dllist_remove(&loop->polling, &ctx->node);
			ctx->polling = 0;
			if (result == -1)
				break;
		} else if (!sllist_empty(&loop->queue)) {
			continue;
		} else {
#if LELY_NO_THREADS
			break;
#else // !LELY_NO_THREADS
			ctx->waiting = 1;
			// Wake waiting threads in LIFO order.
			dllist_push_front(&loop->waiting, &ctx->node);
			int result = cnd_wait(&ctx->cond, &loop->mtx);
			dllist_remove(&loop->waiting, &ctx->node);
			ctx->waiting = 0;
			if (result != thrd_success)
				break;
#endif // !LELY_NO_THREADS
		}
	}
	int empty = sllist_empty(&loop->queue);
	if (poll_task)
		// Requeue the polling task.
		sllist_push_back(&loop->queue, &loop->task._node);
	if (!empty)
		// If any real tasks remain on the queue, wake up any polling or
		// non-polling thread.
		ev_loop_kill_any(loop, 1);
	else if (poll_task)
		// Wake up any non-polling thread so it can start polling.
		ev_loop_kill_any(loop, 0);
	if (!n && ctx && *ctx->pstopped)
		// Reset the thread-local flag used to stop an event loop with
		// ev_loop_kill(), so it will resume on the next run funciton.
		*ctx->pstopped = 0;
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif
	return n;
}

static size_t
ev_loop_ctx_wait_one_until(struct ev_loop_ctx **pctx, ev_loop_t *loop,
		ev_future_t *future, const struct timespec *abs_time)
{
	assert(pctx);
	struct ev_loop_ctx *ctx = *pctx;
	assert(loop);
	assert(!ctx || ctx->loop == loop);
#if LELY_NO_THREADS && LELY_NO_TIMEOUT
	(void)abs_time;
#endif

	size_t n = 0;
#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	int poll_task = 0;
	while (!loop->stopped && (!ctx || (!*ctx->pstopped && !ctx->ready))) {
		// Stop the event loop if no more tasks remain or have been
		// announced with ev_exec_on_task_init(), unless we should be
		// waiting on a future but have not created the event loop
		// context yet.
		if (ev_loop_empty(loop) && !ev_loop_ntasks(loop)
				&& (!future || ctx)) {
			ev_loop_do_stop(loop);
			continue;
		}
		struct ev_task *task = ev_task_from_node(
				sllist_pop_front(&loop->queue));
		if (task && task == &loop->task) {
			// The polling task is not a real task, but is part of
			// the task queue for scheduling purposes.
			poll_task = 1;
			task = NULL;
		}
		if (task) {
			// If a real task is available, execute it and return.
#if !LELY_NO_THREADS
			mtx_unlock(&loop->mtx);
#endif
			assert(task->exec);
			ev_exec_run(task->exec, task);
			n++;
#if !LELY_NO_THREADS
			mtx_lock(&loop->mtx);
#endif
			break;
		}
		if (!ctx) {
			// Only create an event loop context when we have to
			// poll or wait.
#if !LELY_NO_THREADS
			mtx_unlock(&loop->mtx);
#endif
			ctx = *pctx = ev_loop_ctx_create(loop, future);
			if (!ctx) {
#if !LELY_NO_THREADS
				mtx_lock(&loop->mtx);
#endif
				break;
			}
#if !LELY_NO_THREADS
			mtx_lock(&loop->mtx);
#endif
			// We released the lock, so a task may have been queued.
			continue;
		}
		if (ev_loop_can_poll(loop)) {
			ctx->polling = 1;
			// Wake polling threads in LIFO order.
			dllist_push_front(&loop->polling, &ctx->node);
			loop->npolling++;
#if !LELY_NO_TIMEOUT
			int empty = sllist_empty(&loop->queue);
#endif
#if !LELY_NO_THREADS
			mtx_unlock(&loop->mtx);
#endif
			int result = -1;
			int64_t msec = 0;
#if !LELY_NO_TIMEOUT
			if (empty && abs_time) {
				struct timespec now = { 0, 0 };
				if (!timespec_get(&now, TIME_UTC))
					goto error;
				if (timespec_cmp(abs_time, &now) <= 0) {
					set_errnum(ERRNUM_TIMEDOUT);
					goto error;
				}
				msec = timespec_diff_msec(abs_time, &now);
				if (!msec)
					// Prevent a busy loop due to rounding.
					msec = 1;
				else if (msec > INT_MAX)
					msec = INT_MAX;
			}
#endif // !LELY_NO_TIMEOUT
			result = ev_poll_wait(loop->poll, msec);
#if !LELY_NO_TIMEOUT
		error:
#endif
#if !LELY_NO_THREADS
			mtx_lock(&loop->mtx);
#endif
			loop->npolling--;
			dllist_remove(&loop->polling, &ctx->node);
			ctx->polling = 0;
			if (result == -1)
				break;
		} else if (!sllist_empty(&loop->queue)) {
			continue;
#if !LELY_NO_THREADS && !LELY_NO_TIMEOUT
		} else if (abs_time) {
			ctx->waiting = 1;
			// Wake waiting threads in LIFO order.
			dllist_push_front(&loop->waiting, &ctx->node);
			int result = cnd_timedwait(
					&ctx->cond, &loop->mtx, abs_time);
			dllist_remove(&loop->waiting, &ctx->node);
			ctx->waiting = 0;
			if (result != thrd_success) {
				if (result == thrd_timedout)
					set_errnum(ERRNUM_TIMEDOUT);
				break;
			}
#endif // !LELY_NO_THREADS
		} else {
			break;
		}
	}
	int empty = sllist_empty(&loop->queue);
	if (poll_task)
		// Requeue the polling task.
		sllist_push_back(&loop->queue, &loop->task._node);
	if (!empty)
		// If any real tasks remain on the queue, wake up any polling or
		// non-polling thread.
		ev_loop_kill_any(loop, 1);
	else if (poll_task)
		// Wake up any non-polling thread so it can start polling.
		ev_loop_kill_any(loop, 0);
	if (!n && ctx && *ctx->pstopped)
		// Reset the thread-local flag used to stop an event loop with
		// ev_loop_kill(), so it will resume on the next run funciton.
		*ctx->pstopped = 0;
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif
	return n;
}

static int
ev_loop_ctx_kill(struct ev_loop_ctx *ctx, int stop)
{
	assert(ctx);
	assert(ctx->loop);
	assert(ctx->pstopped);
	assert(!ctx->polling || ctx->loop->poll);

	if (*ctx->pstopped)
		return 0;
	else if (stop)
		*ctx->pstopped = 1;

#if !LELY_NO_THREADS
	if (ctx->waiting) {
		cnd_signal(&ctx->cond);
		return 0;
	}
#endif
	return ctx->polling ? ev_poll_kill(ctx->loop->poll, ctx->thr) : 0;
}

static void
ev_loop_std_exec_impl_on_task_init(ev_std_exec_impl_t *impl)
{
	ev_loop_t *loop = ev_loop_from_impl(impl);

#if LELY_NO_THREADS || (LELY_NO_ATOMICS && (!_WIN32 || defined(__MINGW32__)))
	loop->ntasks++;
#elif _WIN64 && !defined(__MINGW32__)
	InterlockedIncrementNoFence64(&loop->ntasks);
#elif _WIN32 && !defined(__MINGW32__)
	InterlockedIncrementNoFence(&loop->ntasks);
#else
	atomic_fetch_add_explicit(&loop->ntasks, 1, memory_order_relaxed);
#endif
}

static void
ev_loop_std_exec_impl_on_task_fini(ev_std_exec_impl_t *impl)
{
	ev_loop_t *loop = ev_loop_from_impl(impl);

#if LELY_NO_THREADS || (LELY_NO_ATOMICS && (!_WIN32 || defined(__MINGW32__)))
	if (!--loop->ntasks) {
#elif _WIN64 && !defined(__MINGW32__)
	if (!InterlockedDecrementRelease64(&loop->ntasks)) {
		MemoryBarrier();
#elif _WIN32 && !defined(__MINGW32__)
	if (!InterlockedDecrementRelease(&loop->ntasks)) {
		MemoryBarrier();
#else
	if (atomic_fetch_sub_explicit(&loop->ntasks, 1, memory_order_release)
			== 1) {
		atomic_thread_fence(memory_order_acquire);
#endif
#if !LELY_NO_THREADS
		mtx_lock(&loop->mtx);
#endif
		if (ev_loop_empty(loop))
			ev_loop_do_stop(loop);
#if !LELY_NO_THREADS
		mtx_unlock(&loop->mtx);
#endif
	}
}

static void
ev_loop_std_exec_impl_post(ev_std_exec_impl_t *impl, struct ev_task *task)
{
	ev_loop_t *loop = ev_loop_from_impl(impl);
	assert(task);

#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	int empty = sllist_empty(&loop->queue);
	sllist_push_back(&loop->queue, &task->_node);
	if (empty)
		ev_loop_kill_any(loop, 1);
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif
}

static size_t
ev_loop_std_exec_impl_abort(ev_std_exec_impl_t *impl, struct ev_task *task)
{
	ev_loop_t *loop = ev_loop_from_impl(impl);

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	if (!task) {
		int poll_task = 0;
		struct slnode *node;
		while ((node = sllist_pop_front(&loop->queue))) {
			if (ev_task_from_node(node) == &loop->task)
				poll_task = 1;
			else
				sllist_push_back(&queue, node);
		}
		if (poll_task)
			sllist_push_back(&loop->queue, &loop->task._node);
	} else if (sllist_remove(&loop->queue, &task->_node)) {
		sllist_push_back(&queue, &task->_node);
	}
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif

	size_t n = 0;
	while (sllist_pop_front(&queue))
		n += n < SIZE_MAX;
	return n;
}

static inline ev_loop_t *
ev_loop_from_impl(const ev_std_exec_impl_t *impl)
{
	assert(impl);

	return structof(impl, ev_loop_t, impl_vptr);
}

static int
ev_loop_empty(const ev_loop_t *loop)
{
	assert(loop);

	if (sllist_empty(&loop->queue))
		return 1;
	struct slnode *node = sllist_first(&loop->queue);
	// The task queue is considered empty if only the polling task remains.
	return node == &loop->task._node && node->next == NULL;
}

static size_t
ev_loop_ntasks(const ev_loop_t *loop)
{
	assert(loop);

#if LELY_NO_THREADS || LELY_NO_ATOMICS || (_WIN32 && !defined(__MINGW32__))
	return loop->ntasks;
#else
	return atomic_load_explicit(
			(atomic_size_t *)&loop->ntasks, memory_order_relaxed);
#endif
}

static void
ev_loop_do_stop(ev_loop_t *loop)
{
	assert(loop);

	if (!loop->stopped) {
		loop->stopped = 1;
#if !LELY_NO_THREADS
		dllist_foreach (&loop->waiting, node) {
			struct ev_loop_ctx *ctx = structof(
					node, struct ev_loop_ctx, node);
			ev_loop_ctx_kill(ctx, 1);
		}
#endif
		dllist_foreach (&loop->polling, node) {
			struct ev_loop_ctx *ctx = structof(
					node, struct ev_loop_ctx, node);
			ev_loop_ctx_kill(ctx, 1);
		}
	}
}

static int
ev_loop_kill_any(ev_loop_t *loop, int polling)
{
	assert(loop);

	struct dlnode *node;
#if !LELY_NO_THREADS
	if ((node = dllist_first(&loop->waiting))) {
		struct ev_loop_ctx *ctx =
				structof(node, struct ev_loop_ctx, node);
		return ev_loop_ctx_kill(ctx, 0);
	}
#endif
	if (polling && (node = dllist_first(&loop->polling))) {
		struct ev_loop_ctx *ctx =
				structof(node, struct ev_loop_ctx, node);
		return ev_loop_ctx_kill(ctx, 0);
	}
	return 0;
}

#endif // !LELY_NO_MALLOC
