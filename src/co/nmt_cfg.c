/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the NMT 'configuration request' functions.
 *
 * @see src/nmt_cfg.h
 *
 * @copyright 2017-2021 Lely Industries N.V.
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

#if !LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG

#include "nmt_cfg.h"
#if !LELY_NO_MALLOC
#include <lely/co/dcf.h>
#endif
#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/val.h>
#include <lely/util/diag.h>

#include <assert.h>
#if !LELY_NO_STDIO
#include <inttypes.h>
#endif
#include <stdlib.h>

#ifndef LELY_CO_NMT_CFG_RESET_TIMEOUT
/**
 * The timeout (in milliseconds) after sending the NMT 'reset communication' or
 * 'reset node' command.
 */
#define LELY_CO_NMT_CFG_RESET_TIMEOUT 1000
#endif

struct __co_nmt_cfg_state;
/// An opaque CANopen NMT 'configuration request' state type.
typedef const struct __co_nmt_cfg_state co_nmt_cfg_state_t;

/// A CANopen NMT 'configuration request' service.
struct __co_nmt_cfg {
	/// A pointer to a CAN network interface.
	can_net_t *net;
	/// A pointer to a CANopen device.
	co_dev_t *dev;
	/// A pointer to an NMT master service.
	co_nmt_t *nmt;
	/// A pointer to the current state.
	co_nmt_cfg_state_t *state;
	/// A pointer to the CAN frame receiver.
	can_recv_t *recv;
	/// A pointer to the CAN timer.
	can_timer_t *timer;
	/// The node-ID.
	co_unsigned8_t id;
	/// The NMT slave assignment (object 1F81).
	co_unsigned32_t assignment;
	/// A pointer to the Client-SDO used to access slave objects.
	co_csdo_t *sdo;
	/// The SDO abort code.
	co_unsigned32_t ac;
	/// The CANopen SDO upload request used for reading sub-objects.
	struct co_sdo_req req;
#if !LELY_NO_MALLOC
	/// The object dictionary stored in object 1F20 (Store DCF).
	co_dev_t *dev_1f20;
#endif
};

/**
 * The CAN receive callback function for a 'configuration request'.
 *
 * @see can_recv_func_t
 */
static int co_nmt_cfg_recv(const struct can_msg *msg, void *data);

/**
 * The CAN timer callback function for a 'configuration request'.
 *
 * @see can_timer_func_t
 */
static int co_nmt_cfg_timer(const struct timespec *tp, void *data);

/**
 * The CANopen SDO download confirmation callback function for a 'configuration
 * request'.
 *
 * @see co_csdo_dn_con_t
 */
static void co_nmt_cfg_dn_con(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac, void *data);

/**
 * Enters the specified state of a 'configuration request; and invokes the exit
 * and entry functions.
 */
static void co_nmt_cfg_enter(co_nmt_cfg_t *cfg, co_nmt_cfg_state_t *next);

/**
 * Invokes the 'CAN frame received' transition function of the current state of
 * a 'configuration request'.
 *
 * @param cfg a pointer to a 'configuration request'.
 * @param msg a pointer to the received CAN frame.
 */
static inline void co_nmt_cfg_emit_recv(
		co_nmt_cfg_t *cfg, const struct can_msg *msg);

/**
 * Invokes the 'timeout' transition function of the current state of a
 * 'configuration request'.
 *
 * @param cfg a pointer to a 'configuration request'.
 * @param tp  a pointer to the current time.
 */
static inline void co_nmt_cfg_emit_time(
		co_nmt_cfg_t *cfg, const struct timespec *tp);

/**
 * Invokes the 'SDO download confirmation' transition function of the current
 * state of a 'configuration request'.
 *
 * @param cfg    a pointer to a 'configuration request'.
 * @param ac     the SDO abort code (0 on success).
 * @param idx    the object index.
 * @param subidx the object sub-index.
 */
static inline void co_nmt_cfg_emit_dn_con(co_nmt_cfg_t *cfg,
		co_unsigned16_t idx, co_unsigned8_t subidx, co_unsigned32_t ac);

