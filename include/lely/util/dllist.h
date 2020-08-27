/**@file
 * This header file is part of the utilities library; it contains the
 * <a href="https://en.wikipedia.org/wiki/Doubly_linked_list">doubly-linked
 * list</a> declarations.
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

#ifndef LELY_UTIL_DLLIST_H_
#define LELY_UTIL_DLLIST_H_

#include <lely/features.h>

#include <assert.h>
#include <stddef.h>

#ifndef LELY_UTIL_DLLIST_INLINE
#define LELY_UTIL_DLLIST_INLINE static inline
#endif

/**
 * A node in a doubly-linked list. To associate a value with a node, embed the
 * node in a struct containing the value and use structof() to obtain the struct
 * from the node.
 */
struct dlnode {
	/// A pointer to the previous node in the list.
	struct dlnode *prev;
	/// A pointer to the next node in the list.
	struct dlnode *next;
};

/// The static initializer for #dlnode.
#define DLNODE_INIT \
	{ \
		NULL, NULL \
	}

/// A doubly-linked list.
struct dllist {
	/// A pointer to the first node in the list.
	struct dlnode *first;
	/// A pointer to the last node in the list.
	struct dlnode *last;
};

/// The static initializer for #dllist.
#define DLLIST_INIT \
	{ \
		NULL, NULL \
	}

