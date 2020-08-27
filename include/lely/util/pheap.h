/**@file
 * This header file is part of the utilities library; it contains the
 * <a href="https://en.wikipedia.org/wiki/Pairing_heap">pairing heap</a>
 * declarations.
 *
 * A pairing heap is a half-sorted tree structure suitable for a priority queue.
 * Compared to a self-balancing binary tree, insertion and retrieval of the
 * first (minimum) element is faster (O(1) vs. O(log(n))), while finding an
 * arbitrary element is slower (O(n) vs. O(log(n))).
 *
 * The pairing heap implemented here is generic and can be used for any kind of
 * key-value pair; only (void) pointers to keys are stored. Upon initialization
 * of the heap, the user is responsible for providing a suitable comparison
 * function (#pheap_cmp_t).
 *
 * @copyright 2015-2020 Lely Industries N.V.
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

#ifndef LELY_UTIL_PHEAP_H_
#define LELY_UTIL_PHEAP_H_

#include <lely/features.h>

#include <assert.h>
#include <stddef.h>

#ifndef LELY_UTIL_PHEAP_INLINE
#define LELY_UTIL_PHEAP_INLINE static inline
#endif

/**
 * A node in a pairing heap. To associate a value with a node, embed the node in
 * a struct containing the value and use structof() to obtain the struct from
 * the node.
 *
 * @see pheap
 */
struct pnode {
	/**
	 * A pointer to the key of this node. The key MUST be set before the
	 * node is inserted into a heap and MUST NOT be modified while the node
	 * is part of the heap.
	 */
	const void *key;
	/// A pointer to the parent node.
	struct pnode *parent;
	/// A pointer to the next sibling node.
	struct pnode *next;
	/// A pointer to the first child node.
	struct pnode *child;
};

/// The static initializer for #pnode.
#define PNODE_INIT \
	{ \
		NULL, NULL, NULL, NULL \
	}

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a comparison function suitable for use in a paring heap.
 * <b>p1</b> and <b>p2</b> MUST be NULL or point to objects of the same type.
 *
 * @returns an integer greater than, equal to, or less than 0 if the object at
 * <b>p1</b> is greater than, equal to, or less than the object at <b>p2</b>.
 */
typedef int pheap_cmp_t(const void *p1, const void *p2);

/// A pairing heap.
struct pheap {
	/// A pointer to the function used to compare two keys.
	pheap_cmp_t *cmp;
	/// A pointer to the root node of the heap.
	struct pnode *root;
	/// The number of nodes stored in the heap.
	size_t num_nodes;
};

/// The static initializer for #pheap.
#define PHEAP_INIT \
	{ \
		NULL, NULL, 0 \
	}

/**
 * Initializes a node in a paring heap.
 *
 * @param node a pointer to the node to be initialized.
 * @param key  a pointer to the key for this node. The key MUST NOT be modified
 *             while the node is part of a heap.
 */
LELY_UTIL_PHEAP_INLINE void pnode_init(struct pnode *node, const void *key);

/// Returns a pointer to the next node (in unspecified order) in a pairing heap.
LELY_UTIL_PHEAP_INLINE struct pnode *pnode_next(const struct pnode *node);

/**
 * Iterates over each node in a pairing heap in unspecified order. It is safe to
 * remove the current node during the iteration. However, since pheap_remove()
 * may change the order of the nodes, it is not guaranteed that all nodes will
 * be visited.
 *
 * @param first a pointer to the first node.
 * @param node  the name of the pointer to the nodes. This variable is declared
 *              in the scope of the loop.
 *
 * @see pnode_next()
 */
#ifdef __COUNTER__
#define pnode_foreach(first, node) pnode_foreach_(__COUNTER__, first, node)
#else
#define pnode_foreach(first, node) pnode_foreach_(__LINE__, first, node)
#endif
#define pnode_foreach_(n, first, node) pnode_foreach__(n, first, node)
// clang-format off
#define pnode_foreach__(n, first, node) \
	for (struct pnode *node = (first), \
			*_pnode_next_##n = (node) ? pnode_next(node) : NULL; \
			(node); (node) = _pnode_next_##n, \
			_pnode_next_##n = (node) ? pnode_next(node) : NULL)
// clang-format on

/**
 * Initializes a pairing heap.
 *
 * @param heap a pointer to the heap to be initialized.
 * @param cmp  a pointer to the function used to compare two keys.
 */
LELY_UTIL_PHEAP_INLINE void pheap_init(struct pheap *heap, pheap_cmp_t *cmp);

/// Returns 1 if the pairing heap is empty, and 0 if not.
LELY_UTIL_PHEAP_INLINE int pheap_empty(const struct pheap *heap);

/**
 * Returns the size (in number of nodes) of a pairing heap. This is an O(1)
 * operation.
 */
LELY_UTIL_PHEAP_INLINE size_t pheap_size(const struct pheap *heap);

/**
 * Inserts a node into a pairing heap. This is an O(1) operation. This function
 * does not check whether a node with the same key already exists, or whether
 * the node is already part of another heap. It is the responsibility of the
 * user to ensure that the node is not part of the heap before calling this
 * function.
 *
 * @see pheap_remove(), pheap_find()
 */
void pheap_insert(struct pheap *heap, struct pnode *node);

/**
 * Removes a node from a pairing heap. This is an (amortized) O(log(n))
 * operation. It is the responsibility of the user to ensure
 * that the node is part of the heap before calling this function.
 *
 * @see pheap_insert(), pheap_contains()
 */
void pheap_remove(struct pheap *heap, struct pnode *node);

/**
 * Finds a node in a pairing heap. This is an O(n) operation.
 *
 * @returns a pointer to the node if found, or NULL if not.
 *
 * @see pheap_insert()
 */
struct pnode *pheap_find(const struct pheap *heap, const void *key);

/**
 * Checks if a node is part of a pairing heap.
 *
 * @returns 1 if the node was found in the heap, and 0 if not.
 */
int pheap_contains(const struct pheap *heap, const struct pnode *node);

/**
 * Returns a pointer to the first (minimum) node in a pairing heap. This is an
 * O(1) operation.
 */
LELY_UTIL_PHEAP_INLINE struct pnode *pheap_first(const struct pheap *heap);

/**
 * Iterates over each node in a pairing heap in unspecified order.
 *
 * @see pnode_foreach(), pheap_first()
 */
#define pheap_foreach(heap, node) pnode_foreach (pheap_first(heap), node)

LELY_UTIL_PHEAP_INLINE void
pnode_init(struct pnode *node, const void *key)
{
	assert(node);

	node->key = key;
}

LELY_UTIL_PHEAP_INLINE struct pnode *
pnode_next(const struct pnode *node)
{
	assert(node);

	if (node->child)
		return node->child;
	do {
		if (node->next)
			return node->next;
	} while ((node = node->parent));
	return NULL;
}

LELY_UTIL_PHEAP_INLINE void
pheap_init(struct pheap *heap, pheap_cmp_t *cmp)
{
	assert(heap);
	assert(cmp);

	heap->cmp = cmp;
	heap->root = NULL;
	heap->num_nodes = 0;
}

LELY_UTIL_PHEAP_INLINE int
pheap_empty(const struct pheap *heap)
{
	return !pheap_size(heap);
}

LELY_UTIL_PHEAP_INLINE size_t
pheap_size(const struct pheap *heap)
{
	assert(heap);

	return heap->num_nodes;
}

LELY_UTIL_PHEAP_INLINE struct pnode *
pheap_first(const struct pheap *heap)
{
	assert(heap);

	return heap->root;
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_PHEAP_H_
