/**@file
 * This header file is part of the CANopen library; it contains the Service Data
 * Object (SDO) declarations.
 *
 * @copyright 2016-2021 Lely Industries N.V.
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

#ifndef LELY_CO_SDO_H_
#define LELY_CO_SDO_H_

#include <lely/co/type.h>
#include <lely/util/membuf.h>

#ifndef LELY_CO_SDO_INLINE
#define LELY_CO_SDO_INLINE static inline
#endif

/// The bit in the SDO COB-ID specifying whether the SDO exists and is valid.
#define CO_SDO_COBID_VALID UINT32_C(0x80000000)

/**
 * The bit in the SDO COB-ID specifying whether to use an 11-bit (0) or 29-bit
 * (1) CAN-ID.
 */
#define CO_SDO_COBID_FRAME UINT32_C(0x20000000)

/// The data type (and object index) of an SDO parameter record.
#define CO_DEFSTRUCT_SDO_PAR 0x0022

/// An SDO parameter record.
struct co_sdo_par {
	/// Highest sub-index supported.
	co_unsigned8_t n;
	/// COB-ID client -> server.
	co_unsigned32_t cobid_req;
	/// COB-ID server -> client.
	co_unsigned32_t cobid_res;
	/// Node-ID of SDO's client resp. server.
	co_unsigned8_t id;
};

/// The static initializer for struct #co_sdo_par.
#define CO_SDO_PAR_INIT \
	{ \
		3, CO_SDO_COBID_VALID, CO_SDO_COBID_VALID, 0 \
	}

/// SDO abort code: Toggle bit not altered.
#define CO_SDO_AC_TOGGLE UINT32_C(0x05030000)

/// SDO abort code: SDO protocol timed out.
#define CO_SDO_AC_TIMEOUT UINT32_C(0x05040000)

/// SDO abort code: Client/server command specifier not valid or unknown.
#define CO_SDO_AC_NO_CS UINT32_C(0x05040001)

/// SDO abort code: Invalid block size (block mode only).
#define CO_SDO_AC_BLK_SIZE UINT32_C(0x05040002)

/// SDO abort code: Invalid sequence number (block mode only).
#define CO_SDO_AC_BLK_SEQ UINT32_C(0x05040003)

/// SDO abort code: CRC error (block mode only).
#define CO_SDO_AC_BLK_CRC UINT32_C(0x05040004)

/// SDO abort code: Out of memory.
#define CO_SDO_AC_NO_MEM UINT32_C(0x05040005)

/// SDO abort code: Unsupported access to an object.
#define CO_SDO_AC_NO_ACCESS UINT32_C(0x06010000)

/// SDO abort code: Attempt to read a write only object.
#define CO_SDO_AC_NO_READ UINT32_C(0x06010001)

/// SDO abort code: Attempt to write a read only object.
#define CO_SDO_AC_NO_WRITE UINT32_C(0x06010002)

/// SDO abort code: Object does not exist in the object dictionary.
#define CO_SDO_AC_NO_OBJ UINT32_C(0x06020000)

/// SDO abort code: Object cannot be mapped to the PDO.
#define CO_SDO_AC_NO_PDO UINT32_C(0x06040041)

/**
 * SDO abort code: The number and length of the objects to be mapped would
 * exceed the PDO length.
 */
#define CO_SDO_AC_PDO_LEN UINT32_C(0x06040042)

/// SDO abort code: General parameter incompatibility reason.
#define CO_SDO_AC_PARAM UINT32_C(0x06040043)

/// SDO abort code: General internal incompatibility in the device.
#define CO_SDO_AC_COMPAT UINT32_C(0x06040047)

/// SDO abort code: Access failed due to a hardware error.
#define CO_SDO_AC_HARDWARE UINT32_C(0x06060000)

/**
 * SDO abort code: Data type does not match, length of service parameter does
 * not match.
 */
#define CO_SDO_AC_TYPE_LEN UINT32_C(0x06070010)

/**
 * SDO abort code: Data type does not match, length of service parameter too
 * high.
 */
#define CO_SDO_AC_TYPE_LEN_HI UINT32_C(0x06070012)

/**
 * SDO abort code: Data type does not match, length of service parameter too
 * low.
 */
