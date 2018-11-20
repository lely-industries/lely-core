/**@file
 * This is the internal header file of the thread-safe lock-free memory page
 * functions.
 *
 * @copyright 2016-2018 Lely Industries N.V.
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

#ifndef LELY_UTIL_INTERN_PAGE_H_
#define LELY_UTIL_INTERN_PAGE_H_

#include "util.h"
#include <lely/libc/stdalign.h>
#ifndef LELY_NO_ATOMICS
#define LELY_NO_ATOMICS 1
#ifndef LELY_NO_THREADS
#include <lely/libc/stdatomic.h>
#ifndef __STDC_NO_ATOMICS__
#undef LELY_NO_ATOMICS
#endif
#endif
#endif
#include <lely/libc/stddef.h>
#include <lely/libc/stdlib.h>
#include <lely/util/errnum.h>

#include <assert.h>

#ifndef LELY_PAGE_SIZE
/// The minimum size (in bytes) of a single memory page.
#define LELY_PAGE_SIZE 65536
#endif

#ifndef LELY_PAGE_ALIGNMENT
/// The alignment (in bytes) of a memory page.
#define LELY_PAGE_ALIGNMENT 4096
#endif

/// A struct representing a single memory page.
struct page {
	/// A pointer to the next page.
	struct page *next;
	/// The size (in bytes) of the page.
	size_t size;
	/**
	 * The offset (in bytes) of the free region with respect to the start of
	 * the page.
	 */
#ifdef LELY_NO_ATOMICS
	size_t free;
#else
	atomic_size_t free;
#endif
};

#ifndef LELY_NO_ATOMICS
typedef _Atomic(struct page *) atomic_page_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a single memory page and prepends it to a list. This function is
 * thread-safe and lock-free (as long as the platform supports lock-free atomic
 * operations). The size of the page is a power of two times #LELY_PAGE_SIZE
 * with an alignment of #LELY_PAGE_ALIGNMENT.
 *
 * @param next the address of a pointer to the next memory page. On success,
 *             *<b>next</b> contains the address of the new page.
 * @param size the minimum number of bytes available on the page.
 *
 * @returns 0 on success, or -1 on error.
 *
 * @see page_destroy()
 */
#ifdef LELY_NO_ATOMICS
static int page_create(struct page **next, size_t size);
#else
static int page_create(volatile atomic_page_t *next, size_t size);
#endif

/**
 * Destroys a memory page and all pages after it.
 *
 * @see page_create()
 */
static void page_destroy(struct page *page);

/**
 * Allocates a space on a memory page. The alignment is inferred from the size.
 * The allocation is thread-safe and lock-free (as long as the platform supports
 * lock-free atomic operations).
 *
 * @param page a pointer to a memory #page.
 * @param size the number of bytes to allocate. The value of <b>size</b> is used
 *             as the alignment if it is a power of two and smaller than the
 *             default alignment (`alignof(max_align_t)`, equal to the alignment
 *             guarantees of malloc()). Otherwise the default alignment is used.
 *
 * @returns a pointer to the allocated memory, or NULL on error.
 *
 * @see page_aligned_alloc(), page_aligned_offset_alloc()
 */
static void *page_alloc(struct page *page, size_t size);

/**
 * Allocates a space on a memory page with the specified alignment. This
 * function is equivalent to
 * `page_aligned_offset_alloc(page, alignment, 0, size)`. The allocation is
 * thread-safe and lock-free (as long as the platform supports lock-free atomic
 * operations).
 *
 * @param page      a pointer to a memory #page.
 * @param alignment the alignment requirements (in bytes). The alignment MUST be
 *                  a power of two and CANNOT be larger than the alignment of
 *                  the page itself.
 * @param size      the number of bytes to allocate.
 *
 * @returns a pointer to the newly allocated memory, or NULL on error.
 *
 * @see page_alloc(), page_aligned_offset_alloc()
 */
static inline void *page_aligned_alloc(
		struct page *page, size_t alignment, size_t size);

