/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the network management (NMT) functions.
 *
 * @see lely/co/nmt.h
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
#include <lely/util/diag.h>
#if !LELY_NO_CO_MASTER
#include <lely/can/buf.h>
#include <lely/co/csdo.h>
#include <lely/util/time.h>
#endif
#include <lely/co/dev.h>
#if !LELY_NO_CO_EMCY
#include <lely/co/emcy.h>
#endif
#include <lely/co/nmt.h>
#include <lely/co/obj.h>
#if !LELY_NO_CO_RPDO
#include <lely/co/rpdo.h>
#endif
#include <lely/co/sdo.h>
#if !LELY_NO_CO_TPDO
#include <lely/co/tpdo.h>
#endif
#include <lely/co/val.h>
#if !LELY_NO_CO_NMT_BOOT
#include "nmt_boot.h"
#endif
#if !LELY_NO_CO_NMT_CFG
#include "nmt_cfg.h"
#endif
#include "nmt_hb.h"
#include "nmt_srv.h"

#include <assert.h>
#include <stdlib.h>
#if LELY_NO_MALLOC
#include <string.h>
#endif

#if LELY_NO_MALLOC
#ifndef CO_NMT_CAN_BUF_SIZE
/**
 * The default size of an NMT CAN frame buffer in the absence of dynamic memory
 * allocation.
 */
#define CO_NMT_CAN_BUF_SIZE 16
#endif
#ifndef CO_NMT_MAX_NHB
/**
 * The default maximum number of heartbeat consumers in the absence of dynamic
 * memory allocation. The default value equals the maximum number of CANopen
 * nodes.
 */
#define CO_NMT_MAX_NHB CO_NUM_NODES
#endif
#endif // LELY_NO_MALLOC

struct __co_nmt_state;
/// An opaque CANopen NMT state type.
typedef const struct __co_nmt_state co_nmt_state_t;

#if !LELY_NO_CO_MASTER
/// A struct containing the state of an NMT slave.
struct co_nmt_slave {
	/// A pointer to the NMT master service.
	co_nmt_t *nmt;
	/**
	 * A pointer to the CAN frame receiver for the boot-up event and node
	 * guarding messages.
	 */
	can_recv_t *recv;
#if !LELY_NO_CO_NG
	/// A pointer to the CAN timer for node guarding.
	can_timer_t *timer;
#endif
	/// The NMT slave assignment (object 1F81).
	co_unsigned32_t assignment;
	/// The expected state of the slave (excluding the toggle bit).
	co_unsigned8_t est;
	/// The received state of the slave (including the toggle bit).
	co_unsigned8_t rst;
#if !LELY_NO_CO_NMT_BOOT
	/// The error status of the 'boot slave' process.
	char es;
	/// A flag specifying whether the 'boot slave' process is in progress.
	unsigned booting : 1;
#endif
#if !LELY_NO_CO_NMT_CFG
	/**
	 * A flag specifying whether an NMT 'configuration request' is in
	 * progress.
	 */
	unsigned configuring : 1;
#endif
	/// A flag specifying whether NMT boot-up message was received from a slave.
	unsigned bootup : 1;
#if !LELY_NO_CO_NMT_BOOT
	/// A flag specifying whether the 'boot slave' process has ended.
	unsigned booted : 1;
	/// A pointer to the NMT 'boot slave' service.
	co_nmt_boot_t *boot;
#endif
#if !LELY_NO_CO_NMT_CFG
	/// A pointer to the NMT 'update configuration' service.
	co_nmt_cfg_t *cfg;
	/// A pointer to the NMT 'configuration request' confirmation function.
	co_nmt_cfg_con_t *cfg_con;
	/// A pointer to user-specified data for #cfg_con.
	void *cfg_data;
#endif
#if !LELY_NO_CO_NG
	/// The guard time (in milliseconds).
	co_unsigned16_t gt;
	/// The lifetime factor.
	co_unsigned8_t ltf;
	/// The number of unanswered node guarding RTRs.
	co_unsigned8_t rtr;
	/**
	 * Indicates whether a node guarding error occurred (#CO_NMT_EC_OCCURRED
	 * or #CO_NMT_EC_RESOLVED).
	 */
	int ng_state;
#endif
};
#endif

/// A CANopen NMT master/slave service.
struct __co_nmt {
	/// A pointer to a CAN network interface.
	can_net_t *net;
	/// A pointer to a CANopen device.
	co_dev_t *dev;
	/// The pending node-ID.
	co_unsigned8_t id;
#if !LELY_NO_CO_DCF_RESTORE
	/// The concise DCF of the application parameters.
	void *dcf_node;
#endif
	/// The concise DCF of the communication parameters.
	void *dcf_comm;
	/// The current state.
	co_nmt_state_t *state;
	/// The NMT service manager.
	struct co_nmt_srv srv;
	/// The NMT startup value (object 1F80).
	co_unsigned32_t startup;
#if !LELY_NO_CO_MASTER
	/// A flag specifying whether the NMT service is a master or a slave.
	int master;
#endif
	/// A pointer to the CAN frame receiver for NMT messages.
	can_recv_t *recv_000;
	/// A pointer to the NMT command indication function.
	co_nmt_cs_ind_t *cs_ind;
	/// A pointer to user-specified data for #cs_ind.
	void *cs_data;
	/// A pointer to the CAN frame receiver for NMT error control messages.
	can_recv_t *recv_700;
#if !LELY_NO_CO_MASTER && !LELY_NO_CO_NG
	/// A pointer to the node guarding event indication function.
	co_nmt_ng_ind_t *ng_ind;
	/// A pointer to user-specified data for #ng_ind.
	void *ng_data;
#endif
	/**
	 * A pointer to the CAN timer for life guarding or heartbeat production.
	 */
	can_timer_t *ec_timer;
	/// The state of the NMT service (including the toggle bit).
	co_unsigned8_t st;
#if !LELY_NO_CO_NG
	/// The guard time (in milliseconds).
	co_unsigned16_t gt;
	/// The lifetime factor.
	co_unsigned8_t ltf;
	/**
	 * Indicates whether a life guarding error occurred (#CO_NMT_EC_OCCURRED
	 * or #CO_NMT_EC_RESOLVED).
	 */
	int lg_state;
	/// A pointer to the life guarding event indication function.
	co_nmt_lg_ind_t *lg_ind;
	/// A pointer to user-specified data for #lg_ind.
	void *lg_data;
#endif
	/// The producer heartbeat time (in milliseconds).
	co_unsigned16_t ms;
	/// An array of pointers to the heartbeat consumers.
#if LELY_NO_MALLOC
	co_nmt_hb_t *hbs[CO_NMT_MAX_NHB];
#else
	co_nmt_hb_t **hbs;
#endif
	/// The number of heartbeat consumers.
	co_unsigned8_t nhb;
	/// A pointer to the heartbeat event indication function.
	co_nmt_hb_ind_t *hb_ind;
	/// A pointer to user-specified data for #hb_ind.
	void *hb_data;
	/// A pointer to the state change event indication function.
	co_nmt_st_ind_t *st_ind;
	/// A pointer to user-specified data for #st_ind.
	void *st_data;
#if !LELY_NO_CO_MASTER
	/// A pointer to the CAN frame buffer for NMT messages.
	struct can_buf buf;
#if LELY_NO_MALLOC
	/**
	 * The static memory buffer used by #buf in the absence of dynamic
	 * memory allocation.
	 */
	struct can_msg begin[CO_NMT_CAN_BUF_SIZE];
#endif
	/// The time at which the next NMT message may be sent.
	struct timespec inhibit;
	/// A pointer to the CAN timer for sending buffered NMT messages.
	can_timer_t *cs_timer;
#if !LELY_NO_CO_LSS
	/// A pointer to the LSS request function.
	co_nmt_lss_req_t *lss_req;
	/// A pointer to user-specified data for #lss_req.
	void *lss_data;
#endif
	/**
	 * A flag indicating if the startup procedure was halted because of a
	 * mandatory slave boot failure.
	 */
	int halt;
	/// An array containing the state of each NMT slave.
	struct co_nmt_slave slaves[CO_NUM_NODES];
	/**
	 * The default SDO timeout (in milliseconds) used during the NMT
	 * 'boot slave' and 'check configuration' processes.
	 */
	int timeout;
#if !LELY_NO_CO_NMT_BOOT
	/// A pointer to the NMT 'boot slave' indication function.
	co_nmt_boot_ind_t *boot_ind;
	/// A pointer to user-specified data for #boot_ind.
	void *boot_data;
#endif
#if !LELY_NO_CO_NMT_CFG
	/// A pointer to the NMT 'configuration request' indication function.
	co_nmt_cfg_ind_t *cfg_ind;
	/// A pointer to user-specified data for #cfg_ind.
	void *cfg_data;
#endif
	/// A pointer to the SDO download progress indication function.
	co_nmt_sdo_ind_t *dn_ind;
	/// A pointer to user-specified data for #dn_ind.
	void *dn_data;
	/// A pointer to the SDO upload progress indication function.
	co_nmt_sdo_ind_t *up_ind;
	/// A pointer to user-specified data for #up_ind.
	void *up_data;
#endif
	/// A pointer to the SYNC indication function.
	co_nmt_sync_ind_t *sync_ind;
	/// A pointer to user-specified data for #sync_ind.
	void *sync_data;
#if !LELY_NO_CO_TPDO
	/**
	 * The number of calls to co_nmt_on_tpdo_event_lock() minus the number
	 * of calls to co_nmt_on_tpdo_event_unlock().
	 */
	size_t tpdo_event_wait;
	/**
	 * A bit mask tracking all Transmit-PDO events indicated by
	 * co_nmt_on_tpdo_event() that have been postponed because
	 * #tpdo_event_wait > 0.
	 */
	unsigned long tpdo_event_mask[CO_NUM_PDOS / LONG_BIT];
#endif
};

#if !LELY_NO_CO_NG

/**
 * The download indication function for CANopen object 100C (Guard time).
 *
 * @see co_sub_dn_ind_t
 */
static co_unsigned32_t co_100c_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);

/**
 * The download indication function for CANopen object 100D (Life time factor).
 *
 * @see co_sub_dn_ind_t
 */
static co_unsigned32_t co_100d_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);

#endif // !LELY_NO_CO_NG

/**
 * The download indication function for CANopen object 1016 (Consumer heartbeat
 * time).
 *
 * @see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1016_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);

/**
 * The download indication function for CANopen object 1017 (Producer heartbeat
 * time).
 *
 * @see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1017_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);

#if !LELY_NO_CO_NMT_CFG
/**
 * The download indication function for (all sub-objects of) CANopen object 1F25
 * (Configuration request).
 *
 * @see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1f25_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);
#endif

/**
 * The download indication function for (all sub-objects of) CANopen object 1F80
 * (NMT startup).
 *
 * @see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1f80_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);

#if !LELY_NO_CO_MASTER
/**
 * The download indication function for (all sub-objects of) CANopen object 1F80
 * (Request NMT).
 *
 * @see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1f82_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);
#endif

/// The CAN receive callback function for NMT messages. @see can_recv_func_t
static int co_nmt_recv_000(const struct can_msg *msg, void *data);

/**
 * The CAN receive callback function for NMT error control (node guarding RTR)
 * messages. In case of an NMT master, this function also receives and processes
 * the boot-up events (see Fig. 13 in CiA 302-2 version 4.1.0).
 *
 * @see can_recv_func_t
 */
static int co_nmt_recv_700(const struct can_msg *msg, void *data);

#if !LELY_NO_CO_MASTER && !LELY_NO_CO_NG
/// The CAN timer callback function for node guarding. @see can_timer_func_t
static int co_nmt_ng_timer(const struct timespec *tp, void *data);
#endif

/**
 * The CAN timer callback function for life guarding or heartbeat production.
 *
 * @see can_timer_func_t
 */
static int co_nmt_ec_timer(const struct timespec *tp, void *data);

#if !LELY_NO_CO_MASTER
/**
 * The CAN timer callback function for sending buffered NMT messages.
 *
 * @see can_timer_func_t
 */
static int co_nmt_cs_timer(const struct timespec *tp, void *data);
#endif

/**
 * The indication function for state change events.
 *
 * @param nmt a pointer to an NMT master/slave service.
 * @param id  the node-ID (in the range [1..127]).
 * @param st  the state of the node.
 */
static void co_nmt_st_ind(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st);

#if !LELY_NO_CO_NG

#if !LELY_NO_CO_MASTER
/// The default node guarding event handler. @see co_nmt_ng_ind_t
static void default_ng_ind(co_nmt_t *nmt, co_unsigned8_t id, int state,
		int reason, void *data);
#endif

/// The default life guarding event handler. @see co_nmt_lg_ind_t
static void default_lg_ind(co_nmt_t *nmt, int state, void *data);

#endif // !LELY_NO_CO_NG

/// The default heartbeat event handler. @see co_nmt_hb_ind_t
static void default_hb_ind(co_nmt_t *nmt, co_unsigned8_t id, int state,
		int reason, void *data);

/// The default state change event handler. @see co_nmt_st_ind_t
static void default_st_ind(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st,
		void *data);

#if !LELY_NO_CO_NMT_BOOT || !LELY_NO_CO_NMT_CFG

/// The SDO download progress indication function. @see co_csdo_ind_t
static void co_nmt_dn_ind(const co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, size_t size, size_t nbyte, void *data);

/// The SDO upload progress indication function. @see co_csdo_ind_t
static void co_nmt_up_ind(const co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, size_t size, size_t nbyte, void *data);

#endif

#if !LELY_NO_CO_TPDO
/// The Transmit-PDO event indication function. @see co_dev_tpdo_event_ind_t
static void co_nmt_tpdo_event_ind(co_unsigned16_t n, void *data);
#if !LELY_NO_CO_MPDO
/// The SAM-MPDO event indication function. @see co_dev_sam_mpdo_event_ind_t
static void co_nmt_sam_mpdo_event_ind(co_unsigned16_t n, co_unsigned16_t idx,
		co_unsigned8_t subidx, void *data);
#endif
#endif // !LELY_NO_CO_TPDO

/**
 * Enters the specified state of an NMT master/slave service and invokes the
 * exit and entry functions.
 */
static void co_nmt_enter(co_nmt_t *nmt, co_nmt_state_t *next);

/**
 * Invokes the 'NMT command received' transition function of the current state
 * of an NMT master/slave service.
 *
 * @param nmt a pointer to an NMT master/slave service.
 * @param cs  the NMT command specifier (one of #CO_NMT_CS_START,
 *            #CO_NMT_CS_STOP, #CO_NMT_CS_ENTER_PREOP, #CO_NMT_CS_RESET_NODE or
 *            #CO_NMT_CS_RESET_COMM).
 */
static inline void co_nmt_emit_cs(co_nmt_t *nmt, co_unsigned8_t cs);