/**
 * Invokes the 'result received' transition function of the current state of a
 * 'configuration request'.
 *
 * @param cfg a pointer to a 'configuration request'.
 * @param ac  the SDO abort code (0 on success).
 */
static inline void co_nmt_cfg_emit_res(co_nmt_cfg_t *cfg, co_unsigned32_t ac);

/// A CANopen NMT 'configuration request' state.
struct __co_nmt_cfg_state {
	/// A pointer to the function invoked when a new state is entered.
	co_nmt_cfg_state_t *(*on_enter)(co_nmt_cfg_t *cfg);
	/**
	 * A pointer to the transition function invoked when a CAN frame has
	 * been received.
	 *
	 * @param cfg a pointer to a 'configuration request'.
	 * @param msg a pointer to the received CAN frame.
	 *
	 * @returns a pointer to the next state.
	 */
	co_nmt_cfg_state_t *(*on_recv)(
			co_nmt_cfg_t *cfg, const struct can_msg *msg);
	/**
	 * A pointer to the transition function invoked when a timeout occurs.
	 *
	 * @param cfg a pointer to a 'configuration request'.
	 * @param tp  a pointer to the current time.
	 *
	 * @returns a pointer to the next state.
	 */
	co_nmt_cfg_state_t *(*on_time)(
			co_nmt_cfg_t *cfg, const struct timespec *tp);
	/**
	 * A pointer to the transition function invoked when an NMT'update
	 * configuration' step completes.
	 *
	 * @param cfg a pointer to a 'configuration request'.
	 * @param ac  the SDO abort code (0 on success).
	 *
	 * @returns a pointer to the next state.
	 */
	co_nmt_cfg_state_t *(*on_res)(co_nmt_cfg_t *cfg, co_unsigned32_t ac);
	/**
	 * A pointer to the transition function invoked when an SDO download
	 * request completes.
	 *
	 * @param cfg    a pointer to a 'configuration request'.
	 * @param idx    the object index.
	 * @param subidx the object sub-index.
	 * @param ac     the SDO abort code (0 on success).
	 *
	 * @returns a pointer to the next state.
	 */
	co_nmt_cfg_state_t *(*on_dn_con)(co_nmt_cfg_t *cfg, co_unsigned16_t idx,
			co_unsigned8_t subidx, co_unsigned32_t ac);
	/// A pointer to the function invoked when the current state is left.
	void (*on_leave)(co_nmt_cfg_t *cfg);
};

#define LELY_CO_DEFINE_STATE(name, ...) \
	static co_nmt_cfg_state_t *const name = \
			&(co_nmt_cfg_state_t){ __VA_ARGS__ };

/// The entry function of the 'abort' state.
static co_nmt_cfg_state_t *co_nmt_cfg_abort_on_enter(co_nmt_cfg_t *cfg);

// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_cfg_abort_state,
	.on_enter = &co_nmt_cfg_abort_on_enter
)
// clang-format on

/// The entry function of the 'restore configuration' state.
static co_nmt_cfg_state_t *co_nmt_cfg_restore_on_enter(co_nmt_cfg_t *cfg);

/**
 * The 'SDO download confirmation' transition function of the 'restore
 * configuration' state.
 */
static co_nmt_cfg_state_t *co_nmt_cfg_restore_on_dn_con(co_nmt_cfg_t *cfg,
		co_unsigned16_t idx, co_unsigned8_t subidx, co_unsigned32_t ac);

// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_cfg_restore_state,
	.on_enter = &co_nmt_cfg_restore_on_enter,
	.on_dn_con = &co_nmt_cfg_restore_on_dn_con
)
// clang-format on

/// The entry function of the 'reset' state.
static co_nmt_cfg_state_t *co_nmt_cfg_reset_on_enter(co_nmt_cfg_t *cfg);

/// The 'CAN frame received' transition function of the 'reset' state.
static co_nmt_cfg_state_t *co_nmt_cfg_reset_on_recv(
		co_nmt_cfg_t *cfg, const struct can_msg *msg);

/// The 'timeout' transition function of the 'reset' state.
static co_nmt_cfg_state_t *co_nmt_cfg_reset_on_time(
		co_nmt_cfg_t *cfg, const struct timespec *tp);

// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_cfg_reset_state,
	.on_enter = &co_nmt_cfg_reset_on_enter,
	.on_recv = &co_nmt_cfg_reset_on_recv,
	.on_time = &co_nmt_cfg_reset_on_time
)
// clang-format on

#if !LELY_NO_MALLOC

/// The entry function of the 'store object 1F20' state.
static co_nmt_cfg_state_t *co_nmt_cfg_store_1f20_on_enter(co_nmt_cfg_t *cfg);

/**
 * The 'SDO download confirmation' transition function of the 'store object
 * 1F20' state.
 */
static co_nmt_cfg_state_t *co_nmt_cfg_store_1f20_on_dn_con(co_nmt_cfg_t *cfg,
		co_unsigned16_t idx, co_unsigned8_t subidx, co_unsigned32_t ac);

/// The exit function of the 'store object 1F20' state.
static void co_nmt_cfg_store_1f20_on_leave(co_nmt_cfg_t *cfg);

// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_cfg_store_1f20_state,
	.on_enter = &co_nmt_cfg_store_1f20_on_enter,
	.on_dn_con = &co_nmt_cfg_store_1f20_on_dn_con,
	.on_leave = &co_nmt_cfg_store_1f20_on_leave
)
// clang-format on

/// The entry function of the 'store object 1F22' state.
static co_nmt_cfg_state_t *co_nmt_cfg_store_1f22_on_enter(co_nmt_cfg_t *cfg);

/**
 * The 'SDO download confirmation' transition function of the 'store object
 * 1F22' state.
 */
static co_nmt_cfg_state_t *co_nmt_cfg_store_1f22_on_dn_con(co_nmt_cfg_t *cfg,
		co_unsigned16_t idx, co_unsigned8_t subidx, co_unsigned32_t ac);

// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_cfg_store_1f22_state,
	.on_enter = &co_nmt_cfg_store_1f22_on_enter,
	.on_dn_con = &co_nmt_cfg_store_1f22_on_dn_con
)
// clang-format on

#endif // !LELY_NO_MALLOC

/// The entry function of the 'user-defined configuration' state.
static co_nmt_cfg_state_t *co_nmt_cfg_user_on_enter(co_nmt_cfg_t *cfg);

/// The 'result received' function of the 'user-defined configuration' state.
static co_nmt_cfg_state_t *co_nmt_cfg_user_on_res(
		co_nmt_cfg_t *cfg, co_unsigned32_t ac);

// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_cfg_user_state,
	.on_enter = &co_nmt_cfg_user_on_enter,
	.on_res = &co_nmt_cfg_user_on_res
)
// clang-format on

#undef LELY_CO_DEFINE_STATE

