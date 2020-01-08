/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the Receive-PDO functions.
 *
 * @see lely/co/rpdo.h
 *
 * @copyright 2016-2020 Lely Industries N.V.
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

#ifndef LELY_NO_CO_RPDO

#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/rpdo.h>
#include <lely/co/sdo.h>
#include <lely/co/val.h>
#include <lely/util/errnum.h>

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

/// A CANopen Receive-PDO.
struct __co_rpdo {
	/// A pointer to a CAN network interface.
	can_net_t *net;
	/// A pointer to a CANopen device.
	co_dev_t *dev;
	/// The PDO number.
	co_unsigned16_t num;
	/// The PDO communication parameter.
	struct co_pdo_comm_par comm;
	/// The PDO mapping parameter.
	struct co_pdo_map_par map;
	/// A pointer to the CAN frame receiver.
	can_recv_t *recv;
	/// A pointer to the CAN timer for deadline monitoring.
	can_timer_t *timer_event;
	/// A pointer to the CAN timer for the synchronous time window.
	can_timer_t *timer_swnd;
	/// A flag indicating we're waiting for a SYNC object to process #msg.
	unsigned int sync : 1;
	/// A flag indicating the synchronous time window has expired.
	unsigned int swnd : 1;
	/// A CAN frame waiting for a SYNC object to be processed.
	struct can_msg msg;
	/// The CANopen SDO download request used for writing sub-objects.
	struct co_sdo_req req;
	/// A pointer to the indication function.
	co_rpdo_ind_t *ind;
	/// A pointer to user-specified data for #ind.
	void *ind_data;
	/// A pointer to the error handling function.
	co_rpdo_err_t *err;
	/// A pointer to user-specified data for #err.
	void *err_data;
};

/**
 * Initializes the CAN frame receiver of a Receive-PDO service. This function
 * is invoked when one of the RPDO communication parameters (objects 1400..15FF)
 * is updated.
 *
 * @returns 0 on success, or -1 on error.
 */
static int co_rpdo_init_recv(co_rpdo_t *pdo);

/**
 * Initializes the CAN timer for deadline monitoring of a Receive-PDO service.
 * This function is invoked by co_rpdo_recv() after receiving an RPDO.
 *
 * @returns 0 on success, or -1 on error.
 */
static int co_rpdo_init_timer_event(co_rpdo_t *pdo);

/**
 * Initializes the CAN timer for the synchronous time window of a Receive-PDO
 * service. This function is invoked by co_rpdo_sync() or when one of the RPDO
 * communication parameters (objects 1400..15FF) is updated.
 *
 * @returns 0 on success, or -1 on error.
 */
static int co_rpdo_init_timer_swnd(co_rpdo_t *pdo);

/**
 * The download indication function for (all sub-objects of) CANopen objects
 * 1400..15FF (RPDO communication parameter).
 *
 * @see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1400_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);

/**
 * The download indication function for (all sub-objects of) CANopen objects
 * 1600..17FF (RPDO mapping parameter).
 *
 * @see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1600_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);

/**
 * The CAN receive callback function for a Receive-PDO service.
 *
 * @see can_recv_func_t
 */
static int co_rpdo_recv(const struct can_msg *msg, void *data);

/**
 * The CAN timer callback function for deadline monitoring of a Receive-PDO
 * service.
 *
 * @see can_timer_func_t
 */
static int co_rpdo_timer_event(const struct timespec *tp, void *data);

/**
 * The CAN timer callback function for the synchronous time window of a
 * Receive-PDO service.
 *
 * @see can_timer_func_t
 */
static int co_rpdo_timer_swnd(const struct timespec *tp, void *data);

/**
 * Parses a CAN frame received by a Receive-PDO service and updates the
 * corresponding objects in the object dictionary.
 *
 * @param pdo a pointer to a Receive-PDO service.
 * @param msg a pointer to the received CAN frame.
 *
 * @returns 0 on success, or an SDO abort code on error.
 */
