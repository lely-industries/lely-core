/*!\file
 * This file is part of the CANopen library; it contains the implementation of
 * the ASCII gateway functions.
 *
 * \see lely/co/gw_txt.h
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

#ifndef LELY_NO_CO_GW_TXT

#include <lely/libc/stdio.h>
#include <lely/util/diag.h>
#include <lely/util/lex.h>
#include <lely/co/dev.h>
#include <lely/co/gw_txt.h>
#include <lely/co/nmt.h>
#include <lely/co/sdo.h>

#include <assert.h>
#include <stdlib.h>

//! A CANopen ASCII gateway.
struct __co_gw_txt {
	//! The number of pending requests.
	size_t pending;
	//! A pointer to the callback function invoked by co_gw_txt_recv().
	co_gw_txt_recv_func_t *recv_func;
	//! A pointer to the user-specified data for #recv_func.
	void *recv_data;
	//! A pointer to the callback function invoked by co_gw_txt_send().
	co_gw_txt_send_func_t *send_func;
	//! A pointer to the user-specified data for #send_func.
	void *send_data;
};

//! Processes a confirmation.
static int co_gw_txt_recv_con(co_gw_txt_t *gw, co_unsigned32_t seq,
		const struct co_gw_con *con);

//! Processes a 'Get version' confirmation.
static int co_gw_txt_recv_get_version(co_gw_txt_t *gw, co_unsigned32_t seq,
		const struct co_gw_con *con);

/*!
 * Processes a confirmation with a non-zero internal error code or SDO abort
 * code.
 */
static int co_gw_txt_recv_err(co_gw_txt_t *gw, co_unsigned32_t seq, int iec,
		co_unsigned32_t ac);

//! Processes an 'Error control event received' indication.
static int co_gw_txt_recv_ec(co_gw_txt_t *gw, const struct co_gw_ind_ec *ind);

//! Processes an 'Emergency event received' indication.
static int co_gw_txt_recv_emcy(co_gw_txt_t *gw,
		const struct co_gw_ind_emcy *ind);

/*!
 * Formats a received indication or confirmation and invokes the callback
 * function to process it.
 *
 * \see co_gw_txt_recv_txt()
 */
static int co_gw_txt_recv_fmt(co_gw_txt_t *gw, const char *format, ...)
		__format_printf(2, 3);

/*!
 * Invokes the callback function to process a received indication or
 * confirmation.
 */
static int co_gw_txt_recv_txt(co_gw_txt_t *gw, const char *txt);

//! Invokes the callback function to send a request.
static void co_gw_txt_send_req(co_gw_txt_t *gw, const struct co_gw_req *req);

//! Sends an 'Enable node guarding' request after parsing its parameters.
static size_t co_gw_txt_send_nmt_set_ng(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, co_unsigned8_t node, const char *begin,
		const char *end, struct floc *at);
//! Sends a 'Start heartbeat consumer' request after parsing its parameters.
static size_t co_gw_txt_send_nmt_set_hb(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, co_unsigned8_t node, const char *begin,
		const char *end, struct floc *at);

//! Sends an 'Initialize gateway' request after parsing its parameters.
static size_t co_gw_txt_send_init(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);
//! Sends a 'Set heartbeat producer' request after parsing its parameters.
static size_t co_gw_txt_send_set_hb(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);
//! Sends a 'Set node-ID' request after parsing its parameters.
static size_t co_gw_txt_send_set_id(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);
//! Sends a 'Set command time-out' request after parsin gits parameters.
static size_t co_gw_txt_send_set_cmd_timeout(co_gw_txt_t *gw, int srv,
		void *data, const char *begin, const char *end,
		struct floc *at);
//! Sends a 'Boot-up forwarding' request after parsing its parameters.
static size_t co_gw_txt_send_set_bootup_ind(co_gw_txt_t *gw, int srv,
		void *data, co_unsigned16_t net, const char *begin,
		const char *end, struct floc *at);

