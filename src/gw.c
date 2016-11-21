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
#include <lely/co/dev.h>
#ifndef LELY_NO_CO_EMCY
#include <lely/co/emcy.h>
#endif
#include <lely/co/gw.h>
#include <lely/co/nmt.h>
#include <lely/co/obj.h>
#ifndef LELY_NO_CO_RPDO
#include <lely/co/rpdo.h>
#endif
#include <lely/co/sdo.h>
#ifndef LELY_NO_CO_TPDO
#include <lely/co/tpdo.h>
#endif

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
	//! The default node-ID.
	co_unsigned8_t def;
	/*!
	 * A flag indicating whether "boot-up event received" commands should be
	 * forwarded (1) or not (0).
	 */
	unsigned bootup_ind:1;
	//! A pointer to the original NMT command indication function.
	co_nmt_cs_ind_t *cs_ind;
	//! A pointer to user-specified data for #cs_ind.
	void *cs_data;
#ifndef LELY_NO_CO_MASTER
	//! A pointer to the original node guarding event indication function.
	co_nmt_ng_ind_t *ng_ind;
	//! A pointer to user-specified data for #ng_ind.
	void *ng_data;
#endif
	//! A pointer to the original life guarding event indication function.
	co_nmt_lg_ind_t *lg_ind;
	//! A pointer to user-specified data for #lg_ind.
	void *lg_data;
	//! A pointer to the original heartbeat event indication function.
	co_nmt_hb_ind_t *hb_ind;
	//! A pointer to user-specified data for #hb_ind.
	void *hb_data;
	//! A pointer to the original state change event indication function.
	co_nmt_st_ind_t *st_ind;
	//! A pointer to user-specified data for #st_ind.
	void *st_data;
};

//! Creates a new CANopen network. \see co_gw_net_destroy()
static struct co_gw_net *co_gw_net_create(co_gw_t *gw, co_unsigned16_t id,
		co_nmt_t *nmt);

//! Destroys a CANopen network. \see co_gw_net_create()
static void co_gw_net_destroy(struct co_gw_net *net);

/*!
 * The callback function invoked when an NMT command is received by a CANopen
 * gateway.
 */
static void co_gw_net_cs_ind(co_nmt_t *nmt, co_unsigned8_t cs, void *data);
#ifndef LELY_NO_CO_MASTER
/*!
 * The callback function invoked when a node guarding event occurs for a node on
 * a CANopen network.
 */
static void co_gw_net_ng_ind(co_nmt_t *nmt, co_unsigned8_t id, int state,
		int reason, void *data);
#endif
/*!
 * The callback function invoked when a life guarding event occurs for a CANopen
 * gateway.
 */
static void co_gw_net_lg_ind(co_nmt_t *nmt, int state, void *data);
/*!
 * The callback function invoked when a heartbeat event occurs for a node on a
 * CANopen network.
 */
static void co_gw_net_hb_ind(co_nmt_t *nmt, co_unsigned8_t id, int state,
		int reason, void *data);
/*!
 * The callback function invoked when a boot-up event or state change is
 * detected for a node on a CANopen network.
 */
static void co_gw_net_st_ind(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned8_t st, void *data);
#ifndef LELY_NO_CO_EMCY
/*!
 * The callback function invoked when an EMCY message is received from node on a
 * CANopen network.
 */
static void co_gw_net_emcy_ind(co_emcy_t *emcy, co_unsigned8_t id,
		co_unsigned16_t ec, co_unsigned8_t er, uint8_t msef[5],
		void *data);
#endif
#ifndef LELY_NO_CO_RPDO
/*!
 * The callback function invoked when a PDO is received from a node on a CANopen
 * network.
 */
static void co_gw_net_rpdo_ind(co_rpdo_t *pdo, co_unsigned32_t ac,
		const void *ptr, size_t n, void *data);
#endif

//! A CANopen gateway.
struct __co_gw {
	//! An array of pointers to the CANopen networks.
	struct co_gw_net *net[CO_GW_NUM_NET];
	//! The command timeout (in milliseconds).
	int timeout;
	//! The default network-ID.
	co_unsigned16_t def;
	/*!
	 * A pointer to the callback function invoked when an indication or
	 * confirmation needs to be sent.
	 */
	co_gw_send_func_t *send_func;
	//! A pointer to the user-specified data for #send_func.
	void *send_data;
	/*!
	 * A pointer to the callback function invoked when a baudrate switch is
	 * needed after an 'Initialize gateway' command is received.
	 */
	co_gw_rate_func_t *rate_func;
	//! A pointer to the user-specified data for #rate_func.
	void *rate_data;
};

#ifndef LELY_NO_CO_RPDO
//! Processes a 'Configure RPDO' request.
static int co_gw_recv_set_rpdo(co_gw_t *gw, co_unsigned16_t net,
		const struct co_gw_req *req);
#endif
#ifndef LELY_NO_CO_TPDO
//! Processes a 'Configure TPDO' request.
static int co_gw_recv_set_tpdo(co_gw_t *gw, co_unsigned16_t net,
		const struct co_gw_req *req);
#endif
#ifndef LELY_NO_CO_RPDO
//! Processes a 'Read PDO data' request.
static int co_gw_recv_pdo_read(co_gw_t *gw, co_unsigned16_t net,
		const struct co_gw_req *req);
