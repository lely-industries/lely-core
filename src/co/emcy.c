/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the emergency (EMCY) object functions.
 *
 * @see lely/co/emcy.h
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

#if !LELY_NO_CO_EMCY

#include <lely/can/buf.h>
#include <lely/co/dev.h>
#include <lely/co/emcy.h>
#include <lely/co/obj.h>
#include <lely/co/sdo.h>
#include <lely/co/val.h>
#include <lely/util/diag.h>
#include <lely/util/endian.h>
#include <lely/util/time.h>

#include <assert.h>
#include <stdlib.h>
#if LELY_NO_MALLOC
#include <string.h>
#endif

#if LELY_NO_MALLOC
#ifndef CO_EMCY_CAN_BUF_SIZE
/**
 * The default size of an EMCY CAN frame buffer in the absence of dynamic memory
 * allocation.
 */
#define CO_EMCY_CAN_BUF_SIZE 16
#endif
#ifndef CO_EMCY_MAX_NMSG
/**
 * The default maximum number of active EMCY messages in the absence of dynamic
 * memory allocation.
 */
#define CO_EMCY_MAX_NMSG 8
#endif
#endif // LELY_NO_MALLOC

/// An EMCY message.
struct co_emcy_msg {
	/// The emergency error code.
	co_unsigned16_t eec;
	/// The error register.
	co_unsigned8_t er;
};

/// A remote CANopen EMCY producer node.
struct co_emcy_node {
	/// The node-ID.
	co_unsigned8_t id;
	/// A pointer to the CAN frame receiver.
	can_recv_t *recv;
};

/**
 * The CAN receive callback function for a remote CANopen EMCY producer node.
 *
 * @see can_recv_func_t
 */
static int co_emcy_node_recv(const struct can_msg *msg, void *data);

/// A CANopen EMCY producer/consumer service.
struct __co_emcy {
	/// A pointer to a CAN network interface.
	can_net_t *net;
	/// A pointer to a CANopen device.
	co_dev_t *dev;
	/// A flag specifying whether the EMCY service is stopped.
	int stopped;
	/// A pointer to the error register object.
	co_sub_t *sub_1001_00;
	/// A pointer to the pre-defined error field object.
	co_obj_t *obj_1003;
	/// The number of messages in #msgs.
	size_t nmsg;
	/// An array of EMCY messages. The first element is the most recent.
#if LELY_NO_MALLOC
	struct co_emcy_msg msgs[CO_EMCY_MAX_NMSG];
#else
	struct co_emcy_msg *msgs;
#endif
	/// The CAN frame buffer.
	struct can_buf buf;
#if LELY_NO_MALLOC
	/**
	 * The static memory buffer used by #buf in the absence of dynamic
	 * memory allocation.
	 */
	struct can_msg begin[CO_EMCY_CAN_BUF_SIZE];
#endif
	/// A pointer to the CAN timer.
	can_timer_t *timer;
	/// The time at which the next EMCY message may be sent.
	struct timespec inhibit;
	/// An array of pointers to remote nodes.
	struct co_emcy_node nodes[CO_NUM_NODES];
	/// A pointer to the indication function.
	co_emcy_ind_t *ind;
	/// A pointer to user-specified data for #ind.
	void *data;
};

/// The pre-defined error field.
struct co_1003 {
	/// Number of errors.
	co_unsigned8_t n;
	/// An array of standard error fields.
	co_unsigned32_t ef[0xfe];
};

/**
 * Sets the value of CANopen object 1003 (Pre-defined error field). This
 * function copies the list of active EMCY messages to the array at object 1003.
 *
 * @returns 0 on success, or -1 on error.
 */
static int co_emcy_set_1003(co_emcy_t *emcy);

/**
 * The download indication function for (all sub-objects of) CANopen object 1003
 * (Pre-defined error field).
 *
 * @see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1003_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);

/**
 * The download indication function for (all sub-objects of) CANopen object 1014
 * (COB-ID emergency message).
 *
 * @see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1014_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);

/**
 * Sets the value of CANopen object 1028 (Emergency consumer object).
 *
 * @param emcy  a pointer to an EMCY service.
 * @param id    the node-ID.
 * @param cobid the COB-ID of the EMCY object.
 */
