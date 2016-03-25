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
#ifndef LELY_NO_CO_NMT_MASTER
#include "nmt_boot.h"
#endif
#include "nmt_ec.h"
#include "nmt_srv.h"

#include <assert.h>
#include <stdlib.h>

#ifndef LELY_CO_NMT_BOOT_TIMEOUT
//! The SDO timeout (in milliseconds) for the NMT 'boot slave' process.
#define LELY_CO_NMT_BOOT_TIMEOUT	100
#endif

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
#ifndef LELY_NO_CO_NMT_MASTER
	/*!
	 * A pointer to the transition function invoked when an 'boot slave'
	 * process completes.
	 *
	 * \param nmt a pointer to an NMT master service.
	 * \param id  the Node-ID of the slave.
	 * \param st  the state of the node (including the toggle bit).
	 * \param es  the error status (in the range ['A'..'O'], or 0 on
	 *            success).
	 *
	 * \returns a pointer to the next state.
	 */
	co_nmt_state_t *(*on_boot)(co_nmt_t *nmt, co_unsigned8_t id,
			co_unsigned8_t st, char es);
#endif
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
#ifndef LELY_NO_CO_NMT_MASTER
	//! A flag specifying whether the NMT service is a master or a slave.
	int master;
#endif
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
#ifndef LELY_NO_CO_NMT_MASTER
	//! An array of pointers to the NMT 'boot slave' services.
	co_nmt_boot_t *boot[CO_NUM_NODES];
	//! A pointer to the NMT 'boot slave' indication function.
	co_nmt_boot_ind_t *boot_ind;
	//! A pointer to user-specified data for #boot_ind.
	void *boot_data;
	//! A pointer to the 'download software' indication function.
	co_nmt_req_ind_t *dn_sw_ind;
	//! A pointer to user-specified data for #dn_sw_ind.
	void *dn_sw_data;
	//! A pointer to the 'download configuration' indication function.
	co_nmt_req_ind_t *dn_cfg_ind;
	//! A pointer to user-specified data for #dn_cfg_ind.
	void *dn_cfg_data;
#endif
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

#ifndef LELY_NO_CO_NMT_MASTER

/*!
 * The CANopen NMT 'boot slave' confirmation callback function.
 *
 * \see co_nmt_boot_ind_t
 */
static void co_nmt_boot_con(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st,
		char es, void *data);

/*!
 * The CANopen NMT 'download software' indication function.
 *
 * \see co_nmt_req_ind_t
 */
static void co_nmt_dn_sw_ind(co_nmt_t *nmt, co_unsigned8_t id, co_csdo_t *sdo,
		void *data);

/*!
 * The CANopen NMT 'download configuration' indication function.
 *
 * \see co_nmt_req_ind_t
 */
static void co_nmt_dn_cfg_ind(co_nmt_t *nmt, co_unsigned8_t id, co_csdo_t *sdo,
		void *data);

#endif

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

#ifndef LELY_NO_CO_NMT_MASTER
/*!
 * Invokes the 'boot slave completed' transition function of the current state
 * of an NMT master service.
 *
 * \param nmt a pointer to an NMT master service.
 * \param id  the Node-ID of the slave.
 * \param st  the state of the node (including the toggle bit).
 * \param es  the error status (in the range ['A'..'O'], or 0 on success).
 */
static inline void co_nmt_emit_boot(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned8_t st, char es);
#endif

//! The 'NMT command received' transition function of the 'initializing' state.
static co_nmt_state_t *co_nmt_init_on_cs(co_nmt_t *nmt, co_unsigned8_t cs);

//! The 'initializing' state.
static const co_nmt_state_t *co_nmt_init_state = &(co_nmt_state_t){
	NULL,
	&co_nmt_init_on_cs,
#ifndef LELY_NO_CO_NMT_MASTER
	NULL,
#endif
	NULL
};

//! The entry function of the 'reset application' state.
static co_nmt_state_t *co_nmt_reset_node_on_enter(co_nmt_t *nmt);

//! The NMT 'reset application' state.
static const co_nmt_state_t *co_nmt_reset_node_state = &(co_nmt_state_t){
	&co_nmt_reset_node_on_enter,
	NULL,
#ifndef LELY_NO_CO_NMT_MASTER
	NULL,
#endif
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
#ifndef LELY_NO_CO_NMT_MASTER
	NULL,
#endif
	NULL
};

