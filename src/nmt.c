/*!\file
 * This file is part of the CANopen library; it contains the implementation of
 * the network management (NMT) functions.
 *
 * \see lely/co/nmt.h
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
#include <lely/util/diag.h>
#include <lely/co/dev.h>
#include <lely/co/nmt.h>
#include <lely/co/obj.h>
#include <lely/co/sdo.h>
#include <lely/co/val.h>
#include "nmt_ec.h"
#include "nmt_srv.h"

#include <assert.h>
#include <stdlib.h>

//! An opaque CANopen NMT state type.
typedef const struct co_nmt_state co_nmt_state_t;

//! A CANopen NMT state.
struct co_nmt_state {
	//! A pointer to the function invoked when a new state is entered.
	co_nmt_state_t *(*on_enter)(co_nmt_t *nmt);
	/*!
	 * A pointer to the transition function invoked when an NMT command is
	 * received.
	 *
	 * \param nmt a pointer to an NMT master/slave service.
	 * \param cs  the NMT command specifier (one of #CO_NMT_CS_START,
	 *            #CO_NMT_CS_STOP, #CO_NMT_CS_ENTER_PREOP,
	 *            #CO_NMT_CS_RESET_NODE or #CO_NMT_CS_RESET_COMM).
	 *
	 * \returns a pointer to the next state.
	 */
	co_nmt_state_t *(*on_cs)(co_nmt_t *nmt, co_unsigned8_t cs);
	//! A pointer to the function invoked when the current state is left.
	void (*on_leave)(co_nmt_t *nmt);
};

//! A CANopen NMT master/slave service.
struct __co_nmt {
	//! A pointer to a CAN network interface.
	can_net_t *net;
	//! A pointer to a CANopen device.
	co_dev_t *dev;
	//! The pending Node-ID.
	co_unsigned8_t id;
	//! The concise DCF of the application parameters.
	void *dcf_node;
	//! The concise DCF of the communication parameters.
	void *dcf_comm;
	//! The current state.
	co_nmt_state_t *state;
	//! The NMT service manager.
	struct co_nmt_srv srv;
	//! The NMT start-up value.
	co_unsigned32_t startup;
	//! A pointer to the CAN frame receiver for NMT messages.
	can_recv_t *recv_000;
	//! A pointer to the NMT command indication function.
	co_nmt_cs_ind_t *cs_ind;
	//! A pointer to user-specified data for #cs_ind.
	void *cs_data;
	//! A pointer to the CAN frame receiver for NMT error control messages.
	can_recv_t *recv_700;
	/*!
	 * A pointer to the CAN timer for life guarding or heartbeat production.
	 */
	can_timer_t *timer;
	//! The state of the NMT service (including the toggle bit).
	co_unsigned8_t st;
	//! The guard time (in milliseconds).
	co_unsigned16_t gt;
	//! The lifetime factor.
	co_unsigned8_t ltf;
	//! The producer heartbeat time (in milliseconds).
	co_unsigned16_t ms;
	/*!
	 * Indicates whether a life guarding error occurred (#CO_NMT_EC_OCCURRED
	 * or #CO_NMT_EC_RESOLVED).
	 */
	int lg_state;
	//! A pointer to the life guarding event indication function.
	co_nmt_lg_ind_t *lg_ind;
	//! A pointer to user-specified data for #lg_ind.
	void *lg_data;
	//! An array of pointers to the heartbeat consumers.
	co_nmt_hb_t **hbs;
	//! The number of heartbeat consumers.
	co_unsigned8_t nhb;
	//! A pointer to the heartbeat event indication function.
	co_nmt_hb_ind_t *hb_ind;
	//! A pointer to user-specified data for #hb_ind.
	void *hb_data;
	//! A pointer to the state change indication function.
	co_nmt_st_ind_t *st_ind;
	//! A pointer to user-specified data for #st_ind.
	void *st_data;
};

/*!
 * Updates and (de)activates the life guarding or heartbeat production services.
 * This function is invoked by the download indication functions when object
 * 100C (Guard time), 100D (Life time factor) or 1017 (Producer heartbeat time)
 * is updated.
 *
 * \returns 0 on success, or -1 on error.
 */
static int co_nmt_update(co_nmt_t *nmt);

/*!
 * The download indication function for CANopen object 100C (Guard time).
 *
 * \see co_sub_dn_ind_t
 */
static co_unsigned32_t co_100c_dn_ind(co_sub_t *sub, struct co_sdo_req *req,
		void *data);

/*!
 * The download indication function for CANopen object 100D (Life time factor).
 *
 * \see co_sub_dn_ind_t
 */
static co_unsigned32_t co_100d_dn_ind(co_sub_t *sub, struct co_sdo_req *req,
		void *data);

/*!
 * The download indication function for CANopen object 1016 (Consumer heartbeat
 * time).
 *
 * \see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1016_dn_ind(co_sub_t *sub, struct co_sdo_req *req,
		void *data);

/*!
 * The download indication function for CANopen object 1017 (Producer heartbeat
 * time).
 *
 * \see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1017_dn_ind(co_sub_t *sub, struct co_sdo_req *req,
		void *data);

/*!
 * The download indication function for (all sub-objects of) CANopen object 1F80
 * (NMT Start-up).
 *
 * \see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1f80_dn_ind(co_sub_t *sub, struct co_sdo_req *req,
		void *data);

//! The CAN receive callback function for NMT messages. \see can_recv_func_t
static int co_nmt_recv_000(const struct can_msg *msg, void *data);

/*!
 * The CAN receive callback function for NMT error control messages.
 *
 * \see can_recv_func_t
 */
static int co_nmt_recv_700(const struct can_msg *msg, void *data);

/*!
 * The CAN timer callback function for a heartbeat producer.
 *
 * \see can_timer_func_t
 */
static int co_nmt_timer(const struct timespec *tp, void *data);

//! The default life guarding event handler.
static void default_lg_ind(co_nmt_t *nmt, int state, void *data);

