/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the device description.
 *
 * @see lely/co/dev.h
 *
 * @copyright 2021 Lely Industries N.V.
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
#include <lely/co/detail/dev.h>
#include <lely/co/detail/obj.h>
#include <lely/util/cmp.h>
#include <lely/util/diag.h>
#if !LELY_NO_CO_TPDO
#include <lely/co/pdo.h>
#endif

#include <assert.h>
#if !LELY_NO_MALLOC
#include <stdlib.h>
#endif

static void co_obj_set_id(
		co_obj_t *obj, co_unsigned8_t new_id, co_unsigned8_t old_id);
static void co_sub_set_id(
		co_sub_t *sub, co_unsigned8_t new_id, co_unsigned8_t old_id);
static void co_val_set_id(co_unsigned16_t type, void *val,
		co_unsigned8_t new_id, co_unsigned8_t old_id);

#if !LELY_NO_MALLOC

void *
__co_dev_alloc(void)
{
	void *ptr = malloc(sizeof(struct __co_dev));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
__co_dev_free(void *ptr)
{
	free(ptr);
}

#endif // !LELY_NO_MALLOC

struct __co_dev *
__co_dev_init(struct __co_dev *dev, co_unsigned8_t id)
{
	assert(dev);

	dev->netid = 0;

	if (!id || (id > CO_NUM_NODES && id != 0xff)) {
		set_errnum(ERRNUM_INVAL);
		return NULL;
	}
	dev->id = id;

	rbtree_init(&dev->tree, &uint16_cmp);

#if !LELY_NO_CO_OBJ_NAME
	dev->name = NULL;

	dev->vendor_name = NULL;
#endif
	dev->vendor_id = 0;
#if !LELY_NO_CO_OBJ_NAME
	dev->product_name = NULL;
#endif
	dev->product_code = 0;
	dev->revision = 0;
#if !LELY_NO_CO_OBJ_NAME
	dev->order_code = NULL;
#endif

	dev->baud = 0;
	dev->rate = 0;

	dev->lss = 0;

	dev->dummy = 0;

#if !LELY_NO_CO_TPDO
	dev->tpdo_event_ind = NULL;
	dev->tpdo_event_data = NULL;
#if !LELY_NO_CO_MPDO
	dev->sam_mpdo_event_ind = NULL;
	dev->sam_mpdo_event_data = NULL;
#endif
#endif // !LELY_NO_CO_TPDO

	return dev;
}

void
__co_dev_fini(struct __co_dev *dev)
{
	assert(dev);
#if LELY_NO_MALLOC
	(void)dev;
#else
	rbtree_foreach (&dev->tree, node)
		co_obj_destroy(structof(node, co_obj_t, node));

#if !LELY_NO_CO_OBJ_NAME
	free(dev->vendor_name);
	free(dev->product_name);
	free(dev->order_code);

	free(dev->name);
#endif
#endif // LELY_NO_MALLOC
}

#if !LELY_NO_MALLOC

co_dev_t *
co_dev_create(co_unsigned8_t id)
{
	int errc = 0;

	co_dev_t *dev = __co_dev_alloc();
	if (!dev) {
		errc = get_errc();
		goto error_alloc_dev;
	}

	if (!__co_dev_init(dev, id)) {
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

void
co_dev_destroy(co_dev_t *dev)
{
	if (dev) {
		__co_dev_fini(dev);
		__co_dev_free(dev);
	}
}

#endif // !LELY_NO_MALLOC

co_unsigned8_t
co_dev_get_netid(const co_dev_t *dev)
{
	assert(dev);

	return dev->netid;
}

int
co_dev_set_netid(co_dev_t *dev, co_unsigned8_t id)
{
	assert(dev);

	if (id > CO_NUM_NETWORKS && id != 0xff) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	dev->netid = id;

	return 0;
}

co_unsigned8_t
co_dev_get_id(const co_dev_t *dev)
{
	assert(dev);

	return dev->id;
}

int
co_dev_set_id(co_dev_t *dev, co_unsigned8_t id)
{
	assert(dev);

	if (!id || (id > CO_NUM_NODES && id != 0xff)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	rbtree_foreach (&dev->tree, node)
		co_obj_set_id(structof(node, co_obj_t, node), id, dev->id);

	dev->id = id;

	return 0;
}

co_unsigned16_t
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

int
co_dev_insert_obj(co_dev_t *dev, co_obj_t *obj)
{
	assert(dev);
	assert(obj);

	if (obj->dev && obj->dev != dev)
		return -1;

	if (obj->dev == dev)
		return 0;

	if (rbtree_find(&dev->tree, obj->node.key))
		return -1;

	obj->dev = dev;
	rbtree_insert(&obj->dev->tree, &obj->node);

	return 0;
}

int
co_dev_remove_obj(co_dev_t *dev, co_obj_t *obj)
{
	assert(dev);
	assert(obj);

	if (obj->dev != dev)
		return -1;

	rbtree_remove(&obj->dev->tree, &obj->node);
	rbnode_init(&obj->node, &obj->idx);
	obj->dev = NULL;

	return 0;
}

co_obj_t *
co_dev_find_obj(const co_dev_t *dev, co_unsigned16_t idx)
{
	assert(dev);

	struct rbnode *node = rbtree_find(&dev->tree, &idx);
	if (!node)
		return NULL;
	return structof(node, co_obj_t, node);
}

co_sub_t *
co_dev_find_sub(const co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx)
{
	co_obj_t *obj = co_dev_find_obj(dev, idx);
	return obj ? co_obj_find_sub(obj, subidx) : NULL;
}

co_obj_t *
co_dev_first_obj(const co_dev_t *dev)
{
	assert(dev);

	struct rbnode *node = rbtree_first(&dev->tree);
	return node ? structof(node, co_obj_t, node) : NULL;
}

co_obj_t *
co_dev_last_obj(const co_dev_t *dev)
{
	assert(dev);

	struct rbnode *node = rbtree_last(&dev->tree);
	return node ? structof(node, co_obj_t, node) : NULL;
}

#if !LELY_NO_CO_OBJ_NAME

const char *
co_dev_get_name(const co_dev_t *dev)
{
	assert(dev);

	return dev->name;
}

int
co_dev_set_name(co_dev_t *dev, const char *name)
{
	assert(dev);

	if (!name || !*name) {
		free(dev->name);
		dev->name = NULL;
		return 0;
	}

	void *ptr = realloc(dev->name, strlen(name) + 1);
	if (!ptr) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return -1;
	}
	dev->name = ptr;
	strcpy(dev->name, name);

	return 0;
}

const char *
co_dev_get_vendor_name(const co_dev_t *dev)
{
	assert(dev);

	return dev->vendor_name;
}

int
co_dev_set_vendor_name(co_dev_t *dev, const char *vendor_name)
{
	assert(dev);

	if (!vendor_name || !*vendor_name) {
		free(dev->vendor_name);
		dev->vendor_name = NULL;
		return 0;
	}

	void *ptr = realloc(dev->vendor_name, strlen(vendor_name) + 1);
	if (!ptr) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return -1;
	}
	dev->vendor_name = ptr;
	strcpy(dev->vendor_name, vendor_name);

	return 0;
}

#endif // !LELY_NO_CO_OBJ_NAME

co_unsigned32_t
co_dev_get_vendor_id(const co_dev_t *dev)
{
	assert(dev);

	return dev->vendor_id;
}

void
co_dev_set_vendor_id(co_dev_t *dev, co_unsigned32_t vendor_id)
{
	assert(dev);

	dev->vendor_id = vendor_id;
}

#if !LELY_NO_CO_OBJ_NAME

const char *
co_dev_get_product_name(const co_dev_t *dev)
{
	assert(dev);

	return dev->product_name;
}

int
co_dev_set_product_name(co_dev_t *dev, const char *product_name)
{
	assert(dev);

	if (!product_name || !*product_name) {
		free(dev->product_name);
		dev->product_name = NULL;
		return 0;
	}

	void *ptr = realloc(dev->product_name, strlen(product_name) + 1);
	if (!ptr) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return -1;
	}
	dev->product_name = ptr;
	strcpy(dev->product_name, product_name);

	return 0;
}

#endif // !LELY_NO_CO_OBJ_NAME

co_unsigned32_t
co_dev_get_product_code(const co_dev_t *dev)
{
	assert(dev);

	return dev->product_code;
}

void
co_dev_set_product_code(co_dev_t *dev, co_unsigned32_t product_code)
{
	assert(dev);

	dev->product_code = product_code;
}

co_unsigned32_t
co_dev_get_revision(const co_dev_t *dev)
{
	assert(dev);

	return dev->revision;
}

void
co_dev_set_revision(co_dev_t *dev, co_unsigned32_t revision)
{
	assert(dev);

	dev->revision = revision;
}

#if !LELY_NO_CO_OBJ_NAME

const char *
co_dev_get_order_code(const co_dev_t *dev)
{
	assert(dev);

	return dev->order_code;
}

int
co_dev_set_order_code(co_dev_t *dev, const char *order_code)
{
	assert(dev);

	if (!order_code || !*order_code) {
		free(dev->order_code);
		dev->order_code = NULL;
		return 0;
	}

	void *ptr = realloc(dev->order_code, strlen(order_code) + 1);
	if (!ptr) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return -1;
	}
	dev->order_code = ptr;
	strcpy(dev->order_code, order_code);

	return 0;
}

