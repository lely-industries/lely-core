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
#include <lely/libc/string.h>
#include <lely/util/cmp.h>
#include <lely/util/diag.h>
#include <lely/util/endian.h>
#include <lely/util/lex.h>
#include <lely/util/print.h>
#include <lely/co/sdo.h>
#include <lely/co/val.h>

#include <assert.h>
#include <stdlib.h>

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
		u->c = (co_##b##_t)CO_##a##_INIT; \
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
		u->c = (co_##b##_t)CO_##a##_MIN; \
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
		u->c = (co_##b##_t)CO_##a##_MAX; \
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

	if (n) {
		if (__unlikely(co_array_alloc(val, n + 1) == -1))
			return -1;
		assert(*val);
		co_array_init(val, n);
		if (os)
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

	if (n) {
		if (__unlikely(co_array_alloc(val, n) == -1))
			return -1;
		assert(*val);
		co_array_init(val, n);
		if (dom)
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
			// We can never get here.
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
			// We can never get here.
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

LELY_CO_EXPORT size_t
co_val_read(co_unsigned16_t type, void *val, const uint8_t *begin,
		const uint8_t *end)
{
	assert(begin);
	assert(!end || end >= begin);

	size_t n = end - begin;

	if (co_type_is_array(type)) {
		if (val) {
			switch (type) {
			case CO_DEFTYPE_VISIBLE_STRING:
				if (__unlikely(co_array_alloc(val, n + 1)
						== -1))
					return 0;
				assert(*(char **)val);
				co_array_init(val, n);
				char *vs = *(char **)val;
				memcpy(vs, begin, n);
				break;
			case CO_DEFTYPE_OCTET_STRING:
				if (__unlikely(co_array_alloc(val, n + 1)
						== -1))
					return 0;
				assert(*(uint8_t **)val);
				co_array_init(val, n);
				uint8_t *os = *(uint8_t **)val;
				memcpy(os, begin, n);
				break;
			case CO_DEFTYPE_UNICODE_STRING:
				if (__unlikely(co_array_alloc(val,
						2 * (n / 2 + 1)) == -1))
					return 0;
				assert(*(char16_t **)val);
				co_array_init(val, 2 * (n / 2));
				char16_t *us = *(char16_t **)val;
				for (size_t i = 0; i + 1 < n; i += 2)
					us[i / 2] = ldle_u16(begin + i);
				break;
			case CO_DEFTYPE_DOMAIN:
				if (__unlikely(co_array_alloc(val, n) == -1))
					return 0;
				assert(*(void **)val);
				co_array_init(val, n);
				void *dom = *(void **)val;
				if (dom)
					memcpy(dom, begin, n);
				break;
			default:
				// We can never get here.
				return 0;
			}
		}
		return n;
	} else {
		union co_val *u = val;
		switch (type) {
		case CO_DEFTYPE_BOOLEAN:
			if (__unlikely(n < 1))
				return 0;
			if (u)
				u->b = !!*(const co_boolean_t *)begin;
			return 1;
		case CO_DEFTYPE_INTEGER8:
			if (__unlikely(n < 1))
				return 0;
			if (u)
				u->i8 = *(const co_integer8_t *)begin;
			return 1;
		case CO_DEFTYPE_INTEGER16:
			if (__unlikely(n < 2))
				return 0;
			if (u)
				u->i16 = ldle_i16(begin);
			return 2;
		case CO_DEFTYPE_INTEGER32:
			if (__unlikely(n < 4))
				return 0;
			if (u)
				u->i32 = ldle_i32(begin);
			return 4;
		case CO_DEFTYPE_UNSIGNED8:
			if (__unlikely(n < 1))
				return 0;
			if (u)
				u->u8 = *(const co_unsigned8_t *)begin;
			return 1;
		case CO_DEFTYPE_UNSIGNED16:
			if (__unlikely(n < 2))
				return 0;
			if (u)
				u->u16 = ldle_u16(begin);
			return 2;
		case CO_DEFTYPE_UNSIGNED32:
			if (__unlikely(n < 4))
				return 0;
			if (u)
				u->u32 = ldle_u32(begin);
			return 4;
		case CO_DEFTYPE_REAL32:
			if (__unlikely(n < 4))
				return 0;
			if (u)
				u->r32 = ldle_flt(begin);
			return 4;
		case CO_DEFTYPE_TIME_OF_DAY:
			if (__unlikely(n < 6))
				return 0;
			if (u) {
				u->t.ms = ldle_u32(begin)
						& UINT32_C(0x0fffffff);
				u->t.days = ldle_u16(begin + 4);
			}
			return 6;
		case CO_DEFTYPE_TIME_DIFF:
			if (__unlikely(n < 6))
				return 0;
			if (u) {
				u->td.ms = ldle_u32(begin)
						& UINT32_C(0x0fffffff);
				u->td.days = ldle_u16(begin + 4);
			}
			return 6;
		case CO_DEFTYPE_INTEGER24:
			if (__unlikely(n < 3))
				return 0;
			if (u) {
				co_unsigned24_t u24 = 0;
				for (size_t i = 0; i < 3; i++)
					u24 |= (co_unsigned24_t)*begin++
							<< 8 * i;
				u->i24 = (u24 & UINT32_C(0x00800000))
						? -(co_integer24_t)u24
						: (co_integer24_t)u24;
			}
			return 3;
		case CO_DEFTYPE_REAL64:
			if (__unlikely(n < 8))
				return 0;
			if (u)
				u->r64 = ldle_dbl(begin);
			return 8;
		case CO_DEFTYPE_INTEGER40:
			if (__unlikely(n < 5))
				return 0;
			if (u) {
				co_unsigned40_t u40 = 0;
				for (size_t i = 0; i < 5; i++)
					u40 |= (co_unsigned40_t)*begin++
							<< 8 * i;
				u->i40 = (u40 & UINT64_C(0x0000008000000000))
						? -(co_integer40_t)u40
						: (co_integer40_t)u40;
			}
			return 5;
		case CO_DEFTYPE_INTEGER48:
			if (__unlikely(n < 6))
				return 0;
			if (u) {
				co_unsigned48_t u48 = 0;
				for (size_t i = 0; i < 6; i++)
					u48 |= (co_unsigned48_t)*begin++
							<< 8 * i;
				u->i48 = (u48 & UINT64_C(0x0000800000000000))
						? -(co_integer48_t)u48
						: (co_integer48_t)u48;
			}
			return 6;
		case CO_DEFTYPE_INTEGER56:
			if (__unlikely(n < 7))
				return 0;
			if (u) {
				co_unsigned56_t u56 = 0;
				for (size_t i = 0; i < 7; i++)
					u56 |= (co_unsigned56_t)*begin++
							<< 8 * i;
				u->i56 = (u56 & UINT64_C(0x0080000000000000))
						? -(co_integer56_t)u56
						: (co_integer56_t)u56;
			}
			return 7;
		case CO_DEFTYPE_INTEGER64:
			if (__unlikely(n < 8))
				return 0;
			if (u)
				u->i64 = ldle_i64(begin);
			return 8;
		case CO_DEFTYPE_UNSIGNED24:
			if (__unlikely(n < 3))
				return 0;
			if (u) {
				u->u24 = 0;
				for (size_t i = 0; i < 3; i++)
					u->u24 |= (co_unsigned24_t)*begin++
							<< 8 * i;
			}
			return 3;
		case CO_DEFTYPE_UNSIGNED40:
			if (__unlikely(n < 5))
				return 0;
			if (u) {
				u->u40 = 0;
				for (size_t i = 0; i < 5; i++)
					u->u40 |= (co_unsigned40_t)*begin++
							<< 8 * i;
			}
			return 5;
		case CO_DEFTYPE_UNSIGNED48:
			if (__unlikely(n < 6))
				return 0;
			if (u) {
				u->u48 = 0;
				for (size_t i = 0; i < 6; i++)
					u->u48 |= (co_unsigned48_t)*begin++
							<< 8 * i;
			}
			return 6;
		case CO_DEFTYPE_UNSIGNED56:
			if (__unlikely(n < 7))
				return 0;
			if (u) {
				u->u56 = 0;
				for (size_t i = 0; i < 7; i++)
					u->u56 |= (co_unsigned56_t)*begin++
							<< 8 * i;
			}
			return 7;
		case CO_DEFTYPE_UNSIGNED64:
			if (__unlikely(n < 8))
				return 0;
			if (u)
				u->u64 = ldle_u64(begin);
			return 8;
		default:
			set_errnum(ERRNUM_INVAL);
			return 0;
		}
	}
}

LELY_CO_EXPORT co_unsigned32_t
co_val_read_sdo(co_unsigned16_t type, void *val, const void *ptr, size_t n)
{
	errc_t errc = get_errc();
	co_unsigned32_t ac = 0;

	const uint8_t *begin = ptr;
	const uint8_t *end = begin ? begin + n : NULL;
	if (__unlikely(!co_val_read(type, val, begin, end))) {
		ac = get_errnum() == ERRNUM_NOMEM
				? CO_SDO_AC_NO_MEM : CO_SDO_AC_ERROR;
		set_errc(errc);
	}

	return ac;
}

LELY_CO_EXPORT size_t
co_val_write(co_unsigned16_t type, const void *val, uint8_t *begin,
		uint8_t *end)
{
	assert(val);

	if (co_type_is_array(type)) {
		const void *ptr = co_val_addressof(type, val);
		size_t n = co_val_sizeof(type, val);
		if (!ptr || !n)
			return 0;
		if (begin && (!end || end - begin >= (ptrdiff_t)n)) {
			switch (type) {
			case CO_DEFTYPE_VISIBLE_STRING:
				memcpy(begin, ptr, n);
				break;
			case CO_DEFTYPE_OCTET_STRING:
				memcpy(begin, ptr, n);
				break;
			case CO_DEFTYPE_UNICODE_STRING:
				n %= 2;
				const char16_t *us = ptr;
				for (size_t i = 0; i + 1 < n; i += 2)
					stle_u16(begin + i, us[i / 2]);
				break;
			case CO_DEFTYPE_DOMAIN:
				memcpy(begin, ptr, n);
				break;
			default:
				// We can never get here.
				return 0;
			}
		}
		return n;
	} else {
		const union co_val *u = val;
		switch (type) {
		case CO_DEFTYPE_BOOLEAN:
			if (begin && (!end || end - begin >= 1))
				*(co_boolean_t *)begin = !!u->b;
			return 1;
		case CO_DEFTYPE_INTEGER8:
			if (begin && (!end || end - begin >= 1))
				*(co_integer8_t *)begin = u->i8;
			return 1;
		case CO_DEFTYPE_INTEGER16:
			if (begin && (!end || end - begin >= 2))
				stle_i16(begin, u->i16);
			return 2;
		case CO_DEFTYPE_INTEGER32:
			if (begin && (!end || end - begin >= 4))
				stle_i32(begin, u->i32);
			return 4;
		case CO_DEFTYPE_UNSIGNED8:
			if (begin && (!end || end - begin >= 1))
				*(co_unsigned8_t *)begin = u->u8;
			return 1;
		case CO_DEFTYPE_UNSIGNED16:
			if (begin && (!end || end - begin >= 2))
				stle_u16(begin, u->u16);
			return 2;
		case CO_DEFTYPE_UNSIGNED32:
			if (begin && (!end || end - begin >= 4))
				stle_u32(begin, u->u32);
			return 4;
		case CO_DEFTYPE_REAL32:
			if (begin && (!end || end - begin >= 4))
				stle_flt(begin, u->r32);
			return 4;
		case CO_DEFTYPE_TIME_OF_DAY:
			if (begin && (!end || end - begin >= 6)) {
				stle_u32(begin, u->t.ms & UINT32_C(0x0fffffff));
				stle_u16(begin, u->t.days);
			}
			return 6;
		case CO_DEFTYPE_TIME_DIFF:
			if (begin && (!end || end - begin >= 6)) {
				stle_u32(begin, u->td.ms
						& UINT32_C(0x0fffffff));
				stle_u16(begin, u->td.days);
			}
			return 6;
		case CO_DEFTYPE_INTEGER24:
			if (begin && (!end || end - begin >= 3)) {
				for (size_t i = 0; i < 3; i++)
					*begin++ = (u->i24 >> 8 * i) & 0xff;
			}
			return 3;
		case CO_DEFTYPE_REAL64:
			if (begin && (!end || end - begin >= 8))
				stle_dbl(begin, u->r64);
			return 8;
		case CO_DEFTYPE_INTEGER40:
			if (begin && (!end || end - begin >= 5)) {
				for (size_t i = 0; i < 5; i++)
					*begin++ = (u->i40 >> 8 * i) & 0xff;
			}
			return 5;
		case CO_DEFTYPE_INTEGER48:
			if (begin && (!end || end - begin >= 6)) {
				for (size_t i = 0; i < 6; i++)
					*begin++ = (u->i48 >> 8 * i) & 0xff;
			}
			return 6;
		case CO_DEFTYPE_INTEGER56:
			if (begin && (!end || end - begin >= 7)) {
				for (size_t i = 0; i < 7; i++)
					*begin++ = (u->i56 >> 8 * i) & 0xff;
			}
			return 7;
		case CO_DEFTYPE_INTEGER64:
			if (begin && (!end || end - begin >= 8))
				stle_i64(begin, u->i64);
			return 8;
		case CO_DEFTYPE_UNSIGNED24:
			if (begin && (!end || end - begin >= 3)) {
				for (size_t i = 0; i < 3; i++)
					*begin++ = (u->u24 >> 8 * i) & 0xff;
			}
			return 3;
		case CO_DEFTYPE_UNSIGNED40:
			if (begin && (!end || end - begin >= 5)) {
				for (size_t i = 0; i < 5; i++)
					*begin++ = (u->u40 >> 8 * i) & 0xff;
			}
			return 5;
		case CO_DEFTYPE_UNSIGNED48:
			if (begin && (!end || end - begin >= 6)) {
				for (size_t i = 0; i < 6; i++)
					*begin++ = (u->u48 >> 8 * i) & 0xff;
			}
			return 6;
		case CO_DEFTYPE_UNSIGNED56:
			if (begin && (!end || end - begin >= 7)) {
				for (size_t i = 0; i < 7; i++)
					*begin++ = (u->u56 >> 8 * i) & 0xff;
			}
			return 7;
		case CO_DEFTYPE_UNSIGNED64:
			if (begin && (!end || end - begin >= 8))
				stle_u64(begin, u->u64);
			return 8;
		default:
			set_errnum(ERRNUM_INVAL);
			return 0;
		}
	}
}

LELY_CO_EXPORT size_t
co_val_lex(co_unsigned16_t type, void *val, const char *begin,
		const char *end, struct floc *at)
{
	assert(val);
	assert(begin);
	assert(!end || end >= begin);

	const char *cp = begin;
	size_t chars = 0;

	union co_val u;
	switch (type) {
	case CO_DEFTYPE_BOOLEAN:
		chars = lex_c99_u8(cp, end, NULL, &u.u8);
		if (chars) {
			cp += chars;
			if (__unlikely(u.u8 > CO_BOOLEAN_MAX)) {
				u.u8 = CO_BOOLEAN_MAX;
				set_errnum(ERRNUM_RANGE);
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"boolean truth value overflow");
			}
			if (val)
				*(co_boolean_t *)val = u.u8;
		}
		break;
	case CO_DEFTYPE_INTEGER8:
		chars = lex_c99_i8(cp, end, NULL, &u.i8);
		if (chars) {
			cp += chars;
			if (__unlikely(get_errnum() == ERRNUM_RANGE
					&& u.i8 == INT8_MIN)) {
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"8-bit signed integer underflow");
			} else if (__unlikely(get_errnum() == ERRNUM_RANGE
					&& u.i8 == INT8_MAX)) {
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"8-bit signed integer overflow");
			}
			if (val)
				*(co_integer8_t *)val = u.i8;
		}
		break;
	case CO_DEFTYPE_INTEGER16:
		chars = lex_c99_i16(cp, end, NULL, &u.i16);
		if (chars) {
			cp += chars;
			if (__unlikely(get_errnum() == ERRNUM_RANGE
					&& u.i16 == INT16_MIN)) {
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"16-bit signed integer underflow");
			} else if (__unlikely(get_errnum() == ERRNUM_RANGE
					&& u.i16 == INT16_MAX)) {
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"16-bit signed integer overflow");
			}
			if (val)
				*(co_integer16_t *)val = u.i16;
		}
		break;
	case CO_DEFTYPE_INTEGER32:
		chars = lex_c99_i32(cp, end, NULL, &u.i32);
		if (chars) {
			cp += chars;
			if (__unlikely(get_errnum() == ERRNUM_RANGE
					&& u.i32 == INT32_MIN)) {
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"32-bit signed integer underflow");
			} else if (__unlikely(get_errnum() == ERRNUM_RANGE
					&& u.i32 == INT32_MAX)) {
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"32-bit signed integer overflow");
			}
			if (val)
				*(co_integer32_t *)val = u.i32;
		}
		break;
	case CO_DEFTYPE_UNSIGNED8:
		chars = lex_c99_u8(cp, end, NULL, &u.u8);
		if (chars) {
			cp += chars;
			if (__unlikely(get_errnum() == ERRNUM_RANGE
					&& u.u8 == UINT8_MAX)) {
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"8-bit unsigned integer overflow");
			}
			if (val)
				*(co_unsigned8_t *)val = u.u8;
		}
		break;
	case CO_DEFTYPE_UNSIGNED16:
		chars = lex_c99_u16(cp, end, NULL, &u.u16);
		if (chars) {
			cp += chars;
			if (__unlikely(get_errnum() == ERRNUM_RANGE
					&& u.u16 == UINT16_MAX)) {
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"16-bit unsigned integer overflow");
			}
			if (val)
				*(co_unsigned16_t *)val = u.u16;
		}
		break;
	case CO_DEFTYPE_UNSIGNED32:
		chars = lex_c99_u32(cp, end, NULL, &u.u32);
		if (chars) {
			cp += chars;
			if (__unlikely(get_errnum() == ERRNUM_RANGE
					&& u.u32 == UINT32_MAX)) {
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"32-bit unsigned integer overflow");
			}
			if (val)
				*(co_unsigned32_t *)val = u.u32;
		}
		break;
	case CO_DEFTYPE_REAL32:
		chars = lex_c99_u32(cp, end, NULL, &u.u32);
		if (chars) {
			cp += chars;
			if (__unlikely(get_errnum() == ERRNUM_RANGE
					&& u.u32 == UINT32_MAX)) {
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"32-bit unsigned integer overflow");
			}
			u.r32 = ((union { uint32_t u32; co_real32_t r32; })
					{ u.u32 }).r32;
			if (val)
				*(co_real32_t *)val = u.r32;
		}
		break;
	case CO_DEFTYPE_VISIBLE_STRING:
		if (end) {
			chars = end - cp;
			if (val) {
				// Copy the string to a temporary buffer.
				char *buf = strndup(cp, chars);
				if (__unlikely(!buf)) {
					if (at)
						diag_at(DIAG_ERROR, get_errc(),
								at,
								"unable to duplicate string");
					return 0;
				}
				int result = co_val_init_vs(val, buf);
				free(buf);
				if (__unlikely(result == -1)) {
					if (at)
						diag_at(DIAG_ERROR, get_errc(),
								at,
								"unable to create value of type VISIBLE_STRING");
					return 0;
				}
			}
		} else {
			chars = strlen(cp) + 1;
			if (val && __unlikely(co_val_init_vs(val, cp) == -1)) {
				if (at)
					diag_at(DIAG_ERROR, get_errc(), at,
							"unable to create value of type VISIBLE_STRING");
				return 0;
			}
		}
		cp += chars;
		break;
	case CO_DEFTYPE_OCTET_STRING: {
		// Count the number of hexadecimal digits.
		while ((!end || cp + chars < end)
				&& isxdigit((unsigned char)cp[chars]))
			chars++;
		if (val) {
			if (__unlikely(co_val_init_os(val, NULL,
					(chars + 1) / 2) == -1)) {
				if (at)
					diag_at(DIAG_ERROR, get_errc(), at,
							"unable to create value of type OCTET_STRING");
				return 0;
			}
			// Parse the octets.
			uint8_t *os = *(void **)val;
			assert(*os);
			for (size_t i = 0; i < chars; i++) {
				if (i % 2) {
					*os <<= 4;
					*os++ |= ctox(cp[i]) & 0xf;
				} else {
					*os = ctox(cp[i]) & 0xf;
				}
			}
		}
		cp += chars;
		break;
	}
	case CO_DEFTYPE_UNICODE_STRING: {
		size_t n = 0;
		chars = lex_base64(NULL, &n, cp, end, NULL);
		if (val) {
			if (__unlikely(co_array_alloc(val, 2 * (n / 2 + 1))
					== -1)) {
				if (at)
					diag_at(DIAG_ERROR, get_errc(), at,
							"unable to create value of type UNICODE_STRING");
				return 0;
			}
			co_array_init(val, 2 * (n / 2));
			// Parse the Unicode characters.
			char16_t *us = *(void **)val;
			assert(*us);
			lex_base64(us, NULL, cp, end, NULL);
			for (size_t i = 0; i + 1 < n; i += 2)
				us[i / 2] = letoh_u16(us[i / 2]);
			break;
		}
		cp += chars;
		break;
	}
	case CO_DEFTYPE_TIME_OF_DAY:
		// TODO: Implement TIME_OF_DAY.
		set_errnum(ERRNUM_NOSYS);
		if (at)
			diag_at(DIAG_ERROR, get_errc(), at,
					"cannot parse value of type TIME_OF_DAY");
		break;
	case CO_DEFTYPE_TIME_DIFF:
		// TODO: Implement TIME_DIFFERENCE.
		set_errnum(ERRNUM_NOSYS);
		if (at)
			diag_at(DIAG_ERROR, get_errc(), at,
					"cannot parse value of type TIME_DIFFERENCE");
		break;
	case CO_DEFTYPE_DOMAIN: {
		size_t n = 0;
		chars = lex_base64(NULL, &n, cp, end, NULL);
		if (val) {
			if (__unlikely(co_val_init_dom(val, NULL, n) == -1)) {
				if (at)
					diag_at(DIAG_ERROR, get_errc(), at,
							"unable to create value of type DOMAIN");
				return 0;
			}
			void *dom = *(void **)val;
			assert(dom);
			lex_base64(dom, NULL, cp, end, NULL);
		}
		cp += chars;
		break;
	}
	case CO_DEFTYPE_INTEGER24:
		chars = lex_c99_i32(cp, end, NULL, &u.i32);
		if (chars) {
			cp += chars;
			if (__unlikely(u.i32 < CO_INTEGER24_MIN)) {
				u.i32 = CO_INTEGER24_MIN;
				set_errnum(ERRNUM_RANGE);
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"24-bit signed integer underflow");
			} else if (__unlikely(u.i32 > CO_INTEGER24_MAX)) {
				u.i32 = CO_INTEGER24_MAX;
				set_errnum(ERRNUM_RANGE);
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"24-bit signed integer overflow");
			}
			if (val)
				*(co_integer24_t *)val = u.i32;
		}
		break;
	case CO_DEFTYPE_REAL64:
		chars = lex_c99_u64(cp, end, NULL, &u.u64);
		if (chars) {
			cp += chars;
			if (__unlikely(get_errnum() == ERRNUM_RANGE
					&& u.u64 == UINT64_MAX)) {
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"64-bit unsigned integer overflow");
			}
			u.r64 = ((union { uint64_t u64; co_real64_t r64; })
					{ u.u64 }).r64;
			if (val)
				*(co_real64_t *)val = u.r64;
		}
		break;
	case CO_DEFTYPE_INTEGER40:
		chars = lex_c99_i64(cp, end, NULL, &u.i64);
		if (chars) {
			cp += chars;
			if (__unlikely(u.i64 < CO_INTEGER40_MIN)) {
				u.i64 = CO_INTEGER40_MIN;
				set_errnum(ERRNUM_RANGE);
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"40-bit signed integer underflow");
			} else if (__unlikely(u.i64 > CO_INTEGER40_MAX)) {
				u.i64 = CO_INTEGER40_MAX;
				set_errnum(ERRNUM_RANGE);
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"40-bit signed integer overflow");
			}
			if (val)
				*(co_integer40_t *)val = u.i64;
		}
		break;
	case CO_DEFTYPE_INTEGER48:
		chars = lex_c99_i64(cp, end, NULL, &u.i64);
		if (chars) {
			cp += chars;
			if (__unlikely(u.i64 < CO_INTEGER48_MIN)) {
				u.i64 = CO_INTEGER48_MIN;
				set_errnum(ERRNUM_RANGE);
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"48-bit signed integer underflow");
			} else if (__unlikely(u.i64 > CO_INTEGER48_MAX)) {
				u.i64 = CO_INTEGER48_MAX;
				set_errnum(ERRNUM_RANGE);
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"48-bit signed integer overflow");
			}
			if (val)
				*(co_integer48_t *)val = u.i64;
		}
		break;
	case CO_DEFTYPE_INTEGER56:
		chars = lex_c99_i64(cp, end, NULL, &u.i64);
		if (chars) {
			cp += chars;
			if (__unlikely(u.i64 < CO_INTEGER56_MIN)) {
				u.i64 = CO_INTEGER56_MIN;
				set_errnum(ERRNUM_RANGE);
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"56-bit signed integer underflow");
			} else if (__unlikely(u.i64 > CO_INTEGER56_MAX)) {
				u.i64 = CO_INTEGER56_MAX;
				set_errnum(ERRNUM_RANGE);
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"56-bit signed integer overflow");
			}
			if (val)
				*(co_integer56_t *)val = u.i64;
		}
		break;
	case CO_DEFTYPE_INTEGER64:
		chars = lex_c99_i64(cp, end, NULL, &u.i64);
		if (chars) {
			cp += chars;
			if (__unlikely(get_errnum() == ERRNUM_RANGE
					&& u.i64 == INT64_MIN)) {
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"64-bit signed integer underflow");
			} else if (__unlikely(get_errnum() == ERRNUM_RANGE
					&& u.i64 == INT64_MAX)) {
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"64-bit signed integer overflow");
			}
			if (val)
				*(co_integer64_t *)val = u.i64;
		}
		break;
	case CO_DEFTYPE_UNSIGNED24:
		chars = lex_c99_u32(cp, end, NULL, &u.u32);
		if (chars) {
			cp += chars;
			if (__unlikely(u.u32 > CO_UNSIGNED24_MAX)) {
				u.u32 = CO_UNSIGNED24_MAX;
				set_errnum(ERRNUM_RANGE);
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"24-bit unsigned integer overflow");
			}
			if (val)
				*(co_unsigned24_t *)val = u.u32;
		}
		break;
	case CO_DEFTYPE_UNSIGNED40:
		chars = lex_c99_u64(cp, end, NULL, &u.u64);
		if (chars) {
			cp += chars;
			if (__unlikely(u.u64 > CO_UNSIGNED40_MAX)) {
				u.u64 = CO_UNSIGNED40_MAX;
				set_errnum(ERRNUM_RANGE);
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"40-bit unsigned integer overflow");
			}
			if (val)
				*(co_unsigned40_t *)val = u.u64;
		}
		break;
	case CO_DEFTYPE_UNSIGNED48:
		chars = lex_c99_u64(cp, end, NULL, &u.u64);
		if (chars) {
			cp += chars;
			if (__unlikely(u.u64 > CO_UNSIGNED48_MAX)) {
				u.u64 = CO_UNSIGNED48_MAX;
				set_errnum(ERRNUM_RANGE);
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"48-bit unsigned integer overflow");
			}
			if (val)
				*(co_unsigned48_t *)val = u.u64;
		}
		break;
	case CO_DEFTYPE_UNSIGNED56:
		chars = lex_c99_u64(cp, end, NULL, &u.u64);
		if (chars) {
			cp += chars;
			if (__unlikely(u.u64 > CO_UNSIGNED56_MAX)) {
				u.u64 = CO_UNSIGNED56_MAX;
				set_errnum(ERRNUM_RANGE);
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"56-bit unsigned integer overflow");
			}
			if (val)
				*(co_unsigned56_t *)val = u.u64;
		}
		break;
	case CO_DEFTYPE_UNSIGNED64:
		chars = lex_c99_u64(cp, end, NULL, &u.u64);
		if (chars) {
			cp += chars;
			if (__unlikely(get_errnum() == ERRNUM_RANGE
					&& u.u64 == UINT64_MAX)) {
				if (at)
					diag_at(DIAG_WARNING, get_errc(), at,
							"64-bit unsigned integer overflow");
			}
			if (val)
				*(co_unsigned64_t *)val = u.u64;
		}
		break;
	default:
		if (at)
			diag_at(DIAG_ERROR, 0, at, "cannot parse value of type 0x%04X",
					type);
		break;
	}

	if (at)
		floc_strninc(at, cp, cp - begin);

	return cp - begin;
}

