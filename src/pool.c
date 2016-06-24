/*!\file
 * This file is part of the utilities library; it contains the implementation of
 * the thread-safe lock-free memory pool allocator.
 *
 * \see lely/util/pool.h
 *
 * \copyright 2016 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
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
#include <lely/util/errnum.h>
#include <lely/util/pool.h>
#include "page.h"

//! A memory pool allocator for fixed-size objects.
struct __pool {
	//! A pointer to the first free element.
#ifdef LELY_NO_ATOMICS
	void *free;
#else
	_Atomic(void *) free;
#endif
	//! A pointer to the most recently allocated memory #page.
#ifdef LELY_NO_ATOMICS
	struct page *page;
#else
	atomic_page_t page;
#endif
	//! The minimum number of elements per page.
	size_t nmemb;
	//! The size (in bytes) of each element.
	size_t size;
};

LELY_UTIL_EXPORT void *
__pool_alloc(void)
{
	void *ptr = malloc(sizeof(struct __pool));
	if (__unlikely(!ptr))
		set_errno(errno);
	return ptr;
}

LELY_UTIL_EXPORT void
__pool_free(void *ptr)
{
	free(ptr);
}

LELY_UTIL_EXPORT struct __pool *
__pool_init(struct __pool *pool, size_t nmemb, size_t size)
{
	assert(pool);

	// Each element should be at least large enough to hold a pointer;
	// otherwise we cannot store a singly-linked list of free elements.
	if (size < sizeof(void *))
		size = sizeof(void *);

	// Round the size up to the nearest alignment.
	if (!powerof2(size))
		size = ALIGN(size, alignof(max_align_t));

#ifdef LELY_NO_ATOMICS
	pool->free = NULL;
#else
	atomic_init(&pool->free, NULL);
#endif

#ifdef LELY_NO_ATOMICS
	pool->page = NULL;
#else
	atomic_init(&pool->page, NULL);
#endif
	if (__unlikely(page_create(&pool->page, nmemb * size) == -1))
		return NULL;

	pool->size = size;
	pool->nmemb = nmemb;

	return pool;
}

LELY_UTIL_EXPORT void
__pool_fini(struct __pool *pool)
{
	assert(pool);

#ifdef LELY_NO_ATOMICS
	page_destroy(pool->page);
#else
	page_destroy(atomic_load(&pool->page));
#endif
}

LELY_UTIL_EXPORT pool_t *
pool_create(size_t nmemb, size_t size)
{
	errc_t errc = 0;

	pool_t *pool = __pool_alloc();
	if (__unlikely(!pool)) {
		errc = get_errc();
		goto error_alloc_pool;
	}

	if (__unlikely(!__pool_init(pool, nmemb, size))) {
		errc = get_errc();
		goto error_init_pool;
	}

	return pool;

error_init_pool:
	__pool_free(pool);
error_alloc_pool:
	set_errc(errc);
	return NULL;
}

LELY_UTIL_EXPORT void
pool_destroy(pool_t *pool)
{
	if (pool) {
		__pool_fini(pool);
		__pool_free(pool);
	}
}

LELY_UTIL_EXPORT void *
pool_alloc(pool_t *pool)
{
	if (__unlikely(!pool))
		return NULL;

	// First try to reuse a previously freed object.
#ifdef LELY_NO_ATOMICS
	void *ptr = pool->free;
	if (ptr) {
		pool->free = *(void **)pool->free;
		return ptr;
	}
#else
	void *ptr = atomic_load_explicit(&pool->free, memory_order_acquire);
	while (__unlikely(ptr && !atomic_compare_exchange_weak_explicit(
			&pool->free, &ptr, *(void **)ptr, memory_order_release,
			memory_order_relaxed)));
	if (ptr)
		return ptr;
#endif

	// If no freed objects are left, allocate memory from an existing page.
#ifdef LELY_NO_ATOMICS
	struct page *page = pool->page;
#else
	struct page *page = atomic_load_explicit(&pool->page,
			memory_order_acquire);
#endif
	ptr = page_alloc(page, pool->size);
	if (__likely(ptr))
		return ptr;

	// If no free space remains on the existing page, create a new page.
	if (__unlikely(page_create(&pool->page, pool->nmemb * pool->size) == -1))
		return NULL;

	return pool_alloc(pool);
}

LELY_UTIL_EXPORT void
pool_free(pool_t *pool, void *ptr)
{
	assert(pool);

	if (!ptr)
		return;

#ifdef LELY_NO_ATOMICS
	*(void **)ptr = pool->free;
	pool->free = ptr;
#else
	// Atomically prepend the object to the beginning of the free list.
	*(void **)ptr = atomic_load_explicit(&pool->free, memory_order_acquire);
	while (__unlikely(!atomic_compare_exchange_weak_explicit(&pool->free,
			(void **)ptr, ptr, memory_order_release,
			memory_order_relaxed)));
#endif
}

LELY_UTIL_EXPORT size_t
pool_size(const pool_t *pool)
{
	assert(pool);

	return pool->size;
}

