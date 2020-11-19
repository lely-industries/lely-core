/**@file
 * This header file is part of the utilities library; it contains memory pool
 * based allocator implementation.
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

#define LELY_UTIL_MEMPOOL_INLINE extern inline
#include <lely/util/mempool.h>

#include <lely/libc/stddef.h>

#include <lely/util/errnum.h>
#include <lely/util/util.h>

static inline struct mempool *
mempool_from_alloc(alloc_t *const alloc)
{
	assert(alloc != NULL);
	return (struct mempool *)((uintptr_t)alloc);
}

static size_t
mempool_size(alloc_t *const alloc)
{
	const struct mempool *const pool = mempool_from_alloc(alloc);
	return pool->cur - pool->beg;
}

static size_t
mempool_capacity(alloc_t *const alloc)
{
	const struct mempool *const pool = mempool_from_alloc(alloc);
	return pool->end - pool->cur;
}

static void *
mempool_alloc(alloc_t *const alloc, size_t alignment, const size_t size)
{
	if (size == 0)
		return NULL;

	if (alignment == 0)
		alignment = _Alignof(max_align_t);

	if (!powerof2(alignment)) {
		set_errnum(ERRNUM_INVAL);
		return NULL;
	}

	if (mempool_capacity(alloc) < size) {
		set_errnum(ERRNUM_NOMEM);
		return NULL;
	}

	struct mempool *const pool = mempool_from_alloc(alloc);
	pool->cur = (void *)ALIGN((uintptr_t)pool->cur, alignment);
	void *result = pool->cur;
	pool->cur += size;

	return result;
}

static void
mempool_free(alloc_t *const alloc, void *const ptr)
{
	assert(alloc != NULL);
	(void)alloc;
	(void)ptr;
}

static struct alloc_vtbl mempool_vtbl = {
	.alloc = mempool_alloc,
	.free = mempool_free,
	.size = mempool_size,
	.capacity = mempool_capacity,
};

void
mempool_init(struct mempool *const pool, uint8_t *const memory,
		const size_t size)
{
	assert(pool != NULL);
	assert(memory != NULL && size > 0);

	pool->vtbl = &mempool_vtbl;

	pool->beg = memory;
	pool->end = memory + size;

	pool->cur = pool->beg;
}
