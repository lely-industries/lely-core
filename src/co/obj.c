/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the object dictionary.
 *
 * @see lely/co/obj.h, src/obj.h
 *
 * @copyright 2020 Lely Industries N.V.
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

#include "co.h"
#include <lely/co/detail/obj.h>
#include <lely/co/dev.h>
#include <lely/co/sdo.h>
#include <lely/util/cmp.h>
#include <lely/util/errnum.h>

#include <assert.h>
#if !LELY_NO_MALLOC
#include <stdlib.h>
#endif
#include <string.h>

#if !LELY_NO_MALLOC

/**
 * Updates an object by allocating a new memory region containing the members
 * and moving the old values.
 */
static void co_obj_update(co_obj_t *obj);

/// Destroys all sub-objects.
static void co_obj_clear(co_obj_t *obj);

void *
__co_obj_alloc(void)
{
	void *ptr = malloc(sizeof(struct __co_obj));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
__co_obj_free(void *ptr)
{
	free(ptr);
}

#endif // !LELY_NO_MALLOC

struct __co_obj *
__co_obj_init(struct __co_obj *obj, co_unsigned16_t idx, void *val, size_t size)
{
	assert(obj);
#if !LELY_NO_MALLOC
	assert(val == NULL);
	assert(size == 0);
#endif

	rbnode_init(&obj->node, &obj->idx);
	obj->dev = NULL;
	obj->idx = idx;

	rbtree_init(&obj->tree, &uint8_cmp);

#if !LELY_NO_CO_OBJ_NAME
	obj->name = NULL;
#endif

	obj->code = CO_OBJECT_VAR;

	obj->val = val;
	obj->size = size;

	return obj;
}

void
__co_obj_fini(struct __co_obj *obj)
{
	assert(obj);

	if (obj->dev)
		co_dev_remove_obj(obj->dev, obj);

#if !LELY_NO_MALLOC
	co_obj_clear(obj);
#endif

#if !LELY_NO_MALLOC && !LELY_NO_CO_OBJ_NAME
	free(obj->name);
#endif
}

#if !LELY_NO_MALLOC

co_obj_t *
co_obj_create(co_unsigned16_t idx)
{
	trace("creating object %04X", idx);

	co_obj_t *obj = __co_obj_alloc();
	if (!obj)
		return NULL;

	return __co_obj_init(obj, idx, NULL, 0);
}

void
co_obj_destroy(co_obj_t *obj)
{
	if (obj) {
		trace("destroying object %04X", obj->idx);
		__co_obj_fini(obj);
		__co_obj_free(obj);
	}
}

#endif // !LELY_NO_MALLOC

co_obj_t *
co_obj_prev(const co_obj_t *obj)
{
	assert(obj);

	struct rbnode *node = rbnode_prev(&obj->node);
	return node ? structof(node, co_obj_t, node) : NULL;
}

co_obj_t *
co_obj_next(const co_obj_t *obj)
{
	assert(obj);

	struct rbnode *node = rbnode_next(&obj->node);
	return node ? structof(node, co_obj_t, node) : NULL;
}

co_dev_t *
co_obj_get_dev(const co_obj_t *obj)
{
	assert(obj);

	return obj->dev;
}

co_unsigned16_t
co_obj_get_idx(const co_obj_t *obj)
{
	assert(obj);

	return obj->idx;
}

co_unsigned8_t
co_obj_get_subidx(const co_obj_t *obj, co_unsigned8_t maxidx,
		co_unsigned8_t *subidx)
{
	assert(obj);

	if (!subidx)
		maxidx = 0;

	if (maxidx) {
		struct rbnode *node = rbtree_first(&obj->tree);
		for (size_t i = 0; node && i < maxidx;
				node = rbnode_next(node), i++)
			subidx[i] = co_sub_get_subidx(
					structof(node, co_sub_t, node));
	}

	return (co_unsigned8_t)rbtree_size(&obj->tree);
}

int
co_obj_insert_sub(co_obj_t *obj, co_sub_t *sub)
{
	assert(obj);
	assert(sub);

	if (sub->obj && sub->obj != obj)
		return -1;

	if (sub->obj == obj)
		return 0;

	if (rbtree_find(&obj->tree, sub->node.key))
		return -1;

	sub->obj = obj;
	rbtree_insert(&sub->obj->tree, &sub->node);

#if !LELY_NO_MALLOC
	co_obj_update(obj);
#endif

	return 0;
}

int
co_obj_remove_sub(co_obj_t *obj, co_sub_t *sub)
{
	assert(obj);
	assert(sub);

	if (sub->obj != obj)
		return -1;

	rbtree_remove(&sub->obj->tree, &sub->node);
	rbnode_init(&sub->node, &sub->subidx);
	sub->obj = NULL;

#if !LELY_NO_MALLOC
	co_val_fini(co_sub_get_type(sub), sub->val);
	sub->val = NULL;

	co_obj_update(obj);
#endif

	return 0;
}

co_sub_t *
co_obj_find_sub(const co_obj_t *obj, co_unsigned8_t subidx)
{
	assert(obj);

	struct rbnode *node = rbtree_find(&obj->tree, &subidx);
	return node ? structof(node, co_sub_t, node) : NULL;
}

co_sub_t *
co_obj_first_sub(const co_obj_t *obj)
{
	assert(obj);

	struct rbnode *node = rbtree_first(&obj->tree);
	return node ? structof(node, co_sub_t, node) : NULL;
}

co_sub_t *
co_obj_last_sub(const co_obj_t *obj)
{
	assert(obj);

	struct rbnode *node = rbtree_last(&obj->tree);
	return node ? structof(node, co_sub_t, node) : NULL;
}

#if !LELY_NO_CO_OBJ_NAME
const char *
co_obj_get_name(const co_obj_t *obj)
{
	assert(obj);

	return obj->name;
}
#endif

#if !LELY_NO_MALLOC && !LELY_NO_CO_OBJ_NAME
int
co_obj_set_name(co_obj_t *obj, const char *name)
{
	assert(obj);

	if (!name || !*name) {
		free(obj->name);
		obj->name = NULL;
		return 0;
	}

	void *ptr = realloc(obj->name, strlen(name) + 1);
	if (!ptr) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return -1;
	}
	obj->name = ptr;
	strcpy(obj->name, name);

	return 0;
}
#endif // !LELY_NO_MALLOC && !LELY_NO_CO_OBJ_NAME

