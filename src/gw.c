/*!\file
 * This file is part of the CANopen library; it contains the implementation of
 * the gateway functions.
 *
 * \see lely/co/gw.h
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

#include "co.h"

#ifndef LELY_NO_CO_GW

#include <lely/util/errnum.h>
#include <lely/co/gw.h>
#include <lely/co/sdo.h>

#include <assert.h>
#include <stdlib.h>

//! A CANopen network.
struct co_gw_net {
	//! A pointer to the CANopen gateway.
	co_gw_t *gw;
	//! The network-ID.
	co_unsigned16_t id;
	//! A pointer to a CANopen NMT master/slave service.
	co_nmt_t *nmt;
};

//! Creates a new CANopen network. \see co_gw_net_destroy()
static struct co_gw_net *co_gw_net_create(co_gw_t *gw, co_unsigned16_t id,
		co_nmt_t *nmt);

//! Destroys a CANopen network. \see co_gw_net_create()
static void co_gw_net_destroy(struct co_gw_net *net);

//! A CANopen gateway.
struct __co_gw {
	//! An array of pointers to the CANopen networks.
	struct co_gw_net *net[CO_GW_NUM_NET];
	/*!
	 * A pointer to the callback function invoked when an indication or
	 * confirmation needs to be sent.
	 */
	co_gw_send_func_t *send_func;
	//! A pointer to the user-specified data for #send_func.
	void *send_data;
};

//! Sends a confirmation with an internal error code or SDO abort code.
static int co_gw_send_con(co_gw_t *gw, const struct co_gw_req *req, int iec,
		co_unsigned32_t ac);
//! Invokes the callback function to send a confirmation or indication.
static int co_gw_send_srv(co_gw_t *gw, const struct co_gw_srv *srv);

LELY_CO_EXPORT const char *
co_gw_iec2str(int iec)
{
	switch (iec) {
	case CO_GW_IEC_BAD_SRV:
		return "Request not supported";
	case CO_GW_IEC_SYNTAX:
		return "Syntax error";
	case CO_GW_IEC_INTERN:
		return "Request not processed due to internal state";
	case CO_GW_IEC_TIMEOUT:
		return "Time-out";
	case CO_GW_IEC_NO_DEF_NET:
		return "No default net set";
	case CO_GW_IEC_NO_DEF_NODE:
		return "No default node set";
	case CO_GW_IEC_BAD_NET:
		return "Unsupported net";
	case CO_GW_IEC_BAD_NODE:
		return "Unsupported node";
	case CO_GW_IEC_NG_OCCURRED:
		return "Lost guarding message";
	case CO_GW_IEC_LG_OCCURRED:
		return "Lost connection";
	case CO_GW_IEC_HB_RESOLVED:
		return "Heartbeat started";
	case CO_GW_IEC_HB_OCCURRED:
		return "Heartbeat lost";
	case CO_GW_IEC_ST_OCCURRED:
		return "Wrong NMT state";
	case CO_GW_IEC_BOOTUP:
		return "Boot-up";
	case CO_GW_IEC_CAN_PASSIVE:
		return "Error passive";
	case CO_GW_IEC_CAN_BUSOFF:
		return "Bus off";
	case CO_GW_IEC_CAN_OVERFLOW:
		return "CAN buffer overflow";
	case CO_GW_IEC_CAN_INIT:
		return "CAN init";
	case CO_GW_IEC_CAN_ACTIVE:
		return "CAN active";
	case CO_GW_IEC_PDO_INUSE:
		return "PDO already used";
	case CO_GW_IEC_PDO_LEN:
		return "PDO length exceeded";
	case CO_GW_IEC_LSS:
		return "LSS error";
	case CO_GW_IEC_LSS_ID:
		return "LSS node-ID not supported";
	case CO_GW_IEC_LSS_RATE:
		return "LSS bit-rate not supported";
	case CO_GW_IEC_LSS_PARAM:
		return "LSS parameter storing failed";
	case CO_GW_IEC_LSS_MEDIA:
		return "LSS command failed because of media error";
	case CO_GW_IEC_NO_MEM:
		return "Running out of memory";
	default:
		return "Unknown error code";
	}
}

LELY_CO_EXPORT void *
__co_gw_alloc(void)
{
	void *ptr = malloc(sizeof(struct __co_gw));
	if (__unlikely(!ptr))
		set_errno(errno);
	return ptr;
}

LELY_CO_EXPORT void
__co_gw_free(void *ptr)
{
	free(ptr);
}

LELY_CO_EXPORT struct __co_gw *
__co_gw_init(struct __co_gw *gw)
{
	assert(gw);

	for (co_unsigned16_t id = 1; id <= CO_GW_NUM_NET; id++)
		gw->net[id - 1] = NULL;

	gw->send_func = NULL;
	gw->send_data = NULL;

	return gw;
}

