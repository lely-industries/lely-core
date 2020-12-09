/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the NMT service manager functions.
 *
 * @see lib/co/nmt_srv.h
 *
 * @copyright 2017-2020 Lely Industries N.V.
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

#include "co.h"
#include <lely/util/diag.h>
#if !LELY_NO_CO_CSDO
#include <lely/co/csdo.h>
#endif
#include <lely/co/dev.h>
#if !LELY_NO_CO_EMCY
#include <lely/co/emcy.h>
#endif
#if !LELY_NO_CO_LSS
#include <lely/co/lss.h>
#endif
#if !LELY_NO_CO_RPDO
#include <lely/co/rpdo.h>
#endif
#include <lely/co/ssdo.h>
#if !LELY_NO_CO_SYNC
#include <lely/co/sync.h>
#endif
#if !LELY_NO_CO_TIME
#include <lely/co/time.h>
#endif
#if !LELY_NO_CO_TPDO
#include <lely/co/tpdo.h>
#endif
#include "nmt_srv.h"

#include <assert.h>

#if !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO
/// Initializes all Receive/Transmit-PDO services. @see co_nmt_srv_fini_pdo()
static int co_nmt_srv_init_pdo(struct co_nmt_srv *srv);
/// Finalizes all Receive/Transmit-PDO services. @see co_nmt_srv_init_pdo()
static void co_nmt_srv_fini_pdo(struct co_nmt_srv *srv);
/// Starts all Receive/Transmit-PDO services. @see co_nmt_srv_stop_pdo()
static int co_nmt_srv_start_pdo(struct co_nmt_srv *srv);
/// Stops all Receive/Transmit-PDO services. @see co_nmt_srv_start_pdo()
static void co_nmt_srv_stop_pdo(struct co_nmt_srv *srv);
#if !LELY_NO_CO_RPDO
/// Invokes co_nmt_err() to handle Receive-PDO errors. @see co_rpdo_err_t
static void co_nmt_srv_rpdo_err(co_rpdo_t *pdo, co_unsigned16_t eec,
		co_unsigned8_t er, void *data);
#endif
#endif

/// Initializes all Server/Client-SDO services. @see co_nmt_srv_fini_sdo()
static int co_nmt_srv_init_sdo(struct co_nmt_srv *srv);
/// Finalizes all Server/Client-SDO services. @see co_nmt_srv_init_sdo()
static void co_nmt_srv_fini_sdo(struct co_nmt_srv *srv);
/// Initializes all Server/Client-SDO services. @see co_nmt_srv_stop_sdo()
static int co_nmt_srv_start_sdo(struct co_nmt_srv *srv);
/// Stop all Server/Client-SDO services. @see co_nmt_srv_start_sdo()
static void co_nmt_srv_stop_sdo(struct co_nmt_srv *srv);

#if !LELY_NO_CO_SYNC
/// Initializes the SYNC producer/consumer service. @see co_nmt_srv_fini_sync()
static int co_nmt_srv_init_sync(struct co_nmt_srv *srv);
/// Finalizes the SYNC producer/consumer service. @see co_nmt_srv_init_sync()
static void co_nmt_srv_fini_sync(struct co_nmt_srv *srv);
/// Starts the SYNC producer/consumer service. @see co_nmt_srv_stop_sync()
static int co_nmt_srv_start_sync(struct co_nmt_srv *srv);
/// Stops the SYNC producer/consumer service. @see co_nmt_srv_start_sync()
static void co_nmt_srv_stop_sync(struct co_nmt_srv *srv);
/// Invokes co_nmt_sync() to handle SYNC objects. @see co_sync_ind_t
static void co_nmt_srv_sync_ind(
		co_sync_t *sync, co_unsigned8_t cnt, void *data);
/// Invokes co_nmt_err() to handle SYNC errors. @see co_sync_err_t
static void co_nmt_srv_sync_err(co_sync_t *sync, co_unsigned16_t eec,
		co_unsigned8_t er, void *data);
