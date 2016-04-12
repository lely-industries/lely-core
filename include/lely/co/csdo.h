/*!\file
 * This header file is part of the CANopen library; it contains the Client-SDO
 * declarations.
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

#ifndef LELY_CO_CSDO_H
#define LELY_CO_CSDO_H

#include <lely/can/net.h>
#include <lely/co/sdo.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * The type of a CANopen Client-SDO download confirmation callback function,
 * invoked when a download request completes (with success or failure).
 *
 * \param sdo    a pointer to a Client-SDO service.
 * \param idx    the object index.
 * \param subidx the object sub-index.
 * \param ac     the abort code (0 on success).
 * \param data   a pointer to user-specified data.
 */
typedef void co_csdo_dn_con_t(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac, void *data);

/*!
 * The type of a CANopen Client-SDO upload confirmation callback function,
 * invoked when an upload request completes (with success or failure).
 *
 * \param sdo    a pointer to a Client-SDO service.
 * \param idx    the object index.
 * \param subidx the object sub-index.
 * \param ac     the abort code (0 on success).
 * \param ptr    a pointer to the uploaded bytes.
 * \param n      the number of bytes at \a ptr.
 * \param data   a pointer to user-specified data.
 */
typedef void co_csdo_up_con_t(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac, const void *ptr,
		size_t n, void *data);

LELY_CO_EXTERN void *__co_csdo_alloc(void);
LELY_CO_EXTERN void __co_csdo_free(void *ptr);
LELY_CO_EXTERN struct __co_csdo *__co_csdo_init(struct __co_csdo *sdo,
		can_net_t *net, co_dev_t *dev, co_unsigned8_t num);
LELY_CO_EXTERN void __co_csdo_fini(struct __co_csdo *sdo);

/*!
 * Creates a new CANopen Client-SDO service.
 *
 * \param net a pointer to a CAN network.
 * \param dev a pointer to a CANopen device describing the client (can be NULL).
 * \param num the SDO number (in the range [1..128]). The SDO parameter record
 *            MUST exist in the object dictionary of \a dev. However, if \a dev
 *            is NULL, \a num is interpreted as a node-ID (in the range
 *            [1..127]) and the default SDO parameters are used.
 *
 * \returns a pointer to a new Client-SDO service, or NULL on error. In the
 * latter case, the error number can be obtained with `get_errnum()`.
 *
 * \see co_csdo_destroy()
 */
LELY_CO_EXTERN co_csdo_t *co_csdo_create(can_net_t *net, co_dev_t *dev,
		co_unsigned8_t num);

//! Destroys a CANopen Client-SDO service. \see co_csdo_create()
LELY_CO_EXTERN void co_csdo_destroy(co_csdo_t *sdo);

//! Returns the SDO number of a Client-SDO.
LELY_CO_EXTERN co_unsigned8_t co_csdo_get_num(const co_csdo_t *sdo);

//! Returns a pointer to the SDO parameter record of a Client-SDO.
LELY_CO_EXTERN const struct co_sdo_par *co_csdo_get_par(const co_csdo_t *sdo);

/*!
 * Returns the timeout (in milliseconds) of a Client-SDO. A return value of 0
 * (the default) means no timeout is being used.
 *
 * \see co_csdo_set_timeout()
 */
LELY_CO_EXTERN int co_csdo_get_timeout(const co_csdo_t *sdo);

/*!
 * Sets the timeout of a Client-SDO. If the timeout expires before receiving a
 * response from a server, the client aborts the transfer.
 *
 * \param sdo     a pointer to a Client-SDO service.
 * \param timeout the timeout (in milliseconds). A value of 0 (the default)
 *                disables the timeout.
 *
 * \see co_csdo_get_timeout()
 */
LELY_CO_EXTERN void co_csdo_set_timeout(co_csdo_t *sdo, int timeout);

/*!
 * Returns 1 if the specified Client-SDO service is idle, and 0 if a transfer is
 * ongoing.
 */
LELY_CO_EXTERN int co_csdo_is_idle(const co_csdo_t *sdo);

/*!
 * Submits an abort transfer request to a remote Server-SDO. This function has
 * no effect if the Client-SDO service is idle (see co_csdo_is_idle()).
 *
 * \param sdo a pointer to a Client-SDO service.
 * \param ac  the abort code.
 */