#if !LELY_NO_CO_NMT_BOOT
/**
 * Invokes the 'boot slave completed' transition function of the current state
 * of an NMT master service.
 *
 * @param nmt a pointer to an NMT master service.
 * @param id  the node-ID of the slave.
 * @param st  the state of the node (including the toggle bit).
 * @param es  the error status (in the range ['A'..'O'], or 0 on success).
 */
static inline void co_nmt_emit_boot(
		co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, char es);
#endif

/// A CANopen NMT state.
struct __co_nmt_state {
	/// A pointer to the function invoked when a new state is entered.
	co_nmt_state_t *(*on_enter)(co_nmt_t *nmt);
	/**
	 * A pointer to the transition function invoked when an NMT command is
	 * received.
	 *
	 * @param nmt a pointer to an NMT master/slave service.
	 * @param cs  the NMT command specifier (one of #CO_NMT_CS_START,
	 *            #CO_NMT_CS_STOP, #CO_NMT_CS_ENTER_PREOP,
	 *            #CO_NMT_CS_RESET_NODE or #CO_NMT_CS_RESET_COMM).
	 *
	 * @returns a pointer to the next state.
	 */
	co_nmt_state_t *(*on_cs)(co_nmt_t *nmt, co_unsigned8_t cs);
#if !LELY_NO_CO_NMT_BOOT
	/**
	 * A pointer to the transition function invoked when an 'boot slave'
	 * process completes.
	 *
	 * @param nmt a pointer to an NMT master service.
	 * @param id  the node-ID of the slave.
	 * @param st  the state of the node (including the toggle bit).
	 * @param es  the error status (in the range ['A'..'O'], or 0 on
	 *            success).
	 *
	 * @returns a pointer to the next state.
	 */
	co_nmt_state_t *(*on_boot)(co_nmt_t *nmt, co_unsigned8_t id,
			co_unsigned8_t st, char es);
#endif
	/// A pointer to the function invoked when the current state is left.
	void (*on_leave)(co_nmt_t *nmt);
};

#define LELY_CO_DEFINE_STATE(name, ...) \
	static co_nmt_state_t *const name = &(co_nmt_state_t){ __VA_ARGS__ };

#if !LELY_NO_CO_NMT_BOOT
/// The default 'boot slave completed' transition function.
static co_nmt_state_t *co_nmt_default_on_boot(
		co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, char es);
#endif

/// The 'NMT command received' transition function of the 'initializing' state.
static co_nmt_state_t *co_nmt_init_on_cs(co_nmt_t *nmt, co_unsigned8_t cs);

/// The 'initializing' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_init_state,
	.on_cs = &co_nmt_init_on_cs
)
// clang-format on

/// The entry function of the 'reset application' state.
static co_nmt_state_t *co_nmt_reset_node_on_enter(co_nmt_t *nmt);

/// The NMT 'reset application' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_reset_node_state,
	.on_enter = &co_nmt_reset_node_on_enter
)
// clang-format on

/// The entry function of the 'reset communication' state.
static co_nmt_state_t *co_nmt_reset_comm_on_enter(co_nmt_t *nmt);

/**
 * The 'NMT command received' transition function of the 'reset communication'
 * state.
 */
static co_nmt_state_t *co_nmt_reset_comm_on_cs(
		co_nmt_t *nmt, co_unsigned8_t cs);

/// The NMT 'reset communication' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_reset_comm_state,
	.on_enter = &co_nmt_reset_comm_on_enter,
	.on_cs = &co_nmt_reset_comm_on_cs
)
// clang-format on

/// The entry function of the 'boot-up' state.
static co_nmt_state_t *co_nmt_bootup_on_enter(co_nmt_t *nmt);

/// The 'NMT command received' transition function of the 'boot-up' state.
static co_nmt_state_t *co_nmt_bootup_on_cs(co_nmt_t *nmt, co_unsigned8_t cs);

/// The NMT 'boot-up' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_bootup_state,
	.on_enter = &co_nmt_bootup_on_enter,
	.on_cs = &co_nmt_bootup_on_cs
)
// clang-format on

/// The entry function of the 'pre-operational' state.
static co_nmt_state_t *co_nmt_preop_on_enter(co_nmt_t *nmt);

/**
 * The 'NMT command received' transition function of the 'pre-operational'
 * state.
 */
static co_nmt_state_t *co_nmt_preop_on_cs(co_nmt_t *nmt, co_unsigned8_t cs);

#if !LELY_NO_CO_NMT_BOOT
/**
 * The 'boot slave completed' transition function of the 'pre-operational'
 * state.
 */
static co_nmt_state_t *co_nmt_preop_on_boot(
		co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, char es);
#endif

/// The NMT 'pre-operational' state.
// clang-format off
#if LELY_NO_CO_NMT_BOOT
LELY_CO_DEFINE_STATE(co_nmt_preop_state,
	.on_enter = &co_nmt_preop_on_enter,
	.on_cs = &co_nmt_preop_on_cs
)
#else
LELY_CO_DEFINE_STATE(co_nmt_preop_state,
	.on_enter = &co_nmt_preop_on_enter,
	.on_cs = &co_nmt_preop_on_cs,
	.on_boot = &co_nmt_preop_on_boot
)
#endif
// clang-format on

/// The entry function of the 'operational' state.
static co_nmt_state_t *co_nmt_start_on_enter(co_nmt_t *nmt);

/// The 'NMT command received' transition function of the 'operational' state.
static co_nmt_state_t *co_nmt_start_on_cs(co_nmt_t *nmt, co_unsigned8_t cs);

/// The NMT 'operational' state.
// clang-format off
#if LELY_NO_CO_NMT_BOOT
LELY_CO_DEFINE_STATE(co_nmt_start_state,
	.on_enter = &co_nmt_start_on_enter,
	.on_cs = &co_nmt_start_on_cs
)
#else
LELY_CO_DEFINE_STATE(co_nmt_start_state,
	.on_enter = &co_nmt_start_on_enter,
	.on_cs = &co_nmt_start_on_cs,
	.on_boot = &co_nmt_default_on_boot
)
#endif
// clang-format on

/// The entry function of the 'stopped' state.
static co_nmt_state_t *co_nmt_stop_on_enter(co_nmt_t *nmt);

/// The 'NMT command received' transition function of the 'stopped' state.
static co_nmt_state_t *co_nmt_stop_on_cs(co_nmt_t *nmt, co_unsigned8_t cs);

/// The NMT 'stopped' state.
// clang-format off
#if LELY_NO_CO_NMT_BOOT
LELY_CO_DEFINE_STATE(co_nmt_stop_state,
	.on_enter = &co_nmt_stop_on_enter,
	.on_cs = &co_nmt_stop_on_cs
)
#else
LELY_CO_DEFINE_STATE(co_nmt_stop_state,
	.on_enter = &co_nmt_stop_on_enter,
	.on_cs = &co_nmt_stop_on_cs,
	.on_boot = &co_nmt_default_on_boot
)
#endif
// clang-format on

#undef LELY_CO_DEFINE_STATE

/// The NMT startup procedure (see Fig. 1 & 2 in CiA 302-2 version 4.1.0).
static co_nmt_state_t *co_nmt_startup(co_nmt_t *nmt);

#if !LELY_NO_CO_MASTER
/// The NMT master startup procedure.
static co_nmt_state_t *co_nmt_startup_master(co_nmt_t *nmt);
#endif

/// The NMT slave startup procedure.
static co_nmt_state_t *co_nmt_startup_slave(co_nmt_t *nmt);

/// Initializes the error control services. @see co_nmt_ec_fini()
static void co_nmt_ec_init(co_nmt_t *nmt);

/// Finalizes the error control services. @see co_nmt_ec_init()
static void co_nmt_ec_fini(co_nmt_t *nmt);

/**
 * Updates and (de)activates the life guarding or heartbeat production services.
 * This function is invoked by the download indication functions when object
 * 100C (Guard time), 100D (Life time factor) or 1017 (Producer heartbeat time)
 * is updated.
 */
static void co_nmt_ec_update(co_nmt_t *nmt);

/**
 * Sends an NMT error control response message.
 *
 * @param nmt a pointer to an NMT master/slave service.
 * @param st  the node state and toggle bit.
 *
 * @returns 0 on success, or -1 on error.
 */
static int co_nmt_ec_send_res(co_nmt_t *nmt, co_unsigned8_t st);

/// Initializes the heartbeat consumer services. @see co_nmt_hb_fini()
static void co_nmt_hb_init(co_nmt_t *nmt);

/// Finalizes the heartbeat consumer services. @see co_nmt_hb_init()
static void co_nmt_hb_fini(co_nmt_t *nmt);

#if !LELY_NO_CO_MASTER

#if !LELY_NO_CO_NMT_BOOT || !LELY_NO_CO_NMT_CFG
/// Find the heartbeat consumer for the specified node.
static co_nmt_hb_t *co_nmt_hb_find(
		co_nmt_t *nmt, co_unsigned8_t id, co_unsigned16_t *pms);
#endif

/// Initializes NMT slave management. @see co_nmt_slaves_fini()
static void co_nmt_slaves_init(co_nmt_t *nmt);

/// Finalizes NMT slave management. @see co_nmt_slaves_fini()
static void co_nmt_slaves_fini(co_nmt_t *nmt);

#if !LELY_NO_CO_NMT_BOOT
/**
 * Starts the NMT 'boot slave' processes.
 *
 * @returns 1 if at least one mandatory slave is booting, 0 if there are no
 * mandatory slaves, or -1 if an error occurred for a mandatory slave.
 */
static int co_nmt_slaves_boot(co_nmt_t *nmt);
#endif

/**
 * Checks if boot-up messages have been received from all mandatory slaves.
 *
 * @returns 1 if all boot-up messages were received, 0 if not.
 */
static int co_nmt_chk_bootup_slaves(const co_nmt_t *nmt);

#endif

/// The services enabled in the NMT 'pre-operational' state.
#define CO_NMT_PREOP_SRV \
	(CO_NMT_STOP_SRV | CO_NMT_SRV_SDO | CO_NMT_SRV_SYNC | CO_NMT_SRV_TIME \
			| CO_NMT_SRV_EMCY)

/// The services enabled in the NMT 'operational' state.
#define CO_NMT_START_SRV (CO_NMT_PREOP_SRV | CO_NMT_SRV_PDO)

/// The services enabled in the NMT 'stopped' state.
#define CO_NMT_STOP_SRV CO_NMT_SRV_LSS

co_unsigned32_t
co_dev_cfg_hb(co_dev_t *dev, co_unsigned8_t id, co_unsigned16_t ms)
{
	assert(dev);

	co_obj_t *obj_1016 = co_dev_find_obj(dev, 0x1016);
	if (!obj_1016)
		return CO_SDO_AC_NO_OBJ;

	co_unsigned8_t n = co_obj_get_val_u8(obj_1016, 0x00);
	co_unsigned8_t i = 0;
	// If the node-ID is valid, find an existing heartbeat consumer with the
	// same ID.
	if (id && id <= CO_NUM_NODES) {
		for (i = 1; i <= n; i++) {
			co_unsigned32_t val_i = co_obj_get_val_u32(obj_1016, i);
			co_unsigned8_t id_i = (val_i >> 16) & 0xff;
			if (id_i == id)
				break;
		}
	}
	// If the node-ID is invalid or no heartbeat consumer exists, find an
	// unused consumer.
	if (!i || i > n) {
		for (i = 1; i <= n; i++) {
			co_unsigned32_t val_i = co_obj_get_val_u32(obj_1016, i);
			co_unsigned8_t id_i = (val_i >> 16) & 0xff;
			if (!id_i || id_i > CO_NUM_NODES)
				break;
		}
	}

	if (!i || i > n)
		return CO_SDO_AC_NO_SUB;
	co_sub_t *sub = co_obj_find_sub(obj_1016, i);
	if (!sub)
		return CO_SDO_AC_NO_SUB;

	co_unsigned32_t val = ((co_unsigned32_t)id << 16) | ms;
	return co_sub_dn_ind_val(sub, CO_DEFTYPE_UNSIGNED32, &val);
}

const char *
co_nmt_es2str(char es)
{
	switch (es) {
	case 'A': return "The CANopen device is not listed in object 1F81.";
	case 'B':
		return "No response received for upload request of object 1000.";
	case 'C':
		return "Value of object 1000 from CANopen device is different to value in object 1F84 (Device type).";
	case 'D':
		return "Value of object 1018 sub-index 01 from CANopen device is different to value in object 1F85 (Vendor-ID).";
	case 'E':
		return "Heartbeat event. No heartbeat message received from CANopen device.";
	case 'F':
		return "Node guarding event. No confirmation for guarding request received from CANopen device.";
	case 'G':
		return "Objects for program download are not configured or inconsistent.";
	case 'H':
		return "Software update is required, but not allowed because of configuration or current status.";
	case 'I':
		return "Software update is required, but program download failed.";
	case 'J': return "Configuration download failed.";
	case 'K':
		return "Heartbeat event during start error control service. No heartbeat message received from CANopen device during start error control service.";
	case 'L': return "NMT slave was initially operational.";
	case 'M':
		return "Value of object 1018 sub-index 02 from CANopen device is different to value in object 1F86 (Product code).";
	case 'N':
		return "Value of object 1018 sub-index 03 from CANopen device is different to value in object 1F87 (Revision number).";
	case 'O':
		return "Value of object 1018 sub-index 04 from CANopen device is different to value in object 1F88 (Serial number).";
	default: return "Unknown error status";
	}
}

