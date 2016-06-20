/*!\file
 * This file is part of the utilities library; it contains the implementation of
 * the id list.
 *
 * \see lely/util/idlist.h
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
#include <lely/util/bitset.h>
#include <lely/util/errnum.h>
#include <lely/util/idlist.h>

#include <assert.h>
#include <stdlib.h>

//! An id list.
struct __idlist {
	//! The #bitset keeping track of which ids are in use.
	struct bitset set;
	//! The total number of ids currently in use.
	int size;
	/*!
	 * A value guaranteed to be smaller than or equal to the first unused
	 * id.
	 */
	int next;
	//! The array containing the pointers to the values.
	void **values;
	//! A pointer to the destructor for #values.
	idlist_dtor_t dtor;
};

LELY_UTIL_EXPORT void *
__idlist_alloc(void)
{
	return malloc(sizeof(struct __idlist));
}

LELY_UTIL_EXPORT void
__idlist_free(void *ptr)
{
	free(ptr);
}

LELY_UTIL_EXPORT struct __idlist *
__idlist_init(struct __idlist *list, int size, idlist_dtor_t dtor)
{
	assert(list);

	errc_t errc = 0;

	if (__unlikely(bitset_init(&list->set, size) == -1)) {
		errc = get_errc();
		goto error_init_set;
	}

	list->size = 0;
	list->next = 0;

	size = bitset_size(&list->set);
	list->values = malloc(size * sizeof(void *));
	if (__unlikely(!list->values)) {
		errc = get_errc();
		goto error_alloc_values;
	}
	for (int i = 0; i < size; i++)
		list->values[i] = NULL;

	list->dtor = dtor;

	return list;

error_alloc_values:
	bitset_fini(&list->set);
error_init_set:
	set_errc(errc);
	return NULL;
}

LELY_UTIL_EXPORT void
__idlist_fini(struct __idlist *list)
{
	assert(list);

	if (list->dtor) {
		int id = 0;
		while ((id = bitset_fns(&list->set, id)) > 0)
			list->dtor(list->values[id - 1]);
	}

	free(list->values);

	bitset_fini(&list->set);
}

LELY_UTIL_EXPORT idlist_t *
idlist_create(int size, idlist_dtor_t dtor)
{
	errc_t errc = 0;

	idlist_t *list = __idlist_alloc();
	if (__unlikely(!list)) {
		errc = get_errc();
		goto error_alloc_list;
	}

	if (__unlikely(!__idlist_init(list, size, dtor))) {
		errc = get_errc();
		goto error_init_list;
	}

	return list;

error_init_list:
	__idlist_free(list);
error_alloc_list:
	set_errc(errc);
	return NULL;
}

LELY_UTIL_EXPORT void
idlist_destroy(idlist_t *list)
{
	if (list) {
		__idlist_fini(list);
		__idlist_free(list);
	}
}

LELY_UTIL_EXPORT int
idlist_empty(const idlist_t *list)
{
	return !idlist_size(list);
}

LELY_UTIL_EXPORT int
idlist_size(const idlist_t *list)
{
	assert(list);

	return list->size;
}

LELY_UTIL_EXPORT int
idlist_capacity(const idlist_t *list)
{
	assert(list);

	return bitset_size(&list->set) - list->size;
}

LELY_UTIL_EXPORT int
idlist_reserve(idlist_t *list, int size)
{
	assert(list);

	errc_t errc = 0;

	int capacity = idlist_capacity(list);
	if (__likely(size <= capacity))
		return capacity;

	int old_size = bitset_size(&list->set);
	// The required size equals the number of used ids plus the requested
	// capacity. To limit the number of allocations, we keep doubling the
	// size of the list until it is large enough.
	int new_size = idlist_size(list);
	while (new_size < idlist_size(list) + size)
		new_size *= 2;

	if (__unlikely(bitset_resize(&list->set, new_size) == -1)) {
		errc = get_errc();
		goto error_resize;
	}
	new_size = bitset_size(&list->set);

	void **values = realloc(list->values, new_size * sizeof(void *));
	if (__unlikely(!values)) {
		errc = get_errc();
		goto error_realloc;
	}
	list->values = values;

	// Clear the new values.
	for (int i = old_size; i < new_size; i++)
		list->values[i] = NULL;

	return idlist_capacity(list);

error_realloc:
	bitset_resize(&list->set, old_size);
error_resize:
	set_errc(errc);
	return 0;
}

LELY_UTIL_EXPORT int
idlist_insert(idlist_t *list, void *value)
{
	assert(list);

	int id = bitset_fnz(&list->set, list->next) - 1;
	if (__unlikely(id == -1))
		return -1;
	list->size++;
	list->next = id + 1;

	bitset_set(&list->set, id);
	list->values[id] = value;

	return id;
}

LELY_UTIL_EXPORT void *
idlist_remove(idlist_t *list, int id)
{
	assert(list);

	if (__unlikely(!bitset_test(&list->set, id)))
		return NULL;

	bitset_clr(&list->set, id);
	list->size--;
	list->next = MIN(list->next, id);

	void *value = list->values[id];
	list->values[id] = NULL;
	return value;
}

LELY_UTIL_EXPORT void *
idlist_find(const idlist_t *list, int id)
{
	assert(list);

	return __likely(bitset_test(&list->set, id)) ? list->values[id] : NULL;
}

