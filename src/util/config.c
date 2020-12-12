/**@file
 * This file is part of the utilities library; it contains the implementation of
 * the configuration functions.
 *
 * @see lely/util/config.h
 *
 * @copyright 2014-2019 Lely Industries N.V.
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

#if !LELY_NO_MALLOC

#include <lely/util/cmp.h>
#include <lely/util/config.h>
#include <lely/util/errnum.h>
#include <lely/util/rbtree.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/// A configuration struct.
struct __config {
	/// The tree containing the sections.
	struct rbtree tree;
};

/// A section in a configuration struct.
struct config_section {
	/// The node of this section in the tree of sections.
	struct rbnode node;
	/// The tree containing the entries.
	struct rbtree tree;
};

static struct rbnode *config_section_create(config_t *config, const char *name);
static void config_section_destroy(struct rbnode *node);

static const char *config_section_set(
		struct rbnode *node, const char *key, const char *value);

static void config_section_foreach(
		struct rbnode *node, config_foreach_func_t *func, void *data);

/// An entry in a configuration section.
struct config_entry {
	/// The node of this entry in the tree of entries.
	struct rbnode node;
	/// The value of the entry.
	char *value;
};

static struct rbnode *config_entry_create(struct config_section *section,
		const char *key, const char *value);
static void config_entry_destroy(struct rbnode *node);

void *
__config_alloc(void)
{
	void *ptr = malloc(sizeof(struct __config));
#if !LELY_NO_ERRNO
	if (ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
__config_free(void *ptr)
{
	free(ptr);
}

struct __config *
__config_init(struct __config *config, int flags)
{
	assert(config);

	rbtree_init(&config->tree,
			(flags & CONFIG_CASE) ? &str_case_cmp : &str_cmp);

	if (!config_section_create(config, ""))
		return NULL;

	return config;
}

void
__config_fini(struct __config *config)
{
	assert(config);

	rbtree_foreach (&config->tree, node) {
		rbtree_remove(&config->tree, node);
		config_section_destroy(node);
	}
}

config_t *
config_create(int flags)
{
	int errc = 0;

	config_t *config = __config_alloc();
	if (!config) {
		errc = get_errc();
		goto error_alloc_config;
	}

	if (!__config_init(config, flags)) {
		errc = get_errc();
		goto error_init_config;
	}

	return config;

error_init_config:
	__config_free(config);
error_alloc_config:
	set_errc(errc);
	return NULL;
}

void
config_destroy(config_t *config)
{
	if (config) {
		__config_fini(config);
		__config_free(config);
	}
}

size_t
config_get_sections(const config_t *config, size_t n, const char **sections)
{
	assert(config);

	if (!sections)
		n = 0;

	if (n) {
		struct rbnode *node = rbtree_first(&config->tree);
		for (size_t i = 0; node && i < n; node = rbnode_next(node), i++)
			sections[i] = node->key;
	}

	return rbtree_size(&config->tree);
}

size_t
config_get_keys(const config_t *config, const char *section, size_t n,
		const char **keys)
{
	assert(config);

	if (!section)
		section = "";

	struct rbnode *node = rbtree_find(&config->tree, section);
	if (!node)
		return 0;
	struct rbtree *tree =
			&structof(node, struct config_section, node)->tree;

	if (!keys)
		n = 0;

	if (n) {
		node = rbtree_first(tree);
		for (size_t i = 0; node && i < n; node = rbnode_next(node), i++)
			keys[i] = node->key;
	}

	return rbtree_size(tree);
}

const char *
config_get(const config_t *config, const char *section, const char *key)
{
	assert(config);

	if (!section)
		section = "";

	if (!key)
		return NULL;

	const struct rbtree *tree = &config->tree;
	struct rbnode *node;

	node = rbtree_find(tree, section);
	if (!node)
		return NULL;
	tree = &structof(node, struct config_section, node)->tree;

	node = rbtree_find(tree, key);
	if (!node)
		return NULL;
	return structof(node, struct config_entry, node)->value;
}

const char *
config_set(config_t *config, const char *section, const char *key,
		const char *value)
{
	assert(config);

	if (!section)
		section = "";

	if (!key)
		return NULL;

	struct rbnode *node = rbtree_find(&config->tree, section);
	// Only create a section if we are not removing an entry.
	if (!node && value)
		node = config_section_create(config, section);

	return node ? config_section_set(node, key, value) : NULL;
}

void
config_foreach(const config_t *config, config_foreach_func_t *func, void *data)
{
	assert(config);

	// Start with the root section.
	struct rbnode *node = rbtree_find(&config->tree, "");
	if (node)
		config_section_foreach(node, func, data);

	rbtree_foreach (&config->tree, node) {
		const char *section = node->key;
		// Skip the root section.
		if (!section || !*section)
			continue;
		config_section_foreach(node, func, data);
	}
}

static struct rbnode *
config_section_create(config_t *config, const char *name)
{
	assert(config);
	assert(name);

	int errc = 0;

	struct config_section *section = malloc(sizeof(*section));
	if (!section) {
#if !LELY_NO_ERRNO
		errc = errno2c(errno);
#endif
		goto error_alloc_section;
	}

	struct rbnode *node = &section->node;

	node->key = malloc(strlen(name) + 1);
	if (!node->key) {
#if !LELY_NO_ERRNO
		errc = errno2c(errno);
#endif
		goto error_alloc_key;
	}
	strcpy((char *)node->key, name);

	rbtree_init(&section->tree, config->tree.cmp);

	rbtree_insert(&config->tree, node);

	return node;

error_alloc_key:
	free(section);
error_alloc_section:
	set_errc(errc);
	return NULL;
}

static void
config_section_destroy(struct rbnode *node)
{
	assert(node);
	struct config_section *section =
			structof(node, struct config_section, node);

	free((char *)node->key);

	rbtree_foreach (&section->tree, node) {
		rbtree_remove(&section->tree, node);
		config_entry_destroy(node);
	}

	free(section);
}

static const char *
config_section_set(struct rbnode *node, const char *key, const char *value)
{
	assert(node);
	struct config_section *section =
			structof(node, struct config_section, node);

	// Remove the existing entry.
	node = rbtree_find(&section->tree, key);
	if (node) {
		rbtree_remove(&section->tree, node);
		config_entry_destroy(node);
	}
	if (!value)
		return NULL;

	node = config_entry_create(section, key, value);
	return node ? structof(node, struct config_entry, node)->value : NULL;
}

static void
config_section_foreach(
		struct rbnode *node, config_foreach_func_t *func, void *data)
{
	assert(node);
	struct config_section *section =
			structof(node, struct config_section, node);
	assert(func);

	rbtree_foreach (&section->tree, node) {
		struct config_entry *entry =
				structof(node, struct config_entry, node);
		func(section->node.key, entry->node.key, entry->value, data);
	}
}

static struct rbnode *
config_entry_create(struct config_section *section, const char *key,
		const char *value)
{
	assert(section);
	assert(key);

	int errc = 0;

	struct config_entry *entry = malloc(sizeof(*entry));
	if (!entry) {
#if !LELY_NO_ERRNO
		errc = errno2c(errno);
#endif
		goto error_alloc_entry;
	}

	struct rbnode *node = &entry->node;

	node->key = malloc(strlen(key) + 1);
	if (!node->key) {
#if !LELY_NO_ERRNO
		errc = errno2c(errno);
#endif
		goto error_alloc_key;
	}
	strcpy((char *)node->key, key);

	entry->value = malloc(strlen(value) + 1);
	if (!entry->value) {
#if !LELY_NO_ERRNO
		errc = errno2c(errno);
#endif
		goto error_alloc_value;
	}
	strcpy(entry->value, value);

	rbtree_insert(&section->tree, node);

	// cppcheck-suppress memleak symbolName=entry.value
	return node;

error_alloc_value:
	free((char *)node->key);
error_alloc_key:
	free(entry);
error_alloc_entry:
	set_errc(errc);
	return NULL;
}

static void
config_entry_destroy(struct rbnode *node)
{
	assert(node);
	struct config_entry *entry = structof(node, struct config_entry, node);

	free((char *)node->key);

	free(entry->value);
	free(entry);
}

#endif // !LELY_NO_MALLOC
