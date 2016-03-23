/*!\file
 * This file is part of the CANopen library; it contains the implementation of
 * the device description.
 *
 * \see lely/co/dev.h
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

#include "co.h"
#include <lely/util/errnum.h>
#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include "obj.h"

#include <assert.h>
#include <stdlib.h>

//! A CANopen device.
struct __co_dev {
	//! The Node-ID.
	co_unsigned8_t id;
	//! The tree containing the object dictionary.
	struct rbtree tree;
	//! A pointer to the vendor name.
	char *vendor_name;
	//! The vendor ID.
	co_unsigned32_t vendor_id;
	//! A pointer to the product name.
	char *product_name;
	//! The product code.
	co_unsigned32_t product_code;
	//! The revision number.
	co_unsigned32_t revision;
	//! A pointer to the order code.
	char *order_code;
	//! The supported bit rates.
	unsigned baud:8;
	//! The data types supported for mapping dummy entries in PDOs.
	co_unsigned32_t dummy;
};

static void co_obj_set_id(co_obj_t *obj, co_unsigned8_t new_id,
		co_unsigned8_t old_id);
static void co_sub_set_id(co_sub_t *sub, co_unsigned8_t new_id,
		co_unsigned8_t old_id);
static void co_val_set_id(co_unsigned16_t type, void *val,
		co_unsigned8_t new_id, co_unsigned8_t old_id);

LELY_CO_EXPORT void *
__co_dev_alloc(void)
{
	return malloc(sizeof(struct __co_dev));
}

LELY_CO_EXPORT void
__co_dev_free(void *ptr)
{
	free(ptr);
}

LELY_CO_EXPORT struct __co_dev *
__co_dev_init(struct __co_dev *dev, co_unsigned8_t id)
{
	assert(dev);

	if (__unlikely(!id || (id > CO_NUM_NODES && id != 0xff))) {
		set_errnum(ERRNUM_INVAL);
		return NULL;
	}
	dev->id = id;

	rbtree_init(&dev->tree, &uint16_cmp);

	dev->vendor_name = NULL;
	dev->vendor_id = 0;
	dev->product_name = NULL;
	dev->product_code = 0;
	dev->revision = 0;
	dev->order_code = NULL;

	dev->baud = 0;

	dev->dummy = 0;

	return dev;
}

LELY_CO_EXPORT void
__co_dev_fini(struct __co_dev *dev)
{
	assert(dev);

	rbtree_foreach(&dev->tree, node)
		co_obj_destroy(structof(node, co_obj_t, node));

	free(dev->vendor_name);
	free(dev->product_name);
	free(dev->order_code);
}

LELY_CO_EXPORT co_dev_t *
co_dev_create(co_unsigned8_t id)
{
	errc_t errc = 0;

	co_dev_t *dev = __co_dev_alloc();
	if (__unlikely(!dev)) {
		errc = get_errc();
		goto error_alloc_dev;
	}

	if (__unlikely(!__co_dev_init(dev, id))) {
		errc = get_errc();
		goto error_init_dev;
	}

	return dev;

error_init_dev:
	__co_dev_free(dev);
error_alloc_dev:
	set_errc(errc);
	return NULL;
}

LELY_CO_EXPORT void
co_dev_destroy(co_dev_t *dev)
{
	if (dev) {
		__co_dev_fini(dev);
		__co_dev_free(dev);
	}
}

LELY_CO_EXPORT co_unsigned8_t
co_dev_get_id(const co_dev_t *dev)
{
	assert(dev);

	return dev->id;
}

LELY_CO_EXPORT int
co_dev_set_id(co_dev_t *dev, co_unsigned8_t id)
{
	assert(dev);

	if (__unlikely(!id || (id > CO_NUM_NODES && id != 0xff))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	rbtree_foreach(&dev->tree, node)
		co_obj_set_id(structof(node, co_obj_t, node), id, dev->id);

	dev->id = id;

	return 0;
}

LELY_CO_EXPORT co_unsigned16_t
co_dev_get_idx(const co_dev_t *dev, co_unsigned16_t maxidx,
		co_unsigned16_t *idx)
{
	assert(dev);

	if (!idx)
		maxidx = 0;

	if (maxidx) {
		struct rbnode *node = rbtree_first(&dev->tree);
		for (size_t i = 0; node && i < maxidx;
				node = rbnode_next(node), i++)
			idx[i] = co_obj_get_idx(structof(node, co_obj_t, node));
	}

	return rbtree_size(&dev->tree);
}

LELY_CO_EXPORT int
co_dev_insert_obj(co_dev_t *dev, co_obj_t *obj)
{
	assert(dev);
	assert(obj);

	if (__unlikely(obj->dev && obj->dev != dev))
		return -1;

	if (__unlikely(obj->dev == dev))
		return 0;

	if (__unlikely(rbtree_find(&dev->tree, obj->node.key)))
		return -1;

	obj->dev = dev;
	rbtree_insert(&obj->dev->tree, &obj->node);

	return 0;
}

LELY_CO_EXPORT int
co_dev_remove_obj(co_dev_t *dev, co_obj_t *obj)
{
	assert(dev);
	assert(obj);

	if (__unlikely(obj->dev != dev))
		return -1;

	rbtree_remove(&obj->dev->tree, &obj->node);
	obj->dev = NULL;

	return 0;
}

LELY_CO_EXPORT co_obj_t *
co_dev_find_obj(const co_dev_t *dev, co_unsigned16_t idx)
{
	assert(dev);

	struct rbnode *node = rbtree_find(&dev->tree, &idx);
	if (!node)
		return NULL;
	return structof(node, co_obj_t, node);
}

LELY_CO_EXPORT co_sub_t *
co_dev_find_sub(const co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx)
{
	co_obj_t *obj = co_dev_find_obj(dev, idx);
	return __likely(obj) ? co_obj_find_sub(obj, subidx) : NULL;
}

LELY_CO_EXPORT const char *
co_dev_get_vendor_name(const co_dev_t *dev)
{
	assert(dev);

	return dev->vendor_name;
}

LELY_CO_EXPORT int
co_dev_set_vendor_name(co_dev_t *dev, const char *vendor_name)
{
	assert(dev);

	if (!vendor_name || !*vendor_name) {
		free(dev->vendor_name);
		dev->vendor_name = NULL;
		return 0;
	}

	void *ptr = realloc(dev->vendor_name, strlen(vendor_name) + 1);
	if (__unlikely(!ptr))
		return -1;
	dev->vendor_name = ptr;
	strcpy(dev->vendor_name, vendor_name);

	return 0;
}

LELY_CO_EXPORT co_unsigned32_t
co_dev_get_vendor_id(const co_dev_t *dev)
{
	assert(dev);

	return dev->vendor_id;
}

LELY_CO_EXPORT void
co_dev_set_vendor_id(co_dev_t *dev, co_unsigned32_t vendor_id)
{
	assert(dev);

	dev->vendor_id = vendor_id;
}

LELY_CO_EXPORT const char *
co_dev_get_product_name(const co_dev_t *dev)
{
	assert(dev);

	return dev->product_name;
}

LELY_CO_EXPORT int
co_dev_set_product_name(co_dev_t *dev, const char *product_name)
{
	assert(dev);

	if (!product_name || !*product_name) {
		free(dev->product_name);
		dev->product_name = NULL;
		return 0;
	}

	void *ptr = realloc(dev->product_name, strlen(product_name) + 1);
	if (__unlikely(!ptr))
		return -1;
	dev->product_name = ptr;
	strcpy(dev->product_name, product_name);

	return 0;
}

LELY_CO_EXPORT co_unsigned32_t
co_dev_get_product_code(const co_dev_t *dev)
{
	assert(dev);

	return dev->product_code;
}

LELY_CO_EXPORT void
co_dev_set_product_code(co_dev_t *dev, co_unsigned32_t product_code)
{
	assert(dev);

	dev->product_code = product_code;
}

LELY_CO_EXPORT co_unsigned32_t
co_dev_get_revision(const co_dev_t *dev)
{
	assert(dev);

	return dev->revision;
}

LELY_CO_EXPORT void
co_dev_set_revision(co_dev_t *dev, co_unsigned32_t revision)
{
	assert(dev);

	dev->revision = revision;
}

LELY_CO_EXPORT const char *
co_dev_get_order_code(const co_dev_t *dev)
{
	assert(dev);

	return dev->order_code;
}

LELY_CO_EXPORT int
co_dev_set_order_code(co_dev_t *dev, const char *order_code)
{
	assert(dev);

	if (!order_code || !*order_code) {
		free(dev->order_code);
		dev->order_code = NULL;
		return 0;
	}

	void *ptr = realloc(dev->order_code, strlen(order_code) + 1);
	if (__unlikely(!ptr))
		return -1;
	dev->order_code = ptr;
	strcpy(dev->order_code, order_code);

	return 0;
}

LELY_CO_EXPORT unsigned int
co_dev_get_baud(const co_dev_t *dev)
{
	assert(dev);

	return dev->baud;
}

LELY_CO_EXPORT void
co_dev_set_baud(co_dev_t *dev, unsigned int baud)
{
	assert(dev);

	dev->baud = baud;
}

LELY_CO_EXPORT co_unsigned32_t
co_dev_get_dummy(const co_dev_t *dev)
{
	assert(dev);

	return dev->dummy;
}

LELY_CO_EXPORT void
co_dev_set_dummy(co_dev_t *dev, co_unsigned32_t dummy)
{
	assert(dev);

	dev->dummy = dummy;
}

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
	LELY_CO_EXPORT co_##b##_t \
	co_dev_get_val_##c(const co_dev_t *dev, co_unsigned16_t idx, \
			co_unsigned8_t subidx) \
	{ \
		co_sub_t *sub = __likely(dev) \
				? co_dev_find_sub(dev, idx, subidx) \
				: NULL; \
		return co_sub_get_val_##c(sub); \
	} \
	\
	LELY_CO_EXPORT size_t \
	co_dev_set_val_##c(co_dev_t *dev, co_unsigned16_t idx, \
			co_unsigned8_t subidx, co_##b##_t c) \
	{ \
		assert(dev); \
	\
		co_sub_t *sub = co_dev_find_sub(dev, idx, subidx); \
		if (__unlikely(!sub)) { \
			set_errnum(ERRNUM_INVAL); \
			return 0; \
		} \
	\
		return co_sub_set_val_##c(sub, c); \
	}
#include <lely/co/def/basic.def>
#undef LELY_CO_DEFINE_TYPE

static void
co_obj_set_id(co_obj_t *obj, co_unsigned8_t new_id, co_unsigned8_t old_id)
{
	assert(obj);

	rbtree_foreach(&obj->tree, node)
		co_sub_set_id(structof(node, co_sub_t, node), new_id, old_id);
}

static void
co_sub_set_id(co_sub_t *sub, co_unsigned8_t new_id, co_unsigned8_t old_id)
{
	assert(sub);

	unsigned int flags = co_sub_get_flags(sub);
	co_unsigned16_t type = co_sub_get_type(sub);
	if (flags & CO_OBJ_FLAGS_MIN_NODEID)
		co_val_set_id(type, &sub->min, new_id, old_id);
	if (flags & CO_OBJ_FLAGS_MAX_NODEID)
		co_val_set_id(type, &sub->max, new_id, old_id);
	if (flags & CO_OBJ_FLAGS_DEF_NODEID)
		co_val_set_id(type, &sub->def, new_id, old_id);
	if (flags & CO_OBJ_FLAGS_VAL_NODEID)
		co_val_set_id(type, sub->val, new_id, old_id);
}

static void
co_val_set_id(co_unsigned16_t type, void *val, co_unsigned8_t new_id,
		co_unsigned8_t old_id)
{
	assert(val);

	union co_val *u = val;
	switch (type) {
#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
	case CO_DEFTYPE_##a: \
		u->c += new_id - old_id; \
		break;
#include <lely/co/def/basic.def>
#undef LELY_CO_DEFINE_TYPE
	}
}