#endif
#ifndef LELY_NO_CO_TPDO
//! Processes a 'Write PDO data' request.
static int co_gw_recv_pdo_write(co_gw_t *gw, co_unsigned16_t net,
		const struct co_gw_req *req);
#endif

#ifndef LELY_NO_CO_MASTER
//! Processes an NMT request.
static int co_gw_recv_nmt_cs(co_gw_t *gw, co_unsigned16_t net,
		co_unsigned8_t node, co_unsigned8_t cs,
		const struct co_gw_req *req);
//! Processes a 'Enable/Disable node guarding' request.
static int co_gw_recv_nmt_set_ng(co_gw_t *gw, co_unsigned16_t net,
		co_unsigned8_t node, const struct co_gw_req *req);
#endif
//! Processes a 'Start/Disable heartbeat consumer' request.
static int co_gw_recv_nmt_set_hb(co_gw_t *gw, co_unsigned16_t net,
		co_unsigned8_t node, const struct co_gw_req *req);

//! Processes an 'Initialize gateway' request.
static int co_gw_recv_init(co_gw_t *gw, co_unsigned16_t net,
		const struct co_gw_req *req);
//! Processes a 'Set heartbeat producer' request.
static int co_gw_recv_set_hb(co_gw_t *gw, co_unsigned16_t net,
		const struct co_gw_req *req);
//! Processes a 'Set node-ID' request.
static int co_gw_recv_set_id(co_gw_t *gw, co_unsigned16_t net,
		const struct co_gw_req *req);
#ifndef LELY_NO_CO_EMCY
//! Processes a 'Start/Stop emergency consumer' request.
static int co_gw_recv_set_emcy(co_gw_t *gw, co_unsigned16_t net,
		co_unsigned8_t node, const struct co_gw_req *req);
#endif
//! Processes a 'Set command time-out' request.
static int co_gw_recv_set_cmd_timeout(co_gw_t *gw, const struct co_gw_req *req);
//! Processes a 'Boot-up forwarding' request.
static int co_gw_recv_set_bootup_ind(co_gw_t *gw, co_unsigned16_t net,
		const struct co_gw_req *req);

//! Processes a 'Set default network' request.
static int co_gw_recv_set_net(co_gw_t *gw, const struct co_gw_req *req);
//! Processes a 'Set default node-ID' request.
static int co_gw_recv_set_node(co_gw_t *gw, co_unsigned16_t net,
		const struct co_gw_req *req);
//! Processes a 'Get version' request.
static int co_gw_recv_get_version(co_gw_t *gw, co_unsigned16_t net,
		const struct co_gw_req *req);

//! Sends a confirmation with an internal error code or SDO abort code.
static int co_gw_send_con(co_gw_t *gw, const struct co_gw_req *req, int iec,
		co_unsigned32_t ac);
//! Sends an 'Error control event received' indication.
static int co_gw_send_ec(co_gw_t *gw, co_unsigned16_t net, co_unsigned8_t node,
		co_unsigned8_t st, int iec);
//! Invokes the callback function to send a confirmation or indication.
static int co_gw_send_srv(co_gw_t *gw, const struct co_gw_srv *srv);

