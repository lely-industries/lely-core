/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the gateway functions.
 *
 * @see lely/co/gw.h
 *
 * @copyright 2021 Lely Industries N.V.
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

#if !LELY_NO_CO_GW

#include <lely/co/csdo.h>
#include <lely/co/dev.h>
#include <lely/util/errnum.h>
#if !LELY_NO_CO_EMCY
#include <lely/co/emcy.h>
#endif
#include <lely/co/gw.h>
#if !LELY_NO_CO_MASTER && !LELY_NO_CO_LSS
#include <lely/co/lss.h>
#endif
#include <lely/co/nmt.h>
#include <lely/co/obj.h>
#if !LELY_NO_CO_RPDO
#include <lely/co/rpdo.h>
#endif
#include <lely/co/sdo.h>
#if !LELY_NO_CO_SYNC
#include <lely/co/sync.h>
#endif
#if !LELY_NO_CO_TIME
#include <lely/co/time.h>
#endif
#if !LELY_NO_CO_TPDO
#include <lely/co/tpdo.h>
#endif

#include <assert.h>
#include <stdlib.h>

struct co_gw_job;

/// A CANopen network.
struct co_gw_net {
	/// A pointer to the CANopen gateway.
	co_gw_t *gw;
	/// The network-ID.
	co_unsigned16_t id;
	/// A pointer to a CANopen NMT master/slave service.
	co_nmt_t *nmt;
	/// The default node-ID.
	co_unsigned8_t def;
#if !LELY_NO_CO_CSDO
	/// The SDO timeout (in milliseconds).
	int timeout;
#endif
	/**
	 * A flag indicating whether "boot-up event received" commands should be
	 * forwarded (1) or not (0).
	 */
	unsigned bootup_ind : 1;
#if !LELY_NO_CO_CSDO
	/// An array of pointers to the SDO upload/download jobs.
	struct co_gw_job *sdo[CO_NUM_NODES];
#endif
#if !LELY_NO_CO_MASTER && !LELY_NO_CO_LSS
	/// A pointer to the LSS job.
	struct co_gw_job *lss;
#endif
	/// A pointer to the original NMT command indication function.
	co_nmt_cs_ind_t *cs_ind;
	/// A pointer to user-specified data for #cs_ind.
	void *cs_data;
#if !LELY_NO_CO_NG
#if !LELY_NO_CO_MASTER
	/// A pointer to the original node guarding event indication function.
	co_nmt_ng_ind_t *ng_ind;
	/// A pointer to user-specified data for #ng_ind.
	void *ng_data;
#endif
	/// A pointer to the original life guarding event indication function.
	co_nmt_lg_ind_t *lg_ind;
	/// A pointer to user-specified data for #lg_ind.
	void *lg_data;
#endif
	/// A pointer to the original heartbeat event indication function.
	co_nmt_hb_ind_t *hb_ind;
	/// A pointer to user-specified data for #hb_ind.
	void *hb_data;
	/// A pointer to the original state change event indication function.
	co_nmt_st_ind_t *st_ind;
	/// A pointer to user-specified data for #st_ind.
	void *st_data;
#if !LELY_NO_CO_MASTER
	/// A pointer to the original 'boot slave' indication function.
	co_nmt_boot_ind_t *boot_ind;
	/// A pointer to user-specified data for #boot_ind.
	void *boot_data;
	/// A pointer to the original SDO download progress indication function.
	co_nmt_sdo_ind_t *dn_ind;
	/// A pointer to user-specified data for #dn_ind.
	void *dn_data;
	/// A pointer to the original SDO upload progress indication function.
	co_nmt_sdo_ind_t *up_ind;
	/// A pointer to user-specified data for #up_ind.
	void *up_data;
#endif
};

/// Creates a new CANopen network. @see co_gw_net_destroy()
static struct co_gw_net *co_gw_net_create(
		co_gw_t *gw, co_unsigned16_t id, co_nmt_t *nmt);

/// Destroys a CANopen network. @see co_gw_net_create()
static void co_gw_net_destroy(struct co_gw_net *net);

/**
 * The callback function invoked when an NMT command is received by a CANopen
 * gateway.
 *
 * @see co_nmt_cs_ind_t
 */
static void co_gw_net_cs_ind(co_nmt_t *nmt, co_unsigned8_t cs, void *data);
#if !LELY_NO_CO_NG
#if !LELY_NO_CO_MASTER
/**
 * The callback function invoked when a node guarding event occurs for a node on
 * a CANopen network.
 *
 * @see co_nmt_ng_ind_t
 */
static void co_gw_net_ng_ind(co_nmt_t *nmt, co_unsigned8_t id, int state,
		int reason, void *data);
#endif
/**
 * The callback function invoked when a life guarding event occurs for a CANopen
 * gateway.
 *
 * @see co_nmt_lg_ind_t
 */
static void co_gw_net_lg_ind(co_nmt_t *nmt, int state, void *data);
#endif // !LELY_NO_CO_NG
/**
 * The callback function invoked when a heartbeat event occurs for a node on a
 * CANopen network.
 *
 * @see co_nmt_hb_ind_t
 */
static void co_gw_net_hb_ind(co_nmt_t *nmt, co_unsigned8_t id, int state,
		int reason, void *data);
/**
 * The callback function invoked when a boot-up event or state change is
 * detected for a node on a CANopen network.
 *
 * @see co_nmt_st_ind_t
 */
static void co_gw_net_st_ind(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned8_t st, void *data);
#if !LELY_NO_CO_NMT_BOOT
/**
 * The callback function invoked when the 'boot slave' process completes for a
 * node on a CANopen network.
 *
 * @see co_nmt_boot_ind_t
 */
static void co_gw_net_boot_ind(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned8_t st, char es, void *data);
#endif
#if !LELY_NO_CO_NMT_BOOT || !LELY_NO_CO_NMT_CFG
/**
 * The callback function invoked to notify the user of the progress of an SDO
 * download request during the 'boot slave' process of a node on a CANopen
 * network.
 *
 * @see co_nmt_sdo_ind_t
 */
static void co_gw_net_dn_ind(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned16_t idx, co_unsigned8_t subidx, size_t size,
		size_t nbyte, void *data);
/**
 * The callback function invoked to notify the user of the progress of an SDO
 * upload request during the 'boot slave' process of a node on a CANopen
 * network.
 *
 * @see co_nmt_sdo_ind_t
 */
static void co_gw_net_up_ind(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned16_t idx, co_unsigned8_t subidx, size_t size,
		size_t nbyte, void *data);
#endif
#if !LELY_NO_CO_SYNC
/**
 * The callback function invoked when a SYNC message is received from a node on
 * a CANopen network.
 */
static void co_gw_net_sync_ind(co_sync_t *sync, co_unsigned8_t cnt, void *data);
#endif
#if !LELY_NO_CO_TIME
/**
 * The callback function invoked when a TIME message is received from a node on
 * a CANopen network.
 */
static void co_gw_net_time_ind(
		co_time_t *time, const struct timespec *tp, void *data);
#endif
#if !LELY_NO_CO_EMCY
/**
 * The callback function invoked when an EMCY message is received from a node on
 * a CANopen network.
 */
static void co_gw_net_emcy_ind(co_emcy_t *emcy, co_unsigned8_t id,
		co_unsigned16_t ec, co_unsigned8_t er, co_unsigned8_t msef[5],
		void *data);
#endif
#if !LELY_NO_CO_RPDO
/**
 * The callback function invoked when a PDO is received from a node on a CANopen
 * network.
 */
static void co_gw_net_rpdo_ind(co_rpdo_t *pdo, co_unsigned32_t ac,
		const void *ptr, size_t n, void *data);
#endif

/// A CANopen gateway network job.
struct co_gw_job {
	/// The address of the pointer to this job in the network.
	struct co_gw_job **pself;
	/// A pointer to the CANopen network.
	struct co_gw_net *net;
	/// A pointer to request-specific data.
	void *data;
	/// A pointer to the destructor for #data.
	void (*dtor)(void *data);
	/// The service parameters of the request.
	struct co_gw_req req;
};

/// The minimum size (in bytes) of a CANopen gateway network job.
#define CO_GW_JOB_SIZE offsetof(struct co_gw_job, req)

/// Creates a new CANopen gateway network job. @see co_gw_job_destroy()
static struct co_gw_job *co_gw_job_create(struct co_gw_job **pself,
		struct co_gw_net *net, void *data, void (*dtor)(void *data),
		const struct co_gw_req *req);
/// Destroys a CANopen gateway network job. @see co_gw_job_create()
static void co_gw_job_destroy(struct co_gw_job *job);

/// Removes a CANopen gateway network job from its network.
static void co_gw_job_remove(struct co_gw_job *job);

#if !LELY_NO_CO_CSDO
/// Creates a new SDO upload/download job.
static struct co_gw_job *co_gw_job_create_sdo(struct co_gw_job **pself,
		struct co_gw_net *net, co_unsigned8_t id,
		const struct co_gw_req *req);
/// Destroys the Client-SDO service in an SDO upload/download job.
static void co_gw_job_sdo_dtor(void *data);
/// The confirmation function for an 'SDO upload' request.
static void co_gw_job_sdo_up_con(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac, const void *ptr,
		size_t n, void *data);
/// The confirmation function for an 'SDO download' request.
static void co_gw_job_sdo_dn_con(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac, void *data);
/// The progress indication function for an SDO upload/download job.
static void co_gw_job_sdo_ind(const co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, size_t size, size_t nbyte, void *data);
#endif

#if !LELY_NO_CO_MASTER && !LELY_NO_CO_LSS
/// Creates a new LSS job.
static struct co_gw_job *co_gw_job_create_lss(struct co_gw_job **pself,
		struct co_gw_net *net, const struct co_gw_req *req);
/**
 * The confirmation function for an 'LSS switch state selective', 'LSS identify
 * remote slave' or 'LSS identify non-configured remote slaves' request.
 */
static void co_gw_job_lss_cs_ind(co_lss_t *lss, co_unsigned8_t cs, void *data);
/**
 * The confirmation function for an 'LSS configure node-ID', 'LSS configure
 * bit-rate' or 'LSS store configuration' request.
 */
static void co_gw_job_lss_err_ind(co_lss_t *lss, co_unsigned8_t cs,
		co_unsigned8_t err, co_unsigned8_t spec, void *data);
/// The confirmation function for an 'Inquire LSS address' request.
static void co_gw_job_lss_lssid_ind(co_lss_t *lss, co_unsigned8_t cs,
		co_unsigned32_t id, void *data);
/// The confirmation function for an 'LSS inquire node-ID' request.
static void co_gw_job_lss_nid_ind(co_lss_t *lss, co_unsigned8_t cs,
		co_unsigned8_t id, void *data);