static co_unsigned32_t co_rpdo_read_frame(
		co_rpdo_t *pdo, const struct can_msg *msg);

void *
__co_rpdo_alloc(void)
{
	void *ptr = malloc(sizeof(struct __co_rpdo));
	if (!ptr)
		set_errc(errno2c(errno));
	return ptr;
}

void
__co_rpdo_free(void *ptr)
{
	free(ptr);
}

struct __co_rpdo *
__co_rpdo_init(struct __co_rpdo *pdo, can_net_t *net, co_dev_t *dev,
		co_unsigned16_t num)
{
	assert(pdo);
	assert(net);
	assert(dev);

	int errc = 0;

	if (!num || num > 512) {
		errc = errnum2c(ERRNUM_INVAL);
		goto error_param;
	}

	// Find the PDO parameters in the object dictionary.
	co_obj_t *obj_1400 = co_dev_find_obj(dev, 0x1400 + num - 1);
	co_obj_t *obj_1600 = co_dev_find_obj(dev, 0x1600 + num - 1);
	if (!obj_1400 || !obj_1600) {
		errc = errnum2c(ERRNUM_INVAL);
		goto error_param;
	}

	pdo->net = net;
	pdo->dev = dev;
	pdo->num = num;

	// Copy the PDO communication parameter record.
	memset(&pdo->comm, 0, sizeof(pdo->comm));
	memcpy(&pdo->comm, co_obj_addressof_val(obj_1400),
			MIN(co_obj_sizeof_val(obj_1400), sizeof(pdo->comm)));

	// Copy the PDO mapping parameter record.
	memset(&pdo->map, 0, sizeof(pdo->map));
	memcpy(&pdo->map, co_obj_addressof_val(obj_1600),
			MIN(co_obj_sizeof_val(obj_1600), sizeof(pdo->map)));

	pdo->recv = NULL;

	pdo->timer_event = NULL;
	pdo->timer_swnd = NULL;

	pdo->sync = 0;
	pdo->swnd = 0;
	pdo->msg = (struct can_msg)CAN_MSG_INIT;

	co_sdo_req_init(&pdo->req);

	pdo->ind = NULL;
	pdo->ind_data = NULL;
	pdo->err = NULL;
	pdo->err_data = NULL;

	// Set the download indication functions PDO communication parameter
	// record.
	co_obj_set_dn_ind(obj_1400, &co_1400_dn_ind, pdo);

	// Set the download indication functions PDO mapping parameter record.
	co_obj_set_dn_ind(obj_1600, &co_1600_dn_ind, pdo);

	if (co_rpdo_init_recv(pdo) == -1) {
		errc = get_errc();
		goto error_init_recv;
	}

	if (co_rpdo_init_timer_swnd(pdo) == -1) {
		errc = get_errc();
		goto error_init_timer_swnd;
	}

	return pdo;

error_init_timer_swnd:
	can_recv_destroy(pdo->recv);
error_init_recv:
	co_obj_set_dn_ind(obj_1600, NULL, NULL);
	co_obj_set_dn_ind(obj_1400, NULL, NULL);
error_param:
	set_errc(errc);
	return NULL;
}

void
__co_rpdo_fini(struct __co_rpdo *pdo)
{
	assert(pdo);
	assert(pdo->num >= 1 && pdo->num <= 512);

	// Remove the download indication functions PDO mapping parameter
	// record.
	co_obj_t *obj_1600 = co_dev_find_obj(pdo->dev, 0x1600 + pdo->num - 1);
	assert(obj_1600);
	co_obj_set_dn_ind(obj_1600, NULL, NULL);

	// Remove the download indication functions PDO communication parameter
	// record.
	co_obj_t *obj_1400 = co_dev_find_obj(pdo->dev, 0x1400 + pdo->num - 1);
	assert(obj_1400);
	co_obj_set_dn_ind(obj_1400, NULL, NULL);

	co_sdo_req_fini(&pdo->req);

	can_timer_destroy(pdo->timer_swnd);
	can_timer_destroy(pdo->timer_event);

	can_recv_destroy(pdo->recv);
}