//! Converts an error number to an internal error code.
static inline int errnum2iec(errnum_t errnum);

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

	gw->timeout = 0;
	gw->def = 0;

	gw->send_func = NULL;
	gw->send_data = NULL;

	gw->rate_func = NULL;
	gw->rate_data = NULL;

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

	int iec = 0;

	// Obtain the network-ID for node- and network-level requests. If the
	// network-ID is 0, use the default ID.
	co_unsigned16_t net = gw->def;
	switch (req->srv) {
#ifndef LELY_NO_CO_RPDO
	case CO_GW_SRV_SET_RPDO:
#endif
#ifndef LELY_NO_CO_TPDO
	case CO_GW_SRV_SET_TPDO:
#endif
#ifndef LELY_NO_CO_RPDO
	case CO_GW_SRV_PDO_READ:
#endif
#ifndef LELY_NO_CO_TPDO
	case CO_GW_SRV_PDO_WRITE:
#endif
#ifndef LELY_NO_CO_MASTER
	case CO_GW_SRV_NMT_START:
	case CO_GW_SRV_NMT_STOP:
	case CO_GW_SRV_NMT_ENTER_PREOP:
	case CO_GW_SRV_NMT_RESET_NODE:
	case CO_GW_SRV_NMT_RESET_COMM:
	case CO_GW_SRV_NMT_NG_ENABLE:
	case CO_GW_SRV_NMT_NG_DISABLE:
#endif
	case CO_GW_SRV_NMT_HB_ENABLE:
	case CO_GW_SRV_NMT_HB_DISABLE:
	case CO_GW_SRV_INIT:
	case CO_GW_SRV_SET_HB:
	case CO_GW_SRV_SET_ID:
#ifndef LELY_NO_CO_EMCY
	case CO_GW_SRV_EMCY_START:
	case CO_GW_SRV_EMCY_STOP:
#endif
	case CO_GW_SRV_SET_BOOTUP_IND:
	case CO_GW_SRV_SET_NODE:
	case CO_GW_SRV_GET_VERSION:
		if (__unlikely(req->size < sizeof(struct co_gw_req_net))) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		struct co_gw_req_net *par = (struct co_gw_req_net *)req;
		if (par->net)
			net = par->net;
		if (__unlikely(!net)) {
			iec = CO_GW_IEC_NO_DEF_NET;
			goto error;
		} else if (__unlikely(net > CO_GW_NUM_NET
				|| !gw->net[net - 1])) {
			iec = CO_GW_IEC_BAD_NET;
			goto error;
		}
		break;
	}
	assert(net <= CO_GW_NUM_NET);
	assert(!net || gw->net[net - 1]);

	// Obtain the node-ID for node-level requests. If the node-ID is 0xff,
	// use the default ID.
	co_unsigned8_t node = net ? gw->net[net - 1]->def : 0;
	switch (req->srv) {
#ifndef LELY_NO_CO_MASTER
	case CO_GW_SRV_NMT_START:
	case CO_GW_SRV_NMT_STOP:
	case CO_GW_SRV_NMT_ENTER_PREOP:
	case CO_GW_SRV_NMT_RESET_NODE:
	case CO_GW_SRV_NMT_RESET_COMM:
	case CO_GW_SRV_NMT_NG_ENABLE:
	case CO_GW_SRV_NMT_NG_DISABLE:
#endif
	case CO_GW_SRV_NMT_HB_ENABLE:
	case CO_GW_SRV_NMT_HB_DISABLE:
#ifndef LELY_NO_CO_EMCY
	case CO_GW_SRV_EMCY_START:
	case CO_GW_SRV_EMCY_STOP:
#endif
		if (__unlikely(req->size < sizeof(struct co_gw_req_node))) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		struct co_gw_req_node *par = (struct co_gw_req_node *)req;
		if (par->node != 0xff)
			node = par->node;
		if (__unlikely(node > CO_NUM_NODES)) {
			iec = CO_GW_IEC_BAD_NODE;
			goto error;
		}
		break;
	}

	// Except for the NMT commands, node-level request require a non-zero
	// node-ID.
	switch (req->srv) {
#ifndef LELY_NO_CO_MASTER
	case CO_GW_SRV_NMT_NG_ENABLE:
	case CO_GW_SRV_NMT_NG_DISABLE:
#endif
	case CO_GW_SRV_NMT_HB_ENABLE:
	case CO_GW_SRV_NMT_HB_DISABLE:
#ifndef LELY_NO_CO_EMCY
	case CO_GW_SRV_EMCY_START:
	case CO_GW_SRV_EMCY_STOP:
#endif
		if (__unlikely(!node)) {
			iec = CO_GW_IEC_NO_DEF_NODE;
			goto error;
		}
	}

	switch (req->srv) {
#ifndef LELY_NO_CO_RPDO
	case CO_GW_SRV_SET_RPDO:
		return co_gw_recv_set_rpdo(gw, net, req);
#endif
#ifndef LELY_NO_CO_TPDO
	case CO_GW_SRV_SET_TPDO:
		return co_gw_recv_set_tpdo(gw, net, req);
#endif
#ifndef LELY_NO_CO_RPDO
	case CO_GW_SRV_PDO_READ:
		return co_gw_recv_pdo_read(gw, net, req);
#endif
#ifndef LELY_NO_CO_TPDO
	case CO_GW_SRV_PDO_WRITE:
		return co_gw_recv_pdo_write(gw, net, req);
#endif
#ifndef LELY_NO_CO_MASTER
	case CO_GW_SRV_NMT_START:
		return co_gw_recv_nmt_cs(gw, net, node, CO_NMT_CS_START, req);
	case CO_GW_SRV_NMT_STOP:
		return co_gw_recv_nmt_cs(gw, net, node, CO_NMT_CS_STOP, req);
	case CO_GW_SRV_NMT_ENTER_PREOP:
		return co_gw_recv_nmt_cs(gw, net, node, CO_NMT_CS_ENTER_PREOP,
				req);
	case CO_GW_SRV_NMT_RESET_NODE:
		return co_gw_recv_nmt_cs(gw, net, node, CO_NMT_CS_RESET_NODE,
				req);
	case CO_GW_SRV_NMT_RESET_COMM:
		return co_gw_recv_nmt_cs(gw, net, node, CO_NMT_CS_RESET_COMM,
				req);
	case CO_GW_SRV_NMT_NG_ENABLE:
	case CO_GW_SRV_NMT_NG_DISABLE:
		return co_gw_recv_nmt_set_ng(gw, net, node, req);
#endif
	case CO_GW_SRV_NMT_HB_ENABLE:
	case CO_GW_SRV_NMT_HB_DISABLE:
		return co_gw_recv_nmt_set_hb(gw, net, node, req);
	case CO_GW_SRV_INIT:
		return co_gw_recv_init(gw, net, req);
	case CO_GW_SRV_SET_HB:
		return co_gw_recv_set_hb(gw, net, req);
	case CO_GW_SRV_SET_ID:
		return co_gw_recv_set_id(gw, net, req);
#ifndef LELY_NO_CO_EMCY
	case CO_GW_SRV_EMCY_START:
	case CO_GW_SRV_EMCY_STOP:
		return co_gw_recv_set_emcy(gw, net, node, req);
#endif
	case CO_GW_SRV_SET_CMD_TIMEOUT:
		return co_gw_recv_set_cmd_timeout(gw, req);
	case CO_GW_SRV_SET_BOOTUP_IND:
		return co_gw_recv_set_bootup_ind(gw, net, req);
	case CO_GW_SRV_SET_NET:
		return co_gw_recv_set_net(gw, req);
	case CO_GW_SRV_SET_NODE:
		return co_gw_recv_set_node(gw, net, req);
	case CO_GW_SRV_GET_VERSION:
		return co_gw_recv_get_version(gw, net, req);
	case CO_GW_SRV_SET_CMD_SIZE:
		// We cannot guarantee a lack of memory resources will never
		// occur.
		return co_gw_send_con(gw, req, CO_GW_IEC_NO_MEM, 0);
	default:
		iec = CO_GW_IEC_BAD_SRV;
		goto error;
	}

