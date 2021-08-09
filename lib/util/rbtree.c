/**@file
 * This file is part of the utilities library; it contains the implementation of
 * the red-black tree.
 *
 * @see lely/util/rbtree.h
 *
 * @copyright 2014-2021 Lely Industries N.V.
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
#define LELY_UTIL_RBTREE_INLINE extern inline
#include <lely/util/rbtree.h>

#include <assert.h>
#include <stdbool.h>

/// Returns the parent of <b>node</b>. <b>node</b> MUST NOT be NULL.
static inline struct rbnode *rbnode_get_parent(const struct rbnode *node);

/**
 * Sets the parent of <b>node</b> to <b>parent</b>. If <b>node</b> is NULL, this
 * function has no effect.
 */
static void rbnode_set_parent(struct rbnode *node, const struct rbnode *parent);

/// Returns <b>true</b> when the node's color is red, <b>false</b> otherwise (black or NULL).
static inline bool rbnode_is_red(const struct rbnode *node);

/**
 * Assigns color from <b>other</b> to the <b>node</b>.  If <b>other</b> is NULL
 * assumes color black.  If <b>node</b> is NULL, this function has no effect.
 */
static inline void rbnode_copy_color(
		struct rbnode *node, const struct rbnode *other);

/// Marks the node as red. If <b>node</b> is NULL, this function has no effect.
static inline void rbnode_mark_red(struct rbnode *node);

/// Marks the node as black. If <b>node</b> is NULL, this function has no effect.
static inline void rbnode_mark_black(struct rbnode *node);

/// Returns the leftmost descendant of <b>node</b>.
static struct rbnode *rbnode_min(struct rbnode *node);

/// Returns the rightmost descendant of <b>node</b>.
static struct rbnode *rbnode_max(struct rbnode *node);

/**
 * Rotates <b>node</b> counter-clockwise (left), i.e., it is replaced in the
 * tree by its right child, while becoming that child's left child.
 */
static void rbtree_rol(struct rbtree *tree, struct rbnode *node);

/**
 * Rotates <b>node</b> clockwise (right), i.e., it is replaced in the tree by
 * its left child, while becoming that child's right child.
 */
static void rbtree_ror(struct rbtree *tree, struct rbnode *node);

/**
 * Replaces <b>old_node</b> by <b>new_node</b> in <b>tree</b> by updating its
 * parent.
 */
static void rbtree_replace(struct rbtree *tree, const struct rbnode *old_node,
		struct rbnode *new_node);

struct rbnode *
rbnode_prev(const struct rbnode *node)
{
	assert(node);

	// If the node has a left subtree, find its rightmost descendant.
	if (node->left)
		return rbnode_max(node->left);
	// Find the lowest ancestor that has the node in its right subtree.
	struct rbnode *parent = rbnode_get_parent(node);
	while (parent && node == parent->left) {
		node = parent;
		parent = rbnode_get_parent(node);
	}
	return parent;
}

struct rbnode *
rbnode_next(const struct rbnode *node)
{
	assert(node);

	// If the node has a right subtree, find its leftmost descendant.
	if (node->right)
		return rbnode_min(node->right);
	// Find the lowest ancestor that has the node in its left subtree.
	struct rbnode *parent = rbnode_get_parent(node);
	while (parent && node == parent->right) {
		node = parent;
		parent = rbnode_get_parent(node);
	}
	return parent;
}