//! The entry function of the 'pre-operational' state.
static co_nmt_state_t *co_nmt_preop_on_enter(co_nmt_t *nmt);

//! The NMT 'pre-operational' state.
static const co_nmt_state_t *co_nmt_preop_state = &(co_nmt_state_t){
	&co_nmt_preop_on_enter,
	NULL,
#ifndef LELY_NO_CO_NMT_MASTER
	NULL,
#endif
	NULL
};

//! The entry function of the 'operational' state.
static co_nmt_state_t *co_nmt_start_on_enter(co_nmt_t *nmt);

//! The 'NMT command received' transition function of the 'operational' state.
static co_nmt_state_t *co_nmt_start_on_cs(co_nmt_t *nmt, co_unsigned8_t cs);

#ifndef LELY_NO_CO_NMT_MASTER
//! The 'boot slave completed' transition function of the 'operational' state.
static co_nmt_state_t *co_nmt_start_on_boot(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned8_t st, char es);
#endif

//! The NMT 'operational' state.
static const co_nmt_state_t *co_nmt_start_state = &(co_nmt_state_t){
	&co_nmt_start_on_enter,
	&co_nmt_start_on_cs,
#ifndef LELY_NO_CO_NMT_MASTER
	&co_nmt_start_on_boot,
#endif
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
#ifndef LELY_NO_CO_NMT_MASTER
	NULL,
#endif
	NULL
};

#ifndef LELY_NO_CO_NMT_MASTER

//! The entry function of the 'master boot-up procedure' state.
static co_nmt_state_t *co_nmt_master_on_enter(co_nmt_t *nmt);

/*!
 * The 'NMT command received' transition function of the 'master boot-up
 * procedure' state.
 */
static co_nmt_state_t *co_nmt_master_on_cs(co_nmt_t *nmt, co_unsigned8_t cs);

/*!
 * The 'boot slave completed' transition function of the 'master boot-up
 * procedure' state.
 */
static co_nmt_state_t *co_nmt_master_on_boot(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned8_t st, char es);

/*!
 * The 'master boot-up procedure' state (see Fig. 1 & 2 in CiA DSP-302-2
 * V4.1.0).
 */
static const co_nmt_state_t *co_nmt_master_state = &(co_nmt_state_t){
	&co_nmt_master_on_enter,
	&co_nmt_master_on_cs,
	&co_nmt_master_on_boot,
	NULL
};

#endif

//! The entry function of the 'slave boot-up procedure' state.
static co_nmt_state_t *co_nmt_slave_on_enter(co_nmt_t *nmt);

//! The 'slave boot-up procedure' state (see Fig. 1 in CiA DSP-302-2 V3.2.1).
static const co_nmt_state_t *co_nmt_slave_state = &(co_nmt_state_t){
	&co_nmt_slave_on_enter,
	NULL,
#ifndef LELY_NO_CO_NMT_MASTER
	NULL,
#endif
	NULL
};

//! The entry function of the 'autostart' state.
static co_nmt_state_t *co_nmt_autostart_on_enter(co_nmt_t *nmt);

//! The 'NMT command received' transition function of the 'autostart' state.
static co_nmt_state_t *co_nmt_autostart_on_cs(co_nmt_t *nmt, co_unsigned8_t cs);

#ifndef LELY_NO_CO_NMT_MASTER
//! The 'boot slave completed' transition function of the 'autostart' state.
static co_nmt_state_t *co_nmt_autostart_on_boot(co_nmt_t *nmt,
		co_unsigned8_t id, co_unsigned8_t st, char es);
#endif

//! The 'autostart' state (see Fig. 1 & 2 in CiA DSP-302-2 V4.1.0).
static const co_nmt_state_t *co_nmt_autostart_state = &(co_nmt_state_t){
	&co_nmt_autostart_on_enter,
	&co_nmt_autostart_on_cs,
#ifndef LELY_NO_CO_NMT_MASTER
	&co_nmt_autostart_on_boot,
#endif
	NULL
};

#ifndef LELY_NO_CO_NMT_MASTER

/*!
 * The 'NMT command received' transition function of the 'halt network boot-up'
 * state.
 */
static co_nmt_state_t *co_nmt_halt_on_cs(co_nmt_t *nmt, co_unsigned8_t cs);

