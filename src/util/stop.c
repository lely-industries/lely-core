/**@file
 * This file is part of the utilities library; it contains the implementation of
 * the stop token.
 *
 * @see lely/util/stop.h
 *
 * @copyright 2020 Lely Industries N.V.
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

#include "util.h"

#if !LELY_NO_MALLOC

#if !LELY_NO_THREADS
#include <lely/libc/stdatomic.h>
#include <lely/libc/threads.h>
#endif
#include <lely/util/errnum.h>
#include <lely/util/stop.h>
#include <lely/util/util.h>

#include <assert.h>
#include <stdlib.h>

/// A stop token.
struct stop_token {
	/**
	 * The number of references to this token or its associated source. Once
	 * the reference count reaches zero, the #stop_source struct containing
	 * this token is reclaimed.
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
#if !LELY_NO_THREADS
	//// The mutex protecting #queue and #func.
	mtx_t mtx;
	/// The condition variable signaled once #func completes.
	cnd_t cond;
#endif
	/// The queue of callback functions registered with this token.
	struct sllist queue;
	/// A pointer to the currently running callback.
	struct stop_func *func;
};

static stop_token_t *stop_token_init(stop_token_t *token);
static void stop_token_fini(stop_token_t *token);

/// A stop source.
struct stop_source {
	/**
	 * Twice the number of references to this source, plus a single (least
	 * significant) bit indicating whether a stop request has been issued.
	 */
#if LELY_NO_THREADS || (LELY_NO_ATOMICS && (!_WIN32 || defined(__MINGW32__)))
	size_t value;
#elif _WIN64 && !defined(__MINGW32__)
	volatile LONGLONG value;
#elif _WIN32 && !defined(__MINGW32__)
	volatile LONG value;
#else
	atomic_size_t value;
#endif
#if !LELY_NO_THREADS
	/// The identifier of the thread invoking stop_source_request_stop().
	thrd_t thr;
#endif
	/// The stop token associated with this source.
	struct stop_token token;
};

static inline stop_source_t *stop_source_from_token(const stop_token_t *token);

static void *stop_source_alloc(void);
static void stop_source_free(void *ptr);

static stop_source_t *stop_source_init(stop_source_t *source);
static void stop_source_fini(stop_source_t *source);

static void stop_source_destroy(stop_source_t *source);

stop_token_t *
stop_token_acquire(stop_token_t *token)
{
	if (token)
#if LELY_NO_THREADS || (LELY_NO_ATOMICS && (!_WIN32 || defined(__MINGW32__)))
		token->refcnt++;
#elif _WIN64 && !defined(__MINGW32__)
		InterlockedIncrementNoFence64(&token->refcnt);
#elif _WIN32 && !defined(__MINGW32__)
		InterlockedIncrementNoFence(&token->refcnt);
#else
		atomic_fetch_add_explicit(
				&token->refcnt, 1, memory_order_relaxed);
#endif
	return token;
}

