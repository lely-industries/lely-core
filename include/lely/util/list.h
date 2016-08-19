/*!\file
 * This header file is part of the utilities library; it contains the
 * <a href="https://en.wikipedia.org/wiki/Linked_list">linked list</a>
 * declarations.
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

#ifndef LELY_UTIL_LIST_H
#define LELY_UTIL_LIST_H

#include <lely/util/util.h>

#include <stddef.h>

#ifndef LELY_UTIL_LIST_INLINE
#define LELY_UTIL_LIST_INLINE	inline
#endif

/*!
 * A node in a doubly linked list. To associate a value with a node, embed the
 * node in a struct containing the value and use structof() to obtain the struct
 * from the node.
 */
struct dlnode {
	//! A pointer to the previous node in the list.
	struct dlnode *prev;
	//! A pointer to the next node in the list.
	struct dlnode *next;
};

#ifdef __cplusplus
extern "C" {
#endif

//! Initializes a node in a doubly linked list.
LELY_UTIL_LIST_INLINE void dlnode_init(struct dlnode *node);

//! Inserts \a node after \a prev.
LELY_UTIL_LIST_INLINE void dlnode_insert_after(struct dlnode *prev,
		struct dlnode *node);

//! Inserts \a node before \a next.
LELY_UTIL_LIST_INLINE void dlnode_insert_before(struct dlnode *next,
		struct dlnode *node);

/*!
 * Removes \a node from a list. Note that this function does _not_ reset the
 * \a prev and \a next fields of the node.
 */
LELY_UTIL_LIST_INLINE void dlnode_remove(struct dlnode *node);

/*!
 * Iterates in order over each node in a doubly linked list. It is safe to
 * remove the current node during the iteration.
 *
 * \param first a pointer to the first node in the list.
 * \param node  the name of the pointer to the nodes. This variable is declared
 *              in the scope of the loop.
 */
#ifdef __COUNTER__
#define dlnode_foreach(first, node) \
	_dlnode_foreach(first, node, __COUNTER__)
#else
#define dlnode_foreach(first, node) \
	_dlnode_foreach(first, node, __LINE__)
#endif
#define _dlnode_foreach(first, node, n) \
	__dlnode_foreach(first, node, n)
#define __dlnode_foreach(first, node, n) \
	for (struct dlnode *(node) = (first), \
			*__dlnode_next_##n = (node) ? (node)->next : NULL; \
			(node); (node) = __dlnode_next_##n, \
			__dlnode_next_##n = (node) ? (node)->next : NULL)

LELY_UTIL_LIST_INLINE void
dlnode_init(struct dlnode *node)
{
	node->prev = NULL;
	node->next = NULL;
}

LELY_UTIL_LIST_INLINE void
dlnode_insert_after(struct dlnode *prev, struct dlnode *node)
{
	node->prev = prev;
	if ((node->next = prev->next) != NULL)
		node->next->prev = node;
	prev->next = node;
}

LELY_UTIL_LIST_INLINE void
dlnode_insert_before(struct dlnode *next, struct dlnode *node)
{
	node->next = next;
	if ((node->prev = next->prev) != NULL)
		node->prev->next = node;
	next->prev = node;
}

LELY_UTIL_LIST_INLINE void
dlnode_remove(struct dlnode *node)
{
	if (node->prev)
		node->prev->next = node->next;
	if (node->next)
		node->next->prev = node->prev;
}

#ifdef __cplusplus
}
#endif

#endif

