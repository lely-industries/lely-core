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

/*!
 * The CANopen NMT 'boot slave' confirmation function, invoked when the 'boot
 * slave' process completes.
 *
 * \param nmt a pointer to an NMT master service.
 * \param id  the Node-ID of the slave (in the range [1..127]).
 * \param st  the state of the node (including the toggle bit).
 * \param es  the error status (in the range ['A'..'O'], or 0 on success).
 */
void co_nmt_boot_con(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st,
		char es);

void *__co_nmt_boot_alloc(void);
void __co_nmt_boot_free(void *ptr);
struct __co_nmt_boot *__co_nmt_boot_init(struct __co_nmt_boot *boot,
		can_net_t *net, co_dev_t *dev, co_nmt_t *nmt, co_unsigned8_t id,
		int timeout);
void __co_nmt_boot_fini(struct __co_nmt_boot *boot);

/*!
 * Creates and starts a new CANopen NMT 'boot slave' service.
 *
 * \param net     a pointer to a CAN network.
 * \param dev     a pointer to a CANopen device.
 * \param nmt     a pointer to an NMT master service.
 * \param id      the Node-ID.
 * \param timeout the SDO timeout (in milliseconds).
 *
 * \returns a pointer to a new 'boot slave' service, or NULL on error.
 *
 * \see co_nmt_boot_destroy()
 */
co_nmt_boot_t *co_nmt_boot_create(can_net_t *net, co_dev_t *dev, co_nmt_t *nmt,
		co_unsigned8_t id, int timeout);

//! Destroys a CANopen NMT 'boot slave' service. \see co_nmt_boot_create()
void co_nmt_boot_destroy(co_nmt_boot_t *boot);

#ifdef __cplusplus
}
#endif

#endif

