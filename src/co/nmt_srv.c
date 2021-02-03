/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the NMT service manager functions.
 *
 * @see src/nmt_srv.h
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
#include <stdlib.h>

#if !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO
/// Initializes all Receive/Transmit-PDO services. @see co_nmt_srv_fini_pdo()
static void co_nmt_srv_init_pdo(
		struct co_nmt_srv *srv, can_net_t *net, co_dev_t *dev);
/// Finalizes all Receive/Transmit-PDO services. @see co_nmt_srv_init_pdo()
static void co_nmt_srv_fini_pdo(struct co_nmt_srv *srv);
#if !LELY_NO_CO_RPDO
/// Invokes co_nmt_err() to handle Receive-PDO errors. @see co_rpdo_err_t
static void co_nmt_srv_rpdo_err(co_rpdo_t *pdo, co_unsigned16_t eec,
		co_unsigned8_t er, void *data);
#endif
#endif

/// Initializes all Server/Client-SDO services. @see co_nmt_srv_fini_sdo()
static void co_nmt_srv_init_sdo(
		struct co_nmt_srv *srv, can_net_t *net, co_dev_t *dev);
/// Finalizes all Server/Client-SDO services. @see co_nmt_srv_init_sdo()
static void co_nmt_srv_fini_sdo(struct co_nmt_srv *srv);

#if !LELY_NO_CO_SYNC
/// Initializes the SYNC producer/consumer service. @see co_nmt_srv_fini_sync()
static void co_nmt_srv_init_sync(
		struct co_nmt_srv *srv, can_net_t *net, co_dev_t *dev);
/// Finalizes the SYNC producer/consumer service. @see co_nmt_srv_init_sync()
static void co_nmt_srv_fini_sync(struct co_nmt_srv *srv);
/// Invokes co_nmt_sync() to handle SYNC objects. @see co_sync_ind_t
static void co_nmt_srv_sync_ind(
		co_sync_t *sync, co_unsigned8_t cnt, void *data);
/// Invokes co_nmt_err() to handle SYNC errors. @see co_sync_err_t
static void co_nmt_srv_sync_err(co_sync_t *sync, co_unsigned16_t eec,
		co_unsigned8_t er, void *data);
#endif

#if !LELY_NO_CO_TIME
/// Initializes the TIME producer/consumer service. @see co_nmt_srv_fini_time()
static void co_nmt_srv_init_time(
		struct co_nmt_srv *srv, can_net_t *net, co_dev_t *dev);
/// Finalizes the TIME producer/consumer service. @see co_nmt_srv_init_time()
static void co_nmt_srv_fini_time(struct co_nmt_srv *srv);
#endif

#if !LELY_NO_CO_EMCY
/// Initializes the EMCY producer/consumer service. @see co_nmt_srv_fini_emcy()
static void co_nmt_srv_init_emcy(
		struct co_nmt_srv *srv, can_net_t *net, co_dev_t *dev);
/// Finalizes the EMCY producer/consumer service. @see co_nmt_srv_init_emcy()
static void co_nmt_srv_fini_emcy(struct co_nmt_srv *srv);
#endif

#if !LELY_NO_CO_LSS
/// Initializes the LSS master/slave service. @see co_nmt_srv_fini_lss()
static void co_nmt_srv_init_lss(struct co_nmt_srv *srv, co_nmt_t *nmt);
/// Finalizes the EMCY master/slave service. @see co_nmt_srv_init_lss()
static void co_nmt_srv_fini_lss(struct co_nmt_srv *srv);
#endif

void
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
}

void
co_nmt_srv_fini(struct co_nmt_srv *srv)
{
	assert(srv);

	co_nmt_srv_set(srv, NULL, 0);
}

