/*!\file
 * This file is part of the CANopen library; it contains the implementation of
 * the CANopen value functions.
 *
 * \see lely/co/val.h
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
#include <lely/libc/stdalign.h>
#include <lely/util/cmp.h>
#include <lely/util/errnum.h>
#include <lely/co/val.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define CO_BOOLEAN_INIT		0
#define CO_INTEGER8_INIT	0
#define CO_INTEGER16_INIT	0
#define CO_INTEGER32_INIT	0
#define CO_UNSIGNED8_INIT	0
#define CO_UNSIGNED16_INIT	0
#define CO_UNSIGNED32_INIT	0
#define CO_REAL32_INIT		0
#define CO_VISIBLE_STRING_INIT	NULL
#define CO_OCTET_STRING_INIT	NULL
#define CO_UNICODE_STRING_INIT	NULL
#define CO_TIME_OF_DAY_INIT	{ 0, 0 }
#define CO_TIME_DIFF_INIT	CO_TIME_OF_DAY_INIT
#define CO_DOMAIN_INIT		NULL
#define CO_INTEGER24_INIT	0
#define CO_REAL64_INIT		0
#define CO_INTEGER40_INIT	0
#define CO_INTEGER48_INIT	0
#define CO_INTEGER56_INIT	0
#define CO_INTEGER64_INIT	0
#define CO_UNSIGNED24_INIT	0
#define CO_UNSIGNED40_INIT	0
#define CO_UNSIGNED48_INIT	0
#define CO_UNSIGNED56_INIT	0
#define CO_UNSIGNED64_INIT	0

#define CO_VISIBLE_STRING_MIN	NULL
#define CO_VISIBLE_STRING_MAX	NULL

#define CO_OCTET_STRING_MIN	NULL
#define CO_OCTET_STRING_MAX	NULL

#define CO_UNICODE_STRING_MIN	NULL
#define CO_UNICODE_STRING_MAX	NULL

#define CO_DOMAIN_MIN	NULL
#define CO_DOMAIN_MAX	NULL

#define CO_ARRAY_OFFSET	ALIGN(sizeof(size_t), alignof(union co_val))

static int co_array_alloc(void *val, size_t size);
static void co_array_free(void *val);
static void co_array_init(void *val, size_t size);
static void co_array_fini(void *val);

static size_t co_array_sizeof(const void *val);

/*!
 * Returns the number of (16-bit) Unicode characters, excluding the terminating
 * null-bytes, in the string at \a s.
 */
static size_t str16len(const char16_t *s);

/*!
 * Copies the (16-bit) Unicode string, including the terminating null-bytes, at
 * \a src to \a dst.
 *
 * \param dst the destination address, which MUST be large enough to hold the
 *            string.
 * \param src a pointer to the (null-terminated) string to be copied.
 *
 * \returns \a dst.
 */
static char16_t *str16cpy(char16_t *dst, const char16_t *src);

/*!
 * Compares two (16-bit) Unicode strings.
 *
 * \param s1 a pointer to the first string.
 * \param s2 a pointer to the second string.
 * \param n  the maximum number of characters to compare.
 *
 * \returns an integer greater than, equal to, or less than 0 if the string at
 * \a s1 is greater than, equal to, or less than the string at \a s2.
 */
static int str16ncmp(const char16_t *s1, const char16_t *s2, size_t n);