#endif

#if !LELY_NO_CO_TIME
/// Initializes the TIME producer/consumer service. @see co_nmt_srv_fini_time()
static int co_nmt_srv_init_time(struct co_nmt_srv *srv);
/// Finalizes the TIME producer/consumer service. @see co_nmt_srv_init_time()
static void co_nmt_srv_fini_time(struct co_nmt_srv *srv);
/// Starts the TIME producer/consumer service. @see co_nmt_srv_stop_time()
static int co_nmt_srv_start_time(struct co_nmt_srv *srv);
/// Stops the TIME producer/consumer service. @see co_nmt_srv_start_time()
static void co_nmt_srv_stop_time(struct co_nmt_srv *srv);
#endif

#if !LELY_NO_CO_EMCY
/// Initializes the EMCY producer/consumer service. @see co_nmt_srv_fini_emcy()
static int co_nmt_srv_init_emcy(struct co_nmt_srv *srv);
/// Finalizes the EMCY producer/consumer service. @see co_nmt_srv_init_emcy()
static void co_nmt_srv_fini_emcy(struct co_nmt_srv *srv);
/// Starts the EMCY producer/consumer service. @see co_nmt_srv_stop_emcy()
static int co_nmt_srv_start_emcy(struct co_nmt_srv *srv);
/// Stops the EMCY producer/consumer service. @see co_nmt_srv_start_emcy()
static void co_nmt_srv_stop_emcy(struct co_nmt_srv *srv);
#endif

#if !LELY_NO_CO_LSS
/// Initializes the LSS master/slave service. @see co_nmt_srv_fini_lss()
static int co_nmt_srv_init_lss(struct co_nmt_srv *srv);
/// Finalizes the EMCY master/slave service. @see co_nmt_srv_init_lss()
static void co_nmt_srv_fini_lss(struct co_nmt_srv *srv);
/// Starts the LSS master/slave service. @see co_nmt_srv_stop_lss()
static int co_nmt_srv_start_lss(struct co_nmt_srv *srv);
/// Stops the EMCY master/slave service. @see co_nmt_srv_start_lss()
static void co_nmt_srv_stop_lss(struct co_nmt_srv *srv);
#endif

/// The maximum number of Client/Server-SDOs.
#define CO_NUM_SDO 128

struct co_nmt_srv *
co_nmt_srv_init(struct co_nmt_srv *srv, co_nmt_t *nmt)
{
	assert(srv);
	assert(nmt);

	srv->nmt = nmt;

	srv->set = 0;

#if !LELY_NO_CO_RPDO
	srv->rpdos = NULL;
	srv->nrpdo = 0;
#endif
#if !LELY_NO_CO_TPDO
	srv->tpdos = NULL;
	srv->ntpdo = 0;
#endif

	srv->ssdos = NULL;
	srv->nssdo = 0;
#if !LELY_NO_CO_CSDO
	srv->csdos = NULL;
	srv->ncsdo = 0;
#endif

#if !LELY_NO_CO_SYNC
	srv->sync = NULL;
#endif
#if !LELY_NO_CO_TIME
	srv->time = NULL;
#endif
#if !LELY_NO_CO_EMCY
	srv->emcy = NULL;
#endif

#if !LELY_NO_CO_LSS
	srv->lss = NULL;
#endif

#if LELY_NO_MALLOC
#if !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO
	if (co_nmt_srv_init_pdo(srv) == -1)
		goto error_init_pdo;
#endif
#if !LELY_NO_CO_SDO
	if (co_nmt_srv_init_sdo(srv) == -1)
		goto error_init_sdo;
#endif
#if !LELY_NO_CO_SYNC
	if (co_nmt_srv_init_sync(srv) == -1)
		goto error_init_sync;
#endif
#if !LELY_NO_CO_TIME
	if (co_nmt_srv_init_time(srv) == -1)
		goto error_init_time;
#endif
#if !LELY_NO_CO_EMCY
	if (co_nmt_srv_init_emcy(srv) == -1)
		goto error_init_emcy;
#endif
#if !LELY_NO_CO_LSS
	if (co_nmt_srv_init_lss(srv) == -1)
		goto error_init_lss;
#endif
#endif // LELY_NO_MALLOC

	return srv;

#if LELY_NO_MALLOC
#if !LELY_NO_CO_LSS
	// co_nmt_srv_fini_lss(srv);
error_init_lss:
#endif
#if !LELY_NO_CO_EMCY
	co_nmt_srv_fini_emcy(srv);
error_init_emcy:
#endif
#if !LELY_NO_CO_TIME
	co_nmt_srv_fini_time(srv);
error_init_time:
#endif
#if !LELY_NO_CO_SYNC
	co_nmt_srv_fini_sync(srv);
error_init_sync:
#endif
	co_nmt_srv_fini_sdo(srv);
error_init_sdo:
#if !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO
	co_nmt_srv_fini_pdo(srv);
error_init_pdo:
#endif
	return NULL;
#endif // LELY_NO_MALLOC
}

