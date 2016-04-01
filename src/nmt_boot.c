/*!\file
 * This file is part of the CANopen library; it contains the implementation of
 * the NMT 'boot slave' functions.
 *
 * \see src/nmt_boot.h
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

#ifndef LELY_NO_CO_NMT_MASTER

#include <lely/util/errnum.h>
#include <lely/util/time.h>
#include <lely/co/csdo.h>
#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/val.h>
#include "nmt_boot.h"

#include <assert.h>
#include <stdlib.h>

//! The timeout (in milliseconds) before trying to boot the slave again.
#define LELY_CO_NMT_BOOT_WAIT_TIMEOUT	1000

//! The timeout (in milliseconds) after sending a node guarding RTR.
#define LELY_CO_NMT_BOOT_RTR_TIMEOUT	100

//! An opaque CANopen NMT 'boot slave' state type.
typedef const struct co_nmt_boot_state co_nmt_boot_state_t;

//! A CANopen NMT 'boot slave' state.
struct co_nmt_boot_state {
	//! A pointer to the function invoked when a new state is entered.
	co_nmt_boot_state_t *(*on_enter)(co_nmt_boot_t *boot);
	/*!
	 * A pointer to the transition function invoked when a timeout occurs.
	 *
	 * \param boot a pointer to a 'boot slave' service.
	 * \param tp   a pointer to the current time.
	 *
	 * \returns a pointer to the next state.
	 */
	co_nmt_boot_state_t *(*on_time)(co_nmt_boot_t *boot,
			const struct timespec *tp);
	/*!
	 * A pointer to the transition function invoked when a CAN frame has
	 * been received.
	 *
	 * \param boot a pointer to a 'boot slave' service.
	 * \param msg  a pointer to the received CAN frame.
	 *
	 * \returns a pointer to the next state.
	 */
	co_nmt_boot_state_t *(*on_recv)(co_nmt_boot_t *boot,
			const struct can_msg *msg);
	/*!
	 * A pointer to the transition function invoked when an SDO upload
	 * request completes.
	 *
	 * \param boot a pointer to a 'boot slave' service.
	 * \param ac   the SDO abort code.
	 * \param ptr  a pointer to the uploaded bytes.
	 * \param n    the number of bytes at \a ptr.
	 *
	 * \returns a pointer to the next state.
	 */
	co_nmt_boot_state_t *(*on_up_con)(co_nmt_boot_t *boot,
			co_unsigned32_t ac, const void *ptr, size_t n);
	/*!
	 * A pointer to the transition function invoked when the result of a
	 * user-implemented step is received.
	 *
	 * \param boot a pointer to a 'boot slave' service.
	 * \param res   the result of a callback to the application.
	 *
	 * \returns a pointer to the next state.
	 */
	co_nmt_boot_state_t *(*on_res)(co_nmt_boot_t *boot, int res);
	//! A pointer to the function invoked when the current state is left.
	void (*on_leave)(co_nmt_boot_t *boot);
};

//! A CANopen NMT 'boot slave' service.
struct __co_nmt_boot {
	//! A pointer to a CAN network interface.
	can_net_t *net;
	//! A pointer to a CANopen device.
	co_dev_t *dev;
	//! A pointer to an NMT master service.
	co_nmt_t *nmt;
	//! A pointer to the 'update software' indication function.
	co_nmt_req_ind_t *up_sw_ind;
	//! A pointer to user-specified data for #up_sw_ind.
	void *up_sw_data;
	//! A pointer to the 'update configuration' indication function.
	co_nmt_req_ind_t *up_cfg_ind;
	//! A pointer to user-specified data for #up_cfg_ind.
	void *up_cfg_data;
	//! A pointer to the current state.
	co_nmt_boot_state_t *state;
	//! A pointer to the CAN frame receiver.
	can_recv_t *recv;
	//! A pointer to the CAN timer.
	can_timer_t *timer;
	//! The Node-ID.
	co_unsigned8_t id;
	//! A pointer to the confirmation function.
	co_nmt_boot_ind_t *con;
	//! A pointer to user-specified data for #con.
	void *data;
	//! The state of the node (including the toggle bit).
	co_unsigned32_t st;
	//! The error status.
	char es;
	//! The time at which the 'boot slave' request was received.
	struct timespec start;
	//! A pointer to the Client-SDO used to read slave objects.
	co_csdo_t *sdo;
	//! The NMT slave assignment (object 1F81).
	co_unsigned32_t assignment;
	//! The consumer heartbeat time (in milliseconds).
	co_unsigned16_t ms;
};

/*!
 * The CAN receive callback function for a 'boot slave' service.
 *
 * \see can_recv_func_t
 */
static int co_nmt_boot_recv(const struct can_msg *msg, void *data);

/*!
 * The CAN timer callback function for a 'boot slave' service.
 *
 * \see can_timer_func_t
 */
static int co_nmt_boot_timer(const struct timespec *tp, void *data);

/*!
 * The CANopen SDO upload confirmation callback function for a 'boot slave'
 * service.
 *
 * \see co_csdo_up_con_t
 */
static void co_nmt_boot_up_con(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac, const void *ptr,
		size_t n, void *data);

/*!
 * Enters the specified state of a 'boot slave' service and invokes the exit and
 * entry functions.
 */
static void co_nmt_boot_enter(co_nmt_boot_t *boot, co_nmt_boot_state_t *next);