//! Sends a 'Set default network' request after parsing its parameters.
static size_t co_gw_txt_send_set_net(co_gw_txt_t *gw, int srv, void *data,
		const char *begin, const char *end, struct floc *at);
//! Sends a 'Set default node-ID' request after parsing its parameters.
static size_t co_gw_txt_send_set_node(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at);
//! Sends a 'Set command size' request after parsing its parameters.
static size_t co_gw_txt_send_set_cmd_size(co_gw_txt_t *gw, int srv, void *data,
		const char *begin, const char *end, struct floc *at);

/*!
 * Lexes the prefix (sequence number and optional network and node-ID) of a
 * request.
 */
static size_t co_gw_txt_lex_prefix(const char *begin, const char *end,
		struct floc *at, co_unsigned32_t *pseq, co_unsigned16_t *pnet,
		co_unsigned8_t *pnode);

//! Lexes the service number of a request.
static size_t co_gw_txt_lex_srv(const char *begin, const char *end,
		struct floc *at, int *psrv);

//! Lexes a single command.
static size_t co_gw_txt_lex_cmd(const char *begin, const char *end,
		struct floc *at);

LELY_CO_EXPORT void *
__co_gw_txt_alloc(void)
{
	void *ptr = malloc(sizeof(struct __co_gw_txt));
	if (__unlikely(!ptr))
		set_errno(errno);
	return ptr;
}

LELY_CO_EXPORT void
__co_gw_txt_free(void *ptr)
{
	free(ptr);
}

LELY_CO_EXPORT struct __co_gw_txt *
__co_gw_txt_init(struct __co_gw_txt *gw)
{
	assert(gw);

	gw->pending = 0;

	gw->recv_func = NULL;
	gw->recv_data = NULL;

	gw->send_func = NULL;
	gw->send_data = NULL;

	return gw;
}

LELY_CO_EXPORT void
__co_gw_txt_fini(struct __co_gw_txt *gw)
{
	assert(gw);
}