void
co_nmt_srv_set(struct co_nmt_srv *srv, co_nmt_t *nmt, int set)
{
	assert(srv);

#if !LELY_NO_CO_LSS
	if ((srv->set & ~set) & CO_NMT_SRV_LSS)
		co_nmt_srv_fini_lss(srv);
#endif
#if !LELY_NO_CO_EMCY
	if ((srv->set & ~set) & CO_NMT_SRV_EMCY)
		co_nmt_srv_fini_emcy(srv);
#endif
#if !LELY_NO_CO_TIME
	if ((srv->set & ~set) & CO_NMT_SRV_TIME)
		co_nmt_srv_fini_time(srv);
#endif
#if !LELY_NO_CO_SYNC
	if ((srv->set & ~set) & CO_NMT_SRV_SYNC)
		co_nmt_srv_fini_sync(srv);
#endif
	if ((srv->set & ~set) & CO_NMT_SRV_SDO)
		co_nmt_srv_fini_sdo(srv);
#if !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO
	if ((srv->set & ~set) & CO_NMT_SRV_PDO)
		co_nmt_srv_fini_pdo(srv);
#endif

	if (nmt) {
		can_net_t *net = co_nmt_get_net(nmt);
		co_dev_t *dev = co_nmt_get_dev(nmt);
#if !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO
		if ((set & ~srv->set) & CO_NMT_SRV_PDO)
			co_nmt_srv_init_pdo(srv, net, dev);
#endif
		if ((set & ~srv->set) & CO_NMT_SRV_SDO)
			co_nmt_srv_init_sdo(srv, net, dev);
#if !LELY_NO_CO_SYNC
		if ((set & ~srv->set) & CO_NMT_SRV_SYNC)
			co_nmt_srv_init_sync(srv, net, dev);
#endif
#if !LELY_NO_CO_TIME
		if ((set & ~srv->set) & CO_NMT_SRV_TIME)
			co_nmt_srv_init_time(srv, net, dev);
#endif
#if !LELY_NO_CO_EMCY
		if ((set & ~srv->set) & CO_NMT_SRV_EMCY)
			co_nmt_srv_init_emcy(srv, net, dev);
#endif
#if !LELY_NO_CO_LSS
		if ((set & ~srv->set) & CO_NMT_SRV_LSS)
			co_nmt_srv_init_lss(srv, nmt);
#endif
	}
}

#if !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO

static void
co_nmt_srv_init_pdo(struct co_nmt_srv *srv, can_net_t *net, co_dev_t *dev)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_PDO));

	srv->set |= CO_NMT_SRV_PDO;

#if !LELY_NO_CO_RPDO
	assert(!srv->rpdos);
	assert(!srv->nrpdo);

	// Create the Receive-PDOs.
	for (co_unsigned16_t i = 0; i < CO_NUM_PDOS; i++) {
		co_obj_t *obj_1400 = co_dev_find_obj(dev, 0x1400 + i);
		co_obj_t *obj_1600 = co_dev_find_obj(dev, 0x1600 + i);
		if (!obj_1400 || !obj_1600)
			continue;

		co_rpdo_t **rpdos =
				realloc(srv->rpdos, (i + 1) * sizeof(*rpdos));
		if (!rpdos) {
#if !LELY_NO_ERRNO
			set_errc(errno2c(errno));
#endif
			goto error;
		}
		srv->rpdos = rpdos;

		for (size_t j = srv->nrpdo; j < i; j++)
			srv->rpdos[j] = NULL;
		srv->rpdos[i] = co_rpdo_create(net, dev, i + 1);
		if (!srv->rpdos[i])
			goto error;
		co_rpdo_set_err(srv->rpdos[i], &co_nmt_srv_rpdo_err, srv->nmt);

		srv->nrpdo = i + 1;
	}
#endif // !LELY_NO_CO_RPDO

#if !LELY_NO_CO_TPDO
	assert(!srv->tpdos);
	assert(!srv->ntpdo);

	// Create the Transmit-PDOs.
	for (co_unsigned16_t i = 0; i < CO_NUM_PDOS; i++) {
		co_obj_t *obj_1800 = co_dev_find_obj(dev, 0x1800 + i);
		co_obj_t *obj_1a00 = co_dev_find_obj(dev, 0x1a00 + i);
		if (!obj_1800 || !obj_1a00)
			continue;

		co_tpdo_t **tpdos =
				realloc(srv->tpdos, (i + 1) * sizeof(*tpdos));
		if (!tpdos) {
#if !LELY_NO_ERRNO
			set_errc(errno2c(errno));
#endif
			goto error;
		}
		srv->tpdos = tpdos;

		for (size_t j = srv->ntpdo; j < i; j++)
			srv->tpdos[j] = NULL;
		srv->tpdos[i] = co_tpdo_create(net, dev, i + 1);
		if (!srv->tpdos[i])
			goto error;

		srv->ntpdo = i + 1;
	}