LELY_CO_EXPORT size_t
co_val_print(co_unsigned16_t type, const void *val, char **pbegin, char *end)
{
	if (co_type_is_array(type)) {
		const void *ptr = co_val_addressof(type, val);
		size_t n = co_val_sizeof(type, val);
		if (!ptr || !n)
			return 0;
		switch (type) {
			case CO_DEFTYPE_VISIBLE_STRING:
				return print_c99_str(ptr, pbegin, end);
			case CO_DEFTYPE_OCTET_STRING: {
				size_t chars = 0;
				for (const uint8_t *os = ptr; n; n--, os++) {
					chars += print_char(otoc(*os >> 4),
							pbegin, end);
					chars += print_char(otoc(*os), pbegin,
							end);
				}
				return chars;
			}
			case CO_DEFTYPE_UNICODE_STRING: {
				char16_t *us = NULL;
				if (__unlikely(co_val_copy(type, &us, val)
						!= n))
					return 0;
				for (size_t i = 0; i + 1 < n; i += 2)
					us[i / 2] = htole_u16(us[i / 2]);
				size_t chars = print_base64(us, n, pbegin, end);
				co_val_fini(type, &us);
				return chars;
			}
			case CO_DEFTYPE_DOMAIN:
				return print_base64(ptr, n, pbegin, end);
			default:
				// We can never get here.
				return 0;
		}
	} else {
		const union co_val *u = val;
		switch (type) {
		case CO_DEFTYPE_BOOLEAN:
			return print_c99_u8(!!u->b, pbegin, end);
		case CO_DEFTYPE_INTEGER8:
			return print_c99_i8(u->i8, pbegin, end);
		case CO_DEFTYPE_INTEGER16:
			return print_c99_i16(u->i16, pbegin, end);
		case CO_DEFTYPE_INTEGER32:
			return print_c99_i32(u->i32, pbegin, end);
		case CO_DEFTYPE_UNSIGNED8:
			return print_c99_u8(u->u8, pbegin, end);
		case CO_DEFTYPE_UNSIGNED16:
			return print_c99_u16(u->u16, pbegin, end);
		case CO_DEFTYPE_UNSIGNED32:
			return print_c99_u32(u->u32, pbegin, end);
		case CO_DEFTYPE_REAL32:
			return print_c99_flt(u->r32, pbegin, end);
		case CO_DEFTYPE_TIME_OF_DAY:
			// TODO: Implement TIME_OF_DAY.
			set_errnum(ERRNUM_NOSYS);
			return 0;
		case CO_DEFTYPE_TIME_DIFF:
			// TODO: Implement TIME_DIFFERENCE.
			set_errnum(ERRNUM_NOSYS);
			return 0;
		case CO_DEFTYPE_INTEGER24:
			return print_c99_i32(u->i24, pbegin, end);
		case CO_DEFTYPE_REAL64:
			return print_c99_dbl(u->r64, pbegin, end);
		case CO_DEFTYPE_INTEGER40:
			return print_c99_i64(u->i40, pbegin, end);
		case CO_DEFTYPE_INTEGER48:
			return print_c99_i64(u->i48, pbegin, end);
		case CO_DEFTYPE_INTEGER56:
			return print_c99_i64(u->i56, pbegin, end);
		case CO_DEFTYPE_INTEGER64:
			return print_c99_i64(u->i64, pbegin, end);
		case CO_DEFTYPE_UNSIGNED24:
			return print_c99_u32(u->u24, pbegin, end);
		case CO_DEFTYPE_UNSIGNED40:
			return print_c99_u64(u->u40, pbegin, end);
		case CO_DEFTYPE_UNSIGNED48:
			return print_c99_u64(u->u48, pbegin, end);
		case CO_DEFTYPE_UNSIGNED56:
			return print_c99_u64(u->u56, pbegin, end);
		case CO_DEFTYPE_UNSIGNED64:
			return print_c99_u64(u->u64, pbegin, end);
		default:
			set_errnum(ERRNUM_INVAL);
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
		if (__unlikely(!ptr)) {
			set_errno(errno);
			return -1;
		}
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

