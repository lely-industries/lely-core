/**@file
 * This header file is part of the utilities library; it contains the
 * single-producer, single-consumer ring buffer declarations.
 *
 * To make the ring buffer generic, a low-level interface is provided which only
 * operates on buffer indices. The user is responsible for reading or writing
 * values to an actual memory buffer (or file).
 *
 * The implementation allows both the producer and consumer to register a signal
 * function which is invoked once a requested number of indices becomes
 * available for reading or writing. This enables the user to implement blocking
 * read or write operations.
 *
 * All ring buffer operations are lock-free, provided the user-defined signal
 * functions are lock-free. If no signal functions are registered, the
 * operations are also wait-free.
 *
 * @copyright 2019-2020 Lely Industries N.V.
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

#if !LELY_NO_ATOMICS
#ifdef __cplusplus
#include <atomic>
#else
#include <lely/libc/stdatomic.h>
#endif
#endif // !LELY_NO_ATOMICS

#include <stddef.h>

#if LELY_NO_ATOMICS
typedef size_t spscring_atomic_t;
#elif defined(__cplusplus)
using spscring_atomic_t = ::std::atomic_size_t;
#else // C11
typedef atomic_size_t spscring_atomic_t;
#endif

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
		_Alignas(LEVEL1_DCACHE_LINESIZE) spscring_atomic_t pos;
		char _pad2[LEVEL1_DCACHE_LINESIZE - sizeof(spscring_atomic_t)];
		_Alignas(LEVEL1_DCACHE_LINESIZE) struct spscring_sig {
			spscring_atomic_t size;
			void (*func)(struct spscring *ring, void *arg);
			void *arg;
		} sig;
		char _pad3[LEVEL1_DCACHE_LINESIZE
				- sizeof(struct spscring_sig)];
	} p, c;
};

/// The static initializer for #spscring.
#if LELY_NO_ATOMICS
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

/**
 * Initializes a single-producer, single-consumer ring buffer with the specified
 * size.
 */
void spscring_init(struct spscring *ring, size_t size);

/// Returns the size of a single-producer, single-consumer ring buffer.
size_t spscring_size(const struct spscring *ring);

/**
 * Returns the total capacity available for a producer in a single-producer
 * single-consumer ring buffer, including wrapping. A subsequent call to
 * spscring_p_alloc() with a size smaller than or equal to the reported capacity
 * is guaranteed to succeed.
 *
 * This function is wait-free. It MUST NOT be invoked by a consumer or by more
 * than one concurrent producer.
 *
 * @returns the total number of indices available for writing.
 */
size_t spscring_p_capacity(struct spscring *ring);

/**
 * Returns the total capacity available for a producer in a single-producer
 * single-consumer ring buffer, without wrapping. A subsequent call to
 * spscring_p_alloc_no_wrap() with a size smaller than or equal to the reported
 * capacity is guaranteed to succeed.
 *
 * This function is wait-free. It MUST NOT be invoked by a consumer or by more
 * than one concurrent producer.
 *
 * @returns the total number of indices available for writing.
 */
size_t spscring_p_capacity_no_wrap(struct spscring *ring);

/**
 * Allocates a consecutive range of indices, including wrapping, in a
 * single-producer, single-consumer ring buffer for writing. The indices are not
 * made available to the consumer until a call to spscring_p_commit(). Without
 * an intervening call to spscring_p_commit(), this function is idempotent.
 *
 * This function is wait-free. It MUST NOT be invoked by a consumer or by more
 * than one concurrent producer.
 *
 * @param ring  a pointer to a ring buffer.
 * @param psize a pointer to a value which, on input, contains the requested
 *              number of indices. If fewer indices are available, *<b>psize</b>
 *              is updated on output to contain the actual number of indices
 *              available for writing.
 *
 * @returns the first index available for writing.
 */
size_t spscring_p_alloc(struct spscring *ring, size_t *psize);