#endif // !LELY_NO_CO_TPDO

	return;

error:
	diag(DIAG_ERROR, get_errc(), "unable to initialize PDO services");
}

static void
co_nmt_srv_fini_pdo(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(srv->set & CO_NMT_SRV_PDO);

	srv->set &= ~CO_NMT_SRV_PDO;

#if !LELY_NO_CO_TPDO
	// Destroy the Transmit-PDOs.
	for (size_t i = 0; i < srv->ntpdo; i++)
		co_tpdo_destroy(srv->tpdos[i]);
	free(srv->tpdos);
	srv->tpdos = NULL;
	srv->ntpdo = 0;
#endif

#if !LELY_NO_CO_RPDO
	// Destroy the Receive-PDOs.
	for (size_t i = 0; i < srv->nrpdo; i++)
		co_rpdo_destroy(srv->rpdos[i]);
	free(srv->rpdos);
	srv->rpdos = NULL;
	srv->nrpdo = 0;
#endif
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

static void
co_nmt_srv_init_sdo(struct co_nmt_srv *srv, can_net_t *net, co_dev_t *dev)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_SDO));
	assert(!srv->ssdos);
	assert(!srv->nssdo);

	srv->set |= CO_NMT_SRV_SDO;

	// Create the Server-SDOs.
	for (co_unsigned8_t i = 0; i < CO_NUM_SDOS; i++) {
		co_obj_t *obj_1200 = co_dev_find_obj(dev, 0x1200 + i);
		// The default Server-SDO does not have to exist in the object
		// dictionary.
		if (i && !obj_1200)
			continue;

		co_ssdo_t **ssdos =
				realloc(srv->ssdos, (i + 1) * sizeof(*ssdos));
		if (!ssdos) {
#if !LELY_NO_ERRNO
			set_errc(errno2c(errno));
#endif
			goto error;
		}
		srv->ssdos = ssdos;

		for (size_t j = srv->nssdo; j < i; j++)
			srv->ssdos[j] = NULL;
		srv->ssdos[i] = co_ssdo_create(net, dev, i + 1);
		if (!srv->ssdos[i])
			goto error;

		srv->nssdo = i + 1;
	}

#if !LELY_NO_CO_CSDO
	assert(!srv->csdos);
	assert(!srv->ncsdo);

	// Create the Client-SDOs.
	for (co_unsigned8_t i = 0; i < CO_NUM_SDOS; i++) {
		co_obj_t *obj_1280 = co_dev_find_obj(dev, 0x1280 + i);
		if (!obj_1280)
			continue;

		co_csdo_t **csdos =
				realloc(srv->csdos, (i + 1) * sizeof(*csdos));
		if (!csdos) {
#if !LELY_NO_ERRNO
			set_errc(errno2c(errno));
#endif
			goto error;
		}
		srv->csdos = csdos;

		for (size_t j = srv->ncsdo; j < i; j++)
			srv->csdos[j] = NULL;
		srv->csdos[i] = co_csdo_create(net, dev, i + 1);
		if (!srv->csdos[i])
			goto error;

		srv->ncsdo = i + 1;
	}
#endif // !LELY_NO_CO_CSDO

	return;

error:
	diag(DIAG_ERROR, get_errc(), "unable to initialize SDO services");
}

