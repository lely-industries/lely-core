/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the ASCII gateway functions.
 *
 * @see lely/co/gw_txt.h
 *
 * @copyright 2017-2019 Lely Industries N.V.
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

#if !LELY_NO_CO_GW_TXT

#include <lely/co/dev.h>
#include <lely/co/gw_txt.h>
#include <lely/co/nmt.h>
#include <lely/co/pdo.h>
#include <lely/co/sdo.h>
#include <lely/co/val.h>
#include <lely/libc/stdio.h>
#include <lely/libc/strings.h>
#include <lely/util/diag.h>
#include <lely/util/lex.h>
#include <lely/util/print.h>

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

/// A CANopen ASCII gateway.
struct __co_gw_txt {
	/// The last internal error code.
	int iec;
	/// The number of pending requests.
	size_t pending;
	/// A pointer to the callback function invoked by co_gw_txt_recv().
	co_gw_txt_recv_func_t *recv_func;
	/// A pointer to the user-specified data for #recv_func.
	void *recv_data;
	/// A pointer to the callback function invoked by co_gw_txt_send().
	co_gw_txt_send_func_t *send_func;
	/// A pointer to the user-specified data for #send_func.
	void *send_data;
};

/// Processes a confirmation.
static int co_gw_txt_recv_con(co_gw_txt_t *gw, co_unsigned32_t seq,
		const struct co_gw_con *con);

/// Processes an 'SDO upload' confirmation.
static int co_gw_txt_recv_sdo_up(co_gw_txt_t *gw, co_unsigned32_t seq,
		const struct co_gw_con_sdo_up *con);

/// Processes a 'Read PDO data' confirmation.
static int co_gw_txt_recv_pdo_read(co_gw_txt_t *gw, co_unsigned32_t seq,
		const struct co_gw_con_pdo_read *con);

/// Processes a 'Get version' confirmation.
static int co_gw_txt_recv_get_version(co_gw_txt_t *gw, co_unsigned32_t seq,
		const struct co_gw_con_get_version *con);

/// Processes an 'Inquire LSS address' confirmation.
static int co_gw_txt_recv_lss_get_lssid(co_gw_txt_t *gw, co_unsigned32_t seq,
		const struct co_gw_con_lss_get_lssid *con);
/// Processes an 'LSS inquire node-ID' confirmation.
static int co_gw_txt_recv_lss_get_id(co_gw_txt_t *gw, co_unsigned32_t seq,
		const struct co_gw_con_lss_get_id *con);

/// Processes an 'LSS Slowscan/Fastscan' confirmation.
static int co_gw_txt_recv__lss_scan(co_gw_txt_t *gw, co_unsigned32_t seq,
		const struct co_gw_con__lss_scan *con);

/**
 * Processes a confirmation with a non-zero internal error code or SDO abort
 * code.
 */
static int co_gw_txt_recv_err(co_gw_txt_t *gw, co_unsigned32_t seq, int iec,
		co_unsigned32_t ac);

/// Processes an 'RPDO received' indication.
static int co_gw_txt_recv_rpdo(
		co_gw_txt_t *gw, const struct co_gw_ind_rpdo *ind);

/// Processes an 'Error control event received' indication.
static int co_gw_txt_recv_ec(co_gw_txt_t *gw, const struct co_gw_ind_ec *ind);

/// Processes an 'Emergency event received' indication.
static int co_gw_txt_recv_emcy(
		co_gw_txt_t *gw, const struct co_gw_ind_emcy *ind);

/// Processes an 'CiA 301 progress indication download' indication.
static int co_gw_txt_recv_sdo(co_gw_txt_t *gw, const struct co_gw_ind_sdo *ind);

/// Processes a 'Boot slave process completed' indication.
static int co_gw_txt_recv__boot(
		co_gw_txt_t *gw, const struct co_gw_ind__boot *ind);

/**
 * Formats a received indication or confirmation and invokes the callback
 * function to process it.
 *
 * @see co_gw_txt_recv_txt()
 */
static int co_gw_txt_recv_fmt(co_gw_txt_t *gw, const char *format, ...)
		format_printf__(2, 3);

/**
 * Invokes the callback function to process a received indication or
 * confirmation.
 */
static int co_gw_txt_recv_txt(co_gw_txt_t *gw, const char *txt);

/// Prints a CANopen value.
static size_t co_gw_txt_print_val(char **pbegin, char *end,
		co_unsigned16_t type, const void *val);

/// Invokes the callback function to send a request.
static void co_gw_txt_send_req(co_gw_txt_t *gw, const struct co_gw_req *req);

/// Sends an 'SDO upload' request after parsing its parameters.
static size_t co_gw_txt_send_sdo_up(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, co_unsigned8_t node, const char *begin,
		const char *end, struct floc *at);
/// Sends an 'SDO download' request after parsing its parameters.
static size_t co_gw_txt_send_sdo_dn(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, co_unsigned8_t node, const char *begin,
		const char *end, struct floc *at);
/// Sends a 'Configure SDO time-out' request after parsing its parameters.
static size_t co_gw_txt_send_set_sdo_timeout(co_gw_txt_t *gw, int srv,
		void *data, co_unsigned16_t net, const char *begin,
		const char *end, struct floc *at);

/// Sends a 'Configure RPDO' request after parsing its parameters.
static size_t co_gw_txt_send_set_rpdo(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);
/// Sends a 'Configure TPDO' request after parsing its parameters.
static size_t co_gw_txt_send_set_tpdo(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);
/// Sends a 'Read PDO data' request after parsing its parameters.
static size_t co_gw_txt_send_pdo_read(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);
/// Sends a 'Write PDO data' request after parsing its parameters.
static size_t co_gw_txt_send_pdo_write(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);

/// Sends an 'Enable node guarding' request after parsing its parameters.
static size_t co_gw_txt_send_nmt_set_ng(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, co_unsigned8_t node, const char *begin,
		const char *end, struct floc *at);
/// Sends a 'Start heartbeat consumer' request after parsing its parameters.
static size_t co_gw_txt_send_nmt_set_hb(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, co_unsigned8_t node, const char *begin,
		const char *end, struct floc *at);

/// Sends an 'Initialize gateway' request after parsing its parameters.
static size_t co_gw_txt_send_init(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);
/// Sends a 'Set heartbeat producer' request after parsing its parameters.
static size_t co_gw_txt_send_set_hb(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);
/// Sends a 'Set node-ID' request after parsing its parameters.
static size_t co_gw_txt_send_set_id(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);
/// Sends a 'Set command time-out' request after parsin gits parameters.
static size_t co_gw_txt_send_set_cmd_timeout(co_gw_txt_t *gw, int srv,
		void *data, const char *begin, const char *end,
		struct floc *at);
/// Sends a 'Boot-up forwarding' request after parsing its parameters.
static size_t co_gw_txt_send_set_bootup_ind(co_gw_txt_t *gw, int srv,
		void *data, co_unsigned16_t net, const char *begin,
		const char *end, struct floc *at);

/// Sends a 'Set default network' request after parsing its parameters.
static size_t co_gw_txt_send_set_net(co_gw_txt_t *gw, int srv, void *data,
		const char *begin, const char *end, struct floc *at);
/// Sends a 'Set default node-ID' request after parsing its parameters.
static size_t co_gw_txt_send_set_node(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);
/// Sends a 'Set command size' request after parsing its parameters.
static size_t co_gw_txt_send_set_cmd_size(co_gw_txt_t *gw, int srv, void *data,
		const char *begin, const char *end, struct floc *at);

/// Sends an 'LSS switch state global' request after parsing its parameters.
static size_t co_gw_txt_send_lss_switch(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);
/// Sends an 'LSS switch state selective' request after parsing its parameters.
static size_t co_gw_txt_send_lss_switch_sel(co_gw_txt_t *gw, int srv,
		void *data, co_unsigned16_t net, const char *begin,
		const char *end, struct floc *at);
/// Sends an 'LSS configure node-ID' request after parsing its parameters.
static size_t co_gw_txt_send_lss_set_id(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);
/// Sends an 'LSS configure bit-rate' request after parsing its parameters.
static size_t co_gw_txt_send_lss_set_rate(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);
/// Sends an 'LSS activate new bit-rate' request after parsing its parameters.
static size_t co_gw_txt_send_lss_switch_rate(co_gw_txt_t *gw, int srv,
		void *data, co_unsigned16_t net, const char *begin,
		const char *end, struct floc *at);
/// Sends an 'Inquire LSS address' request after parsing its parameters.
static size_t co_gw_txt_send_lss_get_lssid(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);
/// Sends an 'LSS identify remote slave' request after parsing its parameters.
static size_t co_gw_txt_send_lss_id_slave(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);

/// Sends an 'LSS Slowscan' request after parsing its parameters.
static size_t co_gw_txt_send__lss_slowscan(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);
/// Sends an 'LSS Fastscan' request after parsing its parameters.
static size_t co_gw_txt_send__lss_fastscan(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);

/**
 * Lexes the prefix (sequence number and optional network and node-ID) of a
 * request.
 */
static size_t co_gw_txt_lex_prefix(const char *begin, const char *end,
		struct floc *at, co_unsigned32_t *pseq, co_unsigned16_t *pnet,
		co_unsigned8_t *pnode);

/// Lexes the service number of a request.
static size_t co_gw_txt_lex_srv(
		const char *begin, const char *end, struct floc *at, int *psrv);

/// Lexes a single command.
static size_t co_gw_txt_lex_cmd(
		const char *begin, const char *end, struct floc *at);

/// Lexes the multiplexer and data type of an SDO upload/download request.
static size_t co_gw_txt_lex_sdo(const char *begin, const char *end,
		struct floc *at, co_unsigned16_t *pidx, co_unsigned8_t *psubidx,
		co_unsigned16_t *ptype);

/// Lexes the communication and mapping parameters of a configure PDO request.
static size_t co_gw_txt_lex_pdo(const char *begin, const char *end,
		struct floc *at, int ext, co_unsigned16_t *pnum,
		struct co_pdo_comm_par *pcomm, struct co_pdo_map_par *pmap);