co_unsigned8_t
co_obj_get_code(const co_obj_t *obj)
{
	assert(obj);

	return obj->code;
}

int
co_obj_set_code(co_obj_t *obj, co_unsigned8_t code)
{
	assert(obj);

	switch (code) {
	case CO_OBJECT_NULL:
	case CO_OBJECT_DOMAIN:
	case CO_OBJECT_DEFTYPE:
	case CO_OBJECT_DEFSTRUCT:
	case CO_OBJECT_VAR:
	case CO_OBJECT_ARRAY:
	case CO_OBJECT_RECORD: obj->code = code; return 0;
	default: set_errnum(ERRNUM_INVAL); return -1;
	}
}

void *
co_obj_addressof_val(const co_obj_t *obj)
{
	return obj ? obj->val : NULL;
}

size_t
co_obj_sizeof_val(const co_obj_t *obj)
{
	return obj ? obj->size : 0;
}

const void *
co_obj_get_val(const co_obj_t *obj, co_unsigned8_t subidx)
{
	co_sub_t *sub = obj ? co_obj_find_sub(obj, subidx) : NULL;
	return co_sub_get_val(sub);
}

size_t
co_obj_set_val(co_obj_t *obj, co_unsigned8_t subidx, const void *ptr, size_t n)
{
	assert(obj);

	co_sub_t *sub = co_obj_find_sub(obj, subidx);
	if (!sub) {
		set_errnum(ERRNUM_INVAL);
		return 0;
	}

	return co_sub_set_val(sub, ptr, n);
}

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
	co_##b##_t co_obj_get_val_##c( \
			const co_obj_t *obj, co_unsigned8_t subidx) \
	{ \
		/* clang-format off */ \
		co_sub_t *sub = obj \
				? co_obj_find_sub(obj, subidx) \
				: NULL; \
		/* clang-format on */ \
		return co_sub_get_val_##c(sub); \
	} \
