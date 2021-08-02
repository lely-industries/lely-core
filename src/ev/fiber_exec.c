/**@file
 * This file is part of the event library; it contains the implementation of the
 * fiber executor functions.
 *
 * @see lely/ev/fiber_exec.h
 *
 * @copyright 2019-2021 Lely Industries N.V.
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
#include <lely/libc/threads.h>
#endif
#include <lely/ev/exec.h>
#include <lely/ev/fiber_exec.h>
#include <lely/ev/task.h>
#include <lely/util/errnum.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef LELY_EV_FIBER_MAX_UNUSED
/// The maximum number of unused fibers per thread.
#define LELY_EV_FIBER_MAX_UNUSED 16
#endif

struct ev_fiber_ctx;

/**
 * The parameters used for creating fibers on this thread and the list of unused
 * fibers.
 */
#if LELY_NO_THREADS
static struct ev_fiber_thrd {
#else
static _Thread_local struct ev_fiber_thrd {
#endif
	/**
	 * The number of invocations of ev_fiber_thrd_init() minus the the
	 * number of invocation of ev_fiber_thrd_fini() for this thread.
	 */
	size_t refcnt;
	/// The flags used when creating each fiber.
	int flags;
	/// The size (in bytes) of the stack frame allocated for each fiber.
	size_t stack_size;
	/// The maximum number of unused fibers for this thread.
	size_t max_unused;
	/// The list of unused fibers.
	struct ev_fiber_ctx *unused;
	/// The number of unused fibers.
	size_t num_unused;
	/// A pointer to the currently running fiber.
	struct ev_fiber_ctx *curr;
	/// A pointer to the previously running (suspended) fiber.
	fiber_t *prev;
} ev_fiber_thrd;

static void ev_fiber_exec_on_task_init(ev_exec_t *exec);
static void ev_fiber_exec_on_task_fini(ev_exec_t *exec);
static int ev_fiber_exec_dispatch(ev_exec_t *exec, struct ev_task *task);
static void ev_fiber_exec_post(ev_exec_t *exec, struct ev_task *task);
static void ev_fiber_exec_defer(ev_exec_t *exec, struct ev_task *task);
static size_t ev_fiber_exec_abort(ev_exec_t *exec, struct ev_task *task);

// clang-format off
static const struct ev_exec_vtbl ev_fiber_exec_vtbl = {
	&ev_fiber_exec_on_task_init,
	&ev_fiber_exec_on_task_fini,
	&ev_fiber_exec_dispatch,
	&ev_fiber_exec_post,
	&ev_fiber_exec_defer,
	&ev_fiber_exec_abort,
	NULL
};
// clang-format on

/// The implementation of a fiber executor.
struct ev_fiber_exec {
	/// A pointer to the virtual table for the executor interface.
	const struct ev_exec_vtbl *exec_vptr;
	/// A pointer to the inner executor.
	ev_exec_t *inner_exec;
	/// The task used to create new fibers.
	struct ev_task task;
	/// A pointer to the #ev_fiber_thrd instance for this executor.
	struct ev_fiber_thrd *thr;
	/// The number of pending fibers available to execute a task.
	size_t pending;
#if !LELY_NO_THREADS
	/// The mutex protecting #posted and #queue.
	mtx_t mtx;
#endif
	/// A flag indicating whether #task has been posted to #inner_exec.
	int posted;
	/// The queue of tasks submitted to this executor.
	struct sllist queue;
};

static void ev_fiber_exec_func(struct ev_task *task);

static inline struct ev_fiber_exec *ev_fiber_exec_from_exec(
		const ev_exec_t *exec);

static int ev_fiber_exec_post_ctx(struct ev_fiber_exec *exec);

/**
 * The context of a fiber used for executing tasks. The context is allocated on
 * the stack of the fiber.
 */
struct ev_fiber_ctx {
	/// A pointer to the fiber containing this context.
	fiber_t *fiber;
	/// A pointer to the next fiber in the list of unused fibers.
	struct ev_fiber_ctx *next;
	/// The executor using this fiber.
	struct ev_fiber_exec *exec;
	/// The task used to resume the fiber.
	struct ev_task task;
	/// The queue of deferred tasks.
	struct sllist queue;
};

static struct ev_fiber_ctx *ev_fiber_ctx_create(struct ev_fiber_exec *exec);
static void ev_fiber_ctx_destroy(struct ev_fiber_ctx *ctx);

static fiber_t *ev_fiber_ctx_fiber_func(fiber_t *fiber, void *arg);
static void ev_fiber_ctx_task_func(struct ev_task *task);

static struct ev_fiber_ctx *ev_fiber_resume(fiber_t *fiber);
static fiber_t *ev_fiber_await_func(fiber_t *fiber, void *arg);

static void ev_fiber_return(void);
static fiber_t *ev_fiber_return_func(fiber_t *fiber, void *arg);

/// The implementation of a fiber mutex.
struct ev_fiber_mtx_impl {
	/// The type of mutex: #ev_fiber_mtx_plain or #ev_fiber_mtx_recursive.
	int type;
#if !LELY_NO_THREADS
	/// The mutex protecting #locked and #queue.
	mtx_t mtx;
#endif
	/// A pointer to the fiber holding the lock.
	struct ev_fiber_ctx *ctx;
	/// The number of times the mutex has been recursively locked.
	size_t locked;
	/// The queue of fibers waiting to acquire the lock.
	struct sllist queue;
};

static fiber_t *ev_fiber_mtx_lock_func(fiber_t *fiber, void *arg);

/**
 * The context of a wait operation on a fiber condition variable. This struct is
 * allocated on the stack of a fiber in ev_fiber_cnd_wait().
 */
struct ev_fiber_cnd_wait {
	/// The node in the queue of fibers waiting on #cond.
	struct slnode node;
	/// A pointer to the condition variable passed to ev_fiber_cnd_wait().
	ev_fiber_cnd_t *cond;
	/// A pointer to the mutex passed to ev_fiber_cnd_wait().
	ev_fiber_mtx_t *mtx;
	/// A pointer to the task (in #ev_fiber_ctx) used to wake up the fiber.
	struct ev_task *task;
};

static fiber_t *ev_fiber_cnd_wait_func(fiber_t *fiber, void *arg);

/// The implementation of a fiber condition variable.
struct ev_fiber_cnd_impl {
#if !LELY_NO_THREADS
	/// The mutex protecting #queue.
	mtx_t mtx;
#endif
	/**
	 * The queue of fibers waiting for the condition variable to be
	 * signaled.
	 */
	struct sllist queue;
};

static void ev_fiber_cnd_wake(struct slnode *node);

int
ev_fiber_thrd_init(int flags, size_t stack_size, size_t max_unused)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;