void *
__co_nmt_alloc(void)
{
	void *ptr = malloc(sizeof(struct __co_nmt));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
__co_nmt_free(void *ptr)
{
	free(ptr);
}

struct __co_nmt *
__co_nmt_init(struct __co_nmt *nmt, can_net_t *net, co_dev_t *dev)
{
	assert(nmt);
	assert(net);
	assert(dev);

	int errc = 0;

	nmt->net = net;
	nmt->dev = dev;

	nmt->id = co_dev_get_id(nmt->dev);

#if !LELY_NO_CO_DCF_RESTORE
	// Store a concise DCF containing the application parameters.
	if (co_dev_write_dcf(nmt->dev, 0x2000, 0x9fff, &nmt->dcf_node) == -1) {
		errc = get_errc();
		goto error_write_dcf_node;
	}
#endif

	// Store a concise DCF containing the communication parameters.
	if (co_dev_write_dcf(nmt->dev, 0x1000, 0x1fff, &nmt->dcf_comm) == -1) {
		errc = get_errc();
		goto error_write_dcf_comm;
	}

	nmt->state = NULL;

	co_nmt_srv_init(&nmt->srv, nmt);

	nmt->startup = 0;
#if !LELY_NO_CO_MASTER
	nmt->master = 0;
#endif

	// Create the CAN frame receiver for NMT messages.
	nmt->recv_000 = can_recv_create();
	if (!nmt->recv_000) {
		errc = get_errc();
		goto error_create_recv_000;
	}
	can_recv_set_func(nmt->recv_000, &co_nmt_recv_000, nmt);

	nmt->cs_ind = NULL;
	nmt->cs_data = NULL;

	// Create the CAN frame receiver for node guarding RTR and boot-up
	// messages.
	nmt->recv_700 = can_recv_create();
	if (!nmt->recv_700) {
		errc = get_errc();
		goto error_create_recv_700;
	}
	can_recv_set_func(nmt->recv_700, &co_nmt_recv_700, nmt);

#if !LELY_NO_CO_MASTER && !LELY_NO_CO_NG
	nmt->ng_ind = &default_ng_ind;
	nmt->ng_data = NULL;
#endif

	nmt->ec_timer = can_timer_create();
	if (!nmt->ec_timer) {
		errc = get_errc();
		goto error_create_ec_timer;
	}
	can_timer_set_func(nmt->ec_timer, &co_nmt_ec_timer, nmt);

	nmt->st = CO_NMT_ST_BOOTUP;
#if !LELY_NO_CO_NG
	nmt->gt = 0;
	nmt->ltf = 0;

	nmt->lg_state = CO_NMT_EC_RESOLVED;
	nmt->lg_ind = &default_lg_ind;
	nmt->lg_data = NULL;
#endif

	nmt->ms = 0;

#if LELY_NO_MALLOC
	memset(nmt->hbs, 0, CO_NMT_MAX_NHB * sizeof(*nmt->hbs));
#else
	nmt->hbs = NULL;
#endif
	nmt->nhb = 0;
	nmt->hb_ind = &default_hb_ind;
	nmt->hb_data = NULL;

	nmt->st_ind = &default_st_ind;
	nmt->st_data = NULL;

#if !LELY_NO_CO_MASTER
	// Create a CAN fame buffer for pending NMT messages that will be sent
	// once the inhibit time has elapsed.
#if LELY_NO_MALLOC
	can_buf_init(&nmt->buf, nmt->begin, CO_NMT_CAN_BUF_SIZE);
	memset(nmt->begin, 0, CO_NMT_CAN_BUF_SIZE * sizeof(*nmt->begin));
#else
	can_buf_init(&nmt->buf, NULL, 0);
#endif

	can_net_get_time(nmt->net, &nmt->inhibit);
	nmt->cs_timer = can_timer_create();
	if (!nmt->cs_timer) {
		errc = get_errc();
		goto error_create_cs_timer;
	}
	can_timer_set_func(nmt->cs_timer, &co_nmt_cs_timer, nmt);

#if !LELY_NO_CO_LSS
	nmt->lss_req = NULL;
	nmt->lss_data = NULL;
#endif

	nmt->halt = 0;

	for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++) {
		struct co_nmt_slave *slave = &nmt->slaves[id - 1];
		slave->nmt = nmt;

		slave->recv = NULL;
#if !LELY_NO_CO_NG
		slave->timer = NULL;
#endif

		slave->assignment = 0;
		slave->est = 0;
		slave->rst = 0;

#if !LELY_NO_CO_NMT_BOOT
		slave->es = 0;

		slave->booting = 0;
		slave->booted = 0;

		slave->boot = NULL;
#endif

#if !LELY_NO_CO_NMT_CFG
		slave->configuring = 0;

		slave->cfg = NULL;
		slave->cfg_con = NULL;
		slave->cfg_data = NULL;
#endif

#if !LELY_NO_CO_NG
		slave->gt = 0;
		slave->ltf = 0;
		slave->rtr = 0;
		slave->ng_state = CO_NMT_EC_RESOLVED;
#endif
	}

	for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++) {
		struct co_nmt_slave *slave = &nmt->slaves[id - 1];

		slave->recv = can_recv_create();
		if (!slave->recv) {
			errc = get_errc();
			goto error_init_slave;
		}
		can_recv_set_func(slave->recv, &co_nmt_recv_700, nmt);

#if !LELY_NO_CO_NG
		slave->timer = can_timer_create();
		if (!slave->timer) {
			errc = get_errc();
			goto error_init_slave;
		}
		can_timer_set_func(slave->timer, &co_nmt_ng_timer, slave);
#endif
	}

	nmt->timeout = LELY_CO_NMT_TIMEOUT;

#if !LELY_NO_CO_NMT_BOOT
	nmt->boot_ind = NULL;
	nmt->boot_data = NULL;
#endif
#if !LELY_NO_CO_NMT_CFG
	nmt->cfg_ind = NULL;
	nmt->cfg_data = NULL;
#endif
	nmt->dn_ind = NULL;
	nmt->dn_data = NULL;
	nmt->up_ind = NULL;
	nmt->up_data = NULL;
#endif
	nmt->sync_ind = NULL;
	nmt->sync_data = NULL;

#if !LELY_NO_CO_TPDO
	nmt->tpdo_event_wait = 0;
	for (int i = 0; i < CO_NUM_PDOS / LONG_BIT; i++)
		nmt->tpdo_event_mask[i] = 0;

	// Set the Transmit-PDO event indication function.
	co_dev_set_tpdo_event_ind(nmt->dev, &co_nmt_tpdo_event_ind, nmt);

#if !LELY_NO_CO_MPDO
	// Set the SAM-MPDO event indication function.
	co_dev_set_sam_mpdo_event_ind(
			nmt->dev, &co_nmt_sam_mpdo_event_ind, nmt);
#endif
#endif // !LELY_NO_CO_TPDO

#if !LELY_NO_CO_NG
	// Set the download indication function for the guard time.
	co_obj_t *obj_100c = co_dev_find_obj(nmt->dev, 0x100c);
	if (obj_100c)
		co_obj_set_dn_ind(obj_100c, &co_100c_dn_ind, nmt);

	// Set the download indication function for the life time factor.
	co_obj_t *obj_100d = co_dev_find_obj(nmt->dev, 0x100d);
	if (obj_100d)
		co_obj_set_dn_ind(obj_100d, &co_100d_dn_ind, nmt);
#endif

	// Set the download indication function for the consumer heartbeat time.
	co_obj_t *obj_1016 = co_dev_find_obj(nmt->dev, 0x1016);
	if (obj_1016)
		co_obj_set_dn_ind(obj_1016, &co_1016_dn_ind, nmt);

	// Set the download indication function for the producer heartbeat time.
	co_obj_t *obj_1017 = co_dev_find_obj(nmt->dev, 0x1017);
	if (obj_1017)
		co_obj_set_dn_ind(obj_1017, &co_1017_dn_ind, nmt);

#if !LELY_NO_CO_NMT_CFG
	// Set the download indication function for the configuration request
	// value.
	co_obj_t *obj_1f25 = co_dev_find_obj(nmt->dev, 0x1f25);
	if (obj_1f25)
		co_obj_set_dn_ind(obj_1f25, &co_1f25_dn_ind, nmt);
#endif

	// Set the download indication function for the NMT startup value.
	co_obj_t *obj_1f80 = co_dev_find_obj(nmt->dev, 0x1f80);
	if (obj_1f80)
		co_obj_set_dn_ind(obj_1f80, &co_1f80_dn_ind, nmt);

#if !LELY_NO_CO_MASTER
	// Set the download indication function for the request NMT value.
	co_obj_t *obj_1f82 = co_dev_find_obj(nmt->dev, 0x1f82);
	if (obj_1f82)
		co_obj_set_dn_ind(obj_1f82, &co_1f82_dn_ind, nmt);
#endif

	co_nmt_enter(nmt, co_nmt_init_state);
	return nmt;

// #if !LELY_NO_CO_MASTER
// 	if (obj_1f82)
// 		co_obj_set_dn_ind(obj_1f82, NULL, NULL);
// #endif
// 	if (obj_1f80)
// 		co_obj_set_dn_ind(obj_1f80, NULL, NULL);
// #if !LELY_NO_CO_NMT_CFG
// 	if (obj_1f25)
// 		co_obj_set_dn_ind(obj_1f25, NULL, NULL);
// #endif
// 	if (obj_1017)
// 		co_obj_set_dn_ind(obj_1017, NULL, NULL);
// 	if (obj_1016)
// 		co_obj_set_dn_ind(obj_1016, NULL, NULL);
// 	if (obj_100d)
// 		co_obj_set_dn_ind(obj_100d, NULL, NULL);
// 	if (obj_100c)
// 		co_obj_set_dn_ind(obj_100c, NULL, NULL);
// #if !LELY_NO_CO_TPDO
// 	co_dev_set_tpdo_event_ind(nmt->dev, NULL, NULL);
// #endif
#if !LELY_NO_CO_MASTER
error_init_slave:
	for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++) {
		struct co_nmt_slave *slave = &nmt->slaves[id - 1];

		can_recv_destroy(slave->recv);
#if !LELY_NO_CO_NG
		can_timer_destroy(slave->timer);
#endif
	}
	can_timer_destroy(nmt->cs_timer);
error_create_cs_timer:
	can_buf_fini(&nmt->buf);
#endif
	can_timer_destroy(nmt->ec_timer);
error_create_ec_timer:
	can_recv_destroy(nmt->recv_700);
error_create_recv_700:
	can_recv_destroy(nmt->recv_000);
error_create_recv_000:
	co_nmt_srv_fini(&nmt->srv);
	co_val_fini(CO_DEFTYPE_DOMAIN, &nmt->dcf_comm);
error_write_dcf_comm:
#if !LELY_NO_CO_DCF_RESTORE
	co_val_fini(CO_DEFTYPE_DOMAIN, &nmt->dcf_node);
error_write_dcf_node:
#endif
	set_errc(errc);
	return NULL;
}

void
__co_nmt_fini(struct __co_nmt *nmt)
{
	assert(nmt);

#if !LELY_NO_CO_MASTER
	// Remove the download indication function for the request NMT value.
	co_obj_t *obj_1f82 = co_dev_find_obj(nmt->dev, 0x1f82);
	if (obj_1f82)
		co_obj_set_dn_ind(obj_1f82, NULL, NULL);
#endif

	// Remove the download indication function for the NMT startup value.
	co_obj_t *obj_1f80 = co_dev_find_obj(nmt->dev, 0x1f80);
	if (obj_1f80)
		co_obj_set_dn_ind(obj_1f80, NULL, NULL);

#if !LELY_NO_CO_NMT_CFG
	// Remove the download indication function for the configuration request
	// value.
	co_obj_t *obj_1f25 = co_dev_find_obj(nmt->dev, 0x1f25);
	if (obj_1f25)
		co_obj_set_dn_ind(obj_1f25, NULL, NULL);
#endif

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

#if !LELY_NO_CO_NG
	// Remove the download indication function for the life time factor.
	co_obj_t *obj_100d = co_dev_find_obj(nmt->dev, 0x100d);
	if (obj_100d)
		co_obj_set_dn_ind(obj_100d, NULL, NULL);

	// Remove the download indication function for the guard time.
	co_obj_t *obj_100c = co_dev_find_obj(nmt->dev, 0x100c);
	if (obj_100c)
		co_obj_set_dn_ind(obj_100c, NULL, NULL);
#endif

#if !LELY_NO_CO_TPDO
	// Remove the Transmit-PDO event indication function.
	co_dev_set_tpdo_event_ind(nmt->dev, NULL, NULL);
#endif

#if !LELY_NO_CO_MASTER
	co_nmt_slaves_fini(nmt);

	for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++) {
		struct co_nmt_slave *slave = &nmt->slaves[id - 1];

		can_recv_destroy(slave->recv);
#if !LELY_NO_CO_NG
		can_timer_destroy(slave->timer);
#endif
	}
#endif

#if !LELY_NO_CO_MASTER
	can_timer_destroy(nmt->cs_timer);
	can_buf_fini(&nmt->buf);
#endif

	co_nmt_hb_fini(nmt);

	co_nmt_ec_fini(nmt);

	can_timer_destroy(nmt->ec_timer);
	can_recv_destroy(nmt->recv_700);

	can_recv_destroy(nmt->recv_000);

	co_nmt_srv_fini(&nmt->srv);

	co_val_fini(CO_DEFTYPE_DOMAIN, &nmt->dcf_comm);
#if !LELY_NO_CO_DCF_RESTORE
	co_val_fini(CO_DEFTYPE_DOMAIN, &nmt->dcf_node);
#endif
}

