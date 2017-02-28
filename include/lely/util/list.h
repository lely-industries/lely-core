/*!\file
 * This header file is part of the utilities library; it contains the
 * <a href="https://en.wikipedia.org/wiki/Linked_list">linked list</a>
 * declarations.
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

#ifndef LELY_UTIL_LIST_H
#define LELY_UTIL_LIST_H

#include <lely/util/util.h>

#include <stddef.h>

#ifndef LELY_UTIL_LIST_INLINE
#define LELY_UTIL_LIST_INLINE	inline
#endif

/*!
 * A node in a doubly-linked list. To associate a value with a node, embed the
 * node in a struct containing the value and use structof() to obtain the struct
 * from the node.
 */
struct dlnode {
	//! A pointer to the previous node in the list.
	struct dlnode *prev;
	//! A pointer to the next node in the list.
	struct dlnode *next;
};

//! A doubly-linked list.
struct dllist {
	//! A pointer to the first node in the list.
	struct dlnode *first;
	//! A pointer to the last node in the list.
	struct dlnode *last;
};

#ifdef __cplusplus
extern "C" {
#endif

//! Initializes a node in a doubly-linked list.
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
 * Iterates in order over each node in a doubly-linked list. It is safe to
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

//!Initializes a doubly-linked list.
LELY_UTIL_LIST_INLINE void dllist_init(struct dllist *list);

//! Returns 1 if the doubly-linked list is empty, and 0 if not.
LELY_UTIL_LIST_INLINE int dllist_empty(const struct dllist *list);

/*!
 * Returns the size (in number of nodes) of a doubly-linked list. This is an
 * O(n) operation.
 */
LELY_UTIL_LIST_INLINE size_t dllist_size(const struct dllist *list);

//! Pushes a node to the front of a doubly-linked list. \see dllist_pop_front()
LELY_UTIL_LIST_INLINE void dllist_push_front(struct dllist *list,
		struct dlnode *node);

//! Pushes a node to the back of a doubly-linked list. \see dllist_pop_back()
LELY_UTIL_LIST_INLINE void dllist_push_back(struct dllist *list,
		struct dlnode *node);

//! Pops a node from the front of a doubly-linked list. \see dllist_push_front()
LELY_UTIL_LIST_INLINE struct dlnode *dllist_pop_front(struct dllist *list);

//! Pops a node from the back of a doubly-linked list. \see dllist_push_back()
LELY_UTIL_LIST_INLINE struct dlnode *dllist_pop_back(struct dllist *list);

/*!
 * Inserts a node into a doubly-linked list. This is an O(1) operation.
 *
 * \see dlnode_insert_after()
 */
LELY_UTIL_LIST_INLINE void dllist_insert_after(struct dllist *list,
		struct dlnode *prev, struct dlnode *node);

/*!
 * Inserts a node into a doubly-linked list. This is an O(1) operation.
 *
 * \see dlnode_insert_before()
 */
LELY_UTIL_LIST_INLINE void dllist_insert_before(struct dllist *list,
		struct dlnode *next, struct dlnode *node);

/*!
 * Removes a node from a doubly-linked list. This is an O(1) operation.
 *
 * \see dlnode_remove()
 */
LELY_UTIL_LIST_INLINE void dllist_remove(struct dllist *list,
		struct dlnode *node);

/*!
 * Returns a pointer to the first node in a doubly-linked list. This is an O(1)
 * operation.
 *
 * \see dllist_last()
 */
LELY_UTIL_LIST_INLINE struct dlnode *dllist_first(const struct dllist *list);

/*!
 * Returns a pointer to the last node in a doubly-linked list. This is an O(1)
 * operation.
 *
 * \see dllist_first()
 */
LELY_UTIL_LIST_INLINE struct dlnode *dllist_last(const struct dllist *list);

/*!
 * Iterates in order over each node in a doubly-linked list. It is safe to
 * remove the current node during the iteration.
 *
 * \param list a pointer to a double-linked list.
 * \param node the name of the pointer to the nodes. This variable is declared
 *             in the scope of the loop.
 */
#define dllist_foreach(list, node) \
	dlnode_foreach(dllist_first(list), node)

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

LELY_UTIL_LIST_INLINE void
dllist_init(struct dllist *list)
{
	list->first = NULL;
	list->last = NULL;
}

LELY_UTIL_LIST_INLINE int
dllist_empty(const struct dllist *list)
{
	return !list->first;
}

LELY_UTIL_LIST_INLINE size_t
dllist_size(const struct dllist *list)
{
	size_t size = 0;
	dllist_foreach(list, node)
		size++;
	return size;
}

LELY_UTIL_LIST_INLINE void
dllist_push_front(struct dllist *list, struct dlnode *node)
{
	node->prev = NULL;
	if ((node->next = list->first))
		node->next->prev = node;
	else
		list->last = node;
	list->first = node;
}

LELY_UTIL_LIST_INLINE void
dllist_push_back(struct dllist *list, struct dlnode *node)
{
	node->next = NULL;
	if ((node->prev = list->last))
		node->prev->next = node;
	else
		list->first = node;
	list->last = node;
}

LELY_UTIL_LIST_INLINE struct dlnode *
dllist_pop_front(struct dllist *list)
{
	struct dlnode *node = list->first;
	if (list->first) {
		if ((list->first = list->first->next))
			list->first->prev = NULL;
		else
			list->last = NULL;
	}
	return node;
}

LELY_UTIL_LIST_INLINE struct dlnode *
dllist_pop_back(struct dllist *list)
{
	struct dlnode *node = list->last;
	if (list->last) {
		if ((list->last = list->last->prev))
			list->last->next = NULL;
		else
			list->first = NULL;
	}
	return node;
}

LELY_UTIL_LIST_INLINE void
dllist_insert_after(struct dllist *list, struct dlnode *prev,
		struct dlnode *node)
{
	dlnode_insert_after(prev, node);
	if (!node->next)
		list->last = node;
}

LELY_UTIL_LIST_INLINE void
dllist_insert_before(struct dllist *list, struct dlnode *next,
		struct dlnode *node)
{
	dlnode_insert_before(next, node);
	if (!node->prev)
		list->first = node;
}

LELY_UTIL_LIST_INLINE void
dllist_remove(struct dllist *list, struct dlnode *node)
{
	if (!node->prev)
		list->first = node->next;
	if (!node->next)
		list->last = node->prev;
	dlnode_remove(node);
}

LELY_UTIL_LIST_INLINE struct dlnode *
dllist_first(const struct dllist *list)
{
	return list->first;
}

LELY_UTIL_LIST_INLINE struct dlnode *
dllist_last(const struct dllist *list)
{
	return list->last;
}

#ifdef __cplusplus
}
#endif

#endif

