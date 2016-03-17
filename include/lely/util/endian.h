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

#include <string.h>

enum {
	//! Big endian byte order.
	ORDER_BIG_ENDIAN = UINT32_C(0x00010203),
	//! Little endian byte order.
	ORDER_LITTLE_ENDIAN = UINT32_C(0x03020100)
};

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Returns the host byte order for integers (either #ORDER_BIG_ENDIAN or
 * #ORDER_LITTLE_ENDIAN).
 */
static inline int int_byte_order(void);

//! Returns 1 if the host byte order for integers is big endian, 0 otherwise.
static inline int int_is_big_endian(void);

//! Returns 1 if the host byte order for integers is little endian, 0 otherwise.
static inline int int_is_little_endian(void);

/*!
 * Returns 1 if the host byte order for integers is equal to the network byte
 * order (big endian), 0 otherwise.
 */
static inline int int_is_network(void);

/*!
 * Returns the host byte order for floating-point numbers (either
 * #ORDER_BIG_ENDIAN or #ORDER_LITTLE_ENDIAN).
 */
static inline int float_byte_order(void);

/*!
 * Returns 1 if the host byte order for floating-point numbers is big endian, 0
 * otherwise.
 */
static inline int float_is_big_endian(void);

/*!
 * Returns 1 if the host byte order for floating-point numbers is little endian,
 * 0 otherwise.
 */
static inline int float_is_little_endian(void);

/*!
 * Returns 1 if the host byte order for floats is equal to the network byte order
 * (big endian), 0 otherwise.
 */
static inline int float_is_network(void);

//! Reverses the byte order of a 16-bit signed int.
static inline int16_t bswap_i16(int16_t i);

//! Reverses the byte order of a 16-bit unsigned int.
static inline uint16_t bswap_u16(uint16_t i);

//! Reverses the byte order of a 32-bit signed int.
static inline int32_t bswap_i32(int32_t i);

//! Reverses the byte order of a 32-bit unsigned int.
static inline uint32_t bswap_u32(uint32_t i);

//! Reverses the byte order of a 64-bit signed int.
static inline int64_t bswap_i64(int64_t i);

//! Reverses the byte order of a 64-bit unsigned int.
static inline uint64_t bswap_u64(uint64_t i);

//! Reverses the byte order of a single-precision floating-point number.
static inline float bswap_flt(float f);

//! Reverses the byte order of a double-precision floating-point number.
static inline double bswap_dbl(double d);

//! Converts a 16-bit signed int from host to big-endian byte order.
static inline int16_t htobe_i16(int16_t i);

//! Converts a 16-bit signed int from big-endian to host byte order.
static inline int16_t betoh_i16(int16_t i);

//! Converts a 16-bit unsigned int from host to big-endian byte order.
static inline uint16_t htobe_u16(uint16_t i);

//! Converts a 16-bit unsigned int from big-endian to host byte order.
static inline uint16_t betoh_u16(uint16_t i);

//! Converts a 16-bit signed int from host to little-endian byte order.
static inline int16_t htole_i16(int16_t i);

//! Converts a 16-bit signed int from little-endian to host byte order.
static inline int16_t letoh_i16(int16_t i);

//! Converts a 16-bit unsigned int from host to little-endian byte order.
static inline uint16_t htole_u16(uint16_t i);

//! Converts a 16-bit unsigned int from little-endian to host byte order.
static inline uint16_t letoh_u16(uint16_t i);

//! Converts a 16-bit signed int from host to network byte order.
static inline int16_t hton_i16(int16_t i);

//! Converts a 16-bit signed int from network to host byte order.
static inline int16_t ntoh_i16(int16_t i);

//! Converts a 16-bit unsigned int from host to network byte order.
static inline uint16_t hton_u16(uint16_t i);

//! Converts a 16-bit unsigned int from network to host byte order.
static inline uint16_t ntoh_u16(uint16_t i);

//! Converts a 32-bit signed int from host to big-endian byte order.
static inline int32_t htobe_i32(int32_t i);

//! Converts a 32-bit signed int from big-endian to host byte order.
static inline int32_t betoh_i32(int32_t i);

//! Converts a 32-bit unsigned int from host to big-endian byte order.
static inline uint32_t htobe_u32(uint32_t i);

//! Converts a 32-bit unsigned int from big-endian to host byte order.
static inline uint32_t betoh_u32(uint32_t i);

//! Converts a 32-bit signed int from host to little-endian byte order.
static inline int32_t htole_i32(int32_t i);

//! Converts a 32-bit signed int from little-endian to host byte order.
static inline int32_t letoh_i32(int32_t i);

//! Converts a 32-bit unsigned int from host to little-endian byte order.
static inline uint32_t htole_u32(uint32_t i);

//! Converts a 32-bit unsigned int from little-endian to host byte order.
static inline uint32_t letoh_u32(uint32_t i);

//! Converts a 32-bit signed int from host to network byte order.
static inline int32_t hton_i32(int32_t i);

//! Converts a 32-bit signed int from network to host byte order.
static inline int32_t ntoh_i32(int32_t i);

//! Converts a 32-bit unsigned int from host to network byte order.
static inline uint32_t hton_u32(uint32_t i);

//! Converts a 32-bit unsigned int from network to host byte order.
static inline uint32_t ntoh_u32(uint32_t i);

//! Converts a 64-bit signed int from host to big-endian byte order.
static inline int64_t htobe_i64(int64_t i);

//! Converts a 64-bit signed int from big-endian to host byte order.
static inline int64_t betoh_i64(int64_t i);

//! Converts a 64-bit unsigned int from host to big-endian byte order.
static inline uint64_t htobe_u64(uint64_t i);

//! Converts a 64-bit unsigned int from big-endian to host byte order.
static inline uint64_t betoh_u64(uint64_t i);

//! Converts a 64-bit signed int from host to little-endian byte order.
static inline int64_t htole_i64(int64_t i);

//! Converts a 64-bit signed int from little-endian to host byte order.
static inline int64_t letoh_i64(int64_t i);

//! Converts a 64-bit unsigned int from host to little-endian byte order.
static inline uint64_t htole_u64(uint64_t i);

//! Converts a 64-bit unsigned int from little-endian to host byte order.
static inline uint64_t letoh_u64(uint64_t i);

//! Converts a 64-bit signed int from host to network byte order.
static inline int64_t hton_i64(int64_t i);

//! Converts a 64-bit signed int from network to host byte order.
static inline int64_t ntoh_i64(int64_t i);

//! Converts a 64-bit unsigned int from host to network byte order.
static inline uint64_t hton_u64(uint64_t i);

//! Converts a 64-bit unsigned int from network to host byte order.
static inline uint64_t ntoh_u64(uint64_t i);

/*!
 * Converts a single-precision floating-point number from host to big-endian
 * byte order.
 */
static inline float htobe_flt(float f);

/*!
 * Converts a single-precision floating-point number from big-endian to host
 * byte order.
 */
static inline float betoh_flt(float f);

/*!
 * Converts a single-precision floating-point number from host to little-endian
 * byte order.
 */
static inline float htole_flt(float f);

/*!
 * Converts a single-precision floating-point number from little-endian to host
 * byte order.
 */
static inline float letoh_flt(float f);

/*!
 * Converts a single-precision floating-point number from host to network byte
 * order.
 */
static inline float hton_flt(float f);

/*!
 * Converts a single-precision floating-point number from network to host byte
 * order.
 */
static inline float ntoh_flt(float f);

/*!
 * Converts a double-precision floating-point number from host to big-endian
 * byte order.
 */
static inline double htobe_dbl(double d);

/*!
 * Converts a double-precision floating-point number from big-endian to host
 * byte order.
 */
static inline double betoh_dbl(double d);

/*!
 * Converts a double-precision floating-point number from host to little-endian
 * byte order.
 */
static inline double htole_dbl(double d);

/*!
 * Converts a double-precision floating-point number from little-endian to host
 * byte order.
 */
static inline double letoh_dbl(double d);

/*!
 * Converts a double-precision floating-point number from host to network byte
 * order.
 */
static inline double hton_dbl(double d);

/*!
 * Converts a double-precision floating-point number from network to host byte
 * order.
 */
static inline double ntoh_dbl(double d);

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

static inline int
int_byte_order(void)
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	return ORDER_BIG_ENDIAN;
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return ORDER_LITTLE_ENDIAN;
#else
	const union {
		uint8_t u8[4];
		uint32_t u32;
	} u = { { 0x00, 0x01, 0x02, 0x03 } };
	return u.u32;
#endif
}

