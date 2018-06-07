/*!\file
 * This header file is part of the CANopen library; it contains the Transmit-PDO
 * declarations.
 *
 * \copyright 2018 Lely Industries N.V.
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

#ifndef LELY_CO_TPDO_H
#define LELY_CO_TPDO_H

#include <lely/can/net.h>
#include <lely/co/pdo.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * The type of a CANopen Transmit-PDO indication function, invoked when a PDO is
 * sent or an error occurs.
 *
 * \param pdo  a pointer to a Transmit-PDO service.
 * \param ac   the SDO abort code: 0 on success, #CO_SDO_AC_NO_OBJ,
 *             #CO_SDO_AC_NO_PDO, CO_SDO_AC_NO_READ or #CO_SDO_AC_PDO_LEN in
 *             case of a mapping error, or #CO_SDO_AC_TIMEOUT in case the
 *             synchronous time window expires.
 * \param ptr  a pointer to the bytes sent.
 * \param n    the number of bytes at \a ptr.
 * \param data a pointer to user-specified data.
 */
typedef void co_tpdo_ind_t(co_tpdo_t *pdo, co_unsigned32_t ac, const void *ptr,
		size_t n, void *data);

LELY_CO_EXTERN void *__co_tpdo_alloc(void);
LELY_CO_EXTERN void __co_tpdo_free(void *ptr);
LELY_CO_EXTERN struct __co_tpdo *__co_tpdo_init(struct __co_tpdo *pdo,
		can_net_t *net, co_dev_t *dev, co_unsigned16_t num);
LELY_CO_EXTERN void __co_tpdo_fini(struct __co_tpdo *pdo);

/*!
 * Creates a new CANopen Transmit-PDO service.
 *
 * \param net a pointer to a CAN network.
 * \param dev a pointer to a CANopen device describing the server.
 * \param num the PDO number (in the range [1..512]). The PDO communication and
 *            mapping parameter records MUST exist in the object dictionary of
 *            \a dev.
 *
 * \returns a pointer to a new Transmit-PDO service, or NULL on error. In the
 * latter case, the error number can be obtained with `get_errnum()`.
 *
 * \see co_tpdo_destroy()
 */
LELY_CO_EXTERN co_tpdo_t *co_tpdo_create(can_net_t *net, co_dev_t *dev,
		co_unsigned16_t num);

//! Destroys a CANopen Transmit-PDO service. \see co_tpdo_create()
LELY_CO_EXTERN void co_tpdo_destroy(co_tpdo_t *pdo);

//! Returns a pointer to the CAN network of a Transmit-PDO.
LELY_CO_EXTERN can_net_t *co_tpdo_get_net(const co_tpdo_t *pdo);

//! Returns a pointer to the CANopen device of a Transmit-PDO.
LELY_CO_EXTERN co_dev_t *co_tpdo_get_dev(const co_tpdo_t *pdo);

//! Returns the PDO number of a Transmit-PDO.
LELY_CO_EXTERN co_unsigned16_t co_tpdo_get_num(const co_tpdo_t *pdo);

/*!
 * Returns a pointer to the PDO communication parameter record of a
 * Transmit-PDO.
 */
LELY_CO_EXTERN const struct co_pdo_comm_par *co_tpdo_get_comm_par(
		const co_tpdo_t *pdo);

//! Returns a pointer to the PDO mapping parameter record of a Transmit-PDO.
LELY_CO_EXTERN const struct co_pdo_map_par *co_tpdo_get_map_par(
		const co_tpdo_t *pdo);

/*!
 * Retrieves the indication function invoked when a Transmit-PDO error occurs.
 *
 * \param pdo   a pointer to a Transmit-PDO service.
 * \param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * \param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * \see co_tpdo_set_ind()
 */
LELY_CO_EXTERN void co_tpdo_get_ind(const co_tpdo_t *pdo, co_tpdo_ind_t **pind,
		void **pdata);

/*!
 * Sets the indication function invoked when a Transmit-PDO error occurs.
 *
 * \param pdo  a pointer to a Transmit-PDO service.
 * \param ind  a pointer to the function to be invoked.
 * \param data a pointer to user-specified data (can be NULL). \a data is
 *             passed as the last parameter to \a ind.
 *
 * \see co_tpdo_get_ind()
 */
LELY_CO_EXTERN void co_tpdo_set_ind(co_tpdo_t *pdo, co_tpdo_ind_t *ind,
		void *data);

/*!
 * Triggers the transmission of an event-driven (asynchronous) PDO. This
 * function returns an error if the inhibit time has not yet elapsed.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_tpdo_sync()
 */
LELY_CO_EXTERN int co_tpdo_event(co_tpdo_t *pdo);

/*!
 * Triggers the transmission of a synchronous PDO.
 *
 * \param pdo a pointer to a Transmit-PDO service.
 * \param cnt the counter value (in the range [0..240]).
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see co_tpdo_event()
 */
LELY_CO_EXTERN int co_tpdo_sync(co_tpdo_t *pdo, co_unsigned8_t cnt);

//! Retrieves the time at which the next event-driven TPDO may be sent.
LELY_CO_EXTERN void co_tpdo_get_next(const co_tpdo_t *pdo, struct timespec *tp);

#ifdef __cplusplus
}
#endif

#endif
