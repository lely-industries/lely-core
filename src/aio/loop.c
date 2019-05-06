/**@file
 * This file is part of the asynchronous I/O library; it contains ...
 *
 * @see lely/aio/loop.h, lely/aio/future.h
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
#if !_WIN32
#include <lely/libc/stdatomic.h>
#endif
#include <lely/libc/threads.h>
#endif
#include <lely/aio/future.h>
#include <lely/aio/loop.h>
#include <lely/aio/queue.h>
#include <lely/util/dllist.h>
#include <lely/util/errnum.h>
#include <lely/util/time.h>
#include <lely/util/util.h>

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef LELY_AIO_LOOP_MAX_FUTURES
/**
 * The maximum number of unused futures per event loop, as well as the maximum
 * number of futures passed to aio_loop_get() or aio_loop_get_until() without
 * triggering dynamic memory allocation. The value is chosen to equal
 * `MAXIMUM_WAIT_OBJECTS - 1` on Windows, where the `- 1` reflects the internal
 * future of the event loop.
 */
#define LELY_AIO_LOOP_MAX_FUTURES 63
#endif

/// A thread-local variable whose address functions as a thread id.
#if LELY_NO_THREADS
static const int aio_loop_thread_id;
#else
static _Thread_local const int aio_loop_thread_id;
#endif

struct aio_future_node {
	aio_future_t *proxy;
	struct dlnode node;
};

#if LELY_NO_THREADS
static struct aio_future_node aio_future_nodes_[LELY_AIO_LOOP_MAX_FUTURES];
static struct aio_future_node *aio_future_nodes;
#else
static _Thread_local struct aio_future_node
		aio_future_nodes_[LELY_AIO_LOOP_MAX_FUTURES];
static _Thread_local struct aio_future_node *aio_future_nodes;
#endif

/**
 * An asynchronous I/O future. A future is part of a promise which is managed by
 * an event loop. Its members are protected by the mutex from that loop.
 */
struct aio_future {
	/**
	 * The state of the future. Since futures are single-shot events, the
	 * state can be set only once.
	 */
	enum aio_future_state state;
#if !LELY_NO_THREADS
	/**
	 * The condition variable used by threads to wait on the future. This
	 * variable is signaled when the future becomes ready or when a task is
	 * submitted to the event loop.
	 */
	cnd_t cond;
	/// The number of threads waiting on #cond.
	size_t waiting;
#endif
	/**
	 * Equal to `&aio_loop_thread_id` if the current thread is polling for
	 * events, and `NULL` if not.
	 */
	const int *polling;
	/// The list of futures to be signaled when this future is signaled.
	struct dllist list;
	/**
	 * The queue of tasks submitted by aio_future_submit_wait() waiting for
	 * the future to become ready.
	 */
	struct aio_queue queue;
};

static aio_future_t *aio_future_init(aio_future_t *future);
static void aio_future_fini(aio_future_t *future);

/// Wakes up one thread waiting on the future.
static void aio_future_signal(aio_future_t *future, aio_poll_t *poll);

/// Wakes up all threads waiting on the future.
static void aio_future_broadcast(aio_future_t *future, aio_poll_t *poll);

/**
 * An asynchronous I/O promise. A promise is managed by an event loop. Its
 * members are protected by the mutex from that loop.
 */
struct aio_promise {
	/// A pointer to the event loop managing the promise.
	aio_loop_t *loop;
	/**
	 * A pointer to the default executor for tasks submitted with
	 * #aio_future_submit_wait().
	 */
	aio_exec_t *exec;
	/**
	 * The number of references to this promise or its future. Once the
	 * reference count reaches zero, this struct is reclaimed by the event
	 * loop.
	 */
#if LELY_NO_THREADS || __STDC_NO_ATOMICS__
	size_t refcnt;
#elif _WIN32
	volatile LONG refcnt;
#else
	atomic_size_t refcnt;
#endif
	/// The (destructor) function invoked when this struct is reclaimed.
	aio_dtor_t *dtor;
	/// The argument passed to #dtor.
	void *arg;
	/// The value of the promise, set by aio_promise_set_value().
	void *value;
	/// The error code of the promise, set by aio_promise_set_errc().
	int errc;
	/// The future used to monitor if the promise has been satisfied.
	struct aio_future future;
	/// The node of this promise in the list of unused promises.
	struct dlnode node;
};

