/**@file
 * This header file is part of the CANopen library; it contains the Receive-PDO
 * declarations.
 *
 * @copyright 2020 Lely Industries N.V.
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

#ifndef LELY_CO_RPDO_H_
#define LELY_CO_RPDO_H_

#include <lely/can/net.h>
#include <lely/co/pdo.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a CANopen Receive-PDO indication function, invoked when a PDO is
 * received or an error occurs.
 *
 * @param pdo  a pointer to a Receive-PDO service.
 * @param ac   the SDO abort code: 0 on success, or #CO_SDO_AC_NO_OBJ,
 *             #CO_SDO_AC_NO_SUB, #CO_SDO_AC_NO_WRITE, #CO_SDO_AC_NO_PDO or
 *             #CO_SDO_AC_PDO_LEN on error.
 * @param ptr  a pointer to the bytes received.
 * @param n    the number of bytes at <b>ptr</b>.
 * @param data a pointer to user-specified data.
 */
typedef void co_rpdo_ind_t(co_rpdo_t *pdo, co_unsigned32_t ac, const void *ptr,
		size_t n, void *data);

/**
 * The type of a CANopen Receive-PDO error handling function, invoked in case of
 * a timeout or length mismatch.
 *
 * @param pdo  a pointer to a Receive-PDO service.
 * @param eec  the emergency error code (0x8210, 0x8220 or 0x8250).
 * @param er   the error register (0x10).
 * @param data a pointer to user-specified data.
 */
typedef void co_rpdo_err_t(co_rpdo_t *pdo, co_unsigned16_t eec,
		co_unsigned8_t er, void *data);

void *__co_rpdo_alloc(void);
void __co_rpdo_free(void *ptr);
struct __co_rpdo *__co_rpdo_init(struct __co_rpdo *pdo, can_net_t *net,
		co_dev_t *dev, co_unsigned16_t num);
void __co_rpdo_fini(struct __co_rpdo *pdo);

/**
 * Creates a new CANopen Receive-PDO service. The service is started as if by
 * co_rpdo_start().
 *
 * @param net a pointer to a CAN network.
 * @param dev a pointer to a CANopen device describing the server.
 * @param num the PDO number (in the range [1..512]). The PDO communication and
 *            mapping parameter records MUST exist in the object dictionary of
 *            <b>dev</b>.
 *
 * @returns a pointer to a new Receive-PDO service, or NULL on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see co_rpdo_destroy()
 */
co_rpdo_t *co_rpdo_create(can_net_t *net, co_dev_t *dev, co_unsigned16_t num);

/// Destroys a CANopen Receive-PDO service. @see co_rpdo_create()
void co_rpdo_destroy(co_rpdo_t *pdo);

/**
 * Starts a Receive-PDO service.
 *
 * @post on success, co_rpdo_is_stopped() returns 0.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_rpdo_stop()
 */
int co_rpdo_start(co_rpdo_t *pdo);

/**
 * Stops a Receive-PDO service.
 *
 * @post co_rpdo_is_stopped() returns 1.
 *
 * @see co_rpdo_start()
 */
void co_rpdo_stop(co_rpdo_t *pdo);

/**
 * Retuns 1 if the specified Receive-PDO service is stopped, and 0 if not.
 *
 * @see co_rpdo_start, co_rpdo_stop()
 */
int co_rpdo_is_stopped(const co_rpdo_t *pdo);

/// Returns a pointer to the CAN network of a Receive-PDO.
can_net_t *co_rpdo_get_net(const co_rpdo_t *pdo);

/// Returns a pointer to the CANopen device of a Receive-PDO.
co_dev_t *co_rpdo_get_dev(const co_rpdo_t *pdo);

/// Returns the PDO number of a Receive-PDO.
co_unsigned16_t co_rpdo_get_num(const co_rpdo_t *pdo);

/**
 * Returns a pointer to the PDO communication parameter record of a Receive-PDO.
 */
const struct co_pdo_comm_par *co_rpdo_get_comm_par(const co_rpdo_t *pdo);

/// Returns a pointer to the PDO mapping parameter record of a Receive-PDO.
const struct co_pdo_map_par *co_rpdo_get_map_par(const co_rpdo_t *pdo);

/**
 * Retrieves the indication function invoked when a Receive-PDO error occurs.
 *
 * @param pdo   a pointer to a Receive-PDO service.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_rpdo_set_ind()
 */
void co_rpdo_get_ind(const co_rpdo_t *pdo, co_rpdo_ind_t **pind, void **pdata);

/**
 * Sets the indication function invoked when a Receive-PDO error occurs.
 *
 * @param pdo  a pointer to a Receive-PDO service.
 * @param ind  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>ind</b>.
 *
 * @see co_rpdo_get_ind()
 */
void co_rpdo_set_ind(co_rpdo_t *pdo, co_rpdo_ind_t *ind, void *data);

/**
 * Retrieves the error handling function of a Receive-PDO service.
 *
 * @param pdo   a pointer to a Receive-PDO service.
 * @param perr  the address at which to store a pointer to the error handling
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_rpdo_set_err()
 */
void co_rpdo_get_err(const co_rpdo_t *pdo, co_rpdo_err_t **perr, void **pdata);

/**
 * Sets the error handling function of a Receive-PDO service.
 *
 * @param pdo  a pointer to a Receive-PDO service.
 * @param err  a pointer to the function to be invoked.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>err</b>.
 *
 * @see co_rpdo_get_err()
 */
void co_rpdo_set_err(co_rpdo_t *pdo, co_rpdo_err_t *err, void *data);

/**
 * Triggers the actuation of a received synchronous PDO.
 *
 * @param pdo a pointer to a Receive-PDO service.
 * @param cnt the counter value (in the range [0..240]).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_rpdo_sync(co_rpdo_t *pdo, co_unsigned8_t cnt);

/**
 * Requests the transmission of a PDO.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_rpdo_rtr(co_rpdo_t *pdo);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_RPDO_H_
