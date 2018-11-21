/**@file
 * This header file is part of the utilities library; it contains the
 * <a href="https://en.wikipedia.org/wiki/Hash_table">hash table</a>
 * declarations.
 *
 * The hash table implemented here is generic and can be used for any kind of
 * key-value pair; only (void) pointers to keys are stored. Upon initialization
 * of the hash table, the user is responsible for providing suitable comparison
 * and hash functions.
 *
 * @copyright 2017-2018 Lely Industries N.V.
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

#ifndef LELY_UTIL_HTAB_H_
#define LELY_UTIL_HTAB_H_

#include <lely/util/util.h>

#include <stddef.h>

#ifndef LELY_UTIL_HTAB_INLINE
#define LELY_UTIL_HTAB_INLINE inline
#endif

/**
 * A node in a hash table tree. To associate a value with a node, embed the node
 * in a struct containing the value and use structof() to obtain the struct from
 * the node.
 *
 * @see htab
 */
struct hnode {
	/**
	 * A pointer to the key of this node. The key MUST be set before the
	 * node is inserted into a table and MUST NOT be modified while the node
	 * is part of the table.
	 */
	const void *key;
	/**
	 * The hash of #key. This value MUST NOT be modified directly by the
	 * user. The hash is stored separately, since comparing hashes is
	 * generally much faster than comparing keys, which requires a callback
	 * through a function pointer.
	 */
	size_t hash;
	/// A pointer to the next node in the chain.
	struct hnode *next;
	/**
	 * The address of the #next field of the previous node in the slot
	 * chain or, if this is the first node, the address of the slot itself.
	 * Note that <b>pprev</b> can never be NULL for a node that is part of a
	 * slot.
	 */
	struct hnode **pprev;
};

/**
 * A hash table. Each slot in the table consists of a chain (doubly-linked list)
 * of nodes.
 */