#ifdef __cplusplus
extern "C" {
#endif

/// Initializes a node in a doubly-linked list.
LELY_UTIL_DLLIST_INLINE void dlnode_init(struct dlnode *node);

/**
 * Inserts <b>node</b> after <b>prev</b>.
 *
 * @returns 1 if <b>prev</b> was the last node in the list and 0 if not.
 */
LELY_UTIL_DLLIST_INLINE int dlnode_insert_after(
		struct dlnode *prev, struct dlnode *node);

/**
 * Inserts <b>node</b> before <b>next</b>.
 *
 * @returns 1 if <b>next</b> was the first node in the list and 0 if not.
 */
LELY_UTIL_DLLIST_INLINE int dlnode_insert_before(
		struct dlnode *next, struct dlnode *node);

/**
 * Removes <b>node</b> from a doubly-list. Note that this function does _not_
 * reset the <b>prev</b> and <b>next</b> fields of the node.
 */
LELY_UTIL_DLLIST_INLINE void dlnode_remove(struct dlnode *node);

/**
 * Iterates in order over each node in a doubly-linked list. It is safe to
 * remove the current node during the iteration.
 *
 * @param first a pointer to the first node in the list.
 * @param node  the name of the pointer to the nodes. This variable is declared
 *              in the scope of the loop.
 */
#ifdef __COUNTER__
#define dlnode_foreach(first, node) dlnode_foreach_(__COUNTER__, first, node)
#else
#define dlnode_foreach(first, node) dlnode_foreach_(__LINE__, first, node)
#endif
#define dlnode_foreach_(n, first, node) dlnode_foreach__(n, first, node)
// clang-format off
#define dlnode_foreach__(n, first, node) \
	for (struct dlnode *node = (first), \
			*_dlnode_next_##n = (node) ? (node)->next : NULL; \
			(node); (node) = _dlnode_next_##n, \
			_dlnode_next_##n = (node) ? (node)->next : NULL)
// clang-format on

/// Initializes a doubly-linked list.
LELY_UTIL_DLLIST_INLINE void dllist_init(struct dllist *list);

/**
 * Returns 1 if the doubly-linked list is empty, and 0 if not. This is an O(1)
 * operation.
 */
LELY_UTIL_DLLIST_INLINE int dllist_empty(const struct dllist *list);

/**
 * Returns the size (in number of nodes) of a doubly-linked list. This is an
 * O(n) operation.
 */
LELY_UTIL_DLLIST_INLINE size_t dllist_size(const struct dllist *list);

/**
 * Pushes a node to the front of a doubly-linked list. This is an O(1)
 * operation.
 *
 * @see dllist_pop_front()
 */
LELY_UTIL_DLLIST_INLINE void dllist_push_front(
		struct dllist *list, struct dlnode *node);

/**
 * Pushes a node to the back of a doubly-linked list. This is an O(1) operation.
 *
 * @see dllist_pop_back()
 */
LELY_UTIL_DLLIST_INLINE void dllist_push_back(
		struct dllist *list, struct dlnode *node);

/**
 * Pops a node from the front of a doubly-linked list. This is an O(1)
 * operation.
 *
 * @see dllist_push_front()
 */
LELY_UTIL_DLLIST_INLINE struct dlnode *dllist_pop_front(struct dllist *list);

/**
 * Pops a node from the back of a doubly-linked list. This is an O(1) operation.
 *
 * @see dllist_push_back()
 */
LELY_UTIL_DLLIST_INLINE struct dlnode *dllist_pop_back(struct dllist *list);

/**
 * Inserts a node into a doubly-linked list. <b>prev</b> MUST be part of
 * <b>list</b>. This is an O(1) operation.
 *
 * @see dlnode_insert_after()
 */
LELY_UTIL_DLLIST_INLINE void dllist_insert_after(
		struct dllist *list, struct dlnode *prev, struct dlnode *node);

/**
 * Inserts a node into a doubly-linked list. <b>next</b> MUST be part of
 * <b>list</b>. This is an O(1) operation.
 *
 * @see dlnode_insert_before()
 */
LELY_UTIL_DLLIST_INLINE void dllist_insert_before(
		struct dllist *list, struct dlnode *next, struct dlnode *node);

/**
 * Removes a node from a doubly-linked list. <b>node</b> MUST be part of
 * <b>list</b>. This is an O(1) operation.
 *
 * @see dlnode_remove()
 */
LELY_UTIL_DLLIST_INLINE void dllist_remove(
		struct dllist *list, struct dlnode *node);

/**
 * Checks if a node is part of a doubly-linked list.
 *
 * @returns 1 if the node was found in the list, and 0 if not.
 */
int dllist_contains(const struct dllist *list, const struct dlnode *node);

/**
 * Appends the doubly-linked list at <b>src</b> to the one at <b>dst</b>. After
 * the operation, the list at <b>src</b> is empty.
 *
 * @returns <b>dst</b>.
 */
LELY_UTIL_DLLIST_INLINE struct dllist *dllist_append(
		struct dllist *dst, struct dllist *src);

/**
 * Returns a pointer to the first node in a doubly-linked list. This is an O(1)
 * operation.
 *
 * @see dllist_last()
 */
LELY_UTIL_DLLIST_INLINE struct dlnode *dllist_first(const struct dllist *list);

/**
 * Returns a pointer to the last node in a doubly-linked list. This is an O(1)
 * operation.
 *
 * @see dllist_first()
 */
LELY_UTIL_DLLIST_INLINE struct dlnode *dllist_last(const struct dllist *list);

/**
 * Iterates in order over each node in a doubly-linked list. It is safe to
 * remove the current node during the iteration.
 *
 * @param list a pointer to a doubly-linked list.
 * @param node the name of the pointer to the nodes. This variable is declared
 *             in the scope of the loop.
 */
#define dllist_foreach(list, node) dlnode_foreach (dllist_first(list), node)

LELY_UTIL_DLLIST_INLINE void
dlnode_init(struct dlnode *node)
{
	assert(node);

	node->prev = NULL;
	node->next = NULL;
}

LELY_UTIL_DLLIST_INLINE int
dlnode_insert_after(struct dlnode *prev, struct dlnode *node)
{
	assert(prev);
	assert(node);

	node->prev = prev;
	if ((node->next = prev->next) != NULL)
		node->next->prev = node;
	prev->next = node;
	return !node->next;
}

LELY_UTIL_DLLIST_INLINE int
dlnode_insert_before(struct dlnode *next, struct dlnode *node)
{
	assert(next);
	assert(node);

	node->next = next;
	if ((node->prev = next->prev) != NULL)
		node->prev->next = node;
	next->prev = node;
	return !node->prev;
}

LELY_UTIL_DLLIST_INLINE void
dlnode_remove(struct dlnode *node)
{
	assert(node);

	if (node->prev)
		node->prev->next = node->next;
	if (node->next)
		node->next->prev = node->prev;
}

LELY_UTIL_DLLIST_INLINE void
dllist_init(struct dllist *list)
{
	assert(list);

	list->first = NULL;
	list->last = NULL;
}

LELY_UTIL_DLLIST_INLINE int
dllist_empty(const struct dllist *list)
{
	assert(list);

	return !list->first;
}

LELY_UTIL_DLLIST_INLINE size_t
dllist_size(const struct dllist *list)
{
	assert(list);

	size_t size = 0;
	dllist_foreach (list, node)
		size++;
	return size;
}

LELY_UTIL_DLLIST_INLINE void
dllist_push_front(struct dllist *list, struct dlnode *node)
{
	assert(list);
	assert(node);

	node->prev = NULL;
	if ((node->next = list->first))
		node->next->prev = node;
	else
		list->last = node;
	list->first = node;
}

LELY_UTIL_DLLIST_INLINE void
dllist_push_back(struct dllist *list, struct dlnode *node)
{
	assert(list);
	assert(node);

	node->next = NULL;
	if ((node->prev = list->last))
		node->prev->next = node;
	else
		list->first = node;
	list->last = node;
}

LELY_UTIL_DLLIST_INLINE struct dlnode *
dllist_pop_front(struct dllist *list)
{
	assert(list);

	struct dlnode *node = list->first;
	if (list->first) {
		if ((list->first = list->first->next))
			list->first->prev = NULL;
		else
			list->last = NULL;
	}
	return node;
}

LELY_UTIL_DLLIST_INLINE struct dlnode *
dllist_pop_back(struct dllist *list)
{
	assert(list);

	struct dlnode *node = list->last;
	if (list->last) {
		if ((list->last = list->last->prev))
			list->last->next = NULL;
		else
			list->first = NULL;
	}
	return node;
}

LELY_UTIL_DLLIST_INLINE void
dllist_insert_after(
		struct dllist *list, struct dlnode *prev, struct dlnode *node)
{
	assert(list);

	if (dlnode_insert_after(prev, node))
		list->last = node;
}

LELY_UTIL_DLLIST_INLINE void
dllist_insert_before(
		struct dllist *list, struct dlnode *next, struct dlnode *node)
{
	assert(list);

	if (dlnode_insert_before(next, node))
		list->first = node;
}

LELY_UTIL_DLLIST_INLINE void
dllist_remove(struct dllist *list, struct dlnode *node)
{
	assert(list);
	assert(node);

	if (!node->prev)
		list->first = node->next;
	if (!node->next)
		list->last = node->prev;
	dlnode_remove(node);
}

LELY_UTIL_DLLIST_INLINE struct dllist *
dllist_append(struct dllist *dst, struct dllist *src)
{
	assert(dst);
	assert(src);

	if (src->first) {
		if (dst->first) {
			src->first->prev = dst->last;
			dst->last->next = src->first;
			dst->last = src->last;
		} else {
			dst->first = src->first;
			dst->last = src->last;
		}
		dllist_init(src);
	}
	return dst;
}

LELY_UTIL_DLLIST_INLINE struct dlnode *
dllist_first(const struct dllist *list)
{
	assert(list);

	return list->first;
}

LELY_UTIL_DLLIST_INLINE struct dlnode *
dllist_last(const struct dllist *list)
{
	assert(list);

	return list->last;
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_DLLIST_H_