co_nmt_t *
co_nmt_create(can_net_t *net, co_dev_t *dev)
{
	int errc = 0;

	co_nmt_t *nmt = __co_nmt_alloc();
	if (!nmt) {
		errc = get_errc();
		goto error_alloc_nmt;
	}

	if (!__co_nmt_init(nmt, net, dev)) {
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

void
co_nmt_destroy(co_nmt_t *nmt)
{
	if (nmt) {
		__co_nmt_fini(nmt);
		__co_nmt_free(nmt);
	}
}

can_net_t *
co_nmt_get_net(const co_nmt_t *nmt)
{
	assert(nmt);

	return nmt->net;
}

co_dev_t *
co_nmt_get_dev(const co_nmt_t *nmt)
{
	assert(nmt);

	return nmt->dev;
}

void
co_nmt_get_cs_ind(const co_nmt_t *nmt, co_nmt_cs_ind_t **pind, void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->cs_ind;
	if (pdata)
		*pdata = nmt->cs_data;
}

void
co_nmt_set_cs_ind(co_nmt_t *nmt, co_nmt_cs_ind_t *ind, void *data)
{
	assert(nmt);

	nmt->cs_ind = ind;
	nmt->cs_data = data;
}

#if !LELY_NO_CO_NG

#if !LELY_NO_CO_MASTER

void
co_nmt_get_ng_ind(const co_nmt_t *nmt, co_nmt_ng_ind_t **pind, void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->ng_ind;
	if (pdata)
		*pdata = nmt->ng_data;
}

void
co_nmt_set_ng_ind(co_nmt_t *nmt, co_nmt_ng_ind_t *ind, void *data)
{
	assert(nmt);

	nmt->ng_ind = ind ? ind : &default_ng_ind;
	nmt->ng_data = ind ? data : NULL;
}

void
co_nmt_on_ng(co_nmt_t *nmt, co_unsigned8_t id, int state, int reason)
{
	assert(nmt);
	(void)reason;

	if (!id || id > CO_NUM_NODES)
		return;

	if (co_nmt_is_master(nmt) && state == CO_NMT_EC_OCCURRED)
		co_nmt_node_err_ind(nmt, id);
}

#endif // !LELY_NO_CO_MASTER

void
co_nmt_get_lg_ind(const co_nmt_t *nmt, co_nmt_lg_ind_t **pind, void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->lg_ind;
	if (pdata)
		*pdata = nmt->lg_data;
}

void
co_nmt_set_lg_ind(co_nmt_t *nmt, co_nmt_lg_ind_t *ind, void *data)
{
	assert(nmt);

	nmt->lg_ind = ind ? ind : &default_lg_ind;
	nmt->lg_data = ind ? data : NULL;
}

void
co_nmt_on_lg(co_nmt_t *nmt, int state)
{
	assert(nmt);

	if (state == CO_NMT_EC_OCCURRED)
		co_nmt_on_err(nmt, 0x8130, 0x10, NULL);
}

#endif // !LELY_NO_CO_MASTER

void
co_nmt_get_hb_ind(const co_nmt_t *nmt, co_nmt_hb_ind_t **pind, void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->hb_ind;
	if (pdata)
		*pdata = nmt->hb_data;
}

void
co_nmt_set_hb_ind(co_nmt_t *nmt, co_nmt_hb_ind_t *ind, void *data)
{
	assert(nmt);

	nmt->hb_ind = ind ? ind : &default_hb_ind;
	nmt->hb_data = ind ? data : NULL;
}

void
co_nmt_on_hb(co_nmt_t *nmt, co_unsigned8_t id, int state, int reason)
{
	assert(nmt);

	if (!id || id > CO_NUM_NODES)
		return;

	if (state == CO_NMT_EC_OCCURRED && reason == CO_NMT_EC_TIMEOUT) {
#if !LELY_NO_CO_MASTER
		if (co_nmt_is_master(nmt)) {
			co_nmt_node_err_ind(nmt, id);
			return;
		}
#endif
		co_nmt_on_err(nmt, 0x8130, 0x10, NULL);
	}
}

void
co_nmt_get_st_ind(const co_nmt_t *nmt, co_nmt_st_ind_t **pind, void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->st_ind;
	if (pdata)
		*pdata = nmt->st_data;
}

void
co_nmt_set_st_ind(co_nmt_t *nmt, co_nmt_st_ind_t *ind, void *data)
{
	assert(nmt);

	nmt->st_ind = ind ? ind : &default_st_ind;
	nmt->st_data = ind ? data : NULL;
}

void
co_nmt_on_st(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st)
{
	assert(nmt);

	if (!id || id > CO_NUM_NODES)
		return;

#if LELY_NO_CO_NMT_BOOT
	(void)nmt;
	(void)st;
#else
	if (co_nmt_is_master(nmt) && st == CO_NMT_ST_BOOTUP) {
		int errc = get_errc();
		co_nmt_boot_req(nmt, id, nmt->timeout);
		set_errc(errc);
	}
#endif
}

#if !LELY_NO_CO_MASTER

#if !LELY_NO_CO_LSS

void
co_nmt_get_lss_req(const co_nmt_t *nmt, co_nmt_lss_req_t **pind, void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->lss_req;
	if (pdata)
		*pdata = nmt->lss_data;
}

void
co_nmt_set_lss_req(co_nmt_t *nmt, co_nmt_lss_req_t *ind, void *data)
{
	assert(nmt);

	nmt->lss_req = ind;
	nmt->lss_data = data;
}

#endif

#if !LELY_NO_CO_NMT_BOOT

void
co_nmt_get_boot_ind(const co_nmt_t *nmt, co_nmt_boot_ind_t **pind, void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->boot_ind;
	if (pdata)
		*pdata = nmt->boot_data;
}

void
co_nmt_set_boot_ind(co_nmt_t *nmt, co_nmt_boot_ind_t *ind, void *data)
{
	assert(nmt);

	nmt->boot_ind = ind;
	nmt->boot_data = data;
}

#endif // !LELY_NO_CO_NMT_BOOT

#if !LELY_NO_CO_NMT_CFG

void
co_nmt_get_cfg_ind(const co_nmt_t *nmt, co_nmt_cfg_ind_t **pind, void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->cfg_ind;
	if (pdata)
		*pdata = nmt->cfg_data;
}

void
co_nmt_set_cfg_ind(co_nmt_t *nmt, co_nmt_cfg_ind_t *ind, void *data)
{
	assert(nmt);

	nmt->cfg_ind = ind;
	nmt->cfg_data = data;
}

#endif // !LELY_NO_CO_NMT_BOOT

void
co_nmt_get_dn_ind(const co_nmt_t *nmt, co_nmt_sdo_ind_t **pind, void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->dn_ind;
	if (pdata)
		*pdata = nmt->dn_data;
}

void
co_nmt_set_dn_ind(co_nmt_t *nmt, co_nmt_sdo_ind_t *ind, void *data)
{
	assert(nmt);

	nmt->dn_ind = ind;
	nmt->dn_data = data;
}

void
co_nmt_get_up_ind(const co_nmt_t *nmt, co_nmt_sdo_ind_t **pind, void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->up_ind;
	if (pdata)
		*pdata = nmt->up_data;
}

void
co_nmt_set_up_ind(co_nmt_t *nmt, co_nmt_sdo_ind_t *ind, void *data)
{
	assert(nmt);

	nmt->up_ind = ind;
	nmt->up_data = data;
}

#endif // !LELY_NO_CO_MASTER

void
co_nmt_get_sync_ind(const co_nmt_t *nmt, co_nmt_sync_ind_t **pind, void **pdata)
{
	assert(nmt);

	if (pind)
		*pind = nmt->sync_ind;
	if (pdata)
		*pdata = nmt->sync_data;
}

void
co_nmt_set_sync_ind(co_nmt_t *nmt, co_nmt_sync_ind_t *ind, void *data)
{
	assert(nmt);

	nmt->sync_ind = ind;
	nmt->sync_data = data;
}

void
co_nmt_on_sync(co_nmt_t *nmt, co_unsigned8_t cnt)
{
	assert(nmt);

	// Handle TPDOs before RPDOs. This prevents a possible race condition if
	// the same object is mapped to both an RPDO and a TPDO. In accordance
	// with CiA 301 v4.2.0 we transmit the value from the previous
	// synchronous window before updating it with a received PDO.
#if !LELY_NO_CO_TPDO
	for (co_unsigned16_t i = 0; i < nmt->srv.ntpdo; i++) {
		if (nmt->srv.tpdos[i])
			co_tpdo_sync(nmt->srv.tpdos[i], cnt);
	}
#endif
#if !LELY_NO_CO_RPDO
	for (co_unsigned16_t i = 0; i < nmt->srv.nrpdo; i++) {
		if (nmt->srv.rpdos[i])
			co_rpdo_sync(nmt->srv.rpdos[i], cnt);
	}
#endif

	if (nmt->sync_ind)
		nmt->sync_ind(nmt, cnt, nmt->sync_data);
}

void
co_nmt_on_err(co_nmt_t *nmt, co_unsigned16_t eec, co_unsigned8_t er,
		const co_unsigned8_t msef[5])
{
	assert(nmt);

	if (eec) {
#if LELY_NO_CO_EMCY
		(void)er;
		(void)msef;
#else
		if (nmt->srv.emcy)
			co_emcy_push(nmt->srv.emcy, eec, er, msef);
#endif
		// In case of a communication error (0x81xx), invoke the
		// behavior specified by 1029:01.
		if ((eec & 0xff00) == 0x8100)
			co_nmt_comm_err_ind(nmt);
	}
}

#if !LELY_NO_CO_TPDO

void
co_nmt_on_tpdo_event(co_nmt_t *nmt, co_unsigned16_t n)
{
	assert(nmt);
	assert(nmt->srv.ntpdo <= CO_NUM_PDOS);

	int errsv = get_errc();
	if (n) {
		co_tpdo_t *pdo = co_nmt_get_tpdo(nmt, n);
		if (pdo) {
			if (nmt->tpdo_event_wait)
				nmt->tpdo_event_mask[(n - 1) / LONG_BIT] |= 1ul
						<< ((n - 1) % LONG_BIT);
			else
				co_tpdo_event(pdo);
		}
	} else {
		for (n = 1; n <= nmt->srv.ntpdo; n++) {
			co_tpdo_t *pdo = co_nmt_get_tpdo(nmt, n);
			if (!pdo)
				continue;
			if (nmt->tpdo_event_wait)
				nmt->tpdo_event_mask[(n - 1) / LONG_BIT] |= 1ul
						<< ((n - 1) % LONG_BIT);
			else
				co_tpdo_event(pdo);
		}
	}
	set_errc(errsv);
}

void
co_nmt_on_tpdo_event_lock(co_nmt_t *nmt)
{
	assert(nmt);

	nmt->tpdo_event_wait++;
}

void
co_nmt_on_tpdo_event_unlock(co_nmt_t *nmt)
{
	assert(nmt);
	assert(nmt->tpdo_event_wait);

	if (--nmt->tpdo_event_wait)
		return;

	// Issue an indication for every postponed Transmit-PDO event.
	int errsv = get_errc();
	for (int i = 0; i < CO_NUM_PDOS / LONG_BIT; i++) {
		if (nmt->tpdo_event_mask[i]) {
			co_unsigned16_t n = i * LONG_BIT + 1;
			for (int j = 0; j < LONG_BIT && n <= nmt->srv.ntpdo
					&& nmt->tpdo_event_mask[i];
					j++, n++) {
				if (!(nmt->tpdo_event_mask[i] & (1ul << j)))
					continue;
				nmt->tpdo_event_mask[i] &= ~(1ul << j);
				co_tpdo_t *pdo = co_nmt_get_tpdo(nmt, n);
				if (pdo)
					co_tpdo_event(pdo);
			}
			nmt->tpdo_event_mask[i] = 0;
		}
	}
	set_errc(errsv);
}

#if !LELY_NO_CO_MPDO
void
co_nmt_on_sam_mpdo_event(co_nmt_t *nmt, co_unsigned16_t n, co_unsigned16_t idx,
		co_unsigned8_t subidx)
{
	assert(nmt);

	co_tpdo_t *pdo = co_nmt_get_tpdo(nmt, n);
	if (pdo)
		co_sam_mpdo_event(pdo, idx, subidx);
}
#endif

#endif // !LELY_NO_CO_TPDO

co_unsigned8_t
co_nmt_get_id(const co_nmt_t *nmt)
{
	assert(nmt);

	return nmt->id;
}

int
co_nmt_set_id(co_nmt_t *nmt, co_unsigned8_t id)
{
	assert(nmt);

	if (!id || (id > CO_NUM_NODES && id != 0xff)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	nmt->id = id;

	return 0;
}

co_unsigned8_t
co_nmt_get_st(const co_nmt_t *nmt)
{
	assert(nmt);

	return nmt->st & ~CO_NMT_ST_TOGGLE;
}

int
co_nmt_is_master(const co_nmt_t *nmt)
{
#if LELY_NO_CO_MASTER
	(void)nmt;

	return 0;
#else
	assert(nmt);

	return nmt->master;
#endif
}

#if !LELY_NO_CO_MASTER

int
co_nmt_get_timeout(const co_nmt_t *nmt)
{
	assert(nmt);

	return nmt->timeout;
}

void
co_nmt_set_timeout(co_nmt_t *nmt, int timeout)
{
	assert(nmt);

	nmt->timeout = timeout;
}

int
co_nmt_cs_req(co_nmt_t *nmt, co_unsigned8_t cs, co_unsigned8_t id)
{
	assert(nmt);

	if (!nmt->master) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	switch (cs) {
	case CO_NMT_CS_START:
	case CO_NMT_CS_STOP:
	case CO_NMT_CS_ENTER_PREOP:
	case CO_NMT_CS_RESET_NODE:
	case CO_NMT_CS_RESET_COMM: break;
	default: set_errnum(ERRNUM_INVAL); return -1;
	}

	if (id > CO_NUM_NODES) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	if (id == co_dev_get_id(nmt->dev))
		return co_nmt_cs_ind(nmt, cs);

	trace("NMT: sending command specifier %d to node %d", cs, id);

	struct can_msg msg = CAN_MSG_INIT;
	msg.id = CO_NMT_CS_CANID;
	msg.len = 2;
	msg.data[0] = cs;
	msg.data[1] = id;

	// Add the frame to the buffer.
	if (!can_buf_write(&nmt->buf, &msg, 1)) {
		if (!can_buf_reserve(&nmt->buf, 1))
			return -1;
		can_buf_write(&nmt->buf, &msg, 1);
	}

	// Send the frame by triggering the inhibit timer.
	return co_nmt_cs_timer(NULL, nmt);
}

#if !LELY_NO_CO_LSS
int
co_nmt_lss_con(co_nmt_t *nmt)
{
	assert(nmt);

	if (!nmt->master || nmt->state != co_nmt_reset_comm_state) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	co_nmt_enter(nmt, co_nmt_bootup_state);

	return 0;
}
#endif

#if !LELY_NO_CO_NMT_BOOT

int
co_nmt_boot_req(co_nmt_t *nmt, co_unsigned8_t id, int timeout)
{
	assert(nmt);

	int errc = 0;

	if (!nmt->master) {
		errc = errnum2c(ERRNUM_PERM);
		goto error_param;
	}

	if (!id || id > CO_NUM_NODES || id == co_dev_get_id(nmt->dev)) {
		errc = errnum2c(ERRNUM_INVAL);
		goto error_param;
	}
	struct co_nmt_slave *slave = &nmt->slaves[id - 1];

	if (slave->booting) {
		errc = errnum2c(ERRNUM_INPROGRESS);
		goto error_param;
	}

	trace("NMT: booting slave %d", id);

	// Disable the heartbeat consumer during the 'boot slave' process.
	co_nmt_hb_t *hb = co_nmt_hb_find(nmt, id, NULL);
	if (hb)
		co_nmt_hb_set_1016(hb, id, 0);

	slave->booting = 1;

	slave->boot = co_nmt_boot_create(nmt->net, nmt->dev, nmt);
	if (!slave->boot) {
		errc = get_errc();
		goto error_create_boot;
	}

	// clang-format off
	if (co_nmt_boot_boot_req(slave->boot, id, timeout, &co_nmt_dn_ind,
			&co_nmt_up_ind, nmt) == -1) {
		// clang-format on
		errc = get_errc();
		goto error_boot_req;
	}

	return 0;

error_boot_req:
	co_nmt_boot_destroy(slave->boot);
	slave->boot = NULL;
error_create_boot:
	slave->booting = 0;
error_param:
	set_errc(errc);
	return -1;
}

int
co_nmt_is_booting(const co_nmt_t *nmt, co_unsigned8_t id)
{
	assert(nmt);

	if (!nmt->master)
		return 0;

	if (!id || id > CO_NUM_NODES || id == co_dev_get_id(nmt->dev))
		return 0;

	return !!nmt->slaves[id - 1].boot;
}

#endif // !LELY_NO_CO_NMT_BOOT

#if !LELY_NO_CO_MASTER
int
co_nmt_chk_bootup(const co_nmt_t *nmt, co_unsigned8_t id)
{
	assert(nmt);

	if (!nmt->master) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	if (id > CO_NUM_NODES) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	if (id == co_dev_get_id(nmt->dev)) {
		switch (co_nmt_get_st(nmt)) {
		case CO_NMT_ST_STOP:
		case CO_NMT_ST_START:
		case CO_NMT_ST_PREOP: return 1;
		default: return 0;
		}
	}

	if (id == 0)
		return co_nmt_chk_bootup_slaves(nmt);
	else
		return !!nmt->slaves[id - 1].bootup;
}
#endif

#if !LELY_NO_CO_NMT_CFG

int
co_nmt_cfg_req(co_nmt_t *nmt, co_unsigned8_t id, int timeout,
		co_nmt_cfg_con_t *con, void *data)
{
	assert(nmt);

	int errc = 0;

	if (!nmt->master) {
		errc = errnum2c(ERRNUM_PERM);
		goto error_param;
	}

	if (!id || id > CO_NUM_NODES || id == co_dev_get_id(nmt->dev)) {
		errc = errnum2c(ERRNUM_INVAL);
		goto error_param;
	}
	struct co_nmt_slave *slave = &nmt->slaves[id - 1];

	if (slave->configuring) {
		errc = errnum2c(ERRNUM_INPROGRESS);
		goto error_param;
	}

	trace("NMT: starting update configuration process for node %d", id);

	// Disable the heartbeat consumer during a configuration request.
	co_nmt_hb_t *hb = co_nmt_hb_find(nmt, id, NULL);
	if (hb)
		co_nmt_hb_set_1016(hb, id, 0);

	slave->configuring = 1;

	slave->cfg = co_nmt_cfg_create(nmt->net, nmt->dev, nmt);
	if (!slave->cfg) {
		errc = get_errc();
		goto error_create_cfg;
	}
	slave->cfg_con = con;
	slave->cfg_data = data;

	// clang-format off
	if (co_nmt_cfg_cfg_req(slave->cfg, id, timeout, &co_nmt_dn_ind,
			&co_nmt_up_ind, nmt) == -1) {
		// clang-format on
		errc = get_errc();
		goto error_cfg_req;
	}

	return 0;

error_cfg_req:
	co_nmt_cfg_destroy(slave->cfg);
	slave->cfg = NULL;
error_create_cfg:
	slave->configuring = 0;
error_param:
	set_errc(errc);
	return -1;
}

int
co_nmt_cfg_res(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned32_t ac)
{
	assert(nmt);

	if (!nmt->master) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	if (!id || id > CO_NUM_NODES || !nmt->slaves[id - 1].cfg) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	return co_nmt_cfg_cfg_res(nmt->slaves[id - 1].cfg, ac);
}

#endif // !LELY_NO_CO_NMT_CFG

#if !LELY_NO_CO_NG
int
co_nmt_ng_req(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned16_t gt,
		co_unsigned8_t ltf)
{
	assert(nmt);

	if (!nmt->master) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	if (!id || id > CO_NUM_NODES || id == co_dev_get_id(nmt->dev)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
	struct co_nmt_slave *slave = &nmt->slaves[id - 1];

	if (!gt || !ltf) {
		can_timer_stop(slave->timer);

		slave->gt = 0;
		slave->ltf = 0;
		slave->rtr = 0;
	} else {
		slave->gt = gt;
		slave->ltf = ltf;
		slave->rtr = 0;

		can_timer_timeout(slave->timer, nmt->net, slave->gt);
	}

	return 0;
}
#endif // !LELY_NO_CO_NG

#endif // !LELY_NO_CO_MASTER

int
co_nmt_cs_ind(co_nmt_t *nmt, co_unsigned8_t cs)
{
	assert(nmt);

	switch (cs) {
	case CO_NMT_CS_START:
	case CO_NMT_CS_STOP:
	case CO_NMT_CS_ENTER_PREOP:
	case CO_NMT_CS_RESET_NODE:
	case CO_NMT_CS_RESET_COMM:
		trace("NMT: received command specifier %d", cs);
		co_nmt_emit_cs(nmt, cs);
		return 0;
	default: set_errnum(ERRNUM_INVAL); return -1;
	}
}

void
co_nmt_comm_err_ind(co_nmt_t *nmt)
{
	assert(nmt);

	diag(DIAG_INFO, 0, "NMT: communication error indicated");
	switch (co_dev_get_val_u8(nmt->dev, 0x1029, 0x01)) {
	case 0:
		if (co_nmt_get_st(nmt) == CO_NMT_ST_START)
			co_nmt_cs_ind(nmt, CO_NMT_CS_ENTER_PREOP);
		break;
	case 2: co_nmt_cs_ind(nmt, CO_NMT_CS_STOP); break;
	}
}

#if !LELY_NO_CO_MASTER
int
co_nmt_node_err_ind(co_nmt_t *nmt, co_unsigned8_t id)
{
	assert(nmt);

	if (!nmt->master) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	if (!id || id > CO_NUM_NODES) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	co_unsigned32_t assignment = co_dev_get_val_u32(nmt->dev, 0x1f81, id);
	// Ignore the error event if the slave is no longer in the network list.
	if (!(assignment & 0x01))
		return 0;
	int mandatory = !!(assignment & 0x08);

	diag(DIAG_INFO, 0, "NMT: error indicated for %s slave %d",
			mandatory ? "mandatory" : "optional", id);

	if (mandatory && (nmt->startup & 0x40)) {
		// If the slave is mandatory and bit 6 of the NMT startup value
		// is set, stop all nodes, including the master.
		co_nmt_cs_req(nmt, CO_NMT_CS_STOP, 0);
		return co_nmt_cs_ind(nmt, CO_NMT_CS_STOP);
	} else if (mandatory && (nmt->startup & 0x10)) {
		// If the slave is mandatory and bit 4 of the NMT startup value
		// is set, reset all nodes, including the master.
		co_nmt_cs_req(nmt, CO_NMT_CS_RESET_NODE, 0);
		return co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE);
	} else {
		// If the slave is not mandatory, or bits 4 and 6 of the NMT
		// startup value are zero, reset the node individually.
		co_nmt_cs_req(nmt, CO_NMT_CS_RESET_NODE, id);
		return 0;
	}
}
#endif

co_rpdo_t *
co_nmt_get_rpdo(const co_nmt_t *nmt, co_unsigned16_t n)
{
#if LELY_NO_CO_RPDO
	(void)nmt;
	(void)n;

	return NULL;
#else
	assert(nmt);

	if (!n || n > nmt->srv.nrpdo)
		return NULL;

	return nmt->srv.rpdos[n - 1];
#endif
}

co_tpdo_t *
co_nmt_get_tpdo(const co_nmt_t *nmt, co_unsigned16_t n)
{
#if LELY_NO_CO_TPDO
	(void)nmt;
	(void)n;

	return NULL;
#else
	assert(nmt);

	if (!n || n > nmt->srv.ntpdo)
		return NULL;

	return nmt->srv.tpdos[n - 1];
#endif
}

co_ssdo_t *
co_nmt_get_ssdo(const co_nmt_t *nmt, co_unsigned8_t n)
{
	assert(nmt);

	if (!n || n > nmt->srv.nssdo)
		return NULL;

	return nmt->srv.ssdos[n - 1];
}

co_csdo_t *
co_nmt_get_csdo(const co_nmt_t *nmt, co_unsigned8_t n)
{
#if LELY_NO_CO_CSDO
	(void)nmt;
	(void)n;

	return NULL;
#else
	assert(nmt);

	if (!n || n > nmt->srv.ncsdo)
		return NULL;

	return nmt->srv.csdos[n - 1];
#endif
}

co_sync_t *
co_nmt_get_sync(const co_nmt_t *nmt)
{
#if LELY_NO_CO_SYNC
	(void)nmt;

	return NULL;
#else
	assert(nmt);

	return nmt->srv.sync;
#endif
}

co_time_t *
co_nmt_get_time(const co_nmt_t *nmt)
{
#if LELY_NO_CO_TIME
	(void)nmt;

	return NULL;
#else
	assert(nmt);

	return nmt->srv.time;
#endif
}

co_emcy_t *
co_nmt_get_emcy(const co_nmt_t *nmt)
{
#if LELY_NO_CO_EMCY
	(void)nmt;

	return NULL;
#else
	assert(nmt);

	return nmt->srv.emcy;
#endif
}

co_lss_t *
co_nmt_get_lss(const co_nmt_t *nmt)
{
#if LELY_NO_CO_LSS
	(void)nmt;

	return NULL;
#else
	assert(nmt);

	return nmt->srv.lss;
#endif
}

#if !LELY_NO_CO_NMT_BOOT
void
co_nmt_boot_con(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, char es)
{
	assert(nmt);
	assert(nmt->master);
	assert(id && id <= CO_NUM_NODES);

	// Update the NMT slave state, including the assignment, in case it
	// changed during the 'boot slave' procedure.
	struct co_nmt_slave *slave = &nmt->slaves[id - 1];
	slave->assignment = co_dev_get_val_u32(nmt->dev, 0x1f81, id);
	slave->est = st & ~CO_NMT_ST_TOGGLE;
	// If we did not (yet) receive a state but the error control service was
	// successfully started, assume the node is pre-operational.
	if (!slave->est && (!es || es == 'L'))
		slave->est = CO_NMT_ST_PREOP;
	slave->rst = st;
	slave->es = es;
	slave->booting = 0;
	slave->booted = 1;
	co_nmt_boot_destroy(slave->boot);
	slave->boot = NULL;

	// Re-enable the heartbeat consumer for the node, if necessary.
	co_unsigned16_t ms = 0;
	co_nmt_hb_t *hb = co_nmt_hb_find(nmt, id, &ms);
	if (hb)
		co_nmt_hb_set_1016(hb, id, ms);

	// Update object 1F82 (Request NMT) with the NMT state.
	co_sub_t *sub = co_dev_find_sub(nmt->dev, 0x1f82, id);
	if (sub)
		co_sub_set_val_u8(sub, st & ~CO_NMT_ST_TOGGLE);

	// If the slave booted successfully and can be started by the NMT
	// service, and if the master is allowed to start the nodes (bit 3 of
	// the NMT startup value) and has to start the slaves individually (bit
	// 1) or is in the operational state, send the NMT 'start' command to
	// the slave.
	// clang-format off
	if (!es && (slave->assignment & 0x05) == 0x05 && !(nmt->startup & 0x08)
			&& (!(nmt->startup & 0x02)
			|| co_nmt_get_st(nmt) == CO_NMT_ST_START))
		// clang-format on
		co_nmt_cs_req(nmt, CO_NMT_CS_START, id);

	// If the error control service was successfully started, resume
	// heartbeat consumption or node guarding.
	if (!es || es == 'L') {
		if (hb) {
			co_nmt_hb_set_st(hb, st);
#if !LELY_NO_CO_NG
			// Disable node guarding.
			slave->assignment &= 0xff;
		} else {
			// Enable node guarding if the guard time and lifetime
			// factor are non-zero.
			co_unsigned16_t gt = (slave->assignment >> 16) & 0xffff;
			co_unsigned8_t ltf = (slave->assignment >> 8) & 0xff;
			if (co_nmt_ng_req(nmt, id, gt, ltf) == -1)
				diag(DIAG_ERROR, get_errc(),
						"unable to guard node %02X",
						id);
#endif
		}
	}

	trace("NMT: slave %d finished booting with error status %c", id,
			es ? es : '0');
	if (nmt->boot_ind)
		nmt->boot_ind(nmt, id, st, es, nmt->boot_data);

	co_nmt_emit_boot(nmt, id, st, es);
}
#endif // !LELY_NO_CO_NMT_BOOT

#if !LELY_NO_CO_NMT_CFG

void
co_nmt_cfg_ind(co_nmt_t *nmt, co_unsigned8_t id, co_csdo_t *sdo)
{
	assert(nmt);
	assert(nmt->master);
	assert(id && id <= CO_NUM_NODES);

	if (nmt->cfg_ind) {
		nmt->cfg_ind(nmt, id, sdo, nmt->cfg_data);
	} else {
		co_nmt_cfg_res(nmt, id, 0);
	}
}

void
co_nmt_cfg_con(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned32_t ac)
{
	assert(nmt);
	assert(nmt->master);
	assert(id && id <= CO_NUM_NODES);

	struct co_nmt_slave *slave = &nmt->slaves[id - 1];
	slave->configuring = 0;
	co_nmt_cfg_destroy(slave->cfg);
	slave->cfg = NULL;

#if !LELY_NO_CO_NMT_BOOT
	// Re-enable the heartbeat consumer for the node, if necessary.
	if (!slave->booting) {
#endif
		co_unsigned16_t ms = 0;
		co_nmt_hb_t *hb = co_nmt_hb_find(nmt, id, &ms);
		if (hb)
			co_nmt_hb_set_1016(hb, id, ms);
#if !LELY_NO_CO_NMT_BOOT
	}
#endif

	trace("NMT: update configuration process completed for slave %d", id);
	if (slave->cfg_con)
		slave->cfg_con(nmt, id, ac, slave->cfg_data);
}

#endif // !LELY_NO_CO_NMT_BOOT

void
co_nmt_hb_ind(co_nmt_t *nmt, co_unsigned8_t id, int state, int reason,
		co_unsigned8_t st)
{
	assert(nmt);
	assert(nmt->hb_ind);

	if (!id || id > CO_NUM_NODES)
		return;

	nmt->hb_ind(nmt, id, state, reason, nmt->hb_data);

	if (reason == CO_NMT_EC_STATE)
		co_nmt_st_ind(nmt, id, st);
}

#if !LELY_NO_CO_NG

static co_unsigned32_t
co_100c_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x100c);
	assert(req);
	co_nmt_t *nmt = data;
	assert(nmt);

	co_unsigned16_t type = co_sub_get_type(sub);
	assert(!co_type_is_array(type));

	union co_val val;
	co_unsigned32_t ac = 0;
	if (co_sdo_req_dn_val(req, type, &val, &ac) == -1)
		return ac;

	if (co_sub_get_subidx(sub))
		return CO_SDO_AC_NO_SUB;

	assert(type == CO_DEFTYPE_UNSIGNED16);
	co_unsigned16_t gt = val.u16;
	co_unsigned16_t gt_old = co_sub_get_val_u16(sub);
	if (gt == gt_old)
		return 0;

	nmt->gt = gt;

	co_sub_dn(sub, &val);

	co_nmt_ec_update(nmt);
	return 0;
}

static co_unsigned32_t
co_100d_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x100d);
	assert(req);
	co_nmt_t *nmt = data;
	assert(nmt);

	co_unsigned16_t type = co_sub_get_type(sub);
	assert(!co_type_is_array(type));

	union co_val val;
	co_unsigned32_t ac = 0;
	if (co_sdo_req_dn_val(req, type, &val, &ac) == -1)
		return ac;

	if (co_sub_get_subidx(sub))
		return CO_SDO_AC_NO_SUB;

	assert(type == CO_DEFTYPE_UNSIGNED8);
	co_unsigned8_t ltf = val.u8;
	co_unsigned8_t ltf_old = co_sub_get_val_u8(sub);
	if (ltf == ltf_old)
		return 0;

	nmt->ltf = ltf;

	co_sub_dn(sub, &val);

	co_nmt_ec_update(nmt);
	return 0;
}