static inline aio_promise_t *aio_promise_from_future(
		const aio_future_t *future);

static aio_promise_t *aio_promise_alloc(void);
static void aio_promise_free(aio_promise_t *promise);

/// An asynchronous I/O event loop.
struct aio_loop {
	/// A pointer to the interface used to poll for events (can be `NULL`).
	aio_poll_t *poll;
	/// The polling task.
	struct aio_task task;
#if !LELY_NO_THREADS
	/**
	 * The mutex protecting the task queue, as well as all promises and
	 * futures managed by this event loop.
	 */
	mtx_t mtx;
#endif
	/**
	 * The future used to stop the event loop with aio_loop_stop(). All
	 * wait functions implicitly monitor this future.
	 */
	struct aio_future future;
	/// The list of unused promises.
	struct dllist unused;
	/// The number of promises in #unused.
	size_t nunused;
	/// The queue of pending tasks, including #task if #poll is not `NULL`.
	struct aio_queue queue;
	/**
	 * The number of pending tasks. This equals the number of calls to
	 * aio_loop_on_task_started() minus those to
	 * aio_loop_on_task_finished().
	 */
	size_t ntasks;
};

static int aio_loop_is_ready(const aio_loop_t *loop,
		aio_future_t *const *futures, size_t nfutures);

static aio_future_t *aio_loop_create_proxy(aio_loop_t *loop,
		aio_future_t *const *futures, size_t nfutures);
static void aio_loop_destroy_proxy(aio_loop_t *loop,
		aio_future_t *const *futures, size_t nfutures,
		aio_future_t *proxy);

static void aio_loop_do_stop(aio_loop_t *loop);
static void aio_loop_do_post(aio_loop_t *loop, struct aio_task *task);
static void aio_loop_do_on_task_started(aio_loop_t *loop);
static void aio_loop_do_on_task_finished(aio_loop_t *loop);

void *
aio_loop_alloc(void)
{
	void *ptr = malloc(sizeof(aio_loop_t));
	if (!ptr)
		set_errc(errno2c(errno));
	return ptr;
}

aio_loop_t *
aio_loop_init(aio_loop_t *loop, aio_poll_t *poll)
{
	assert(loop);

	int errc = 0;

	loop->poll = poll;
	loop->task = (struct aio_task)AIO_TASK_INIT(NULL, NULL);

#if !LELY_NO_THREADS
	if (mtx_init(&loop->mtx, mtx_plain) != thrd_success) {
		errc = get_errc();
		goto error_init_mtx;
	}
#endif

	if (!aio_future_init(&loop->future)) {
		errc = get_errc();
		goto error_init_future;
	}

	dllist_init(&loop->unused);
	loop->nunused = 0;

	aio_queue_init(&loop->queue);
	loop->ntasks = 0;

	if (loop->poll)
		aio_queue_push(&loop->queue, &loop->task);

	return loop;

error_init_future:
#if !LELY_NO_THREADS
	mtx_destroy(&loop->mtx);
error_init_mtx:
#endif
	set_errc(errc);
	return NULL;
}

void
aio_loop_fini(aio_loop_t *loop)
{
	assert(loop);

	dllist_foreach (&loop->unused, node) {
		dllist_remove(&loop->unused, node);
		aio_promise_free(structof(node, aio_promise_t, node));
	}

	aio_future_fini(&loop->future);

#if !LELY_NO_THREADS
	mtx_destroy(&loop->mtx);
#endif
}

void
aio_loop_free(void *ptr)
{
	free(ptr);
}

