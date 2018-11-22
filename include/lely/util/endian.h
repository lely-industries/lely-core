/**@file
 * This header file is part of the utilities library; it contains the byte order
 * (endianness) function definitions.
 *
 * @copyright 2013-2018 Lely Industries N.V.
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

#ifndef LELY_UTIL_ENDIAN_H_
#define LELY_UTIL_ENDIAN_H_

#include <lely/util/bits.h>
#include <lely/util/float.h>

#include <string.h>

#ifndef LELY_UTIL_ENDIAN_INLINE
#define LELY_UTIL_ENDIAN_INLINE static inline
#endif

#ifndef LELY_BIG_ENDIAN
#if defined(__BIG_ENDIAN__) || defined(__big_endian__) \
		|| (__GNUC__ && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) \
		|| defined(__ARMEB__) || defined(__AARCH64EB__) \
		|| defined(__THUMBEB__)
/// The target platform is big-endian.
#define LELY_BIG_ENDIAN 1
#endif
#endif

#if LELY_BIG_ENDIAN
#undef LELY_LITTLE_ENDIAN
#endif

#ifndef LELY_LITTLE_ENDIAN
#if defined(__LITTLE_ENDIAN__) || defined(__little_endian__) \
		|| (__GNUC__ && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) \
		|| defined(__i386__) || defined(_M_IX86) \
		|| defined(__x86_64__) || defined(_M_AMD64) \
		|| defined(__ARMEL__) || defined(__AARCH64EL__) \
		|| defined(__THUMBEL__)
/// The target platform is little-endian.
#define LELY_LITTLE_ENDIAN 1
#endif
#endif

#if LELY_LITTLE_ENDIAN
#undef LELY_BIG_ENDIAN
#endif

#if !LELY_BIG_ENDIAN && !LELY_LITTLE_ENDIAN
#error Unable to determine byte order or byte order is not supported.
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef htobe16
/// Converts a 16-bit unsigned integer from host to big-endian byte order.
LELY_UTIL_ENDIAN_INLINE uint_least16_t htobe16(uint_least16_t x);
#endif

#ifndef betoh16
/// Converts a 16-bit unsigned integer from big-endian to host byte order.
LELY_UTIL_ENDIAN_INLINE uint_least16_t betoh16(uint_least16_t x);
#endif

#ifndef htole16
/// Converts a 16-bit unsigned integer from host to little-endian byte order.
LELY_UTIL_ENDIAN_INLINE uint_least16_t htole16(uint_least16_t x);
#endif

#ifndef letoh16
/// Converts a 16-bit unsigned integer from little-endian to host byte order.
LELY_UTIL_ENDIAN_INLINE uint_least16_t letoh16(uint_least16_t x);
#endif

#ifndef htobe32
/// Converts a 32-bit unsigned integer from host to big-endian byte order.
LELY_UTIL_ENDIAN_INLINE uint_least32_t htobe32(uint_least32_t x);
#endif

#ifndef betoh32
/// Converts a 32-bit unsigned integer from big-endian to host byte order.
LELY_UTIL_ENDIAN_INLINE uint_least32_t betoh32(uint_least32_t x);
#endif

#ifndef htole32
/// Converts a 32-bit unsigned integer from host to little-endian byte order.
LELY_UTIL_ENDIAN_INLINE uint_least32_t htole32(uint_least32_t x);
#endif

#ifndef letoh32
/// Converts a 32-bit unsigned integer from little-endian to host byte order.
LELY_UTIL_ENDIAN_INLINE uint_least32_t letoh32(uint_least32_t x);
#endif

#ifndef htobe64
/// Converts a 64-bit unsigned integer from host to big-endian byte order.
LELY_UTIL_ENDIAN_INLINE uint_least64_t htobe64(uint_least64_t x);
#endif

#ifndef betoh64
/// Converts a 64-bit unsigned integer from big-endian to host byte order.
LELY_UTIL_ENDIAN_INLINE uint_least64_t betoh64(uint_least64_t x);
#endif

#ifndef htole64
/// Converts a 64-bit unsigned integer from host to little-endian byte order.
LELY_UTIL_ENDIAN_INLINE uint_least64_t htole64(uint_least64_t x);
#endif

#ifndef letoh64
/// Converts a 64-bit unsigned integer from little-endian to host byte order.
LELY_UTIL_ENDIAN_INLINE uint_least64_t letoh64(uint_least64_t x);
#endif

/// Stores a 16-bit signed integer in big-endian byte order.
LELY_UTIL_ENDIAN_INLINE void stbe_i16(void *ptr, int_least16_t x);

/// Loads a 16-bit signed integer in big-endian byte order.
LELY_UTIL_ENDIAN_INLINE int_least16_t ldbe_i16(const void *ptr);

/// Stores a 16-bit unsigned integer in big-endian byte order.
LELY_UTIL_ENDIAN_INLINE void stbe_u16(void *ptr, uint_least16_t x);

/// Loads a 16-bit unsigned integer in big-endian byte order.
LELY_UTIL_ENDIAN_INLINE uint_least16_t ldbe_u16(const void *ptr);

/// Stores a 16-bit signed integer in little-endian byte order.
LELY_UTIL_ENDIAN_INLINE void stle_i16(void *ptr, int_least16_t x);

/// Loads a 16-bit signed integer in little-endian byte order.
LELY_UTIL_ENDIAN_INLINE int_least16_t ldle_i16(const void *ptr);

/// Stores a 16-bit unsigned integer in little-endian byte order.
LELY_UTIL_ENDIAN_INLINE void stle_u16(void *ptr, uint_least16_t x);

/// Loads a 16-bit unsigned integer in little-endian byte order.
LELY_UTIL_ENDIAN_INLINE uint_least16_t ldle_u16(const void *ptr);

/// Stores a 32-bit signed integer in big-endian byte order.
LELY_UTIL_ENDIAN_INLINE void stbe_i32(void *ptr, int_least32_t x);

/// Loads a 32-bit signed integer in big-endian byte order.
LELY_UTIL_ENDIAN_INLINE int_least32_t ldbe_i32(const void *ptr);

/// Stores a 32-bit unsigned integer in big-endian byte order.
LELY_UTIL_ENDIAN_INLINE void stbe_u32(void *ptr, uint_least32_t x);

/// Loads a 32-bit unsigned integer in big-endian byte order.
LELY_UTIL_ENDIAN_INLINE uint_least32_t ldbe_u32(const void *ptr);

/// Stores a 32-bit signed integer in little-endian byte order.
LELY_UTIL_ENDIAN_INLINE void stle_i32(void *ptr, int_least32_t x);

/// Loads a 32-bit signed integer in little-endian byte order.
LELY_UTIL_ENDIAN_INLINE int_least32_t ldle_i32(const void *ptr);

/// Stores a 32-bit unsigned integer in little-endian byte order.
LELY_UTIL_ENDIAN_INLINE void stle_u32(void *ptr, uint_least32_t x);

/// Loads a 32-bit unsigned integer in little-endian byte order.
LELY_UTIL_ENDIAN_INLINE uint_least32_t ldle_u32(const void *ptr);

/// Stores a 64-bit signed integer in big-endian byte order.
LELY_UTIL_ENDIAN_INLINE void stbe_i64(void *ptr, int_least64_t x);

/// Loads a 64-bit signed integer in big-endian byte order.
LELY_UTIL_ENDIAN_INLINE int_least64_t ldbe_i64(const void *ptr);

/// Stores a 64-bit unsigned integer in big-endian byte order.
LELY_UTIL_ENDIAN_INLINE void stbe_u64(void *ptr, uint_least64_t x);

/// Loads a 64-bit unsigned integer in big-endian byte order.
LELY_UTIL_ENDIAN_INLINE uint_least64_t ldbe_u64(const void *ptr);

/// Stores a 64-bit signed integer in little-endian byte order.
LELY_UTIL_ENDIAN_INLINE void stle_i64(void *ptr, int_least64_t x);

/// Loads a 64-bit signed integer in little-endian byte order.
LELY_UTIL_ENDIAN_INLINE int_least64_t ldle_i64(const void *ptr);

/// Stores a 64-bit unsigned integer in little-endian byte order.
LELY_UTIL_ENDIAN_INLINE void stle_u64(void *ptr, uint_least64_t x);

/// Loads a 64-bit unsigned integer in little-endian byte order.
LELY_UTIL_ENDIAN_INLINE uint_least64_t ldle_u64(const void *ptr);

#ifdef LELY_FLT16_TYPE

/**
 * Stores an IEEE 754 half-precision binary floating-point number in big-endian
 * byte order.
 */
