/**@file
 * This file is part of the event library; it contains the implementation of the
 * futures and promises functions.
 *
 * @see lely/ev/future.h
 *
 * @copyright 2018-2020 Lely Industries N.V.
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
#endif
#include <lely/libc/stddef.h>
#if !LELY_NO_THREADS
#include <lely/libc/threads.h>
#endif
#include <lely/ev/exec.h>
#include <lely/ev/future.h>
#include <lely/ev/task.h>
#include <lely/util/errnum.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef LELY_EV_FUTURE_MAX
#define LELY_EV_FUTURE_MAX MAX((LELY_VLA_SIZE_MAX / sizeof(ev_future_t *)), 1)
#endif

/// The state of a future.
enum ev_future_state {
	/// The future is waiting.
	EV_FUTURE_WAITING,
	/**
	 * The promise is in the process of being satisfied (i.e.,
	 * ev_promise_set_acquire() has been invoked).
	 */
	EV_FUTURE_SETTING,
	/**
	 * The future is ready (i.e., ev_promise_set_release() has been
	 * invoked).
	 */
	EV_FUTURE_READY
};

/// A future.
struct ev_future {
	/**
	 * The state of the future. Since futures are single-shot events, the
	 * state can be set only once.
	 */
#if LELY_NO_THREADS || (LELY_NO_ATOMICS && (!_WIN32 || defined(__MINGW32__)))
	int state;
#elif _WIN32 && !defined(__MINGW32__)
	volatile LONG state;
#else
	atomic_int state;
#endif
	/// The value of the future, set by ev_promise_set_release().
	void *value;
#if !LELY_NO_THREADS
	//// The mutex protecting the task queue.
	mtx_t mtx;
#endif
	/**
	 * The queue of tasks submitted by ev_future_submit() waiting
	 * for the future to become ready.
	 */
	struct sllist queue;
};

static ev_future_t *ev_future_init(ev_future_t *future);
static void ev_future_fini(ev_future_t *future);

/// A promise.
struct ev_promise {
	/**
	 * The number of references to this promise or its future. Once the
	 * reference count reaches zero, this struct is reclaimed.
	 */
#if LELY_NO_THREADS || (LELY_NO_ATOMICS && (!_WIN32 || defined(__MINGW32__)))
	size_t refcnt;
#elif _WIN64 && !defined(__MINGW32__)
	volatile LONGLONG refcnt;
#elif _WIN32 && !defined(__MINGW32__)
	volatile LONG refcnt;
#else
	atomic_size_t refcnt;
#endif
	/// The (destructor) function invoked when this struct is reclaimed.
	ev_promise_dtor_t *dtor;
	/// The future used to monitor if the promise has been satisfied.
	struct ev_future future;
};

#define EV_PROMISE_SIZE ALIGN(sizeof(ev_promise_t), _Alignof(max_align_t))

static inline ev_promise_t *ev_promise_from_future(const ev_future_t *future);

static void *ev_promise_alloc(size_t size);
static void ev_promise_free(void *ptr);

static ev_promise_t *ev_promise_init(
		ev_promise_t *promise, ev_promise_dtor_t *dtor);
static void ev_promise_fini(ev_promise_t *promise);

static void ev_future_pop(ev_future_t *future, struct sllist *queue,
		struct ev_task *task);

static ev_future_t *ev_future_when_all_nv(
		ev_exec_t *exec, size_t n, ev_future_t *future, va_list ap);

struct ev_future_when_all {
	size_t idx;
	ev_promise_t *promise;
	struct ev_task task;
	size_t n;
	ev_future_t *futures[];
};

static void ev_future_when_all_func(struct ev_task *task);

static ev_future_t *ev_future_when_any_nv(
		ev_exec_t *exec, size_t n, ev_future_t *future, va_list ap);

struct ev_future_when_any {
	size_t idx;
	ev_promise_t *promise;
	struct ev_task task;
};

static void ev_future_when_any_func(struct ev_task *task);