aio_loop_t *
aio_loop_create(aio_poll_t *poll)
{
	int errc = 0;

	aio_loop_t *loop = aio_loop_alloc();
	if (!loop) {
		errc = get_errc();
		goto error_alloc;
	}

	aio_loop_t *tmp = aio_loop_init(loop, poll);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	loop = tmp;

	return loop;

error_init:
	aio_loop_free(loop);
error_alloc:
	set_errc(errc);
	return NULL;
}

void
aio_loop_destroy(aio_loop_t *loop)
{
	if (loop) {
		aio_loop_fini(loop);
		aio_loop_free(loop);
	}
}

aio_poll_t *
aio_loop_get_poll(const aio_loop_t *loop)
{
	assert(loop);

	return loop->poll;
}

struct aio_task *
aio_loop_get(aio_loop_t *loop, aio_future_t *const *futures, size_t nfutures)
{
	assert(loop);

	int errc = get_errc();
	struct aio_task *task = NULL;

#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif

	if (!loop->ntasks)
		aio_loop_do_stop(loop);

	if (aio_loop_is_ready(loop, futures, nfutures))
		goto done;

	task = aio_queue_front(&loop->queue);
	if (task && task != &loop->task)
		goto done;

	aio_future_t *proxy = aio_loop_create_proxy(loop, futures, nfutures);
	if (!proxy) {
		errc = get_errc();
		task = NULL;
		goto done;
	}

#if !LELY_NO_THREADS
	if (!task) {
		proxy->waiting++;
		do {
			if (cnd_wait(&proxy->cond, &loop->mtx)
					!= thrd_success) {
				errc = get_errc();
				break;
			}
			if (aio_loop_is_ready(loop, futures, nfutures))
				break;
		} while (!(task = aio_queue_front(&loop->queue)));
		proxy->waiting--;
	}
#endif

	if (task == &loop->task) {
		assert(loop->poll);
		assert(!proxy->polling);
		aio_queue_pop(&loop->queue);
		task = NULL;
		proxy->polling = &aio_loop_thread_id;
		do {
			// Do not wait if another task is already available.
			int msec = aio_queue_empty(&loop->queue) ? -1 : 0;
#if !LELY_NO_THREADS
			mtx_unlock(&loop->mtx);
#endif
			set_errc(0);
			aio_poll_wait(loop->poll, msec);
			int errsv = get_errc();
#if !LELY_NO_THREADS
			mtx_lock(&loop->mtx);
#endif
			if (errsv) {
				errc = errsv;
				break;
			}
			if (aio_loop_is_ready(loop, futures, nfutures))
				break;
		} while (!(task = aio_queue_front(&loop->queue)));
		proxy->polling = NULL;
		// Re-queue the polling task.
		aio_loop_do_post(loop, &loop->task);
	}

	aio_loop_destroy_proxy(loop, futures, nfutures, proxy);

done:
	if (task)
		aio_queue_pop(&loop->queue);
	else if (!aio_queue_empty(&loop->queue))
		aio_future_signal(&loop->future, aio_loop_get_poll(loop));

#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif

	set_errc(errc);
	return task;
}

