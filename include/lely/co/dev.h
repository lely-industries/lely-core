/**@file
 * This header file is part of the CANopen library; it contains the device
 * description declarations.
 *
 * @copyright 2019-2021 Lely Industries N.V.
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

#ifndef LELY_CO_DEV_H_
#define LELY_CO_DEV_H_

#include <lely/co/type.h>

#include <stddef.h>

/// The data type (and object index) of an identity record.
#define CO_DEFSTRUCT_ID 0x0023

/// An identity record.
struct co_id {
	/// Highest sub-index supported.
	co_unsigned8_t n;
	/// Vendor-ID.
	co_unsigned32_t vendor_id;
	/// Product code.
	co_unsigned32_t product_code;
	/// Revision number.
	co_unsigned32_t revision;
	/// Serial number.
	co_unsigned32_t serial_nr;
};

/// The static initializer for struct #co_id.
#define CO_ID_INIT \
	{ \
		4, 0, 0, 0, 0 \
	}

/// The maximum number of CANopen networks.
#define CO_NUM_NETWORKS 127

/// The maximum number of nodes in a CANopen network.
#define CO_NUM_NODES 127

/// A bit rate of 1 Mbit/s.
#define CO_BAUD_1000 0x0001

/// A bit rate of 800 kbit/s.
#define CO_BAUD_800 0x0002

/// A bit rate of 500 kbit/s.
#define CO_BAUD_500 0x0004

/// A bit rate of 250 kbit/s.
#define CO_BAUD_250 0x0008

/// A bit rate of 125 kbit/s.
#define CO_BAUD_125 0x0020

/// A bit rate of 50 kbit/s.
#define CO_BAUD_50 0x0040

/// A bit rate of 20 kbit/s.
#define CO_BAUD_20 0x0080

/// A bit rate of 10 kbit/s.
#define CO_BAUD_10 0x0100

/// Automatic bit rate detection.
#define CO_BAUD_AUTO 0x0200

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a CANopen Transmit-PDO event indication function, invoked by
 * co_dev_tpdo_event() when an event is indicated for (a sub-object mapped into)
 * an acyclic or event-driven PDO.
 *
 * @param num  the PDO number (in the range [1..512]).
 * @param data a pointer to user-specified data.
 */
typedef void co_dev_tpdo_event_ind_t(co_unsigned16_t num, void *data);

/**
 * The type of a CANopen source address mode multiplex PDO event indication
 * function, invoked by co_dev_sam_mpdo_event() when an event is indicated for
 * (a sub-object mapped into) a SAM-MPDO.
 *
 * @param num    the PDO number (in the range [1..512]).
 * @param idx    the object index.
 * @param subidx the object sub-index.
 * @param data   a pointer to user-specified data.
 */
typedef void co_dev_sam_mpdo_event_ind_t(co_unsigned16_t num,
		co_unsigned16_t idx, co_unsigned8_t subidx, void *data);

#if !LELY_NO_MALLOC
void *__co_dev_alloc(void);
void __co_dev_free(void *ptr);
#endif

struct __co_dev *__co_dev_init(struct __co_dev *dev, co_unsigned8_t id);
void __co_dev_fini(struct __co_dev *dev);

#if !LELY_NO_MALLOC

/**
 * Creates a new CANopen device.
 *
 * @param id the node-ID of the device (in the range [1..127, 255]). If
 *           <b>id</b> is 255, the device is unconfigured.
 *
 * @returns a pointer to a new CANopen device, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see co_dev_destroy()
 */
co_dev_t *co_dev_create(co_unsigned8_t id);

/**
 * Destroys a CANopen device, including all objects in its object dictionary.
 *
 * @see co_dev_create()
 */
void co_dev_destroy(co_dev_t *dev);

#endif // !LELY_NO_MALLOC

/// Returns the network-ID of a CANopen device. @see co_dev_set_netid()
co_unsigned8_t co_dev_get_netid(const co_dev_t *dev);

/**
 * Sets the network-ID of a CANopen device.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_dev_get_netid()
 */
int co_dev_set_netid(co_dev_t *dev, co_unsigned8_t id);

/// Returns the node-ID of a CANopen device. @see co_dev_set_id()
co_unsigned8_t co_dev_get_id(const co_dev_t *dev);