static inline int
int_is_big_endian(void)
{
	return int_byte_order() == ORDER_BIG_ENDIAN;
}

static inline int
int_is_little_endian(void)
{
	return int_byte_order() == ORDER_LITTLE_ENDIAN;
}

static inline int
int_is_network(void)
{
	return int_is_big_endian();
}

static inline int
float_byte_order(void)
{
#if defined(__FLOAT_WORD_ORDER__) \
		&& __FLOAT_WORD_ORDER__ == __ORDER_BIG_ENDIAN__
	return ORDER_BIG_ENDIAN;
#elif defined(__FLOAT_WORD_ORDER__) \
		&& __FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return ORDER_LITTLE_ENDIAN;
#else
	return int_byte_order();
#endif
}

static inline int
float_is_big_endian(void)
{
	return float_byte_order() == ORDER_BIG_ENDIAN;
}

static inline int
float_is_little_endian(void)
{
	return float_byte_order() == ORDER_LITTLE_ENDIAN;
}

static inline int
float_is_network(void)
{
	return float_is_big_endian();
}

static inline int16_t bswap_i16(int16_t i) { return bswap_u16(i); }

static inline uint16_t
bswap_u16(uint16_t i)
{
#if __GNUC_PREREQ(4, 8) || __has_builtin(__builtin_bswap16)
	return __builtin_bswap16(i);
#else
	return ((i & 0xff) << 8) | ((i >> 8) & 0xff);
#endif
}

