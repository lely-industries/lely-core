/*!\file
 * This header file is part of the CANopen library; it contains the CANopen
 * value declarations.
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

#ifndef LELY_CO_VAL_H
#define LELY_CO_VAL_H

#include <lely/co/type.h>

#include <float.h>
#include <stddef.h>

//! The minimum value of a boolean truth value (false).
#define CO_BOOLEAN_MIN		0

//! The maximum value of a boolean truth value (true).
#define CO_BOOLEAN_MAX		1

//! The minimum value of an 8-bit signed integer.
#define CO_INTEGER8_MIN		INT8_MIN

//! The maximum value of an 8-bit signed integer.
#define CO_INTEGER8_MAX		INT8_MAX

//! The minimum value of a 16-bit signed integer.
#define CO_INTEGER16_MIN	INT16_MIN

//! The maximum value of a 16-bit signed integer.
#define CO_INTEGER16_MAX	INT16_MAX

//! The minimum value of a 32-bit signed integer.
#define CO_INTEGER32_MIN	INT32_MIN

//! The maximum value of a 32-bit signed integer.
#define CO_INTEGER32_MAX	INT32_MAX

//! The minimum value of an 8-bit unsigned integer.
#define CO_UNSIGNED8_MIN	0

//! The maximum value of an 8-bit unsigned integer.
#define CO_UNSIGNED8_MAX	UINT8_MAX

//! The minimum value of a 16-bit unsigned integer.
#define CO_UNSIGNED16_MIN	0

//! The maximum value of a 16-bit unsigned integer.
#define CO_UNSIGNED16_MAX	UINT16_MAX

//! The minimum value of a 32-bit unsigned integer.
#define CO_UNSIGNED32_MIN	0

//! The maximum value of a 32-bit unsigned integer.
#define CO_UNSIGNED32_MAX	UINT32_MAX

//! The minimum value of a 32-bit IEEE-754 floating-point number.
#define CO_REAL32_MIN		(-FLT_MAX)

//! The maximum value of a 32-bit IEEE-754 floating-point number.
#define CO_REAL32_MAX		FLT_MAX

//! The minimum value of a 48-bit structure representing the absolute time.
#define CO_TIME_OF_DAY_MIN	{ 0, 0 }

//! The maximum value of a 48-bit structure representing the absolute time.
#define CO_TIME_OF_DAY_MAX	{ UINT32_C(0x0fffffff), UINT16_MAX }

//! The minimum value of a 48-bit structure representing a time difference.
#define CO_TIME_DIFF_MIN	CO_TIME_OF_DAY_MIN

//! The maximum value of a 48-bit structure representing a time difference.
#define CO_TIME_DIFF_MAX	CO_TIME_OF_DAY_MAX

//! The minimum value of a 24-bit signed integer (encoded as an int32_t).
#define CO_INTEGER24_MIN	(-INT32_C(0x00800000))

//! The maximum value of a 24-bit signed integer (encoded as an int32_t).
#define CO_INTEGER24_MAX	INT32_C(0x007fffff)

//! The minimum value of a 64-bit IEEE-754 floating-point number.
#define CO_REAL64_MIN		(-DBL_MAX)

//! The maximum value of a 64-bit IEEE-754 floating-point number.
#define CO_REAL64_MAX		DBL_MAX

//! The minimum value of a 40-bit signed integer (encoded as an int64_t).
#define CO_INTEGER40_MIN	(-INT64_C(0x0000008000000000))

//! The maximum value of a 40-bit signed integer (encoded as an int64_t).
#define CO_INTEGER40_MAX	INT64_C(0x0000007fffffffff)

//! The minimum value of a 48-bit signed integer (encoded as an int64_t).
#define CO_INTEGER48_MIN	(-INT64_C(0x0000800000000000))

//! The maximum value of a 48-bit signed integer (encoded as an int64_t).
#define CO_INTEGER48_MAX	INT64_C(0x00007fffffffffff)

//! The minimum value of a 56-bit signed integer (encoded as an int64_t).
#define CO_INTEGER56_MIN	(-INT64_C(0x0080000000000000))

//! The maximum value of a 56-bit signed integer (encoded as an int64_t).
#define CO_INTEGER56_MAX	INT64_C(0x007fffffffffffff)

//! The minimum value of a 64-bit signed integer.
#define CO_INTEGER64_MIN	INT64_MIN

//! The maximum value of a 64-bit signed integer.
#define CO_INTEGER64_MAX	INT64_MAX

//! The minimum value of a 24-bit unsigned integer (encoded as a uint32_t).
#define CO_UNSIGNED24_MIN	0

//! The maximum value of a 24-bit unsigned integer (encoded as a uint32_t).
#define CO_UNSIGNED24_MAX	UINT32_C(0x00ffffff)

//! The minimum value of a 40-bit unsigned integer (encoded as a uint64_t).
#define CO_UNSIGNED40_MIN	0

//! The maximum value of a 40-bit unsigned integer (encoded as a uint64_t).
#define CO_UNSIGNED40_MAX	UINT64_C(0x000000ffffffffff)

//! The minimum value of a 48-bit unsigned integer (encoded as a uint64_t).
#define CO_UNSIGNED48_MIN	0

//! The maximum value of a 48-bit unsigned integer (encoded as a uint64_t).
#define CO_UNSIGNED48_MAX	UINT64_C(0x0000ffffffffffff)

//! The minimum value of a 56-bit unsigned integer (encoded as a uint64_t).
#define CO_UNSIGNED56_MIN	0

//! The maximum value of a 56-bit unsigned integer (encoded as a uint64_t).
#define CO_UNSIGNED56_MAX	UINT64_C(0x00ffffffffffffff)

//! The minimum value of a 64-bit unsigned integer.
#define CO_UNSIGNED64_MIN	0

//! The maximum value of a 64-bit unsigned integer.
#define CO_UNSIGNED64_MAX	UINT64_MAX

//! A union of the CANopen static data types.
union co_val {
#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
	co_##b##_t c;
#include <lely/co/def/type.def>
#undef LELY_CO_DEFINE_TYPE
};

#if __STDC_VERSION__ >= 199901L

//! Converts a visible string literal to a CANopen array.
#define CO_VISIBLE_STRING_C(c) \
	(((struct { \
		size_t size; \
		union { \
			union co_val val; \
			char vs[sizeof(c)]; \
		} u; \
	}){ \
		.size = sizeof(c) - 1, \
		.u = { .vs = c } \
	}).u.vs)

//! Converts an octet string literal to a CANopen array.
#define CO_OCTET_STRING_C(c) \
	(((struct { \
		size_t size; \
		union { \
			union co_val val; \
			uint8_t os[sizeof(c)]; \
		} u; \
	}){ \
		.size = sizeof(c) - 1, \
		.u = { .os = c } \
	}).u.os)

#if __STDC_VERSION__ >= 201112L

//! Converts a (16-bit) Unicode string literal to a CANopen array.
#define CO_UNICODE_STRING_C(c) \
	(((struct { \
		size_t size; \
		union { \
			union co_val val; \
			char16_t us[sizeof(c)]; \
		} u; \
	}){ \
		.size = sizeof(c) - 2, \
		.u = { .us = c } \
	}).u.us)

#endif // __STDC_VERSION__ >= 201112L

#define _CO_ARRAY(...)	__VA_ARGS__

//! Converts an array literal with elements of type \a type to a CANopen array.
#define CO_DOMAIN_C(type, ...) \
	_CO_DOMAIN_C(type, _CO_ARRAY(__VA_ARGS__))

#define _CO_DOMAIN_C(type, c) \
	(((struct { \
		size_t size; \
		union { \
			union co_val val; \
			type dom[sizeof((type[])c) / sizeof(type)]; \
		} u; \
	}){ \
		.size = sizeof((type[])c), \
		.u = { .dom = c } \
	}).u.dom)

#endif // __STDC_VERSION__ >= 199901L

// The file location struct from <lely/util/diag.h>.
struct floc;

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Initializes a value of the specified data type to zero.
 *
 * \param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * \param val  the address of the value to be initialized. In the case of
 *             strings or domains, this MUST be the address of pointer.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_val_fini()
 */