LELY_CO_EXTERN void co_csdo_abort_req(co_csdo_t *sdo, co_unsigned32_t ac);

/*!
 * Submits a download request to a remote Server-SDO. This requests the server
 * to download the value and is equivalent to a write operation into a remote
 * object dictionary. Note that the request will fail if another transfer is in
 * progress (see co_csdo_is_idle()).
 *
 * \param sdo    a pointer to a Client-SDO service.
 * \param idx    the remote object index.
 * \param subidx the remote object sub-index.
 * \param ptr    a pointer to the bytes to be downloaded.
 * \param n      the number of bytes at \a ptr.
 * \param con    a pointer to the confirmation function (can be NULL).
 * \param data   a pointer to user-specified data (can be NULL). \a data is
 *               passed as the last parameter to \a con.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_CO_EXTERN int co_csdo_dn_req(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, const void *ptr, size_t n,
		co_csdo_dn_con_t *con, void *data);

/*!
 * Submits a download request to a remote Server-SDO. This requests the server
 * to download the value and is equivalent to a write operation into a remote
 * object dictionary. Note that the request will fail if another transfer is in
 * progress (see co_csdo_is_idle()).
 *
 * \param sdo    a pointer to a Client-SDO service.
 * \param idx    the remote object index.
 * \param subidx the remote object sub-index.
 * \param type   the data type (in the range [1..27]). This MUST be the object
 *               index of one of the static data types.
 * \param val    the address of the value to be written. In case of string or
 *               domains, this MUST be the address of pointer.
 * \param con    a pointer to the confirmation function (can be NULL).
 * \param data   a pointer to user-specified data (can be NULL). \a data is
 *               passed as the last parameter to \a con.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_CO_EXTERN int co_csdo_dn_val_req(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned16_t type, const void *val,
		co_csdo_dn_con_t *con, void *data);

/*!
 * Submits an upload request to a remote Server-SDO. This requests the server
 * to upload the value and is equivalent to a read operation from a remote
 * object dictionary. Note that the request will fail if another transfer is in
 * progress (see co_csdo_is_idle()).
 *
 * \param sdo    a pointer to a Client-SDO service.
 * \param idx    the remote object index.
 * \param subidx the remote object sub-index.
 * \param con    a pointer to the confirmation function (can be NULL).
 * \param data   a pointer to user-specified data (can be NULL). \a data is
 *               passed as the last parameter to \a con.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_CO_EXTERN int co_csdo_up_req(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_csdo_up_con_t *con, void *data);

/*!
 * Submits a block download request to a remote Server-SDO. This requests the
 * server to download the value and is equivalent to a write operation into a
 * remote object dictionary. Note that the request will fail if another transfer
 * is in progress (see co_csdo_is_idle()).
 *
 * \param sdo    a pointer to a Client-SDO service.
 * \param idx    the remote object index.
 * \param subidx the remote object sub-index.
 * \param ptr    a pointer to the bytes to be downloaded.
 * \param n      the number of bytes at \a ptr.
 * \param con    a pointer to the confirmation function (can be NULL).
 * \param data   a pointer to user-specified data (can be NULL). \a data is
 *               passed as the last parameter to \a con.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_CO_EXTERN int co_csdo_blk_dn_req(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, const void *ptr, size_t n,
		co_csdo_dn_con_t *con, void *data);

/*!
 * Submits a block upload request to a remote Server-SDO. This requests the
 * server to upload the value and is equivalent to a read operation from a
 * remote object dictionary. Note that the request will fail if another transfer
 * is in progress (see co_csdo_is_idle()).
 *
 * \param sdo    a pointer to a Client-SDO service.
 * \param idx    the remote object index.
 * \param subidx the remote object sub-index.
 * \param pst    the protocol switch threshold. If \a pst is non-zero, and the
 *               number of bytes to be uploaded is less than or equal to \a pst,
 *               the server may switch to the SDO upload protocol.
 * \param con    a pointer to the confirmation function (can be NULL).
 * \param data   a pointer to user-specified data (can be NULL). \a data is
 *               passed as the last parameter to \a con.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_CO_EXTERN int co_csdo_blk_up_req(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, uint8_t pst, co_csdo_up_con_t *con,
		void *data);

#ifdef __cplusplus
}
#endif

#endif