//! The CANopen NMT heartbeat event indication function. \see co_nmt_hb_ind_t
static void co_nmt_hb_ind(co_nmt_t *nmt, co_unsigned8_t id, int state,
		void *data);

//! The CANopen NMT event change indication function. \see co_nmt_st_ind_t
static void co_nmt_st_ind(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st,
		void *data);

//! The default heartbeat event handler.
static void default_hb_ind(co_nmt_t *nmt, co_unsigned8_t id, int state,
		void *data);

/*!
 * Enters the specified state of an NMT master/slave service and invokes the
 * exit and entry functions.
 */
static inline void co_nmt_enter(co_nmt_t *nmt, co_nmt_state_t *next);

/*!
 * Invokes the 'NMT command received' transition function of the current state
 * of an NMT master/slave service.
 *
 * \param nmt a pointer to an NMT master/slave service.
 * \param cs  the NMT command specifier (one of #CO_NMT_CS_START,
 *            #CO_NMT_CS_STOP, #CO_NMT_CS_ENTER_PREOP, #CO_NMT_CS_RESET_NODE or
 *            #CO_NMT_CS_RESET_COMM).
 */
static inline void co_nmt_emit_cs(co_nmt_t *nmt, co_unsigned8_t cs);

//! The 'NMT command received' transition function of the 'initializing' state.
static co_nmt_state_t *co_nmt_init_on_cs(co_nmt_t *nmt, co_unsigned8_t cs);

//! The 'initializing' state.
static const co_nmt_state_t *co_nmt_init_state = &(co_nmt_state_t){
	NULL,
	&co_nmt_init_on_cs,
	NULL
};

//! The entry function of the 'reset application' state.
static co_nmt_state_t *co_nmt_reset_node_on_enter(co_nmt_t *nmt);

//! The NMT 'reset application' state.
static const co_nmt_state_t *co_nmt_reset_node_state = &(co_nmt_state_t){
	&co_nmt_reset_node_on_enter,
	NULL,
	NULL
};

//! The entry function of the 'reset communication' state.
static co_nmt_state_t *co_nmt_reset_comm_on_enter(co_nmt_t *nmt);

/*!
 * The 'NMT command received' transition function of the 'reset communication'
 * state.
 */
static co_nmt_state_t *co_nmt_reset_comm_on_cs(co_nmt_t *nmt,
		co_unsigned8_t cs);

//! The NMT 'reset communication' state.
static const co_nmt_state_t *co_nmt_reset_comm_state = &(co_nmt_state_t){
	&co_nmt_reset_comm_on_enter,
	&co_nmt_reset_comm_on_cs,
	NULL
};

//! The entry function of the 'pre-operational' state.
static co_nmt_state_t *co_nmt_preop_on_enter(co_nmt_t *nmt);

//! The NMT 'pre-operational' state.
static const co_nmt_state_t *co_nmt_preop_state = &(co_nmt_state_t){
	&co_nmt_preop_on_enter,
	NULL,
	NULL
};

//! The entry function of the 'operational' state.
static co_nmt_state_t *co_nmt_start_on_enter(co_nmt_t *nmt);

//! The 'NMT command received' transition function of the 'operational' state.
static co_nmt_state_t *co_nmt_start_on_cs(co_nmt_t *nmt, co_unsigned8_t cs);

//! The NMT 'operational' state.
static const co_nmt_state_t *co_nmt_start_state = &(co_nmt_state_t){
	&co_nmt_start_on_enter,
	&co_nmt_start_on_cs,
	NULL
};

//! The entry function of the 'stopped' state.
static co_nmt_state_t *co_nmt_stop_on_enter(co_nmt_t *nmt);

//! The 'NMT command received' transition function of the 'stopped' state.
static co_nmt_state_t *co_nmt_stop_on_cs(co_nmt_t *nmt, co_unsigned8_t cs);

//! The NMT 'stopped' state.
static const co_nmt_state_t *co_nmt_stop_state = &(co_nmt_state_t){
	&co_nmt_stop_on_enter,
	&co_nmt_stop_on_cs,
	NULL
};

//! The entry function of the 'slave boot-up procedure' state.
static co_nmt_state_t *co_nmt_slave_on_enter(co_nmt_t *nmt);

//! The 'slave boot-up procedure' state (see Fig. 1 in CiA DSP-302 V3.2.1).
static const co_nmt_state_t *co_nmt_slave_state = &(co_nmt_state_t){
	&co_nmt_slave_on_enter,
	NULL,
	NULL
};

//! The entry function of the 'autostart' state.
static co_nmt_state_t *co_nmt_autostart_on_enter(co_nmt_t *nmt);

//! The 'NMT command received' transition function of the 'autostart' state.
static co_nmt_state_t *co_nmt_autostart_on_cs(co_nmt_t *nmt, co_unsigned8_t cs);

//! The 'autostart' state (see Fig. 1 & 2 in CiA DSP-302 V3.2.1).
static const co_nmt_state_t *co_nmt_autostart_state = &(co_nmt_state_t){
	&co_nmt_autostart_on_enter,
	&co_nmt_autostart_on_cs,
	NULL
};

//! Initializes the error control services. \see co_nmt_ec_fini()
static void co_nmt_ec_init(co_nmt_t *nmt);

//! Finalizes the error control services. \see co_nmt_ec_init()
static void co_nmt_ec_fini(co_nmt_t *nmt);

//! Initializes the heartbeat consumer services. \see co_nmt_hb_fini()
static void co_nmt_hb_init(co_nmt_t *nmt);

//! Finalizes the heartbeat consumer services. \see co_nmt_hb_init()
static void co_nmt_hb_fini(co_nmt_t *nmt);

/*!
 * Sends an NMT error control response message.
 *
 * \param nmt a pointer to an NMT master/slave service.
 * \param st  the node state and toggle bit.
 *
 * \returns 0 on success, or -1 on error.
 */
