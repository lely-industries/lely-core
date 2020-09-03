/**@file
 * This header file is part of the CANopen library; it contains the CANopen type
 * definitions.
 *
 * @copyright 2016-2020 Lely Industries N.V.
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

#ifndef LELY_CO_TYPE_H_
#define LELY_CO_TYPE_H_

#include <lely/co/co.h>
#include <lely/libc/uchar.h>
#include <lely/util/float.h>

#include <stdint.h>

/// The data type (and object index) of a boolean truth value.
#define CO_DEFTYPE_BOOLEAN 0x0001

/// The data type (and object index) of an 8-bit signed integer.
#define CO_DEFTYPE_INTEGER8 0x0002

/// The data type (and object index) of a 16-bit signed integer.
#define CO_DEFTYPE_INTEGER16 0x0003

/// The data type (and object index) of a 32-bit signed integer.
#define CO_DEFTYPE_INTEGER32 0x0004

/// The data type (and object index) of an 8-bit unsigned integer.
#define CO_DEFTYPE_UNSIGNED8 0x0005

/// The data type (and object index) of a 16-bit unsigned integer.
#define CO_DEFTYPE_UNSIGNED16 0x0006

/// The data type (and object index) of a 32-bit unsigned integer.
#define CO_DEFTYPE_UNSIGNED32 0x0007

/// The data type (and object index) of a 32-bit IEEE-754 floating-point number.
#define CO_DEFTYPE_REAL32 0x0008

/// The data type (and object index) of an array of visible characters.
#define CO_DEFTYPE_VISIBLE_STRING 0x0009

/// The data type (and object index) of an array of octets.
#define CO_DEFTYPE_OCTET_STRING 0x000a

/// The data type (and object index) of an array of (16-bit) Unicode characters.
#define CO_DEFTYPE_UNICODE_STRING 0x000b

/**
 * The data type (and object index) of a 48-bit structure representing the
 * absolute time.
 */
#define CO_DEFTYPE_TIME_OF_DAY 0x000c

/**
 * The data type (and object index) of a 48-bit structure representing a time
 * difference.
 */
#define CO_DEFTYPE_TIME_DIFF 0x000d

/// The data type (and object index) of an arbitrary large block of data.
#define CO_DEFTYPE_DOMAIN 0x000f

/// The data type (and object index) of a 24-bit signed integer.
#define CO_DEFTYPE_INTEGER24 0x0010

/// The data type (and object index) of a 64-bit IEEE-754 floating-point number.
#define CO_DEFTYPE_REAL64 0x0011

/// The data type (and object index) of a 40-bit signed integer.
#define CO_DEFTYPE_INTEGER40 0x0012

/// The data type (and object index) of a 48-bit signed integer.
#define CO_DEFTYPE_INTEGER48 0x0013

/// The data type (and object index) of a 56-bit signed integer.
#define CO_DEFTYPE_INTEGER56 0x0014

/// The data type (and object index) of a 64-bit signed integer.
#define CO_DEFTYPE_INTEGER64 0x0015

/// The data type (and object index) of a 24-bit unsigned integer.
#define CO_DEFTYPE_UNSIGNED24 0x0016

/// The data type (and object index) of a 40-bit unsigned integer.
#define CO_DEFTYPE_UNSIGNED40 0x0018

/// The data type (and object index) of a 48-bit unsigned integer.
#define CO_DEFTYPE_UNSIGNED48 0x0019

/// The data type (and object index) of a 56-bit unsigned integer.
#define CO_DEFTYPE_UNSIGNED56 0x001a

/// The data type (and object index) of a 64-bit unsigned integer.
#define CO_DEFTYPE_UNSIGNED64 0x001b

#define LELY_CO_DEFINE_TYPE(a, b, c, d) typedef d co_##b##_t;
#include <lely/co/def/type.def>
#undef LELY_CO_DEFINE_TYPE

/// A 48-bit struct used to describe absolute and relative times.
struct __co_time_of_day {
	/// Milliseconds after midnight.
	co_unsigned32_t ms;
	/// The number of days since January 1, 1984.
	co_unsigned16_t days;
};

/// A 48-bit struct used to describe absolute and relative times.
struct __co_time_diff {
	/// Milliseconds after midnight.
	co_unsigned32_t ms;
	/// The number of days since January 1, 1984.
	co_unsigned16_t days;
};

#ifdef __cplusplus
extern "C" {
#endif

/// Returns 1 if the specified (static) data type is a basic type, and 0 if not.
int co_type_is_basic(co_unsigned16_t type);

/// Returns 1 if the specified (static) data type is an array, and 0 if not.
int co_type_is_array(co_unsigned16_t type);

/**
 * Returns the native size (in bytes) of a value of the specified data type, or
 * 1 if it is not a static data type. In case of strings or domains, this
 * function returns the size of a pointer.
 *
 * @see co_type_alignof()
 */
size_t co_type_sizeof(co_unsigned16_t type);

/**
 * Returns the alignment requirements (in bytes) of a value of the specified
 * data type, or 1 if it is not a static data type. In case of strings or
 * domains, this function returns the alignment requirements of a pointer.
 *
 * @see co_type_sizeof()
 */
size_t co_type_alignof(co_unsigned16_t type);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_TYPE_H_
