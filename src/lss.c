/*!\file
 * This file is part of the CANopen library; it contains the implementation of
 * the Layer Setting Services (LSS) and protocols functions.
 *
 * \see lely/co/lss.h
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

#ifndef LELY_NO_CO_LSS

#include <lely/util/endian.h>
#include <lely/util/errnum.h>
#include <lely/co/dev.h>
#include <lely/co/lss.h>
#include <lely/co/nmt.h>
#include <lely/co/obj.h>

#include <assert.h>
#include <stdlib.h>

struct __co_lss_state;
//! An opaque CANopen LSS state type.
typedef const struct __co_lss_state co_lss_state_t;

//! A CANopen LSS master/slave service.
struct __co_lss {
	//! A pointer to a CAN network interface.
	can_net_t *net;
	//! A pointer to a CANopen device.
	co_dev_t *dev;
	//! A pointer to an NMT master/slave service.
	co_nmt_t *nmt;
	//! A pointer to the current state.
	co_lss_state_t *state;
#ifndef LELY_NO_CO_MASTER
	//! A flag specifying whether the LSS service is a master or a slave.
	int master;
#endif
	//! A pointer to the CAN frame receiver.
	can_recv_t *recv;
	//! The expected command specifier.
	co_unsigned8_t cs;
	//! The LSSPos value.
	co_unsigned8_t lsspos;
	//! A pointer to the 'activate bit timing' indication function.
	co_lss_rate_ind_t *rate_ind;
	//! A pointer to user-specified data for #rate_ind.
	void *rate_data;
	//! A pointer to the 'store configuration' indication function.
	co_lss_store_ind_t *store_ind;
	//! A pointer to user-specified data for #store_ind.
	void *store_data;
};

//! The CAN receive callback function for an LSS service. \see can_recv_func_t
static int co_lss_recv(const struct can_msg *msg, void *data);

/*!
 * Enters the specified state of an LSS service and invokes the exit and entry
 * functions.
 */
static void co_lss_enter(co_lss_t *lss, co_lss_state_t *next);

/*!
 * Invokes the 'CAN frame received' transition function of the current state of
 * an LSS service.
 *
 * \param lss a pointer to an LSS service.
 * \param msg a pointer to the received CAN frame.
 */
static inline void co_lss_emit_recv(co_lss_t *lss, const struct can_msg *msg);

//! A CANopen LSS state.
struct __co_lss_state {
	//! A pointer to the function invoked when a new state is entered.
	co_lss_state_t *(*on_enter)(co_lss_t *lss);
	/*!
	 * A pointer to the transition function invoked when a CAN frame has
	 * been received.
	 *
	 * \param lss a pointer to an LSS service.
	 * \param msg a pointer to the received CAN frame.
	 *
	 * \returns a pointer to the next state.
	 */
	co_lss_state_t *(*on_recv)(co_lss_t *lss, const struct can_msg *msg);
	//! A pointer to the function invoked when the current state is left.
	void (*on_leave)(co_lss_t *lss);
};

#define LELY_CO_DEFINE_STATE(name, ...) \
	static co_lss_state_t *const name = &(co_lss_state_t){ __VA_ARGS__  };

//! The entry function of the 'waiting' state an LSS master or slave.
static co_lss_state_t *co_lss_wait_on_enter(co_lss_t *lss);

//! The 'waiting' state of an LSS master or slave.
LELY_CO_DEFINE_STATE(co_lss_wait_state,
	.on_enter = &co_lss_wait_on_enter
)

//! The entry function of the 'waiting' state of an LSS slave.
static co_lss_state_t *co_lss_wait_slave_on_enter(co_lss_t *lss);

/*!
 * The 'CAN frame received' transition function of the 'waiting' state of an LSS
 * slave.
 */
static co_lss_state_t *co_lss_wait_slave_on_recv(co_lss_t *lss,
		const struct can_msg *msg);

//! The 'waiting' state of an LSS slave.
LELY_CO_DEFINE_STATE(co_lss_wait_slave_state,
	.on_enter = &co_lss_wait_slave_on_enter,
	.on_recv = &co_lss_wait_slave_on_recv
)

/*!
 * The 'CAN frame received' transition function of the 'configuration' state of
 * an LSS slave.
 */
static co_lss_state_t *co_lss_cfg_on_recv(co_lss_t *lss,
		const struct can_msg *msg);

