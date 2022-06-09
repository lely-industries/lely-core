/**@file
 * This header file is part of the CANopen library; it contains the Process Data
 * Object (PDO) declarations.
 *
 * @copyright 2016-2022 Lely Industries N.V.
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

#ifndef LELY_CO_PDO_H_
#define LELY_CO_PDO_H_

#include <lely/co/type.h>

/// The maximum number of Receive/Transmit-PDOs.
#define CO_NUM_PDOS 512

/**
 * The maximum number of mapped application objects in a single PDO. This value
 * is also the highest sub-index in the PDO mapping parameter object.
 */
#define CO_PDO_NUM_MAPS 0x40u

/// The bit in the PDO COB-ID specifying whether the PDO exists and is valid.
#define CO_PDO_COBID_VALID UINT32_C(0x80000000)

/// The bit in the PDO COB-ID specifying whether RTR is allowed.
#define CO_PDO_COBID_RTR UINT32_C(0x40000000)

/**
 * The bit in the PDO COB-ID specifying whether to use an 11-bit (0) or 29-bit
 * (1) CAN-ID.
 */
#define CO_PDO_COBID_FRAME UINT32_C(0x20000000)

/**
 * The value of sub-index 0 of the PDO mapping parameter record indicating a
 * a source address mode multiplex PDO (SAM-MPDO).
 */
#define CO_PDO_MAP_SAM_MPDO 0xfe

/**
 * The value of sub-index 0 of the PDO mapping parameter record indicating a
 * a destination address mode multiplex PDO (DAM-MPDO).
 */
#define CO_PDO_MAP_DAM_MPDO 0xff

/// The data type (and object index) of a PDO communication parameter record.
#define CO_DEFSTRUCT_PDO_COMM_PAR 0x0020

/// A PDO communication parameter record.
struct co_pdo_comm_par {
	/// Highest sub-index supported.
	co_unsigned8_t n;
	/// COB-ID.
	co_unsigned32_t cobid;
	/// Transmission type.
	co_unsigned8_t trans;
	/// Inhibit time.
	co_unsigned16_t inhibit;
	co_unsigned8_t reserved;
	/// Event timer.
	co_unsigned16_t event;
	/// SYNC start value.
	co_unsigned8_t sync;
};

/// The static initializer from struct #co_pdo_comm_par.
#define CO_PDO_COMM_PAR_INIT \
	{ \
		6, CO_PDO_COBID_VALID, 0, 0, 0, 0, 0 \
	}

/// The data type (and object index) of a PDO mapping parameter record.
#define CO_DEFSTRUCT_PDO_MAP_PAR 0x0021

/// A PDO mapping parameter record.
struct co_pdo_map_par {
	/// Number of mapped objects in PDO.
	co_unsigned8_t n;
	/// An array of objects to be mapped.
	co_unsigned32_t map[CO_PDO_NUM_MAPS];
};

/// The static initializer from struct #co_pdo_map_par.
// clang-format off
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
// clang-format on

// The CANopen SDO upload/download request from lely/co/sdo.h.
struct co_sdo_req;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Checks if the specified object is valid and can be mapped into a Receive-PDO.
 *
 * @param dev    a pointer to a CANopen device.
 * @param idx    the object index.
 * @param subidx the object sub-index.
 *
 * @returns 0 if the object is valid and can be mapped, or an SDO abort code on
 * error.
 */
co_unsigned32_t co_dev_chk_rpdo(const co_dev_t *dev, co_unsigned16_t idx,
		co_unsigned8_t subidx);

/**
 * Configures the communication and parameters of a Receive-PDO service. This
 * function disables the RPDO before configuring the parameters and re-enables
 * it on success.
 *
 * @param dev  a pointer to a CANopen device.
 * @param num  the PDO number (in the range [1..512]).
 * @param comm a pointer to the communication parameters.
 * @param map  a pointer to the mapping parameters.
 *
 * @returns 0 on success, or an SDO abort code on error.
 *
 * @see co_dev_cfg_rpdo_comm(), co_dev_cfg_rpdo_map()
 */
co_unsigned32_t co_dev_cfg_rpdo(const co_dev_t *dev, co_unsigned16_t num,
		const struct co_pdo_comm_par *comm,
		const struct co_pdo_map_par *map);

/**
 * Configures the communication parameters of a Receive-PDO service by updating
 * CANopen object 1400 - 15FF (RPDO communication parameter).
 *
 * @param dev a pointer to a CANopen device.
 * @param num the PDO number (in the range [1..512]).
 * @param par a pointer to the communication parameters.
 *
 * @returns 0 on success, or an SDO abort code on error.
 */
