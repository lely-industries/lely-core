/**@file
 * This header file is part of the CANopen library; it contains the Transmit-PDO
 * declarations.
 *
 * @copyright 2021 Lely Industries N.V.
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

#ifndef LELY_CO_TPDO_H_
#define LELY_CO_TPDO_H_

#include <lely/can/net.h>
#include <lely/co/pdo.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a CANopen Transmit-PDO indication function, invoked when a PDO is
 * sent or an error occurs.
 *
 * @param pdo  a pointer to a Transmit-PDO service.
 * @param ac   the SDO abort code: 0 on success, #CO_SDO_AC_NO_OBJ,
 *             #CO_SDO_AC_NO_PDO, CO_SDO_AC_NO_READ or #CO_SDO_AC_PDO_LEN in
 *             case of a mapping error, #CO_SDO_AC_TIMEOUT in case the
 *             synchronous time window expires, or #CO_SDO_AC_ERROR if an I/O
 *             error occurs.
 * @param ptr  a pointer to the bytes sent.
 * @param n    the number of bytes at <b>ptr</b>.
 * @param data a pointer to user-specified data.
 */
typedef void co_tpdo_ind_t(co_tpdo_t *pdo, co_unsigned32_t ac, const void *ptr,
		size_t n, void *data);

/**
 * The type of a CANopen Transmit-PDO sampling indication function, invoked when
 * the device starts sampling after the reception of a SYNC event. This function
 * MUST cause co_tpdo_sample_res() to be invoked once the sampling completes.
 *
 * @param pdo  a pointer to a Transmit-PDO service.
 * @param data a pointer to user-specified data.
 *
 * @returns 0 on success, or -1 on error. In the latter case, implementations
 * SHOULD set the error number with `set_errnum()`.
 */
typedef int co_tpdo_sample_ind_t(co_tpdo_t *pdo, void *data);

void *__co_tpdo_alloc(void);
void __co_tpdo_free(void *ptr);
struct __co_tpdo *__co_tpdo_init(struct __co_tpdo *pdo, can_net_t *net,
		co_dev_t *dev, co_unsigned16_t num);
void __co_tpdo_fini(struct __co_tpdo *pdo);

/**
 * Creates a new CANopen Transmit-PDO service. The service is started as if by
 * co_tpdo_start().
 *
 * @param net a pointer to a CAN network.
 * @param dev a pointer to a CANopen device describing the server.
 * @param num the PDO number (in the range [1..512]). The PDO communication and
 *            mapping parameter records MUST exist in the object dictionary of
 *            <b>dev</b>.
 *
 * @returns a pointer to a new Transmit-PDO service, or NULL on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see co_tpdo_destroy()
 */
co_tpdo_t *co_tpdo_create(can_net_t *net, co_dev_t *dev, co_unsigned16_t num);

/// Destroys a CANopen Transmit-PDO service. @see co_tpdo_create()
void co_tpdo_destroy(co_tpdo_t *pdo);

/**
 * Starts a Transmit-PDO service.
 *
 * @post on success, co_tpdo_is_stopped() returns 0.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_tpdo_stop()
 */
int co_tpdo_start(co_tpdo_t *pdo);

/**
 * Stops a Transmit-PDO service.
 *
 * @post co_tpdo_is_stopped() returns 1.
 *
 * @see co_tpdo_start()
 */
void co_tpdo_stop(co_tpdo_t *pdo);

/**
 * Retuns 1 if the specified Transmit-PDO service is stopped, and 0 if not.
 *
 * @see co_tpdo_start, co_tpdo_stop()
 */
int co_tpdo_is_stopped(const co_tpdo_t *pdo);

/// Returns a pointer to the CAN network of a Transmit-PDO.
can_net_t *co_tpdo_get_net(const co_tpdo_t *pdo);

/// Returns a pointer to the CANopen device of a Transmit-PDO.
co_dev_t *co_tpdo_get_dev(const co_tpdo_t *pdo);

/// Returns the PDO number of a Transmit-PDO.
co_unsigned16_t co_tpdo_get_num(const co_tpdo_t *pdo);

/**
 * Returns a pointer to the PDO communication parameter record of a
 * Transmit-PDO.
 */