//! The 'configuration' state of an LSS slave.
LELY_CO_DEFINE_STATE(co_lss_cfg_state,
	.on_recv = &co_lss_cfg_on_recv
)

#undef LELY_CO_DEFINE_STATE

/*!
 * Implements the switch state selective service for an LSS slave. See Fig. 32
 * in CiA 305 version 3.0.0.
 *
 * \param lss a pointer to an LSS slave service.
 * \param cs  the command specifier (in the range [0x40..0x43]).
 * \param id  the current LSS number to be checked.
 *
 * \returns a pointer to the next state.
 */
static co_lss_state_t *co_lss_switch_sel(co_lss_t *lss, co_unsigned8_t cs,
		co_unsigned32_t id);

/*!
 * Implements the LSS identify remote slave service for an LSS slave. See
 * Fig. 42 in CiA 305 version 3.0.0.
 *
 * \param lss a pointer to an LSS slave service.
 * \param cs  the command specifier (in the range [0x46..0x4b]).
 * \param id  the current LSS number to be checked.
 */
static void co_lss_id_slave(co_lss_t *lss, co_unsigned8_t cs,
		co_unsigned32_t id);

/*!
 * Implements the LSS identify non-configured remote slave service for an LSS
 * slave. See Fig. 44 in CiA 305 version 3.0.0.
 */
static void co_lss_id_non_cfg_slave(const co_lss_t *lss);

/*!
 * Implements the LSS fastscan service for an LSS slave. See Fig. 46 in CiA 305
 * version 3.0.0.
 *
 * \param lss     a pointer to an LSS slave service.
 * \param id      the value of LSS number which is currently determined by the
 *                LSS master.
 * \param bitchk  the least significant bit of \a id to be checked.
 * \param lsssub  the index of \a id in the LSS address (in the range [0..3]).
 * \param lssnext the value if \a lsssub in the next request.
 *
 * \returns a pointer to the next state.
 */
static co_lss_state_t *co_lss_fastscan(co_lss_t *lss, co_unsigned32_t id,
		co_unsigned8_t bitchk, co_unsigned8_t lsssub,
		co_unsigned8_t lssnext);

/*!
 * Initializes an LSS request CAN frame.
 *
 * \param lss a pointer to an LSS service.
 * \param msg a pointer to the CAN frame to be initialized.
 * \param cs  the command specifier.
 */
static void co_lss_init_req(const co_lss_t *lss, struct can_msg *msg,
		co_unsigned8_t cs);

LELY_CO_EXPORT void *
__co_lss_alloc(void)
{
	return malloc(sizeof(struct __co_lss));
}

LELY_CO_EXPORT void
__co_lss_free(void *ptr)
{
	free(ptr);
}

LELY_CO_EXPORT struct __co_lss *
__co_lss_init(struct __co_lss *lss, can_net_t *net, co_dev_t *dev,
		co_nmt_t *nmt)
{
	assert(lss);
	assert(net);
	assert(dev);
	assert(nmt);

	errc_t errc = 0;

	lss->net = net;
	lss->dev = dev;
	lss->nmt = nmt;

	lss->state = NULL;

#ifndef LELY_NO_CO_MASTER
	lss->master = 0;
#endif

	lss->recv = can_recv_create();
	if (__unlikely(!lss->recv)) {
		errc = get_errc();
		goto error_create_recv;
	}
	can_recv_set_func(lss->recv, &co_lss_recv, lss);

	lss->cs = 0;
	lss->lsspos = 0;

	lss->rate_ind = NULL;
	lss->rate_data = NULL;
	lss->store_ind = NULL;
	lss->store_data = NULL;

	co_lss_enter(lss, co_lss_wait_state);
	return lss;

	can_recv_destroy(lss->recv);
error_create_recv:
	set_errc(errc);
	return NULL;
}

LELY_CO_EXPORT void
__co_lss_fini(struct __co_lss *lss)
{
	__unused_var(lss);
}

LELY_CO_EXPORT co_lss_t *
co_lss_create(can_net_t *net, co_dev_t *dev, co_nmt_t *nmt)
{
	errc_t errc = 0;

	co_lss_t *lss = __co_lss_alloc();
	if (__unlikely(!lss)) {
		errc = get_errc();
		goto error_alloc_lss;
	}

	if (__unlikely(!__co_lss_init(lss, net, dev, nmt))) {
		errc = get_errc();
		goto error_init_lss;
	}

	return lss;

error_init_lss:
	__co_lss_free(lss);
error_alloc_lss:
	set_errc(errc);
	return NULL;
}