error:
	return co_gw_send_con(gw, req, iec, 0);
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

LELY_CO_EXPORT void
co_gw_get_rate_func(const co_gw_t *gw, co_gw_rate_func_t **pfunc, void **pdata)
{
	assert(gw);

	if (pfunc)
		*pfunc = gw->rate_func;
	if (pdata)
		*pdata = gw->rate_data;
}

LELY_CO_EXPORT void
co_gw_set_rate_func(co_gw_t *gw, co_gw_rate_func_t *func, void *data)
{
	assert(gw);

	gw->rate_func = func;
	gw->rate_data = data;
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

	net->def = 0;
	net->bootup_ind = 1;

	co_nmt_get_cs_ind(net->nmt, &net->cs_ind, &net->cs_data);
	co_nmt_set_cs_ind(net->nmt, &co_gw_net_cs_ind, net);
#ifndef LELY_NO_CO_MASTER
	co_nmt_get_ng_ind(net->nmt, &net->ng_ind, &net->ng_data);
	co_nmt_set_ng_ind(net->nmt, &co_gw_net_ng_ind, net);
#endif
	co_nmt_get_lg_ind(net->nmt, &net->lg_ind, &net->lg_data);
	co_nmt_set_lg_ind(net->nmt, &co_gw_net_lg_ind, net);
	co_nmt_get_hb_ind(net->nmt, &net->hb_ind, &net->hb_data);
	co_nmt_set_hb_ind(net->nmt, &co_gw_net_hb_ind, net);
	co_nmt_get_st_ind(net->nmt, &net->st_ind, &net->st_data);
	co_nmt_set_st_ind(net->nmt, &co_gw_net_st_ind, net);

#ifndef LELY_NO_CO_RPDO
	if (co_nmt_get_st(net->nmt) == CO_NMT_ST_START) {
		for (co_unsigned16_t i = 1; i <= 512; i++) {
			co_rpdo_t *pdo = co_nmt_get_rpdo(nmt, i);
			if (pdo)
				co_rpdo_set_ind(pdo, &co_gw_net_rpdo_ind, net);
		}
	}
#endif

	return net;
}

static void
co_gw_net_destroy(struct co_gw_net *net)
{
	if (net) {
#ifndef LELY_NO_CO_RPDO
		for (co_unsigned16_t i = 1; i <= 512; i++) {
			co_rpdo_t *pdo = co_nmt_get_rpdo(net->nmt, i);
			if (pdo)
				co_rpdo_set_ind(pdo, NULL, NULL);
		}
#endif

#ifndef LELY_NO_CO_EMCY
		co_emcy_t *emcy = co_nmt_get_emcy(net->nmt);
		if (emcy)
			co_emcy_set_ind(emcy, NULL, NULL);
#endif

		co_nmt_set_st_ind(net->nmt, net->st_ind, net->st_data);
		co_nmt_set_hb_ind(net->nmt, net->hb_ind, net->hb_data);
		co_nmt_set_lg_ind(net->nmt, net->lg_ind, net->lg_data);
#ifndef LELY_NO_CO_MASTER
		co_nmt_set_ng_ind(net->nmt, net->ng_ind, net->ng_data);
#endif
		co_nmt_set_cs_ind(net->nmt, net->cs_ind, net->cs_data);

		free(net);
	}
}

static void
co_gw_net_cs_ind(co_nmt_t *nmt, co_unsigned8_t cs, void *data)
{
	struct co_gw_net *net = data;
	assert(net);

	switch (cs) {
	case CO_NMT_CS_START: {
#ifndef LELY_NO_CO_EMCY
		co_emcy_t *emcy = co_nmt_get_emcy(nmt);
		if (emcy)
			co_emcy_set_ind(emcy, &co_gw_net_emcy_ind, net);
#endif

#ifndef LELY_NO_CO_RPDO
		for (co_unsigned16_t i = 1; i <= 512; i++) {
			co_rpdo_t *pdo = co_nmt_get_rpdo(nmt, i);
			if (pdo)
				co_rpdo_set_ind(pdo, &co_gw_net_rpdo_ind, net);
		}
#endif
		break;
	}
	case CO_NMT_CS_ENTER_PREOP: {
#ifndef LELY_NO_CO_EMCY
		co_emcy_t *emcy = co_nmt_get_emcy(nmt);
		if (emcy)
			co_emcy_set_ind(emcy, &co_gw_net_emcy_ind, net);
#endif
		break;
	}
	}

	if (net->cs_ind)
		net->cs_ind(nmt, cs, net->cs_data);
}