co_rpdo_t *
co_rpdo_create(can_net_t *net, co_dev_t *dev, co_unsigned16_t num)
{
	trace("creating Receive-PDO %d", num);

	int errc = 0;

	co_rpdo_t *pdo = __co_rpdo_alloc();
	if (!pdo) {
		errc = get_errc();
		goto error_alloc_pdo;
	}

	if (!__co_rpdo_init(pdo, net, dev, num)) {
		errc = get_errc();
		goto error_init_pdo;
	}

	return pdo;

error_init_pdo:
	__co_rpdo_free(pdo);
error_alloc_pdo:
	set_errc(errc);
	return NULL;
}

void
co_rpdo_destroy(co_rpdo_t *rpdo)
{
	if (rpdo) {
		trace("destroying Receive-PDO %d", rpdo->num);
		__co_rpdo_fini(rpdo);
		__co_rpdo_free(rpdo);
	}
}

can_net_t *
co_rpdo_get_net(const co_rpdo_t *pdo)
{
	assert(pdo);

	return pdo->net;
}

co_dev_t *
co_rpdo_get_dev(const co_rpdo_t *pdo)
{
	assert(pdo);

	return pdo->dev;
}

co_unsigned16_t
co_rpdo_get_num(const co_rpdo_t *pdo)
{
	assert(pdo);

	return pdo->num;
}

const struct co_pdo_comm_par *
co_rpdo_get_comm_par(const co_rpdo_t *pdo)
{
	assert(pdo);

	return &pdo->comm;
}

const struct co_pdo_map_par *
co_rpdo_get_map_par(const co_rpdo_t *pdo)
{
	assert(pdo);

	return &pdo->map;
}

void
co_rpdo_get_ind(const co_rpdo_t *pdo, co_rpdo_ind_t **pind, void **pdata)
{
	assert(pdo);

	if (pind)
		*pind = pdo->ind;
	if (pdata)
		*pdata = pdo->ind_data;
}

void
co_rpdo_set_ind(co_rpdo_t *pdo, co_rpdo_ind_t *ind, void *data)
{
	assert(pdo);

	pdo->ind = ind;
	pdo->ind_data = data;
}

void
co_rpdo_get_err(const co_rpdo_t *pdo, co_rpdo_err_t **perr, void **pdata)
{
	assert(pdo);

	if (perr)
		*perr = pdo->err;
	if (pdata)
		*pdata = pdo->err_data;
}

void
co_rpdo_set_err(co_rpdo_t *pdo, co_rpdo_err_t *err, void *data)
{
	assert(pdo);

	pdo->err = err;
	pdo->err_data = data;
}

int
co_rpdo_rtr(co_rpdo_t *pdo)
{
	assert(pdo);

	// Check whether the PDO exists and is valid.
	if (pdo->comm.cobid & CO_PDO_COBID_VALID)
		return 0;

	struct can_msg msg = CAN_MSG_INIT;
	msg.id = pdo->comm.cobid;
	if (pdo->comm.cobid & CO_PDO_COBID_FRAME) {
		msg.id &= CAN_MASK_EID;
		msg.flags |= CAN_FLAG_IDE;
	} else {
		msg.id &= CAN_MASK_BID;
	}
	msg.flags |= CAN_FLAG_RTR;
	return can_net_send(pdo->net, &msg);
}

int
co_rpdo_sync(co_rpdo_t *pdo, co_unsigned8_t cnt)
{
	assert(pdo);

	if (cnt > 240) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	// Ignore SYNC objects if the transmission type is not synchronous.
	if (pdo->comm.trans > 0xf0)
		return 0;

	// Reset the time window for synchronous PDOs.
	pdo->swnd = 0;
	co_rpdo_init_timer_swnd(pdo);

	// Check if we have a CAN frame waiting for a SYNC object.
	if (!pdo->sync)
		return 0;
	pdo->sync = 0;

	return co_rpdo_read_frame(pdo, &pdo->msg) ? -1 : 0;
}