LELY_CO_EXPORT void
co_lss_destroy(co_lss_t *lss)
{
	if (lss) {
		__co_lss_fini(lss);
		__co_lss_free(lss);
	}
}

LELY_CO_EXPORT void
co_lss_get_rate_ind(const co_lss_t *lss, co_lss_rate_ind_t **pind, void **pdata)
{
	assert(lss);

	if (pind)
		*pind = lss->rate_ind;
	if (pdata)
		*pdata = lss->rate_data;
}

LELY_CO_EXPORT void
co_lss_set_rate_ind(co_lss_t *lss, co_lss_rate_ind_t *ind, void *data)
{
	assert(lss);

	lss->rate_ind = ind;
	lss->rate_data = data;
}

LELY_CO_EXPORT void
co_lss_get_store_ind(const co_lss_t *lss, co_lss_store_ind_t **pind,
		void **pdata)
{
	assert(lss);

	if (pind)
		*pind = lss->store_ind;
	if (pdata)
		*pdata = lss->store_data;
}

LELY_CO_EXPORT void
co_lss_set_store_ind(co_lss_t *lss, co_lss_store_ind_t *ind, void *data)
{
	assert(lss);

	lss->store_ind = ind;
	lss->store_data = data;
}

static int
co_lss_recv(const struct can_msg *msg, void *data)
{
	assert(msg);
	co_lss_t *lss = data;
	assert(lss);

	co_lss_emit_recv(lss, msg);

	return 0;
}

LELY_CO_EXPORT int
co_lss_is_master(const co_lss_t *lss)
{
#ifdef LELY_NO_CO_MASTER
	__unused_var(lss);

	return 0;
#else
	assert(lss);

	return lss->master;
#endif
}

static void
co_lss_enter(co_lss_t *lss, co_lss_state_t *next)
{
	assert(lss);
	assert(lss->state);

	while (next) {
		co_lss_state_t *prev = lss->state;
		lss->state = next;

		if (prev && prev->on_leave)
			prev->on_leave(lss);

		next = next->on_enter ? next->on_enter(lss) : NULL;
	}
}

static inline void
co_lss_emit_recv(co_lss_t *lss, const struct can_msg *msg)
{
	assert(lss);
	assert(lss->state);
	assert(lss->state->on_recv);

	co_lss_enter(lss, lss->state->on_recv(lss, msg));
}

static co_lss_state_t *
co_lss_wait_on_enter(co_lss_t *lss)
{
	assert(lss);

#ifndef LELY_NO_CO_MASTER
	// Only an NMT master can be an LSS master.
	if ((lss->master = co_nmt_is_master(lss->nmt)))
		return NULL;
#endif

	return co_lss_wait_slave_state;
}

static co_lss_state_t *
co_lss_wait_slave_on_enter(co_lss_t *lss)
{
	assert(lss);

	lss->cs = 0;
	lss->lsspos = 0;

	// Start receiving LSS commands from the master.
	can_recv_start(lss->recv, lss->net, 0x7e5, 0);

	return NULL;
}

static co_lss_state_t *
co_lss_wait_slave_on_recv(co_lss_t *lss, const struct can_msg *msg)
{
	assert(lss);
	assert(msg);

	if (__unlikely(!msg->len))
		return NULL;

	uint8_t cs = msg->data[0];
	switch (cs) {
	// Switch state global (see Fig. 31 in CiA 305 version 3.0.0).
	case 0x04:
		if (__unlikely(msg->len < 2))
			return NULL;
		switch (msg->data[1]) {
		case 0x00:
			// Re-enter the waiting state.
			return co_lss_wait_state;
		case 0x01:
			// Switch to the configuration state.
			return co_lss_cfg_state;
		}
		break;
	// Switch state selective.
	case 0x40: case 0x41: case 0x42: case 0x43:
		if (msg->len < 5)
			return NULL;
		return co_lss_switch_sel(lss, cs, ldle_u32(msg->data + 1));
	// LSS identify remote slave.
	case 0x46: case 0x47: case 0x48: case 0x49: case 0x4a: case 0x4b:
		if (msg->len < 5)
			return NULL;
		co_lss_id_slave(lss, msg->data[0], ldle_u32(msg->data + 1));
		break;
	// LSS identify non-configured remote slave.
	case 0x4c:
		co_lss_id_non_cfg_slave(lss);
		break;
	// LSS Fastscan.
	case 0x51:
		if (msg->len < 8)
			return NULL;
		return co_lss_fastscan(lss, ldle_u32(msg->data + 1),
				msg->data[5], msg->data[6], msg->data[7]);
	}

	return NULL;
}

