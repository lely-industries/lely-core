/*!\file
 * This file is part of the utilities library; it contains the implementation of
 * the hash table.
 *
 * \see lely/util/htab.h
 *
 * \copyright 2017 Lely Industries N.V.
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
#define LELY_UTIL_HTAB_INLINE	extern inline LELY_DLL_EXPORT
#include <lely/util/htab.h>

#include <assert.h>
#include <stdlib.h>

LELY_UTIL_EXPORT int
htab_init(struct htab *tab, int (*eq)(const void *, const void *),
		size_t (*hash)(const void *), size_t num_slots)
{
	assert(tab);
	assert(eq);
	assert(hash);

	if (__unlikely(!num_slots)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	struct hnode **slots = malloc(num_slots * sizeof(struct hnode *));
	if (__unlikely(!slots)) {
		set_errno(errno);
		return -1;
	}
	for (size_t i = 0; i < num_slots; i++)
		slots[i] = NULL;

	tab->eq = eq;
	tab->hash = hash;
	tab->num_slots = num_slots;
	tab->slots = slots;
	tab->num_nodes = 0;

	return 0;
}

LELY_UTIL_EXPORT void
htab_fini(struct htab *tab)
{
	assert(tab);

	tab->eq = NULL;
	tab->hash = NULL;
	tab->num_slots = 0;
	free(tab->slots);
	tab->slots = NULL;
	tab->num_nodes = 0;
}

LELY_UTIL_EXPORT int
htab_resize(struct htab *tab, size_t num_slots)
{
	assert(tab);
	assert(tab->slots);

	if (__unlikely(!num_slots)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	struct hnode **slots = malloc(num_slots * sizeof(struct hnode *));
	if (__unlikely(!slots)) {
		set_errno(errno);
		return -1;
	}
	for (size_t i = 0; i < num_slots; i++)
		slots[i] = NULL;

	// Re-initialize the hash table with the new (empty) slot array.
	size_t old_num_slots = tab->num_slots;
	tab->num_slots = num_slots;
	struct hnode **old_slots = tab->slots;
	tab->slots = slots;
	tab->num_nodes = 0;

	// Copy all nodes back into the table.
	for (size_t i = 0; i < old_num_slots; i++) {
		struct hnode *node = old_slots[i];
		while (node) {
			struct hnode *next = node->next;
			htab_insert(tab, node);
			node = next;
		};
	}

	return 0;
}

LELY_UTIL_EXPORT void
htab_insert(struct htab *tab, struct hnode *node)
{
	assert(tab);
	assert(tab->hash);
	assert(tab->slots);
	assert(node);

	node->hash = tab->hash(node->key);
	hnode_insert(&tab->slots[node->hash % tab->num_slots], node);

	tab->num_nodes++;
}

LELY_UTIL_EXPORT void
htab_remove(struct htab *tab, struct hnode *node)
{
	assert(tab);
	assert(node);

	hnode_remove(node);

	tab->num_nodes--;
}

LELY_UTIL_EXPORT struct hnode *
htab_find(const struct htab *tab, const void *key)
{
	assert(tab);
	assert(tab->eq);
	assert(tab->hash);
	assert(tab->slots);

	size_t hash = tab->hash(key);
	struct hnode *node = tab->slots[hash % tab->num_slots];
	while (node) {
		if (node->hash == hash && (node->key == key
				|| tab->eq(node->key, key)))
			return node;
		node = node->next;
	}
	return NULL;
}