	if (thr->refcnt++)
		return 1;

	int result = fiber_thrd_init(flags & ~FIBER_GUARD_STACK);
	if (result == -1) {
		thr->refcnt--;
	} else {
		thr->flags = flags;
		thr->stack_size = stack_size;

		if (!(thr->max_unused = max_unused))
			thr->max_unused = LELY_EV_FIBER_MAX_UNUSED;
		thr->unused = NULL;
		thr->num_unused = 0;

		thr->curr = NULL;
		thr->prev = NULL;
	}
	return result;
}

void
ev_fiber_thrd_fini(void)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;
	assert(!thr->curr);

	if (--thr->refcnt)
		return;

	struct ev_fiber_ctx *ctx;
	while ((ctx = thr->unused)) {
		thr->unused = ctx->next;
		fiber_destroy(ctx->fiber);
	}

	fiber_thrd_fini();
}

void *
ev_fiber_exec_alloc(void)
{
	struct ev_fiber_exec *exec = malloc(sizeof(*exec));
	if (!exec) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return NULL;
	}
	// Suppress a GCC maybe-uninitialized warning.
	exec->exec_vptr = NULL;
	// cppcheck-suppress memleak symbolName=exec
	return &exec->exec_vptr;
}

void
ev_fiber_exec_free(void *ptr)
{
	if (ptr)
		free(ev_fiber_exec_from_exec(ptr));
}

