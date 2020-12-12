/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the synchronization (SYNC) object functions.
 *
 * @see lely/co/sync.h
 *
 * @copyright 2017-2020 Lely Industries N.V.
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

#if !LELY_NO_CO_SYNC

#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/sdo.h>
#include <lely/co/sync.h>
#include <lely/co/val.h>
#include <lely/util/errnum.h>
#include <lely/util/time.h>

#include <assert.h>
#include <stdlib.h>

/// A CANopen SYNC producer/consumer service.
struct __co_sync {
	/// A pointer to a CAN network interface.
	can_net_t *net;
	/// A pointer to a CANopen device.
	co_dev_t *dev;
	/// A flag specifying whether the SYNC service is stopped.
	int stopped;
	/// The SYNC COB-ID.
	co_unsigned32_t cobid;
	/// The communication cycle period (in microseconds).
	co_unsigned32_t us;
	/// The synchronous counter overflow value.
	co_unsigned8_t max_cnt;
	/// A pointer to the CAN frame receiver.
	can_recv_t *recv;
	/// A pointer to the CAN timer.
	can_timer_t *timer;
	/// The counter value.
	co_unsigned8_t cnt;
	/// A pointer to the indication function.
	co_sync_ind_t *ind;
	/// A pointer to user-specified data for #ind.
	void *ind_data;
	/// A pointer to the error handling function.
	co_sync_err_t *err;
	/// A pointer to user-specified data for #err.
	void *err_data;
};

/**
 * Updates and (de)activates a SYNC producer/consumer service. This function is
 * invoked by the download indication functions when one of the SYNC
 * configuration objects (1005, 1006 or 1019) is updated.
 */
static void co_sync_update(co_sync_t *sync);

/**
 * The download indication function for (all sub-objects of) CANopen object 1005
 * (COB-ID SYNC message).
 *
 * @see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1005_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);

/**
 * The download indication function for (all sub-objects of) CANopen object 1006
 * (Communication cycle period).
 *
 * @see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1006_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);

/**
 * The download indication function for (all sub-objects of) CANopen object 1019
 * (Synchronous counter overflow value).
 *
 * @see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1019_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);

/**
 * The CAN receive callback function for a SYNC consumer service.
 *
 * @see can_recv_func_t
 */
static int co_sync_recv(const struct can_msg *msg, void *data);

/**
 * The CAN timer callback function for a SYNC producer service.
 *
 * @see can_timer_func_t
 */
static int co_sync_timer(const struct timespec *tp, void *data);