LELY_CO_EXTERN int co_val_init(co_unsigned16_t type, void *val);

/*!
 * Initializes a value of the specified data type with its lower limit.
 *
 * \param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * \param val  the address of the value to be initialized. In the case of
 *             strings or domains, this MUST be the address of pointer, which
 *             will be set to NULL.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_val_fini()
 */
LELY_CO_EXTERN int co_val_init_min(co_unsigned16_t type, void *val);

/*!
 * Initializes a value of the specified data type with its upper limit.
 *
 * \param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * \param val  the address of the value to be initialized. In the case of
 *             strings or domains, this MUST be the address of pointer, which
 *             will be set to NULL.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_val_fini()
 */
LELY_CO_EXTERN int co_val_init_max(co_unsigned16_t type, void *val);

/*!
 * Initializes an array of visible characters (#CO_DEFTYPE_VISIBLE_STRING).
 *
 * \param val the address of a pointer. On success, *\a val points to the first
 *            character in the string.
 * \param vs  a pointer to the (null-terminated) string with which *\a val
 *            should be initialized (can be NULL).
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_val_fini()
 */
LELY_CO_EXTERN int co_val_init_vs(char **val, const char *vs);

/*!
 * Initializes an array of octets (#CO_DEFTYPE_OCTET_STRING).
 *
 * \param val the address of a pointer. On success, *\a val points to the first
 *            octet in the string.
 * \param os  a pointer to the array of octets with which *\a val should be
 *            initialized (can be NULL).
 * \param n   the number of octets at \a os.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_val_fini()
 */