static inline int32_t bswap_i32(int32_t i) { return bswap_u32(i); }

static inline uint32_t
bswap_u32(uint32_t i)
{
#if __GNUC_PREREQ(4, 3) || __has_builtin(__builtin_bswap32)
	return __builtin_bswap32(i);
#else
	return ((i & 0xff) << 24) | ((i & 0xff00) << 8) | ((i >> 8) & 0xff00)
			| ((i >> 24) & 0xff);
#endif
}

static inline int64_t bswap_i64(int64_t i) { return bswap_u64(i); }

static inline uint64_t
bswap_u64(uint64_t i)
{
#if __GNUC_PREREQ(4, 3) || __has_builtin(__builtin_bswap64)
	return __builtin_bswap64(i);
#else
	return ((i & 0xff) << 56) | ((i & 0xff00) << 40)
			| ((i & 0xff0000) << 24) | ((i & 0xff000000) << 8)
			| ((i >> 8) & 0xff000000) | ((i >> 24) & 0xff0000)
			| ((i >> 40) & 0xff00) | ((i >> 56) & 0xff);
#endif
}

static inline float
bswap_flt(float f)
{
	union {
		float f;
		uint32_t i;
	} u = { f };
	u.i = bswap_u32(u.i);
	return u.f;
}

static inline double
bswap_dbl(double d)
{
	union {
		double d;
		uint64_t i;
	} u = { d };
	u.i = bswap_u64(u.i);
	return u.d;
}

static inline int16_t htobe_i16(int16_t i) { return htobe_u16(i); }

static inline int16_t betoh_i16(int16_t i) { return betoh_u16(i); }

static inline uint16_t
htobe_u16(uint16_t i)
{
	return int_is_big_endian() ? i : bswap_u16(i);
}

static inline uint16_t betoh_u16(uint16_t i) { return htobe_u16(i); }

static inline int16_t htole_i16(int16_t i) { return htole_u16(i); }

static inline int16_t letoh_i16(int16_t i) { return letoh_u16(i); }

static inline uint16_t
htole_u16(uint16_t i)
{
	return int_is_little_endian() ? i : bswap_u16(i);
}

static inline uint16_t letoh_u16(uint16_t i) { return htole_u16(i); }

static inline int16_t hton_i16(int16_t i) { return hton_u16(i); }

static inline int16_t ntoh_i16(int16_t i) { return ntoh_u16(i); }

static inline uint16_t
hton_u16(uint16_t i)
{
	return int_is_network() ? i : bswap_u16(i);
}

static inline uint16_t ntoh_u16(uint16_t i) { return hton_u16(i); }

static inline int32_t htobe_i32(int32_t i) { return htobe_u32(i); }

static inline int32_t betoh_i32(int32_t i) { return betoh_u32(i); }

static inline uint32_t
htobe_u32(uint32_t i)
{
	return int_is_big_endian() ? i : bswap_u32(i);
}

static inline uint32_t betoh_u32(uint32_t i) { return htobe_u32(i); }

static inline int32_t htole_i32(int32_t i) { return htole_u32(i); }

static inline int32_t letoh_i32(int32_t i) { return letoh_u32(i); }

static inline uint32_t
htole_u32(uint32_t i)
{
	return int_is_little_endian() ? i : bswap_u32(i);
}

static inline uint32_t letoh_u32(uint32_t i) { return htole_u32(i); }

static inline int32_t hton_i32(int32_t i) { return hton_u32(i); }

static inline int32_t ntoh_i32(int32_t i) { return ntoh_u32(i); }

static inline uint32_t
hton_u32(uint32_t i)
{
	return int_is_network() ? i : bswap_u32(i);
}

static inline uint32_t ntoh_u32(uint32_t i) { return hton_u32(i); }

static inline int64_t htobe_i64(int64_t i) { return htobe_u64(i); }