static int co_nmt_send_res(co_nmt_t *nmt, co_unsigned8_t st);

//! The services enabled in the NMT 'pre-operational' state.
#define CO_NMT_PREOP_SRV \
	(CO_NMT_SRV_SDO | CO_NMT_SRV_SYNC | CO_NMT_SRV_TIME | CO_NMT_SRV_EMCY)

//! The services enabled in the NMT 'operational' state.
#define CO_NMT_START_SRV \
	(CO_NMT_PREOP_SRV | CO_NMT_SRV_PDO)

//! The services enabled in the NMT 'stopped' state.
#define CO_NMT_STOP_SRV	0

LELY_CO_EXPORT void *
__co_nmt_alloc(void)
{
	return malloc(sizeof(struct __co_nmt));
}

LELY_CO_EXPORT void
__co_nmt_free(void *ptr)
{
	free(ptr);
}

LELY_CO_EXPORT struct __co_nmt *
__co_nmt_init(struct __co_nmt *nmt, can_net_t *net, co_dev_t *dev)
{
	assert(nmt);
	assert(net);
	assert(dev);

	errc_t errc = 0;

	nmt->net = net;
	nmt->dev = dev;

	nmt->id = co_dev_get_id(nmt->dev);

	// Store a concise DCF containing the application parameters.
	if (__unlikely(co_dev_write_dcf(nmt->dev, 0x2000, 0x9fff,
			&nmt->dcf_node) == -1)) {
		errc = get_errc();
		goto error_write_dcf_node;
	}

	// Store a concise DCF containing the communication parameters.
	if (__unlikely(co_dev_write_dcf(nmt->dev, 0x1000, 0x1fff,
			&nmt->dcf_comm) == -1)) {
		errc = get_errc();
		goto error_write_dcf_comm;
	}

	co_nmt_srv_init(&nmt->srv);

	nmt->state = co_nmt_init_state;

	nmt->startup = 0;

	// Create the CAN frame receiver for NMT messages.
	nmt->recv_000 = can_recv_create();
	if (__unlikely(!nmt->recv_000)) {
		errc = get_errc();
		goto error_create_recv_000;
	}
	can_recv_set_func(nmt->recv_000, &co_nmt_recv_000, nmt);

	nmt->cs_ind = NULL;
	nmt->cs_data = NULL;

	nmt->recv_700 = NULL;
	nmt->timer = NULL;

	nmt->st = CO_NMT_ST_BOOTUP;
	nmt->gt = 0;
	nmt->ltf = 0;
	nmt->ms = 0;

	nmt->lg_state = CO_NMT_EC_RESOLVED;
	nmt->lg_ind = &default_lg_ind;
	nmt->lg_data = NULL;

	nmt->hbs = NULL;
	nmt->nhb = 0;

	nmt->hb_ind = &default_hb_ind;
	nmt->hb_data = NULL;
	nmt->st_ind = NULL;
	nmt->st_data = NULL;

	// Set the download indication function for the guard time.
	co_obj_t *obj_100c = co_dev_find_obj(nmt->dev, 0x100c);
	if (obj_100c)
		co_obj_set_dn_ind(obj_100c, &co_100c_dn_ind, nmt);

	// Set the download indication function for the life time factor.
	co_obj_t *obj_100d = co_dev_find_obj(nmt->dev, 0x100d);
	if (obj_100d)
		co_obj_set_dn_ind(obj_100d, &co_100d_dn_ind, nmt);

	// Set the download indication function for the consumer heartbeat time.
	co_obj_t *obj_1016 = co_dev_find_obj(nmt->dev, 0x1016);
	if (obj_1016)
		co_obj_set_dn_ind(obj_1016, &co_1016_dn_ind, nmt);

	// Set the download indication function for the producer heartbeat time.
	co_obj_t *obj_1017 = co_dev_find_obj(nmt->dev, 0x1017);
	if (obj_1017)
		co_obj_set_dn_ind(obj_1017, &co_1017_dn_ind, nmt);

	// Set the download indication function for the NMT start-up value.
	co_obj_t *obj_1f80 = co_dev_find_obj(nmt->dev, 0x1f80);
	if (obj_1f80)
		co_obj_set_dn_ind(obj_1f80, &co_1f80_dn_ind, nmt);

	return nmt;

	if (obj_1f80)
		co_obj_set_dn_ind(obj_1f80, NULL, NULL);
	if (obj_1017)
		co_obj_set_dn_ind(obj_1017, NULL, NULL);
	if (obj_1016)
		co_obj_set_dn_ind(obj_1016, NULL, NULL);
	if (obj_100d)
		co_obj_set_dn_ind(obj_100d, NULL, NULL);
	if (obj_100c)
		co_obj_set_dn_ind(obj_100c, NULL, NULL);
	can_recv_destroy(nmt->recv_000);
error_create_recv_000:
	co_val_fini(CO_DEFTYPE_DOMAIN, &nmt->dcf_comm);
error_write_dcf_comm:
	co_val_fini(CO_DEFTYPE_DOMAIN, &nmt->dcf_node);
error_write_dcf_node:
	set_errc(errc);
	return NULL;
}