static void co_emcy_set_1028(
		co_emcy_t *emcy, co_unsigned8_t id, co_unsigned32_t cobid);

/**
 * The download indication function for (all sub-objects of) CANopen object 1028
 * (Emergency consumer object).
 *
 * @see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1028_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);

/**
 * The CAN timer callback function for an EMCY service.
 *
 * @see can_timer_func_t
 */
static int co_emcy_timer(const struct timespec *tp, void *data);

/**
 * Adds an EMCY message to the CAN frame buffer and sends it if possible.
 *
 * @param emcy a pointer to an EMCY service.
 * @param eec  the emergency error code.
 * @param er   the error register.
 * @param msef the manufacturer-specific error code (can be NULL).
 *
 * @returns 0 on success, or -1 on error.
 */
static int co_emcy_send(co_emcy_t *emcy, co_unsigned16_t eec, co_unsigned8_t er,
		const co_unsigned8_t msef[5]);

/**
 * Sends any messages in the CAN frame buffer unless the inhibit time has not
 * yet elapsed, in which case it sets the CAN timer.
 */
static void co_emcy_flush(co_emcy_t *emcy);

void *
__co_emcy_alloc(void)
{
	void *ptr = malloc(sizeof(struct __co_emcy));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
__co_emcy_free(void *ptr)
{
	free(ptr);
}

struct __co_emcy *
__co_emcy_init(struct __co_emcy *emcy, can_net_t *net, co_dev_t *dev)
{
	assert(emcy);
	assert(net);
	assert(dev);

	int errc = 0;

	emcy->net = net;
	emcy->dev = dev;

	emcy->stopped = 1;

	emcy->sub_1001_00 = co_dev_find_sub(emcy->dev, 0x1001, 0x00);
	if (!emcy->sub_1001_00) {
		errc = errnum2c(ERRNUM_NOSYS);
		goto error_sub_1001_00;
	}
	emcy->obj_1003 = co_dev_find_obj(emcy->dev, 0x1003);

	emcy->nmsg = 0;
#if LELY_NO_MALLOC
	memset(emcy->msgs, 0, CO_EMCY_MAX_NMSG * sizeof(*emcy->msgs));
#else
	emcy->msgs = NULL;
#endif

	// Create a CAN frame buffer for pending EMCY messages that will be send
	// once the inhibit time has elapsed.
#if LELY_NO_MALLOC
	can_buf_init(&emcy->buf, emcy->begin, CO_EMCY_CAN_BUF_SIZE);
	memset(emcy->begin, 0, CO_EMCY_CAN_BUF_SIZE * sizeof(*emcy->begin));
#else
	can_buf_init(&emcy->buf, NULL, 0);
#endif

	emcy->timer = can_timer_create();
	if (!emcy->timer) {
		errc = get_errc();
		goto error_create_timer;
	}
	can_timer_set_func(emcy->timer, &co_emcy_timer, emcy);

	emcy->inhibit = (struct timespec){ 0, 0 };

	emcy->ind = NULL;
	emcy->data = NULL;

	for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++) {
		struct co_emcy_node *node = &emcy->nodes[id - 1];
		node->id = id;
		node->recv = NULL;
	}

	co_obj_t *obj_1028 = co_dev_find_obj(emcy->dev, 0x1028);
	if (obj_1028) {
		// Initialize the nodes.
		for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++) {
			co_sub_t *sub = co_obj_find_sub(obj_1028, id);
			if (!sub)
				continue;
			struct co_emcy_node *node = &emcy->nodes[id - 1];
			node->recv = can_recv_create();
			if (!node->recv) {
				errc = get_errc();
				goto error_create_recv;
			}
			can_recv_set_func(node->recv, &co_emcy_node_recv, node);
		}
	}

	if (co_emcy_start(emcy) == -1) {
		errc = get_errc();
		goto error_start;
	}

	return emcy;

	// co_emcy_stop(emcy);
error_start:
error_create_recv:
	for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++)
		can_recv_destroy(emcy->nodes[id - 1].recv);
	can_timer_destroy(emcy->timer);
error_create_timer:
	can_buf_fini(&emcy->buf);
error_sub_1001_00:
	set_errc(errc);
	return NULL;
}