LELY_CO_EXPORT void
__co_gw_fini(struct __co_gw *gw)
{
	assert(gw);

	for (co_unsigned16_t id = 1; id <= CO_GW_NUM_NET; id++)
		co_gw_net_destroy(gw->net[id - 1]);
}

LELY_CO_EXPORT co_gw_t *
co_gw_create(void)
{
	errc_t errc = 0;

	co_gw_t *gw = __co_gw_alloc();
	if (__unlikely(!gw)) {
		errc = get_errc();
		goto error_alloc_gw;
	}

	if (__unlikely(!__co_gw_init(gw))) {
		errc = get_errc();
		goto error_init_gw;
	}

	return gw;

error_init_gw:
	__co_gw_free(gw);
error_alloc_gw:
	set_errc(errc);
	return NULL;
}

LELY_CO_EXPORT void
co_gw_destroy(co_gw_t *gw)
{
	if (gw) {
		__co_gw_fini(gw);
		__co_gw_free(gw);
	}
}

LELY_CO_EXPORT int
co_gw_init_net(co_gw_t *gw, co_unsigned16_t id, co_nmt_t *nmt)
{
	assert(gw);

	if (__unlikely(co_gw_fini_net(gw, id) == -1))
		return -1;

	gw->net[id - 1] = co_gw_net_create(gw, id, nmt);
	return __likely(gw->net[id - 1]) ? 0 : -1;
}

LELY_CO_EXPORT int
co_gw_fini_net(co_gw_t *gw, co_unsigned16_t id)
{
	assert(gw);

	if (__unlikely(!id || id > CO_GW_NUM_NET)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	co_gw_net_destroy(gw->net[id - 1]);
	gw->net[id - 1] = NULL;

	return 0;
}

LELY_CO_EXPORT int
co_gw_recv(co_gw_t *gw, const struct co_gw_req *req)
{
	assert(gw);
	assert(req);

	if (__unlikely(req->size < sizeof(*req))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	switch (req->srv) {
	default:
		return co_gw_send_con(gw, req, CO_GW_IEC_BAD_SRV, 0);
	}
}

LELY_CO_EXPORT void
co_gw_get_send_func(const co_gw_t *gw, co_gw_send_func_t **pfunc, void **pdata)
{
	assert(gw);

	if (pfunc)
		*pfunc = gw->send_func;
	if (pdata)
		*pdata = gw->send_data;
}

LELY_CO_EXPORT void
co_gw_set_send_func(co_gw_t *gw, co_gw_send_func_t *func, void *data)
{
	assert(gw);

	gw->send_func = func;
	gw->send_data = data;
}

static struct co_gw_net *
co_gw_net_create(co_gw_t *gw, co_unsigned16_t id, co_nmt_t *nmt)
{
	assert(gw);
	assert(nmt);

	struct co_gw_net *net = malloc(sizeof(*net));
	if (__unlikely(!net)) {
		set_errno(errno);
		return NULL;
	}

	net->gw = gw;
	net->id = id;
	net->nmt = nmt;

	return net;
}

static void
co_gw_net_destroy(struct co_gw_net *net)
{
	if (net) {
		free(net);
	}
}

static int
co_gw_send_con(co_gw_t *gw, const struct co_gw_req *req, int iec,
		co_unsigned32_t ac)
{
	assert(req);

	// Convert SDO abort codes to their equivalent internal error codes
	// where possible.
	switch (ac) {
	case CO_SDO_AC_TIMEOUT:
		ac = 0;
		iec = CO_GW_IEC_TIMEOUT;
		break;
	case CO_SDO_AC_NO_MEM:
		ac = 0;
		iec = CO_GW_IEC_NO_MEM;
		break;
	case CO_SDO_AC_PDO_LEN:
		ac = 0;
		iec = CO_GW_IEC_PDO_LEN;
	}

	// Copy the service number and user-specified data from the request to
	// the confirmation.
	struct co_gw_con con = {
		.size = sizeof(con),
		.srv = req->srv,
		.data = req->data,
		.iec = iec,
		.ac = ac
	};
	return co_gw_send_srv(gw, (struct co_gw_srv *)&con);
}

static int
co_gw_send_srv(co_gw_t *gw, const struct co_gw_srv *srv)
{
	assert(gw);
	assert(srv);

	if (__unlikely(!gw->send_func)) {
		set_errnum(ERRNUM_NOSYS);
		return -1;
	}

	return gw->send_func(srv, gw->send_data) ? -1 : 0;
}

#endif // !LELY_NO_CO_GW