/// The confirmation function for an 'LSS Slowscan/Fastscan' request.
static void co_gw_job_lss_scan_ind(co_lss_t *lss, co_unsigned8_t cs,
		const struct co_id *id, void *data);
#endif

/// A CANopen gateway.
struct __co_gw {
	/// An array of pointers to the CANopen networks.
	struct co_gw_net *net[CO_GW_NUM_NET];
	/// The command timeout (in milliseconds).
	int timeout;
	/// The default network-ID.
	co_unsigned16_t def;
	/**
	 * A pointer to the callback function invoked when an indication or
	 * confirmation needs to be sent.
	 */
	co_gw_send_func_t *send_func;
	/// A pointer to the user-specified data for #send_func.
	void *send_data;
	/**
	 * A pointer to the callback function invoked when a baudrate switch is
	 * needed after an 'Initialize gateway' command is received.
	 */
	co_gw_rate_func_t *rate_func;
	/// A pointer to the user-specified data for #rate_func.
	void *rate_data;
};

#if !LELY_NO_CO_CSDO
/// Processes an 'SDO upload' request.
static int co_gw_recv_sdo_up(co_gw_t *gw, co_unsigned16_t net,
		co_unsigned8_t node, const struct co_gw_req *req);
/// Processes an 'SDO download' request.
static int co_gw_recv_sdo_dn(co_gw_t *gw, co_unsigned16_t net,
		co_unsigned8_t node, const struct co_gw_req *req);
