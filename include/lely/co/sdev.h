/**@file
 * This header file is part of the CANopen library; it contains the static
 * device description declarations.
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

#ifndef LELY_CO_SDEV_H_
#define LELY_CO_SDEV_H_

#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/val.h>

/// A static CANopen device. @see co_dev_create_from_sdev()
struct co_sdev {
	/// The node-ID.
	co_unsigned8_t id;
	/// A pointer to the name of the device.
	const char *name;
	/// A pointer to the vendor name.
	const char *vendor_name;
	/// The vendor ID.
	co_unsigned32_t vendor_id;
	/// A pointer to the product name.
	const char *product_name;
	/// The product code.
	co_unsigned32_t product_code;
	/// The revision number.
	co_unsigned32_t revision;
	/// A pointer to the order code.
	const char *order_code;
	/// The supported bit rates.
	unsigned baud : 10;
	/// The (pending) baudrate (in kbit/s).
	co_unsigned16_t rate;
	/// A flag specifying whether LSS is supported (1) or not (0).
	int lss;
	/// The data types supported for mapping dummy entries in PDOs.
	co_unsigned32_t dummy;
	/// The number of objects in #objs.
	co_unsigned16_t nobj;
	/// An array of objects.
	const struct co_sobj *objs;
};

/// A static CANopen object. @see #co_sdev
struct co_sobj {
#if !LELY_NO_CO_OBJ_NAME
	/// A pointer to the name of the object.
	const char *name;
#endif
	/// The object index.
	co_unsigned16_t idx;
	/// The object code.
	co_unsigned8_t code;
	/// The number of sub-objects in #subs.
	co_unsigned8_t nsub;
	/// An array of sub-objects.
	const struct co_ssub *subs;
};

/// A static CANopen sub-object. @see #co_sobj
struct co_ssub {
#if !LELY_NO_CO_OBJ_NAME
	/// A pointer to the name of the sub-object.
	const char *name;
#endif
	/// The object sub-index.
	co_unsigned8_t subidx;
	/// The data type.
	co_unsigned16_t type;
#if !LELY_NO_CO_OBJ_LIMITS
	/// The lower limit of #val.
	union co_val min;
	/// The upper limit of #val.
	union co_val max;
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
	/// The default value of #val.
	union co_val def;
#endif
	/// The sub-object value.
	union co_val val;
	/// The access type.
	uint_least32_t access : 5;
	/// A flag indicating if it is possible to map this object into a PDO.
	uint_least32_t pdo_mapping : 1;
	/// The object flags.
	uint_least32_t flags : 26;
};

#ifdef __cplusplus
extern "C" {
#endif

struct __co_dev *__co_dev_init_from_sdev(
		struct __co_dev *dev, const struct co_sdev *sdev);

/**
 * Creates a CANopen device from a static device description.
 *
 * @returns a pointer to a new device, or NULL on error. In the latter case, the
 * error number can be obtained with get_errc().
 */
co_dev_t *co_dev_create_from_sdev(const struct co_sdev *sdev);

/**
 * Prints a C99 static initializer code fragment for a static device description
 * (struct #co_sdev) to a string buffer.
 *
 * @param s   the address of the output buffer. If <b>s</b> is not NULL, at most
 *            `n - 1` characters are written, plus a terminating null byte.
 * @param n   the size (in bytes) of the buffer at <b>s</b>. If <b>n</b> is
 *            zero, nothing is written.
 * @param dev a pointer to a CANopen device description to be printed.
 *
 * @returns the number of characters that would have been written had the
 * buffer been sufficiently large, not counting the terminating null byte, or a
 * negative number on error. In the latter case, the error number is stored in
 * `errno`.
 */
int snprintf_c99_sdev(char *s, size_t n, const co_dev_t *dev);

/**
 * Equivalent to snprintf_c99_sdev(), except that it allocates a string large
 * enough to hold the output, including the terminating null byte.
 *
 * @param ps  the address of a value which, on success, contains a pointer to
 *            the allocated string. This pointer SHOULD be passed to `free()` to
 *            release the allocated storage.
 * @param dev a pointer to a CANopen device description to be printed.
 *
 * @returns the number of characters written, not counting the terminating null
 * byte, or a negative number on error. In the latter case, the error number is
 * stored in `errno`.
 */
int asprintf_c99_sdev(char **ps, const co_dev_t *dev);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_SDEV_H_