LELY_CO_EXTERN int co_val_init_os(uint8_t **val, const uint8_t *os, size_t n);

/*!
 * Initializes an array of (16-bit) Unicode characters
 * (#CO_DEFTYPE_UNICODE_STRING).
 *
 * \param val the address of a pointer. On success, *\a val points to the first
 *            character in the string.
 * \param us  a pointer to the (null-terminated) string with which *\a val
 *            should be initialized (can be NULL).
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_val_fini()
 */
LELY_CO_EXTERN int co_val_init_us(char16_t **val, const char16_t *us);

/*!
 * Initializes an arbitrary large block of data (#CO_DEFTYPE_DOMAIN).
 *
 * \param val the address of a pointer. On success, *\a val points to the first
 *            byte.
 * \param dom a pointer to the bytes with which *\a val should be initialized
 *            (can be NULL).
 * \param n   the number of bytes at \a dom.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_val_fini()
 */
LELY_CO_EXTERN int co_val_init_dom(void **val, const void *dom, size_t n);

/*!
 * Finalizes a value of the specified data type.
 *
 * \param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * \param val  the address of the value to be finalized (can be NULL). In the
 *             case of strings or domains, this MUST be the address of pointer.
 *
 * \see co_val_init()
 */
LELY_CO_EXTERN void co_val_fini(co_unsigned16_t type, void *val);

/*!
 * Returns the address of the first byte in a value of the specified data type.
 * In the case of strings or domains, this is the address of the first byte in
 * the array.
 *
 * \param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * \param val  the address of the value (can be NULL). In the case of strings or
 *             domains, this MUST be the address of pointer.
 *
 * \see co_val_sizeof()
 */
LELY_CO_EXTERN const void *co_val_addressof(co_unsigned16_t type,
		const void *val);

/*!
 * Returns the size (in bytes) of a value of the specified data type. In the
 * case of strings or domains, this is the number of bytes in the array.
 *
 * \param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * \param val  the address of the value (can be NULL). In the case of strings or
 *             domains, this MUST be the address of pointer.
 *
 * \see co_val_addressof()
 */