/*!
 * The 'boot slave completed' transition function of the 'halt network boot-up'
 * state.
 */
static co_nmt_state_t *co_nmt_halt_on_boot(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned8_t st, char es);

//! The 'halt network boot-up' state.
static const co_nmt_state_t *co_nmt_halt_state = &(co_nmt_state_t){
	NULL,
	&co_nmt_halt_on_cs,
	&co_nmt_halt_on_boot,
	NULL
};

#endif

//! Initializes the error control services. \see co_nmt_ec_fini()
static void co_nmt_ec_init(co_nmt_t *nmt);

//! Finalizes the error control services. \see co_nmt_ec_init()
static void co_nmt_ec_fini(co_nmt_t *nmt);

//! Initializes the heartbeat consumer services. \see co_nmt_hb_fini()
static void co_nmt_hb_init(co_nmt_t *nmt);

//! Finalizes the heartbeat consumer services. \see co_nmt_hb_init()
static void co_nmt_hb_fini(co_nmt_t *nmt);

#ifndef LELY_NO_CO_NMT_MASTER

/*!
 * Starts the NMT 'boot slave' process.
 *
 * \returns 1 if at least one mandatory slave is booting, 0 if there are no
 * mandatory slaves, or -1 if an error occurred for a mandatory slave.
 *
 * \see co_nmt_boot_fini()
 */
static int co_nmt_boot_init(co_nmt_t *nmt);

// Stops the NMT 'boot slave' process. \see co_nmt_boot_init()
static void co_nmt_boot_fini(co_nmt_t *nmt);

/*!
 * Processes the confirmation of a 'boot slave' process and notifies the user.
 *
 * \param nmt a pointer to an NMT master service.
 * \param id  the Node-ID of the slave.
 * \param st  the state of the node (including the toggle bit).
 * \param es  the error status (in the range ['A'..'O'], or 0 on success).
 *
 * \returns 0 on success, or -1 if the 'boot slave' process failed and the slave
 * is mandatory.
 */
static int co_nmt_boot_ind(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st,
		char es);

#endif

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

#ifndef LELY_NO_CO_NMT_MASTER
LELY_CO_EXPORT const char *
co_nmt_es_str(char es)
{
	switch (es) {
	case 'A': return "The slave no longer exists in the Network list";
	case 'B': return "No response on access to Actual Device Type received";
	case 'C': return "Actual Device Type of the slave node did not match";
	case 'D': return "Actual Vendor ID of the slave node did not match";
	case 'E':
	case 'F': return "Slave node did not respond with its state";
	case 'G': return "Application software version Date or Time were not configured";
	case 'H': return "Automatic software update was not allowed";
	case 'I': return "Automatic software update failed";
	case 'J': return "Automatic configuration download failed";
	case 'K': return "The slave node did not send its heartbeat message";
	case 'L': return "Slave was initially operational";
	case 'M': return "Actual Product Code of the slave node did not match";
	case 'N': return "Actual Revision Number of the slave node did not match";
	case 'O': return "Actual Serial Number of the slave node did not match";
	default: return "Unknown error status";
	}
}
#endif

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
#ifndef LELY_NO_CO_NMT_MASTER
	nmt->master = 0;
#endif

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

#ifndef LELY_NO_CO_NMT_MASTER
	for (co_unsigned8_t i = 0; i < CO_NUM_NODES; i++)
		nmt->boot[i] = NULL;

	nmt->boot_ind = NULL;
	nmt->boot_data = NULL;
	nmt->dn_sw_ind = NULL;
	nmt->dn_sw_data = NULL;
	nmt->dn_cfg_ind = NULL;
	nmt->dn_cfg_data = NULL;
#endif

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

#ifndef LELY_NO_CO_NMT_MASTER
	co_nmt_boot_fini(nmt);
#endif

	co_nmt_hb_fini(nmt);

	co_nmt_ec_fini(nmt);

	can_timer_destroy(nmt->timer);
	can_recv_destroy(nmt->recv_700);

	can_recv_destroy(nmt->recv_000);

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

#ifndef LELY_NO_CO_NMT_MASTER

LELY_CO_EXPORT void
co_nmt_get_boot_ind(const co_nmt_t *nmt, co_nmt_boot_ind_t **pind, void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->boot_ind;
	if (pdata)
		*pdata = nmt->boot_data;
}

