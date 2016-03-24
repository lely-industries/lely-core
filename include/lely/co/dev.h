/*!\file
 * This header file is part of the CANopen library; it contains the device
 * description declarations.
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

#ifndef LELY_CO_DEV_H
#define LELY_CO_DEV_H

#include <lely/co/type.h>

#include <stddef.h>

//! The maximum number of nodes in a CANopen network.
#define CO_NUM_NODES	127

//! A bit rate of 10 kbit/s.
#define CO_BAUD_10	0x01

//! A bit rate of 20 kbit/s.
#define CO_BAUD_20	0x02

//! A bit rate of 50 kbit/s.
#define CO_BAUD_50	0x04

//! A bit rate of 125 kbit/s.
#define CO_BAUD_125	0x08

//! A bit rate of 250 kbit/s.
#define CO_BAUD_250	0x10

//! A bit rate of 500 kbit/s.
#define CO_BAUD_500	0x20

//! A bit rate of 800 kbit/s.
#define CO_BAUD_800	0x40

//! A bit rate of 1 Mbit/s.
#define CO_BAUD_1000	0x80

#ifdef __cplusplus
extern "C" {
#endif

LELY_CO_EXTERN void *__co_dev_alloc(void);
LELY_CO_EXTERN void __co_dev_free(void *ptr);
LELY_CO_EXTERN struct __co_dev *__co_dev_init(struct __co_dev *dev,
		co_unsigned8_t id);
LELY_CO_EXTERN void __co_dev_fini(struct __co_dev *dev);

/*!
 * Creates a new CANopen device.
 *
 * \param id the Node-ID of the device (in the range [1..127, 255]).
 *
 * \returns a pointer to a new CANopen device, or NULL on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 *
 * \see co_dev_destroy()
 */
LELY_CO_EXTERN co_dev_t *co_dev_create(co_unsigned8_t id);

/*!
 * Destroys a CANopen device, including all objects in its object dictionary.
 *
 * \see co_dev_create()
 */
LELY_CO_EXTERN void co_dev_destroy(co_dev_t *dev);

//! Returns the Node-ID of a CANopen device. \see co_dev_set_id()
LELY_CO_EXTERN co_unsigned8_t co_dev_get_id(const co_dev_t *dev);

/*!
 * Sets the Node-ID of a CANopen device. This function will also update any
 * sub-object values of the form `$NODEID { "+" number }`.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_dev_get_id()
 */
LELY_CO_EXTERN int co_dev_set_id(co_dev_t *dev, co_unsigned8_t id);

/*!
 * Retrieves a list of object indices in the object dictionary of a CANopen
 * device.
 *
 * \param dev    a pointer to a CANopen device.
 * \param maxidx the maximum number of object indices to return.
 * \param idx    an array of at least \a maxidx indices (can be NULL). On
 *               success, *\a idx contains the object indices.
 *
 * \returns the total number of object indices in the object dictionary (which
 * may be different from \a maxidx).
 */
LELY_CO_EXTERN co_unsigned16_t co_dev_get_idx(const co_dev_t *dev,
		co_unsigned16_t maxidx, co_unsigned16_t *idx);

/*!
 * Inserts an object into the object dictionary of a CANopen device. This
 * function fails if the object is already part of the object dictionary of
 * another device, or if another object with the same index already exists.
 *
 * \param dev a pointer to a CANopen device.
 * \param obj a pointer to the object to be inserted.
 *
 * \returns 0 on success, or -1 on error.
 *
 * \see co_dev_remove_obj(), co_dev_find_obj()
 */
LELY_CO_EXTERN int co_dev_insert_obj(co_dev_t *dev, co_obj_t *obj);

/*!
 * Removes an object from the object dictionary a CANopen device.
 *
 * \param dev a pointer to a CANopen device.
 * \param obj a pointer to the object to be removed.
 *
 * \returns 0 on success, or -1 on error.
 *
 * \see co_dev_insert_obj()
 */
LELY_CO_EXTERN int co_dev_remove_obj(co_dev_t *dev, co_obj_t *obj);

/*!
 * Finds an object in the object dictionary of a CANopen device.
 *
 * \param dev a pointer to a CANopen device.
 * \param idx the object index.
 *
 * \returns a pointer to the object if found, or NULL if not.
 *
 * \see co_dev_insert_obj()
 */