void
co_nmt_srv_fini(struct co_nmt_srv *srv)
{
	assert(srv);

	co_nmt_srv_set(srv, 0);

#if LELY_NO_MALLOC
#if !LELY_NO_CO_LSS
	co_nmt_srv_fini_lss(srv);
#endif
#if !LELY_NO_CO_EMCY
	co_nmt_srv_fini_emcy(srv);
#endif
#if !LELY_NO_CO_TIME
	co_nmt_srv_fini_time(srv);
#endif
#if !LELY_NO_CO_SYNC
	co_nmt_srv_fini_sync(srv);
#endif
	co_nmt_srv_fini_sdo(srv);
#if !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO
	co_nmt_srv_fini_pdo(srv);
#endif
#endif // LELY_NO_MALLOC
}

void
co_nmt_srv_set(struct co_nmt_srv *srv, int set)
{
	assert(srv);

	int errsv = get_errc();
	set_errc(0);

#if !LELY_NO_CO_LSS
	if ((srv->set & ~set) & CO_NMT_SRV_LSS) {
		co_nmt_srv_stop_lss(srv);
#if !LELY_NO_MALLOC
		co_nmt_srv_fini_lss(srv);
#endif
	}
#endif
#if !LELY_NO_CO_EMCY
	if ((srv->set & ~set) & CO_NMT_SRV_EMCY) {
		co_nmt_srv_stop_emcy(srv);
#if !LELY_NO_MALLOC
		co_nmt_srv_fini_emcy(srv);
#endif
	}
#endif
#if !LELY_NO_CO_TIME
	if ((srv->set & ~set) & CO_NMT_SRV_TIME) {
		co_nmt_srv_stop_time(srv);
#if !LELY_NO_MALLOC
		co_nmt_srv_fini_time(srv);
#endif
	}
#endif
#if !LELY_NO_CO_SYNC
	if ((srv->set & ~set) & CO_NMT_SRV_SYNC) {
		co_nmt_srv_stop_sync(srv);
#if !LELY_NO_MALLOC
		co_nmt_srv_fini_sync(srv);
#endif
	}
#endif
	if ((srv->set & ~set) & CO_NMT_SRV_SDO) {
		co_nmt_srv_stop_sdo(srv);
#if !LELY_NO_MALLOC
		co_nmt_srv_fini_sdo(srv);
#endif
	}
#if !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO
	if ((srv->set & ~set) & CO_NMT_SRV_PDO) {
		co_nmt_srv_stop_pdo(srv);
#if !LELY_NO_MALLOC
		co_nmt_srv_fini_pdo(srv);
#endif
	}
#endif

#if !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO
	if ((set & ~srv->set) & CO_NMT_SRV_PDO) {
#if !LELY_NO_MALLOC
		if (!co_nmt_srv_init_pdo(srv))
#endif
			co_nmt_srv_start_pdo(srv);
	}
#endif
	if ((set & ~srv->set) & CO_NMT_SRV_SDO) {
#if !LELY_NO_MALLOC
		if (!co_nmt_srv_init_sdo(srv))
#endif
			co_nmt_srv_start_sdo(srv);
	}
#if !LELY_NO_CO_SYNC
	if ((set & ~srv->set) & CO_NMT_SRV_SYNC) {
#if !LELY_NO_MALLOC
		if (!co_nmt_srv_init_sync(srv))
#endif
			co_nmt_srv_start_sync(srv);
	}
#endif
#if !LELY_NO_CO_TIME
	if ((set & ~srv->set) & CO_NMT_SRV_TIME) {
#if !LELY_NO_MALLOC
		if (!co_nmt_srv_init_time(srv))
#endif
			co_nmt_srv_start_time(srv);
	}
#endif
#if !LELY_NO_CO_EMCY
	if ((set & ~srv->set) & CO_NMT_SRV_EMCY) {
#if !LELY_NO_MALLOC
		if (!co_nmt_srv_init_emcy(srv))
#endif
			co_nmt_srv_start_emcy(srv);
	}
#endif
#if !LELY_NO_CO_LSS
	if ((set & ~srv->set) & CO_NMT_SRV_LSS) {
#if !LELY_NO_MALLOC
		if (!co_nmt_srv_init_lss(srv))
#endif
			co_nmt_srv_start_lss(srv);
	}
#endif

	set_errc(errsv);
}

