/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the Receive-PDO functions.
 *
 * @see lely/co/rpdo.h
 *
 * @copyright 2016-2021 Lely Industries N.V.
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

#if !LELY_NO_CO_RPDO

#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/rpdo.h>
#include <lely/co/sdo.h>
#include <lely/co/val.h>
#include <lely/util/errnum.h>
#include <lely/util/time.h>

#include <assert.h>
#if !LELY_NO_STDIO
#include <inttypes.h>
#endif
#include <stdlib.h>

/// A CANopen Receive-PDO.
struct __co_rpdo {
	/// A pointer to a CAN network interface.
	can_net_t *net;
	/// A pointer to a CANopen device.
	co_dev_t *dev;
	/// The PDO number.
	co_unsigned16_t num;
	/// A flag specifying whether the Receive-PDO service is stopped.
	int stopped;
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
 */
static void co_rpdo_init_recv(co_rpdo_t *pdo);

/**
 * Initializes the CAN timer for deadline monitoring of a Receive-PDO service.
 * This function is invoked by co_rpdo_recv() after receiving an RPDO.
 */
static void co_rpdo_init_timer_event(co_rpdo_t *pdo);

/**
 * Initializes the CAN timer for the synchronous time window of a Receive-PDO
 * service. This function is invoked by co_rpdo_sync() or when one of the RPDO
 * communication parameters (objects 1400..15FF) is updated.
 */
static void co_rpdo_init_timer_swnd(co_rpdo_t *pdo);

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
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
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