LELY_CO_EXTERN co_obj_t *co_dev_find_obj(const co_dev_t *dev,
		co_unsigned16_t idx);

/*!
 * Finds a sub-object in the object dictionary of a CANopen device.
 *
 * \param dev    a pointer to a CANopen device.
 * \param idx    the object index.
 * \param subidx the object sub-index.
 *
 * \returns a pointer to the sub-object if found, or NULL if not.
 */
LELY_CO_EXTERN co_sub_t *co_dev_find_sub(const co_dev_t *dev,
		co_unsigned16_t idx, co_unsigned8_t subidx);

/*!
 * Returns a pointer to the vendor name of a CANopen device.
 *
 * \see co_dev_set_vendor_name()
 */
LELY_CO_EXTERN const char *co_dev_get_vendor_name(const co_dev_t *dev);

/*!
 * Sets the vendor name of a CANopen device.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_dev_get_vendor_name()
 */
LELY_CO_EXTERN int co_dev_set_vendor_name(co_dev_t *dev,
		const char *vendor_name);

//! Returns the vendor ID of a CANopen device. \see co_dev_set_vendor_id()
LELY_CO_EXTERN co_unsigned32_t co_dev_get_vendor_id(const co_dev_t *dev);

//! Sets the vendor ID of a CANopen device. \see co_dev_get_vendor_id()
LELY_CO_EXTERN void co_dev_set_vendor_id(co_dev_t *dev,
		co_unsigned32_t vendor_id);

/*!
 * Returns a pointer to the product name of a CANopen device.
 *
 * \see co_dev_set_product_name()
 */
LELY_CO_EXTERN const char *co_dev_get_product_name(const co_dev_t *dev);

/*!
 * Sets the product name of a CANopen device.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_dev_get_product_name()
 */
LELY_CO_EXTERN int co_dev_set_product_name(co_dev_t *dev,
		const char *product_name);

//! Returns the product code of a CANopen device. \see co_dev_set_product_code()
LELY_CO_EXTERN co_unsigned32_t co_dev_get_product_code(const co_dev_t *dev);

//! Sets the product code of a CANopen device. \see co_dev_get_product_code()
LELY_CO_EXTERN void co_dev_set_product_code(co_dev_t *dev,
		co_unsigned32_t product_code);

//! Returns the revision number of a CANopen device. \see co_dev_set_revision()
LELY_CO_EXTERN co_unsigned32_t co_dev_get_revision(const co_dev_t *dev);

//! Sets the revision number of a CANopen device. \see co_dev_get_revision()
LELY_CO_EXTERN void co_dev_set_revision(co_dev_t *dev,
		co_unsigned32_t revision);

/*!
 * Returns a pointer to the order code of a CANopen device.
 *
 * \see co_dev_set_order_code()
 */
LELY_CO_EXTERN const char *co_dev_get_order_code(const co_dev_t *dev);

/*!
 * Sets the order code of a CANopen device.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_dev_get_order_code()
 */
LELY_CO_EXTERN int co_dev_set_order_code(co_dev_t *dev, const char *order_code);

/*!
 * Returns the supported bit rates of a CANopen device (any combination of
 * #CO_BAUD_10, #CO_BAUD_20, #CO_BAUD_50, #CO_BAUD_125, #CO_BAUD_250,
 * #CO_BAUD_500, #CO_BAUD_800 and #CO_BAUD_1000).
 *
 * \see co_dev_set_baud()
 */
LELY_CO_EXTERN unsigned int co_dev_get_baud(const co_dev_t *dev);

/*!
 * Sets the supported bit rates of a CANopen device.
 *
 * \param dev  a pointer to a CANopen device.
 * \param baud the supported bit rates (any combination of #CO_BAUD_10,
 *             #CO_BAUD_20, #CO_BAUD_50, #CO_BAUD_125, #CO_BAUD_250,
 *             #CO_BAUD_500, #CO_BAUD_800 and #CO_BAUD_1000).
 *
 * \see co_dev_get_baud()
 */
LELY_CO_EXTERN void co_dev_set_baud(co_dev_t *dev, unsigned int baud);

/*!
 * Returns the data types supported by a CANopen device for mapping dummy
 * entries in PDOs (one bit for each of the basic types).
 *
 * \see co_dev_set_dummy()
 */