static int
co_rpdo_init_recv(co_rpdo_t *pdo)
{
	assert(pdo);

	if (!(pdo->comm.cobid & CO_PDO_COBID_VALID)) {
		if (!pdo->recv) {
			pdo->recv = can_recv_create();
			if (!pdo->recv)
				return -1;
			can_recv_set_func(pdo->recv, co_rpdo_recv, pdo);
		}
		// Register the receiver under the specified CAN-ID.
		uint_least32_t id = pdo->comm.cobid;
		uint_least8_t flags = 0;
		if (id & CO_PDO_COBID_FRAME) {
			id &= CAN_MASK_EID;
			flags |= CAN_FLAG_IDE;
		} else {
			id &= CAN_MASK_BID;
		}
		can_recv_start(pdo->recv, pdo->net, id, flags);
	} else if (pdo->recv) {
		can_recv_destroy(pdo->recv);
		pdo->recv = NULL;
	}

	return 0;
}

static int
co_rpdo_init_timer_event(co_rpdo_t *pdo)
{
	assert(pdo);

	if (!(pdo->comm.cobid & CO_PDO_COBID_VALID) && pdo->comm.event) {
		if (!pdo->timer_event) {
			pdo->timer_event = can_timer_create();
			if (!pdo->timer_event)
				return -1;
			can_timer_set_func(pdo->timer_event,
					co_rpdo_timer_event, pdo);
		}
		can_timer_timeout(pdo->timer_event, pdo->net, pdo->comm.event);
	} else if (pdo->timer_event) {
		can_timer_destroy(pdo->timer_event);
		pdo->timer_event = NULL;
	}

	return 0;
}

static int
co_rpdo_init_timer_swnd(co_rpdo_t *pdo)
{
	assert(pdo);

	// Ignore the synchronous window length unless the RPDO is valid and
	// synchronous.
	co_unsigned32_t swnd = co_dev_get_val_u32(pdo->dev, 0x1007, 0x00);
	if (!(pdo->comm.cobid & CO_PDO_COBID_VALID) && pdo->comm.trans <= 0xf0
			&& swnd) {
		if (!pdo->timer_swnd) {
			pdo->timer_swnd = can_timer_create();
			if (!pdo->timer_swnd)
				return -1;
			can_timer_set_func(pdo->timer_swnd, co_rpdo_timer_swnd,
					pdo);
		}
		can_timer_timeout(pdo->timer_swnd, pdo->net, swnd);
	} else if (pdo->timer_swnd) {
		can_timer_destroy(pdo->timer_swnd);
		pdo->timer_swnd = NULL;
	}

	return 0;
}

