/**@file
 * This header file is part of the utilities library; it contains the bit
 * function definitions.
 *
 * @copyright 2014-2020 Lely Industries N.V.
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

#ifndef LELY_UTIL_BITS_H_
#define LELY_UTIL_BITS_H_

#include <lely/features.h>

#include <stdint.h>

#ifdef _MSC_VER
#include <intrin.h>
#include <stdlib.h>
#endif

#ifndef LELY_UTIL_BITS_INLINE
#define LELY_UTIL_BITS_INLINE static inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Reverses the byte order of the 16-bit unsigned integer <b>x</b>. This
 * function assumes 8-bit bytes.
 */
LELY_UTIL_BITS_INLINE uint_least16_t bswap16(uint_least16_t x);

/**
 * Reverses the byte order of the 32-bit unsigned integer <b>x</b>. This
 * function assumes 8-bit bytes.
 */
LELY_UTIL_BITS_INLINE uint_least32_t bswap32(uint_least32_t x);

/**
 * Reverses the byte order of the 64-bit unsigned integer <b>x</b>. This
 * function assumes 8-bit bytes.
 */
LELY_UTIL_BITS_INLINE uint_least64_t bswap64(uint_least64_t x);
/**
 * Counts the number of leading set bits in the unsigned 8-bit integer <b>x</b>.
 */
LELY_UTIL_BITS_INLINE int cls8(uint_least8_t x);

/**
 * Counts the number of leading zero bits in the unsigned 8-bit integer
 * <b>x</b>.
 */
#if defined(_MSC_VER) || defined(__GNUC__) || __has_builtin(__builtin_clz)
LELY_UTIL_BITS_INLINE int clz8(uint_least8_t x);
#else
int clz8(uint_least8_t x);
#endif

/**
 * Counts the number of leading set bits in the unsigned 16-bit integer
 * <b>x</b>.
 */
LELY_UTIL_BITS_INLINE int cls16(uint_least16_t x);

/**
 * Counts the number of leading zero bits in the unsigned 16-bit integer
 * <b>x</b>.
 */
LELY_UTIL_BITS_INLINE int clz16(uint_least16_t x);

/**
 * Counts the number of leading set bits in the unsigned 32-bit integer
 * <b>x</b>.
 */
LELY_UTIL_BITS_INLINE int cls32(uint_least32_t x);

/**
 * Counts the number of leading zero bits in the unsigned 32-bit integer
 * <b>x</b>.
 */
LELY_UTIL_BITS_INLINE int clz32(uint_least32_t x);

/**
 * Counts the number of leading set bits in the unsigned 64-bit integer
 * <b>x</b>.
 */
LELY_UTIL_BITS_INLINE int cls64(uint_least64_t x);

/**
 * Counts the number of leading zero bits in the unsigned 64-bit integer
 * <b>x</b>.
 */
LELY_UTIL_BITS_INLINE int clz64(uint_least64_t x);

/**
 * Counts the number of trailing set bits in the unsigned 8-bit integer
 * <b>x</b>.
 */
LELY_UTIL_BITS_INLINE int cts8(uint_least8_t x);

/**
 * Counts the number of trailing zero bits in the unsigned 8-bit integer
 * <b>x</b>.
 */
#if defined(_MSC_VER) || defined(__GNUC__) || __has_builtin(__builtin_ctz)
LELY_UTIL_BITS_INLINE int ctz8(uint_least8_t x);
#else
int ctz8(uint_least8_t x);
#endif

/**
 * Counts the number of trailing set bits in the unsigned 16-bit integer
 * <b>x</b>.
 */
LELY_UTIL_BITS_INLINE int cts16(uint_least16_t x);

/**
 * Counts the number of trailing zero bits in the unsigned 16-bit integer
 * <b>x</b>.
 */
LELY_UTIL_BITS_INLINE int ctz16(uint_least16_t x);

/**
 * Counts the number of trailing set bits in the unsigned 32-bit integer
 * <b>x</b>.
 */