void *
__co_nmt_cfg_alloc(void)
{
	void *ptr = malloc(sizeof(struct __co_nmt_cfg));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
__co_nmt_cfg_free(void *ptr)
{
	free(ptr);
}

struct __co_nmt_cfg *
__co_nmt_cfg_init(struct __co_nmt_cfg *cfg, can_net_t *net, co_dev_t *dev,
		co_nmt_t *nmt)
{
	assert(cfg);
	assert(net);
	assert(dev);
	assert(nmt);

	int errc = 0;

	cfg->net = net;
	cfg->dev = dev;
	cfg->nmt = nmt;

	cfg->state = NULL;

	cfg->recv = can_recv_create();
	if (!cfg->recv) {
		errc = get_errc();
		goto error_create_recv;
	}
	can_recv_set_func(cfg->recv, &co_nmt_cfg_recv, cfg);

	cfg->timer = can_timer_create();
	if (!cfg->timer) {
		errc = get_errc();
		goto error_create_timer;
	}
	can_timer_set_func(cfg->timer, &co_nmt_cfg_timer, cfg);

	cfg->id = 0;
	cfg->assignment = 0;

	cfg->sdo = NULL;

	cfg->ac = 0;

	co_sdo_req_init(&cfg->req);
#if !LELY_NO_MALLOC
	cfg->dev_1f20 = NULL;
#endif

	return cfg;

	// can_timer_destroy(cfg->timer);
error_create_timer:
	can_recv_destroy(cfg->recv);
error_create_recv:
	set_errc(errc);
	return NULL;
}

void
__co_nmt_cfg_fini(struct __co_nmt_cfg *cfg)
{
	assert(cfg);

#if !LELY_NO_MALLOC
	assert(!cfg->dev_1f20);
#endif
	co_sdo_req_fini(&cfg->req);

	co_csdo_destroy(cfg->sdo);

	can_timer_destroy(cfg->timer);
	can_recv_destroy(cfg->recv);
}

co_nmt_cfg_t *
co_nmt_cfg_create(can_net_t *net, co_dev_t *dev, co_nmt_t *nmt)
{
	int errc = 0;

	co_nmt_cfg_t *cfg = __co_nmt_cfg_alloc();
	if (!cfg) {
		errc = get_errc();
		goto error_alloc_cfg;
	}

	if (!__co_nmt_cfg_init(cfg, net, dev, nmt)) {
		errc = get_errc();
		goto error_init_cfg;
	}

	return cfg;

error_init_cfg:
	__co_nmt_cfg_free(cfg);
error_alloc_cfg:
	set_errc(errc);
	return NULL;
}

void
co_nmt_cfg_destroy(co_nmt_cfg_t *cfg)
{
	if (cfg) {
		__co_nmt_cfg_fini(cfg);
		__co_nmt_cfg_free(cfg);
	}
}

int
co_nmt_cfg_cfg_req(co_nmt_cfg_t *cfg, co_unsigned8_t id, int timeout,
		co_csdo_ind_t *dn_ind, co_csdo_ind_t *up_ind, void *data)
{
	assert(cfg);

	if (!id || id > CO_NUM_NODES) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	if (cfg->state) {
		set_errnum(ERRNUM_INPROGRESS);
		return -1;
	}

	cfg->id = id;

	co_csdo_destroy(cfg->sdo);
	cfg->sdo = co_csdo_create(cfg->net, NULL, cfg->id);
	if (!cfg->sdo)
		return -1;
	co_csdo_set_timeout(cfg->sdo, timeout);
	co_csdo_set_dn_ind(cfg->sdo, dn_ind, data);
	co_csdo_set_up_ind(cfg->sdo, up_ind, data);

	co_nmt_cfg_enter(cfg, co_nmt_cfg_restore_state);

	return 0;
}

int
co_nmt_cfg_cfg_res(co_nmt_cfg_t *cfg, co_unsigned32_t ac)
{
	assert(cfg);

	co_nmt_cfg_emit_res(cfg, ac);

	return 0;
}

static int
co_nmt_cfg_recv(const struct can_msg *msg, void *data)
{
	assert(msg);
	co_nmt_cfg_t *cfg = data;
	assert(cfg);

	co_nmt_cfg_emit_recv(cfg, msg);

	return 0;
}

static int
co_nmt_cfg_timer(const struct timespec *tp, void *data)
{
	assert(tp);
	co_nmt_cfg_t *cfg = data;
	assert(cfg);

	co_nmt_cfg_emit_time(cfg, tp);

	return 0;
}

static void
co_nmt_cfg_dn_con(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned32_t ac, void *data)
{
	(void)sdo;
	co_nmt_cfg_t *cfg = data;
	assert(cfg);

	co_nmt_cfg_emit_dn_con(cfg, idx, subidx, ac);
}

static void
co_nmt_cfg_enter(co_nmt_cfg_t *cfg, co_nmt_cfg_state_t *next)
{
	assert(cfg);

	while (next) {
		co_nmt_cfg_state_t *prev = cfg->state;
		cfg->state = next;

		if (prev && prev->on_leave)
			prev->on_leave(cfg);

		next = next->on_enter ? next->on_enter(cfg) : NULL;
	}
}

static inline void
co_nmt_cfg_emit_recv(co_nmt_cfg_t *cfg, const struct can_msg *msg)
{
	assert(cfg);
	assert(cfg->state);
	assert(cfg->state->on_recv);

	co_nmt_cfg_enter(cfg, cfg->state->on_recv(cfg, msg));
}

static inline void
co_nmt_cfg_emit_time(co_nmt_cfg_t *cfg, const struct timespec *tp)
{
	assert(cfg);
	assert(cfg->state);
	assert(cfg->state->on_time);

	co_nmt_cfg_enter(cfg, cfg->state->on_time(cfg, tp));
}

static inline void
co_nmt_cfg_emit_dn_con(co_nmt_cfg_t *cfg, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac)
{
	assert(cfg);
	assert(cfg->state);
	assert(cfg->state->on_dn_con);

	co_nmt_cfg_enter(cfg, cfg->state->on_dn_con(cfg, idx, subidx, ac));
}

static inline void
co_nmt_cfg_emit_res(co_nmt_cfg_t *cfg, co_unsigned32_t ac)
{
	assert(cfg);
	assert(cfg->state);
	assert(cfg->state->on_res);

	co_nmt_cfg_enter(cfg, cfg->state->on_res(cfg, ac));
}

static co_nmt_cfg_state_t *
co_nmt_cfg_abort_on_enter(co_nmt_cfg_t *cfg)
{
	assert(cfg);

	can_recv_stop(cfg->recv);
	can_timer_stop(cfg->timer);

	co_nmt_cfg_con(cfg->nmt, cfg->id, cfg->ac);

	return NULL;
}

static co_nmt_cfg_state_t *
co_nmt_cfg_restore_on_enter(co_nmt_cfg_t *cfg)
{
	assert(cfg);

	cfg->ac = 0;

	// Retrieve the slave assignment for the node.
	cfg->assignment = co_dev_get_val_u32(cfg->dev, 0x1f81, cfg->id);

	// Abort the configuration request if the slave is not in the network
	// list.
	if (!(cfg->assignment & 0x01))
		return co_nmt_cfg_abort_state;

	// Check if the slave can be used without prior resetting (bit 7).
	if (!(cfg->assignment & 0x80))
#if LELY_NO_MALLOC
		return co_nmt_cfg_user_state;
#else
		return co_nmt_cfg_store_1f20_state;
#endif

	// Retrieve the sub-index of object 1011 of the slave that is used to
	// initiate the restore operation.
	co_unsigned8_t subidx = co_dev_get_val_u8(cfg->dev, 0x1f8a, cfg->id);

	// If the sub-index is 0, no restore is sent to the slave.
	if (!subidx)
#if LELY_NO_MALLOC
		return co_nmt_cfg_user_state;
#else
		return co_nmt_cfg_store_1f20_state;
#endif

	// Write the value 'load' to sub-index of object 1011 on the slave.
	// clang-format off
	if (co_csdo_dn_val_req(cfg->sdo, 0x1011, subidx, CO_DEFTYPE_UNSIGNED32,
			&(co_unsigned32_t){ UINT32_C(0x64616f6c) },
			&co_nmt_cfg_dn_con, cfg) == -1) {
		// clang-format on
		cfg->ac = CO_SDO_AC_ERROR;
		return co_nmt_cfg_abort_state;
	}

	return NULL;
}

static co_nmt_cfg_state_t *
co_nmt_cfg_restore_on_dn_con(co_nmt_cfg_t *cfg, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac)
{
	assert(cfg);
	(void)idx;

	if (ac) {
		cfg->ac = ac;
		return co_nmt_cfg_abort_state;
	}

	switch (subidx) {
	case 0x02:
		// Issue the NMT reset communication command after restoring
		// communication related parameters.
		co_nmt_cs_req(cfg->nmt, CO_NMT_CS_RESET_COMM, cfg->id);
		break;
	default:
		// Issue the NMT reset node command after restoring application
		// or manufacturer-specific parameters.
		co_nmt_cs_req(cfg->nmt, CO_NMT_CS_RESET_NODE, cfg->id);
		break;
	}

	return co_nmt_cfg_reset_state;
}

static co_nmt_cfg_state_t *
co_nmt_cfg_reset_on_enter(co_nmt_cfg_t *cfg)
{
	assert(cfg);

	// Start the CAN frame receiver for the boot-up message.
	can_recv_start(cfg->recv, cfg->net, CO_NMT_EC_CANID(cfg->id), 0);
	// Wait until we receive a boot-up message.
	can_timer_timeout(cfg->timer, cfg->net, LELY_CO_NMT_CFG_RESET_TIMEOUT);

	return NULL;
}

static co_nmt_cfg_state_t *
co_nmt_cfg_reset_on_recv(co_nmt_cfg_t *cfg, const struct can_msg *msg)
{
	(void)cfg;
	(void)msg;

	can_recv_stop(cfg->recv);
#if LELY_NO_MALLOC
	return co_nmt_cfg_user_state;
#else
	return co_nmt_cfg_store_1f20_state;
#endif
}

static co_nmt_cfg_state_t *
co_nmt_cfg_reset_on_time(co_nmt_cfg_t *cfg, const struct timespec *tp)
{
	assert(cfg);
	(void)tp;

	cfg->ac = CO_SDO_AC_TIMEOUT;
	return co_nmt_cfg_abort_state;
}

#if !LELY_NO_MALLOC

static co_nmt_cfg_state_t *
co_nmt_cfg_store_1f20_on_enter(co_nmt_cfg_t *cfg)
{
#if LELY_NO_CO_DCF
	(void)cfg;

	return co_nmt_cfg_store_1f22_state;
#else // !LELY_NO_CO_DCF
	assert(cfg);

	// Check if the DCF is available and the format is plain ASCII.
	co_sub_t *sub = co_dev_find_sub(cfg->dev, 0x1f20, cfg->id);
	if (!sub || co_dev_get_val_u8(cfg->dev, 0x1f21, cfg->id) != 0)
		return co_nmt_cfg_store_1f22_state;

	// Upload the DCF.
	struct co_sdo_req *req = &cfg->req;
	co_sdo_req_clear(req);
	cfg->ac = co_sub_up_ind(sub, req);
	if (cfg->ac) {
#if !LELY_NO_STDIO
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" on upload request of object 1F20:%02X (Store DCF): %s",
				cfg->ac, cfg->id, co_sdo_ac2str(cfg->ac));
#endif
		return co_nmt_cfg_abort_state;
	}

	if (!co_sdo_req_first(req) || !co_sdo_req_last(req)) {
#if !LELY_NO_STDIO
		diag(DIAG_WARNING, 0,
				"object 1F20:%02X (Store DCF) unusable for configuration request",
				cfg->id);
#endif
		return co_nmt_cfg_store_1f22_state;
	}

	// Ignore an empty DCF.
	if (!req->nbyte)
		return co_nmt_cfg_store_1f22_state;

	// Parse the DCF.
	assert(!cfg->dev_1f20);
	cfg->dev_1f20 = co_dev_create_from_dcf_text(
			req->buf, (const char *)req->buf + req->nbyte, NULL);
	if (!cfg->dev_1f20) {
		cfg->ac = CO_SDO_AC_ERROR;
		return co_nmt_cfg_abort_state;
	}

	return co_nmt_cfg_store_1f20_on_dn_con(cfg, 0, 0, 0);
#endif // !LELY_NO_CO_DCF
}