ev_exec_t *
ev_fiber_exec_init(ev_exec_t *exec_, ev_exec_t *inner_exec)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;
	assert(thr->refcnt);
	struct ev_fiber_exec *exec = ev_fiber_exec_from_exec(exec_);
	assert(inner_exec);

	exec->exec_vptr = &ev_fiber_exec_vtbl;

	exec->inner_exec = inner_exec;
	exec->task = (struct ev_task)EV_TASK_INIT(
			exec->inner_exec, &ev_fiber_exec_func);

	exec->thr = thr;

	exec->pending = 0;

#if !LELY_NO_THREADS
	if (mtx_init(&exec->mtx, mtx_plain) != thrd_success)
		return NULL;
#endif

	exec->posted = 0;

	sllist_init(&exec->queue);

	return exec_;
}

void
ev_fiber_exec_fini(ev_exec_t *exec_)
{
	struct ev_fiber_exec *exec = ev_fiber_exec_from_exec(exec_);
	assert(exec->thr == &ev_fiber_thrd);

	ev_fiber_exec_abort(exec_, NULL);

#if !LELY_NO_THREADS
	mtx_lock(&exec->mtx);
#endif
	// Abort ev_fiber_exec_func().
	if (exec->posted && ev_exec_abort(exec->task.exec, &exec->task))
		exec->posted = 0;
	assert(exec->posted == 0);
#if !LELY_NO_THREADS
	mtx_unlock(&exec->mtx);

	mtx_destroy(&exec->mtx);
#endif
}

ev_exec_t *
ev_fiber_exec_create(ev_exec_t *inner_exec)
{
	int errc = 0;

	ev_exec_t *exec = ev_fiber_exec_alloc();
	if (!exec) {
		errc = get_errc();
		goto error_alloc;
	}

	ev_exec_t *tmp = ev_fiber_exec_init(exec, inner_exec);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	exec = tmp;

	return exec;

error_init:
	ev_fiber_exec_free((void *)exec);
error_alloc:
	set_errc(errc);
	return NULL;
}

void
ev_fiber_exec_destroy(ev_exec_t *exec)
{
	if (exec) {
		ev_fiber_exec_fini(exec);
		ev_fiber_exec_free((void *)exec);
	}
}

ev_exec_t *
ev_fiber_exec_get_inner_exec(const ev_exec_t *exec_)
{
	struct ev_fiber_exec *exec = ev_fiber_exec_from_exec(exec_);

	return exec->inner_exec;
}

void
ev_fiber_await(ev_future_t *future)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;
	assert(thr->curr);

	thr->prev = fiber_resume_with(thr->prev, ev_fiber_await_func, future);
}

int
ev_fiber_mtx_init(ev_fiber_mtx_t *mtx, int type)
{
	assert(mtx);

	if (type & ~ev_fiber_mtx_recursive) {
		set_errnum(ERRNUM_INVAL);
		return ev_fiber_error;
	}

	struct ev_fiber_mtx_impl *impl = malloc(sizeof(*impl));
	if (!impl)
		return ev_fiber_nomem;

	impl->type = type;

#if !LELY_NO_THREADS
	switch (mtx_init(&impl->mtx, mtx_plain)) {
	case thrd_error: free(impl); return ev_fiber_error;
	case thrd_nomem: free(impl); return ev_fiber_nomem;
	default: break;
	}
#endif

	impl->ctx = NULL;
	impl->locked = 0;
	sllist_init(&impl->queue);

	mtx->_impl = impl;

	return ev_fiber_success;
}

void
ev_fiber_mtx_destroy(ev_fiber_mtx_t *mtx)
{
	assert(mtx);
	struct ev_fiber_mtx_impl *impl = mtx->_impl;
	assert(impl);

	assert(!impl->locked);
	assert(sllist_empty(&impl->queue));
#if !LELY_NO_THREADS
	mtx_destroy(&impl->mtx);
#endif
	free(impl);

	mtx->_impl = NULL;
}

