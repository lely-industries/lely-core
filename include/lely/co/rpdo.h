/*!\file
 * This header file is part of the CANopen library; it contains the Receive-PDO
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

#ifndef LELY_CO_RPDO_H
#define LELY_CO_RPDO_H

#include <lely/can/net.h>
#include <lely/co/pdo.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * The type of a CANopen Receive-PDO indication function, invoked when a PDO is
 * received or an error occurs.
 *
 * \param pdo  a pointer to a Receive-PDO service.
 * \param ac   the SDO abort code: 0 on success, #CO_SDO_AC_NO_OBJ,
 *             #CO_SDO_AC_NO_PDO or #CO_SDO_AC_PDO_LEN in case of a mapping
 *             error, or #CO_SDO_AC_TIMEOUT in case the event timer expires.
 * \param ptr  a pointer to the bytes received.
 * \param n    the number of bytes at \a ptr.
 * \param data a pointer to user-specified data.
 */
typedef void co_rpdo_ind_t(co_rpdo_t *pdo, co_unsigned32_t ac, const void *ptr,
		size_t n, void *data);

LELY_CO_EXTERN void *__co_rpdo_alloc(void);
LELY_CO_EXTERN void __co_rpdo_free(void *ptr);
LELY_CO_EXTERN struct __co_rpdo *__co_rpdo_init(struct __co_rpdo *pdo,
		can_net_t *net, co_dev_t *dev, co_unsigned16_t num);
LELY_CO_EXTERN void __co_rpdo_fini(struct __co_rpdo *pdo);

/*!
 * Creates a new CANopen Receive-PDO service.
 *
 * \param net a pointer to a CAN network.
 * \param dev a pointer to a CANopen device describing the server.
 * \param num the PDO number (in the range [1..512]). The PDO communication and
 *            mapping parameter records MUST exist in the object dictionary of
 *            \a dev.
 *
 * \returns a pointer to a new Receive-PDO service, or NULL on error. In the
 * latter case, the error number can be obtained with `get_errnum()`.
 *
 * \see co_rpdo_destroy()
 */
LELY_CO_EXTERN co_rpdo_t *co_rpdo_create(can_net_t *net, co_dev_t *dev,
		co_unsigned16_t num);

//! Destroys a CANopen Receive-PDO service. \see co_rpdo_create()
LELY_CO_EXTERN void co_rpdo_destroy(co_rpdo_t *pdo);

//! Returns a pointer to the CAN network of a Receive-PDO.
LELY_CO_EXTERN can_net_t *co_rpdo_get_net(const co_rpdo_t *pdo);

//! Returns a pointer to the CANopen device of a Receive-PDO.
LELY_CO_EXTERN co_dev_t *co_rpdo_get_dev(const co_rpdo_t *pdo);

//! Returns the PDO number of a Receive-PDO.
LELY_CO_EXTERN co_unsigned16_t co_rpdo_get_num(const co_rpdo_t *pdo);

/*!
 * Returns a pointer to the PDO communication parameter record of a Receive-PDO.
 */
LELY_CO_EXTERN const struct co_pdo_comm_par *co_rpdo_get_comm_par(
		const co_rpdo_t *pdo);

//! Returns a pointer to the PDO mapping parameter record of a Receive-PDO.
LELY_CO_EXTERN const struct co_pdo_map_par *co_rpdo_get_map_par(
		const co_rpdo_t *pdo);

/*!
 * Retrieves the indication function invoked when a Receive-PDO error occurs.
 *
 * \param pdo   a pointer to a Receive-PDO service.
 * \param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * \param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * \see co_rpdo_set_ind()
 */
LELY_CO_EXTERN void co_rpdo_get_ind(const co_rpdo_t *pdo, co_rpdo_ind_t **pind,
		void **pdata);

/*!
 * Sets the indication function invoked when a Receive-PDO error occurs.
 *
 * \param pdo  a pointer to a Receive-PDO service.
 * \param ind  a pointer to the function to be invoked.
 * \param data a pointer to user-specified data (can be NULL). \a data is
 *             passed as the last parameter to \a ind.
 *
 * \see co_rpdo_get_ind()
 */
LELY_CO_EXTERN void co_rpdo_set_ind(co_rpdo_t *pdo, co_rpdo_ind_t *ind,
		void *data);

/*!
 * Triggers the actuation of a received synchronous PDO.
 *
 * \param pdo a pointer to a Receive-PDO service.
 * \param cnt the counter value (in the range [0..240]).
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_CO_EXTERN int co_rpdo_sync(co_rpdo_t *pdo, co_unsigned8_t cnt);

/*!
 * Requests the transmission of a PDO.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_CO_EXTERN int co_rpdo_rtr(co_rpdo_t *pdo);

#ifdef __cplusplus
}
#endif

#endif

