/**@file
 * This header file is part of the CANopen library; it contains the Client-SDO
 * declarations.
 *
 * @copyright 2017-2020 Lely Industries N.V.
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

#ifndef LELY_CO_CSDO_H_
#define LELY_CO_CSDO_H_

#include <lely/can/net.h>
#include <lely/co/sdo.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a CANopen Client-SDO download confirmation callback function,
 * invoked when a download request completes (with success or failure).
 *
 * @param sdo    a pointer to a Client-SDO service (NULL in case of a local
 *               device).
 * @param idx    the object index.
 * @param subidx the object sub-index.
 * @param ac     the abort code (0 on success).
 * @param data   a pointer to user-specified data.
 */
typedef void co_csdo_dn_con_t(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac, void *data);

/**
 * The type of a CANopen Client-SDO upload confirmation callback function,
 * invoked when an upload request completes (with success or failure).
 *
 * @param sdo    a pointer to a Client-SDO service (NULL in case of a local
 *               device).
 * @param idx    the object index.
 * @param subidx the object sub-index.
 * @param ac     the abort code (0 on success).
 * @param ptr    a pointer to the uploaded bytes.
 * @param n      the number of bytes at <b>ptr</b>.
 * @param data   a pointer to user-specified data.
 */
typedef void co_csdo_up_con_t(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac, const void *ptr,
		size_t n, void *data);

/**
 * The type of a CANopen Client-SDO request progress indication function, used
 * to notify the user of the progress of the current upload/download request.
 * The indication function is invoked after the size of the value has been
 * sent/received, and again after each block (of at most 127 segments) is
 * sent/received. The last invocation occurs before the upload/download
 * confirmation. No notification is generated for expedited transfers.
 *
 * @param sdo    a pointer to a Client-SDO service.
 * @param idx    the object index.
 * @param subidx the object sub-index.
 * @param size   The total size (in bytes) of the value being
 *               uploaded/downloaded.
 * @param nbyte  The number of bytes already uploaded/downloaded.
 * @param data   a pointer to user-specified data.
 */
typedef void co_csdo_ind_t(const co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, size_t size, size_t nbyte, void *data);

