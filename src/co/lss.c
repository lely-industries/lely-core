/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the Layer Setting Services (LSS) and protocols functions.
 *
 * @see lely/co/lss.h
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

#if !LELY_NO_CO_LSS

#include <lely/co/lss.h>
#include <lely/co/nmt.h>
#include <lely/co/obj.h>
#include <lely/co/val.h>
#include <lely/util/endian.h>
#include <lely/util/errnum.h>
#include <lely/util/time.h>

#include <assert.h>
#include <stdlib.h>

struct __co_lss_state;
/// An opaque CANopen LSS state type.
typedef const struct __co_lss_state co_lss_state_t;

/// A CANopen LSS master/slave service.
struct __co_lss {
	/// A pointer to an NMT master/slave service.
	co_nmt_t *nmt;
	/// A pointer to a CAN network interface.
	can_net_t *net;
	/// A pointer to a CANopen device.
	co_dev_t *dev;
	/// A pointer to the current state.
	co_lss_state_t *state;
#if !LELY_NO_CO_MASTER
	/// A flag specifying whether the LSS service is a master or a slave.
	int master;
	/// The inhibit time (in multiples of 100 microseconds).
	co_unsigned16_t inhibit;
	/// The index of the next frame to be sent.
	int next;
#endif
	/// A pointer to the CAN frame receiver.
	can_recv_t *recv;
#if !LELY_NO_CO_MASTER
	/// The timeout (in milliseconds).
	int timeout;
	/// A pointer to the CAN timer.
	can_timer_t *timer;
#endif
	/// The expected command specifier.
	co_unsigned8_t cs;
	/// The LSSPos value.
	co_unsigned8_t lsspos;
#if !LELY_NO_CO_MASTER
	/// The lower bound of the LSS address used during the Slowscan service.
	struct co_id lo;
	/// The upper bound of the LSS address used during the Slowscan service.
	struct co_id hi;
	/// The mask used during the Fastscan service.
	struct co_id mask;
	/// The least-significant bit being checked during the Fastscan service.
	co_unsigned8_t bitchk;
	/**
	 * The index of the current LSS number being checked during the Fastscan
	 * service.
	 */
	co_unsigned8_t lsssub;
	/// The received error code.
	co_unsigned8_t err;
	/// The received implementation-specific error code.
	co_unsigned8_t spec;
	/// The received LSS number.
	co_unsigned32_t lssid;
	/// The received node-ID.
	co_unsigned8_t nid;
	/// The LSS address obtained from the LSS Slowscan or Fastscan service.
	struct co_id id;
#endif
	/// A pointer to the 'activate bit timing' indication function.
	co_lss_rate_ind_t *rate_ind;
	/// A pointer to user-specified data for #rate_ind.
	void *rate_data;
	/// A pointer to the 'store configuration' indication function.
	co_lss_store_ind_t *store_ind;
	/// A pointer to user-specified data for #store_ind.
	void *store_data;
#if !LELY_NO_CO_MASTER
	/// A pointer to the command indication function.
	co_lss_cs_ind_t *cs_ind;
	/// A pointer to user-specified data for #cs_ind.
	void *cs_data;
	/// A pointer to the error indication function.
	co_lss_err_ind_t *err_ind;
	/// A pointer to user-specified data for #err_ind.
	void *err_data;
	/// A pointer to the inquire identity indication function.
	co_lss_lssid_ind_t *lssid_ind;
	/// A pointer to user-specified data for #lssid_ind.
	void *lssid_data;
	/// A pointer to the inquire node-ID indication function.
	co_lss_nid_ind_t *nid_ind;
	/// A pointer to user-specified data for #nid_ind.
	void *nid_data;
	/// A pointer to the identify remote slave indication function.
	co_lss_scan_ind_t *scan_ind;
	/// A pointer to user-specified data for #scan_ind.
	void *scan_data;
#endif
};

/// The CAN receive callback function for an LSS service. @see can_recv_func_t
static int co_lss_recv(const struct can_msg *msg, void *data);

#if !LELY_NO_CO_MASTER
/// The CAN timer callback function for an LSS service. @see can_timer_func_t
static int co_lss_timer(const struct timespec *tp, void *data);
#endif

/**
 * Enters the specified state of an LSS service and invokes the exit and entry
 * functions.
 */
static void co_lss_enter(co_lss_t *lss, co_lss_state_t *next);

/**
 * Invokes the 'CAN frame received' transition function of the current state of
 * an LSS service.
 *
 * @param lss a pointer to an LSS service.
 * @param msg a pointer to the received CAN frame.
 */
static inline void co_lss_emit_recv(co_lss_t *lss, const struct can_msg *msg);

#if !LELY_NO_CO_MASTER
/**
 * Invokes the 'timeout' transition function of the current state of an LSS
 * service.
 *
 * @param lss a pointer to an LSS service.
 * @param tp  a pointer to the current time.
 */
static inline void co_lss_emit_time(co_lss_t *lss, const struct timespec *tp);
#endif

/// A CANopen LSS state.
struct __co_lss_state {
	/// A pointer to the function invoked when a new state is entered.
	co_lss_state_t *(*on_enter)(co_lss_t *lss);
	/**
	 * A pointer to the transition function invoked when a CAN frame has
	 * been received.
	 *
	 * @param lss a pointer to an LSS service.
	 * @param msg a pointer to the received CAN frame.
	 *
	 * @returns a pointer to the next state.
	 */
	co_lss_state_t *(*on_recv)(co_lss_t *lss, const struct can_msg *msg);
#if !LELY_NO_CO_MASTER
	/**
	 * A pointer to the transition function invoked when a timeout occurs.
	 *
	 * @param lss a pointer to an LSS service.
	 * @param tp  a pointer to the current time.
	 *
	 * @returns a pointer to the next state.
	 */
	co_lss_state_t *(*on_time)(co_lss_t *lss, const struct timespec *tp);
#endif
	/// A pointer to the function invoked when the current state is left.
	void (*on_leave)(co_lss_t *lss);
};

#define LELY_CO_DEFINE_STATE(name, ...) \
	static co_lss_state_t *const name = &(co_lss_state_t){ __VA_ARGS__ };

/// The 'stopped' state of an LSS master or slave.
LELY_CO_DEFINE_STATE(co_lss_stopped_state, NULL)

/// The entry function of the 'waiting' state an LSS master or slave.
static co_lss_state_t *co_lss_wait_on_enter(co_lss_t *lss);

/// The 'waiting' state of an LSS master or slave.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_wait_state,
	.on_enter = &co_lss_wait_on_enter
)
// clang-format on

/// The entry function of the 'waiting' state of an LSS slave.
static co_lss_state_t *co_lss_wait_slave_on_enter(co_lss_t *lss);

/**
 * The 'CAN frame received' transition function of the 'waiting' state of an LSS
 * slave.
 */
static co_lss_state_t *co_lss_wait_slave_on_recv(
		co_lss_t *lss, const struct can_msg *msg);

/// The 'waiting' state of an LSS slave.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_wait_slave_state,
	.on_enter = &co_lss_wait_slave_on_enter,
	.on_recv = &co_lss_wait_slave_on_recv
)
// clang-format on

/**
 * The 'CAN frame received' transition function of the 'configuration' state of
 * an LSS slave.
 */
static co_lss_state_t *co_lss_cfg_on_recv(
		co_lss_t *lss, const struct can_msg *msg);

/// The 'configuration' state of an LSS slave.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_cfg_state,
	.on_recv = &co_lss_cfg_on_recv
)
// clang-format on

#if !LELY_NO_CO_MASTER

/// The 'CAN frame received' transition function of the command received state.
static co_lss_state_t *co_lss_cs_on_recv(
		co_lss_t *lss, const struct can_msg *msg);

/// The 'timeout' transition function of the command received state.
static co_lss_state_t *co_lss_cs_on_time(
		co_lss_t *lss, const struct timespec *tp);

/// The command received state.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_cs_state,
	.on_recv = &co_lss_cs_on_recv,
	.on_time = &co_lss_cs_on_time
)
// clang-format on

/// The entry function of the command received finalization state.
static co_lss_state_t *co_lss_cs_fini_on_enter(co_lss_t *lss);

/// The exit function of the command received finalization state.
static void co_lss_cs_fini_on_leave(co_lss_t *lss);

/// The command received finalization state.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_cs_fini_state,
	.on_enter = &co_lss_cs_fini_on_enter,
	.on_leave = &co_lss_cs_fini_on_leave
)
// clang-format on

/// The entry function of the 'switch state selective' state.
static co_lss_state_t *co_lss_switch_sel_on_enter(co_lss_t *lss);

/// The 'timeout' transition function of the 'switch state selective' state.
static co_lss_state_t *co_lss_switch_sel_on_time(
		co_lss_t *lss, const struct timespec *tp);

/// The 'switch state selective' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_switch_sel_state,
	.on_enter = &co_lss_switch_sel_on_enter,
	.on_time = &co_lss_switch_sel_on_time
)
// clang-format on