void
stop_token_release(stop_token_t *token)
{
	if (!token)
		return;
#if LELY_NO_THREADS || (LELY_NO_ATOMICS && (!_WIN32 || defined(__MINGW32__)))
	if (!--token->refcnt) {
#elif _WIN64 && !defined(__MINGW32__)
	if (!InterlockedDecrementRelease64(&token->refcnt)) {
		MemoryBarrier();
#elif _WIN32 && !defined(__MINGW32__)
	if (!InterlockedDecrementRelease(&token->refcnt)) {
		MemoryBarrier();
#else
	if (atomic_fetch_sub_explicit(&token->refcnt, 1, memory_order_release)
			== 1) {
		atomic_thread_fence(memory_order_acquire);
#endif
		stop_source_destroy(stop_source_from_token(token));
	}
}

int
stop_token_stop_requested(const stop_token_t *token)
{
	return stop_source_stop_requested(stop_source_from_token(token));
}

int
stop_token_stop_possible(const stop_token_t *token)
{
	const stop_source_t *source = stop_source_from_token(token);

#if LELY_NO_THREADS || LELY_NO_ATOMICS
	size_t value = source->value;
#else
	size_t value = atomic_load_explicit(
			(atomic_size_t *)&source->value, memory_order_relaxed);
#endif
	return value != 0;
}

int
stop_token_insert(stop_token_t *token, struct stop_func *func)
{
	stop_source_t *source = stop_source_from_token(token);
	assert(func);

#if LELY_NO_THREADS || LELY_NO_ATOMICS
	size_t value = source->value;
#else
	size_t value = atomic_load_explicit(
			(atomic_size_t *)&source->value, memory_order_relaxed);
#endif
	// If no stop request has been issued, add the callback to the queue.
	if (!(value & 1)) {
#if !LELY_NO_THREADS
		mtx_lock(&token->mtx);
		// Check again, in case a stop request was issued before we
		// acquired the lock.
		if (!stop_token_stop_requested(token)) {
#endif
			sllist_push_front(&token->queue, &func->_node);
#if !LELY_NO_THREADS
			mtx_unlock(&token->mtx);
#endif
			return 0;
#if !LELY_NO_THREADS
		}
		mtx_unlock(&token->mtx);
#endif
	}

	// A stop request was issued, so invoke the callback immediately.
	func->func(func);
	return 1;
}

void
stop_token_remove(stop_token_t *token, struct stop_func *func)
{
	assert(token);
	assert(func);

#if !LELY_NO_THREADS
	mtx_lock(&token->mtx);
#endif
	if (func == token->func) {
#if !LELY_NO_THREADS
		stop_source_t *source = stop_source_from_token(token);
		// Only wait for the callback to complete if it is running on
		// another thread.
		if (!thrd_equal(thrd_current(), source->thr)) {
			do
				cnd_wait(&token->cond, &token->mtx);
			while (func == token->func);
		}
#endif
	} else {
		sllist_remove(&token->queue, &func->_node);
	}
#if !LELY_NO_THREADS
	mtx_unlock(&token->mtx);
#endif
}

stop_source_t *
stop_source_create(void)
{
	int errc = 0;

	stop_source_t *source = stop_source_alloc();
	if (!source) {
		errc = get_errc();
		goto error_alloc;
	}

	stop_source_t *tmp = stop_source_init(source);
	if (!tmp) {
		errc = get_errc();
		goto error_init;
	}
	source = tmp;

	return source;

error_init:
	stop_source_free(source);
error_alloc:
	set_errc(errc);
	return NULL;
}

stop_source_t *
stop_source_acquire(stop_source_t *source)
{
	if (source) {
		stop_token_acquire(&source->token);
#if LELY_NO_THREADS || (LELY_NO_ATOMICS && (!_WIN32 || defined(__MINGW32__)))
		source->value += 2;
#elif _WIN64 && !defined(__MINGW32__)
		InterlockedAddNoFence64(&source->value, 2);
#elif _WIN32 && !defined(__MINGW32__)
		InterlockedaddNoFence(&source->value, 2);
#else
		atomic_fetch_add_explicit(
				&source->value, 2, memory_order_relaxed);
#endif
	}
	return source;
}

void
stop_source_release(stop_source_t *source)
{
	if (source) {
#if LELY_NO_THREADS
		source->value -= 2;
#elif _WIN64 && !defined(__MINGW32__)
		InterlockedAddRelease64(&source->value, -2);
#elif _WIN32 && !defined(__MINGW32__)
		InterlockedAddRelease(&source->value, -2);
#else
		atomic_fetch_sub_explicit(
				&source->value, 2, memory_order_release);
#endif
		stop_token_release(&source->token);
	}
}

int
stop_source_request_stop(stop_source_t *source)
{
	assert(source);
	stop_token_t *token = &source->token;

	// Perform a quick check to see if a stop request has already been
	// issued.
#if LELY_NO_THREADS || LELY_NO_ATOMICS
	if (source->value & 1)
#else
	if (atomic_load_explicit(&source->value, memory_order_relaxed) & 1)
#endif
		return 0;

#if !LELY_NO_THREADS
	// Register the stop request while holding the lock.
	mtx_lock(&token->mtx);
#endif
#if LELY_NO_THREADS || (LELY_NO_ATOMICS && (!_WIN32 || defined(__MINGW32__)))
	source->value |= 1;
#else
#if _WIN64 && !defined(__MINGW32__)
	LONGLONG value = InterlockedOrNoFence64(&source->value, 1);
#elif _WIN32 && !defined(__MINGW32__)
	LONG value = InterlockedOrNoFence(&source->value, 1);
#else
	size_t value = atomic_fetch_or_explicit(
			&source->value, 1, memory_order_release);
#endif
	// Check again, in case a stop request was issued before we acquired the
	// lock.
	if (value & 1) {
#if !LELY_NO_THREADS
		mtx_unlock(&token->mtx);
#endif
		return 0;
	}
#endif

#if !LELY_NO_THREADS
	// Record the identifier of the calling thread, to prevent a deadlock in
	// stop_token_remove().
	source->thr = thrd_current();
#endif

	// Invoke all registered callbacks.
	struct slnode *node;
	while ((node = sllist_pop_front(&token->queue))) {
		struct stop_func *func =
				structof(node, struct stop_func, _node);
		// Store a pointer to the currently running callback.
		token->func = func;
#if !LELY_NO_THREADS
		mtx_unlock(&token->mtx);
#endif
		assert(func->func);
		func->func(func);
#if !LELY_NO_THREADS
		mtx_lock(&token->mtx);
#endif
		// Signal stop_token_remove() that the callback has been
		// completed.
		token->func = NULL;
#if !LELY_NO_THREADS
		cnd_signal(&token->cond);
#endif
	}
#if !LELY_NO_THREADS
	mtx_unlock(&token->mtx);
#endif

	return 1;
}

int
stop_source_stop_requested(const stop_source_t *source)
{
	assert(source);

#if LELY_NO_THREADS || LELY_NO_ATOMICS
	return source->value & 1;
#else
	// clang-format off
	return atomic_load_explicit((atomic_size_t *)&source->value,
			memory_order_acquire) & 1;
	// clang-format on
#endif
}

stop_token_t *
stop_source_get_token(stop_source_t *source)
{
	return stop_token_acquire(source ? &source->token : NULL);
}

static stop_token_t *
stop_token_init(stop_token_t *token)
{
	assert(token);

#if !LELY_NO_THREADS
	int errc = 0;
#endif

#if LELY_NO_THREADS || LELY_NO_ATOMICS || (_WIN32 && !defined(__MINGW32__))
	token->refcnt = 1;
#else
	atomic_init(&token->refcnt, 1);
#endif

#if !LELY_NO_THREADS
	if (mtx_init(&token->mtx, mtx_plain) != thrd_success) {
		errc = get_errc();
		goto error_init_mtx;
	}

	if (cnd_init(&token->cond) != thrd_success) {
		errc = get_errc();
		goto error_init_cond;
	}
#endif

	sllist_init(&token->queue);
	token->func = NULL;

	return token;

#if !LELY_NO_THREADS
	// cnd_destroy(&token->cond);
error_init_cond:
	mtx_destroy(&token->mtx);
error_init_mtx:
	set_errc(errc);
	return NULL;
#endif
}

static void
stop_token_fini(stop_token_t *token)
{
#if LELY_NO_THREADS
	(void)token;
#else
	assert(token);

	cnd_destroy(&token->cond);
	mtx_destroy(&token->mtx);
#endif
}

static inline stop_source_t *
stop_source_from_token(const stop_token_t *token)
{
	assert(token);

	return structof(token, stop_source_t, token);
}

static void *
stop_source_alloc(void)
{
	void *ptr = malloc(sizeof(stop_source_t));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

static void
stop_source_free(void *ptr)
{
	free(ptr);
}

static stop_source_t *
stop_source_init(stop_source_t *source)
{
	assert(source);

#if LELY_NO_THREADS || LELY_NO_ATOMICS || (_WIN32 && !defined(__MINGW32__))
	source->value = 2;
#else
	atomic_init(&source->value, 2);
#endif

	if (!stop_token_init(&source->token))
		return NULL;

	return source;
}

static void
stop_source_fini(stop_source_t *source)
{
	assert(source);

	stop_token_fini(&source->token);
}

static void
stop_source_destroy(stop_source_t *source)
{
	if (source) {
		stop_source_fini(source);
		stop_source_free(source);
	}
}

#endif // !LELY_NO_MALLOC
