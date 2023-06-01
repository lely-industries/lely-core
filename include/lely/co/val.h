/**@file
 * This header file is part of the CANopen library; it contains the CANopen
 * value declarations.
 *
 * @copyright 2016-2023 Lely Industries N.V.
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

#ifndef LELY_CO_VAL_H_
#define LELY_CO_VAL_H_

#include <lely/co/type.h>
#include <lely/util/util.h>

#include <float.h>
#include <stddef.h>

#if !LELY_NO_CO_DCF || !LELY_NO_CO_OBJ_FILE
// The read file buffer from <lely/util/frbuf.h>
struct __frbuf;
// The write file buffer from <lely/util/fwbuf.h>
struct __fwbuf;
#endif

/// The default value of a boolean truth value (false).
#define CO_BOOLEAN_INIT 0

/// The minimum value of a boolean truth value (false).
#define CO_BOOLEAN_MIN 0

/// The maximum value of a boolean truth value (true).
#define CO_BOOLEAN_MAX 1

/// The default value of an 8-bit signed integer.
#define CO_INTEGER8_INIT 0

/// The minimum value of an 8-bit signed integer.
#define CO_INTEGER8_MIN (-INT8_C(0x7f) - 1)

/// The maximum value of an 8-bit signed integer.
#define CO_INTEGER8_MAX INT8_C(0x7f)

/// The default value of a 16-bit signed integer.
#define CO_INTEGER16_INIT 0

/// The minimum value of a 16-bit signed integer.
#define CO_INTEGER16_MIN (-INT16_C(0x7fff) - 1)

/// The maximum value of a 16-bit signed integer.
#define CO_INTEGER16_MAX INT16_C(0x7fff)

/// The default value of a 32-bit signed integer.
#define CO_INTEGER32_INIT 0

/// The minimum value of a 32-bit signed integer.
#define CO_INTEGER32_MIN (-INT32_C(0x7fffffff) - 1)

/// The maximum value of a 32-bit signed integer.
#define CO_INTEGER32_MAX INT32_C(0x7fffffff)

/// The default value of an 8-bit unsigned integer.
#define CO_UNSIGNED8_INIT 0

/// The minimum value of an 8-bit unsigned integer.
#define CO_UNSIGNED8_MIN 0

/// The maximum value of an 8-bit unsigned integer.
#define CO_UNSIGNED8_MAX UINT8_C(0xff)

/// The default value of a 16-bit unsigned integer.
#define CO_UNSIGNED16_INIT 0

/// The minimum value of a 16-bit unsigned integer.
#define CO_UNSIGNED16_MIN 0

/// The maximum value of a 16-bit unsigned integer.
#define CO_UNSIGNED16_MAX UINT16_C(0xffff)

/// The default value of a 32-bit unsigned integer.
#define CO_UNSIGNED32_INIT 0

/// The minimum value of a 32-bit unsigned integer.
#define CO_UNSIGNED32_MIN 0

/// The maximum value of a 32-bit unsigned integer.
#define CO_UNSIGNED32_MAX UINT32_C(0xffffffff)

/// The default value of a 32-bit IEEE-754 floating-point number.
#define CO_REAL32_INIT 0

/// The minimum value of a 32-bit IEEE-754 floating-point number.
#define CO_REAL32_MIN (-FLT_MAX)

/// The maximum value of a 32-bit IEEE-754 floating-point number.
#define CO_REAL32_MAX FLT_MAX

/// The default value of an array of visible characters.
#define CO_VISIBLE_STRING_INIT NULL

// The "minimum value" of an array of visible characters.
#define CO_VISIBLE_STRING_MIN CO_VISIBLE_STRING_INIT

// The "maximum value" of an array of visible characters.
#define CO_VISIBLE_STRING_MAX CO_VISIBLE_STRING_INIT

/// The default value of an array of octets.
#define CO_OCTET_STRING_INIT NULL

// The "minimum value" of an array of octets.
#define CO_OCTET_STRING_MIN CO_OCTET_STRING_INIT

// The "maximum value" of an array of octets.
#define CO_OCTET_STRING_MAX CO_OCTET_STRING_INIT

/// The default value of an array of (16-bit) Unicode characters.
#define CO_UNICODE_STRING_INIT NULL

// The "minimum value" of an array of (16-bit) Unicode characters.
#define CO_UNICODE_STRING_MIN CO_UNICODE_STRING_INIT

// The "maximum value" of an array of (16-bit) Unicode characters.
#define CO_UNICODE_STRING_MAX CO_UNICODE_STRING_INIT

/// The default value of a 48-bit structure representing the absolute time.
#define CO_TIME_OF_DAY_INIT \
	{ \
		0, 0 \
	}

/// The minimum value of a 48-bit structure representing the absolute time.
#define CO_TIME_OF_DAY_MIN \
	{ \
		0, 0 \
	}

/// The maximum value of a 48-bit structure representing the absolute time.
#define CO_TIME_OF_DAY_MAX \
	{ \
		UINT32_C(0x0fffffff), UINT16_C(0xffff) \
	}

/// The default value of a 48-bit structure representing a time difference.
#define CO_TIME_DIFF_INIT CO_TIME_OF_DAY_INIT

/// The minimum value of a 48-bit structure representing a time difference.
#define CO_TIME_DIFF_MIN CO_TIME_OF_DAY_MIN

/// The maximum value of a 48-bit structure representing a time difference.
#define CO_TIME_DIFF_MAX CO_TIME_OF_DAY_MAX

/// The default value of an arbitrary large block of data..
#define CO_DOMAIN_INIT NULL

// The "minimum value" of an arbitrary large block of data..
#define CO_DOMAIN_MIN CO_DOMAIN_INIT

// The "maximum value" of an arbitrary large block of data..
#define CO_DOMAIN_MAX CO_DOMAIN_INIT

/// The default value of a 24-bit signed integer (encoded as an int32_t).
#define CO_INTEGER24_INIT 0

/// The minimum value of a 24-bit signed integer (encoded as an int32_t).
#define CO_INTEGER24_MIN (-INT32_C(0x007fffff) - 1)

/// The maximum value of a 24-bit signed integer (encoded as an int32_t).
#define CO_INTEGER24_MAX INT32_C(0x007fffff)

/// The default value of a 64-bit IEEE-754 floating-point number.
#define CO_REAL64_INIT 0

/// The minimum value of a 64-bit IEEE-754 floating-point number.
#define CO_REAL64_MIN (-DBL_MAX)

/// The maximum value of a 64-bit IEEE-754 floating-point number.
#define CO_REAL64_MAX DBL_MAX

/// The default value of a 40-bit signed integer (encoded as an int64_t).
#define CO_INTEGER40_INIT 0

/// The minimum value of a 40-bit signed integer (encoded as an int64_t).
#define CO_INTEGER40_MIN (-INT64_C(0x0000007fffffffff) - 1)

/// The maximum value of a 40-bit signed integer (encoded as an int64_t).
#define CO_INTEGER40_MAX INT64_C(0x0000007fffffffff)

/// The default value of a 48-bit signed integer (encoded as an int64_t).
#define CO_INTEGER48_INIT 0

/// The minimum value of a 48-bit signed integer (encoded as an int64_t).
#define CO_INTEGER48_MIN (-INT64_C(0x00007fffffffffff) - 1)

/// The maximum value of a 48-bit signed integer (encoded as an int64_t).
#define CO_INTEGER48_MAX INT64_C(0x00007fffffffffff)

/// The default value of a 56-bit signed integer (encoded as an int64_t).
#define CO_INTEGER56_INIT 0

/// The minimum value of a 56-bit signed integer (encoded as an int64_t).
#define CO_INTEGER56_MIN (-INT64_C(0x007fffffffffffff) - 1)

/// The maximum value of a 56-bit signed integer (encoded as an int64_t).
#define CO_INTEGER56_MAX INT64_C(0x007fffffffffffff)

/// The default value of a 64-bit signed integer.
#define CO_INTEGER64_INIT 0

/// The minimum value of a 64-bit signed integer.
#define CO_INTEGER64_MIN (-INT64_C(0x7fffffffffffffff) - 1)

/// The maximum value of a 64-bit signed integer.
#define CO_INTEGER64_MAX INT64_C(0x7fffffffffffffff)

/// The default value of a 24-bit unsigned integer (encoded as a uint32_t).
#define CO_UNSIGNED24_INIT 0

/// The minimum value of a 24-bit unsigned integer (encoded as a uint32_t).
#define CO_UNSIGNED24_MIN 0

/// The maximum value of a 24-bit unsigned integer (encoded as a uint32_t).
#define CO_UNSIGNED24_MAX UINT32_C(0x00ffffff)

/// The default value of a 40-bit unsigned integer (encoded as a uint64_t).
#define CO_UNSIGNED40_INIT 0

/// The minimum value of a 40-bit unsigned integer (encoded as a uint64_t).
#define CO_UNSIGNED40_MIN 0

/// The maximum value of a 40-bit unsigned integer (encoded as a uint64_t).
#define CO_UNSIGNED40_MAX UINT64_C(0x000000ffffffffff)

/// The default value of a 48-bit unsigned integer (encoded as a uint64_t).
#define CO_UNSIGNED48_INIT 0

/// The minimum value of a 48-bit unsigned integer (encoded as a uint64_t).
#define CO_UNSIGNED48_MIN 0

/// The maximum value of a 48-bit unsigned integer (encoded as a uint64_t).
#define CO_UNSIGNED48_MAX UINT64_C(0x0000ffffffffffff)

/// The default value of a 56-bit unsigned integer (encoded as a uint64_t).
#define CO_UNSIGNED56_INIT 0

/// The minimum value of a 56-bit unsigned integer (encoded as a uint64_t).
#define CO_UNSIGNED56_MIN 0

/// The maximum value of a 56-bit unsigned integer (encoded as a uint64_t).
#define CO_UNSIGNED56_MAX UINT64_C(0x00ffffffffffffff)

/// The default value of a 64-bit unsigned integer.
#define CO_UNSIGNED64_INIT 0

/// The minimum value of a 64-bit unsigned integer.
#define CO_UNSIGNED64_MIN 0

/// The maximum value of a 64-bit unsigned integer.
#define CO_UNSIGNED64_MAX UINT64_C(0xffffffffffffffff)

/// A union of the CANopen static data types.
union co_val {
#define LELY_CO_DEFINE_TYPE(a, b, c, d) co_##b##_t c;
#include <lely/co/def/type.def>
#undef LELY_CO_DEFINE_TYPE
};

/// The header directly preceding the bytes in a CANopen array.
struct co_array_hdr {
	/// The total capacity (in bytes).
	size_t capacity;
	/// The current size (in bytes).
	size_t size;
};

#ifndef CO_ARRAY_CAPACITY
/// The default capacity (in bytes) of a statically allocated CANopen array.
#if LELY_NO_MALLOC
#define CO_ARRAY_CAPACITY 256
#else
#define CO_ARRAY_CAPACITY 0
#endif
#endif

#if LELY_NO_MALLOC

/// A CANopen array.
struct co_array {
	/// The header containing the capacity and current size.
	struct co_array_hdr hdr;
	union {
		/// The bytes in the array.
		char data[CO_ARRAY_CAPACITY];
		// Ensure the correct alignment.
		union co_val _val;
	} u;
};

/// The static initializer for #co_array.
#define CO_ARRAY_INIT \
	{ \
		{ CO_ARRAY_CAPACITY, 0 }, \
		{ \
			{ \
				0 \
			} \
		} \
	}

#endif // LELY_NO_MALLOC

#if __STDC_VERSION__ >= 199901L

#if LELY_NO_MALLOC
/**
 * An empty, statically allocated CANopen array literal with the default
 * capacity.
 */