void
__co_emcy_fini(struct __co_emcy *emcy)
{
	assert(emcy);

	co_emcy_stop(emcy);

	for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++)
		can_recv_destroy(emcy->nodes[id - 1].recv);

	can_timer_destroy(emcy->timer);

	can_buf_fini(&emcy->buf);

#if !LELY_NO_MALLOC
	free(emcy->msgs);
#endif
}

co_emcy_t *
co_emcy_create(can_net_t *net, co_dev_t *dev)
{
	trace("creating EMCY producer service");

	int errc = 0;

	co_emcy_t *emcy = __co_emcy_alloc();
	if (!emcy) {
		errc = get_errc();
		goto error_alloc_emcy;
	}

	if (!__co_emcy_init(emcy, net, dev)) {
		errc = get_errc();
		goto error_init_emcy;
	}

	return emcy;

error_init_emcy:
	__co_emcy_free(emcy);
error_alloc_emcy:
	set_errc(errc);
	return NULL;
}

void
co_emcy_destroy(co_emcy_t *emcy)
{
	if (emcy) {
		trace("destroying EMCY producer service");
		__co_emcy_fini(emcy);
		__co_emcy_free(emcy);
	}
}

int
co_emcy_start(co_emcy_t *emcy)
{
	assert(emcy);

	if (!emcy->stopped)
		return 0;

	can_net_get_time(emcy->net, &emcy->inhibit);

	// Set the download indication function for the pre-defined error field.
	if (emcy->obj_1003)
		co_obj_set_dn_ind(emcy->obj_1003, &co_1003_dn_ind, emcy);

	// Set the download indication function for the EMCY COB-ID object.
	co_obj_t *obj_1014 = co_dev_find_obj(emcy->dev, 0x1014);
	if (obj_1014)
		co_obj_set_dn_ind(obj_1014, &co_1014_dn_ind, emcy);

	co_obj_t *obj_1028 = co_dev_find_obj(emcy->dev, 0x1028);
	if (obj_1028) {
		// Set the download indication function for the emergency
		// consumer object.
		co_obj_set_dn_ind(obj_1028, &co_1028_dn_ind, emcy);
		// Start the CAN frame receivers for the nodes.
		co_unsigned8_t maxid = MIN(co_obj_get_val_u8(obj_1028, 0x00),
				CO_NUM_NODES);
		for (co_unsigned8_t id = 1; id <= maxid; id++) {
			co_sub_t *sub = co_obj_find_sub(obj_1028, id);
			if (sub)
				co_emcy_set_1028(emcy, id,
						co_sub_get_val_u32(sub));
		}
	}

	emcy->stopped = 0;

	return 0;
}

void
co_emcy_stop(co_emcy_t *emcy)
{
	assert(emcy);

	if (emcy->stopped)
		return;

	can_timer_stop(emcy->timer);

	co_obj_t *obj_1028 = co_dev_find_obj(emcy->dev, 0x1028);
	if (obj_1028) {
		// Stop all CAN frame receivers.
		for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++) {
			co_sub_t *sub = co_obj_find_sub(obj_1028, id);
			if (sub)
				can_recv_stop(emcy->nodes[id - 1].recv);
		}
		// Remove the download indication function for the emergency
		// consumer object.
		co_obj_set_dn_ind(obj_1028, NULL, NULL);
	}

	// Remove the download indication function for the EMCY COB-ID object.
	co_obj_t *obj_1014 = co_dev_find_obj(emcy->dev, 0x1014);
	if (obj_1014)
		co_obj_set_dn_ind(obj_1014, NULL, NULL);

	// Remove the download indication function for the pre-defined error
	// field.
	if (emcy->obj_1003)
		co_obj_set_dn_ind(emcy->obj_1003, NULL, NULL);

	emcy->stopped = 1;
}

int
co_emcy_is_stopped(const co_emcy_t *emcy)
{
	assert(emcy);

	return emcy->stopped;
}

can_net_t *
co_emcy_get_net(const co_emcy_t *emcy)
{
	assert(emcy);

	return emcy->net;
}

co_dev_t *
co_emcy_get_dev(const co_emcy_t *emcy)
{
	assert(emcy);

	return emcy->dev;
}

