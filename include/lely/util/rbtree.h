/**@file
 * This header file is part of the utilities library; it contains the
 * <a href="https://en.wikipedia.org/wiki/Red-black_tree">red-black tree</a>
 * declarations.
 *
 * A red-black tree is a type of self-balancing binary tree. This implementation
 * is based on chapters 12 and 13 in: \n
 * T. H. Cormen et al., <em>Introduction to Algorithms</em> (third edition), MIT
 * Press (2009).
 *
 * The red-black tree implemented here is generic and can be used for any kind
 * of key-value pair; only (void) pointers to keys are stored. Upon
 * initialization of the tree, the user is responsible for providing a suitable
 * comparison function (#rbtree_cmp_t).
 *
 * @copyright 2014-2020 Lely Industries N.V.
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

#ifndef LELY_UTIL_RBTREE_H_
#define LELY_UTIL_RBTREE_H_

#include <lely/features.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#ifndef LELY_UTIL_RBTREE_INLINE
#define LELY_UTIL_RBTREE_INLINE static inline
#endif

/**
 * A node in a red-black tree. To associate a value with a node, embed the node
 * in a struct containing the value and use structof() to obtain the struct from
 * the node.
 *
 * @see rbtree
 */
struct rbnode {
	/**
	 * A pointer to the key for this node. The key MUST be set before the
	 * node is inserted into a tree and MUST NOT be modified while the node
	 * is part of the tree.
	 */
	const void *key;
	/**
	 * A pointer to the parent node. The least significant bit contains the
	 * color of this node (0 = black, 1 = red).
	 */
	uintptr_t parent;
	/// A pointer to the left child node.
	struct rbnode *left;
	/// A pointer to the right child node.
	struct rbnode *right;
};

/// The static initializer for #rbnode.
#define RBNODE_INIT \
	{ \
		NULL, 0, NULL, NULL \
	}

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a comparison function suitable for use in a red-black tree.
 * <b>p1</b> and <b>p2</b> MUST be NULL or point to objects of the same type.
 *
 * @returns an integer greater than, equal to, or less than 0 if the object at
 * <b>p1</b> is greater than, equal to, or less than the object at <b>p2</b>.
 */
typedef int rbtree_cmp_t(const void *, const void *);

/// A red-black tree.
struct rbtree {
	/// A pointer to the function used to compare two keys.
	rbtree_cmp_t *cmp;
	/// A pointer to the root node of the tree.
	struct rbnode *root;
	/// The number of nodes stored in the tree.
	size_t num_nodes;
};

/// The static initializer for #rbtree.
#define RBTREE_INIT \
	{ \
		NULL, NULL, 0 \
	}

/**
 * Initializes a node in a red-black tree.
 *
 * @param node a pointer to the node to be initialized.
 * @param key  a pointer to the key for this node. The key MUST NOT be modified
 *             while the node is part of a tree.
 */
LELY_UTIL_RBTREE_INLINE void rbnode_init(struct rbnode *node, const void *key);

/**
 * Returns a pointer to the previous (in-order) node in a red-black tree with
 * respect to <b>node</b>. This is, at worst, an O(log(n)) operation.
 *
 * @see rbnode_next()
 */
struct rbnode *rbnode_prev(const struct rbnode *node);

/**
 * Returns a pointer to the next (in-order) node in a red-black tree with
 * respect to <b>node</b>. This is, at worst, an O(log(n)) operation. However,
 * visiting all nodes in order is an O(n) operation, and therefore, on average,
 * O(1) for each node.
 *
 * @see rbnode_prev()
 */
struct rbnode *rbnode_next(const struct rbnode *node);

/**
 * Iterates over each node in a red-black tree in ascending order. It is safe to
 * remove the current node during the iteration.
 *
 * @param first a pointer to the first node.
 * @param node  the name of the pointer to the nodes. This variable is declared
 *              in the scope of the loop.
 *
 * @see rbnode_next()
 */
