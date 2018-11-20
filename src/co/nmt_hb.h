/**@file
 * This is the internal header file of the NMT heartbeat consumer declarations.
 *
 * @see lely/co/nmt.h
 *
 * @copyright 2016-2018 Lely Industries N.V.
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

#ifndef LELY_CO_INTERN_NMT_HB_H_
#define LELY_CO_INTERN_NMT_HB_H_

#include "co.h"
#include <lely/co/nmt.h>

struct __co_nmt_hb;
#ifndef __cplusplus
/// An opaque CANopen NMT heartbeat consumer type.
typedef struct __co_nmt_hb co_nmt_hb_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The CANopen NMT heartbeat indication function, invoked when a heartbeat event
 * occurs.
 *
 * @param nmt    a pointer to an NMT master/slave service.
 * @param id     the node-ID (in the range [1..127]).
 * @param state  indicates whether the event occurred (#CO_NMT_EC_OCCURRED) or
 *               was resolved (#CO_NMT_EC_RESOLVED). Note that heartbeat state
 *               change events only occur and are never resolved.
 * @param reason indicates whether the event occurred because of a timeout
 *               (#CO_NMT_EC_TIMEOUT) or a state change (#CO_NMT_EC_STATE).
 * @param st     the state of the node (if <b>reason</b> is #CO_NMT_EC_STATE).
 */
void co_nmt_hb_ind(co_nmt_t *nmt, co_unsigned8_t id, int state, int reason,
		co_unsigned8_t st);

void *__co_nmt_hb_alloc(void);
void __co_nmt_hb_free(void *ptr);
struct __co_nmt_hb *__co_nmt_hb_init(
		struct __co_nmt_hb *hb, can_net_t *net, co_nmt_t *nmt);
void __co_nmt_hb_fini(struct __co_nmt_hb *hb);

/**
 * Creates a new CANopen NMT heartbeat consumer service.
 *
 * @param net a pointer to a CAN network.
 * @param nmt a pointer to an NMT master/slave service.
 *
 * @returns a pointer to a new heartbeat consumer service, or NULL on error. In
 * the latter case, the error number can be obtained with get_errc().
 *
 * @see co_nmt_hb_destroy()
 */
co_nmt_hb_t *co_nmt_hb_create(can_net_t *net, co_nmt_t *nmt);

/// Destroys a CANopen NMT heartbeat consumer service. @see co_nmt_hb_create()
void co_nmt_hb_destroy(co_nmt_hb_t *hb);

/**
 * Processes the value of CANopen object 1016 (Consumer heartbeat time) for the
 * specified heartbeat consumer. If the node-ID is valid and the heartbeat time
 * is non-zero, the heartbeat consumer is activated. Note that this only
 * activates the CAN frame receiver for heartbeat messages. The CAN timer for
 * heartbeat events is not activated until the first heartbeat message is
 * received or co_nmt_hb_set_st() is invoked.
 *
 * @param hb a pointer to a heartbeat consumer service.
 * @param id the node-ID.
 * @param ms the heartbeat time (in milliseconds).
 */
void co_nmt_hb_set_1016(co_nmt_hb_t *hb, co_unsigned8_t id, co_unsigned16_t ms);

/**
 * Sets the expected state of a remote NMT node. If the heartbeat consumer is
 * active, invocation of this function is equivalent to reception of a heartbeat
 * message with the specified state and will (re)activate the CAN timer for
 * heartbeat events.
 *
 * @param hb a pointer to a heartbeat consumer service.
 * @param st the state of the node (excluding the toggle bit).
 */
void co_nmt_hb_set_st(co_nmt_hb_t *hb, co_unsigned8_t st);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_INTERN_NMT_HB_H_