LELY_UTIL_ENDIAN_INLINE void stbe_flt16(void *ptr, flt16_t x);

/**
 * Loads an IEEE 754 half-precision binary floating-point number in big-endian
 * byte order.
 */
LELY_UTIL_ENDIAN_INLINE flt16_t ldbe_flt16(const void *ptr);

/**
 * Stores an IEEE 754 half-precision binary floating-point number in
 * little-endian byte order.
 */
LELY_UTIL_ENDIAN_INLINE void stle_flt16(void *ptr, flt16_t x);

/**
 * Loads an IEEE 754 half-precision binary floating-point number in
 * little-endian byte order.
 */
LELY_UTIL_ENDIAN_INLINE flt16_t ldle_flt16(const void *ptr);

#endif // LELY_FLT16_TYPE

#ifdef LELY_FLT32_TYPE

/**
 * Stores an IEEE 754 single-precision binary floating-point number in
 * big-endian byte order.
 */
LELY_UTIL_ENDIAN_INLINE void stbe_flt32(void *ptr, flt32_t x);

/**
 * Loads an IEEE 754 single-precision binary floating-point number in
 * big-endian byte order.
 */
LELY_UTIL_ENDIAN_INLINE flt32_t ldbe_flt32(const void *ptr);

/**
 * Stores an IEEE 754 single-precision binary floating-point number in
 * little-endian byte order.
 */