static co_nmt_cfg_state_t *
co_nmt_cfg_store_1f20_on_dn_con(co_nmt_cfg_t *cfg, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac)
{
	assert(cfg);
	assert(cfg->dev_1f20);

	if (ac) {
		cfg->ac = ac;
		return co_nmt_cfg_abort_state;
	}

	// Find the next (or first) sub-object in the object dictionary.
	co_obj_t *obj;
	co_sub_t *sub;
	if (idx) {
		obj = co_dev_find_obj(cfg->dev_1f20, idx);
		assert(obj);
		sub = co_obj_find_sub(obj, subidx);
		assert(sub);
		sub = co_sub_next(sub);
	} else {
		obj = co_dev_first_obj(cfg->dev_1f20);
		if (!obj)
			return co_nmt_cfg_store_1f22_state;
		sub = co_obj_first_sub(obj);
	}

	// Find the next sub-object to be written.
	co_unsigned16_t type;
	const void *val;
	for (;; sub = co_sub_next(sub)) {
		while (!sub) {
			obj = co_obj_next(obj);
			if (!obj)
				return co_nmt_cfg_store_1f22_state;
			sub = co_obj_first_sub(obj);
		}
		// Skip read-only sub-objects.
		if (!(co_sub_get_access(sub) & CO_ACCESS_WRITE))
			continue;
		// Skip file-based sub-objects.
		if (co_sub_get_flags(sub) & CO_OBJ_FLAGS_DOWNLOAD_FILE)
			continue;
		// Skip sub-objects containing the default value.
		type = co_sub_get_type(sub);
		val = co_sub_get_val(sub);
#if !LELY_NO_CO_OBJ_DEFAULT
		const void *def = co_sub_get_def(sub);
		if (!co_val_cmp(type, def, val))
			continue;
#endif
		break;
	}

	// Write the value to the slave.
	idx = co_obj_get_idx(obj);
	subidx = co_sub_get_subidx(sub);
	// clang-format off
	if (co_csdo_dn_val_req(cfg->sdo, idx, subidx, type, val,
			&co_nmt_cfg_dn_con, cfg) == -1) {
		// clang-format on
		cfg->ac = CO_SDO_AC_ERROR;
		return co_nmt_cfg_abort_state;
	}

	return NULL;
}