#if !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO

static int
co_nmt_srv_init_pdo(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_PDO));
	alloc_t *alloc = co_nmt_get_alloc(srv->nmt);
	can_net_t *net = co_nmt_get_net(srv->nmt);
	co_dev_t *dev = co_nmt_get_dev(srv->nmt);

#if !LELY_NO_CO_RPDO
	assert(!srv->rpdos);
	assert(!srv->nrpdo);

	size_t nrpdo = 0;
	for (co_unsigned16_t i = 0; i < CO_NUM_PDOS; i++) {
		co_obj_t *obj_1400 = co_dev_find_obj(dev, 0x1400 + i);
		co_obj_t *obj_1600 = co_dev_find_obj(dev, 0x1600 + i);
		if (!obj_1400 || !obj_1600)
			continue;
		nrpdo = i + 1;
	}

	// Create the Receive-PDOs.
	if (nrpdo) {
		srv->rpdos = mem_alloc(alloc, _Alignof(co_rpdo_t *),
				nrpdo * sizeof(co_rpdo_t *));
		if (!srv->rpdos)
			goto error;

		for (co_unsigned16_t i = 0; i < nrpdo; i++) {
			co_rpdo_t **ppdo = &srv->rpdos[srv->nrpdo++];
			*ppdo = NULL;

			co_obj_t *obj_1400 = co_dev_find_obj(dev, 0x1400 + i);
			co_obj_t *obj_1600 = co_dev_find_obj(dev, 0x1600 + i);
			if (!obj_1400 || !obj_1600)
				continue;

			if (!(*ppdo = co_rpdo_create(net, dev, i + 1)))
				goto error;
			co_rpdo_set_err(*ppdo, &co_nmt_srv_rpdo_err, srv->nmt);
		}
	}
#endif // !LELY_NO_CO_RPDO