/**
 * Allocates a consecutive range of indices, without wrapping, in a
 * single-producer, single-consumer ring buffer for writing. The indices are not
 * made available to the consumer until a call to spscring_p_commit(). Without
 * an intervening call to spscring_p_commit(), this function is idempotent.
 *
 * This function is wait-free. It MUST NOT be invoked by a consumer or by more
 * than one concurrent producer.
 *
 * @param ring  a pointer to a ring buffer.
 * @param psize a pointer to a value which, on input, contains the requested
 *              number of indices. If fewer indices are available, *<b>psize</b>
 *              is updated on output to contain the actual number of indices
 *              available for writing.
 *
 * @returns the first index available for writing.
 */
size_t spscring_p_alloc_no_wrap(struct spscring *ring, size_t *psize);

/**
 * Makes the specified number of indices available to a consumer and, if this
 * satisfies a wait operation registered by the consumer, invokes the consumer
 * signal function . <b>size</b> MUST not exceed the range obtained from a
 * preceding call to spscring_p_alloc() or spscring_p_alloc_no_wrap(). However,
 * it is possible to incrementally commit a range of indices without invoking
 * spscring_p_alloc() or spscring_p_alloc_no_wrap() again.
 *
 * This function is wait-free, unless the consumer has registered a wait, in
 * which case it is lock-free if, and only if, the consumer signal function is
 * lock-free. This function MUST NOT be invoked by a consumer or by more than
 * one concurrent producer.
 *
 * @returns the first index available for writing.
 */
size_t spscring_p_commit(struct spscring *ring, size_t size);

/**
 * Checks if the requested range of indices, including wrapping, in a
 * single-producer, single-consumer ring buffer is available for writing and, if
 * not, registers a signal function to be invoked once the requested range
 * becomes available.
 *
 * This function is lock-free. It MAY busy-wait while aborting a previous wait
 * operation with spscring_p_abort_wait(). This function MUST NOT be invoked by
 * a consumer or by more than one concurrent producer.
 *
 * @param ring a pointer to a ring buffer.
 * @param size the number of indices available for writing before the wait
 *             condition is satisfied.
 * @param func the signal function to be invoked when the wait condition is
 *             satisfied.
 * @param arg  the user-specific value to be passed as the second argument to
 *             <b>func</b>.
 *
 * @returns 1 if the wait operation was submitted, and 0 if the wait condition
 * was already satisfied and requested number of indices are available. In the
 * latter case, the signal function is _not_ invoked.
 */
int spscring_p_submit_wait(struct spscring *ring, size_t size,
		void (*func)(struct spscring *ring, void *arg), void *arg);

/**
 * Aborts a wait operation previously registered with spscring_p_submit_wait().
 *
 * This function is lock-free. It MAY busy-wait while spscring_c_commit() is
 * reading the function pointer and argument of an ongoing wait. This function
 * MUST NOT be invoked by a consumer or by more than one concurrent producer.
 *
 * @returns 1 if a wait operation was canceled, and 0 if no wait operation was
 * registered.
 */
int spscring_p_abort_wait(struct spscring *ring);

/**
 * Returns the total capacity available for a consumer in a single-producer
 * single-consumer ring buffer, including wrapping. A subsequent call to
 * spscring_c_alloc() with a size smaller than or equal to the reported capacity
 * is guaranteed to succeed.
 *
 * This function is wait-free. It MUST NOT be invoked by a producer or by more
 * than one concurrent consumer.
 *
 * @returns the total number of indices available for writing.
 */
size_t spscring_c_capacity(struct spscring *ring);

/**
 * Returns the total capacity available for a consumer in a single-producer
 * single-consumer ring buffer, without wrapping. A subsequent call to
 * spscring_c_alloc_no_wrap() with a size smaller than or equal to the reported
 * capacity is guaranteed to succeed.
 *
 * This function is wait-free. It MUST NOT be invoked by a producer or by more
 * than one concurrent consumer.
 *
 * @returns the total number of indices available for writing.
 */
