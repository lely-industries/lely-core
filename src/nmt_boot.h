/*!\file
 * This is the internal header file of the NMT 'boot slave' declarations.
 *
 * \see lely/co/nmt.h
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

#ifndef LELY_CO_INTERN_NMT_BOOT_H
#define LELY_CO_INTERN_NMT_BOOT_H

#include "co.h"
#include <lely/co/nmt.h>

struct __co_nmt_boot;
#ifndef __cplusplus
//! An opaque CANopen NMT 'boot slave' service type.
typedef struct __co_nmt_boot co_nmt_boot_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

void *__co_nmt_boot_alloc(void);
void __co_nmt_boot_free(void *ptr);
struct __co_nmt_boot *__co_nmt_boot_init(struct __co_nmt_boot *boot,
		can_net_t *net, co_dev_t *dev, co_nmt_t *nmt);
void __co_nmt_boot_fini(struct __co_nmt_boot *boot);

/*!
 * Creates a new CANopen NMT 'boot slave' service.
 *
 * \param net a pointer to a CAN network.
 * \param dev a pointer to a CANopen device.
 * \param nmt a pointer to an NMT master service.
 *
 * \returns a pointer to a new 'boot slave' service, or NULL on error.
 *
 * \see co_nmt_boot_destroy()
 */
co_nmt_boot_t *co_nmt_boot_create(can_net_t *net, co_dev_t *dev, co_nmt_t *nmt);

//! Destroys a CANopen NMT 'boot slave' service. \see co_nmt_boot_create()
void co_nmt_boot_destroy(co_nmt_boot_t *boot);

/*!
 * Retrieves the indication function invoked when the NMT 'boot slave' process
 * reaches the 'download software' step.
 *
 * \param boot a pointer to a 'boot slave' service.
 * \param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * \param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * \see co_nmt_boot_set_dn_sw_ind()
 */
void co_nmt_boot_get_dn_sw_ind(co_nmt_boot_t *boot, co_nmt_req_ind_t **pind,
		void **pdata);

/*!
 * Sets the indication function invoked when the NMT 'boot slave' process
 * reaches the 'download software' step.
 *
 * \param boot a pointer to a 'boot slave' service.
 * \param ind  a pointer to the function to be invoked.
 * \param data a pointer to user-specified data (can be NULL). \a data is
 *             passed as the last parameter to \a ind.
 *
 * \see co_nmt_boot_get_dn_sw_ind()
 */
void co_nmt_boot_set_dn_sw_ind(co_nmt_boot_t *boot, co_nmt_req_ind_t *ind,
		void *data);

/*!
 * Retrieves the indication function invoked when the NMT 'boot slave' process
 * reaches the 'download configuration' step.
 *
 * \param boot a pointer to a 'boot slave' service.
 * \param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * \param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * \see co_nmt_boot_set_dn_cfg_ind()
 */
void co_nmt_boot_get_dn_cfg_ind(co_nmt_boot_t *boot, co_nmt_req_ind_t **pind,
		void **pdata);

/*!
 * Sets the indication function invoked when the NMT 'boot slave' process
 * reaches the 'download configuration' step.
 *
 * \param boot a pointer to a 'boot slave' service.
 * \param ind  a pointer to the function to be invoked.
 * \param data a pointer to user-specified data (can be NULL). \a data is
 *             passed as the last parameter to \a ind.
 *
 * \see co_nmt_boot_get_dn_cfg_ind()
 */
void co_nmt_boot_set_dn_cfg_ind(co_nmt_boot_t *boot, co_nmt_req_ind_t *ind,
		void *data);

/*!
 * Starts the NMT 'boot slave' process.
 *
 * \param boot    a pointer to a 'boot slave' service.
 * \param id      the Node-ID.
 * \param timeout the SDO timeout (in milliseconds).
 * \param con     a pointer to the confirmation function (can be NULL).
 * \param data    a pointer to user-specified data (can be NULL). \a data is
 *                passed as the last parameter to \a con.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
int co_nmt_boot_boot_req(co_nmt_boot_t *boot, co_unsigned8_t id, int timeout,
		co_nmt_boot_ind_t *con, void *data);

/*!
 * Indicates the result of a user-implemented step requested by the NMT 'boot
 * slave' process.
 *
 * \param boot a pointer to a 'boot slave' service.
 * \param res  the result of the request. A non-zero value is interpreted as an
 *             error.
 */
void co_nmt_boot_req_res(co_nmt_boot_t *boot, int res);

#ifdef __cplusplus
}
#endif

#endif