	if (!num || num > CO_NUM_PDOS) {
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

	pdo->stopped = 1;

	memset(&pdo->comm, 0, sizeof(pdo->comm));
	memset(&pdo->map, 0, sizeof(pdo->map));

	pdo->recv = can_recv_create();
	if (!pdo->recv) {
		errc = get_errc();
		goto error_create_recv;
	}
	can_recv_set_func(pdo->recv, &co_rpdo_recv, pdo);

	pdo->timer_event = can_timer_create();
	if (!pdo->timer_event) {
		errc = get_errc();
		goto error_create_timer_event;
	}
	can_timer_set_func(pdo->timer_event, &co_rpdo_timer_event, pdo);

	pdo->timer_swnd = can_timer_create();
	if (!pdo->timer_swnd) {
		errc = get_errc();
		goto error_create_timer_swnd;
	}
	can_timer_set_func(pdo->timer_swnd, &co_rpdo_timer_swnd, pdo);

	pdo->sync = 0;
	pdo->swnd = 0;
	pdo->msg = (struct can_msg)CAN_MSG_INIT;

	co_sdo_req_init(&pdo->req);

	pdo->ind = NULL;
	pdo->ind_data = NULL;
	pdo->err = NULL;
	pdo->err_data = NULL;

	if (co_rpdo_start(pdo) == -1) {
		errc = get_errc();
		goto error_start;
	}

	return pdo;

	// co_rpdo_stop(pdo);
error_start:
	can_timer_destroy(pdo->timer_swnd);
error_create_timer_swnd:
	can_timer_destroy(pdo->timer_event);
error_create_timer_event:
	can_recv_destroy(pdo->recv);
error_create_recv:
error_param:
	set_errc(errc);
	return NULL;
}

void
__co_rpdo_fini(struct __co_rpdo *pdo)
{
	assert(pdo);
	assert(pdo->num >= 1 && pdo->num <= CO_NUM_PDOS);

	co_rpdo_stop(pdo);

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

int
co_rpdo_start(co_rpdo_t *pdo)
{
	assert(pdo);

	if (!pdo->stopped)
		return 0;

	co_obj_t *obj_1400 = co_dev_find_obj(pdo->dev, 0x1400 + pdo->num - 1);
	assert(obj_1400);
	// Copy the PDO communication parameter record.
	memcpy(&pdo->comm, co_obj_addressof_val(obj_1400),
			MIN(co_obj_sizeof_val(obj_1400), sizeof(pdo->comm)));
	// Set the download indication functions PDO communication parameter
	// record.
	co_obj_set_dn_ind(obj_1400, &co_1400_dn_ind, pdo);

	co_obj_t *obj_1600 = co_dev_find_obj(pdo->dev, 0x1600 + pdo->num - 1);
	assert(obj_1600);
	// Copy the PDO mapping parameter record.
	memcpy(&pdo->map, co_obj_addressof_val(obj_1600),
			MIN(co_obj_sizeof_val(obj_1600), sizeof(pdo->map)));
	// Set the download indication functions PDO mapping parameter record.
	co_obj_set_dn_ind(obj_1600, &co_1600_dn_ind, pdo);

	pdo->sync = 0;
	pdo->swnd = 0;

	co_rpdo_init_recv(pdo);

	pdo->stopped = 0;

	return 0;
}

void
co_rpdo_stop(co_rpdo_t *pdo)
{
	assert(pdo);

	if (pdo->stopped)
		return;

	can_timer_stop(pdo->timer_swnd);
	can_timer_stop(pdo->timer_event);

	can_recv_stop(pdo->recv);

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

	pdo->stopped = 1;
}

int
co_rpdo_is_stopped(const co_rpdo_t *pdo)
{
	assert(pdo);

	return pdo->stopped;
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

static void
co_rpdo_init_recv(co_rpdo_t *pdo)
{
	assert(pdo);

	if (!(pdo->comm.cobid & CO_PDO_COBID_VALID)) {
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
	} else {
		// Stop the receiver unless the RPDO is valid.
		can_recv_stop(pdo->recv);
	}
}

static void
co_rpdo_init_timer_event(co_rpdo_t *pdo)
{
	assert(pdo);

	can_timer_stop(pdo->timer_event);
	if (!(pdo->comm.cobid & CO_PDO_COBID_VALID) && pdo->comm.event)
		can_timer_timeout(pdo->timer_event, pdo->net, pdo->comm.event);
}

static void
co_rpdo_init_timer_swnd(co_rpdo_t *pdo)
{
	assert(pdo);

	can_timer_stop(pdo->timer_swnd);
	// Ignore the synchronous window length unless the RPDO is valid and
	// synchronous.
	co_unsigned32_t swnd = co_dev_get_val_u32(pdo->dev, 0x1007, 0x00);
	if (!(pdo->comm.cobid & CO_PDO_COBID_VALID) && pdo->comm.trans <= 0xf0
			&& swnd) {
		struct timespec start = { 0, 0 };
		can_net_get_time(pdo->net, &start);
		timespec_add_usec(&start, swnd);
		can_timer_start(pdo->timer_swnd, pdo->net, &start, NULL);
	}
}

static co_unsigned32_t
co_1400_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(req);
	co_rpdo_t *pdo = data;
	assert(pdo);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1400 + pdo->num - 1);

	co_unsigned16_t type = co_sub_get_type(sub);
	assert(!co_type_is_array(type));

	union co_val val;
	co_unsigned32_t ac = 0;
	if (co_sdo_req_dn_val(req, type, &val, &ac) == -1)
		return ac;

	switch (co_sub_get_subidx(sub)) {
	case 0: return CO_SDO_AC_NO_WRITE;
	case 1: {
		assert(type == CO_DEFTYPE_UNSIGNED32);
		co_unsigned32_t cobid = val.u32;
		co_unsigned32_t cobid_old = co_sub_get_val_u32(sub);
		if (cobid == cobid_old)
			return 0;

		// The CAN-ID cannot be changed when the PDO is and remains
		// valid.
		int valid = !(cobid & CO_PDO_COBID_VALID);
		int valid_old = !(cobid_old & CO_PDO_COBID_VALID);
		uint_least32_t canid = cobid & CAN_MASK_EID;
		uint_least32_t canid_old = cobid_old & CAN_MASK_EID;
		if (valid && valid_old && canid != canid_old)
			return CO_SDO_AC_PARAM_VAL;

		// A 29-bit CAN-ID is only valid if the frame bit is set.
		if (!(cobid & CO_PDO_COBID_FRAME)
				&& (cobid & (CAN_MASK_EID ^ CAN_MASK_BID)))
			return CO_SDO_AC_PARAM_VAL;

		pdo->comm.cobid = cobid;

		pdo->sync = 0;
		pdo->swnd = 0;

		co_rpdo_init_recv(pdo);
		can_timer_stop(pdo->timer_swnd);
		break;
	}
	case 2: {
		assert(type == CO_DEFTYPE_UNSIGNED8);
		co_unsigned8_t trans = val.u8;
		co_unsigned8_t trans_old = co_sub_get_val_u8(sub);
		if (trans == trans_old)
			return 0;

		// Transmission types 0xF1..0xFD are reserved.
		if (trans > 0xf0 && trans < 0xfe)
			return CO_SDO_AC_PARAM_VAL;

		pdo->comm.trans = trans;

		break;
	}
	case 3: {
		assert(type == CO_DEFTYPE_UNSIGNED16);
		co_unsigned16_t inhibit = val.u16;
		co_unsigned16_t inhibit_old = co_sub_get_val_u16(sub);
		if (inhibit == inhibit_old)
			return 0;

		// The inhibit time cannot be changed while the PDO exists and
		// is valid.
		if (!(pdo->comm.cobid & CO_PDO_COBID_VALID))
			return CO_SDO_AC_PARAM_VAL;

		pdo->comm.inhibit = inhibit;
		break;
	}
	case 5: {
		assert(type == CO_DEFTYPE_UNSIGNED16);
		co_unsigned16_t event = val.u16;
		co_unsigned16_t event_old = co_sub_get_val_u16(sub);
		if (event == event_old)
			return 0;

		pdo->comm.event = event;
		break;
	}
	default: return CO_SDO_AC_NO_SUB;
	}

	co_sub_dn(sub, &val);

	return 0;
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
	assert(!co_type_is_array(type));

	union co_val val;
	if (co_sdo_req_dn_val(req, type, &val, &ac) == -1)
		return ac;

	int valid = !(pdo->comm.cobid & CO_PDO_COBID_VALID);

	if (!co_sub_get_subidx(sub)) {
		assert(type == CO_DEFTYPE_UNSIGNED8);
		co_unsigned8_t n = val.u8;
		co_unsigned8_t n_old = co_sub_get_val_u8(sub);
		if (n == n_old)
			return 0;

		// The PDO mapping cannot be changed when the PDO is valid.
		if (valid)
			return CO_SDO_AC_PARAM_VAL;

		if (n <= CO_PDO_NUM_MAPS) {
			size_t bits = 0;
			for (size_t i = 1; i <= n; i++) {
				co_unsigned32_t map = pdo->map.map[i - 1];
				if (!map)
					continue;

				co_unsigned16_t idx = (map >> 16) & 0xffff;
				co_unsigned8_t subidx = (map >> 8) & 0xff;
				co_unsigned8_t len = map & 0xff;

				// Check the PDO length.
				if ((bits += len) > CAN_MAX_LEN * 8)
					return CO_SDO_AC_PDO_LEN;

				// Check whether the sub-object exists and can
				// be mapped into a PDO (or is a valid dummy
				// entry).
				// clang-format off
				if ((ac = co_dev_chk_rpdo(
						pdo->dev, idx, subidx)))
					// clang-format on
					return ac;
			}
#if LELY_NO_CO_MPDO
		} else {
#else
		} else if (n != CO_PDO_MAP_SAM_MPDO
				&& n != CO_PDO_MAP_DAM_MPDO) {
#endif
			return CO_SDO_AC_PARAM_VAL;
		}

		pdo->map.n = n;
	} else {
		assert(type == CO_DEFTYPE_UNSIGNED32);
		co_unsigned32_t map = val.u32;
		co_unsigned32_t map_old = co_sub_get_val_u32(sub);
		if (map == map_old)
			return 0;

		// The PDO mapping cannot be changed when the PDO is valid or
		// sub-index 0x00 is non-zero.
		if (valid || pdo->map.n)
			return CO_SDO_AC_PARAM_VAL;

		if (map) {
			co_unsigned16_t idx = (map >> 16) & 0xffff;
			co_unsigned8_t subidx = (map >> 8) & 0xff;
			// Check whether the sub-object exists and can be mapped
			// into a PDO (or is a valid dummy entry).
			if ((ac = co_dev_chk_rpdo(pdo->dev, idx, subidx)))
				return ac;
		}

		pdo->map.map[co_sub_get_subidx(sub) - 1] = map;
	}

	co_sub_dn(sub, &val);

	return 0;
}

static int
co_rpdo_recv(const struct can_msg *msg, void *data)
{
	assert(msg);
	assert(!(msg->flags & CAN_FLAG_RTR));
#if !LELY_NO_CANFD
	assert(!(msg->flags & CAN_FLAG_EDL));
#endif

	co_rpdo_t *pdo = data;
	assert(pdo);

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

#if !defined(NDEBUG) && !LELY_NO_STDIO && !LELY_NO_DIAG
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
		} else if (!ac && pdo->map.n <= CO_PDO_NUM_MAPS) {
			size_t offset = 0;
			for (size_t i = 0; i < pdo->map.n; i++)
				offset += (pdo->map.map[i]) & 0xff;
			if ((offset + 7) / 8 < n)
				// Generate an error message if the PDO length
				// exceeds the mapping.
				pdo->err(pdo, 0x8220, 0x10, pdo->err_data);
#if !LELY_NO_CO_MPDO
		} else if ((ac == CO_SDO_AC_NO_OBJ || ac == CO_SDO_AC_NO_SUB)
				&& pdo->map.n == 0xff) {
			// In case of a DAM-MPDO, generate an error message if
			// the destination object does not exist.
			pdo->err(pdo, 0x8230, 0x10, pdo->err_data);
#endif
		}
	}

	return ac;
}

#endif // !LELY_NO_CO_RPDO