LELY_CO_EXTERN size_t co_val_sizeof(co_unsigned16_t type, const void *val);

/*!
 * Constructs a value of the specified data type.
 *
 * \param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * \param val  the address of the value to be constructed. In the case of
 *             strings or domains, this MUST be the address of pointer. Note
 *             that this value is _not_ finalized before the copy is performed.
 * \param ptr  a pointer to the bytes to be copied. In case of strings or
 *             domains, \a ptr MUST point to the first byte in the array.
 * \param n    the number of bytes at \a ptr. In case of strings, \a n SHOULD
 *             exclude the terminating null byte(s).
 *
 * \returns the number of bytes copied (i.e., \a n), or 0 on error. In the
 * latter case, the error number can be obtained with `get_errnum()`.
 */
LELY_CO_EXTERN size_t co_val_make(co_unsigned16_t type, void *val,
		const void *ptr, size_t n);

/*!
 * Copies one value to another. In case of strings or domains, this function
 * performs a deep copy (i.e, it copies the array).
 *
 * \param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * \param dst  a pointer to the destination value. In the case of strings or
 *             domains, this MUST be the address of pointer. Note that this
 *             value is _not_ finalized before the copy is performed.
 * \param src  a pointer to the source value. In the case of strings or domains,
 *             this MUST be the address of pointer.
 *
 * \returns the number of bytes copied (i.e., the result of
 * `co_val_sizeof(type, src)`), or 0 on error. In the latter case, the error
 * number can be obtained with `get_errnum()`.
 *
 * \see co_val_move()
 */
LELY_CO_EXTERN size_t co_val_copy(co_unsigned16_t type, void *dst,
		const void *src);

/*!
 * Moves one value to another. In case of strings or domains, this function
 * performs a shallow copy (i.e., it copies the pointer to the array).
 *
 * \param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * \param dst  a pointer to the destination value. In the case of strings or
 *             domains, this MUST be the address of pointer. Note that this
 *             value is _not_ finalized before the move is performed.
 * \param src  a pointer to the source value. In the case of strings or domains,
 *             this MUST be the address of pointer (which is set to NULL on
 *             exit).
 *
 * \returns the number of bytes copied (i.e., the result of
 * `co_type_sizeof(type)`), or 0 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_val_copy()
 */
LELY_CO_EXTERN size_t co_val_move(co_unsigned16_t type, void *dst, void *src);

/*!
 * Compares two values of the specified data type.
 *
 * \param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * \param v1   a pointer to the first value. In case of string or domains, this
 *             MUST be the address of pointer.
 * \param v2   a pointer to the second value. In case of string or domains, this
 *             MUST be the address of pointer.
 *
 * \returns an integer greater than, equal to, or less than 0 if *\a v1 is
 * greater than, equal to, or less than *\a v2.
 */
LELY_CO_EXTERN int co_val_cmp(co_unsigned16_t type, const void *v1,
		const void *v2);

/*!
 * Lexes a value of the specified data type from a memory buffer.
 *
 * \param type  the data type (in the range [1..27]). This MUST be the object
 *              index of one of the static data types.
 * \param val   the address at which to store the value. On success, if \a val
 *              is not NULL, *\a val contains the lexed value. On error, *\a val
 *              val is left untouched. In the case of strings or domains, \a val
 *              MUST be the address of pointer. Note that this value is _not_
 *              finalized before the parsed value is stored.
 * \param begin a pointer to the start of the buffer.
 * \param end   a pointer to the end of the buffer (can be NULL if the buffer is
 *              null-terminated).
 * \param at    an optional pointer to the file location of \a begin (used for
 *              diagnostic purposes). On success, if `at != NULL`, *\a at points
 *              to one past the last character lexed. On error, *\a at is left
 *              untouched.
 *
 * \returns the number of characters read.
 */
LELY_CO_EXTERN size_t co_val_lex(co_unsigned16_t type, void *val,
		const char *begin, const char *end, struct floc *at);

#ifdef __cplusplus
}
#endif

#endif