void
rbtree_insert(struct rbtree *tree, struct rbnode *node)
{
	assert(tree);
	assert(tree->cmp);
	assert(node);

	// Find the parent of the new node.
	struct rbnode *parent = NULL;
	struct rbnode *next = tree->root;
	while (next) {
		parent = next;
		// clang-format off
		next = tree->cmp(node->key, next->key) < 0
				? next->left : next->right;
		// clang-format on
	}
	// Attach the node to its parent.
	if (!parent)
		tree->root = node;
	else if (tree->cmp(node->key, parent->key) < 0)
		parent->left = node;
	else
		parent->right = node;
	tree->num_nodes++;
	// Initialize the node.
	rbnode_set_parent(node, parent);
	rbnode_mark_red(node);
	node->left = NULL;
	node->right = NULL;
	// Fix violations of the red-black properties.
	while (rbnode_is_red(parent)) {
		struct rbnode *gparent = rbnode_get_parent(parent);
		if (parent == gparent->left) {
			if (rbnode_is_red(gparent->right)) {
				// Case 1: flip colors.
				rbnode_mark_red(gparent);
				rbnode_mark_black(parent);
				rbnode_mark_black(gparent->right);
				node = gparent;
			} else {
				if (node == parent->right) {
					// Case 2: left rotate at grandparent.
					node = parent;
					rbtree_rol(tree, node);
					parent = rbnode_get_parent(node);
					gparent = rbnode_get_parent(parent);
				}
				// Case 3: right rotate at grandparent.
				rbnode_mark_red(gparent);
				rbnode_mark_black(parent);
				rbtree_ror(tree, gparent);
			}
		} else {
			if (rbnode_is_red(gparent->left)) {
				// Case 1: flip colors.
				rbnode_mark_red(gparent);
				rbnode_mark_black(parent);
				rbnode_mark_black(gparent->left);
				node = gparent;
			} else {
				if (node == parent->left) {
					// Case 2: right rotate at grandparent.
					node = parent;
					rbtree_ror(tree, node);
					parent = rbnode_get_parent(node);
					gparent = rbnode_get_parent(parent);
				}
				// Case 3: left rotate at grandparent.
				rbnode_mark_red(gparent);
				rbnode_mark_black(parent);
				rbtree_rol(tree, gparent);
			}
		}
		parent = rbnode_get_parent(node);
	}
	rbnode_mark_black(tree->root);
}

void
rbtree_remove(struct rbtree *tree, struct rbnode *node)
{
	assert(tree);
	assert(node);

	bool red = rbnode_is_red(node);
	struct rbnode *parent = rbnode_get_parent(node);
	// Remove the node from the tree. After removal, node points to the
	// subtree in which we might have introduced red-black property
	// violations.
	if (!node->left) {
		// There is no left subtree, so we can replace the node by its
		// right subtree.
		rbtree_replace(tree, node, node->right);
		node = node->right;
	} else if (!node->right) {
		// There is no right subtree, so we can replace the node by its
		// left subtree.
		rbtree_replace(tree, node, node->left);
		node = node->left;
	} else {
		// There is both a left and a right subtree, so we replace the
		// node by its successor. First, find the successor and give it
		// the same color as the node to be removed.
		struct rbnode *next = rbnode_min(node->right);
		red = rbnode_is_red(next);
		rbnode_copy_color(next, node);
		struct rbnode *tmp = next->right;
		if (rbnode_get_parent(next) == node) {
			parent = next;
		} else {
			parent = rbnode_get_parent(next);
			// If the successor is not a child of the node to be
			// removed, we remove it from its parent.
			rbtree_replace(tree, next, next->right);
			next->right = node->right;
			rbnode_set_parent(next->right, next);
		}
		// Replace the node to be removed by its successor.
		rbtree_replace(tree, node, next);
		// The successor has no left subtree (by definition). Attach the
		// left subtree of the node to be removed to the successor.
		next->left = node->left;
		rbnode_set_parent(next->left, next);
		node = tmp;
	}
	tree->num_nodes--;
	// Fix violations of the red-black properties. This can only occur if
	// the removed node (or its successor) was black.
	if (red)
		return;
	while (node != tree->root && !rbnode_is_red(node)) {
		if (node == parent->left) {
			struct rbnode *tmp = parent->right;
			if (rbnode_is_red(tmp)) {
				// Case 1: left rotate at parent.
				rbnode_mark_red(parent);
				rbnode_mark_black(tmp);
				rbtree_rol(tree, parent);
				tmp = parent->right;
			}
			if (!rbnode_is_red(tmp->left)
					&& !rbnode_is_red(tmp->right)) {
				// Case 2: color flip at sibling.
				rbnode_mark_red(tmp);
				node = parent;
				parent = rbnode_get_parent(node);
			} else {
				if (!rbnode_is_red(tmp->right)) {
					// Case 3: right rotate at sibling.
					rbnode_mark_red(tmp);
					rbnode_mark_black(tmp->left);
					rbtree_ror(tree, tmp);
					tmp = parent->right;
				}
				// Case 4: left rotate at parent and color flip.
				rbnode_copy_color(tmp, parent);
				rbnode_mark_black(parent);
				rbnode_mark_black(tmp->right);
				rbtree_rol(tree, parent);
				node = tree->root;
			}
		} else {
			struct rbnode *tmp = parent->left;
			if (rbnode_is_red(tmp)) {
				// Case 1: right rotate at parent.
				rbnode_mark_red(parent);
				rbnode_mark_black(tmp);
				rbtree_ror(tree, parent);
				tmp = parent->left;
			}
			if (!rbnode_is_red(tmp->right)
					&& !rbnode_is_red(tmp->left)) {
				// Case 2: color flip at sibling.
				rbnode_mark_red(tmp);
				node = parent;
				parent = rbnode_get_parent(node);
			} else {
				if (!rbnode_is_red(tmp->left)) {
					// Case 3: left rotate at sibling.
					rbnode_mark_red(tmp);
					rbnode_mark_black(tmp->right);
					rbtree_rol(tree, tmp);
					tmp = parent->left;
				}
				// Case 4: right rotate at parent and color
				// flip.
				rbnode_copy_color(tmp, parent);
				rbnode_mark_black(parent);
				rbnode_mark_black(tmp->left);
				rbtree_ror(tree, parent);
				node = tree->root;
			}
		}
	}
	rbnode_mark_black(node);
}