void *
__co_sync_alloc(void)
{
	void *ptr = malloc(sizeof(struct __co_sync));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
__co_sync_free(void *ptr)
{
	free(ptr);
}

struct __co_sync *
__co_sync_init(struct __co_sync *sync, can_net_t *net, co_dev_t *dev)
{
	assert(sync);
	assert(net);
	assert(dev);

	int errc = 0;

	sync->net = net;
	sync->dev = dev;

	sync->stopped = 1;

	// Retrieve the SYNC COB-ID.
	co_obj_t *obj_1005 = co_dev_find_obj(sync->dev, 0x1005);
	if (!obj_1005) {
		errc = errnum2c(ERRNUM_NOSYS);
		goto error_obj_1005;
	}

	sync->cobid = 0;
	sync->us = 0;
	sync->max_cnt = 0;

	sync->recv = can_recv_create();
	if (!sync->recv) {
		errc = get_errc();
		goto error_create_recv;
	}
	can_recv_set_func(sync->recv, &co_sync_recv, sync);

	sync->timer = can_timer_create();
	if (!sync->timer) {
		errc = get_errc();
		goto error_create_timer;
	}
	can_timer_set_func(sync->timer, &co_sync_timer, sync);

	sync->cnt = 1;

	sync->ind = NULL;
	sync->ind_data = NULL;
	sync->err = NULL;
	sync->err_data = NULL;

	if (co_sync_start(sync) == -1) {
		errc = get_errc();
		goto error_start;
	}

	return sync;

	// co_sync_stop(sync);
error_start:
	can_timer_destroy(sync->timer);
error_create_timer:
	can_recv_destroy(sync->recv);
error_create_recv:
	co_obj_set_dn_ind(obj_1005, NULL, NULL);
error_obj_1005:
	set_errc(errc);
	return NULL;
}

void
__co_sync_fini(struct __co_sync *sync)
{
	assert(sync);

	co_sync_stop(sync);

	can_timer_destroy(sync->timer);
	can_recv_destroy(sync->recv);
}

co_sync_t *
co_sync_create(can_net_t *net, co_dev_t *dev)
{
	trace("creating SYNC service");

	int errc = 0;

	co_sync_t *sync = __co_sync_alloc();
	if (!sync) {
		errc = get_errc();
		goto error_alloc_sync;
	}

	if (!__co_sync_init(sync, net, dev)) {
		errc = get_errc();
		goto error_init_sync;
	}

	return sync;

error_init_sync:
	__co_sync_free(sync);
error_alloc_sync:
	set_errc(errc);
	return NULL;
}

void
co_sync_destroy(co_sync_t *sync)
{
	if (sync) {
		trace("destroying SYNC service");
		__co_sync_fini(sync);
		__co_sync_free(sync);
	}
}

int
co_sync_start(co_sync_t *sync)
{
	assert(sync);

	if (!sync->stopped)
		return 0;

	co_obj_t *obj_1005 = co_dev_find_obj(sync->dev, 0x1005);
	// Retrieve the COB-ID.
	sync->cobid = co_obj_get_val_u32(obj_1005, 0x00);
	// Set the download indication function for the SYNC COB-ID object.
	co_obj_set_dn_ind(obj_1005, &co_1005_dn_ind, sync);

	co_obj_t *obj_1006 = co_dev_find_obj(sync->dev, 0x1006);
	// Retrieve the communication cycle period (in microseconds).
	sync->us = co_obj_get_val_u32(obj_1006, 0x00);
	// Set the download indication function for the communication cycle
	// period object.
	if (obj_1006)
		co_obj_set_dn_ind(obj_1006, &co_1006_dn_ind, sync);

	co_obj_t *obj_1019 = co_dev_find_obj(sync->dev, 0x1019);
	// Retrieve the synchronous counter overflow value.
	sync->max_cnt = co_obj_get_val_u8(obj_1019, 0x00);
	// Set the download indication function for the synchronous counter
	// overflow value object.
	if (obj_1019)
		co_obj_set_dn_ind(obj_1019, &co_1019_dn_ind, sync);

	co_sync_update(sync);

	sync->stopped = 0;

	return 0;
}

void
co_sync_stop(co_sync_t *sync)
{
	assert(sync);

	if (sync->stopped)
		return;

	// Remove the download indication function for the synchronous counter
	// overflow value object.
	co_obj_t *obj_1019 = co_dev_find_obj(sync->dev, 0x1019);
	if (obj_1019)
		co_obj_set_dn_ind(obj_1019, NULL, NULL);

	// Remove the download indication function for the communication cycle
	// period object.
	co_obj_t *obj_1006 = co_dev_find_obj(sync->dev, 0x1006);
	if (obj_1006)
		co_obj_set_dn_ind(obj_1006, NULL, NULL);

	// Remove the download indication function for the SYNC COB-ID object.
	co_obj_t *obj_1005 = co_dev_find_obj(sync->dev, 0x1005);
	assert(obj_1005);
	co_obj_set_dn_ind(obj_1005, NULL, NULL);

	sync->stopped = 1;
}

int
co_sync_is_stopped(const co_sync_t *sync)
{
	assert(sync);

	return sync->stopped;
}

can_net_t *
co_sync_get_net(const co_sync_t *sync)
{
	assert(sync);

	return sync->net;
}

co_dev_t *
co_sync_get_dev(const co_sync_t *sync)
{
	assert(sync);

	return sync->dev;
}

void
co_sync_get_ind(const co_sync_t *sync, co_sync_ind_t **pind, void **pdata)
{
	assert(sync);

	if (pind)
		*pind = sync->ind;
	if (pdata)
		*pdata = sync->ind_data;
}

void
co_sync_set_ind(co_sync_t *sync, co_sync_ind_t *ind, void *data)
{
	assert(sync);

	sync->ind = ind;
	sync->ind_data = data;
}

void
co_sync_get_err(const co_sync_t *sync, co_sync_err_t **perr, void **pdata)
{
	assert(sync);

	if (perr)
		*perr = sync->err;
	if (pdata)
		*pdata = sync->err_data;
}

void
co_sync_set_err(co_sync_t *sync, co_sync_err_t *err, void *data)
{
	assert(sync);

	sync->err = err;
	sync->err_data = data;
}

static void
co_sync_update(co_sync_t *sync)
{
	assert(sync);

	if (!(sync->cobid & CO_SYNC_COBID_PRODUCER)) {
		// Register the receiver under the specified CAN-ID.
		uint_least32_t id = sync->cobid;
		uint_least8_t flags = 0;
		if (id & CO_SYNC_COBID_FRAME) {
			id &= CAN_MASK_EID;
			flags |= CAN_FLAG_IDE;
		} else {
			id &= CAN_MASK_BID;
		}
		can_recv_start(sync->recv, sync->net, id, flags);
	} else {
		// Stop the receiver if we are a producer, to prevent receiving
		// our own SYNC messages.
		can_recv_stop(sync->recv);
	}

	if ((sync->cobid & CO_SYNC_COBID_PRODUCER) && sync->us) {
		// Start SYNC transmission at the next multiple of the SYNC
		// period.
		struct timespec start = { 0, 0 };
		can_net_get_time(sync->net, &start);
		int_least64_t nsec = start.tv_sec * INT64_C(1000000000)
				+ start.tv_nsec;
		nsec %= (uint_least64_t)sync->us * 1000;
		timespec_sub_nsec(&start, nsec);
		timespec_add_usec(&start, sync->us);
		struct timespec interval = { 0, 0 };
		timespec_add_usec(&interval, sync->us);
		can_timer_start(sync->timer, sync->net, &start, &interval);
	} else {
		// Stop the SYNC timer unless we are an active SYNC producer
		// (with a non-zero communication cycle period).
		can_timer_stop(sync->timer);
	}

	sync->cnt = 1;
}

static co_unsigned32_t
co_1005_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1005);
	assert(req);
	co_sync_t *sync = data;
	assert(sync);

	co_unsigned16_t type = co_sub_get_type(sub);
	assert(!co_type_is_array(type));

	union co_val val;
	co_unsigned32_t ac = 0;
	if (co_sdo_req_dn_val(req, type, &val, &ac) == -1)
		return ac;

	if (co_sub_get_subidx(sub))
		return CO_SDO_AC_NO_SUB;

	assert(type == CO_DEFTYPE_UNSIGNED32);
	co_unsigned32_t cobid = val.u32;
	co_unsigned32_t cobid_old = co_sub_get_val_u32(sub);
	if (cobid == cobid_old)
		return 0;

	// The CAN-ID cannot be changed while the producer is and remains
	// active.
	int active = cobid & CO_SYNC_COBID_PRODUCER;
	int active_old = cobid_old & CO_SYNC_COBID_PRODUCER;
	uint_least32_t canid = cobid & CAN_MASK_EID;
	uint_least32_t canid_old = cobid_old & CAN_MASK_EID;
	if (active && active_old && canid != canid_old)
		return CO_SDO_AC_PARAM_VAL;

	// A 29-bit CAN-ID is only valid if the frame bit is set.
	if (!(cobid & CO_SYNC_COBID_FRAME)
			&& (cobid & (CAN_MASK_EID ^ CAN_MASK_BID)))
		return CO_SDO_AC_PARAM_VAL;

	sync->cobid = cobid;

	co_sub_dn(sub, &val);

	co_sync_update(sync);
	return 0;
}