int
co_emcy_push(co_emcy_t *emcy, co_unsigned16_t eec, co_unsigned8_t er,
		const co_unsigned8_t msef[5])
{
	assert(emcy);

	if (!eec) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	// Bit 0 (generic error) shall be signaled at any error situation.
	er |= 0x01;

	if (msef)
		diag(DIAG_INFO, 0, "EMCY: %04X %02X %u %u %u %u %u", eec, er,
				msef[0], msef[1], msef[2], msef[3], msef[4]);
	else
		diag(DIAG_INFO, 0, "EMCY: %04X %02X", eec, er);

#if LELY_NO_MALLOC
	if (emcy->nmsg > CO_EMCY_MAX_NMSG - 1) {
		set_errnum(ERRNUM_NOMEM);
		return -1;
	}
#else
	// Make room on the stack.
	struct co_emcy_msg *msgs = realloc(emcy->msgs,
			(emcy->nmsg + 1) * sizeof(struct co_emcy_msg));
	if (!msgs) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return -1;
	}
	emcy->msgs = msgs;
#endif

	if (emcy->nmsg) {
		// Copy the current error register.
		er |= emcy->msgs[0].er;
		// Move the older messages.
		memmove(emcy->msgs + 1, emcy->msgs,
				emcy->nmsg * sizeof(struct co_emcy_msg));
	}
	emcy->nmsg++;
	// Push the error to the stack.
	emcy->msgs[0].eec = eec;
	emcy->msgs[0].er = er;

	// Update the pre-defined error field.
	if (emcy->obj_1003)
		co_emcy_set_1003(emcy);

	return co_emcy_send(emcy, eec, er, msef);
}

int
co_emcy_pop(co_emcy_t *emcy, co_unsigned16_t *peec, co_unsigned8_t *per)
{
	assert(emcy);

	co_emcy_peek(emcy, peec, per);

	if (!emcy->nmsg)
		return 0;

	// Move the older messages.
	if (--emcy->nmsg)
		memmove(emcy->msgs, emcy->msgs + 1,
				emcy->nmsg * sizeof(struct co_emcy_msg));

	// Update the pre-defined error field.
	if (emcy->obj_1003)
		co_emcy_set_1003(emcy);

	if (emcy->nmsg) {
		// Store the next error in the error register and the
		// manufacturer-specific field.
		co_unsigned8_t msef[5] = { 0 };
		stle_u16(msef, emcy->msgs[0].eec);
		return co_emcy_send(emcy, 0, emcy->msgs[0].er, msef);
	} else {
		return co_emcy_send(emcy, 0, 0, NULL);
	}
}

void
co_emcy_peek(const co_emcy_t *emcy, co_unsigned16_t *peec, co_unsigned8_t *per)
{
	assert(emcy);

	if (peec)
		*peec = emcy->nmsg ? emcy->msgs[0].eec : 0;
	if (per)
		*per = emcy->nmsg ? emcy->msgs[0].er : 0;
}

int
co_emcy_clear(co_emcy_t *emcy)
{
	assert(emcy);

	if (!emcy->nmsg)
		return 0;

	// Clear the stack.
	emcy->nmsg = 0;

	// Clear the pre-defined error field.
	if (emcy->obj_1003)
		co_emcy_set_1003(emcy);

	// Send the 'error reset/no error' message.
	return co_emcy_send(emcy, 0, 0, NULL);
}

void
co_emcy_get_ind(const co_emcy_t *emcy, co_emcy_ind_t **pind, void **pdata)
{
	assert(emcy);

	if (pind)
		*pind = emcy->ind;
	if (pdata)
		*pdata = emcy->data;
}

void
co_emcy_set_ind(co_emcy_t *emcy, co_emcy_ind_t *ind, void *data)
{
	assert(emcy);

	emcy->ind = ind;
	emcy->data = data;
}