/*!
 * Invokes the 'timeout' transition function of the current state of a 'boot
 * slave' service.
 *
 * \param boot a pointer to a 'boot slave' service.
 * \param tp   a pointer to the current time.
 */
static inline void co_nmt_boot_emit_time(co_nmt_boot_t *boot,
		const struct timespec *tp);

/*!
 * Invokes the 'CAN frame received' transition function of the current state of
 * a 'boot slave' service.
 *
 * \param boot a pointer to a 'boot slave' service.
 * \param msg  a pointer to the received CAN msg.
 */
static inline void co_nmt_boot_emit_recv(co_nmt_boot_t *boot,
		const struct can_msg *msg);

/*!
 * Invokes the 'SDO upload confirmation' transition function of the current
 * state of a 'boot slave' service.
 *
 * \param boot  a pointer to a 'boot slave' service.
 * \param ac   the SDO abort code.
 * \param ptr  a pointer to the uploaded bytes.
 * \param n    the number of bytes at \a ptr.
 */
static inline void co_nmt_boot_emit_up_con(co_nmt_boot_t *boot,
		co_unsigned32_t ac, const void *ptr, size_t n);

/*!
 * Invokes the 'result received' transition function of the current state of a
 * 'boot slave' service.
 *
 * \param boot  a pointer to a 'boot slave' service.
 * \param res   the result of a callback to the application.
 */
static inline void co_nmt_boot_emit_res(co_nmt_boot_t *boot, int res);

//! The 'timeout' transition function of the 'wait asynchronously' state.
static co_nmt_boot_state_t *co_nmt_boot_wait_on_time(co_nmt_boot_t *boot,
		const struct timespec *tp);

//! The 'wait asynchronously' state.
static const co_nmt_boot_state_t *co_nmt_boot_wait_state =
		&(co_nmt_boot_state_t){
	NULL, &co_nmt_boot_wait_on_time, NULL, NULL, NULL, NULL
};

//! The entry function of the 'abort' state.
static co_nmt_boot_state_t *co_nmt_boot_abort_on_enter(co_nmt_boot_t *boot);

//! The 'abort' state.
static const co_nmt_boot_state_t *co_nmt_boot_abort_state =
		&(co_nmt_boot_state_t){
	&co_nmt_boot_abort_on_enter, NULL, NULL, NULL, NULL, NULL
};

//! The entry function of the 'error' state.
static co_nmt_boot_state_t *co_nmt_boot_error_on_enter(co_nmt_boot_t *boot);

//! The exit function of the 'error' state.
static void co_nmt_boot_error_on_leave(co_nmt_boot_t *boot);

//! The 'error' state.
static const co_nmt_boot_state_t *co_nmt_boot_error_state =
		&(co_nmt_boot_state_t){
	&co_nmt_boot_error_on_enter,
	NULL,
	NULL,
	NULL,
	NULL,
	&co_nmt_boot_error_on_leave
};

//! The entry function of the 'check device type' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_device_type_on_enter(
		co_nmt_boot_t *boot);

