/**@file
 * This header file is part of the utilities library; it contains the
 * singly-<a href="https://en.wikipedia.org/wiki/Linked_list">linked list</a>
 * declarations.
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

#ifndef LELY_UTIL_SLLIST_H_
#define LELY_UTIL_SLLIST_H_

#include <lely/features.h>

#include <assert.h>
#include <stddef.h>

#ifndef LELY_UTIL_SLLIST_INLINE
#define LELY_UTIL_SLLIST_INLINE static inline
#endif

/**
 * A node in a singly-linked list. To associate a value with a node, embed the
 * node in a struct containing the value and use structof() to obtain the struct
 * from the node.
 */
struct slnode {
	/// A pointer to the next node in the list.
	struct slnode *next;
};

/// The static initializer for #slnode.
#define SLNODE_INIT \
	{ \
		NULL \
	}

/// A singly-linked list.
struct sllist {
	/// A pointer to the first node in the list.
	struct slnode *first;
	/// A pointer to the next field of the last node in the list.
	struct slnode **plast;
};

#ifdef __cplusplus
extern "C" {
#endif

/// Initializes a node in a singly-linked list.
LELY_UTIL_SLLIST_INLINE void slnode_init(struct slnode *node);

/**
 * Iterates in order over each node in a singly-linked list. It is safe to
 * remove the current node during the iteration.
 *
 * @param first a pointer to the first node in the list.
 * @param node  the name of the pointer to the nodes. This variable is declared
 *              in the scope of the loop.
 */
#ifdef __COUNTER__
#define slnode_foreach(first, node) slnode_foreach_(__COUNTER__, first, node)
#else
#define slnode_foreach(first, node) slnode_foreach_(__LINE__, first, node)
#endif
#define slnode_foreach_(n, first, node) slnode_foreach__(n, first, node)
// clang-format off
#define slnode_foreach__(n, first, node) \
	for (struct slnode *node = (first), \
			*_slnode_next_##n = (node) ? (node)->next : NULL; \
			(node); (node) = _slnode_next_##n, \
			_slnode_next_##n = (node) ? (node)->next : NULL)
// clang-format on

/// Initializes a singly-linked list.
LELY_UTIL_SLLIST_INLINE void sllist_init(struct sllist *list);

/**
 * Returns 1 if the singly-linked list is empty, and 0 if not. This is an O(1)
 * operation.
 */
LELY_UTIL_SLLIST_INLINE int sllist_empty(const struct sllist *list);

/**
 * Returns the size (in number of nodes) of a singly-linked list. This is an
 * O(n) operation.
 */
LELY_UTIL_SLLIST_INLINE size_t sllist_size(const struct sllist *list);

/**
 * Pushes a node to the front of a singly-linked list. This is an O(1)
 * operation.
 *
 * @see sllist_pop_front()
 */
LELY_UTIL_SLLIST_INLINE void sllist_push_front(
		struct sllist *list, struct slnode *node);

/**
 * Pushes a node to the back of a singly-linked list. This is an O(1) operation.
 *
 * @see sllist_pop_back()
 */
LELY_UTIL_SLLIST_INLINE void sllist_push_back(
		struct sllist *list, struct slnode *node);

/**
 * Pops a node from the front of a singly-linked list. This is an O(1)
 * operation.
 *
 * @see sllist_push_front()
 */
LELY_UTIL_SLLIST_INLINE struct slnode *sllist_pop_front(struct sllist *list);

/**
 * Pops a node from the back of a singly-linked list. This is an O(n) operation.
 *
 * @see sllist_push_back()
 */
struct slnode *sllist_pop_back(struct sllist *list);

/**
 * Removes a node from a singly-linked list. This is an O(n) operation.
 *
 * @returns <b>node</b> on success, or NULL if the node was not part of the
 * list.
 */
struct slnode *sllist_remove(struct sllist *list, struct slnode *node);

/**
 * Checks if a node is part of a singly-linked list.
 *
 * @returns 1 if the node was found in the list, and 0 if not.
 */
int sllist_contains(const struct sllist *list, const struct slnode *node);

/**
 * Appends the singly-linked list at <b>src</b> to the one at <b>dst</b>. After
 * the operation, the list at <b>src</b> is empty.
 *
 * @returns <b>dst</b>.
 */
LELY_UTIL_SLLIST_INLINE struct sllist *sllist_append(
		struct sllist *dst, struct sllist *src);

/**
 * Returns a pointer to the first node in a singly-linked list. This is an O(1)
 * operation.
 *
 * @see sllist_last()
 */
LELY_UTIL_SLLIST_INLINE struct slnode *sllist_first(const struct sllist *list);

/**
 * Returns a pointer to the last node in a singly-linked list. This is an O(1)
 * operation.
 *
 * @see sllist_first()
 */
struct slnode *sllist_last(const struct sllist *list);

/**
 * Iterates in order over each node in a singly-linked list. It is safe to
 * remove the current node during the iteration.
 *
 * @param list a pointer to a singly-linked list.
 * @param node the name of the pointer to the nodes. This variable is declared
 *             in the scope of the loop.
 */
#define sllist_foreach(list, node) slnode_foreach (sllist_first(list), node)

LELY_UTIL_SLLIST_INLINE void
slnode_init(struct slnode *node)
{
	assert(node);

	node->next = NULL;
}

LELY_UTIL_SLLIST_INLINE void
sllist_init(struct sllist *list)
{
	assert(list);

	*(list->plast = &list->first) = NULL;
}

LELY_UTIL_SLLIST_INLINE int
sllist_empty(const struct sllist *list)
{
	assert(list);

	return !list->first;
}

LELY_UTIL_SLLIST_INLINE size_t
sllist_size(const struct sllist *list)
{
	assert(list);

	size_t size = 0;
	sllist_foreach (list, node)
		size++;
	return size;
}

LELY_UTIL_SLLIST_INLINE void
sllist_push_front(struct sllist *list, struct slnode *node)
{
	assert(list);
	assert(node);

	if (!(node->next = list->first))
		list->plast = &node->next;
	list->first = node;
}

LELY_UTIL_SLLIST_INLINE void
sllist_push_back(struct sllist *list, struct slnode *node)
{
	assert(list);
	assert(node);

	*list->plast = node;
	list->plast = &(*list->plast)->next;
	*list->plast = NULL;
}

LELY_UTIL_SLLIST_INLINE struct slnode *
sllist_pop_front(struct sllist *list)
{
	assert(list);

	struct slnode *node = list->first;
	if (node) {
		if (!(list->first = node->next))
			list->plast = &list->first;
		node->next = NULL;
	}
	return node;
}

LELY_UTIL_SLLIST_INLINE struct sllist *
sllist_append(struct sllist *dst, struct sllist *src)
{
	assert(dst);
	assert(src);

	if (src->first) {
		*dst->plast = src->first;
		dst->plast = src->plast;
		sllist_init(src);
	}
	return dst;
}

LELY_UTIL_SLLIST_INLINE struct slnode *
sllist_first(const struct sllist *list)
{
	assert(list);

	return list->first;
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_SLLIST_H_