static int
co_emcy_node_recv(const struct can_msg *msg, void *data)
{
	assert(msg);
	struct co_emcy_node *node = data;
	assert(node);
	assert(node->id > 0 && node->id <= CO_NUM_NODES);
	co_emcy_t *emcy = structof(node, co_emcy_t, nodes[node->id - 1]);
	assert(emcy);

	// Ignore remote frames.
	if (msg->flags & CAN_FLAG_RTR)
		return 0;

#if !LELY_NO_CANFD
	// Ignore CAN FD format frames.
	if (msg->flags & CAN_FLAG_EDL)
		return 0;
#endif

	// Extract the parameters from the frame.
	co_unsigned16_t eec = 0;
	if (msg->len >= 2)
		eec = ldle_u16(msg->data);
	co_unsigned8_t er = msg->len >= 3 ? msg->data[2] : 0;
	co_unsigned8_t msef[5] = { 0 };
	if (msg->len >= 4)
		memcpy(msef, msg->data + 3,
				MAX((uint_least8_t)(msg->len - 3), 5));

	// Notify the user.
	trace("EMCY: received %04X %02X", eec, er);
	if (emcy->ind)
		emcy->ind(emcy, node->id, eec, er, msef, emcy->data);

	return 0;
}

static int
co_emcy_set_1003(co_emcy_t *emcy)
{
	assert(emcy);
	assert(emcy->obj_1003);

	struct co_1003 *val_1003 = co_obj_addressof_val(emcy->obj_1003);
	co_unsigned8_t nsubidx = co_obj_get_subidx(emcy->obj_1003, 0, NULL);
	if (!nsubidx)
		return 0;

	// Copy the emergency error codes.
	val_1003->n = MIN((co_unsigned8_t)emcy->nmsg, nsubidx - 1);
	for (int i = 0; i < val_1003->n; i++)
		val_1003->ef[i] = emcy->msgs[i].eec;
	for (int i = val_1003->n; i < nsubidx - 1; i++)
		val_1003->ef[i] = 0;

	return 0;
}

static co_unsigned32_t
co_1003_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1003);
	assert(req);
	co_emcy_t *emcy = data;
	assert(emcy);

	co_unsigned16_t type = co_sub_get_type(sub);
	assert(!co_type_is_array(type));

	union co_val val;
	co_unsigned32_t ac = 0;
	if (co_sdo_req_dn_val(req, type, &val, &ac) == -1)
		return ac;

	if (co_sub_get_subidx(sub))
		return CO_SDO_AC_NO_WRITE;

	// Only the value 0 is allowed.
	assert(type == CO_DEFTYPE_UNSIGNED8);
	if (val.u8)
		return CO_SDO_AC_PARAM_VAL;

	emcy->nmsg = 0;

	co_sub_dn(sub, &val);

	co_emcy_set_1003(emcy);
	return 0;
}

static co_unsigned32_t
co_1014_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1014);
	assert(req);
	(void)data;

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

	// The CAN-ID cannot be changed when the EMCY is and remains valid.
	int valid = !(cobid & CO_EMCY_COBID_VALID);
	int valid_old = !(cobid_old & CO_EMCY_COBID_VALID);
	uint_least32_t canid = cobid & CAN_MASK_EID;
	uint_least32_t canid_old = cobid_old & CAN_MASK_EID;
	if (valid && valid_old && canid != canid_old)
		return CO_SDO_AC_PARAM_VAL;

	// A 29-bit CAN-ID is only valid if the frame bit is set.
	if (!(cobid & CO_EMCY_COBID_FRAME)
			&& (cobid & (CAN_MASK_EID ^ CAN_MASK_BID)))
		return CO_SDO_AC_PARAM_VAL;

	co_sub_dn(sub, &val);

	return 0;
}

static void
co_emcy_set_1028(co_emcy_t *emcy, co_unsigned8_t id, co_unsigned32_t cobid)
{
	assert(emcy);
	assert(id && id <= CO_NUM_NODES);
	struct co_emcy_node *node = &emcy->nodes[id - 1];

	if (!(cobid & CO_EMCY_COBID_VALID)) {
		// Register the receiver under the specified CAN-ID.
		uint_least32_t id = cobid;
		uint_least8_t flags = 0;
		if (id & CO_EMCY_COBID_FRAME) {
			id &= CAN_MASK_EID;
			flags |= CAN_FLAG_IDE;
		} else {
			id &= CAN_MASK_BID;
		}
		can_recv_start(node->recv, emcy->net, id, flags);
	} else {
		// Stop the receiver unless the EMCY COB-ID is valid.
		can_recv_stop(node->recv);
	}
}