int
ev_fiber_mtx_lock(ev_fiber_mtx_t *mtx)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;
	assert(mtx);
	struct ev_fiber_mtx_impl *impl = mtx->_impl;
	assert(impl);

	if (!thr->curr) {
		set_errnum(ERRNUM_PERM);
		return ev_fiber_error;
	}

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif
	if (impl->locked) {
		if (impl->ctx == thr->curr) {
			if (!(impl->type & ev_fiber_mtx_recursive)) {
#if !LELY_NO_THREADS
				mtx_unlock(&impl->mtx);
#endif
				set_errnum(ERRNUM_PERM);
				return ev_fiber_error;
			}
			assert(impl->locked < SIZE_MAX);
			impl->locked++;
		} else {
			// The inner mutex will be unlocked in
			// ev_fiber_mtx_lock_func().
			thr->prev = fiber_resume_with(thr->prev,
					ev_fiber_mtx_lock_func, impl);
			impl->ctx = thr->curr;
			assert(impl->locked == 1);
		}
	} else {
		assert(impl->ctx == NULL);
		assert(sllist_empty(&impl->queue));
		impl->ctx = thr->curr;
		impl->locked = 1;
#if !LELY_NO_THREADS
		mtx_unlock(&impl->mtx);
#endif
	}

	return ev_fiber_success;
}

int
ev_fiber_mtx_trylock(ev_fiber_mtx_t *mtx)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;
	assert(mtx);
	struct ev_fiber_mtx_impl *impl = mtx->_impl;
	assert(impl);

	if (!thr->curr) {
		set_errnum(ERRNUM_PERM);
		return ev_fiber_error;
	}

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif
	if (impl->locked) {
		if (impl->ctx == thr->curr) {
			if (!(impl->type & ev_fiber_mtx_recursive)) {
#if !LELY_NO_THREADS
				mtx_unlock(&impl->mtx);
#endif
				set_errnum(ERRNUM_PERM);
				return ev_fiber_error;
			}
			assert(impl->locked < SIZE_MAX);
			impl->locked++;
#if !LELY_NO_THREADS
			mtx_unlock(&impl->mtx);
#endif
			return ev_fiber_success;
		} else {
#if !LELY_NO_THREADS
			mtx_unlock(&impl->mtx);
#endif
			return ev_fiber_busy;
		}
	} else {
		assert(impl->ctx == NULL);
		assert(sllist_empty(&impl->queue));
		impl->ctx = thr->curr;
		impl->locked = 1;
#if !LELY_NO_THREADS
		mtx_unlock(&impl->mtx);
#endif
		return ev_fiber_success;
	}
}

int
ev_fiber_mtx_unlock(ev_fiber_mtx_t *mtx)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;
	assert(mtx);
	struct ev_fiber_mtx_impl *impl = mtx->_impl;
	assert(impl);

	if (!thr->curr) {
		set_errnum(ERRNUM_PERM);
		return ev_fiber_error;
	}

	struct ev_task *task = NULL;

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif
	if (impl->ctx != thr->curr || !impl->locked) {
#if !LELY_NO_THREADS
		mtx_unlock(&impl->mtx);
#endif
		set_errnum(ERRNUM_PERM);
		return ev_fiber_error;
	}
	assert((impl->type & ev_fiber_mtx_recursive) || impl->locked == 1);
	if (!--impl->locked) {
		impl->ctx = NULL;
		if ((task = ev_task_from_node(sllist_pop_front(&impl->queue))))
			impl->locked++;
	}
#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	if (task)
		ev_exec_post(task->exec, task);

	return ev_fiber_success;
}