LELY_CO_EXPORT void
co_nmt_set_boot_ind(co_nmt_t *nmt, co_nmt_boot_ind_t *ind, void *data)
{
	assert(nmt);

	nmt->boot_ind = ind;
	nmt->boot_data = data;
}

LELY_CO_EXPORT void
co_nmt_get_dn_sw_ind(const co_nmt_t *nmt, co_nmt_req_ind_t **pind,
		void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->dn_sw_ind;
	if (pdata)
		*pdata = nmt->dn_sw_data;
}

LELY_CO_EXPORT void
co_nmt_set_dn_sw_ind(co_nmt_t *nmt, co_nmt_req_ind_t *ind, void *data)
{
	assert(nmt);

	nmt->dn_sw_ind = ind;
	nmt->dn_sw_data = data;
}

LELY_CO_EXPORT void
co_nmt_get_dn_cfg_ind(const co_nmt_t *nmt, co_nmt_req_ind_t **pind,
		void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->dn_cfg_ind;
	if (pdata)
		*pdata = nmt->dn_cfg_data;
}

LELY_CO_EXPORT void
co_nmt_set_dn_cfg_ind(co_nmt_t *nmt, co_nmt_req_ind_t *ind, void *data)
{
	assert(nmt);

	nmt->dn_cfg_ind = ind;
	nmt->dn_cfg_data = data;
}

#endif // !LELY_NO_CO_NMT_MASTER

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
co_nmt_is_master(const co_nmt_t *nmt)
{
#ifdef LELY_NO_CO_NMT_MASTER
	__unused_var(nmt);

	return 0;
#else
	assert(nmt);

	return nmt->master;
#endif
}

#ifndef LELY_NO_CO_NMT_MASTER
LELY_CO_EXPORT int
co_nmt_cs_req(co_nmt_t *nmt, co_unsigned8_t cs, co_unsigned8_t id)
{
	assert(nmt);

	if (__unlikely(!nmt->master)) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

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

	if (__unlikely(id > CO_NUM_NODES)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	struct can_msg msg = CAN_MSG_INIT;
	msg.id = 0x000;
	msg.len = 2;
	msg.data[0] = cs;
	msg.data[1] = id;

	return can_net_send(nmt->net, &msg);
}
#endif

#ifndef LELY_NO_CO_NMT_MASTER

LELY_CO_EXPORT int
co_nmt_boot_req(co_nmt_t *nmt, co_unsigned8_t id, int timeout)
{
	assert(nmt);

	errc_t errc = 0;

	if (__unlikely(!nmt->master)) {
		errc = errnum2c(ERRNUM_PERM);
		goto error_param;
	}

	if (__unlikely(!id || id > CO_NUM_NODES)) {
		errc = errnum2c(ERRNUM_INVAL);
		goto error_param;
	}

	if (!nmt->boot[id - 1]) {
		nmt->boot[id - 1] = co_nmt_boot_create(nmt->net, nmt->dev, nmt);
		if (__unlikely(!nmt->boot[id - 1])) {
			errc = get_errc();
			goto error_create_boot;
		}
	}
	co_nmt_boot_set_dn_sw_ind(nmt->boot[id - 1], &co_nmt_dn_sw_ind, NULL);
	co_nmt_boot_set_dn_cfg_ind(nmt->boot[id - 1], &co_nmt_dn_cfg_ind, NULL);

	if (__unlikely(co_nmt_boot_boot_req(nmt->boot[id - 1], id, timeout,
			&co_nmt_boot_con, NULL) == -1)) {
		errc = get_errc();
		goto error_boot_req;
	}

	// Disable the heartbeat consumer service for the node.
	co_obj_t *obj_1016 = co_dev_find_obj(nmt->dev, 0x1016);
	for (size_t i = 0; i < nmt->nhb; i++) {
		co_unsigned32_t val = co_obj_get_val_u32(obj_1016, i + 1);
		if (id != ((val >> 16) & 0xff))
			continue;
		co_nmt_hb_set_1016(nmt->hbs[i], 0, 0);
	}

	return 0;

error_boot_req:
	co_nmt_boot_destroy(nmt->boot[id - 1]);
	nmt->boot[id - 1] = NULL;
error_create_boot:
error_param:
	set_errc(errc);
	return -1;
}

LELY_CO_EXPORT int
co_nmt_req_res(co_nmt_t *nmt, co_unsigned8_t id, int res)
{
	assert(nmt);

	if (__unlikely(!nmt->master)) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	if (__unlikely(!id || id > CO_NUM_NODES)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	co_nmt_boot_req_res(nmt->boot[id - 1], res);
	return 0;
}

#endif // !LELY_NO_CO_NMT_MASTER

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

#ifndef LELY_NO_CO_NMT_MASTER
LELY_CO_EXPORT int
co_nmt_node_err_ind(co_nmt_t *nmt, co_unsigned8_t id)
{
	assert(nmt);

	if (!nmt->master)
		return 0;

	if (__unlikely(!id || id > CO_NUM_NODES)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	co_unsigned32_t assignment = co_dev_get_val_u32(nmt->dev, 0x1f81, id);
	// Ignore the error event if the slave is no longer in the network list.
	if (!(assignment & 0x01))
		return 0;
	int mandatory = !!(assignment & 0x08);

	if (mandatory && (nmt->startup & 0x40)) {
		// If the slave is mandatory and bit 6 of the NMT start-up value
		// is set, stop all nodes, including the master.
		co_nmt_cs_req(nmt, CO_NMT_CS_STOP, 0);
		return co_nmt_cs_ind(nmt, CO_NMT_CS_STOP);
	} else if (mandatory && (nmt->startup & 0x10)) {
		// If the slave is mandatory and bit 4 of the NMT start-up value
		// is set, reset all nodes, including the master.
		co_nmt_cs_req(nmt, CO_NMT_CS_RESET_NODE, 0);
		return co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE);
	} else {
		// If the slave is not mandatory, or bits 4 and 6 of the NMT
		// start-up value are zero, reset the node individually.
		co_nmt_cs_req(nmt, CO_NMT_CS_RESET_NODE, id);
		if (!(assignment & 0x04))
			return 0;
		return co_nmt_boot_req(nmt, id, LELY_CO_NMT_BOOT_TIMEOUT);
	}
}
#endif

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
#ifndef LELY_NO_CO_NMT_MASTER
	assert(!nmt->master);
#endif

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
	__unused_var(msg);
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
#ifdef LELY_NO_CO_NMT_MASTER
		__unused_var(id);
		co_nmt_comm_err_ind(nmt);
#else
		if (co_nmt_is_master(nmt))
			co_nmt_node_err_ind(nmt, id);
		else
			co_nmt_comm_err_ind(nmt);
#endif
	}
}