/**
 * Submits a download request to a local device. This is equivalent to a write
 * operation into an dictionary.
 *
 * @param dev    a pointer to CANopen device.
 * @param idx    the remote object index.
 * @param subidx the remote object sub-index.
 * @param ptr    a pointer to the bytes to be downloaded.
 * @param n      the number of bytes at <b>ptr</b>.
 * @param con    a pointer to the confirmation function (can be NULL).
 * @param data   a pointer to user-specified data (can be NULL). <b>data</b> is
 *               passed as the last parameter to <b>con</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_dev_dn_req(co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx,
		const void *ptr, size_t n, co_csdo_dn_con_t *con, void *data);

/**
 * Submits a download request to a local device. This is equivalent to a write
 * operation into an object dictionary.
 *
 * @param dev    a pointer to CANopen device.
 * @param idx    the remote object index.
 * @param subidx the remote object sub-index.
 * @param type   the data type (in the range [1..27]). This MUST be the object
 *               index of one of the static data types.
 * @param val    the address of the value to be written. In case of string or
 *               domains, this MUST be the address of pointer.
 * @param con    a pointer to the confirmation function (can be NULL).
 * @param data   a pointer to user-specified data (can be NULL). <b>data</b> is
 *               passed as the last parameter to <b>con</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_dev_dn_val_req(co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned16_t type, const void *val, co_csdo_dn_con_t *con,
		void *data);

/**
 * Submits a series of download requests to a local device. This function calls
 * co_dev_dn_req() for each entry in the specified concise DCF.
 *
 * @param dev   a pointer to CANopen device.
 * @param begin a pointer the the first byte in a concise DCF (see object 1F22
 *              in CiA 302-3 version 4.1.0).
 * @param end   a pointer to one past the last byte in the concise DCF. At most
 *              `end - begin` bytes are read.
 * @param con   a pointer to the confirmation function (can be NULL).
 * @param data  a pointer to user-specified data (can be NULL). <b>data</b> is
 *              passed as the last parameter to <b>con</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_dev_dn_dcf_req(co_dev_t *dev, const uint_least8_t *begin,
		const uint_least8_t *end, co_csdo_dn_con_t *con, void *data);

/**
 * Submits an upload request to a local device. This is equivalent to a read
 * operation from an object dictionary.
 *
 * @param dev    a pointer to CANopen device.
 * @param idx    the remote object index.
 * @param subidx the remote object sub-index.
 * @param con    a pointer to the confirmation function (can be NULL).
 * @param data   a pointer to user-specified data (can be NULL). <b>data</b> is
 *               passed as the last parameter to <b>con</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_dev_up_req(const co_dev_t *dev, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_csdo_up_con_t *con, void *data);

void *__co_csdo_alloc(void);
void __co_csdo_free(void *ptr);
struct __co_csdo *__co_csdo_init(struct __co_csdo *sdo, can_net_t *net,
		co_dev_t *dev, co_unsigned8_t num);
void __co_csdo_fini(struct __co_csdo *sdo);

/**
 * Creates a new CANopen Client-SDO service. The service is started as if by
 * co_csdo_start().
 *
 * @param net a pointer to a CAN network.
 * @param dev a pointer to a CANopen device describing the client (can be NULL).
 * @param num the SDO number (in the range [1..128]). The SDO parameter record
 *            MUST exist in the object dictionary of <b>dev</b>. However, if
 *            <b>dev</b> is NULL, <b>num</b> is interpreted as a node-ID (in the
 *            range [1..127]) and the default SDO parameters are used.
 *
 * @returns a pointer to a new Client-SDO service, or NULL on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see co_csdo_destroy()
 */
co_csdo_t *co_csdo_create(can_net_t *net, co_dev_t *dev, co_unsigned8_t num);

/// Destroys a CANopen Client-SDO service. @see co_csdo_create()
void co_csdo_destroy(co_csdo_t *sdo);

/**
 * Starts a Client-SDO service.
 *
 * @post on success, co_csdo_is_stopped() returns 0.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_csdo_stop()
 */
int co_csdo_start(co_csdo_t *sdo);

/**
 * Stops a Client-SDO service. Any ongoing request is aborted.
 *
 * @post co_csdo_is_stopped() returns 1.
 *
 * @see co_csdo_start()
 */
void co_csdo_stop(co_csdo_t *sdo);

/**
 * Retuns 1 if the specified Client-SDO service is stopped, and 0 if not.
 *
 * @see co_csdo_start, co_csdo_stop()
 */
int co_csdo_is_stopped(const co_csdo_t *sdo);

/// Returns a pointer to the CAN network of a Client-SDO.
can_net_t *co_csdo_get_net(const co_csdo_t *sdo);

/// Returns a pointer to the CANopen device of a Client-SDO.
co_dev_t *co_csdo_get_dev(const co_csdo_t *sdo);

/// Returns the SDO number of a Client-SDO.
co_unsigned8_t co_csdo_get_num(const co_csdo_t *sdo);

/// Returns a pointer to the SDO parameter record of a Client-SDO.
const struct co_sdo_par *co_csdo_get_par(const co_csdo_t *sdo);

/**
 * Returns the timeout (in milliseconds) of a Client-SDO. A return value of 0
 * (the default) means no timeout is being used.
 *
 * @see co_csdo_set_timeout()
 */
int co_csdo_get_timeout(const co_csdo_t *sdo);

/**
 * Sets the timeout of a Client-SDO. If the timeout expires before receiving a
 * response from a server, the client aborts the transfer.
 *
 * @param sdo     a pointer to a Client-SDO service.
 * @param timeout the timeout (in milliseconds). A value of 0 (the default)
 *                disables the timeout.
 *
 * @see co_csdo_get_timeout()
 */