\
	size_t co_obj_set_val_##c( \
			co_obj_t *obj, co_unsigned8_t subidx, co_##b##_t c) \
	{ \
		assert(obj); \
\
		co_sub_t *sub = co_obj_find_sub(obj, subidx); \
		if (!sub) { \
			set_errnum(ERRNUM_INVAL); \
			return 0; \
		} \
\
		return co_sub_set_val_##c(sub, c); \
	}
#include <lely/co/def/basic.def>
#undef LELY_CO_DEFINE_TYPE

void
co_obj_set_dn_ind(co_obj_t *obj, co_sub_dn_ind_t *ind, void *data)
{
	assert(obj);

	rbtree_foreach (&obj->tree, node)
		co_sub_set_dn_ind(structof(node, co_sub_t, node), ind, data);
}

#if !LELY_NO_CO_OBJ_UPLOAD
void
co_obj_set_up_ind(co_obj_t *obj, co_sub_up_ind_t *ind, void *data)
{
	assert(obj);

	rbtree_foreach (&obj->tree, node)
		co_sub_set_up_ind(structof(node, co_sub_t, node), ind, data);
}
#endif

#if !LELY_NO_MALLOC

void *
__co_sub_alloc(void)
{
	void *ptr = malloc(sizeof(struct __co_sub));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
__co_sub_free(void *ptr)
{
	free(ptr);
}

#endif // !LELY_NO_MALLOC

struct __co_sub *
__co_sub_init(struct __co_sub *sub, co_unsigned8_t subidx, co_unsigned16_t type,
		void *val)
{
	assert(sub);
#if !LELY_NO_MALLOC
	assert(val == NULL);
#endif

	rbnode_init(&sub->node, &sub->subidx);
	sub->obj = NULL;
	sub->subidx = subidx;

#if !LELY_NO_CO_OBJ_NAME
	sub->name = NULL;
#endif

	sub->type = type;
#if !LELY_NO_CO_OBJ_LIMITS
	if (co_val_init_min(sub->type, &sub->min) == -1)
		return NULL;
	if (co_val_init_max(sub->type, &sub->max) == -1)
		return NULL;
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
	if (co_val_init(sub->type, &sub->def) == -1)
		return NULL;
#endif
	sub->val = val;

	sub->access = CO_ACCESS_RW;
	sub->pdo_mapping = 0;
	sub->flags = 0;

	sub->dn_ind = &co_sub_default_dn_ind;
	sub->dn_data = NULL;
#if !LELY_NO_CO_OBJ_UPLOAD
	sub->up_ind = &co_sub_default_up_ind;
	sub->up_data = NULL;
#endif

	return sub;
}

void
__co_sub_fini(struct __co_sub *sub)
{
	assert(sub);

	if (sub->obj)
		co_obj_remove_sub(sub->obj, sub);

#if !LELY_NO_CO_OBJ_DEFAULT
	co_val_fini(sub->type, &sub->def);
#endif
#if !LELY_NO_CO_OBJ_LIMITS
	co_val_fini(sub->type, &sub->max);
	co_val_fini(sub->type, &sub->min);
#endif

#if !LELY_NO_MALLOC && !LELY_NO_CO_OBJ_NAME
	free(sub->name);
#endif
}

#if !LELY_NO_MALLOC

co_sub_t *
co_sub_create(co_unsigned8_t subidx, co_unsigned16_t type)
{
	int errc = 0;

	co_sub_t *sub = __co_sub_alloc();
	if (!sub) {
		errc = get_errc();
		goto error_alloc_sub;
	}

	if (!__co_sub_init(sub, subidx, type, NULL)) {
		errc = get_errc();
		goto error_init_sub;
	}

	return sub;

error_init_sub:
	__co_sub_free(sub);
error_alloc_sub:
	set_errc(errc);
	return NULL;
}

void
co_sub_destroy(co_sub_t *sub)
{
	if (sub) {
		__co_sub_fini(sub);
		__co_sub_free(sub);
	}
}

#endif // !LELY_NO_MALLOC

co_sub_t *
co_sub_prev(const co_sub_t *sub)
{
	assert(sub);

	struct rbnode *node = rbnode_prev(&sub->node);
	return node ? structof(node, co_sub_t, node) : NULL;
}

co_sub_t *
co_sub_next(const co_sub_t *sub)
{
	assert(sub);

	struct rbnode *node = rbnode_next(&sub->node);
	return node ? structof(node, co_sub_t, node) : NULL;
}

co_obj_t *
co_sub_get_obj(const co_sub_t *sub)
{
	assert(sub);

	return sub->obj;
}

co_unsigned8_t
co_sub_get_subidx(const co_sub_t *sub)
{
	assert(sub);

	return sub->subidx;
}

#if !LELY_NO_CO_OBJ_NAME
const char *
co_sub_get_name(const co_sub_t *sub)
{
	assert(sub);

	return sub->name;
}
#endif

#if !LELY_NO_MALLOC && !LELY_NO_CO_OBJ_NAME
int
co_sub_set_name(co_sub_t *sub, const char *name)
{
	assert(sub);

	if (!name || !*name) {
		free(sub->name);
		sub->name = NULL;
		return 0;
	}

	void *ptr = realloc(sub->name, strlen(name) + 1);
	if (!ptr) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return -1;
	}
	sub->name = ptr;
	strcpy(sub->name, name);

	return 0;
}
#endif // !LELY_NO_MALLOC && !LELY_NO_CO_OBJ_NAME

