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

#ifdef __cplusplus
}
#endif

#endif