LELY_CO_EXPORT void
__co_nmt_fini(struct __co_nmt *nmt)
{
	assert(nmt);

	// Remove the download indication function for the NMT start-up value.
	co_obj_t *obj_1f80 = co_dev_find_obj(nmt->dev, 0x1f80);
	if (obj_1f80)
		co_obj_set_dn_ind(obj_1f80, NULL, NULL);

	// Remove the download indication function for the producer heartbeat
	// time.
	co_obj_t *obj_1017 = co_dev_find_obj(nmt->dev, 0x1017);
	if (obj_1017)
		co_obj_set_dn_ind(obj_1017, NULL, NULL);

	// Remove the download indication function for the consumer heartbeat
	// time.
	co_obj_t *obj_1016 = co_dev_find_obj(nmt->dev, 0x1016);
	if (obj_1016)
		co_obj_set_dn_ind(obj_1016, NULL, NULL);

	// Remove the download indication function for the life time factor.
	co_obj_t *obj_100d = co_dev_find_obj(nmt->dev, 0x100d);
	if (obj_100d)
		co_obj_set_dn_ind(obj_100d, NULL, NULL);

	// Remove the download indication function for the guard time.
	co_obj_t *obj_100c = co_dev_find_obj(nmt->dev, 0x100c);
	if (obj_100c)
		co_obj_set_dn_ind(obj_100c, NULL, NULL);

	co_nmt_hb_fini(nmt);

	co_nmt_ec_fini(nmt);

	can_timer_destroy(nmt->timer);
	can_recv_destroy(nmt->recv_700);

	co_nmt_srv_fini(&nmt->srv);

	co_val_fini(CO_DEFTYPE_DOMAIN, &nmt->dcf_comm);
	co_val_fini(CO_DEFTYPE_DOMAIN, &nmt->dcf_node);
}

LELY_CO_EXPORT co_nmt_t *
co_nmt_create(can_net_t *net, co_dev_t *dev)
{
	errc_t errc = 0;

	co_nmt_t *nmt = __co_nmt_alloc();
	if (__unlikely(!nmt)) {
		errc = get_errc();
		goto error_alloc_nmt;
	}

	if (__unlikely(!__co_nmt_init(nmt, net, dev))) {
		errc = get_errc();
		goto error_init_nmt;
	}

	return nmt;

error_init_nmt:
	__co_nmt_free(nmt);
error_alloc_nmt:
	set_errc(errc);
	return NULL;
}

LELY_CO_EXPORT void
co_nmt_destroy(co_nmt_t *nmt)
{
	if (nmt) {
		__co_nmt_fini(nmt);
		__co_nmt_free(nmt);
	}
}

LELY_CO_EXPORT void
co_nmt_get_cs_ind(const co_nmt_t *nmt, co_nmt_cs_ind_t **pind, void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->cs_ind;
	if (pdata)
		*pdata = nmt->cs_data;
}

LELY_CO_EXPORT void
co_nmt_set_cs_ind(co_nmt_t *nmt, co_nmt_cs_ind_t *ind, void *data)
{
	assert(nmt);

	nmt->cs_ind = ind;
	nmt->cs_data = data;
}


LELY_CO_EXPORT void
co_nmt_get_lg_ind(const co_nmt_t *nmt, co_nmt_lg_ind_t **pind, void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->lg_ind;
	if (pdata)
		*pdata = nmt->lg_data;
}

LELY_CO_EXPORT void
co_nmt_set_lg_ind(co_nmt_t *nmt, co_nmt_lg_ind_t *ind, void *data)
{
	assert(nmt);

	nmt->lg_ind = ind;
	nmt->lg_data = data;
}

LELY_CO_EXPORT void
co_nmt_get_hb_ind(const co_nmt_t *nmt, co_nmt_hb_ind_t **pind, void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->hb_ind;
	if (pdata)
		*pdata = nmt->hb_data;
}

LELY_CO_EXPORT void
co_nmt_set_hb_ind(co_nmt_t *nmt, co_nmt_hb_ind_t *ind, void *data)
{
	assert(nmt);

	nmt->hb_ind = ind;
	nmt->hb_data = data;
}

LELY_CO_EXPORT void
co_nmt_get_st_ind(const co_nmt_t *nmt, co_nmt_st_ind_t **pind, void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->st_ind;
	if (pdata)
		*pdata = nmt->st_data;
}

LELY_CO_EXPORT void
co_nmt_set_st_ind(co_nmt_t *nmt, co_nmt_st_ind_t *ind, void *data)
{
	assert(nmt);

	nmt->st_ind = ind;
	nmt->st_data = data;
}

LELY_CO_EXPORT co_unsigned8_t
co_nmt_get_id(const co_nmt_t *nmt)
{
	assert(nmt);

	return nmt->id;
}

LELY_CO_EXPORT int
co_nmt_set_id(co_nmt_t *nmt, co_unsigned8_t id)
{
	assert(nmt);

	if (__unlikely(!id || (id > CO_NUM_NODES && id != 0xff))) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	nmt->id = id;

	return 0;
}

LELY_CO_EXPORT co_unsigned8_t
co_nmt_get_state(const co_nmt_t *nmt)
{
	assert(nmt);

	return nmt->st & ~CO_NMT_ST_TOGGLE;
}

