/**@file
 * This file is part of the utilities library; it contains the implementation of
 * the factory pattern.
 *
 * @see lely/util/factory.h
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

#include "util.h"
#ifndef LELY_NO_THREADS
#include <lely/libc/threads.h>
#endif
#include <lely/util/cmp.h>
#include <lely/util/errnum.h>
#include <lely/util/factory.h>
#include <lely/util/rbtree.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/// An entry in #factory.
struct factory_entry {
	/// The node in #factory.
	struct rbnode node;
	/// A pointer to the constructor function.
	factory_ctor_t *ctor;
	/// A pointer to the destructor function.
	factory_dtor_t *dtor;
	/// A pointer to the type name.
	char name[1];
};

#ifndef LELY_NO_THREADS
/// The flag ensuring factory_init() is only called once.
static once_flag factory_once = ONCE_FLAG_INIT;
/// The initialization function for #factory_mtx.
static void __cdecl factory_init(void);
/// The finalization function for #factory_mtx.
static void __cdecl factory_fini(void);
/// The mutex protecting #factory.
static mtx_t factory_mtx;
#endif
/// The tree containing all registered constructor and destructor functions.
static struct rbtree factory = { &str_cmp, NULL, 0 };

LELY_UTIL_EXPORT void *
factory_ctor_create(factory_ctor_t *ctor, ...)
{
	va_list ap;
	va_start(ap, ctor);
	void *ptr = factory_ctor_vcreate(ctor, ap);
	va_end(ap);
	return ptr;
}

LELY_UTIL_EXPORT void *
factory_ctor_vcreate(factory_ctor_t *ctor, va_list ap)
{
	assert(ctor);

	return ctor(ap);
}

LELY_UTIL_EXPORT void
factory_dtor_destroy(factory_dtor_t *dtor, void *ptr)
{
	if (dtor)
		dtor(ptr);
}

LELY_UTIL_EXPORT int
factory_insert(const char *name, factory_ctor_t *ctor, factory_dtor_t *dtor)
{
	assert(name);
	assert(ctor);

#ifndef LELY_NO_THREADS
	call_once(&factory_once, &factory_init);
	mtx_lock(&factory_mtx);
#endif
	struct rbnode *node = rbtree_find(&factory, name);
	if (node) {
		struct factory_entry *entry =
				structof(node, struct factory_entry, node);
		entry->ctor = ctor;
		entry->dtor = dtor;
	} else {
		size_t size = offsetof(struct factory_entry, name)
				+ strlen(name) + 1;
		struct factory_entry *entry = malloc(size);
		if (__unlikely(!entry)) {
#ifndef LELY_NO_THREADS
			mtx_unlock(&factory_mtx);
#endif
			set_errno(errno);
			return -1;
		}

		rbnode_init(&entry->node, entry->name);
		entry->ctor = ctor;
		entry->dtor = dtor;
		strcpy(entry->name, name);

		rbtree_insert(&factory, &entry->node);
	}
#ifndef LELY_NO_THREADS
	mtx_unlock(&factory_mtx);
#endif

	return 0;
}

LELY_UTIL_EXPORT void
factory_remove(const char *name)
{
	assert(name);

#ifndef LELY_NO_THREADS
	call_once(&factory_once, &factory_init);
	mtx_lock(&factory_mtx);
#endif
	struct rbnode *node = rbtree_find(&factory, name);
	if (node)
		rbtree_remove(&factory, node);
#ifndef LELY_NO_THREADS
	mtx_unlock(&factory_mtx);
#endif

	if (node)
		free(structof(node, struct factory_entry, node));
}

LELY_UTIL_EXPORT factory_ctor_t *
factory_find_ctor(const char *name)
{
	factory_ctor_t *ctor = NULL;

#ifndef LELY_NO_THREADS
	call_once(&factory_once, &factory_init);
	mtx_lock(&factory_mtx);
#endif
	struct rbnode *node = rbtree_find(&factory, name);
	if (node)
		ctor = structof(node, struct factory_entry, node)->ctor;
#ifndef LELY_NO_THREADS
	mtx_unlock(&factory_mtx);
#endif

	return ctor;
}

LELY_UTIL_EXPORT factory_dtor_t *
factory_find_dtor(const char *name)
{
	factory_dtor_t *dtor = NULL;

#ifndef LELY_NO_THREADS
	call_once(&factory_once, &factory_init);
	mtx_lock(&factory_mtx);
#endif
	struct rbnode *node = rbtree_find(&factory, name);
	if (node)
		dtor = structof(node, struct factory_entry, node)->dtor;
#ifndef LELY_NO_THREADS
	mtx_unlock(&factory_mtx);
#endif

	return dtor;
}

#ifndef LELY_NO_THREADS

static void __cdecl
factory_init(void)
{
	mtx_init(&factory_mtx, mtx_plain);

	atexit(&factory_fini);
}

static void __cdecl
factory_fini(void)
{
	mtx_destroy(&factory_mtx);
}

#endif

