/*!\file
 * This header file is part of the utilities library; it contains the byte order
 * (endianness) function definitions.
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

#ifndef LELY_UTIL_ENDIAN_H
#define LELY_UTIL_ENDIAN_H

#include <lely/libc/stdint.h>
#include <lely/util/util.h>

#ifdef _MSC_VER
#include <stdlib.h>
#endif
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Reverses the byte order of a 16-bit unsigned int.
static inline uint16_t bswap_u16(uint16_t i);

//! Reverses the byte order of a 32-bit unsigned int.
static inline uint32_t bswap_u32(uint32_t i);

//! Reverses the byte order of a 64-bit unsigned int.
static inline uint64_t bswap_u64(uint64_t i);

//! Converts a 16-bit unsigned int from host to big-endian byte order.
static inline uint16_t htobe_u16(uint16_t i);

//! Converts a 16-bit unsigned int from big-endian to host byte order.
static inline uint16_t betoh_u16(uint16_t i);

//! Converts a 16-bit unsigned int from host to little-endian byte order.
static inline uint16_t htole_u16(uint16_t i);

//! Converts a 16-bit unsigned int from little-endian to host byte order.
static inline uint16_t letoh_u16(uint16_t i);

//! Converts a 16-bit unsigned int from host to network byte order.
static inline uint16_t hton_u16(uint16_t i);

//! Converts a 16-bit unsigned int from network to host byte order.
static inline uint16_t ntoh_u16(uint16_t i);

//! Converts a 32-bit unsigned int from host to big-endian byte order.
static inline uint32_t htobe_u32(uint32_t i);

//! Converts a 32-bit unsigned int from big-endian to host byte order.
static inline uint32_t betoh_u32(uint32_t i);

//! Converts a 32-bit unsigned int from host to little-endian byte order.
static inline uint32_t htole_u32(uint32_t i);

//! Converts a 32-bit unsigned int from little-endian to host byte order.
static inline uint32_t letoh_u32(uint32_t i);

//! Converts a 32-bit unsigned int from host to network byte order.
static inline uint32_t hton_u32(uint32_t i);

//! Converts a 32-bit unsigned int from network to host byte order.
static inline uint32_t ntoh_u32(uint32_t i);

//! Converts a 64-bit unsigned int from host to big-endian byte order.
static inline uint64_t htobe_u64(uint64_t i);

//! Converts a 64-bit unsigned int from big-endian to host byte order.
static inline uint64_t betoh_u64(uint64_t i);

//! Converts a 64-bit unsigned int from host to little-endian byte order.
static inline uint64_t htole_u64(uint64_t i);

//! Converts a 64-bit unsigned int from little-endian to host byte order.
static inline uint64_t letoh_u64(uint64_t i);

//! Converts a 64-bit unsigned int from host to network byte order.
static inline uint64_t hton_u64(uint64_t i);

//! Converts a 64-bit unsigned int from network to host byte order.
static inline uint64_t ntoh_u64(uint64_t i);

//! Loads a 16-bit signed int in big-endian byte order.
static inline int16_t ldbe_i16(const void *ptr);

//! Stores a 16-bit signed int in big-endian byte order.
static inline void stbe_i16(void *ptr, int16_t i);

//! Loads a 16-bit unsigned int in big-endian byte order.
static inline uint16_t ldbe_u16(const void *ptr);

//! Stores a 16-bit unsigned int in big-endian byte order.
static inline void stbe_u16(void *ptr, uint16_t i);

//! Loads a 16-bit signed int in little-endian byte order.
static inline int16_t ldle_i16(const void *ptr);

//! Stores a 16-bit signed int in little-endian byte order.
static inline void stle_i16(void *ptr, int16_t i);

//! Loads a 16-bit unsigned int in little-endian byte order.
static inline uint16_t ldle_u16(const void *ptr);

//! Stores a 16-bit unsigned int in little-endian byte order.
static inline void stle_u16(void *ptr, uint16_t i);

//! Loads a 16-bit signed int in network byte order.
static inline int16_t ldn_i16(const void *ptr);

//! Stores a 16-bit signed int in network byte order.
static inline void stn_i16(void *ptr, int16_t i);

//! Loads a 16-bit unsigned int in network byte order.
static inline uint16_t ldn_u16(const void *ptr);

//! Stores a 16-bit unsigned int in network byte order.
static inline void stn_u16(void *ptr, uint16_t i);

//! Loads a 32-bit signed int in big-endian byte order.
static inline int32_t ldbe_i32(const void *ptr);

//! Stores a 32-bit signed int in big-endian byte order.
static inline void stbe_i32(void *ptr, int32_t i);

//! Loads a 32-bit unsigned int in big-endian byte order.
static inline uint32_t ldbe_u32(const void *ptr);

//! Stores a 32-bit unsigned int in big-endian byte order.
static inline void stbe_u32(void *ptr, uint32_t i);

//! Loads a 32-bit signed int in little-endian byte order.
static inline int32_t ldle_i32(const void *ptr);

//! Stores a 32-bit signed int in little-endian byte order.
static inline void stle_i32(void *ptr, int32_t i);

//! Loads a 32-bit unsigned int in little-endian byte order.
static inline uint32_t ldle_u32(const void *ptr);

//! Stores a 32-bit unsigned int in little-endian byte order.
static inline void stle_u32(void *ptr, uint32_t i);

//! Loads a 32-bit signed int in network byte order.
static inline int32_t ldn_i32(const void *ptr);

//! Stores a 32-bit signed int in network byte order.
static inline void stn_i32(void *ptr, int32_t i);

//! Loads a 32-bit unsigned int in network byte order.
static inline uint32_t ldn_u32(const void *ptr);

//! Stores a 32-bit unsigned int in network byte order.
static inline void stn_u32(void *ptr, uint32_t i);

//! Loads a 64-bit signed int in big-endian byte order.
static inline int64_t ldbe_i64(const void *ptr);

//! Stores a 64-bit signed int in big-endian byte order.
static inline void stbe_i64(void *ptr, int64_t i);

//! Loads a 64-bit unsigned int in big-endian byte order.
static inline uint64_t ldbe_u64(const void *ptr);

//! Stores a 64-bit unsigned int in big-endian byte order.
static inline void stbe_u64(void *ptr, uint64_t i);

//! Loads a 64-bit signed int in little-endian byte order.
static inline int64_t ldle_i64(const void *ptr);

//! Stores a 64-bit signed int in little-endian byte order.
static inline void stle_i64(void *ptr, int64_t i);

//! Loads a 64-bit unsigned int in little-endian byte order.
static inline uint64_t ldle_u64(const void *ptr);

//! Stores a 64-bit unsigned int in little-endian byte order.
static inline void stle_u64(void *ptr, uint64_t i);

//! Loads a 64-bit signed int in network byte order.
static inline int64_t ldn_i64(const void *ptr);

//! Stores a 64-bit signed int in network byte order.
static inline void stn_i64(void *ptr, int64_t i);

//! Loads a 64-bit unsigned int in network byte order.
static inline uint64_t ldn_u64(const void *ptr);

//! Stores a 64-bit unsigned int in network byte order.
static inline void stn_u64(void *ptr, uint64_t i);

//! Loads a single-precision floating-point number in big-endian byte order.
static inline float ldbe_flt(const void *ptr);

//! Stores a single-precision floating-point number in big-endian byte order.
static inline void stbe_flt(void *ptr, float f);

//! Loads a single-precision floating-point number in little-endian byte order.
static inline float ldle_flt(const void *ptr);

//! Stores a single-precision floating-point number in little-endian byte order.
static inline void stle_flt(void *ptr, float f);

//! Loads a single-precision floating-point number in network byte order.
static inline float ldn_flt(const void *ptr);

//! Stores a single-precision floating-point number in network byte order.
static inline void stn_flt(void *ptr, float f);

//! Loads a double-precision floating-point number in big-endian byte order.
static inline double ldbe_dbl(const void *ptr);

//! Stores a double-precision floating-point number in big-endian byte order.
static inline void stbe_dbl(void *ptr, double d);

//! Loads a double-precision floating-point number in little-endian byte order.
static inline double ldle_dbl(const void *ptr);

//! Stores a double-precision floating-point number in little-endian byte order.
static inline void stle_dbl(void *ptr, double d);

//! Loads a double-precision floating-point number in network byte order.
static inline double ldn_dbl(const void *ptr);

//! Stores a double-precision floating-point number in network byte order.
static inline void stn_dbl(void *ptr, double d);

static inline uint16_t
bswap_u16(uint16_t i)
{
#ifdef _MSC_VER
	return _byteswap_ushort(i);
#elif __GNUC_PREREQ(4, 8) || __has_builtin(__builtin_bswap16)
	return __builtin_bswap16(i);
#else
	return ((i & 0xff) << 8) | ((i >> 8) & 0xff);
#endif
}

static inline uint32_t
bswap_u32(uint32_t i)
{
#ifdef _MSC_VER
	return _byteswap_ulong(i);
#elif __GNUC_PREREQ(4, 3) || __has_builtin(__builtin_bswap32)
	return __builtin_bswap32(i);
#else
	return ((i & 0xff) << 24) | ((i & 0xff00) << 8) | ((i >> 8) & 0xff00)
			| ((i >> 24) & 0xff);
#endif
}

static inline uint64_t
bswap_u64(uint64_t i)
{
#ifdef _MSC_VER
	return _byteswap_uint64(i);
#elif __GNUC_PREREQ(4, 3) || __has_builtin(__builtin_bswap64)
	return __builtin_bswap64(i);
#else
	return ((i & 0xff) << 56) | ((i & 0xff00) << 40)
			| ((i & 0xff0000) << 24) | ((i & 0xff000000) << 8)
			| ((i >> 8) & 0xff000000) | ((i >> 24) & 0xff0000)
			| ((i >> 40) & 0xff00) | ((i >> 56) & 0xff);
#endif
}

static inline uint16_t
htobe_u16(uint16_t i)
{
#if LELY_BIG_ENDIAN
	return i;
#elif LELY_LITTLE_ENDIAN
	return bswap_u16(i);
#else
	uint8_t b[] = { (uint8_t)(i >> 8), (uint8_t)i };
	memcpy(&i, b, sizeof(i));
	return i;
#endif
}

static inline uint16_t betoh_u16(uint16_t i) { return htobe_u16(i); }

static inline uint16_t
htole_u16(uint16_t i)
{
#if LELY_BIG_ENDIAN
	return bswap_u16(i);
#elif LELY_LITTLE_ENDIAN
	return i;
#else
	uint8_t b[] = { (uint8_t)i, (uint8_t)(i >> 8) };
	memcpy(&i, b, sizeof(i));
	return i;
#endif
}

static inline uint16_t letoh_u16(uint16_t i) { return htole_u16(i); }

static inline uint16_t hton_u16(uint16_t i) { return htobe_u16(i); }

static inline uint16_t ntoh_u16(uint16_t i) { return hton_u16(i); }

static inline uint32_t
htobe_u32(uint32_t i)
{
#if LELY_BIG_ENDIAN
	return i;
#elif LELY_LITTLE_ENDIAN
	return bswap_u32(i);
#else
	uint8_t b[] = {
		(uint8_t)(i >> 24),
		(uint8_t)(i >> 16),
		(uint8_t)(i >> 8),
		(uint8_t)i
	};
	memcpy(&i, b, sizeof(i));
	return i;
#endif
}

static inline uint32_t betoh_u32(uint32_t i) { return htobe_u32(i); }

static inline uint32_t
htole_u32(uint32_t i)
{
#if LELY_BIG_ENDIAN
	return bswap_u32(i);
#elif LELY_LITTLE_ENDIAN
	return i;
#else
	uint8_t b[] = {
		(uint8_t)i,
		(uint8_t)(i >> 8),
		(uint8_t)(i >> 16),
		(uint8_t)(i >> 24)
	};
	memcpy(&i, b, sizeof(i));
	return i;
#endif
}

static inline uint32_t letoh_u32(uint32_t i) { return htole_u32(i); }

static inline uint32_t hton_u32(uint32_t i) { return htobe_u32(i); }

static inline uint32_t ntoh_u32(uint32_t i) { return hton_u32(i); }

static inline uint64_t
htobe_u64(uint64_t i)
{
#if LELY_BIG_ENDIAN
	return i;
#elif LELY_LITTLE_ENDIAN
	return bswap_u64(i);
#else
	uint8_t b[] = {
		(uint8_t)(i >> 56),
		(uint8_t)(i >> 48),
		(uint8_t)(i >> 40),
		(uint8_t)(i >> 32),
		(uint8_t)(i >> 24),
		(uint8_t)(i >> 16),
		(uint8_t)(i >> 8),
		(uint8_t)i
	};
	memcpy(&i, b, sizeof(i));
	return i;
#endif
}

static inline uint64_t betoh_u64(uint64_t i) { return htobe_u64(i); }

static inline uint64_t
htole_u64(uint64_t i)
{
#if LELY_BIG_ENDIAN
	return bswap_u64(i);
#elif LELY_LITTLE_ENDIAN
	return i;
#else
	uint8_t b[] = {
		(uint8_t)i,
		(uint8_t)(i >> 8),
		(uint8_t)(i >> 16),
		(uint8_t)(i >> 24),
		(uint8_t)(i >> 32),
		(uint8_t)(i >> 40),
		(uint8_t)(i >> 48),
		(uint8_t)(i >> 56)
	};
	memcpy(&i, b, sizeof(i));
	return i;
#endif
}

static inline uint64_t letoh_u64(uint64_t i) { return htole_u64(i); }

static inline uint64_t hton_u64(uint64_t i) { return htobe_u64(i); }

static inline uint64_t ntoh_u64(uint64_t i) { return hton_u64(i); }

static inline int16_t ldbe_i16(const void *ptr) { return ldbe_u16(ptr); }

static inline void stbe_i16(void *ptr, int16_t i) { stbe_u16(ptr, i); }

static inline uint16_t
ldbe_u16(const void *ptr)
{
	uint16_t i;
	memcpy(&i, ptr, sizeof(i));
	return betoh_u16(i);
}

static inline void
stbe_u16(void *ptr, uint16_t i)
{
	i = htobe_u16(i);
	memcpy(ptr, &i, sizeof(i));
}

static inline int16_t ldle_i16(const void *ptr) { return ldle_u16(ptr); }

static inline void stle_i16(void *ptr, int16_t i) { stle_u16(ptr, i); }

static inline uint16_t
ldle_u16(const void *ptr)
{
	uint16_t i;
	memcpy(&i, ptr, sizeof(i));
	return letoh_u16(i);
}

static inline void
stle_u16(void *ptr, uint16_t i)
{
	i = htole_u16(i);
	memcpy(ptr, &i, sizeof(i));
}

static inline int16_t ldn_i16(const void *ptr) { return ldn_u16(ptr); }

static inline void stn_i16(void *ptr, int16_t i) { stn_u16(ptr, i); }

static inline uint16_t
ldn_u16(const void *ptr)
{
	uint16_t i;
	memcpy(&i, ptr, sizeof(i));
	return ntoh_u16(i);
}

static inline void
stn_u16(void *ptr, uint16_t i)
{
	i = hton_u16(i);
	memcpy(ptr, &i, sizeof(i));
}

static inline int32_t ldbe_i32(const void *ptr) { return ldbe_u32(ptr); }

static inline void stbe_i32(void *ptr, int32_t i) { stbe_u32(ptr, i); }

static inline uint32_t
ldbe_u32(const void *ptr)
{
	uint32_t i;
	memcpy(&i, ptr, sizeof(i));
	return betoh_u32(i);
}

static inline void
stbe_u32(void *ptr, uint32_t i)
{
	i = htobe_u32(i);
	memcpy(ptr, &i, sizeof(i));
}

static inline int32_t ldle_i32(const void *ptr) { return ldle_u32(ptr); }

static inline void stle_i32(void *ptr, int32_t i) { stle_u32(ptr, i); }

static inline uint32_t
ldle_u32(const void *ptr)
{
	uint32_t i;
	memcpy(&i, ptr, sizeof(i));
	return letoh_u32(i);
}

static inline void
stle_u32(void *ptr, uint32_t i)
{
	i = htole_u32(i);
	memcpy(ptr, &i, sizeof(i));
}

static inline int32_t ldn_i32(const void *ptr) { return ldn_u32(ptr); }

static inline void stn_i32(void *ptr, int32_t i) { stn_u32(ptr, i); }

static inline uint32_t
ldn_u32(const void *ptr)
{
	uint32_t i;
	memcpy(&i, ptr, sizeof(i));
	return ntoh_u32(i);
}

static inline void
stn_u32(void *ptr, uint32_t i)
{
	i = hton_u32(i);
	memcpy(ptr, &i, sizeof(i));
}

static inline int64_t ldbe_i64(const void *ptr) { return ldbe_u64(ptr); }

static inline void stbe_i64(void *ptr, int64_t i) { stbe_u64(ptr, i); }

static inline uint64_t
ldbe_u64(const void *ptr)
{
	uint64_t i;
	memcpy(&i, ptr, sizeof(i));
	return betoh_u64(i);
}

static inline void
stbe_u64(void *ptr, uint64_t i)
{
	i = htobe_u64(i);
	memcpy(ptr, &i, sizeof(i));
}

static inline int64_t ldle_i64(const void *ptr) { return ldle_u64(ptr); }

static inline void stle_i64(void *ptr, int64_t i) { stle_u64(ptr, i); }

static inline uint64_t
ldle_u64(const void *ptr)
{
	uint64_t i;
	memcpy(&i, ptr, sizeof(i));
	return letoh_u64(i);
}

static inline void
stle_u64(void *ptr, uint64_t i)
{
	i = htole_u64(i);
	memcpy(ptr, &i, sizeof(i));
}

static inline int64_t ldn_i64(const void *ptr) { return ldn_u64(ptr); }

static inline void stn_i64(void *ptr, int64_t i) { stn_u64(ptr, i); }

static inline uint64_t
ldn_u64(const void *ptr)
{
	uint64_t i;
	memcpy(&i, ptr, sizeof(i));
	return ntoh_u64(i);
}

static inline void
stn_u64(void *ptr, uint64_t i)
{
	i = hton_u64(i);
	memcpy(ptr, &i, sizeof(i));
}

static inline float
ldbe_flt(const void *ptr)
{
	uint32_t i = ldbe_u32(ptr);
	float f;
	memcpy(&f, &i, sizeof(i));
	return f;
}

static inline void
stbe_flt(void *ptr, float f)
{
	uint32_t i;
	memcpy(&i, &f, sizeof(i));
	stbe_u32(ptr, i);
}

static inline float
ldle_flt(const void *ptr)
{
	uint32_t i = ldle_u32(ptr);
	float f;
	memcpy(&f, &i, sizeof(i));
	return f;
}

static inline void
stle_flt(void *ptr, float f)
{
	uint32_t i;
	memcpy(&i, &f, sizeof(i));
	stle_u32(ptr, i);
}

static inline float
ldn_flt(const void *ptr)
{
	uint32_t i = ldn_u32(ptr);
	float f;
	memcpy(&f, &i, sizeof(i));
	return f;
}

static inline void
stn_flt(void *ptr, float f)
{
	uint32_t i;
	memcpy(&i, &f, sizeof(i));
	stn_u32(ptr, i);
}

static inline double
ldbe_dbl(const void *ptr)
{
	uint64_t i = ldbe_u64(ptr);
	double d;
	memcpy(&d, &i, sizeof(i));
	return d;
}

static inline void
stbe_dbl(void *ptr, double d)
{
	uint64_t i;
	memcpy(&i, &d, sizeof(i));
	stbe_u64(ptr, i);
}

static inline double
ldle_dbl(const void *ptr)
{
	uint64_t i = ldle_u64(ptr);
	double d;
	memcpy(&d, &i, sizeof(i));
	return d;
}

static inline void
stle_dbl(void *ptr, double d)
{
	uint64_t i;
	memcpy(&i, &d, sizeof(i));
	stle_u64(ptr, i);
}

static inline double
ldn_dbl(const void *ptr)
{
	uint64_t i = ldn_u64(ptr);
	double d;
	memcpy(&d, &i, sizeof(i));
	return d;
}

static inline void
stn_dbl(void *ptr, double d)
{
	uint64_t i;
	memcpy(&i, &d, sizeof(i));
	stn_u64(ptr, i);
}

#ifdef __cplusplus
}
#endif

#endif