LELY_CO_EXPORT int
co_nmt_cs_ind(co_nmt_t *nmt, co_unsigned8_t cs)
{
	assert(nmt);

	switch (cs) {
	case CO_NMT_CS_START:
	case CO_NMT_CS_STOP:
	case CO_NMT_CS_ENTER_PREOP:
	case CO_NMT_CS_RESET_NODE:
	case CO_NMT_CS_RESET_COMM:
		break;
	default:
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	co_nmt_emit_cs(nmt, cs);

	return 0;
}

LELY_CO_EXPORT int
co_nmt_comm_err_ind(co_nmt_t *nmt)
{
	assert(nmt);

	switch (co_dev_get_val_u8(nmt->dev, 0x1029, 0x01)) {
	case 0:
		if (co_nmt_get_state(nmt) != CO_NMT_ST_START)
			return 0;
		return co_nmt_cs_ind(nmt, CO_NMT_CS_ENTER_PREOP);
	case 2:
		return co_nmt_cs_ind(nmt, CO_NMT_CS_STOP);
	default:
		return 0;
	}
}

LELY_CO_EXPORT co_rpdo_t *
co_nmt_get_rpdo(const co_nmt_t *nmt, co_unsigned16_t n)
{
	assert(nmt);

	if (__unlikely(!n || n > nmt->srv.nrpdo))
		return NULL;

	return nmt->srv.rpdos[n - 1];
}

LELY_CO_EXPORT co_tpdo_t *
co_nmt_get_tpdo(const co_nmt_t *nmt, co_unsigned16_t n)
{
	assert(nmt);

	if (__unlikely(!n || n > nmt->srv.ntpdo))
		return NULL;

	return nmt->srv.tpdos[n - 1];
}

LELY_CO_EXPORT co_ssdo_t *
co_nmt_get_ssdo(const co_nmt_t *nmt, co_unsigned8_t n)
{
	assert(nmt);

	if (__unlikely(!n || n > nmt->srv.nssdo))
		return NULL;

	return nmt->srv.ssdos[n - 1];
}

LELY_CO_EXPORT co_csdo_t *
co_nmt_get_csdo(const co_nmt_t *nmt, co_unsigned8_t n)
{
	assert(nmt);

	if (__unlikely(!n || n > nmt->srv.ncsdo))
		return NULL;

	return nmt->srv.csdos[n - 1];
}

LELY_CO_EXPORT co_sync_t *
co_nmt_get_sync(const co_nmt_t *nmt)
{
	assert(nmt);

	return nmt->srv.sync;
}

LELY_CO_EXPORT co_time_t *
co_nmt_get_time(const co_nmt_t *nmt)
{
	assert(nmt);

	return nmt->srv.time;
}

LELY_CO_EXPORT co_emcy_t *
co_nmt_get_emcy(const co_nmt_t *nmt)
{
	assert(nmt);

	return nmt->srv.emcy;
}

static int
co_nmt_update(co_nmt_t *nmt)
{
	assert(nmt);

	// Heartbeat production has precedence over life guarding.
	int lt = nmt->ms ? 0 : nmt->gt * nmt->ltf;

	if (lt) {
		if (!nmt->recv_700) {
			nmt->recv_700 = can_recv_create();
			if (__unlikely(!nmt->recv_700))
				return -1;
			can_recv_set_func(nmt->recv_700, &co_nmt_recv_700, nmt);
		}
		// Start the CAN frame receiver for node guarding RTRs.
		can_recv_start(nmt->recv_700, nmt->net,
				0x700 + co_dev_get_id(nmt->dev), CAN_FLAG_RTR);
	} else if (nmt->recv_700) {
		can_recv_destroy(nmt->recv_700);
		nmt->recv_700 = NULL;
	}

	if (nmt->ms || lt) {
		if (!nmt->timer) {
			nmt->timer = can_timer_create();
			if (__unlikely(!nmt->timer))
				return -1;
			can_timer_set_func(nmt->timer, &co_nmt_timer, nmt);
		}
		if (nmt->ms) {
			// Start the CAN timer for the heartbeat producer.
			struct timespec interval = { 0, nmt->ms * 1000000 };
			can_timer_start(nmt->timer, nmt->net, NULL, &interval);
		}
	} else if (nmt->timer) {
		can_timer_destroy(nmt->timer);
		nmt->timer = NULL;
	}

	return 0;
}

static co_unsigned32_t
co_100c_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x100c);
	assert(req);
	co_nmt_t *nmt = data;
	assert(nmt);

	co_unsigned32_t ac = 0;

	co_unsigned16_t type = co_sub_get_type(sub);
	union co_val val;
	if (__unlikely(co_sdo_req_dn(req, type, &val, &ac) == -1))
		return ac;

	if (__unlikely(co_sub_get_subidx(sub))) {
		ac = CO_SDO_AC_NO_SUB;
		goto error;
	}

	assert(type == CO_DEFTYPE_UNSIGNED16);
	co_unsigned16_t gt = val.u16;
	co_unsigned16_t gt_old = co_sub_get_val_u16(sub);
	if (gt == gt_old)
		goto error;

	nmt->gt = gt;

	co_sub_dn(sub, &val);
	co_val_fini(type, &val);

	co_nmt_update(nmt);
	return 0;

error:
	co_val_fini(type, &val);
	return ac;
}

static co_unsigned32_t
co_100d_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x100d);
	assert(req);
	co_nmt_t *nmt = data;
	assert(nmt);

	co_unsigned32_t ac = 0;

	co_unsigned16_t type = co_sub_get_type(sub);
	union co_val val;
	if (__unlikely(co_sdo_req_dn(req, type, &val, &ac) == -1))
		return ac;

	if (__unlikely(co_sub_get_subidx(sub))) {
		ac = CO_SDO_AC_NO_SUB;
		goto error;
	}

	assert(type == CO_DEFTYPE_UNSIGNED8);
	co_unsigned8_t ltf = val.u8;
	co_unsigned8_t ltf_old = co_sub_get_val_u8(sub);
	if (ltf == ltf_old)
		return 0;

	nmt->ltf = ltf;

	co_sub_dn(sub, &val);
	co_val_fini(type, &val);

	co_nmt_update(nmt);
	return 0;

error:
	co_val_fini(type, &val);
	return ac;
}

static co_unsigned32_t
co_1016_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1016);
	assert(req);
	co_nmt_t *nmt = data;
	assert(nmt);

	co_unsigned32_t ac = 0;

	co_unsigned16_t type = co_sub_get_type(sub);
	union co_val val;
	if (__unlikely(co_sdo_req_dn(req, type, &val, &ac) == -1))
		return ac;

	co_unsigned8_t subidx = co_sub_get_subidx(sub);
	if (__unlikely(!subidx)) {
		ac = CO_SDO_AC_NO_WO;
		goto error;
	}
	if (__unlikely(subidx > nmt->nhb)) {
		ac = CO_SDO_AC_NO_SUB;
		goto error;
	}

	assert(type == CO_DEFTYPE_UNSIGNED32);
	if (val.u32 == co_sub_get_val_u32(sub))
		goto error;

	co_unsigned8_t id = (val.u32 >> 16) & 0xff;
	co_unsigned16_t ms = val.u32 & 0xffff;

	// If the heartbeat consumer is active (valid Node-ID and non-zero
	// heartbeat time), check the other entries for duplicate Node-IDs.
	co_obj_t *obj_1016 = co_dev_find_obj(nmt->dev, 0x1016);
	if (id && id <= CO_NUM_NODES && ms) {
		for (co_unsigned8_t i = 1; i <= CO_NUM_NODES; i++) {
			// Skip the current entry.
			if (i == subidx)
				continue;
			co_unsigned32_t val_i = co_obj_get_val_u32(obj_1016, i);
			co_unsigned8_t id_i = (val_i >> 16) & 0xff;
			co_unsigned16_t ms_i = val_i & 0xffff;
			// It's not allowed to have two active heartbeat
			// consumers with the same Node-ID.
			if (__unlikely(id_i == id && ms_i)) {
				ac = CO_SDO_AC_PARAM;
				goto error;
			}
		}
	}

	co_sub_dn(sub, &val);
	co_val_fini(type, &val);

	co_nmt_hb_set_1016(nmt->hbs[subidx - 1], id, ms);
	return 0;