struct aio_task *
aio_loop_get_until(aio_loop_t *loop, aio_future_t *const *futures,
		size_t nfutures, const struct timespec *abs_time)
{
	assert(loop);

	int errc = get_errc();
	struct aio_task *task = NULL;

#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif

	if (!loop->ntasks)
		aio_loop_do_stop(loop);

	if (aio_loop_is_ready(loop, futures, nfutures))
		goto done;

	task = aio_queue_front(&loop->queue);
	if (task && task != &loop->task)
		goto done;

	aio_future_t *proxy = aio_loop_create_proxy(loop, futures, nfutures);
	if (!proxy) {
		errc = get_errc();
		task = NULL;
		goto done;
	}

#if !LELY_NO_THREADS && !LELY_NO_TIMEDWAIT
	if (!task && abs_time) {
		proxy->waiting++;
		do {
			int result = cnd_timedwait(
					&proxy->cond, &loop->mtx, abs_time);
			if (result != thrd_success) {
				errc = result == thrd_timedout
						? errnum2c(ERRNUM_TIMEDOUT)
						: get_errc();
				break;
			}
			if (aio_loop_is_ready(loop, futures, nfutures))
				break;
		} while (!(task = aio_queue_front(&loop->queue)));
		proxy->waiting--;
	}
#endif

	if (task == &loop->task) {
		assert(loop->poll);
		assert(!proxy->polling);
		aio_queue_pop(&loop->queue);
		task = NULL;
		proxy->polling = &aio_loop_thread_id;
		do {
			int64_t msec = 0;
#if !LELY_NO_TIMEDWAIT
			if (abs_time && aio_queue_empty(&loop->queue)) {
				struct timespec now = { 0, 0 };
				if (!timespec_get(&now, TIME_UTC)) {
					errc = get_errc();
					break;
				}
				if (timespec_cmp(abs_time, &now) <= 0) {
					errc = errnum2c(ERRNUM_TIMEDOUT);
					break;
				}
				msec = timespec_diff_msec(abs_time, &now);
				if (!msec)
					// Prevent a busy loop due to rounding.
					msec = 1;
				else if (msec > INT_MAX)
					msec = INT_MAX;
			}
#endif
#if !LELY_NO_THREADS
			mtx_unlock(&loop->mtx);
#endif
			set_errc(0);
			aio_poll_wait(loop->poll, msec);
			int errsv = get_errc();
#if !LELY_NO_THREADS
			mtx_lock(&loop->mtx);
#endif
			if (errsv) {
				errc = errsv;
				break;
			}
			if (aio_loop_is_ready(loop, futures, nfutures))
				break;
		} while (!(task = aio_queue_front(&loop->queue)) && abs_time);
		proxy->polling = NULL;
		// Re-queue the polling task.
		aio_loop_do_post(loop, &loop->task);
	}

	aio_loop_destroy_proxy(loop, futures, nfutures, proxy);

done:
	if (task)
		aio_queue_pop(&loop->queue);
	else if (!aio_queue_empty(&loop->queue))
		aio_future_signal(&loop->future, aio_loop_get_poll(loop));

#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif

	set_errc(errc);
	return task;
}

void
aio_loop_post(aio_loop_t *loop, struct aio_task *task)
{
	assert(loop);

#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	aio_loop_do_on_task_started(loop);
	aio_loop_do_post(loop, task);
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif
}

void
aio_loop_on_task_started(aio_loop_t *loop)
{
	assert(loop);

#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	aio_loop_do_on_task_started(loop);
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif
}

void
aio_loop_on_task_finished(aio_loop_t *loop)
{
	assert(loop);

#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	aio_loop_do_on_task_finished(loop);
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif
}

size_t
aio_loop_run(aio_loop_t *loop, aio_future_t *const *futures, size_t nfutures)
{
	size_t n = 0;
	struct aio_task *task;
	while ((task = aio_loop_get(loop, futures, nfutures))) {
		aio_exec_run(task->exec, task);
		n += n < SIZE_MAX;
	}
	return n;
}

size_t
aio_loop_run_until(aio_loop_t *loop, aio_future_t *const *futures,
		size_t nfutures, const struct timespec *abs_time)
{
	size_t n = 0;
	struct aio_task *task;
	while ((task = aio_loop_get_until(loop, futures, nfutures, abs_time))) {
		aio_exec_run(task->exec, task);
		n += n < SIZE_MAX;
	}
	return n;
}

void
aio_loop_stop(aio_loop_t *loop)
{
	assert(loop);

#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	aio_loop_do_stop(loop);
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif
}

int
aio_loop_stopped(const aio_loop_t *loop)
{
	assert(loop);

#if !LELY_NO_THREADS
	mtx_lock((mtx_t *)&loop->mtx);
#endif
	int stopped = loop->future.state != AIO_FUTURE_WAITING;
#if !LELY_NO_THREADS
	mtx_unlock((mtx_t *)&loop->mtx);
#endif
	return stopped;
}

void
aio_loop_restart(aio_loop_t *loop)
{
	assert(loop);

#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	loop->future.state = AIO_FUTURE_WAITING;
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif
}