LELY_UTIL_ENDIAN_INLINE void stle_flt32(void *ptr, flt32_t x);

/**
 * Loads an IEEE 754 single-precision binary floating-point number in
 * little-endian byte order.
 */
LELY_UTIL_ENDIAN_INLINE flt32_t ldle_flt32(const void *ptr);

#endif // LELY_FLT32_TYPE

#ifdef LELY_FLT64_TYPE

/**
 * Stores an IEEE 754 double-precision binary floating-point number in
 * big-endian byte order.
 */
LELY_UTIL_ENDIAN_INLINE void stbe_flt64(void *ptr, flt64_t x);

/**
 * Loads an IEEE 754 double-precision binary floating-point number in
 * big-endian byte order.
 */
LELY_UTIL_ENDIAN_INLINE flt64_t ldbe_flt64(const void *ptr);

/**
 * Stores an IEEE 754 double-precision binary floating-point number in
 * little-endian byte order.
 */
LELY_UTIL_ENDIAN_INLINE void stle_flt64(void *ptr, flt64_t x);

/**
 * Loads an IEEE 754 double-precision binary floating-point number in
 * little-endian byte order.
 */
LELY_UTIL_ENDIAN_INLINE flt64_t ldle_flt64(const void *ptr);

#endif // LELY_FLT64_TYPE

/**
 * Copies <b>n</b> bits from a source to a destination buffer. The buffers MUST
 * NOT overlap. This function assumes a big-endian bit ordering (i.e., bit 0 is
 * the most significant bit).
 *
 * @param dst    a pointer to the destination buffer.
 * @param dstbit the destination offset (in bits) with respect to <b>dst</b>.
 * @param src    a pointer to the source buffer.
 * @param srcbit the source offset (in bits) with respect to <b>src</b>.
 * @param n      the number of bits to copy.
 */
void bcpybe(void *dst, int dstbit, const void *src, int srcbit, size_t n);

/**
 * Copies <b>n</b> bits from a source to a destination buffer. The buffers MUST
 * NOT overlap. This function assumes a little-endian bit ordering (i.e., bit 0
 * is the least significant bit).
 *
 * @param dst    a pointer to the destination buffer.
 * @param dstbit the destination offset (in bits) with respect to <b>dst</b>.
 * @param src    a pointer to the source buffer.
 * @param srcbit the source offset (in bits) with respect to <b>src</b>.
 * @param n      the number of bits to copy.
 */
void bcpyle(void *dst, int dstbit, const void *src, int srcbit, size_t n);

#ifndef htobe16
inline uint_least16_t
htobe16(uint_least16_t x)
{
	x &= UINT16_C(0xffff);
#if LELY_BIG_ENDIAN
	return x;
#elif LELY_LITTLE_ENDIAN
	return bswap16(x);
#endif
}
#endif