#ifndef LELY_NO_CO_NMT_MASTER

static void
co_nmt_boot_con(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, char es,
		void *data)
{
	assert(nmt);
	assert(nmt->master);
	assert(nmt->boot[id - 1]);
	assert(id && id <= CO_NUM_NODES);
	__unused_var(data);

	co_nmt_boot_destroy(nmt->boot[id - 1]);
	nmt->boot[id - 1] = NULL;

	co_nmt_emit_boot(nmt, id, st, es);
}

static void
co_nmt_dn_sw_ind(co_nmt_t *nmt, co_unsigned8_t id, co_csdo_t *sdo, void *data)
{
	assert(nmt);
	assert(nmt->master);
	assert(nmt->boot[id - 1]);
	assert(id && id <= CO_NUM_NODES);
	assert(sdo);
	__unused_var(data);

	if (nmt->dn_sw_ind)
		nmt->dn_sw_ind(nmt, id, sdo, nmt->dn_sw_data);
	else
		co_nmt_boot_req_res(nmt->boot[id - 1], -1);
}

static void
co_nmt_dn_cfg_ind(co_nmt_t *nmt, co_unsigned8_t id, co_csdo_t *sdo, void *data)
{
	assert(nmt);
	assert(nmt->master);
	assert(nmt->boot[id - 1]);
	assert(id && id <= CO_NUM_NODES);
	assert(sdo);
	__unused_var(data);

	if (nmt->dn_cfg_ind)
		nmt->dn_cfg_ind(nmt, id, sdo, nmt->dn_cfg_data);
	else
		co_nmt_boot_req_res(nmt->boot[id - 1], -1);
}

#endif

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

#ifndef LELY_NO_CO_NMT_MASTER
static inline void
co_nmt_emit_boot(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, char es)
{
	assert(nmt);
	assert(nmt->state);
	assert(nmt->state->on_boot);

	co_nmt_enter(nmt, nmt->state->on_boot(nmt, id, st, es));
}
#endif

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