aio_promise_t *
aio_promise_create(
		aio_loop_t *loop, aio_exec_t *exec, aio_dtor_t *dtor, void *arg)
{
	assert(loop);
	assert(exec);

	struct dlnode *node = NULL;
#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	if ((node = dllist_pop_front(&loop->unused)))
		loop->nunused--;
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif
	aio_promise_t *promise =
			node ? structof(node, aio_promise_t, node) : NULL;

	if (!promise) {
		promise = aio_promise_alloc();
		if (!promise)
			return NULL;
		promise->loop = loop;
	}

	assert(promise->loop == loop);
	promise->exec = exec;

	promise->dtor = dtor;
	promise->arg = arg;

	assert(promise->value == NULL);
	assert(promise->errc == 0);

	aio_future_t *future = aio_future_create(promise);
	future->state = AIO_FUTURE_WAITING;
#if !LELY_NO_THREADS
	assert(future->waiting == 0);
#endif
	assert(future->polling == NULL);
	assert(dllist_empty(&future->list));
	assert(aio_queue_empty(&future->queue));

	dlnode_init(&promise->node);

	return promise;
}

void
aio_promise_destroy(aio_promise_t *promise)
{
	if (promise) {
		aio_promise_set_errc(promise, errnum2c(ERRNUM_CANCELED));
		aio_future_destroy(&promise->future);
	}
}

void
aio_promise_cancel(aio_promise_t *promise)
{
	assert(promise);
	aio_loop_t *loop = promise->loop;
	assert(loop);
	aio_future_t *future = &promise->future;

	struct aio_queue queue;
	aio_queue_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	if (future->state == AIO_FUTURE_WAITING) {
		future->state = AIO_FUTURE_CANCELED;

		aio_queue_move(&queue, &future->queue, NULL);

		aio_future_broadcast(future, aio_loop_get_poll(loop));
	}
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif

	aio_queue_cancel(&queue, 0);
}

void
aio_promise_set_value(aio_promise_t *promise, void *value)
{
	assert(promise);
	aio_loop_t *loop = promise->loop;
	assert(loop);
	aio_future_t *future = &promise->future;

	struct aio_queue queue;
	aio_queue_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	if (future->state == AIO_FUTURE_WAITING) {
		promise->value = value;
		future->state = AIO_FUTURE_VALUE;

		aio_queue_move(&queue, &future->queue, NULL);

		aio_future_broadcast(future, aio_loop_get_poll(loop));
	}
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif

	aio_queue_cancel(&queue, 0);
}

void
aio_promise_set_errc(aio_promise_t *promise, int errc)
{
	assert(promise);
	aio_loop_t *loop = promise->loop;
	assert(loop);
	aio_future_t *future = &promise->future;

	struct aio_queue queue;
	aio_queue_init(&queue);

#if !LELY_NO_THREADS
	mtx_lock(&loop->mtx);
#endif
	if (future->state == AIO_FUTURE_WAITING) {
		promise->errc = errc;
		future->state = AIO_FUTURE_ERROR;

		aio_queue_move(&queue, &future->queue, NULL);

		aio_future_broadcast(future, aio_loop_get_poll(loop));
	}
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif

	aio_queue_cancel(&queue, 0);
}

aio_future_t *
aio_future_create(aio_promise_t *promise)
{
	assert(promise);

#if LELY_NO_THREADS || __STDC_NO_ATOMICS__
	promise->refcnt++;
#elif _WIN32
#ifdef __MINGW32__
	InterlockedIncrement(&promise->refcnt);
#else
	InterlockedIncrementNoFence(&promise->refcnt);
#endif
#else
	atomic_fetch_add_explicit(&promise->refcnt, 1, memory_order_relaxed);
#endif
	return &promise->future;
}