error:
	co_val_fini(type, &val);
	return ac;
}

static co_unsigned32_t
co_1017_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1017);
	assert(req);
	co_nmt_t *nmt = data;
	assert(nmt);

	co_unsigned32_t ac = 0;

	co_unsigned16_t type = co_sub_get_type(sub);
	union co_val val;
	if (__unlikely(co_sdo_req_dn(req, type, &val, &ac) == -1))
		return ac;

	if (__unlikely(co_sub_get_subidx(sub))) {
		ac = CO_SDO_AC_NO_SUB;
		goto error;
	}

	assert(type == CO_DEFTYPE_UNSIGNED16);
	co_unsigned16_t ms = val.u16;
	co_unsigned16_t ms_old = co_sub_get_val_u16(sub);
	if (ms == ms_old)
		goto error;

	nmt->ms = ms;

	co_sub_dn(sub, &val);
	co_val_fini(type, &val);

	co_nmt_update(nmt);
	return 0;

error:
	co_val_fini(type, &val);
	return ac;
}

static co_unsigned32_t
co_1f80_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1f80);
	assert(req);
	__unused_var(data);

	co_unsigned32_t ac = 0;

	co_unsigned16_t type = co_sub_get_type(sub);
	union co_val val;
	if (__unlikely(co_sdo_req_dn(req, type, &val, &ac) == -1))
		return ac;

	if (__unlikely(co_sub_get_subidx(sub))) {
		ac = CO_SDO_AC_NO_SUB;
		goto error;
	}

	assert(type == CO_DEFTYPE_UNSIGNED32);
	co_unsigned32_t startup = val.u32;
	co_unsigned32_t startup_old = co_sub_get_val_u32(sub);
	if (startup == startup_old)
		goto error;

	// Only bits 0..4 and 6 are supported.
	if (__unlikely((startup ^ startup_old) != 0x5f)) {
		ac = CO_SDO_AC_NO_WO;
		goto error;
	}

	co_sub_dn(sub, &val);
error:
	co_val_fini(type, &val);
	return ac;
}

static int
co_nmt_recv_000(const struct can_msg *msg, void *data)
{
	assert(msg);
	co_nmt_t *nmt = data;
	assert(nmt);

	if (__unlikely(msg->len < 2))
		return 0;
	co_unsigned8_t cs = msg->data[0];
	co_unsigned8_t id = msg->data[1];

	// Ignore NMT commands to other nodes.
	if (id && id != co_dev_get_id(nmt->dev))
		return 0;

	co_nmt_emit_cs(nmt, cs);

	return 0;
}

static int
co_nmt_recv_700(const struct can_msg *msg, void *data)
{
	assert(msg);
	co_nmt_t *nmt = data;
	assert(nmt);
	assert(nmt->gt && nmt->ltf);

	// Respond with the state and flip the toggle bit.
	co_nmt_send_res(nmt, nmt->st);
	nmt->st ^= CO_NMT_ST_TOGGLE;

	// Reset the life guarding timer.
	can_timer_timeout(nmt->timer, nmt->net, nmt->gt * nmt->ltf);

	if (nmt->lg_state == CO_NMT_EC_OCCURRED) {
		// Notify the user of the resolution of a life guarding error.
		nmt->lg_state = CO_NMT_EC_RESOLVED;
		if (nmt->lg_ind)
			nmt->lg_ind(nmt, nmt->lg_state, nmt->lg_data);
	}

	return 0;
}

static int
co_nmt_timer(const struct timespec *tp, void *data)
{
	__unused_var(tp);
	co_nmt_t *nmt = data;
	assert(nmt);

	if (nmt->ms) {
		// Send the state of the NMT service (excluding the toggle bit).
		co_nmt_send_res(nmt, nmt->st & ~CO_NMT_ST_TOGGLE);
	} else if (nmt->gt && nmt->ltf) {
		// Notify the user of the occurrence of a life guarding error.
		nmt->lg_state = CO_NMT_EC_OCCURRED;
		if (nmt->lg_ind)
			nmt->lg_ind(nmt, nmt->lg_state, nmt->lg_data);
	}

	return 0;
}

static void
default_lg_ind(co_nmt_t *nmt, int state, void *data)
{
	assert(nmt);
	__unused_var(data);

	if (state == CO_NMT_EC_OCCURRED)
		co_nmt_comm_err_ind(nmt);
}

static void
co_nmt_hb_ind(co_nmt_t *nmt, co_unsigned8_t id, int state, void *data)
{
	assert(nmt);
	__unused_var(data);

	if (nmt->hb_ind)
		nmt->hb_ind(nmt, id, state, nmt->hb_data);
}

static void
co_nmt_st_ind(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, void *data)
{
	assert(nmt);
	__unused_var(data);

	if (nmt->st_ind)
		nmt->st_ind(nmt, id, st, nmt->st_data);
}