LELY_UTIL_BITS_INLINE int cts32(uint_least32_t x);

/**
 * Counts the number of trailing zero bits in the unsigned 32-bit integer
 * <b>x</b>.
 */
LELY_UTIL_BITS_INLINE int ctz32(uint_least32_t x);

/**
 * Counts the number of trailing set bits in the unsigned 64-bit integer
 * <b>x</b>.
 */
LELY_UTIL_BITS_INLINE int cts64(uint_least64_t x);

/**
 * Counts the number of trailing zero bits in the unsigned 64-bit integer
 * <b>x</b>.
 */
LELY_UTIL_BITS_INLINE int ctz64(uint_least64_t x);

/**
 * Returns the index (starting from one) of the first set bit in the unsigned
 * 8-bit integer <b>x</b>, or 0 if all bits are zero.
 */
#if defined(_MSC_VER) || defined(__GNUC__) || __has_builtin(__builtin_ffs)
LELY_UTIL_BITS_INLINE int ffs8(uint_least8_t x);
#else
int ffs8(uint_least8_t x);
#endif

/**
 * Returns the index (starting from one) of the first zero bit in the unsigned
 * 8-bit integer <b>x</b>, or 0 if all bits are set.
 */
LELY_UTIL_BITS_INLINE int ffz8(uint_least8_t x);

/**
 * Returns the index (starting from one) of the first set bit in the unsigned
 * 16-bit integer <b>x</b>, or 0 if all bits are zero.
 */
LELY_UTIL_BITS_INLINE int ffs16(uint_least16_t x);

/**
 * Returns the index (starting from one) of the first zero bit in the unsigned
 * 16-bit integer <b>x</b>, or 0 if all bits are set.
 */
LELY_UTIL_BITS_INLINE int ffz16(uint_least16_t x);

/**
 * Returns the index (starting from one) of the first set bit in the unsigned
 * 32-bit integer <b>x</b>, or 0 if all bits are zero.
 */
LELY_UTIL_BITS_INLINE int ffs32(uint_least32_t x);

/**
 * Returns the index (starting from one) of the first zero bit in the unsigned
 * 32-bit integer <b>x</b>, or 0 if all bits are set.
 */
LELY_UTIL_BITS_INLINE int ffz32(uint_least32_t x);

/**
 * Returns the index (starting from one) of the first set bit in the unsigned
 * 64-bit integer <b>x</b>, or 0 if all bits are zero.
 */
LELY_UTIL_BITS_INLINE int ffs64(uint_least64_t x);

/**
 * Returns the index (starting from one) of the first zero bit in the unsigned
 * 64-bit integer <b>x</b>, or 0 if all bits are set.
 */
LELY_UTIL_BITS_INLINE int ffz64(uint_least64_t x);

/// Returns the parity of the unsigned 8-bit integer <b>x</b>.
#if defined(__GNUC__) || __has_builtin(__builtin_parity)
LELY_UTIL_BITS_INLINE int parity8(uint_least8_t x);
#else
int parity8(uint_least8_t x);
#endif

/// Returns the parity of the unsigned 16-bit integer <b>x</b>.
LELY_UTIL_BITS_INLINE int parity16(uint_least16_t x);

/// Returns the parity of the unsigned 32-bit integer <b>x</b>.
LELY_UTIL_BITS_INLINE int parity32(uint_least32_t x);

/// Returns the parity of the unsigned 64-bit integer <b>x</b>.
LELY_UTIL_BITS_INLINE int parity64(uint_least64_t x);

/**
 * Returns the population count (the number of set bits) in the unsigned 8-bit
 * integer <b>x</b>.
 */
#if defined(__GNUC__) || __has_builtin(__builtin_popcount)
LELY_UTIL_BITS_INLINE int popcount8(uint_least8_t x);
#else
int popcount8(uint_least8_t x);
#endif

/**
 * Returns the population count (the number of set bits) in the unsigned 16-bit
 * integer <b>x</b>.
 */