void co_csdo_set_timeout(co_csdo_t *sdo, int timeout);

/**
 * Retrieves the indication function used to notify the user of the progress of
 * the current SDO download request.
 *
 * @param sdo   a pointer to a Client-SDO service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_csdo_set_dn_ind()
 */
void co_csdo_get_dn_ind(
		const co_csdo_t *sdo, co_csdo_ind_t **pind, void **pdata);

/**
 * Sets the indication function used to notify the user of the progress of the
 * current SDO download request.
 *
 * @param sdo  a pointer to a Client-SDO service.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_csdo_get_dn_ind()
 */
void co_csdo_set_dn_ind(co_csdo_t *sdo, co_csdo_ind_t *ind, void *data);

/**
 * Retrieves the indication function used to notify the user of the progress of
 * the current SDO upload request.
 *
 * @param sdo   a pointer to a Client-SDO service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_csdo_set_up_ind()
 */
void co_csdo_get_up_ind(
		const co_csdo_t *sdo, co_csdo_ind_t **pind, void **pdata);

/**
 * Sets the indication function used to notify the user of the progress of the
 * current SDO upload request.
 *
 * @param sdo  a pointer to a Client-SDO service.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_csdo_get_up_ind()
 */
void co_csdo_set_up_ind(co_csdo_t *sdo, co_csdo_ind_t *ind, void *data);

/**
 * Returns 1 of the COB-IDs of the specified Client-SDO service are valid, and 0
 * if not.
 */
int co_csdo_is_valid(const co_csdo_t *sdo);

/**
 * Returns 1 if the specified Client-SDO service is idle, and 0 if a transfer is
 * ongoing.
 */
int co_csdo_is_idle(const co_csdo_t *sdo);

/**
 * Submits an abort transfer request to a remote Server-SDO. This function has
 * no effect if the Client-SDO service is idle (see co_csdo_is_idle()).
 *
 * @param sdo a pointer to a Client-SDO service.
 * @param ac  the abort code.
 */
void co_csdo_abort_req(co_csdo_t *sdo, co_unsigned32_t ac);