/// The 'CAN frame received' transition function of the error received state.
static co_lss_state_t *co_lss_err_on_recv(
		co_lss_t *lss, const struct can_msg *msg);

/// The 'timeout' transition function of the error received state.
static co_lss_state_t *co_lss_err_on_time(
		co_lss_t *lss, const struct timespec *tp);

/// The exit function of the error received state.
static void co_lss_err_on_leave(co_lss_t *lss);

/// The error received state.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_err_state,
	.on_recv = &co_lss_err_on_recv,
	.on_time = &co_lss_err_on_time,
	.on_leave = &co_lss_err_on_leave
)
// clang-format on

/// The 'CAN frame received' transition function of the inquire identity state.
static co_lss_state_t *co_lss_lssid_on_recv(
		co_lss_t *lss, const struct can_msg *msg);

/// The 'timeout' transition function of the inquire identity state.
static co_lss_state_t *co_lss_lssid_on_time(
		co_lss_t *lss, const struct timespec *tp);

/// The exit function of the inquire identity state.
static void co_lss_lssid_on_leave(co_lss_t *lss);

/// The inquire identity state.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_lssid_state,
	.on_recv = &co_lss_lssid_on_recv,
	.on_time = &co_lss_lssid_on_time,
	.on_leave = &co_lss_lssid_on_leave
)
// clang-format on

/// The 'CAN frame received' transition function of the inquire node-ID state.
static co_lss_state_t *co_lss_nid_on_recv(
		co_lss_t *lss, const struct can_msg *msg);

/// The 'timeout' transition function of the inquire node-ID state.
static co_lss_state_t *co_lss_nid_on_time(
		co_lss_t *lss, const struct timespec *tp);

/// The exit function of the inquire node-ID state.
static void co_lss_nid_on_leave(co_lss_t *lss);

/// The inquire node-ID state.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_nid_state,
	.on_recv = &co_lss_nid_on_recv,
	.on_time = &co_lss_nid_on_time,
	.on_leave = &co_lss_nid_on_leave
)
// clang-format on

/// The entry function of the 'identify remote slave' state.
static co_lss_state_t *co_lss_id_slave_on_enter(co_lss_t *lss);

/// The 'timeout' transition function of the 'identify remote slave' state.
static co_lss_state_t *co_lss_id_slave_on_time(
		co_lss_t *lss, const struct timespec *tp);

/// The 'identify remote slave' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_id_slave_state,
	.on_enter = &co_lss_id_slave_on_enter,
	.on_time = &co_lss_id_slave_on_time
)
// clang-format on

/// The entry function of the Slowscan initialization state.
static co_lss_state_t *co_lss_slowscan_init_on_enter(co_lss_t *lss);

/**
 * The 'CAN frame received' transition function of the Slowscan initialization
 * state.
 */
static co_lss_state_t *co_lss_slowscan_init_on_recv(
		co_lss_t *lss, const struct can_msg *msg);

/// The 'timeout' transition function of the Slowscan initialization state.
static co_lss_state_t *co_lss_slowscan_init_on_time(
		co_lss_t *lss, const struct timespec *tp);

/// The Slowscan initialization state.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_slowscan_init_state,
	.on_enter = &co_lss_slowscan_init_on_enter,
	.on_recv = &co_lss_slowscan_init_on_recv,
	.on_time = &co_lss_slowscan_init_on_time
)
// clang-format on

/// The entry function of the Slowscan scanning state.
static co_lss_state_t *co_lss_slowscan_scan_on_enter(co_lss_t *lss);

/// The 'CAN frame received' transition function of the Slowscan scanning state.
static co_lss_state_t *co_lss_slowscan_scan_on_recv(
		co_lss_t *lss, const struct can_msg *msg);

/// The 'timeout' transition function of the Slowscan scanning state.
static co_lss_state_t *co_lss_slowscan_scan_on_time(
		co_lss_t *lss, const struct timespec *tp);

static co_lss_state_t *co_lss_slowscan_scan_on_res(co_lss_t *lss, int timeout);

/// The Slowscan scanning state.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_slowscan_scan_state,
	.on_enter = &co_lss_slowscan_scan_on_enter,
	.on_recv = &co_lss_slowscan_scan_on_recv,
	.on_time = &co_lss_slowscan_scan_on_time
)
// clang-format on

/// The 'CAN frame received' transition function of the Slowscan waiting state.
static co_lss_state_t *co_lss_slowscan_wait_on_recv(
		co_lss_t *lss, const struct can_msg *msg);

/// The 'timeout' transition function of the Slowscan waiting state.
static co_lss_state_t *co_lss_slowscan_wait_on_time(
		co_lss_t *lss, const struct timespec *tp);

/// The Slowscan waiting state.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_slowscan_wait_state,
	.on_recv = &co_lss_slowscan_wait_on_recv,
	.on_time = &co_lss_slowscan_wait_on_time
)
// clang-format on

/// The entry function of the Slowscan 'switch state selective' state.
static co_lss_state_t *co_lss_slowscan_switch_on_enter(co_lss_t *lss);

/**
 * The 'CAN frame received' transition function of the Slowscan 'switch state
 * selective' state.
 */
static co_lss_state_t *co_lss_slowscan_switch_on_recv(
		co_lss_t *lss, const struct can_msg *msg);

/**
 * The 'timeout' transition function of the Slowscan 'switch state selective'
 * state.
 */
static co_lss_state_t *co_lss_slowscan_switch_on_time(
		co_lss_t *lss, const struct timespec *tp);

/// The Slowscan 'switch state selective' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_slowscan_switch_state,
	.on_enter = &co_lss_slowscan_switch_on_enter,
	.on_recv = &co_lss_slowscan_switch_on_recv,
	.on_time = &co_lss_slowscan_switch_on_time
)
// clang-format on

/// The entry function of the Slowscan finalization state.
static co_lss_state_t *co_lss_slowscan_fini_on_enter(co_lss_t *lss);

/// The exit function of the Slowscan finalization state.
static void co_lss_slowscan_fini_on_leave(co_lss_t *lss);

/// The Slowscan finalization state.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_slowscan_fini_state,
	.on_enter = &co_lss_slowscan_fini_on_enter,
	.on_leave = &co_lss_slowscan_fini_on_leave
)
// clang-format on

/**
 * The 'CAN frame received' transition function of the Fastscan initialization
 * state.
 */
static co_lss_state_t *co_lss_fastscan_init_on_recv(
		co_lss_t *lss, const struct can_msg *msg);

/// The 'timeout' transition function of the Fastscan initialization state.
static co_lss_state_t *co_lss_fastscan_init_on_time(
		co_lss_t *lss, const struct timespec *tp);

/// The Fastscan initialization state.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_fastscan_init_state,
	.on_recv = &co_lss_fastscan_init_on_recv,
	.on_time = &co_lss_fastscan_init_on_time
)
// clang-format on

/// The entry function of the Fastscan scanning state.
static co_lss_state_t *co_lss_fastscan_scan_on_enter(co_lss_t *lss);

/// The 'CAN frame received' transition function of the Fastscan scanning state.
static co_lss_state_t *co_lss_fastscan_scan_on_recv(
		co_lss_t *lss, const struct can_msg *msg);

/// The 'timeout' transition function of the Fastscan scanning state.
static co_lss_state_t *co_lss_fastscan_scan_on_time(
		co_lss_t *lss, const struct timespec *tp);

static co_lss_state_t *co_lss_fastscan_scan_on_res(co_lss_t *lss, int timeout);

/// The Fastscan scanning state.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_fastscan_scan_state,
	.on_enter = &co_lss_fastscan_scan_on_enter,
	.on_recv = &co_lss_fastscan_scan_on_recv,
	.on_time = &co_lss_fastscan_scan_on_time
)
// clang-format on

/// The 'CAN frame received' transition function of the Fastscan waiting state.
static co_lss_state_t *co_lss_fastscan_wait_on_recv(
		co_lss_t *lss, const struct can_msg *msg);

/// The 'timeout' transition function of the Fastscan waiting state.
static co_lss_state_t *co_lss_fastscan_wait_on_time(
		co_lss_t *lss, const struct timespec *tp);

/// The Fastscan waiting state.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_fastscan_wait_state,
	.on_recv = &co_lss_fastscan_wait_on_recv,
	.on_time = &co_lss_fastscan_wait_on_time
)
// clang-format on

/// The entry function of the Fastscan finalization state.
static co_lss_state_t *co_lss_fastscan_fini_on_enter(co_lss_t *lss);

/// The exit function of the Fastscan finalization state.
static void co_lss_fastscan_fini_on_leave(co_lss_t *lss);

/// The Fastscan finalization state.
// clang-format off
LELY_CO_DEFINE_STATE(co_lss_fastscan_fini_state,
	.on_enter = &co_lss_fastscan_fini_on_enter,
	.on_leave = &co_lss_fastscan_fini_on_leave
)
// clang-format on