/*!
 * The 'SDO upload confirmation' transition function of the 'check device type'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_device_type_on_up_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac, const void *ptr,
		size_t n);

//! The 'check device type' state (see Fig. 5 in CiA 302-2 version 4.1.0).
static const co_nmt_boot_state_t *co_nmt_boot_chk_device_type_state =
		&(co_nmt_boot_state_t){
	&co_nmt_boot_chk_device_type_on_enter,
	NULL,
	NULL,
	&co_nmt_boot_chk_device_type_on_up_con,
	NULL,
	NULL
};

//! The entry function of the 'check vendor ID' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_vendor_id_on_enter(
		co_nmt_boot_t *boot);

/*!
 * The 'SDO upload confirmation' transition function of the 'check vendor ID'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_vendor_id_on_up_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac, const void *ptr,
		size_t n);

//! The 'check vendor ID' state (see Fig. 5 in CiA 302-2 version 4.1.0).
static const co_nmt_boot_state_t *co_nmt_boot_chk_vendor_id_state =
		&(co_nmt_boot_state_t){
	&co_nmt_boot_chk_vendor_id_on_enter,
	NULL,
	NULL,
	&co_nmt_boot_chk_vendor_id_on_up_con,
	NULL,
	NULL
};

//! The entry function of the 'check product code' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_product_code_on_enter(
		co_nmt_boot_t *boot);

/*!
 * The 'SDO upload confirmation' transition function of the 'check product code'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_product_code_on_up_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac, const void *ptr,
		size_t n);

//! The 'check product code' state (see Fig. 5 in CiA 302-2 version 4.1.0).
static const co_nmt_boot_state_t *co_nmt_boot_chk_product_code_state =
		&(co_nmt_boot_state_t){
	&co_nmt_boot_chk_product_code_on_enter,
	NULL,
	NULL,
	&co_nmt_boot_chk_product_code_on_up_con,
	NULL,
	NULL
};

//! The entry function of the 'check revision number' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_revision_on_enter(
		co_nmt_boot_t *boot);

/*!
 * The 'SDO upload confirmation' transition function of the 'check revision
 * number' state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_revision_on_up_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac, const void *ptr,
		size_t n);

//! The 'check revision number' state (see Fig. 5 in CiA 302-2 version 4.1.0).
static const co_nmt_boot_state_t *co_nmt_boot_chk_revision_state =
		&(co_nmt_boot_state_t){
	&co_nmt_boot_chk_revision_on_enter,
	NULL,
	NULL,
	&co_nmt_boot_chk_revision_on_up_con,
	NULL,
	NULL
};

//! The entry function of the 'check serial number' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_serial_nr_on_enter(
		co_nmt_boot_t *boot);

/*!
 * The 'SDO upload confirmation' transition function of the 'check serial
 * number' state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_serial_nr_on_up_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac, const void *ptr,
		size_t n);

//! The 'check serial number' state (see Fig. 5 in CiA 302-2 version 4.1.0).
static const co_nmt_boot_state_t *co_nmt_boot_chk_serial_nr_state =
		&(co_nmt_boot_state_t){
	&co_nmt_boot_chk_serial_nr_on_enter,
	NULL,
	NULL,
	&co_nmt_boot_chk_serial_nr_on_up_con,
	NULL,
	NULL
};

//! The entry function of the 'check node state' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_node_on_enter(co_nmt_boot_t *boot);

//! The 'timeout' transition function of the 'check node state' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_node_on_time(co_nmt_boot_t *boot,
		const struct timespec *tp);

/*!
 * The 'CAN frame received' transition function of the 'check node state' state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_node_on_recv(co_nmt_boot_t *boot,
		const struct can_msg *msg);

//! The 'check node state' state (see Fig. 6 in CiA 302-2 version 4.1.0).
static const co_nmt_boot_state_t *co_nmt_boot_chk_node_state =
		&(co_nmt_boot_state_t){
	&co_nmt_boot_chk_node_on_enter,
	&co_nmt_boot_chk_node_on_time,
	&co_nmt_boot_chk_node_on_recv,
	NULL,
	NULL,
	NULL
};

//! The entry function of the 'check software' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_sw_on_enter(co_nmt_boot_t *boot);

/*!
 * The 'SDO upload confirmation' transition function of the 'check software'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_sw_on_up_con(co_nmt_boot_t *boot,
		co_unsigned32_t ac, const void *ptr, size_t n);

//! The 'check software' state (see Fig. 6 in CiA 302-2 version 4.1.0).
static const co_nmt_boot_state_t *co_nmt_boot_chk_sw_state =
		&(co_nmt_boot_state_t){
	&co_nmt_boot_chk_sw_on_enter,
	NULL,
	NULL,
	&co_nmt_boot_chk_sw_on_up_con,
	NULL,
	NULL
};

//! The entry function of the 'update software request' state.
static co_nmt_boot_state_t *co_nmt_boot_up_sw_req_on_enter(co_nmt_boot_t *boot);

//! The exit function of the 'update software request' state.
static void co_nmt_boot_up_sw_req_on_leave(co_nmt_boot_t *boot);

//! The 'update software request' state (see Fig. 3 in CiA 302-3 version 4.1.0).
static const co_nmt_boot_state_t *co_nmt_boot_up_sw_req_state =
		&(co_nmt_boot_state_t){
	&co_nmt_boot_up_sw_req_on_enter,
	NULL,
	NULL,
	NULL,
	NULL,
	&co_nmt_boot_up_sw_req_on_leave
};

/*!
 * The 'result received' transition function of the 'update software result'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_up_sw_res_on_res(co_nmt_boot_t *boot,
		int res);

//! The 'update software result' state (see Fig. 3 in CiA 302-3 version 4.1.0).
static const co_nmt_boot_state_t *co_nmt_boot_up_sw_res_state =
		&(co_nmt_boot_state_t){
	NULL, NULL, NULL, NULL, &co_nmt_boot_up_sw_res_on_res, NULL
};

//! The entry function of the 'check configuration date' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_cfg_date_on_enter(
		co_nmt_boot_t *boot);

/*!
 * The 'SDO upload confirmation' transition function of the 'check configuration
 * date' state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_cfg_date_on_up_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac, const void *ptr,
		size_t n);

/*!
 * The 'check configuration date' state (see Fig. 8 in CiA 302-2 version 4.1.0).
 */
static const co_nmt_boot_state_t *co_nmt_boot_chk_cfg_date_state =
		&(co_nmt_boot_state_t){
	&co_nmt_boot_chk_cfg_date_on_enter,
	NULL,
	NULL,
	&co_nmt_boot_chk_cfg_date_on_up_con,
	NULL,
	NULL
};

/*!
 * The 'SDO upload confirmation' transition function of the 'check configuration
 * time' state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_cfg_time_on_up_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac, const void *ptr,
		size_t n);

/*!
 * The 'check configuration time' state (see Fig. 8 in CiA 302-2 version 4.1.0).
 */
static const co_nmt_boot_state_t *co_nmt_boot_chk_cfg_time_state =
		&(co_nmt_boot_state_t){
	NULL, NULL, NULL, &co_nmt_boot_chk_cfg_time_on_up_con, NULL, NULL
};

//! The entry function of the 'update configuration' state.
static co_nmt_boot_state_t *co_nmt_boot_up_cfg_on_enter(co_nmt_boot_t *boot);

/*!
 * The 'update configuration' state (see Fig. 8 in CiA 302-2 version 4.1.0).
 */
static const co_nmt_boot_state_t *co_nmt_boot_up_cfg_state =
		&(co_nmt_boot_state_t){
	&co_nmt_boot_up_cfg_on_enter, NULL, NULL, NULL, NULL, NULL
};