#ifndef LELY_NO_CO_NMT_MASTER
	// Destroy the NMT 'boot slave' services.
	co_nmt_boot_fini(nmt);
#endif

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

#ifndef LELY_NO_CO_NMT_MASTER
	// Destroy the NMT 'boot slave' services.
	co_nmt_boot_fini(nmt);
#endif

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
#ifndef LELY_NO_CO_NMT_MASTER
	// Bit 0 of the NMT start-up value determines whether we are a
	// master or a slave.
	nmt->master = !!(nmt->startup & 0x01);
#endif

	// Start receiving NMT commands.
	if (!co_nmt_is_master(nmt))
		can_recv_start(nmt->recv_000, nmt->net, 0x000, 0);

	// Remain in the 'reset communication' state if the Node-ID is invalid.
	if (nmt->id == 0xff)
		return NULL;

	// Enable error control services.
	co_nmt_ec_init(nmt);

	// Enable heartbeat consumption.
	co_nmt_hb_init(nmt);

	// Send the boot-up signal to notify the master we exist.
	if (!co_nmt_is_master(nmt))
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

#ifdef LELY_NO_CO_NMT_MASTER
	return co_nmt_slave_state;
#else
	return nmt->master ? co_nmt_master_state : co_nmt_slave_state;
#endif
}

static co_nmt_state_t *
co_nmt_start_on_enter(co_nmt_t *nmt)
{
	assert(nmt);

	nmt->st = CO_NMT_ST_START | (nmt->st & CO_NMT_ST_TOGGLE);

	// Enable all services.
	co_nmt_srv_set(&nmt->srv, nmt->net, nmt->dev, CO_NMT_START_SRV);

#ifndef LELY_NO_CO_NMT_MASTER
	// If we're the master and bit 3 of the NMT start-up value is 0 and bit
	// 1 is 1, send the NMT start remote node command to all nodes.
	if (nmt->master && !(nmt->startup & 0x08) && (nmt->startup & 0x02))
		co_nmt_cs_req(nmt, CO_NMT_CS_START, 0);
#endif

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

#ifndef LELY_NO_CO_NMT_MASTER
static co_nmt_state_t *
co_nmt_start_on_boot(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st,
		char es)
{
	assert(nmt);

	if (nmt->master)
		co_nmt_boot_ind(nmt, id, st, es);
	return NULL;
}
#endif

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

#ifndef LELY_NO_CO_NMT_MASTER

static co_nmt_state_t *
co_nmt_master_on_enter(co_nmt_t *nmt)
{
	assert(nmt);
	assert(nmt->master);

	// TODO: Implement flying master process.

	co_obj_t *obj_1f81 = co_dev_find_obj(nmt->dev, 0x1f81);

	// Check if any node has the keep-alive bit set.
	int keep = 0;
	for (co_unsigned8_t id = 1; !keep && id <= CO_NUM_NODES; id++)
		keep = (co_obj_get_val_u32(obj_1f81, id) & 0x11) == 0x11;

	// Send the NMT 'reset communication' command to slaves with the
	// keep-alive bit not set.
	if (keep) {
		for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++) {
			if ((co_obj_get_val_u32(obj_1f81, id) & 0x11) != 0x11)
				co_nmt_cs_req(nmt, CO_NMT_CS_RESET_COMM, id);
		}
	} else {
		co_nmt_cs_req(nmt, CO_NMT_CS_RESET_COMM, 0);
	}

	// Start the 'boot slave' process.
	switch (co_nmt_boot_init(nmt)) {
	case -1:
		// Halt the network boot-up procedure if the 'boot slave'
		// process failed for a mandatory slave.
		return co_nmt_halt_state;
	case 0:
		return co_nmt_autostart_state;
	default:
		// Wait for all mandatory slaves to finish booting.
		return NULL;
	}
}