int
ev_fiber_cnd_init(ev_fiber_cnd_t *cond)
{
	assert(cond);

	struct ev_fiber_cnd_impl *impl = malloc(sizeof(*impl));
	if (!impl)
		return ev_fiber_nomem;

#if !LELY_NO_THREADS
	switch (mtx_init(&impl->mtx, mtx_plain)) {
	case thrd_error: free(impl); return ev_fiber_error;
	case thrd_nomem: free(impl); return ev_fiber_nomem;
	default: break;
	}
#endif

	sllist_init(&impl->queue);

	cond->_impl = impl;

	return ev_fiber_success;
}

void
ev_fiber_cnd_destroy(ev_fiber_cnd_t *cond)
{
	assert(cond);
	struct ev_fiber_cnd_impl *impl = cond->_impl;
	assert(impl);

	assert(sllist_empty(&impl->queue));
#if !LELY_NO_THREADS
	mtx_destroy(&impl->mtx);
#endif
	free(impl);

	cond->_impl = NULL;
}

int
ev_fiber_cnd_signal(ev_fiber_cnd_t *cond)
{
	assert(cond);
	struct ev_fiber_cnd_impl *impl = cond->_impl;
	assert(impl);

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif
	struct slnode *node = sllist_pop_front(&impl->queue);
#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	if (node)
		ev_fiber_cnd_wake(node);

	return ev_fiber_success;
}

int
ev_fiber_cnd_broadcast(ev_fiber_cnd_t *cond)
{
	assert(cond);
	struct ev_fiber_cnd_impl *impl = cond->_impl;
	assert(impl);

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif
	sllist_append(&queue, &impl->queue);
#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	struct slnode *node;
	while ((node = sllist_pop_front(&queue)))
		ev_fiber_cnd_wake(node);

	return ev_fiber_success;
}

int
ev_fiber_cnd_wait(ev_fiber_cnd_t *cond, ev_fiber_mtx_t *mtx)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;
	assert(mtx);
	struct ev_fiber_mtx_impl *impl = mtx->_impl;
	assert(impl);

	if (!thr->curr) {
		set_errnum(ERRNUM_PERM);
		return ev_fiber_error;
	}

	struct ev_fiber_cnd_wait wait = { .cond = cond, .mtx = mtx };

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif
	if (impl->ctx != thr->curr || impl->locked != 1) {
#if !LELY_NO_THREADS
		mtx_unlock(&impl->mtx);
#endif
		set_errnum(ERRNUM_PERM);
		return ev_fiber_error;
	}
	assert(impl->locked);

	// The inner mutex will be unlocked in ev_fiber_cnd_wait_func().
	thr->prev = fiber_resume_with(thr->prev, ev_fiber_cnd_wait_func, &wait);

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif
	assert(!impl->ctx);
	impl->ctx = thr->curr;
	assert(impl->locked == 1);
#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	return ev_fiber_success;
}

static void
ev_fiber_exec_on_task_init(ev_exec_t *exec_)
{
	struct ev_fiber_exec *exec = ev_fiber_exec_from_exec(exec_);

	ev_exec_on_task_init(exec->inner_exec);
}

static void
ev_fiber_exec_on_task_fini(ev_exec_t *exec_)
{
	struct ev_fiber_exec *exec = ev_fiber_exec_from_exec(exec_);

	ev_exec_on_task_fini(exec->inner_exec);
}

static int
ev_fiber_exec_dispatch(ev_exec_t *exec_, struct ev_task *task)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;
	struct ev_fiber_exec *exec = ev_fiber_exec_from_exec(exec_);
	assert(task);
	assert(!task->exec || task->exec == exec_);

	// Post the task if the executor is not currently executing a fiber.
	struct ev_fiber_ctx *ctx = thr->curr;
	if (!ctx || ctx->exec != exec) {
		ev_fiber_exec_post(exec_, task);
		return 0;
	}

	if (!task->exec)
		task->exec = exec_;

	// Execute the task immediately.
	if (task->func)
		task->func(task);

	return 1;
}

static void
ev_fiber_exec_post(ev_exec_t *exec_, struct ev_task *task)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;
	struct ev_fiber_exec *exec = ev_fiber_exec_from_exec(exec_);
	assert(task);
	assert(!task->exec || task->exec == exec_);

	if (!task->exec)
		task->exec = exec_;
	ev_fiber_exec_on_task_init(exec_);

	// Append the task to the queue.