static co_unsigned32_t
co_1006_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1006);
	assert(req);
	co_sync_t *sync = data;
	assert(sync);

	co_unsigned16_t type = co_sub_get_type(sub);
	assert(!co_type_is_array(type));

	union co_val val;
	co_unsigned32_t ac = 0;
	if (co_sdo_req_dn_val(req, type, &val, &ac) == -1)
		return ac;

	if (co_sub_get_subidx(sub))
		return CO_SDO_AC_NO_SUB;

	assert(type == CO_DEFTYPE_UNSIGNED32);
	co_unsigned32_t us = val.u32;
	co_unsigned32_t us_old = co_sub_get_val_u32(sub);
	if (us == us_old)
		return 0;

	sync->us = us;

	co_sub_dn(sub, &val);

	co_sync_update(sync);
	return 0;
}

static co_unsigned32_t
co_1019_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1019);
	assert(req);
	co_sync_t *sync = data;
	assert(sync);

	co_unsigned16_t type = co_sub_get_type(sub);
	assert(!co_type_is_array(type));

	union co_val val;
	co_unsigned32_t ac = 0;
	if (co_sdo_req_dn_val(req, type, &val, &ac) == -1)
		return ac;

	if (co_sub_get_subidx(sub))
		return CO_SDO_AC_NO_SUB;

	assert(type == CO_DEFTYPE_UNSIGNED8);
	co_unsigned8_t max_cnt = val.u8;
	co_unsigned8_t max_cnt_old = co_sub_get_val_u8(sub);
	if (max_cnt == max_cnt_old)
		return 0;

	// The synchronous counter overflow value cannot be changed while the
	// communication cycle period is non-zero.
	if (sync->us)
		return CO_SDO_AC_DATA_DEV;

	if (max_cnt == 1 || max_cnt > 240)
		return CO_SDO_AC_PARAM_VAL;

	sync->max_cnt = max_cnt;

	co_sub_dn(sub, &val);

	co_sync_update(sync);
	return 0;
}

