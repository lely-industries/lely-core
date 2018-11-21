/**@file
 * This header file is part of the utilities library; it contains the
 * <a href="https://en.wikipedia.org/wiki/Linked_list">linked list</a>
 * declarations.
 *
 * @copyright 2013-2018 Lely Industries N.V.
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

#ifndef LELY_UTIL_LIST_H_
#define LELY_UTIL_LIST_H_

#include <lely/util/util.h>

#include <stddef.h>

#ifndef LELY_UTIL_LIST_INLINE
#define LELY_UTIL_LIST_INLINE static inline
#endif

/**
 * A node in a singly-linked list. To associate a value with a node, embed the
 * node in a struct containing the value and use `structof()` to obtain the
 * struct from the node.
 */
struct slnode {
	/// A pointer to the next node in the list.
	struct slnode *next;
};

/// A singly-linked list.
struct sllist {
	/// A pointer to the first node in the list.
	struct slnode *first;
	/**
	 * The address of the <b>next</b> field in the last node in the list. If
	 * the list is empty (#first is `NULL`), this equals `&first`.
	 */
	struct slnode **plast;
};

/**
 * A node in a doubly-linked list. To associate a value with a node, embed the
 * node in a struct containing the value and use `structof()` to obtain the
 * struct from the node.
 */
struct dlnode {
	/// A pointer to the previous node in the list.
	struct dlnode *prev;
	/// A pointer to the next node in the list.
	struct dlnode *next;
};

/// A doubly-linked list.
struct dllist {
	/// A pointer to the first node in the list.
	struct dlnode *first;
	/// A pointer to the last node in the list.
	struct dlnode *last;
};