#endif // !LELY_NO_CO_NG

static co_unsigned32_t
co_1016_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1016);
	assert(req);
	co_nmt_t *nmt = data;
	assert(nmt);

	co_unsigned16_t type = co_sub_get_type(sub);
	assert(!co_type_is_array(type));

	union co_val val;
	co_unsigned32_t ac = 0;
	if (co_sdo_req_dn_val(req, type, &val, &ac) == -1)
		return ac;

	co_unsigned8_t subidx = co_sub_get_subidx(sub);
	if (!subidx)
		return CO_SDO_AC_NO_WRITE;
	if (subidx > nmt->nhb)
		return CO_SDO_AC_NO_SUB;

	assert(type == CO_DEFTYPE_UNSIGNED32);
	if (val.u32 == co_sub_get_val_u32(sub))
		return 0;

	co_unsigned8_t id = (val.u32 >> 16) & 0xff;
	co_unsigned16_t ms = val.u32 & 0xffff;

	// If the heartbeat consumer is active (valid node-ID and non-zero
	// heartbeat time), check the other entries for duplicate node-IDs.
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
			// consumers with the same node-ID.
			if (id_i == id && ms_i)
				return CO_SDO_AC_PARAM;
		}
		// Disable heartbeat consumption for booting slaves or slaves
		// that are being configured.
