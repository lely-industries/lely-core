/*!\file
 * This header file is part of the utilities library; it contains the hash
 * function declarations.
 *
 * \copyright 2017 Lely Industries N.V.
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

#ifndef LELY_UTIL_HASH_H
#define LELY_UTIL_HASH_H

#include <lely/libc/stddef.h>
#include <lely/libc/stdint.h>
#include <lely/util/util.h>

#ifndef LELY_UTIL_HASH_INLINE
#define LELY_UTIL_HASH_INLINE	inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

LELY_UTIL_HASH_INLINE size_t hashhash(size_t seed, size_t hash);

//! Returns the hash value of an integer of type `size_t`.
LELY_UTIL_HASH_INLINE size_t sizehash(size_t n);

//! Returns the hash value of a pointer.
LELY_UTIL_HASH_INLINE size_t ptrhash(const void *ptr);

//! Returns the FNV-1a hash of a string.
LELY_UTIL_EXTERN size_t strhash(const char *s);

//! Returns the FNV-1a hash of at most \a n characters of a string.
LELY_UTIL_EXTERN size_t strnhash(const char *s, size_t n);

//! Returns the case-independent FNV-1a hash of a string.
LELY_UTIL_EXTERN size_t strcasehash(const char *s);

/*!
 * Returns the case-independent FNV-1a hash of at most \a n characters of a
 * string.
 */
LELY_UTIL_EXTERN size_t strncasehash(const char *s, size_t n);

LELY_UTIL_HASH_INLINE size_t
hashhash(size_t seed, size_t hash)
{
	// The constant is 2^n / phi, where phi = (1 + sqrt(5)) / 2, i.e., the
	// golden ratio.
#if __WORDSIZE == 64
	hash += UINT64_C(11400714819323198485);
#else
	hash += UINT32_C(2654435769);
#endif
	seed ^= (seed << 6) + (seed >> 2) + hash;
	return seed;
}

LELY_UTIL_HASH_INLINE size_t
sizehash(size_t n)
{
	// The constant is 2^n / phi, where phi = (1 + sqrt(5)) / 2, i.e., the
	// golden ratio.
#if __WORDSIZE == 64
	n *= UINT64_C(11400714819323198485);
#else
	n *= UINT32_C(2654435769);
#endif
	// The middle bits of the multiplication depend on most of the initial
	// bits. Shift them to the lower bits so a simple mask operation can be
	// used to generate smaller hash values.
	const int bits = sizeof(size_t) * CHAR_BIT / 2;
	return (n << bits) | (n >> bits);
}

LELY_UTIL_HASH_INLINE size_t
ptrhash(const void *ptr)
{
	uintptr_t x = (uintptr_t)ptr;
	// Because of alignment requirements, the lower bits of most pointers
	// will be 0. Add a shifted value to improve the hash.
	return sizehash(x + x / sizeof(max_align_t));
}

#ifdef __cplusplus
}
#endif

#endif

