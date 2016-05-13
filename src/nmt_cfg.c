/*!\file
 * This file is part of the CANopen library; it contains the implementation of
 * the NMT 'configuration request' functions.
 *
 * \see src/nmt_cfg.h
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

#ifndef LELY_NO_CO_MASTER

#include <lely/util/errnum.h>
#include <lely/co/csdo.h>
#include <lely/co/dev.h>
#include "nmt_cfg.h"

#include <assert.h>
#include <stdlib.h>

struct __co_nmt_cfg_state;
//! An opaque CANopen NMT 'configuration request' state type.
typedef const struct __co_nmt_cfg_state co_nmt_cfg_state_t;

//! A CANopen NMT 'configuration request' service.
struct __co_nmt_cfg {
	//! A pointer to a CAN network interface.
	can_net_t *net;
	//! A pointer to a CANopen device.
	co_dev_t *dev;
	//! A pointer to an NMT master service.
	co_nmt_t *nmt;
	//! A pointer to the current state.
	co_nmt_cfg_state_t *state;
	//! The node-ID.
	co_unsigned8_t id;
	//! The NMT slave assignment (object 1F81).
	co_unsigned32_t assignment;
	//! A pointer to the Client-SDO used to access slave objects.
	co_csdo_t *sdo;
	//! The SDO abort code.
	co_unsigned32_t ac;
};

/*!
 * The CANopen SDO download confirmation callback function for a 'configuration
 * request'.
 *
 * \see co_csdo_dn_con_t
 */
static void co_nmt_cfg_dn_con(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac, void *data);

/*!
 * Enters the specified state of a 'configuration request; and invokes the exit
 * and entry functions.
 */
static void co_nmt_cfg_enter(co_nmt_cfg_t *cfg, co_nmt_cfg_state_t *next);

/*!
 * Invokes the 'SDO download confirmation' transition function of the current
 * state of a 'boot slave' service.
 *
 * \param cfg    a pointer to a 'configuration request'.
 * \param ac     the SDO abort code (0 on success).
 * \param idx    the object index.
 * \param subidx the object sub-index.
 */
static inline void co_nmt_cfg_emit_dn_con(co_nmt_cfg_t *cfg,
		co_unsigned16_t idx, co_unsigned8_t subidx, co_unsigned32_t ac);

/*!
 * Invokes the 'result received' transition function of the current state of a
 * 'configuration request'.
 *
 * \param cfg a pointer to a 'configuration request'.
 * \param ac  the SDO abort code (0 on success).
 */
static inline void co_nmt_cfg_emit_res(co_nmt_cfg_t *cfg, co_unsigned32_t ac);

//! A CANopen NMT 'configuration request' state.
struct __co_nmt_cfg_state {
	//! A pointer to the function invoked when a new state is entered.
	co_nmt_cfg_state_t *(*on_enter)(co_nmt_cfg_t *cfg);
	/*!
	 * A pointer to the transition function invoked when an NMT'update
	 * configuration' step completes.
	 *
	 * \param cfg a pointer to a 'configuration request'.
	 * \param ac  the SDO abort code (0 on success).
	 *
	 * \returns a pointer to the next state.
	 */
	co_nmt_cfg_state_t *(*on_res)(co_nmt_cfg_t *cfg, co_unsigned32_t ac);
	/*!
	 * A pointer to the transition function invoked when an SDO download
	 * request completes.
	 *
	 * \param cfg    a pointer to a 'configuration request'.
	 * \param idx    the object index.
	 * \param subidx the object sub-index.
	 * \param ac     the SDO abort code (0 on success).
	 *
	 * \returns a pointer to the next state.
	 */
	co_nmt_cfg_state_t *(*on_dn_con)(co_nmt_cfg_t *cfg, co_unsigned16_t idx,
			co_unsigned8_t subidx, co_unsigned32_t ac);
	//! A pointer to the function invoked when the current state is left.
	void (*on_leave)(co_nmt_cfg_t *cfg);
};

#define LELY_CO_DEFINE_STATE(name, ...) \
	static co_nmt_cfg_state_t *const name = \
			&(co_nmt_cfg_state_t){ __VA_ARGS__  };

//! The entry function of the 'initialization' state.
static co_nmt_cfg_state_t *co_nmt_cfg_init_on_enter(co_nmt_cfg_t *cfg);

//! The 'result received' function of the 'initialization' state.
static co_nmt_cfg_state_t *co_nmt_cfg_init_on_res(co_nmt_cfg_t *cfg,
		co_unsigned32_t ac);

LELY_CO_DEFINE_STATE(co_nmt_cfg_init_state,
	.on_enter = &co_nmt_cfg_init_on_enter,
	.on_res = &co_nmt_cfg_init_on_res
)

//! The entry function of the 'abort' state.
static co_nmt_cfg_state_t *co_nmt_cfg_abort_on_enter(co_nmt_cfg_t *cfg);

LELY_CO_DEFINE_STATE(co_nmt_cfg_abort_state,
	.on_enter = &co_nmt_cfg_abort_on_enter
)

//! The entry function of the 'restore configuration' state.
static co_nmt_cfg_state_t *co_nmt_cfg_restore_on_enter(co_nmt_cfg_t *cfg);