LELY_UTIL_BITS_INLINE int popcount16(uint_least16_t x);

/**
 * Returns the population count (the number of set bits) in the unsigned 32-bit
 * integer <b>x</b>.
 */
LELY_UTIL_BITS_INLINE int popcount32(uint_least32_t x);

/**
 * Returns the population count (the number of set bits) in the unsigned 64-bit
 * integer <b>x</b>.
 */
LELY_UTIL_BITS_INLINE int popcount64(uint_least64_t x);

/// Rotates the 8-bit unsigned integer <b>x</b> left by <b>n</b> bits.
LELY_UTIL_BITS_INLINE uint_least8_t rol8(uint_least8_t x, unsigned int n);

/// Rotates the 8-bit unsigned integer <b>x</b> right by <b>n</b> bits.
LELY_UTIL_BITS_INLINE uint_least8_t ror8(uint_least8_t x, unsigned int n);

/// Rotates the 16-bit unsigned integer <b>x</b> left by <b>n</b> bits.
LELY_UTIL_BITS_INLINE uint_least16_t rol16(uint_least16_t x, unsigned int n);

/// Rotates the 16-bit unsigned integer <b>x</b> right by <b>n</b> bits.
LELY_UTIL_BITS_INLINE uint_least16_t ror16(uint_least16_t x, unsigned int n);

/// Rotates the 32-bit unsigned integer <b>x</b> left by <b>n</b> bits.
LELY_UTIL_BITS_INLINE uint_least32_t rol32(uint_least32_t x, unsigned int n);

/// Rotates the 32-bit unsigned integer <b>x</b> right by <b>n</b> bits.
LELY_UTIL_BITS_INLINE uint_least32_t ror32(uint_least32_t x, unsigned int n);

/// Rotates the 64-bit unsigned integer <b>x</b> left by <b>n</b> bits.
LELY_UTIL_BITS_INLINE uint_least64_t rol64(uint_least64_t x, unsigned int n);

/// Rotates the 64-bit unsigned integer <b>x</b> right by <b>n</b> bits.
LELY_UTIL_BITS_INLINE uint_least64_t ror64(uint_least64_t x, unsigned int n);

LELY_UTIL_BITS_INLINE uint_least16_t
bswap16(uint_least16_t x)
{
#ifdef _MSC_VER
	return _byteswap_ushort(x);
#elif GNUC_PREREQ(4, 8) || __has_builtin(__builtin_bswap16)
	return __builtin_bswap16(x);
#else
	return ((x & 0xff) << 8) | ((x >> 8) & 0xff);
#endif
}

LELY_UTIL_BITS_INLINE uint_least32_t
bswap32(uint_least32_t x)
{
#ifdef _MSC_VER
	return _byteswap_ulong(x);
#elif GNUC_PREREQ(4, 3) || __has_builtin(__builtin_bswap32)
	return __builtin_bswap32(x);
#else
	return ((x & 0xff) << 24) | ((x & 0xff00u) << 8) | ((x >> 8) & 0xff00u)
			| ((x >> 24) & 0xff);
#endif
}

LELY_UTIL_BITS_INLINE uint_least64_t
bswap64(uint_least64_t x)
{
#ifdef _MSC_VER
	return _byteswap_uint64(x);
#elif GNUC_PREREQ(4, 3) || __has_builtin(__builtin_bswap64)
	return __builtin_bswap64(x);
#else
	return ((x & 0xff) << 56) | ((x & 0xff00u) << 40)
			| ((x & 0xff0000ul) << 24) | ((x & 0xff000000ul) << 8)
			| ((x >> 8) & 0xff000000ul) | ((x >> 24) & 0xff0000ul)
			| ((x >> 40) & 0xff00u) | ((x >> 56) & 0xff);
#endif
}

LELY_UTIL_BITS_INLINE int
cls8(uint_least8_t x)
{
	return clz8(~x);
}

#if defined(_MSC_VER) || defined(__GNUC__) || __has_builtin(__builtin_clz)
LELY_UTIL_BITS_INLINE int
clz8(uint_least8_t x)
{
	x &= UINT8_C(0xff);
#ifdef _MSC_VER
	unsigned long Index;
	return _BitScanReverse(&Index, x) ? (7 - Index) : 8;
#elif defined(__GNUC__) || __has_builtin(__builtin_clz)
	return (x != 0) ? (__builtin_clz(x) - 24) : 8;
#endif
}
#endif // _MSC_VER || __GNUC__ || __has_builtin(__builtin_clz)

LELY_UTIL_BITS_INLINE int
cls16(uint_least16_t x)
{
	return clz16(~x);
}

LELY_UTIL_BITS_INLINE int
clz16(uint_least16_t x)
{
	x &= UINT16_C(0xffff);
#ifdef _MSC_VER
	unsigned long Index;
	return _BitScanReverse(&Index, x) ? (15 - Index) : 16;
#elif defined(__GNUC__) || __has_builtin(__builtin_clz)
	return (x != 0) ? (__builtin_clz(x) - 16) : 16;
#else
	return ((x >> 8) != 0) ? clz8(x >> 8) : (clz8((uint_least8_t)x) + 8);
#endif
}

LELY_UTIL_BITS_INLINE int
cls32(uint_least32_t x)
{
	return clz32(~x);
}

LELY_UTIL_BITS_INLINE int
clz32(uint_least32_t x)
{
	x &= UINT32_C(0xffffffff);
#ifdef _MSC_VER
	unsigned long Index;
	return _BitScanReverse(&Index, x) ? 31 - Index : 32;
#elif (defined(__GNUC__) || __has_builtin(__builtin_clz)) && __WORDSIZE == 64
	return (x != 0) ? __builtin_clz(x) : 32;
#elif defined(__GNUC__) || __has_builtin(__builtin_clzl)
	return (x != 0) ? __builtin_clzl(x) : 32;
#else
	return ((x >> 16) != 0) ? clz16(x >> 16)
				: (clz16((uint_least16_t)x) + 16);
#endif
}

LELY_UTIL_BITS_INLINE int
cls64(uint_least64_t x)
{
	return clz64(~x);
}

LELY_UTIL_BITS_INLINE int
clz64(uint_least64_t x)
{
	x &= UINT64_C(0xffffffffffffffff);
#if defined(_MSC_VER) && _WIN64
	unsigned long Index;
	return _BitScanReverse64(&Index, x) ? (63 - Index) : 64;
#elif (defined(__GNUC__) || __has_builtin(__builtin_clzl)) && LONG_BIT == 64
	return (x != 0) ? __builtin_clzl(x) : 64;
#elif defined(__GNUC__) || __has_builtin(__builtin_clzll)
	return (x != 0) ? __builtin_clzll(x) : 64;
#else
	return ((x >> 32) != 0) ? clz32(x >> 32)
				: (clz32((uint_least32_t)x) + 32);
#endif
}

LELY_UTIL_BITS_INLINE int
cts8(uint_least8_t x)
{
	return ctz8(~x);
}

#if defined(_MSC_VER) || defined(__GNUC__) || __has_builtin(__builtin_ctz)
LELY_UTIL_BITS_INLINE int
ctz8(uint_least8_t x)
{
	x &= UINT8_C(0xff);
#ifdef _MSC_VER
	unsigned long Index;
	return _BitScanForward(&Index, x) ? Index : 8;
#elif defined(__GNUC__) || __has_builtin(__builtin_ctz)
	return (x != 0) ? __builtin_ctz(x) : 8;
#endif
}
#endif // _MSC_VER || __GNUC__ || __has_builtin(__builtin_ctz)

LELY_UTIL_BITS_INLINE int
cts16(uint_least16_t x)
{
	return ctz16(~x);
}