static co_nmt_state_t *
co_nmt_master_on_cs(co_nmt_t *nmt, co_unsigned8_t cs)
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
co_nmt_master_on_boot(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st,
		char es)
{
	assert(nmt);
	assert(nmt->master);

	// If the 'boot slave' process failed for a mandatory slave, halt the
	// network boot-up procedure.
	if (__unlikely(co_nmt_boot_ind(nmt, id, st, es) == -1))
		return co_nmt_halt_state;

	co_obj_t *obj_1f81 = co_dev_find_obj(nmt->dev, 0x1f81);

	// Wait for any mandatory slaves that have not yet finished booting.
	int wait = 0;
	for (co_unsigned8_t id = 1; !wait && id <= CO_NUM_NODES; id++)
		wait = (co_obj_get_val_u32(obj_1f81, id) & 0x0d) == 0x0d
				&& nmt->boot[id - 1];
	return wait ? NULL : co_nmt_autostart_state;
}

#endif // !LELY_NO_CO_NMT_MASTER

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

#ifndef LELY_NO_CO_NMT_MASTER

static co_nmt_state_t *
co_nmt_autostart_on_boot(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st,
		char es)
{
	assert(nmt);

	if (nmt->master)
		co_nmt_boot_ind(nmt, id, st, es);
	return NULL;
}

static co_nmt_state_t *
co_nmt_halt_on_cs(co_nmt_t *nmt, co_unsigned8_t cs)
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
co_nmt_halt_on_boot(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st,
		char es)
{
	assert(nmt);
	assert(nmt->master);

	co_nmt_boot_ind(nmt, id, st, es);
	return NULL;
}

#endif

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

#ifndef LELY_NO_CO_NMT_MASTER

static int
co_nmt_boot_init(co_nmt_t *nmt)
{
	assert(nmt);
	assert(nmt->master);

	co_nmt_boot_fini(nmt);

	co_obj_t *obj_1f81 = co_dev_find_obj(nmt->dev, 0x1f81);

	int res = 0;
	for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++) {
		co_unsigned32_t assignment = co_obj_get_val_u32(obj_1f81, id);
		// Skip those slaves that are not in the network list (bit 0),
		// or that we are not allowed to boot (bit 2).
		if ((assignment & 0x05) != 0x05)
			continue;
		int mandatory = !!(assignment & 0x08);
		// Start the 'boot slave' process.
		if (__unlikely(co_nmt_boot_req(nmt, id,
				LELY_CO_NMT_BOOT_TIMEOUT) == -1)) {
			// Halt the network boot-up procedure if the 'boot
			// slave' process failed for a mandatory slave.
			if (mandatory)
				res = -1;
		} else if (!res && mandatory) {
			// Wait for all mandatory slaves to finish booting.
			res = 1;
		}
	}
	return res;
}

static void
co_nmt_boot_fini(co_nmt_t *nmt)
{
	assert(nmt);

	for (co_unsigned8_t i = 0; i < CO_NUM_NODES; i++) {
		co_nmt_boot_destroy(nmt->boot[i]);
		nmt->boot[i] = NULL;
	}
}

static int
co_nmt_boot_ind(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, char es)
{
	assert(nmt);
	assert(nmt->master);
	assert(id && id <= CO_NUM_NODES);

	co_unsigned32_t assignment = co_dev_get_val_u32(nmt->dev, 0x1f81, id);
	int mandatory = (assignment & 0x09) == 0x09;

	// If the master is allowed to start the nodes (bit 3 of the NMT
	// start-up value) and has to start the slaves individually (bit 1) or
	// is in the operational state, send the NMT 'start' command for this
	// slave.
	if (!es && ((nmt->startup & 0x0a) == 0x0a
			|| co_nmt_get_state(nmt) == CO_NMT_ST_START))
		co_nmt_cs_req(nmt, CO_NMT_CS_START, id);

	// Enable the heartbeat consumer service for the node.
	co_obj_t *obj_1016 = co_dev_find_obj(nmt->dev, 0x1016);
	for (size_t i = 0; i < nmt->nhb; i++) {
		co_unsigned32_t val = co_obj_get_val_u32(obj_1016, i + 1);
		if (id != ((val >> 16) & 0xff))
			continue;
		co_nmt_hb_set_1016(nmt->hbs[i], id, val & 0xffff);
		// If the error control service was successfully started,
		// update the state of the node, which also sets the timeout for
		// the next heartbeat message,
		if (!es || es == 'L')
			co_nmt_hb_set_st(nmt->hbs[i], st);
	}

	if (nmt->boot_ind)
		nmt->boot_ind(nmt, id, st, es, nmt->boot_data);

	// If the 'boot slave' process failed for a mandatory slave, return an
	// error.
	return es && es != 'L' && mandatory ? -1 : 0;
}

#endif

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