/// Lexes a data type.
static size_t co_gw_txt_lex_type(const char *begin, const char *end,
		struct floc *at, co_unsigned16_t *ptype);

/// Lexes a value for an SDO download request.
static size_t co_gw_txt_lex_val(const char *begin, const char *end,
		struct floc *at, co_unsigned16_t type, void *val);

/**
 * Lexes an array of visible characters (#CO_DEFTYPE_VISIBLE_STRING), excluding
 * the delimiting quotes, for an SDO download request.
 */
static size_t co_gw_txt_lex_vs(const char *begin, const char *end,
		struct floc *at, char *s, size_t *pn);

/// Lexes the transmission type of a configure PDO request.
static size_t co_gw_txt_lex_trans(const char *begin, const char *end,
		struct floc *at, co_unsigned8_t *ptrans);

/// Lexes an LSS address.
static size_t co_gw_txt_lex_id(const char *begin, const char *end,
		struct floc *at, struct co_id *pid);

/// Lexes an LSS address range.
static size_t co_gw_txt_lex_id_sel(const char *begin, const char *end,
		struct floc *at, struct co_id *plo, struct co_id *phi);

void *
__co_gw_txt_alloc(void)
{
	void *ptr = malloc(sizeof(struct __co_gw_txt));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
__co_gw_txt_free(void *ptr)
{
	free(ptr);
}

struct __co_gw_txt *
__co_gw_txt_init(struct __co_gw_txt *gw)
{
	assert(gw);

	gw->iec = 0;
	gw->pending = 0;

	gw->recv_func = NULL;
	gw->recv_data = NULL;

	gw->send_func = NULL;
	gw->send_data = NULL;

	return gw;
}

void
__co_gw_txt_fini(struct __co_gw_txt *gw)
{
	(void)gw;
}

co_gw_txt_t *
co_gw_txt_create(void)
{
	int errc = 0;

	co_gw_txt_t *gw = __co_gw_txt_alloc();
	if (!gw) {
		errc = get_errc();
		goto error_alloc_gw;
	}

	if (!__co_gw_txt_init(gw)) {
		errc = get_errc();
		goto error_init_gw;
	}

	return gw;

error_init_gw:
	__co_gw_txt_free(gw);
error_alloc_gw:
	set_errc(errc);
	return NULL;
}

void
co_gw_txt_destroy(co_gw_txt_t *gw)
{
	if (gw) {
		__co_gw_txt_fini(gw);
		__co_gw_txt_free(gw);
	}
}

int
co_gw_txt_iec(co_gw_txt_t *gw)
{
	assert(gw);

	int iec = gw->iec;
	gw->iec = 0;
	return iec;
}

size_t
co_gw_txt_pending(const co_gw_txt_t *gw)
{
	assert(gw);

	return gw->pending;
}

int
co_gw_txt_recv(co_gw_txt_t *gw, const struct co_gw_srv *srv)
{
	assert(srv);

	if (srv->size < sizeof(*srv)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	switch (srv->srv) {
	case CO_GW_SRV_SDO_UP:
	case CO_GW_SRV_SDO_DN:
	case CO_GW_SRV_SET_SDO_TIMEOUT:
	case CO_GW_SRV_SET_RPDO:
	case CO_GW_SRV_SET_TPDO:
	case CO_GW_SRV_PDO_READ:
	case CO_GW_SRV_PDO_WRITE:
	case CO_GW_SRV_NMT_START:
	case CO_GW_SRV_NMT_STOP:
	case CO_GW_SRV_NMT_ENTER_PREOP:
	case CO_GW_SRV_NMT_RESET_NODE:
	case CO_GW_SRV_NMT_RESET_COMM:
	case CO_GW_SRV_NMT_NG_ENABLE:
	case CO_GW_SRV_NMT_NG_DISABLE:
	case CO_GW_SRV_NMT_HB_ENABLE:
	case CO_GW_SRV_NMT_HB_DISABLE:
	case CO_GW_SRV_INIT:
	case CO_GW_SRV_SET_HB:
	case CO_GW_SRV_SET_ID:
	case CO_GW_SRV_SET_CMD_TIMEOUT:
	case CO_GW_SRV_SET_BOOTUP_IND:
	case CO_GW_SRV_SET_NET:
	case CO_GW_SRV_SET_NODE:
	case CO_GW_SRV_GET_VERSION:
	case CO_GW_SRV_SET_CMD_SIZE:
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
		if (srv->size < sizeof(struct co_gw_con)) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		const struct co_gw_con *con = (const struct co_gw_con *)srv;
		co_unsigned32_t seq = (uintptr_t)con->data;
		return co_gw_txt_recv_con(gw, seq, con);
	case CO_GW_SRV_RPDO:
		if (srv->size < CO_GW_IND_RPDO_SIZE) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		return co_gw_txt_recv_rpdo(
				gw, (const struct co_gw_ind_rpdo *)srv);
	case CO_GW_SRV_EC:
		if (srv->size < sizeof(struct co_gw_ind_ec)) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		return co_gw_txt_recv_ec(gw, (const struct co_gw_ind_ec *)srv);
	case CO_GW_SRV_EMCY:
		if (srv->size < sizeof(struct co_gw_ind_emcy)) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		return co_gw_txt_recv_emcy(
				gw, (const struct co_gw_ind_emcy *)srv);
	case CO_GW_SRV_SDO:
		if (srv->size < sizeof(struct co_gw_ind_sdo)) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		return co_gw_txt_recv_sdo(
				gw, (const struct co_gw_ind_sdo *)srv);
	case CO_GW_SRV__SYNC:
	case CO_GW_SRV__TIME:
		// Ignore synchronization and time stamp events.
		return 0;
	case CO_GW_SRV__BOOT:
		if (srv->size < sizeof(struct co_gw_ind__boot)) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		return co_gw_txt_recv__boot(
				gw, (const struct co_gw_ind__boot *)srv);
	default: set_errnum(ERRNUM_INVAL); return -1;
	}
}

void
co_gw_txt_get_recv_func(const co_gw_txt_t *gw, co_gw_txt_recv_func_t **pfunc,
		void **pdata)
{
	assert(gw);

	if (pfunc)
		*pfunc = gw->recv_func;
	if (pdata)
		*pdata = gw->recv_data;
}

size_t
co_gw_txt_send(co_gw_txt_t *gw, const char *begin, const char *end,
		struct floc *at)
{
	assert(gw);
	assert(begin);
	assert(!end || end >= begin);

	struct floc *floc = NULL;
	struct floc floc_;
	if (at) {
		floc = &floc_;
		*floc = *at;
	}

	int iec = CO_GW_IEC_SYNTAX;

	const char *cp = begin;
	size_t chars = 0;

	// Skip leading whitespace and/or comments.
	for (;;) {
		cp += lex_ctype(&isspace, cp, end, floc);
		if (!(chars = lex_line_comment("#", cp, end, floc)))
			break;
		cp += chars;
	}

	// Ignore empty requests.
	if ((end && cp >= end) || !*cp)
		goto done;

	co_unsigned32_t seq = 0;
	co_unsigned16_t net = 0;
	co_unsigned8_t node = 0xff;
	if (!(chars = co_gw_txt_lex_prefix(cp, end, floc, &seq, &net, &node)))
		goto error;
	cp += chars;
	void *data = (void *)(uintptr_t)seq;

	int srv = 0;
	if (!(chars = co_gw_txt_lex_srv(cp, end, floc, &srv)))
		goto error;
	cp += chars;

	switch (srv) {
	case CO_GW_SRV_SET_CMD_TIMEOUT:
	case CO_GW_SRV_SET_NET:
	case CO_GW_SRV_GET_VERSION:
	case CO_GW_SRV_SET_CMD_SIZE:
		if (node != 0xff) {
			diag_if(DIAG_ERROR, 0, floc,
					"node-ID specified before global command");
			goto error;
		}
		if (net) {
			diag_if(DIAG_ERROR, 0, floc,
					"network-ID specified before global command");
			goto error;
		}
		break;
	case CO_GW_SRV_SET_SDO_TIMEOUT:
	case CO_GW_SRV_SET_RPDO:
	case CO_GW_SRV_SET_TPDO:
	case CO_GW_SRV_INIT:
	case CO_GW_SRV_SET_HB:
	case CO_GW_SRV_SET_ID:
	case CO_GW_SRV_SET_BOOTUP_IND:
	case CO_GW_SRV_SET_NODE:
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
		// A single number preceding the command is normally interpreted
		// as the node-ID. However, in this case we take it to be the
		// network-ID.
		if (net) {
			diag_if(DIAG_ERROR, 0, floc,
					"node-ID specified before network-level command");
			goto error;
		}
		net = node == 0xff ? 0 : node;
		node = 0xff;
		break;
	}

	cp += lex_ctype(&isblank, cp, end, at);

	chars = 0;
	switch (srv) {
	case CO_GW_SRV_SDO_UP:
		chars = co_gw_txt_send_sdo_up(
				gw, srv, data, net, node, cp, end, floc);
		break;
	case CO_GW_SRV_SDO_DN:
		chars = co_gw_txt_send_sdo_dn(
				gw, srv, data, net, node, cp, end, floc);
		break;
	case CO_GW_SRV_SET_SDO_TIMEOUT:
		chars = co_gw_txt_send_set_sdo_timeout(
				gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV_SET_RPDO:
		chars = co_gw_txt_send_set_rpdo(
				gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV_SET_TPDO:
		chars = co_gw_txt_send_set_tpdo(
				gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV_PDO_READ:
		chars = co_gw_txt_send_pdo_read(
				gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV_PDO_WRITE:
		chars = co_gw_txt_send_pdo_write(
				gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV_NMT_START:
	case CO_GW_SRV_NMT_STOP:
	case CO_GW_SRV_NMT_ENTER_PREOP:
	case CO_GW_SRV_NMT_RESET_NODE:
	case CO_GW_SRV_NMT_RESET_COMM:
	case CO_GW_SRV_NMT_NG_DISABLE:
	case CO_GW_SRV_NMT_HB_DISABLE: {
		// 'start', 'stop', 'preop[erational]', 'reset node',
		// 'reset comm[unication]', 'disable guarding' and
		// 'disable heartbeat' are node-level commands without any
		// additional parameters.
		struct co_gw_req_node req = { .size = sizeof(req),
			.srv = srv,
			.data = data,
			.net = net,
			.node = node };
		co_gw_txt_send_req(gw, (struct co_gw_req *)&req);
		goto done;
	}
	case CO_GW_SRV_NMT_NG_ENABLE:
		chars = co_gw_txt_send_nmt_set_ng(
				gw, srv, data, net, node, cp, end, floc);
		break;
	case CO_GW_SRV_NMT_HB_ENABLE:
		chars = co_gw_txt_send_nmt_set_hb(
				gw, srv, data, net, node, cp, end, floc);
		break;
	case CO_GW_SRV_INIT:
		chars = co_gw_txt_send_init(gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV_SET_HB:
		chars = co_gw_txt_send_set_hb(
				gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV_SET_ID:
		chars = co_gw_txt_send_set_id(
				gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV_SET_CMD_TIMEOUT:
		chars = co_gw_txt_send_set_cmd_timeout(
				gw, srv, data, cp, end, floc);
		break;
	case CO_GW_SRV_SET_BOOTUP_IND:
		chars = co_gw_txt_send_set_bootup_ind(
				gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV_SET_NET:
		chars = co_gw_txt_send_set_net(gw, srv, data, cp, end, floc);
		break;
	case CO_GW_SRV_SET_NODE:
		chars = co_gw_txt_send_set_node(
				gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV_GET_VERSION:
	case CO_GW_SRV_LSS_STORE:
	case CO_GW_SRV_LSS_GET_ID:
	case CO_GW_SRV_LSS_ID_NON_CFG_SLAVE: {
		// 'info version', 'lss_store', 'lss_get_node' and
		// 'lss_ident_nonconf' are network-level commands without any
		// additional parameters.
		struct co_gw_req_net req = { .size = sizeof(req),
			.srv = srv,
			.data = data,
			.net = net };
		co_gw_txt_send_req(gw, (struct co_gw_req *)&req);
		goto done;
	}
	case CO_GW_SRV_SET_CMD_SIZE:
		chars = co_gw_txt_send_set_cmd_size(
				gw, srv, data, cp, end, floc);
		break;
	case CO_GW_SRV_LSS_SWITCH:
		chars = co_gw_txt_send_lss_switch(
				gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV_LSS_SWITCH_SEL:
		chars = co_gw_txt_send_lss_switch_sel(
				gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV_LSS_SET_ID:
		chars = co_gw_txt_send_lss_set_id(
				gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV_LSS_SET_RATE:
		chars = co_gw_txt_send_lss_set_rate(
				gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV_LSS_SWITCH_RATE:
		chars = co_gw_txt_send_lss_switch_rate(
				gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV_LSS_GET_LSSID:
		chars = co_gw_txt_send_lss_get_lssid(
				gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV_LSS_ID_SLAVE:
		chars = co_gw_txt_send_lss_id_slave(
				gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV__LSS_SLOWSCAN:
		chars = co_gw_txt_send__lss_slowscan(
				gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV__LSS_FASTSCAN:
		chars = co_gw_txt_send__lss_fastscan(
				gw, srv, data, net, cp, end, floc);
		break;
	}
	if (!chars)
		goto error;
	cp += chars;

	// Skip trailing whitespace and/or comments.
	cp += lex_ctype(&isblank, cp, end, floc);
	cp += lex_line_comment("#", cp, end, floc);

done:
	if ((!end || cp < end) && *cp && !isbreak((unsigned char)*cp))
		diag_if(DIAG_ERROR, 0, floc,
				"expected line break after request");

	iec = 0;

error:
	// Skip all characters until (and including) the next line break.
	chars = 0;
	while ((!end || cp + chars < end) && cp[chars] && cp[chars++] != '\n')
		;
	cp += chars;

	if (iec)
		gw->iec = iec;

	return floc_lex(at, begin, cp);
}

void
co_gw_txt_set_recv_func(
		co_gw_txt_t *gw, co_gw_txt_recv_func_t *func, void *data)
{
	assert(gw);

	gw->recv_func = func;
	gw->recv_data = data;
}

void
co_gw_txt_get_send_func(const co_gw_txt_t *gw, co_gw_txt_send_func_t **pfunc,
		void **pdata)
{
	assert(gw);

	if (pfunc)
		*pfunc = gw->send_func;
	if (pdata)
		*pdata = gw->send_data;
}

void
co_gw_txt_set_send_func(
		co_gw_txt_t *gw, co_gw_txt_send_func_t *func, void *data)
{
	assert(gw);

	gw->send_func = func;
	gw->send_data = data;
}

static int
co_gw_txt_recv_con(co_gw_txt_t *gw, co_unsigned32_t seq,
		const struct co_gw_con *con)
{
	assert(gw);
	assert(con);

	if (gw->pending)
		gw->pending--;

	if (con->iec || con->ac)
		return co_gw_txt_recv_err(gw, seq, con->iec, con->ac);

	switch (con->srv) {
	case CO_GW_SRV_SDO_UP:
		if (con->size < CO_GW_CON_SDO_UP_SIZE) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		return co_gw_txt_recv_sdo_up(
				gw, seq, (const struct co_gw_con_sdo_up *)con);
	case CO_GW_SRV_PDO_READ:
		if (con->size < CO_GW_CON_PDO_READ_SIZE) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		return co_gw_txt_recv_pdo_read(gw, seq,
				(const struct co_gw_con_pdo_read *)con);
	case CO_GW_SRV_GET_VERSION:
		if (con->size < sizeof(struct co_gw_con_get_version)) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		return co_gw_txt_recv_get_version(gw, seq,
				(const struct co_gw_con_get_version *)con);
	case CO_GW_SRV_LSS_GET_LSSID:
		if (con->size < sizeof(struct co_gw_con_lss_get_lssid)) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		return co_gw_txt_recv_lss_get_lssid(gw, seq,
				(const struct co_gw_con_lss_get_lssid *)con);
	case CO_GW_SRV_LSS_GET_ID:
		if (con->size < sizeof(struct co_gw_con_lss_get_id)) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		return co_gw_txt_recv_lss_get_id(gw, seq,
				(const struct co_gw_con_lss_get_id *)con);
	case CO_GW_SRV__LSS_SLOWSCAN:
	case CO_GW_SRV__LSS_FASTSCAN:
		if (con->size < sizeof(struct co_gw_con__lss_scan)) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		return co_gw_txt_recv__lss_scan(gw, seq,
				(const struct co_gw_con__lss_scan *)con);
	default: return co_gw_txt_recv_err(gw, seq, 0, 0);
	}
}

static int
co_gw_txt_recv_sdo_up(co_gw_txt_t *gw, co_unsigned32_t seq,
		const struct co_gw_con_sdo_up *con)
{
	assert(con);
	assert(con->srv == CO_GW_SRV_SDO_UP);

	if (con->size < CO_GW_CON_SDO_UP_SIZE + con->len) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	union co_val val;
	const uint_least8_t *bp = con->val;
	if (co_val_read(con->type, &val, bp, bp + con->len) != con->len)
		co_gw_txt_recv_err(gw, seq, 0, CO_SDO_AC_TYPE_LEN);

	size_t chars = co_gw_txt_print_val(NULL, NULL, con->type, &val);
#if __STDC_NO_VLA__
	int result = -1;
	char *buf = malloc(chars + 1);
	if (buf) {
		char *cp = buf;
		co_gw_txt_print_val(&cp, cp + chars, con->type, &val);
		*cp = '\0';

		result = co_gw_txt_recv_fmt(gw, "[%" PRIu32 "] %s", seq, buf);

		free(buf);
	}
#else
	char buf[chars + 1];
	char *cp = buf;
	co_gw_txt_print_val(&cp, cp + chars, con->type, &val);
	*cp = '\0';

	int result = co_gw_txt_recv_fmt(gw, "[%" PRIu32 "] %s", seq, buf);
#endif

	co_val_fini(con->type, &val);

	return result;
}

static int
co_gw_txt_recv_pdo_read(co_gw_txt_t *gw, co_unsigned32_t seq,
		const struct co_gw_con_pdo_read *con)
{
	assert(con);
	assert(con->srv == CO_GW_SRV_PDO_READ);

	if (con->size < CO_GW_CON_PDO_READ_SIZE + con->n * sizeof(*con->val)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	int errc = get_errc();

	char *buf;
	int result = asprintf(
			&buf, "[%" PRIu32 "] %u pdo %u", seq, con->net, con->n);
	if (result < 0) {
		errc = get_errc();
		buf = NULL;
		goto error;
	}

	for (co_unsigned8_t i = 0; i < con->n; i++) {
		char *tmp;
		result = asprintf(&tmp, "%s 0x%" PRIx64, buf, con->val[i]);
		if (result < 0) {
			errc = get_errc();
			goto error;
		}
		free(buf);
		buf = tmp;
	}

	result = co_gw_txt_recv_txt(gw, buf);

error:
	free(buf);
	set_errc(errc);
	return result;
}

static int
co_gw_txt_recv_get_version(co_gw_txt_t *gw, co_unsigned32_t seq,
		const struct co_gw_con_get_version *con)
{
	assert(con);
	assert(con->srv == CO_GW_SRV_GET_VERSION);

	return co_gw_txt_recv_fmt(gw,
			"[%" PRIu32 "] %" PRIu32 " %" PRIu32 " %u.%u %" PRIu32
			" %u %u.%u %u.%u",
			seq, con->vendor_id, con->product_code,
			(uint_least16_t)((con->revision >> 16) & 0xffff),
			(uint_least16_t)(con->revision & 0xffff),
			con->serial_nr, con->gw_class, con->prot_hi,
			con->prot_lo, CO_GW_TXT_IMPL_HI, CO_GW_TXT_IMPL_LO);
}

static int
co_gw_txt_recv_lss_get_lssid(co_gw_txt_t *gw, co_unsigned32_t seq,
		const struct co_gw_con_lss_get_lssid *con)
{
	assert(con);
	assert(con->srv == CO_GW_SRV_LSS_GET_LSSID);

	return co_gw_txt_recv_fmt(
			gw, "[%" PRIu32 "] 0x%08" PRIx32, seq, con->id);
}

static int
co_gw_txt_recv_lss_get_id(co_gw_txt_t *gw, co_unsigned32_t seq,
		const struct co_gw_con_lss_get_id *con)
{
	assert(con);
	assert(con->srv == CO_GW_SRV_LSS_GET_ID);

	return co_gw_txt_recv_fmt(gw, "[%" PRIu32 "] %u", seq, con->id);
}

static int
co_gw_txt_recv__lss_scan(co_gw_txt_t *gw, co_unsigned32_t seq,
		const struct co_gw_con__lss_scan *con)
{
	assert(con);

	return co_gw_txt_recv_fmt(gw,
			"[%" PRIu32 "] 0x%08" PRIx32 " 0x%08" PRIx32
			" 0x%08" PRIx32 " 0x%08" PRIx32,
			seq, con->id.vendor_id, con->id.product_code,
			con->id.revision, con->id.serial_nr);
}

static int
co_gw_txt_recv_err(co_gw_txt_t *gw, co_unsigned32_t seq, int iec,
		co_unsigned32_t ac)
{
	if (iec) {
		gw->iec = iec;
		return co_gw_txt_recv_fmt(gw, "[%" PRIu32 "] ERROR: %d (%s)",
				seq, iec, co_gw_iec2str(iec));
	} else if (ac) {
		gw->iec = CO_GW_IEC_INTERN;
		return co_gw_txt_recv_fmt(gw,
				"[%" PRIu32 "] ERROR: %08" PRIX32 " (%s)", seq,
				ac, co_sdo_ac2str(ac));
	} else {
		return co_gw_txt_recv_fmt(gw, "[%" PRIu32 "] OK", seq);
	}
}

static int
co_gw_txt_recv_rpdo(co_gw_txt_t *gw, const struct co_gw_ind_rpdo *ind)
{
	assert(ind);
	assert(ind->srv == CO_GW_SRV_RPDO);

	if (ind->size < CO_GW_IND_RPDO_SIZE + ind->n * sizeof(*ind->val)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	int errc = get_errc();

	char *buf;
	int result = asprintf(&buf, "%u pdo %u %u", ind->net, ind->num, ind->n);
	if (result < 0) {
		errc = get_errc();
		buf = NULL;
		goto error;
	}

	for (co_unsigned8_t i = 0; i < ind->n; i++) {
		char *tmp;
		result = asprintf(&tmp, "%s 0x%" PRIx64, buf, ind->val[i]);
		if (result < 0) {
			errc = get_errc();
			goto error;
		}
		free(buf);
		buf = tmp;
	}

	result = co_gw_txt_recv_txt(gw, buf);

error:
	free(buf);
	set_errc(errc);
	return result;
}

static int
co_gw_txt_recv_ec(co_gw_txt_t *gw, const struct co_gw_ind_ec *ind)
{
	assert(ind);
	assert(ind->srv == CO_GW_SRV_EC);

	if (ind->iec == CO_GW_IEC_BOOTUP)
		return co_gw_txt_recv_fmt(
				gw, "%u %u BOOT_UP", ind->net, ind->node);

	const char *str;
	switch (ind->st) {
	case CO_NMT_ST_STOP: str = "ERRORx STOP"; break;
	case CO_NMT_ST_START: str = "ERRORx OPER"; break;
	case CO_NMT_ST_RESET_NODE: str = "ERRORx RAPP"; break;
	case CO_NMT_ST_RESET_COMM: str = "ERRORx RCOM"; break;
	case CO_NMT_ST_PREOP: str = "ERRORx PREO"; break;
	default: str = "ERROR"; break;
	}
	if (ind->iec)
		return co_gw_txt_recv_fmt(gw, "%u %u %s %d (%s)", ind->net,
				ind->node, str, ind->iec,
				co_gw_iec2str(ind->iec));
	else
		return co_gw_txt_recv_fmt(
				gw, "%u %u %s", ind->net, ind->node, str);
}

static int
co_gw_txt_recv_emcy(co_gw_txt_t *gw, const struct co_gw_ind_emcy *ind)
{
	assert(ind);
	assert(ind->srv == CO_GW_SRV_EMCY);

	return co_gw_txt_recv_fmt(gw, "%u %u EMCY %04X %02X %u %u %u %u %u",
			ind->net, ind->node, ind->ec, ind->er, ind->msef[0],
			ind->msef[1], ind->msef[2], ind->msef[3], ind->msef[4]);
}

static int
co_gw_txt_recv_sdo(co_gw_txt_t *gw, const struct co_gw_ind_sdo *ind)
{
	assert(ind);
	assert(ind->srv == CO_GW_SRV_SDO);

	return co_gw_txt_recv_fmt(gw, "%u %u SDO%c %" PRIu32, ind->net,
			ind->node, ind->up ? 'r' : 'w', ind->nbyte);
}

static int
co_gw_txt_recv__boot(co_gw_txt_t *gw, const struct co_gw_ind__boot *ind)
{
	assert(ind);
	assert(ind->srv == CO_GW_SRV__BOOT);

	if (ind->es) {
		return co_gw_txt_recv_fmt(gw, "%u %u USER BOOT %c (%s)",
				ind->net, ind->node, ind->es,
				co_nmt_es2str(ind->es));
	} else {
		const char *str;
		switch (ind->st) {
		case CO_NMT_ST_STOP: str = "STOP"; break;
		case CO_NMT_ST_START: str = "OPER"; break;
		case CO_NMT_ST_RESET_NODE: str = "RAPP"; break;
		case CO_NMT_ST_RESET_COMM: str = "RCOM"; break;
		case CO_NMT_ST_PREOP: str = "PREOP"; break;
		default: str = "ERROR"; break;
		}
		return co_gw_txt_recv_fmt(gw, "%u %u USER BOOT %s", ind->net,
				ind->node, str);
	}
}

static int
co_gw_txt_recv_fmt(co_gw_txt_t *gw, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
#if __STDC_NO_VLA__
	int result = -1;
	char *buf = NULL;
	int n = vasprintf(&buf, format, ap);
	if (n > 0) {
		result = co_gw_txt_recv_txt(gw, buf);
		free(buf);
	}
#else
	va_list aq;
	va_copy(aq, ap);
	int n = vsnprintf(NULL, 0, format, aq);
	va_end(aq);

	assert(n > 0);
	char buf[n + 1];
	vsprintf(buf, format, ap);
	int result = co_gw_txt_recv_txt(gw, buf);
#endif
	va_end(ap);

	return result;
}

static int
co_gw_txt_recv_txt(co_gw_txt_t *gw, const char *txt)
{
	assert(gw);
	assert(txt);

	if (!gw->recv_func) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	return gw->recv_func(txt, gw->recv_data) ? -1 : 0;
}

static size_t
co_gw_txt_print_val(
		char **pbegin, char *end, co_unsigned16_t type, const void *val)
{
	const union co_val *u = val;
	assert(u);
	switch (type) {
	case CO_DEFTYPE_REAL32: return print_c99_flt(pbegin, end, u->r32);
	case CO_DEFTYPE_VISIBLE_STRING: {
		size_t chars = 0;
		chars += print_char(pbegin, end, '\"');
		const char *vs = u->vs;
		while (vs && *vs) {
			char32_t c32 = 0;
			vs += lex_utf8(vs, NULL, NULL, &c32);
			if (c32 == '\"') {
				chars += print_char(pbegin, end, '\"');
				chars += print_char(pbegin, end, '\"');
			} else {
				chars += print_c99_esc(pbegin, end, c32);
			}
		}
		chars += print_char(pbegin, end, '\"');
		return chars;
	}
	case CO_DEFTYPE_OCTET_STRING:
		return co_val_print(CO_DEFTYPE_DOMAIN, val, pbegin, end);
	case CO_DEFTYPE_REAL64: return print_c99_dbl(pbegin, end, u->r64);
	default: return co_val_print(type, val, pbegin, end);
	}
}

static size_t
co_gw_txt_send_sdo_up(co_gw_txt_t *gw, int srv, void *data, co_unsigned16_t net,
		co_unsigned8_t node, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_SDO_UP);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned16_t idx = 0;
	co_unsigned8_t subidx = 0;
	co_unsigned16_t type = 0;
	if (!(chars = co_gw_txt_lex_sdo(begin, end, at, &idx, &subidx, &type)))
		return 0;
	cp += chars;

	struct co_gw_req_sdo_up req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.node = node,
		.idx = idx,
		.subidx = subidx,
		.type = type };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_sdo_dn(co_gw_txt_t *gw, int srv, void *data, co_unsigned16_t net,
		co_unsigned8_t node, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_SDO_DN);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned16_t idx = 0;
	co_unsigned8_t subidx = 0;
	co_unsigned16_t type = 0;
	if (!(chars = co_gw_txt_lex_sdo(begin, end, at, &idx, &subidx, &type)))
		goto error_lex_sdo;
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	union co_val val;
	co_val_init(type, &val);

	if (!(chars = co_gw_txt_lex_val(cp, end, at, type, &val))) {
		diag_if(DIAG_ERROR, 0, at, "unable to parse value");
		goto error_lex_val;
	}
	cp += chars;

	size_t n = co_val_write(type, &val, NULL, NULL);

	size_t size = MAX(CO_GW_REQ_SDO_DN_SIZE + n,
			sizeof(struct co_gw_req_sdo_dn));
	struct co_gw_req_sdo_dn *req = malloc(size);
	if (!req) {
		diag_if(DIAG_ERROR, get_errc(), at, "unable to create value");
		goto error_malloc_req;
	}

	*req = (struct co_gw_req_sdo_dn){ .size = size,
		.srv = srv,
		.data = data,
		.net = net,
		.node = node,
		.idx = idx,
		.subidx = subidx,
		.len = n };

	uint_least8_t *bp = req->val;
	if (co_val_write(type, &val, bp, bp + n) != n) {
		diag_if(DIAG_ERROR, get_errc(), at, "unable to write value");
		goto error_write_val;
	}

	co_gw_txt_send_req(gw, (struct co_gw_req *)req);

	free(req);
	co_val_fini(type, &val);

	return cp - begin;

error_write_val:
	free(req);
error_malloc_req:
error_lex_val:
	co_val_fini(type, &val);
error_lex_sdo:
	return 0;
}

static size_t
co_gw_txt_send_set_sdo_timeout(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_SET_SDO_TIMEOUT);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	long timeout = 0;
	if (!(chars = lex_c99_long(cp, end, at, &timeout))) {
		diag_if(DIAG_ERROR, 0, at, "expected SDO time-out");
		return 0;
	}
	cp += chars;

	struct co_gw_req_set_sdo_timeout req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.timeout = timeout };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_set_rpdo(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_SET_RPDO);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned16_t num = 0;
	struct co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
	struct co_pdo_map_par map = CO_PDO_MAP_PAR_INIT;
	if (!(chars = co_gw_txt_lex_pdo(cp, end, at, 0, &num, &comm, &map)))
		return 0;
	cp += chars;

	struct co_gw_req_set_rpdo req = { .size = CO_GW_REQ_SET_RPDO_SIZE
				+ map.n * sizeof(*map.map),
		.srv = srv,
		.data = data,
		.net = net,
		.num = num,
		.cobid = comm.cobid,
		.trans = comm.trans,
		.n = map.n };
	for (co_unsigned8_t i = 0; i < map.n; i++)
		req.map[i] = map.map[i];
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_set_tpdo(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_SET_TPDO);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	// Check if this is an extended configure TPDO command.
	int ext = 0;
	if ((chars = lex_char('x', cp, end, at))) {
		cp += chars;
		cp += lex_ctype(&isblank, cp, end, at);
		ext = 1;
	}

	co_unsigned16_t num = 0;
	struct co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
	struct co_pdo_map_par map = CO_PDO_MAP_PAR_INIT;
	if (!(chars = co_gw_txt_lex_pdo(cp, end, at, ext, &num, &comm, &map)))
		return 0;
	cp += chars;

	struct co_gw_req_set_tpdo req = { .size = CO_GW_REQ_SET_TPDO_SIZE
				+ map.n * sizeof(*map.map),
		.srv = srv,
		.data = data,
		.net = net,
		.num = num,
		.cobid = comm.cobid,
		.trans = comm.trans,
		.inhibit = comm.inhibit,
		.event = comm.event,
		.sync = comm.sync,
		.n = map.n };
	for (co_unsigned8_t i = 0; i < map.n; i++)
		req.map[i] = map.map[i];
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_pdo_read(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_PDO_READ);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned16_t num = 0;
	if (!(chars = lex_c99_u16(cp, end, at, &num))) {
		diag_if(DIAG_ERROR, 0, at, "expected PDO number");
		return 0;
	}
	cp += chars;
	if (!num || num > 512) {
		diag_if(DIAG_ERROR, 0, at,
				"PDO number must be in the range [1..512]");
		return 0;
	}

	struct co_gw_req_pdo_read req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.num = num };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_pdo_write(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_PDO_WRITE);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned16_t num = 0;
	if (!(chars = lex_c99_u16(cp, end, at, &num))) {
		diag_if(DIAG_ERROR, 0, at, "expected PDO number");
		return 0;
	}
	cp += chars;

	if (!num || num > 512) {
		diag_if(DIAG_ERROR, 0, at,
				"PDO number must be in the range [1..512]");
		return 0;
	}

	cp += lex_ctype(&isblank, cp, end, at);

	struct co_gw_req_pdo_write req = { .size = CO_GW_REQ_PDO_WRITE_SIZE,
		.srv = srv,
		.data = data,
		.net = net,
		.num = num };

	if (!(chars = lex_c99_u8(cp, end, at, &req.n))) {
		diag_if(DIAG_ERROR, 0, at, "expected number of values");
		return 0;
	}
	cp += chars;
	if (req.n > 0x40) {
		diag_if(DIAG_ERROR, 0, at,
				"number of values must be in the range [0..64]");
		return 0;
	}

	for (co_unsigned8_t i = 0; i < req.n; i++) {
		cp += lex_ctype(&isblank, cp, end, at);

		chars = lex_c99_u64(cp, end, at, &req.val[i]);
		if (!chars) {
			diag_if(DIAG_ERROR, 0, at, "expected value");
			return 0;
		}
		cp += chars;
	}

	req.size += req.n * sizeof(*req.val);
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_nmt_set_ng(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, co_unsigned8_t node, const char *begin,
		const char *end, struct floc *at)
{
	assert(srv == CO_GW_SRV_NMT_NG_ENABLE);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned16_t gt = 0;
	if (!(chars = lex_c99_u16(cp, end, at, &gt))) {
		diag_if(DIAG_ERROR, 0, at, "expected guard time");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	co_unsigned8_t ltf = 0;
	if (!(chars = lex_c99_u8(cp, end, at, &ltf))) {
		diag_if(DIAG_ERROR, 0, at, "expected lifetime factor");
		return 0;
	}
	cp += chars;

	struct co_gw_req_nmt_set_ng req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.node = node,
		.gt = gt,
		.ltf = ltf };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_nmt_set_hb(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, co_unsigned8_t node, const char *begin,
		const char *end, struct floc *at)
{
	assert(srv == CO_GW_SRV_NMT_HB_ENABLE);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned16_t ms = 0;
	if (!(chars = lex_c99_u16(cp, end, at, &ms))) {
		diag_if(DIAG_ERROR, 0, at, "expected heartbeat time");
		return 0;
	}
	cp += chars;

	struct co_gw_req_nmt_set_hb req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.node = node,
		.ms = ms };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_init(co_gw_txt_t *gw, int srv, void *data, co_unsigned16_t net,
		const char *begin, const char *end, struct floc *at)
{
	assert(srv == CO_GW_SRV_INIT);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned8_t bitidx = 0;
	if (!(chars = lex_c99_u8(cp, end, at, &bitidx))) {
		diag_if(DIAG_ERROR, 0, at, "expected bit timing index");
		return 0;
	}
	cp += chars;

	if (bitidx > 9) {
		diag_if(DIAG_ERROR, 0, at,
				"the bit timing must be in the range [0..9]");
		return 0;
	}

	struct co_gw_req_init req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.bitidx = bitidx };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_set_hb(co_gw_txt_t *gw, int srv, void *data, co_unsigned16_t net,
		const char *begin, const char *end, struct floc *at)
{
	assert(srv == CO_GW_SRV_SET_HB);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned16_t ms = 0;
	if (!(chars = lex_c99_u16(cp, end, at, &ms))) {
		diag_if(DIAG_ERROR, 0, at, "expected heartbeat time");
		return 0;
	}
	cp += chars;

	struct co_gw_req_set_hb req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.ms = ms };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_set_id(co_gw_txt_t *gw, int srv, void *data, co_unsigned16_t net,
		const char *begin, const char *end, struct floc *at)
{
	assert(srv == CO_GW_SRV_SET_ID);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned8_t node = 0;
	if (!(chars = lex_c99_u8(cp, end, at, &node))) {
		diag_if(DIAG_ERROR, 0, at, "expected node-ID");
		return 0;
	}
	cp += chars;

	if (!node || (node > CO_NUM_NODES && node != 0xff)) {
		diag_if(DIAG_ERROR, 0, at,
				"the node-ID must be in the range [1..%u, 255]",
				CO_NUM_NODES);
		return 0;
	}

	struct co_gw_req_node req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.node = node };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_set_cmd_timeout(co_gw_txt_t *gw, int srv, void *data,
		const char *begin, const char *end, struct floc *at)
{
	assert(srv == CO_GW_SRV_SET_CMD_TIMEOUT);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	long timeout = 0;
	if (!(chars = lex_c99_long(cp, end, at, &timeout))) {
		diag_if(DIAG_ERROR, 0, at, "expected command time-out");
		return 0;
	}
	cp += chars;

	struct co_gw_req_set_cmd_timeout req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.timeout = timeout };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_set_bootup_ind(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_SET_BOOTUP_IND);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	int cs;

	chars = co_gw_txt_lex_cmd(cp, end, at);
	if (chars && !strncasecmp("Disable", cp, chars)) {
		cp += chars;
		cs = 0;
	} else if (chars && !strncasecmp("Enable", cp, chars)) {
		cp += chars;
		cs = 1;
	} else {
		diag_if(DIAG_ERROR, 0, at,
				"expected 'Disable' or 'Enable' after 'boot_up_indication'");
		return 0;
	}

	struct co_gw_req_set_bootup_ind req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.cs = cs };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_set_net(co_gw_txt_t *gw, int srv, void *data, const char *begin,
		const char *end, struct floc *at)
{
	assert(srv == CO_GW_SRV_SET_NET);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned16_t net = 0;
	if (!(chars = lex_c99_u16(cp, end, at, &net))) {
		diag_if(DIAG_ERROR, 0, at, "expected network-ID");
		return 0;
	}
	cp += chars;

	if (net > CO_GW_NUM_NET) {
		diag_if(DIAG_ERROR, 0, at,
				"the network-ID must be in the range [0, 1..%u]",
				CO_GW_NUM_NET);
		return 0;
	}

	struct co_gw_req_net req = {
		.size = sizeof(req), .srv = srv, .data = data, .net = net
	};
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_set_node(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_SET_NODE);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned8_t node = 0;
	if (!(chars = lex_c99_u8(cp, end, at, &node))) {
		diag_if(DIAG_ERROR, 0, at, "expected node-ID");
		return 0;
	}
	cp += chars;

	if (node > CO_NUM_NODES) {
		diag_if(DIAG_ERROR, 0, at,
				"the node-ID must be in the range [0, 1..%u]",
				CO_NUM_NODES);
		return 0;
	}

	struct co_gw_req_node req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.node = node };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_set_cmd_size(co_gw_txt_t *gw, int srv, void *data,
		const char *begin, const char *end, struct floc *at)
{
	assert(srv == CO_GW_SRV_SET_CMD_SIZE);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned32_t n = 0;
	if (!(chars = lex_c99_u32(cp, end, at, &n))) {
		diag_if(DIAG_ERROR, 0, at, "expected command size");
		return 0;
	}
	cp += chars;

	struct co_gw_req_set_cmd_size req = {
		.size = sizeof(req), .srv = srv, .data = data, .n = n
	};
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_lss_switch(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_LSS_SWITCH);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned8_t mode = 0;
	if (!(chars = lex_c99_u8(cp, end, at, &mode)) || mode > 1) {
		diag_if(DIAG_ERROR, 0, at, "expected 0 or 1");
		return 0;
	}
	cp += chars;

	struct co_gw_req_lss_switch req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.mode = mode };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_lss_switch_sel(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_LSS_SWITCH_SEL);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	struct co_id id = CO_ID_INIT;
	if (!(chars = co_gw_txt_lex_id(cp, end, at, &id)))
		return 0;
	cp += chars;

	struct co_gw_req_lss_switch_sel req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.id = id };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_lss_set_id(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_LSS_SET_ID);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned8_t node = 0;
	if (!(chars = lex_c99_u8(cp, end, at, &node))) {
		diag_if(DIAG_ERROR, 0, at, "expected node-ID");
		return 0;
	}
	cp += chars;

	if (!node || (node > CO_NUM_NODES && node != 0xff)) {
		diag_if(DIAG_ERROR, 0, at,
				"the node-ID must be in the range [1..%u, 255]",
				CO_NUM_NODES);
		return 0;
	}

	struct co_gw_req_node req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.node = node };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_lss_set_rate(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_LSS_SET_RATE);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned8_t bitsel = 0;
	if (!(chars = lex_c99_u8(cp, end, at, &bitsel))) {
		diag_if(DIAG_ERROR, 0, at, "expected table selector");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	co_unsigned8_t bitidx = 0;
	if (!(chars = lex_c99_u8(cp, end, at, &bitidx))) {
		diag_if(DIAG_ERROR, 0, at, "expected table index");
		return 0;
	}
	cp += chars;

	struct co_gw_req_lss_set_rate req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.bitsel = bitsel,
		.bitidx = bitidx };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_lss_switch_rate(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_LSS_SWITCH_RATE);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned16_t delay = 0;
	if (!(chars = lex_c99_u16(cp, end, at, &delay))) {
		diag_if(DIAG_ERROR, 0, at, "expected switch delay");
		return 0;
	}
	cp += chars;

	struct co_gw_req_lss_switch_rate req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.delay = delay };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_lss_get_lssid(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_LSS_GET_LSSID);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned8_t cs = 0;
	if (!(chars = lex_c99_u8(cp, end, at, &cs))) {
		diag_if(DIAG_ERROR, 0, at, "expected code for LSS number");
		return 0;
	}
	cp += chars;
	if (cs < 0x5a || cs > 0x5d) {
		diag_if(DIAG_ERROR, 0, at,
				"code must be in the range [0x5A..0x5D]");
		return 0;
	}

	struct co_gw_req_lss_get_lssid req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.cs = cs };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_lss_id_slave(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_LSS_ID_SLAVE);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	struct co_id lo = CO_ID_INIT;
	struct co_id hi = CO_ID_INIT;
	if (!(chars = co_gw_txt_lex_id_sel(cp, end, at, &lo, &hi)))
		return 0;
	cp += chars;

	struct co_gw_req_lss_id_slave req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.lo = lo,
		.hi = hi };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send__lss_slowscan(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV__LSS_SLOWSCAN);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	struct co_id lo = CO_ID_INIT;
	struct co_id hi = CO_ID_INIT;
	if (!(chars = co_gw_txt_lex_id_sel(cp, end, at, &lo, &hi)))
		return 0;
	cp += chars;

	struct co_gw_req__lss_scan req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.id_1 = lo,
		.id_2 = hi };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send__lss_fastscan(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV__LSS_FASTSCAN);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	struct co_id id = CO_ID_INIT;
	struct co_id mask = CO_ID_INIT;

	if (!(chars = lex_c99_u32(cp, end, at, &id.vendor_id))) {
		diag_if(DIAG_ERROR, 0, at, "expected vendor-ID");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = lex_c99_u32(cp, end, at, &mask.vendor_id))) {
		diag_if(DIAG_ERROR, 0, at, "expected vendor-ID mask");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = lex_c99_u32(cp, end, at, &id.product_code))) {
		diag_if(DIAG_ERROR, 0, at, "expected product code");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = lex_c99_u32(cp, end, at, &mask.product_code))) {
		diag_if(DIAG_ERROR, 0, at, "expected product code mask");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = lex_c99_u32(cp, end, at, &id.revision))) {
		diag_if(DIAG_ERROR, 0, at, "expected revision number");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = lex_c99_u32(cp, end, at, &mask.revision))) {
		diag_if(DIAG_ERROR, 0, at, "expected revision number mask");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = lex_c99_u32(cp, end, at, &id.serial_nr))) {
		diag_if(DIAG_ERROR, 0, at, "expected serial number");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = lex_c99_u32(cp, end, at, &mask.serial_nr))) {
		diag_if(DIAG_ERROR, 0, at, "expected serial number mask");
		return 0;
	}
	cp += chars;

	struct co_gw_req__lss_scan req = { .size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.id_1 = id,
		.id_2 = mask };
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static void
co_gw_txt_send_req(co_gw_txt_t *gw, const struct co_gw_req *req)
{
	assert(gw);
	assert(req);

	if (gw->send_func) {
		gw->pending++;
		if (gw->send_func(req, gw->send_data)) {
			gw->pending--;
			diag(DIAG_ERROR, get_errc(),
					"unable to send gateway request");
		}
	}
}

static size_t
co_gw_txt_lex_prefix(const char *begin, const char *end, struct floc *at,
		co_unsigned32_t *pseq, co_unsigned16_t *pnet,
		co_unsigned8_t *pnode)
{
	assert(begin);
	assert(!end || end >= begin);

	const char *cp = begin;
	size_t chars = 0;

	if (!(chars = lex_char('[', cp, end, at)))
		diag_if(DIAG_WARNING, 0, at,
				"expected '[' before sequence number");
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	co_unsigned32_t seq = 0;
	if (!(chars = lex_c99_u32(cp, end, at, &seq))) {
		diag_if(DIAG_ERROR, 0, at, "expected sequence number");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = lex_char(']', cp, end, at)))
		diag_if(DIAG_WARNING, 0, at,
				"expected ']' after sequence number");
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	co_unsigned16_t net = 0;
	co_unsigned8_t node = 0xff;

	// Try to parse the optional network-ID.
	if ((chars = lex_c99_u16(cp, end, at, &net))) {
		cp += chars;
		cp += lex_ctype(&isblank, cp, end, at);

		// Try to parse the optional node-ID.
		if ((chars = lex_c99_u8(cp, end, at, &node))) {
			cp += chars;
			cp += lex_ctype(&isblank, cp, end, at);

			if (!net || net > CO_GW_NUM_NET) {
				diag_if(DIAG_ERROR, 0, at,
						"the network-ID must be in the range [1..%u]",
						CO_GW_NUM_NET);
				return 0;
			}

			if (node > CO_NUM_NODES) {
				diag_if(DIAG_ERROR, 0, at,
						"the node-ID must be in the range [0..%u]",
						CO_NUM_NODES);
				return 0;
			}
		} else {
			if (net > CO_NUM_NODES) {
				diag_if(DIAG_ERROR, 0, at,
						"the node-ID must be in the range [0..%u]",
						CO_NUM_NODES);
				return 0;
			}
			// If only a single ID was provided, interpret it as the
			// node-ID.
			node = net;
			net = 0;
		}
	}

	if (pseq)
		*pseq = seq;
	if (pnet)
		*pnet = net;
	if (pnode)
		*pnode = node;

	return cp - begin;
}

static size_t
co_gw_txt_lex_srv(
		const char *begin, const char *end, struct floc *at, int *psrv)
{
	assert(begin);
	assert(!end || end >= begin);

	const char *cp = begin;
	size_t chars = 0;

	int srv = 0;

	cp += lex_ctype(&isblank, cp, end, at);

	chars = co_gw_txt_lex_cmd(cp, end, at);
	if (!chars) {
		diag_if(DIAG_ERROR, 0, at, "expected command");
		return 0;
	}

	if (!strncasecmp("_lss_fastscan", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV__LSS_FASTSCAN;
	} else if (!strncasecmp("_lss_slowscan", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV__LSS_SLOWSCAN;
	} else if (!strncasecmp("boot_up_indication", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_SET_BOOTUP_IND;
	} else if (!strncasecmp("disable", cp, chars)) {
		cp += chars;
		cp += lex_ctype(&isblank, cp, end, at);

		chars = co_gw_txt_lex_cmd(cp, end, at);
		if (chars && !strncasecmp("guarding", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_NMT_NG_DISABLE;
		} else if (chars && !strncasecmp("heartbeat", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_NMT_HB_DISABLE;
		} else {
			diag_if(DIAG_ERROR, 0, at,
					"expected 'guarding' or 'heartbeat'");
			return 0;
		}
	} else if (!strncasecmp("enable", cp, chars)) {
		cp += chars;
		cp += lex_ctype(&isblank, cp, end, at);

		chars = co_gw_txt_lex_cmd(cp, end, at);
		if (chars && !strncasecmp("guarding", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_NMT_NG_ENABLE;
		} else if (chars && !strncasecmp("heartbeat", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_NMT_HB_ENABLE;
		} else {
			diag_if(DIAG_ERROR, 0, at,
					"expected 'guarding' or 'heartbeat'");
			return 0;
		}
	} else if (!strncasecmp("info", cp, chars)) {
		cp += chars;
		cp += lex_ctype(&isblank, cp, end, at);

		chars = co_gw_txt_lex_cmd(cp, end, at);
		if (chars && !strncasecmp("version", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_GET_VERSION;
		} else {
			diag_if(DIAG_ERROR, 0, at, "expected 'version'");
			return 0;
		}
	} else if (!strncasecmp("init", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_INIT;
	} else if (!strncasecmp("lss_activate_bitrate", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_LSS_SWITCH_RATE;
	} else if (!strncasecmp("lss_conf_bitrate", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_LSS_SET_RATE;
	} else if (!strncasecmp("lss_get_node", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_LSS_GET_ID;
	} else if (!strncasecmp("lss_identity", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_LSS_ID_SLAVE;
	} else if (!strncasecmp("lss_ident_nonconf", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_LSS_ID_NON_CFG_SLAVE;
	} else if (!strncasecmp("lss_inquire_addr", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_LSS_GET_LSSID;
	} else if (!strncasecmp("lss_set_node", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_LSS_SET_ID;
	} else if (!strncasecmp("lss_store", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_LSS_STORE;
	} else if (!strncasecmp("lss_switch_glob", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_LSS_SWITCH;
	} else if (!strncasecmp("lss_switch_sel", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_LSS_SWITCH_SEL;
	} else if (!strncasecmp("preop", cp, chars)
			|| !strncasecmp("preoperational", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_NMT_ENTER_PREOP;
	} else if (!strncasecmp("r", cp, chars)
			|| !strncasecmp("read", cp, chars)) {
		cp += chars;
		cp += lex_ctype(&isblank, cp, end, at);

		if ((chars = co_gw_txt_lex_cmd(cp, end, at))) {
			if (!strncasecmp("p", cp, chars)
					|| !strncasecmp("pdo", cp, chars)) {
				cp += chars;
				srv = CO_GW_SRV_PDO_READ;
			} else {
				diag_if(DIAG_ERROR, 0, at, "expected 'p[do]'");
				return 0;
			}
		} else {
			srv = CO_GW_SRV_SDO_UP;
		}
	} else if (!strncasecmp("reset", cp, chars)) {
		cp += chars;
		cp += lex_ctype(&isblank, cp, end, at);

		chars = co_gw_txt_lex_cmd(cp, end, at);
		// clang-format off
		if (chars && (!strncasecmp("comm", cp, chars)
				|| !strncasecmp("communication", cp, chars))) {
			// clang-format on
			cp += chars;
			srv = CO_GW_SRV_NMT_RESET_COMM;
		} else if (chars && !strncasecmp("node", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_NMT_RESET_NODE;
		} else {
			diag_if(DIAG_ERROR, 0, at,
					"expected 'node' or 'comm[unication]'");
			return 0;
		}
	} else if (!strncasecmp("set", cp, chars)) {
		cp += chars;
		cp += lex_ctype(&isblank, cp, end, at);

		chars = co_gw_txt_lex_cmd(cp, end, at);
		if (chars && !strncasecmp("command_size", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_SET_CMD_SIZE;
		} else if (chars
				&& !strncasecmp("command_timeout", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_SET_CMD_TIMEOUT;
		} else if (chars && !strncasecmp("heartbeat", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_SET_HB;
		} else if (chars && !strncasecmp("id", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_SET_ID;
		} else if (chars && !strncasecmp("network", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_SET_NET;
		} else if (chars && !strncasecmp("node", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_SET_NODE;
		} else if (chars && !strncasecmp("rpdo", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_SET_RPDO;
		} else if (chars && !strncasecmp("sdo_timeout", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_SET_SDO_TIMEOUT;
		} else if (chars && !strncasecmp("tpdo", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_SET_TPDO;
		} else if (chars && !strncasecmp("tpdox", cp, chars)) {
			// Wait with parsing the 'x' until co_gw_txt_lex_pdo().
			cp += chars - 1;
			if (at)
				at->column--;
			srv = CO_GW_SRV_SET_TPDO;
		} else {
			diag_if(DIAG_ERROR, 0, at,
					"expected 'command_size', 'command_timeout', 'heartbeat', 'id', 'network', 'node', 'rpdo', 'sdo_timeout' or 'tpdo[x]'");
			return 0;
		}
	} else if (!strncasecmp("start", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_NMT_START;
	} else if (!strncasecmp("stop", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_NMT_STOP;
	} else if (!strncasecmp("w", cp, chars)
			|| !strncasecmp("write", cp, chars)) {
		cp += chars;
		cp += lex_ctype(&isblank, cp, end, at);

		if ((chars = co_gw_txt_lex_cmd(cp, end, at))) {
			if (!strncasecmp("p", cp, chars)
					|| !strncasecmp("pdo", cp, chars)) {
				cp += chars;
				srv = CO_GW_SRV_PDO_WRITE;
			} else {
				diag_if(DIAG_ERROR, 0, at, "expected 'p[do]'");
				return 0;
			}
		} else {
			srv = CO_GW_SRV_SDO_DN;
		}
	} else {
		diag_if(DIAG_ERROR, 0, at,
				"expected '_lss_fastscan', '_lss_slowscan', 'boot_up_indication', 'disable', 'enable', 'info', 'init', 'lss_activate_bitrate', 'lss_conf_bitrate', 'lss_get_node', 'lss_identity', 'lss_ident_nonconf', 'lss_inquire_addr', 'lss_set_node', 'lss_store', 'lss_switch_glob', 'lss_switch_sel', 'preop[erational]', 'r[ead]', 'reset', 'set', 'start', 'stop', or 'w[rite]'");
		return 0;
	}

	if (psrv)
		*psrv = srv;

	return cp - begin;
}

static size_t
co_gw_txt_lex_cmd(const char *begin, const char *end, struct floc *at)
{
	assert(begin);
	assert(!end || end >= begin);

	const char *cp = begin;

	if ((end && cp >= end) || !(*cp == '_' || isalpha((unsigned char)*cp)))
		return 0;
	cp++;

	while ((!end || cp < end)
			&& (*cp == '_' || isalnum((unsigned char)*cp)))
		cp++;

	return floc_lex(at, begin, cp);
}

static size_t
co_gw_txt_lex_sdo(const char *begin, const char *end, struct floc *at,
		co_unsigned16_t *pidx, co_unsigned8_t *psubidx,
		co_unsigned16_t *ptype)
{
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	if (!(chars = lex_c99_u16(cp, end, at, pidx))) {
		diag_if(DIAG_ERROR, 0, at, "expected object index");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = lex_c99_u8(cp, end, at, psubidx))) {
		diag_if(DIAG_ERROR, 0, at, "expected object sub-index");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = co_gw_txt_lex_type(cp, end, at, ptype)))
		return 0;
	cp += chars;

	return cp - begin;
}

static size_t
co_gw_txt_lex_pdo(const char *begin, const char *end, struct floc *at, int ext,
		co_unsigned16_t *pnum, struct co_pdo_comm_par *pcomm,
		struct co_pdo_map_par *pmap)
{
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned16_t num = 0;
	if (!(chars = lex_c99_u16(cp, end, at, &num))) {
		diag_if(DIAG_ERROR, 0, at, "expected PDO number");
		return 0;
	}
	cp += chars;

	if (!num || num > 512) {
		diag_if(DIAG_ERROR, 0, at,
				"PDO number must be in the range [1..512]");
		return 0;
	}

	cp += lex_ctype(&isblank, cp, end, at);

	struct co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;

	if (!(chars = lex_c99_u32(cp, end, at, &comm.cobid))) {
		diag_if(DIAG_ERROR, 0, at, "expected COB-ID");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = co_gw_txt_lex_trans(cp, end, at, &comm.trans)))
		return 0;
	cp += chars;
	if (ext && comm.trans >= 0xfe) {
		if (!(chars = lex_c99_u16(cp, end, at, &comm.inhibit))) {
			diag_if(DIAG_ERROR, 0, at, "expected inhibit time");
			return 0;
		}
		cp += chars;
	}
	cp += lex_ctype(&isblank, cp, end, at);

	if (ext && comm.trans >= 0xfd) {
		if (!(chars = lex_c99_u16(cp, end, at, &comm.event))) {
			diag_if(DIAG_ERROR, 0, at, "expected event timer");
			return 0;
		}
		cp += chars;
		cp += lex_ctype(&isblank, cp, end, at);
	} else if (ext && comm.trans <= 240) {
		if (!(chars = lex_c99_u8(cp, end, at, &comm.sync))) {
			diag_if(DIAG_ERROR, 0, at, "expected SYNC start value");
			return 0;
		}
		cp += chars;
		cp += lex_ctype(&isblank, cp, end, at);
	}

	struct co_pdo_map_par map = CO_PDO_MAP_PAR_INIT;

	if (!(chars = lex_c99_u8(cp, end, at, &map.n))) {
		diag_if(DIAG_ERROR, 0, at, "expected number of values");
		return 0;
	}
	cp += chars;
	if (map.n > 0x40) {
		diag_if(DIAG_ERROR, 0, at,
				"number of mapped values must be in the range [0..64]");
		return 0;
	}

	for (co_unsigned8_t i = 0; i < map.n; i++) {
		cp += lex_ctype(&isblank, cp, end, at);

		co_unsigned16_t idx = 0;
		co_unsigned8_t subidx = 0;
		chars = lex_c99_u16(cp, end, at, &idx);
		if (chars) {
			cp += chars;
			cp += lex_ctype(&isblank, cp, end, at);

			chars = lex_c99_u8(cp, end, at, &subidx);
			if (!chars) {
				diag_if(DIAG_ERROR, 0, at,
						"expected object sub-index");
				return 0;
			}
			cp += chars;
			cp += lex_ctype(&isblank, cp, end, at);
		}

		co_unsigned16_t type = 0;
		chars = co_gw_txt_lex_type(cp, end, at, &type);
		if (!chars)
			return 0;
		cp += chars;
		if (!co_type_is_basic(type)) {
			diag_if(DIAG_ERROR, 0, at,
					"only basic types can be mapped to PDOs");
			return 0;
		}

		co_unsigned8_t len = co_type_sizeof(type) * CHAR_BIT;
		// If no multiplexer was specified, use the type as the object
		// index (dummy mapping).
		map.map[i] = ((co_unsigned32_t)(idx ? idx : type) << 16)
				| ((co_unsigned32_t)subidx << 8) | len;
	}

	if (pnum)
		*pnum = num;

	if (pcomm)
		*pcomm = comm;

	if (pmap) {
		pmap->n = map.n;
		for (co_unsigned8_t i = 0; i < map.n; i++)
			pmap->map[i] = map.map[i];
	}

	return cp - begin;
}

static size_t
co_gw_txt_lex_type(const char *begin, const char *end, struct floc *at,
		co_unsigned16_t *ptype)
{
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned16_t type = 0;

	chars = co_gw_txt_lex_cmd(cp, end, at);
	if (chars && !strncasecmp("b", cp, chars)) {
		type = CO_DEFTYPE_BOOLEAN;
	} else if (chars && !strncasecmp("d", cp, chars)) {
		type = CO_DEFTYPE_DOMAIN;
	} else if (chars && !strncasecmp("i16", cp, chars)) {
		type = CO_DEFTYPE_INTEGER16;
	} else if (chars && !strncasecmp("i24", cp, chars)) {
		type = CO_DEFTYPE_INTEGER24;
	} else if (chars && !strncasecmp("i32", cp, chars)) {
		type = CO_DEFTYPE_INTEGER32;
	} else if (chars && !strncasecmp("i40", cp, chars)) {
		type = CO_DEFTYPE_INTEGER40;
	} else if (chars && !strncasecmp("i48", cp, chars)) {
		type = CO_DEFTYPE_INTEGER48;
	} else if (chars && !strncasecmp("i56", cp, chars)) {
		type = CO_DEFTYPE_INTEGER56;
	} else if (chars && !strncasecmp("i64", cp, chars)) {
		type = CO_DEFTYPE_INTEGER64;
	} else if (chars && !strncasecmp("i8", cp, chars)) {
		type = CO_DEFTYPE_INTEGER8;
	} else if (chars && !strncasecmp("os", cp, chars)) {
		type = CO_DEFTYPE_OCTET_STRING;
	} else if (chars && !strncasecmp("r32", cp, chars)) {
		type = CO_DEFTYPE_REAL32;
	} else if (chars && !strncasecmp("r64", cp, chars)) {
		type = CO_DEFTYPE_REAL64;
	} else if (chars && !strncasecmp("t", cp, chars)) {
		type = CO_DEFTYPE_TIME_OF_DAY;
	} else if (chars && !strncasecmp("td", cp, chars)) {
		type = CO_DEFTYPE_TIME_DIFF;
	} else if (chars && !strncasecmp("u16", cp, chars)) {
		type = CO_DEFTYPE_UNSIGNED16;
	} else if (chars && !strncasecmp("u24", cp, chars)) {
		type = CO_DEFTYPE_UNSIGNED24;
	} else if (chars && !strncasecmp("u32", cp, chars)) {
		type = CO_DEFTYPE_UNSIGNED32;
	} else if (chars && !strncasecmp("u40", cp, chars)) {
		type = CO_DEFTYPE_UNSIGNED40;
	} else if (chars && !strncasecmp("u48", cp, chars)) {
		type = CO_DEFTYPE_UNSIGNED48;
	} else if (chars && !strncasecmp("u56", cp, chars)) {
		type = CO_DEFTYPE_UNSIGNED56;
	} else if (chars && !strncasecmp("u64", cp, chars)) {
		type = CO_DEFTYPE_UNSIGNED64;
	} else if (chars && !strncasecmp("u8", cp, chars)) {
		type = CO_DEFTYPE_UNSIGNED8;
	} else if (chars && !strncasecmp("us", cp, chars)) {
		type = CO_DEFTYPE_UNICODE_STRING;
	} else if (chars && !strncasecmp("vs", cp, chars)) {
		type = CO_DEFTYPE_VISIBLE_STRING;
	} else {
		diag_if(DIAG_ERROR, 0, at, "expected data type");
		return 0;
	}
	cp += chars;

	if (ptype)
		*ptype = type;

	return cp - begin;
}

static size_t
co_gw_txt_lex_val(const char *begin, const char *end, struct floc *at,
		co_unsigned16_t type, void *val)
{
	assert(begin);
	assert(!end || end >= begin);

	switch (type) {
	case CO_DEFTYPE_REAL32: return lex_c99_flt(begin, end, at, val);
	case CO_DEFTYPE_VISIBLE_STRING: {
		const char *cp = begin;
		size_t chars = 0;

		if (!(chars = lex_char('\"', cp, end, at)))
			diag_if(DIAG_WARNING, 0, at,
					"expected '\"' before string");
		cp += chars;

		size_t n = 0;
		chars = co_gw_txt_lex_vs(cp, end, at, NULL, &n);
		if (val) {
			if (co_val_init_vs_n(val, 0, n) == -1) {
				diag_if(DIAG_ERROR, get_errc(), at,
						"unable to create value of type VISIBLE_STRING");
				return 0;
			}
			// Parse the characters.
			char *vs = *(void **)val;
			assert(vs);
			co_gw_txt_lex_vs(cp, end, NULL, vs, &n);
		}
		cp += chars;

		if (!(chars = lex_char('\"', cp, end, at)))
			diag_if(DIAG_WARNING, 0, at,
					"expected '\"' after string");
		cp += chars;

		return cp - begin;
	}
	case CO_DEFTYPE_OCTET_STRING:
		return co_val_lex(CO_DEFTYPE_DOMAIN, val, begin, end, at);
	case CO_DEFTYPE_REAL64: return lex_c99_dbl(begin, end, at, val);
	default: return co_val_lex(type, val, begin, end, at);
	}
}

static size_t
co_gw_txt_lex_vs(const char *begin, const char *end, struct floc *at, char *s,
		size_t *pn)
{
	assert(begin);

	struct floc *floc = NULL;
	struct floc floc_;
	if (at) {
		floc = &floc_;
		*floc = *at;
	}

	const char *cp = begin;

	size_t n = 0;
	// cppcheck-suppress nullPointerArithmetic
	char *ends = s + (s && pn ? *pn : 0);

	while ((!end || cp < end) && *cp && !isbreak((unsigned char)*cp)) {
		char32_t c32 = 0;
		if (*cp == '\"') {
			if ((end && cp + 1 >= end) || cp[1] != '\"')
				break;
			c32 = *cp;
			cp += floc_lex(floc, cp, cp + 2);
		} else if (*cp == '\\') {
			cp += lex_c99_esc(cp, end, floc, &c32);
		} else {
			cp += lex_utf8(cp, end, floc, &c32);
		}
		if (s || pn)
			n += print_utf8(&s, ends, c32);
	}

	if (pn)
		*pn = n;

	if (at)
		*at = *floc;

	return cp - begin;
}

static size_t
co_gw_txt_lex_trans(const char *begin, const char *end, struct floc *at,
		co_unsigned8_t *ptrans)
{
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	chars = co_gw_txt_lex_cmd(cp, end, at);
	if (!chars) {
		diag_if(DIAG_ERROR, 0, at, "expected transmission type");
		return 0;
	}

	co_unsigned8_t trans = 0;

	if (!strncasecmp("event", cp, 5)) {
		cp += 5;
		if (at)
			at->column -= chars - 5;
		trans = 0xff;
	} else if (!strncasecmp("rtr", cp, chars)) {
		cp += chars;
		trans = 0xfd;
	} else if (!strncasecmp("sync", cp, 4)) {
		cp += 4;
		if (at)
			at->column -= chars - 4;
		if (!(chars = lex_c99_u8(cp, end, at, &trans))) {
			diag_if(DIAG_ERROR, 0, at, "expected SYNC period");
			return 0;
		}
		cp += chars;
		if (trans > 240) {
			diag_if(DIAG_ERROR, 0, at,
					"SYNC period must be in the range [0..240]");
			return 0;
		}
	} else {
		diag_if(DIAG_ERROR, 0, at,
				"expected 'event', 'rtr' or 'sync<0..240>'");
		return 0;
	}

	if (ptrans)
		*ptrans = trans;

	return cp - begin;
}

static size_t
co_gw_txt_lex_id(const char *begin, const char *end, struct floc *at,
		struct co_id *pid)
{
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	struct co_id id = CO_ID_INIT;

	if (!(chars = lex_c99_u32(cp, end, at, &id.vendor_id))) {
		diag_if(DIAG_ERROR, 0, at, "expected vendor-ID");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = lex_c99_u32(cp, end, at, &id.product_code))) {
		diag_if(DIAG_ERROR, 0, at, "expected product code");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = lex_c99_u32(cp, end, at, &id.revision))) {
		diag_if(DIAG_ERROR, 0, at, "expected revision number");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = lex_c99_u32(cp, end, at, &id.serial_nr))) {
		diag_if(DIAG_ERROR, 0, at, "expected serial number");
		return 0;
	}
	cp += chars;

	if (pid)
		*pid = id;

	return cp - begin;
}

static size_t
co_gw_txt_lex_id_sel(const char *begin, const char *end, struct floc *at,
		struct co_id *plo, struct co_id *phi)
{
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	struct co_id lo = CO_ID_INIT;

	if (!(chars = lex_c99_u32(cp, end, at, &lo.vendor_id))) {
		diag_if(DIAG_ERROR, 0, at, "expected vendor-ID");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = lex_c99_u32(cp, end, at, &lo.product_code))) {
		diag_if(DIAG_ERROR, 0, at, "expected product code");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	struct co_id hi = lo;

	if (!(chars = lex_c99_u32(cp, end, at, &lo.revision))) {
		diag_if(DIAG_ERROR, 0, at,
				"expected lower bound for revision number");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = lex_c99_u32(cp, end, at, &hi.revision))) {
		diag_if(DIAG_ERROR, 0, at,
				"expected upper bound for revision number");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = lex_c99_u32(cp, end, at, &lo.serial_nr))) {
		diag_if(DIAG_ERROR, 0, at,
				"expected lower bound for serial number");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (!(chars = lex_c99_u32(cp, end, at, &hi.serial_nr))) {
		diag_if(DIAG_ERROR, 0, at,
				"expected upper bound for serial number");
		return 0;
	}
	cp += chars;

	if (plo)
		*plo = lo;

	if (phi)
		*phi = hi;

	return cp - begin;
}

#endif // !LELY_NO_CO_GW_TXT