#if !LELY_NO_CO_NMT_BOOT
		if (nmt->slaves[id - 1].boot)
			ms = 0;
#endif
#if !LELY_NO_CO_NMT_CFG
		if (nmt->slaves[id - 1].cfg)
			ms = 0;
#endif
	}

	co_sub_dn(sub, &val);

	co_nmt_hb_set_1016(nmt->hbs[subidx - 1], id, ms);
	return 0;
}

static co_unsigned32_t
co_1017_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1017);
	assert(req);
	co_nmt_t *nmt = data;
	assert(nmt);

	co_unsigned16_t type = co_sub_get_type(sub);
	assert(!co_type_is_array(type));

	union co_val val;
	co_unsigned32_t ac = 0;
	if (co_sdo_req_dn_val(req, type, &val, &ac) == -1)
		return ac;

	if (co_sub_get_subidx(sub))
		return CO_SDO_AC_NO_SUB;

	assert(type == CO_DEFTYPE_UNSIGNED16);
	co_unsigned16_t ms = val.u16;
	co_unsigned16_t ms_old = co_sub_get_val_u16(sub);
	if (ms == ms_old)
		return 0;

	nmt->ms = ms;

	co_sub_dn(sub, &val);

	co_nmt_ec_update(nmt);
	return 0;
}

#if !LELY_NO_CO_NMT_CFG
static co_unsigned32_t
co_1f25_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1f25);
	assert(req);
	co_nmt_t *nmt = data;
	assert(nmt);

	co_unsigned16_t type = co_sub_get_type(sub);
	assert(!co_type_is_array(type));

	union co_val val;
	co_unsigned32_t ac = 0;
	if (co_sdo_req_dn_val(req, type, &val, &ac) == -1)
		return ac;

	co_unsigned8_t subidx = co_sub_get_subidx(sub);
	if (!subidx)
		return CO_SDO_AC_NO_WRITE;

	// Sub-index 80 indicates all nodes.
	co_unsigned8_t id = subidx == 0x80 ? 0 : subidx;
	// Abort with an error if the node-ID is unknown.
	if (id > CO_NUM_NODES
			// cppcheck-suppress knownConditionTrueFalse
			|| (id && !(nmt->slaves[id - 1].assignment & 0x01)))
		return CO_SDO_AC_PARAM_VAL;

	// Check if the value 'conf' was downloaded.
	if (!nmt->master || val.u32 != UINT32_C(0x666e6f63))
		return CO_SDO_AC_DATA_CTL;

	// cppcheck-suppress knownConditionTrueFalse
	if (id) {
		// Check if the entry for this node is present in object 1F20
		// (Store DCF) or 1F22 (Concise DCF).
#if LELY_NO_CO_DCF
		if (!co_dev_get_val(nmt->dev, 0x1f22, id))
#else
		if (!co_dev_get_val(nmt->dev, 0x1f20, id)
				&& !co_dev_get_val(nmt->dev, 0x1f22, id))
#endif
			return CO_SDO_AC_NO_DATA;
		// Abort if the slave is already being configured.
		if (nmt->slaves[id - 1].cfg)
			return CO_SDO_AC_DATA_DEV;
		co_nmt_cfg_req(nmt, id, nmt->timeout, NULL, NULL);
	} else {
		// Check if object 1F20 (Store DCF) or 1F22 (Concise DCF)
		// exists.
#if LELY_NO_CO_DCF
		if (!co_dev_find_obj(nmt->dev, 0x1f22))
#else
		if (!co_dev_find_obj(nmt->dev, 0x1f20)
				&& !co_dev_find_obj(nmt->dev, 0x1f22))
#endif
			return CO_SDO_AC_NO_DATA;
		for (id = 1; id <= CO_NUM_NODES; id++) {
			struct co_nmt_slave *slave = &nmt->slaves[id - 1];
			// Skip slaves that are not in the network list or are
			// already being configured.
			if (!(slave->assignment & 0x01) || slave->configuring)
				continue;
			co_nmt_cfg_req(nmt, id, nmt->timeout, NULL, NULL);
		}
	}

	return 0;
}
#endif // !LELY_NO_CO_NMT_CFG

static co_unsigned32_t
co_1f80_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1f80);
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
	co_unsigned32_t startup = val.u32;
	co_unsigned32_t startup_old = co_sub_get_val_u32(sub);
	if (startup == startup_old)
		return 0;

	// Only bits 0..4 and 6 are supported.
	if ((startup ^ startup_old) != 0x5f)
		return CO_SDO_AC_PARAM_VAL;

	co_sub_dn(sub, &val);

	return 0;
}

#if !LELY_NO_CO_MASTER
static co_unsigned32_t
co_1f82_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1f82);
	assert(req);
	co_nmt_t *nmt = data;
	assert(nmt);

	co_unsigned16_t type = co_sub_get_type(sub);
	assert(!co_type_is_array(type));

	union co_val val;
	co_unsigned32_t ac = 0;
	if (co_sdo_req_dn_val(req, type, &val, &ac) == -1)
		return ac;

	co_unsigned8_t subidx = co_sub_get_subidx(sub);
	if (!subidx)
		return CO_SDO_AC_NO_WRITE;

	// Sub-index 80 indicates all nodes.
	co_unsigned8_t id = subidx == 0x80 ? 0 : subidx;
	// Abort with an error if the node-ID is unknown.
	if (id > CO_NUM_NODES
			// cppcheck-suppress knownConditionTrueFalse
			|| (id && !(nmt->slaves[id - 1].assignment & 0x01)))
		return CO_SDO_AC_PARAM_VAL;

	if (!nmt->master)
		return CO_SDO_AC_DATA_CTL;

	assert(type == CO_DEFTYPE_UNSIGNED8);
	switch (val.u8) {
	case CO_NMT_ST_STOP: co_nmt_cs_req(nmt, CO_NMT_CS_STOP, id); break;
	case CO_NMT_ST_START: co_nmt_cs_req(nmt, CO_NMT_CS_START, id); break;
	case CO_NMT_ST_RESET_NODE:
		co_nmt_cs_req(nmt, CO_NMT_CS_RESET_NODE, id);
		break;
	case CO_NMT_ST_RESET_COMM:
		co_nmt_cs_req(nmt, CO_NMT_CS_RESET_COMM, id);
		break;
	case CO_NMT_ST_PREOP:
		co_nmt_cs_req(nmt, CO_NMT_CS_ENTER_PREOP, id);
		break;
	default: ac = CO_SDO_AC_PARAM_VAL; break;
	}

	return 0;
}
#endif // !LELY_NO_CO_MASTER

static int
co_nmt_recv_000(const struct can_msg *msg, void *data)
{
	assert(msg);
	co_nmt_t *nmt = data;
	assert(nmt);

#if !LELY_NO_CO_MASTER
	// Ignore NMT commands if we're the master.
	if (nmt->master)
		return 0;
#endif

	if (msg->len < 2)
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
	assert(msg->id > 0x700 && msg->id <= 0x77f);
	co_nmt_t *nmt = data;
	assert(nmt);

	if (msg->flags & CAN_FLAG_RTR) {
#if !LELY_NO_CO_NG
		assert(nmt->gt && nmt->ltf);
		assert(nmt->lg_ind);

		// Respond with the state and flip the toggle bit.
		co_nmt_ec_send_res(nmt, nmt->st);
		nmt->st ^= CO_NMT_ST_TOGGLE;

		// Reset the life guarding timer.
		can_timer_timeout(nmt->ec_timer, nmt->net, nmt->gt * nmt->ltf);

		if (nmt->lg_state == CO_NMT_EC_OCCURRED) {
			diag(DIAG_INFO, 0, "NMT: life guarding event resolved");
			// Notify the user of the resolution of a life guarding
			// error.
			nmt->lg_state = CO_NMT_EC_RESOLVED;
			nmt->lg_ind(nmt, nmt->lg_state, nmt->lg_data);
		}
#endif
#if !LELY_NO_CO_MASTER
	} else {
		assert(nmt->master);
#if !LELY_NO_CO_NG
		assert(nmt->ng_ind);
#endif

		co_unsigned8_t id = (msg->id - 0x700) & 0x7f;
		if (!id)
			return 0;
		struct co_nmt_slave *slave = &nmt->slaves[id - 1];

		if (msg->len < 1)
			return 0;
		co_unsigned8_t st = msg->data[0];

		if (st == CO_NMT_ST_BOOTUP) {
			// The expected state after a boot-up event is
			// pre-operational.
			slave->est = CO_NMT_ST_PREOP;
			// Record the reception of the boot-up message.
			slave->bootup = 1;

			// Inform the application of the boot-up event.
			co_nmt_st_ind(nmt, id, st);
			return 0;
		}

		// Ignore messages from booting slaves or slaves that are being
		// configured.
#if !LELY_NO_CO_NMT_BOOT
		if (slave->booting)
			return 0;
#endif
#if !LELY_NO_CO_NMT_CFG
		if (slave->configuring)
			return 0;
#endif

#if !LELY_NO_CO_NG
		// Ignore messages if node guarding is disabled.
		if (!slave->gt || !slave->ltf)
			return 0;

		// Check the toggle bit and ignore the message if it does not
		// match.
		if (!((st ^ slave->rst) & CO_NMT_ST_TOGGLE))
			return 0;
		slave->rst ^= CO_NMT_ST_TOGGLE;

		// Notify the application of the resolution of a node guarding
		// timeout.
		if (slave->rtr >= slave->ltf) {
			diag(DIAG_INFO, 0,
					"NMT: node guarding time out resolved for node %d",
					id);
			nmt->ng_ind(nmt, id, CO_NMT_EC_RESOLVED,
					CO_NMT_EC_TIMEOUT, nmt->ng_data);
		}
		slave->rtr = 0;

		// Notify the application of the occurrence or resolution of an
		// unexpected state change.
		if (slave->est != (st & ~CO_NMT_ST_TOGGLE)
				&& slave->ng_state == CO_NMT_EC_RESOLVED) {
			diag(DIAG_INFO, 0,
					"NMT: node guarding state change occurred for node %d",
					id);
			slave->ng_state = CO_NMT_EC_OCCURRED;
			nmt->ng_ind(nmt, id, slave->ng_state, CO_NMT_EC_STATE,
					nmt->ng_data);
		} else if (slave->est == (st & ~CO_NMT_ST_TOGGLE)
				&& slave->ng_state == CO_NMT_EC_OCCURRED) {
			diag(DIAG_INFO, 0,
					"NMT: node guarding state change resolved for node %d",
					id);
			slave->ng_state = CO_NMT_EC_RESOLVED;
			nmt->ng_ind(nmt, id, slave->ng_state, CO_NMT_EC_STATE,
					nmt->ng_data);
		}

		// Notify the application of the occurrence of a state change.
		if (st != slave->rst)
			co_nmt_st_ind(nmt, id, st);
#endif // !LELY_NO_CO_NG
#endif // !LELY_NO_CO_MASTER
	}

	return 0;
}

#if !LELY_NO_CO_MASTER && !LELY_NO_CO_NG
static int
co_nmt_ng_timer(const struct timespec *tp, void *data)
{
	(void)tp;
	struct co_nmt_slave *slave = data;
	assert(slave);
	assert(slave->gt && slave->ltf);
	co_nmt_t *nmt = slave->nmt;
	assert(nmt);
	assert(nmt->master);
	assert(nmt->ng_ind);
	co_unsigned8_t id = slave - nmt->slaves + 1;
	assert(id && id <= CO_NUM_NODES);

	// Reset the timer for the next RTR.
	can_timer_timeout(slave->timer, nmt->net, slave->gt);

#if !LELY_NO_CO_NMT_BOOT
	// Do not send node guarding RTRs to slaves that have not finished
	// booting.
	if (!slave->booted)
		return 0;
#endif

	// Notify the application once of the occurrence of a node guarding
	// timeout.
	if (slave->rtr <= slave->ltf && ++slave->rtr == slave->ltf) {
		diag(DIAG_INFO, 0,
				"NMT: node guarding time out occurred for node %d",
				id);
		nmt->ng_ind(nmt, id, CO_NMT_EC_OCCURRED, CO_NMT_EC_TIMEOUT,
				nmt->ng_data);
		return 0;
	}

	struct can_msg msg = CAN_MSG_INIT;
	msg.id = CO_NMT_EC_CANID(id);
	msg.flags |= CAN_FLAG_RTR;

	return can_net_send(nmt->net, &msg);
}
#endif // !LELY_NO_CO_MASTER && !LELY_NO_CO_NG

static int
co_nmt_ec_timer(const struct timespec *tp, void *data)
{
	(void)tp;
	co_nmt_t *nmt = data;
	assert(nmt);

	if (nmt->ms) {
		// Send the state of the NMT service (excluding the toggle bit).
		co_nmt_ec_send_res(nmt, nmt->st & ~CO_NMT_ST_TOGGLE);
#if !LELY_NO_CO_NG
	} else if (nmt->gt && nmt->ltf) {
		assert(nmt->lg_ind);
		// Notify the user of the occurrence of a life guarding error.
		diag(DIAG_INFO, 0, "NMT: life guarding event occurred");
		nmt->lg_state = CO_NMT_EC_OCCURRED;
		nmt->lg_ind(nmt, nmt->lg_state, nmt->lg_data);
#endif
	}

	return 0;
}