#define CO_ARRAY_C ((void *)((struct co_array)CO_ARRAY_INIT).u.data)
#endif

#define _CO_ARRAY(...) __VA_ARGS__

/**
 * Converts a visible string literal to a CANopen array with a capacity of at
 * least <b>n</b> bytes.
 */
// clang-format off
#define CO_VISIBLE_STRING_NC(n, c) \
	(((struct { \
		struct co_array_hdr hdr; \
		union { \
			char vs[MAX(n, sizeof(c))]; \
			union co_val val; \
		} u; \
	}){ \
		.hdr = { \
			.capacity = MAX(n, sizeof(c)), \
			.size = sizeof(c) - 1 \
		}, \
		.u = { .vs = c } \
	}).u.vs)
// clang-format on

/// Converts a visible string literal to a CANopen array.
#define CO_VISIBLE_STRING_C(c) CO_VISIBLE_STRING_NC(CO_ARRAY_CAPACITY, c)

/**
 * Converts an octet string literal to a CANopen array with a capacity of at
 * least <b>n</b> bytes.
 */
// clang-format off
#define CO_OCTET_STRING_NC(n, c) \
	(((struct { \
		struct co_array_hdr hdr; \
		union { \
			uint_least8_t os[MAX(n, sizeof(c))]; \
			union co_val val; \
		} u; \
	}){ \
		.hdr = { \
			.capacity = MAX(n, sizeof(c)), \
			.size = sizeof(c) - 1 \
		}, \
		.u = { .os = c } \
	}).u.os)