co_unsigned32_t co_dev_cfg_rpdo_comm(const co_dev_t *dev, co_unsigned16_t num,
		const struct co_pdo_comm_par *par);

/**
 * Configures the mapping parameters of a Receive-PDO service by updating
 * CANopen object 1600 - 17FF (RPDO mapping parameter). It is the responsibility
 * of the caller to disable the RPDO before changing the mapping.
 *
 * @param dev a pointer to a CANopen device.
 * @param num the PDO number (in the range [1..512]).
 * @param par a pointer to the mapping parameters.
 *
 * @returns 0 on success, or an SDO abort code on error.
 */
co_unsigned32_t co_dev_cfg_rpdo_map(const co_dev_t *dev, co_unsigned16_t num,
		const struct co_pdo_map_par *par);

/**
 * Checks if the specified object is valid and can be mapped into a
 * Transmit-PDO.
 *
 * @param dev    a pointer to a CANopen device.
 * @param idx    the object index.
 * @param subidx the object sub-index.
 *
 * @returns 0 if the object is valid and can be mapped, or an SDO abort code on
 * error.
 */
co_unsigned32_t co_dev_chk_tpdo(const co_dev_t *dev, co_unsigned16_t idx,
		co_unsigned8_t subidx);

/**
 * Configures the communication and parameters of a Transmit-PDO service. This
 * function disables the TPDO before configuring the parameters and re-enables
 * it on success.
 *
 * @param dev  a pointer to a CANopen device.
 * @param num  the PDO number (in the range [1..512]).
 * @param comm a pointer to the communication parameters.
 * @param map  a pointer to the mapping parameters.
 *
 * @returns 0 on success, or an SDO abort code on error.
 *
 * @see co_dev_cfg_tpdo_comm(), co_dev_cfg_tpdo_map()
 */
co_unsigned32_t co_dev_cfg_tpdo(const co_dev_t *dev, co_unsigned16_t num,
		const struct co_pdo_comm_par *comm,
		const struct co_pdo_map_par *map);

/**
 * Configures the communication parameters of a Transmit-PDO service by updating
 * CANopen object 1800 - 19FF (TPDO communication parameter).
 *
 * @param dev a pointer to a CANopen device.
 * @param num the PDO number (in the range [1..512]).
 * @param par a pointer to the communication parameters.
 *
 * @returns 0 on success, or an SDO abort code on error.
 */
co_unsigned32_t co_dev_cfg_tpdo_comm(const co_dev_t *dev, co_unsigned16_t num,
		const struct co_pdo_comm_par *par);

/**
 * Configures the mapping parameters of a Transmit-PDO service by updating
 * CANopen object 1A00 - 1BFF (TPDO mapping parameter). It is the responsibility
 * of the caller to disable the TPDO before changing the mapping.
 *
 * @param dev a pointer to a CANopen device.
 * @param num the PDO number (in the range [1..512]).
 * @param par a pointer to the mapping parameters.
 *
 * @returns 0 on success, or an SDO abort code on error.
 */
co_unsigned32_t co_dev_cfg_tpdo_map(const co_dev_t *dev, co_unsigned16_t num,
		const struct co_pdo_map_par *par);

/**
 * Checks if the specified object is part of the object scanner list (objects
 * 1FA0..1FCF) and can be transmitted with a SAM-MPDO. Note that this function
 * does _not_ check if the object is valid and can be mapped into a TPDO.
 *
 * @param dev    a pointer to a CANopen device.
 * @param idx    the object index.
 * @param subidx the object sub-index.
 *
 * @returns 1 if the object was found in the object scanner list, and 0 if not.
 *
 * @see co_dev_chk_tpdo()
 */
int co_dev_chk_sam_mpdo(const co_dev_t *dev, co_unsigned16_t idx,
		co_unsigned8_t subidx);

/**
 * Checks if the specified remote object is part of the object dispatching list
 * (objects 1FD0..1FFF) and can be mapped to a local object with a SAM-MPDO.
 * Note that this function does not check if the local object is valid and can
 * be mapped to an RPDO.
 *
 * @param dev     a pointer to a CANopen device.
 * @param id      the remote node-ID (in the range [1..127]).
 * @param idx     the remote object index.
 * @param subidx  the remote object sub-index.
 * @param pidx    the address at which to store the local object index (can be
 *                NULL).
 * @param psubidx the address at which to store the object sub-index (can be
 *                NULL).
 *
 * @returns 1 if the remote object was found in the object dispatching list, and
 * 0 if not.
 *
 * @see co_dev_chk_rpdo()
 */
int co_dev_map_sam_mpdo(const co_dev_t *dev, co_unsigned8_t id,
		co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned16_t *pidx, co_unsigned8_t *psubidx);