#if !LELY_NO_THREADS
	mtx_lock(&exec->mtx);
#endif
	sllist_push_back(&exec->queue, &task->_node);
	int post = !exec->posted && exec->thr != thr;
	if (post)
		exec->posted = 1;
#if !LELY_NO_THREADS
	mtx_unlock(&exec->mtx);
#endif

	// If no pending fibers are available (and the calling thread is the
	// thread of this executor), try to post a new fiber immediately.
	if (exec->thr == thr && !exec->pending)
		ev_fiber_exec_post_ctx(exec);
	// Otherwise, post a task to post a fiber.
	if (post)
		ev_exec_post(exec->task.exec, &exec->task);
}

static void
ev_fiber_exec_defer(ev_exec_t *exec_, struct ev_task *task)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;
	struct ev_fiber_exec *exec = ev_fiber_exec_from_exec(exec_);
	assert(task);
	assert(!task->exec || task->exec == exec_);

	// Post the task if the executor is not currently executing a fiber.
	struct ev_fiber_ctx *ctx = thr->curr;
	if (!ctx || ctx->exec != exec) {
		ev_fiber_exec_post(exec_, task);
		return;
	}

	if (!task->exec)
		task->exec = exec_;
	ev_fiber_exec_on_task_init(exec_);

	// Push the task to the deferred queue of the fiber.
	sllist_push_back(&ctx->queue, &task->_node);
}

static size_t
ev_fiber_exec_abort(ev_exec_t *exec_, struct ev_task *task)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;
	struct ev_fiber_exec *exec = ev_fiber_exec_from_exec(exec_);

	struct sllist queue;
	sllist_init(&queue);

	struct ev_fiber_ctx *ctx = thr->curr;
	if (ctx && ctx->exec == exec) {
		// Try to find and abort the task(s) in the deferred queue of
		// the fiber.
		if (!task)
			sllist_append(&queue, &ctx->queue);
		else if (sllist_remove(&ctx->queue, &task->_node))
			sllist_push_back(&queue, &task->_node);
	}

#if !LELY_NO_THREADS
	mtx_lock(&exec->mtx);
#endif
	if (!task)
		sllist_append(&queue, &exec->queue);
	else if (sllist_remove(&exec->queue, &task->_node))
		sllist_push_back(&queue, &task->_node);
#if !LELY_NO_THREADS
	mtx_unlock(&exec->mtx);
#endif

	size_t n = 0;
	while (sllist_pop_front(&queue)) {
		ev_fiber_exec_on_task_fini(exec_);
		n += n < SIZE_MAX;
	}
	return n;
}

static void
ev_fiber_exec_func(struct ev_task *task)
{
	assert(task);
	struct ev_fiber_exec *exec = structof(task, struct ev_fiber_exec, task);
	assert(exec->thr == &ev_fiber_thrd);

#if !LELY_NO_THREADS
	mtx_lock(&exec->mtx);
#endif
	assert(exec->posted);
	exec->posted = 0;
#if !LELY_NO_THREADS
	mtx_unlock(&exec->mtx);
#endif

	// If no pending fibers are available, try to post a new fiber.
	if (!exec->pending)
		ev_fiber_exec_post_ctx(exec);
}

static inline struct ev_fiber_exec *
ev_fiber_exec_from_exec(const ev_exec_t *exec)
{
	assert(exec);

	return structof(exec, struct ev_fiber_exec, exec_vptr);
}

static int
ev_fiber_exec_post_ctx(struct ev_fiber_exec *exec)
{
	int errsv = get_errc();
	struct ev_fiber_ctx *ctx = ev_fiber_ctx_create(exec);
	if (!ctx) {
		// Ignore the error; one of the existing fibers can pick up the
		// submitted task.
		set_errc(errsv);
		return -1;
	}

	exec->pending++;
	ev_exec_post(ctx->task.exec, &ctx->task);

	return 0;
}