// clang-format on

/// Converts an octet string literal to a CANopen array.
#define CO_OCTET_STRING_C(c) CO_OCTET_STRING_NC(CO_ARRAY_CAPACITY, c)

/**
 * Converts a (16-bit) Unicode string literal to a CANopen array with a capacity
 * of at least <b>n</b> bytes
 */
#define CO_UNICODE_STRING_NC(n, ...) \
	_CO_UNICODE_STRING_NC(n, _CO_ARRAY(__VA_ARGS__))

// clang-format off
#define _CO_UNICODE_STRING_NC(n, c) \
	(((struct { \
		struct co_array_hdr hdr; \
		union { \
			char16_t us[MAX(ALIGN(n, sizeof(char16_t)), \
							sizeof((char16_t[])c)) \
					/ sizeof(char16_t)]; \
			union co_val val; \
		} u; \
	}){ \
		.hdr = { \
			.capacity = MAX(ALIGN(n, sizeof(char16_t)), \
					sizeof((char16_t[])c)), \
			.size = sizeof((char16_t[])c) - sizeof(char16_t) \
		}, \
		.u = { .us = c } \
	}).u.us)
// clang-format on

/// Converts a (16-bit) Unicode string literal to a CANopen array.
#define CO_UNICODE_STRING_C(...) \
	_CO_UNICODE_STRING_NC(CO_ARRAY_CAPACITY, _CO_ARRAY(__VA_ARGS__))