LELY_UTIL_BITS_INLINE int
ctz16(uint_least16_t x)
{
	x &= UINT16_C(0xffff);
#ifdef _MSC_VER
	unsigned long Index;
	return _BitScanForward(&Index, x) ? Index : 16;
#elif defined(__GNUC__) || __has_builtin(__builtin_ctz)
	return (x != 0) ? __builtin_ctz(x) : 16;
#else
	return ((x & UINT8_C(0xff)) != 0) ? ctz8((uint_least8_t)x)
					  : ctz8(x >> 8) + 8;
#endif
}

LELY_UTIL_BITS_INLINE int
cts32(uint_least32_t x)
{
	return ctz32(~x);
}

LELY_UTIL_BITS_INLINE int
ctz32(uint_least32_t x)
{
	x &= UINT32_C(0xffffffff);
#ifdef _MSC_VER
	unsigned long Index;
	return _BitScanForward(&Index, x) ? Index : 32;
#elif (defined(__GNUC__) || __has_builtin(__builtin_ctz)) && __WORDSIZE == 64
	return (x != 0) ? __builtin_ctz(x) : 32;
#elif defined(__GNUC__) || __has_builtin(__builtin_ctzl)
	return (x != 0) ? __builtin_ctzl(x) : 32;
#else
	// clang-format off
	return (x & UINT16_C(0xffff))
			? ctz16((uint_least16_t)x) : ctz16(x >> 16) + 16;
	// clang-format on
#endif
}

LELY_UTIL_BITS_INLINE int
cts64(uint_least64_t x)
{
	return ctz64(~x);
}

LELY_UTIL_BITS_INLINE int
ctz64(uint_least64_t x)
{
	x &= UINT64_C(0xffffffffffffffff);
#if (defined(__GNUC__) || __has_builtin(__builtin_ctzl)) && LONG_BIT == 64
	return (x != 0) ? __builtin_ctzl(x) : 64;
#elif defined(__GNUC__) || __has_builtin(__builtin_ctzll)
	return (x != 0) ? __builtin_ctzll(x) : 64;
#else
	// clang-format off
	return (x & UINT32_C(0xffffffff))
			? ctz32((uint_least32_t)x) : ctz32(x >> 32) + 32;
	// clang-format on
#endif
}

#if defined(_MSC_VER) || defined(__GNUC__) || __has_builtin(__builtin_ffs)
LELY_UTIL_BITS_INLINE int
ffs8(uint_least8_t x)
{
	x &= UINT8_C(0xff);
#ifdef _MSC_VER
	unsigned long Index;
	return _BitScanForward(&Index, x) ? Index + 1 : 0;
#elif defined(__GNUC__) || __has_builtin(__builtin_ffs)
	return __builtin_ffs(x);
#endif
}
#endif // _MSC_VER || __GNUC__ || __has_builtin(__builtin_ffs)

LELY_UTIL_BITS_INLINE int
ffz8(uint_least8_t x)
{
	return ffs8(~x);
}

LELY_UTIL_BITS_INLINE int
ffs16(uint_least16_t x)
{
	x &= UINT16_C(0xffff);
#ifdef _MSC_VER
	unsigned long Index;
	return _BitScanForward(&Index, x) ? Index + 1 : 0;
#elif defined(__GNUC__) || __has_builtin(__builtin_ffs)
	return __builtin_ffs(x);
#else
	// clang-format off
	return (x != 0) ? ((x & UINT8_C(0xff))
			? ffs8((uint_least8_t)x) : ffs8(x >> 8) + 8) : 0;
	// clang-format on
#endif
}

LELY_UTIL_BITS_INLINE int
ffz16(uint_least16_t x)
{
	return ffs16(~x);
}