static co_lss_state_t *
co_lss_cfg_on_recv(co_lss_t *lss, const struct can_msg *msg)
{
	assert(lss);
	assert(msg);

	errc_t errc = get_errc();
	co_obj_t *obj_1018 = co_dev_find_obj(lss->dev, 0x1018);
	struct can_msg req;

	if (__unlikely(!msg->len))
		return NULL;

	co_unsigned8_t cs = msg->data[0];
	switch (cs) {
	// Switch state global (see Fig. 31 in CiA 305 version 3.0.0).
	case 0x04:
		if (__unlikely(msg->len < 2))
			return NULL;
		switch (msg->data[1]) {
		case 0x00:
			// Switch to the waiting state.
			return co_lss_wait_state;
		case 0x01:
			// Re-enter the configuration state.
			return co_lss_cfg_state;
		}
		break;
	// Configure node-ID (see Fig. 33 in CiA 305 version 3.0.0).
	case 0x11:
		if (msg->len < 2)
			return NULL;
		// Configure the pending node-ID.
		co_lss_init_req(lss, &req, cs);
		if (co_nmt_set_id(lss->nmt, msg->data[1]) == -1) {
			// Discard the error code if the node-ID was invalid.
			set_errc(errc);
			req.data[1] = 1;
		}
		can_net_send(lss->net, &req);
		break;
	// Configure bit timing parameters (see Fig. 34 in CiA 305 version
	// 3.0.0).
	case 0x13:
		if (msg->len < 3)
			return NULL;
		// Configure the pending baudrate.
		co_lss_init_req(lss, &req, cs);
		if (__unlikely(!lss->rate_ind || msg->data[1])) {
			req.data[1] = 1;
		} else {
			unsigned int baud = co_dev_get_baud(lss->dev);
			switch (msg->data[2]) {
			case 0:
				if (!(req.data[1] = !(baud & CO_BAUD_1000)))
					co_dev_set_rate(lss->dev, 1000);
				break;
			case 1:
				if (!(req.data[1] = !(baud & CO_BAUD_800)))
					co_dev_set_rate(lss->dev, 800);
				break;
			case 2:
				if (!(req.data[1] = !(baud & CO_BAUD_500)))
					co_dev_set_rate(lss->dev, 500);
				break;
			case 3:
				if (!(req.data[1] = !(baud & CO_BAUD_250)))
					co_dev_set_rate(lss->dev, 250);
				break;
			case 4:
				if (!(req.data[1] = !(baud & CO_BAUD_125)))
					co_dev_set_rate(lss->dev, 135);
				break;
			case 6:
				if (!(req.data[1] = !(baud & CO_BAUD_50)))
					co_dev_set_rate(lss->dev, 50);
				break;
			case 7:
				if (!(req.data[1] = !(baud & CO_BAUD_20)))
					co_dev_set_rate(lss->dev, 20);
				break;
			case 8:
				if (!(req.data[1] = !(baud & CO_BAUD_10)))
					co_dev_set_rate(lss->dev, 10);
				break;
			case 9:
				if (!(req.data[1] = !(baud & CO_BAUD_AUTO)))
					co_dev_set_rate(lss->dev, 0);
				break;
			default:
				req.data[1] = 1;
				break;
			}
		}
		can_net_send(lss->net, &req);
		break;
	// Activate bit timing parameters (see Fig. 35 in CiA 305 version
	// 3.0.0).
	case 0x15:
		if (msg->len < 3 || !lss->rate_ind)
			return NULL;
		// Invoke the user-specified callback function to perform the
		// baudrate switch.
		lss->rate_ind(lss, co_dev_get_rate(lss->dev),
				ldle_u16(msg->data + 1), lss->rate_data);
		break;
	// Store configuration (see Fig. 36 in CiA 305 version 3.0.0).
	case 0x17:
		co_lss_init_req(lss, &req, cs);
		if (lss->store_ind) {
			// Store the pending node-ID and baudrate.
			if (__unlikely(lss->store_ind(lss,
					co_nmt_get_id(lss->nmt),
					co_dev_get_rate(lss->dev),
					lss->store_data) == -1)) {
				// Discard the error code.
				set_errc(errc);
				req.data[1] = 2;
			}
		} else {
			req.data[1] = 1;
		}
		can_net_send(lss->net, &req);
		break;
	// LSS identify remote slave.
	case 0x46: case 0x47: case 0x48: case 0x49: case 0x4a: case 0x4b:
		if (msg->len < 5)
			return NULL;
		co_lss_id_slave(lss, cs, ldle_u32(msg->data + 1));
		break;
	// LSS identify non-configured remote slave.
	case 0x4c:
		co_lss_id_non_cfg_slave(lss);
		break;
	// Inquire identity vendor-ID (Fig. 37 in CiA 305 version 3.0.0).
	case 0x5a:
		co_lss_init_req(lss, &req, cs);
		stle_u32(req.data + 1, co_obj_get_val_u32(obj_1018, 0x01));
		can_net_send(lss->net, &req);
		break;
	// Inquire identity product-code (Fig. 38 in CiA 305 version 3.0.0).
	case 0x5b:
		co_lss_init_req(lss, &req, cs);
		stle_u32(req.data + 1, co_obj_get_val_u32(obj_1018, 0x02));
		can_net_send(lss->net, &req);
		break;
	// Inquire identity revision number (Fig. 39 in CiA 305 version 3.0.0).
	case 0x5c:
		co_lss_init_req(lss, &req, cs);
		stle_u32(req.data + 1, co_obj_get_val_u32(obj_1018, 0x03));
		can_net_send(lss->net, &req);
		break;
	// Inquire identity serial-number (Fig. 40 in CiA 305 version 3.0.0).
	case 0x5d:
		co_lss_init_req(lss, &req, cs);
		stle_u32(req.data + 1, co_obj_get_val_u32(obj_1018, 0x04));
		can_net_send(lss->net, &req);
		break;
	// Inquire node-ID (Fig. 41 in CiA 305 version 3.0.0).
	case 0x5e:
		co_lss_init_req(lss, &req, cs);
		// Respond with the active or pending node-ID, depending on
		// whether the device is in the NMT state Initializing.
		switch (co_nmt_get_st(lss->nmt)) {
		case CO_NMT_ST_BOOTUP:
		case CO_NMT_ST_RESET_NODE:
		case CO_NMT_ST_RESET_COMM:
			req.data[1] = co_nmt_get_id(lss->nmt);
			break;
		default:
			req.data[1] = co_dev_get_id(lss->dev);
			break;
		}
		can_net_send(lss->net, &req);
		break;
	}

	return NULL;
}

