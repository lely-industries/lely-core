/**@file
 * This is the internal header file of the NMT 'configuration request'
 * declarations.
 *
 * @see lely/co/nmt.h
 *
 * @copyright 2017-2018 Lely Industries N.V.
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

#ifndef LELY_CO_INTERN_NMT_CFG_H_
#define LELY_CO_INTERN_NMT_CFG_H_

#include "co.h"
#include <lely/co/csdo.h>
#include <lely/co/nmt.h>

struct __co_nmt_cfg;
#ifndef __cplusplus
/// An opaque CANopen NMT 'configuration request' type.
typedef struct __co_nmt_cfg co_nmt_cfg_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The CANopen NMT 'update configuration' indication function, invoked when a
 * configuration request is received.
 *
 * @param nmt a pointer to an NMT master service.
 * @param id  the node-ID of the slave (in the range [1..127]).
 * @param sdo a pointer to a Client-SDO connected to the slave.
 */
void co_nmt_cfg_ind(co_nmt_t *nmt, co_unsigned8_t id, co_csdo_t *sdo);

/**
 * The CANopen NMT 'configuration request' confirmation function, invoked when a
 * configuration request completes (with success or failure).
 *
 * @param nmt a pointer to an NMT master service.
 * @param id  the node-ID of the slave (in the range [1..127]).
 * @param ac  the SDO abort code (0 on success).
 */
void co_nmt_cfg_con(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned32_t ac);

void *__co_nmt_cfg_alloc(void);
void __co_nmt_cfg_free(void *ptr);
struct __co_nmt_cfg *__co_nmt_cfg_init(struct __co_nmt_cfg *boot,
		can_net_t *net, co_dev_t *dev, co_nmt_t *nmt);
void __co_nmt_cfg_fini(struct __co_nmt_cfg *boot);

/**
 * Creates a new CANopen NMT 'configuration request'.
 *
 * @param net a pointer to a CAN network.
 * @param dev a pointer to a CANopen device.
 * @param nmt a pointer to an NMT master service.
 *
 * @returns a pointer to a new NMT 'configuration request', or NULL on error. In
 * the latter case, the error number can be obtained with get_errc().
 *
 * @see co_nmt_cfg_destroy()
 */
co_nmt_cfg_t *co_nmt_cfg_create(can_net_t *net, co_dev_t *dev, co_nmt_t *nmt);

/// Destroys a CANopen NMT 'configuration request'. @see co_nmt_cfg_create()
void co_nmt_cfg_destroy(co_nmt_cfg_t *boot);

/**
 * Starts a CANopen NMT 'configuration request'.
 *
 * @param cfg     a pointer to an NMT 'configuration request'.
 * @param id      the node-ID.
 * @param timeout the SDO timeout (in milliseconds). See co_csdo_set_timeout().
 * @param dn_ind  a pointer to the SDO download progress indication function
 *                (can be NULL).
 * @param up_ind  a pointer to the SDO upload progress indication function (can
 *                be NULL).
 * @param data    a pointer to user-specified data (can be NULL). <b>data</b> is
 *                passed as the last parameter to <b>dn_ind</b> and
 *                <b>up_ind</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_nmt_cfg_cfg_req(co_nmt_cfg_t *cfg, co_unsigned8_t id, int timeout,
		co_csdo_ind_t *dn_ind, co_csdo_ind_t *up_ind, void *data);

/**
 * Indicates the result of the 'update configuration' step of an NMT
 * 'configuration request'.
 *
 * @param cfg a pointer to an NMT 'configuration request'.
 * @param ac  the SDO abort code (0 on success).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_nmt_cfg_cfg_res(co_nmt_cfg_t *cfg, co_unsigned32_t ac);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_INTERN_NMT_CFG_H_