/**
 * Sets the node-ID of a CANopen device. This function will also update any
 * sub-object values of the form `$NODEID { "+" number }`.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_dev_get_id()
 */
int co_dev_set_id(co_dev_t *dev, co_unsigned8_t id);

/**
 * Retrieves a list of object indices in the object dictionary of a CANopen
 * device.
 *
 * @param dev    a pointer to a CANopen device.
 * @param maxidx the maximum number of object indices to return.
 * @param idx    an array of at least <b>maxidx</b> indices (can be NULL). On
 *               success, *<b>idx</b> contains the object indices.
 *
 * @returns the total number of object indices in the object dictionary (which
 * may be different from <b>maxidx</b>).
 */
co_unsigned16_t co_dev_get_idx(const co_dev_t *dev, co_unsigned16_t maxidx,
		co_unsigned16_t *idx);

/**
 * Inserts an object into the object dictionary of a CANopen device. This
 * function fails if the object is already part of the object dictionary of
 * another device, or if another object with the same index already exists.
 *
 * @param dev a pointer to a CANopen device.
 * @param obj a pointer to the object to be inserted.
 *
 * @returns 0 on success, or -1 on error.
 *
 * @see co_dev_remove_obj(), co_dev_find_obj()
 */
int co_dev_insert_obj(co_dev_t *dev, co_obj_t *obj);

/**
 * Removes an object from the object dictionary a CANopen device.
 *
 * @param dev a pointer to a CANopen device.
 * @param obj a pointer to the object to be removed.
 *
 * @returns 0 on success, or -1 on error.
 *
 * @see co_dev_insert_obj()
 */
int co_dev_remove_obj(co_dev_t *dev, co_obj_t *obj);

/**
 * Finds an object in the object dictionary of a CANopen device.
 *
 * @param dev a pointer to a CANopen device.
 * @param idx the object index.
 *
 * @returns a pointer to the object if found, or NULL if not.
 *
 * @see co_dev_insert_obj()
 */
co_obj_t *co_dev_find_obj(const co_dev_t *dev, co_unsigned16_t idx);

/**
 * Finds a sub-object in the object dictionary of a CANopen device.
 *
 * @param dev    a pointer to a CANopen device.
 * @param idx    the object index.
 * @param subidx the object sub-index.
 *
 * @returns a pointer to the sub-object if found, or NULL if not.
 */
co_sub_t *co_dev_find_sub(const co_dev_t *dev, co_unsigned16_t idx,
		co_unsigned8_t subidx);

/**
 * Finds the first object (with the lowest index) in the object dictionary of a
 * CANopen device.
 *
 * @returns a pointer to the first object, or NULL if the object dictionary is
 * empty.
 *
 * @see co_dev_last_obj(), co_obj_next()
 */
co_obj_t *co_dev_first_obj(const co_dev_t *dev);

/**
 * Finds the last object (with the highest index) in the object dictionary of a
 * CANopen device.
 *
 * @returns a pointer to the last object, or NULL if the object dictionary is
 * empty.
 *
 * @see co_dev_first_obj(), co_obj_prev()
 */
co_obj_t *co_dev_last_obj(const co_dev_t *dev);

#if !LELY_NO_CO_OBJ_NAME

/// Returns the name of a CANopen device. @see co_dev_set_name()
const char *co_dev_get_name(const co_dev_t *dev);

/**
 * Sets the name of a CANopen device.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_dev_get_name()
 */
int co_dev_set_name(co_dev_t *dev, const char *name);

/**
 * Returns a pointer to the vendor name of a CANopen device.
 *
 * @see co_dev_set_vendor_name()
 */
const char *co_dev_get_vendor_name(const co_dev_t *dev);

/**
 * Sets the vendor name of a CANopen device.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_dev_get_vendor_name()
 */
int co_dev_set_vendor_name(co_dev_t *dev, const char *vendor_name);

#endif // !LELY_NO_CO_OBJ_NAME

/// Returns the vendor ID of a CANopen device. @see co_dev_set_vendor_id()
co_unsigned32_t co_dev_get_vendor_id(const co_dev_t *dev);

/// Sets the vendor ID of a CANopen device. @see co_dev_get_vendor_id()
void co_dev_set_vendor_id(co_dev_t *dev, co_unsigned32_t vendor_id);

