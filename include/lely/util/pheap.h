/*!\file
 * This header file is part of the utilities library; it contains the
 * <a href="http://en.wikipedia.org/wiki/Pairing_heap">pairing heap</a>
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
 * function (#cmp_t).
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

#ifndef LELY_UTIL_PHEAP_H
#define LELY_UTIL_PHEAP_H

#include <lely/util/cmp.h>

/*!
 * A node in a pairing heap. To associate a value with a node, embed the node in
 * a struct containing the value and use structof() to obtain the struct from
 * the node.
 *
 * \see pheap_in
 */
struct pnode {
	/*!
	 * A pointer to the key of this node. The key MUST be set before the
	 * node is inserted into a heap and MUST NOT be modified while the node
	 * is part of the heap.
	 */
	const void *key;
	//! A pointer to the parent node.
	struct pnode *parent;
	//! A pointer to the next sibling node.
	struct pnode *next;
	//! A pointer to the first child node.
	struct pnode *child;
};

//! A pairing heap.
struct pheap {
	//! A pointer to the function used to compare two keys.
	cmp_t *cmp;
	//! A pointer to the root node of the heap.
	struct pnode *root;
	//! The number of nodes stored in the heap.
	size_t num_nodes;
};

#ifdef __cplusplus
extern "C" {
#endif

//! Returns a pointer to the next node (in unspecified order) in a pairing heap.
static inline struct pnode *pnode_next(const struct pnode *node);

/*!
 * Initializes a pairing heap.
 *
 * \param heap a pointer to the heap to be initialized.
 * \param cmp  a pointer to the function used to compare two keys.
 */
static inline void pheap_init(struct pheap *heap, cmp_t *cmp);

//! Returns 1 if the pairing heap is empty, and 0 if not.
static inline int pheap_empty(const struct pheap *heap);

/*!
 * Returns the size (in number of nodes) of a pairing heap. This is an O(1)
 * operation.
 */
static inline size_t pheap_size(const struct pheap *heap);

/*!
 * Inserts a node into a pairing heap. This is an O(1) operation. This function
 * does not check whether a node with the same key already exists, or whether
 * the node is already part of another heap.
 *
 * \see pheap_remove(), pheap_find()
 */
LELY_UTIL_EXTERN void pheap_insert(struct pheap *heap, struct pnode *node);

/*!
 * Removes a node from a pairing heap. This is an (amortized) O(log(n))
 * operation.
 *
 * \see pheap_insert()
 */
LELY_UTIL_EXTERN void pheap_remove(struct pheap *heap, struct pnode *node);

/*!
 * Finds a node in a pairing heap. This is an O(n) operation.
 *
 * \returns a pointer to the node if found, or NULL if not.
 *
 * \see pheap_insert()
 */
LELY_UTIL_EXTERN struct pnode *pheap_find(const struct pheap *heap,
		const void *key);

/*!
 * Returns a pointer to the first (minimum) node in a pairing heap. This is an
 * O(1) operation.
 */
static inline struct pnode *pheap_first(const struct pheap *heap);

/*!
 * Iterates over each node in a pairing heap in unspecified order.
 *
 * \param heap a pointer to a pairing heap.
 * \param node the name of the pointer to the nodes. This variable is declared
 *             in the scope of the loop.
 *
 * \see pheap_first(), pnode_next()
 */
#define pheap_foreach(heap, node) \
	for (struct pnode *(node) = pheap_first(heap); (node); \
			(node) = pnode_next(node))

static inline struct pnode *
pnode_next(const struct pnode *node)
{
	if (node->child)
		return node->child;
	do {
		if (node->next)
			return node->next;
	} while ((node = node->parent));
	return NULL;
}

static inline void
pheap_init(struct pheap *heap, cmp_t cmp)
{
	heap->cmp = cmp;
	heap->root = NULL;
	heap->num_nodes = 0;
}

static inline int
pheap_empty(const struct pheap *heap)
{
	return !pheap_size(heap);
}

static inline size_t
pheap_size(const struct pheap *heap)
{
	return heap->num_nodes;
}

static inline struct pnode *
pheap_first(const struct pheap *heap)
{
	return heap->root;
}

#ifdef __cplusplus
}
#endif

#endif