#endif // !LELY_NO_CO_MASTER

#undef LELY_CO_DEFINE_STATE

/**
 * Implements the switch state selective service for an LSS slave. See Fig. 32
 * in CiA 305 version 3.0.0.
 *
 * @param lss a pointer to an LSS slave service.
 * @param cs  the command specifier (in the range [0x40..0x43]).
 * @param id  the current LSS number to be checked.
 *
 * @returns a pointer to the next state.
 */
static co_lss_state_t *co_lss_switch_sel(
		co_lss_t *lss, co_unsigned8_t cs, co_unsigned32_t id);

/**
 * Implements the LSS identify remote slave service for an LSS slave. See
 * Fig. 42 in CiA 305 version 3.0.0.
 *
 * @param lss a pointer to an LSS slave service.
 * @param cs  the command specifier (in the range [0x46..0x4b]).
 * @param id  the current LSS number to be checked.
 */
static void co_lss_id_slave(
		co_lss_t *lss, co_unsigned8_t cs, co_unsigned32_t id);

/**
 * Implements the LSS identify non-configured remote slave service for an LSS
 * slave. See Fig. 44 in CiA 305 version 3.0.0.
 */
static void co_lss_id_non_cfg_slave(const co_lss_t *lss);

/**
 * Implements the LSS fastscan service for an LSS slave. See Fig. 46 in CiA 305
 * version 3.0.0.
 *
 * @param lss     a pointer to an LSS slave service.
 * @param id      the value of LSS number which is currently determined by the
 *                LSS master.
 * @param bitchk  the least significant bit of <b>id</b> to be checked.
 * @param lsssub  the index of <b>id</b> in the LSS address (in the range
 *                [0..3]).
 * @param lssnext the value if <b>lsssub</b> in the next request.
 *
 * @returns a pointer to the next state.
 */
static co_lss_state_t *co_lss_fastscan(co_lss_t *lss, co_unsigned32_t id,
		co_unsigned8_t bitchk, co_unsigned8_t lsssub,
		co_unsigned8_t lssnext);

/**
 * Initializes an LSS request CAN frame.
 *
 * @param lss a pointer to an LSS service.
 * @param msg a pointer to the CAN frame to be initialized.
 * @param cs  the command specifier.
 */
static void co_lss_init_req(
		const co_lss_t *lss, struct can_msg *msg, co_unsigned8_t cs);

#if !LELY_NO_CO_MASTER

/**
 * Sends a single frame of a switch state selective request (see Fig. 32 in CiA
 * 305 version 3.0.0). After the last frame has been sent, this function invokes
 * `co_lss_init_ind(lss, 0x44)` to prepare the master for the response from the
 * slave.
 *
 * @param lss a pointer to an LSS master service.
 * @param id  a pointer to the LSS address of the slave to be configured.
 *
 * @returns 0 on success, or -1 on error.
 */
static int co_lss_send_switch_sel_req(co_lss_t *lss, const struct co_id *id);

/**
 * Sends a single frame of an LSS identify remote slave request (see Fig. 42 in
 * CiA 305 version 3.0.0). After the last frame has been sent, this function
 * invokes `co_lss_init_ind(lss, 0x4f)` to prepare the master for the response
 * from the slave.
 *
 * @param lss a pointer to an LSS master service.
 * @param lo  a pointer to the lower bound of the LSS address.
 * @param hi  a pointer to the upper bound of the LSS address. The vendor-ID and
 *            product-code MUST be the same as in *<b>lo</b>.
 *
 * @returns 0 on success, or -1 on error.
 */
static int co_lss_send_id_slave_req(
		co_lss_t *lss, const struct co_id *lo, const struct co_id *hi);

/**
 * Sends an LSS Fastscan request (see Fig. 46 in CiA 305 version 3.0.0).
 *
 * @param lss     a pointer to an LSS master service.
 * @param id      the current LSS number to be checked.
 * @param bitchk  the least-significant bit to be checked.
 * @param lsssub  the index of <b>id</b>.
 * @param lssnext the index of the next LSS number to be checked.
 *
 * @returns 0 on success, or -1 on error.
 */
static int co_lss_send_fastscan_req(const co_lss_t *lss, co_unsigned32_t id,
		co_unsigned8_t bitchk, co_unsigned8_t lsssub,
		co_unsigned8_t lssnext);

/**
 * Prepares an LSS master to receive an indication from a slave.
 *
 * @param lss a pointer to an LSS master service.
 * @param cs  the expected command specifier.
 */
static void co_lss_init_ind(co_lss_t *lss, co_unsigned8_t cs);

/**
 * Returns a pointer to the specified number in an LSS address.
 *
 * @param id  a pointer to an LSS address.
 * @param sub the index of the number (in the range [0..3]).
 *
 * @returns the address of the LSS number, or NULL on error.
 */
static inline co_unsigned32_t *co_id_sub(struct co_id *id, co_unsigned8_t sub);

#endif // !LELY_NO_CO_MASTER