#if !LELY_NO_CO_OBJ_NAME

/**
 * Returns a pointer to the product name of a CANopen device.
 *
 * @see co_dev_set_product_name()
 */
const char *co_dev_get_product_name(const co_dev_t *dev);

/**
 * Sets the product name of a CANopen device.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_dev_get_product_name()
 */
int co_dev_set_product_name(co_dev_t *dev, const char *product_name);

#endif // !LELY_NO_CO_OBJ_NAME

/// Returns the product code of a CANopen device. @see co_dev_set_product_code()
co_unsigned32_t co_dev_get_product_code(const co_dev_t *dev);

/// Sets the product code of a CANopen device. @see co_dev_get_product_code()
void co_dev_set_product_code(co_dev_t *dev, co_unsigned32_t product_code);

/// Returns the revision number of a CANopen device. @see co_dev_set_revision()
co_unsigned32_t co_dev_get_revision(const co_dev_t *dev);

/// Sets the revision number of a CANopen device. @see co_dev_get_revision()
void co_dev_set_revision(co_dev_t *dev, co_unsigned32_t revision);

#if !LELY_NO_CO_OBJ_NAME

/**
 * Returns a pointer to the order code of a CANopen device.
 *
 * @see co_dev_set_order_code()
 */
const char *co_dev_get_order_code(const co_dev_t *dev);

/**
 * Sets the order code of a CANopen device.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_dev_get_order_code()
 */
int co_dev_set_order_code(co_dev_t *dev, const char *order_code);

#endif // !LELY_NO_CO_OBJ_NAME

/**
 * Returns the supported bit rates of a CANopen device (any combination of
 * #CO_BAUD_1000, #CO_BAUD_800, #CO_BAUD_500, #CO_BAUD_250, #CO_BAUD_125,
 * #CO_BAUD_50, #CO_BAUD_20, #CO_BAUD_10 and #CO_BAUD_AUTO).
 *
 * @see co_dev_set_baud()
 */
unsigned int co_dev_get_baud(const co_dev_t *dev);

/**
 * Sets the supported bit rates of a CANopen device.
 *
 * @param dev  a pointer to a CANopen device.
 * @param baud the supported bit rates (any combination of #CO_BAUD_1000,
 *             #CO_BAUD_800, #CO_BAUD_500, #CO_BAUD_250, #CO_BAUD_125,
 *             #CO_BAUD_50, #CO_BAUD_20, #CO_BAUD_10 and #CO_BAUD_AUTO).
 *
 * @see co_dev_get_baud()
 */
void co_dev_set_baud(co_dev_t *dev, unsigned int baud);

/**
 * Returns the (pending) baudrate of a CANopen device (in kbit/s).
 *
 * @see co_dev_set_rate()
 */
co_unsigned16_t co_dev_get_rate(const co_dev_t *dev);

/**
 * Sets the (pending) baudrate of a CANopen device.
 *
 * @param dev  a pointer to a CANopen device.
 * @param rate the baudrate (in kbit/s).
 *
 * @see co_dev_get_rate()
 */
void co_dev_set_rate(co_dev_t *dev, co_unsigned16_t rate);

/// Returns 1 if LSS is supported and 0 if not. @see co_dev_set_lss()
int co_dev_get_lss(const co_dev_t *dev);

/// Sets the LSS support flag. @see co_dev_get_lss()
void co_dev_set_lss(co_dev_t *dev, int lss);

/**
 * Returns the data types supported by a CANopen device for mapping dummy
 * entries in PDOs (one bit for each of the basic types).
 *
 * @see co_dev_set_dummy()
 */
co_unsigned32_t co_dev_get_dummy(const co_dev_t *dev);

/**
 * Sets the data types supported by a CANopen device for mapping dummy entries
 * in PDOs.
 *
 * @param dev   a pointer to a CANopen device.
 * @param dummy the data types supported for mapping dummy entries in PDOs (one
 *              bit for each of the basic types).
 *
 * @see co_dev_get_dummy()
 */
void co_dev_set_dummy(co_dev_t *dev, co_unsigned32_t dummy);