/**
 * Converts an array literal with elements of type <b>type</b> to a CANopen
 * array with a capacity of at least <b>n</b> bytes.
 */
#define CO_DOMAIN_NC(type, n, ...) \
	_CO_DOMAIN_NC(type, n, _CO_ARRAY(__VA_ARGS__))

// clang-format off
#define _CO_DOMAIN_NC(type, n, c) \
	(((struct { \
		struct co_array_hdr hdr; \
		union { \
			type dom[MAX(ALIGN(n, sizeof(type)), sizeof((type[])c)) \
					/ sizeof(type)]; \
			union co_val val; \
		} u; \
	}){ \
		.hdr = { \
			.capacity = MAX(ALIGN(n, sizeof(type)), \
					sizeof((type[])c)), \
			.size = sizeof((type[])c) \
		}, \
		.u = { .dom = c } \
	}).u.dom)
// clang-format on

/**
 * Converts an array literal with elements of type <b>type</b> to a CANopen
 * array.
 */
#define CO_DOMAIN_C(type, ...) \
	_CO_DOMAIN_NC(type, CO_ARRAY_CAPACITY, _CO_ARRAY(__VA_ARGS__))

#endif // __STDC_VERSION__ >= 199901L

// The file location struct from <lely/util/diag.h>.
struct floc;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes a value of the specified data type to zero.
 *
 * @param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * @param val  the address of the value to be initialized. In the case of
 *             strings or domains, this MUST be the address of pointer.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_val_fini()
 */
