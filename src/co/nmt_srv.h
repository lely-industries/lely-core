/**@file
 * This is the internal header file of the NMT service manager declarations.
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

#ifndef LELY_CO_INTERN_NMT_SRV_H_
#define LELY_CO_INTERN_NMT_SRV_H_

#include "co.h"
#include <lely/co/nmt.h>

/// A CANopen NMT service manager.
struct co_nmt_srv {
	/// A pointer to the NMT master/slave service.
	co_nmt_t *nmt;
	/**
	 * The currently enabled CANopen services (any combination of
	 * #CO_NMT_SRV_PDO, #CO_NMT_SRV_SDO, #CO_NMT_SRV_SYNC, #CO_NMT_SRV_TIME
	 * and #CO_NMT_SRV_EMCY).
	 */
	int set;
#if !LELY_NO_CO_RPDO
	/// An array of pointers to the Receive-PDO services.
	co_rpdo_t **rpdos;
	/// The number of Receive-PDO services.
	co_unsigned16_t nrpdo;
#endif
#if !LELY_NO_CO_TPDO
	/// An array of pointers to the Transmit-PDO services.
	co_tpdo_t **tpdos;
	/// The number of Transmit-PDO services.
	co_unsigned16_t ntpdo;
#endif
	/// An array of pointers to the Server-SDO services.
	co_ssdo_t **ssdos;
	/// The number of Server-SDO services.
	co_unsigned8_t nssdo;
#if !LELY_NO_CO_CSDO
	/// An array of pointers to the Client-SDO services.
	co_csdo_t **csdos;
	/// The number of Client-SDO services.
	co_unsigned8_t ncsdo;
#endif
#if !LELY_NO_CO_SYNC
	/// A pointer to the SYNC producer/consumer service.
	co_sync_t *sync;
#endif
#if !LELY_NO_CO_TIME
	/// A pointer to the TIME producer/consumer service.
	co_time_t *time;
#endif
#if !LELY_NO_CO_EMCY
	/// A pointer to the EMCY producer/consumer service.
	co_emcy_t *emcy;
#endif
#if !LELY_NO_CO_LSS
	/// A pointer to the LSS master/slave service.
	co_lss_t *lss;
#endif
};

/// The Receive/Transmit-PDO services.
#define CO_NMT_SRV_PDO 0x01

/// The Server/Client-SDO services.
#define CO_NMT_SRV_SDO 0x02

/// The SYNC producer/consumer service
#define CO_NMT_SRV_SYNC 0x04

/// The TIME producer/consumer service.
#define CO_NMT_SRV_TIME 0x08

/// The EMCY producer/consumer service.
#define CO_NMT_SRV_EMCY 0x10

/// The LSS master/slave service.
#define CO_NMT_SRV_LSS 0x20

#ifdef __cplusplus
extern "C" {
#endif

/// Initializes a CANopen NMT service manager. @see co_nmt_srv_fini()
void co_nmt_srv_init(struct co_nmt_srv *srv, co_nmt_t *nmt);

/// Finalizes a CANopen NMT service manager. @see co_nmt_srv_init()
void co_nmt_srv_fini(struct co_nmt_srv *srv);

/**
 * Enables/disables the specified CANopen services.
 *
 * @param srv a pointer to a CANopen NMT service manager.
 * @param nmt a pointer to an NMT master/slave service.
 * @param set the services to be enabled (any combination of #CO_NMT_SRV_PDO,
 *            #CO_NMT_SRV_SDO, #CO_NMT_SRV_SYNC, #CO_NMT_SRV_TIME and
 *            #CO_NMT_SRV_EMCY). Services not part of <b>set</b> will be
 *            disabled.
 */
void co_nmt_srv_set(struct co_nmt_srv *srv, co_nmt_t *nmt, int set);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_INTERN_NMT_SRV_H_
