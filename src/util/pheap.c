/**@file
 * This file is part of the utilities library; it contains the implementation of
 * the pairing heap.
 *
 * @see lely/util/pheap.h
 *
 * @copyright 2015-2018 Lely Industries N.V.
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
static struct pnode *pnode_find(
		struct pnode *node, const void *key, pheap_cmp_t *cmp);

void
pheap_insert(struct pheap *heap, struct pnode *node)
{
	assert(heap);
	assert(heap->cmp);
	assert(node);

	if (!heap->root) {
		node->parent = NULL;
		node->child = NULL;
		node->next = NULL;
		heap->root = node;
	} else if (heap->cmp(node->key, heap->root->key) < 0) {
		node->parent = NULL;
		node->child = heap->root;
		node->next = NULL;
		heap->root->parent = node;
		heap->root = node;
	} else {
		node->parent = heap->root;
		node->child = NULL;
		node->next = heap->root->child;
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
		struct pnode **pchild = &node->parent->child;
		while (*pchild != node)
			pchild = &(*pchild)->next;
		*pchild = node->next;
		node->next = NULL;
	} else {
		heap->root = NULL;
	}

	node->child = pnode_merge_pairs(node->child, heap->cmp);

	heap->root = pnode_merge(heap->root, node->child, heap->cmp);
	if (heap->root)
		heap->root->parent = NULL;

	heap->num_nodes--;
}

struct pnode *
pheap_find(const struct pheap *heap, const void *key)
{
	assert(heap);

	return pnode_find(heap->root, key, heap->cmp);
}

static struct pnode *
pnode_merge(struct pnode *n1, struct pnode *n2, pheap_cmp_t *cmp)
{
	if (!n1)
		return n2;
	if (!n2)
		return n1;

	assert(cmp);
	if (cmp(n1->key, n2->key) < 0) {
		n2->parent = n1;
		n2->next = n1->child;
		n1->child = n2;
		return n1;
	} else {
		n1->parent = n2;
		n1->next = n2->child;
		n2->child = n1;
		return n2;
	}
}

static struct pnode *
pnode_merge_pairs(struct pnode *node, pheap_cmp_t *cmp)
{
	if (!node)
		return NULL;

	if (!node->next)
		return node;

	struct pnode *next = pnode_merge_pairs(node->next->next, cmp);
	node = pnode_merge(node, node->next, cmp);
	node = pnode_merge(node, next, cmp);
	node->next = NULL;

	return node;
}

static struct pnode *
pnode_find(struct pnode *node, const void *key, pheap_cmp_t *cmp)
{
	assert(cmp);

	for (; node; node = node->next) {
		int c = cmp(key, node->key);
		if (!c) {
			return node;
		} else if (c > 0) {
			struct pnode *child = pnode_find(node->child, key, cmp);
			if (child)
				return child;
		}
	}

	return NULL;
}