struct rbnode *
rbtree_find(const struct rbtree *tree, const void *key)
{
	assert(tree);
	assert(tree->cmp);

	struct rbnode *node = tree->root;
	while (node) {
		int cmp = tree->cmp(key, node->key);
		if (!cmp)
			break;
		node = cmp < 0 ? node->left : node->right;
	}
	return node;
}

bool
rbtree_contains(const struct rbtree *tree, const struct rbnode *node)
{
	assert(tree);

	while (node) {
		if (node == tree->root)
			return true;
		node = rbnode_get_parent(node);
	}
	return false;
}

struct rbnode *
rbtree_first(const struct rbtree *tree)
{
	assert(tree);

	return tree->root ? rbnode_min(tree->root) : NULL;
}

struct rbnode *
rbtree_last(const struct rbtree *tree)
{
	assert(tree);

	return tree->root ? rbnode_max(tree->root) : NULL;
}

static inline struct rbnode *
rbnode_get_parent(const struct rbnode *node)
{
	assert(node);

	return (struct rbnode *)(node->parent & ~(uintptr_t)1);
}

static void
rbnode_set_parent(struct rbnode *node, const struct rbnode *parent)
{
	if (!node)
		return;

	node->parent = (uintptr_t)parent | (uintptr_t)rbnode_is_red(node);
}

static inline bool
rbnode_is_red(const struct rbnode *node)
{
	return node ? node->parent & (uintptr_t)1 : false;
}

static inline void
rbnode_mark_red(struct rbnode *node)
{
	assert(node);
	node->parent |= (uintptr_t)1;
}

static inline void
rbnode_mark_black(struct rbnode *node)
{
	if (node)
		node->parent &= ~(uintptr_t)1;
}

static inline void
rbnode_copy_color(struct rbnode *node, const struct rbnode *other)
{
	if (rbnode_is_red(other))
		rbnode_mark_red(node);
	else
		rbnode_mark_black(node);
}

static struct rbnode *
rbnode_min(struct rbnode *node)
{
	assert(node);

	while (node->left)
		node = node->left;
	return node;
}

static struct rbnode *
rbnode_max(struct rbnode *node)
{
	assert(node);

	while (node->right)
		node = node->right;
	return node;
}

static void
rbtree_rol(struct rbtree *tree, struct rbnode *node)
{
	assert(tree);
	assert(node);
	assert(node->right);

	struct rbnode *tmp = node->right;
	node->right = tmp->left;
	rbnode_set_parent(tmp->left, node);
	tmp->left = node;
	struct rbnode *parent = rbnode_get_parent(node);
	rbnode_set_parent(node, tmp);
	rbnode_set_parent(tmp, parent);
	if (!parent)
		tree->root = tmp;
	else if (node == parent->left)
		parent->left = tmp;
	else
		parent->right = tmp;
}

static void
rbtree_ror(struct rbtree *tree, struct rbnode *node)
{
	assert(tree);
	assert(node);
	assert(node->left);

	struct rbnode *tmp = node->left;
	node->left = tmp->right;
	rbnode_set_parent(tmp->right, node);
	tmp->right = node;
	struct rbnode *parent = rbnode_get_parent(node);
	rbnode_set_parent(node, tmp);
	rbnode_set_parent(tmp, parent);
	if (!parent)
		tree->root = tmp;
	else if (node == parent->right)
		parent->right = tmp;
	else
		parent->left = tmp;
}

static void
rbtree_replace(struct rbtree *tree, const struct rbnode *old_node,
		struct rbnode *new_node)
{
	struct rbnode *parent = rbnode_get_parent(old_node);
	if (!parent)
		tree->root = new_node;
	else if (old_node == parent->left)
		parent->left = new_node;
	else
		parent->right = new_node;
	rbnode_set_parent(new_node, parent);
}