const struct co_pdo_comm_par *co_tpdo_get_comm_par(const co_tpdo_t *pdo);

/// Returns a pointer to the PDO mapping parameter record of a Transmit-PDO.
const struct co_pdo_map_par *co_tpdo_get_map_par(const co_tpdo_t *pdo);

/**
 * Retrieves the indication function invoked when a Transmit-PDO is sent or an
 * error occurs.
 *
 * @param pdo   a pointer to a Transmit-PDO service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_tpdo_set_ind()
 */
void co_tpdo_get_ind(const co_tpdo_t *pdo, co_tpdo_ind_t **pind, void **pdata);

/**
 * Sets the indication function invoked when a Transmit-PDO is sent or an error
 * occurs.
 *
 * @param pdo  a pointer to a Transmit-PDO service.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_tpdo_get_ind()
 */
void co_tpdo_set_ind(co_tpdo_t *pdo, co_tpdo_ind_t *ind, void *data);

/**
 * Retrieves the indication function invoked when a Transmit-PDO starts sampling
 * after the reception of a SYNC event.
 *
 * @param pdo   a pointer to a Transmit-PDO service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_tpdo_set_sample_ind()
 */
void co_tpdo_get_sample_ind(const co_tpdo_t *pdo, co_tpdo_sample_ind_t **pind,
		void **pdata);

/**
 * Sets the indication function invoked when a Transmit-PDO starts sampling
 * after the reception of a SYNC event.
 *
 * @param pdo  a pointer to a Transmit-PDO service.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_tpdo_get_sample_ind()
 */
void co_tpdo_set_sample_ind(
		co_tpdo_t *pdo, co_tpdo_sample_ind_t *ind, void *data);

/**
 * Triggers the transmission of an acyclic or event-driven PDO. This function
 * has no effect if the PDO is not valid, not event-driven or a multiplex PDO.
 * An error is returned if the inhibit time has not yet elapsed.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_tpdo_sync()
 */
int co_tpdo_event(co_tpdo_t *pdo);

/**
 * Triggers the transmission of a synchronous PDO. This function has no effect
 * if the PDO is not valid or not SYNC-driven.
 *
 * @param pdo a pointer to a Transmit-PDO service.
 * @param cnt the counter value (in the range [0..240]).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_tpdo_event()
 */
int co_tpdo_sync(co_tpdo_t *pdo, co_unsigned8_t cnt);

/**
 * Indicates the result of the sampling step after the reception of a SYNC
 * event. This function MUST be called upon completion.
 *
 * @param pdo a pointer to a Transmit-PDO service.
 * @param ac  the SDO abort code (0 on success). The PDO is not sent if
 *            `ac != 0`.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_tpdo_sample_ind_t
 */
int co_tpdo_sample_res(co_tpdo_t *pdo, co_unsigned32_t ac);

/// Retrieves the time at which the next event-driven TPDO may be sent.
void co_tpdo_get_next(const co_tpdo_t *pdo, struct timespec *tp);

/**
 * Triggers the transmission of a DAM-MPDO. This function has no effect if the
 * PDO is not a valid DAM-MPDO. An error is returned if the inhibit time has not
 * yet elapsed.
 *
 * @param pdo    a pointer to a Transmit-PDO service.
 * @param id     the node-ID of the MPDO consumer (0 for all nodes, [1..127] for
 *               a specific node).
 * @param idx    the remote object index.
 * @param subidx the remote object sub-index.
 * @param data   the bytes to be transmitted.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_dam_mpdo_event(co_tpdo_t *pdo, co_unsigned8_t id, co_unsigned16_t idx,
		co_unsigned8_t subidx, const co_unsigned8_t data[4]);

/**
 * Triggers the transmission of a DAM-MPDO. This function has no effect if the
 * PDO is not a valid SAM-MPDO. An error is returned if the inhibit time has not
 * yet elapsed.
 *
 * @param pdo    a pointer to a Transmit-PDO service.
 * @param idx    the object index.
 * @param subidx the object sub-index.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_sam_mpdo_event(
		co_tpdo_t *pdo, co_unsigned16_t idx, co_unsigned8_t subidx);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_TPDO_H_
