/*!\file
 * This header file is part of the utilities library; it contains the comparison
 * function definitions.
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

#ifndef LELY_UTIL_CMP_H
#define LELY_UTIL_CMP_H

#include <lely/libc/stdint.h>
#include <lely/libc/strings.h>
#include <lely/libc/uchar.h>
#include <lely/util/util.h>

#include <stddef.h>
#include <string.h>
#include <wchar.h>

#ifndef LELY_UTIL_CMP_INLINE
#define LELY_UTIL_CMP_INLINE	inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * The type of a generic comparison function. Functions with this signature can
 * be used with, e.g., qsort() from stdlib.h. \a p1 and \a p2 MUST be NULL or
 * point to objects of the same type.
 *
 * \returns an integer greater than, equal to, or less than 0 if the object at
 * \a p1 is greater than, equal to, or less than the object at \a p2.
 */
typedef int __cdecl cmp_t(const void *p1, const void *p2);

#define LELY_UTIL_DEFINE_TYPE(name, type) \
	LELY_UTIL_CMP_INLINE int __cdecl name##_cmp(const void *p1, \
			const void *p2);
#include <lely/util/def/type.def>
LELY_UTIL_DEFINE_TYPE(ptr,)
LELY_UTIL_DEFINE_TYPE(str,)
LELY_UTIL_DEFINE_TYPE(str_case,)
#undef LELY_UTIL_DEFINE_TYPE

#define LELY_UTIL_DEFINE_TYPE(name, type) \
	LELY_UTIL_CMP_INLINE int __cdecl \
	name##_cmp(const void *p1, const void *p2) \
	{ \
		if (p1 == p2) \
			return 0; \
	\
		if (__unlikely(!p1)) \
			return -1; \
		if (__unlikely(!p2)) \
			return 1; \
	\
		type v1 = *(const type *)p1; \
		type v2 = *(const type *)p2; \
		return (v2 < v1) - (v1 < v2); \
	}
#include <lely/util/def/type.def>
#undef LELY_UTIL_DEFINE_TYPE

LELY_UTIL_CMP_INLINE int __cdecl
ptr_cmp(const void *p1, const void *p2)
{
	uintptr_t v1 = (uintptr_t)p1;
	uintptr_t v2 = (uintptr_t)p2;
	return (v2 < v1) - (v1 < v2);
}

LELY_UTIL_CMP_INLINE int __cdecl
str_cmp(const void *p1, const void *p2)
{
	if (p1 == p2)
		return 0;

	if (__unlikely(!p1))
		return -1;
	if (__unlikely(!p2))
		return 1;

	return strcmp((const char *)p1, (const char *)p2);
}

LELY_UTIL_CMP_INLINE int __cdecl
str_case_cmp(const void *p1, const void *p2)
{
	if (p1 == p2)
		return 0;

	if (__unlikely(!p1))
		return -1;
	if (__unlikely(!p2))
		return 1;

	return strcasecmp((const char *)p1, (const char *)p2);
}

#ifdef __cplusplus
}
#endif

#endif