#ifndef LELY_NO_CO_MASTER
static void
co_gw_net_ng_ind(co_nmt_t *nmt, co_unsigned8_t id, int state, int reason,
		void *data)
{
	struct co_gw_net *net = data;
	assert(net);

	if (state == CO_NMT_EC_OCCURRED) {
		int iec = 0;
		switch (state) {
		case CO_NMT_EC_TIMEOUT: iec = CO_GW_IEC_NG_OCCURRED; break;
		case CO_NMT_EC_STATE: iec = CO_GW_IEC_ST_OCCURRED; break;
		}
		co_gw_send_ec(net->gw, net->id, id, 0, iec);
	}

	if (net->ng_ind)
		net->ng_ind(nmt, id, state, reason, net->ng_data);
}
#endif

static void
co_gw_net_lg_ind(co_nmt_t *nmt, int state, void *data)
{
	struct co_gw_net *net = data;
	assert(net);

	co_dev_t *dev = co_nmt_get_dev(nmt);

	co_unsigned8_t id = co_dev_get_id(dev);
	co_gw_send_ec(net->gw, net->id, id, 0, CO_GW_IEC_LG_OCCURRED);

	if (net->lg_ind)
		net->lg_ind(nmt, state, net->lg_data);
}

static void
co_gw_net_hb_ind(co_nmt_t *nmt, co_unsigned8_t id, int state, int reason,
		void *data)
{
	struct co_gw_net *net = data;
	assert(net);

	if (reason == CO_NMT_EC_TIMEOUT) {
		int iec = 0;
		switch (state) {
		case CO_NMT_EC_OCCURRED: iec = CO_GW_IEC_HB_OCCURRED; break;
		case CO_NMT_EC_RESOLVED: iec = CO_GW_IEC_HB_RESOLVED; break;
		}
		co_gw_send_ec(net->gw, net->id, id, 0, iec);
	}

	if (net->hb_ind)
		net->hb_ind(nmt, id, state, reason, net->hb_data);
}

static void
co_gw_net_st_ind(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st,
		void *data)
{
	struct co_gw_net *net = data;
	assert(net);

	co_dev_t *dev = co_nmt_get_dev(nmt);

	// Ignore state change indications of the gateway itself.
	if (id == co_dev_get_id(dev))
		return;

	if (st == CO_NMT_ST_BOOTUP && !net->bootup_ind)
		return;

	co_gw_send_ec(net->gw, net->id, id, st,
			st == CO_NMT_ST_BOOTUP ? CO_GW_IEC_BOOTUP : 0);

	if (net->st_ind)
		net->st_ind(nmt, id, st, net->st_data);
}

#ifndef LELY_NO_CO_EMCY
static void
co_gw_net_emcy_ind(co_emcy_t *emcy, co_unsigned8_t id, co_unsigned16_t ec,
		co_unsigned8_t er, uint8_t msef[5], void *data)
{
	__unused_var(emcy);
	struct co_gw_net *net = data;
	assert(net);

	struct co_gw_ind_emcy ind = {
		.size = sizeof(ind),
		.srv = CO_GW_SRV_EMCY,
		.net = net->id,
		.node = id,
		.ec = ec,
		.er = er,
		.msef = { msef[0], msef[1], msef[2], msef[3], msef[4] }
	};
	co_gw_send_srv(net->gw, (struct co_gw_srv *)&ind);
}
#endif

#ifndef LELY_NO_CO_RPDO
static void
co_gw_net_rpdo_ind(co_rpdo_t *pdo, co_unsigned32_t ac, const void *ptr,
		size_t n, void *data)
{
	struct co_gw_net *net = data;
	assert(net);

	if (__unlikely(ac))
		return;

	struct co_gw_ind_rpdo ind = {
		.size = CO_GW_IND_RPDO_SIZE,
		.srv = CO_GW_SRV_RPDO,
		.net = net->id,
		.num = co_rpdo_get_num(pdo),
		.n = 0x40
	};

	const struct co_pdo_map_par *par = co_rpdo_get_map_par(pdo);
	if (__unlikely(co_pdo_unmap(par, ptr, n, ind.val, &ind.n)))
		return;
	ind.size += ind.n * sizeof(*ind.val);

	co_gw_send_srv(net->gw, (struct co_gw_srv *)&ind);
}
#endif