//! The entry function of the 'update configuration request' state.
static co_nmt_boot_state_t *co_nmt_boot_up_cfg_req_on_enter(
		co_nmt_boot_t *boot);

//! The exit function of the 'update configuration request' state.
static void co_nmt_boot_up_cfg_req_on_leave(co_nmt_boot_t *boot);

/*!
 * The 'update configuration request' state (see Fig. 8 in CiA 302-2 version
 * 4.1.0).
 */
static const co_nmt_boot_state_t *co_nmt_boot_up_cfg_req_state =
		&(co_nmt_boot_state_t){
	&co_nmt_boot_up_cfg_req_on_enter,
	NULL,
	NULL,
	NULL,
	NULL,
	&co_nmt_boot_up_cfg_req_on_leave
};

/*!
 * The 'result received' transition function of the 'update configuration
 * result' state.
 */
static co_nmt_boot_state_t *co_nmt_boot_up_cfg_res_on_res(co_nmt_boot_t *boot,
		int res);

/*!
 * The 'update configuration result' state (see Fig. 8 in CiA 302-2 version
 * 4.1.0).
 */
static const co_nmt_boot_state_t *co_nmt_boot_up_cfg_res_state =
		&(co_nmt_boot_state_t){
	NULL, NULL, NULL, NULL, &co_nmt_boot_up_cfg_res_on_res, NULL
};

//! The entry function of the 'start error control' state.
static co_nmt_boot_state_t *co_nmt_boot_ec_on_enter(co_nmt_boot_t *boot);

//! The 'timeout' transition function of the 'start error control' state.
static co_nmt_boot_state_t *co_nmt_boot_ec_on_time(co_nmt_boot_t *boot,
		const struct timespec *tp);

/*!
 * The 'CAN frame received' transition function of the 'start error control'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_ec_on_recv(co_nmt_boot_t *boot,
		const struct can_msg *msg);

//! The 'start error control' state (see Fig. 11 in CiA 302-2 version 4.1.0).
static const co_nmt_boot_state_t *co_nmt_boot_ec_state =
		&(co_nmt_boot_state_t){
	&co_nmt_boot_ec_on_enter,
	&co_nmt_boot_ec_on_time,
	&co_nmt_boot_ec_on_recv,
	NULL,
	NULL,
	NULL
};

/*!
 * Issues and an SDO upload request to the slave. co_nmt_boot_up_con() is called
 * upon completion of the request.
 *
 * \param boot   a pointer to a 'boot slave' service.
 * \param idx    the remote object index.
 * \param subidx the remote object sub-index.
 *
 * \returns 0 on success, or -1 on error.
 */
static int co_nmt_boot_up(co_nmt_boot_t *boot, co_unsigned16_t idx,
		co_unsigned8_t subidx);

/*!
 * Compares the result of an SDO upload request to the value of a local
 * sub-object.
 *
 * \param boot   a pointer to a 'boot slave' service.
 * \param idx    the local object index.
 * \param subidx the local object sub-index.
 * \param ac     the SDO abort code.
 * \param ptr    a pointer to the uploaded bytes.
 * \param n      the number of bytes at \a ptr.
 *
 * \returns 1 if the received value matches that of the specified sub-object,
 * and 0 if not.
 */
static int co_nmt_boot_chk(co_nmt_boot_t *boot, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac, const void *ptr,
		size_t n);

/*!
 * Sends a node guarding RTR to the slave.
 *
 * \returns 0 on success, or -1 on error.
 */
static int co_nmt_boot_send_rtr(co_nmt_boot_t *boot);

void *
__co_nmt_boot_alloc(void)
{
	return malloc(sizeof(struct __co_nmt_boot));
}

void
__co_nmt_boot_free(void *ptr)
{
	free(ptr);
}

struct __co_nmt_boot *
__co_nmt_boot_init(struct __co_nmt_boot *boot, can_net_t *net, co_dev_t *dev,
		co_nmt_t *nmt)
{
	assert(boot);
	assert(net);
	assert(dev);
	assert(nmt);

	errc_t errc = 0;

	boot->net = net;
	boot->dev = dev;
	boot->nmt = nmt;

	boot->up_sw_ind = NULL;
	boot->up_sw_data = NULL;

	boot->up_cfg_ind = NULL;
	boot->up_cfg_data = NULL;

	boot->state = co_nmt_boot_wait_state;

	boot->recv = can_recv_create();
	if (__unlikely(!boot->recv)) {
		errc = get_errc();
		goto error_create_recv;
	}
	can_recv_set_func(boot->recv, &co_nmt_boot_recv, boot);

	boot->timer = can_timer_create();
	if (__unlikely(!boot->timer)) {
		errc = get_errc();
		goto error_create_timer;
	}
	can_timer_set_func(boot->timer, &co_nmt_boot_timer, boot);

	boot->id = 0;

	boot->con = NULL;
	boot->data = NULL;

	boot->st = 0;
	boot->es = 0;

	boot->start = (struct timespec){ 0, 0 };

	boot->sdo = NULL;

	boot->assignment = 0;
	boot->ms = 0;

	return boot;

	can_timer_destroy(boot->timer);
error_create_timer:
	can_recv_destroy(boot->recv);
error_create_recv:
	set_errc(errc);
	return NULL;
}