static inline int64_t betoh_i64(int64_t i) { return betoh_u64(i); }

static inline uint64_t
htobe_u64(uint64_t i)
{
	return int_is_big_endian() ? i : bswap_u64(i);
}

static inline uint64_t betoh_u64(uint64_t i) { return htobe_u64(i); }

static inline int64_t htole_i64(int64_t i) { return htole_u64(i); }

static inline int64_t letoh_i64(int64_t i) { return letoh_u64(i); }

static inline uint64_t
htole_u64(uint64_t i)
{
	return int_is_little_endian() ? i : bswap_u64(i);
}

static inline uint64_t letoh_u64(uint64_t i) { return htole_u64(i); }

static inline int64_t hton_i64(int64_t i) { return hton_u64(i); }

static inline int64_t ntoh_i64(int64_t i) { return ntoh_u64(i); }

static inline uint64_t
hton_u64(uint64_t i)
{
	return int_is_network() ? i : bswap_u64(i);
}

static inline uint64_t ntoh_u64(uint64_t i) { return hton_u64(i); }

static inline float
htobe_flt(float f)
{
	return float_is_big_endian() ? f : bswap_flt(f);
}

static inline float betoh_flt(float f) { return htobe_flt(f); }

static inline float
htole_flt(float f)
{
	return float_is_little_endian() ? f : bswap_flt(f);
}

static inline float letoh_flt(float f) { return htole_flt(f); }

static inline float
hton_flt(float f)
{
	return float_is_network() ? f : bswap_flt(f);
}

static inline float ntoh_flt(float f) { return hton_flt(f); }

static inline double
htobe_dbl(double d)
{
	return float_is_big_endian() ? d : bswap_dbl(d);
}

static inline double betoh_dbl(double d) { return htobe_dbl(d); }

static inline double
htole_dbl(double d)
{
	return float_is_little_endian() ? d : bswap_dbl(d);
}

static inline double letoh_dbl(double d) { return htole_dbl(d); }

static inline double
hton_dbl(double d)
{
	return float_is_network() ? d : bswap_dbl(d);
}

static inline double ntoh_dbl(double d) { return hton_dbl(d); }

static inline int16_t
ldbe_i16(const void *ptr)
{
	return ldbe_u16(ptr);
}

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

static inline int16_t
ldle_i16(const void *ptr)
{
	return ldle_u16(ptr);
}

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

static inline int32_t
ldbe_i32(const void *ptr)
{
	return ldbe_u32(ptr);
}

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

static inline int32_t
ldle_i32(const void *ptr)
{
	return ldle_u32(ptr);
}

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

static inline int64_t
ldbe_i64(const void *ptr)
{
	return ldbe_u64(ptr);
}

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

static inline int64_t
ldle_i64(const void *ptr)
{
	return ldle_u64(ptr);
}

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
	float f;
	memcpy(&f, ptr, sizeof(f));
	return betoh_flt(f);
}

static inline void
stbe_flt(void *ptr, float f)
{
	f = htobe_flt(f);
	memcpy(ptr, &f, sizeof(f));
}

static inline float
ldle_flt(const void *ptr)
{
	float f;
	memcpy(&f, ptr, sizeof(f));
	return letoh_flt(f);
}

static inline void
stle_flt(void *ptr, float f)
{
	f = htole_flt(f);
	memcpy(ptr, &f, sizeof(f));
}

static inline float
ldn_flt(const void *ptr)
{
	float f;
	memcpy(&f, ptr, sizeof(f));
	return ntoh_flt(f);
}

static inline void
stn_flt(void *ptr, float f)
{
	f = hton_flt(f);
	memcpy(ptr, &f, sizeof(f));
}

static inline double
ldbe_dbl(const void *ptr)
{
	double d;
	memcpy(&d, ptr, sizeof(d));
	return betoh_dbl(d);
}

static inline void
stbe_dbl(void *ptr, double d)
{
	d = htobe_dbl(d);
	memcpy(ptr, &d, sizeof(d));
}

static inline double
ldle_dbl(const void *ptr)
{
	double d;
	memcpy(&d, ptr, sizeof(d));
	return letoh_dbl(d);
}

static inline void
stle_dbl(void *ptr, double d)
{
	d = htole_dbl(d);
	memcpy(ptr, &d, sizeof(d));
}

static inline double
ldn_dbl(const void *ptr)
{
	double d;
	memcpy(&d, ptr, sizeof(d));
	return ntoh_dbl(d);
}

static inline void
stn_dbl(void *ptr, double d)
{
	d = hton_dbl(d);
	memcpy(ptr, &d, sizeof(d));
}

#ifdef __cplusplus
}
#endif

#endif