static co_unsigned32_t
co_1028_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1028);
	assert(req);
	co_emcy_t *emcy = data;
	assert(emcy);

	co_unsigned16_t type = co_sub_get_type(sub);
	assert(!co_type_is_array(type));

	union co_val val;
	co_unsigned32_t ac = 0;
	if (co_sdo_req_dn_val(req, type, &val, &ac) == -1)
		return ac;

	co_unsigned8_t id = co_sub_get_subidx(sub);
	if (!id)
		return CO_SDO_AC_NO_WRITE;
	co_unsigned8_t maxid = MIN(co_obj_get_val_u8(co_sub_get_obj(sub), 0),
			CO_NUM_NODES);
	if (id > maxid)
		return CO_SDO_AC_NO_SUB;

	assert(type == CO_DEFTYPE_UNSIGNED32);
	co_unsigned32_t cobid = val.u32;
	co_unsigned32_t cobid_old = co_sub_get_val_u32(sub);
	if (cobid == cobid_old)
		return 0;

	// The CAN-ID cannot be changed when the EMCY is and remains valid.
	int valid = !(cobid & CO_EMCY_COBID_VALID);
	int valid_old = !(cobid_old & CO_EMCY_COBID_VALID);
	uint_least32_t canid = cobid & CAN_MASK_EID;
	uint_least32_t canid_old = cobid_old & CAN_MASK_EID;
	if (valid && valid_old && canid != canid_old)
		return CO_SDO_AC_PARAM_VAL;

	// A 29-bit CAN-ID is only valid if the frame bit is set.
	if (!(cobid & CO_EMCY_COBID_FRAME)
			&& (cobid & (CAN_MASK_EID ^ CAN_MASK_BID)))
		return CO_SDO_AC_PARAM_VAL;

	co_emcy_set_1028(emcy, id, cobid);

	co_sub_dn(sub, &val);

	return 0;
}

static int
co_emcy_timer(const struct timespec *tp, void *data)
{
	(void)tp;
	co_emcy_t *emcy = data;
	assert(emcy);

	co_emcy_flush(emcy);

	return 0;
}

static int
co_emcy_send(co_emcy_t *emcy, co_unsigned16_t eec, co_unsigned8_t er,
		const co_unsigned8_t msef[5])
{
	assert(emcy);

	// Update the error register in the object dictionary.
	co_sub_set_val_u8(emcy->sub_1001_00, er);

	// Check whether the EMCY COB-ID exists and is valid.
	co_obj_t *obj_1014 = co_dev_find_obj(emcy->dev, 0x1014);
	if (!obj_1014)
		return 0;
	co_unsigned32_t cobid = co_obj_get_val_u32(obj_1014, 0x00);
	if (cobid & CO_EMCY_COBID_VALID)
		return 0;

	// Create the frame.
	struct can_msg msg = CAN_MSG_INIT;
	msg.id = cobid;
	if (cobid & CO_EMCY_COBID_FRAME) {
		msg.id &= CAN_MASK_EID;
		msg.flags |= CAN_FLAG_IDE;
	} else {
		msg.id &= CAN_MASK_BID;
	}
	msg.len = CAN_MAX_LEN;
	stle_u32(msg.data, eec);
	msg.data[2] = er;
	if (msef) {
		memcpy(msg.data + 3, msef, 5);
	}

	// Add the frame to the buffer.
	if (!can_buf_write(&emcy->buf, &msg, 1)) {
		if (!can_buf_reserve(&emcy->buf, 1))
			return -1;
		can_buf_write(&emcy->buf, &msg, 1);
	}

	co_emcy_flush(emcy);
	return 0;
}

static void
co_emcy_flush(co_emcy_t *emcy)
{
	assert(emcy);

	co_unsigned16_t inhibit = co_dev_get_val_u16(emcy->dev, 0x1015, 0x00);

	struct timespec now = { 0, 0 };
	can_net_get_time(emcy->net, &now);

	while (can_buf_size(&emcy->buf)) {
		if (timespec_cmp(&now, &emcy->inhibit) < 0) {
			can_timer_start(emcy->timer, emcy->net, &emcy->inhibit,
					NULL);
			return;
		}
		// Update the inhibit time.
		emcy->inhibit = now;
		timespec_add_usec(&emcy->inhibit, inhibit * 100);
		// Send the frame.
		struct can_msg msg;
		if (can_buf_read(&emcy->buf, &msg, 1))
			can_net_send(emcy->net, &msg);
	}
}

#endif // !LELY_NO_CO_EMCY