void
__co_nmt_boot_fini(struct __co_nmt_boot *boot)
{
	assert(boot);

	co_csdo_destroy(boot->sdo);

	can_timer_destroy(boot->timer);
	can_recv_destroy(boot->recv);
}

co_nmt_boot_t *
co_nmt_boot_create(can_net_t *net, co_dev_t *dev, co_nmt_t *nmt)
{
	errc_t errc = 0;

	co_nmt_boot_t *boot = __co_nmt_boot_alloc();
	if (__unlikely(!boot)) {
		errc = get_errc();
		goto error_alloc_boot;
	}

	if (__unlikely(!__co_nmt_boot_init(boot, net, dev, nmt))) {
		errc = get_errc();
		goto error_init_boot;
	}

	return boot;

error_init_boot:
	__co_nmt_boot_free(boot);
error_alloc_boot:
	set_errc(errc);
	return NULL;
}

void
co_nmt_boot_destroy(co_nmt_boot_t *boot)
{
	if (boot) {
		__co_nmt_boot_fini(boot);
		__co_nmt_boot_free(boot);
	}
}

void
co_nmt_boot_get_up_sw_ind(co_nmt_boot_t *boot, co_nmt_req_ind_t **pind,
		void **pdata)
{
	assert(boot);

	if (pind)
		*pind = boot->up_sw_ind;
	if (pdata)
		*pdata = boot->up_sw_data;
}

void
co_nmt_boot_set_up_sw_ind(co_nmt_boot_t *boot, co_nmt_req_ind_t *ind,
		void *data)
{
	assert(boot);

	boot->up_sw_ind = ind;
	boot->up_sw_data = data;
}

void
co_nmt_boot_get_up_cfg_ind(co_nmt_boot_t *boot, co_nmt_req_ind_t **pind,
		void **pdata)
{
	assert(boot);

	if (pind)
		*pind = boot->up_cfg_ind;
	if (pdata)
		*pdata = boot->up_cfg_data;
}

void
co_nmt_boot_set_up_cfg_ind(co_nmt_boot_t *boot, co_nmt_req_ind_t *ind,
		void *data)
{
	assert(boot);

	boot->up_cfg_ind = ind;
	boot->up_cfg_data = data;
}

int
co_nmt_boot_boot_req(co_nmt_boot_t *boot, co_unsigned8_t id, int timeout,
		co_nmt_boot_ind_t *con, void *data)
{
	assert(boot);

	if (__unlikely(!id || id > CO_NUM_NODES)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	// Check whether we are in the waiting state.
	if (__unlikely(boot->state != co_nmt_boot_wait_state)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	boot->id = id;

	boot->con = con;
	boot->data = data;

	boot->st = 0;
	boot->es = 0;

	can_net_get_time(boot->net, &boot->start);

	co_csdo_destroy(boot->sdo);
	boot->sdo = co_csdo_create(boot->net, NULL, boot->id);
	if (__unlikely(!boot->sdo))
		return -1;
	co_csdo_set_timeout(boot->sdo, timeout);

	co_nmt_boot_emit_time(boot, NULL);

	return 0;
}

void
co_nmt_boot_req_res(co_nmt_boot_t *boot, int res)
{
	assert(boot);

	co_nmt_boot_emit_res(boot, res);
}

static int
co_nmt_boot_recv(const struct can_msg *msg, void *data)
{
	assert(msg);
	co_nmt_boot_t *boot = data;
	assert(boot);

	co_nmt_boot_emit_recv(boot, msg);

	return 0;
}

static int
co_nmt_boot_timer(const struct timespec *tp, void *data)
{
	assert(tp);
	co_nmt_boot_t *boot = data;
	assert(boot);

	co_nmt_boot_emit_time(boot, tp);

	return 0;
}

static void
co_nmt_boot_up_con(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned32_t ac, const void *ptr, size_t n, void *data)
{
	__unused_var(sdo);
	__unused_var(idx);
	__unused_var(subidx);
	co_nmt_boot_t *boot = data;
	assert(boot);

	co_nmt_boot_emit_up_con(boot, ac, ptr, n);
}

static void
co_nmt_boot_enter(co_nmt_boot_t *boot, co_nmt_boot_state_t *next)
{
	assert(boot);
	assert(boot->state);

	while (next) {
		co_nmt_boot_state_t *prev = boot->state;
		boot->state = next;

		if (prev->on_leave)
			prev->on_leave(boot);

		next = next->on_enter ? next->on_enter(boot) : NULL;
	}
}

static inline void
co_nmt_boot_emit_time(co_nmt_boot_t *boot, const struct timespec *tp)
{
	assert(boot);
	assert(boot->state);

	if (boot->state->on_time)
		co_nmt_boot_enter(boot, boot->state->on_time(boot, tp));
}

static inline void
co_nmt_boot_emit_recv(co_nmt_boot_t *boot, const struct can_msg *msg)
{
	assert(boot);
	assert(boot->state);

	if (boot->state->on_recv)
		co_nmt_boot_enter(boot, boot->state->on_recv(boot, msg));
}

static inline void
co_nmt_boot_emit_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);
	assert(boot->state);

	if (boot->state->on_up_con)
		co_nmt_boot_enter(boot,
				boot->state->on_up_con(boot, ac, ptr, n));
}

static inline void
co_nmt_boot_emit_res(co_nmt_boot_t *boot, int res)
{
	assert(boot);
	assert(boot->state);

	if (boot->state->on_res)
		co_nmt_boot_enter(boot, boot->state->on_res(boot, res));
}