static void
default_hb_ind(co_nmt_t *nmt, co_unsigned8_t id, int state, void *data)
{
	assert(nmt);
	__unused_var(data);

	if (state == CO_NMT_EC_OCCURRED) {
		__unused_var(id);
		co_nmt_comm_err_ind(nmt);
	}
}

static inline void
co_nmt_enter(co_nmt_t *nmt, co_nmt_state_t *next)
{
	assert(nmt);
	assert(nmt->state);

	while (next) {
		co_nmt_state_t *prev = nmt->state;
		nmt->state = next;

		if (prev->on_leave)
			prev->on_leave(nmt);

		next = next->on_enter ? next->on_enter(nmt) : NULL;
	}
}

static inline void
co_nmt_emit_cs(co_nmt_t *nmt, co_unsigned8_t cs)
{
	assert(nmt);
	assert(nmt->state);
	assert(nmt->state->on_cs);

	co_nmt_enter(nmt, nmt->state->on_cs(nmt, cs));
}

static co_nmt_state_t *
co_nmt_init_on_cs(co_nmt_t *nmt, co_unsigned8_t cs)
{
	__unused_var(nmt);

	switch (cs) {
	case CO_NMT_CS_RESET_NODE:
		return co_nmt_reset_node_state;
	default:
		return NULL;
	}
}

static co_nmt_state_t *
co_nmt_reset_node_on_enter(co_nmt_t *nmt)
{
	assert(nmt);

	nmt->st = CO_NMT_ST_BOOTUP;

	// Disable all services.
	co_nmt_srv_set(&nmt->srv, nmt->net, nmt->dev, 0);

	// Disable heartbeat consumption.
	co_nmt_hb_fini(nmt);

	// Disable error control services.
	co_nmt_ec_fini(nmt);

	// Stop receiving NMT commands.
	can_recv_stop(nmt->recv_000);

	// Reset application parameters.
	if (__unlikely(co_dev_read_dcf(nmt->dev, NULL, NULL, &nmt->dcf_node)
			== -1))
		diag(DIAG_ERROR, get_errc(), "unable to reset application parameters");

	if (nmt->cs_ind)
		nmt->cs_ind(nmt, CO_NMT_CS_RESET_NODE, nmt->cs_data);

	// Automatically switch to the 'reset communication' state.
	return co_nmt_reset_comm_state;
}

static co_nmt_state_t *
co_nmt_reset_comm_on_enter(co_nmt_t *nmt)
{
	assert(nmt);

	nmt->st = CO_NMT_ST_BOOTUP;

	// Disable all services.
	co_nmt_srv_set(&nmt->srv, nmt->net, nmt->dev, 0);

	// Disable heartbeat consumption.
	co_nmt_hb_fini(nmt);

	// Disable error control services.
	co_nmt_ec_fini(nmt);

	// Stop receiving NMT commands.
	can_recv_stop(nmt->recv_000);

	// Reset communication parameters.
	if (__unlikely(co_dev_read_dcf(nmt->dev, NULL, NULL, &nmt->dcf_comm)
			== -1))
		diag(DIAG_ERROR, get_errc(), "unable to reset communication parameters");

	// Update the Node-ID if necessary.
	if (nmt->id != co_dev_get_id(nmt->dev)) {
		co_dev_set_id(nmt->dev, nmt->id);
		if (__unlikely(co_dev_write_dcf(nmt->dev, 0x1000, 0x1fff,
				&nmt->dcf_comm) == -1))
			diag(DIAG_ERROR, get_errc(), "unable to store communication parameters");
	}

	// Load the NMT start-up value.
	nmt->startup = co_dev_get_val_u32(nmt->dev, 0x1f80, 0x00);

	// Start receiving NMT commands.
	can_recv_start(nmt->recv_000, nmt->net, 0x000, 0);

	// Remain in the 'reset communication' state if the Node-ID is invalid.
	if (nmt->id == 0xff)
		return NULL;

	// Enable error control services.
	co_nmt_ec_init(nmt);

	// Enable heartbeat consumption.
	co_nmt_hb_init(nmt);

	// Send the boot-up signal to notify the master we exist.
	co_nmt_send_res(nmt, nmt->st);

	if (nmt->cs_ind)
		nmt->cs_ind(nmt, CO_NMT_CS_RESET_COMM, nmt->cs_data);

	// Automatically switch to the 'pre-operational' state.
	return co_nmt_preop_state;
}

static co_nmt_state_t *
co_nmt_reset_comm_on_cs(co_nmt_t *nmt, co_unsigned8_t cs)
{
	__unused_var(nmt);

	switch (cs) {
	case CO_NMT_CS_RESET_NODE:
		return co_nmt_reset_node_state;
	case CO_NMT_CS_RESET_COMM:
		return co_nmt_reset_comm_state;
	default:
		return NULL;
	}
}

static co_nmt_state_t *
co_nmt_preop_on_enter(co_nmt_t *nmt)
{
	assert(nmt);

	nmt->st = CO_NMT_ST_PREOP | (nmt->st & CO_NMT_ST_TOGGLE);

	// Enable all services except PDO.
	co_nmt_srv_set(&nmt->srv, nmt->net, nmt->dev, CO_NMT_PREOP_SRV);

	if (nmt->cs_ind)
		nmt->cs_ind(nmt, CO_NMT_CS_ENTER_PREOP, nmt->cs_data);

	return co_nmt_slave_state;
}

static co_nmt_state_t *
co_nmt_start_on_enter(co_nmt_t *nmt)
{
	assert(nmt);

	nmt->st = CO_NMT_ST_START | (nmt->st & CO_NMT_ST_TOGGLE);

	// Enable all services.
	co_nmt_srv_set(&nmt->srv, nmt->net, nmt->dev, CO_NMT_START_SRV);

	if (nmt->cs_ind)
		nmt->cs_ind(nmt, CO_NMT_CS_START, nmt->cs_data);

	return NULL;
}