int co_val_init(co_unsigned16_t type, void *val);

/**
 * Initializes a value of the specified data type with its lower limit.
 *
 * @param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * @param val  the address of the value to be initialized. In the case of
 *             strings or domains, this MUST be the address of pointer, which
 *             will be set to NULL.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_val_fini()
 */
int co_val_init_min(co_unsigned16_t type, void *val);

/**
 * Initializes a value of the specified data type with its upper limit.
 *
 * @param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * @param val  the address of the value to be initialized. In the case of
 *             strings or domains, this MUST be the address of pointer, which
 *             will be set to NULL.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_val_fini()
 */
int co_val_init_max(co_unsigned16_t type, void *val);

/**
 * Initializes an array of visible characters (#CO_DEFTYPE_VISIBLE_STRING).
 *
 * @param val the address of a pointer. On success, *<b>val</b> points to the
 *            first character in the string.
 * @param vs  a pointer to the (null-terminated) string with which *<b>val</b>
 *            should be initialized (can be NULL).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_val_fini()
 */
int co_val_init_vs(char **val, const char *vs);

/**
 * Initializes an array of visible characters (#CO_DEFTYPE_VISIBLE_STRING).
 *
 * @param val the address of a pointer. On success, *<b>val</b> points to the
 *            first character in the string.
 * @param vs  a pointer to the string with which *<b>val</b> should be
 *            initialized (can be NULL).
 * @param n   the number of characters in the value to be created (excluding the
 *            terminating null byte) and the maximum number of characters to
 *            copy from <b>vs</b> (unless <b>vs</b> is NULL).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_val_fini()
 */
int co_val_init_vs_n(char **val, const char *vs, size_t n);

/**
 * Initializes an array of octets (#CO_DEFTYPE_OCTET_STRING).
 *
 * @param val the address of a pointer. On success, *<b>val</b> points to the
 *            first octet in the string.
 * @param os  a pointer to the array of octets with which *<b>val</b> should be
 *            initialized (can be NULL).
 * @param n   the number of octets in the value to be created (and the number of
 *            octets at <b>os</b> unless <b>os</b> is NULL).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_val_fini()
 */
int co_val_init_os(uint_least8_t **val, const uint_least8_t *os, size_t n);

/**
 * Initializes an array of (16-bit) Unicode characters
 * (#CO_DEFTYPE_UNICODE_STRING).
 *
 * @param val the address of a pointer. On success, *<b>val</b> points to the
 *            first character in the string.
 * @param us  a pointer to the (null-terminated) string with which *<b>val</b>
 *            should be initialized (can be NULL).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_val_fini()
 */
int co_val_init_us(char16_t **val, const char16_t *us);

/**
 * Initializes an array of (16-bit) Unicode characters
 * (#CO_DEFTYPE_UNICODE_STRING).
 *
 * @param val the address of a pointer. On success, *<b>val</b> points to the
 *            first character in the string.
 * @param us  a pointer to the string with which *<b>val</b> should be
 *            initialized (can be NULL).
 * @param n   the number of (16-bit) Unicode characters in the value to be
 *            created (excluding the terminating null bytes) and the maximum
 *            number of characters to copy from <b>us</b> (unless <b>us</b> is
 *            NULL).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_val_fini()
 */
int co_val_init_us_n(char16_t **val, const char16_t *us, size_t n);