size_t spscring_c_capacity_no_wrap(struct spscring *ring);

/**
 * Allocates a consecutive range of indices, including wrapping, in a
 * single-producer, single-consumer ring buffer for reading. The indices are not
 * made available to the producer until a call to spscring_c_commit(). Without
 * an intervening call to spscring_c_commit(), this function is idempotent.
 *
 * This function is wait-free. It MUST NOT be invoked by a producer or by more
 * than one concurrent consumer.
 *
 * @param ring  a pointer to a ring buffer.
 * @param psize a pointer to a value which, on input, contains the requested
 *              number of indices. If fewer indices are available, *<b>psize</b>
 *              is updated on output to contain the actual number of indices
 *              available for reading.
 *
 * @returns the first index available for reading.
 */
size_t spscring_c_alloc(struct spscring *ring, size_t *psize);

/**
 * Allocates a consecutive range of indices, without wrapping, in a
 * single-producer, single-consumer ring buffer for reading. The indices are not
 * made available to the producer until a call to spscring_c_commit(). Without
 * an intervening call to spscring_c_commit(), this function is idempotent.
 *
 * This function is wait-free. It MUST NOT be invoked by a producer or by more
 * than one concurrent consumer.
 *
 * @param ring  a pointer to a ring buffer.
 * @param psize a pointer to a value which, on input, contains the requested
 *              number of indices. If fewer indices are available, *<b>psize</b>
 *              is updated on output to contain the actual number of indices
 *              available for reading.
 *
 * @returns the first index available for reading.
 */
size_t spscring_c_alloc_no_wrap(struct spscring *ring, size_t *psize);

/**
 * Makes the specified number of indices available to a producer and, if this
 * satisfies a wait operation registered by the producer, invokes the producer
 * signal function . <b>size</b> MUST not exceed the range obtained from a
 * preceding call to spscring_c_alloc() or spscring_c_alloc_no_wrap(). However,
 * it is possible to incrementally commit a range of indices without invoking
 * spscring_c_alloc() or spscring_c_alloc_no_wrap() again.
 *
 * This function is wait-free, unless the producer has registered a wait, in
 * which case it is lock-free if, and only if, the producer signal function is
 * lock-free. This function MUST NOT be invoked by a producer or by more than
 * one concurrent consumer.
 *
 * @returns the first index available for reading.
 */
size_t spscring_c_commit(struct spscring *ring, size_t size);

/**
 * Checks if the requested range of indices, including wrapping, in a
 * single-producer, single-consumer ring buffer is available for reading and, if
 * not, registers a signal function to be invoked once the requested range
 * becomes available.
 *
 * This function is lock-free. It MAY busy-wait while aborting a previous wait
 * operation with spscring_c_abort_wait(). This function MUST NOT be invoked by
 * a producer or by more than one concurrent consumer.
 *
 * @param ring a pointer to a ring buffer.
 * @param size the number of indices available for reading before the wait
 *             condition is satisfied.
 * @param func the signal function to be invoked when the wait condition is
 *             satisfied.
 * @param arg  the user-specific value to be passed as the second argument to
 *             <b>func</b>.
 *
 * @returns 1 if the wait operation was submitted, and 0 if the wait condition
 * was already satisfied and requested number of indices are available. In the
 * latter case, the signal function is _not_ invoked.
 */
int spscring_c_submit_wait(struct spscring *ring, size_t size,
		void (*func)(struct spscring *ring, void *arg), void *arg);

/**
 * Aborts a wait operation previously registered with spscring_c_submit_wait().
 *
 * This function is lock-free. It MAY busy-wait while spscring_p_commit() is
 * reading the function pointer and argument of an ongoing wait. This function
 * MUST NOT be invoked by a producer or by more than one concurrent consumer.
 *
 * @returns 1 if a wait operation was canceled, and 0 if no wait operation was
 * registered.
 */
int spscring_c_abort_wait(struct spscring *ring);

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_SPSCRING_H_