static co_nmt_state_t *
co_nmt_start_on_cs(co_nmt_t *nmt, co_unsigned8_t cs)
{
	__unused_var(nmt);

	switch (cs) {
	case CO_NMT_CS_STOP:
		return co_nmt_stop_state;
	case CO_NMT_CS_ENTER_PREOP:
		return co_nmt_preop_state;
	case CO_NMT_CS_RESET_NODE:
		return co_nmt_reset_node_state;
	case CO_NMT_CS_RESET_COMM:
		return co_nmt_reset_comm_state;
	default:
		return NULL;
	}
}

static co_nmt_state_t *
co_nmt_stop_on_enter(co_nmt_t *nmt)
{
	assert(nmt);

	nmt->st = CO_NMT_ST_STOP | (nmt->st & CO_NMT_ST_TOGGLE);

	// Disable all services.
	co_nmt_srv_set(&nmt->srv, nmt->net, nmt->dev, CO_NMT_STOP_SRV);

	if (nmt->cs_ind)
		nmt->cs_ind(nmt, CO_NMT_CS_STOP, nmt->cs_data);

	return NULL;
}

static co_nmt_state_t *
co_nmt_stop_on_cs(co_nmt_t *nmt, co_unsigned8_t cs)
{
	__unused_var(nmt);

	switch (cs) {
	case CO_NMT_CS_START:
		return co_nmt_start_state;
	case CO_NMT_CS_ENTER_PREOP:
		return co_nmt_preop_state;
	case CO_NMT_CS_RESET_NODE:
		return co_nmt_reset_node_state;
	case CO_NMT_CS_RESET_COMM:
		return co_nmt_reset_comm_state;
	default:
		return NULL;
	}
}

static co_nmt_state_t *
co_nmt_slave_on_enter(co_nmt_t *nmt)
{
	__unused_var(nmt);

	return co_nmt_autostart_state;
}

static co_nmt_state_t *
co_nmt_autostart_on_enter(co_nmt_t *nmt)
{
	assert(nmt);

	// Enter the operational state automatically if bit 2 of the NMT
	// start-up value is 0.
	return (nmt->startup & 0x04) ? NULL : co_nmt_start_state;
}

static co_nmt_state_t *
co_nmt_autostart_on_cs(co_nmt_t *nmt, co_unsigned8_t cs)
{
	__unused_var(nmt);

	switch (cs) {
	case CO_NMT_CS_START:
		return co_nmt_start_state;
	case CO_NMT_CS_STOP:
		return co_nmt_stop_state;
	case CO_NMT_CS_RESET_NODE:
		return co_nmt_reset_node_state;
	case CO_NMT_CS_RESET_COMM:
		return co_nmt_reset_comm_state;
	default:
		return NULL;
	}
}

static void
co_nmt_ec_init(co_nmt_t *nmt)
{
	assert(nmt);

	// Enable life guarding or heartbeat production.
	nmt->gt = co_dev_get_val_u16(nmt->dev, 0x100c, 0x00);
	nmt->ltf = co_dev_get_val_u8(nmt->dev, 0x100d, 0x00);
	nmt->ms = co_dev_get_val_u16(nmt->dev, 0x1017, 0x00);

	if (__unlikely(co_nmt_update(nmt) == -1))
		diag(DIAG_ERROR, get_errc(), "unable to start life guarding or heartbeat production");
}

static void
co_nmt_ec_fini(co_nmt_t *nmt)
{
	assert(nmt);

	// Disable life guarding and heartbeat production.
	nmt->gt = 0;
	nmt->ltf = 0;
	nmt->ms = 0;

	nmt->lg_state = CO_NMT_EC_RESOLVED;

	co_nmt_update(nmt);
}

static void
co_nmt_hb_init(co_nmt_t *nmt)
{
	assert(nmt);

	// Create and initialize the heartbeat consumers.
	assert(!nmt->hbs);
	assert(!nmt->nhb);
	co_obj_t *obj_1016 = co_dev_find_obj(nmt->dev, 0x1016);
	if (obj_1016) {
		nmt->nhb = co_obj_get_val_u8(obj_1016, 0);
		nmt->hbs = calloc(nmt->nhb, sizeof(*nmt->hbs));
		if (__unlikely(nmt->nhb && !nmt->hbs)) {
			nmt->nhb = 0;
			diag(DIAG_ERROR, get_errc(), "unable to create heartbeat consumers");
		}
	}

	for (size_t i = 0; i < nmt->nhb; i++) {
		nmt->hbs[i] = co_nmt_hb_create(nmt->net, nmt->dev, nmt);
		if (__unlikely(!nmt->hbs[i])) {
			diag(DIAG_ERROR, get_errc(), "unable to create heartbeat consumer 0x%02X",
					(co_unsigned8_t)(i + 1));
			continue;
		}
		co_nmt_hb_set_hb_ind(nmt->hbs[i], &co_nmt_hb_ind, NULL);
		co_nmt_hb_set_st_ind(nmt->hbs[i], &co_nmt_st_ind, NULL);

		co_unsigned32_t val = co_obj_get_val_u32(obj_1016, i + 1);
		co_unsigned8_t id = (val >> 16) & 0xff;
		co_unsigned16_t ms = val & 0xffff;
		co_nmt_hb_set_1016(nmt->hbs[i], id, ms);
	}
}

static void
co_nmt_hb_fini(co_nmt_t *nmt)
{
	assert(nmt);

	// Destroy all heartbeat consumers.
	for (size_t i = 0; i < nmt->nhb; i++)
		co_nmt_hb_destroy(nmt->hbs[i]);
	free(nmt->hbs);
	nmt->hbs = NULL;
	nmt->nhb = 0;
}

static int
co_nmt_send_res(co_nmt_t *nmt, co_unsigned8_t st)
{
	assert(nmt);

	struct can_msg msg = CAN_MSG_INIT;
	msg.id = 0x700 + co_dev_get_id(nmt->dev);
	msg.len = 1;
	msg.data[0] = st;

	return can_net_send(nmt->net, &msg);
}