void
aio_future_destroy(aio_future_t *future)
{
	if (future) {
		aio_promise_t *promise = aio_promise_from_future(future);
#if LELY_NO_THREADS || __STDC_NO_ATOMICS__
		if (!--promise->refcnt) {
#elif _WIN32
		if (!InterlockedDecrementRelease(&promise->refcnt)) {
			MemoryBarrier();
#else
		// clang-format off
		if (atomic_fetch_sub_explicit(&promise->refcnt, 1,
				memory_order_release) == 1) {
			// clang-format on
			atomic_thread_fence(memory_order_acquire);
#endif
			aio_loop_t *loop = promise->loop;
			assert(loop);

			promise->exec = NULL;

			aio_dtor_t *dtor = promise->dtor;
			promise->dtor = NULL;
			void *arg = promise->arg;
			promise->arg = NULL;

			promise->value = NULL;
			promise->errc = 0;

#if !LELY_NO_THREADS
			mtx_lock(&loop->mtx);
			assert(future->waiting == 0);
#endif
			assert(future->polling == NULL);
			assert(dllist_empty(&future->list));
			assert(aio_queue_empty(&future->queue));
			if (loop->nunused < LELY_AIO_LOOP_MAX_FUTURES) {
				dllist_push_back(&loop->unused, &promise->node);
				loop->nunused++;
#if !LELY_NO_THREADS
				mtx_unlock(&loop->mtx);
#endif
			} else {
#if !LELY_NO_THREADS
				mtx_unlock(&loop->mtx);
#endif
				aio_promise_free(promise);
			}

			if (dtor)
				dtor(arg);
		}
	}
}

aio_loop_t *
aio_future_get_loop(const aio_future_t *future)
{
	const aio_promise_t *promise = aio_promise_from_future(future);

	return promise->loop;
}

aio_exec_t *
aio_future_get_exec(const aio_future_t *future)
{
	const aio_promise_t *promise = aio_promise_from_future(future);

	return promise->exec;
}

enum aio_future_state
aio_future_get_state(const aio_future_t *future)
{
#if !LELY_NO_THREADS
	const aio_promise_t *promise = aio_promise_from_future(future);
	aio_loop_t *loop = promise->loop;
	assert(loop);

	mtx_lock(&loop->mtx);
#endif
	enum aio_future_state state = future->state;
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif
	return state;
}

int
aio_future_is_ready(const aio_future_t *future)
{
	return aio_future_get_state(future) != AIO_FUTURE_WAITING;
}

int
aio_future_is_canceled(const aio_future_t *future)
{
	return aio_future_get_state(future) == AIO_FUTURE_CANCELED;
}

int
aio_future_has_value(const aio_future_t *future)
{
	return aio_future_get_state(future) == AIO_FUTURE_VALUE;
}

int
aio_future_has_errc(const aio_future_t *future)
{
	return aio_future_get_state(future) == AIO_FUTURE_ERROR;
}

void *
aio_future_get_value(const aio_future_t *future)
{
	const aio_promise_t *promise = aio_promise_from_future(future);
#if !LELY_NO_THREADS
	aio_loop_t *loop = promise->loop;
	assert(loop);

	mtx_lock(&loop->mtx);
#endif
	assert(future->state == AIO_FUTURE_VALUE);
	void *value = promise->value;
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif
	return value;
}

int
aio_future_get_errc(const aio_future_t *future)
{
	const aio_promise_t *promise = aio_promise_from_future(future);
#if !LELY_NO_THREADS
	aio_loop_t *loop = promise->loop;
	assert(loop);

	mtx_lock(&loop->mtx);
#endif
	assert(future->state == AIO_FUTURE_ERROR);
	int errc = promise->errc;
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif
	return errc;
}

void
aio_future_submit_wait(aio_future_t *future, struct aio_task *task)
{
	aio_promise_t *promise = aio_promise_from_future(future);
#if !LELY_NO_THREADS
	aio_loop_t *loop = promise->loop;
	assert(loop);
#endif
	assert(task);

	if (!task->exec)
		task->exec = promise->exec;
	task->errc = task->func ? errnum2c(ERRNUM_INPROGRESS) : 0;

	if (task->func) {
		aio_exec_on_task_started(task->exec);
#if !LELY_NO_THREADS
		mtx_lock(&loop->mtx);
#endif
		if (future->state != AIO_FUTURE_WAITING) {
#if !LELY_NO_THREADS
			mtx_unlock(&loop->mtx);
#endif
			task->errc = 0;
			aio_exec_post(task->exec, task);
			aio_exec_on_task_finished(task->exec);
		} else {
			aio_queue_push(&future->queue, task);
#if !LELY_NO_THREADS
			mtx_unlock(&loop->mtx);
#endif
		}
	}
}

size_t
aio_future_cancel(aio_future_t *future, struct aio_task *task)
{
	assert(future);

	struct aio_queue queue;
	aio_queue_init(&queue);

#if !LELY_NO_THREADS
	aio_promise_t *promise = aio_promise_from_future(future);
	aio_loop_t *loop = promise->loop;
	assert(loop);

	mtx_lock(&loop->mtx);
#endif
	aio_queue_move(&queue, &future->queue, task);
#if !LELY_NO_THREADS
	mtx_unlock(&loop->mtx);
#endif

	return aio_queue_cancel(&queue, errnum2c(ERRNUM_CANCELED));
}

void
aio_future_run_wait(aio_future_t *future)
{
	aio_loop_t *loop = aio_future_get_loop(future);

	aio_loop_run(loop, &future, 1);
}

void
aio_future_run_wait_until(aio_future_t *future, const struct timespec *abs_time)
{
	aio_loop_t *loop = aio_future_get_loop(future);

	aio_loop_run_until(loop, &future, 1, abs_time);
}

static aio_future_t *
aio_future_init(aio_future_t *future)
{
	assert(future);

	future->state = AIO_FUTURE_WAITING;

#if !LELY_NO_THREADS
	if (cnd_init(&future->cond) != thrd_success)
		return NULL;
	future->waiting = 0;
#endif
	future->polling = NULL;

	dllist_init(&future->list);

	aio_queue_init(&future->queue);

	return future;
}

static void
aio_future_fini(aio_future_t *future)
{
	assert(future);

#if LELY_NO_THREADS
	(void)future;
#else
	cnd_destroy(&future->cond);
#endif
}

static void
aio_future_signal(aio_future_t *future, aio_poll_t *poll)
{
	assert(future);

	struct dlnode *node = dllist_first(&future->list);
	for (;;) {
#if !LELY_NO_THREADS
		if (future->waiting) {
			cnd_signal(&future->cond);
			return;
		}
#endif
		if (future->polling && future->polling != &aio_loop_thread_id) {
			aio_poll_stop(poll);
			return;
		}
		if (!node)
			return;
		future = structof(node, struct aio_future_node, node)->proxy;
		node = node->next;
	}
}

static void
aio_future_broadcast(aio_future_t *future, aio_poll_t *poll)
{
	assert(future);

	struct dlnode *node = dllist_first(&future->list);
	for (;;) {
#if !LELY_NO_THREADS
		if (future->waiting)
			cnd_broadcast(&future->cond);
#endif
		if (future->polling && future->polling != &aio_loop_thread_id)
			aio_poll_stop(poll);
		if (!node)
			return;
		future = structof(node, struct aio_future_node, node)->proxy;
		node = node->next;
	}
}

static inline aio_promise_t *
aio_promise_from_future(const aio_future_t *future)
{
	assert(future);

	return structof(future, aio_promise_t, future);
}

static aio_promise_t *
aio_promise_alloc(void)
{
	int errc = 0;

	aio_promise_t *promise = malloc(sizeof(*promise));
	if (!promise) {
		errc = errno2c(errno);
		goto error_malloc_promise;
	}

	promise->loop = NULL;
	promise->exec = NULL;

#if LELY_NO_THREADS || __STDC_NO_ATOMICS__ || _WIN32
	promise->refcnt = 0;
#else
	atomic_init(&promise->refcnt, 0);
#endif
	promise->dtor = NULL;
	promise->arg = NULL;

	promise->value = NULL;
	promise->errc = 0;

	if (!aio_future_init(&promise->future)) {
		errc = get_errc();
		goto error_init_future;
	}

	dlnode_init(&promise->node);

	return promise;

error_init_future:
	free(promise);
error_malloc_promise:
	set_errc(errc);
	return NULL;
}

static void
aio_promise_free(aio_promise_t *promise)
{
	if (promise) {
		aio_future_fini(&promise->future);
		free(promise);
	}
}

static int
aio_loop_is_ready(const aio_loop_t *loop, aio_future_t *const *futures,
		size_t nfutures)
{
	assert(loop);
	assert(futures || !nfutures);

	const aio_future_t *future = &loop->future;
	do {
		if (future && future->state != AIO_FUTURE_WAITING)
			return 1;
		if (futures)
			future = *(futures++);
	} while (nfutures--);
	return 0;
}

static aio_future_t *
aio_loop_create_proxy(
		aio_loop_t *loop, aio_future_t *const *futures, size_t nfutures)
{
	assert(loop);
	assert(futures || !nfutures);
	assert(!aio_future_nodes);

	aio_future_t *proxy = &loop->future;
	if (!nfutures)
		return proxy;

	if (nfutures <= LELY_AIO_LOOP_MAX_FUTURES) {
		aio_future_nodes = aio_future_nodes_;
	} else {
		aio_future_nodes = malloc(nfutures * sizeof(*aio_future_nodes));
		if (!aio_future_nodes) {
			set_errc(errno2c(errno));
			return NULL;
		}
	}

	// Find the future with the smallest number of waiting threads.
	size_t min = SIZE_MAX;
	for (size_t i = 0; i < nfutures; i++) {
		if (!futures[i])
			continue;
		size_t n = !!futures[i]->polling;
#if !LELY_NO_THREADS
		n += futures[i]->waiting;
#endif
		if (n < min) {
			proxy = futures[i];
			// A future with no waiting threads is always the best
			// choice.
			if (!(min = n))
				break;
		}
	}

	struct aio_future_node *node = aio_future_nodes;
	aio_future_t *future = &loop->future;
	for (size_t i = 0; i <= nfutures; future = futures[i++]) {
		if (!future || future == proxy)
			continue;
		node->proxy = proxy;
		dllist_push_back(&future->list, &node->node);
		node++;
	}

	return proxy;
}

static void
aio_loop_destroy_proxy(aio_loop_t *loop, aio_future_t *const *futures,
		size_t nfutures, aio_future_t *proxy)
{
	assert(loop);
	assert(futures || !nfutures);
	assert(proxy);
	assert(proxy != &loop->future || !nfutures);
	assert(aio_future_nodes || !nfutures);

	if (!nfutures)
		return;

	struct aio_future_node *node = aio_future_nodes;
	aio_future_t *future = &loop->future;
	for (size_t i = 0; i <= nfutures; future = futures[i++]) {
		if (!future || future == proxy)
			continue;
		dllist_remove(&future->list, &node->node);
		node++;
	}

	if (aio_future_nodes != aio_future_nodes_)
		free(aio_future_nodes);
	aio_future_nodes = NULL;
}

static void
aio_loop_do_stop(aio_loop_t *loop)
{
	assert(loop);
	aio_future_t *future = &loop->future;

	if (future->state == AIO_FUTURE_WAITING) {
		future->state = AIO_FUTURE_CANCELED;
		aio_future_broadcast(future, aio_loop_get_poll(loop));
	}
}

static void
aio_loop_do_post(aio_loop_t *loop, struct aio_task *task)
{
	assert(loop);
	assert(task);

	if (!task->func && task != &loop->task)
		return;

	aio_queue_push(&loop->queue, task);

	aio_future_signal(&loop->future, aio_loop_get_poll(loop));
}

static void
aio_loop_do_on_task_started(aio_loop_t *loop)
{
	assert(loop);

	loop->ntasks++;
}

static void
aio_loop_do_on_task_finished(aio_loop_t *loop)
{
	assert(loop);
	assert(loop->ntasks);

	if (!--loop->ntasks)
		aio_loop_do_stop(loop);
}
