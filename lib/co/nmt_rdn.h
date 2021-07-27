/**@file
 * This is the internal header file of the NMT redundancy manager
 * declarations.
 *
 * @see lely/co/nmt.h
 *
 * @copyright 2021 N7 Space Sp. z o.o.
 *
 * The NMT redundancy manager was developed under a programme of,
 * and funded by, the European Space Agency.
 *
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

#ifndef LELY_CO_INTERN_NMT_RDN_H_
#define LELY_CO_INTERN_NMT_RDN_H_

#include "co.h"
#include <lely/co/nmt.h>

#include <stdbool.h>

struct co_nmt_rdn;
/// An opaque CANopen NMT redundancy manager type.
typedef struct co_nmt_rdn co_nmt_rdn_t;

/// The Redundancy Master Heartbeat Consumer index
#ifndef CO_NMT_RDN_MASTER_HB_IDX
#define CO_NMT_RDN_MASTER_HB_IDX 1u
#endif

/// The index of the Redundancy Object
#ifndef CO_NMT_RDN_REDUNDANCY_OBJ_IDX
#define CO_NMT_RDN_REDUNDANCY_OBJ_IDX 0x2000u
#endif

/// The maximum number of the sub-indices in the Redundancy Object
#define CO_NMT_REDUNDANCY_OBJ_MAX_IDX 4u

/// The Redundancy Object's sub-indices
#define CO_NMT_RDN_BDEFAULT_SUBIDX 0x01u
#define CO_NMT_RDN_TTOGGLE_SUBIDX 0x02u
#define CO_NMT_RDN_NTOGGLE_SUBIDX 0x03u
#define CO_NMT_RDN_CTOGGLE_SUBIDX 0x04u

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The CANopen NMT redundancy indication function, invoked when a redundancy
 * event occurs.
 *
 * @param nmt a pointer to an NMT master/slave service.
 */
void co_nmt_ecss_rdn_ind(co_nmt_t *nmt, co_unsigned8_t bus_id, int reason);

/**
 * Checks if the structure of the Redundancy object conforms to ECSS.
 *
 * @param dev a pointer to a CANopen device.
 *
 * @returns <b>true</b> if the Redundancy Object is conformant (or not
 *           present), and <b>false</b> if not.
 */
bool co_nmt_rdn_chk_dev(const co_dev_t *dev);

/// Returns the alignment (in bytes) of the #co_nmt_rdn_t structure.
size_t co_nmt_rdn_alignof(void);

/// Returns the size (in bytes) of the #co_nmt_rdn_t structure.
size_t co_nmt_rdn_sizeof(void);

/**
 * Creates a new CANopen NMT redundancy management service.
 *
 * @param net a pointer to a CAN network.
 * @param nmt a pointer to an NMT master/slave service.
 *
 * @returns a pointer to a new redundancy manger service, or NULL on error. In
 * the latter case, the error number can be obtained with get_errc().
 *
 * @see co_nmt_rdn_destroy()
 */
co_nmt_rdn_t *co_nmt_rdn_create(can_net_t *net, co_nmt_t *nmt);

/// Destroys a CANopen NMT heartbeat consumer service. @see co_nmt_rdn_create()
void co_nmt_rdn_destroy(co_nmt_rdn_t *rdn);

/**
 * Returns a pointer to the allocator used to allocate the NMT redundancy
 * manager service.
 *
 * @see can_net_get_alloc()
 */
alloc_t *co_nmt_rdn_get_alloc(const co_nmt_rdn_t *rdn);

/**
 * Sets the alternate CAN bus identifier which the NMT redundancy manager
 * service could utilize.
 *
 * @param rdn    a pointer to an NMT redundancy manager service.
 * @param bus_id a CAN bus identifier.
 */
void co_nmt_rdn_set_alternate_bus_id(co_nmt_rdn_t *rdn, co_unsigned8_t bus_id);

/**
 * Sets the Redundancy Master's Node-ID.
 *
 * @param rdn a pointer to an NMT redundancy manager service.
 * @param id  the Redundancy Master's Node-ID (in the range [1..127]).
 * @param ms  the Redundancy Master's heartbeat time (in milliseconds).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int co_nmt_rdn_set_master_id(
		co_nmt_rdn_t *rdn, co_unsigned8_t id, co_unsigned16_t ms);

/// Returns the Redundancy Master node-ID.
co_unsigned8_t co_nmt_rdn_get_master_id(const co_nmt_rdn_t *rdn);

/// Selects the default bus (Bdefault) as active. For slave nodes starts bus
/// toggle timer.
void co_nmt_rdn_select_default_bus(co_nmt_rdn_t *rdn);

/// Sets currently active bus as default (Bdefault) and reset counters.
void co_nmt_rdn_set_active_bus_default(co_nmt_rdn_t *rdn);

/// Registers a missed Redundancy Master heartbeat message event.
void co_nmt_rdn_slave_missed_hb(co_nmt_rdn_t *rdn);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_INTERN_NMT_RDN_H_
