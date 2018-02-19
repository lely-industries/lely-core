/*!\file
 * This file is part of the CANopen library; it contains the implementation of
 * the device description.
 *
 * \see lely/co/dev.h
 *
 * \copyright 2018 Lely Industries N.V.
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
#ifndef LELY_NO_CO_DCF
#include <lely/util/frbuf.h>
#include <lely/util/fwbuf.h>
#endif
#include <lely/co/dev.h>
#include "obj.h"

#include <assert.h>
#include <stdlib.h>

//! A CANopen device.
struct __co_dev {
	//! The network-ID.
	co_unsigned8_t netid;
	//! The node-ID.
	co_unsigned8_t id;
	//! The tree containing the object dictionary.
	struct rbtree tree;
	//! A pointer to the name of the device.
	char *name;
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
	unsigned baud:10;
	//! The (pending) baudrate (in kbit/s).
	co_unsigned16_t rate;
	//! A flag specifying whether LSS is supported (1) or not (0).
	int lss;
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
	void *ptr = malloc(sizeof(struct __co_dev));
	if (__unlikely(!ptr))
		set_errno(errno);
	return ptr;
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

	dev->netid = 0;

	if (__unlikely(!id || (id > CO_NUM_NODES && id != 0xff))) {
		set_errnum(ERRNUM_INVAL);
		return NULL;
	}
	dev->id = id;

	rbtree_init(&dev->tree, &uint16_cmp);

	dev->name = NULL;

	dev->vendor_name = NULL;
	dev->vendor_id = 0;
	dev->product_name = NULL;
	dev->product_code = 0;
	dev->revision = 0;
	dev->order_code = NULL;

	dev->baud = 0;
	dev->rate = 0;

	dev->lss = 0;

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

	free(dev->name);
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
co_dev_get_netid(const co_dev_t *dev)
{
	assert(dev);

	return dev->netid;
}

LELY_CO_EXPORT int
co_dev_set_netid(co_dev_t *dev, co_unsigned8_t id)
{
	assert(dev);

	if (__unlikely(id > CO_NUM_NETWORKS && id != 0xff)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	dev->netid = id;

	return 0;
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

	return (co_unsigned16_t)rbtree_size(&dev->tree);
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
co_dev_get_name(const co_dev_t *dev)
{
	assert(dev);

	return dev->name;
}

LELY_CO_EXPORT int
co_dev_set_name(co_dev_t *dev, const char *name)
{
	assert(dev);

	if (!name || !*name) {
		free(dev->name);
		dev->name = NULL;
		return 0;
	}

	void *ptr = realloc(dev->name, strlen(name) + 1);
	if (__unlikely(!ptr)) {
		set_errno(errno);
		return -1;
	}
	dev->name = ptr;
	strcpy(dev->name, name);

	return 0;
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
	if (__unlikely(!ptr)) {
		set_errno(errno);
		return -1;
	}
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
	if (__unlikely(!ptr)) {
		set_errno(errno);
		return -1;
	}
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
	if (__unlikely(!ptr)) {
		set_errno(errno);
		return -1;
	}
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

LELY_CO_EXPORT co_unsigned16_t
co_dev_get_rate(const co_dev_t *dev)
{
	assert(dev);

	return dev->rate;
}

LELY_CO_EXPORT void
co_dev_set_rate(co_dev_t *dev, co_unsigned16_t rate)
{
	assert(dev);

	dev->rate = rate;
}

LELY_CO_EXPORT int
co_dev_get_lss(const co_dev_t *dev)
{
	assert(dev);

	return dev->lss;
}

LELY_CO_EXPORT void
co_dev_set_lss(co_dev_t *dev, int lss)
{
	assert(dev);

	dev->lss = !!lss;
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

LELY_CO_EXPORT const void *
co_dev_get_val(const co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx)
{
	co_sub_t *sub = __likely(dev)
			? co_dev_find_sub(dev, idx, subidx)
			: NULL;
	return co_sub_get_val(sub);
}

LELY_CO_EXPORT size_t
co_dev_set_val(co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx,
		const void *ptr, size_t n)
{
	assert(dev);

	co_sub_t *sub = co_dev_find_sub(dev, idx, subidx);
	if (__unlikely(!sub)) {
		set_errnum(ERRNUM_INVAL);
		return 0;
	}

	return co_sub_set_val(sub, ptr, n);
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

LELY_CO_EXPORT size_t
co_dev_read_sub(co_dev_t *dev, co_unsigned16_t *pidx, co_unsigned8_t *psubidx,
		const uint8_t *begin, const uint8_t *end)
{
	if (__unlikely(!begin || !end || end - begin < 2 + 1 + 4))
		return 0;

	// Read the object index.
	co_unsigned16_t idx;
	if (__unlikely(co_val_read(CO_DEFTYPE_UNSIGNED16, &idx, begin, end)
			!= 2))
		return 0;
	begin += 2;
	// Read the object sub-index.
	co_unsigned8_t subidx;
	if (__unlikely(co_val_read(CO_DEFTYPE_UNSIGNED8, &subidx, begin, end)
			!= 1))
		return 0;
	begin += 1;
	// Read the value size (in bytes).
	co_unsigned32_t size;
	if (__unlikely(co_val_read(CO_DEFTYPE_UNSIGNED32, &size, begin, end)
			!= 4))
		return 0;
	begin += 4;

	if (__unlikely(end - begin < (ptrdiff_t)size))
		return 0;

	// Read the value into the sub-object, if it exists.
	co_sub_t *sub = co_dev_find_sub(dev, idx, subidx);
	if (sub) {
		co_unsigned16_t type = co_sub_get_type(sub);
		union co_val val;
		co_val_init(type, &val);
		if (__likely(co_val_read(type, &val, begin, begin + size)
				== size))
			co_sub_set_val(sub, co_val_addressof(type, &val),
					co_val_sizeof(type, &val));
		co_val_fini(type, &val);
	}

	if (pidx)
		*pidx = idx;
	if (psubidx)
		*psubidx = subidx;

	return 2 + 1 + 4 + size;
}

LELY_CO_EXPORT size_t
co_dev_write_sub(const co_dev_t *dev, co_unsigned16_t idx,
		co_unsigned8_t subidx, uint8_t *begin, uint8_t *end)
{
	co_sub_t *sub = co_dev_find_sub(dev, idx, subidx);
	if (__unlikely(!sub))
		return 0;
	co_unsigned16_t type = co_sub_get_type(sub);
	const void *val = co_sub_get_val(sub);

	co_unsigned32_t size = co_val_write(type, val, NULL, NULL);
	if (__unlikely(!size && co_val_sizeof(type, val)))
		return 0;

	if (begin && (!end || end - begin >= (ptrdiff_t)(2 + 1 + 4 + size))) {
		// Write the object index.
		if (__unlikely(co_val_write(CO_DEFTYPE_UNSIGNED16, &idx,
				begin, end) != 2))
			return 0;
		begin += 2;
		// Write the object sub-index.
		if (__unlikely(co_val_write(CO_DEFTYPE_UNSIGNED8, &subidx,
				begin, end) != 1))
			return 0;
		begin += 1;
		// Write the value size (in bytes).
		if (__unlikely(co_val_write(CO_DEFTYPE_UNSIGNED32, &size,
				begin, end) != 4))
			return 0;
		begin += 4;
		// Write the value.
		if (__unlikely(co_val_write(type, val, begin, end) != size))
			return 0;
	}

	return 2 + 1 + 4 + size;
}

LELY_CO_EXPORT int
co_dev_read_dcf(co_dev_t *dev, co_unsigned16_t *pmin, co_unsigned16_t *pmax,
		void *const *ptr)
{
	assert(dev);
	assert(ptr);

	co_unsigned16_t min = CO_UNSIGNED16_MAX;
	co_unsigned16_t max = CO_UNSIGNED16_MIN;

	size_t size = co_val_sizeof(CO_DEFTYPE_DOMAIN, ptr);
	const uint8_t *begin = *ptr;
	const uint8_t *end = begin + size;

	// Read the total number of sub-indices.
	co_unsigned32_t n;
	size = co_val_read(CO_DEFTYPE_UNSIGNED32, &n, begin, end);
	if (__unlikely(size != 4))
		return 0;
	begin += size;

	for (size_t i = 0; i < n; i++) {
		// Read the value of the sub-object.
		co_unsigned16_t idx;
		size = co_dev_read_sub(dev, &idx, NULL, begin, end);
		if (__unlikely(!size))
			return 0;
		begin += size;

		// Keep track of the index range.
		min = MIN(min, idx);
		max = MAX(max, idx);
	}

	if (pmin)
		*pmin = min;
	if (pmax)
		*pmax = max;

	return 0;
}

#ifndef LELY_NO_CO_DCF
LELY_CO_EXPORT int
co_dev_read_dcf_file(co_dev_t *dev, co_unsigned16_t *pmin,
		co_unsigned16_t *pmax, const char *filename)
{
	errc_t errc = 0;

	frbuf_t *buf = frbuf_create(filename);
	if (__unlikely(!buf)) {
		errc = get_errc();
		goto error_create_buf;
	}

	int64_t size = frbuf_get_size(buf);
	if (size == -1) {
		errc = get_errc();
		goto error_get_size;
	}

	void *dom = NULL;
	if (__unlikely(co_val_init_dom(&dom, NULL, size) == -1)) {
		errc = get_errc();
		goto error_init_dom;
	}

	if (__unlikely(frbuf_read(buf, dom, size) != size)) {
		errc = get_errc();
		goto error_read;
	}

	if (co_dev_read_dcf(dev, pmin, pmax, &dom) == -1) {
		errc = get_errc();
		goto error_read_dcf;
	}

	co_val_fini(CO_DEFTYPE_DOMAIN, &dom);
	frbuf_destroy(buf);

	return 0;

error_read_dcf:
error_read:
	co_val_fini(CO_DEFTYPE_DOMAIN, &dom);
error_init_dom:
error_get_size:
	frbuf_destroy(buf);
error_create_buf:
	set_errc(errc);
	return -1;
}
#endif

LELY_CO_EXPORT int
co_dev_write_dcf(const co_dev_t *dev, co_unsigned16_t min, co_unsigned16_t max,
		void **ptr)
{
	assert(dev);
	assert(ptr);

	errc_t errc = 0;

	// Get the list of object indices.
	co_unsigned16_t maxidx = co_dev_get_idx(dev, 0, NULL);
	co_unsigned16_t *idx = calloc(maxidx, sizeof(co_unsigned16_t));
	if (__unlikely(!idx)) {
		errc = errno2c(errno);
		goto error_malloc_idx;
	}
	maxidx = co_dev_get_idx(dev, maxidx, idx);

	size_t size = 4;
	co_unsigned32_t n = 0;

	for (size_t i = 0; i < maxidx; i++) {
		if (idx[i] < min || idx[i] > max)
			continue;

		// Get the list of object sub-indices.
		co_obj_t *obj = co_dev_find_obj(dev, idx[i]);
		co_unsigned8_t subidx[0xff];
		co_unsigned8_t maxsubidx = co_obj_get_subidx(obj, 0xff, subidx);

		// Count the number of sub-objects and compute the size (in
		// bytes).
		for (size_t j = 0; j < maxsubidx; j++, n++)
			size += co_dev_write_sub(dev, idx[i], subidx[j], NULL,
					NULL);
	}

	// Create a DOMAIN for the concise DCF.
	if (__unlikely(co_val_init_dom(ptr, NULL, size) == -1)) {
		errc = get_errc();
		goto error_init_dom;
	}
	uint8_t *begin = *ptr;
	uint8_t *end = begin + size;

	// Write the total number of sub-indices.
	begin += co_val_write(CO_DEFTYPE_UNSIGNED32, &n, begin, end);

	for (size_t i = 0; i < maxidx; i++) {
		if (idx[i] < min || idx[i] > max)
			continue;

		// Get the list of object sub-indices.
		co_obj_t *obj = co_dev_find_obj(dev, idx[i]);
		co_unsigned8_t subidx[0xff];
		co_unsigned8_t maxsubidx = co_obj_get_subidx(obj, 0xff, subidx);

		// Write the value of the sub-object.
		for (size_t j = 0; j < maxsubidx; j++, n++)
			begin += co_dev_write_sub(dev, idx[i], subidx[j], begin,
					end);
	}

	free(idx);

	return 0;

error_init_dom:
	free(idx);
error_malloc_idx:
	set_errc(errc);
	return -1;
}

#ifndef LELY_NO_CO_DCF
LELY_CO_EXPORT int
co_dev_write_dcf_file(const co_dev_t *dev, co_unsigned16_t min,
		co_unsigned16_t max, const char *filename)
{
	errc_t errc = 0;

	void *dom = NULL;
	if (__unlikely(co_dev_write_dcf(dev, min, max, &dom) == -1)) {
		errc = get_errc();
		goto error_write_dcf;
	}

	fwbuf_t *buf = fwbuf_create(filename);
	if (__unlikely(!buf)) {
		errc = get_errc();
		goto error_create_buf;
	}

	size_t nbyte = co_val_sizeof(CO_DEFTYPE_DOMAIN, &dom);
	if (__unlikely(fwbuf_write(buf, dom, nbyte) != (ssize_t)nbyte)) {
		errc = get_errc();
		goto error_write;
	}

	if (__unlikely(fwbuf_commit(buf) == -1)) {
		errc = get_errc();
		goto error_commit;
	}

	fwbuf_destroy(buf);
	co_val_fini(CO_DEFTYPE_DOMAIN, &dom);

	return 0;

error_commit:
error_write:
	fwbuf_destroy(buf);
error_create_buf:
	co_val_fini(CO_DEFTYPE_DOMAIN, &dom);
error_write_dcf:
	set_errc(errc);
	return -1;
}
#endif

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
#ifndef LELY_NO_CO_OBJ_LIMITS
	if (flags & CO_OBJ_FLAGS_MIN_NODEID)
		co_val_set_id(type, &sub->min, new_id, old_id);
	if (flags & CO_OBJ_FLAGS_MAX_NODEID)
		co_val_set_id(type, &sub->max, new_id, old_id);
#endif
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