static co_unsigned32_t
co_1400_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(req);
	co_rpdo_t *pdo = data;
	assert(pdo);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1400 + pdo->num - 1);

	co_unsigned32_t ac = 0;

	co_unsigned16_t type = co_sub_get_type(sub);
	union co_val val;
	if (co_sdo_req_dn_val(req, type, &val, &ac) == -1)
		return ac;

	switch (co_sub_get_subidx(sub)) {
	case 0: ac = CO_SDO_AC_NO_WRITE; goto error;
	case 1: {
		assert(type == CO_DEFTYPE_UNSIGNED32);
		co_unsigned32_t cobid = val.u32;
		co_unsigned32_t cobid_old = co_sub_get_val_u32(sub);
		if (cobid == cobid_old)
			goto error;

		// The CAN-ID cannot be changed when the PDO is and remains
		// valid.
		int valid = !(cobid & CO_PDO_COBID_VALID);
		int valid_old = !(cobid_old & CO_PDO_COBID_VALID);
		uint_least32_t canid = cobid & CAN_MASK_EID;
		uint_least32_t canid_old = cobid_old & CAN_MASK_EID;
		if (valid && valid_old && canid != canid_old) {
			ac = CO_SDO_AC_PARAM_VAL;
			goto error;
		}

		// A 29-bit CAN-ID is only valid if the frame bit is set.
		if (!(cobid & CO_PDO_COBID_FRAME)
				&& (cobid & (CAN_MASK_EID ^ CAN_MASK_BID))) {
			ac = CO_SDO_AC_PARAM_VAL;
			goto error;
		}

		pdo->comm.cobid = cobid;

		pdo->sync = 0;
		pdo->swnd = 0;

		co_rpdo_init_recv(pdo);
		co_rpdo_init_timer_swnd(pdo);
		break;
	}
	case 2: {
		assert(type == CO_DEFTYPE_UNSIGNED8);
		co_unsigned8_t trans = val.u8;
		co_unsigned8_t trans_old = co_sub_get_val_u8(sub);
		if (trans == trans_old)
			goto error;

		// Transmission types 0xF1..0xFD are reserved.
		if (trans > 0xf0 && trans < 0xfe) {
			ac = CO_SDO_AC_PARAM_VAL;
			goto error;
		}

		pdo->comm.trans = trans;

		pdo->swnd = 0;

		co_rpdo_init_timer_swnd(pdo);
		break;
	}
	case 3: {
		assert(type == CO_DEFTYPE_UNSIGNED16);
		co_unsigned16_t inhibit = val.u16;
		co_unsigned16_t inhibit_old = co_sub_get_val_u16(sub);
		if (inhibit == inhibit_old)
			goto error;

		// The inhibit time cannot be changed while the PDO exists and
		// is valid.
		if (!(pdo->comm.cobid & CO_PDO_COBID_VALID)) {
			ac = CO_SDO_AC_PARAM_VAL;
			goto error;
		}

		pdo->comm.inhibit = inhibit;
		break;
	}
	case 5: {
		assert(type == CO_DEFTYPE_UNSIGNED16);
		co_unsigned16_t event = val.u16;
		co_unsigned16_t event_old = co_sub_get_val_u16(sub);
		if (event == event_old)
			goto error;

		pdo->comm.event = event;
		break;
	}
	default: ac = CO_SDO_AC_NO_SUB; goto error;
	}

	co_sub_dn(sub, &val);
error:
	co_val_fini(type, &val);
	return ac;
}

static co_unsigned32_t
co_1600_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(req);
	co_rpdo_t *pdo = data;
	assert(pdo);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1600 + pdo->num - 1);

	co_unsigned32_t ac = 0;

	co_unsigned16_t type = co_sub_get_type(sub);
	union co_val val;
	if (co_sdo_req_dn_val(req, type, &val, &ac) == -1)
		return ac;

	int valid = !(pdo->comm.cobid & CO_PDO_COBID_VALID);

	if (!co_sub_get_subidx(sub)) {
		assert(type == CO_DEFTYPE_UNSIGNED8);
		co_unsigned8_t n = val.u8;
		co_unsigned8_t n_old = co_sub_get_val_u8(sub);
		if (n == n_old)
			goto error;

		// The PDO mapping cannot be changed when the PDO is valid.
		if (valid || n > 0x40) {
			ac = CO_SDO_AC_PARAM_VAL;
			goto error;
		}

		size_t bits = 0;
		for (size_t i = 1; i <= n; i++) {
			co_unsigned32_t map = pdo->map.map[i - 1];
			if (!map)
				continue;

			co_unsigned16_t idx = (map >> 16) & 0xffff;
			co_unsigned8_t subidx = (map >> 8) & 0xff;
			co_unsigned8_t len = map & 0xff;

			// Check the PDO length.
			if ((bits += len) > CAN_MAX_LEN * 8) {
				ac = CO_SDO_AC_PDO_LEN;
				goto error;
			}

			// Check whether the sub-object exists and can be mapped
			// into a PDO (or is a valid dummy entry).
			ac = co_dev_chk_rpdo(pdo->dev, idx, subidx);
			if (ac)
				goto error;
		}

		pdo->map.n = n;
	} else {
		assert(type == CO_DEFTYPE_UNSIGNED32);
		co_unsigned32_t map = val.u32;
		co_unsigned32_t map_old = co_sub_get_val_u32(sub);
		if (map == map_old)
			goto error;

		// The PDO mapping cannot be changed when the PDO is valid or
		// sub-index 0x00 is non-zero.
		if (valid || pdo->map.n) {
			ac = CO_SDO_AC_PARAM_VAL;
			goto error;
		}

		if (map) {
			co_unsigned16_t idx = (map >> 16) & 0xffff;
			co_unsigned8_t subidx = (map >> 8) & 0xff;
			// Check whether the sub-object exists and can be mapped
			// into a PDO (or is a valid dummy entry).
			ac = co_dev_chk_rpdo(pdo->dev, idx, subidx);
			if (ac)
				goto error;
		}

		pdo->map.map[co_sub_get_subidx(sub) - 1] = map;
	}

	co_sub_dn(sub, &val);