/**
 * Allocates a space on a memory page with the specified alignment. The
 * allocation is thread-safe and lock-free (as long as the platform supports
 * lock-free atomic operations).
 *
 * @param page      a pointer to a memory #page.
 * @param alignment the alignment requirements (in bytes). The alignment MUST be
 *                  a power of two and CANNOT be larger than the alignment of
 *                  the page itself.
 * @param offset    the offset (in bytes) into the memory region of the address
 *                  that is to be aligned. If non-zero, it is not the beginning
 *                  of the memory region that is aligned, but the byte at the
 *                  offset. This is useful for allocating nested structures.
 * @param size      the number of bytes to allocate.
 *
 * @returns a pointer to the newly allocated memory, or NULL on error.
 *
 * @see page_alloc(), page_aligned_alloc()
 */
static void *page_aligned_offset_alloc(struct page *page, size_t alignment,
		size_t offset, size_t size);

static int
#ifdef LELY_NO_ATOMICS
page_create(struct page **next, size_t size)
#else
page_create(volatile atomic_page_t *next, size_t size)
#endif
{
	assert(next);

	size += ALIGN(sizeof(struct page), (size_t)ALIGNED_ALLOC_SIZE);
	size_t page_size = LELY_PAGE_SIZE;
	while (page_size < size)
		page_size *= 2;

	struct page *page = aligned_alloc(LELY_PAGE_ALIGNMENT, page_size);
	if (__unlikely(!page)) {
		set_errno(errno);
		return -1;
	}

	page->next = NULL;
	page->size = page_size;
#ifdef LELY_NO_ATOMICS
	page->free = ALIGN(sizeof(struct page), (size_t)ALIGNED_ALLOC_SIZE);
#else
	atomic_init(&page->free,
			ALIGN(sizeof(struct page), (size_t)ALIGNED_ALLOC_SIZE));
#endif

#ifdef LELY_NO_ATOMICS
	page->next = *next;
	*next = page;
#else
	// Atomically prepend the page to the beginning of the list.
	page->next = atomic_load_explicit(next, memory_order_acquire);
	while (__unlikely(!atomic_compare_exchange_weak_explicit(next,
			&page->next, page, memory_order_release,
			memory_order_relaxed)))
		;
#endif

	return 0;
}

static void
page_destroy(struct page *page)
{
	while (page) {
		struct page *next = page->next;
		aligned_free(page);
		page = next;
	}
}

static inline void *
page_alloc(struct page *page, size_t size)
{
	size_t alignment = alignof(max_align_t);
	if (powerof2(size))
		alignment = MIN(alignment, size);

	return page_aligned_offset_alloc(page, alignment, 0, size);
}

static inline void *
page_aligned_alloc(struct page *page, size_t alignment, size_t size)
{
	return page_aligned_offset_alloc(page, alignment, 0, size);
}

static void *
page_aligned_offset_alloc(
		struct page *page, size_t alignment, size_t offset, size_t size)
{
	// clang-format off
	if (__unlikely(!page || !powerof2(alignment)
			|| alignment > LELY_PAGE_ALIGNMENT || offset > size))
		// clang-format on
		return NULL;

	alignment = MAX(alignment, 1);

#ifdef LELY_NO_ATOMICS
	size_t begin = ALIGN(page->free + offset, alignment) - offset;
	if (__unlikely(begin + size > page->size))
		return NULL;
	page->free = begin + size;
#else
	// Try to atomically reserve space on the page.
	unsigned long free =
			atomic_load_explicit(&page->free, memory_order_acquire);
	size_t begin;
	do {
		begin = ALIGN(free + offset, alignment) - offset;
		if (__unlikely(begin + size > page->size))
			return NULL;
	} while (__unlikely(!atomic_compare_exchange_weak_explicit(&page->free,
			&free, begin + size, memory_order_release,
			memory_order_relaxed)));
#endif

	return (char *)page + begin;
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_INTERN_PAGE_H_