struct htab {
	/// A pointer to the function used to compare two keys for equality.
	int (*eq)(const void *, const void *);
	/// A pointer to the function used to compute the hash of a key.
	size_t (*hash)(const void *);
	/// The number of slots.
	size_t num_slots;
	/**
	 * A pointer to an array #num_slots slots. Each slot contains a pointer
	 * to the first node in the chain (or NULL if the chain is empty).
	 */
	struct hnode **slots;
	/// The number of nodes stored in the hash table.
	size_t num_nodes;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes a node in a hash table.
 *
 * @param node a pointer to the node to be initialized.
 * @param key  a pointer to the key for this node. The key MUST NOT be modified
 *             while the node is part of a table.
 */
LELY_UTIL_HTAB_INLINE void hnode_init(struct hnode *node, const void *key);

/**
 * Inserts <b>node</b> into a chain at *<b>pprev</b>, which can point to the
 * previous node or to the chain itself.
 */
LELY_UTIL_HTAB_INLINE void hnode_insert(
		struct hnode **pprev, struct hnode *node);

/// Removes <b>node</b> from a chain.
LELY_UTIL_HTAB_INLINE void hnode_remove(struct hnode *node);

/**
 * Iterates over each node in a slot in a hash table. The order of the nodes is
 * undefined. It is safe to remove the current node during the iteration.
 *
 * @param slot a pointer to the first node in a slot.
 * @param node the name of the pointer to the nodes. This variable is declared
 *             in the scope of the loop.
 */
#ifdef __COUNTER__
#define hnode_foreach(slot, node) hnode_foreach_(__COUNTER__, slot, node)
#else
#define hnode_foreach(slot, node) hnode_foreach_(__LINE__, slot, node)
#endif
#define hnode_foreach_(n, slot, node) hnode_foreach__(n, slot, node)
// clang-format off
#define hnode_foreach__(n, slot, node) \
	for (struct hnode *(node) = (slot), \
			*_hnode_next_##n = (node) ? (node)->next : NULL; \
			(node); (node) = _hnode_next_##n, \
			_hnode_next_##n = (node) ? (node)->next : NULL)
// clang-format on

/**
 * Initializes a hash table and allocates the slot array.
 *
 * @param tab       a pointer to the table to be initialized.
 * @param eq        a pointer to the function used to compare two keys for
 *                  equality. This function SHOULD return a non-zero value if
 *                  the keys are equal, and 0 if not.
 * @param hash      a pointer to the function used to compute the hash of a key.
 * @param num_slots the number of slots.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see htab_fini()
 */
int htab_init(struct htab *tab, int (*eq)(const void *, const void *),
		size_t (*hash)(const void *), size_t num_slots);

/// Finalizes a hash table and frees the slot array. @see htab_init()
void htab_fini(struct htab *tab);

/// Returns 1 if the hash table is empty, and 0 if not.
LELY_UTIL_HTAB_INLINE int htab_empty(const struct htab *tab);

/**
 * Returns the size (in number of nodes) of a hash table . This is an O(1)
 * operation.
 */
LELY_UTIL_HTAB_INLINE size_t htab_size(const struct htab *tab);

/**
 * Resizes a hash table. This is an O(n) operation. This function will rehash
 * all node keys and redistribute the nodes over the slots; it MUST therefore
 * NOT be invoked while iterating over the nodes with #htab_foreach().
 *
 * @param tab       a pointer to a hash table.
 * @param num_slots the new number of slots.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int htab_resize(struct htab *tab, size_t num_slots);

/**
 * Inserts a node into a hash table. This is an O(1) operation. This function
 * does not check whether a node with the same key already exists, or whether
 * the node is already part of another hash table.
 *
 * @see htab_remove(), htab_find()
 */
void htab_insert(struct htab *tab, struct hnode *node);

/**
 * Removes a node from a hash table. This is an O(1) operation.
 *
 * @see htab_insert()
 */
void htab_remove(struct htab *tab, struct hnode *node);

/**
 * Finds and returns a node in a hash table. This is an O(1) operation. If
 * multiple nodes in the table have the same key, the first node in the slot
 * chain with this key is returned. Unless the table has been resized with
 * htab_resize(), this is typically the most recently inserted node.
 *
 * @returns a pointer to the node if found, or NULL if not.
 *
 * @see htab_insert()
 */
struct hnode *htab_find(const struct htab *tab, const void *key);

/**
 * Iterates over each node in a hash table. The order of the nodes is undefined.
 * It is safe to remove the current node during the iteration.
 *
 * @param tab  a pointer to a hash table.
 * @param node the name of the pointer to the nodes. This variable is declared
 *             in the scope of the loop.
 *
 * @see hnode_foreach()
 */
#ifdef __COUNTER__
#define htab_foreach(tab, node) htab_foreach_(__COUNTER__, tab, node)
#else
#define htab_foreach(tab, node) htab_foreach_(__LINE__, tab, node)
#endif
#define htab_foreach_(n, tab, node) htab_foreach__(n, tab, node)
// clang-format off
#define htab_foreach__(n, tab, node) \
	for (size_t _htab_slot_##n = 0; _htab_slot_##n < (tab)->num_slots; \
			_htab_slot_##n++) \
		hnode_foreach__(n, (tab)->slots[_htab_slot_##n], node)
// clang-format on

LELY_UTIL_HTAB_INLINE void
hnode_init(struct hnode *node, const void *key)
{
	node->key = key;
	node->hash = 0;
	node->next = NULL;
	node->pprev = NULL;
}

LELY_UTIL_HTAB_INLINE void
hnode_insert(struct hnode **pprev, struct hnode *node)
{
	if ((node->next = *pprev))
		node->next->pprev = &node->next;
	node->pprev = pprev;
	*node->pprev = node;
}

LELY_UTIL_HTAB_INLINE void
hnode_remove(struct hnode *node)
{
	if ((*node->pprev = node->next))
		node->next->pprev = node->pprev;
}

LELY_UTIL_HTAB_INLINE int
htab_empty(const struct htab *tab)
{
	return !htab_size(tab);
}

LELY_UTIL_HTAB_INLINE size_t
htab_size(const struct htab *tab)
{
	return tab->num_nodes;
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_HTAB_H_
