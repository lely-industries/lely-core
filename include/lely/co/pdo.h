/*!\file
 * This header file is part of the CANopen library; it contains the Process Data
 * Object (PDO) declarations.
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

#ifndef LELY_CO_PDO_H
#define LELY_CO_PDO_H

#include <lely/co/type.h>

//! The bit in the PDO COB-ID specifying whether the PDO exists and is valid.
#define CO_PDO_COBID_VALID	UINT32_C(0x80000000)

//! The bit in the PDO COB-ID specifying whether RTR is allowed.
#define CO_PDO_COBID_RTR	UINT32_C(0x40000000)

/*!
 * The bit in the PDO COB-ID specifying whether to use an 11-bit (0) or 29-bit
 * (1) CAN-ID.
 */
#define CO_PDO_COBID_FRAME	UINT32_C(0x20000000)

//! The data type (and object index) of a PDO communication parameter record.
#define CO_DEFSTRUCT_PDO_COMM_PAR	0x0020

//! A PDO communication parameter record.
struct co_pdo_comm_par {
	//! Highest sub-index supported.
	co_unsigned8_t n;
	//! COB-ID.
	co_unsigned32_t cobid;
	//! Transmission type.
	co_unsigned8_t trans;
	//! Inhibit time.
	co_unsigned16_t inhibit;
	co_unsigned8_t reserved;
	//! Event timer.
	co_unsigned16_t event;
	//! SYNC start value.
	co_unsigned8_t sync;
};

//! The static initializer from struct #co_pdo_comm_par.
#define CO_PDO_COMM_PAR_INIT	{ 6, CO_PDO_COBID_VALID, 0, 0, 0, 0, 0 }

//! The data type (and object index) of a PDO mapping parameter record.
#define CO_DEFSTRUCT_PDO_MAP_PAR	0x0021

//! A PDO mapping parameter record.
struct co_pdo_map_par {
	//! Number of mapped objects in PDO.
	co_unsigned8_t n;
	//! An array of objects to be mapped.
	co_unsigned32_t map[0x40];
};

//! The static initializer from struct #co_pdo_map_par.
#define CO_PDO_MAP_PAR_INIT \
	{ \
		0, \
		{ \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0 \
		} \
	}

// The CANopen SDO upload/download request from lely/co/sdo.h.
struct co_sdo_req;

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Checks if the specified object is valid and can be mapped into a Receive-PDO.
 *
 * \param dev    a pointer to a CANopen device.
 * \param idx    the object index.
 * \param subidx the object sub-index.
 *
 * \returns 0 if the object is valid and can be mapped, or an SDO abort code on
 * error.
 */
LELY_CO_EXTERN co_unsigned32_t co_dev_chk_rpdo(const co_dev_t *dev,
		co_unsigned16_t idx, co_unsigned8_t subidx);

/*!
 * Checks if the specified object is valid and can be mapped into a
 * Transmit-PDO.
 *
 * \param dev    a pointer to a CANopen device.
 * \param idx    the object index.
 * \param subidx the object sub-index.
 *
 * \returns 0 if the object is valid and can be mapped, or an SDO abort code on
 * error.
 */
LELY_CO_EXTERN co_unsigned32_t co_dev_chk_tpdo(const co_dev_t *dev,
		co_unsigned16_t idx, co_unsigned8_t subidx);

/*!
 * Maps values into a PDO.
 *
 * \param par a pointer to the PDO mapping parameters.
 * \param val a pointer to the values to map.
 * \param n   the number of values at \a val.
 * \param buf the address at which to store the mapped values (can be NULL).
 * \param pn  the address of a value containing the size (in bytes) of the
 *            buffer at \a buf. On exit, if \a pn is not NULL, *\a pn contains
 *            the number of bytes that would have been written had the buffer at
 *            \a buf been sufficiently large.
 *
 * \returns 0 on success, or an SDO abort code on error.
 *
 * \see co_pdo_unmap()
 */
LELY_CO_EXTERN co_unsigned32_t co_pdo_map(const struct co_pdo_map_par *par,
		const co_unsigned64_t *val, size_t n, uint8_t *buf, size_t *pn);

/*!
 * Unmaps a PDO into its constituent values.
 *
 * \param par a pointer to the PDO mapping parameters.
 * \param buf a pointer to the mapped values.
 * \param n   the number of bytes at \a buf.
 * \param val the address at which to store the unmapped values (can be NULL).
 * \param pn  the address of a value containing the size (in number of values)
 *            of the buffer at \a val. On exit, if \a pn is not NULL, *\a pn
 *            contains the number of values that would have been written had the
 *            buffer at \a val been sufficiently large.
 *
 * \returns 0 on success, or an SDO abort code on error.
 *
 * \see co_pdo_map()
 */
LELY_CO_EXTERN co_unsigned32_t co_pdo_unmap(const struct co_pdo_map_par *par,
		const uint8_t *buf, size_t n, co_unsigned64_t *val, size_t *pn);

/*!
 * Performs a PDO read service by reading the mapped values and writing them to
 * the object dictionary through a local SDO download request.
 *
 * \param par a pointer to the PDO mapping parameters.
 * \param dev a pointer to a CANopen device.
 * \param req a pointer to the CANopen SDO download request used for writing to
 *            the object dictionary.
 * \param buf a pointer to the mapped values.
 * \param n   the number of bytes at \a buf.
 *
 * \returns 0 on success, or an SDO abort code on error.
 */
LELY_CO_EXTERN co_unsigned32_t co_pdo_read(const struct co_pdo_map_par *par,
		co_dev_t *dev, struct co_sdo_req *req, const uint8_t *buf,
		size_t n);

/*!
 * Performs a PDO write service by writing the mapped values obtained from the
 * object dictionary through a local SDO upload request.
 *
 * \param par a pointer to the PDO mapping parameters.
 * \param dev a pointer to a CANopen device.
 * \param req a pointer to the CANopen SDO upload request used for reading from
 *            the object dictionary.
 * \param buf the address at which to store the mapped values (can be NULL).
 * \param pn  the address of a value containing the size (in bytes) of the
 *            buffer at \a buf. On exit, if \a pn is not NULL, *\a pn contains
 *            the number of bytes that would have been written had the buffer at
 *            \a buf been sufficiently large.
 *
 * \returns 0 on success, or an SDO abort code on error.
 */
LELY_CO_EXTERN co_unsigned32_t co_pdo_write(const struct co_pdo_map_par *par,
		const co_dev_t *dev, struct co_sdo_req *req, uint8_t *buf,
		size_t *pn);

#ifdef __cplusplus
}
#endif

#endif