#ifdef __cplusplus
extern "C" {
#endif

/// Initializes a node in a singly-linked list.
LELY_UTIL_LIST_INLINE void slnode_init(struct slnode *node);

/**
 * Inserts <b>node</b> after <b>prev</b>.
 *
 * @returns 1 if <b>prev</b> was the last node in the list and 0 if not.
 */
LELY_UTIL_LIST_INLINE int slnode_insert_after(
		struct slnode *prev, struct slnode *node);

/// Inserts <b>node</b> before <b>next</b>.
LELY_UTIL_LIST_INLINE void slnode_insert_before(
		struct slnode **pnext, struct slnode *node);

/**
 * Removes <b>node</b> from a singly-list. Note that this function does _not_
 * reset the <b>next</b> field of the node.
 *
 * @returns 1 if *<b>pnode</b> was the last node in the list and 0 if not.
 */
LELY_UTIL_LIST_INLINE int slnode_remove(struct slnode **pnode);

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
	for (struct slnode *(node) = (first), \
			*_slnode_next_##n = (node) ? (node)->next : NULL; \
			(node); (node) = _slnode_next_##n, \
			_slnode_next_##n = (node) ? (node)->next : NULL)
// clang-format on

/**
 * Returns 1 if the doubly-linked list is empty, and 0 if not. This is an O(1)
 * operation.
 */
LELY_UTIL_LIST_INLINE int dllist_empty(const struct dllist *list);

/**
 * Returns the size (in number of nodes) of a singly-linked list. This is an
 * O(n) operation.
 */
LELY_UTIL_LIST_INLINE size_t sllist_size(const struct sllist *list);

/**
 * Pushes a node to the front of a singly-linked list. This is an O(1)
 * operation.
 *
 * @see sllist_pop_front()
 */
LELY_UTIL_LIST_INLINE void sllist_push_front(
		struct sllist *list, struct slnode *node);

/**
 * Pushes a node to the back of a singly-linked list. This is an O(1) operation.
 *
 * @see sllist_pop_back()
 */
LELY_UTIL_LIST_INLINE void sllist_push_back(
		struct sllist *list, struct slnode *node);

/**
 * Pops a node from the front of a singly-linked list. This is an O(1)
 * operation.
 *
 * @see sllist_push_front()
 */
LELY_UTIL_LIST_INLINE struct slnode *sllist_pop_front(struct sllist *list);

/**
 * Pops a node from the back of a singly-linked list. This is an O(n) operation.
 *
 * @see sllist_push_back()
 */
LELY_UTIL_LIST_INLINE struct slnode *sllist_pop_back(struct sllist *list);

/**
 * Inserts a node into a singly-linked list. *<b>prev</b> MUST be part of
 * *<b>list</b>. This is an O(1) operation.
 *
 * @see slnode_insert_after()
 */
LELY_UTIL_LIST_INLINE void sllist_insert_after(
		struct sllist *list, struct slnode *prev, struct slnode *node);

/**
 * Inserts a node into a singly-linked list. <b>next</b> MUST be part of
 * <b>list</b>. This is an O(n) operation.
 *
 * @see slnode_insert_before()
 */
LELY_UTIL_LIST_INLINE void sllist_insert_before(
		struct sllist *list, struct slnode *next, struct slnode *node);

/**
 * Removes a node from a singly-linked list. <b>node</b> MUST be part of
 * <b>list</b>. This is an O(n) operation.
 *
 * @see slnode_remove()
 */
LELY_UTIL_LIST_INLINE void sllist_remove(
		struct sllist *list, struct slnode *node);

/**
 * Returns a pointer to the first node in a singly-linked list. This is an O(1)
 * operation.
 */
LELY_UTIL_LIST_INLINE struct slnode *sllist_first(const struct sllist *list);

/**
 * Returns a pointer to the last node in a singly-linked list. This is an O(1)
 * operation.
 */
LELY_UTIL_LIST_INLINE struct slnode *sllist_last(const struct sllist *list);

/**
 * Iterates in order over each node in a singly-linked list. It is safe to
 * remove the current node during the iteration.
 *
 * @param list a pointer to a singly-linked list.
 * @param node the name of the pointer to the nodes. This variable is declared
 *             in the scope of the loop.
 */
#define sllist_foreach(list, node) slnode_foreach (sllist_first(list), node)

/// Initializes a node in a doubly-linked list.
LELY_UTIL_LIST_INLINE void dlnode_init(struct dlnode *node);

/**
 * Inserts <b>node</b> after <b>prev</b>.
 *
 * @returns 1 if <b>prev</b> was the last node in the list and 0 if not.
 */
LELY_UTIL_LIST_INLINE int dlnode_insert_after(
		struct dlnode *prev, struct dlnode *node);

/**
 * Inserts <b>node</b> before <b>next</b>.
 *
 * @returns 1 if <b>next</b> was the first node in the list and 0 if not.
 */
LELY_UTIL_LIST_INLINE int dlnode_insert_before(
		struct dlnode *next, struct dlnode *node);

/**
 * Removes <b>node</b> from a doubly-list. Note that this function does _not_
 * reset the <b>prev</b> and <b>next</b> fields of the node.
 */
LELY_UTIL_LIST_INLINE void dlnode_remove(struct dlnode *node);

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
	for (struct dlnode *(node) = (first), \
			*_dlnode_next_##n = (node) ? (node)->next : NULL; \
			(node); (node) = _dlnode_next_##n, \
			_dlnode_next_##n = (node) ? (node)->next : NULL)
// clang-format on

/// Initializes a doubly-linked list.
LELY_UTIL_LIST_INLINE void dllist_init(struct dllist *list);

/**
 * Returns 1 if the doubly-linked list is empty, and 0 if not. This is an O(1)
 * operation.
 */
LELY_UTIL_LIST_INLINE int dllist_empty(const struct dllist *list);

/**
 * Returns the size (in number of nodes) of a doubly-linked list. This is an
 * O(n) operation.
 */
LELY_UTIL_LIST_INLINE size_t dllist_size(const struct dllist *list);

/**
 * Pushes a node to the front of a doubly-linked list. This is an O(1)
 * operation.
 *
 * @see dllist_pop_front()
 */
LELY_UTIL_LIST_INLINE void dllist_push_front(
		struct dllist *list, struct dlnode *node);

/**
 * Pushes a node to the back of a doubly-linked list. This is an O(1) operation.
 *
 * @see dllist_pop_back()
 */
LELY_UTIL_LIST_INLINE void dllist_push_back(
		struct dllist *list, struct dlnode *node);

/**
 * Pops a node from the front of a doubly-linked list. This is an O(1)
 * operation.
 *
 * @see dllist_push_front()
 */
LELY_UTIL_LIST_INLINE struct dlnode *dllist_pop_front(struct dllist *list);

/**
 * Pops a node from the back of a doubly-linked list. This is an O(1) operation.
 *
 * @see dllist_push_back()
 */
LELY_UTIL_LIST_INLINE struct dlnode *dllist_pop_back(struct dllist *list);

/**
 * Inserts a node into a doubly-linked list. *<b>prev</b> MUST be part of
 * *<b>list</b>. This is an O(1) operation.
 *
 * @see dlnode_insert_after()
 */
LELY_UTIL_LIST_INLINE void dllist_insert_after(
		struct dllist *list, struct dlnode *prev, struct dlnode *node);

/**
 * Inserts a node into a doubly-linked list. *<b>next</b> MUST be part of
 * *<b>list</b>. This is an O(1) operation.
 *
 * @see dlnode_insert_before()
 */
LELY_UTIL_LIST_INLINE void dllist_insert_before(
		struct dllist *list, struct dlnode *next, struct dlnode *node);

/**
 * Removes a node from a doubly-linked list. *<b>node</b> MUST be part of
 * *<b>list</b>. This is an O(1) operation.
 *
 * @see dlnode_remove()
 */
LELY_UTIL_LIST_INLINE void dllist_remove(
		struct dllist *list, struct dlnode *node);

/**
 * Returns a pointer to the first node in a doubly-linked list. This is an O(1)
 * operation.
 *
 * @see dllist_last()
 */
LELY_UTIL_LIST_INLINE struct dlnode *dllist_first(const struct dllist *list);

/**
 * Returns a pointer to the last node in a doubly-linked list. This is an O(1)
 * operation.
 *
 * @see dllist_first()
 */
LELY_UTIL_LIST_INLINE struct dlnode *dllist_last(const struct dllist *list);

/**
 * Iterates in order over each node in a doubly-linked list. It is safe to
 * remove the current node during the iteration.
 *
 * @param list a pointer to a double-linked list.
 * @param node the name of the pointer to the nodes. This variable is declared
 *             in the scope of the loop.
 */
#define dllist_foreach(list, node) dlnode_foreach (dllist_first(list), node)

inline void
slnode_init(struct slnode *node)
{
	node->next = NULL;
}

inline int
slnode_insert_after(struct slnode *prev, struct slnode *node)
{
	node->next = prev->next;
	prev->next = node;
	return !node->next;
}

inline void
slnode_insert_before(struct slnode **pnext, struct slnode *node)
{
	node->next = *pnext;
	*pnext = node;
}

inline int
slnode_remove(struct slnode **pnode)
{
	return !(*pnode = (*pnode)->next);
}

inline void
sllist_init(struct sllist *list)
{
	list->first = NULL;
	list->plast = &list->first;
}

inline int
sllist_empty(const struct sllist *list)
{
	return !list->first;
}

inline size_t
sllist_size(const struct sllist *list)
{
	size_t size = 0;
	sllist_foreach (list, pnode)
		size++;
	return size;
}

inline void
sllist_push_front(struct sllist *list, struct slnode *node)
{
	if (!(node->next = list->first))
		list->plast = &node->next;
	list->first = node;
}

inline void
sllist_push_back(struct sllist *list, struct slnode *node)
{
	node->next = NULL;
	*list->plast = node;
	list->plast = &(*list->plast)->next;
}

inline struct slnode *
sllist_pop_front(struct sllist *list)
{
	struct slnode *node = list->first;
	if (list->first && !(list->first = list->first->next))
		list->plast = &list->first;
	return node;
}

inline struct slnode *
sllist_pop_back(struct sllist *list)
{
	if (!*(list->plast = &list->first))
		return NULL;
	for (; (*list->plast)->next; list->plast = &(*list->plast)->next)
		;
	struct slnode *node = *list->plast;
	*list->plast = NULL;
	return node;
}

inline void
sllist_insert_after(
		struct sllist *list, struct slnode *prev, struct slnode *node)
{
	if (slnode_insert_after(prev, node))
		list->plast = &node->next;
}

inline void
sllist_insert_before(
		struct sllist *list, struct slnode *next, struct slnode *node)
{
	struct slnode **pnext;
	for (pnext = &list->first; *pnext != next; pnext = &(*pnext)->next)
		;
	slnode_insert_before(pnext, node);
}

inline void
sllist_remove(struct sllist *list, struct slnode *node)
{
	struct slnode **pnode;
	for (pnode = &list->first; *pnode != node; pnode = &(*pnode)->next)
		;
	if (slnode_remove(pnode))
		list->plast = pnode;
}

inline struct slnode *
sllist_first(const struct sllist *list)
{
	return list->first;
}

inline struct slnode *
sllist_last(const struct sllist *list)
{
	return list->first ? structof(list->plast, struct slnode, next) : NULL;
}

inline void
dlnode_init(struct dlnode *node)
{
	node->prev = NULL;
	node->next = NULL;
}

inline int
dlnode_insert_after(struct dlnode *prev, struct dlnode *node)
{
	node->prev = prev;
	if ((node->next = prev->next) != NULL)
		node->next->prev = node;
	prev->next = node;
	return !node->next;
}

inline int
dlnode_insert_before(struct dlnode *next, struct dlnode *node)
{
	node->next = next;
	if ((node->prev = next->prev) != NULL)
		node->prev->next = node;
	next->prev = node;
	return !node->prev;
}

inline void
dlnode_remove(struct dlnode *node)
{
	if (node->prev)
		node->prev->next = node->next;
	if (node->next)
		node->next->prev = node->prev;
}

inline void
dllist_init(struct dllist *list)
{
	list->first = NULL;
	list->last = NULL;
}

inline int
dllist_empty(const struct dllist *list)
{
	return !list->first;
}

inline size_t
dllist_size(const struct dllist *list)
{
	size_t size = 0;
	dllist_foreach (list, node)
		size++;
	return size;
}

inline void
dllist_push_front(struct dllist *list, struct dlnode *node)
{
	node->prev = NULL;
	if ((node->next = list->first))
		node->next->prev = node;
	else
		list->last = node;
	list->first = node;
}

inline void
dllist_push_back(struct dllist *list, struct dlnode *node)
{
	node->next = NULL;
	if ((node->prev = list->last))
		node->prev->next = node;
	else
		list->first = node;
	list->last = node;
}

inline struct dlnode *
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

inline struct dlnode *
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

inline void
dllist_insert_after(
		struct dllist *list, struct dlnode *prev, struct dlnode *node)
{
	if (dlnode_insert_after(prev, node))
		list->last = node;
}

inline void
dllist_insert_before(
		struct dllist *list, struct dlnode *next, struct dlnode *node)
{
	if (dlnode_insert_before(next, node))
		list->first = node;
}

inline void
dllist_remove(struct dllist *list, struct dlnode *node)
{
	if (!node->prev)
		list->first = node->next;
	if (!node->next)
		list->last = node->prev;
	dlnode_remove(node);
}

inline struct dlnode *
dllist_first(const struct dllist *list)
{
	return list->first;
}

inline struct dlnode *
dllist_last(const struct dllist *list)
{
	return list->last;
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_LIST_H_