#endif // !LELY_NO_CO_OBJ_NAME

unsigned int
co_dev_get_baud(const co_dev_t *dev)
{
	assert(dev);

	return dev->baud;
}

void
co_dev_set_baud(co_dev_t *dev, unsigned int baud)
{
	assert(dev);

	dev->baud = baud;
}

co_unsigned16_t
co_dev_get_rate(const co_dev_t *dev)
{
	assert(dev);

	return dev->rate;
}

void
co_dev_set_rate(co_dev_t *dev, co_unsigned16_t rate)
{
	assert(dev);

	dev->rate = rate;
}

int
co_dev_get_lss(const co_dev_t *dev)
{
	assert(dev);

	return dev->lss;
}

void
co_dev_set_lss(co_dev_t *dev, int lss)
{
	assert(dev);

	dev->lss = !!lss;
}

co_unsigned32_t
co_dev_get_dummy(const co_dev_t *dev)
{
	assert(dev);

	return dev->dummy;
}

void
co_dev_set_dummy(co_dev_t *dev, co_unsigned32_t dummy)
{
	assert(dev);

	dev->dummy = dummy;
}

const void *
co_dev_get_val(const co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx)
{
	co_sub_t *sub = dev ? co_dev_find_sub(dev, idx, subidx) : NULL;
	return co_sub_get_val(sub);
}

size_t
co_dev_set_val(co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx,
		const void *ptr, size_t n)
{
	assert(dev);

	co_sub_t *sub = co_dev_find_sub(dev, idx, subidx);
	if (!sub) {
		set_errnum(ERRNUM_INVAL);
		return 0;
	}

	return co_sub_set_val(sub, ptr, n);
}

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
	co_##b##_t co_dev_get_val_##c(const co_dev_t *dev, \
			co_unsigned16_t idx, co_unsigned8_t subidx) \
	{ \
		/* clang-format off */ \
		co_sub_t *sub = dev \
				? co_dev_find_sub(dev, idx, subidx) \
				: NULL; \
		/* clang-format on */ \
		return co_sub_get_val_##c(sub); \
	} \
\
	size_t co_dev_set_val_##c(co_dev_t *dev, co_unsigned16_t idx, \
			co_unsigned8_t subidx, co_##b##_t c) \
	{ \
		assert(dev); \
\
		co_sub_t *sub = co_dev_find_sub(dev, idx, subidx); \
		if (!sub) { \
			set_errnum(ERRNUM_INVAL); \
			return 0; \
		} \
\
		return co_sub_set_val_##c(sub, c); \
	}
#include <lely/co/def/basic.def>
#undef LELY_CO_DEFINE_TYPE

size_t
co_dev_read_sub(co_dev_t *dev, co_unsigned16_t *pidx, co_unsigned8_t *psubidx,
		const uint_least8_t *begin, const uint_least8_t *end)
{
	if (!begin || !end || end - begin < 2 + 1 + 4)
		return 0;

	// Read the object index.
	co_unsigned16_t idx;
	if (co_val_read(CO_DEFTYPE_UNSIGNED16, &idx, begin, end) != 2)
		return 0;
	begin += 2;
	// Read the object sub-index.
	co_unsigned8_t subidx;
	if (co_val_read(CO_DEFTYPE_UNSIGNED8, &subidx, begin, end) != 1)
		return 0;
	begin += 1;
	// Read the value size (in bytes).
	co_unsigned32_t size;
	if (co_val_read(CO_DEFTYPE_UNSIGNED32, &size, begin, end) != 4)
		return 0;
	begin += 4;

	if (end - begin < (ptrdiff_t)size)
		return 0;

	// Read the value into the sub-object, if it exists.
	co_sub_t *sub = co_dev_find_sub(dev, idx, subidx);
	if (sub) {
		co_unsigned16_t type = co_sub_get_type(sub);
		union co_val val;
#if LELY_NO_MALLOC
		struct co_array array = CO_ARRAY_INIT;
		if (co_type_is_array(type))
			co_val_init_array(&val, &array);
		else
#endif
			co_val_init(type, &val);
		if (co_val_read(type, &val, begin, begin + size) == size)
			co_sub_set_val(sub, co_val_addressof(type, &val),
					co_val_sizeof(type, &val));
#if !LELY_NO_MALLOC
		co_val_fini(type, &val);
#endif
	}

	if (pidx)
		*pidx = idx;
	if (psubidx)
		*psubidx = subidx;

	return 2 + 1 + 4 + size;
}

size_t
co_dev_write_sub(const co_dev_t *dev, co_unsigned16_t idx,
		co_unsigned8_t subidx, uint_least8_t *begin, uint_least8_t *end)
{
	co_sub_t *sub = co_dev_find_sub(dev, idx, subidx);
	if (!sub)
		return 0;
	co_unsigned16_t type = co_sub_get_type(sub);
	const void *val = co_sub_get_val(sub);

	co_unsigned32_t size = co_val_write(type, val, NULL, NULL);
	if (!size && co_val_sizeof(type, val))
		return 0;

	if (begin && (!end || end - begin >= (ptrdiff_t)(2 + 1 + 4 + size))) {
		// Write the object index.
		if (co_val_write(CO_DEFTYPE_UNSIGNED16, &idx, begin, end) != 2)
			return 0;
		begin += 2;
		// Write the object sub-index.
		if (co_val_write(CO_DEFTYPE_UNSIGNED8, &subidx, begin, end)
				!= 1)
			return 0;
		begin += 1;
		// Write the value size (in bytes).
		if (co_val_write(CO_DEFTYPE_UNSIGNED32, &size, begin, end) != 4)
			return 0;
		begin += 4;
		// Write the value.
		if (co_val_write(type, val, begin, end) != size)
			return 0;
	}

	return 2 + 1 + 4 + size;
}

int
co_dev_read_dcf(co_dev_t *dev, co_unsigned16_t *pmin, co_unsigned16_t *pmax,
		void *const *ptr)
{
	assert(dev);
	assert(ptr);

	co_unsigned16_t min = CO_UNSIGNED16_MAX;
	co_unsigned16_t max = CO_UNSIGNED16_MIN;

	size_t size = co_val_sizeof(CO_DEFTYPE_DOMAIN, ptr);
	const uint_least8_t *begin = *ptr;
	const uint_least8_t *end = begin + size;

	// Read the total number of sub-indices.
	co_unsigned32_t n;
	size = co_val_read(CO_DEFTYPE_UNSIGNED32, &n, begin, end);
	if (size != 4)
		return -1;
	begin += size;

	for (size_t i = 0; i < n; i++) {
		// Read the value of the sub-object.
		co_unsigned16_t idx;
		size = co_dev_read_sub(dev, &idx, NULL, begin, end);
		if (!size)
			return -1;
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

#if !LELY_NO_STDIO
int
co_dev_read_dcf_file(co_dev_t *dev, co_unsigned16_t *pmin,
		co_unsigned16_t *pmax, const char *filename)
{
	void *dom = NULL;
	if (!co_val_read_file(CO_DEFTYPE_DOMAIN, &dom, filename))
		return -1;

	if (co_dev_read_dcf(dev, pmin, pmax, &dom) == -1) {
		co_val_fini(CO_DEFTYPE_DOMAIN, &dom);
		return -1;
	}

	co_val_fini(CO_DEFTYPE_DOMAIN, &dom);
	return 0;
}
#endif

int
co_dev_write_dcf(const co_dev_t *dev, co_unsigned16_t min, co_unsigned16_t max,
		void **ptr)
{
	assert(dev);
	assert(ptr);

	size_t size = 4;
	co_unsigned32_t n = 0;

	// Count the number of matching sub-objects and compute the total size
	// (in bytes).
	for (co_obj_t *obj = co_dev_first_obj(dev); obj;
			obj = co_obj_next(obj)) {
		co_unsigned16_t idx = co_obj_get_idx(obj);
		if (idx < min)
			continue;
		if (idx > max)
			break;
		for (co_sub_t *sub = co_obj_first_sub(obj); sub;
				sub = co_sub_next(sub)) {
			co_unsigned8_t subidx = co_sub_get_subidx(sub);
			size += co_dev_write_sub(dev, idx, subidx, NULL, NULL);
			n++;
		}
	}

	// Create a DOMAIN for the concise DCF.
	if (co_val_init_dom(ptr, NULL, size) == -1)
		return -1;

	uint_least8_t *begin = *ptr;
	uint_least8_t *end = begin + size;

	// Write the total number of sub-indices.
	begin += co_val_write(CO_DEFTYPE_UNSIGNED32, &n, begin, end);

	// Write the sub-objects.
	for (co_obj_t *obj = co_dev_first_obj(dev); obj;
			obj = co_obj_next(obj)) {
		co_unsigned16_t idx = co_obj_get_idx(obj);
		if (idx < min)
			continue;
		if (idx > max)
			break;
		for (co_sub_t *sub = co_obj_first_sub(obj); sub;
				sub = co_sub_next(sub)) {
			co_unsigned8_t subidx = co_sub_get_subidx(sub);
			begin += co_dev_write_sub(dev, idx, subidx, begin, end);
		}
	}

	return 0;
}

#if !LELY_NO_STDIO
int
co_dev_write_dcf_file(const co_dev_t *dev, co_unsigned16_t min,
		co_unsigned16_t max, const char *filename)
{
	void *dom = NULL;
	if (co_dev_write_dcf(dev, min, max, &dom) == -1)
		return -1;

	size_t nbyte = co_val_sizeof(CO_DEFTYPE_DOMAIN, &dom);
	if (co_val_write_file(CO_DEFTYPE_DOMAIN, &dom, filename) != nbyte) {
		co_val_fini(CO_DEFTYPE_DOMAIN, &dom);
		return -1;
	}

	co_val_fini(CO_DEFTYPE_DOMAIN, &dom);
	return 0;
}
#endif

#if !LELY_NO_CO_TPDO

void
co_dev_get_tpdo_event_ind(const co_dev_t *dev, co_dev_tpdo_event_ind_t **pind,
		void **pdata)
{
	assert(dev);

	if (pind)
		*pind = dev->tpdo_event_ind;
	if (pdata)
		*pdata = dev->tpdo_event_data;
}

void
co_dev_set_tpdo_event_ind(
		co_dev_t *dev, co_dev_tpdo_event_ind_t *ind, void *data)
{
	assert(dev);

	dev->tpdo_event_ind = ind;
	dev->tpdo_event_data = data;
}

void
co_dev_tpdo_event(co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx)
{
	assert(dev);

	// Check if the specified sub-object can be mapped into a TPDO.
	if (co_dev_chk_tpdo(dev, idx, subidx))
		return;

	const co_obj_t *obj_1800 = NULL;
	// Find the first TPDO.
	for (co_unsigned16_t i = 0; i < CO_NUM_PDOS && !obj_1800; i++)
		obj_1800 = co_dev_find_obj(dev, 0x1800 + i);
	for (; obj_1800; obj_1800 = co_obj_next(obj_1800)) {
		co_unsigned16_t i = co_obj_get_idx(obj_1800) - 0x1800;
		if (i >= CO_NUM_PDOS)
			break;
		// Check if this is a valid acyclic or event-driven PDO.
		const struct co_pdo_comm_par *comm =
				co_obj_addressof_val(obj_1800);
		assert(comm);
		if (comm->n < 2 || (comm->cobid & CO_PDO_COBID_VALID)
				|| !(!comm->trans || comm->trans >= 0xfe))
			continue;
		// Check if the sub-object is mapped into this PDO.
		const co_obj_t *obj_1a00 = co_dev_find_obj(dev, 0x1a00 + i);
		if (!obj_1a00)
			continue;
		const struct co_pdo_map_par *map =
				co_obj_addressof_val(obj_1a00);
		assert(map);
		// Check whether this is an MPDO.
		if (map->n > 0x40)
			continue;
		for (size_t j = 0; j < map->n; j++) {
			if (((map->map[j] >> 16) & 0xffff) != idx)
				continue;
			if (((map->map[j] >> 8) & 0xff) != subidx)
				continue;
			// Issue a single indication for this PDO.
			if (dev->tpdo_event_ind)
				dev->tpdo_event_ind(
						i + 1, dev->tpdo_event_data);
			break;
		}
	}
}

#if !LELY_NO_CO_MPDO

void
co_dev_get_sam_mpdo_event_ind(const co_dev_t *dev,
		co_dev_sam_mpdo_event_ind_t **pind, void **pdata)
{
	assert(dev);

	if (pind)
		*pind = dev->sam_mpdo_event_ind;
	if (pdata)
		*pdata = dev->sam_mpdo_event_data;
}

void
co_dev_set_sam_mpdo_event_ind(
		co_dev_t *dev, co_dev_sam_mpdo_event_ind_t *ind, void *data)
{
	assert(dev);

	dev->sam_mpdo_event_ind = ind;
	dev->sam_mpdo_event_data = data;
}

void
co_dev_sam_mpdo_event(co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx)
{
	assert(dev);

	// Check whether the sub-object can be mapped into a SAM-MPDO.
	if (!co_dev_chk_sam_mpdo(dev, idx, subidx))
		return;

	// Check if the specified sub-object can be mapped into a TPDO.
	if (co_dev_chk_tpdo(dev, idx, subidx))
		return;

	const co_obj_t *obj_1800 = NULL;
	// Find the first TPDO.
	for (co_unsigned16_t i = 0; i < CO_NUM_PDOS && !obj_1800; i++)
		obj_1800 = co_dev_find_obj(dev, 0x1800 + i);
	for (; obj_1800; obj_1800 = co_obj_next(obj_1800)) {
		co_unsigned16_t i = co_obj_get_idx(obj_1800) - 0x1800;
		if (i >= CO_NUM_PDOS)
			break;
		// Check if this is a valid event-driven PDO.
		const struct co_pdo_comm_par *comm =
				co_obj_addressof_val(obj_1800);
		assert(comm);
		if (comm->n < 2 || (comm->cobid & CO_PDO_COBID_VALID)
				|| comm->trans < 0xfe)
			continue;
		// Check if this is a SAM-MPDO.
		const co_obj_t *obj_1a00 = co_dev_find_obj(dev, 0x1a00 + i);
		if (!obj_1a00)
			continue;
		const struct co_pdo_map_par *map =
				co_obj_addressof_val(obj_1a00);
		assert(map);
		if (map->n != CO_PDO_MAP_SAM_MPDO)
			continue;
		// Issue an indication for this PDO.
		if (dev->sam_mpdo_event_ind)
			dev->sam_mpdo_event_ind(i + 1, idx, subidx,
					dev->sam_mpdo_event_data);
		// Only a single SAM-MPDO producer per CANopen device is
		// allowed.
		break;
	}
}

#endif // !LELY_NO_CO_MPDO

#endif // !LELY_NO_CO_TPDO

static void
co_obj_set_id(co_obj_t *obj, co_unsigned8_t new_id, co_unsigned8_t old_id)
{
	assert(obj);

	rbtree_foreach (&obj->tree, node)
		co_sub_set_id(structof(node, co_sub_t, node), new_id, old_id);
}

static void
co_sub_set_id(co_sub_t *sub, co_unsigned8_t new_id, co_unsigned8_t old_id)
{
	assert(sub);

	unsigned int flags = co_sub_get_flags(sub);
	co_unsigned16_t type = co_sub_get_type(sub);
#if !LELY_NO_CO_OBJ_LIMITS
	if (flags & CO_OBJ_FLAGS_MIN_NODEID)
		co_val_set_id(type, &sub->min, new_id, old_id);
	if (flags & CO_OBJ_FLAGS_MAX_NODEID)
		co_val_set_id(type, &sub->max, new_id, old_id);
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
	if (flags & CO_OBJ_FLAGS_DEF_NODEID)
		co_val_set_id(type, &sub->def, new_id, old_id);
#endif
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
