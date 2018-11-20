/**@file
 * This header file is part of the utilities library; it contains the
 * thread-safe lock-free memory pool allocator declarations.
 *
 * A pool allocator is used to allocate objects of the same (typically small)
 * size, but with different lifetimes. Objects can be allocated and freed
 * individually. Nevertheless, the total amount of memory used by the pool never
 * decreases since memory pages in the pool are never freed, even when all
 * objects on the page are freed. This prevents repeated calls to `malloc()` and
 * `free()`.
 *
 * Apart from alignment restrictions and the size of the #page headers, this
 * implementation has zero space overhead for the individual objects. All
 * operations, including the creation of the pool, are O(1), and none of them
 * touch unused memory. Additionally, the pool allocator reuses the memory from
 * freed objects before using uninitialized memory or allocating new memory
 * pages. On most modern operating systems this means that it is safe to create
 * pools with large numbers of elements, since memory won't actually be reserved
 * until objects are allocated from it.
 *
 * Free elements in the pool are tracked with a singly-linked list, with
 * recently freed items near the beginning of the list. The minimum object size
 * is therefore the size of a pointer. To prevent having to initialize every
 * free element on a page by adding it to the list, which would be an O(n)
 * operation, we only allocate an extra object from a page when the free list is
 * empty. Since all of this is simply a matter of pointer manipulation, we never
 * touch memory that is not used by an object.
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

#ifndef LELY_UTIL_POOL_H_
#define LELY_UTIL_POOL_H_

#include <lely/util/util.h>

#include <stddef.h>

struct __pool;
#ifndef __cplusplus
/// An opaque pool memory allocator type.
typedef struct __pool pool_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

LELY_UTIL_EXTERN void *__pool_alloc(void);
LELY_UTIL_EXTERN void __pool_free(void *ptr);
LELY_UTIL_EXTERN struct __pool *__pool_init(
		struct __pool *pool, size_t nmemb, size_t size);
LELY_UTIL_EXTERN void __pool_fini(struct __pool *pool);

/**
 * Creates a new pool allocator with enough space for <b>nmemb</b> objects of
 * <b>size</b> bytes. This space is allocated from a single memory #page. Each
 * subsequent page will have the same size.
 *
 * @param nmemb the minimum number of elements available in the pool.
 * @param size  the size (in bytes) of each element.
 *
 * @returns a pointer to a new pool allocator, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see pool_destroy()
 */
LELY_UTIL_EXTERN pool_t *pool_create(size_t nmemb, size_t size);

/**
 * Destroys a pool allocator and frees all objects allocated with it.
 *
 * @see pool_create()
 */
LELY_UTIL_EXTERN void pool_destroy(pool_t *pool);

/**
 * Allocates an object of pool_size() bytes from a pool. The allocation is
 * thread-safe and lock-free (as long as the platform supports lock-free atomic
 * operations).
 *
 * @param pool a pointer to a pool allocator.
 *
 * @returns a pointer to the newly allocated object, or NULL on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see pool_free()
 */
LELY_UTIL_EXTERN void *pool_alloc(pool_t *pool);

/**
 * Frees the memory allocated for an object. The deallocation is thread-safe and
 * lock-free (as long as the platform supports lock-free atomic operations).
 *
 * @param pool a pointer to a pool allocator.
 * @param ptr  a pointer to an object allocated from the pool (can be NULL).
 *
 * @see pool_alloc()
 */
LELY_UTIL_EXTERN void pool_free(pool_t *pool, void *ptr);

/// Returns the size (in bytes) of the objects in a pool.
LELY_UTIL_EXTERN size_t pool_size(const pool_t *pool);

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_POOL_H_