#if !LELY_NO_CO_TPDO
	assert(!srv->tpdos);
	assert(!srv->ntpdo);

	size_t ntpdo = 0;
	for (co_unsigned16_t i = 0; i < CO_NUM_PDOS; i++) {
		co_obj_t *obj_1800 = co_dev_find_obj(dev, 0x1800 + i);
		co_obj_t *obj_1a00 = co_dev_find_obj(dev, 0x1a00 + i);
		if (!obj_1800 || !obj_1a00)
			continue;
		ntpdo = i + 1;
	}

	// Create the Transmit-PDOs.
	if (ntpdo) {
		srv->tpdos = mem_alloc(alloc, _Alignof(co_tpdo_t *),
				ntpdo * sizeof(co_tpdo_t *));
		if (!srv->tpdos)
			goto error;

		for (co_unsigned16_t i = 0; i < ntpdo; i++) {
			co_tpdo_t **ppdo = &srv->tpdos[srv->ntpdo++];
			*ppdo = NULL;

			co_obj_t *obj_1800 = co_dev_find_obj(dev, 0x1800 + i);
			co_obj_t *obj_1a00 = co_dev_find_obj(dev, 0x1a00 + i);
			if (!obj_1800 || !obj_1a00)
				continue;

			if (!(*ppdo = co_tpdo_create(net, dev, i + 1)))
				goto error;
		}
	}
#endif // !LELY_NO_CO_TPDO

	return 0;

error:
	diag(DIAG_ERROR, get_errc(), "unable to initialize PDO services");
	co_nmt_srv_fini_pdo(srv);
	return -1;
}

static void
co_nmt_srv_fini_pdo(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_PDO));
	alloc_t *alloc = co_nmt_get_alloc(srv->nmt);

#if !LELY_NO_CO_TPDO
	// Destroy the Transmit-PDOs.
	for (size_t i = 0; i < srv->ntpdo; i++)
		co_tpdo_destroy(srv->tpdos[i]);
	mem_free(alloc, srv->tpdos);
	srv->tpdos = NULL;
	srv->ntpdo = 0;
#endif

#if !LELY_NO_CO_RPDO
	// Destroy the Receive-PDOs.
	for (size_t i = 0; i < srv->nrpdo; i++)
		co_rpdo_destroy(srv->rpdos[i]);
	mem_free(alloc, srv->rpdos);
	srv->rpdos = NULL;
	srv->nrpdo = 0;
#endif
}

static int
co_nmt_srv_start_pdo(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_PDO));

	srv->set |= CO_NMT_SRV_PDO;

#if !LELY_NO_CO_RPDO
	// Start the Receive-PDOs.
	for (size_t i = 0; i < srv->nrpdo; i++) {
		if (co_rpdo_start(srv->rpdos[i]) == -1)
			goto error;
	}
#endif

#if !LELY_NO_CO_TPDO
	// Start the Transmit-PDOs.
	for (size_t i = 0; i < srv->ntpdo; i++) {
		if (co_tpdo_start(srv->tpdos[i]) == -1)
			goto error;
	}
#endif

	return 0;

error:
	diag(DIAG_ERROR, get_errc(), "unable to start PDO services");
	co_nmt_srv_stop_pdo(srv);
	return -1;
}

static void
co_nmt_srv_stop_pdo(struct co_nmt_srv *srv)
{
	assert(srv);

	if (!(srv->set & CO_NMT_SRV_PDO))
		return;

#if !LELY_NO_CO_TPDO
	// Stop the Transmit-PDOs.
	for (size_t i = 0; i < srv->ntpdo; i++)
		co_tpdo_stop(srv->tpdos[i]);
#endif

#if !LELY_NO_CO_RPDO
	// Stop the Receive-PDOs.
	for (size_t i = 0; i < srv->nrpdo; i++)
		co_rpdo_stop(srv->rpdos[i]);
#endif

	srv->set &= ~CO_NMT_SRV_PDO;
}

#if !LELY_NO_CO_RPDO
static void
co_nmt_srv_rpdo_err(co_rpdo_t *pdo, co_unsigned16_t eec, co_unsigned8_t er,
		void *data)
{
	(void)pdo;
	co_nmt_t *nmt = data;
	assert(nmt);

	co_nmt_on_err(nmt, eec, er, NULL);
}
#endif

#endif // !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO

static int
co_nmt_srv_init_sdo(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_SDO));
	assert(!srv->ssdos);
	assert(!srv->nssdo);
	alloc_t *alloc = co_nmt_get_alloc(srv->nmt);
	can_net_t *net = co_nmt_get_net(srv->nmt);
	co_dev_t *dev = co_nmt_get_dev(srv->nmt);

	size_t nssdo = 0;
	for (co_unsigned8_t i = 0; i < CO_NUM_SDO; i++) {
		co_obj_t *obj_1200 = co_dev_find_obj(dev, 0x1200 + i);
		// The default Server-SDO does not have to exist in the object
		// dictionary.
		if (i && !obj_1200)
			continue;
		nssdo = i + 1;
	}

	// Create the Server-SDOs.
	if (nssdo) {
		srv->ssdos = mem_alloc(alloc, _Alignof(co_ssdo_t *),
				nssdo * sizeof(co_ssdo_t *));
		if (!srv->ssdos)
			goto error;

		for (co_unsigned8_t i = 0; i < nssdo; i++) {
			co_ssdo_t **psdo = &srv->ssdos[srv->nssdo++];
			*psdo = NULL;

			co_obj_t *obj_1200 = co_dev_find_obj(dev, 0x1200 + i);
			if (i && !obj_1200)
				continue;

			if (!(*psdo = co_ssdo_create(net, dev, i + 1)))
				goto error;
		}
	}

#if !LELY_NO_CO_CSDO
	assert(!srv->csdos);
	assert(!srv->ncsdo);

	size_t ncsdo = 0;
	for (co_unsigned8_t i = 0; i < CO_NUM_SDO; i++) {
		co_obj_t *obj_1280 = co_dev_find_obj(dev, 0x1280 + i);
		if (!obj_1280)
			continue;
		ncsdo = i + 1;
	}

	// Create the Client-SDOs.
	if (ncsdo) {
		srv->csdos = mem_alloc(alloc, _Alignof(co_csdo_t *),
				ncsdo * sizeof(co_csdo_t *));
		if (!srv->csdos)
			goto error;

		for (co_unsigned8_t i = 0; i < ncsdo; i++) {
			co_csdo_t **psdo = &srv->csdos[srv->ncsdo++];
			*psdo = NULL;

			co_obj_t *obj_1280 = co_dev_find_obj(dev, 0x1280 + i);
			if (!obj_1280)
				continue;

			if (!(*psdo = co_csdo_create(net, dev, i + 1)))
				goto error;
		}
	}
#endif // !LELY_NO_CO_CSDO

	return 0;

error:
	diag(DIAG_ERROR, get_errc(), "unable to initialize SDO services");
	co_nmt_srv_fini_sdo(srv);
	return -1;
}

static void
co_nmt_srv_fini_sdo(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_SDO));
	alloc_t *alloc = co_nmt_get_alloc(srv->nmt);

#if !LELY_NO_CO_CSDO
	// Destroy the Client-SDOs.
	for (size_t i = 0; i < srv->ncsdo; i++)
		co_csdo_destroy(srv->csdos[i]);
	mem_free(alloc, srv->csdos);
	srv->csdos = NULL;
	srv->ncsdo = 0;
#endif

	// Destroy the Server-SDOs (skipping the default one).
	for (size_t i = 0; i < srv->nssdo; i++)
		co_ssdo_destroy(srv->ssdos[i]);
	mem_free(alloc, srv->ssdos);
	srv->ssdos = NULL;
	srv->nssdo = 0;
}

static int
co_nmt_srv_start_sdo(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_SDO));

	srv->set |= CO_NMT_SRV_SDO;

	// Start the Server-SDOs (skipping the default one).
	for (size_t i = 0; i < srv->nssdo; i++) {
		if (co_ssdo_start(srv->ssdos[i]) == -1)
			goto error;
	}

#if !LELY_NO_CO_CSDO
	// Start the Client-SDOs.
	for (size_t i = 0; i < srv->ncsdo; i++) {
		if (co_csdo_start(srv->csdos[i]) == -1)
			goto error;
	}
