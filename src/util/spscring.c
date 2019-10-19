/**@file
 * This file is part of the utilities library; it contains the implementation of
 * the single-producer single-consumer ring buffer.
 *
 * @see lely/util/spscring.h
 *
 * @copyright 2019 Lely Industries N.V.
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
#include <lely/util/spscring.h>

#include <assert.h>
#include <stdint.h>

static size_t spscring_p_alloc_slow(struct spscring *ring, size_t *psize);
static size_t spscring_c_alloc_slow(struct spscring *ring, size_t *psize);

static void spscring_pc_signal(struct spscring *ring);
static void spscring_cp_signal(struct spscring *ring);

static inline size_t spscring_atomic_load(const volatile void *object);
static inline void spscring_atomic_store(volatile void *object, size_t desired);
static inline _Bool spscring_atomic_compare_exchange_strong(
		volatile void *object, size_t *expected, size_t desired);
static inline _Bool spscring_atomic_compare_exchange_weak(
		volatile void *object, size_t *expected, size_t desired);

static inline void spscring_yield(void);

void
spscring_init(struct spscring *ring, size_t size)
{
	assert(ring);
	assert(size <= SIZE_MAX / 2 + 1);

	*ring = (struct spscring)SPSCRING_INIT(size);
}

size_t
spscring_size(const struct spscring *ring)
{
	assert(ring);
	assert(ring->p.ctx.size == ring->c.ctx.size);

	return ring->p.ctx.size;
}

size_t
spscring_p_capacity(const struct spscring *ring)
{
	assert(ring);
	const struct spscring_ctx *ctx = &ring->p.ctx;
	assert(ctx->size <= SIZE_MAX / 2 + 1);
	assert(ctx->end <= ctx->size);
	assert(ctx->pos <= ctx->end);

	size_t cpos = spscring_atomic_load(&ring->c.pos);
	cpos -= ctx->base - ctx->size;
	assert(ctx->pos <= cpos);
	assert(cpos - ctx->pos <= ctx->size);
	return cpos - ctx->pos;
}

size_t
spscring_p_alloc(struct spscring *ring, size_t *psize)
{
	assert(ring);
	struct spscring_ctx *ctx = &ring->p.ctx;
	assert(ctx->end <= ctx->size);
	assert(ctx->pos <= ctx->end);
	assert(psize);

	size_t capacity = ctx->end - ctx->pos;
	if (*psize > capacity) {
		if (ctx->end == ctx->size && capacity > 0)
			*psize = capacity;
		else
			return spscring_p_alloc_slow(ring, psize);
	}
	return ctx->pos;
}

size_t
spscring_p_commit(struct spscring *ring, size_t size)
{
	assert(ring);
	struct spscring_ctx *ctx = &ring->p.ctx;
	assert(ctx->end <= ctx->size);
	assert(ctx->pos <= ctx->end);
	assert(size <= ctx->pos - ctx->end);

	size_t ppos = ctx->pos += size;
	spscring_atomic_store(&ring->p.pos, ctx->base + ppos);

	spscring_pc_signal(ring);

	return ppos;
}

int
spscring_p_submit_wait(struct spscring *ring, size_t size,
		void (*func)(struct spscring *ring, void *arg), void *arg)
{
	assert(ring);
	struct spscring_ctx *ctx = &ring->p.ctx;
	assert(ctx->size <= SIZE_MAX / 2 + 1);
	struct spscring_sig *sig = &ring->p.sig;

	if (size > ctx->size)
		size = ctx->size;

	// Abort the previous wait, if any.
	spscring_p_abort_wait(ring);

	sig->func = func;
	sig->arg = arg;

	// Check if the wait condition is already satisfied.
	if (size <= spscring_p_capacity(ring))
		return 0;

	// Register the wait with the consumer.
	spscring_atomic_store(&sig->size, size);

	// Check if a concurrent consumer has already satisfied the wait
	// condition.
	if (size <= spscring_p_capacity(ring)) {
		// Try to stop the signal function from being invoked.
		// clang-format off
		if (spscring_atomic_compare_exchange_strong(
				&sig->size, &size, 0))
			// clang-format on
			return 0;
		// The consumer has already begun invoking the signal function.
		assert(size == 0 || size == SIZE_MAX);
	}

	return 1;
}

int
spscring_p_abort_wait(struct spscring *ring)
{
	assert(ring);
	struct spscring_sig *sig = &ring->p.sig;

	size_t size = spscring_atomic_load(&sig->size);
	while (size) {
		// Abort the wait, unless spscring_cp_signal() just started
		// reading the function pointer and argument.
		if (size == SIZE_MAX) {
			spscring_yield();
			// Busy-wait until spscring_cp_signal() is done reading
			// the function pointer and argument.
			size = spscring_atomic_load(&sig->size);
			// clang-format off
		} else if (spscring_atomic_compare_exchange_weak(
				&sig->size, &size, 0)) {
			// clang-format on
			return 1;
		}
	}
	return 0;
}

size_t
spscring_c_capacity(const struct spscring *ring)
{
	assert(ring);
	const struct spscring_ctx *ctx = &ring->c.ctx;
	assert(ctx->size <= SIZE_MAX / 2 + 1);
	assert(ctx->end <= ctx->size);
	assert(ctx->pos <= ctx->end);

	size_t ppos = spscring_atomic_load(&ring->p.pos);
	ppos -= ctx->base;
	assert(ctx->pos <= ppos);
	assert(ppos - ctx->pos <= ctx->size);
	return ppos - ctx->pos;
}

size_t
spscring_c_alloc(struct spscring *ring, size_t *psize)
{
	assert(ring);
	struct spscring_ctx *ctx = &ring->c.ctx;
	assert(ctx->end <= ctx->size);
	assert(ctx->pos <= ctx->end);
	assert(psize);

	size_t capacity = ctx->end - ctx->pos;
	if (*psize > capacity) {
		if (ctx->end == ctx->size && capacity > 0)
			*psize = capacity;
		else
			return spscring_c_alloc_slow(ring, psize);
	}
	return ctx->pos;
}

size_t
spscring_c_commit(struct spscring *ring, size_t size)
{
	assert(ring);
	struct spscring_ctx *ctx = &ring->c.ctx;
	assert(ctx->end <= ctx->size);
	assert(ctx->pos <= ctx->end);
	assert(size <= ctx->pos - ctx->end);

	size_t cpos = ctx->pos += size;
	spscring_atomic_store(&ring->c.pos, ctx->base + cpos);

	spscring_cp_signal(ring);

	return cpos;
}

int
spscring_c_submit_wait(struct spscring *ring, size_t size,
		void (*func)(struct spscring *ring, void *arg), void *arg)
{
	assert(ring);
	struct spscring_ctx *ctx = &ring->c.ctx;
	assert(ctx->size <= SIZE_MAX / 2 + 1);
	struct spscring_sig *sig = &ring->c.sig;

	if (size > ctx->size)
		size = ctx->size;

	// Abort the previous wait, if any.
	spscring_c_abort_wait(ring);

	sig->func = func;
	sig->arg = arg;

	// Check if the wait condition is already satisfied.
	if (size <= spscring_c_capacity(ring))
		return 0;

	// Register the wait with the producer.
	spscring_atomic_store(&sig->size, size);

	// Check if a concurrent producer has already satisfied the wait
	// condition.
	if (size <= spscring_c_capacity(ring)) {
		// Try to stop the signal function from being invoked.
		// clang-format off
		if (spscring_atomic_compare_exchange_strong(
				&sig->size, &size, 0))
			// clang-format on
			return 0;
		// The producer has already begun invoking the signal function.
		assert(size == 0 || size == SIZE_MAX);
	}

	return 1;
}

int
spscring_c_abort_wait(struct spscring *ring)
{
	assert(ring);
	struct spscring_sig *sig = &ring->c.sig;

	size_t size = spscring_atomic_load(&sig->size);
	while (size) {
		// Abort the wait, unless spscring_pc_signal() just started
		// reading the function pointer and argument.
		if (size == SIZE_MAX) {
			spscring_yield();
			// Busy-wait until spscring_pc_signal() is done reading
			// the function pointer and argument.
			size = spscring_atomic_load(&sig->size);
			// clang-format off
		} else if (spscring_atomic_compare_exchange_weak(
					&sig->size, &size, 0)) {
			// clang-format on
			return 1;
		}
	}
	return 0;
}

static size_t
spscring_p_alloc_slow(struct spscring *ring, size_t *psize)
{
	assert(ring);
	struct spscring_ctx *ctx = &ring->p.ctx;
	assert(ctx->size <= SIZE_MAX / 2 + 1);
	assert(ctx->end <= ctx->size);
	assert(ctx->pos <= ctx->end);
	assert(psize);

	if (ctx->pos == ctx->size) {
		ctx->base += ctx->size;
		ctx->pos = 0;
	}

	size_t cpos = spscring_atomic_load(&ring->c.pos);
	cpos -= ctx->base - ctx->size;
	assert(ctx->pos <= cpos);

	ctx->end = cpos < ctx->size ? cpos : ctx->size;

	size_t capacity = ctx->end - ctx->pos;
	if (*psize > capacity)
		*psize = capacity;

	return ctx->pos;
}

static size_t
spscring_c_alloc_slow(struct spscring *ring, size_t *psize)
{
	assert(ring);
	struct spscring_ctx *ctx = &ring->c.ctx;
	assert(ctx->size <= SIZE_MAX / 2 + 1);
	assert(ctx->end <= ctx->size);
	assert(ctx->pos <= ctx->end);
	assert(psize);

	if (ctx->pos == ctx->size) {
		ctx->base += ctx->size;
		ctx->pos = 0;
	}

	size_t ppos = spscring_atomic_load(&ring->p.pos);
	ppos -= ctx->base;
	assert(ctx->pos <= ppos);

	ctx->end = ppos < ctx->size ? ppos : ctx->size;

	size_t capacity = ctx->end - ctx->pos;
	if (*psize > capacity)
		*psize = capacity;

	return ctx->pos;
}

static void
spscring_pc_signal(struct spscring *ring)
{
	assert(ring);
	struct spscring_ctx *ctx = &ring->p.ctx;
	struct spscring_sig *sig = &ring->c.sig;

	// Check if the consumer has registered a wait.
	size_t size = spscring_atomic_load(&sig->size);
	while (size) {
		// Abort if the wait condition is not satisfied.
		if (size > ctx->size - spscring_p_capacity(ring))
			return;

		// Try to set the requested size to the special value SIZE_MAX
		// to notify spscring_c_abort_wait() that we're about to read
		// the function pointer and argument.
		// clang-format off
		if (spscring_atomic_compare_exchange_weak(
				&sig->size, &size, SIZE_MAX)) {
			// clang-format on
			// Copy the function pointer and argument to allow the
			// consumer to register a different signal function for
			// the next wait.
			void (*func)(struct spscring * ring, void *arg) =
					sig->func;
			void *arg = sig->arg;

			// Set the requested size to 0 to indicate that the wait
			// condition was satisfied.
			spscring_atomic_store(&sig->size, 0);

			// Invoke the signal function, if specified.
			if (func)
				func(ring, arg);
			return;
		}

		spscring_yield();
	}
}

static void
spscring_cp_signal(struct spscring *ring)
{
	assert(ring);
	struct spscring_ctx *ctx = &ring->c.ctx;
	struct spscring_sig *sig = &ring->p.sig;

	// Check if the producer has registered a wait.
	size_t size = spscring_atomic_load(&sig->size);
	while (size) {
		// Abort if the wait condition is not satisfied.
		if (size > ctx->size - spscring_c_capacity(ring))
			return;

		// Try to set the requested size to the special value SIZE_MAX
		// to notify spscring_p_abort_wait() that we're about to read
		// the function pointer and argument.
		// clang-format off
		if (spscring_atomic_compare_exchange_weak(
				&sig->size, &size, SIZE_MAX)) {
			// clang-format on
			// Copy the function pointer and argument to allow the
			// producer to register a different signal function for
			// the next wait.
			void (*func)(struct spscring * ring, void *arg) =
					sig->func;
			void *arg = sig->arg;

			// Set the requested size to 0 to indicate that the wait
			// condition was satisfied.
			spscring_atomic_store(&sig->size, 0);

			// Invoke the signal function, if specified.
			if (func)
				func(ring, arg);

			return;
		}

		spscring_yield();
	}
}

static inline size_t
spscring_atomic_load(const volatile void *object)
{
#if __STDC_NO_ATOMICS__
	return *(const volatile size_t *)object;
#else
	return atomic_load_explicit(
			(atomic_size_t *)object, memory_order_acquire);
#endif
}

static inline void
spscring_atomic_store(volatile void *object, size_t desired)
{
#if __STDC_NO_ATOMICS__
	*(volatile size_t *)object = desired;
#else
	atomic_store_explicit(
			(atomic_size_t *)object, desired, memory_order_release);
#endif
}

static inline _Bool
spscring_atomic_compare_exchange_strong(
		volatile void *object, size_t *expected, size_t desired)
{
#if __STDC_NO_ATOMICS__
#if _WIN32
#if _WIN64
	size_t tmp = InterlockedCompareExchange64(
			(volatile LONGLONG *)object, desired, *expected);
#else
	size_t tmp = InterlockedCompareExchange(
			(volatile LONG *)object, desired, *expected);
#endif
	_Bool result = tmp == *expected;
	if (!result)
		*expected = tmp;
	return result;
#else
	assert(*(volatile size_t *)object == *expected);
	(void)expected;
	*(volatile size_t *)object = desired;
	return 1;
#endif
#else
	return atomic_compare_exchange_strong_explicit((atomic_size_t *)object,
			expected, desired, memory_order_release,
			memory_order_relaxed);
#endif
}

static inline _Bool
spscring_atomic_compare_exchange_weak(
		volatile void *object, size_t *expected, size_t desired)
{
#if __STDC_NO_ATOMICS__
	return spscring_atomic_compare_exchange_strong(
			object, expected, desired);
#else
	return atomic_compare_exchange_weak_explicit((atomic_size_t *)object,
			expected, desired, memory_order_release,
			memory_order_relaxed);
#endif
}

static inline void
spscring_yield(void)
{
#if _WIN32
	YieldProcessor();
#elif (defined(__i386__) || defined(__x86_64__)) \
		&& (GNUC_PREREQ(4, 7) || __has_builtin(__builtin_ia32_pause))
	__builtin_ia32_pause();
#endif
}