/*!
 * The 'SDO download confirmation' transition function of the 'restore
 * configuration' state.
 */
static co_nmt_cfg_state_t *co_nmt_cfg_restore_on_dn_con(co_nmt_cfg_t *cfg,
		co_unsigned16_t idx, co_unsigned8_t subidx, co_unsigned32_t ac);

LELY_CO_DEFINE_STATE(co_nmt_cfg_restore_state,
	.on_enter = &co_nmt_cfg_restore_on_enter,
	.on_dn_con = &co_nmt_cfg_restore_on_dn_con
)

#undef LELY_CO_DEFINE_STATE

void *
__co_nmt_cfg_alloc(void)
{
	return malloc(sizeof(struct __co_nmt_cfg));
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

	cfg->net = net;
	cfg->dev = dev;
	cfg->nmt = nmt;

	cfg->state = NULL;

	cfg->id = 0;
	cfg->assignment = 0;

	cfg->sdo = NULL;

	cfg->ac = 0;

	return cfg;
}

void
__co_nmt_cfg_fini(struct __co_nmt_cfg *cfg)
{
	assert(cfg);

	co_csdo_destroy(cfg->sdo);
}

co_nmt_cfg_t *
co_nmt_cfg_create(can_net_t *net, co_dev_t *dev, co_nmt_t *nmt)
{
	errc_t errc = 0;

	co_nmt_cfg_t *cfg = __co_nmt_cfg_alloc();
	if (__unlikely(!cfg)) {
		errc = get_errc();
		goto error_alloc_cfg;
	}

	if (__unlikely(!__co_nmt_cfg_init(cfg, net, dev, nmt))) {
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
co_nmt_cfg_cfg_req(co_nmt_cfg_t *cfg, co_unsigned8_t id, int timeout)
{
	assert(cfg);

	if (__unlikely(!id || id > CO_NUM_NODES)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	if (__unlikely(cfg->state)) {
		set_errnum(ERRNUM_INPROGRESS);
		return -1;
	}

	cfg->id = id;

	co_csdo_destroy(cfg->sdo);
	cfg->sdo = co_csdo_create(cfg->net, NULL, cfg->id);
	if (__unlikely(!cfg->sdo))
		return -1;
	co_csdo_set_timeout(cfg->sdo, timeout);

	co_nmt_cfg_enter(cfg, co_nmt_cfg_init_state);

	return 0;
}

int
co_nmt_cfg_cfg_res(co_nmt_cfg_t *cfg, co_unsigned32_t ac)
{
	assert(cfg);

	co_nmt_cfg_emit_res(cfg, ac);

	return 0;
}

static void
co_nmt_cfg_dn_con(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned32_t ac, void *data)
{
	__unused_var(sdo);
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
co_nmt_cfg_init_on_enter(co_nmt_cfg_t *cfg)
{
	assert(cfg);

	cfg->ac = 0;

	// Retrieve the slave assignment for the node.
	cfg->assignment = co_dev_get_val_u32(cfg->dev, 0x1f81, cfg->id);

	// Abort the configuration request if the slave is not in the network
	// list.
	if (!(cfg->assignment & 0x01))
		return co_nmt_cfg_abort_state;

	co_nmt_cfg_ind(cfg->nmt, cfg->id, cfg->sdo);

	return NULL;
}

static co_nmt_cfg_state_t *
co_nmt_cfg_init_on_res(co_nmt_cfg_t *cfg, co_unsigned32_t ac)
{
	assert(cfg);

	if (__unlikely(ac)) {
		cfg->ac = ac;
		return co_nmt_cfg_abort_state;
	}

	// We are done if the slave can be used without prior resetting (bit 7).
	if (!(cfg->assignment & 0x80))
		return co_nmt_cfg_abort_state;

	return co_nmt_cfg_restore_state;
}

static co_nmt_cfg_state_t *
co_nmt_cfg_abort_on_enter(co_nmt_cfg_t *cfg)
{
	assert(cfg);

	co_nmt_cfg_con(cfg->nmt, cfg->id, cfg->ac);

	return NULL;
}

static co_nmt_cfg_state_t *
co_nmt_cfg_restore_on_enter(co_nmt_cfg_t *cfg)
{
	assert(cfg);

	// Retrieve the sub-index of object 1011 of the slave that is used to
	// initiate the restore operation.
	co_unsigned8_t subidx = co_dev_get_val_u8(cfg->dev, 0x1f8a, cfg->id);

	// If the sub-index is 0, no restore is sent to the slave.
	if (!subidx)
		return co_nmt_cfg_abort_state;

	// Write the value 'load' to sub-index of object 1011 on the slave.
	if (__unlikely(co_csdo_dn_val_req(cfg->sdo, 0x1011, subidx,
			CO_DEFTYPE_UNSIGNED32,
			&(co_unsigned32_t){ UINT32_C(0x64616f6c) },
			&co_nmt_cfg_dn_con, cfg) == -1)) {
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
	__unused_var(idx);

	if (__unlikely(ac)) {
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

	return co_nmt_cfg_abort_state;
}

#endif // !LELY_NO_CO_MASTER

