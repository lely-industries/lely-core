/*!\file
 * This file is part of the CANopen library; it contains the implementation of
 * the static device description functions.
 *
 * \see lely/co/sdev.h
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

#ifndef LELY_NO_CO_SDEV

#include <lely/util/errnum.h>
#include <lely/util/lex.h>
#include <lely/util/print.h>
#include <lely/co/sdev.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
// Include inttypes.h after stdio.h to enforce declarations of format
// specifiers in Newlib.
#include <inttypes.h>

static int co_sdev_load(const struct co_sdev *sdev, co_dev_t *dev);
static int co_sobj_load(const struct co_sobj *sobj, co_obj_t *obj);
static int co_ssub_load(const struct co_ssub *ssub, co_sub_t *sub);

static int snprintf_c99_sobj(char *s, size_t n, const co_obj_t *obj);
static int snprintf_c99_ssub(char *s, size_t n, const co_sub_t *sub);
static int snprintf_c99_sval(char *s, size_t n, co_unsigned16_t type,
		const void *val);
static int snprintf_c99_esc(char *s, size_t n, const char *esc);

LELY_CO_EXPORT struct __co_dev *
__co_dev_init_from_sdev(struct __co_dev *dev, const struct co_sdev *sdev)
{
	assert(dev);

	errc_t errc = 0;

	if (__unlikely(!sdev)) {
		errc = errnum2c(ERRNUM_INVAL);
		goto error_param;
	}

	if (__unlikely(!__co_dev_init(dev, sdev->id))) {
		errc = get_errc();
		goto error_init_dev;
	}

	if (__unlikely(co_sdev_load(sdev, dev) == -1)) {
		errc = get_errc();
		goto error_load_sdev;
	}

	return dev;

error_load_sdev:
	__co_dev_fini(dev);
error_init_dev:
error_param:
	set_errc(errc);
	return NULL;
}

LELY_CO_EXPORT co_dev_t *
co_dev_create_from_sdev(const struct co_sdev *sdev)
{
	errc_t errc = 0;

	co_dev_t *dev = __co_dev_alloc();
	if (__unlikely(!dev)) {
		errc = get_errc();
		goto error_alloc_dev;
	}

	if (__unlikely(!__co_dev_init_from_sdev(dev, sdev))) {
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

LELY_CO_EXPORT int
snprintf_c99_sdev(char *s, size_t n, const co_dev_t *dev)
{
	if (!s)
		n = 0;

	if (__unlikely(!dev))
		return 0;

	errc_t errc = 0;

	int r, t = 0;
	const char *name;

	r = snprintf(s, n, "{\n\t.id = 0x%02x,\n", co_dev_get_id(dev));
	if (__unlikely(r < 0)) {
		errc = get_errc();
		goto error_print_dev;
	}
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	name = co_dev_get_name(dev);
	if (name) {
		r = snprintf(s, n, "\t.name = CO_SDEV_STRING(\"");
		if (__unlikely(r < 0)) {
			errc = get_errc();
			goto error_print_dev;
		}
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		r = snprintf_c99_esc(s, n, name);
		if (__unlikely(r < 0)) {
			errc = get_errc();
			goto error_print_dev;
		}
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		r = snprintf(s, n, "\"),\n");
	} else {
		r = snprintf(s, n, "\t.name = NULL,\n");
	}
	if (__unlikely(r < 0)) {
		errc = get_errc();
		goto error_print_dev;
	}
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	name = co_dev_get_vendor_name(dev);
	if (name) {
		r = snprintf(s, n, "\t.vendor_name = CO_SDEV_STRING(\"");
		if (__unlikely(r < 0)) {
			errc = get_errc();
			goto error_print_dev;
		}
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		r = snprintf_c99_esc(s, n, name);
		if (__unlikely(r < 0)) {
			errc = get_errc();
			goto error_print_dev;
		}
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		r = snprintf(s, n, "\"),\n");
	} else {
		r = snprintf(s, n, "\t.vendor_name = NULL,\n");
	}
	if (__unlikely(r < 0)) {
		errc = get_errc();
		goto error_print_dev;
	}
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	r = snprintf(s, n, "\t.vendor_id = 0x%08" PRIx32 ",\n",
			co_dev_get_vendor_id(dev));
	if (__unlikely(r < 0)) {
		errc = get_errc();
		goto error_print_dev;
	}
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	name = co_dev_get_product_name(dev);
	if (name) {
		r = snprintf(s, n, "\t.product_name = CO_SDEV_STRING(\"");
		if (__unlikely(r < 0)) {
			errc = get_errc();
			goto error_print_dev;
		}
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		r = snprintf_c99_esc(s, n, name);
		if (__unlikely(r < 0)) {
			errc = get_errc();
			goto error_print_dev;
		}
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		r = snprintf(s, n, "\"),\n");
	} else {
		r = snprintf(s, n, "\t.product_name = NULL,\n");
	}
	if (__unlikely(r < 0)) {
		errc = get_errc();
		goto error_print_dev;
	}
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	r = snprintf(s, n, "\t.product_code = 0x%08" PRIx32 ",\n\t.revision = 0x%08" PRIx32 ",\n",
			co_dev_get_product_code(dev), co_dev_get_revision(dev));
	if (__unlikely(r < 0)) {
		errc = get_errc();
		goto error_print_dev;
	}
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	name = co_dev_get_order_code(dev);
	if (name) {
		r = snprintf(s, n, "\t.order_code = CO_SDEV_STRING(\"");
		if (__unlikely(r < 0)) {
			errc = get_errc();
			goto error_print_dev;
		}
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		r = snprintf_c99_esc(s, n, name);
		if (__unlikely(r < 0)) {
			errc = get_errc();
			goto error_print_dev;
		}
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		r = snprintf(s, n, "\"),\n");
	} else {
		r = snprintf(s, n, "\t.order_code = NULL,\n");
	}
	if (__unlikely(r < 0)) {
		errc = get_errc();
		goto error_print_dev;
	}
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	r = snprintf(s, n, "\t.baud = 0");
	if (__unlikely(r < 0)) {
		errc = get_errc();
		goto error_print_dev;
	}
	t += r; r = MIN((size_t)r, n); s += r; n -= r;
	unsigned int baud = co_dev_get_baud(dev);
#define LELY_CO_DEFINE_BAUD(x) \
	if (baud & CO_BAUD_##x) { \
		r = snprintf(s, n, "\n\t\t| CO_BAUD_" #x); \
		if (__unlikely(r < 0)) { \
			errc = get_errc(); \
			goto error_print_dev; \
		} \
		t += r; r = MIN((size_t)r, n); s += r; n -= r; \
	}
LELY_CO_DEFINE_BAUD(1000)
LELY_CO_DEFINE_BAUD(800)
LELY_CO_DEFINE_BAUD(500)
LELY_CO_DEFINE_BAUD(250)
LELY_CO_DEFINE_BAUD(125)
LELY_CO_DEFINE_BAUD(50)
LELY_CO_DEFINE_BAUD(20)
LELY_CO_DEFINE_BAUD(10)
LELY_CO_DEFINE_BAUD(AUTO)
#undef LELY_CO_DEFINE_BAUD

	r = snprintf(s, n, ",\n\t.rate = %d,\n\t.lss = %d,\n\t.dummy = 0x%08" PRIx32 ",\n",
			co_dev_get_rate(dev), co_dev_get_lss(dev),
			co_dev_get_dummy(dev));
	if (__unlikely(r < 0)) {
		errc = get_errc();
		goto error_print_dev;
	}
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	co_unsigned16_t maxidx = co_dev_get_idx(dev, 0, NULL);
	co_unsigned16_t *idx = malloc(maxidx * sizeof(co_unsigned16_t));
	if (__unlikely(!idx)) {
		errc = get_errc();
		goto error_malloc_idx;
	}
	co_dev_get_idx(dev, maxidx, idx);

	r = snprintf(s, n, "\t.nobj = %d,\n\t.objs = (const struct co_sobj[]){",
			maxidx);
	if (__unlikely(r < 0)) {
		errc = get_errc();
		goto error_print_obj;
	}
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	for (size_t i = 0; i < maxidx; i++) {
		r = snprintf(s, n, i ? ", {\n" : "{\n");
		if (__unlikely(r < 0)) {
			errc = get_errc();
			goto error_print_obj;
		}
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		r = snprintf_c99_sobj(s, n, co_dev_find_obj(dev, idx[i]));
		if (__unlikely(r < 0)) {
			errc = get_errc();
			goto error_print_obj;
		}
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		r = snprintf(s, n, "\t}");
		if (__unlikely(r < 0)) {
			errc = get_errc();
			goto error_print_obj;
		}
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
	}

	r = snprintf(s, n, "}\n}");
	if (__unlikely(r < 0)) {
		errc = get_errc();
		goto error_print_obj;
	}
	t += r;

	free(idx);

	return t;

error_print_obj:
	free(idx);
error_malloc_idx:
error_print_dev:
	set_errc(errc);
	return r;
}

static int
co_sdev_load(const struct co_sdev *sdev, co_dev_t *dev)
{
	assert(sdev);
	assert(dev);

	if (__unlikely(co_dev_set_name(dev, sdev->name) == -1))
		return -1;

	if (__unlikely(co_dev_set_vendor_name(dev, sdev->vendor_name) == -1))
		return -1;

	co_dev_set_vendor_id(dev, sdev->vendor_id);

	if (__unlikely(co_dev_set_product_name(dev, sdev->product_name) == -1))
		return -1;

	co_dev_set_product_code(dev, sdev->product_code);
	co_dev_set_revision(dev, sdev->revision);

	if (__unlikely(co_dev_set_order_code(dev, sdev->order_code) == -1))
		return -1;

	co_dev_set_baud(dev, sdev->baud);
	co_dev_set_rate(dev, sdev->rate);

	co_dev_set_lss(dev, sdev->lss);

	co_dev_set_dummy(dev, sdev->dummy);

	for (size_t i = 0; i < sdev->nobj; i++) {
		const struct co_sobj *sobj = &sdev->objs[i];
		co_obj_t *obj = co_obj_create(sobj->idx);
		if (__unlikely(!obj))
			return -1;
		if (__unlikely(co_dev_insert_obj(dev, obj) == -1)) {
			errc_t errc = get_errc();
			co_obj_destroy(obj);
			set_errc(errc);
			return -1;
		}
		if (__unlikely(co_sobj_load(sobj, obj) == -1))
			return -1;
	}

	return 0;
}

static int
co_sobj_load(const struct co_sobj *sobj, co_obj_t *obj)
{
	assert(sobj);
	assert(obj);

	if (__unlikely(co_obj_set_name(obj, sobj->name) == -1))
		return -1;

	if (__unlikely(co_obj_set_code(obj, sobj->code) == -1))
		return -1;

	for (size_t i = 0; i < sobj->nsub; i++) {
		const struct co_ssub *ssub = &sobj->subs[i];
		co_sub_t *sub = co_sub_create(ssub->subidx, ssub->type);
		if (__unlikely(!sub))
			return -1;
		if (__unlikely(co_obj_insert_sub(obj, sub) == -1)) {
			errc_t errc = get_errc();
			co_sub_destroy(sub);
			set_errc(errc);
			return -1;
		}
		if (__unlikely(co_ssub_load(ssub, sub) == -1))
			return -1;
	}

	return 0;
}

static int
co_ssub_load(const struct co_ssub *ssub, co_sub_t *sub)
{
	assert(ssub);
	assert(sub);

	if (__unlikely(co_sub_set_name(sub, ssub->name) == -1))
		return -1;

	if (__unlikely(co_sub_set_access(sub, ssub->access) == -1))
		return -1;

	const void *ptr;
	size_t n;

	ptr = co_val_addressof(ssub->type, &ssub->min);
	n = co_val_sizeof(ssub->type, &ssub->min);
	if (__unlikely(n && !co_sub_set_min(sub, ptr, n)))
		return -1;

	ptr = co_val_addressof(ssub->type, &ssub->max);
	n = co_val_sizeof(ssub->type, &ssub->max);
	if (__unlikely(n && !co_sub_set_max(sub, ptr, n)))
		return -1;

	ptr = co_val_addressof(ssub->type, &ssub->def);
	n = co_val_sizeof(ssub->type, &ssub->def);
	if (__unlikely(n && !co_sub_set_def(sub, ptr, n)))
		return -1;

	ptr = co_val_addressof(ssub->type, &ssub->val);
	n = co_val_sizeof(ssub->type, &ssub->val);
	if (__unlikely(n && !co_sub_set_val(sub, ptr, n)))
		return -1;

	co_sub_set_pdo_mapping(sub, ssub->pdo_mapping);
	co_sub_set_flags(sub, ssub->flags);

	return 0;
}

static int
snprintf_c99_sobj(char *s, size_t n, const co_obj_t *obj)
{
	if (!s)
		n = 0;

	if (__unlikely(!obj))
		return 0;

	int r, t = 0;

	const char *name = co_obj_get_name(obj);
	if (name) {
		r = snprintf(s, n, "\t\t.name = CO_SDEV_STRING(\"");
		if (__unlikely(r < 0))
			return r;
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		r = snprintf_c99_esc(s, n, name);
		if (__unlikely(r < 0))
			return r;
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		r = snprintf(s, n, "\"),\n");
	} else {
		r = snprintf(s, n, "\t\t.name = NULL,\n");
	}
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	r = snprintf(s, n, "\t\t.idx = 0x%04x,\n\t\t.code = ",
			co_obj_get_idx(obj));
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	co_unsigned8_t code = co_obj_get_code(obj);
	switch (code) {
	case CO_OBJECT_NULL:
		r = snprintf(s, n, "CO_OBJECT_NULL,\n");
		break;
	case CO_OBJECT_DOMAIN:
		r = snprintf(s, n, "CO_OBJECT_DOMAIN,\n");
		break;
	case CO_OBJECT_DEFTYPE:
		r = snprintf(s, n, "CO_OBJECT_DEFTYPE,\n");
		break;
	case CO_OBJECT_DEFSTRUCT:
		r = snprintf(s, n, "CO_OBJECT_DEFSTRUCT,\n");
		break;
	case CO_OBJECT_VAR:
		r = snprintf(s, n, "CO_OBJECT_VAR,\n");
		break;
	case CO_OBJECT_ARRAY:
		r = snprintf(s, n, "CO_OBJECT_ARRAY,\n");
		break;
	case CO_OBJECT_RECORD:
		r = snprintf(s, n, "CO_OBJECT_RECORD,\n");
		break;
	default:
		r = snprintf(s, n, "0x%02x,\n", code);
		break;
	}
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	co_unsigned8_t subidx[0xff];
	co_unsigned8_t maxsubidx = co_obj_get_subidx(obj, 0xff, subidx);

	r = snprintf(s, n, "\t\t.nsub = %d,\n\t\t.subs = (const struct co_ssub[]){",
			maxsubidx);
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	for (size_t i = 0; i < maxsubidx; i++) {
		r = snprintf(s, n, i ? ", {\n" : "{\n");
		if (__unlikely(r < 0))
			return r;
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		r = snprintf_c99_ssub(s, n, co_obj_find_sub(obj, subidx[i]));
		if (__unlikely(r < 0))
			return r;
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		r = snprintf(s, n, "\t\t}");
		if (__unlikely(r < 0))
			return r;
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
	}

	r = snprintf(s, n, "}\n");
	if (__unlikely(r < 0))
		return r;
	t += r;

	return t;
}

static int
snprintf_c99_ssub(char *s, size_t n, const co_sub_t *sub)
{
	if (!s)
		n = 0;

	if (__unlikely(!sub))
		return 0;

	int r, t = 0;

	const char *name = co_sub_get_name(sub);
	if (name) {
		r = snprintf(s, n, "\t\t\t.name = CO_SDEV_STRING(\"");
		if (__unlikely(r < 0))
			return r;
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		r = snprintf_c99_esc(s, n, name);
		if (__unlikely(r < 0))
			return r;
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
		r = snprintf(s, n, "\"),\n");
	} else {
		r = snprintf(s, n, "\t\t\t.name = NULL,\n");
	}
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	r = snprintf(s, n, "\t\t\t.subidx = 0x%02x,\n\t\t\t.type = ",
			co_sub_get_subidx(sub));
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	co_unsigned16_t type = co_sub_get_type(sub);
	switch (type) {
#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
	case CO_DEFTYPE_##a: \
		r = snprintf(s, n, "CO_DEFTYPE_" #a ",\n"); \
		break;
#include <lely/co/def/type.def>
#undef LELY_CO_DEFINE_TYPE
	default:
		r = snprintf(s, n, "0x%04x,\n", type);
		break;
	}
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	r = snprintf(s, n, "\t\t\t.min = ");
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;
	r = snprintf_c99_sval(s, n, type, co_sub_get_min(sub));
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	r = snprintf(s, n, ",\n\t\t\t.max = ");
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;
	r = snprintf_c99_sval(s, n, type, co_sub_get_max(sub));
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	r = snprintf(s, n, ",\n\t\t\t.def = ");
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;
	r = snprintf_c99_sval(s, n, type, co_sub_get_def(sub));
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	r = snprintf(s, n, ",\n\t\t\t.val = ");
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;
#ifndef LELY_NO_CO_OBJ_FILE
	if (type == CO_DEFTYPE_DOMAIN
			&& ((co_sub_get_flags(sub) & CO_OBJ_FLAGS_UPLOAD_FILE)
			|| (co_sub_get_flags(sub) & CO_OBJ_FLAGS_DOWNLOAD_FILE)))
		r = snprintf_c99_sval(s, n, CO_DEFTYPE_VISIBLE_STRING,
				co_sub_get_val(sub));
	else
#endif
		r = snprintf_c99_sval(s, n, type, co_sub_get_val(sub));
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	r = snprintf(s, n, ",\n\t\t\t.access = ");
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;
	unsigned int access = co_sub_get_access(sub);
	switch (access) {
	case CO_ACCESS_RO:
		r = snprintf(s, n, "CO_ACCESS_RO,\n");
		break;
	case CO_ACCESS_WO:
		r = snprintf(s, n, "CO_ACCESS_WO,\n");
		break;
	case CO_ACCESS_RW:
		r = snprintf(s, n, "CO_ACCESS_RW,\n");
		break;
	case CO_ACCESS_RWR:
		r = snprintf(s, n, "CO_ACCESS_RWR,\n");
		break;
	case CO_ACCESS_RWW:
		r = snprintf(s, n, "CO_ACCESS_RWW,\n");
		break;
	case CO_ACCESS_CONST:
		r = snprintf(s, n, "CO_ACCESS_CONST,\n");
		break;
	default:
		r = snprintf(s, n, "0x%x,\n", access);
		break;
	}
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	r = snprintf(s, n, "\t\t\t.pdo_mapping = %d,\n",
			co_sub_get_pdo_mapping(sub));
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;

	r = snprintf(s, n, "\t\t\t.flags = 0\n");
	if (__unlikely(r < 0))
		return r;
	t += r; r = MIN((size_t)r, n); s += r; n -= r;
	unsigned int flags = co_sub_get_flags(sub);
#define LELY_CO_DEFINE_FLAGS(x) \
	if (flags & CO_OBJ_FLAGS_##x) { \
		r = snprintf(s, n, "\t\t\t\t| CO_OBJ_FLAGS_" #x "\n"); \
		if (__unlikely(r < 0)) \
			return r; \
		t += r; r = MIN((size_t)r, n); s += r; n -= r; \
	}
LELY_CO_DEFINE_FLAGS(READ)
LELY_CO_DEFINE_FLAGS(WRITE)
#ifndef LELY_NO_CO_OBJ_FILE
LELY_CO_DEFINE_FLAGS(UPLOAD_FILE)
LELY_CO_DEFINE_FLAGS(DOWNLOAD_FILE)
#endif
LELY_CO_DEFINE_FLAGS(MIN_NODEID)
LELY_CO_DEFINE_FLAGS(MAX_NODEID)
LELY_CO_DEFINE_FLAGS(DEF_NODEID)
LELY_CO_DEFINE_FLAGS(VAL_NODEID)
#undef LELY_CO_DEFINE_FLAGS

	return t;
}

static int
snprintf_c99_sval(char *s, size_t n, co_unsigned16_t type, const void *val)
{
	if (!s)
		n = 0;

	if (__unlikely(!val))
		return 0;

	int r, t = 0;

	const union co_val *u = val;
	switch (type) {
	case CO_DEFTYPE_BOOLEAN:
		r = snprintf(s, n, "{ .b = %d }", !!u->b);
		break;
	case CO_DEFTYPE_INTEGER8:
		r = snprintf(s, n, "{ .i8 = %" PRIi8 " }", u->i8);
		break;
	case CO_DEFTYPE_INTEGER16:
		r = snprintf(s, n, "{ .i16 = %" PRIi16 " }", u->i16);
		break;
	case CO_DEFTYPE_INTEGER32:
		r = snprintf(s, n, "{ .i32 = %" PRIi32 "l }", u->i32);
		break;
	case CO_DEFTYPE_UNSIGNED8:
		r = snprintf(s, n, "{ .u8 = 0x%02" PRIx8 " }", u->u8);
		break;
	case CO_DEFTYPE_UNSIGNED16:
		r = snprintf(s, n, "{ .u16 = 0x%04" PRIx16 "u }", u->u16);
		break;
	case CO_DEFTYPE_UNSIGNED32:
		r = snprintf(s, n, "{ .u32 = 0x%08" PRIx32 "ul }", u->u32);
		break;
	case CO_DEFTYPE_REAL32:
		r = snprintf(s, n, "{ .r32 = %.*g }", DECIMAL_DIG,
				(double)u->r32);
		break;
	case CO_DEFTYPE_VISIBLE_STRING:
		if (u->vs) {
			r = snprintf(s, n, "{ .vs = CO_VISIBLE_STRING_C(\"");
			if (__unlikely(r < 0))
				return r;
			t += r; r = MIN((size_t)r, n); s += r; n -= r;
			r = snprintf_c99_esc(s, n, u->vs);
			if (__unlikely(r < 0))
				return r;
			t += r; r = MIN((size_t)r, n); s += r; n -= r;
			r = snprintf(s, n, "\") }");
		} else {
			r = snprintf(s, n, "{ .vs = NULL }");
		}
		break;
	case CO_DEFTYPE_OCTET_STRING:
		if (u->os) {
			r = snprintf(s, n, "{ .os = CO_OCTET_STRING_C(\n\t\t\t\t\"");
			if (__unlikely(r < 0))
				return r;
			t += r; r = MIN((size_t)r, n); s += r; n -= r;
			size_t size = co_val_sizeof(type, val);
			for (size_t i = 0; i < size; i++) {
				r = snprintf(s, n, i && !(i % 8)
						? "\"\n\t\t\t\t\"\\x%02x"
						: "\\x%02x",
						u->os[i]);
				if (__unlikely(r < 0))
					return r;
				t += r; r = MIN((size_t)r, n); s += r; n -= r;
			}
			r = snprintf(s, n, "\"\n\t\t\t) }");
		} else {
			r = snprintf(s, n, "{ .vs = NULL }");
		}
		break;
	case CO_DEFTYPE_UNICODE_STRING:
		// TODO: Implement UNICODE_STRING.
		r = snprintf(s, n, "{ .us = NULL }");
		break;
	case CO_DEFTYPE_TIME_OF_DAY:
		r = snprintf(s, n, "{ .t = { "
				".ms = 0x%08" PRIx32 ", "
				".days = %0x04" PRIx16 " "
				"} }",
				u->t.ms, u->t.days);
		break;
	case CO_DEFTYPE_TIME_DIFF:
		r = snprintf(s, n, "{ .t = { "
				".ms = 0x%08" PRIx32 ", "
				".days = %0x04" PRIx16 " "
				"} }",
				u->td.ms, u->td.days);
		break;
	case CO_DEFTYPE_DOMAIN:
		// TODO: Implement DOMAIN.
		r = snprintf(s, n, "{ .dom = NULL }");
		break;
	case CO_DEFTYPE_INTEGER24:
		r = snprintf(s, n, "{ .i24 = %" PRIi32 "l }", u->i24);
		break;
	case CO_DEFTYPE_REAL64:
		r = snprintf(s, n, "{ .r64 = %.*g }", DECIMAL_DIG, u->r64);
	case CO_DEFTYPE_INTEGER40:
		r = snprintf(s, n, "{ .i40 = %" PRIi64 "ll }", u->i40);
		break;
	case CO_DEFTYPE_INTEGER48:
		r = snprintf(s, n, "{ .i48 = %" PRIi64 "ll }", u->i48);
		break;
	case CO_DEFTYPE_INTEGER56:
		r = snprintf(s, n, "{ .i56 = %" PRIi64 "ll }", u->i56);
		break;
	case CO_DEFTYPE_INTEGER64:
		r = snprintf(s, n, "{ .i64 = %" PRIi64 "ll }", u->i64);
		break;
	case CO_DEFTYPE_UNSIGNED24:
		r = snprintf(s, n, "{ .u24 = 0x%06" PRIx32 "ul }", u->u24);
		break;
	case CO_DEFTYPE_UNSIGNED40:
		r = snprintf(s, n, "{ .u40 = 0x%010" PRIx64 "ull }", u->u40);
		break;
	case CO_DEFTYPE_UNSIGNED48:
		r = snprintf(s, n, "{ .u48 = 0x%012" PRIx64 "ull }", u->u48);
		break;
	case CO_DEFTYPE_UNSIGNED56:
		r = snprintf(s, n, "{ .u56 = 0x%014" PRIx64 "ull }", u->u56);
		break;
	case CO_DEFTYPE_UNSIGNED64:
		r = snprintf(s, n, "{ .u64 = 0x%016" PRIx64 "ull }", u->u64);
		break;
	default:
		r = 0;
		break;
	}
	if (__unlikely(r < 0))
		return r;
	t += r;

	return t;
}

static int
snprintf_c99_esc(char *s, size_t n, const char *esc)
{
	if (!s)
		n = 0;

	if (__unlikely(!esc))
		return 0;

	int r, t = 0;

	size_t chars = 0;
	for (;;) {
		// Read the next UTF-8 encoded Unicode character.
		char32_t c32;
		chars = lex_utf8(esc, NULL, NULL, &c32);
		if (!chars || !c32)
			break;
		esc += chars;
		// Print the C99 escape sequence to a temporary buffer.
		char buf[12] = { '\0' };
		char *cp = buf;
		chars = print_c99_esc(c32, &cp, buf + sizeof(buf));
		// Print the character to the string.
		r = snprintf(s, n, "%s", buf);
		if (__unlikely(r < 0))
			return r;
		t += r; r = MIN((size_t)r, n); s += r; n -= r;
	}

	return t;
}

#endif // !LELY_NO_CO_SDEV

