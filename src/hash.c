/*!\file
 * This file is part of the utilities library; it exposes the hash functions.
 *
 * \see lely/util/hash.h
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

#include "util.h"
#define LELY_UTIL_HASH_INLINE	extern inline LELY_DLL_EXPORT
#include <lely/util/hash.h>

#include <assert.h>
#include <ctype.h>

#if __WORDSIZE == 64
#define FNV_OFFSET	UINT64_C(14695981039346656037)
#define FNV_PRIME	UINT64_C(1099511628211)
#else
#define FNV_OFFSET	UINT32_C(2166136261)
#define FNV_PRIME	UINT32_C(16777619)
#endif


LELY_UTIL_EXPORT size_t
strhash(const char *s)
{
	assert(s);

	size_t hash = FNV_OFFSET;
	while (*s)
		hash = (hash ^ *s++) * FNV_PRIME;
	return hash;
}

LELY_UTIL_EXPORT size_t
strnhash(const char *s, size_t n)
{
	assert(s || !n);

	size_t hash = FNV_OFFSET;
	while (n-- && *s)
		hash = (hash ^ *s++) * FNV_PRIME;
	return hash;
}

LELY_UTIL_EXPORT size_t
strcasehash(const char *s)
{
	assert(s);

	size_t hash = FNV_OFFSET;
	while (*s)
		hash = (hash ^ tolower(*s++)) * FNV_PRIME;
	return hash;
}

LELY_UTIL_EXPORT size_t
strncasehash(const char *s, size_t n)
{
	assert(s || !n);

	size_t hash = FNV_OFFSET;
	while (n-- && *s)
		hash = (hash ^ tolower(*s++)) * FNV_PRIME;
	return hash;
}

