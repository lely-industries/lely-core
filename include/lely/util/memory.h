/**@file
 * This header file is part of the utilities library; it contains the abstract
 * memory allocator declarations.
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

#ifndef LELY_UTIL_MEMORY_H_
#define LELY_UTIL_MEMORY_H_

#include <lely/compat/features.h>

#include <stddef.h>

/// An abstract memory allocator.
typedef const struct alloc_vtbl *const alloc_t;

#ifdef __cplusplus
extern "C" {
#endif

/// The virtual table for memory allcators.
struct alloc_vtbl {
	void *(*alloc)(alloc_t *alloc, size_t alignment, size_t size);
	void (*free)(alloc_t *alloc, void *ptr);
	size_t (*size)(const alloc_t *alloc);
	size_t (*capacity)(const alloc_t *alloc);
};

/**
 * Allocates space for an object of the specified alignmend and size.
 *
 * @param alloc     a pointer to a memory allocator. If <b>alloc</b> is NULL,
 *                  the default memory allocator is used.
 * @param alignment the alignment (in bytes) of the space to be allocated.
 *                  <b>alignment</b> MUST be an integral multiple of 2. If
 *                  <b>alignment</b> is 0, the default alignment is used.
 * @param size      the number of bytes to allocate. <b>size</b> is rounded up
 *                  to the nearest integral multiple if <b>alignment</b>.
 *
 * returns a pointer to the allocated space, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
void *mem_alloc(alloc_t *alloc, size_t alignment, size_t size);

/**
 * Frees space for an object previously allocated by mem_alloc().
 *
 * @param alloc a pointer to a memory allocator. If <b>alloc</b> is NULL, the
 *              default memory allocator is used.
 * @param ptr   a pointer to the space to be freed. If <b>ptr</b> is NULL, no
 *              action occurs. Otherwise, <b>ptr</b> MUST point to space that
 *              has been previously allocated by <b>alloc</b> and has not not
 *              yet freed.
 */
void mem_free(alloc_t *alloc, void *ptr);

/**
 * Returns the total number of bytes allocated by a memory allocator, or 0 if
 * the allocator does not track memory usage. Note that because of allocation
 * overhead, the total size MAY be larger than the sum of the sizes specified to
 * mem_alloc().
 */
size_t mem_size(const alloc_t *alloc);

/**
 * Returns the number of bytes available for allocation by a memory allocator,
 * or `SIZE_MAX` if the allocator does not track memory capacity.
 */
size_t mem_capacity(const alloc_t *alloc);

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_MEMORY_H_