/**
 * Returns a pointer to the current value of a CANopen sub-object. In the case
 * of strings or domains, this is the address of a pointer to the first byte in
 * the array.
 *
 * @see co_dev_set_val(), co_sub_get_val()
 */
const void *co_dev_get_val(const co_dev_t *dev, co_unsigned16_t idx,
		co_unsigned8_t subidx);

/**
 * Sets the current value of a CANopen sub-object.
 *
 * @param dev    a pointer to a CANopen device.
 * @param idx    the object index.
 * @param subidx the object sub-index.
 * @param ptr    a pointer to the bytes to be copied. In case of strings or
 *               domains, <b>ptr</b> MUST point to the first byte in the array.
 * @param n      the number of bytes at <b>ptr</b>. In case of strings, <b>n</b>
 *               SHOULD exclude the terminating null byte(s).
 *
 * @returns the number of bytes copied (i.e., <b>n</b>), or 0 on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see co_dev_get_val(), co_sub_set_val()
 */
size_t co_dev_set_val(co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx,
		const void *ptr, size_t n);

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
	co_##b##_t co_dev_get_val_##c(const co_dev_t *dev, \
			co_unsigned16_t idx, co_unsigned8_t subidx); \
	size_t co_dev_set_val_##c(co_dev_t *dev, co_unsigned16_t idx, \
			co_unsigned8_t subidx, co_##b##_t c);
#include <lely/co/def/basic.def>
#undef LELY_CO_DEFINE_TYPE

/**
 * Reads a value from a memory buffer, in the concise DCF format, and stores it
 * in a sub-object in the object dictionary of a CANopen device. If the
 * sub-object does not exist, the value is discarded.
 *
 * @param dev     a pointer to a CANopen device.
 * @param pidx    the address at which to store the object index (can be NULL).
 * @param psubidx the address at which to store the object sub-index (can be
 *                NULL).
 * @param begin   a pointer to the start of the buffer.
 * @param end     a pointer to one past the last byte in the buffer.
 *
 * @returns the number of bytes read on success (at least 7), or 0 on error.
 *
 * @see co_dev_write_sub()
 */
size_t co_dev_read_sub(co_dev_t *dev, co_unsigned16_t *pidx,
		co_unsigned8_t *psubidx, const uint_least8_t *begin,
		const uint_least8_t *end);

/**
 * Loads the value of a sub-object from the object dictionary of a CANopen
 * device, and writes it to a memory buffer, in the concise DCF format.
 *
 * @param dev    a pointer to a CANopen device.
 * @param idx    the object index.
 * @param subidx the object sub-index.
 * @param begin  a pointer to the start of the buffer. If <b>begin</b> is NULL,
 *               nothing is written.
 * @param end    a pointer to one past the last byte in the buffer. If
 *               <b>end</b> is not NULL, and the buffer is too small
 *               (i.e., `end - begin` is less than the return value), nothing is
 *               written.
 *
 * @returns the number of bytes that would have been written had the buffer been
 * sufficiently large, or 0 on error.
 *
 * @see co_dev_read_sub()
 */
size_t co_dev_write_sub(const co_dev_t *dev, co_unsigned16_t idx,
		co_unsigned8_t subidx, uint_least8_t *begin,
		uint_least8_t *end);

/**
 * Reads the values of a range of objects from a memory buffer, in the concise
 * DCF format, and stores them in the object dictionary of a CANopen device. If
 * an object does not exist, the value is discarded.
 *
 * @param dev  a pointer to a CANopen device.
 * @param pmin the address at which to store the minimum object index (can be
 *             NULL).
 * @param pmax the address at which to store the maximum object index (can be
 *             NULL).
 * @param ptr  the address of a pointer to a DOMAIN value.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_dev_write_dcf()
 */
int co_dev_read_dcf(co_dev_t *dev, co_unsigned16_t *pmin, co_unsigned16_t *pmax,
		void *const *ptr);

/**
 * Reads the values of a range of objects from a file, in the concise DCF
 * format, and stores them in the object dictionary of a CANopen device. If an
 * object does not exist, the value is discarded.
 *
 * @param dev      a pointer to a CANopen device.
 * @param pmin     the address at which to store the minimum object index (can
 *                 be NULL).
 * @param pmax     the address at which to store the maximum object index (can
 *                 be NULL).
 * @param filename a pointer to the name of the file.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_dev_read_dcf()
 */
int co_dev_read_dcf_file(co_dev_t *dev, co_unsigned16_t *pmin,
		co_unsigned16_t *pmax, const char *filename);

/**
 * Loads the values of a range of objects in the object dictionary of a CANopen
 * device, and writes them to a memory buffer, in the concise DCF format.
 *
 * @param dev a pointer to a CANopen device.
 * @param min the minimum object index.
 * @param max the maximum object index.
 * @param ptr the address of a pointer. On success, *<b>ptr</b> points to a
 *            DOMAIN value.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_dev_read_dcf()
 */
int co_dev_write_dcf(const co_dev_t *dev, co_unsigned16_t min,
		co_unsigned16_t max, void **ptr);

/**
 * Loads the values of a range of objects in the object dictionary of a CANopen
 * device, and writes them to a file, in the concise DCF format.
 *
 * @param dev      a pointer to a CANopen device.
 * @param min      the minimum object index.
 * @param max      the maximum object index.
 * @param filename a pointer to the name of the file.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_dev_write_dcf()
 */
int co_dev_write_dcf_file(const co_dev_t *dev, co_unsigned16_t min,
		co_unsigned16_t max, const char *filename);

/**
 * Retrieves the indication function invoked by co_dev_tpdo_event() when an
 * event is indicated for (a sub-object mapped into) an acyclic or event-driven
 * Transmit-PDO.
 *
 * @param dev   a pointer to a CANopen device.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_dev_set_tpdo_event_ind()
 */
void co_dev_get_tpdo_event_ind(const co_dev_t *dev,
		co_dev_tpdo_event_ind_t **pind, void **pdata);

/**
 * Sets the indication function invoked by co_dev_tpdo_event() when an event is
 * indicated for (a sub-object mapped into) an acyclic or event-driven
 * Transmit-PDO.
 *
 * @param dev  a pointer to a CANopen device.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_dev_get_tpdo_event_ind()
 */
void co_dev_set_tpdo_event_ind(
		co_dev_t *dev, co_dev_tpdo_event_ind_t *ind, void *data);

/**
 * Checks if the specified sub-object in the object dictionary of a CANopen
 * device can be mapped into a PDO and, if so, issues an indication for every
 * valid, acylic or event-driven Transmit-PDO into which the sub-object is
 * mapped by invoking the user-defined callback function set with
 * co_dev_set_tpdo_event_ind(). At most one event is indicated for every
 * matching TPDO.
 *
 * @param dev    a pointer to a CANopen device.
 * @param idx    the object index.
 * @param subidx the object sub-index.
 *
 * @see co_dev_tpdo_event_ind_t
 */
void co_dev_tpdo_event(
		co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx);

/**
 * Retrieves the indication function invoked by co_dev_sam_mpdo_event() when an
 * event is indicated for (a sub-object mapped into) a SAM-MPDO.
 *
 * @param dev   a pointer to a CANopen device.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_dev_set_sam_mpdo_event_ind()
 */
void co_dev_get_sam_mpdo_event_ind(const co_dev_t *dev,
		co_dev_sam_mpdo_event_ind_t **pind, void **pdata);

/**
 * Sets the indication function invoked by co_dev_sam_mpdo_event() when an event is
 * indicated for (a sub-object mapped into) a SAM-MPDO.
 *
 * @param dev  a pointer to a CANopen device.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_dev_get_sam_mpdo_event_ind()
 */
void co_dev_set_sam_mpdo_event_ind(
		co_dev_t *dev, co_dev_sam_mpdo_event_ind_t *ind, void *data);

/**
 * Checks if the specified sub-object in the object dictionary of a CANopen
 * device can be mapped into a source address mode multiplex PDO and, if so,
 * issues an indication for the SAM-MPDO prdoucer Transmit-PDO, if any, by
 * invoking the user-defined callback function set with
 * co_dev_set_sam_mpdo_event_ind().
 *
 * @param dev    a pointer to a CANopen device.
 * @param idx    the object index.
 * @param subidx the object sub-index.
 *
 * @see co_dev_sam_mpdo_event_ind_t
 */
void co_dev_sam_mpdo_event(
		co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_DEV_H_