#ifndef betoh16
inline uint_least16_t
betoh16(uint_least16_t x)
{
	return htobe16(x);
}
#endif

#ifndef htole16
inline uint_least16_t
htole16(uint_least16_t x)
{
	x &= UINT16_C(0xffff);
#if LELY_BIG_ENDIAN
	return bswap16(x);
#elif LELY_LITTLE_ENDIAN
	return x;
#endif
}
#endif

#ifndef letoh16
inline uint_least16_t
letoh16(uint_least16_t x)
{
	return htole16(x);
}
#endif

#ifndef htobe32
inline uint_least32_t
htobe32(uint_least32_t x)
{
	x &= UINT32_C(0xffffffff);
#if LELY_BIG_ENDIAN
	return x;
#elif LELY_LITTLE_ENDIAN
	return bswap32(x);
#endif
}
#endif

#ifndef betoh32
inline uint_least32_t
betoh32(uint_least32_t x)
{
	return htobe32(x);
}
#endif

#ifndef htole32
inline uint_least32_t
htole32(uint_least32_t x)
{
	x &= UINT32_C(0xffffffff);
#if LELY_BIG_ENDIAN
	return bswap32(x);
#elif LELY_LITTLE_ENDIAN
	return x;
#endif
}
#endif

#ifndef letoh32
inline uint_least32_t
letoh32(uint_least32_t x)
{
	return htole32(x);
}
#endif

#ifndef htobe64
inline uint_least64_t
htobe64(uint_least64_t x)
{
	x &= UINT64_C(0xffffffffffffffff);
#if LELY_BIG_ENDIAN
	return x;
#elif LELY_LITTLE_ENDIAN
	return bswap64(x);
#endif
}
#endif

#ifndef betoh64
inline uint_least64_t
betoh64(uint_least64_t x)
{
	return htobe64(x);
}
#endif

#ifndef htole64
inline uint_least64_t
htole64(uint_least64_t x)
{
	x &= UINT64_C(0xffffffffffffffff);
#if LELY_BIG_ENDIAN
	return bswap64(x);
#elif LELY_LITTLE_ENDIAN
	return x;
#endif
}
#endif

#ifndef letoh64
inline uint_least64_t
letoh64(uint_least64_t x)
{
	return htole64(x);
}
#endif

inline void
stbe_i16(void *ptr, int_least16_t x)
{
	stbe_u16(ptr, x);
}

inline int_least16_t
ldbe_i16(const void *ptr)
{
	return ldbe_u16(ptr);
}

inline void
stbe_u16(void *ptr, uint_least16_t x)
{
	x = htobe16(x);
	memcpy(ptr, &x, sizeof(x));
}

inline uint_least16_t
ldbe_u16(const void *ptr)
{
	uint_least16_t x = 0;
	memcpy(&x, ptr, sizeof(x));
	return betoh16(x);
}

inline void
stle_i16(void *ptr, int_least16_t x)
{
	stle_u16(ptr, x);
}

inline int_least16_t
ldle_i16(const void *ptr)
{
	return ldle_u16(ptr);
}

inline void
stle_u16(void *ptr, uint_least16_t x)
{
	x = htole16(x);
	memcpy(ptr, &x, sizeof(x));
}

inline uint_least16_t
ldle_u16(const void *ptr)
{
	uint_least16_t x = 0;
	memcpy(&x, ptr, sizeof(x));
	return letoh16(x);
}

inline void
stbe_i32(void *ptr, int_least32_t x)
{
	stbe_u32(ptr, x);
}

inline int_least32_t
ldbe_i32(const void *ptr)
{
	return ldbe_u32(ptr);
}

inline void
stbe_u32(void *ptr, uint_least32_t x)
{
	x = htobe32(x);
	memcpy(ptr, &x, sizeof(x));
}

inline uint_least32_t
ldbe_u32(const void *ptr)
{
	uint_least32_t x = 0;
	memcpy(&x, ptr, sizeof(x));
	return betoh32(x);
}

inline void
stle_i32(void *ptr, int_least32_t x)
{
	stle_u32(ptr, x);
}

inline int_least32_t
ldle_i32(const void *ptr)
{
	return ldle_u32(ptr);
}

inline void
stle_u32(void *ptr, uint_least32_t x)
{
	x = htole32(x);
	memcpy(ptr, &x, sizeof(x));
}