/**
 * Initializes an arbitrary large block of data (#CO_DEFTYPE_DOMAIN).
 *
 * @param val the address of a pointer. On success, *<b>val</b> points to the
 *            first byte.
 * @param dom a pointer to the bytes with which *<b>val</b> should be
 *            initialized (can be NULL).
 * @param n   the number of bytes in the value to be created (and the number of
 *            bytes at <b>dom</b> unless <b>dom</b> is NULL).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_val_fini()
 */
int co_val_init_dom(void **val, const void *dom, size_t n);

#if LELY_NO_MALLOC
/// Initializes a value to point to the specified CANopen array.
static inline void co_val_init_array(void *val, struct co_array *array);
#endif

/**
 * Finalizes a value of the specified data type. It is safe to invoke this
 * function multiple times on the same value.
 *
 * @param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * @param val  the address of the value to be finalized (can be NULL). In the
 *             case of strings or domains, this MUST be the address of pointer.
 *
 * @see co_val_init()
 */
void co_val_fini(co_unsigned16_t type, void *val);

/**
 * Returns the address of the first byte in a value of the specified data type.
 * In the case of strings or domains, this is the address of the first byte in
 * the array.
 *
 * @param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * @param val  the address of the value (can be NULL). In the case of strings or
 *             domains, this MUST be the address of pointer.
 *
 * @see co_val_sizeof()
 */
const void *co_val_addressof(co_unsigned16_t type, const void *val);

/**
 * Returns the size (in bytes) of a value of the specified data type. In the
 * case of strings or domains, this is the number of bytes in the array.
 *
 * @param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * @param val  the address of the value (can be NULL). In the case of strings or
 *             domains, this MUST be the address of pointer.
 *
 * @see co_val_addressof()
 */
size_t co_val_sizeof(co_unsigned16_t type, const void *val);

/**
 * Constructs a value of the specified data type.
 *
 * @param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * @param val  the address of the value to be constructed. In the case of
 *             strings or domains, this MUST be the address of pointer. Note
 *             that this value is _not_ finalized before the copy is performed.
 * @param ptr  a pointer to the bytes to be copied. In case of strings or
 *             domains, <b>ptr</b> MUST point to the first byte in the array.
 * @param n    the number of bytes at <b>ptr</b>. In case of strings, <b>n</b>
 *             SHOULD exclude the terminating null byte(s).
 *
 * @returns the number of bytes copied (i.e., <b>n</b>), or 0 on error. In the
 * latter case, the error number can be obtained with get_errc().
 */
size_t co_val_make(co_unsigned16_t type, void *val, const void *ptr, size_t n);

/**
 * Copies one value to another. In case of strings or domains, this function
 * performs a deep copy (i.e, it copies the array).
 *
 * @param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * @param dst  a pointer to the destination value. In the case of strings or
 *             domains, this MUST be the address of pointer. Note that this
 *             value is _not_ finalized before the copy is performed.
 * @param src  a pointer to the source value. In the case of strings or domains,
 *             this MUST be the address of pointer.
 *
 * @returns the number of bytes copied (i.e., the result of
 * `co_val_sizeof(type, src)`), or 0 on error. In the latter case, the error
 * number can be obtained with get_errc().
 *
 * @see co_val_move()
 */
size_t co_val_copy(co_unsigned16_t type, void *dst, const void *src);

/**
 * Moves one value to another. In case of strings or domains, this function
 * performs a shallow copy (i.e., it copies the pointer to the array).
 *
 * @param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * @param dst  a pointer to the destination value. In the case of strings or
 *             domains, this MUST be the address of pointer. Note that this
 *             value is _not_ finalized before the move is performed.
 * @param src  a pointer to the source value. In the case of strings or domains,
 *             this MUST be the address of pointer (which is set to NULL on
 *             exit).
 *
 * @returns the number of bytes copied (i.e., the result of
 * `co_type_sizeof(type)`), or 0 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_val_copy()
 */
size_t co_val_move(co_unsigned16_t type, void *dst, void *src);