ev_promise_t *
ev_promise_create(size_t size, ev_promise_dtor_t *dtor)
{
	int errc = 0;

	ev_promise_t *promise = ev_promise_alloc(size);
	if (!promise) {
		errc = get_errc();
		goto error_alloc;
	}

	ev_promise_t *tmp = ev_promise_init(promise, dtor);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	promise = tmp;

	return promise;

error_init:
	ev_promise_free(promise);
error_alloc:
	set_errc(errc);
	return NULL;
}

ev_promise_t *
ev_promise_acquire(ev_promise_t *promise)
{
	if (promise)
#if LELY_NO_THREADS || (LELY_NO_ATOMICS && (!_WIN32 || defined(__MINGW32__)))
		promise->refcnt++;
#elif _WIN64 && !defined(__MINGW32__)
		InterlockedIncrementNoFence64(&promise->refcnt);
#elif _WIN32 && !defined(__MINGW32__)
		InterlockedIncrementNoFence(&promise->refcnt);
#else
		atomic_fetch_add_explicit(
				&promise->refcnt, 1, memory_order_relaxed);
#endif
	return promise;
}

void
ev_promise_release(ev_promise_t *promise)
{
	if (!promise)
		return;
#if LELY_NO_THREADS || (LELY_NO_ATOMICS && (!_WIN32 || defined(__MINGW32__)))
	if (!--promise->refcnt) {
#elif _WIN64 && !defined(__MINGW32__)
	if (!InterlockedDecrementRelease64(&promise->refcnt)) {
		MemoryBarrier();
#elif _WIN32 && !defined(__MINGW32__)
	if (!InterlockedDecrementRelease(&promise->refcnt)) {
		MemoryBarrier();
#else
	if (atomic_fetch_sub_explicit(&promise->refcnt, 1, memory_order_release)
			== 1) {
		atomic_thread_fence(memory_order_acquire);
#endif
		ev_promise_fini(promise);
		ev_promise_free(promise);
	}
}

int
ev_promise_is_unique(const ev_promise_t *promise)
{
	assert(promise);

#if LELY_NO_THREADS || LELY_NO_ATOMICS
	return promise->refcnt == 1;
#else
	// clang-format off
	return atomic_load_explicit((atomic_size_t *)&promise->refcnt,
			memory_order_relaxed) == 1;
	// clang-format on
#endif
}

void *
ev_promise_data(const ev_promise_t *promise)
{
	return promise ? (char *)promise + EV_PROMISE_SIZE : NULL;
}

int
ev_promise_set(ev_promise_t *promise, void *value)
{
	int result = ev_promise_set_acquire(promise);
	if (result)
		ev_promise_set_release(promise, value);
	return result;
}

int
ev_promise_set_acquire(ev_promise_t *promise)
{
	assert(promise);
	ev_future_t *future = &promise->future;

#if LELY_NO_THREADS || (LELY_NO_ATOMICS && (!_WIN32 || defined(__MINGW32__)))
	int result = future->state == EV_FUTURE_WAITING;
	if (result)
		future->state = EV_FUTURE_SETTING;
	return result;
#elif _WIN32 && !defined(__MINGW32__)
	// clang-format off
	return InterlockedCompareExchange(&future->state, EV_FUTURE_SETTING,
			EV_FUTURE_WAITING) == EV_FUTURE_WAITING;
	// clang-format on
#else
	int state = EV_FUTURE_WAITING;
	return atomic_compare_exchange_strong_explicit(&future->state, &state,
			EV_FUTURE_SETTING, memory_order_acq_rel,
			memory_order_acquire);
#endif
}

void
ev_promise_set_release(ev_promise_t *promise, void *value)
{
	assert(promise);
	ev_future_t *future = &promise->future;
#if LELY_NO_THREADS || LELY_NO_ATOMICS
	assert(future->state == EV_FUTURE_SETTING);
#else
	assert(atomic_load_explicit(&future->state, memory_order_acquire)
			== EV_FUTURE_SETTING);
#endif

	future->value = value;

	struct sllist queue;
	sllist_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&future->mtx);
#endif

	sllist_append(&queue, &future->queue);

#if LELY_NO_THREADS || LELY_NO_ATOMICS
	future->state = EV_FUTURE_READY;
#else
	atomic_store_explicit(
			&future->state, EV_FUTURE_READY, memory_order_release);
#endif

#if !LELY_NO_THREADS
	mtx_unlock(&future->mtx);
#endif

	ev_task_queue_post(&queue);
}

ev_future_t *
ev_promise_get_future(ev_promise_t *promise)
{
	assert(promise);

	return ev_future_acquire(&promise->future);
}

ev_future_t *
ev_future_acquire(ev_future_t *future)
{
	if (future)
		ev_promise_acquire(ev_promise_from_future(future));
	return future;
}

void
ev_future_release(ev_future_t *future)
{
	if (future)
		ev_promise_release(ev_promise_from_future(future));
}

int
ev_future_is_unique(const ev_future_t *future)
{
	return ev_promise_is_unique(ev_promise_from_future(future));
}

int
ev_future_is_ready(const ev_future_t *future)
{
#if LELY_NO_THREADS || LELY_NO_ATOMICS
	return future->state == EV_FUTURE_READY;
#else
	// clang-format off
	return atomic_load_explicit((atomic_int *)&future->state,
			memory_order_acquire) == EV_FUTURE_READY;
	// clang-format on
#endif
}

void *
ev_future_get(const ev_future_t *future)
{
	assert(future);
	assert(ev_future_is_ready(future));

	return future->value;
}

void
ev_future_submit(ev_future_t *future, struct ev_task *task)
{
	assert(future);
	assert(task);
	assert(task->exec);

	ev_exec_on_task_init(task->exec);

#if !LELY_NO_THREADS
	mtx_lock(&future->mtx);
#endif
	if (ev_future_is_ready(future)) {
#if !LELY_NO_THREADS
		mtx_unlock(&future->mtx);
#endif
		ev_exec_post(task->exec, task);
		ev_exec_on_task_fini(task->exec);
	} else {
		sllist_push_back(&future->queue, &task->_node);
#if !LELY_NO_THREADS
		mtx_unlock(&future->mtx);
#endif
	}
}

size_t
ev_future_cancel(ev_future_t *future, struct ev_task *task)
{
	assert(future);

	struct sllist queue;
	sllist_init(&queue);

	ev_future_pop(future, &queue, task);

	return ev_task_queue_post(&queue);
}

size_t
ev_future_abort(ev_future_t *future, struct ev_task *task)
{
	assert(future);

	struct sllist queue;
	sllist_init(&queue);

	ev_future_pop(future, &queue, task);

	return ev_task_queue_abort(&queue);
}

ev_future_t *
ev_future_when_all(ev_exec_t *exec, ev_future_t *future, ...)
{
	va_list ap;
	va_start(ap, future);
	ev_future_t *result = ev_future_when_all_v(exec, future, ap);
	va_end(ap);
	return result;
}

ev_future_t *
ev_future_when_all_v(ev_exec_t *exec, ev_future_t *future, va_list ap)
{
	size_t n;
	va_list aq;
	va_copy(aq, ap);
	ev_future_t *arg = future;
	// NOLINTNEXTLINE(clang-analyzer-valist.Uninitialized)
	for (n = 0; arg; n++, arg = va_arg(aq, ev_future_t *))
		;
	va_end(aq);
	return ev_future_when_all_nv(exec, n, future, ap);
}

ev_future_t *
ev_future_when_all_n(ev_exec_t *exec, size_t n, ev_future_t *const *futures)
{
	assert(exec);
	assert(!n || futures);

	ev_promise_t *promise = ev_promise_create(
			sizeof(struct ev_future_when_all)
					+ n * sizeof(ev_future_t *),
			NULL);
	if (!promise)
		return NULL;

	struct ev_future_when_all *when = ev_promise_data(promise);
	when->idx = 0;
	when->promise = promise;
	when->task = (struct ev_task)EV_TASK_INIT(
			exec, &ev_future_when_all_func);

	if ((when->n = n)) {
		for (size_t i = 0; i < n; i++) {
			assert(futures[i]);
			when->futures[i] = ev_future_acquire(futures[i]);
		}
		ev_future_submit(when->futures[when->idx], &when->task);
	} else {
		ev_promise_set(promise, NULL);
	}

	return ev_promise_get_future(promise);
}

ev_future_t *
ev_future_when_any(ev_exec_t *exec, ev_future_t *future, ...)
{
	va_list ap;
	va_start(ap, future);
	ev_future_t *result = ev_future_when_any_v(exec, future, ap);
	va_end(ap);
	return result;
}

ev_future_t *
ev_future_when_any_v(ev_exec_t *exec, ev_future_t *future, va_list ap)
{
	size_t n;
	va_list aq;
	va_copy(aq, ap);
	ev_future_t *arg = future;
	// NOLINTNEXTLINE(clang-analyzer-valist.Uninitialized)
	for (n = 0; arg; n++, arg = va_arg(aq, ev_future_t *))
		;
	va_end(aq);
	return ev_future_when_any_nv(exec, n, future, ap);
}

ev_future_t *
ev_future_when_any_n(ev_exec_t *exec, size_t n, ev_future_t *const *futures)
{
	assert(exec);
	assert(!n || futures);

	ev_promise_t *promise = ev_promise_create(
			n * sizeof(struct ev_future_when_any), NULL);
	if (!promise)
		return NULL;

	if (n) {
		struct ev_future_when_any *wait = ev_promise_data(promise);
		for (size_t i = 0; i < n; i++) {
			assert(futures[i]);
			wait[i].idx = i;
			wait[i].promise = ev_promise_acquire(promise);
			wait[i].task = (struct ev_task)EV_TASK_INIT(
					exec, &ev_future_when_any_func);
			ev_future_submit(futures[i], &wait[i].task);
		}
	} else {
		ev_promise_set(promise, NULL);
	}

	ev_future_t *future = ev_promise_get_future(promise);
#if LELY_NO_THREADS || LELY_NO_ATOMICS
	assert(promise->refcnt >= 2);
#else
	assert(atomic_load(&promise->refcnt) >= 2);
#endif
	// Prevent false positive in scan-build.
#ifndef __clang_analyzer__
	ev_promise_release(promise);
#endif
	// NOLINTNEXTLINE(clang-analyzer-unix.Malloc)
	return future;
}

static ev_future_t *
ev_future_init(ev_future_t *future)
{
	assert(future);

#if LELY_NO_THREADS || LELY_NO_ATOMICS || (_WIN32 && !defined(__MINGW32__))
	future->state = EV_FUTURE_WAITING;
#else
	atomic_init(&future->state, EV_FUTURE_WAITING);
#endif

	future->value = NULL;

#if !LELY_NO_THREADS
	if (mtx_init(&future->mtx, mtx_plain) != thrd_success)
		return NULL;
#endif

	sllist_init(&future->queue);

	return future;
}

static void
ev_future_fini(ev_future_t *future)
{
	assert(future);

	ev_task_queue_post(&future->queue);

#if !LELY_NO_THREADS
	mtx_destroy(&future->mtx);
#endif
}

static void
ev_future_pop(ev_future_t *future, struct sllist *queue, struct ev_task *task)
{
	assert(future);
	assert(queue);

#if !LELY_NO_THREADS
	mtx_lock(&future->mtx);
#endif
	if (!task)
		sllist_append(queue, &future->queue);
	else if (sllist_remove(&future->queue, &task->_node))
		sllist_push_back(queue, &task->_node);
#if !LELY_NO_THREADS
	mtx_unlock(&future->mtx);
#endif
}

static inline ev_promise_t *
ev_promise_from_future(const ev_future_t *future)
{
	assert(future);

	return structof(future, ev_promise_t, future);
}

static void *
ev_promise_alloc(size_t size)
{
	// cppcheck-suppress AssignmentAddressToInteger
	void *ptr = calloc(1, EV_PROMISE_SIZE + size);
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

static void
ev_promise_free(void *ptr)
{
	free(ptr);
}

static ev_promise_t *
ev_promise_init(ev_promise_t *promise, ev_promise_dtor_t *dtor)
{
	assert(promise);

#if LELY_NO_THREADS || LELY_NO_ATOMICS || (_WIN32 && !defined(__MINGW32__))
	promise->refcnt = 1;
#else
	atomic_init(&promise->refcnt, 1);
#endif

	promise->dtor = dtor;

	if (!ev_future_init(&promise->future))
		return NULL;

	return promise;
}

static void
ev_promise_fini(ev_promise_t *promise)
{
	assert(promise);

	ev_future_fini(&promise->future);

	if (promise->dtor)
		promise->dtor(ev_promise_data(promise));
}

static ev_future_t *
ev_future_when_all_nv(
		ev_exec_t *exec, size_t n, ev_future_t *future, va_list ap)
{
	if (n <= LELY_EV_FUTURE_MAX) {
#if __STDC_NO_VLA__ || defined(_MSC_VER)
		ev_future_t *futures[LELY_EV_FUTURE_MAX];
#else
		ev_future_t *futures[n ? n : 1];
#endif
		for (size_t i = 0; i < n; i++) {
			futures[i] = future;
			future = va_arg(ap, ev_future_t *);
		}
		return ev_future_when_all_n(exec, n, futures);
	} else {
		ev_future_t **futures = malloc(n * sizeof(*futures));
		if (!futures) {
#if !LELY_NO_ERRNO
			set_errc(errno2c(errno));
#endif
			return NULL;
		}
		for (size_t i = 0; i < n; i++) {
			futures[i] = future;
			future = va_arg(ap, ev_future_t *);
		}
		ev_future_t *result = ev_future_when_all_n(exec, n, futures);
#if !LELY_NO_ERRNO
		int errsv = errno;
#endif
		free(futures);
#if !LELY_NO_ERRNO
		errno = errsv;
#endif
		return result;
	}
}

static void
ev_future_when_all_func(struct ev_task *task)
{
	assert(task);
	struct ev_future_when_all *when =
			structof(task, struct ev_future_when_all, task);

	ev_future_t **pfuture = when->futures + when->idx;
	if (ev_future_is_ready(*pfuture)) {
		ev_future_release(*pfuture);
		while (++when->idx < when->n && ev_future_is_unique(*++pfuture))
			ev_future_release(*pfuture);
		if (when->idx < when->n) {
			ev_future_submit(*pfuture, &when->task);
			return;
		}
	} else {
		for (size_t i = when->idx; i < when->n; i++)
			ev_future_release(*pfuture++);
	}

	ev_promise_set(when->promise, &when->idx);
	ev_promise_release(when->promise);
}

static ev_future_t *
ev_future_when_any_nv(
		ev_exec_t *exec, size_t n, ev_future_t *future, va_list ap)
{
	if (n <= LELY_EV_FUTURE_MAX) {
#if __STDC_NO_VLA__ || defined(_MSC_VER)
		ev_future_t *futures[LELY_EV_FUTURE_MAX];
#else
		ev_future_t *futures[n ? n : 1];
#endif
		for (size_t i = 0; i < n; i++) {
			futures[i] = future;
			future = va_arg(ap, ev_future_t *);
		}
		return ev_future_when_any_n(exec, n, futures);
	} else {
		ev_future_t **futures = malloc(n * sizeof(*futures));
		if (!futures) {
#if !LELY_NO_ERRNO
			set_errc(errno2c(errno));
#endif
			return NULL;
		}
		for (size_t i = 0; i < n; i++) {
			futures[i] = future;
			future = va_arg(ap, ev_future_t *);
		}
		ev_future_t *result = ev_future_when_any_n(exec, n, futures);
#if !LELY_NO_ERRNO
		int errsv = errno;
#endif
		free(futures);
#if !LELY_NO_ERRNO
		errno = errsv;
#endif
		return result;
	}
}

static void
ev_future_when_any_func(struct ev_task *task)
{
	assert(task);
	struct ev_future_when_any *when =
			structof(task, struct ev_future_when_any, task);

	ev_promise_set(when->promise, &when->idx);
	ev_promise_release(when->promise);
}

#endif // !LELY_NO_MALLOC