#define CO_SDO_AC_TYPE_LEN_LO UINT32_C(0x06070013)

/// SDO abort code: Sub-index does not exist.
#define CO_SDO_AC_NO_SUB UINT32_C(0x06090011)

/// SDO abort code: Invalid value for parameter (download only).
#define CO_SDO_AC_PARAM_VAL UINT32_C(0x06090030)

/// SDO abort code: Value of parameter written too high (download only).
#define CO_SDO_AC_PARAM_HI UINT32_C(0x06090031)

/// SDO abort code: Value of parameter written too low (download only).
#define CO_SDO_AC_PARAM_LO UINT32_C(0x06090032)

/// SDO abort code: Maximum value is less than minimum value (download only).
#define CO_SDO_AC_PARAM_RANGE UINT32_C(0x06090036)

/// SDO abort code: Resource not available: SDO connection.
#define CO_SDO_AC_NO_SDO UINT32_C(0x060a0023)

/// SDO abort code: General error.
#define CO_SDO_AC_ERROR UINT32_C(0x08000000)

/// SDO abort code: Data cannot be transferred or stored to the application.
#define CO_SDO_AC_DATA UINT32_C(0x08000020)

/**
 * SDO abort code: Data cannot be transferred or stored to the application
 * because of local control.
 */
#define CO_SDO_AC_DATA_CTL UINT32_C(0x08000021)

/**
 * SDO abort code: Data cannot be transferred or stored to the application
 * because of the present device state.
 */
#define CO_SDO_AC_DATA_DEV UINT32_C(0x08000022)

/**
 * SDO abort code: Object dictionary dynamic generation fails or no object
 * dictionary is present (e.g. object dictionary is generated from file and
 * generation fails because of a file error).
 */
#define CO_SDO_AC_NO_OD UINT32_C(0x08000023)

/// SDO abort code: No data available.
#define CO_SDO_AC_NO_DATA UINT32_C(0x08000024)

/// The maximum number of Client/Server-SDOs.
#define CO_NUM_SDOS 128

/// A CANopen SDO upload/download request.
struct co_sdo_req {
	/**
	 * The total size (in bytes) of the value to be uploaded/downloaded.
	 * This value MUST be set at the beginning of a new request and MUST NOT
	 * change afterwards.
	 */
	size_t size;
	/// A pointer to the next bytes to be uploaded/downloaded.
	const void *buf;
	/// The number of bytes available at #buf.
	size_t nbyte;
	/**
	 * The offset of the bytes at #buf. For the first segment,
	 * `#offset == 0`. For the last segment, `#offset + #nbyte == #size`.
	 */
	size_t offset;
	/**
	 * A memory buffer for use by the upload/download indication function.
	 * The memory buffer will be cleared at the beginning of every new
	 * request, but otherwise left untouched.
	 */
	struct membuf membuf;
};

/// The static initializer for struct #co_sdo_req.
#define CO_SDO_REQ_INIT \
	{ \
		0, NULL, 0, 0, MEMBUF_INIT \
	}