/// Processes a 'Configure SDO time-out' request.
static int co_gw_recv_set_sdo_timeout(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
#endif

#if !LELY_NO_CO_RPDO
/// Processes a 'Configure RPDO' request.
static int co_gw_recv_set_rpdo(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
#endif
#if !LELY_NO_CO_TPDO
/// Processes a 'Configure TPDO' request.
static int co_gw_recv_set_tpdo(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
#endif
#if !LELY_NO_CO_RPDO
/// Processes a 'Read PDO data' request.
static int co_gw_recv_pdo_read(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
#endif
#if !LELY_NO_CO_TPDO
/// Processes a 'Write PDO data' request.
static int co_gw_recv_pdo_write(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
#endif

#if !LELY_NO_CO_MASTER
/// Processes an NMT request.
static int co_gw_recv_nmt_cs(co_gw_t *gw, co_unsigned16_t net,
		co_unsigned8_t node, co_unsigned8_t cs,
		const struct co_gw_req *req);
#if !LELY_NO_CO_NG
/// Processes a 'Enable/Disable node guarding' request.
static int co_gw_recv_nmt_set_ng(co_gw_t *gw, co_unsigned16_t net,
		co_unsigned8_t node, const struct co_gw_req *req);
#endif
#endif // !LELY_NO_CO_MASTER
/// Processes a 'Start/Disable heartbeat consumer' request.
static int co_gw_recv_nmt_set_hb(co_gw_t *gw, co_unsigned16_t net,
		co_unsigned8_t node, const struct co_gw_req *req);

/// Processes an 'Initialize gateway' request.
static int co_gw_recv_init(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
/// Processes a 'Set heartbeat producer' request.
static int co_gw_recv_set_hb(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
/// Processes a 'Set node-ID' request.
static int co_gw_recv_set_id(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
#if !LELY_NO_CO_EMCY
/// Processes a 'Start/Stop emergency consumer' request.
static int co_gw_recv_set_emcy(co_gw_t *gw, co_unsigned16_t net,
		co_unsigned8_t node, const struct co_gw_req *req);
#endif
/// Processes a 'Set command time-out' request.
static int co_gw_recv_set_cmd_timeout(co_gw_t *gw, const struct co_gw_req *req);
/// Processes a 'Boot-up forwarding' request.
static int co_gw_recv_set_bootup_ind(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);

/// Processes a 'Set default network' request.
static int co_gw_recv_set_net(co_gw_t *gw, const struct co_gw_req *req);
/// Processes a 'Set default node-ID' request.
static int co_gw_recv_set_node(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
/// Processes a 'Get version' request.
static int co_gw_recv_get_version(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);

#if !LELY_NO_CO_MASTER && !LELY_NO_CO_LSS
/// Processes an 'LSS switch state global' request.
static int co_gw_recv_lss_switch(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
/// Processes an 'LSS switch state selective' request.
static int co_gw_recv_lss_switch_sel(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
/// Processes an 'LSS configure node-ID' request.
static int co_gw_recv_lss_set_id(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
/// Processes an 'LSS configure bit-rate' request.
static int co_gw_recv_lss_set_rate(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
/// Processes an 'LSS activate new bit-rate' request.
static int co_gw_recv_lss_switch_rate(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
/// Processes an 'LSS store configuration' request.
static int co_gw_recv_lss_store(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
/// Processes an 'Inquire LSS address' request.
static int co_gw_recv_lss_get_lssid(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
/// Processes an 'LSS inquire node-ID' request.
static int co_gw_recv_lss_get_id(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
/// Processes an 'LSS identify remote slave' request.
static int co_gw_recv_lss_id_slave(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
/// Processes an 'LSS identify non-configured remote slaves' request.
static int co_gw_recv_lss_id_non_cfg_slave(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);

/// Processes an 'LSS Slowscan' request.
static int co_gw_recv__lss_slowscan(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
/// Processes an 'LSS Fastscan' request.
static int co_gw_recv__lss_fastscan(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req);
#endif

/// Sends a confirmation with an internal error code or SDO abort code.
static int co_gw_send_con(co_gw_t *gw, const struct co_gw_req *req, int iec,
		co_unsigned32_t ac);
/// Sends an 'Error control event received' indication.
static int co_gw_send_ec(co_gw_t *gw, co_unsigned16_t net, co_unsigned8_t node,
		co_unsigned8_t st, int iec);
/// Invokes the callback function to send a confirmation or indication.
static int co_gw_send_srv(co_gw_t *gw, const struct co_gw_srv *srv);

/// Converts an error number to an internal error code.
static inline int errnum2iec(errnum_t errnum);

const char *
co_gw_iec2str(int iec)
{
	switch (iec) {
	case CO_GW_IEC_BAD_SRV: return "Request not supported";
	case CO_GW_IEC_SYNTAX: return "Syntax error";
	case CO_GW_IEC_INTERN:
		return "Request not processed due to internal state";
	case CO_GW_IEC_TIMEOUT: return "Time-out";
	case CO_GW_IEC_NO_DEF_NET: return "No default net set";
	case CO_GW_IEC_NO_DEF_NODE: return "No default node set";
	case CO_GW_IEC_BAD_NET: return "Unsupported net";
	case CO_GW_IEC_BAD_NODE: return "Unsupported node";
	case CO_GW_IEC_NG_OCCURRED: return "Lost guarding message";
	case CO_GW_IEC_LG_OCCURRED: return "Lost connection";
	case CO_GW_IEC_HB_RESOLVED: return "Heartbeat started";
	case CO_GW_IEC_HB_OCCURRED: return "Heartbeat lost";
	case CO_GW_IEC_ST_OCCURRED: return "Wrong NMT state";
	case CO_GW_IEC_BOOTUP: return "Boot-up";
	case CO_GW_IEC_CAN_PASSIVE: return "Error passive";
	case CO_GW_IEC_CAN_BUSOFF: return "Bus off";
	case CO_GW_IEC_CAN_OVERFLOW: return "CAN buffer overflow";
	case CO_GW_IEC_CAN_INIT: return "CAN init";
	case CO_GW_IEC_CAN_ACTIVE: return "CAN active";
	case CO_GW_IEC_PDO_INUSE: return "PDO already used";
	case CO_GW_IEC_PDO_LEN: return "PDO length exceeded";
	case CO_GW_IEC_LSS: return "LSS error";
	case CO_GW_IEC_LSS_ID: return "LSS node-ID not supported";
	case CO_GW_IEC_LSS_RATE: return "LSS bit-rate not supported";
	case CO_GW_IEC_LSS_PARAM: return "LSS parameter storing failed";
	case CO_GW_IEC_LSS_MEDIA:
		return "LSS command failed because of media error";
	case CO_GW_IEC_NO_MEM: return "Running out of memory";
	default: return "Unknown error code";
	}
}

void *
__co_gw_alloc(void)
{
	void *ptr = malloc(sizeof(struct __co_gw));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
__co_gw_free(void *ptr)
{
	free(ptr);
}

struct __co_gw *
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

void
__co_gw_fini(struct __co_gw *gw)
{
	assert(gw);

	for (co_unsigned16_t id = 1; id <= CO_GW_NUM_NET; id++)
		co_gw_net_destroy(gw->net[id - 1]);
}

co_gw_t *
co_gw_create(void)
{
	int errc = 0;

	co_gw_t *gw = __co_gw_alloc();
	if (!gw) {
		errc = get_errc();
		goto error_alloc_gw;
	}

	if (!__co_gw_init(gw)) {
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

void
co_gw_destroy(co_gw_t *gw)
{
	if (gw) {
		__co_gw_fini(gw);
		__co_gw_free(gw);
	}
}

int
co_gw_init_net(co_gw_t *gw, co_unsigned16_t id, co_nmt_t *nmt)
{
	assert(gw);

	if (!id)
		id = co_dev_get_netid(co_nmt_get_dev(nmt));

	if (co_gw_fini_net(gw, id) == -1)
		return -1;

	gw->net[id - 1] = co_gw_net_create(gw, id, nmt);
	return gw->net[id - 1] ? 0 : -1;
}

int
co_gw_fini_net(co_gw_t *gw, co_unsigned16_t id)
{
	assert(gw);

	if (!id || id > CO_GW_NUM_NET) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	co_gw_net_destroy(gw->net[id - 1]);
	gw->net[id - 1] = NULL;

	return 0;
}

int
co_gw_recv(co_gw_t *gw, const struct co_gw_req *req)
{
	assert(gw);
	assert(req);

	if (req->size < sizeof(*req)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	int iec = 0;

	// Obtain the network-ID for node- and network-level requests. If the
	// network-ID is 0, use the default ID.
	co_unsigned16_t net = gw->def;
	switch (req->srv) {
#if !LELY_NO_CO_CSDO
	case CO_GW_SRV_SDO_UP:
	case CO_GW_SRV_SDO_DN:
	case CO_GW_SRV_SET_SDO_TIMEOUT:
#endif
#if !LELY_NO_CO_RPDO
	case CO_GW_SRV_SET_RPDO:
#endif
#if !LELY_NO_CO_TPDO
	case CO_GW_SRV_SET_TPDO:
#endif
#if !LELY_NO_CO_RPDO
	case CO_GW_SRV_PDO_READ:
#endif
#if !LELY_NO_CO_TPDO
	case CO_GW_SRV_PDO_WRITE:
#endif
#if !LELY_NO_CO_MASTER
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
#if !LELY_NO_CO_EMCY
	case CO_GW_SRV_EMCY_START:
	case CO_GW_SRV_EMCY_STOP:
#endif
	case CO_GW_SRV_SET_BOOTUP_IND:
	case CO_GW_SRV_SET_NODE:
	case CO_GW_SRV_GET_VERSION:
#if !LELY_NO_CO_MASTER && !LELY_NO_CO_LSS
	case CO_GW_SRV_LSS_SWITCH:
	case CO_GW_SRV_LSS_SWITCH_SEL:
	case CO_GW_SRV_LSS_SET_ID:
	case CO_GW_SRV_LSS_SET_RATE:
	case CO_GW_SRV_LSS_SWITCH_RATE:
	case CO_GW_SRV_LSS_STORE:
	case CO_GW_SRV_LSS_GET_LSSID:
	case CO_GW_SRV_LSS_GET_ID:
	case CO_GW_SRV_LSS_ID_SLAVE:
	case CO_GW_SRV_LSS_ID_NON_CFG_SLAVE:
	case CO_GW_SRV__LSS_SLOWSCAN:
	case CO_GW_SRV__LSS_FASTSCAN:
#endif
		if (req->size < sizeof(struct co_gw_req_net)) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		struct co_gw_req_net *par = (struct co_gw_req_net *)req;
		if (par->net)
			net = par->net;
		if (!net) {
			iec = CO_GW_IEC_NO_DEF_NET;
			goto error;
		} else if (net > CO_GW_NUM_NET || !gw->net[net - 1]) {
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
#if !LELY_NO_CO_CSDO
	case CO_GW_SRV_SDO_UP:
	case CO_GW_SRV_SDO_DN:
#endif
#if !LELY_NO_CO_MASTER
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
#if !LELY_NO_CO_EMCY
	case CO_GW_SRV_EMCY_START:
	case CO_GW_SRV_EMCY_STOP:
#endif
		if (req->size < sizeof(struct co_gw_req_node)) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		struct co_gw_req_node *par = (struct co_gw_req_node *)req;
		if (par->node != 0xff)
			node = par->node;
		if (node > CO_NUM_NODES) {
			iec = CO_GW_IEC_BAD_NODE;
			goto error;
		}
		break;
	}

	// Except for the NMT commands, node-level request require a non-zero
	// node-ID.
	switch (req->srv) {
#if !LELY_NO_CO_CSDO
	case CO_GW_SRV_SDO_UP:
	case CO_GW_SRV_SDO_DN:
#endif
#if !LELY_NO_CO_MASTER
	case CO_GW_SRV_NMT_NG_ENABLE:
	case CO_GW_SRV_NMT_NG_DISABLE:
#endif
	case CO_GW_SRV_NMT_HB_ENABLE:
	case CO_GW_SRV_NMT_HB_DISABLE:
#if !LELY_NO_CO_EMCY
	case CO_GW_SRV_EMCY_START:
	case CO_GW_SRV_EMCY_STOP:
#endif
		if (!node) {
			iec = CO_GW_IEC_NO_DEF_NODE;
			goto error;
		}
	}

	switch (req->srv) {
#if !LELY_NO_CO_CSDO
	case CO_GW_SRV_SDO_UP:
		trace("gateway: received 'SDO upload' request");
		return co_gw_recv_sdo_up(gw, net, node, req);
	case CO_GW_SRV_SDO_DN:
		trace("gateway: received 'SDO download' request");
		return co_gw_recv_sdo_dn(gw, net, node, req);
	case CO_GW_SRV_SET_SDO_TIMEOUT:
		trace("gateway: received 'Configure SDO time-out' request");
		return co_gw_recv_set_sdo_timeout(gw, net, req);
#endif
#if !LELY_NO_CO_RPDO
	case CO_GW_SRV_SET_RPDO:
		trace("gateway: received 'Configure RPDO' request");
		return co_gw_recv_set_rpdo(gw, net, req);
#endif
#if !LELY_NO_CO_TPDO
	case CO_GW_SRV_SET_TPDO:
		trace("gateway: received 'Configure TPDO' request");
		return co_gw_recv_set_tpdo(gw, net, req);
#endif
#if !LELY_NO_CO_RPDO
	case CO_GW_SRV_PDO_READ:
		trace("gateway: received 'Read PDO data' request");
		return co_gw_recv_pdo_read(gw, net, req);
#endif
#if !LELY_NO_CO_TPDO
	case CO_GW_SRV_PDO_WRITE:
		trace("gateway: received 'Write PDO data' request");
		return co_gw_recv_pdo_write(gw, net, req);
#endif
#if !LELY_NO_CO_MASTER
	case CO_GW_SRV_NMT_START:
		trace("gateway: received 'Start node' request");
		return co_gw_recv_nmt_cs(gw, net, node, CO_NMT_CS_START, req);
	case CO_GW_SRV_NMT_STOP:
		trace("gateway: received 'Stop node' request");
		return co_gw_recv_nmt_cs(gw, net, node, CO_NMT_CS_STOP, req);
	case CO_GW_SRV_NMT_ENTER_PREOP:
		trace("gateway: received 'Set node to pre-operational' request");
		return co_gw_recv_nmt_cs(
				gw, net, node, CO_NMT_CS_ENTER_PREOP, req);
	case CO_GW_SRV_NMT_RESET_NODE:
		trace("gateway: received 'Reset node' request");
		return co_gw_recv_nmt_cs(
				gw, net, node, CO_NMT_CS_RESET_NODE, req);
	case CO_GW_SRV_NMT_RESET_COMM:
		trace("gateway: received 'Reset communication' request");
		return co_gw_recv_nmt_cs(
				gw, net, node, CO_NMT_CS_RESET_COMM, req);
#if !LELY_NO_CO_NG
	case CO_GW_SRV_NMT_NG_ENABLE:
	case CO_GW_SRV_NMT_NG_DISABLE:
		trace("gateway: received '%s node guarding' request",
				req->srv == CO_GW_SRV_NMT_NG_ENABLE
						? "Enable"
						: "Disable");
		return co_gw_recv_nmt_set_ng(gw, net, node, req);
#endif
#endif // !LELY_NO_CO_MASTER
	case CO_GW_SRV_NMT_HB_ENABLE:
	case CO_GW_SRV_NMT_HB_DISABLE:
		trace("gateway: received '%s heartbeat consumer' request",
				req->srv == CO_GW_SRV_NMT_HB_ENABLE
						? "Start"
						: "Disable");
		return co_gw_recv_nmt_set_hb(gw, net, node, req);
	case CO_GW_SRV_INIT:
		trace("gateway: received 'Initialize gateway' request");
		return co_gw_recv_init(gw, net, req);
	case CO_GW_SRV_SET_HB:
		trace("gateway: received 'Set heartbeat producer' request");
		return co_gw_recv_set_hb(gw, net, req);
	case CO_GW_SRV_SET_ID:
		trace("gateway: received 'Set node-ID' request");
		return co_gw_recv_set_id(gw, net, req);
#if !LELY_NO_CO_EMCY
	case CO_GW_SRV_EMCY_START:
	case CO_GW_SRV_EMCY_STOP:
		trace("gateway: received '%s emergency consumer' request",
				// clang-format off
				req->srv == CO_GW_SRV_EMCY_START
						? "Start" : "Stop");
		// clang-format on
		return co_gw_recv_set_emcy(gw, net, node, req);
#endif
	case CO_GW_SRV_SET_CMD_TIMEOUT:
		trace("gateway: received 'Set command time-out' request");
		return co_gw_recv_set_cmd_timeout(gw, req);
	case CO_GW_SRV_SET_BOOTUP_IND:
		trace("gateway: received 'Boot-up forwarding' request");
		return co_gw_recv_set_bootup_ind(gw, net, req);
	case CO_GW_SRV_SET_NET:
		trace("gateway: received 'Set default network' request");
		return co_gw_recv_set_net(gw, req);
	case CO_GW_SRV_SET_NODE:
		trace("gateway: received 'Set default node-ID' request");
		return co_gw_recv_set_node(gw, net, req);
	case CO_GW_SRV_GET_VERSION:
		trace("gateway: received 'Get version' request");
		return co_gw_recv_get_version(gw, net, req);
	case CO_GW_SRV_SET_CMD_SIZE:
		trace("gateway: received 'Set command size' request");
		// We cannot guarantee a lack of memory resources will never
		// occur.
		return co_gw_send_con(gw, req, CO_GW_IEC_NO_MEM, 0);
#if !LELY_NO_CO_MASTER && !LELY_NO_CO_LSS
	case CO_GW_SRV_LSS_SWITCH:
		trace("gateway: received 'LSS switch state global' request");
		return co_gw_recv_lss_switch(gw, net, req);
	case CO_GW_SRV_LSS_SWITCH_SEL:
		trace("gateway: received 'LSS switch state selective' request");
		return co_gw_recv_lss_switch_sel(gw, net, req);
	case CO_GW_SRV_LSS_SET_ID:
		trace("gateway: received 'LSS configure node-ID' request");
		return co_gw_recv_lss_set_id(gw, net, req);
	case CO_GW_SRV_LSS_SET_RATE:
		trace("gateway: received 'LSS configure bit-rate' request");
		return co_gw_recv_lss_set_rate(gw, net, req);
	case CO_GW_SRV_LSS_SWITCH_RATE:
		trace("gateway: received 'LSS activate new bit-rate' request");
		return co_gw_recv_lss_switch_rate(gw, net, req);
	case CO_GW_SRV_LSS_STORE:
		trace("gateway: received 'LSS store configuration' request");
		return co_gw_recv_lss_store(gw, net, req);
	case CO_GW_SRV_LSS_GET_LSSID:
		trace("gateway: received 'Inquire LSS address' request");
		return co_gw_recv_lss_get_lssid(gw, net, req);
	case CO_GW_SRV_LSS_GET_ID:
		trace("gateway: received 'LSS inquire node-ID' request");
		return co_gw_recv_lss_get_id(gw, net, req);
	case CO_GW_SRV_LSS_ID_SLAVE:
		trace("gateway: received 'LSS identify remote slave' request");
		return co_gw_recv_lss_id_slave(gw, net, req);
	case CO_GW_SRV_LSS_ID_NON_CFG_SLAVE:
		trace("gateway: received 'LSS identify non-configure remote slaves' request");
		return co_gw_recv_lss_id_non_cfg_slave(gw, net, req);
	case CO_GW_SRV__LSS_SLOWSCAN:
		trace("gateway: received 'LSS Slowscan' request");
		return co_gw_recv__lss_slowscan(gw, net, req);
	case CO_GW_SRV__LSS_FASTSCAN:
		trace("gateway: received 'LSS Fastscan' request");
		return co_gw_recv__lss_fastscan(gw, net, req);
#endif
	default: iec = CO_GW_IEC_BAD_SRV; goto error;
	}

error:
	return co_gw_send_con(gw, req, iec, 0);
}

void
co_gw_get_send_func(const co_gw_t *gw, co_gw_send_func_t **pfunc, void **pdata)
{
	assert(gw);

	if (pfunc)
		*pfunc = gw->send_func;
	if (pdata)
		*pdata = gw->send_data;
}

void
co_gw_set_send_func(co_gw_t *gw, co_gw_send_func_t *func, void *data)
{
	assert(gw);

	gw->send_func = func;
	gw->send_data = data;
}

void
co_gw_get_rate_func(const co_gw_t *gw, co_gw_rate_func_t **pfunc, void **pdata)
{
	assert(gw);

	if (pfunc)
		*pfunc = gw->rate_func;
	if (pdata)
		*pdata = gw->rate_data;
}

void
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
	if (!net) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return NULL;
	}

	net->gw = gw;
	net->id = id;
	net->nmt = nmt;

	net->def = 0;
#if !LELY_NO_CO_CSDO
	net->timeout = 0;
#endif
	net->bootup_ind = 1;

#if !LELY_NO_CO_CSDO
	for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++)
		net->sdo[id - 1] = NULL;
#endif
#if !LELY_NO_CO_MASTER && !LELY_NO_CO_LSS
	net->lss = NULL;
#endif

	co_nmt_get_cs_ind(net->nmt, &net->cs_ind, &net->cs_data);
	co_nmt_set_cs_ind(net->nmt, &co_gw_net_cs_ind, net);
#if !LELY_NO_CO_NG
#if !LELY_NO_CO_MASTER
	co_nmt_get_ng_ind(net->nmt, &net->ng_ind, &net->ng_data);
	co_nmt_set_ng_ind(net->nmt, &co_gw_net_ng_ind, net);
#endif
	co_nmt_get_lg_ind(net->nmt, &net->lg_ind, &net->lg_data);
	co_nmt_set_lg_ind(net->nmt, &co_gw_net_lg_ind, net);
#endif
	co_nmt_get_hb_ind(net->nmt, &net->hb_ind, &net->hb_data);
	co_nmt_set_hb_ind(net->nmt, &co_gw_net_hb_ind, net);
	co_nmt_get_st_ind(net->nmt, &net->st_ind, &net->st_data);
	co_nmt_set_st_ind(net->nmt, &co_gw_net_st_ind, net);
#if !LELY_NO_CO_NMT_BOOT
	co_nmt_get_boot_ind(net->nmt, &net->boot_ind, &net->boot_data);
	co_nmt_set_boot_ind(net->nmt, &co_gw_net_boot_ind, net);
#endif
#if !LELY_NO_CO_NMT_BOOT || !LELY_NO_CO_NMT_CFG
	co_nmt_get_dn_ind(net->nmt, &net->dn_ind, &net->dn_data);
	co_nmt_set_dn_ind(net->nmt, &co_gw_net_dn_ind, net);
	co_nmt_get_dn_ind(net->nmt, &net->up_ind, &net->up_data);
	co_nmt_set_dn_ind(net->nmt, &co_gw_net_up_ind, net);
#endif

#if !LELY_NO_CO_SYNC
	co_sync_t *sync = co_nmt_get_sync(nmt);
	if (sync)
		co_sync_set_ind(sync, &co_gw_net_sync_ind, net);
#endif

#if !LELY_NO_CO_TIME
	co_time_t *time = co_nmt_get_time(nmt);
	if (time)
		co_time_set_ind(time, &co_gw_net_time_ind, net);
#endif

#if !LELY_NO_CO_EMCY
	co_emcy_t *emcy = co_nmt_get_emcy(nmt);
	if (emcy)
		co_emcy_set_ind(emcy, &co_gw_net_emcy_ind, net);
#endif

#if !LELY_NO_CO_RPDO
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
#if !LELY_NO_CO_RPDO
		for (co_unsigned16_t i = 1; i <= 512; i++) {
			co_rpdo_t *pdo = co_nmt_get_rpdo(net->nmt, i);
			if (pdo)
				co_rpdo_set_ind(pdo, NULL, NULL);
		}
#endif

#if !LELY_NO_CO_EMCY
		co_emcy_t *emcy = co_nmt_get_emcy(net->nmt);
		if (emcy)
			co_emcy_set_ind(emcy, NULL, NULL);
#endif

#if !LELY_NO_CO_TIME
		co_time_t *time = co_nmt_get_time(net->nmt);
		if (time)
			co_time_set_ind(time, NULL, NULL);
#endif

#if !LELY_NO_CO_SYNC
		co_sync_t *sync = co_nmt_get_sync(net->nmt);
		if (sync)
			co_sync_set_ind(sync, NULL, NULL);
#endif

#if !LELY_NO_CO_NMT_BOOT
		co_nmt_set_boot_ind(net->nmt, net->boot_ind, net->boot_data);
#endif
		co_nmt_set_st_ind(net->nmt, net->st_ind, net->st_data);
		co_nmt_set_hb_ind(net->nmt, net->hb_ind, net->hb_data);
#if !LELY_NO_CO_NG
		co_nmt_set_lg_ind(net->nmt, net->lg_ind, net->lg_data);
#if !LELY_NO_CO_MASTER
		co_nmt_set_ng_ind(net->nmt, net->ng_ind, net->ng_data);
#endif
#endif
		co_nmt_set_cs_ind(net->nmt, net->cs_ind, net->cs_data);

#if !LELY_NO_CO_CSDO
		for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++)
			co_gw_job_destroy(net->sdo[id - 1]);
#endif
#if !LELY_NO_CO_MASTER && !LELY_NO_CO_LSS
		co_gw_job_destroy(net->lss);
#endif

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
#if !LELY_NO_CO_SYNC
		co_sync_t *sync = co_nmt_get_sync(nmt);
		if (sync)
			co_sync_set_ind(sync, &co_gw_net_sync_ind, net);
#endif

#if !LELY_NO_CO_TIME
		co_time_t *time = co_nmt_get_time(nmt);
		if (time)
			co_time_set_ind(time, &co_gw_net_time_ind, net);
#endif

#if !LELY_NO_CO_EMCY
		co_emcy_t *emcy = co_nmt_get_emcy(nmt);
		if (emcy)
			co_emcy_set_ind(emcy, &co_gw_net_emcy_ind, net);
#endif

#if !LELY_NO_CO_RPDO
		for (co_unsigned16_t i = 1; i <= 512; i++) {
			co_rpdo_t *pdo = co_nmt_get_rpdo(nmt, i);
			if (pdo)
				co_rpdo_set_ind(pdo, &co_gw_net_rpdo_ind, net);
		}
#endif

		break;
	}
	case CO_NMT_CS_ENTER_PREOP: {
#if !LELY_NO_CO_SYNC
		co_sync_t *sync = co_nmt_get_sync(nmt);
		if (sync)
			co_sync_set_ind(sync, &co_gw_net_sync_ind, net);
#endif

#if !LELY_NO_CO_TIME
		co_time_t *time = co_nmt_get_time(nmt);
		if (time)
			co_time_set_ind(time, &co_gw_net_time_ind, net);
#endif

#if !LELY_NO_CO_EMCY
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

#if !LELY_NO_CO_NG

#if !LELY_NO_CO_MASTER
static void
co_gw_net_ng_ind(co_nmt_t *nmt, co_unsigned8_t id, int state, int reason,
		void *data)
{
	struct co_gw_net *net = data;
	assert(net);

	if (state == CO_NMT_EC_OCCURRED) {
		int iec = 0;
		switch (reason) {
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

#endif // !LELY_NO_CO_NG

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
co_gw_net_st_ind(
		co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, void *data)
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

#if !LELY_NO_CO_NMT_BOOT
static void
co_gw_net_boot_ind(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, char es,
		void *data)
{
	struct co_gw_net *net = data;
	assert(net);

	struct co_gw_ind__boot ind = { .size = sizeof(ind),
		.srv = CO_GW_SRV__BOOT,
		.net = net->id,
		.node = id,
		.st = st,
		.es = es };
	co_gw_send_srv(net->gw, (struct co_gw_srv *)&ind);

	if (net->boot_ind)
		net->boot_ind(nmt, id, st, es, net->boot_data);
}
#endif

#if !LELY_NO_CO_NMT_BOOT || !LELY_NO_CO_NMT_CFG

static void
co_gw_net_dn_ind(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned16_t idx,
		co_unsigned8_t subidx, size_t size, size_t nbyte, void *data)
{
	struct co_gw_net *net = data;
	assert(net);

	struct co_gw_ind_sdo ind = { .size = sizeof(ind),
		.srv = CO_GW_SRV_SDO,
		.net = net->id,
		.node = id,
		.nbyte = nbyte,
		.up = 0,
		.data = NULL,
		._size = size };
	co_gw_send_srv(net->gw, (struct co_gw_srv *)&ind);

	if (net->dn_ind)
		net->dn_ind(nmt, id, idx, subidx, size, nbyte, net->dn_data);
}

static void
co_gw_net_up_ind(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned16_t idx,
		co_unsigned8_t subidx, size_t size, size_t nbyte, void *data)
{
	struct co_gw_net *net = data;
	assert(net);

	struct co_gw_ind_sdo ind = { .size = sizeof(ind),
		.srv = CO_GW_SRV_SDO,
		.net = net->id,
		.node = id,
		.nbyte = nbyte,
		.up = 1,
		.data = NULL,
		._size = size };
	co_gw_send_srv(net->gw, (struct co_gw_srv *)&ind);

	if (net->up_ind)
		net->up_ind(nmt, id, idx, subidx, size, nbyte, net->up_data);
}

#endif // !LELY_NO_CO_NMT_BOOT || !LELY_NO_CO_NMT_CFG

#if !LELY_NO_CO_SYNC
static void
co_gw_net_sync_ind(co_sync_t *sync, co_unsigned8_t cnt, void *data)
{
	(void)sync;
	struct co_gw_net *net = data;
	assert(net);

	struct co_gw_ind__sync ind = { .size = sizeof(ind),
		.srv = CO_GW_SRV__SYNC,
		.net = net->id,
		.cnt = cnt };
	co_gw_send_srv(net->gw, (struct co_gw_srv *)&ind);

	co_nmt_on_sync(net->nmt, cnt);
}
#endif

#if !LELY_NO_CO_TIME
static void
co_gw_net_time_ind(co_time_t *time, const struct timespec *tp, void *data)
{
	(void)time;
	struct co_gw_net *net = data;
	assert(net);

	struct co_gw_ind__time ind = { .size = sizeof(ind),
		.srv = CO_GW_SRV__TIME,
		.net = net->id,
		.ts = *tp };
	co_gw_send_srv(net->gw, (struct co_gw_srv *)&ind);
}
#endif

#if !LELY_NO_CO_EMCY
static void
co_gw_net_emcy_ind(co_emcy_t *emcy, co_unsigned8_t id, co_unsigned16_t ec,
		co_unsigned8_t er, co_unsigned8_t msef[5], void *data)
{
	(void)emcy;
	struct co_gw_net *net = data;
	assert(net);

	struct co_gw_ind_emcy ind = { .size = sizeof(ind),
		.srv = CO_GW_SRV_EMCY,
		.net = net->id,
		.node = id,
		.ec = ec,
		.er = er,
		.msef = { msef[0], msef[1], msef[2], msef[3], msef[4] } };
	co_gw_send_srv(net->gw, (struct co_gw_srv *)&ind);
}
#endif

static struct co_gw_job *
co_gw_job_create(struct co_gw_job **pself, struct co_gw_net *net, void *data,
		void (*dtor)(void *data), const struct co_gw_req *req)
{
	assert(pself);
	assert(req);

	if (*pself)
		co_gw_job_destroy(*pself);

	*pself = malloc(CO_GW_JOB_SIZE + req->size);
	if (!*pself) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return NULL;
	}

	(*pself)->pself = pself;
	(*pself)->net = net;
	(*pself)->data = data;
	(*pself)->dtor = dtor;
	memcpy(&(*pself)->req, req, req->size);

	return *pself;
}

static void
co_gw_job_destroy(struct co_gw_job *job)
{
	if (job) {
		co_gw_job_remove(job);

		if (job->dtor)
			job->dtor(job->data);

		free(job);
	}
}

static void
co_gw_job_remove(struct co_gw_job *job)
{
	assert(job);

	if (job->pself && *job->pself == job)
		*job->pself = NULL;
}

#if !LELY_NO_CO_CSDO

static struct co_gw_job *
co_gw_job_create_sdo(struct co_gw_job **pself, struct co_gw_net *net,
		co_unsigned8_t id, const struct co_gw_req *req)
{
	assert(pself);
	assert(net);

	co_gw_t *gw = net->gw;

	int errc = 0;

	if (*pself) {
		errc = errnum2c(ERRNUM_BUSY);
		goto error_param;
	}

	co_csdo_t *sdo = co_csdo_create(co_nmt_get_net(net->nmt), NULL, id);
	if (!sdo) {
		errc = get_errc();
		goto error_create_sdo;
	}

	// The actual SDO timeout is limited by the global gateway command
	// timeout.
	int timeout = net->timeout;
	if (gw->timeout)
		timeout = timeout ? MIN(timeout, gw->timeout) : gw->timeout;
	co_csdo_set_timeout(sdo, timeout);

	struct co_gw_job *job = co_gw_job_create(
			pself, net, sdo, &co_gw_job_sdo_dtor, req);
	if (!job) {
		errc = get_errc();
		goto error_create_job;
	}

	return job;

error_create_job:
	co_csdo_destroy(sdo);
error_create_sdo:
error_param:
	set_errc(errc);
	return NULL;
}

static void
co_gw_job_sdo_dtor(void *data)
{
	co_csdo_t *sdo = data;

	co_csdo_destroy(sdo);
}

static void
co_gw_job_sdo_up_con(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned32_t ac, const void *ptr, size_t n, void *data)
{
	(void)sdo;
	(void)idx;
	(void)subidx;
	struct co_gw_job *job = data;
	assert(job);
	assert(job->net);

	co_gw_job_remove(job);

	if (job->req.srv != CO_GW_SRV_SDO_UP)
		goto done;
	struct co_gw_req_sdo_up *req = (struct co_gw_req_sdo_up *)&job->req;

	if (ac) {
		co_gw_send_con(job->net->gw, &job->req, 0, ac);
	} else {
		size_t size = MAX(CO_GW_CON_SDO_UP_SIZE + n,
				sizeof(struct co_gw_con_sdo_up));
		int errc = get_errc();
		struct co_gw_con_sdo_up *con = malloc(size);
		if (con) {
			*con = (struct co_gw_con_sdo_up){ .size = size,
				.srv = req->srv,
				.data = req->data,
				.type = req->type,
				.len = n };
			memcpy(con->val, ptr, n);
			co_gw_send_srv(job->net->gw, (struct co_gw_srv *)con);

			free(con);
		} else {
			set_errc(errc);
			co_gw_send_con(job->net->gw, &job->req,
					CO_GW_IEC_NO_MEM, 0);
		}
	}

done:
	co_gw_job_destroy(job);
}

static void
co_gw_job_sdo_dn_con(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned32_t ac, void *data)
{
	(void)sdo;
	(void)idx;
	(void)subidx;
	struct co_gw_job *job = data;
	assert(job);
	assert(job->net);

	co_gw_job_remove(job);

	if (job->req.srv == CO_GW_SRV_SDO_DN)
		co_gw_send_con(job->net->gw, &job->req, 0, ac);

	co_gw_job_destroy(job);
}

static void
co_gw_job_sdo_ind(const co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, size_t size, size_t nbyte, void *data)
{
	(void)sdo;
	(void)idx;
	(void)subidx;
	struct co_gw_job *job = data;
	assert(job);
	assert(job->net);

	struct co_gw_ind_sdo ind = { .size = sizeof(ind),
		.srv = CO_GW_SRV_SDO,
		.net = job->net->id,
		.node = co_csdo_get_num(job->data),
		.nbyte = nbyte,
		.up = job->req.srv == CO_GW_SRV_SDO_UP,
		.data = job->req.data,
		._size = size };
	co_gw_send_srv(job->net->gw, (struct co_gw_srv *)&ind);
}

#endif // !LELY_NO_CO_CSDO

#if !LELY_NO_CO_MASTER && !LELY_NO_CO_LSS

static struct co_gw_job *
co_gw_job_create_lss(struct co_gw_job **pself, struct co_gw_net *net,
		const struct co_gw_req *req)
{
	assert(pself);
	assert(net);

	co_gw_t *gw = net->gw;

	if (*pself) {
		set_errnum(ERRNUM_BUSY);
		return NULL;
	}

	co_lss_t *lss = co_nmt_get_lss(net->nmt);
	if (!lss) {
		set_errnum(ERRNUM_PERM);
		return NULL;
	}

	// The LSS timeout is limited by the global gateway command timeout.
	int timeout = LELY_CO_LSS_TIMEOUT;
	if (gw->timeout)
		timeout = MIN(timeout, gw->timeout);
	co_lss_set_timeout(lss, timeout);

	return co_gw_job_create(pself, net, lss, NULL, req);
}

static void
co_gw_job_lss_cs_ind(co_lss_t *lss, co_unsigned8_t cs, void *data)
{
	(void)lss;
	struct co_gw_job *job = data;
	assert(job);
	assert(job->net);

	co_gw_job_remove(job);

	int iec = CO_GW_IEC_TIMEOUT;
	if (cs) {
		iec = CO_GW_IEC_LSS;
		switch (job->req.srv) {
		case CO_GW_SRV_LSS_SWITCH_SEL:
			if (cs == 0x44)
				iec = 0;
			break;
		case CO_GW_SRV_LSS_ID_SLAVE:
			if (cs == 0x4f)
				iec = 0;
			break;
		case CO_GW_SRV_LSS_ID_NON_CFG_SLAVE:
			if (cs == 0x50)
				iec = 0;
			break;
		}
	}
	co_gw_send_con(job->net->gw, &job->req, iec, 0);

	co_gw_job_destroy(job);
}

static void
co_gw_job_lss_err_ind(co_lss_t *lss, co_unsigned8_t cs, co_unsigned8_t err,
		co_unsigned8_t spec, void *data)
{
	(void)lss;
	(void)spec;
	struct co_gw_job *job = data;
	assert(job);
	assert(job->net);

	co_gw_job_remove(job);

	int iec = CO_GW_IEC_TIMEOUT;
	if (cs) {
		iec = CO_GW_IEC_LSS;
		switch (job->req.srv) {
		case CO_GW_SRV_LSS_SET_ID:
			if (cs == 0x11)
				iec = err ? CO_GW_IEC_LSS_ID : 0;
			break;
		case CO_GW_SRV_LSS_SET_RATE:
			if (cs == 0x13)
				iec = err ? CO_GW_IEC_LSS_RATE : 0;
			break;
		case CO_GW_SRV_LSS_STORE:
			if (cs == 0x17)
				iec = err ? CO_GW_IEC_LSS_PARAM : 0;
			break;
		}
	}
	co_gw_send_con(job->net->gw, &job->req, iec, 0);

	co_gw_job_destroy(job);
}

static void
co_gw_job_lss_lssid_ind(co_lss_t *lss, co_unsigned8_t cs, co_unsigned32_t id,
		void *data)
{
	(void)lss;
	struct co_gw_job *job = data;
	assert(job);
	assert(job->net);
	assert(job->req.srv == CO_GW_SRV_LSS_GET_LSSID);
	assert(job->req.size >= sizeof(struct co_gw_req_lss_get_lssid));

	struct co_gw_req_lss_get_lssid *req =
			(struct co_gw_req_lss_get_lssid *)&job->req;

	co_gw_job_remove(job);

	int iec = 0;
	if (!cs)
		iec = CO_GW_IEC_TIMEOUT;
	else if (cs != req->cs)
		iec = CO_GW_IEC_LSS;

	if (iec) {
		co_gw_send_con(job->net->gw, &job->req, iec, 0);
	} else {
		struct co_gw_con_lss_get_lssid con = { .size = sizeof(con),
			.srv = req->srv,
			.data = req->data,
			.id = id };
		co_gw_send_srv(job->net->gw, (struct co_gw_srv *)&con);
	}

	co_gw_job_destroy(job);
}

static void
co_gw_job_lss_nid_ind(
		co_lss_t *lss, co_unsigned8_t cs, co_unsigned8_t id, void *data)
{
	(void)lss;
	struct co_gw_job *job = data;
	assert(job);
	assert(job->net);
	assert(job->req.srv == CO_GW_SRV_LSS_GET_ID);

	co_gw_job_remove(job);

	int iec = 0;
	if (!cs)
		iec = CO_GW_IEC_TIMEOUT;
	else if (cs != 0x5e)
		iec = CO_GW_IEC_LSS;

	if (iec) {
		co_gw_send_con(job->net->gw, &job->req, iec, 0);
	} else {
		struct co_gw_con_lss_get_id con = { .size = sizeof(con),
			.srv = job->req.srv,
			.data = job->req.data,
			.id = id };
		co_gw_send_srv(job->net->gw, (struct co_gw_srv *)&con);
	}

	co_gw_job_destroy(job);
}

static void
co_gw_job_lss_scan_ind(co_lss_t *lss, co_unsigned8_t cs, const struct co_id *id,
		void *data)
{
	(void)lss;
	struct co_gw_job *job = data;
	assert(job);
	assert(job->net);

	co_gw_job_remove(job);

	int iec = CO_GW_IEC_TIMEOUT;
	if (cs) {
		iec = CO_GW_IEC_LSS;
		switch (job->req.srv) {
		case CO_GW_SRV__LSS_SLOWSCAN:
			if (cs == 0x44)
				iec = 0;
			break;
		case CO_GW_SRV__LSS_FASTSCAN:
			if (cs == 0x4f)
				iec = 0;
			break;
		}
	}

	if (iec) {
		co_gw_send_con(job->net->gw, &job->req, iec, 0);
	} else {
		assert(id);
		struct co_gw_con__lss_scan con = { .size = sizeof(con),
			.srv = job->req.srv,
			.data = job->req.data,
			.id = *id };
		co_gw_send_srv(job->net->gw, (struct co_gw_srv *)&con);
	}

	co_gw_job_destroy(job);
}

#endif // !LELY_NO_CO_MASTER && !LELY_NO_CO_LSS

#if !LELY_NO_CO_RPDO
static void
co_gw_net_rpdo_ind(co_rpdo_t *pdo, co_unsigned32_t ac, const void *ptr,
		size_t n, void *data)
{
	struct co_gw_net *net = data;
	assert(net);

	if (ac)
		return;

	struct co_gw_ind_rpdo ind = { .size = CO_GW_IND_RPDO_SIZE,
		.srv = CO_GW_SRV_RPDO,
		.net = net->id,
		.num = co_rpdo_get_num(pdo),
		.n = 0x40 };

	const struct co_pdo_map_par *par = co_rpdo_get_map_par(pdo);
	if (co_pdo_unmap(par, ptr, n, ind.val, &ind.n))
		return;
	ind.size += ind.n * sizeof(*ind.val);

	co_gw_send_srv(net->gw, (struct co_gw_srv *)&ind);
}
#endif

#if !LELY_NO_CO_CSDO

static int
co_gw_recv_sdo_up(co_gw_t *gw, co_unsigned16_t net, co_unsigned8_t node,
		const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_SDO_UP);
	assert(node && node <= CO_NUM_NODES);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_dev_t *dev = co_nmt_get_dev(nmt);

	if (req->size < sizeof(struct co_gw_req_sdo_up)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	int iec = 0;
	int errc = get_errc();

	struct co_gw_job *job = NULL;
	if (node == co_dev_get_id(dev)) {
		job = co_gw_job_create(&gw->net[net - 1]->sdo[node - 1],
				gw->net[net - 1], NULL, NULL, req);
		if (!job) {
			iec = errnum2iec(get_errnum());
			goto error_create_job;
		}
		const struct co_gw_req_sdo_up *par =
				(const struct co_gw_req_sdo_up *)&job->req;

		// clang-format off
		if (co_dev_up_req(dev, par->idx, par->subidx,
				&co_gw_job_sdo_up_con, job) == -1) {
			// clang-format on
			iec = errnum2iec(get_errnum());
			goto error_up_req;
		}
	} else {
		if (!co_nmt_is_master(gw->net[net - 1]->nmt)) {
			// TODO: Add client-SDO support for slaves where
			// possible.
			iec = CO_GW_IEC_BAD_NODE;
			goto error_srv;
		}

		job = co_gw_job_create_sdo(&gw->net[net - 1]->sdo[node - 1],
				gw->net[net - 1], node, req);
		if (!job) {
			iec = errnum2iec(get_errnum());
			goto error_create_job;
		}
		const struct co_gw_req_sdo_up *par =
				(const struct co_gw_req_sdo_up *)&job->req;

		co_csdo_set_up_ind(job->data, &co_gw_job_sdo_ind, job);
		// clang-format off
		if (co_csdo_up_req(job->data, par->idx, par->subidx,
				&co_gw_job_sdo_up_con, job) == -1) {
			// clang-format on
			iec = errnum2iec(get_errnum());
			goto error_up_req;
		}
	}

	return 0;

error_up_req:
	co_gw_job_destroy(job);
error_create_job:
	set_errc(errc);
error_srv:
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv_sdo_dn(co_gw_t *gw, co_unsigned16_t net, co_unsigned8_t node,
		const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_SDO_DN);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_dev_t *dev = co_nmt_get_dev(nmt);

	if (req->size < CO_GW_REQ_SDO_DN_SIZE) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_sdo_dn *par =
			(const struct co_gw_req_sdo_dn *)req;
	if (par->size < CO_GW_REQ_SDO_DN_SIZE + par->len) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	int iec = 0;
	int errc = get_errc();

	struct co_gw_job *job = NULL;
	if (node == co_dev_get_id(dev)) {
		job = co_gw_job_create(&gw->net[net - 1]->sdo[node - 1],
				gw->net[net - 1], NULL, NULL, req);
		if (!job) {
			iec = errnum2iec(get_errnum());
			goto error_create_job;
		}
		par = (const struct co_gw_req_sdo_dn *)&job->req;

		// clang-format off
		if (co_dev_dn_req(dev, par->idx, par->subidx, par->val,
				par->len, &co_gw_job_sdo_dn_con, job) == -1) {
			// clang-format on
			iec = errnum2iec(get_errnum());
			goto error_dn_req;
		}
	} else {
		if (!co_nmt_is_master(gw->net[net - 1]->nmt)) {
			// TODO: Add client-SDO support for slaves where
			// possible.
			iec = CO_GW_IEC_BAD_NODE;
			goto error_srv;
		}

		job = co_gw_job_create_sdo(&gw->net[net - 1]->sdo[node - 1],
				gw->net[net - 1], node, req);
		if (!job) {
			iec = errnum2iec(get_errnum());
			goto error_create_job;
		}
		par = (const struct co_gw_req_sdo_dn *)&job->req;

		co_csdo_set_dn_ind(job->data, &co_gw_job_sdo_ind, job);
		// clang-format off
		if (co_csdo_dn_req(job->data, par->idx, par->subidx, par->val,
				par->len, &co_gw_job_sdo_dn_con, job) == -1) {
			// clang-format on
			iec = errnum2iec(get_errnum());
			goto error_dn_req;
		}
	}

	return 0;

error_dn_req:
	co_gw_job_destroy(job);
error_create_job:
	set_errc(errc);
error_srv:
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv_set_sdo_timeout(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_SET_SDO_TIMEOUT);

#if !LELY_NO_CO_MASTER
	co_nmt_t *nmt = gw->net[net - 1]->nmt;
#endif

	if (req->size < sizeof(struct co_gw_req_set_sdo_timeout)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_set_sdo_timeout *par =
			(const struct co_gw_req_set_sdo_timeout *)req;

	gw->net[net - 1]->timeout = par->timeout;

#if !LELY_NO_CO_MASTER
	// The actual NMT SDO timeout is limited by the global gateway command
	// timeout.
	int timeout = par->timeout;
	if (gw->timeout)
		timeout = timeout ? MIN(timeout, gw->timeout) : gw->timeout;
	co_nmt_set_timeout(nmt, timeout);
#endif

	return co_gw_send_con(gw, req, 0, 0);
}

#endif /// LELY_NO_CO_CSDO

#if !LELY_NO_CO_RPDO
static int
co_gw_recv_set_rpdo(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_SET_RPDO);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_dev_t *dev = co_nmt_get_dev(nmt);

	if (req->size < CO_GW_REQ_SET_RPDO_SIZE) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_set_rpdo *par =
			(const struct co_gw_req_set_rpdo *)req;
	// clang-format off
	if (par->n > 0x40 || par->size < CO_GW_REQ_SET_RPDO_SIZE
			+ par->n * sizeof(*par->map)) {
		// clang-format on
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	struct co_pdo_comm_par comm = {
		.n = 2, .cobid = par->cobid, .trans = par->trans
	};

	struct co_pdo_map_par map = CO_PDO_MAP_PAR_INIT;
	map.n = par->n;
	for (co_unsigned8_t i = 0; i < par->n; i++)
		map.map[i] = par->map[i];

	co_unsigned32_t ac = co_dev_cfg_rpdo(dev, par->num, &comm, &map);

	return co_gw_send_con(gw, req, 0, ac);
}
#endif

#if !LELY_NO_CO_TPDO
static int
co_gw_recv_set_tpdo(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_SET_TPDO);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_dev_t *dev = co_nmt_get_dev(nmt);

	if (req->size < CO_GW_REQ_SET_TPDO_SIZE) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_set_tpdo *par =
			(const struct co_gw_req_set_tpdo *)req;
	// clang-format off
	if (par->n > 0x40 || par->size < CO_GW_REQ_SET_TPDO_SIZE
			+ par->n * sizeof(*par->map)) {
		// clang-format on
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	struct co_pdo_comm_par comm = { .n = 6,
		.cobid = par->cobid,
		.trans = par->trans,
		.inhibit = par->inhibit,
		.event = par->event,
		.sync = par->sync };

	struct co_pdo_map_par map = CO_PDO_MAP_PAR_INIT;
	map.n = par->n;
	for (co_unsigned8_t i = 0; i < par->n; i++)
		map.map[i] = par->map[i];

	co_unsigned32_t ac = co_dev_cfg_tpdo(dev, par->num, &comm, &map);

	return co_gw_send_con(gw, req, 0, ac);
}
#endif

#if !LELY_NO_CO_RPDO
static int
co_gw_recv_pdo_read(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_PDO_READ);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_dev_t *dev = co_nmt_get_dev(nmt);

	if (req->size < sizeof(struct co_gw_req_pdo_read)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_pdo_read *par =
			(const struct co_gw_req_pdo_read *)req;

	int iec = 0;
	co_unsigned32_t ac = 0;

	co_rpdo_t *pdo = co_nmt_get_rpdo(nmt, par->num);
	if (!pdo) {
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
	uint_least8_t buf[CAN_MAX_LEN];
	size_t n = sizeof(buf);
	ac = co_pdo_up(map, dev, &sdo_req, buf, &n);
	co_sdo_req_fini(&sdo_req);
	if (ac)
		goto error;

	struct co_gw_con_pdo_read con = { .size = sizeof(con),
		.srv = req->srv,
		.data = req->data,
		.net = net,
		.num = par->num };

	// Unmap the PDO values.
	ac = co_pdo_unmap(map, buf, n, con.val, &con.n);
	if (ac)
		goto error;

	con.size += con.n * sizeof(*con.val);
	return co_gw_send_srv(gw, (struct co_gw_srv *)&con);

error:
	return co_gw_send_con(gw, req, iec, ac);
}
#endif

#if !LELY_NO_CO_TPDO
static int
co_gw_recv_pdo_write(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_PDO_WRITE);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_dev_t *dev = co_nmt_get_dev(nmt);

	if (req->size < CO_GW_REQ_PDO_WRITE_SIZE) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_pdo_write *par =
			(const struct co_gw_req_pdo_write *)req;
	// clang-format off
	if (par->n > 0x40 || par->size < CO_GW_REQ_PDO_WRITE_SIZE
			+ par->n * sizeof(*par->val)) {
		// clang-format on
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	int iec = 0;
	co_unsigned32_t ac = 0;

	co_tpdo_t *pdo = co_nmt_get_tpdo(nmt, par->num);
	if (!pdo) {
		iec = CO_GW_IEC_INTERN;
		goto error;
	}
	const struct co_pdo_map_par *map = co_tpdo_get_map_par(pdo);

	// Map the values into a PDO.
	uint_least8_t buf[CAN_MAX_LEN] = { 0 };
	size_t n = sizeof(buf);
	ac = co_pdo_map(map, par->val, par->n, buf, &n);
	if (ac)
		goto error;

	// Write the mapped values to the object dictionary.
	struct co_sdo_req sdo_req = CO_SDO_REQ_INIT;
	ac = co_pdo_dn(map, dev, &sdo_req, buf, n);
	co_sdo_req_fini(&sdo_req);
	if (ac)
		goto error;

	// Trigger the event-based TPDO, if necessary.
	int errc = get_errc();
	if (co_tpdo_event(pdo) == -1) {
		iec = errnum2iec(get_errnum());
		set_errc(errc);
	}

error:
	return co_gw_send_con(gw, req, iec, ac);
}
#endif

#if !LELY_NO_CO_MASTER

static int
co_gw_recv_nmt_cs(co_gw_t *gw, co_unsigned16_t net, co_unsigned8_t node,
		co_unsigned8_t cs, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;

	int iec = 0;

	int errc = get_errc();
	if (co_nmt_cs_req(nmt, cs, node) == -1) {
		iec = errnum2iec(get_errnum());
		set_errc(errc);
	}

	return co_gw_send_con(gw, req, iec, 0);
}

#if !LELY_NO_CO_NG
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
		if (req->size < sizeof(struct co_gw_req_nmt_set_ng)) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		const struct co_gw_req_nmt_set_ng *par =
				(const struct co_gw_req_nmt_set_ng *)req;

		gt = par->gt;
		ltf = par->ltf;
	}

	int iec = 0;

	int errc = get_errc();
	if (co_nmt_ng_req(nmt, node, gt, ltf) == -1) {
		iec = errnum2iec(get_errnum());
		set_errc(errc);
	}

	return co_gw_send_con(gw, req, iec, 0);
}
#endif // !LELY_NO_CO_NG

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
		if (req->size < sizeof(struct co_gw_req_nmt_set_hb)) {
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

	if (req->size < sizeof(struct co_gw_req_init)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_init *par = (const struct co_gw_req_init *)req;

	int iec = 0;

	unsigned int baud = co_dev_get_baud(dev);
	co_unsigned16_t rate;
	switch (par->bitidx) {
	case 0:
		if (!(baud & CO_BAUD_1000)) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 1000;
		break;
	case 1:
		if (!(baud & CO_BAUD_800)) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 800;
		break;
	case 2:
		if (!(baud & CO_BAUD_500)) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 500;
		break;
	case 3:
		if (!(baud & CO_BAUD_250)) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 250;
		break;
	case 4:
		if (!(baud & CO_BAUD_125)) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 125;
		break;
	case 6:
		if (!(baud & CO_BAUD_50)) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 50;
		break;
	case 7:
		if (!(baud & CO_BAUD_20)) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 20;
		break;
	case 8:
		if (!(baud & CO_BAUD_10)) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 10;
		break;
	case 9:
		if (!(baud & CO_BAUD_AUTO)) {
			iec = CO_GW_IEC_LSS_RATE;
			goto error;
		}
		rate = 0;
		break;
	default: iec = CO_GW_IEC_LSS_RATE; goto error;
	}
	if (gw->rate_func)
		gw->rate_func(net, rate, gw->rate_data);

	int errc = get_errc();
	if (co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE) == -1) {
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

	if (req->size < sizeof(struct co_gw_req_set_hb)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_set_hb *par =
			(const struct co_gw_req_set_hb *)req;

	co_unsigned32_t ac = 0;

	co_obj_t *obj = co_dev_find_obj(dev, 0x1017);
	if (!obj) {
		ac = CO_SDO_AC_NO_OBJ;
		goto error;
	}
	co_sub_t *sub = co_obj_find_sub(obj, 0x00);
	if (!sub) {
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

	if (req->size < sizeof(struct co_gw_req_node)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_node *par = (const struct co_gw_req_node *)req;

	int iec = 0;

	if (!par->node || (par->node > CO_NUM_NODES && par->node != 0xff)) {
		iec = CO_GW_IEC_BAD_NODE;
		goto error;
	}

	co_nmt_set_id(nmt, par->node);

error:
	return co_gw_send_con(gw, req, iec, 0);
}

#if !LELY_NO_CO_EMCY
static int
co_gw_recv_set_emcy(co_gw_t *gw, co_unsigned16_t net, co_unsigned8_t node,
		const struct co_gw_req *req)
{
	assert(gw);
	assert(req);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_dev_t *dev = co_nmt_get_dev(nmt);

	if (req->size < sizeof(struct co_gw_req_set_emcy)) {
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
	if (!obj) {
		ac = CO_SDO_AC_NO_OBJ;
		goto error;
	}
	co_sub_t *sub = co_obj_find_sub(obj, node);
	if (!sub) {
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

	if (req->size < sizeof(struct co_gw_req_set_cmd_timeout)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_set_cmd_timeout *par =
			(const struct co_gw_req_set_cmd_timeout *)req;

	gw->timeout = par->timeout;

#if !LELY_NO_CO_MASTER
	for (co_unsigned16_t id = 1; id <= CO_GW_NUM_NET; id++) {
		if (!gw->net[id - 1])
			continue;
		// Limit the NMT SDO timeout for each network by the global
		// gateway command timeout.
		int timeout = gw->net[id - 1]->timeout;
		if (gw->timeout)
			// clang-format off
			timeout = timeout
					? MIN(timeout, gw->timeout)
					: gw->timeout;
		// clang-format on
		co_nmt_set_timeout(gw->net[id - 1]->nmt, timeout);
	}
#endif

	return co_gw_send_con(gw, req, 0, 0);
}

static int
co_gw_recv_set_bootup_ind(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_SET_BOOTUP_IND);

	if (req->size < sizeof(struct co_gw_req_set_bootup_ind)) {
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

	if (req->size < sizeof(struct co_gw_req_net)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_net *par = (const struct co_gw_req_net *)req;

	int iec = 0;

	if (par->net > CO_GW_NUM_NET) {
		iec = CO_GW_IEC_BAD_NET;
		goto error;
	}

	gw->def = par->net;

error:
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv_set_node(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_SET_NODE);

	if (req->size < sizeof(struct co_gw_req_node)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_node *par = (const struct co_gw_req_node *)req;

	int iec = 0;

	if (par->node > CO_NUM_NODES) {
		iec = CO_GW_IEC_BAD_NODE;
		goto error;
	}

	gw->net[net - 1]->def = par->node;

error:
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv_get_version(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_GET_VERSION);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_dev_t *dev = co_nmt_get_dev(nmt);

	struct co_gw_con_get_version con = { .size = sizeof(con),
		.srv = req->srv,
		.data = req->data,
		.vendor_id = co_dev_get_val_u32(dev, 0x1018, 0x01),
		.product_code = co_dev_get_val_u32(dev, 0x1018, 0x02),
		.revision = co_dev_get_val_u32(dev, 0x1018, 0x03),
		.serial_nr = co_dev_get_val_u32(dev, 0x1018, 0x04),
		.gw_class = co_nmt_is_master(gw->net[net - 1]->nmt) ? 3 : 1,
		.prot_hi = CO_GW_PROT_HI,
		.prot_lo = CO_GW_PROT_LO };
	return co_gw_send_srv(gw, (struct co_gw_srv *)&con);
}

#if !LELY_NO_CO_MASTER && !LELY_NO_CO_LSS

static int
co_gw_recv_lss_switch(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_LSS_SWITCH);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_lss_t *lss = co_nmt_get_lss(nmt);

	if (req->size < sizeof(struct co_gw_req_lss_switch)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_lss_switch *par =
			(const struct co_gw_req_lss_switch *)req;

	int iec = 0;

	if (!lss) {
		iec = CO_GW_IEC_BAD_SRV;
		goto error;
	}

	int errc = get_errc();
	if (co_lss_switch_req(lss, par->mode) == -1) {
		iec = errnum2iec(get_errnum());
		set_errc(errc);
	}

error:
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv_lss_switch_sel(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_LSS_SWITCH_SEL);

	if (req->size < sizeof(struct co_gw_req_lss_switch_sel)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_lss_switch_sel *par =
			(const struct co_gw_req_lss_switch_sel *)req;

	int iec = 0;
	int errc = get_errc();

	struct co_gw_job *job = co_gw_job_create_lss(
			&gw->net[net - 1]->lss, gw->net[net - 1], req);
	if (!job) {
		iec = errnum2iec(get_errnum());
		goto error_create_job;
	}

	// clang-format off
	if (co_lss_switch_sel_req(job->data, &par->id, &co_gw_job_lss_cs_ind,
			job) == -1) {
		// clang-format on
		iec = errnum2iec(get_errnum());
		goto error_switch_sel_req;
	}

	return 0;

error_switch_sel_req:
	co_gw_job_destroy(job);
error_create_job:
	set_errc(errc);
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv_lss_set_id(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_LSS_SET_ID);

	if (req->size < sizeof(struct co_gw_req_node)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_node *par = (const struct co_gw_req_node *)req;

	int iec = 0;
	int errc = get_errc();

	struct co_gw_job *job = co_gw_job_create_lss(
			&gw->net[net - 1]->lss, gw->net[net - 1], req);
	if (!job) {
		iec = errnum2iec(get_errnum());
		goto error_create_job;
	}

	if (co_lss_set_id_req(job->data, par->node, &co_gw_job_lss_err_ind, job)
			== -1) {
		iec = errnum2iec(get_errnum());
		goto error_set_id_req;
	}

	return 0;

error_set_id_req:
	co_gw_job_destroy(job);
error_create_job:
	set_errc(errc);
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv_lss_set_rate(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_LSS_SET_RATE);

	if (req->size < sizeof(struct co_gw_req_lss_set_rate)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_lss_set_rate *par =
			(const struct co_gw_req_lss_set_rate *)req;

	int iec = 0;

	if (par->bitsel) {
		iec = CO_GW_IEC_LSS_RATE;
		goto error_srv;
	}

	co_unsigned16_t rate;
	switch (par->bitidx) {
	case 0: rate = 1000; break;
	case 1: rate = 800; break;
	case 2: rate = 500; break;
	case 3: rate = 250; break;
	case 4: rate = 125; break;
	case 6: rate = 50; break;
	case 7: rate = 20; break;
	case 8: rate = 10; break;
	case 9: rate = 0; break;
	default: iec = CO_GW_IEC_LSS_RATE; goto error_srv;
	}

	int errc = get_errc();

	struct co_gw_job *job = co_gw_job_create_lss(
			&gw->net[net - 1]->lss, gw->net[net - 1], req);
	if (!job) {
		iec = errnum2iec(get_errnum());
		goto error_create_job;
	}

	if (co_lss_set_rate_req(job->data, rate, &co_gw_job_lss_err_ind, job)
			== -1) {
		iec = errnum2iec(get_errnum());
		goto error_set_rate_req;
	}

	return 0;

error_set_rate_req:
	co_gw_job_destroy(job);
error_create_job:
	set_errc(errc);
error_srv:
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv_lss_switch_rate(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_LSS_SWITCH_RATE);

	co_nmt_t *nmt = gw->net[net - 1]->nmt;
	co_lss_t *lss = co_nmt_get_lss(nmt);

	if (req->size < sizeof(struct co_gw_req_lss_switch_rate)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_lss_switch_rate *par =
			(const struct co_gw_req_lss_switch_rate *)req;

	int iec = 0;

	if (!lss) {
		iec = CO_GW_IEC_BAD_SRV;
		goto error;
	}

	int errc = get_errc();
	if (co_lss_switch_rate_req(lss, par->delay) == -1) {
		iec = errnum2iec(get_errnum());
		set_errc(errc);
	}

error:
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv_lss_store(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_LSS_STORE);

	int iec = 0;
	int errc = get_errc();

	struct co_gw_job *job = co_gw_job_create_lss(
			&gw->net[net - 1]->lss, gw->net[net - 1], req);
	if (!job) {
		iec = errnum2iec(get_errnum());
		goto error_create_job;
	}

	if (co_lss_store_req(job->data, &co_gw_job_lss_err_ind, job) == -1) {
		iec = errnum2iec(get_errnum());
		goto error_store_req;
	}

	return 0;

error_store_req:
	co_gw_job_destroy(job);
error_create_job:
	set_errc(errc);
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv_lss_get_lssid(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_LSS_GET_LSSID);

	if (req->size < sizeof(struct co_gw_req_lss_get_lssid)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_lss_get_lssid *par =
			(const struct co_gw_req_lss_get_lssid *)req;

	int iec = 0;
	int errc = get_errc();

	struct co_gw_job *job = co_gw_job_create_lss(
			&gw->net[net - 1]->lss, gw->net[net - 1], req);
	if (!job) {
		iec = errnum2iec(get_errnum());
		goto error_create_job;
	}

	switch (par->cs) {
	case 0x5a:
		// clang-format off
		if (co_lss_get_vendor_id_req(job->data,
				&co_gw_job_lss_lssid_ind, job) == -1) {
			// clang-format on
			iec = errnum2iec(get_errnum());
			goto error_switch_sel_req;
		}
		break;
	case 0x5b:
		// clang-format off
		if (co_lss_get_product_code_req(job->data,
				&co_gw_job_lss_lssid_ind, job) == -1) {
			// clang-format on
			iec = errnum2iec(get_errnum());
			goto error_switch_sel_req;
		}
		break;
	case 0x5c:
		// clang-format off
		if (co_lss_get_revision_req(job->data, &co_gw_job_lss_lssid_ind,
				job) == -1) {
			// clang-format on
			iec = errnum2iec(get_errnum());
			goto error_switch_sel_req;
		}
		break;
	case 0x5d:
		// clang-format off
		if (co_lss_get_serial_nr_req(job->data,
				&co_gw_job_lss_lssid_ind, job) == -1) {
			// clang-format on
			iec = errnum2iec(get_errnum());
			goto error_switch_sel_req;
		}
		break;
	default: iec = CO_GW_IEC_LSS; goto error_cs;
	}

	return 0;

error_switch_sel_req:
error_cs:
	co_gw_job_destroy(job);
error_create_job:
	set_errc(errc);
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv_lss_get_id(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_LSS_GET_ID);

	int iec = 0;
	int errc = get_errc();

	struct co_gw_job *job = co_gw_job_create_lss(
			&gw->net[net - 1]->lss, gw->net[net - 1], req);
	if (!job) {
		iec = errnum2iec(get_errnum());
		goto error_create_job;
	}

	if (co_lss_get_id_req(job->data, &co_gw_job_lss_nid_ind, job) == -1) {
		iec = errnum2iec(get_errnum());
		goto error_get_id_req;
	}

	return 0;

error_get_id_req:
	co_gw_job_destroy(job);
error_create_job:
	set_errc(errc);
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv_lss_id_slave(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_LSS_ID_SLAVE);

	if (req->size < sizeof(struct co_gw_req_lss_id_slave)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req_lss_id_slave *par =
			(const struct co_gw_req_lss_id_slave *)req;

	int iec = 0;
	int errc = get_errc();

	struct co_gw_job *job = co_gw_job_create_lss(
			&gw->net[net - 1]->lss, gw->net[net - 1], req);
	if (!job) {
		iec = errnum2iec(get_errnum());
		goto error_create_job;
	}

	// clang-format off
	if (co_lss_id_slave_req(job->data, &par->lo, &par->hi,
			&co_gw_job_lss_cs_ind, job) == -1) {
		// clang-format on
		iec = errnum2iec(get_errnum());
		goto error_id_slave_req;
	}

	return 0;

error_id_slave_req:
	co_gw_job_destroy(job);
error_create_job:
	set_errc(errc);
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv_lss_id_non_cfg_slave(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV_LSS_ID_NON_CFG_SLAVE);

	int iec = 0;
	int errc = get_errc();

	struct co_gw_job *job = co_gw_job_create_lss(
			&gw->net[net - 1]->lss, gw->net[net - 1], req);
	if (!job) {
		iec = errnum2iec(get_errnum());
		goto error_create_job;
	}

	if (co_lss_id_non_cfg_slave_req(job->data, &co_gw_job_lss_cs_ind, job)
			== -1) {
		iec = errnum2iec(get_errnum());
		goto error_id_non_cfg_slave_req;
	}

	return 0;

error_id_non_cfg_slave_req:
	co_gw_job_destroy(job);
error_create_job:
	set_errc(errc);
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv__lss_slowscan(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV__LSS_SLOWSCAN);

	if (req->size < sizeof(struct co_gw_req__lss_scan)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req__lss_scan *par =
			(const struct co_gw_req__lss_scan *)req;

	int iec = 0;
	int errc = get_errc();

	struct co_gw_job *job = co_gw_job_create_lss(
			&gw->net[net - 1]->lss, gw->net[net - 1], req);
	if (!job) {
		iec = errnum2iec(get_errnum());
		goto error_create_job;
	}

	// clang-format off
	if (co_lss_slowscan_req(job->data, &par->id_1, &par->id_2,
			&co_gw_job_lss_scan_ind, job) == -1) {
		// clang-format on
		iec = errnum2iec(get_errnum());
		goto error_slowscan_req;
	}

	return 0;

error_slowscan_req:
	co_gw_job_destroy(job);
error_create_job:
	set_errc(errc);
	return co_gw_send_con(gw, req, iec, 0);
}

static int
co_gw_recv__lss_fastscan(
		co_gw_t *gw, co_unsigned16_t net, const struct co_gw_req *req)
{
	assert(gw);
	assert(net && net <= CO_GW_NUM_NET && gw->net[net - 1]);
	assert(req);
	assert(req->srv == CO_GW_SRV__LSS_FASTSCAN);

	if (req->size < sizeof(struct co_gw_req__lss_scan)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_req__lss_scan *par =
			(const struct co_gw_req__lss_scan *)req;

	int iec = 0;
	int errc = get_errc();

	struct co_gw_job *job = co_gw_job_create_lss(
			&gw->net[net - 1]->lss, gw->net[net - 1], req);
	if (!job) {
		iec = errnum2iec(get_errnum());
		goto error_create_job;
	}

	// clang-format off
	if (co_lss_fastscan_req(job->data, &par->id_1, &par->id_2,
			&co_gw_job_lss_scan_ind, job) == -1) {
		// clang-format on
		iec = errnum2iec(get_errnum());
		goto error_fastscan_req;
	}

	return 0;

error_fastscan_req:
	co_gw_job_destroy(job);
error_create_job:
	set_errc(errc);
	return co_gw_send_con(gw, req, iec, 0);
}

#endif // !LELY_NO_CO_MASTER && !LELY_NO_CO_LSS

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
	case CO_SDO_AC_PDO_LEN: ac = 0; iec = CO_GW_IEC_PDO_LEN;
	}

	// Copy the service number and user-specified data from the request to
	// the confirmation.
	struct co_gw_con con = { .size = sizeof(con),
		.srv = req->srv,
		.data = req->data,
		.iec = iec,
		.ac = ac };
	return co_gw_send_srv(gw, (struct co_gw_srv *)&con);
}

static int
co_gw_send_ec(co_gw_t *gw, co_unsigned16_t net, co_unsigned8_t node,
		co_unsigned8_t st, int iec)
{
	struct co_gw_ind_ec ind = { .size = sizeof(ind),
		.srv = CO_GW_SRV_EC,
		.net = net,
		.node = node,
		.st = st,
		.iec = iec };
	return co_gw_send_srv(gw, (struct co_gw_srv *)&ind);
}

static int
co_gw_send_srv(co_gw_t *gw, const struct co_gw_srv *srv)
{
	assert(gw);
	assert(srv);

	if (!gw->send_func) {
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