LELY_UTIL_BITS_INLINE int
ffs32(uint_least32_t x)
{
	x &= UINT32_C(0xffffffff);
#ifdef _MSC_VER
	unsigned long Index;
	return _BitScanForward(&Index, x) ? Index + 1 : 0;
#elif (defined(__GNUC__) || __has_builtin(__builtin_ffs)) && __WORDSIZE == 64
	return __builtin_ffs(x);
#elif defined(__GNUC__) || __has_builtin(__builtin_ffsl)
	return __builtin_ffsl(x);
#else
	// clang-format off
	return (x != 0) ? ((x & UINT16_C(0xffff))
			? ffs16((uint_least16_t)x) : ffs16(x >> 16) + 16) : 0;
	// clang-format on
#endif
}

LELY_UTIL_BITS_INLINE int
ffz32(uint_least32_t x)
{
	return ffs32(~x);
}

LELY_UTIL_BITS_INLINE int
ffs64(uint_least64_t x)
{
	x &= UINT64_C(0xffffffffffffffff);
#if defined(_MSC_VER) && _WIN64
	unsigned long Index;
	return _BitScanForward64(&Index, x) ? Index + 1 : 0;
#elif (defined(__GNUC__) || __has_builtin(__builtin_ffsl)) && LONG_BIT == 64
	return __builtin_ffsl(x);
#elif defined(__GNUC__) || __has_builtin(__builtin_ffsll)
	return __builtin_ffsll(x);
#else
	// clang-format off
	return (x != 0) ? ((x & UINT32_C(0xffffffff))
			? ffs32((uint_least32_t)x) : ffs32(x >> 32) + 32) : 0;
	// clang-format on
#endif
}

LELY_UTIL_BITS_INLINE int
ffz64(uint_least64_t x)
{
	return ffs64(~x);
}

#if defined(__GNUC__) || __has_builtin(__builtin_parity)
LELY_UTIL_BITS_INLINE int
parity8(uint_least8_t x)
{
	x &= UINT8_C(0xff);
	return __builtin_parity(x);
}
#endif // __GNUC__ || __has_builtin(__builtin_parity)

LELY_UTIL_BITS_INLINE int
parity16(uint_least16_t x)
{
	x &= UINT16_C(0xffff);
#if defined(__GNUC__) || __has_builtin(__builtin_parity)
	return __builtin_parity(x);
#else
	return parity8((uint_least8_t)x) ^ parity8(x >> 8);
#endif
}

LELY_UTIL_BITS_INLINE int
parity32(uint_least32_t x)
{
	x &= UINT32_C(0xffffffff);
#if (defined(__GNUC__) || __has_builtin(__builtin_parity)) && __WORDSIZE == 64
	return __builtin_parity(x);
#elif defined(__GNUC__) || __has_builtin(__builtin_parityl)
	return __builtin_parityl(x);
#else
	return parity16((uint_least16_t)x) ^ parity16(x >> 16);
#endif
}

LELY_UTIL_BITS_INLINE int
parity64(uint_least64_t x)
{
	x &= UINT64_C(0xffffffffffffffff);
#if (defined(__GNUC__) || __has_builtin(__builtin_parityl)) && LONG_BIT == 64
	return __builtin_parityl(x);
#elif defined(__GNUC__) || __has_builtin(__builtin_parityll)
	return __builtin_parityll(x);
#else
	return parity32((uint_least32_t)x) ^ parity32(x >> 32);
#endif
}

#if defined(__GNUC__) || __has_builtin(__builtin_popcount)
LELY_UTIL_BITS_INLINE int
popcount8(uint_least8_t x)
{
	x &= UINT8_C(0xff);
	return __builtin_popcount(x);
}
#endif // __GNUC__ || __has_builtin(__builtin_popcount)

LELY_UTIL_BITS_INLINE int
popcount16(uint_least16_t x)
{
	x &= UINT16_C(0xffff);
#if defined(__GNUC__) || __has_builtin(__builtin_popcount)
	return __builtin_popcount(x);
#else
	return popcount8((uint_least8_t)x) + popcount8(x >> 8);
#endif
}

