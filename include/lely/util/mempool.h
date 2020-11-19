/**@file
 * This header file is part of the utilities library; it contains memory pool
 * based allocator declaration.
 *
 * @copyright 2020 N7 Space sp. z o.o.
 *
 * Memory pool allocator was developed under a programme of,
 * and funded by, the European Space Agency.
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

#ifndef LELY_UTIL_MEMPOOL_H_
#define LELY_UTIL_MEMPOOL_H_

#include <assert.h>
#include <stdint.h>

#include <lely/util/memory.h>

#ifndef LELY_UTIL_MEMPOOL_INLINE
#define LELY_UTIL_MEMPOOL_INLINE static inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// A memory pool
struct mempool {
	/// Pointer to 'virtual table' of the allocator. Must be first field.
	const struct alloc_vtbl *vtbl;
	/// A pointer to the first byte in the pool.
	uint8_t *beg;
	/// A pointer one past the last byte in the pool.
	uint8_t *end;
	/// A pointer to next free byte in the pool.
	uint8_t *cur;
};

/**
 * Initialized a memory pool allocator.
 *
 * NOTE: This allocator newer really frees a memory - this allocation scheme
 * should be used in systems, that initialize once at a start and can work with
 * fixed memory buffer.
 *
 * @param pool    a pointer to the memory pool.
 * @param memory  a pointer to the memory region to be used by the pool.
 *                MUST NOT be NULL.
 * @param size    the number of bytes availabe at <b>memory</b>.
 *                MUST NOT be zero.
 *
 * @see mem_alloc()
 * @see mem_free()
 * @see mem_size()
 * @see mem_capacity()
 */
void mempool_init(struct mempool *pool, uint8_t *memory, size_t size);

/**
 * Converts pointer to memory pool to pointer to base allocator interface.
 *
 * @param pool a pointer to the memory pool.
 *
 * @return pointer to the allocator interface.
 *
 * @see mem_alloc()
 * @see mem_free()
 * @see mem_size()
 * @see mem_capacity()
 */
LELY_UTIL_MEMPOOL_INLINE alloc_t *mempool_to_alloc_t(
		const struct mempool *const pool);

LELY_UTIL_MEMPOOL_INLINE alloc_t *
mempool_to_alloc_t(const struct mempool *const pool)
{
	assert(pool != NULL);
	return &pool->vtbl;
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_MEMPOOL_H_