co_unsigned16_t
co_sub_get_type(const co_sub_t *sub)
{
	assert(sub);

	return sub->type;
}

#if !LELY_NO_CO_OBJ_LIMITS

const void *
co_sub_addressof_min(const co_sub_t *sub)
{
	return sub ? co_val_addressof(sub->type, &sub->min) : NULL;
}

size_t
co_sub_sizeof_min(const co_sub_t *sub)
{
	return sub ? co_val_sizeof(sub->type, &sub->min) : 0;
}

const void *
co_sub_get_min(const co_sub_t *sub)
{
	return sub ? &sub->min : NULL;
}

size_t
co_sub_set_min(co_sub_t *sub, const void *ptr, size_t n)
{
	assert(sub);

	co_val_fini(sub->type, &sub->min);
	return co_val_make(sub->type, &sub->min, ptr, n);
}

const void *
co_sub_addressof_max(const co_sub_t *sub)
{
	return sub ? co_val_addressof(sub->type, &sub->max) : NULL;
}

size_t
co_sub_sizeof_max(const co_sub_t *sub)
{
	return sub ? co_val_sizeof(sub->type, &sub->max) : 0;
}

const void *
co_sub_get_max(const co_sub_t *sub)
{
	return sub ? &sub->max : NULL;
}

size_t
co_sub_set_max(co_sub_t *sub, const void *ptr, size_t n)
{
	assert(sub);

	co_val_fini(sub->type, &sub->max);
	return co_val_make(sub->type, &sub->max, ptr, n);
}

#endif // !LELY_NO_CO_OBJ_LIMITS

#if !LELY_NO_CO_OBJ_DEFAULT

const void *
co_sub_addressof_def(const co_sub_t *sub)
{
	return sub ? co_val_addressof(sub->type, &sub->def) : NULL;
}

size_t
co_sub_sizeof_def(const co_sub_t *sub)
{
	return sub ? co_val_sizeof(sub->type, &sub->def) : 0;
}

const void *
co_sub_get_def(const co_sub_t *sub)
{
	return sub ? &sub->def : NULL;
}

size_t
co_sub_set_def(co_sub_t *sub, const void *ptr, size_t n)
{
	assert(sub);

	co_val_fini(sub->type, &sub->def);
	return co_val_make(sub->type, &sub->def, ptr, n);
}

#endif // !LELY_NO_CO_OBJ_DEFAULT

const void *
co_sub_addressof_val(const co_sub_t *sub)
{
	return sub ? co_val_addressof(sub->type, sub->val) : NULL;
}

size_t
co_sub_sizeof_val(const co_sub_t *sub)
{
	return sub ? co_val_sizeof(sub->type, sub->val) : 0;
}