static void
co_nmt_cfg_store_1f20_on_leave(co_nmt_cfg_t *cfg)
{
	assert(cfg);

	co_dev_destroy(cfg->dev_1f20);
	cfg->dev_1f20 = NULL;
}

static co_nmt_cfg_state_t *
co_nmt_cfg_store_1f22_on_enter(co_nmt_cfg_t *cfg)
{
	assert(cfg);

	co_sub_t *sub = co_dev_find_sub(cfg->dev, 0x1f22, cfg->id);
	if (!sub)
		return co_nmt_cfg_user_state;

	// Upload the concise DCF.
	struct co_sdo_req *req = &cfg->req;
	co_sdo_req_clear(req);
	cfg->ac = co_sub_up_ind(sub, req);
	if (cfg->ac) {
#if !LELY_NO_STDIO
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" on upload request of object 1F22:%02X (Concise DCF): %s",
				cfg->ac, cfg->id, co_sdo_ac2str(cfg->ac));
#endif
		return co_nmt_cfg_abort_state;
	}

	if (!co_sdo_req_first(req) || !co_sdo_req_last(req)) {
		diag(DIAG_WARNING, 0,
				"object 1F22:%02X (Concise DCF) unusable for configuration request",
				cfg->id);
		return co_nmt_cfg_user_state;
	}

	// Ignore an empty concise DCF.
	if (!req->nbyte)
		return co_nmt_cfg_user_state;

	// Submit download requests for all entries in the concise DCF.
	const uint_least8_t *begin = req->buf;
	const uint_least8_t *end = begin + req->nbyte;
	if (co_csdo_dn_dcf_req(cfg->sdo, begin, end, &co_nmt_cfg_dn_con, cfg)
			== -1) {
		cfg->ac = CO_SDO_AC_ERROR;
		return co_nmt_cfg_abort_state;
	}

	return NULL;
}

static co_nmt_cfg_state_t *
co_nmt_cfg_store_1f22_on_dn_con(co_nmt_cfg_t *cfg, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac)
{
	assert(cfg);
	(void)idx;
	(void)subidx;

	if (ac) {
		cfg->ac = ac;
		return co_nmt_cfg_abort_state;
	}

	return co_nmt_cfg_user_state;
}

#endif // !LELY_NO_MALLOC

static co_nmt_cfg_state_t *
co_nmt_cfg_user_on_enter(co_nmt_cfg_t *cfg)
{
	assert(cfg);

	co_nmt_cfg_ind(cfg->nmt, cfg->id, cfg->sdo);

	return NULL;
}

static co_nmt_cfg_state_t *
co_nmt_cfg_user_on_res(co_nmt_cfg_t *cfg, co_unsigned32_t ac)
{
	assert(cfg);

	cfg->ac = ac;
	return co_nmt_cfg_abort_state;
}

#endif // !LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG
