/**@file
 * This header file is part of the utilities library; it contains the
 * <a href="https://en.wikipedia.org/wiki/Bidirectional_map">bidirectional
 * map</a> declarations.
 *
 * The bidirectional map is implemented as a pair of red-black trees, one for
 * lookups by key, the other for lookups by value. The implementation is generic
 * and can be used for any kind of key-value pair. Only (void) pointers to keys
 * and values are stored. Upon initialization of the map, the user is
 * responsible for providing suitable comparison functions (#cmp_t).
 *
 * @copyright 2016-2020 Lely Industries N.V.
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

#ifndef LELY_UTIL_BIMAP_H_
#define LELY_UTIL_BIMAP_H_

#include <lely/util/cmp.h>
#include <lely/util/rbtree.h>

#include <assert.h>

#ifndef LELY_UTIL_BIMAP_INLINE
#define LELY_UTIL_BIMAP_INLINE static inline
#endif

/**
 * A node in a bidirectional map. To associate a value with a node, embed the
 * node in a struct containing the value and use structof() to obtain the struct
 * from the node.
 *
 * @see bimap
 */
struct binode {
	/// The node used to lookup values by key.
	struct rbnode key;
	/// The node used to lookup keys by value.
	struct rbnode value;
};

/// A bidirectional map.
struct bimap {
	/// The red-black tree used to store the keys.
	struct rbtree keys;
	/// The red-black tree used to store the values.
	struct rbtree values;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes a node in a bidirectional map.
 *
 * @param node  a pointer to the node to be initialized.
 * @param key   a pointer to the key of the node. The key MUST NOT be modified
 *              while the node is part of a map.
 * @param value a pointer to the value of the node. The value MUST NOT be
 *              modified while the node is part of a map.
 */
LELY_UTIL_BIMAP_INLINE void binode_init(
		struct binode *node, const void *key, const void *value);

/**
 * Returns a pointer to the previous (in-order) node by key in a bidirectional
 * map with respect to <b>node</b>. This is, at worst, an O(log(n)) operation.
 *
 * @see binode_next_by_key(), binode_prev_by_value()
 */
LELY_UTIL_BIMAP_INLINE struct binode *binode_prev_by_key(
		const struct binode *node);

/**
 * Returns a pointer to the next (in-order) node by key in a bidirectional map
 * with respect to <b>node</b>. This is, at worst, an O(log(n)) operation.
 * However, visiting all nodes in order is an O(n) operation, and therefore, on
 * average, O(1) for each node.
 *
 * @see binode_prev_by_key(), binode_next_by_value()
 */
LELY_UTIL_BIMAP_INLINE struct binode *binode_next_by_key(
		const struct binode *node);

/**
 * Returns a pointer to the previous (in-order) node by value in a bidirectional
 * map with respect to <b>node</b>. This is, at worst, an O(log(n)) operation.
 *
 * @see binode_next_by_value(), binode_prev_by_key()
 */
LELY_UTIL_BIMAP_INLINE struct binode *binode_prev_by_value(
		const struct binode *node);

/**
 * Returns a pointer to the next (in-order) node by value in a bidirectional map
 * with respect to <b>node</b>. This is, at worst, an O(log(n)) operation.
 * However, visiting all nodes in order is an O(n) operation, and therefore, on
 * average, O(1) for each node.
 *
 * @see binode_prev_by_value(), binode_next_by_key()
 */
LELY_UTIL_BIMAP_INLINE struct binode *binode_next_by_value(
		const struct binode *node);

/**
 * Iterates over each node in a bidirectional map in ascending order by key. It
 * is safe to remove the current node during the iteration.
 *
 * @param first a pointer to the first node.
 * @param node  the name of the pointer to the nodes. This variable is declared
 *              in the scope of the loop.
 *
 * @see binode_next_by_key()
 */
#ifdef __COUNTER__
#define binode_foreach_by_key(first, node) \
	_binode_foreach_by_key(first, node, __COUNTER__)
#else
#define binode_foreach_by_key(first, node) \
	_binode_foreach_by_key(first, node, __LINE__)
#endif
#define _binode_foreach_by_key(first, node, n) \
	__binode_foreach_by_key(first, node, n)
// clang-format off
#define __binode_foreach_by_key(first, node, n) \
	for (struct binode *node = (first), \
			*__binode_next_by_key_##n = (node) \
			? binode_next_by_key(node) : NULL; \
			(node); (node) = __binode_next_by_key_##n, \
			__binode_next_by_key_##n = (node) \
			? binode_next_by_key(node) : NULL)
// clang-format on

/**
 * Iterates over each node in a bidirectional map in ascending order by value.
 * It is safe to remove the current node during the iteration.
 *
 * @param first a pointer to the first node.
 * @param node  the name of the pointer to the nodes. This variable is declared
 *              in the scope of the loop.
 *
 * @see binode_next_by_value()
 */
#ifdef __COUNTER__
#define binode_foreach_by_value(first, node) \
	_binode_foreach_by_value(first, node, __COUNTER__)
#else
#define binode_foreach_by_value(first, node) \
	_binode_foreach_by_value(first, node, __LINE__)
#endif
#define _binode_foreach_by_value(first, node, n) \
	__binode_foreach_by_value(first, node, n)
// clang-format off
#define __binode_foreach_by_value(first, node, n) \
	for (struct binode *node = (first), \
			*__binode_next_by_value_##n = (node) \
			? binode_next_by_value(node) : NULL; \
			(node); (node) = __binode_next_by_value_##n, \
			__binode_next_by_value_##n = (node) \
			? binode_next_by_value(node) : NULL)
// clang-format on

/**
 * Initializes a bidirectional map.
 *
 * @param map       a pointer to the map to be initialized.
 * @param key_cmp   a pointer to the function used to compare two keys.
 * @param value_cmp a pointer to the function used to compare two values.
 */
LELY_UTIL_BIMAP_INLINE void bimap_init(
		struct bimap *map, cmp_t *key_cmp, cmp_t *value_cmp);

/// Returns 1 if the bidirectional map is empty, and 0 if not.
LELY_UTIL_BIMAP_INLINE int bimap_empty(const struct bimap *map);

/**
 * Returns the size (in number of nodes) of a bidirectional map. This is an O(1)
 * operation.
 */
LELY_UTIL_BIMAP_INLINE size_t bimap_size(const struct bimap *map);

/**
 * Inserts a node into a bidirectional map. This is an O(log(n)) operation. This
 * function does not check whether a node with the same key or value already
 * exists, or whether the node is already part of another map.
 *
 * @see bimap_remove(), bimap_find_by_key(), bimap_find_by_value()
 */
LELY_UTIL_BIMAP_INLINE void bimap_insert(
		struct bimap *map, struct binode *node);

/**
 * Removes a node from a bidirectional map. This is an O(log(n)) operation.
 *
 * @see bimap_insert()
 */
LELY_UTIL_BIMAP_INLINE void bimap_remove(
		struct bimap *map, struct binode *node);

/**
 * Finds a node by key in a bidirectional map. This is an O(log(n)) operation.
 *
 * @returns a pointer to the node if found, or NULL if not.
 *
 * @see bimap_insert(), bimap_find_by_value()
 */
LELY_UTIL_BIMAP_INLINE struct binode *bimap_find_by_key(
		const struct bimap *map, const void *key);

/**
 * Finds a node by value in a bidirectional map. This is an O(log(n)) operation.
 *
 * @returns a pointer to the node if found, or NULL if not.
 *
 * @see bimap_insert(), bimap_find_by_key()
 */
LELY_UTIL_BIMAP_INLINE struct binode *bimap_find_by_value(
		const struct bimap *map, const void *value);

/**
 * Returns a pointer to the first (leftmost) node by key in a bidirectional map.
 * This is an O(log(n)) operation.
 *
 * @see bimap_last_by_key(), bimap_first_by_value()
 */
LELY_UTIL_BIMAP_INLINE struct binode *bimap_first_by_key(
		const struct bimap *map);

/**
 * Returns a pointer to the last (rightmost) node by key in a bidirectional map.
 * This is an O(log(n)) operation.
 *
 * @see bimap_first_by_key(), bimap_last_by_value()
 */
LELY_UTIL_BIMAP_INLINE struct binode *bimap_last_by_key(
		const struct bimap *map);

/**
 * Returns a pointer to the first (leftmost) node by value in a bidirectional
 * map. This is an O(log(n)) operation.
 *
 * @see bimap_last_by_value(), bimap_first_by_key()
 */
LELY_UTIL_BIMAP_INLINE struct binode *bimap_first_by_value(
		const struct bimap *map);

/**
 * Returns a pointer to the last (rightmost) node by value in a bidirectional
 * map. This is an O(log(n)) operation.
 *
 * @see bimap_first_by_value(), bimap_last_by_key()
 */
LELY_UTIL_BIMAP_INLINE struct binode *bimap_last_by_value(
		const struct bimap *map);

/**
 * Iterates over each node in a bidirectional map in ascending order by key.
 *
 * @see binode_foreach_by_key(), bimap_first_by_key()
 */
#define bimap_foreach_by_key(map, node) \
	binode_foreach_by_key (bimap_first_by_key(map), node)

/**
 * Iterates over each node in a bidirectional map in ascending order by value.
 *
 * @see binode_foreach_by_value(), bimap_first_by_value()
 */
#define bimap_foreach_by_value(map, node) \
	binode_foreach_by_value (bimap_first_by_value(map), node)

LELY_UTIL_BIMAP_INLINE void
binode_init(struct binode *node, const void *key, const void *value)
{
	assert(node);

	rbnode_init(&node->key, key);
	rbnode_init(&node->value, value);
}

LELY_UTIL_BIMAP_INLINE struct binode *
binode_prev_by_key(const struct binode *node)
{
	assert(node);

	struct rbnode *prev = rbnode_prev(&node->key);
	return prev ? structof(prev, struct binode, key) : NULL;
}

LELY_UTIL_BIMAP_INLINE struct binode *
binode_next_by_key(const struct binode *node)
{
	assert(node);

	struct rbnode *next = rbnode_next(&node->key);
	return next ? structof(next, struct binode, key) : NULL;
}

LELY_UTIL_BIMAP_INLINE struct binode *
binode_prev_by_value(const struct binode *node)
{
	assert(node);

	struct rbnode *prev = rbnode_prev(&node->value);
	return prev ? structof(prev, struct binode, value) : NULL;
}

LELY_UTIL_BIMAP_INLINE struct binode *
binode_next_by_value(const struct binode *node)
{
	assert(node);

	struct rbnode *next = rbnode_next(&node->value);
	return next ? structof(next, struct binode, value) : NULL;
}

LELY_UTIL_BIMAP_INLINE void
bimap_init(struct bimap *map, cmp_t *key_cmp, cmp_t *value_cmp)
{
	assert(map);
	assert(key_cmp);
	assert(value_cmp);

	rbtree_init(&map->keys, key_cmp);
	rbtree_init(&map->values, value_cmp);
}

LELY_UTIL_BIMAP_INLINE int
bimap_empty(const struct bimap *map)
{
	return !bimap_size(map);
}

LELY_UTIL_BIMAP_INLINE size_t
bimap_size(const struct bimap *map)
{
	assert(map);

	return rbtree_size(&map->keys);
}

LELY_UTIL_BIMAP_INLINE void
bimap_insert(struct bimap *map, struct binode *node)
{
	assert(map);
	assert(node);

	rbtree_insert(&map->keys, &node->key);
	rbtree_insert(&map->values, &node->value);
}

LELY_UTIL_BIMAP_INLINE void
bimap_remove(struct bimap *map, struct binode *node)
{
	assert(map);
	assert(node);

	rbtree_remove(&map->keys, &node->key);
	rbtree_remove(&map->values, &node->value);
}

LELY_UTIL_BIMAP_INLINE struct binode *
bimap_find_by_key(const struct bimap *map, const void *key)
{
	assert(map);
	assert(key);

	struct rbnode *node = rbtree_find(&map->keys, key);
	return node ? structof(node, struct binode, key) : NULL;
}

LELY_UTIL_BIMAP_INLINE struct binode *
bimap_find_by_value(const struct bimap *map, const void *value)
{
	assert(map);
	assert(value);

	struct rbnode *node = rbtree_find(&map->values, value);
	return node ? structof(node, struct binode, value) : NULL;
}

LELY_UTIL_BIMAP_INLINE struct binode *
bimap_first_by_key(const struct bimap *map)
{
	assert(map);

	struct rbnode *node = rbtree_first(&map->keys);
	return node ? structof(node, struct binode, key) : NULL;
}

LELY_UTIL_BIMAP_INLINE struct binode *
bimap_last_by_key(const struct bimap *map)
{
	assert(map);

	struct rbnode *node = rbtree_last(&map->keys);
	return node ? structof(node, struct binode, key) : NULL;
}

LELY_UTIL_BIMAP_INLINE struct binode *
bimap_first_by_value(const struct bimap *map)
{
	assert(map);

	struct rbnode *node = rbtree_first(&map->values);
	return node ? structof(node, struct binode, value) : NULL;
}

LELY_UTIL_BIMAP_INLINE struct binode *
bimap_last_by_value(const struct bimap *map)
{
	assert(map);

	struct rbnode *node = rbtree_last(&map->values);
	return node ? structof(node, struct binode, value) : NULL;
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_BIMAP_H_