#ifdef __COUNTER__
#define rbnode_foreach(first, node) rbnode_foreach_(__COUNTER__, first, node)
#else
#define rbnode_foreach(first, node) rbnode_foreach_(__LINE__, first, node)
#endif
#define rbnode_foreach_(n, first, node) rbnode_foreach__(n, first, node)
// clang-format off
#define rbnode_foreach__(n, first, node) \
	for (struct rbnode *node = (first), \
			*_rbnode_next_##n = (node) ? rbnode_next(node) : NULL; \
			(node); (node) = _rbnode_next_##n, \
			_rbnode_next_##n = (node) ? rbnode_next(node) : NULL)
// clang-format on

/**
 * Initializes a red-black tree.
 *
 * @param tree a pointer to the tree to be initialized.
 * @param cmp  a pointer to the function used to compare two keys.
 */
LELY_UTIL_RBTREE_INLINE void rbtree_init(
		struct rbtree *tree, rbtree_cmp_t *cmp);

/// Returns 1 if the red-black tree is empty, and 0 if not.
LELY_UTIL_RBTREE_INLINE int rbtree_empty(const struct rbtree *tree);

/**
 * Returns the size (in number of nodes) of a red-black tree. This is an O(1)
 * operation.
 */
LELY_UTIL_RBTREE_INLINE size_t rbtree_size(const struct rbtree *tree);

/**
 * Inserts a node into a red-black tree. This is an O(log(n)) operation. This
 * function does not check whether a node with the same key already exists, or
 * whether the node is already part of another tree.
 *
 * @see rbtree_remove(), rbtree_find()
 */
void rbtree_insert(struct rbtree *tree, struct rbnode *node);

/**
 * Removes a node from a red-black tree. This is an O(log(n)) operation.
 *
 * @see rbtree_insert()
 */
void rbtree_remove(struct rbtree *tree, struct rbnode *node);

/**
 * Checks if a node is part of a red-black tree.
 *
 * @returns 1 if the node was found in the tree, and 0 if not.
 */
int rbtree_contains(const struct rbtree *tree, const struct rbnode *node);

/**
 * Finds a node in a red-black tree. This is an O(log(n)) operation.
 *
 * @returns a pointer to the node if found, or NULL if not.
 *
 * @see rbtree_insert()
 */
struct rbnode *rbtree_find(const struct rbtree *tree, const void *key);

/**
 * Returns a pointer to the first (leftmost) node in a red-black tree. This is
 * an O(log(n)) operation.
 *
 * @see rbtree_last()
 */
struct rbnode *rbtree_first(const struct rbtree *tree);

/**
 * Returns a pointer to the last (rightmost) node in a red-black tree. This is
 * an O(log(n)) operation.
 *
 * @see rbtree_first()
 */
struct rbnode *rbtree_last(const struct rbtree *tree);

/**
 * Returns a pointer to the root node in a red-black tree. This is an O(1)
 * operation.
 */
LELY_UTIL_RBTREE_INLINE struct rbnode *rbtree_root(const struct rbtree *tree);

/**
 * Iterates over each node in a red-black tree in ascending order.
 *
 * @see rbnode_foreach(), rbtree_first()
 */
#define rbtree_foreach(tree, node) rbnode_foreach (rbtree_first(tree), node)

LELY_UTIL_RBTREE_INLINE void
rbnode_init(struct rbnode *node, const void *key)
{
	assert(node);

	node->key = key;
	node->parent = 0;
	node->left = NULL;
	node->right = NULL;
}

LELY_UTIL_RBTREE_INLINE void
rbtree_init(struct rbtree *tree, rbtree_cmp_t *cmp)
{
	assert(tree);
	assert(cmp);

	tree->cmp = cmp;
	tree->root = NULL;
	tree->num_nodes = 0;
}

LELY_UTIL_RBTREE_INLINE int
rbtree_empty(const struct rbtree *tree)
{
	return !rbtree_size(tree);
}

LELY_UTIL_RBTREE_INLINE size_t
rbtree_size(const struct rbtree *tree)
{
	assert(tree);

	return tree->num_nodes;
}

LELY_UTIL_RBTREE_INLINE struct rbnode *
rbtree_root(const struct rbtree *tree)
{
	assert(tree);

	return tree->root;
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_RBTREE_H_
