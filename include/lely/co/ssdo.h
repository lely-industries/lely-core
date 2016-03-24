/*!\file
 * This header file is part of the CANopen library; it contains the Server-SDO
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

#ifndef LELY_CO_SSDO_H
#define LELY_CO_SSDO_H

#include <lely/can/net.h>
#include <lely/co/sdo.h>

#ifdef __cplusplus
extern "C" {
#endif

LELY_CO_EXTERN void *__co_ssdo_alloc(void);
LELY_CO_EXTERN void __co_ssdo_free(void *ptr);
LELY_CO_EXTERN struct __co_ssdo *__co_ssdo_init(struct __co_ssdo *sdo,
		can_net_t *net, co_dev_t *dev, co_unsigned8_t num);
LELY_CO_EXTERN void __co_ssdo_fini(struct __co_ssdo *sdo);

/*!
 * Creates a new CANopen Server-SDO service.
 *
 * \param net a pointer to a CAN network.
 * \param dev a pointer to a CANopen device describing the server.
 * \param num the SDO number (in the range [1..128]). Except when \a num is 1,
 *            the SDO parameter record MUST exist in the object dictionary of
 *            \a dev.
 *
 * \returns a pointer to a new Server-SDO service, or NULL on error. In the
 * latter case, the error number can be obtained with `get_errnum()`.
 *
 * \see co_ssdo_destroy()
 */
LELY_CO_EXTERN co_ssdo_t *co_ssdo_create(can_net_t *net, co_dev_t *dev,
		co_unsigned8_t num);

//! Destroys a CANopen Server-SDO service. \see co_ssdo_create()
LELY_CO_EXTERN void co_ssdo_destroy(co_ssdo_t *sdo);

//! Returns the SDO number of a Server-SDO.
LELY_CO_EXTERN co_unsigned8_t co_ssdo_get_num(const co_ssdo_t *sdo);

//! Returns a pointer to the SDO parameter record of a Server-SDO.
LELY_CO_EXTERN const struct co_sdo_par *co_ssdo_get_par(const co_ssdo_t *sdo);

/*!
 * Returns the timeout (in milliseconds) of a Server-SDO. A return value of 0
 * (the default) means no timeout is being used.
 *
 * \see co_ssdo_set_timeout()
 */
LELY_CO_EXTERN int co_ssdo_get_timeout(const co_ssdo_t *sdo);

/*!
 * Sets the timeout of a Server-SDO. If the timeout expires before receiving a
 * request from a client, the server aborts the transfer.
 *
 * \param sdo     a pointer to a Server-SDO service.
 * \param timeout the timeout (in milliseconds). A value of 0 (the default)
 *                disables the timeout.
 *
 * \see co_ssdo_get_timeout()
 */
LELY_CO_EXTERN void co_ssdo_set_timeout(co_ssdo_t *sdo, int timeout);

#ifdef __cplusplus
}
#endif

#endif

