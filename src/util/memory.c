/**@file
 * This file is part of the utilities library; it contains the implementation of
 * the default memory allocator.
 *
 * @see lely/util/memory.h
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
#if !LELY_NO_THREADS && !LELY_NO_ATOMICS
#include <lely/libc/stdatomic.h>
#endif
#include <lely/libc/stddef.h>
#include <lely/util/errnum.h>
#include <lely/util/memory.h>
#include <lely/util/util.h>

#ifndef LELY_HAVE_MALLOC_USABLE_SIZE
#if defined(__GLIBC__) && !_WIN32
#define LELY_HAVE_MALLOC_USABLE_SIZE 1
#endif
#endif

#include <assert.h>
#if !LELY_NO_MALLOC
#include <stdint.h>
#if !_WIN32
#include <stdlib.h>
#endif
#if _WIN32 || LELY_HAVE_MALLOC_USABLE_SIZE
#include <malloc.h>
#endif
#endif

static void *default_mem_alloc(size_t alignment, size_t size);
static void default_mem_free(void *ptr);

#if LELY_NO_THREADS || LELY_NO_ATOMICS
static size_t default_mem_size;
#else
static atomic_size_t default_mem_size;
#endif

void *
mem_alloc(alloc_t *alloc, size_t alignment, size_t size)
{
	if (alloc) {
		assert((*alloc)->alloc);
		return (*alloc)->alloc(alloc, alignment, size);
	} else {
		return default_mem_alloc(alignment, size);
	}
}

void
mem_free(alloc_t *alloc, void *ptr)
{
	if (alloc) {
		assert((*alloc)->free);
		(*alloc)->free(alloc, ptr);
	} else {
		default_mem_free(ptr);
	}
}

size_t
mem_size(const alloc_t *alloc)
{
	if (alloc) {
		assert((*alloc)->size);
		return (*alloc)->size(alloc);
	} else {
#if LELY_NO_THREADS || LELY_NO_ATOMICS
		return default_mem_size;
#else
		return atomic_load_explicit(
				&default_mem_size, memory_order_relaxed);
#endif
	}
}

size_t
mem_capacity(const alloc_t *alloc)
{
	if (alloc) {
		assert((*alloc)->capacity);
		return (*alloc)->capacity(alloc);
	} else {
#if LELY_NO_MALLOC
		return 0;
#else
		return SIZE_MAX;
#endif
	}
}

static void *
default_mem_alloc(size_t alignment, size_t size)
{
	if (!size)
		return NULL;

	if (!alignment)
		alignment = _Alignof(max_align_t);

#if LELY_NO_MALLOC
	if (!powerof2(alignment)) {
		set_errnum(ERRNUM_INVAL);
		return NULL;
	}

	set_errnum(ERRNUM_NOMEM);
	void *ptr = NULL;
#elif _WIN32
	int errsv = errno;
	void *ptr = _aligned_malloc(size, alignment);
	if (!ptr) {
		set_errc(errno2c(errno));
		errno = errsv;
	}
#elif __STDC_VERSION__ >= 201112L
	if (powerof2(alignment))
		size = ALIGN(size, alignment);

	void *ptr = aligned_alloc(alignment, size);
#elif _POSIX_C_SOURCE >= 200112L
	void *ptr = NULL;
	int errc = posix_memalign(&ptr, alignment, size);
	if (errc)
		set_errc(errc);
#else
	if (!powerof2(alignment) || alignment > _Alignof(max_align_t)) {
		set_errnum(ERRNUM_INVAL);
		return NULL;
	}

	void *ptr = malloc(size);
#endif

#if !LELY_NO_MALLOC && !_WIN32 && LELY_HAVE_MALLOC_USABLE_SIZE
	if (ptr) {
		size = malloc_usable_size(ptr);
#if LELY_NO_THREADS || LELY_NO_ATOMICS
		default_mem_size += size;
#else
		atomic_fetch_add_explicit(
				&default_mem_size, size, memory_order_relaxed);
#endif
	}
#endif // !LELY_NO_MALLOC && !_WIN32 && LELY_HAVE_MALLOC_USABLE_SIZE

	return ptr;
}

static void
default_mem_free(void *ptr)
{
#if LELY_NO_MALLOC
	(void)ptr;
#elif _WIN32
	_aligned_free(ptr);
#else
#if LELY_HAVE_MALLOC_USABLE_SIZE
	if (ptr) {
		size_t size = malloc_usable_size(ptr);
#if LELY_NO_THREADS || LELY_NO_ATOMICS
		assert(default_mem_size >= size);
		default_mem_size -= size;
#else
		atomic_fetch_sub_explicit(
				&default_mem_size, size, memory_order_relaxed);
#endif
	}
#endif // LELY_HAVE_MALLOC_USABLE_SIZE
	free(ptr);
#endif
}