LELY_CO_EXTERN co_unsigned32_t co_dev_get_dummy(const co_dev_t *dev);

/*!
 * Sets the data types supported by a CANopen device for mapping dummy entries
 * in PDOs.
 *
 * \param dev   a pointer to a CANopen device.
 * \param dummy the data types supported for mapping dummy entries in PDOs (one
 *              bit for each of the basic types).
 *
 * \see co_dev_get_dummy()
 */
LELY_CO_EXTERN void co_dev_set_dummy(co_dev_t *dev, co_unsigned32_t dummy);

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
	LELY_CO_EXTERN co_##b##_t co_dev_get_val_##c(const co_dev_t *dev, \
			co_unsigned16_t idx, co_unsigned8_t subidx); \
	LELY_CO_EXTERN size_t co_dev_set_val_##c(co_dev_t *dev, \
			co_unsigned16_t idx, co_unsigned8_t subidx, \
			co_##b##_t c);
#include <lely/co/def/basic.def>
#undef LELY_CO_DEFINE_TYPE

/*!
 * Reads a value from a memory buffer, in the concise DCF format, and stores it
 * in a sub-object in the object dictionary of a CANopen device. If the
 * sub-object does not exist, the value is discarded.
 *
 * \param dev     a pointer to a CANopen device.
 * \param pidx    the address at which to store the object index (can be NULL).
 * \param psubidx the address at which to store the object sub-index (can be
 *                NULL).
 * \param begin   a pointer to the start of the buffer.
 * \param end     a pointer to the end of the buffer.
 *
 * \returns the number of bytes read on success (at least 7), or 0 on error.
 *
 * \see co_dev_write_sub()
 */
LELY_CO_EXTERN size_t co_dev_read_sub(co_dev_t *dev, co_unsigned16_t *pidx,
		co_unsigned8_t *psubidx, const uint8_t *begin,
		const uint8_t *end);

/*!
 * Loads the value of a sub-object from the object dictionary of a CANopen
 * device, and writes it to a memory buffer, in the concise DCF format.
 *
 * \param dev    a pointer to a CANopen device.
 * \param idx    the object index.
 * \param subidx the object sub-index.
 * \param begin  a pointer to the start of the buffer. If \a begin is NULL,
 *               nothing is written.
 * \param end    a pointer to the end of the buffer. If \a end is not NULL, and
 *               the buffer is too small (i.e., `end - begin` is less than the
 *               return value), nothing is written.
 *
 * \returns the number of bytes that would have been written had the buffer been
 * sufficiently large, or 0 on error.
 *
 * \see co_dev_read_sub()
 */
LELY_CO_EXTERN size_t co_dev_write_sub(const co_dev_t *dev, co_unsigned16_t idx,
		co_unsigned8_t subidx, uint8_t *begin, uint8_t *end);

/*!
 * Reads the values of a range of objects from a memory buffer, in the concise
 * DCF format, and stores them in the object dictionary of a CANopen device. If
 * an object does not exist, the value is discarded.
 *
 * \param dev  a pointer to a CANopen device.
 * \param pmin the address at which to store the minimum object index (can be
 *             NULL).
 * \param pmax the address at which to store the maximum object index (can be
 *             NULL).
 * \param ptr  the address of a pointer to a DOMAIN value.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errnum().
 *
 * \see co_dev_read_dcf()
 */
LELY_CO_EXTERN int co_dev_read_dcf(co_dev_t *dev, co_unsigned16_t *pmin,
		co_unsigned16_t *pmax, void *const *ptr);

/*!
 * Loads the values of a range of objects in the object dictionary of a CANopen
 * device, and writes them to a memory buffer, in the concise DCF format.
 *
 * \param dev a pointer to a CANopen device.
 * \param min the minimum object index.
 * \param max the maximum object index.
 * \param ptr the address of a pointer. On success, *\a ptr points to a DOMAIN
 *            value.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errnum().
 *
 * \see co_dev_read_dcf()
 */
LELY_CO_EXTERN int co_dev_write_dcf(const co_dev_t *dev, co_unsigned16_t min,
		co_unsigned16_t max, void **ptr);

#ifdef __cplusplus
}
#endif

#endif