static int
co_sync_recv(const struct can_msg *msg, void *data)
{
	assert(msg);
	assert(!(msg->flags & CAN_FLAG_RTR));
#if !LELY_NO_CANFD
	assert(!(msg->flags & CAN_FLAG_EDL));
#endif
	co_sync_t *sync = data;
	assert(sync);

	co_unsigned8_t len = sync->max_cnt ? 1 : 0;
	if (msg->len != len && sync->err)
		sync->err(sync, 0x8240, 0x10, sync->err_data);

	co_unsigned8_t cnt = len && msg->len == len ? msg->data[0] : 0;
	if (sync->ind)
		sync->ind(sync, cnt, sync->ind_data);

	return 0;
}

static int
co_sync_timer(const struct timespec *tp, void *data)
{
	(void)tp;
	co_sync_t *sync = data;
	assert(sync);

	struct can_msg msg = CAN_MSG_INIT;
	msg.id = sync->cobid;
	if (sync->cobid & CO_SYNC_COBID_FRAME) {
		msg.id &= CAN_MASK_EID;
		msg.flags |= CAN_FLAG_IDE;
	} else {
		msg.id &= CAN_MASK_BID;
	}
	if (sync->max_cnt) {
		msg.len = 1;
		msg.data[0] = sync->cnt;
		sync->cnt = sync->cnt < sync->max_cnt ? sync->cnt + 1 : 1;
	}
	can_net_send(sync->net, &msg);

	co_unsigned8_t cnt = msg.len ? msg.data[0] : 0;
	if (sync->ind)
		sync->ind(sync, cnt, sync->ind_data);

	return 0;
}

#endif // !LELY_NO_CO_SYNC