LELY_CO_EXPORT co_gw_txt_t *
co_gw_txt_create(void)
{
	errc_t errc = 0;

	co_gw_txt_t *gw = __co_gw_txt_alloc();
	if (__unlikely(!gw)) {
		errc = get_errc();
		goto error_alloc_gw;
	}

	if (__unlikely(!__co_gw_txt_init(gw))) {
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

LELY_CO_EXPORT void
co_gw_txt_destroy(co_gw_txt_t *gw)
{
	if (gw) {
		__co_gw_txt_fini(gw);
		__co_gw_txt_free(gw);
	}
}

LELY_CO_EXPORT size_t
co_gw_txt_pending(const co_gw_txt_t *gw)
{
	assert(gw);

	return gw->pending;
}

LELY_CO_EXPORT int
co_gw_txt_recv(co_gw_txt_t *gw, const struct co_gw_srv *srv)
{
	assert(srv);

	if (__unlikely(srv->size < sizeof(*srv))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	switch (srv->srv) {
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
	case CO_GW_SRV_SET_CMD_SIZE: {
		if (__unlikely(srv->size < sizeof(struct co_gw_con))) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		const struct co_gw_con *con = (const struct co_gw_con *)srv;
		co_unsigned32_t seq = (uintptr_t)con->data;
		return co_gw_txt_recv_con(gw, seq, con);
	}
	case CO_GW_SRV_EC: {
		if (__unlikely(srv->size < sizeof(struct co_gw_ind_ec))) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		const struct co_gw_ind_ec *ind =
				(const struct co_gw_ind_ec *)srv;
		return co_gw_txt_recv_ec(gw, ind);
	}
	case CO_GW_SRV_EMCY: {
		if (__unlikely(srv->size < sizeof(struct co_gw_ind_emcy))) {
			set_errnum(ERRNUM_INVAL);
			return -1;
		}
		const struct co_gw_ind_emcy *ind =
				(const struct co_gw_ind_emcy *)srv;
		return co_gw_txt_recv_emcy(gw, ind);
	}
	default:
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
}

LELY_CO_EXPORT void
co_gw_txt_get_recv_func(const co_gw_txt_t *gw, co_gw_txt_recv_func_t **pfunc,
		void **pdata)
{
	assert(gw);

	if (pfunc)
		*pfunc = gw->recv_func;
	if (pdata)
		*pdata = gw->recv_data;
}

LELY_CO_EXPORT size_t
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

	const char *cp = begin;
	size_t chars = 0;

	// Skip leading whitespace.
	cp += lex_ctype(&isspace, cp, end, floc);

	co_unsigned32_t seq = 0;
	co_unsigned16_t net = 0;
	co_unsigned8_t node = 0xff;
	if (__unlikely(!(chars = co_gw_txt_lex_prefix(cp, end, floc, &seq, &net,
			&node))))
		goto error;
	cp += chars;
	void *data = (void *)(uintptr_t)seq;

	int srv = 0;
	if (__unlikely(!(chars = co_gw_txt_lex_srv(cp, end, floc, &srv))))
		goto error;
	cp += chars;

	switch (srv) {
	case CO_GW_SRV_SET_CMD_TIMEOUT:
	case CO_GW_SRV_SET_NET:
	case CO_GW_SRV_GET_VERSION:
	case CO_GW_SRV_SET_CMD_SIZE:
		if (__unlikely(node != 0xff)) {
			diag_if(DIAG_ERROR, 0, floc,
					"node-ID specified before global command");
			goto error;
		}
		if (__unlikely(net)) {
			diag_if(DIAG_ERROR, 0, floc,
					"network-ID specified before global command");
			goto error;
		}
		break;
	case CO_GW_SRV_INIT:
	case CO_GW_SRV_SET_HB:
	case CO_GW_SRV_SET_ID:
	case CO_GW_SRV_SET_BOOTUP_IND:
	case CO_GW_SRV_SET_NODE:
		// A single number preceding the command is normally interpreted
		// as the node-ID. However, in this case we take it to be the
		// network-ID.
		if (__unlikely(net)) {
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
		struct co_gw_req_node req = {
			.size = sizeof(req),
			.srv = srv,
			.data = data,
			.net = net,
			.node = node
		};
		co_gw_txt_send_req(gw, (struct co_gw_req *)&req);
		goto done;
	}
	case CO_GW_SRV_NMT_NG_ENABLE:
		chars = co_gw_txt_send_nmt_set_ng(gw, srv, data, net, node, cp,
				end, floc);
		break;
	case CO_GW_SRV_NMT_HB_ENABLE:
		chars = co_gw_txt_send_nmt_set_hb(gw, srv, data, net, node, cp,
				end, floc);
		break;
	case CO_GW_SRV_INIT:
		chars = co_gw_txt_send_init(gw, srv, data, net, cp, end, floc);
		break;
	case CO_GW_SRV_SET_HB:
		chars = co_gw_txt_send_set_hb(gw, srv, data, net, cp, end,
				floc);
		break;
	case CO_GW_SRV_SET_ID:
		chars = co_gw_txt_send_set_id(gw, srv, data, net, cp, end,
				floc);
		break;
	case CO_GW_SRV_SET_CMD_TIMEOUT:
		chars = co_gw_txt_send_set_cmd_timeout(gw, srv, data, cp, end,
				floc);
		break;
	case CO_GW_SRV_SET_BOOTUP_IND:
		chars = co_gw_txt_send_set_bootup_ind(gw, srv, data, net, cp,
				end, floc);
		break;
	case CO_GW_SRV_SET_NET:
		chars = co_gw_txt_send_set_net(gw, srv, data, cp, end, floc);
		break;
	case CO_GW_SRV_SET_NODE:
		chars = co_gw_txt_send_set_node(gw, srv, data, net, cp, end,
				floc);
		break;
	case CO_GW_SRV_GET_VERSION: {
		// 'info version' is a network-level command without any
		// additional parameters.
		struct co_gw_req_net req = {
			.size = sizeof(req),
			.srv = srv,
			.data = data,
			.net = net
		};
		co_gw_txt_send_req(gw, (struct co_gw_req *)&req);
		goto done;
	}
	case CO_GW_SRV_SET_CMD_SIZE:
		chars = co_gw_txt_send_set_cmd_size(gw, srv, data, cp, end,
				floc);
		break;
	}
	if (__unlikely(!chars))
		goto error;
	cp += chars;

	// Skip trailing whitespace.
	cp += lex_ctype(&isspace, cp, end, floc);

done:
	if ((!end || cp < end) && *cp && !isbreak((unsigned char)*cp))
		diag_if(DIAG_ERROR, 0, floc,
				"expected line break after request");

error:
	// Skip all characters until (and including) the next line break.
	chars = 0;
	while ((!end || cp + chars < end) && cp[chars] && cp[chars++] != '\n');
	cp += chars;

	return floc_lex(at, begin, cp);
}

LELY_CO_EXPORT void
co_gw_txt_set_recv_func(co_gw_txt_t *gw, co_gw_txt_recv_func_t *func,
		void *data)
{
	assert(gw);

	gw->recv_func = func;
	gw->recv_data = data;
}

LELY_CO_EXPORT void
co_gw_txt_get_send_func(const co_gw_txt_t *gw, co_gw_txt_send_func_t **pfunc,
		void **pdata)
{
	assert(gw);

	if (pfunc)
		*pfunc = gw->send_func;
	if (pdata)
		*pdata = gw->send_data;
}

LELY_CO_EXPORT void
co_gw_txt_set_send_func(co_gw_txt_t *gw, co_gw_txt_send_func_t *func,
		void *data)
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

	if (__likely(gw->pending))
		gw->pending--;

	if (con->iec || con->ac)
		return co_gw_txt_recv_err(gw, seq, con->iec, con->ac);

	switch (con->srv) {
	case CO_GW_SRV_GET_VERSION:
		return co_gw_txt_recv_get_version(gw, seq, con);
	default:
		return co_gw_txt_recv_err(gw, seq, 0, 0);
	}
}

static int
co_gw_txt_recv_get_version(co_gw_txt_t *gw, co_unsigned32_t seq,
		const struct co_gw_con *con)
{
	assert(con);
	assert(con->srv == CO_GW_SRV_GET_VERSION);

	if (__unlikely(con->size < sizeof(struct co_gw_con_get_version))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	const struct co_gw_con_get_version *par =
			(const struct co_gw_con_get_version *)con;

	return co_gw_txt_recv_fmt(gw, "[%u] %u %u %u.%u %u %u %u.%u %u.%u",
			seq, par->vendor_id, par->product_code,
			(par->revision >> 16) & 0xffff, par->revision & 0xffff,
			par->serial_nr, par->gw_class, par->prot_hi,
			par->prot_lo, CO_GW_TXT_IMPL_HI, CO_GW_TXT_IMPL_LO);
}

static int
co_gw_txt_recv_err(co_gw_txt_t *gw, co_unsigned32_t seq, int iec,
		co_unsigned32_t ac)
{
	if (iec)
		return co_gw_txt_recv_fmt(gw, "[%u] ERROR: %d (%s)", seq, iec,
				co_gw_iec2str(iec));
	else if (ac)
		return co_gw_txt_recv_fmt(gw, "[%u] ERROR: %08X (%s)", seq, ac,
				co_sdo_ac2str(ac));
	else
		return co_gw_txt_recv_fmt(gw, "[%u] OK", seq);
}

static int
co_gw_txt_recv_ec(co_gw_txt_t *gw, const struct co_gw_ind_ec *ind)
{
	assert(ind);
	assert(ind->srv == CO_GW_SRV_EC);

	if (ind->iec == CO_GW_IEC_BOOTUP)
		return co_gw_txt_recv_fmt(gw, "%u %u BOOT_UP", ind->net,
				ind->node);

	const char *str;
	switch (ind->st) {
	case CO_NMT_ST_STOP: str = "ERRORx STOP"; break;
	case CO_NMT_ST_START: str = "ERRORx OPER"; break;
	case CO_NMT_ST_RESET_NODE: str = "ERRORx RAPP"; break;
	case CO_NMT_ST_RESET_COMM: str = "ERRORx RCOM"; break;
	case CO_NMT_ST_PREOP: str = "ERRORx PREOP"; break;
	default: str = "ERROR"; break;
	}
	if (ind->iec)
		return co_gw_txt_recv_fmt(gw, "%u %u %s %d (%s)", ind->net,
				ind->node, str, ind->iec,
				co_gw_iec2str(ind->iec));
	else
		return co_gw_txt_recv_fmt(gw, "%u %u %s", ind->net,
				ind->node, str);
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
co_gw_txt_recv_fmt(co_gw_txt_t *gw, const char *format, ...)
{
	int result = -1;

	va_list ap;
	va_start(ap, format);
#if __STDC_NO_VLA__
	char *buf = NULL;
	int n = vasprintf(&buf, format, ap);
	if (__likely(n > 0)) {
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
	result = co_gw_txt_recv_txt(gw, buf);
#endif
	va_end(ap);

	return result;
}

static int
co_gw_txt_recv_txt(co_gw_txt_t *gw, const char *txt)
{
	assert(gw);
	assert(txt);

	if (__unlikely(!gw->recv_func)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	return gw->recv_func(txt, gw->recv_data) ? -1 : 0;
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
	if (__unlikely(!(chars = lex_c99_u16(cp, end, at, &gt)))) {
		diag_if(DIAG_ERROR, 0, at, "expected guard time");
		return 0;
	}
	cp += chars;

	cp += lex_ctype(&isblank, cp, end, at);

	co_unsigned8_t ltf = 0;
	if (__unlikely(!(chars = lex_c99_u8(cp, end, at, &ltf)))) {
		diag_if(DIAG_ERROR, 0, at, "expected lifetime factor");
		return 0;
	}
	cp += chars;

	struct co_gw_req_nmt_set_ng req = {
		.size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.node = node,
		.gt = gt,
		.ltf = ltf
	};
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
	if (__unlikely(!(chars = lex_c99_u16(cp, end, at, &ms)))) {
		diag_if(DIAG_ERROR, 0, at, "expected heartbeat time");
		return 0;
	}
	cp += chars;

	struct co_gw_req_nmt_set_hb req = {
		.size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.node = node,
		.ms = ms
	};
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
	if (__unlikely(!(chars = lex_c99_u8(cp, end, at, &bitidx)))) {
		diag_if(DIAG_ERROR, 0, at, "expected bit timing index");
		return 0;
	}
	cp += chars;

	if (__unlikely(bitidx > 9)) {
		diag_if(DIAG_ERROR, 0, at,
				"the bit timing must be in the range [0..9]");
		return 0;
	}

	struct co_gw_req_init req = {
		.size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.bitidx = bitidx
	};
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_set_hb(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_SET_HB);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned16_t ms = 0;
	if (__unlikely(!(chars = lex_c99_u16(cp, end, at, &ms)))) {
		diag_if(DIAG_ERROR, 0, at, "expected heartbeat time");
		return 0;
	}
	cp += chars;

	struct co_gw_req_set_hb req = {
		.size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.ms = ms
	};
	co_gw_txt_send_req(gw, (struct co_gw_req *)&req);

	return cp - begin;
}

static size_t
co_gw_txt_send_set_id(co_gw_txt_t *gw, int srv, void *data,
		co_unsigned16_t net, const char *begin, const char *end,
		struct floc *at)
{
	assert(srv == CO_GW_SRV_SET_ID);
	assert(begin);

	const char *cp = begin;
	size_t chars = 0;

	co_unsigned8_t node = 0;
	if (__unlikely(!(chars = lex_c99_u8(cp, end, at, &node)))) {
		diag_if(DIAG_ERROR, 0, at, "expected node-ID");
		return 0;
	}
	cp += chars;

	if (__unlikely(!node || (node > CO_NUM_NODES && node != 0xff))) {
		diag_if(DIAG_ERROR, 0, at,
				"the node-ID must be in the range [1..%u, 255]",
				CO_NUM_NODES);
		return 0;
	}

	struct co_gw_req_node req = {
		.size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.node = node
	};
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
	if (__unlikely(!(chars = lex_c99_long(cp, end, at, &timeout)))) {
		diag_if(DIAG_ERROR, 0, at, "expected command time-out");
		return 0;
	}
	cp += chars;

	struct co_gw_req_set_cmd_timeout req = {
		.size = sizeof(req),
		.srv = srv,
		.data = data,
		.timeout = timeout
	};
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
	if (!strncmp("Disable", cp, chars)) {
		cp += chars;
		cs = 0;
	} else if (!strncmp("Disable", cp, chars)) {
		cp += chars;
		cs = 1;
	} else {
		diag_if(DIAG_ERROR, 0, at,
				"expected 'Disable' or 'Enable' after 'boot_up_indication'");
		return 0;
	}

	struct co_gw_req_set_bootup_ind req = {
		.size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.cs = cs
	};
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
	if (__unlikely(!(chars = lex_c99_u16(cp, end, at, &net)))) {
		diag_if(DIAG_ERROR, 0, at, "expected network-ID");
		return 0;
	}
	cp += chars;

	if (__unlikely(net > CO_GW_NUM_NET)) {
		diag_if(DIAG_ERROR, 0, at,
				"the network-ID must be in the range [0, 1..%u]",
				CO_GW_NUM_NET);
		return 0;
	}

	struct co_gw_req_net req = {
		.size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net
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
	if (__unlikely(!(chars = lex_c99_u8(cp, end, at, &node)))) {
		diag_if(DIAG_ERROR, 0, at, "expected node-ID");
		return 0;
	}
	cp += chars;

	if (__unlikely(node > CO_NUM_NODES)) {
		diag_if(DIAG_ERROR, 0, at,
				"the node-ID must be in the range [0, 1..%u]",
				CO_NUM_NODES);
		return 0;
	}

	struct co_gw_req_node req = {
		.size = sizeof(req),
		.srv = srv,
		.data = data,
		.net = net,
		.node = node
	};
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
	if (__unlikely(!(chars = lex_c99_u32(cp, end, at, &n)))) {
		diag_if(DIAG_ERROR, 0, at, "expected command size");
		return 0;
	}
	cp += chars;

	struct co_gw_req_set_cmd_size req = {
		.size = sizeof(req),
		.srv = srv,
		.data = data,
		.n = n
	};
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
		if (__unlikely(gw->send_func(req, gw->send_data))) {
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

	if (__unlikely(!(chars = lex_char('[', cp, end, at))))
		diag_if(DIAG_WARNING, 0, at,
					"expected '[' before sequence number");
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	co_unsigned32_t seq = 0;
	if (__unlikely(!(chars = lex_c99_u32(cp, end, at, &seq)))) {
		diag_if(DIAG_ERROR, 0, at, "expected sequence number");
		return 0;
	}
	cp += chars;
	cp += lex_ctype(&isblank, cp, end, at);

	if (__unlikely(!(chars = lex_char(']', cp, end, at))))
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

			if (__unlikely(!net || net > CO_GW_NUM_NET)) {
				diag_if(DIAG_ERROR, 0, at,
						"the network-ID must be in the range [1..%u]",
						CO_GW_NUM_NET);
				return 0;
			}

			if (__unlikely(node > CO_NUM_NODES)) {
				diag_if(DIAG_ERROR, 0, at,
						"the node-ID must be in the range [0..%u]",
						CO_NUM_NODES);
				return 0;
			}
		} else {
			if (__unlikely(net > CO_NUM_NODES)) {
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
co_gw_txt_lex_srv(const char *begin, const char *end, struct floc *at,
		int *psrv)
{
	assert(begin);
	assert(!end || end >= begin);

	const char *cp = begin;
	size_t chars = 0;

	int srv = 0;

	cp += lex_ctype(&isblank, cp, end, at);

	chars = co_gw_txt_lex_cmd(cp, end, at);
	if (!strncmp("boot_up_indication", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_SET_BOOTUP_IND;
	} else if (!strncmp("disable", cp, chars)) {
		cp += chars;
		cp += lex_ctype(&isblank, cp, end, at);

		chars = co_gw_txt_lex_cmd(cp, end, at);
		if (!strncmp("guarding", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_NMT_NG_DISABLE;
		} else if (!strncmp("heartbeat", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_NMT_HB_DISABLE;
		} else {
			diag_if(DIAG_ERROR, 0, at,
					"expected 'guarding' or 'heartbeat'");
			return 0;
		}
	} else if (!strncmp("enable", cp, chars)) {
		cp += chars;
		cp += lex_ctype(&isblank, cp, end, at);

		chars = co_gw_txt_lex_cmd(cp, end, at);
		if (!strncmp("guarding", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_NMT_NG_ENABLE;
		} else if (!strncmp("heartbeat", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_NMT_HB_ENABLE;
		} else {
			diag_if(DIAG_ERROR, 0, at,
					"expected 'guarding' or 'heartbeat'");
			return 0;
		}
	} else if (!strncmp("info", cp, chars)) {
		cp += chars;
		cp += lex_ctype(&isblank, cp, end, at);

		chars = co_gw_txt_lex_cmd(cp, end, at);
		if (!strncmp("version", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_GET_VERSION;
		} else {
			diag_if(DIAG_ERROR, 0, at, "expected 'version'");
			return 0;
		}
	} else if (!strncmp("init", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_INIT;
	} else if (!strncmp("preop", cp, chars)
			|| !strncmp("preoperational", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_NMT_ENTER_PREOP;
	} else if (!strncmp("reset", cp, chars)) {
		cp += chars;
		cp += lex_ctype(&isblank, cp, end, at);

		chars = co_gw_txt_lex_cmd(cp, end, at);
		if (!strncmp("comm", cp, chars)
				|| !strncmp("communication", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_NMT_RESET_COMM;
		} else if (!strncmp("node", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_NMT_RESET_NODE;
		} else {
			diag_if(DIAG_ERROR, 0, at,
					"expected 'node' or 'comm[unication]'");
			return 0;
		}
	} else if (!strncmp("set", cp, chars)) {
		cp += chars;
		cp += lex_ctype(&isblank, cp, end, at);

		chars = co_gw_txt_lex_cmd(cp, end, at);
		if (!strncmp("command_size", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_SET_CMD_SIZE;
		} else if (!strncmp("command_timeout", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_SET_CMD_TIMEOUT;
		} else if (!strncmp("heartbeat", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_SET_HB;
		} else if (!strncmp("id", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_SET_ID;
		} else if (!strncmp("network", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_SET_NET;
		} else if (!strncmp("node", cp, chars)) {
			cp += chars;
			srv = CO_GW_SRV_SET_NODE;
		} else {
			diag_if(DIAG_ERROR, 0, at,
					"expected 'command_size', 'command_timeout', 'heartbeat', 'id', 'network' or 'node'");
			return 0;
		}
	} else if (!strncmp("start", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_NMT_START;
	} else if (!strncmp("stop", cp, chars)) {
		cp += chars;
		srv = CO_GW_SRV_NMT_STOP;
	} else {
		diag_if(DIAG_ERROR, 0, at, "expected 'boot_up_indication', 'disable', 'enable', 'info', 'init', 'preop[erational]', 'reset', 'set', 'start' or 'stop'");
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

#endif // !LELY_NO_CO_GW_TXT