#endif

	return 0;

error:
	diag(DIAG_ERROR, get_errc(), "unable to start SDO services");
	co_nmt_srv_stop_sdo(srv);
	return -1;
}

static void
co_nmt_srv_stop_sdo(struct co_nmt_srv *srv)
{
	assert(srv);

	if (!(srv->set & CO_NMT_SRV_SDO))
		return;

#if !LELY_NO_CO_CSDO
	// Stop the Client-SDOs.
	for (size_t i = 0; i < srv->ncsdo; i++)
		co_csdo_stop(srv->csdos[i]);
#endif

	// Stop the Server-SDOs (skipping the default one).
	for (size_t i = 0; i < srv->nssdo; i++)
		co_ssdo_stop(srv->ssdos[i]);

	srv->set &= ~CO_NMT_SRV_SDO;
}

#if !LELY_NO_CO_SYNC

static int
co_nmt_srv_init_sync(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_SYNC));
	assert(!srv->sync);
	can_net_t *net = co_nmt_get_net(srv->nmt);
	co_dev_t *dev = co_nmt_get_dev(srv->nmt);

	co_obj_t *obj_1005 = co_dev_find_obj(dev, 0x1005);
	if (!obj_1005)
		return 0;

	srv->sync = co_sync_create(net, dev);
	if (!srv->sync) {
		diag(DIAG_ERROR, get_errc(),
				"unable to initialize SYNC service");
		return -1;
	}

	co_sync_set_ind(srv->sync, &co_nmt_srv_sync_ind, srv->nmt);
	co_sync_set_err(srv->sync, &co_nmt_srv_sync_err, srv->nmt);

	return 0;
}

static void
co_nmt_srv_fini_sync(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_SYNC));

	if (srv->sync) {
		co_sync_destroy(srv->sync);
		srv->sync = NULL;
	}
}

static int
co_nmt_srv_start_sync(struct co_nmt_srv *srv)
{
	assert(srv);

	if (!srv->sync)
		return 0;

	assert(!(srv->set & CO_NMT_SRV_SYNC));

	if (co_sync_start(srv->sync) == -1) {
		diag(DIAG_ERROR, get_errc(), "unable to start SYNC service");
		return -1;
	}

	srv->set |= CO_NMT_SRV_SYNC;

	return 0;
}

static void
co_nmt_srv_stop_sync(struct co_nmt_srv *srv)
{
	assert(srv);

	if (!(srv->set & CO_NMT_SRV_SYNC))
		return;

	co_sync_stop(srv->sync);

	srv->set &= ~CO_NMT_SRV_SYNC;
}

static void
co_nmt_srv_sync_ind(co_sync_t *sync, co_unsigned8_t cnt, void *data)
{
	(void)sync;
	co_nmt_t *nmt = data;
	assert(nmt);

	co_nmt_on_sync(nmt, cnt);
}

static void
co_nmt_srv_sync_err(co_sync_t *sync, co_unsigned16_t eec, co_unsigned8_t er,
		void *data)
{
	(void)sync;
	co_nmt_t *nmt = data;
	assert(nmt);

	co_nmt_on_err(nmt, eec, er, NULL);
}

#endif // !LELY_NO_CO_SYNC

#if !LELY_NO_CO_TIME

static int
co_nmt_srv_init_time(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_TIME));
	assert(!srv->time);
	can_net_t *net = co_nmt_get_net(srv->nmt);
	co_dev_t *dev = co_nmt_get_dev(srv->nmt);

	co_obj_t *obj_1012 = co_dev_find_obj(dev, 0x1012);
	if (!obj_1012)
		return 0;

	srv->time = co_time_create(net, dev);
	if (!srv->time) {
		diag(DIAG_ERROR, get_errc(),
				"unable to initialize TIME service");
		return -1;
	}

	return 0;
}

static void
co_nmt_srv_fini_time(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_TIME));

	if (srv->time) {
		co_time_destroy(srv->time);
		srv->time = NULL;
	}
}