LELY_CO_EXPORT int
co_val_init(co_unsigned16_t type, void *val)
{
	union co_val *u = val;
	assert(u);

	switch (type) {
#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
	case CO_DEFTYPE_##a: \
		u->c = (co_##b##_t)CO_##a##_INIT;
		return 0;
#include <lely/co/def/type.def>
#undef LELY_CO_DEFINE_TYPE
	default:
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
}

LELY_CO_EXPORT int
co_val_init_min(co_unsigned16_t type, void *val)
{
	union co_val *u = val;
	assert(u);

	switch (type) {
#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
	case CO_DEFTYPE_##a: \
		u->c = (co_##b##_t)CO_##a##_MIN;
		return 0;
#include <lely/co/def/type.def>
#undef LELY_CO_DEFINE_TYPE
	default:
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
}

LELY_CO_EXPORT int
co_val_init_max(co_unsigned16_t type, void *val)
{
	union co_val *u = val;
	assert(u);

	switch (type) {
#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
	case CO_DEFTYPE_##a: \
		u->c = (co_##b##_t)CO_##a##_MAX;
		return 0;
#include <lely/co/def/type.def>
#undef LELY_CO_DEFINE_TYPE
	default:
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
}

LELY_CO_EXPORT int
co_val_init_vs(char **val, const char *vs)
{
	assert(val);

	if (vs) {
		size_t n = strlen(vs);
		if (__unlikely(co_array_alloc(val, n + 1) == -1))
			return -1;
		assert(*val);
		co_array_init(val, n);
		strcpy(*val, vs);
	} else {
		*val = NULL;
	}

	return 0;
}

LELY_CO_EXPORT int
co_val_init_os(uint8_t **val, const uint8_t *os, size_t n)
{
	assert(val);

	if (os && n) {
		if (__unlikely(co_array_alloc(val, n + 1) == -1))
			return -1;
		assert(*val);
		co_array_init(val, n);
		memcpy(*val, os, n);
	} else {
		*val = NULL;
	}

	return 0;
}

LELY_CO_EXPORT int
co_val_init_us(char16_t **val, const char16_t *us)
{
	assert(val);

	if (us) {
		size_t n = str16len(us);
		if (__unlikely(co_array_alloc(val, 2 * (n + 1)) == -1))
			return -1;
		assert(*val);
		co_array_init(val, 2 * n);
		str16cpy(*val, us);
	} else {
		*val = NULL;
	}

	return 0;
}

LELY_CO_EXPORT int
co_val_init_dom(void **val, const void *dom, size_t n)
{
	assert(val);

	if (dom && n) {
		if (__unlikely(co_array_alloc(val, n) == -1))
			return -1;
		assert(*val);
		co_array_init(val, n);
		memcpy(*val, dom, n);
	} else {
		*val = NULL;
	}

	return 0;
}

LELY_CO_EXPORT void
co_val_fini(co_unsigned16_t type, void *val)
{
	assert(val);

	if (co_type_is_array(type)) {
		co_array_free(val);
		co_array_fini(val);
	}
}

LELY_CO_EXPORT const void *
co_val_addressof(co_unsigned16_t type, const void *val)
{
	if (__unlikely(!val))
		return NULL;

	return co_type_is_array(type) ? *(const void **)val : val;
}

LELY_CO_EXPORT size_t
co_val_sizeof(co_unsigned16_t type, const void *val)
{
	if (__unlikely(!val))
		return 0;

	return co_type_is_array(type)
			? co_array_sizeof(val)
			: co_type_sizeof(type);
}

LELY_CO_EXPORT size_t
co_val_make(co_unsigned16_t type, void *val, const void *ptr, size_t n)
{
	assert(val);

	if (!ptr)
		n = 0;

	switch (type) {
	case CO_DEFTYPE_VISIBLE_STRING:
		n = ptr ? strlen(ptr) : 0;
		co_val_init_vs(val, ptr);
		break;
	case CO_DEFTYPE_OCTET_STRING:
		co_val_init_os(val, ptr, n);
		break;
	case CO_DEFTYPE_UNICODE_STRING:
		n = ptr ? str16len(ptr) : 0;
		co_val_init_us(val, ptr);
		break;
	case CO_DEFTYPE_DOMAIN:
		co_val_init_dom(val, ptr, n);
		break;
	default:
		if (__unlikely(!ptr || co_type_sizeof(type) != n))
			return 0;

		memcpy(val, ptr, n);
	}
	return n;
}

LELY_CO_EXPORT size_t
co_val_copy(co_unsigned16_t type, void *dst, const void *src)
{
	assert(dst);
	assert(src);

	size_t n;
	if (co_type_is_array(type)) {
		const void *ptr = co_val_addressof(type, src);
		n = co_val_sizeof(type, src);
		switch (type) {
		case CO_DEFTYPE_VISIBLE_STRING:
			if (__unlikely(co_val_init_vs(dst, ptr) == -1))
				return 0;
			break;
		case CO_DEFTYPE_OCTET_STRING:
			if (__unlikely(co_val_init_os(dst, ptr, n) == -1))
				return 0;
			break;
		case CO_DEFTYPE_UNICODE_STRING:
			if (__unlikely(co_val_init_us(dst, ptr) == -1))
				return 0;
			break;
		case CO_DEFTYPE_DOMAIN:
			if (__unlikely(co_val_init_dom(dst, ptr, n) == -1))
				return 0;
			break;
		default:
			return 0;
		}
	} else {
		n = co_type_sizeof(type);
		memcpy(dst, src, n);
	}
	return n;
}

LELY_CO_EXPORT size_t
co_val_move(co_unsigned16_t type, void *dst, void *src)
{
	assert(dst);
	assert(src);

	size_t n = co_type_sizeof(type);
	memcpy(dst, src, n);

	if (co_type_is_array(type))
		co_array_fini(src);

	return n;
}

LELY_CO_EXPORT int
co_val_cmp(co_unsigned16_t type, const void *v1, const void *v2)
{
	if (v1 == v2)
		return 0;

	if (__unlikely(!v1))
		return -1;
	if (__unlikely(!v2))
		return 1;

	int cmp = 0;
	if (co_type_is_array(type)) {
		const void *p1 = co_val_addressof(type, v1);
		const void *p2 = co_val_addressof(type, v2);
		if (!p1)
			return -1;
		if (!p2)
			return 1;

		size_t n1 = co_val_sizeof(type, v1);
		size_t n2 = co_val_sizeof(type, v2);
		switch (type) {
		case CO_DEFTYPE_VISIBLE_STRING:
			cmp = strncmp(p1, p2, MIN(n1, n2));
			break;
		case CO_DEFTYPE_OCTET_STRING:
			cmp = memcmp(p1, p2, MIN(n1, n2));
			break;
		case CO_DEFTYPE_UNICODE_STRING:
			cmp = str16ncmp(p1, p2, MIN(n1, n2) / 2);
			break;
		case CO_DEFTYPE_DOMAIN:
			cmp = memcmp(p1, p2, MIN(n1, n2));
			break;
		default:
			return 0;
		}
		if (!cmp)
			cmp = (n1 > n2) - (n1 < n2);
		return cmp;
	} else {
		const union co_val *u1 = v1;
		const union co_val *u2 = v2;
		switch (type) {
		case CO_DEFTYPE_BOOLEAN:
			return bool_cmp(v1, v2);
		case CO_DEFTYPE_INTEGER8:
			return int8_cmp(v1, v2);
		case CO_DEFTYPE_INTEGER16:
			return int16_cmp(v1, v2);
		case CO_DEFTYPE_INTEGER32:
			return int32_cmp(v1, v2);
		case CO_DEFTYPE_UNSIGNED8:
			return uint8_cmp(v1, v2);
		case CO_DEFTYPE_UNSIGNED16:
			return uint16_cmp(v1, v2);
		case CO_DEFTYPE_UNSIGNED32:
			return uint32_cmp(v1, v2);
		case CO_DEFTYPE_REAL32:
			return flt_cmp(v1, v2);
		case CO_DEFTYPE_TIME_OF_DAY:
			cmp = uint32_cmp(&u1->t.ms, &u2->t.ms);
			if (!cmp)
				cmp = uint16_cmp(&u1->t.days, &u2->t.days);
			return cmp;
		case CO_DEFTYPE_TIME_DIFF:
			cmp = uint32_cmp(&u1->td.ms, &u2->td.ms);
			if (!cmp)
				cmp = uint16_cmp(&u1->td.days, &u2->td.days);
			return cmp;
		case CO_DEFTYPE_INTEGER24:
			return int32_cmp(v1, v2);
		case CO_DEFTYPE_REAL64:
			return dbl_cmp(v1, v2);
		case CO_DEFTYPE_INTEGER40:
			return int64_cmp(v1, v2);
		case CO_DEFTYPE_INTEGER48:
			return int64_cmp(v1, v2);
		case CO_DEFTYPE_INTEGER56:
			return int64_cmp(v1, v2);
		case CO_DEFTYPE_INTEGER64:
			return int64_cmp(v1, v2);
		case CO_DEFTYPE_UNSIGNED24:
			return uint32_cmp(v1, v2);
		case CO_DEFTYPE_UNSIGNED40:
			return uint64_cmp(v1, v2);
		case CO_DEFTYPE_UNSIGNED48:
			return uint64_cmp(v1, v2);
		case CO_DEFTYPE_UNSIGNED56:
			return uint64_cmp(v1, v2);
		case CO_DEFTYPE_UNSIGNED64:
			return uint64_cmp(v1, v2);
		default:
			return 0;
		}
	}
}

static int
co_array_alloc(void *val, size_t size)
{
	assert(val);

	if (size) {
		char *ptr = calloc(1, CO_ARRAY_OFFSET + size);
		if (__unlikely(!ptr))
			return -1;
		*(char **)val = ptr + CO_ARRAY_OFFSET;
	} else {
		*(char **)val = NULL;
	}

	return 0;
}

static void
co_array_free(void *val)
{
	assert(val);

	char *ptr = *(char **)val;
	if (ptr)
		free(ptr - CO_ARRAY_OFFSET);
}

static void
co_array_init(void *val, size_t size)
{
	assert(val);

	char *ptr = *(char **)val;
	assert(!size || ptr);
	if (ptr)
		*(size_t *)(ptr - CO_ARRAY_OFFSET) = size;
}

static void
co_array_fini(void *val)
{
	assert(val);

	*(char **)val = NULL;
}

static size_t
co_array_sizeof(const void *val)
{
	assert(val);

	const char *ptr = *(const char **)val;
	return ptr ? *(const size_t *)(ptr - CO_ARRAY_OFFSET) : 0;
}

static size_t
str16len(const char16_t *s)
{
	const char16_t *cp = s;
	for (; *cp; cp++);
	return cp - s;
}

static char16_t *
str16cpy(char16_t *dst, const char16_t *src)
{
	char16_t *cp = dst;
	while (*src)
		*cp++ = *src++;
	*cp = 0;

	return dst;
}

static int
str16ncmp(const char16_t *s1, const char16_t *s2, size_t n)
{
	int result = 0;
	while (n-- && !(result = *s1 - *s2++) && *s1++);
	return result;
}