/**
 * Compares two values of the specified data type.
 *
 * @param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * @param v1   a pointer to the first value. In case of string or domains, this
 *             MUST be the address of pointer.
 * @param v2   a pointer to the second value. In case of string or domains, this
 *             MUST be the address of pointer.
 *
 * @returns an integer greater than, equal to, or less than 0 if *<b>v1</b> is
 * greater than, equal to, or less than *<b>v2</b>.
 */
int co_val_cmp(co_unsigned16_t type, const void *v1, const void *v2);

/**
 * Reads a value of the specified data type from a memory buffer.
 *
 * @param type  the data type (in the range [1..27]). This MUST be the object
 *              index of one of the static data types.
 * @param val   the address at which to store the value. On success, if
 *              <b>val</b> is not NULL, *<b>val</b> contains the read value. On
 *              error, *<b>val</b> is left untouched. In the case of strings or
 *              domains, <b>val</b> MUST be the address of a pointer. Note that
 *              this value is _not_ finalized before the read value is stored.
 * @param begin a pointer to the start of the buffer.
 * @param end   a pointer to one past the last byte in the buffer. At most
 *              `end - begin` bytes are read. For strings and domains, all
 *              bytes are considered to be part of the value.
 *
 * @returns the number of bytes read, or 0 on error. In the latter case, the
 * error number can be obtained with get_errc().
 *
 * @see co_val_write()
 */
size_t co_val_read(co_unsigned16_t type, void *val, const uint_least8_t *begin,
		const uint_least8_t *end);

#if !LELY_NO_STDIO

/**
 * Reads a value of the specified data type from a file.
 *
 * @param type     the data type (in the range [1..27]). This MUST be the object
 *                 index of one of the static data types.
 * @param val      the address at which to store the value. On success, if
 *                 <b>val</b> is not NULL, *<b>val</b> contains the read value.
 *                 In the case of strings or domains, <b>val</b> MUST be the
 *                 address of a pointer. Note that this value is _not_ finalized
 *                 before the read value is stored.
 * @param filename a pointer to the name of the file.
 *
 * @returns the number of bytes read, or 0 on error. In the latter case, the
 * error number can be obtained with get_errc().
 *
 * @see co_val_read_frbuf()
 */
size_t co_val_read_file(co_unsigned16_t type, void *val, const char *filename);

/**
 * Reads a value of the specified data type from the current position in a read
 * file buffer.
 *
 * @param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * @param val  the address at which to store the value. On success, if
 *             <b>val</b> is not NULL, *<b>val</b> contains the read value. In
 *             the case of strings or domains, <b>val</b> MUST be the address of
 *             a pointer. Note that this value is _not_ finalized before the
 *             read value is stored.
 * @param buf  a pointer to a read file buffer.
 *
 * @returns the number of bytes read, or 0 on error. In the latter case, the
 * error number can be obtained with get_errc().
 *
 * @see co_val_read_file(), frbuf_read()
 */
size_t co_val_read_frbuf(co_unsigned16_t type, void *val, struct __frbuf *buf);

#endif // !LELY_NO_STDIO

/**
 * Reads a value of the specified data type from an SDO buffer.
 *
 * @param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * @param val  the address at which to store the value. On success, if
 *             <b>val</b> is not NULL, *<b>val</b> contains the read value. On
 *             error, *<b>val</b> is left untouched. In the case of strings or
 *             domains, <b>val</b> MUST be the address of pointer. Note that
 *             this value is _not_ finalized before the read value is stored.
 * @param ptr  a pointer to the bytes in the SDO.
 * @param n    the number of bytes at <b>ptr</b>.
 *
 * @returns 0 on success, or an SDO abort code on error.
 *
 * @see co_val_read()
 */
co_unsigned32_t co_val_read_sdo(
		co_unsigned16_t type, void *val, const void *ptr, size_t n);