static co_lss_state_t *
co_lss_switch_sel(co_lss_t *lss, co_unsigned8_t cs, co_unsigned32_t id)
{
	assert(lss);

	co_obj_t *obj_1018 = co_dev_find_obj(lss->dev, 0x1018);
	struct can_msg req;

	switch (cs) {
	case 0x40:
		if (id != co_obj_get_val_u32(obj_1018, 0x01)) {
			lss->cs = 0;
			return NULL;
		}
		lss->cs = 0x41;
		return NULL;
	case 0x41:
		if (cs != lss->cs || id != co_obj_get_val_u32(obj_1018, 0x02)) {
			lss->cs = 0;
			return NULL;
		}
		lss->cs = 0x42;
		return NULL;
	case 0x42:
		if (cs != lss->cs || id != co_obj_get_val_u32(obj_1018, 0x03)) {
			lss->cs = 0;
			return NULL;
		}
		lss->cs = 0x43;
		return NULL;
	case 0x43:
		if (cs != lss->cs || id != co_obj_get_val_u32(obj_1018, 0x04)) {
			lss->cs = 0;
			return NULL;
		}
		lss->cs = 0;
		// Notify the master of the state switch.
		co_lss_init_req(lss, &req, 0x44);
		can_net_send(lss->net, &req);
		// Switch to the configuration state.
		return co_lss_cfg_state;
	default:
		return NULL;
	}
}