void *
__co_lss_alloc(void)
{
	void *ptr = malloc(sizeof(struct __co_lss));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
__co_lss_free(void *ptr)
{
	free(ptr);
}

struct __co_lss *
__co_lss_init(struct __co_lss *lss, co_nmt_t *nmt)
{
	assert(lss);
	assert(nmt);

	int errc = 0;

	lss->nmt = nmt;
	lss->net = co_nmt_get_net(lss->nmt);
	lss->dev = co_nmt_get_dev(lss->nmt);

	lss->state = co_lss_stopped_state;

#if !LELY_NO_CO_MASTER
	lss->master = 0;
	lss->inhibit = LELY_CO_LSS_INHIBIT;
	lss->next = 0;
#endif

	lss->recv = can_recv_create();
	if (!lss->recv) {
		errc = get_errc();
		goto error_create_recv;
	}
	can_recv_set_func(lss->recv, &co_lss_recv, lss);

#if !LELY_NO_CO_MASTER
	lss->timeout = LELY_CO_LSS_TIMEOUT;

	lss->timer = can_timer_create();
	if (!lss->timer) {
		errc = get_errc();
		goto error_create_timer;
	}
	can_timer_set_func(lss->timer, &co_lss_timer, lss);
#endif

	lss->cs = 0;
	lss->lsspos = 0;
#if !LELY_NO_CO_MASTER
	lss->lo = (struct co_id)CO_ID_INIT;
	lss->hi = (struct co_id)CO_ID_INIT;
	lss->mask = (struct co_id)CO_ID_INIT;
	lss->bitchk = 0;
	lss->lsssub = 0;
	lss->err = 0;
	lss->spec = 0;
	lss->lssid = 0;
	lss->nid = 0;
	lss->id = (struct co_id)CO_ID_INIT;
#endif

	lss->rate_ind = NULL;
	lss->rate_data = NULL;
	lss->store_ind = NULL;
	lss->store_data = NULL;
#if !LELY_NO_CO_MASTER
	lss->cs_ind = NULL;
	lss->cs_data = NULL;
	lss->err_ind = NULL;
	lss->err_data = NULL;
	lss->lssid_ind = NULL;
	lss->lssid_data = NULL;
	lss->nid_ind = NULL;
	lss->nid_data = NULL;
	lss->scan_ind = NULL;
	lss->scan_data = NULL;
#endif

	if (co_lss_start(lss) == -1) {
		errc = get_errc();
		goto error_start;
	}

	return lss;

	// co_lss_stop(lss);
error_start:
#if !LELY_NO_CO_MASTER
	can_timer_destroy(lss->timer);
error_create_timer:
#endif
	can_recv_destroy(lss->recv);
error_create_recv:
	set_errc(errc);
	return NULL;
}

void
__co_lss_fini(struct __co_lss *lss)
{
	assert(lss);

	co_lss_stop(lss);

#if !LELY_NO_CO_MASTER
	can_timer_destroy(lss->timer);
#endif
	can_recv_destroy(lss->recv);
}

co_lss_t *
co_lss_create(co_nmt_t *nmt)
{
	trace("creating LSS");

	int errc = 0;

	co_lss_t *lss = __co_lss_alloc();
	if (!lss) {
		errc = get_errc();
		goto error_alloc_lss;
	}

	if (!__co_lss_init(lss, nmt)) {
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

void
co_lss_destroy(co_lss_t *lss)
{
	if (lss) {
		trace("destroying LSS");
		__co_lss_fini(lss);
		__co_lss_free(lss);
	}
}

int
co_lss_start(co_lss_t *lss)
{
	assert(lss);

	if (!co_lss_is_stopped(lss))
		return 0;

	co_lss_enter(lss, co_lss_wait_state);

	return 0;
}

void
co_lss_stop(co_lss_t *lss)
{
	assert(lss);

	if (co_lss_is_stopped(lss))
		return;

#if !LELY_NO_CO_MASTER
	can_timer_stop(lss->timer);
#endif
	can_recv_stop(lss->recv);

	co_lss_enter(lss, co_lss_stopped_state);
}

int
co_lss_is_stopped(const co_lss_t *lss)
{
	assert(lss);

	return lss->state == co_lss_stopped_state;
}

co_nmt_t *
co_lss_get_nmt(const co_lss_t *lss)
{
	assert(lss);

	return lss->nmt;
}

void
co_lss_get_rate_ind(const co_lss_t *lss, co_lss_rate_ind_t **pind, void **pdata)
{
	assert(lss);

	if (pind)
		*pind = lss->rate_ind;
	if (pdata)
		*pdata = lss->rate_data;
}

void
co_lss_set_rate_ind(co_lss_t *lss, co_lss_rate_ind_t *ind, void *data)
{
	assert(lss);

	lss->rate_ind = ind;
	lss->rate_data = data;
}

void
co_lss_get_store_ind(
		const co_lss_t *lss, co_lss_store_ind_t **pind, void **pdata)
{
	assert(lss);

	if (pind)
		*pind = lss->store_ind;
	if (pdata)
		*pdata = lss->store_data;
}

void
co_lss_set_store_ind(co_lss_t *lss, co_lss_store_ind_t *ind, void *data)
{
	assert(lss);

	lss->store_ind = ind;
	lss->store_data = data;
}

#if !LELY_NO_CO_MASTER

co_unsigned16_t
co_lss_get_inhibit(const co_lss_t *lss)
{
	assert(lss);

	return lss->inhibit;
}

void
co_lss_set_inhibit(co_lss_t *lss, co_unsigned16_t inhibit)
{
	assert(lss);

	lss->inhibit = inhibit;
}

int
co_lss_get_timeout(const co_lss_t *lss)
{
	assert(lss);

	return lss->timeout;
}

void
co_lss_set_timeout(co_lss_t *lss, int timeout)
{
	assert(lss);

	if (lss->timeout && timeout <= 0)
		can_timer_stop(lss->timer);

	lss->timeout = MAX(0, timeout);
}

#endif // !LELY_NO_CO_MASTER

int
co_lss_is_master(const co_lss_t *lss)
{
#if LELY_NO_CO_MASTER
	(void)lss;

	return 0;
#else
	assert(lss);

	return lss->master;
#endif
}

#if !LELY_NO_CO_MASTER

int
co_lss_is_idle(const co_lss_t *lss)
{
	assert(lss);

	return lss->state == co_lss_wait_state;
}

void
co_lss_abort_req(co_lss_t *lss)
{
	assert(lss);

	co_lss_enter(lss, co_lss_wait_state);
}

int
co_lss_switch_req(co_lss_t *lss, co_unsigned8_t mode)
{
	if (!co_lss_is_master(lss) || !co_lss_is_idle(lss)) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	if (mode > 0x01) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	trace("LSS: switch state global");

	// Switch state global (see Fig. 31 in CiA 305 version 3.0.0).
	struct can_msg req;
	co_lss_init_req(lss, &req, 0x04);
	req.data[1] = mode;
	return can_net_send(lss->net, &req);
}

int
co_lss_switch_sel_req(co_lss_t *lss, const struct co_id *id,
		co_lss_cs_ind_t *ind, void *data)
{
	assert(id);

	if (!co_lss_is_master(lss) || !co_lss_is_idle(lss)) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	trace("LSS: switch state selective");

	lss->id = *id;

	lss->cs_ind = ind;
	lss->cs_data = data;
	co_lss_enter(lss, co_lss_switch_sel_state);

	return 0;
}

int
co_lss_set_id_req(co_lss_t *lss, co_unsigned8_t id, co_lss_err_ind_t *ind,
		void *data)
{
	if (!co_lss_is_master(lss) || !co_lss_is_idle(lss)) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	if (!id || (id > CO_NUM_NODES && id != 0xff)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	trace("LSS: configure node-ID");

	// Configure node-ID (see Fig. 33 in CiA 305 version 3.0.0).
	struct can_msg req;
	co_lss_init_req(lss, &req, 0x11);
	req.data[1] = id;
	if (can_net_send(lss->net, &req) == -1)
		return -1;

	// Wait for response.
	co_lss_init_ind(lss, req.data[0]);
	lss->err_ind = ind;
	lss->err_data = data;
	co_lss_enter(lss, co_lss_err_state);

	return 0;
}

int
co_lss_set_rate_req(co_lss_t *lss, co_unsigned16_t rate, co_lss_err_ind_t *ind,
		void *data)
{
	if (!co_lss_is_master(lss) || !co_lss_is_idle(lss)) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	// Configure bit timing parameters (see Fig. 34 in CiA 305 version
	// 3.0.0).
	struct can_msg req;
	co_lss_init_req(lss, &req, 0x13);
	req.data[1] = 0;
	switch (rate) {
	case 1000: req.data[2] = 0; break;
	case 800: req.data[2] = 1; break;
	case 500: req.data[2] = 2; break;
	case 250: req.data[2] = 3; break;
	case 125: req.data[2] = 4; break;
	case 50: req.data[2] = 6; break;
	case 20: req.data[2] = 7; break;
	case 10: req.data[2] = 8; break;
	case 0: req.data[2] = 9; break;
	default: set_errnum(ERRNUM_INVAL); return 0;
	}

	trace("LSS: configure bit timing parameters");

	if (can_net_send(lss->net, &req) == -1)
		return -1;

	// Wait for response.
	co_lss_init_ind(lss, req.data[0]);
	lss->err_ind = ind;
	lss->err_data = data;
	co_lss_enter(lss, co_lss_err_state);

	return 0;
}

int
co_lss_switch_rate_req(co_lss_t *lss, int delay)
{
	if (!co_lss_is_master(lss) || !co_lss_is_idle(lss)) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	if (delay < CO_UNSIGNED16_MIN || delay > CO_UNSIGNED16_MAX) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	trace("LSS: activate bit timing parameters");

	// Activate bit timing parameters (see Fig. 35 in CiA 305 version
	// 3.0.0).
	struct can_msg req;
	co_lss_init_req(lss, &req, 0x15);
	stle_u16(req.data + 1, (co_unsigned16_t)delay);
	return can_net_send(lss->net, &req);
}

int
co_lss_store_req(co_lss_t *lss, co_lss_err_ind_t *ind, void *data)
{
	if (!co_lss_is_master(lss) || !co_lss_is_idle(lss)) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	trace("LSS: store configuration");

	// Store configuration (see Fig. 36 in CiA 305 version 3.0.0).
	struct can_msg req;
	co_lss_init_req(lss, &req, 0x17);
	if (can_net_send(lss->net, &req) == -1)
		return -1;

	// Wait for response.
	co_lss_init_ind(lss, req.data[0]);
	lss->err_ind = ind;
	lss->err_data = data;
	co_lss_enter(lss, co_lss_err_state);

	return 0;
}

int
co_lss_get_vendor_id_req(co_lss_t *lss, co_lss_lssid_ind_t *ind, void *data)
{
	if (!co_lss_is_master(lss) || !co_lss_is_idle(lss)) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	trace("LSS: inquire identity vendor-ID");

	// Inquire identity vendor-ID (see Fig. 37 in CiA 305 version 3.0.0).
	struct can_msg req;
	co_lss_init_req(lss, &req, 0x5a);
	if (can_net_send(lss->net, &req) == -1)
		return -1;

	// Wait for response.
	co_lss_init_ind(lss, req.data[0]);
	lss->lssid_ind = ind;
	lss->lssid_data = data;
	co_lss_enter(lss, co_lss_lssid_state);

	return 0;
}

int
co_lss_get_product_code_req(co_lss_t *lss, co_lss_lssid_ind_t *ind, void *data)
{
	if (!co_lss_is_master(lss) || !co_lss_is_idle(lss)) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	trace("LSS: inquire identity product-code");

	// Inquire identity product-code (see Fig. 38 in CiA 305 version 3.0.0).
	struct can_msg req;
	co_lss_init_req(lss, &req, 0x5b);
	if (can_net_send(lss->net, &req) == -1)
		return -1;

	// Wait for response.
	co_lss_init_ind(lss, req.data[0]);
	lss->lssid_ind = ind;
	lss->lssid_data = data;
	co_lss_enter(lss, co_lss_lssid_state);

	return 0;
}

int
co_lss_get_revision_req(co_lss_t *lss, co_lss_lssid_ind_t *ind, void *data)
{
	if (!co_lss_is_master(lss) || !co_lss_is_idle(lss)) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	trace("LSS: inquire identity revision-number");

	// Inquire identity revision-number (see Fig. 39 in CiA 305 version
	// 3.0.0).
	struct can_msg req;
	co_lss_init_req(lss, &req, 0x5c);
	if (can_net_send(lss->net, &req) == -1)
		return -1;

	// Wait for response.
	co_lss_init_ind(lss, req.data[0]);
	lss->lssid_ind = ind;
	lss->lssid_data = data;
	co_lss_enter(lss, co_lss_lssid_state);

	return 0;
}

int
co_lss_get_serial_nr_req(co_lss_t *lss, co_lss_lssid_ind_t *ind, void *data)
{
	if (!co_lss_is_master(lss) || !co_lss_is_idle(lss)) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	trace("LSS: inquire identity serial number");

	// Inquire identity serial-number (see Fig. 40 in CiA 305 version
	// 3.0.0).
	struct can_msg req;
	co_lss_init_req(lss, &req, 0x5d);
	if (can_net_send(lss->net, &req) == -1)
		return -1;

	// Wait for response.
	co_lss_init_ind(lss, req.data[0]);
	lss->lssid_ind = ind;
	lss->lssid_data = data;
	co_lss_enter(lss, co_lss_lssid_state);

	return 0;
}

int
co_lss_get_id_req(co_lss_t *lss, co_lss_nid_ind_t *ind, void *data)
{
	if (!co_lss_is_master(lss) || !co_lss_is_idle(lss)) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	trace("LSS: inquire node-ID");

	// Inquire node-ID (see Fig. 41 in CiA 305 version 3.0.0).
	struct can_msg req;
	co_lss_init_req(lss, &req, 0x5e);
	if (can_net_send(lss->net, &req) == -1)
		return -1;

	// Wait for response.
	co_lss_init_ind(lss, req.data[0]);
	lss->nid_ind = ind;
	lss->nid_data = data;
	co_lss_enter(lss, co_lss_nid_state);

	return 0;
}

int
co_lss_id_slave_req(co_lss_t *lss, const struct co_id *lo,
		const struct co_id *hi, co_lss_cs_ind_t *ind, void *data)
{
	assert(lo);
	assert(hi);

	if (!co_lss_is_master(lss) || !co_lss_is_idle(lss)) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	if (lo->vendor_id != hi->vendor_id
			|| lo->product_code != hi->product_code
			|| lo->revision > hi->revision
			|| lo->serial_nr > hi->serial_nr) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	trace("LSS: identify remote slave");

	lss->lo = *lo;
	lss->lo.n = 4;
	lss->hi = *hi;
	lss->hi.n = 4;

	lss->cs_ind = ind;
	lss->cs_data = data;
	co_lss_enter(lss, co_lss_id_slave_state);

	return 0;
}

int
co_lss_id_non_cfg_slave_req(co_lss_t *lss, co_lss_cs_ind_t *ind, void *data)
{
	if (!co_lss_is_master(lss) || !co_lss_is_idle(lss)) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	trace("LSS: identify non-configured remote slave");

	// LSS identify non-configured remote slave (see Fig. 44 in CiA 305
	// version 3.0.0).
	struct can_msg req;
	co_lss_init_req(lss, &req, 0x4c);
	if (can_net_send(lss->net, &req) == -1)
		return -1;

	// Wait for response (see Fig. 45 in CiA 305 version 3.0.0).
	co_lss_init_ind(lss, 0x50);
	lss->cs_ind = ind;
	lss->cs_data = data;
	co_lss_enter(lss, co_lss_cs_state);

	return 0;
}

int
co_lss_slowscan_req(co_lss_t *lss, const struct co_id *lo,
		const struct co_id *hi, co_lss_scan_ind_t *ind, void *data)
{
	assert(lo);
	assert(hi);

	if (!co_lss_is_master(lss) || !co_lss_is_idle(lss)) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	if (lo->vendor_id != hi->vendor_id
			|| lo->product_code != hi->product_code
			|| lo->revision > hi->revision
			|| lo->serial_nr > hi->serial_nr) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	trace("LSS: Slowscan");

	lss->lo = *lo;
	lss->lo.n = 4;
	lss->hi = *hi;
	lss->hi.n = 4;

	lss->id = (struct co_id)CO_ID_INIT;

	lss->scan_ind = ind;
	lss->scan_data = data;
	co_lss_enter(lss, co_lss_slowscan_init_state);

	return 0;
}

int
co_lss_fastscan_req(co_lss_t *lss, const struct co_id *id,
		const struct co_id *mask, co_lss_scan_ind_t *ind, void *data)
{
	if (!co_lss_is_master(lss) || !co_lss_is_idle(lss)) {
		set_errnum(ERRNUM_PERM);
		return -1;
	}

	trace("LSS: Fastscan");

	lss->id = (struct co_id)CO_ID_INIT;
	lss->mask = (struct co_id)CO_ID_INIT;
	if (mask) {
		lss->mask = *mask;
		lss->mask.n = 4;
		if (id) {
			lss->id = *id;
			lss->id.n = 4;
			// Clear all unmasked bits in the LSS address.
			lss->id.vendor_id &= lss->mask.vendor_id;
			lss->id.product_code &= lss->mask.product_code;
			lss->id.revision &= lss->mask.revision;
			lss->id.serial_nr &= lss->mask.serial_nr;
		}
	}
	lss->bitchk = 0x80;
	lss->lsssub = 0;

	// LSS Fastscan (see Fig. 46 in CiA 305 version 3.0.0).
	struct can_msg req;
	co_lss_init_req(lss, &req, 0x51);
	req.data[5] = lss->bitchk;
	if (can_net_send(lss->net, &req) == -1)
		return -1;

	// Wait for response (see Fig. 43 in CiA 305 version 3.0.0).
	co_lss_init_ind(lss, 0x4f);
	lss->scan_ind = ind;
	lss->scan_data = data;
	co_lss_enter(lss, co_lss_fastscan_init_state);

	return 0;
}

#endif // !LELY_NO_CO_MASTER

static int
co_lss_recv(const struct can_msg *msg, void *data)
{
	assert(msg);
	co_lss_t *lss = data;
	assert(lss);

	co_lss_emit_recv(lss, msg);

	return 0;
}

#if !LELY_NO_CO_MASTER
static int
co_lss_timer(const struct timespec *tp, void *data)
{
	assert(tp);
	co_lss_t *lss = data;
	assert(lss);

	co_lss_emit_time(lss, tp);

	return 0;
}
#endif

static void
co_lss_enter(co_lss_t *lss, co_lss_state_t *next)
{
	assert(lss);

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

#if !LELY_NO_CO_MASTER
static inline void
co_lss_emit_time(co_lss_t *lss, const struct timespec *tp)
{
	assert(lss);
	assert(lss->state);
	assert(lss->state->on_time);

	co_lss_enter(lss, lss->state->on_time(lss, tp));
}
#endif

static co_lss_state_t *
co_lss_wait_on_enter(co_lss_t *lss)
{
#if LELY_NO_CO_MASTER
	(void)lss;
#else
	assert(lss);

	// Only an NMT master can be an LSS master.
	lss->master = co_nmt_is_master(lss->nmt);
	if (lss->master)
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
	can_recv_start(lss->recv, lss->net, CO_LSS_CANID(1), 0);

	return NULL;
}

static co_lss_state_t *
co_lss_wait_slave_on_recv(co_lss_t *lss, const struct can_msg *msg)
{
	assert(lss);
	assert(msg);

	if (!msg->len)
		return NULL;

	co_unsigned8_t cs = msg->data[0];
	switch (cs) {
	// Switch state global (see Fig. 31 in CiA 305 version 3.0.0).
	case 0x04:
		if (msg->len < 2)
			return NULL;
		switch (msg->data[1]) {
		case 0x00:
			// Re-enter the waiting state.
			trace("LSS: switching to waiting state");
			return co_lss_wait_state;
		case 0x01:
			// Switch to the configuration state.
			trace("LSS: switching to configuration state");
			return co_lss_cfg_state;
		}
		break;
	// Switch state selective.
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
		if (msg->len < 5)
			return NULL;
		return co_lss_switch_sel(lss, cs, ldle_u32(msg->data + 1));
	// LSS identify remote slave.
	case 0x46:
	case 0x47:
	case 0x48:
	case 0x49:
	case 0x4a:
	case 0x4b:
		if (msg->len < 5)
			return NULL;
		co_lss_id_slave(lss, msg->data[0], ldle_u32(msg->data + 1));
		break;
	// LSS identify non-configured remote slave.
	case 0x4c: co_lss_id_non_cfg_slave(lss); break;
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

	int errc = get_errc();
	co_obj_t *obj_1018 = co_dev_find_obj(lss->dev, 0x1018);
	struct can_msg req;

	if (!msg->len)
		return NULL;

	co_unsigned8_t cs = msg->data[0];
	switch (cs) {
	// Switch state global (see Fig. 31 in CiA 305 version 3.0.0).
	case 0x04:
		if (msg->len < 2)
			return NULL;
		switch (msg->data[1]) {
		case 0x00:
			// Switch to the waiting state.
			trace("LSS: switching to waiting state");
			return co_lss_wait_state;
		case 0x01:
			// Re-enter the configuration state.
			trace("LSS: switching to configuration state");
			return co_lss_cfg_state;
		}
		break;
	// Configure node-ID (see Fig. 33 in CiA 305 version 3.0.0).
	case 0x11:
		if (msg->len < 2)
			return NULL;
		// Configure the pending node-ID.
		trace("LSS: configuring node-ID");
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
		trace("LSS: configuring bit timing parameters");
		co_lss_init_req(lss, &req, cs);
		if (!lss->rate_ind || msg->data[1]) {
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
					co_dev_set_rate(lss->dev, 125);
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
			default: req.data[1] = 1; break;
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
		trace("LSS: activating bit timing parameters");
		lss->rate_ind(lss, co_dev_get_rate(lss->dev),
				ldle_u16(msg->data + 1), lss->rate_data);
		break;
	// Store configuration (see Fig. 36 in CiA 305 version 3.0.0).
	case 0x17:
		trace("LSS: storing configuration");
		co_lss_init_req(lss, &req, cs);
		if (lss->store_ind) {
			// Store the pending node-ID and baudrate.
			// clang-format off
			if (lss->store_ind(lss, co_nmt_get_id(lss->nmt),
					co_dev_get_rate(lss->dev),
					lss->store_data) == -1) {
				// clang-format on
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
	case 0x46:
	case 0x47:
	case 0x48:
	case 0x49:
	case 0x4a:
	case 0x4b:
		if (msg->len < 5)
			return NULL;
		co_lss_id_slave(lss, cs, ldle_u32(msg->data + 1));
		break;
	// LSS identify non-configured remote slave.
	case 0x4c: co_lss_id_non_cfg_slave(lss); break;
	// Inquire identity vendor-ID (Fig. 37 in CiA 305 version 3.0.0).
	case 0x5a:
		trace("LSS: sending vendor-ID");
		co_lss_init_req(lss, &req, cs);
		stle_u32(req.data + 1, co_obj_get_val_u32(obj_1018, 0x01));
		can_net_send(lss->net, &req);
		break;
	// Inquire identity product-code (Fig. 38 in CiA 305 version 3.0.0).
	case 0x5b:
		trace("LSS: sending product-code");
		co_lss_init_req(lss, &req, cs);
		stle_u32(req.data + 1, co_obj_get_val_u32(obj_1018, 0x02));
		can_net_send(lss->net, &req);
		break;
	// Inquire identity revision-number (Fig. 39 in CiA 305 version 3.0.0).
	case 0x5c:
		trace("LSS: sending revision-number");
		co_lss_init_req(lss, &req, cs);
		stle_u32(req.data + 1, co_obj_get_val_u32(obj_1018, 0x03));
		can_net_send(lss->net, &req);
		break;
	// Inquire identity serial-number (Fig. 40 in CiA 305 version 3.0.0).
	case 0x5d:
		trace("LSS: sending serial-number");
		co_lss_init_req(lss, &req, cs);
		stle_u32(req.data + 1, co_obj_get_val_u32(obj_1018, 0x04));
		can_net_send(lss->net, &req);
		break;
	// Inquire node-ID (Fig. 41 in CiA 305 version 3.0.0).
	case 0x5e:
		trace("LSS: sending node-ID");
		co_lss_init_req(lss, &req, cs);
		// Respond with the active or pending node-ID, depending on
		// whether the device is in the NMT state Initializing.
		switch (co_nmt_get_st(lss->nmt)) {
		case CO_NMT_ST_BOOTUP:
		case CO_NMT_ST_RESET_NODE:
		case CO_NMT_ST_RESET_COMM:
			req.data[1] = co_nmt_get_id(lss->nmt);
			break;
		default: req.data[1] = co_dev_get_id(lss->dev); break;
		}
		can_net_send(lss->net, &req);
		break;
	}

	return NULL;
}

#if !LELY_NO_CO_MASTER

static co_lss_state_t *
co_lss_cs_on_recv(co_lss_t *lss, const struct can_msg *msg)
{
	assert(lss);
	assert(msg);

	if (msg->len < 1 || msg->data[0] != lss->cs)
		return NULL;

	return co_lss_cs_fini_state;
}

static co_lss_state_t *
co_lss_cs_on_time(co_lss_t *lss, const struct timespec *tp)
{
	assert(lss);
	(void)tp;

	lss->cs = 0;
	return co_lss_cs_fini_state;
}

static co_lss_state_t *
co_lss_cs_fini_on_enter(co_lss_t *lss)
{
	(void)lss;

	return co_lss_wait_state;
}

static void
co_lss_cs_fini_on_leave(co_lss_t *lss)
{
	assert(lss);

	can_timer_stop(lss->timer);
	can_recv_stop(lss->recv);

	if (lss->cs_ind)
		lss->cs_ind(lss, lss->cs, lss->cs_data);
}

static co_lss_state_t *
co_lss_switch_sel_on_enter(co_lss_t *lss)
{
	assert(lss);

	lss->next = 0;
	lss->cs = 0;
	return co_lss_switch_sel_on_time(lss, NULL);
}

static co_lss_state_t *
co_lss_switch_sel_on_time(co_lss_t *lss, const struct timespec *tp)
{
	assert(lss);
	(void)tp;

	// Switch state selective (see Fig. 32 in CiA 305 version 3.0.0).
	if (co_lss_send_switch_sel_req(lss, &lss->id) == -1) {
		// Abort if sending the CAN frame failed.
		lss->cs = 0;
		return co_lss_cs_fini_state;
	}

	// If the last frame was sent, wait for the response.
	if (lss->cs == 0x44)
		return co_lss_cs_state;

	// Wait for the inhibit time to pass.
	return NULL;
}

static co_lss_state_t *
co_lss_err_on_recv(co_lss_t *lss, const struct can_msg *msg)
{
	assert(lss);
	assert(msg);

	if (msg->len < 3 || msg->data[0] != lss->cs)
		return NULL;

	lss->err = msg->data[1];
	lss->spec = lss->err == 0xff ? msg->data[2] : 0;
	return co_lss_wait_state;
}

static co_lss_state_t *
co_lss_err_on_time(co_lss_t *lss, const struct timespec *tp)
{
	assert(lss);
	(void)tp;

	lss->cs = 0;
	return co_lss_wait_state;
}

static void
co_lss_err_on_leave(co_lss_t *lss)
{
	assert(lss);

	can_timer_stop(lss->timer);
	can_recv_stop(lss->recv);

	if (lss->err_ind)
		lss->err_ind(lss, lss->cs, lss->err, lss->spec, lss->err_data);
}

static co_lss_state_t *
co_lss_lssid_on_recv(co_lss_t *lss, const struct can_msg *msg)
{
	assert(lss);
	assert(msg);

	if (msg->len < 5 || msg->data[0] != lss->cs)
		return NULL;

	lss->lssid = ldle_u32(msg->data + 1);
	return co_lss_wait_state;
}

static co_lss_state_t *
co_lss_lssid_on_time(co_lss_t *lss, const struct timespec *tp)
{
	assert(lss);
	(void)tp;

	lss->cs = 0;
	return co_lss_wait_state;
}

static void
co_lss_lssid_on_leave(co_lss_t *lss)
{
	assert(lss);

	can_timer_stop(lss->timer);
	can_recv_stop(lss->recv);

	if (lss->lssid_ind)
		lss->lssid_ind(lss, lss->cs, lss->lssid, lss->lssid_data);
}

static co_lss_state_t *
co_lss_nid_on_recv(co_lss_t *lss, const struct can_msg *msg)
{
	assert(lss);
	assert(msg);

	if (msg->len < 2 || msg->data[0] != lss->cs)
		return NULL;

	lss->nid = msg->data[1];
	return co_lss_wait_state;
}

static co_lss_state_t *
co_lss_nid_on_time(co_lss_t *lss, const struct timespec *tp)
{
	assert(lss);
	(void)tp;

	lss->cs = 0;
	return co_lss_wait_state;
}

static void
co_lss_nid_on_leave(co_lss_t *lss)
{
	assert(lss);

	can_timer_stop(lss->timer);
	can_recv_stop(lss->recv);

	if (lss->nid_ind)
		lss->nid_ind(lss, lss->cs, lss->nid, lss->nid_data);
}

static co_lss_state_t *
co_lss_id_slave_on_enter(co_lss_t *lss)
{
	assert(lss);

	lss->next = 0;
	lss->cs = 0;
	return co_lss_id_slave_on_time(lss, NULL);
}

static co_lss_state_t *
co_lss_id_slave_on_time(co_lss_t *lss, const struct timespec *tp)
{
	assert(lss);
	(void)tp;

	// LSS identify remote slave (see Fig. 42 in CiA 305 version 3.0.0).
	if (co_lss_send_id_slave_req(lss, &lss->lo, &lss->hi) == -1) {
		// Abort if sending the CAN frame failed.
		lss->cs = 0;
		return co_lss_cs_fini_state;
	}

	// If the last frame was sent, wait for the response.
	if (lss->cs == 0x4f)
		return co_lss_cs_state;

	// Wait for the inhibit time to pass.
	return NULL;
}

static co_lss_state_t *
co_lss_slowscan_init_on_enter(co_lss_t *lss)
{
	assert(lss);

	lss->next = 0;
	lss->cs = 0;
	return co_lss_slowscan_init_on_time(lss, NULL);
}

static co_lss_state_t *
co_lss_slowscan_init_on_recv(co_lss_t *lss, const struct can_msg *msg)
{
	assert(lss);
	assert(msg);

	if (msg->len < 1 || msg->data[0] != lss->cs)
		return NULL;

	return co_lss_slowscan_scan_state;
}

static co_lss_state_t *
co_lss_slowscan_init_on_time(co_lss_t *lss, const struct timespec *tp)
{
	assert(lss);
	(void)tp;

	// Abort if we did not receive a response on the first request.
	if (lss->cs == 0x4f) {
		lss->cs = 0;
		return co_lss_slowscan_fini_state;
	}

	// LSS identify remote slave (see Fig. 42 in CiA 305 version 3.0.0).
	if (co_lss_send_id_slave_req(lss, &lss->lo, &lss->hi) == -1) {
		// Abort if sending the CAN frame failed.
		lss->cs = 0;
		return co_lss_slowscan_fini_state;
	}

	// Wait for the inhibit time to pass.
	return NULL;
}

static co_lss_state_t *
co_lss_slowscan_scan_on_enter(co_lss_t *lss)
{
	assert(lss);

	// Calculate the midpoint while avoiding integer overflow.
	struct co_id *id = &lss->id;
	*id = lss->lo;
	if (id->revision < lss->hi.revision) {
		id->revision += (lss->hi.revision - id->revision) / 2;
		id->serial_nr = lss->hi.serial_nr;
	} else {
		id->serial_nr += (lss->hi.serial_nr - id->serial_nr) / 2;
	}

	lss->next = 0;
	lss->cs = 0;
	return co_lss_slowscan_scan_on_time(lss, NULL);
}

static co_lss_state_t *
co_lss_slowscan_scan_on_recv(co_lss_t *lss, const struct can_msg *msg)
{
	assert(lss);
	assert(msg);

	if (msg->len < 1 || msg->data[0] != lss->cs)
		return NULL;

	// Wait until the timeout expires before handling the response.
	return co_lss_slowscan_wait_state;
}

static co_lss_state_t *
co_lss_slowscan_scan_on_time(co_lss_t *lss, const struct timespec *tp)
{
	assert(lss);
	(void)tp;

	if (lss->cs == 0x4f)
		return co_lss_slowscan_scan_on_res(lss, 1);

	// LSS identify remote slave (see Fig. 42 in CiA 305 version 3.0.0).
	if (co_lss_send_id_slave_req(lss, &lss->lo, &lss->id) == -1) {
		// Abort if sending the CAN frame failed.
		lss->cs = 0;
		return co_lss_slowscan_fini_state;
	}

	// Wait for the inhibit time to pass.
	return NULL;
}

static co_lss_state_t *
co_lss_slowscan_scan_on_res(co_lss_t *lss, int timeout)
{
	assert(lss);

	if (lss->lo.revision == lss->hi.revision
			&& lss->lo.serial_nr == lss->hi.serial_nr) {
		// Abort if we timeout after sending the final LSS address.
		if (timeout) {
			lss->cs = 0;
			return co_lss_slowscan_fini_state;
		}
		// Switch the slave to the LSS configuration state.
		co_lss_init_ind(lss, 0x44);
		return co_lss_slowscan_switch_state;
	}

	// Update the bounds on the LSS address.
	if (timeout) {
		if (lss->id.revision < lss->hi.revision)
			lss->lo.revision = lss->id.revision + 1;
		else
			lss->lo.serial_nr = lss->id.serial_nr + 1;
	} else {
		lss->hi = lss->id;
	}

	// Start the next cycle.
	return co_lss_slowscan_scan_state;
}

static co_lss_state_t *
co_lss_slowscan_wait_on_recv(co_lss_t *lss, const struct can_msg *msg)
{
	(void)lss;
	(void)msg;

	// Ignore further responses from slaves.
	return NULL;
}

static co_lss_state_t *
co_lss_slowscan_wait_on_time(co_lss_t *lss, const struct timespec *tp)
{
	(void)lss;
	(void)tp;

	// All slaves should have responded by now.
	return co_lss_slowscan_scan_on_res(lss, 0);
}

static co_lss_state_t *
co_lss_slowscan_switch_on_enter(co_lss_t *lss)
{
	assert(lss);

	lss->next = 0;
	lss->cs = 0;
	return co_lss_slowscan_switch_on_time(lss, NULL);
}

static co_lss_state_t *
co_lss_slowscan_switch_on_recv(co_lss_t *lss, const struct can_msg *msg)
{
	assert(lss);
	assert(msg);

	if (msg->len < 1 || msg->data[0] != lss->cs)
		return NULL;

	return co_lss_slowscan_fini_state;
}

static co_lss_state_t *
co_lss_slowscan_switch_on_time(co_lss_t *lss, const struct timespec *tp)
{
	assert(lss);
	(void)tp;

	// Abort if we did not receive a response.
	if (lss->cs == 0x44) {
		lss->cs = 0;
		return co_lss_slowscan_fini_state;
	}

	// Switch state selective (see Fig. 32 in CiA 305 version 3.0.0).
	if (co_lss_send_switch_sel_req(lss, &lss->id) == -1) {
		// Abort if sending the CAN frame failed.
		lss->cs = 0;
		return co_lss_slowscan_fini_state;
	}

	// Wait for the inhibit time to pass.
	return NULL;
}

static co_lss_state_t *
co_lss_slowscan_fini_on_enter(co_lss_t *lss)
{
	(void)lss;

	return co_lss_wait_state;
}

static void
co_lss_slowscan_fini_on_leave(co_lss_t *lss)
{
	assert(lss);

	can_timer_stop(lss->timer);
	can_recv_stop(lss->recv);

	if (lss->scan_ind)
		lss->scan_ind(lss, lss->cs, lss->cs ? &lss->id : NULL,
				lss->scan_data);
}

static co_lss_state_t *
co_lss_fastscan_init_on_recv(co_lss_t *lss, const struct can_msg *msg)
{
	assert(lss);
	assert(msg);

	if (msg->len < 1 || msg->data[0] != lss->cs)
		return NULL;

	lss->bitchk = 31;
	return co_lss_fastscan_scan_state;
}

static co_lss_state_t *
co_lss_fastscan_init_on_time(co_lss_t *lss, const struct timespec *tp)
{
	assert(lss);
	(void)tp;

	// Abort if we did not receive a response on the reset request.
	lss->cs = 0;
	return co_lss_fastscan_fini_state;
}

static co_lss_state_t *
co_lss_fastscan_scan_on_enter(co_lss_t *lss)
{
	assert(lss);

	const co_unsigned32_t *pid = co_id_sub(&lss->id, lss->lsssub);
	assert(pid);
	const co_unsigned32_t *pmask = co_id_sub(&lss->mask, lss->lsssub);
	assert(pmask);

	// Find the next unknown bit.
	for (; lss->bitchk && (*pmask & (UINT32_C(1) << lss->bitchk));
			lss->bitchk--)
		;

	co_unsigned8_t lssnext = lss->lsssub;
	// If we obtained the complete LSS number, send it again and prepare for
	// the next number.
	if (!lss->bitchk && (*pmask & 1)) {
		if (lssnext < 3) {
			lssnext++;
		} else {
			lssnext = 0;
		}
	}

	// LSS Fastscan (see Fig. 46 in CiA 305 version 3.0.0).
	// clang-format off
	if (co_lss_send_fastscan_req(lss, *pid, lss->bitchk, lss->lsssub,
			lssnext) == -1) {
		// Abort if sending the CAN frame failed.
		// clang-format on
		lss->cs = 0;
		return co_lss_fastscan_fini_state;
	}

	// Restart the timeout for the next response.
	can_timer_timeout(lss->timer, lss->net, lss->timeout);
	return NULL;
}

static co_lss_state_t *
co_lss_fastscan_scan_on_recv(co_lss_t *lss, const struct can_msg *msg)
{
	assert(lss);
	assert(msg);

	if (msg->len < 1 || msg->data[0] != lss->cs)
		return NULL;

	// Wait until the timeout expires before handling the response.
	return co_lss_fastscan_wait_state;
}

static co_lss_state_t *
co_lss_fastscan_scan_on_time(co_lss_t *lss, const struct timespec *tp)
{
	assert(lss);
	(void)tp;

	return co_lss_fastscan_scan_on_res(lss, 1);
}

static co_lss_state_t *
co_lss_fastscan_scan_on_res(co_lss_t *lss, int timeout)
{
	assert(lss);
	assert(lss->bitchk <= 31);
	assert(lss->lsssub < 4);

	co_unsigned32_t *pid = co_id_sub(&lss->id, lss->lsssub);
	assert(pid);
	co_unsigned32_t *pmask = co_id_sub(&lss->mask, lss->lsssub);
	assert(pmask);

	if (!lss->bitchk && (*pmask & 1)) {
		// Abort if we timeout after sending the complete LSS number.
		if (timeout) {
			lss->cs = 0;
			return co_lss_fastscan_fini_state;
		}
		// We're done if this was the last LSS number.
		if (++lss->lsssub == 4)
			return co_lss_fastscan_fini_state;
		lss->bitchk = 31;
	} else {
		// Update the LSS address. A timeout indicates the bit is 1.
		if (timeout)
			*pid |= UINT32_C(1) << lss->bitchk;
		*pmask |= UINT32_C(1) << lss->bitchk;
	}

	// Start the next cycle.
	return co_lss_fastscan_scan_state;
}

static co_lss_state_t *
co_lss_fastscan_wait_on_recv(co_lss_t *lss, const struct can_msg *msg)
{
	(void)lss;
	(void)msg;

	// Ignore further responses from slaves.
	return NULL;
}

static co_lss_state_t *
co_lss_fastscan_wait_on_time(co_lss_t *lss, const struct timespec *tp)
{
	(void)lss;
	(void)tp;

	// All slaves should have responded by now.
	return co_lss_fastscan_scan_on_res(lss, 0);
}

static co_lss_state_t *
co_lss_fastscan_fini_on_enter(co_lss_t *lss)
{
	(void)lss;

	return co_lss_wait_state;
}

static void
co_lss_fastscan_fini_on_leave(co_lss_t *lss)
{
	assert(lss);

	can_timer_stop(lss->timer);
	can_recv_stop(lss->recv);

	if (lss->scan_ind)
		lss->scan_ind(lss, lss->cs, lss->cs ? &lss->id : NULL,
				lss->scan_data);
}

#endif // !LELY_NO_CO_MASTER

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
		trace("LSS: switching to configuration state");
		return co_lss_cfg_state;
	default: return NULL;
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
		// Check the product-code.
		if (cs != lss->cs || id != co_obj_get_val_u32(obj_1018, 0x02)) {
			lss->cs = 0;
			return;
		}
		lss->cs = 0x48;
		break;
	case 0x48:
		// Check the lower bound of the revision-number.
		if (cs != lss->cs || id > co_obj_get_val_u32(obj_1018, 0x03)) {
			lss->cs = 0;
			return;
		}
		lss->cs = 0x49;
		break;
	case 0x49:
		// Check the upper bound of the revision-number.
		if (cs != lss->cs || id < co_obj_get_val_u32(obj_1018, 0x03)) {
			lss->cs = 0;
			return;
		}
		lss->cs = 0x4a;
		break;
	case 0x4a:
		// Check the lower bound of the serial-number.
		if (cs != lss->cs || id > co_obj_get_val_u32(obj_1018, 0x04)) {
			lss->cs = 0;
			return;
		}
		lss->cs = 0x4b;
		break;
	case 0x4b:
		// Check the upper bound of the serial-number.
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
	case CO_NMT_ST_RESET_COMM: break;
	default: return;
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

	if (bitchk > 31 && bitchk != 0x80)
		return NULL;

	if (bitchk == 0x80) {
		lss->lsspos = 0;
	} else {
		if (lss->lsspos > 3 || lss->lsspos != lsssub)
			return NULL;
		// Check if the unmasked bits of the specified IDNumber match.
		co_unsigned32_t pid[] = { co_obj_get_val_u32(obj_1018, 0x01),
			co_obj_get_val_u32(obj_1018, 0x02),
			co_obj_get_val_u32(obj_1018, 0x03),
			co_obj_get_val_u32(obj_1018, 0x04) };
		if ((id ^ pid[lss->lsspos]) & ~((UINT32_C(1) << bitchk) - 1))
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
	msg->id = CO_LSS_CANID(co_lss_is_master(lss));
	msg->len = CAN_MAX_LEN;
	msg->data[0] = cs;
}

#if !LELY_NO_CO_MASTER

static int
co_lss_send_switch_sel_req(co_lss_t *lss, const struct co_id *id)
{
	assert(id);

	// Switch state selective (see Fig. 32 in CiA 305 version 3.0.0).
	struct can_msg req;
	switch (lss->next) {
	case 0:
		co_lss_init_req(lss, &req, 0x40);
		stle_u32(req.data + 1, id->vendor_id);
		break;
	case 1:
		co_lss_init_req(lss, &req, 0x41);
		stle_u32(req.data + 1, id->product_code);
		break;
	case 2:
		co_lss_init_req(lss, &req, 0x42);
		stle_u32(req.data + 1, id->revision);
		break;
	case 3:
		co_lss_init_req(lss, &req, 0x43);
		stle_u32(req.data + 1, id->serial_nr);
		break;
	default: return -1;
	}
	if (can_net_send(lss->net, &req) == -1)
		return -1;

	if (++lss->next < 4) {
		can_recv_stop(lss->recv);
		// Wait until the inhibit time has elapsed.
		struct timespec start = { 0, 0 };
		can_net_get_time(lss->net, &start);
		timespec_add_usec(&start, 100 * lss->inhibit);
		can_timer_start(lss->timer, lss->net, &start, NULL);
	} else {
		// Wait for response (see Fig. 32 in CiA 305 version 3.0.0).
		co_lss_init_ind(lss, 0x44);
	}

	return 0;
}

static int
co_lss_send_id_slave_req(
		co_lss_t *lss, const struct co_id *lo, const struct co_id *hi)
{
	assert(lo);
	assert(hi);
	assert(lo->vendor_id == hi->vendor_id);
	assert(lo->product_code == hi->product_code);
	assert(lo->revision <= hi->revision);
	assert(lo->serial_nr <= hi->serial_nr);

	// LSS identify remote slave (see Fig. 42 in CiA 305 version 3.0.0).
	struct can_msg req;
	switch (lss->next) {
	case 0:
		co_lss_init_req(lss, &req, 0x46);
		stle_u32(req.data + 1, lo->vendor_id);
		break;
	case 1:
		co_lss_init_req(lss, &req, 0x47);
		stle_u32(req.data + 1, lo->product_code);
		break;
	case 2:
		co_lss_init_req(lss, &req, 0x48);
		stle_u32(req.data + 1, lo->revision);
		break;
	case 3:
		co_lss_init_req(lss, &req, 0x49);
		stle_u32(req.data + 1, hi->revision);
		break;
	case 4:
		co_lss_init_req(lss, &req, 0x4a);
		stle_u32(req.data + 1, lo->serial_nr);
		break;
	case 5:
		co_lss_init_req(lss, &req, 0x4b);
		stle_u32(req.data + 1, hi->serial_nr);
		break;
	default: return -1;
	}
	if (can_net_send(lss->net, &req) == -1)
		return -1;

	if (++lss->next < 6) {
		can_recv_stop(lss->recv);
		// Wait until the inhibit time has elapsed.
		struct timespec start = { 0, 0 };
		can_net_get_time(lss->net, &start);
		timespec_add_usec(&start, 100 * lss->inhibit);
		can_timer_start(lss->timer, lss->net, &start, NULL);
	} else {
		// Wait for response (see Fig. 43 in CiA 305 version 3.0.0).
		co_lss_init_ind(lss, 0x4f);
	}

	return 0;
}

static int
co_lss_send_fastscan_req(const co_lss_t *lss, co_unsigned32_t id,
		co_unsigned8_t bitchk, co_unsigned8_t lsssub,
		co_unsigned8_t lssnext)
{
	// LSS Fastscan (see Fig. 46 in CiA 305 version 3.0.0).
	struct can_msg req;
	co_lss_init_req(lss, &req, 0x51);
	stle_u32(req.data + 1, id);
	req.data[5] = bitchk;
	req.data[6] = lsssub;
	req.data[7] = lssnext;
	return can_net_send(lss->net, &req);
}

static void
co_lss_init_ind(co_lss_t *lss, co_unsigned8_t cs)
{
	assert(lss);

	lss->cs = cs;
	lss->err = 0;
	lss->spec = 0;
	lss->lssid = 0;
	lss->nid = 0;

	can_recv_start(lss->recv, lss->net, CO_LSS_CANID(0), 0);
	can_timer_timeout(lss->timer, lss->net, lss->timeout);
}

static inline co_unsigned32_t *
co_id_sub(struct co_id *id, co_unsigned8_t sub)
{
	assert(id);
	assert(sub < 4);

	switch (sub) {
	case 0: return &id->vendor_id;
	case 1: return &id->product_code;
	case 2: return &id->revision;
	case 3: return &id->serial_nr;
	default: return NULL;
	}
}

#endif // !LELY_NO_CO_MASTER

#endif // !LELY_NO_CO_LSS