/**
 * Writes a value of the specified data type to a memory buffer.
 *
 * @param type  the data type (in the range [1..27]). This MUST be the object
 *              index of one of the static data types.
 * @param val   the address of the value to be written. In case of strings or
 *              domains, this MUST be the address of pointer.
 * @param begin a pointer to the start of the buffer. If <b>begin</b> is NULL,
 *              nothing is written.
 * @param end   a pointer to one past the last byte in the buffer. If
 *              <b>end</b> is not NULL, and the buffer is too small
 *              (i.e., `end - begin` is less than the return value), nothing is
 *              written.
 *
 * @returns the number of bytes that would have been written had the buffer been
 * sufficiently large, or 0 on error. In the latter case, the error number can
 * be obtained with get_errc().
 *
 * @see co_val_read()
 */
size_t co_val_write(co_unsigned16_t type, const void *val, uint_least8_t *begin,
		uint_least8_t *end);

#if !LELY_NO_STDIO

/**
 * Writes a value of the specified data type to a file.
 *
 * @param type     the data type (in the range [1..27]). This MUST be the object
 *                 index of one of the static data types.
 * @param val      the address of the value to be written. In case of strings or
 *                 domains, this MUST be the address of pointer.
 * @param filename a pointer to the name of the file.
 *
 * @returns the number of bytes written, or 0 on error. In the latter case, the
 * error number can be obtained with get_errc().
 *
 * @see co_val_write_fwbuf()
 */
size_t co_val_write_file(
		co_unsigned16_t type, const void *val, const char *filename);

/**
 * Writes a value of the specified data type to the current position in a write
 * file buffer.
 *
 * @param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * @param val  the address of the value to be written. In case of strings or
 *             domains, this MUST be the address of pointer.
 * @param buf  a pointer to a write file buffer.
 *
 * @returns the number of bytes written, or 0 on error. In the latter case, the
 * error number can be obtained with get_errc().
 *
 * @see co_val_write_file(), fwbuf_write()
 */
size_t co_val_write_fwbuf(
		co_unsigned16_t type, const void *val, struct __fwbuf *buf);

/**
 * Lexes a value of the specified data type from a memory buffer.
 *
 * @param type  the data type (in the range [1..27]). This MUST be the object
 *              index of one of the static data types.
 * @param val   the address at which to store the value. On success, if
 *              <b>val</b> is not NULL, *<b>val</b> contains the lexed value. On
 *              error, *<b>val</b> is left untouched. In the case of strings or
 *              domains, <b>val</b> MUST be the address of pointer. Note that
 *              this value is _not_ finalized before the parsed value is stored.
 * @param begin a pointer to the start of the buffer.
 * @param end   a pointer to one past the last character in the buffer (can be
 *              NULL if the buffer is null-terminated).
 * @param at    an optional pointer to the file location of <b>begin</b> (used
 *              for diagnostic purposes). On success, if `at != NULL`,
 *              *<b>at</b> points to one past the last character lexed. On
 *              error, *<b>at</b> is left untouched.
 *
 * @returns the number of characters read.
 */
size_t co_val_lex(co_unsigned16_t type, void *val, const char *begin,
		const char *end, struct floc *at);

/**
 * Prints a value of the specified data type to a memory buffer.
 *
 * @param type   the data type (in the range [1..27]). This MUST be the object
 *               index of one of the static data types.
 * @param val    the address of the value to be written. In case of string or
 *               domains, this MUST be the address of pointer.
 * @param pbegin the address of a pointer to the start of the buffer. If
 *               <b>pbegin</b> or *<b>pbegin</b> is NULL, nothing is written;
 *               Otherwise, on exit, *<b>pbegin</b> points to one past the last
 *               character written.
 * @param end    a pointer to one past the last character in the buffer. If
 *               <b>end</b> is not NULL, at most `end - *pbegin` characters are
 *               written, and the output may be truncated.
 *
 * @returns the number of characters that would have been written had the buffer
 * been sufficiently large.
 */
size_t co_val_print(co_unsigned16_t type, const void *val, char **pbegin,
		char *end);

#endif // !LELY_NO_STDIO

#if LELY_NO_MALLOC
static inline void
co_val_init_array(void *val, struct co_array *array)
{
	if (val)
		*(char **)val = array ? array->u.data : NULL;
}
#endif

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_VAL_H_