LELY_UTIL_BITS_INLINE int
popcount32(uint_least32_t x)
{
	x &= UINT32_C(0xffffffff);
#if (defined(__GNUC__) || __has_builtin(__builtin_popcount)) && __WORDSIZE == 64
	return __builtin_popcount(x);
#elif defined(__GNUC__) || __has_builtin(__builtin_popcountl)
	return __builtin_popcountl(x);
#else
	return popcount16((uint_least16_t)x) + popcount16(x >> 16);
#endif
}

LELY_UTIL_BITS_INLINE int
popcount64(uint_least64_t x)
{
	x &= UINT64_C(0xffffffffffffffff);
#if (defined(__GNUC__) || __has_builtin(__builtin_popcountl)) && LONG_BIT == 64
	return __builtin_popcountl(x);
#elif defined(__GNUC__) || __has_builtin(__builtin_popcountll)
	return __builtin_popcountll(x);
#else
	return popcount32((uint_least32_t)x) + popcount32(x >> 32);
#endif
}

LELY_UTIL_BITS_INLINE uint_least8_t
rol8(uint_least8_t x, unsigned int n)
{
	x &= UINT8_C(0xff);
	n %= 8;
#ifdef _MSC_VER
	return _rotl8(x, n);
#else
	return (n != 0) ? (uint_least8_t)((x << n) | (x >> (8U - n))) : x;
#endif
}

LELY_UTIL_BITS_INLINE uint_least8_t
ror8(uint_least8_t x, unsigned int n)
{
	x &= UINT8_C(0xff);
	n %= 8;
#ifdef _MSC_VER
	return _rotr8(x, n);
#else
	return (n != 0) ? (uint_least8_t)((x >> n) | (x << (8U - n))) : x;
#endif
}

LELY_UTIL_BITS_INLINE uint_least16_t
rol16(uint_least16_t x, unsigned int n)
{
	x &= UINT16_C(0xffff);
	n %= 16;
#ifdef _MSC_VER
	return _rotl16(x, n);
#else
	return (n != 0) ? (uint_least16_t)((x << n) | (x >> (16U - n))) : x;
#endif
}

LELY_UTIL_BITS_INLINE uint_least16_t
ror16(uint_least16_t x, unsigned int n)
{
	x &= UINT16_C(0xffff);
	n %= 16;
#ifdef _MSC_VER
	return _rotr16(x, n);
#else
	return (n != 0) ? (uint_least16_t)((x >> n) | (x << (16U - n))) : x;
#endif
}

LELY_UTIL_BITS_INLINE uint_least32_t
rol32(uint_least32_t x, unsigned int n)
{
	x &= UINT32_C(0xffffffff);
	n %= 32;
#ifdef _MSC_VER
	return _rotl(x, n);
#else
	return (n != 0) ? (uint_least32_t)((x << n) | (x >> (32U - n))) : x;
#endif
}

LELY_UTIL_BITS_INLINE uint_least32_t
ror32(uint_least32_t x, unsigned int n)
{
	x &= UINT32_C(0xffffffff);
	n %= 32;
#ifdef _MSC_VER
	return _rotr(x, n);
#else
	return (n != 0) ? (uint_least32_t)((x >> n) | (x << (32U - n))) : x;
#endif
}

LELY_UTIL_BITS_INLINE uint_least64_t
rol64(uint_least64_t x, unsigned int n)
{
	x &= UINT64_C(0xffffffffffffffff);
	n %= 64;
#ifdef _MSC_VER
	return _rotl64(x, n);
#else
	return (n != 0) ? (uint_least64_t)((x << n) | (x >> (64U - n))) : x;
#endif
}

LELY_UTIL_BITS_INLINE uint_least64_t
ror64(uint_least64_t x, unsigned int n)
{
	x &= UINT64_C(0xffffffffffffffff);
	n %= 64;
#ifdef _MSC_VER
	return _rotr64(x, n);
#else
	return (n != 0) ? (uint_least64_t)((x >> n) | (x << (64U - n))) : x;
#endif
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_BITS_H_