static int
co_nmt_srv_start_time(struct co_nmt_srv *srv)
{
	assert(srv);

	if (!srv->time)
		return 0;

	assert(!(srv->set & CO_NMT_SRV_TIME));

	if (co_time_start(srv->time) == -1) {
		diag(DIAG_ERROR, get_errc(), "unable to start TIME service");
		return -1;
	}

	srv->set |= CO_NMT_SRV_TIME;

	return 0;
}

static void
co_nmt_srv_stop_time(struct co_nmt_srv *srv)
{
	assert(srv);

	if (!(srv->set & CO_NMT_SRV_TIME))
		return;

	co_time_stop(srv->time);

	srv->set &= ~CO_NMT_SRV_TIME;
}

#endif // !LELY_NO_CO_TIME

#if !LELY_NO_CO_EMCY

static int
co_nmt_srv_init_emcy(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_EMCY));
	assert(!srv->emcy);
	can_net_t *net = co_nmt_get_net(srv->nmt);
	co_dev_t *dev = co_nmt_get_dev(srv->nmt);

	co_obj_t *obj_1001 = co_dev_find_obj(dev, 0x1001);
	if (!obj_1001)
		return 0;

	srv->emcy = co_emcy_create(net, dev);
	if (!srv->emcy) {
		diag(DIAG_ERROR, get_errc(),
				"unable to initialize EMCY service");
		return -1;
	}

	return 0;
}

static void
co_nmt_srv_fini_emcy(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_EMCY));

	if (srv->emcy) {
		co_emcy_destroy(srv->emcy);
		srv->emcy = NULL;
	}
}

static int
co_nmt_srv_start_emcy(struct co_nmt_srv *srv)
{
	assert(srv);

	if (!srv->emcy)
		return 0;

	assert(!(srv->set & CO_NMT_SRV_EMCY));

	if (co_emcy_start(srv->emcy) == -1) {
		diag(DIAG_ERROR, get_errc(), "unable to start EMCY service");
		return -1;
	}

	srv->set |= CO_NMT_SRV_EMCY;

	return 0;
}

static void
co_nmt_srv_stop_emcy(struct co_nmt_srv *srv)
{
	assert(srv);

	if (!(srv->set & CO_NMT_SRV_EMCY))
		return;

	co_emcy_stop(srv->emcy);

	srv->set &= ~CO_NMT_SRV_EMCY;
}

#endif // !LELY_NO_CO_EMCY

#if !LELY_NO_CO_LSS

static int
co_nmt_srv_init_lss(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_LSS));
	assert(!srv->lss);
	co_nmt_t *nmt = srv->nmt;

	if (!co_dev_get_lss(co_nmt_get_dev(nmt)))
		return 0;

	srv->lss = co_lss_create(nmt);
	if (!srv->lss) {
		diag(DIAG_ERROR, get_errc(),
				"unable to initialize LSS service");
		return -1;
	}

	return 0;
}

static void
co_nmt_srv_fini_lss(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_LSS));

	if (srv->lss) {
		co_lss_destroy(srv->lss);
		srv->lss = NULL;
	}
}

static int
co_nmt_srv_start_lss(struct co_nmt_srv *srv)
{
	assert(srv);

	if (!srv->lss)
		return 0;

	assert(!(srv->set & CO_NMT_SRV_LSS));

	if (co_lss_start(srv->lss) == -1) {
		diag(DIAG_ERROR, get_errc(), "unable to start LSS service");
		return -1;
	}

	srv->set |= CO_NMT_SRV_LSS;

	return 0;
}

static void
co_nmt_srv_stop_lss(struct co_nmt_srv *srv)
{
	assert(srv);

	if (!(srv->set & CO_NMT_SRV_LSS))
		return;

	co_lss_stop(srv->lss);

	srv->set &= ~CO_NMT_SRV_LSS;
}

#endif // !LELY_NO_CO_LSS