/**
 * Submits a download request to a remote Server-SDO. This requests the server
 * to download the value and is equivalent to a write operation into a remote
 * object dictionary. Note that the request will fail if another transfer is in
 * progress (see co_csdo_is_idle()).
 *
 * @param sdo    a pointer to a Client-SDO service.
 * @param idx    the remote object index.
 * @param subidx the remote object sub-index.
 * @param ptr    a pointer to the bytes to be downloaded. It is the
 *               responsibility of the user to ensure that the buffer remains
 *               valid until the operation completes.
 * @param n      the number of bytes at <b>ptr</b>.
 * @param con    a pointer to the confirmation function (can be NULL).
 * @param data   a pointer to user-specified data (can be NULL). <b>data</b> is
 *               passed as the last parameter to <b>con</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_csdo_dn_req(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		const void *ptr, size_t n, co_csdo_dn_con_t *con, void *data);

/**
 * Submits a download request to a remote Server-SDO. This requests the server
 * to download the value and is equivalent to a write operation into a remote
 * object dictionary. Note that the request will fail if another transfer is in
 * progress (see co_csdo_is_idle()).
 *
 * @param sdo    a pointer to a Client-SDO service.
 * @param idx    the remote object index.
 * @param subidx the remote object sub-index.
 * @param type   the data type (in the range [1..27]). This MUST be the object
 *               index of one of the static data types.
 * @param val    the address of the value to be written. In case of string or
 *               domains, this MUST be the address of pointer.
 * @param con    a pointer to the confirmation function (can be NULL).
 * @param data   a pointer to user-specified data (can be NULL). <b>data</b> is
 *               passed as the last parameter to <b>con</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_csdo_dn_val_req(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned16_t type, const void *val,
		co_csdo_dn_con_t *con, void *data);

/**
 * Submits a series of download requests to a remote Server-SDO. This function
 * calls co_csdo_dn_req() for each entry in the specified concise DCF.
 *
 * @param sdo   a pointer to a Client-SDO service.
 * @param begin a pointer the the first byte in a concise DCF (see object 1F22
 *              in CiA 302-3 version 4.1.0).
 * @param end   a pointer to one past the last byte in the concise DCF. At most
 *              `end - begin` bytes are read.
 * @param con   a pointer to the confirmation function (can be NULL).
 * @param data  a pointer to user-specified data (can be NULL). <b>data</b> is
 *              passed as the last parameter to <b>con</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_csdo_dn_dcf_req(co_csdo_t *sdo, const uint_least8_t *begin,
		const uint_least8_t *end, co_csdo_dn_con_t *con, void *data);

/**
 * Submits an upload request to a remote Server-SDO. This requests the server
 * to upload the value and is equivalent to a read operation from a remote
 * object dictionary. Note that the request will fail if another transfer is in
 * progress (see co_csdo_is_idle()).
 *
 * @param sdo    a pointer to a Client-SDO service.
 * @param idx    the remote object index.
 * @param subidx the remote object sub-index.
 * @param con    a pointer to the confirmation function (can be NULL).
 * @param data   a pointer to user-specified data (can be NULL). <b>data</b> is
 *               passed as the last parameter to <b>con</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_csdo_up_req(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_csdo_up_con_t *con, void *data);

/**
 * Submits a block download request to a remote Server-SDO. This requests the
 * server to download the value and is equivalent to a write operation into a
 * remote object dictionary. Note that the request will fail if another transfer
 * is in progress (see co_csdo_is_idle()).
 *
 * @param sdo    a pointer to a Client-SDO service.
 * @param idx    the remote object index.
 * @param subidx the remote object sub-index.
 * @param ptr    a pointer to the bytes to be downloaded.
 * @param n      the number of bytes at <b>ptr</b>.
 * @param con    a pointer to the confirmation function (can be NULL).
 * @param data   a pointer to user-specified data (can be NULL). <b>data</b> is
 *               passed as the last parameter to <b>con</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_csdo_blk_dn_req(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, const void *ptr, size_t n,
		co_csdo_dn_con_t *con, void *data);

/**
 * Submits a block download request to a remote Server-SDO. This requests the
 * server to download the value and is equivalent to a write operation into a
 * remote object dictionary. Note that the request will fail if another transfer
 * is in progress (see co_csdo_is_idle()).
 *
 * @param sdo    a pointer to a Client-SDO service.
 * @param idx    the remote object index.
 * @param subidx the remote object sub-index.
 * @param type   the data type (in the range [1..27]). This MUST be the object
 *               index of one of the static data types.
 * @param val    the address of the value to be written. In case of string or
 *               domains, this MUST be the address of pointer.
 * @param con    a pointer to the confirmation function (can be NULL).
 * @param data   a pointer to user-specified data (can be NULL). <b>data</b> is
 *               passed as the last parameter to <b>con</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_csdo_blk_dn_val_req(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned16_t type, const void *val,
		co_csdo_dn_con_t *con, void *data);

/**
 * Submits a block upload request to a remote Server-SDO. This requests the
 * server to upload the value and is equivalent to a read operation from a
 * remote object dictionary. Note that the request will fail if another transfer
 * is in progress (see co_csdo_is_idle()).
 *
 * @param sdo    a pointer to a Client-SDO service.
 * @param idx    the remote object index.
 * @param subidx the remote object sub-index.
 * @param pst    the protocol switch threshold. If <b>pst</b> is non-zero, and
 *               the number of bytes to be uploaded is less than or equal to
 *               <b>pst</b>, the server may switch to the SDO upload protocol.
 * @param con    a pointer to the confirmation function (can be NULL).
 * @param data   a pointer to user-specified data (can be NULL). <b>data</b> is
 *               passed as the last parameter to <b>con</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_csdo_blk_up_req(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned8_t pst,
		co_csdo_up_con_t *con, void *data);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_CSDO_H_