error:
	co_val_fini(type, &val);
	return ac;
}

static int
co_rpdo_recv(const struct can_msg *msg, void *data)
{
	assert(msg);
	co_rpdo_t *pdo = data;
	assert(pdo);

	// Ignore remote frames.
	if (msg->flags & CAN_FLAG_RTR)
		return 0;

#ifndef LELY_NO_CANFD
	// Ignore CAN FD format frames.
	if (msg->flags & CAN_FLAG_EDL)
		return 0;
#endif

	// Reset the event timer.
	co_rpdo_init_timer_event(pdo);

	if (pdo->comm.trans <= 0xf0) {
		// In case of a synchronous RPDO, save the frame to be processed
		// after the next SYNC object.
		if (!pdo->swnd) {
			pdo->sync = 1;
			pdo->msg = *msg;
		}
	} else if (pdo->comm.trans >= 0xfe) {
		// In case of an event-driven RPDO, process the frame directly.
		co_rpdo_read_frame(pdo, msg);
	}

	return 0;
}

static int
co_rpdo_timer_event(const struct timespec *tp, void *data)
{
	(void)tp;
	co_rpdo_t *pdo = data;
	assert(pdo);

	trace("RPDO %d: no PDO received in synchronous window", pdo->num);

	// Generate an error if an RPDO timeout occurred.
	if (pdo->err)
		pdo->err(pdo, 0x8250, 0x10, pdo->err_data);

	return 0;
}

static int
co_rpdo_timer_swnd(const struct timespec *tp, void *data)
{
	(void)tp;
	co_rpdo_t *pdo = data;
	assert(pdo);

	// The synchronous time window has expired.
	pdo->swnd = 1;

	return 0;
}

static co_unsigned32_t
co_rpdo_read_frame(co_rpdo_t *pdo, const struct can_msg *msg)
{
	assert(pdo);
	assert(msg);

	size_t n = MIN(msg->len, CAN_MAX_LEN);
	co_unsigned32_t ac =
			co_pdo_dn(&pdo->map, pdo->dev, &pdo->req, msg->data, n);

#ifndef NDEBUG
	if (ac)
		trace("RPDO %d: PDO error %08" PRIX32 " (%s)", pdo->num, ac,
				co_sdo_ac2str(ac));
#endif

	// Invoke the user-defined callback function.
	if (pdo->ind)
		pdo->ind(pdo, ac, msg->data, n, pdo->ind_data);

	if (pdo->err) {
		if (ac == CO_SDO_AC_PDO_LEN) {
			// Generate an error message if the PDO was not
			// processed because too few bytes were available.
			pdo->err(pdo, 0x8210, 0x10, pdo->err_data);
		} else if (!ac) {
			size_t offset = 0;
			for (size_t i = 0; i < MIN(pdo->map.n, 0x40u); i++)
				offset += (pdo->map.map[i]) & 0xff;
			if ((offset + 7) / 8 < n)
				// Generate an error message if the PDO length
				// exceeds the mapping.
				pdo->err(pdo, 0x8220, 0x10, pdo->err_data);
		}
	}

	return ac;
}

#endif // !LELY_NO_CO_RPDO
