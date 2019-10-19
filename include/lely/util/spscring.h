/**@file
 * This header file is part of the utilities library; it contains the
 * single-producer, single-consumer ring buffer declarations.
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

#ifndef LELY_UTIL_SPSCRING_H_
#define LELY_UTIL_SPSCRING_H_

#include <lely/features.h>

#ifdef __cplusplus
#include <atomic>
using ::std::atomic_size_t;
#else
#include <lely/libc/stdatomic.h>
#endif

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// A single-producer, single-consumer ring buffer.
struct spscring {
	struct {
		_Alignas(LEVEL1_DCACHE_LINESIZE) struct spscring_ctx {
			size_t size;
			size_t base;
			size_t pos;
			size_t end;
		} ctx;
		char _pad1[LEVEL1_DCACHE_LINESIZE
				- sizeof(struct spscring_ctx)];
#if !defined(__cplusplus) && __STDC_NO_ATOMICS__
		_Alignas(LEVEL1_DCACHE_LINESIZE) volatile size_t pos;
		char _pad2[LEVEL1_DCACHE_LINESIZE - sizeof(size_t)];
#else
		_Alignas(LEVEL1_DCACHE_LINESIZE) atomic_size_t pos;
		char _pad2[LEVEL1_DCACHE_LINESIZE - sizeof(atomic_size_t)];
#endif
		_Alignas(LEVEL1_DCACHE_LINESIZE) struct spscring_sig {
#if !defined(__cplusplus) && __STDC_NO_ATOMICS__
			volatile size_t size;
#else
			atomic_size_t size;
#endif
			void (*func)(struct spscring *ring, void *arg);
			void *arg;
		} sig;
		char _pad3[LEVEL1_DCACHE_LINESIZE
				- sizeof(struct spscring_sig)];
	} p, c;
};

/// The static initializer for #spscring.
#if !defined(__cplusplus) && __STDC_NO_ATOMICS__
#define SPSCRING_INIT(size) \
	{ \
		{ { (size), 0, 0, 0 }, { 0 }, 0, { 0 }, { 0, NULL, NULL }, \
			{ 0 } }, \
		{ \
			{ (size), 0, 0, 0 }, { 0 }, 0, { 0 }, \
					{ 0, NULL, NULL }, { 0 }, \
		} \
	}
#else
#define SPSCRING_INIT(size) \
	{ \
		{ \
			{ (size), 0, 0, 0 }, \
			{ 0 }, \
			ATOMIC_VAR_INIT(0), \
			{ 0 }, \
			{ ATOMIC_VAR_INIT(0), NULL, NULL }, \
			{ 0 }, \
		}, \
		{ \
			{ (size), 0, 0, 0 }, { 0 }, ATOMIC_VAR_INIT(0), { 0 }, \
					{ ATOMIC_VAR_INIT(0), NULL, NULL }, \
					{ 0 }, \
		} \
	}
#endif

void spscring_init(struct spscring *ring, size_t size);

size_t spscring_size(const struct spscring *ring);

/**
 * Returns the total capacity available for a producer in a single-producer
 * single-consumer ring buffer. Note that, because of wrapping, the reported
 * capacity MAY be larger than the consecutive capacity reported by
 * spscring_p_alloc().
 *
 * This function is wait-free. It MUST NOT be invoked by a consumer or by more
 * than one concurrent producer.
 *
 * @returns the total number of indices available for writing.
 */
size_t spscring_p_capacity(const struct spscring *ring);

/**
 *
 * This function is wait-free. It MUST NOT be invoked by a consumer or by more
 * than one concurrent producer.
 *
 * @returns the first index available for writing.
 */
size_t spscring_p_alloc(struct spscring *ring, size_t *psize);

/**
 *
 * This function is wait-free, unless the consumer has registered a wait, in
 * which case it is lock-free if, and only if, the consumer signal function is
 * lock-free. This function MUST NOT be invoked by a consumer or by more than
 * one concurrent producer.
 *
 */
size_t spscring_p_commit(struct spscring *ring, size_t size);

int spscring_p_submit_wait(struct spscring *ring, size_t size,
		void (*func)(struct spscring *ring, void *arg), void *arg);

/**
 *
 * This function is lock-free. It MAY busy-wait while spscring_c_commit() is
 * reading the function pointer and argument of an ongoing wait. This function
 * MUST NOT be invoked by a consumer or by more than one concurrent producer.
 *
 */
int spscring_p_abort_wait(struct spscring *ring);

/**
 * Returns the total capacity available for a consumer in a single-producer
 * single-consumer ring buffer. Note that, because of wrapping, the reported
 * capacity MAY be larger than the consecutive capacity reported by
 * spscring_c_alloc().
 *
 * This function is wait-free. It MUST NOT be invoked by a producer or by more
 * than one concurrent consumer.
 *
 * @returns the total number of indices available for writing.
 */
size_t spscring_c_capacity(const struct spscring *ring);

size_t spscring_c_alloc(struct spscring *ring, size_t *psize);

size_t spscring_c_commit(struct spscring *ring, size_t size);

int spscring_c_submit_wait(struct spscring *ring, size_t size,
		void (*func)(struct spscring *ring, void *arg), void *arg);

int spscring_c_abort_wait(struct spscring *ring);

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_SPSCRING_H_