static struct ev_fiber_ctx *
ev_fiber_ctx_create(struct ev_fiber_exec *exec)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;
	assert(thr->refcnt);
	assert(exec);
	assert(exec->thr == thr);

	// Use a fiber from the unused list before creating a new one.
	struct ev_fiber_ctx *ctx = thr->unused;
	if (ctx) {
		thr->unused = ctx->next;
		ctx->next = NULL;
		assert(thr->num_unused);
		thr->num_unused--;
	} else {
		fiber_t *fiber = fiber_create(&ev_fiber_ctx_fiber_func, NULL,
				thr->flags, sizeof(struct ev_fiber_ctx),
				thr->stack_size);
		if (!fiber)
			return NULL;
		ctx = fiber_data(fiber);
		*ctx = (struct ev_fiber_ctx){
			.fiber = fiber,
		};
		sllist_init(&ctx->queue);
	}

	// Associate the fiber with the executor.
	ctx->exec = exec;
	ctx->task = (struct ev_task)EV_TASK_INIT(
			exec->inner_exec, &ev_fiber_ctx_task_func);

	return ctx;
}

static void
ev_fiber_ctx_destroy(struct ev_fiber_ctx *ctx)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;
	assert(thr->refcnt);

	if (ctx) {
		assert(sllist_empty(&ctx->queue));
		if (thr->num_unused < thr->max_unused) {
			ctx->next = thr->unused;
			thr->unused = ctx;
			thr->num_unused++;
		} else {
			fiber_destroy(ctx->fiber);
		}
	}
}

static fiber_t *
ev_fiber_ctx_fiber_func(fiber_t *fiber, void *arg)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;
	(void)arg;
	struct ev_fiber_ctx *ctx = fiber_data(NULL);

	thr->prev = fiber;

	struct ev_task *task = NULL;
	for (;;) {
		if (!task) {
			// Check if there are any tasks waiting on the queue of
			// the executor.
			assert(ctx->exec->pending);
			ctx->exec->pending--;
#if !LELY_NO_THREADS
			mtx_lock(&ctx->exec->mtx);
#endif
			task = ev_task_from_node(
					sllist_pop_front(&ctx->exec->queue));
#if !LELY_NO_THREADS
			mtx_unlock(&ctx->exec->mtx);
#endif
			// If no tasks are available, return and destroy the
			// fiber or put it on the unused list.
			if (!task) {
				ev_fiber_return();
				continue;
			}
		}

		// Execute the task.
		ev_exec_t *exec = task->exec;
		assert(exec == &ctx->exec->exec_vptr);
		if (task->func)
			task->func(task);
		ev_fiber_exec_on_task_fini(exec);

		// Check if there are any deferred tasks remaining.
		task = ev_task_from_node(sllist_pop_front(&ctx->queue));
		if (!task)
			ctx->exec->pending++;

		ev_fiber_await(NULL);
	}

	return NULL;
}

static void
ev_fiber_ctx_task_func(struct ev_task *task)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;
	assert(task);
	struct ev_fiber_ctx *ctx = structof(task, struct ev_fiber_ctx, task);

	thr->curr = ctx;
	fiber_t *fiber = fiber_resume(ctx->fiber);
	assert(!fiber);
	(void)fiber;
}

static struct ev_fiber_ctx *
ev_fiber_resume(fiber_t *fiber)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;

	struct ev_fiber_ctx *ctx = thr->curr;
	thr->curr = NULL;
	ctx->fiber = fiber;

	// If there are tasks on the queue, but no pending fibers to execute
	// them, try to post a new fiber.
	struct ev_fiber_exec *exec = ctx->exec;
	if (!exec->pending) {
#if !LELY_NO_THREADS
		mtx_lock(&exec->mtx);
#endif
		int empty = sllist_empty(&exec->queue);
#if !LELY_NO_THREADS
		mtx_unlock(&exec->mtx);
#endif
		if (!empty)
			ev_fiber_exec_post_ctx(exec);
	}

	return ctx;
}