#if !LELY_NO_CO_MASTER
static int
co_nmt_cs_timer(const struct timespec *tp, void *data)
{
	(void)tp;
	co_nmt_t *nmt = data;
	assert(nmt);
	assert(nmt->master);

	co_unsigned16_t inhibit = co_dev_get_val_u16(nmt->dev, 0x102a, 0x00);

	can_timer_stop(nmt->cs_timer);

	struct timespec now = { 0, 0 };
	can_net_get_time(nmt->net, &now);

	struct can_msg msg;
	while (can_buf_peek(&nmt->buf, &msg, 1)) {
		assert(msg.id == CO_NMT_CS_CANID);
		assert(msg.len == 2);
		// Wait until the inhibit time has elapsed.
		if (inhibit && timespec_cmp(&now, &nmt->inhibit) < 0) {
			can_timer_start(nmt->cs_timer, nmt->net, &nmt->inhibit,
					NULL);
			return 0;
		}
		// Try to send the frame.
		if (can_net_send(nmt->net, &msg) == -1)
			return -1;
		can_buf_read(&nmt->buf, NULL, 1);
		// Update the expected state of the node(s).
		co_unsigned8_t st = 0;
		switch (msg.data[0]) {
		case CO_NMT_CS_START: st = CO_NMT_ST_START; break;
		case CO_NMT_CS_STOP: st = CO_NMT_ST_STOP; break;
		case CO_NMT_CS_ENTER_PREOP: st = CO_NMT_ST_PREOP; break;
		}
		co_unsigned8_t id = msg.data[1];
		assert(id <= CO_NUM_NODES);
		if (id) {
			if (nmt->slaves[id - 1].est)
				nmt->slaves[id - 1].est = st;
		} else {
			for (id = 1; id <= CO_NUM_NODES; id++) {
				if (nmt->slaves[id - 1].est)
					nmt->slaves[id - 1].est = st;
			}
		}
		// Update the inhibit time.
		can_net_get_time(nmt->net, &now);
		nmt->inhibit = now;
		timespec_add_usec(&nmt->inhibit, inhibit * 100);
	}

	return 0;
}
#endif // !LELY_NO_CO_MASTER

static void
co_nmt_st_ind(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st)
{
	assert(nmt);
	assert(nmt->st_ind);

	if (!id || id > CO_NUM_NODES)
		return;

#if !LELY_NO_CO_MASTER
	if (nmt->master) {
		nmt->slaves[id - 1].rst = st;

		// Update object 1F82 (Request NMT) with the NMT state.
		co_sub_t *sub = co_dev_find_sub(nmt->dev, 0x1f82, id);
		if (sub)
			co_sub_set_val_u8(sub, st & ~CO_NMT_ST_TOGGLE);
	}
#endif

	nmt->st_ind(nmt, id, st & ~CO_NMT_ST_TOGGLE, nmt->st_data);
}

#if !LELY_NO_CO_NG

#if !LELY_NO_CO_MASTER
static void
default_ng_ind(co_nmt_t *nmt, co_unsigned8_t id, int state, int reason,
		void *data)
{
	(void)data;

	co_nmt_on_ng(nmt, id, state, reason);
}
#endif

static void
default_lg_ind(co_nmt_t *nmt, int state, void *data)
{
	(void)data;

	co_nmt_on_lg(nmt, state);
}

#endif // !LELY_NO_CO_NG

static void
default_hb_ind(co_nmt_t *nmt, co_unsigned8_t id, int state, int reason,
		void *data)
{
	(void)data;

	co_nmt_on_hb(nmt, id, state, reason);
}

static void
default_st_ind(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, void *data)
{
	(void)data;

	co_nmt_on_st(nmt, id, st);
}

#if !LELY_NO_CO_NMT_BOOT || !LELY_NO_CO_NMT_CFG

static void
co_nmt_dn_ind(const co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		size_t size, size_t nbyte, void *data)
{
	co_nmt_t *nmt = data;
	assert(nmt);

	if (nmt->dn_ind)
		nmt->dn_ind(nmt, co_csdo_get_num(sdo), idx, subidx, size, nbyte,
				nmt->dn_data);
}

static void
co_nmt_up_ind(const co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		size_t size, size_t nbyte, void *data)
{
	co_nmt_t *nmt = data;
	assert(nmt);

	if (nmt->up_ind)
		nmt->up_ind(nmt, co_csdo_get_num(sdo), idx, subidx, size, nbyte,
				nmt->up_data);
}

#endif // !LELY_NO_CO_NMT_BOOT || !LELY_NO_CO_NMT_CFG

#if !LELY_NO_CO_TPDO

static void
co_nmt_tpdo_event_ind(co_unsigned16_t n, void *data)
{
	co_nmt_t *nmt = data;
	assert(nmt);

	co_nmt_on_tpdo_event(nmt, n);
}

#if !LELY_NO_CO_MPDO
static void
co_nmt_sam_mpdo_event_ind(co_unsigned16_t n, co_unsigned16_t idx,
		co_unsigned8_t subidx, void *data)
{
	co_nmt_t *nmt = data;
	assert(nmt);

	co_nmt_on_sam_mpdo_event(nmt, n, idx, subidx);
}
#endif

#endif // !LELY_NO_CO_TPDO

static void
co_nmt_enter(co_nmt_t *nmt, co_nmt_state_t *next)
{
	assert(nmt);

	while (next) {
		co_nmt_state_t *prev = nmt->state;
		nmt->state = next;

		if (prev && prev->on_leave)
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

#if !LELY_NO_CO_NMT_BOOT

static inline void
co_nmt_emit_boot(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, char es)
{
	assert(nmt);
	assert(nmt->state);
	assert(nmt->state->on_boot);

	co_nmt_enter(nmt, nmt->state->on_boot(nmt, id, st, es));
}

static co_nmt_state_t *
co_nmt_default_on_boot(
		co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, char es)
{
	(void)nmt;
	(void)id;
	(void)st;
	(void)es;

	return NULL;
}

#endif // !LELY_NO_CO_NMT_BOOT

static co_nmt_state_t *
co_nmt_init_on_cs(co_nmt_t *nmt, co_unsigned8_t cs)
{
	(void)nmt;

	switch (cs) {
	case CO_NMT_CS_RESET_NODE: return co_nmt_reset_node_state;
	default: return NULL;
	}
}

static co_nmt_state_t *
co_nmt_reset_node_on_enter(co_nmt_t *nmt)
{
	assert(nmt);

	diag(DIAG_INFO, 0, "NMT: entering reset application state");

#if !LELY_NO_CO_MASTER
	// Disable NMT slave management.
	co_nmt_slaves_fini(nmt);
	nmt->halt = 0;
#endif

	// Disable all services.
	co_nmt_srv_set(&nmt->srv, nmt, 0);

	// Disable heartbeat consumption.
	co_nmt_hb_fini(nmt);

	// Disable error control services.
	co_nmt_ec_fini(nmt);

	// Stop receiving NMT commands.
	can_recv_stop(nmt->recv_000);

#if !LELY_NO_CO_DCF_RESTORE
	// Reset application parameters.
	if (co_dev_read_dcf(nmt->dev, NULL, NULL, &nmt->dcf_node) == -1)
		diag(DIAG_ERROR, get_errc(),
				"unable to reset application parameters");
#endif

	nmt->st = CO_NMT_ST_RESET_NODE;
	co_nmt_st_ind(nmt, co_dev_get_id(nmt->dev), 0);

	if (nmt->cs_ind)
		nmt->cs_ind(nmt, CO_NMT_CS_RESET_NODE, nmt->cs_data);

	return co_nmt_reset_comm_state;
}

static co_nmt_state_t *
co_nmt_reset_comm_on_enter(co_nmt_t *nmt)
{
	assert(nmt);

	diag(DIAG_INFO, 0, "NMT: entering reset communication state");

#if !LELY_NO_CO_MASTER
	// Disable NMT slave management.
	co_nmt_slaves_fini(nmt);
	nmt->halt = 0;
#endif

	// Disable all services.
	co_nmt_srv_set(&nmt->srv, nmt, 0);

	// Disable heartbeat consumption.
	co_nmt_hb_fini(nmt);

	// Disable error control services.
	co_nmt_ec_fini(nmt);

	// Stop receiving NMT commands.
	can_recv_stop(nmt->recv_000);

	// Reset communication parameters.
	if (co_dev_read_dcf(nmt->dev, NULL, NULL, &nmt->dcf_comm) == -1)
		diag(DIAG_ERROR, get_errc(),
				"unable to reset communication parameters");

	// Update the node-ID if necessary.
	if (nmt->id != co_dev_get_id(nmt->dev)) {
		co_dev_set_id(nmt->dev, nmt->id);
		co_val_fini(CO_DEFTYPE_DOMAIN, &nmt->dcf_comm);
		if (co_dev_write_dcf(nmt->dev, 0x1000, 0x1fff, &nmt->dcf_comm)
				== -1)
			diag(DIAG_ERROR, get_errc(),
					"unable to store communication parameters");
	}

	// Load the NMT startup value.
	nmt->startup = co_dev_get_val_u32(nmt->dev, 0x1f80, 0x00);
#if !LELY_NO_CO_MASTER
	// Bit 0 of the NMT startup value determines whether we are a master or
	// a slave.
	nmt->master = !!(nmt->startup & 0x01);
#endif
	diag(DIAG_INFO, 0, "NMT: running as %s",
			co_nmt_is_master(nmt) ? "master" : "slave");

	nmt->st = CO_NMT_ST_RESET_COMM;
	co_nmt_st_ind(nmt, co_dev_get_id(nmt->dev), 0);

	// Start receiving NMT commands.
	if (!co_nmt_is_master(nmt))
		can_recv_start(nmt->recv_000, nmt->net, CO_NMT_CS_CANID, 0);

	// Enable LSS.
	co_nmt_srv_set(&nmt->srv, nmt, CO_NMT_SRV_LSS);

	if (nmt->cs_ind)
		nmt->cs_ind(nmt, CO_NMT_CS_RESET_COMM, nmt->cs_data);

#if !LELY_NO_CO_MASTER && !LELY_NO_CO_LSS
	// If LSS is required, invoked the user-defined callback function and
	// wait for the process to complete.
	if (nmt->master && nmt->lss_req) {
		nmt->lss_req(nmt, co_nmt_get_lss(nmt), nmt->lss_data);
		return NULL;
	}
#endif

	return co_nmt_bootup_state;
}

static co_nmt_state_t *
co_nmt_reset_comm_on_cs(co_nmt_t *nmt, co_unsigned8_t cs)
{
	(void)nmt;

	switch (cs) {
	case CO_NMT_CS_RESET_NODE: return co_nmt_reset_node_state;
	case CO_NMT_CS_RESET_COMM: return co_nmt_reset_comm_state;
	default: return NULL;
	}
}

static co_nmt_state_t *
co_nmt_bootup_on_enter(co_nmt_t *nmt)
{
	assert(nmt);

	// Don't enter the 'pre-operational' state if the node-ID is invalid.
	if (co_dev_get_id(nmt->dev) == 0xff) {
		diag(DIAG_INFO, 0, "NMT: unconfigured node-ID");
		return NULL;
	}

	// Enable error control services.
	co_nmt_ec_init(nmt);

	// Enable heartbeat consumption.
	co_nmt_hb_init(nmt);

	// Send the boot-up signal to notify the master we exist.
	co_nmt_ec_send_res(nmt, CO_NMT_ST_BOOTUP);

	return co_nmt_preop_state;
}

static co_nmt_state_t *
co_nmt_bootup_on_cs(co_nmt_t *nmt, co_unsigned8_t cs)
{
	(void)nmt;

	switch (cs) {
	case CO_NMT_CS_RESET_NODE: return co_nmt_reset_node_state;
	case CO_NMT_CS_RESET_COMM: return co_nmt_reset_comm_state;
	default: return NULL;
	}
}

static co_nmt_state_t *
co_nmt_preop_on_enter(co_nmt_t *nmt)
{
	assert(nmt);

	diag(DIAG_INFO, 0, "NMT: entering pre-operational state");

#if !LELY_NO_CO_MASTER
	// Disable NMT slave management.
	co_nmt_slaves_fini(nmt);
	nmt->halt = 0;
#endif

	// Enable all services except PDO.
	co_nmt_srv_set(&nmt->srv, nmt, CO_NMT_PREOP_SRV);

	nmt->st = CO_NMT_ST_PREOP | (nmt->st & CO_NMT_ST_TOGGLE);
	co_nmt_st_ind(nmt, co_dev_get_id(nmt->dev), nmt->st);

	if (nmt->cs_ind)
		nmt->cs_ind(nmt, CO_NMT_CS_ENTER_PREOP, nmt->cs_data);

	return co_nmt_startup(nmt);
}

static co_nmt_state_t *
co_nmt_preop_on_cs(co_nmt_t *nmt, co_unsigned8_t cs)
{
	(void)nmt;

	switch (cs) {
	case CO_NMT_CS_START: return co_nmt_start_state;
	case CO_NMT_CS_STOP: return co_nmt_stop_state;
	case CO_NMT_CS_RESET_NODE: return co_nmt_reset_node_state;
	case CO_NMT_CS_RESET_COMM: return co_nmt_reset_comm_state;
	default: return NULL;
	}
}

#if !LELY_NO_CO_NMT_BOOT
static co_nmt_state_t *
co_nmt_preop_on_boot(
		co_nmt_t *nmt, co_unsigned8_t id, co_unsigned8_t st, char es)
{
	assert(nmt);
	assert(nmt->master);
	assert(id && id <= CO_NUM_NODES);
	(void)st;

	// If the 'boot slave' process failed for a mandatory slave, halt the
	// network boot-up procedure.
	if ((nmt->slaves[id - 1].assignment & 0x09) == 0x09 && es && es != 'L')
		nmt->halt = 1;

	// Wait for any mandatory slaves that have not yet finished booting.
	int wait = nmt->halt;
	for (co_unsigned8_t id = 1; !wait && id <= CO_NUM_NODES; id++)
		wait = (nmt->slaves[id - 1].assignment & 0x09) == 0x09
				&& nmt->slaves[id - 1].boot;
	if (!wait) {
		trace("NMT: all mandatory slaves started successfully");
		return co_nmt_startup_slave(nmt);
	}
	return NULL;
}
#endif

static co_nmt_state_t *
co_nmt_start_on_enter(co_nmt_t *nmt)
{
	assert(nmt);

	diag(DIAG_INFO, 0, "NMT: entering operational state");

#if !LELY_NO_CO_TPDO
	// Reset all Transmit-PDO events.
	for (int i = 0; i < CO_NUM_PDOS / LONG_BIT; i++)
		nmt->tpdo_event_mask[i] = 0;
#endif

	// Enable all services.
	co_nmt_srv_set(&nmt->srv, nmt, CO_NMT_START_SRV);

	nmt->st = CO_NMT_ST_START | (nmt->st & CO_NMT_ST_TOGGLE);
	co_nmt_st_ind(nmt, co_dev_get_id(nmt->dev), nmt->st);

#if !LELY_NO_CO_NMT_BOOT
	// If we're the master and bit 3 of the NMT startup value is 0 and bit 1
	// is 1, send the NMT start remote node command to all nodes (see Fig. 2
	// in CiA 302-2 version 4.1.0).
	if (nmt->master && (nmt->startup & 0x0a) == 0x02) {
		// Check if all slaves booted successfully.
		int boot = 1;
		for (co_unsigned8_t id = 1; boot && id <= CO_NUM_NODES; id++) {
			struct co_nmt_slave *slave = &nmt->slaves[id - 1];
			// Skip those slaves that are not in the network list.
			if (!(slave->assignment & 0x01))
				continue;
			// Check if the slave finished booting successfully and
			// can be started by the master.
			boot = slave->booted && (!slave->es || slave->es == 'L')
					&& !(slave->assignment & 0x04);
		}
		if (boot) {
			// Start all NMT slaves at once.
			co_nmt_cs_req(nmt, CO_NMT_CS_START, 0);
		} else {
			for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++) {
				struct co_nmt_slave *slave =
						&nmt->slaves[id - 1];
				// Skip those slaves that are not in the network
				// list (bit 0), or that we are not allowed to
				// boot (bit 2).
				if ((slave->assignment & 0x05) != 0x05)
					continue;
				// Only start slaves that have finished booting
				// successfully and are not already (expected to
				// be) operational.
				if (slave->booted
						&& (!slave->es || slave->es == 'L')
						&& slave->est != CO_NMT_ST_START)
					co_nmt_cs_req(nmt, CO_NMT_CS_START, id);
			}
		}
	}
#endif

	if (nmt->cs_ind)
		nmt->cs_ind(nmt, CO_NMT_CS_START, nmt->cs_data);

	return NULL;
}

static co_nmt_state_t *
co_nmt_start_on_cs(co_nmt_t *nmt, co_unsigned8_t cs)
{
	(void)nmt;

	switch (cs) {
	case CO_NMT_CS_STOP: return co_nmt_stop_state;
	case CO_NMT_CS_ENTER_PREOP: return co_nmt_preop_state;
	case CO_NMT_CS_RESET_NODE: return co_nmt_reset_node_state;
	case CO_NMT_CS_RESET_COMM: return co_nmt_reset_comm_state;
	default: return NULL;
	}
}

static co_nmt_state_t *
co_nmt_stop_on_enter(co_nmt_t *nmt)
{
	assert(nmt);

	diag(DIAG_INFO, 0, "NMT: entering stopped state");

	// Disable all services (except LSS).
	co_nmt_srv_set(&nmt->srv, nmt, CO_NMT_STOP_SRV);

	nmt->st = CO_NMT_ST_STOP | (nmt->st & CO_NMT_ST_TOGGLE);
	co_nmt_st_ind(nmt, co_dev_get_id(nmt->dev), nmt->st);

	if (nmt->cs_ind)
		nmt->cs_ind(nmt, CO_NMT_CS_STOP, nmt->cs_data);

	return NULL;
}

static co_nmt_state_t *
co_nmt_stop_on_cs(co_nmt_t *nmt, co_unsigned8_t cs)
{
	(void)nmt;

	switch (cs) {
	case CO_NMT_CS_START: return co_nmt_start_state;
	case CO_NMT_CS_ENTER_PREOP: return co_nmt_preop_state;
	case CO_NMT_CS_RESET_NODE: return co_nmt_reset_node_state;
	case CO_NMT_CS_RESET_COMM: return co_nmt_reset_comm_state;
	default: return NULL;
	}
}

static co_nmt_state_t *
co_nmt_startup(co_nmt_t *nmt)
{
	assert(nmt);

#if !LELY_NO_CO_MASTER
	if (nmt->master)
		return co_nmt_startup_master(nmt);
#endif
	return co_nmt_startup_slave(nmt);
}

#if !LELY_NO_CO_MASTER
static co_nmt_state_t *
co_nmt_startup_master(co_nmt_t *nmt)
{
	assert(nmt);
	assert(nmt->master);

	// Enable NMT slave management.
	co_nmt_slaves_init(nmt);

#if LELY_NO_CO_NMT_BOOT
	// Send the NMT 'reset communication' command to all slaves.
	co_nmt_cs_req(nmt, CO_NMT_CS_RESET_COMM, 0);

	return co_nmt_startup_slave(nmt);
#else
	// Check if any node has the keep-alive bit set.
	int keep = 0;
	for (co_unsigned8_t id = 1; !keep && id <= CO_NUM_NODES; id++)
		keep = (nmt->slaves[id - 1].assignment & 0x11) == 0x11;

	// Send the NMT 'reset communication' command to all slaves with
	// the keep-alive bit _not_ set. This includes slaves which are not in
	// the network list.
	if (keep) {
		for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++) {
			// Do not reset the master itself.
			if (id == co_dev_get_id(nmt->dev))
				continue;
			if ((nmt->slaves[id - 1].assignment & 0x11) != 0x11)
				co_nmt_cs_req(nmt, CO_NMT_CS_RESET_COMM, id);
		}
	} else {
		co_nmt_cs_req(nmt, CO_NMT_CS_RESET_COMM, 0);
	}

	// Start the 'boot slave' processes.
	switch (co_nmt_slaves_boot(nmt)) {
	case -1:
		// Halt the network boot-up procedure if the 'boot slave'
		// process failed for a mandatory slave.
		nmt->halt = 1;
		return NULL;
	case 0: return co_nmt_startup_slave(nmt);
	default:
		// Wait for all mandatory slaves to finish booting.
		trace("NMT: waiting for mandatory slaves to start");
		return NULL;
	}
#endif
}
#endif

