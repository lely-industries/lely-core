/**@file
 * This file is part of the utilities library; it exposes the singly linked list
 * functions.
 *
 * @see lely/util/sllist.h
 *
 * @copyright 2013-2020 Lely Industries N.V.
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
#define LELY_UTIL_SLLIST_INLINE extern inline
#include <lely/util/sllist.h>
#include <lely/util/util.h>

#include <assert.h>

struct slnode *
sllist_pop_back(struct sllist *list)
{
	assert(list);

	struct slnode **pnode = &list->first;
	while (*pnode && (*pnode)->next)
		pnode = &(*pnode)->next;
	struct slnode *node = *pnode;
	if (node)
		*(list->plast = pnode) = NULL;
	return node;
}

struct slnode *
sllist_remove(struct sllist *list, struct slnode *node)
{
	assert(list);

	if (node) {
		struct slnode **pnode = &list->first;
		while (*pnode && *pnode != node)
			pnode = &(*pnode)->next;
		if ((node = *pnode)) {
			if (!(*pnode = node->next))
				list->plast = pnode;
			node->next = NULL;
		}
	}
	return node;
}

int
sllist_contains(const struct sllist *list, const struct slnode *node)
{
	assert(list);

	if (!node)
		return 0;

	sllist_foreach (list, node_) {
		if (node_ == node)
			return 1;
	}

	return 0;
}

struct slnode *
sllist_last(const struct sllist *list)
{
	assert(list);

	return list->plast != &list->first
			? structof(list->plast, struct slnode, next)
			: NULL;
}