#ifdef __cplusplus
extern "C" {
#endif

/// Returns a string describing an SDO abort code.
const char *co_sdo_ac2str(co_unsigned32_t ac);

/// Initializes a CANopen SDO upload/download request. @see co_sdo_req_fini()
void co_sdo_req_init(struct co_sdo_req *req);

/// Finalizes a CANopen SDO upload/download request. @see co_sdo_req_init()
void co_sdo_req_fini(struct co_sdo_req *req);

/// Clears a CANopen SDO upload/download request, including its buffer.
void co_sdo_req_clear(struct co_sdo_req *req);

/**
 * Returns 1 if the specified request includes the first segment, and 0
 * otherwise.
 */
LELY_CO_SDO_INLINE int co_sdo_req_first(const struct co_sdo_req *req);

/**
 * Returns 1 if the specified request includes the last segment, and 0
 * otherwise.
 */
LELY_CO_SDO_INLINE int co_sdo_req_last(const struct co_sdo_req *req);

/**
 * Copies the next segment of the specified CANopen SDO download request to the
 * internal buffer and, on the last segment, returns the buffer.
 *
 * @param req    a pointer to a CANopen SDO download request.
 * @param pptr   the address of a pointer which, on success, points to the first
 *               byte in the buffer (can be NULL).
 * @param pnbyte the address of a value which, on success, contains the total
 *               number of bytes in the buffer (can be NULL).
 * @param pac    the address of a value which, on error, contains the SDO abort
 *               code (can be NULL).
 *
 * @returns 0 if all segments have been copied, and -1 if one or more segments
 * remain or an error has occurred. In the latter case, *<b>pac</b> contains the
 * SDO abort code.
 */
int co_sdo_req_dn(struct co_sdo_req *req, const void **pptr, size_t *pnbyte,
		co_unsigned32_t *pac);

/**
 * Copies the next segment of the specified CANopen SDO download request to the
 * internal buffer and, on the last segment, reads the value.
 *
 * @param req  a pointer to a CANopen SDO download request.
 * @param type the data type of the value.
 * @param val  the address of the value to be downloaded (can be NULL).
 * @param pac  the address of a value which, on error, contains the SDO abort
 *             code (can be NULL).
 *
 * @returns 0 if all segments have been copied and the value has been read, and
 * -1 if one or more segments remain or an error has occurred. In the latter
 * case, *<b>pac</b> contains the SDO abort code.
 *
 * @see co_val_read()
 */
int co_sdo_req_dn_val(struct co_sdo_req *req, co_unsigned16_t type, void *val,
		co_unsigned32_t *pac);

/**
 * Copies the next segment of the specified CANopen SDO download request to the
 * internal buffer and, on the last segment, writes the value to the specified
 * file.
 *
 * @param req      a pointer to a CANopen SDO download request.
 * @param filename a pointer to the name of the file.
 * @param pac      the address of a value which, on error, contains the SDO
 *                 abort code (can be NULL).
 *
 * @returns 0 if all segments have been copied and the value has been read, and
 * -1 if one or more segments remain or an error has occurred. In the latter
 * case, *<b>pac</b> contains the SDO abort code.
 */
int co_sdo_req_dn_file(struct co_sdo_req *req, const char *filename,
		co_unsigned32_t *pac);

/**
 * Writes the specified bytes to a buffer and constructs a CANopen SDO upload
 * request.
 *
 * @param req  a pointer to a CANopen SDO upload request.
 * @param ptr  a pointer to the bytes to be uploaded.
 * @param n    the number of bytes at <b>ptr</b>.
 * @param pac  the address of a value which, on error, contains the SDO abort
 *             code (can be NULL).
 *
 * @returns 0 on success, or -1 on error. In the latter case, *<b>pac</b>
 * contains the SDO abort code.
 */
int co_sdo_req_up(struct co_sdo_req *req, const void *ptr, size_t n,
		co_unsigned32_t *pac);

/**
 * Writes the specified value to a buffer and constructs a CANopen SDO upload
 * request.
 *
 * @param req  a pointer to a CANopen SDO upload request.
 * @param type the data type of the value.
 * @param val  the address of the value to be uploaded. In case of string or
 *             domains, this MUST be the address of pointer.
 * @param pac  the address of a value which, on error, contains the SDO abort
 *             code (can be NULL).
 *
 * @returns 0 on success, or -1 on error. In the latter case, *<b>pac</b>
 * contains the SDO abort code.
 */
int co_sdo_req_up_val(struct co_sdo_req *req, co_unsigned16_t type,
		const void *val, co_unsigned32_t *pac);

/**
 * Loads the specified file into a buffer and constructs a CANopen SDO upload
 * request.
 *
 * @param req      a pointer to a CANopen SDO upload request.
 * @param filename a pointer to the name of the file.
 * @param pac      the address of a value which, on error, contains the SDO
 *                 abort code (can be NULL).
 *
 * @returns 0 on success, or -1 on error. In the latter case, *<b>pac</b>
 * contains the SDO abort code.
 */
int co_sdo_req_up_file(struct co_sdo_req *req, const char *filename,
		co_unsigned32_t *pac);

LELY_CO_SDO_INLINE int
co_sdo_req_first(const struct co_sdo_req *req)
{
	return !req->offset;
}

LELY_CO_SDO_INLINE int
co_sdo_req_last(const struct co_sdo_req *req)
{
	return req->offset + req->nbyte >= req->size;
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_SDO_H_
