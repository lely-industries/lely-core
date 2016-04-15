/*!\file
 * This header file is part of the CANopen library; it contains the Layer
 * Setting Services (LSS) and protocols declarations.
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

#ifndef LELY_CO_LSS_H
#define LELY_CO_LSS_H

#include <lely/can/net.h>
#include <lely/co/type.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * The type of a CANopen LSS 'activate bit timing' indication function, invoked
 * when a baudrate switch is requested.
 *
 * \param lss   a pointer to an LSS slave service.
 * \param rate  the new baudrate (in kbit/s), or 0 for automatic bit rate
 *              detection.
 * \param delay the delay (in ms) before the switch and the delay after the
 *              switch during which CAN frames MUST NOT be sent.
 * \param data  a pointer to user-specified data.
 */
typedef void co_lss_rate_ind_t(co_lss_t *lss, co_unsigned16_t rate, int delay,
		void *data);

/*!
 * The type of a CANopen LSS 'store configuration' indication function, invoked
 * when the pending node-ID and baudrate should be copied to the persistent
 * node-ID and baudrate.
 *
 * \param lss  a pointer to an LSS slave service.
 * \param id   the node-ID.
 * \param rate the new baudrate (in kbit/s), or 0 for automatic bit rate
 *             detection.
 * \param data a pointer to user-specified data.
 *
 * \returns 0 on success, or -1 on error.
 */
typedef int co_lss_store_ind_t(co_lss_t *lss, co_unsigned8_t id,
		co_unsigned16_t rate, void *data);

LELY_CO_EXTERN void *__co_lss_alloc(void);
LELY_CO_EXTERN void __co_lss_free(void *ptr);
LELY_CO_EXTERN struct __co_lss *__co_lss_init(struct __co_lss *lss,
		can_net_t *net, co_dev_t *dev, co_nmt_t *nmt);
LELY_CO_EXTERN void __co_lss_fini(struct __co_lss *lss);

/*!
 * Creates a new CANopen LSS master/slave service.
 *
 * \param net a pointer to a CAN network.
 * \param dev a pointer to a CANopen device.
 * \param nmt a pointer to an NMT master/slave service.
 *
 * \returns a pointer to a new LSS service, or NULL on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 *
 * \see co_lss_destroy()
 */
LELY_CO_EXTERN co_lss_t *co_lss_create(can_net_t *net, co_dev_t *dev,
		co_nmt_t *nmt);

//! Destroys a CANopen LSS master/slave service. \see co_lss_create()
LELY_CO_EXTERN void co_lss_destroy(co_lss_t *lss);

/*!
 * Retrieves the indication function invoked when an LSS 'activate bit timing'
 * request is received.
 *
 * \param lss   a pointer to an LSS slave service.
 * \param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * \param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * \see co_lss_set_rate_ind()
 */
LELY_CO_EXTERN void co_lss_get_rate_ind(const co_lss_t *lss,
		co_lss_rate_ind_t **pind, void **pdata);

/*!
 * Sets the indication function invoked when an LSS 'activate bit timing'
 * request is received.
 *
 * \param lss  a pointer to an LSS slave service.
 * \param ind  a pointer to the function to be invoked.
 * \param data a pointer to user-specified data (can be NULL). \a data is
 *             passed as the last parameter to \a ind.
 *
 * \see co_lss_get_rate_ind()
 */
LELY_CO_EXTERN void co_lss_set_rate_ind(co_lss_t *lss, co_lss_rate_ind_t *ind,
		void *data);

/*!
 * Retrieves the indication function invoked when an LSS 'store configuration'
 * request is received.
 *
 * \param lss   a pointer to an LSS slave service.
 * \param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * \param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * \see co_lss_set_store_ind()
 */
LELY_CO_EXTERN void co_lss_get_store_ind(const co_lss_t *lss,
		co_lss_store_ind_t **pind, void **pdata);

/*!
 * Sets the indication function invoked when an LSS 'store configuration'
 * request is received.
 *
 * \param lss  a pointer to an LSS slave service.
 * \param ind  a pointer to the function to be invoked.
 * \param data a pointer to user-specified data (can be NULL). \a data is
 *             passed as the last parameter to \a ind.
 *
 * \see co_lss_get_store_ind()
 */
LELY_CO_EXTERN void co_lss_set_store_ind(co_lss_t *lss, co_lss_store_ind_t *ind,
		void *data);

//! Returns 1 if the specified CANopen LSS service is a master, and 0 if not.
LELY_CO_EXTERN int co_lss_is_master(const co_lss_t *lss);

#ifdef __cplusplus
}
#endif

#endif