inline uint_least32_t
ldle_u32(const void *ptr)
{
	uint_least32_t x = 0;
	memcpy(&x, ptr, sizeof(x));
	return letoh32(x);
}

inline void
stbe_i64(void *ptr, int_least64_t x)
{
	stbe_u64(ptr, x);
}

inline int_least64_t
ldbe_i64(const void *ptr)
{
	return ldbe_u64(ptr);
}

inline void
stbe_u64(void *ptr, uint_least64_t x)
{
	x = htobe64(x);
	memcpy(ptr, &x, sizeof(x));
}

inline uint_least64_t
ldbe_u64(const void *ptr)
{
	uint_least64_t x = 0;
	memcpy(&x, ptr, sizeof(x));
	return betoh64(x);
}

inline void
stle_i64(void *ptr, int_least64_t x)
{
	stle_u64(ptr, x);
}

inline int_least64_t
ldle_i64(const void *ptr)
{
	return ldle_u64(ptr);
}

inline void
stle_u64(void *ptr, uint_least64_t x)
{
	x = htole64(x);
	memcpy(ptr, &x, sizeof(x));
}

inline uint_least64_t
ldle_u64(const void *ptr)
{
	uint_least64_t x = 0;
	memcpy(&x, ptr, sizeof(x));
	return letoh64(x);
}

#ifdef LELY_FLT16_TYPE

inline void
stbe_flt16(void *ptr, flt16_t x)
{
	uint_least16_t tmp = 0;
	memcpy(&tmp, &x, sizeof(x));
	stbe_u16(ptr, tmp);
}

inline flt16_t
ldbe_flt16(const void *ptr)
{
	flt16_t x = 0;
	uint_least16_t tmp = ldbe_u16(ptr);
	memcpy(&x, &tmp, sizeof(x));
	return x;
}

inline void
stle_flt16(void *ptr, flt16_t x)
{
	uint_least16_t tmp = 0;
	memcpy(&tmp, &x, sizeof(x));
	stle_u16(ptr, tmp);
}

inline flt16_t
ldle_flt16(const void *ptr)
{
	flt16_t x = 0;
	uint_least16_t tmp = ldle_u16(ptr);
	memcpy(&x, &tmp, sizeof(x));
	return x;
}

#endif // LELY_FLT16_TYPE

#ifdef LELY_FLT32_TYPE

inline void
stbe_flt32(void *ptr, flt32_t x)
{
	uint_least32_t tmp = 0;
	memcpy(&tmp, &x, sizeof(x));
	stbe_u32(ptr, tmp);
}

inline flt32_t
ldbe_flt32(const void *ptr)
{
	flt32_t x = 0;
	uint_least32_t tmp = ldbe_u32(ptr);
	memcpy(&x, &tmp, sizeof(x));
	return x;
}

inline void
stle_flt32(void *ptr, flt32_t x)
{
	uint_least32_t tmp = 0;
	memcpy(&tmp, &x, sizeof(x));
	stle_u32(ptr, tmp);
}

inline flt32_t
ldle_flt32(const void *ptr)
{
	flt32_t x = 0;
	uint_least32_t tmp = ldle_u32(ptr);
	memcpy(&x, &tmp, sizeof(x));
	return x;
}

#endif // LELY_FLT32_TYPE

#ifdef LELY_FLT64_TYPE

inline void
stbe_flt64(void *ptr, flt64_t x)
{
	uint_least64_t tmp = 0;
	memcpy(&tmp, &x, sizeof(x));
	stbe_u64(ptr, tmp);
}

inline flt64_t
ldbe_flt64(const void *ptr)
{
	flt64_t x = 0;
	uint_least64_t tmp = ldbe_u64(ptr);
	memcpy(&x, &tmp, sizeof(x));
	return x;
}

inline void
stle_flt64(void *ptr, flt64_t x)
{
	uint_least64_t tmp = 0;
	memcpy(&tmp, &x, sizeof(x));
	stle_u64(ptr, tmp);
}

inline flt64_t
ldle_flt64(const void *ptr)
{
	flt64_t x = 0;
	uint_least64_t tmp = ldle_u64(ptr);
	memcpy(&x, &tmp, sizeof(x));
	return x;
}

#endif // LELY_FLT64_TYPE

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_ENDIAN_H_