static co_nmt_state_t *
co_nmt_startup_slave(co_nmt_t *nmt)
{
	assert(nmt);

	// Enter the operational state automatically if bit 2 of the NMT startup
	// value is 0.
	return (nmt->startup & 0x04) ? NULL : co_nmt_start_state;
}

static void
co_nmt_ec_init(co_nmt_t *nmt)
{
	assert(nmt);

	// Enable life guarding or heartbeat production.
#if !LELY_NO_CO_NG
	nmt->gt = co_dev_get_val_u16(nmt->dev, 0x100c, 0x00);
	nmt->ltf = co_dev_get_val_u8(nmt->dev, 0x100d, 0x00);
#endif
	nmt->ms = co_dev_get_val_u16(nmt->dev, 0x1017, 0x00);

#if !LELY_NO_CO_NG
	nmt->lg_state = CO_NMT_EC_RESOLVED;
#endif

	co_nmt_ec_update(nmt);
}

static void
co_nmt_ec_fini(co_nmt_t *nmt)
{
	assert(nmt);

	// Disable life guarding and heartbeat production.
#if !LELY_NO_CO_NG
	nmt->gt = 0;
	nmt->ltf = 0;
#endif
	nmt->ms = 0;

#if !LELY_NO_CO_NG
	nmt->lg_state = CO_NMT_EC_RESOLVED;
#endif

	co_nmt_ec_update(nmt);
}

static void
co_nmt_ec_update(co_nmt_t *nmt)
{
	assert(nmt);

#if !LELY_NO_CO_NG
	// Heartbeat production has precedence over life guarding.
	int lt = nmt->ms ? 0 : nmt->gt * nmt->ltf;
#if !LELY_NO_CO_MASTER
	// Disable life guarding for the master.
	if (nmt->master)
		lt = 0;
#endif

	if (lt) {
		// Start the CAN frame receiver for node guarding RTRs.
		can_recv_start(nmt->recv_700, nmt->net,
				CO_NMT_EC_CANID(co_dev_get_id(nmt->dev)),
				CAN_FLAG_RTR);
	} else {
		can_recv_stop(nmt->recv_700);
	}
#endif

	// Start the CAN timer for heartbeat production or life guarding, if
	// necessary.
#if LELY_NO_CO_NG
	int ms = nmt->ms;
#else
	int ms = nmt->ms ? nmt->ms : lt;
#endif
	if (ms) {
		struct timespec interval = { ms / 1000, (ms % 1000) * 1000000 };
		can_timer_start(nmt->ec_timer, nmt->net, NULL, &interval);
	} else {
		can_timer_stop(nmt->ec_timer);
	}
}

static int
co_nmt_ec_send_res(co_nmt_t *nmt, co_unsigned8_t st)
{
	assert(nmt);

	struct can_msg msg = CAN_MSG_INIT;
	msg.id = CO_NMT_EC_CANID(co_dev_get_id(nmt->dev));
	msg.len = 1;
	msg.data[0] = st;

	return can_net_send(nmt->net, &msg);
}

static void
co_nmt_hb_init(co_nmt_t *nmt)
{
	assert(nmt);

	// Create and initialize the heartbeat consumers.
#if LELY_NO_MALLOC
	memset(nmt->hbs, 0, CO_NMT_MAX_NHB * sizeof(*nmt->hbs));
#else
	assert(!nmt->hbs);
#endif
	assert(!nmt->nhb);
	co_obj_t *obj_1016 = co_dev_find_obj(nmt->dev, 0x1016);
	if (obj_1016) {
		nmt->nhb = co_obj_get_val_u8(obj_1016, 0x00);
#if LELY_NO_MALLOC
		if (nmt->nhb > CO_NMT_MAX_NHB) {
			set_errnum(ERRNUM_NOMEM);
#else // !LELY_NO_MALLOC
		nmt->hbs = calloc(nmt->nhb, sizeof(*nmt->hbs));
		if (!nmt->hbs && nmt->nhb) {
#if !LELY_NO_ERRNO
			set_errc(errno2c(errno));
#endif
#endif // !LELY_NO_MALLOC
			diag(DIAG_ERROR, get_errc(),
					"unable to create heartbeat consumers");
			nmt->nhb = 0;
		}
	}

	for (co_unsigned8_t i = 0; i < nmt->nhb; i++) {
		nmt->hbs[i] = co_nmt_hb_create(nmt->net, nmt);
		if (!nmt->hbs[i]) {
			diag(DIAG_ERROR, get_errc(),
					"unable to create heartbeat consumer 0x%02X",
					(co_unsigned8_t)(i + 1));
			continue;
		}

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
#if !LELY_NO_MALLOC
	free(nmt->hbs);
	nmt->hbs = NULL;
#endif
	nmt->nhb = 0;
}

#if !LELY_NO_CO_MASTER

#if !LELY_NO_CO_NMT_BOOT || !LELY_NO_CO_NMT_CFG
static co_nmt_hb_t *
co_nmt_hb_find(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned16_t *pms)
{
	assert(nmt);
	assert(id && id <= CO_NUM_NODES);

	const co_obj_t *obj_1016 = co_dev_find_obj(nmt->dev, 0x1016);
	if (!obj_1016)
		return NULL;

	for (co_unsigned8_t i = 0; i < nmt->nhb; i++) {
		co_unsigned32_t val = co_obj_get_val_u32(obj_1016, i + 1);
		if (id == ((val >> 16) & 0xff)) {
			if (pms)
				*pms = val & 0xffff;
			return nmt->hbs[i];
		}
	}
	return NULL;
}
#endif

static void
co_nmt_slaves_init(co_nmt_t *nmt)
{
	assert(nmt);
	assert(nmt->master);

	co_nmt_slaves_fini(nmt);

	for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++)
		// Start listening for boot-up notifications.
		can_recv_start(nmt->slaves[id - 1].recv, nmt->net,
				CO_NMT_EC_CANID(id), 0);

	co_obj_t *obj_1f81 = co_dev_find_obj(nmt->dev, 0x1f81);
	if (!obj_1f81)
		return;

	co_unsigned8_t n = co_obj_get_val_u8(obj_1f81, 0x00);
	for (co_unsigned8_t i = 0; i < MIN(n, CO_NUM_NODES); i++)
		nmt->slaves[i].assignment = co_obj_get_val_u32(obj_1f81, i + 1);
}

static void
co_nmt_slaves_fini(co_nmt_t *nmt)
{
	assert(nmt);

	for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++) {
		struct co_nmt_slave *slave = &nmt->slaves[id - 1];

		can_recv_stop(slave->recv);
#if !LELY_NO_CO_NG
		can_timer_stop(slave->timer);
#endif

		slave->assignment = 0;
		slave->est = 0;
		slave->rst = 0;

#if !LELY_NO_CO_NMT_BOOT
		slave->es = 0;

		slave->booting = 0;
		slave->booted = 0;

		co_nmt_boot_destroy(slave->boot);
		slave->boot = NULL;
#endif
		slave->bootup = 0;

#if !LELY_NO_CO_NMT_CFG
		slave->configuring = 0;

		co_nmt_cfg_destroy(slave->cfg);
		slave->cfg = NULL;
		slave->cfg_con = NULL;
		slave->cfg_data = NULL;
#endif

#if !LELY_NO_CO_NG
		slave->gt = 0;
		slave->ltf = 0;
		slave->rtr = 0;
		slave->ng_state = CO_NMT_EC_RESOLVED;
#endif
	}
}

#if !LELY_NO_CO_NMT_BOOT
static int
co_nmt_slaves_boot(co_nmt_t *nmt)
{
	assert(nmt);
	assert(nmt->master);

	int res = 0;
	for (co_unsigned8_t id = 1; id <= CO_NUM_NODES; id++) {
		struct co_nmt_slave *slave = &nmt->slaves[id - 1];
		// Skip those slaves that are not in the network list (bit 0).
		if ((slave->assignment & 0x01) != 0x01)
			continue;
		int mandatory = !!(slave->assignment & 0x08);
		// Wait for all mandatory slaves to finish booting.
		if (!res && mandatory)
			res = 1;
		// Optional slaves with the keep-alive bit _not_ set are booted
		// when we receive their boot-up signal.
		if (!mandatory && !(slave->assignment & 0x10))
			continue;
		// Halt the network boot-up procedure if the 'boot slave'
		// process failed for a mandatory slave with the keep-alive bit
		// set.
		if (co_nmt_boot_req(nmt, id, nmt->timeout) == -1 && mandatory)
			res = -1;
	}
	return res;
}
#endif

static int
co_nmt_chk_bootup_slaves(const co_nmt_t *nmt)
{
	for (co_unsigned8_t node_id = 1; node_id <= CO_NUM_NODES; node_id++) {
		const struct co_nmt_slave *const slave =
				&nmt->slaves[node_id - 1];
		// Skip those slaves that are not in the network list (bit 0).
		if ((slave->assignment & 0x01) != 0x01)
			continue;
		// Skip non-mandatory slaves (bit 3).
		if ((slave->assignment & 0x08) != 0x08)
			continue;
		// Check if we have received a boot-up message from a slave.
		if (!slave->bootup)
			return 0;
	}
	return 1;
}

#endif // !LELY_NO_CO_MASTER