const void *
co_sub_get_val(const co_sub_t *sub)
{
	return sub ? sub->val : NULL;
}

size_t
co_sub_set_val(co_sub_t *sub, const void *ptr, size_t n)
{
	assert(sub);

	co_val_fini(sub->type, sub->val);
	return co_val_make(sub->type, sub->val, ptr, n);
}

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
	co_##b##_t co_sub_get_val_##c(const co_sub_t *sub) \
	{ \
		static const co_##b##_t val; \
\
		if (!sub || sub->type != CO_DEFTYPE_##a || !sub->val) \
			return val; \
		return ((union co_val *)sub->val)->c; \
	} \
\
	size_t co_sub_set_val_##c(co_sub_t *sub, co_##b##_t c) \
	{ \
		assert(sub); \
\
		if (sub->type != CO_DEFTYPE_##a) { \
			set_errnum(ERRNUM_INVAL); \
			return 0; \
		} \
\
		return co_sub_set_val(sub, &c, sizeof(c)); \
	}
#include <lely/co/def/basic.def>
#undef LELY_CO_DEFINE_TYPE

#if !LELY_NO_CO_OBJ_LIMITS
co_unsigned32_t
co_sub_chk_val(const co_sub_t *sub, co_unsigned16_t type, const void *val)
{
	assert(sub);

	// Arrays do not have a minimum or maximum value.
	if (!co_type_is_basic(sub->type))
		return 0;

	if (sub->type != type)
		return CO_SDO_AC_TYPE_LEN;

	assert(val);

	// Check whether the value is within bounds.
	if (co_val_cmp(sub->type, &sub->min, &sub->max) > 0)
		return CO_SDO_AC_PARAM_RANGE;
	if (co_val_cmp(sub->type, val, &sub->max) > 0)
		return CO_SDO_AC_PARAM_HI;
	if (co_val_cmp(sub->type, val, &sub->min) < 0)
		return CO_SDO_AC_PARAM_LO;

	return 0;
}
#endif

unsigned int
co_sub_get_access(const co_sub_t *sub)
{
	assert(sub);

	return sub->access;
}

int
co_sub_set_access(co_sub_t *sub, unsigned int access)
{
	assert(sub);

	switch (access) {
	case CO_ACCESS_RO:
	case CO_ACCESS_WO:
	case CO_ACCESS_RW:
	case CO_ACCESS_RWR:
	case CO_ACCESS_RWW:
	case CO_ACCESS_CONST: sub->access = access; return 0;
	default: set_errnum(ERRNUM_INVAL); return -1;
	}
}

int
co_sub_get_pdo_mapping(const co_sub_t *sub)
{
	assert(sub);

	return sub->pdo_mapping;
}

void
co_sub_set_pdo_mapping(co_sub_t *sub, int pdo_mapping)
{
	assert(sub);

	sub->pdo_mapping = !!pdo_mapping;
}

unsigned int
co_sub_get_flags(const co_sub_t *sub)
{
	assert(sub);

	return sub->flags;
}

void
co_sub_set_flags(co_sub_t *sub, unsigned int flags)
{
	assert(sub);

	sub->flags = flags;
}

#if !LELY_NO_CO_OBJ_FILE

const char *
co_sub_get_upload_file(const co_sub_t *sub)
{
	assert(sub);

	if (!(sub->flags & CO_OBJ_FLAGS_UPLOAD_FILE))
		return NULL;

	assert(sub->type == CO_DEFTYPE_DOMAIN);
	return co_sub_addressof_val(sub);
}

int
co_sub_set_upload_file(co_sub_t *sub, const char *filename)
{
	assert(sub);
	assert(filename);

	if (!(co_sub_get_flags(sub) & CO_OBJ_FLAGS_UPLOAD_FILE)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	assert(sub->type == CO_DEFTYPE_DOMAIN);
	return co_sub_set_val(sub, filename, strlen(filename) + 1) ? 0 : -1;
}

const char *
co_sub_get_download_file(const co_sub_t *sub)
{
	assert(sub);

	if (!(sub->flags & CO_OBJ_FLAGS_DOWNLOAD_FILE))
		return NULL;

	assert(sub->type == CO_DEFTYPE_DOMAIN);
	return co_sub_addressof_val(sub);
}

int
co_sub_set_download_file(co_sub_t *sub, const char *filename)
{
	assert(sub);
	assert(filename);

	if (!(co_sub_get_flags(sub) & CO_OBJ_FLAGS_DOWNLOAD_FILE)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	assert(sub->type == CO_DEFTYPE_DOMAIN);
	return co_sub_set_val(sub, filename, strlen(filename) + 1) ? 0 : -1;
}

#endif // !LELY_NO_CO_OBJ_FILE

void
co_sub_get_dn_ind(const co_sub_t *sub, co_sub_dn_ind_t **pind, void **pdata)
{
	assert(sub);

	if (pind)
		*pind = sub->dn_ind;
	if (pdata)
		*pdata = sub->dn_data;
}

void
co_sub_set_dn_ind(co_sub_t *sub, co_sub_dn_ind_t *ind, void *data)
{
	assert(sub);

	sub->dn_ind = ind ? ind : &co_sub_default_dn_ind;
	sub->dn_data = ind ? data : NULL;
}

int
co_sub_on_dn(co_sub_t *sub, struct co_sdo_req *req, co_unsigned32_t *pac)
{
	assert(sub);
	assert(req);

#if !LELY_NO_CO_OBJ_FILE
	// clang-format off
	if (co_sub_get_type(sub) == CO_DEFTYPE_DOMAIN && (co_sub_get_flags(sub)
			& CO_OBJ_FLAGS_DOWNLOAD_FILE))
		// clang-format on
		return co_sdo_req_dn_file(req, co_sub_addressof_val(sub), pac);
#endif

	// Read the value.
	co_unsigned16_t type = co_sub_get_type(sub);
	union co_val val;
#if LELY_NO_MALLOC
	struct co_array array = CO_ARRAY_INIT;
	if (co_type_is_array(type))
		co_val_init_array(&val, &array);
#endif
	if (co_sdo_req_dn_val(req, type, &val, pac) == -1)
		return -1;

#if !LELY_NO_CO_OBJ_LIMITS
	// Accept the value if it is within bounds.
	co_unsigned32_t ac = co_sub_chk_val(sub, type, &val);
	if (ac) {
#if !LELY_NO_MALLOC
		co_val_fini(type, &val);
#endif
		if (pac)
			*pac = ac;
		return -1;
	}
#endif

	co_sub_dn(sub, &val);
#if !LELY_NO_MALLOC
	co_val_fini(type, &val);
#endif

	return 0;
}

co_unsigned32_t
co_sub_dn_ind(co_sub_t *sub, struct co_sdo_req *req)
{
	if (!sub)
		return CO_SDO_AC_NO_SUB;

	if (!(sub->access & CO_ACCESS_WRITE))
		return CO_SDO_AC_NO_WRITE;

	if (!req)
		return CO_SDO_AC_ERROR;

	assert(sub->dn_ind);
	return sub->dn_ind(sub, req, sub->dn_data);
}

co_unsigned32_t
co_sub_dn_ind_val(co_sub_t *sub, co_unsigned16_t type, const void *val)
{
	if (co_sub_get_type(sub) != type)
		return CO_SDO_AC_TYPE_LEN;

	struct co_sdo_req req = CO_SDO_REQ_INIT;
	co_unsigned32_t ac = 0;

	int errc = get_errc();

	if (co_sdo_req_up_val(&req, type, val, &ac) == -1)
		goto error;

	ac = co_sub_dn_ind(sub, &req);

error:
	co_sdo_req_fini(&req);
	set_errc(errc);
	return ac;
}

int
co_sub_dn(co_sub_t *sub, void *val)
{
	assert(sub);

	if (!(sub->flags & CO_OBJ_FLAGS_WRITE)) {
#if LELY_NO_MALLOC
		if (!co_val_copy(sub->type, sub->val, val))
			return -1;
#else
		co_val_fini(sub->type, sub->val);
		if (!co_val_move(sub->type, sub->val, val))
			return -1;
#endif
	}

	return 0;
}

#if !LELY_NO_CO_OBJ_UPLOAD

void
co_sub_get_up_ind(const co_sub_t *sub, co_sub_up_ind_t **pind, void **pdata)
{
	assert(sub);

	if (pind)
		*pind = sub->up_ind;
	if (pdata)
		*pdata = sub->up_data;
}

void
co_sub_set_up_ind(co_sub_t *sub, co_sub_up_ind_t *ind, void *data)
{
	assert(sub);

	sub->up_ind = ind ? ind : &co_sub_default_up_ind;
	sub->up_data = ind ? data : NULL;
}

#endif // !LELY_NO_CO_OBJ_UPLOAD

int
co_sub_on_up(const co_sub_t *sub, struct co_sdo_req *req, co_unsigned32_t *pac)
{
	assert(sub);
	assert(req);

#if !LELY_NO_CO_OBJ_FILE
	if (co_sub_get_type(sub) == CO_DEFTYPE_DOMAIN
			&& (co_sub_get_flags(sub) & CO_OBJ_FLAGS_UPLOAD_FILE)) {
		const char *filename = co_sub_addressof_val(sub);
		// Ignore an empty UploadFile attribute.
		if (!filename || !*filename)
			return 0;
		return co_sdo_req_up_file(req, filename, pac);
	}
#endif

	const void *val = co_sub_get_val(sub);
	if (!val) {
		if (pac)
			*pac = CO_SDO_AC_NO_DATA;
		return -1;
	}

	return co_sdo_req_up_val(req, co_sub_get_type(sub), val, pac);
}

co_unsigned32_t
co_sub_up_ind(const co_sub_t *sub, struct co_sdo_req *req)
{
	if (!sub)
		return CO_SDO_AC_NO_SUB;

	if (!(sub->access & CO_ACCESS_READ))
		return CO_SDO_AC_NO_READ;

	if (!req)
		return CO_SDO_AC_ERROR;

#if LELY_NO_CO_OBJ_UPLOAD
	return co_sub_default_up_ind(sub, req, NULL);
#else
	assert(sub->up_ind);
	return sub->up_ind(sub, req, sub->up_data);
#endif
}

co_unsigned32_t
co_sub_default_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	(void)data;

	co_unsigned32_t ac = 0;
	co_sub_on_dn(sub, req, &ac);
	return ac;
}

co_unsigned32_t
co_sub_default_up_ind(const co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	(void)data;

	co_unsigned32_t ac = 0;
	co_sub_on_up(sub, req, &ac);
	return ac;
}

#if !LELY_NO_MALLOC

static void
co_obj_update(co_obj_t *obj)
{
	assert(obj);

	// Compute the total size and alignment (in bytes) of the object.
	size_t align = 1;
	size_t size = 0;
	rbtree_foreach (&obj->tree, node) {
		co_sub_t *sub = structof(node, co_sub_t, node);
		co_unsigned16_t type = co_sub_get_type(sub);
		align = MAX(align, co_type_alignof(type));
		size = ALIGN(size, co_type_alignof(type));
		size += co_type_sizeof(type);
	}
	size = ALIGN(size, align);

	void *val = NULL;
	if (size) {
		val = calloc(1, size);
		if (!val) {
#if !LELY_NO_ERRNO
			set_errc(errno2c(errno));
#endif
			return;
		}
	}

	// Initialize the values of the sub-objects.
	size_t offset = 0;
	rbtree_foreach (&obj->tree, node) {
		co_sub_t *sub = structof(node, co_sub_t, node);
		co_unsigned16_t type = co_sub_get_type(sub);
		// Compute the offset of the sub-object.
		offset = ALIGN(offset, co_type_alignof(type));
		// Move the old value, if it exists.
		void *src = sub->val;
		sub->val = (char *)val + offset;
		if (src)
			co_val_move(type, sub->val, src);
		offset += co_type_sizeof(type);
	}

	free(obj->val);
	obj->val = val;
	obj->size = size;
}

static void
co_obj_clear(co_obj_t *obj)
{
	assert(obj);

	rbtree_foreach (&obj->tree, node)
		co_sub_destroy(structof(node, co_sub_t, node));

	free(obj->val);
	obj->val = NULL;
}

#endif // !LELY_NO_MALLOC