#ifndef LELY_NO_CO_RPDO
static int
co_gw_recv_set_rpdo(co_gw_t *gw, co_unsigned16_t net,
		const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_SET_RPDO);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_dev_t *dev = co_nmt_get_dev(nmt);

	if (__unlikely(req->size < CO_GW_REQ_SET_RPDO_SIZE)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_set_rpdo *par =
			(const struct co_gw_req_set_rpdo *)req;
	if (__unlikely(par->n > 0x40 || par->size < CO_GW_REQ_SET_RPDO_SIZE
			+ par->n * sizeof(*par->map))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	struct co_pdo_comm_par comm = {
		.n = 2,
		.cobid = par->cobid,
		.trans = par->trans
	};

	struct co_pdo_map_par map = CO_PDO_MAP_PAR_INIT;
	map.n = par->n;
	for (co_unsigned8_t i = 0; i < par->n; i++)
		map.map[i] = par->map[i];

	co_unsigned32_t ac = co_dev_cfg_rpdo(dev, par->num, &comm, &map);

	return co_gw_send_con(gw, req, 0, ac);
}
#endif

#ifndef LELY_NO_CO_TPDO
static int
co_gw_recv_set_tpdo(co_gw_t *gw, co_unsigned16_t net,
		const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_SET_TPDO);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_dev_t *dev = co_nmt_get_dev(nmt);

	if (__unlikely(req->size < CO_GW_REQ_SET_TPDO_SIZE)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_set_tpdo *par =
			(const struct co_gw_req_set_tpdo *)req;
	if (__unlikely(par->n > 0x40 || par->size < CO_GW_REQ_SET_TPDO_SIZE
			+ par->n * sizeof(*par->map))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	struct co_pdo_comm_par comm = {
		.n = 6,
		.cobid = par->cobid,
		.trans = par->trans,
		.inhibit = par->inhibit,
		.event = par->event,
		.sync = par->sync
	};

	struct co_pdo_map_par map = CO_PDO_MAP_PAR_INIT;
	map.n = par->n;
	for (co_unsigned8_t i = 0; i < par->n; i++)
		map.map[i] = par->map[i];

	co_unsigned32_t ac = co_dev_cfg_tpdo(dev, par->num, &comm, &map);

	return co_gw_send_con(gw, req, 0, ac);
}
#endif

#ifndef LELY_NO_CO_RPDO
static int
co_gw_recv_pdo_read(co_gw_t *gw, co_unsigned16_t net,
		const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_PDO_READ);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_dev_t *dev = co_nmt_get_dev(nmt);

	if (__unlikely(req->size < sizeof(struct co_gw_req_pdo_read))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_pdo_read *par =
			(const struct co_gw_req_pdo_read *)req;

	int iec = 0;
	co_unsigned32_t ac = 0;

	co_rpdo_t *pdo = co_nmt_get_rpdo(nmt, par->num);
	if (__unlikely(!pdo)) {
		iec = CO_GW_IEC_INTERN;
		goto error;
	}
	const struct co_pdo_comm_par *comm = co_rpdo_get_comm_par(pdo);
	if (comm->trans == 0xfc || comm->trans == 0xfd) {
		// TODO(jseldenthuis@lely.com): Send an RTR and wait for the
		// result.
		iec = CO_GW_IEC_INTERN;
		goto error;
	}
	const struct co_pdo_map_par *map = co_rpdo_get_map_par(pdo);

	// Read the mapped values from the object dictionary.
	struct co_sdo_req sdo_req = CO_SDO_REQ_INIT;
	uint8_t buf[CAN_MAX_LEN];
	size_t n = sizeof(buf);
	ac = co_pdo_up(map, dev, &sdo_req, buf, &n);
	co_sdo_req_fini(&sdo_req);
	if (__unlikely(ac))
		goto error;

	struct co_gw_con_pdo_read con = {
		.size = sizeof(con),
		.srv = req->srv,
		.data = req->data,
		.net = net,
		.num = par->num
	};

	// Unmap the PDO values.
	ac = co_pdo_unmap(map, buf, n, con.val, &con.n);
	if (__unlikely(ac))
		goto error;

	con.size += con.n * sizeof(*con.val);
	return co_gw_send_srv(gw, (struct co_gw_srv *)&con);

error:
	return co_gw_send_con(gw, req, iec, ac);
}
#endif

#ifndef LELY_NO_CO_TPDO
static int
co_gw_recv_pdo_write(co_gw_t *gw, co_unsigned16_t net,
		const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_PDO_WRITE);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_dev_t *dev = co_nmt_get_dev(nmt);

	if (__unlikely(req->size < CO_GW_REQ_PDO_WRITE_SIZE)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_pdo_write *par =
			(const struct co_gw_req_pdo_write *)req;
	if (__unlikely(par->n > 0x40 || par->size < CO_GW_REQ_PDO_WRITE_SIZE
			+ par->n * sizeof(*par->val))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	int iec = 0;
	co_unsigned32_t ac = 0;

	co_tpdo_t *pdo = co_nmt_get_tpdo(nmt, par->num);
	if (__unlikely(!pdo)) {
		iec = CO_GW_IEC_INTERN;
		goto error;
	}
	const struct co_pdo_map_par *map = co_tpdo_get_map_par(pdo);

	// Map the values into a PDO.
	uint8_t buf[CAN_MAX_LEN];
	size_t n = sizeof(buf);
	ac = co_pdo_map(map, par->val, par->n, buf, &n);
	if (__unlikely(ac))
		goto error;

	// Write the mapped values to the object dictionary.
	struct co_sdo_req sdo_req = CO_SDO_REQ_INIT;
	ac = co_pdo_dn(map, dev, &sdo_req, buf, n);
	co_sdo_req_fini(&sdo_req);
	if (__unlikely(ac))
		goto error;

	// Trigger the event-based TPDO, if necessary.
	errc_t errc = 0;
	if (__unlikely(co_tpdo_event(pdo) == -1)) {
		iec = errnum2iec(get_errnum());
		set_errc(errc);
	}

error:
	return co_gw_send_con(gw, req, iec, ac);
}
#endif

#ifndef LELY_NO_CO_MASTER

static int
co_gw_recv_nmt_cs(co_gw_t *gw, co_unsigned16_t net, co_unsigned8_t node,
		co_unsigned8_t cs, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;

	int iec = 0;

	errc_t errc = get_errc();
	if (__unlikely(co_nmt_cs_req(nmt, cs, node) == -1)) {
		iec = errnum2iec(get_errnum());
		set_errc(errc);
	}

	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv_nmt_set_ng(co_gw_t *gw, co_unsigned16_t net, co_unsigned8_t node,
		const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;

	co_unsigned16_t gt = 0;
	co_unsigned8_t ltf = 0;
	if (req->srv == CO_GW_SRV_NMT_NG_ENABLE) {
		if (__unlikely(req->size
				< sizeof(struct co_gw_req_nmt_set_ng))) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		const struct co_gw_req_nmt_set_ng *par =
				(const struct co_gw_req_nmt_set_ng *)req;

		gt = par->gt;
		ltf = par->ltf;
	}

	int iec = 0;

	errc_t errc = get_errc();
	if (__unlikely(co_nmt_ng_req(nmt, node, gt, ltf) == -1)) {
		iec = errnum2iec(get_errnum());
		set_errc(errc);
	}

	return co_gw_send_con(gw, req, iec, 0);
}

#endif // !LELY_NO_CO_MASTER

static int
co_gw_recv_nmt_set_hb(co_gw_t *gw, co_unsigned16_t net, co_unsigned8_t node,
		const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_dev_t *dev = co_nmt_get_dev(nmt);

	co_unsigned16_t ms = 0;
	if (req->srv == CO_GW_SRV_NMT_HB_ENABLE) {
		if (__unlikely(req->size
				< sizeof(struct co_gw_req_nmt_set_hb))) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		const struct co_gw_req_nmt_set_hb *par =
				(const struct co_gw_req_nmt_set_hb *)req;

		ms = par->ms;
	}

	co_unsigned32_t ac = co_dev_cfg_hb(dev, node, ms);

	return co_gw_send_con(gw, req, 0, ac);
}

static int
co_gw_recv_init(co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_INIT);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_dev_t *dev = co_nmt_get_dev(nmt);

	if (__unlikely(req->size < sizeof(struct co_gw_req_init))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_init *par = (const struct co_gw_req_init *)req;

	int iec = 0;

	unsigned int baud = co_dev_get_baud(dev);
	co_unsigned16_t rate;
	switch (par->bitidx) {
	case 0:
		if (__unlikely(!(baud & CO_BAUD_1000))) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 1000;
		break;
	case 1:
		if (__unlikely(!(baud & CO_BAUD_800))) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 800;
		break;
	case 2:
		if (__unlikely(!(baud & CO_BAUD_500))) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 500;
		break;
	case 3:
		if (__unlikely(!(baud & CO_BAUD_250))) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 250;
		break;
	case 4:
		if (__unlikely(!(baud & CO_BAUD_125))) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 125;
		break;
	case 6:
		if (__unlikely(!(baud & CO_BAUD_50))) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 50;
		break;
	case 7:
		if (__unlikely(!(baud & CO_BAUD_20))) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 20;
		break;
	case 8:
		if (__unlikely(!(baud & CO_BAUD_10))) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 10;
		break;
	case 9:
		if (__unlikely(!(baud & CO_BAUD_AUTO))) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 0;
		break;
	default:
		iec = CO_GW_IEC_LSS_RATE;
		goto error;
	}
	if (gw->rate_func)
		gw->rate_func(net, rate, gw->rate_data);

	errc_t errc = get_errc();
	if (__unlikely(co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE) == -1)) {
		iec = errnum2iec(get_errnum());
		set_errc(errc);
	}

error:
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv_set_hb(co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_SET_HB);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_dev_t *dev = co_nmt_get_dev(nmt);

	if (__unlikely(req->size < sizeof(struct co_gw_req_set_hb))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_set_hb *par =
			(const struct co_gw_req_set_hb *)req;

	co_unsigned32_t ac = 0;

	co_obj_t *obj = co_dev_find_obj(dev, 0x1017);
	if (__unlikely(!obj)) {
		ac = CO_SDO_AC_NO_OBJ;
		goto error;
	}
	co_sub_t *sub = co_obj_find_sub(obj, 0x00);
	if (__unlikely(!sub)) {
		ac = CO_SDO_AC_NO_SUB;
		goto error;
	}
	ac = co_sub_dn_ind_val(sub, CO_DEFTYPE_UNSIGNED16, &par->ms);

error:
	return co_gw_send_con(gw, req, 0, ac);
}

static int
co_gw_recv_set_id(co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_SET_ID);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;

	if (__unlikely(req->size < sizeof(struct co_gw_req_node))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_node *par = (const struct co_gw_req_node *)req;

	int iec = 0;

	if (__unlikely(!par->node
			|| (par->node > CO_NUM_NODES && par->node != 0xff))) {
		iec = CO_GW_IEC_BAD_NODE;
		goto error;
	}

	co_nmt_set_id(nmt, par->node);

error:
	return co_gw_send_con(gw, req, iec, 0);
}

#ifndef LELY_NO_CO_EMCY
static int
co_gw_recv_set_emcy(co_gw_t *gw, co_unsigned16_t net, co_unsigned8_t node,
		const struct co_gw_req *req)
{
	assert(gw);
	assert(req);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_dev_t *dev = co_nmt_get_dev(nmt);

	if (__unlikely(req->size < sizeof(struct co_gw_req_set_emcy))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_set_emcy *par =
			(const struct co_gw_req_set_emcy *)req;

	co_unsigned32_t ac = 0;

	co_unsigned32_t cobid = par->cobid;
	if (par->srv == CO_GW_SRV_EMCY_START)
		cobid &= ~CO_EMCY_COBID_VALID;
	else
		cobid |= CO_EMCY_COBID_VALID;

	co_obj_t *obj = co_dev_find_obj(dev, 0x1028);
	if (__unlikely(!obj)) {
		ac = CO_SDO_AC_NO_OBJ;
		goto error;
	}
	co_sub_t *sub = co_obj_find_sub(obj, node);
	if (__unlikely(!sub)) {
		ac = CO_SDO_AC_NO_SUB;
		goto error;
	}
	ac = co_sub_dn_ind_val(sub, CO_DEFTYPE_UNSIGNED32, &cobid);

error:
	return co_gw_send_con(gw, req, 0, ac);
}
#endif

static int
co_gw_recv_set_cmd_timeout(co_gw_t *gw, const struct co_gw_req *req)
{
	assert(gw);
	assert(req);
	assert(req->srv == CO_GW_SRV_SET_CMD_TIMEOUT);

	if (__unlikely(req->size < sizeof(struct co_gw_req_set_cmd_timeout))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_set_cmd_timeout *par =
			(const struct co_gw_req_set_cmd_timeout *)req;

	gw->timeout = par->timeout;

	return co_gw_send_con(gw, req, 0, 0);
}

static int
co_gw_recv_set_bootup_ind(co_gw_t *gw, co_unsigned16_t net,
		const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_SET_BOOTUP_IND);

	if (__unlikely(req->size < sizeof(struct co_gw_req_set_bootup_ind))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_set_bootup_ind *par =
			(const struct co_gw_req_set_bootup_ind *)req;

	gw->net[net - 1]->bootup_ind = par->cs;

	return co_gw_send_con(gw, req, 0, 0);
}

static int
co_gw_recv_set_net(co_gw_t *gw, const struct co_gw_req *req)
{
	assert(gw);
	assert(req);
	assert(req->srv == CO_GW_SRV_SET_NET);

	if (__unlikely(req->size < sizeof(struct co_gw_req_net))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_net *par = (const struct co_gw_req_net *)req;

	int iec = 0;

	if (__unlikely(par->net > CO_GW_NUM_NET)) {
		iec = CO_GW_IEC_BAD_NET;
		goto error;
	}

	gw->def = par->net;

error:
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv_set_node(co_gw_t *gw, co_unsigned16_t net,
		const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_SET_NODE);

	if (__unlikely(req->size < sizeof(struct co_gw_req_node))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_node *par = (const struct co_gw_req_node *)req;

	int iec = 0;

	if (__unlikely(par->node > CO_NUM_NODES)) {
		iec = CO_GW_IEC_BAD_NODE;
		goto error;
	}

	gw->net[net - 1]->def = par->node;

error:
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv_get_version(co_gw_t *gw, co_unsigned16_t net,
		const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_GET_VERSION);

	co_dev_t *dev = co_nmt_get_dev(gw->net[net - 1]->nmt);
	struct co_gw_con_get_version con = {
		.size = sizeof(con),
		.srv = req->srv,
		.data = req->data,
		.vendor_id = co_dev_get_val_u32(dev, 0x1018, 0x01),
		.product_code = co_dev_get_val_u32(dev, 0x1018, 0x02),
		.revision = co_dev_get_val_u32(dev, 0x1018, 0x03),
		.serial_nr = co_dev_get_val_u32(dev, 0x1018, 0x04),
		.gw_class = co_nmt_is_master(gw->net[net - 1]->nmt) ? 3 : 1,
		.prot_hi = CO_GW_PROT_HI,
		.prot_lo = CO_GW_PROT_LO
	};
	return co_gw_send_srv(gw, (struct co_gw_srv *)&con);
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
co_gw_send_ec(co_gw_t *gw, co_unsigned16_t net, co_unsigned8_t node,
		co_unsigned8_t st, int iec)
{
	struct co_gw_ind_ec ind = {
		.size = sizeof(ind),
		.srv = CO_GW_SRV_EC,
		.net = net,
		.node = node,
		.st = st,
		.iec = iec
	};
	return co_gw_send_srv(gw, (struct co_gw_srv *)&ind);
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

static inline int
errnum2iec(errnum_t errnum)
{
	switch (errnum) {
	case 0: return 0;
	case ERRNUM_INVAL: return CO_GW_IEC_SYNTAX;
	case ERRNUM_NOMEM: return CO_GW_IEC_NO_MEM;
	case ERRNUM_PERM: return CO_GW_IEC_BAD_SRV;
	default: return CO_GW_IEC_INTERN;
	}
}

#endif // !LELY_NO_CO_GW