static co_nmt_boot_state_t *
co_nmt_boot_wait_on_time(co_nmt_boot_t *boot, const struct timespec *tp)
{
	__unused_var(boot);
	__unused_var(tp);

	// Retrieve the slave assignment for the node.
	boot->assignment = co_dev_get_val_u32(boot->dev, 0x1f81, boot->id);

	// Find the consumer heartbeat time for the node.
	boot->ms = 0;
	co_obj_t *obj_1016 = co_dev_find_obj(boot->dev, 0x1016);
	if (obj_1016) {
		co_unsigned8_t n = co_obj_get_val_u8(obj_1016, 0x00);
		for (size_t i = 1; i <= n; i++) {
			co_unsigned32_t val = co_obj_get_val_u32(obj_1016, i);
			if (((val >> 16) & 0x7f) == boot->id)
				boot->ms = val & 0xffff;
		}
	}

	// Abort the 'boot slave' process if the slave is not in the network
	// list.
	if (!(boot->assignment & 0x01)) {
		boot->es = 'A';
		return co_nmt_boot_abort_state;
	}

	if (!(boot->assignment & 0x04))
		// Skip booting and start the error control service.
		return co_nmt_boot_ec_state;

	return co_nmt_boot_chk_device_type_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_abort_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	can_recv_stop(boot->recv);
	can_timer_stop(boot->timer);

	// If the node is already operational, end the 'boot slave' process with
	// error status L.
	if (!boot->es && (boot->st & ~CO_NMT_ST_TOGGLE) == CO_NMT_ST_START)
		boot->es = 'L';

	// Retry on error status B (see Fig. 4 in CiA 302-2 version 4.1.0).
	if (boot->es == 'B') {
		int wait = 0;
		if (boot->assignment & 0x08) {
			// Obtain the time (in milliseconds) the master will
			// wait for a mandatory slave to boot.
			co_unsigned32_t boot_time = co_dev_get_val_u32(
					boot->dev, 0x1f89, 0x00);
			// Check if this time has elapsed.
			if (boot_time) {
				struct timespec now = { 0, 0 };
				can_net_get_time(boot->net, &now);
				wait = timespec_diff_msec(&now, &boot->start)
						< boot_time;
			}
		}
		// If the slave is not mandatory, or the boot time has not yet
		// elapsed, wait asynchronously for a while and retry the 'boot
		// slave' process.
		if (wait) {
			can_timer_timeout(boot->timer, boot->net,
					LELY_CO_NMT_BOOT_WAIT_TIMEOUT);
			return co_nmt_boot_wait_state;
		}
	}

	return co_nmt_boot_error_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_error_on_enter(co_nmt_boot_t *boot)
{
	__unused_var(boot);

	return co_nmt_boot_wait_state;
}

static void
co_nmt_boot_error_on_leave(co_nmt_boot_t *boot)
{
	assert(boot);

	if (boot->con)
		boot->con(boot->nmt, boot->id, boot->st, boot->es, boot->data);
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_device_type_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	boot->es = 'B';

	// Read the device type of the slave (object 1000).
	if (__unlikely(co_nmt_boot_up(boot, 0x1000, 0x00) == -1))
		return co_nmt_boot_abort_state;

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_device_type_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

	if (__unlikely(ac))
		return co_nmt_boot_abort_state;

	// If the expected device type (sub-object 1F84:ID) is 0, skip the check
	// and proceed with the vendor ID.
	co_unsigned32_t device_type =
			co_dev_get_val_u32(boot->dev, 0x1f84, boot->id);
	if (__unlikely(device_type && !co_nmt_boot_chk(boot, 0x1f84, boot->id,
			ac, ptr, n))) {
		boot->es = 'C';
		return co_nmt_boot_abort_state;
	}

	return co_nmt_boot_chk_vendor_id_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_vendor_id_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	// If the expected vendor ID (sub-object 1F85:ID) is 0, skip the check
	// and proceed with the product code.
	co_unsigned32_t vendor_id =
			co_dev_get_val_u32(boot->dev, 0x1f85, boot->id);
	if (!vendor_id)
		return co_nmt_boot_chk_product_code_state;

	boot->es = 'D';

	// Read the vendor ID of the slave (sub-object 1018:01).
	if (__unlikely(co_nmt_boot_up(boot, 0x1018, 0x01) == -1))
		return co_nmt_boot_abort_state;

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_vendor_id_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
	const void *ptr, size_t n)
{
	assert(boot);

	if (__unlikely(!co_nmt_boot_chk(boot, 0x1f85, boot->id, ac, ptr, n)))
		return co_nmt_boot_abort_state;

	return co_nmt_boot_chk_product_code_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_product_code_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	// If the expected product code (sub-object 1F86:ID) is 0, skip the
	// check and proceed with the revision number.
	co_unsigned32_t product_code =
			co_dev_get_val_u32(boot->dev, 0x1f86, boot->id);
	if (!product_code)
		return co_nmt_boot_chk_revision_state;

	boot->es = 'M';

	// Read the product code of the slave (sub-object 1018:02).
	if (__unlikely(co_nmt_boot_up(boot, 0x1018, 0x02) == -1))
		return co_nmt_boot_abort_state;

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_product_code_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

	if (__unlikely(!co_nmt_boot_chk(boot, 0x1f86, boot->id, ac, ptr, n)))
		return co_nmt_boot_abort_state;

	return co_nmt_boot_chk_revision_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_revision_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	// If the expected revision number (sub-object 1F87:ID) is 0, skip the
	// check and proceed with the serial number.
	co_unsigned32_t revision =
			co_dev_get_val_u32(boot->dev, 0x1f87, boot->id);
	if (!revision)
		return co_nmt_boot_chk_serial_nr_state;

	boot->es = 'N';

	// Read the revision number of the slave (sub-object 1018:03).
	if (__unlikely(co_nmt_boot_up(boot, 0x1018, 0x03) == -1))
		return co_nmt_boot_abort_state;

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_revision_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

	if (__unlikely(!co_nmt_boot_chk(boot, 0x1f87, boot->id, ac, ptr, n)))
		return co_nmt_boot_abort_state;

	return co_nmt_boot_chk_serial_nr_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_serial_nr_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	// If the expected serial number (sub-object 1F88:ID) is 0, skip the
	// check and proceed to 'check node state'.
	co_unsigned32_t serial_nr =
			co_dev_get_val_u32(boot->dev, 0x1f88, boot->id);
	if (!serial_nr)
		return co_nmt_boot_chk_node_state;

	boot->es = 'O';

	// Read the serial number of the slave (sub-object 1018:04).
	if (__unlikely(co_nmt_boot_up(boot, 0x1018, 0x04) == -1))
		return co_nmt_boot_abort_state;

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_serial_nr_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

	if (__unlikely(!co_nmt_boot_chk(boot, 0x1f88, boot->id, ac, ptr, n)))
		return co_nmt_boot_abort_state;

	return co_nmt_boot_chk_node_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_node_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	// If the keep-alive bit is set, check the node state.
	if (boot->assignment & 0x10) {
		int ms;
		if (boot->ms) {
			ms = boot->ms;
			boot->es = 'E';
		} else {
			ms = LELY_CO_NMT_BOOT_RTR_TIMEOUT;
			boot->es = 'F';
			// If we're not a heartbeat consumer, start node
			// guarding by sending the first RTR.
			co_nmt_boot_send_rtr(boot);
		}

		// Start the CAN frame receiver for the heartbeat or node guard
		// message.
		can_recv_start(boot->recv, boot->net, 0x700 + boot->id, 0);
		// Start the CAN timer in case we do not receive a heartbeat
		// indication or a node guard confirmation.
		can_timer_timeout(boot->timer, boot->net, ms);

		return NULL;
	}

	return co_nmt_boot_chk_sw_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_node_on_time(co_nmt_boot_t *boot, const struct timespec *tp)
{
	__unused_var(boot);
	__unused_var(tp);

	return co_nmt_boot_abort_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_node_on_recv(co_nmt_boot_t *boot, const struct can_msg *msg)
{
	assert(boot);

	can_recv_stop(boot->recv);
	can_timer_stop(boot->timer);

	assert(msg);
	assert(msg->len >= 1);
	boot->st = msg->data[0];

	if ((boot->st & ~CO_NMT_ST_TOGGLE) == CO_NMT_ST_START) {
		// If the node is already operational, skip the 'check and
		// update software version' and 'check configuration' steps and
		// proceed immediately to 'start error control service'.
		return co_nmt_boot_ec_state;
	} else {
		// If the node is not operational, send the NMT 'reset
		// communication' command and proceed as if the keep-alive bit
		// was not set.
		co_nmt_cs_req(boot->nmt, CO_NMT_CS_RESET_COMM, boot->id);
		return co_nmt_boot_chk_sw_state;
	}
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_sw_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	if (boot->assignment & 0x20) {
		boot->es = 'G';

		// Abort if the expected program software identification
		// (sub-object 1F55:ID) is 0.
		co_unsigned32_t sw_id =
				co_dev_get_val_u32(boot->dev, 0x1f55, boot->id);
		if (!sw_id)
			return co_nmt_boot_abort_state;

		// Read the program software identification of the slave
		// (sub-object 1F56:01).
		if (__unlikely(co_nmt_boot_up(boot, 0x1f56, 0x01) == -1))
			return co_nmt_boot_abort_state;

		return NULL;
	}

	return co_nmt_boot_chk_cfg_date_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_sw_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

	if (__unlikely(ac))
		return co_nmt_boot_abort_state;

	// If the program software identification matches the expected value,
	// proceed to 'check configuration'.
	if (co_nmt_boot_chk(boot, 0x1f55, boot->id, ac, ptr, n))
		return co_nmt_boot_chk_cfg_date_state;

	// Do not update the software if software update (bit 6) is not allowed
	// or if the keep-alive bit (bit 4) is set.
	if ((boot->assignment & 0x50) != 0x40) {
		boot->es = 'H';
		return co_nmt_boot_abort_state;
	}

	boot->es = 'I';

	if (!boot->up_sw_ind)
		return co_nmt_boot_abort_state;

	return co_nmt_boot_up_sw_req_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_up_sw_req_on_enter(co_nmt_boot_t *boot)
{
	__unused_var(boot);

	return co_nmt_boot_up_sw_res_state;
}

static void
co_nmt_boot_up_sw_req_on_leave(co_nmt_boot_t *boot)
{
	assert(boot);
	assert(boot->up_sw_ind);

	boot->up_sw_ind(boot->nmt, boot->id, boot->sdo, boot->up_sw_data);
}

static co_nmt_boot_state_t *
co_nmt_boot_up_sw_res_on_res(co_nmt_boot_t *boot, int res)
{
	__unused_var(boot);

	if (__unlikely(res))
		return co_nmt_boot_abort_state;

	return co_nmt_boot_chk_cfg_date_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_cfg_date_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	boot->es = 'J';

	// If the expected configuration date (sub-object 1F26:ID) or time
	// (sub-object 1F27:ID) are not configured, proceed to 'update
	// configuration'.
	co_unsigned32_t cfg_date =
			co_dev_get_val_u32(boot->dev, 0x1f26, boot->id);
	co_unsigned32_t cfg_time =
			co_dev_get_val_u32(boot->dev, 0x1f27, boot->id);
	if (!cfg_date || !cfg_time)
		return co_nmt_boot_up_cfg_state;

	// Read the configuration date of the slave (sub-object 1020:01).
	if (__unlikely(co_nmt_boot_up(boot, 0x1020, 0x01) == -1))
		return co_nmt_boot_abort_state;

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_cfg_date_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

	// If the configuration date does not match the expected value, skip
	// checking the time and proceed to 'update configuration'.
	if (!co_nmt_boot_chk(boot, 0x1f26, boot->id, ac, ptr, n))
		return co_nmt_boot_up_cfg_state;

	// Read the configuration time of the slave (sub-object 1020:02).
	if (__unlikely(co_nmt_boot_up(boot, 0x1020, 0x02) == -1))
		return co_nmt_boot_abort_state;

	return co_nmt_boot_chk_cfg_time_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_cfg_time_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

	// If the configuration time does not match the expected value, proceed
	// to 'update configuration'.
	if (!co_nmt_boot_chk(boot, 0x1f27, boot->id, ac, ptr, n))
		return co_nmt_boot_up_cfg_state;

	return co_nmt_boot_ec_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_up_cfg_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	boot->es = 'J';

	if (!boot->up_cfg_ind)
		return co_nmt_boot_abort_state;

	return co_nmt_boot_up_cfg_req_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_up_cfg_req_on_enter(co_nmt_boot_t *boot)
{
	__unused_var(boot);

	return co_nmt_boot_up_cfg_res_state;
}

static void
co_nmt_boot_up_cfg_req_on_leave(co_nmt_boot_t *boot)
{
	assert(boot);
	assert(boot->up_cfg_ind);

	boot->up_cfg_ind(boot->nmt, boot->id, boot->sdo, boot->up_cfg_data);
}

static co_nmt_boot_state_t *
co_nmt_boot_up_cfg_res_on_res(co_nmt_boot_t *boot, int res)
{
	__unused_var(boot);

	if (__unlikely(res))
		return co_nmt_boot_abort_state;

	return co_nmt_boot_ec_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_ec_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	if (boot->ms) {
		boot->es = 'K';
		// Start the CAN frame receiver for heartbeat messages.
		can_recv_start(boot->recv, boot->net, 0x700 + boot->id, 0);
		// Wait for the first heartbeat indication.
		can_timer_timeout(boot->timer, boot->net, boot->ms);
		return NULL;
	} else if (boot->assignment & 0x01) {
		// If the guard time is non-zero, start node guarding by sending
		// the first RTR, but do not wait for the response.
		co_unsigned16_t guard = (boot->assignment >> 16) & 0xffff;
		if (guard)
			co_nmt_boot_send_rtr(boot);
	}

	boot->es = 0;
	return co_nmt_boot_abort_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_ec_on_time(co_nmt_boot_t *boot, const struct timespec *tp)
{
	__unused_var(boot);
	__unused_var(tp);

	return co_nmt_boot_abort_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_ec_on_recv(co_nmt_boot_t *boot, const struct can_msg *msg)
{
	assert(boot);
	assert(msg);

	if (msg->len >= 1) {
		boot->st = msg->data[0];
		boot->es = 0;
	}

	return co_nmt_boot_abort_state;
}

static int
co_nmt_boot_up(co_nmt_boot_t *boot, co_unsigned16_t idx, co_unsigned8_t subidx)
{
	assert(boot);

	return co_csdo_up_req(boot->sdo, idx, subidx, &co_nmt_boot_up_con,
			boot);
}

static int
co_nmt_boot_chk(co_nmt_boot_t *boot, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned32_t ac, const void *ptr, size_t n)
{
	assert(boot);

	co_sub_t *sub = co_dev_find_sub(boot->dev, idx, subidx);
	if (__unlikely(!sub))
		return 0;
	co_unsigned16_t type = co_sub_get_type(sub);

	if (__unlikely(ac))
		return 0;

	union co_val val;
	if (__unlikely(!co_val_read(type, &val, ptr, (const uint8_t *)ptr + n)))
		return 0;

	int eq = !co_val_cmp(type, &val, co_sub_get_val(sub));
	co_val_fini(type, &val);
	return eq;
}

static int
co_nmt_boot_send_rtr(co_nmt_boot_t *boot)
{
	assert(boot);

	struct can_msg msg = CAN_MSG_INIT;
	msg.id = 0x700 + boot->id;
	msg.flags |= CAN_FLAG_RTR;

	return can_net_send(boot->net, &msg);
}

#endif // !LELY_NO_CO_NMT_MASTER

