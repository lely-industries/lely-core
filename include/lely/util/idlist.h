/*!\file
 * This header file is part of the utilities library; it contains the id list
 * declarations.
 *
 * An id list is a type of dictionary inspired by POSIX file descriptors. When a
 * value is inserted into the list, a numerical id is returned which serves as
 * the key. Valid ids start from 0, and the list always returns the smallest
 * unused id. The list keeps track of used ids with a #bitset.
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

#ifndef LELY_UTIL_IDLIST_H
#define LELY_UTIL_IDLIST_H

#include <lely/util/util.h>

struct __idlist;
#ifndef __cplusplus
//! An opaque id list.
typedef struct __idlist idlist_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

//! The function pointer type used for a destructor for a value in an id list.
typedef void (*idlist_dtor_t)(void *value);

LELY_UTIL_EXTERN void *__idlist_alloc(void);
LELY_UTIL_EXTERN void __idlist_free(void *ptr);
LELY_UTIL_EXTERN struct __idlist *__idlist_init(struct __idlist *list, int size,
		idlist_dtor_t dtor);
LELY_UTIL_EXTERN void __idlist_fini(struct __idlist *list);

/*!
 * Creates an id list.
 *
 * \param size the minimum number of values the id list can contain before
 *             before having to be resized. This is also the maximum value that
 *             can be returned by idlist_insert().
 * \param dtor an optional pointer to the destructor for values. If not NULL,
 *             this function is invoked by idlist_destroy() for each remaining
 *             value in the list.
 *
 * \returns a pointer to a new id list, or NULL on error. In the latter case,
 * the error number can be obtained with get_errnum().
 *
 * \see idlist_destroy()
 */
LELY_UTIL_EXTERN idlist_t *idlist_create(int size, idlist_dtor_t dtor);

//! Destroys an id list, including all remaining values. \see idlist_create()
LELY_UTIL_EXTERN void idlist_destroy(idlist_t *list);

//! Returns 1 if the id list is empty, and 0 if not.
LELY_UTIL_EXTERN int idlist_empty(const idlist_t *list);

//! Returns the size (in number of used ids) of an id list.
LELY_UTIL_EXTERN int idlist_size(const idlist_t *list);

//! Returns the number of unused ids available in an id list.
LELY_UTIL_EXTERN int idlist_capacity(const idlist_t *list);

/*!
 * Resizes an id list, if necessary, to make room for at least an additional
 * \a size ids.
 *
 * \returns the new capacity of the id list, or 0 on error. The new capacity can
 * be larger than the requested capacity.
 */
LELY_UTIL_EXTERN int idlist_reserve(idlist_t *list, int size);

/*!
 * Allocates a new id from an id list and associates it with \a value. The new
 * id is guaranteed to be the smallest unused id.
 *
 * \param list  a pointer to an id list.
 * \param value a pointer to the value to be inserted.
 *
 * \returns the id on success, or -1 if the list is full. In the latter case,
 * the id list can be resized with idlist_resize().
 *
 * \see idlist_remove(), idlist_find()
 */
LELY_UTIL_EXTERN int idlist_insert(idlist_t *list, void *value);

/*!
 * Frees an id from an id list and returns its value. Note that this function
 * does _not_ invoke the destructor specified to idlist_create().
 *
 * \returns a pointer to the value associated with the removed id, or NULL if it
 * is not in use.
 *
 * \see idlist_insert()
 */
LELY_UTIL_EXTERN void *idlist_remove(idlist_t *list, int id);

/*!
 * Returns a pointer to the value associated with an id in an id list, or NULL
 * if the id is not in use.
 *
 * \see idlist_insert()
 */
LELY_UTIL_EXTERN void *idlist_find(const idlist_t *list, int id);

#ifdef __cplusplus
}
#endif

#endif