static fiber_t *
ev_fiber_await_func(fiber_t *fiber, void *arg)
{
	ev_future_t *future = arg;

	struct ev_fiber_ctx *ctx = ev_fiber_resume(fiber);

	if (future)
		ev_future_submit(future, &ctx->task);
	else
		ev_exec_post(ctx->task.exec, &ctx->task);

	return NULL;
}

static void
ev_fiber_return(void)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;
	assert(thr->curr);

	thr->prev = fiber_resume_with(thr->prev, ev_fiber_return_func, NULL);
}

static fiber_t *
ev_fiber_return_func(fiber_t *fiber, void *arg)
{
	struct ev_fiber_thrd *thr = &ev_fiber_thrd;
	(void)arg;

	struct ev_fiber_ctx *ctx = thr->curr;
	thr->curr = NULL;
	ctx->fiber = fiber;

	// Destroy the fiber or put it back on the unused list.
	ev_fiber_ctx_destroy(ctx);

	return NULL;
}

static fiber_t *
ev_fiber_mtx_lock_func(fiber_t *fiber, void *arg)
{
	struct ev_fiber_mtx_impl *impl = arg;
	assert(impl);

	struct ev_fiber_ctx *ctx = ev_fiber_resume(fiber);

	// Append the fiber to the queue of waiting fibers.
	sllist_push_back(&impl->queue, &ctx->task._node);
#if !LELY_NO_THREADS
	// Unlock the inner mutex locked in ev_fiber_mtx_lock().
	mtx_unlock(&impl->mtx);
#endif

	return NULL;
}

static fiber_t *
ev_fiber_cnd_wait_func(fiber_t *fiber, void *arg)
{
	struct ev_fiber_cnd_wait *wait = arg;
	assert(wait);
	ev_fiber_cnd_t *cond = wait->cond;
	assert(cond);
	struct ev_fiber_cnd_impl *cond_impl = cond->_impl;
	assert(cond_impl);
	ev_fiber_mtx_t *mtx = wait->mtx;
	struct ev_fiber_mtx_impl *mtx_impl = mtx->_impl;
	assert(mtx_impl);

	struct ev_fiber_ctx *ctx = ev_fiber_resume(fiber);

	wait->task = &ctx->task;
#if !LELY_NO_THREADS
	mtx_lock(&cond_impl->mtx);
#endif
	sllist_push_back(&cond_impl->queue, &wait->node);
#if !LELY_NO_THREADS
	mtx_unlock(&cond_impl->mtx);
#endif

	// Unlock the mutex for the duration of the wait.
	mtx_impl->ctx = NULL;
	assert(mtx_impl->locked == 1);
	mtx_impl->locked = 0;
	struct ev_task *task =
			ev_task_from_node(sllist_pop_front(&mtx_impl->queue));
	if (task)
		mtx_impl->locked++;
#if !LELY_NO_THREADS
	mtx_unlock(&mtx_impl->mtx);
#endif
	// cppcheck-suppress duplicateCondition
	if (task)
		ev_exec_post(task->exec, task);

	return NULL;
}

static void
ev_fiber_cnd_wake(struct slnode *node)
{
	assert(node);
	struct ev_fiber_cnd_wait *wait =
			structof(node, struct ev_fiber_cnd_wait, node);
	ev_fiber_mtx_t *mtx = wait->mtx;
	assert(mtx);
	struct ev_fiber_mtx_impl *impl = mtx->_impl;
	assert(impl);
	struct ev_task *task = wait->task;
	assert(task);

#if !LELY_NO_THREADS
	mtx_lock(&impl->mtx);
#endif
	if (impl->locked) {
		sllist_push_back(&impl->queue, &task->_node);
		task = NULL;
	} else {
		assert(sllist_empty(&impl->queue));
		impl->locked = 1;
	}
#if !LELY_NO_THREADS
	mtx_unlock(&impl->mtx);
#endif

	if (task)
		ev_exec_post(task->exec, task);
}

#endif // !LELY_NO_MALLOC