static void
co_nmt_srv_fini_sdo(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(srv->set & CO_NMT_SRV_SDO);

	srv->set &= ~CO_NMT_SRV_SDO;

#if !LELY_NO_CO_CSDO
	// Destroy the Client-SDOs.
	for (size_t i = 0; i < srv->ncsdo; i++)
		co_csdo_destroy(srv->csdos[i]);
	free(srv->csdos);
	srv->csdos = NULL;
	srv->ncsdo = 0;
#endif

	// Destroy the Server-SDOs (skipping the default one).
	for (size_t i = 0; i < srv->nssdo; i++)
		co_ssdo_destroy(srv->ssdos[i]);
	free(srv->ssdos);
	srv->ssdos = NULL;
	srv->nssdo = 0;
}

#if !LELY_NO_CO_SYNC

static void
co_nmt_srv_init_sync(struct co_nmt_srv *srv, can_net_t *net, co_dev_t *dev)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_SYNC));
	assert(!srv->sync);

	srv->set |= CO_NMT_SRV_SYNC;

	co_obj_t *obj_1005 = co_dev_find_obj(dev, 0x1005);
	if (!obj_1005)
		return;

	srv->sync = co_sync_create(net, dev);
	if (!srv->sync) {
		diag(DIAG_ERROR, get_errc(),
				"unable to initialize SYNC service");
		return;
	}
	co_sync_set_ind(srv->sync, &co_nmt_srv_sync_ind, srv->nmt);
	co_sync_set_err(srv->sync, &co_nmt_srv_sync_err, srv->nmt);
}

static void
co_nmt_srv_fini_sync(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(srv->set & CO_NMT_SRV_SYNC);

	srv->set &= ~CO_NMT_SRV_SYNC;

	co_sync_destroy(srv->sync);
	srv->sync = NULL;
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

static void
co_nmt_srv_init_time(struct co_nmt_srv *srv, can_net_t *net, co_dev_t *dev)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_TIME));
	assert(!srv->time);

	srv->set |= CO_NMT_SRV_TIME;

	co_obj_t *obj_1012 = co_dev_find_obj(dev, 0x1012);
	if (!obj_1012)
		return;

	srv->time = co_time_create(net, dev);
	if (!srv->time)
		diag(DIAG_ERROR, get_errc(),
				"unable to initialize TIME service");
}

static void
co_nmt_srv_fini_time(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(srv->set & CO_NMT_SRV_TIME);

	srv->set &= ~CO_NMT_SRV_TIME;

	co_time_destroy(srv->time);
	srv->time = NULL;
}

#endif // !LELY_NO_CO_TIME

#if !LELY_NO_CO_EMCY

static void
co_nmt_srv_init_emcy(struct co_nmt_srv *srv, can_net_t *net, co_dev_t *dev)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_EMCY));
	assert(!srv->emcy);

	srv->set |= CO_NMT_SRV_EMCY;

	co_obj_t *obj_1001 = co_dev_find_obj(dev, 0x1001);
	if (!obj_1001)
		return;

	srv->emcy = co_emcy_create(net, dev);
	if (!srv->emcy)
		diag(DIAG_ERROR, get_errc(),
				"unable to initialize EMCY service");
}

static void
co_nmt_srv_fini_emcy(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(srv->set & CO_NMT_SRV_EMCY);

	srv->set &= ~CO_NMT_SRV_EMCY;

	co_emcy_destroy(srv->emcy);
	srv->emcy = NULL;
}

#endif // !LELY_NO_CO_EMCY

#if !LELY_NO_CO_LSS

static void
co_nmt_srv_init_lss(struct co_nmt_srv *srv, co_nmt_t *nmt)
{
	assert(srv);
	assert(!(srv->set & CO_NMT_SRV_LSS));
	assert(!srv->lss);

	if (!co_dev_get_lss(co_nmt_get_dev(nmt)))
		return;

	srv->set |= CO_NMT_SRV_LSS;

	srv->lss = co_lss_create(nmt);
	if (!srv->lss)
		diag(DIAG_ERROR, get_errc(),
				"unable to initialize LSS service");
}

static void
co_nmt_srv_fini_lss(struct co_nmt_srv *srv)
{
	assert(srv);
	assert(srv->set & CO_NMT_SRV_LSS);

	srv->set &= ~CO_NMT_SRV_LSS;

	co_lss_destroy(srv->lss);
	srv->lss = NULL;
}

#endif // !LELY_NO_CO_LSS
