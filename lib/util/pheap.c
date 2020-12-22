/**@file
 * This file is part of the utilities library; it contains the implementation of
 * the pairing heap.
 *
 * @see lely/util/pheap.h
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

#include "util.h"
#define LELY_UTIL_PHEAP_INLINE extern inline
#include <lely/util/pheap.h>

#include <assert.h>

static struct pnode *pnode_merge(
		struct pnode *n1, struct pnode *n2, pheap_cmp_t *cmp);
static struct pnode *pnode_merge_pairs(struct pnode *node, pheap_cmp_t *cmp);

void
pheap_insert(struct pheap *heap, struct pnode *node)
{
	assert(heap);
	assert(heap->cmp);
	assert(node);

	if (!heap->root) {
		node->parent = NULL;
		node->next = NULL;
		node->child = NULL;
		heap->root = node;
	} else if (heap->cmp(node->key, heap->root->key) < 0) {
		node->parent = NULL;
		node->next = NULL;
		node->child = heap->root;
		heap->root->parent = node;
		heap->root = node;
	} else {
		node->parent = heap->root;
		node->next = heap->root->child;
		node->child = NULL;
		heap->root->child = node;
	}

	heap->num_nodes++;
}

void
pheap_remove(struct pheap *heap, struct pnode *node)
{
	assert(heap);
	assert(heap->cmp);
	assert(node);

	if (node->parent) {
		// Obtain the address of the pointer to this node in the parent
		// or previous sibling.
		struct pnode **pnode = &node->parent->child;
		while (*pnode != node)
			pnode = &(*pnode)->next;
		// Remove the node from the list.
		*pnode = node->next;
	} else {
		heap->root = NULL;
	}

	node->parent = NULL;
	node->next = NULL;
	struct pnode *child = node->child;
	node->child = NULL;

	if (child) {
		child = pnode_merge_pairs(child, heap->cmp);
		heap->root = pnode_merge(heap->root, child, heap->cmp);
		heap->root->parent = NULL;
	}

	heap->num_nodes--;
}

struct pnode *
pheap_find(const struct pheap *heap, const void *key)
{
	assert(heap);
	assert(heap->cmp);

	struct pnode *node = heap->root;
	struct pnode *parent = NULL;
	while (node) {
		int c = heap->cmp(key, node->key);
		if (!c) {
			return node;
		} else if (c > 0 && node->child) {
			parent = node;
			node = node->child;
		} else {
			node = node->next;
			while (!node && parent) {
				node = parent->next;
				parent = parent->parent;
			}
		}
	}
	return node;
}

int
pheap_contains(const struct pheap *heap, const struct pnode *node)
{
	assert(heap);

	while (node) {
		if (node == heap->root)
			return 1;
		node = node->parent;
	}
	return 0;
}

static struct pnode *
pnode_merge(struct pnode *n1, struct pnode *n2, pheap_cmp_t *cmp)
{
	assert(n2);

	if (!n1)
		return n2;

	assert(cmp);
	if (cmp(n1->key, n2->key) >= 0) {
		struct pnode *tmp = n1;
		n1 = n2;
		n2 = tmp;
	}

	n2->parent = n1;
	n2->next = n1->child;
	n1->child = n2;

	return n1;
}

static struct pnode *
pnode_merge_pairs(struct pnode *node, pheap_cmp_t *cmp)
{
	assert(node);

	while (node->next) {
		struct pnode *next = node->next->next;
		node->next->next = NULL;
		node = pnode_merge(node, node->next, cmp);
		node->next = next;
	}
	return node;
}