static void
co_lss_id_slave(co_lss_t *lss, co_unsigned8_t cs, co_unsigned32_t id)
{
	assert(lss);

	co_obj_t *obj_1018 = co_dev_find_obj(lss->dev, 0x1018);
	struct can_msg req;

	switch (cs) {
	case 0x46:
		// Check the vendor-ID.
		if (id != co_obj_get_val_u32(obj_1018, 0x01)) {
			lss->cs = 0;
			return;
		}
		lss->cs = 0x47;
		break;
	case 0x47:
		// Check the product code.
		if (cs != lss->cs || id != co_obj_get_val_u32(obj_1018, 0x02)) {
			lss->cs = 0;
			return;
		}
		lss->cs = 0x48;
		break;
	case 0x48:
		// Check the lower bound of the revision number.
		if (cs != lss->cs || id > co_obj_get_val_u32(obj_1018, 0x03)) {
			lss->cs = 0;
			return;
		}
		lss->cs = 0x49;
		break;
	case 0x49:
		// Check the upper bound of the revision number.
		if (cs != lss->cs || id < co_obj_get_val_u32(obj_1018, 0x03)) {
			lss->cs = 0;
			return;
		}
		lss->cs = 0x4a;
		break;
	case 0x4a:
		// Check the lower bound of the serial number.
		if (cs != lss->cs || id > co_obj_get_val_u32(obj_1018, 0x04)) {
			lss->cs = 0;
			return;
		}
		lss->cs = 0x4b;
		break;
	case 0x4b:
		// Check the upper bound of the serial number.
		if (cs != lss->cs || id < co_obj_get_val_u32(obj_1018, 0x04)) {
			lss->cs = 0;
			return;
		}
		lss->cs = 0;
		// Notify the master that it is a match.
		co_lss_init_req(lss, &req, 0x4f);
		can_net_send(lss->net, &req);
		break;
	}
}

static void
co_lss_id_non_cfg_slave(const co_lss_t *lss)
{
	assert(lss);

	// Check if both the active and the pending node-ID are invalid.
	if (co_dev_get_id(lss->dev) != 0xff || co_nmt_get_id(lss->nmt) != 0xff)
		return;

	// Check if the device is in the NMT state Initialization.
	switch (co_nmt_get_st(lss->nmt)) {
	case CO_NMT_ST_BOOTUP:
	case CO_NMT_ST_RESET_NODE:
	case CO_NMT_ST_RESET_COMM:
		break;
	default:
		return;
	}

	struct can_msg req;
	co_lss_init_req(lss, &req, 0x50);
	can_net_send(lss->net, &req);
}

static co_lss_state_t *
co_lss_fastscan(co_lss_t *lss, co_unsigned32_t id, co_unsigned8_t bitchk,
		co_unsigned8_t lsssub, co_unsigned8_t lssnext)
{
	assert(lss);

	co_obj_t *obj_1018 = co_dev_find_obj(lss->dev, 0x1018);
	struct can_msg req;
	co_lss_state_t *next = NULL;

	if (__unlikely(bitchk > 31 && bitchk != 0x80))
		return NULL;

	if (bitchk == 0x80) {
		lss->lsspos = 0;
	} else {
		if (lss->lsspos > 3 || lss->lsspos != lsssub)
			return NULL;
		// Check if the unmasked bits of the specified IDNumber match.
		co_unsigned32_t pid[] = {
			co_obj_get_val_u32(obj_1018, 0x01),
			co_obj_get_val_u32(obj_1018, 0x02),
			co_obj_get_val_u32(obj_1018, 0x03),
			co_obj_get_val_u32(obj_1018, 0x04)
		};
		if ((id ^ pid[lss->lsspos]) & ~((1 << bitchk) - 1))
			return NULL;
		lss->lsspos = lssnext;
		// If this was the final bit, switch to the configuration state.
		if (!bitchk && lss->lsspos < lsssub)
			next = co_lss_cfg_state;
	}

	// Notify the master that it is a match.
	co_lss_init_req(lss, &req, 0x4f);
	can_net_send(lss->net, &req);

	return next;
}

static void
co_lss_init_req(const co_lss_t *lss, struct can_msg *msg, co_unsigned8_t cs)
{
	assert(lss);
	assert(msg);

	*msg = (struct can_msg)CAN_MSG_INIT;
	msg->id = 0x7e4 + !!co_lss_is_master(lss);
	msg->len = CAN_MAX_LEN;
	msg->data[0] = cs;
}

#endif // !LELY_NO_CO_LSS