/**
 * Maps values into a PDO.
 *
 * @param par a pointer to the PDO mapping parameters.
 * @param val a pointer to the values to map.
 * @param n   the number of values at <b>val</b>.
 * @param buf the address at which to store the mapped values (can be NULL).
 * @param pn  the address of a value containing the size (in bytes) of the
 *            buffer at <b>buf</b>. On exit, if <b>pn</b> is not NULL,
 *            *<b>pn</b> contains the number of bytes that would have been
 *            written had the buffer at <b>buf</b> been sufficiently large.
 *
 * @returns 0 on success, or an SDO abort code on error.
 *
 * @see co_pdo_unmap()
 */
co_unsigned32_t co_pdo_map(const struct co_pdo_map_par *par,
		const co_unsigned64_t *val, co_unsigned8_t n,
		uint_least8_t *buf, size_t *pn);

/**
 * Unmaps a PDO into its constituent values.
 *
 * @param par a pointer to the PDO mapping parameters.
 * @param buf a pointer to the mapped values.
 * @param n   the number of bytes at <b>buf</b>.
 * @param val the address at which to store the unmapped values (can be NULL).
 * @param pn  the address of a value containing the size (in number of values)
 *            of the buffer at <b>val</b>. On exit, if <b>pn</b> is not NULL,
 *            *<b>pn</b> contains the number of values that would have been
 *            written had the buffer at <b>val</b> been sufficiently large.
 *
 * @returns 0 on success, or an SDO abort code on error.
 *
 * @see co_pdo_map()
 */
co_unsigned32_t co_pdo_unmap(const struct co_pdo_map_par *par,
		const uint_least8_t *buf, size_t n, co_unsigned64_t *val,
		co_unsigned8_t *pn);

/**
 * Writes mapped PDO values to the object dictionary through a local SDO
 * download request.
 *
 * @param par a pointer to the PDO mapping parameters.
 * @param dev a pointer to a CANopen device.
 * @param req a pointer to the CANopen SDO download request used for writing to
 *            the object dictionary.
 * @param buf a pointer to the mapped values.
 * @param n   the number of bytes at <b>buf</b>.
 * @param chk a flag indicating whether co_dev_chk_rpdo() should be invoked
 *            before writing values to the object dictionary.
 *
 * @returns 0 on success, or an SDO abort code on error.
 *
 * @see co_dev_chk_rpdo(), co_dev_map_sam_mpdo()
 */
co_unsigned32_t co_pdo_dn(const struct co_pdo_map_par *par, co_dev_t *dev,
		struct co_sdo_req *req, const uint_least8_t *buf, size_t n,
		int chk);

/**
 * Reads mapped PDO values from the object dictionary through a local SDO upload
 * request. Note that this function does _not_ support multiplex PDOs (see
 * co_sam_mpdo_up()).
 *
 * @param par a pointer to the PDO mapping parameters.
 * @param dev a pointer to a CANopen device.
 * @param req a pointer to the CANopen SDO upload request used for reading from
 *            the object dictionary.
 * @param buf the address at which to store the mapped values (can be NULL).
 * @param pn  the address of a value containing the size (in bytes) of the
 *            buffer at <b>buf</b>. On exit, if <b>pn</b> is not NULL,
 *            *<b>pn</b> contains the number of bytes that would have been
 *            written had the buffer at <b>buf</b> been sufficiently large.
 * @param chk a flag indicating whether co_dev_chk_tpdo() should be invoked
 *            before reading values from the object dictionary.
 *
 * @returns 0 on success, or an SDO abort code on error.
 *
 * @see co_dev_chk_tpdo()
 */
co_unsigned32_t co_pdo_up(const struct co_pdo_map_par *par, const co_dev_t *dev,
		struct co_sdo_req *req, uint_least8_t *buf, size_t *pn,
		int chk);

/**
 * Reads the value of the specified SAM-MPDO-mapped object from the local object
 * dictionary through a local SDO upload request.
 *
 * @param dev    a pointer to a CANopen device.
 * @param idx    the object index.
 * @param subidx the object sub-index.
 * @param req    a pointer to the CANopen SDO upload request used for reading
 *               from the object dictionary.
 * @param buf    the address at which to store the mapped value (can be NULL).

 * @returns 0 on success, or an SDO abort code on error.
 *
 * @see co_dev_chk_tpdo(), co_dev_chk_sam_mpdo()
 */
co_unsigned32_t co_sam_mpdo_up(const co_dev_t *dev, co_unsigned16_t idx,
		co_unsigned8_t subidx, struct co_sdo_req *req,
		uint_least8_t buf[4]);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_PDO_H_
