/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the NMT 'boot slave' functions.
 *
 * @see src/nmt_boot.h
 *
 * @copyright 2021 Lely Industries N.V.
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

#if !LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT

#include "nmt_boot.h"
#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/val.h>
#include <lely/util/diag.h>
#include <lely/util/time.h>

#include <assert.h>
#if !LELY_NO_STDIO
#include <inttypes.h>
#endif
#include <stdlib.h>

#ifndef LELY_CO_NMT_BOOT_WAIT_TIMEOUT
/// The timeout (in milliseconds) before trying to boot the slave again.
#define LELY_CO_NMT_BOOT_WAIT_TIMEOUT 1000
#endif

#ifndef LELY_CO_NMT_BOOT_SDO_RETRY
/// The number of times an SDO request is retried after a timeout.
#define LELY_CO_NMT_BOOT_SDO_RETRY 3
#endif

#if !LELY_NO_CO_NG
#ifndef LELY_CO_NMT_BOOT_RTR_TIMEOUT
/// The timeout (in milliseconds) after sending a node guarding RTR.
#define LELY_CO_NMT_BOOT_RTR_TIMEOUT 100
#endif
#endif

#ifndef LELY_CO_NMT_BOOT_RESET_TIMEOUT
/**
 * The timeout (in milliseconds) after sending the NMT 'reset communication'
 * command.
 */
#define LELY_CO_NMT_BOOT_RESET_TIMEOUT 1000
#endif

#ifndef LELY_CO_NMT_BOOT_CHECK_TIMEOUT
/**
 * The timeout (in milliseconds) before checking the flash status indication or
 * the program control of a slave again.
 */
#define LELY_CO_NMT_BOOT_CHECK_TIMEOUT 100
#endif

struct __co_nmt_boot_state;
/// An opaque CANopen NMT 'boot slave' state type.
typedef const struct __co_nmt_boot_state co_nmt_boot_state_t;

/// A CANopen NMT 'boot slave' service.
struct __co_nmt_boot {
	/// A pointer to a CAN network interface.
	can_net_t *net;
	/// A pointer to a CANopen device.
	co_dev_t *dev;
	/// A pointer to an NMT master service.
	co_nmt_t *nmt;
	/// A pointer to the current state.
	co_nmt_boot_state_t *state;
	/// A pointer to the CAN frame receiver.
	can_recv_t *recv;
	/// A pointer to the CAN timer.
	can_timer_t *timer;
	/// The node-ID.
	co_unsigned8_t id;
	/// The SDO timeout (in milliseconds).
	int timeout;
	/// A pointer to the Client-SDO used to access slave objects.
	co_csdo_t *sdo;
	/// The time at which the 'boot slave' request was received.
	struct timespec start;
	/// The NMT slave assignment (object 1F81).
	co_unsigned32_t assignment;
	/// The consumer heartbeat time (in milliseconds).
	co_unsigned16_t ms;
	/// The CANopen SDO upload request used for reading sub-objects.
	struct co_sdo_req req;
	/// The number of SDO retries remaining.
	int retry;
	/// The state of the node (including the toggle bit).
	co_unsigned8_t st;
	/// The error status.
	char es;
};

/**
 * The CAN receive callback function for a 'boot slave' service.
 *
 * @see can_recv_func_t
 */
static int co_nmt_boot_recv(const struct can_msg *msg, void *data);

/**
 * The CAN timer callback function for a 'boot slave' service.
 *
 * @see can_timer_func_t
 */
static int co_nmt_boot_timer(const struct timespec *tp, void *data);

/**
 * The CANopen SDO download confirmation callback function for a 'boot slave'
 * service.
 *
 * @see co_csdo_dn_con_t
 */
static void co_nmt_boot_dn_con(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac, void *data);

/**
 * The CANopen SDO upload confirmation callback function for a 'boot slave'
 * service.
 *
 * @see co_csdo_up_con_t
 */
static void co_nmt_boot_up_con(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac, const void *ptr,
		size_t n, void *data);

/**
 * The CANopen NMT 'configuration request' confirmation callback function for a
 * 'boot slave' service.
 *
 * @see co_nmt_cfg_con_t
 */
static void co_nmt_boot_cfg_con(co_nmt_t *nmt, co_unsigned8_t id,
		co_unsigned32_t ac, void *data);

/**
 * Enters the specified state of a 'boot slave' service and invokes the exit and
 * entry functions.
 */
static void co_nmt_boot_enter(co_nmt_boot_t *boot, co_nmt_boot_state_t *next);

/**
 * Invokes the 'CAN frame received' transition function of the current state of
 * a 'boot slave' service.
 *
 * @param boot a pointer to a 'boot slave' service.
 * @param msg  a pointer to the received CAN frame.
 */
static inline void co_nmt_boot_emit_recv(
		co_nmt_boot_t *boot, const struct can_msg *msg);

/**
 * Invokes the 'timeout' transition function of the current state of a 'boot
 * slave' service.
 *
 * @param boot a pointer to a 'boot slave' service.
 * @param tp   a pointer to the current time.
 */
static inline void co_nmt_boot_emit_time(
		co_nmt_boot_t *boot, const struct timespec *tp);

/**
 * Invokes the 'SDO download confirmation' transition function of the current
 * state of a 'boot slave' service.
 *
 * @param boot a pointer to a 'boot slave' service.
 * @param ac   the SDO abort code (0 on success).
 */
static inline void co_nmt_boot_emit_dn_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac);

/**
 * Invokes the 'SDO upload confirmation' transition function of the current
 * state of a 'boot slave' service.
 *
 * @param boot a pointer to a 'boot slave' service.
 * @param ac   the SDO abort code (0 on success).
 * @param ptr  a pointer to the uploaded bytes.
 * @param n    the number of bytes at <b>ptr</b>.
 */
static inline void co_nmt_boot_emit_up_con(co_nmt_boot_t *boot,
		co_unsigned32_t ac, const void *ptr, size_t n);

/**
 * Invokes the 'configuration request confirmation' transition function of the
 * current state of a 'boot slave' service.
 *
 * @param boot a pointer to a 'boot slave' service.
 * @param ac   the SDO abort code (0 on success).
 */
static inline void co_nmt_boot_emit_cfg_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac);

/// A CANopen NMT 'boot slave' state.
struct __co_nmt_boot_state {
	/// A pointer to the function invoked when a new state is entered.
	co_nmt_boot_state_t *(*on_enter)(co_nmt_boot_t *boot);
	/**
	 * A pointer to the transition function invoked when a CAN frame has
	 * been received.
	 *
	 * @param boot a pointer to a 'boot slave' service.
	 * @param msg  a pointer to the received CAN frame.
	 *
	 * @returns a pointer to the next state.
	 */
	co_nmt_boot_state_t *(*on_recv)(
			co_nmt_boot_t *boot, const struct can_msg *msg);
	/**
	 * A pointer to the transition function invoked when a timeout occurs.
	 *
	 * @param boot a pointer to a 'boot slave' service.
	 * @param tp   a pointer to the current time.
	 *
	 * @returns a pointer to the next state.
	 */
	co_nmt_boot_state_t *(*on_time)(
			co_nmt_boot_t *boot, const struct timespec *tp);
	/**
	 * A pointer to the transition function invoked when an SDO download
	 * request completes.
	 *
	 * @param boot a pointer to a 'boot slave' service.
	 * @param ac   the SDO abort code (0 on success).
	 *
	 * @returns a pointer to the next state.
	 */
	co_nmt_boot_state_t *(*on_dn_con)(
			co_nmt_boot_t *boot, co_unsigned32_t ac);
	/**
	 * A pointer to the transition function invoked when an SDO upload
	 * request completes.
	 *
	 * @param boot a pointer to a 'boot slave' service.
	 * @param ac   the SDO abort code.
	 * @param ptr  a pointer to the uploaded bytes.
	 * @param n    the number of bytes at <b>ptr</b>.
	 *
	 * @returns a pointer to the next state.
	 */
	co_nmt_boot_state_t *(*on_up_con)(co_nmt_boot_t *boot,
			co_unsigned32_t ac, const void *ptr, size_t n);
	/**
	 * A pointer to the transition function invoked when an NMT
	 * 'configuration request' completes.
	 *
	 * @param boot a pointer to a 'boot slave' service.
	 * @param ac   the SDO abort code (0 on success).
	 *
	 * @returns a pointer to the next state.
	 */
	co_nmt_boot_state_t *(*on_cfg_con)(
			co_nmt_boot_t *boot, co_unsigned32_t ac);
	/// A pointer to the function invoked when the current state is left.
	void (*on_leave)(co_nmt_boot_t *boot);
};

#define LELY_CO_DEFINE_STATE(name, ...) \
	static co_nmt_boot_state_t *const name = \
			&(co_nmt_boot_state_t){ __VA_ARGS__ };

/// The 'timeout' transition function of the 'wait asynchronously' state.
static co_nmt_boot_state_t *co_nmt_boot_wait_on_time(
		co_nmt_boot_t *boot, const struct timespec *tp);

/// The 'wait asynchronously' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_wait_state,
	.on_time = &co_nmt_boot_wait_on_time
)
// clang-format on

/// The entry function of the 'abort' state.
static co_nmt_boot_state_t *co_nmt_boot_abort_on_enter(co_nmt_boot_t *boot);

/// The 'abort' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_abort_state,
	.on_enter = &co_nmt_boot_abort_on_enter
)
// clang-format on

/// The entry function of the 'error' state.
static co_nmt_boot_state_t *co_nmt_boot_error_on_enter(co_nmt_boot_t *boot);

/// The exit function of the 'error' state.
static void co_nmt_boot_error_on_leave(co_nmt_boot_t *boot);

/// The 'error' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_error_state,
	.on_enter = &co_nmt_boot_error_on_enter,
	.on_leave = &co_nmt_boot_error_on_leave
)
// clang-format on

/// The entry function of the 'check device type' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_device_type_on_enter(
		co_nmt_boot_t *boot);

/**
 * The 'SDO upload confirmation' transition function of the 'check device type'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_device_type_on_up_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac, const void *ptr,
		size_t n);

/// The 'check device type' state (see Fig. 5 in CiA 302-2 version 4.1.0).
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_chk_device_type_state,
	.on_enter = &co_nmt_boot_chk_device_type_on_enter,
	.on_up_con = &co_nmt_boot_chk_device_type_on_up_con
)
// clang-format on

/// The entry function of the 'check vendor ID' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_vendor_id_on_enter(
		co_nmt_boot_t *boot);

/**
 * The 'SDO upload confirmation' transition function of the 'check vendor ID'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_vendor_id_on_up_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac, const void *ptr,
		size_t n);

/// The 'check vendor ID' state (see Fig. 5 in CiA 302-2 version 4.1.0).
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_chk_vendor_id_state,
	.on_enter = &co_nmt_boot_chk_vendor_id_on_enter,
	.on_up_con = &co_nmt_boot_chk_vendor_id_on_up_con
)
// clang-format on

/// The entry function of the 'check product code' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_product_code_on_enter(
		co_nmt_boot_t *boot);

/**
 * The 'SDO upload confirmation' transition function of the 'check product code'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_product_code_on_up_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac, const void *ptr,
		size_t n);

/// The 'check product code' state (see Fig. 5 in CiA 302-2 version 4.1.0).
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_chk_product_code_state,
	.on_enter = &co_nmt_boot_chk_product_code_on_enter,
	.on_up_con = &co_nmt_boot_chk_product_code_on_up_con
)
// clang-format on

/// The entry function of the 'check revision number' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_revision_on_enter(
		co_nmt_boot_t *boot);

/**
 * The 'SDO upload confirmation' transition function of the 'check revision
 * number' state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_revision_on_up_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac, const void *ptr,
		size_t n);

/// The 'check revision number' state (see Fig. 5 in CiA 302-2 version 4.1.0).
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_chk_revision_state,
	.on_enter = &co_nmt_boot_chk_revision_on_enter,
	.on_up_con = &co_nmt_boot_chk_revision_on_up_con
)
// clang-format on

/// The entry function of the 'check serial number' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_serial_nr_on_enter(
		co_nmt_boot_t *boot);

/**
 * The 'SDO upload confirmation' transition function of the 'check serial
 * number' state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_serial_nr_on_up_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac, const void *ptr,
		size_t n);

/// The 'check serial number' state (see Fig. 5 in CiA 302-2 version 4.1.0).
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_chk_serial_nr_state,
	.on_enter = &co_nmt_boot_chk_serial_nr_on_enter,
	.on_up_con = &co_nmt_boot_chk_serial_nr_on_up_con
)
// clang-format on

/// The entry function of the 'check node state' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_node_on_enter(co_nmt_boot_t *boot);

/**
 * The 'CAN frame received' transition function of the 'check node state' state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_node_on_recv(
		co_nmt_boot_t *boot, const struct can_msg *msg);

/// The 'timeout' transition function of the 'check node state' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_node_on_time(
		co_nmt_boot_t *boot, const struct timespec *tp);

/// The exit function of the 'check node state' state.
static void co_nmt_boot_chk_node_on_leave(co_nmt_boot_t *boot);

/// The 'check node state' state (see Fig. 6 in CiA 302-2 version 4.1.0).
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_chk_node_state,
	.on_enter = &co_nmt_boot_chk_node_on_enter,
	.on_recv = &co_nmt_boot_chk_node_on_recv,
	.on_time = &co_nmt_boot_chk_node_on_time,
	.on_leave = &co_nmt_boot_chk_node_on_leave
)
// clang-format on

/// The entry function of the 'reset communication' state.
static co_nmt_boot_state_t *co_nmt_boot_reset_comm_on_enter(
		co_nmt_boot_t *boot);

/**
 * The 'CAN frame received' transition function of the 'reset communication'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_reset_comm_on_recv(
		co_nmt_boot_t *boot, const struct can_msg *msg);

/// The 'timeout' transition function of the 'reset communication' state.
static co_nmt_boot_state_t *co_nmt_boot_reset_comm_on_time(
		co_nmt_boot_t *boot, const struct timespec *tp);

/// The 'reset communication' state (see Fig. 6 in CiA 302-2 version 4.1.0).
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_reset_comm_state,
	.on_enter = &co_nmt_boot_reset_comm_on_enter,
	.on_recv = &co_nmt_boot_reset_comm_on_recv,
	.on_time = &co_nmt_boot_reset_comm_on_time
)
// clang-format on

/// The entry function of the 'check software' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_sw_on_enter(co_nmt_boot_t *boot);

/**
 * The 'SDO upload confirmation' transition function of the 'check software'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_sw_on_up_con(co_nmt_boot_t *boot,
		co_unsigned32_t ac, const void *ptr, size_t n);

/// The 'check software' state (see Fig. 6 in CiA 302-2 version 4.1.0).
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_chk_sw_state,
	.on_enter = &co_nmt_boot_chk_sw_on_enter,
	.on_up_con = &co_nmt_boot_chk_sw_on_up_con
)
// clang-format on

/// The entry function of the 'stop program' state.
static co_nmt_boot_state_t *co_nmt_boot_stop_prog_on_enter(co_nmt_boot_t *boot);

/**
 * The 'SDO download confirmation' transition function of the 'stop program'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_stop_prog_on_dn_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac);

/**
 * The 'SDO upload confirmation' transition function of the 'stop program'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_stop_prog_on_up_con(co_nmt_boot_t *boot,
		co_unsigned32_t ac, const void *ptr, size_t n);

/// The 'stop program' state (see Fig. 3 in CiA 302-3 version 4.1.0).
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_stop_prog_state,
	.on_enter = &co_nmt_boot_stop_prog_on_enter,
	.on_dn_con = &co_nmt_boot_stop_prog_on_dn_con,
	.on_up_con = &co_nmt_boot_stop_prog_on_up_con
)
// clang-format on

/// The entry function of the 'clear program' state.
static co_nmt_boot_state_t *co_nmt_boot_clear_prog_on_enter(
		co_nmt_boot_t *boot);

/**
 * The 'SDO download confirmation' transition function of the 'clear program'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_clear_prog_on_dn_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac);

/// The 'clear program' state (see Fig. 3 in CiA 302-3 version 4.1.0).
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_clear_prog_state,
	.on_enter = &co_nmt_boot_clear_prog_on_enter,
	.on_dn_con = &co_nmt_boot_clear_prog_on_dn_con
)
// clang-format on

/// The entry function of the 'download program' state.
static co_nmt_boot_state_t *co_nmt_boot_blk_dn_prog_on_enter(
		co_nmt_boot_t *boot);

/**
 * The 'SDO download confirmation' transition function of the 'download program'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_blk_dn_prog_on_dn_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac);

/// The 'download program' state (see Fig. 3 in CiA 302-3 version 4.1.0).
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_blk_dn_prog_state,
	.on_enter = &co_nmt_boot_blk_dn_prog_on_enter,
	.on_dn_con = &co_nmt_boot_blk_dn_prog_on_dn_con
)
// clang-format on

/// The entry function of the 'download program' state.
static co_nmt_boot_state_t *co_nmt_boot_dn_prog_on_enter(co_nmt_boot_t *boot);

/**
 * The 'SDO download confirmation' transition function of the 'download program'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_dn_prog_on_dn_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac);

/// The 'download program' state (see Fig. 3 in CiA 302-3 version 4.1.0).
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_dn_prog_state,
	.on_enter = &co_nmt_boot_dn_prog_on_enter,
	.on_dn_con = &co_nmt_boot_dn_prog_on_dn_con
)
// clang-format on

/// The entry function of the 'wait for end of flashing' state.
static co_nmt_boot_state_t *co_nmt_boot_wait_flash_on_enter(
		co_nmt_boot_t *boot);

/// The 'timeout' transition function of the 'wait for end of flashing' state.
static co_nmt_boot_state_t *co_nmt_boot_wait_flash_on_time(
		co_nmt_boot_t *boot, const struct timespec *tp);

/**
 * The 'SDO upload confirmation' transition function of the 'wait for end of
 * flashing' state.
 */
static co_nmt_boot_state_t *co_nmt_boot_wait_flash_on_up_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac, const void *ptr,
		size_t n);

/// The 'check flashing' state (see Fig. 3 in CiA 302-3 version 4.1.0).
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_wait_flash_state,
	.on_enter = &co_nmt_boot_wait_flash_on_enter,
	.on_time = &co_nmt_boot_wait_flash_on_time,
	.on_up_con = &co_nmt_boot_wait_flash_on_up_con
)
// clang-format on

/// The entry function of the 'check program SW ID state.
static co_nmt_boot_state_t *co_nmt_boot_chk_prog_on_enter(co_nmt_boot_t *boot);

/**
 * The 'SDO upload confirmation' transition function of the 'check program
 * SW ID' state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_prog_on_up_con(co_nmt_boot_t *boot,
		co_unsigned32_t ac, const void *ptr, size_t n);

/// The 'check program SW ID' state (see Fig. 8 in CiA 302-2 version 4.1.0).
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_chk_prog_state,
	.on_enter = &co_nmt_boot_chk_prog_on_enter,
	.on_up_con = &co_nmt_boot_chk_prog_on_up_con
)
// clang-format on

/// The entry function of the 'start program' state.
static co_nmt_boot_state_t *co_nmt_boot_start_prog_on_enter(
		co_nmt_boot_t *boot);

/**
 * The 'SDO download confirmation' transition function of the 'start program'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_start_prog_on_dn_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac);

/// The 'start program' state (see Fig. 3 in CiA 302-3 version 4.1.0).
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_start_prog_state,
	.on_enter = &co_nmt_boot_start_prog_on_enter,
	.on_dn_con = &co_nmt_boot_start_prog_on_dn_con
)
// clang-format on

/// The entry function of the 'wait till program is started' state.
static co_nmt_boot_state_t *co_nmt_boot_wait_prog_on_enter(co_nmt_boot_t *boot);

/**
 * The 'timeout' transition function of the 'wait till program is started'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_wait_prog_on_time(
		co_nmt_boot_t *boot, const struct timespec *tp);

/**
 * The 'SDO upload confirmation' transition function of the 'wait till program
 * is started' state.
 */
static co_nmt_boot_state_t *co_nmt_boot_wait_prog_on_up_con(co_nmt_boot_t *boot,
		co_unsigned32_t ac, const void *ptr, size_t n);

/**
 * The 'wait till program is started' state (see Fig. 8 in CiA 302-2 version
 * 4.1.0).
 */
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_wait_prog_state,
	.on_enter = &co_nmt_boot_wait_prog_on_enter,
	.on_time = &co_nmt_boot_wait_prog_on_time,
	.on_up_con = &co_nmt_boot_wait_prog_on_up_con
)
// clang-format on

/// The entry function of the 'check configuration date' state.
static co_nmt_boot_state_t *co_nmt_boot_chk_cfg_date_on_enter(
		co_nmt_boot_t *boot);

/**
 * The 'SDO upload confirmation' transition function of the 'check configuration
 * date' state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_cfg_date_on_up_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac, const void *ptr,
		size_t n);

/**
 * The 'check configuration date' state (see Fig. 8 in CiA 302-2 version 4.1.0).
 */
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_chk_cfg_date_state,
	.on_enter = &co_nmt_boot_chk_cfg_date_on_enter,
	.on_up_con = &co_nmt_boot_chk_cfg_date_on_up_con
)
// clang-format on

/**
 * The 'SDO upload confirmation' transition function of the 'check configuration
 * time' state.
 */
static co_nmt_boot_state_t *co_nmt_boot_chk_cfg_time_on_up_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac, const void *ptr,
		size_t n);

/**
 * The 'check configuration time' state (see Fig. 8 in CiA 302-2 version 4.1.0).
 */
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_chk_cfg_time_state,
	.on_up_con = &co_nmt_boot_chk_cfg_time_on_up_con
)
// clang-format on

/// The entry function of the 'update configuration' state.
static co_nmt_boot_state_t *co_nmt_boot_up_cfg_on_enter(co_nmt_boot_t *boot);

/**
 * The 'configuration request confirmation' transition unction of the 'update
 * configuration' state.
 */
static co_nmt_boot_state_t *co_nmt_boot_up_cfg_on_cfg_con(
		co_nmt_boot_t *boot, co_unsigned32_t ac);

/**
 * The 'update configuration' state (see Fig. 8 in CiA 302-2 version 4.1.0).
 */
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_up_cfg_state,
	.on_enter = &co_nmt_boot_up_cfg_on_enter,
	.on_cfg_con = &co_nmt_boot_up_cfg_on_cfg_con
)
// clang-format on

/// The entry function of the 'start error control' state.
static co_nmt_boot_state_t *co_nmt_boot_ec_on_enter(co_nmt_boot_t *boot);

/**
 * The 'CAN frame received' transition function of the 'start error control'
 * state.
 */
static co_nmt_boot_state_t *co_nmt_boot_ec_on_recv(
		co_nmt_boot_t *boot, const struct can_msg *msg);

/// The 'timeout' transition function of the 'start error control' state.
static co_nmt_boot_state_t *co_nmt_boot_ec_on_time(
		co_nmt_boot_t *boot, const struct timespec *tp);

/// The 'start error control' state (see Fig. 11 in CiA 302-2 version 4.1.0).
// clang-format off
LELY_CO_DEFINE_STATE(co_nmt_boot_ec_state,
	.on_enter = &co_nmt_boot_ec_on_enter,
	.on_recv = &co_nmt_boot_ec_on_recv,
	.on_time = &co_nmt_boot_ec_on_time
)
// clang-format on

#undef LELY_CO_DEFINE_STATE

/**
 * Issues an SDO download request to the slave. co_nmt_boot_dn_con() is called
 * upon completion of the request.
 *
 * @param boot   a pointer to a 'boot slave' service.
 * @param idx    the remote object index.
 * @param subidx the remote object sub-index.
 * @param type   the data type.
 * @param val    the address of the value to be written.
 *
 * @returns 0 on success, or -1 on error.
 */
static int co_nmt_boot_dn(co_nmt_boot_t *boot, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned16_t type, const void *val);

/**
 * Issues an SDO upload request to the slave. co_nmt_boot_up_con() is called
 * upon completion of the request.
 *
 * @param boot   a pointer to a 'boot slave' service.
 * @param idx    the remote object index.
 * @param subidx the remote object sub-index.
 *
 * @returns 0 on success, or -1 on error.
 */
static int co_nmt_boot_up(co_nmt_boot_t *boot, co_unsigned16_t idx,
		co_unsigned8_t subidx);

/**
 * Compares the result of an SDO upload request to the value of a local
 * sub-object.
 *
 * @param boot   a pointer to a 'boot slave' service.
 * @param idx    the local object index.
 * @param subidx the local object sub-index.
 * @param ptr    a pointer to the uploaded bytes.
 * @param n      the number of bytes at <b>ptr</b>.
 *
 * @returns 1 if the received value matches that of the specified sub-object,
 * and 0 if not.
 */
static int co_nmt_boot_chk(co_nmt_boot_t *boot, co_unsigned16_t idx,
		co_unsigned8_t subidx, const void *ptr, size_t n);

#if !LELY_NO_CO_NG
/**
 * Sends a node guarding RTR to the slave.
 *
 * @returns 0 on success, or -1 on error.
 */
static int co_nmt_boot_send_rtr(co_nmt_boot_t *boot);
#endif

void *
__co_nmt_boot_alloc(void)
{
	void *ptr = malloc(sizeof(struct __co_nmt_boot));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
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

	int errc = 0;

	boot->net = net;
	boot->dev = dev;
	boot->nmt = nmt;

	boot->state = NULL;

	boot->recv = can_recv_create();
	if (!boot->recv) {
		errc = get_errc();
		goto error_create_recv;
	}
	can_recv_set_func(boot->recv, &co_nmt_boot_recv, boot);

	boot->timer = can_timer_create();
	if (!boot->timer) {
		errc = get_errc();
		goto error_create_timer;
	}
	can_timer_set_func(boot->timer, &co_nmt_boot_timer, boot);

	boot->id = 0;

	boot->timeout = 0;
	boot->sdo = NULL;

	boot->start = (struct timespec){ 0, 0 };
	can_net_get_time(boot->net, &boot->start);

	boot->assignment = 0;
	boot->ms = 0;

	boot->st = 0;
	boot->es = 0;

	co_sdo_req_init(&boot->req);
	boot->retry = 0;

	co_nmt_boot_enter(boot, co_nmt_boot_wait_state);
	return boot;

	// can_timer_destroy(boot->timer);
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

	co_sdo_req_fini(&boot->req);

	co_csdo_destroy(boot->sdo);

	can_timer_destroy(boot->timer);
	can_recv_destroy(boot->recv);
}

co_nmt_boot_t *
co_nmt_boot_create(can_net_t *net, co_dev_t *dev, co_nmt_t *nmt)
{
	int errc = 0;

	co_nmt_boot_t *boot = __co_nmt_boot_alloc();
	if (!boot) {
		errc = get_errc();
		goto error_alloc_boot;
	}

	if (!__co_nmt_boot_init(boot, net, dev, nmt)) {
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

int
co_nmt_boot_boot_req(co_nmt_boot_t *boot, co_unsigned8_t id, int timeout,
		co_csdo_ind_t *dn_ind, co_csdo_ind_t *up_ind, void *data)
{
	assert(boot);

	if (!id || id > CO_NUM_NODES) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	if (boot->state != co_nmt_boot_wait_state) {
		set_errnum(ERRNUM_INPROGRESS);
		return -1;
	}

	boot->id = id;

	boot->timeout = timeout;
	co_csdo_destroy(boot->sdo);
	boot->sdo = co_csdo_create(boot->net, NULL, boot->id);
	if (!boot->sdo)
		return -1;
	co_csdo_set_timeout(boot->sdo, boot->timeout);
	co_csdo_set_dn_ind(boot->sdo, dn_ind, data);
	co_csdo_set_up_ind(boot->sdo, up_ind, data);

	co_nmt_boot_emit_time(boot, NULL);

	return 0;
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
co_nmt_boot_dn_con(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned32_t ac, void *data)
{
	(void)sdo;
	(void)idx;
	(void)subidx;
	co_nmt_boot_t *boot = data;
	assert(boot);

	co_nmt_boot_emit_dn_con(boot, ac);
}

static void
co_nmt_boot_up_con(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned32_t ac, const void *ptr, size_t n, void *data)
{
	(void)sdo;
	(void)idx;
	(void)subidx;
	co_nmt_boot_t *boot = data;
	assert(boot);

	co_nmt_boot_emit_up_con(boot, ac, ptr, n);
}

static void
co_nmt_boot_cfg_con(co_nmt_t *nmt, co_unsigned8_t id, co_unsigned32_t ac,
		void *data)
{
	(void)nmt;
	(void)id;
	co_nmt_boot_t *boot = data;
	assert(boot);

	co_nmt_boot_emit_cfg_con(boot, ac);
}

static void
co_nmt_boot_enter(co_nmt_boot_t *boot, co_nmt_boot_state_t *next)
{
	assert(boot);

	while (next) {
		co_nmt_boot_state_t *prev = boot->state;
		boot->state = next;

		if (prev && prev->on_leave)
			prev->on_leave(boot);

		next = next->on_enter ? next->on_enter(boot) : NULL;
	}
}

static inline void
co_nmt_boot_emit_recv(co_nmt_boot_t *boot, const struct can_msg *msg)
{
	assert(boot);
	assert(boot->state);
	assert(boot->state->on_recv);

	co_nmt_boot_enter(boot, boot->state->on_recv(boot, msg));
}

static inline void
co_nmt_boot_emit_time(co_nmt_boot_t *boot, const struct timespec *tp)
{
	assert(boot);
	assert(boot->state);
	assert(boot->state->on_time);

	co_nmt_boot_enter(boot, boot->state->on_time(boot, tp));
}

static inline void
co_nmt_boot_emit_dn_con(co_nmt_boot_t *boot, co_unsigned32_t ac)
{
	assert(boot);
	assert(boot->state);
	assert(boot->state->on_dn_con);

	co_nmt_boot_enter(boot, boot->state->on_dn_con(boot, ac));
}

static inline void
co_nmt_boot_emit_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);
	assert(boot->state);
	assert(boot->state->on_up_con);

	co_nmt_boot_enter(boot, boot->state->on_up_con(boot, ac, ptr, n));
}

static inline void
co_nmt_boot_emit_cfg_con(co_nmt_boot_t *boot, co_unsigned32_t ac)
{
	assert(boot);
	assert(boot->state);
	assert(boot->state->on_cfg_con);

	co_nmt_boot_enter(boot, boot->state->on_cfg_con(boot, ac));
}

static co_nmt_boot_state_t *
co_nmt_boot_wait_on_time(co_nmt_boot_t *boot, const struct timespec *tp)
{
	(void)boot;
	(void)tp;

	boot->st = 0;
	boot->es = 0;

	// Retrieve the slave assignment for the node.
	boot->assignment = co_dev_get_val_u32(boot->dev, 0x1f81, boot->id);

	// Find the consumer heartbeat time for the node.
	boot->ms = 0;
	co_obj_t *obj_1016 = co_dev_find_obj(boot->dev, 0x1016);
	if (obj_1016) {
		co_unsigned8_t n = co_obj_get_val_u8(obj_1016, 0x00);
		for (size_t i = 1; i <= n; i++) {
			co_unsigned32_t val =
					co_obj_get_val_u32(obj_1016, i & 0xff);
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
		int wait = 1;
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
	(void)boot;

	return co_nmt_boot_wait_state;
}

static void
co_nmt_boot_error_on_leave(co_nmt_boot_t *boot)
{
	assert(boot);

	co_nmt_boot_con(boot->nmt, boot->id, boot->st, boot->es);
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_device_type_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	boot->es = 'B';

	// The device type check may follow an NMT 'reset communication'
	// command, in which case we may have to give the slave some time to
	// complete the state change. Start the first SDO request by simulating
	// a timeout.
	boot->retry = LELY_CO_NMT_BOOT_SDO_RETRY + 1;
	return co_nmt_boot_chk_device_type_on_up_con(
			boot, CO_SDO_AC_TIMEOUT, NULL, 0);
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_device_type_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

	// Retry the SDO request on timeout (this includes the first attempt).
	if (ac == CO_SDO_AC_TIMEOUT && boot->retry--) {
		// Read the device type of the slave (object 1000).
		if (co_nmt_boot_up(boot, 0x1000, 0x00) == -1)
			return co_nmt_boot_abort_state;
		return NULL;
	} else if (ac) {
#if !LELY_NO_STDIO
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" received on upload request of object 1000 (Device type) to node %02X: %s",
				ac, boot->id, co_sdo_ac2str(ac));
#endif
		return co_nmt_boot_abort_state;
	}

	// If the expected device type (sub-object 1F84:ID) is 0, skip the check
	// and proceed with the vendor ID.
	co_unsigned32_t device_type =
			co_dev_get_val_u32(boot->dev, 0x1f84, boot->id);
	if (device_type && !co_nmt_boot_chk(boot, 0x1f84, boot->id, ptr, n)) {
		boot->es = 'C';
		return co_nmt_boot_abort_state;
	}

	can_recv_stop(boot->recv);
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
	if (co_nmt_boot_up(boot, 0x1018, 0x01) == -1)
		return co_nmt_boot_abort_state;

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_vendor_id_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

#if !LELY_NO_STDIO
	if (ac)
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" received on upload request of sub-object 1018:01 (Vendor-ID) to node %02X: %s",
				ac, boot->id, co_sdo_ac2str(ac));
#endif

	if (ac || !co_nmt_boot_chk(boot, 0x1f85, boot->id, ptr, n))
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
	if (co_nmt_boot_up(boot, 0x1018, 0x02) == -1)
		return co_nmt_boot_abort_state;

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_product_code_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

#if !LELY_NO_STDIO
	if (ac)
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" received on upload request of sub-object 1018:02 (Product code) to node %02X: %s",
				ac, boot->id, co_sdo_ac2str(ac));
#endif

	if (ac || !co_nmt_boot_chk(boot, 0x1f86, boot->id, ptr, n))
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
	if (co_nmt_boot_up(boot, 0x1018, 0x03) == -1)
		return co_nmt_boot_abort_state;

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_revision_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

#if !LELY_NO_STDIO
	if (ac)
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" received on upload request of sub-object 1018:03 (Revision number) to node %02X: %s",
				ac, boot->id, co_sdo_ac2str(ac));
#endif

	if (ac || !co_nmt_boot_chk(boot, 0x1f87, boot->id, ptr, n))
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
	if (co_nmt_boot_up(boot, 0x1018, 0x04) == -1)
		return co_nmt_boot_abort_state;

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_serial_nr_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

#if !LELY_NO_STDIO
	if (ac)
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" received on upload request of sub-object 1018:04 (Serial number) to node %02X: %s",
				ac, boot->id, co_sdo_ac2str(ac));
#endif

	if (ac || !co_nmt_boot_chk(boot, 0x1f88, boot->id, ptr, n))
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
			boot->es = 'E';
			ms = boot->ms;
		} else {
			boot->es = 'F';
#if LELY_NO_CO_NG
			return co_nmt_boot_abort_state;
#else
			ms = LELY_CO_NMT_BOOT_RTR_TIMEOUT;
			// If we're not a heartbeat consumer, start node
			// guarding by sending the first RTR.
			co_nmt_boot_send_rtr(boot);
#endif
		}

		// Start the CAN frame receiver for the heartbeat or node guard
		// message.
		can_recv_start(boot->recv, boot->net, CO_NMT_EC_CANID(boot->id),
				0);
		// Start the CAN timer in case we do not receive a heartbeat
		// indication or a node guard confirmation.
		can_timer_timeout(boot->timer, boot->net, ms);

		return NULL;
	}

	return co_nmt_boot_chk_sw_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_node_on_recv(co_nmt_boot_t *boot, const struct can_msg *msg)
{
	assert(boot);
	assert(msg);
	assert(msg->id == (uint_least32_t)CO_NMT_EC_CANID(boot->id));

	can_timer_stop(boot->timer);

	if (msg->len >= 1) {
		boot->st = msg->data[0];
		if ((boot->st & ~CO_NMT_ST_TOGGLE) == CO_NMT_ST_START)
			// If the node is already operational, skip the 'check
			// and update software version' and 'check
			// configuration' steps and proceed immediately to
			// 'start error control service'.
			return co_nmt_boot_ec_state;
	}
	boot->st = 0;
	// If the node is not operational, send the NMT 'reset
	// communication' command and proceed as if the keep-alive bit
	// was not set.
	co_nmt_cs_req(boot->nmt, CO_NMT_CS_RESET_COMM, boot->id);
	return co_nmt_boot_reset_comm_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_node_on_time(co_nmt_boot_t *boot, const struct timespec *tp)
{
	(void)boot;
	(void)tp;

	return co_nmt_boot_abort_state;
}

static void
co_nmt_boot_chk_node_on_leave(co_nmt_boot_t *boot)
{
	assert(boot);

	can_recv_stop(boot->recv);
}

static co_nmt_boot_state_t *
co_nmt_boot_reset_comm_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	// Start the CAN frame receiver for the boot-up message.
	can_recv_start(boot->recv, boot->net, CO_NMT_EC_CANID(boot->id), 0);
	// Wait until we receive a boot-up message.
	can_timer_timeout(
			boot->timer, boot->net, LELY_CO_NMT_BOOT_RESET_TIMEOUT);

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_reset_comm_on_recv(co_nmt_boot_t *boot, const struct can_msg *msg)
{
	(void)boot;
	(void)msg;

	can_recv_stop(boot->recv);
	return co_nmt_boot_chk_sw_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_reset_comm_on_time(co_nmt_boot_t *boot, const struct timespec *tp)
{
	(void)boot;
	(void)tp;

	return co_nmt_boot_abort_state;
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

		// The software version check may follow an NMT 'reset
		// communication' command, in which case we may have to give the
		// slave some time to complete the state change. Start the first
		// SDO request by simulating a timeout.
		boot->retry = LELY_CO_NMT_BOOT_SDO_RETRY + 1;
		return co_nmt_boot_chk_sw_on_up_con(
				boot, CO_SDO_AC_TIMEOUT, NULL, 0);
	}

	// Continue with the 'check configuration' step if the software version
	// check is not necessary.
	return co_nmt_boot_chk_cfg_date_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_sw_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

	// Retry the SDO request on timeout (this includes the first attempt).
	if (ac == CO_SDO_AC_TIMEOUT && boot->retry--) {
		// Read the program software identification of the slave
		// (sub-object 1F56:01).
		if (co_nmt_boot_up(boot, 0x1f56, 0x01) == -1)
			return co_nmt_boot_abort_state;
		return NULL;
	} else if (ac) {
#if !LELY_NO_STDIO
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" received on upload request of sub-object 1F56:01 (Program software identification) to node %02X: %s",
				ac, boot->id, co_sdo_ac2str(ac));
#endif
		return co_nmt_boot_abort_state;
	}

	// If the program software identification matches the expected value,
	// proceed to 'check configuration'.
	if (co_nmt_boot_chk(boot, 0x1f55, boot->id, ptr, n))
		return co_nmt_boot_chk_cfg_date_state;

	// Do not update the software if software update (bit 6) is not allowed
	// or if the keep-alive bit (bit 4) is set.
	if ((boot->assignment & 0x50) != 0x40) {
		boot->es = 'H';
		return co_nmt_boot_abort_state;
	}

	boot->es = 'I';

	return co_nmt_boot_stop_prog_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_stop_prog_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	// Read the program control of the slave (sub-object 1F51:01).
	if (co_nmt_boot_up(boot, 0x1f51, 0x01) == -1)
		return co_nmt_boot_abort_state;

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_stop_prog_on_dn_con(co_nmt_boot_t *boot, co_unsigned32_t ac)
{
#if LELY_NO_STDIO
	(void)boot;
#else
	assert(boot);
#endif

	// The download SDO request may be unconfirmed on some devices since it
	// stops the program on the slave (and may cause a restart of the
	// bootloader). We therefore ignore timeouts.
	if (ac && ac != CO_SDO_AC_TIMEOUT) {
#if !LELY_NO_STDIO
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" received on download request of sub-object 1F51:01 (Program control) to node %02X: %s",
				ac, boot->id, co_sdo_ac2str(ac));
#endif
		return co_nmt_boot_abort_state;
	}

	return co_nmt_boot_clear_prog_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_stop_prog_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

	// If the value is already 0 (Program stopped), do not write a 0 (Stop
	// program), but skip to the 'clear program' state.
	co_unsigned8_t val = 0;
	// clang-format off
	if (!ac && co_val_read(CO_DEFTYPE_UNSIGNED8, &val, ptr,
			(const uint_least8_t *)ptr + n) && !val)
		// clang-format on
		return co_nmt_boot_clear_prog_state;

	// Write a 0 (Stop program) to the program control of the slave
	// (sub-object 1F51:01).
	// clang-format off
	if (co_nmt_boot_dn(boot, 0x1f51, 0x01, CO_DEFTYPE_UNSIGNED8,
			&(co_unsigned8_t){ 0 }) == -1)
		// clang-format on
		return co_nmt_boot_abort_state;

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_clear_prog_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	// The 'clear program' command follows the 'stop program' command, which
	// may have triggered a reboot of the slave. In that case we may have to
	// give the slave some time to finish booting. Start the first SDO
	// request by simulating a timeout.
	boot->retry = LELY_CO_NMT_BOOT_SDO_RETRY + 1;
	return co_nmt_boot_clear_prog_on_dn_con(boot, CO_SDO_AC_TIMEOUT);
}

static co_nmt_boot_state_t *
co_nmt_boot_clear_prog_on_dn_con(co_nmt_boot_t *boot, co_unsigned32_t ac)
{
	assert(boot);

	// Retry the SDO request on timeout.
	if (ac == CO_SDO_AC_TIMEOUT && boot->retry--) {
		// Write a 3 (Clear program) to the program control of the slave
		// (sub-object 1F51:01).
		// clang-format off
		if (co_nmt_boot_dn(boot, 0x1f51, 0x01, CO_DEFTYPE_UNSIGNED8,
				&(co_unsigned8_t){ 3 }) == -1)
			// clang-format on
			return co_nmt_boot_abort_state;
		return NULL;
	} else if (ac) {
#if !LELY_NO_STDIO
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" received on download request of sub-object 1F51:01 (Program control) to node %02X: %s",
				ac, boot->id, co_sdo_ac2str(ac));
#endif
		return co_nmt_boot_abort_state;
	}

	return co_nmt_boot_blk_dn_prog_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_blk_dn_prog_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	co_sub_t *sub = co_dev_find_sub(boot->dev, 0x1f58, boot->id);
	if (!sub)
		return co_nmt_boot_abort_state;

	// Upload the program data.
	struct co_sdo_req *req = &boot->req;
	co_sdo_req_clear(req);
	co_unsigned32_t ac = co_sub_up_ind(sub, req);
	if (ac || !co_sdo_req_first(req) || !co_sdo_req_last(req)) {
#if !LELY_NO_STDIO
		if (ac)
			diag(DIAG_ERROR, 0,
					"SDO abort code %08" PRIX32
					" on upload request of object 1F58:%02X (Program data): %s",
					ac, boot->id, co_sdo_ac2str(ac));
#endif
		return co_nmt_boot_abort_state;
	}

	// The 'clear program' step may take some time to complete, causing an
	// immediate 'download program' to generate a timeout. Start the first
	// attempt by simulating a timeout.
	boot->retry = LELY_CO_NMT_BOOT_SDO_RETRY + 1;
	return co_nmt_boot_blk_dn_prog_on_dn_con(boot, CO_SDO_AC_TIMEOUT);
}

static co_nmt_boot_state_t *
co_nmt_boot_blk_dn_prog_on_dn_con(co_nmt_boot_t *boot, co_unsigned32_t ac)
{
	(void)boot;

	// Retry the SDO request on timeout (this includes the first attempt).
	if (ac == CO_SDO_AC_TIMEOUT && boot->retry--) {
		struct co_sdo_req *req = &boot->req;
		// Write the program data (sub-object 1F58:ID) to the program
		// data of the slave (sub-object 1F50:01) using SDO block
		// transfer.
		// clang-format off
		if (co_csdo_blk_dn_req(boot->sdo, 0x1f50, 0x01, req->buf,
				req->size, &co_nmt_boot_dn_con, boot) == -1)
			// clang-format on
			return co_nmt_boot_abort_state;
		return NULL;
	} else if (ac) {
		// If SDO block transfer is not supported, fall back to SDO
		// segmented transfer.
		return co_nmt_boot_dn_prog_state;
	}

	return co_nmt_boot_wait_flash_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_dn_prog_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	// If SDO block transfer is not supported, we may still have to wait for
	// the 'clear program' step to complete before successfully doing a
	// segmented SDO transfer. Start the first attempt by simulating a
	// timeout.
	boot->retry = LELY_CO_NMT_BOOT_SDO_RETRY + 1;
	return co_nmt_boot_dn_prog_on_dn_con(boot, CO_SDO_AC_TIMEOUT);
}

static co_nmt_boot_state_t *
co_nmt_boot_dn_prog_on_dn_con(co_nmt_boot_t *boot, co_unsigned32_t ac)
{
	assert(boot);

	// Retry the SDO request on timeout (this includes the first attempt).
	if (ac == CO_SDO_AC_TIMEOUT && boot->retry--) {
		struct co_sdo_req *req = &boot->req;
		// Write the program data (sub-object 1F58:ID) to the program
		// data of the slave (sub-object 1F50:01) using SDO segmented
		// transfer.
		// clang-format off
		if (co_csdo_dn_req(boot->sdo, 0x1f50, 0x01, req->buf, req->size,
				&co_nmt_boot_dn_con, boot) == -1)
			// clang-format on
			return co_nmt_boot_abort_state;
		return NULL;
	} else if (ac) {
#if !LELY_NO_STDIO
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" received on download request of sub-object 1F50:01 (Program data) to node %02X: %s",
				ac, boot->id, co_sdo_ac2str(ac));
#endif
		return co_nmt_boot_abort_state;
	}

	return co_nmt_boot_wait_flash_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_wait_flash_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	// Wait for a while before checking the flash status indication.
	can_timer_timeout(
			boot->timer, boot->net, LELY_CO_NMT_BOOT_CHECK_TIMEOUT);

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_wait_flash_on_time(co_nmt_boot_t *boot, const struct timespec *tp)
{
	assert(boot);
	(void)tp;

	// Read the flash status indication of the slave (sub-object 1F57:01).
	if (co_nmt_boot_up(boot, 0x1f57, 0x01) == -1)
		return co_nmt_boot_abort_state;

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_wait_flash_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
#if LELY_NO_STDIO
	(void)boot;
	(void)ac;
#else
	assert(boot);

	if (ac)
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" received on upload request of sub-object 1F57:01 (Flash status indication) to node %02X: %s",
				ac, boot->id, co_sdo_ac2str(ac));
#endif

	// If the flash status indication is not valid, try again.
	co_unsigned32_t val = 0;
	// clang-format off
	if (!co_val_read(CO_DEFTYPE_UNSIGNED32, &val, ptr,
			(const uint_least8_t *)ptr + n) || (val & 0x01))
		// clang-format on
		return co_nmt_boot_wait_flash_state;

	co_unsigned8_t st = (val >> 1) & 0x7f;
	switch (st) {
	case 0: return co_nmt_boot_chk_prog_state;
	case 1:
		diag(DIAG_ERROR, 0,
				"flash status identification %d: No valid program available",
				st);
		break;
	case 2:
		diag(DIAG_ERROR, 0,
				"flash status identification %d: Data format unknown",
				st);
		break;
	case 3:
		diag(DIAG_ERROR, 0,
				"flash status identification %d: Data format error or data CRC error",
				st);
		break;
	case 4:
		diag(DIAG_ERROR, 0,
				"flash status identification %d: Flash not cleared before write",
				st);
		break;
	case 5:
		diag(DIAG_ERROR, 0,
				"flash status identification %d: Flash write error",
				st);
		break;
	case 6:
		diag(DIAG_ERROR, 0,
				"flash status identification %d: General address error",
				st);
		break;
	case 7:
		diag(DIAG_ERROR, 0,
				"flash status identification %d: Flash secured (= write access currently forbidden)",
				st);
		break;
	case 63:
		diag(DIAG_ERROR, 0,
				"flash status identification %d: Unspecified error",
				st);
		break;
	default:
#if !LELY_NO_STDIO
		if (st > 63)
			diag(DIAG_ERROR, 0,
					"flash status identification %d: Manufacturer-specific error: 0x%08" PRIX32
					"",
					st, (val >> 16) & 0xffff);
#endif
		break;
	}

	return co_nmt_boot_abort_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_prog_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	// Read the program software identification of the slave (sub-object
	// 1F56:01).
	if (co_nmt_boot_up(boot, 0x1f56, 0x01) == -1)
		return co_nmt_boot_abort_state;

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_prog_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

#if !LELY_NO_STDIO
	if (ac)
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" received on upload request of sub-object 1F56:01 (Program software identification) to node %02X: %s",
				ac, boot->id, co_sdo_ac2str(ac));
#endif

	if (ac || !co_nmt_boot_chk(boot, 0x1f55, boot->id, ptr, n))
		return co_nmt_boot_abort_state;

	return co_nmt_boot_start_prog_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_start_prog_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	// Write a 1 (Start program) to the program control of the slave
	// (sub-object 1F51:01).
	// clang-format off
	if (co_nmt_boot_dn(boot, 0x1f51, 0x01, CO_DEFTYPE_UNSIGNED8,
			&(co_unsigned8_t){ 1 }) == -1)
		// clang-format on
		return co_nmt_boot_abort_state;

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_start_prog_on_dn_con(co_nmt_boot_t *boot, co_unsigned32_t ac)
{
#if LELY_NO_STDIO
	(void)boot;
#else
	assert(boot);
#endif

	if (ac) {
#if !LELY_NO_STDIO
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" received on download request of sub-object 1F51:01 (Program control) to node %02X: %s",
				ac, boot->id, co_sdo_ac2str(ac));
#endif
		return co_nmt_boot_abort_state;
	}

	return co_nmt_boot_wait_prog_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_wait_prog_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	// Wait for a while before checking the program control.
	can_timer_timeout(
			boot->timer, boot->net, LELY_CO_NMT_BOOT_CHECK_TIMEOUT);

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_wait_prog_on_time(co_nmt_boot_t *boot, const struct timespec *tp)
{
	assert(boot);
	(void)tp;

	// The 'start program' step may take some time to complete, causing an
	// immediate SDO upload request to generate a timeout. Start the first
	// attempt by simulating a timeout.
	boot->retry = LELY_CO_NMT_BOOT_SDO_RETRY + 1;
	return co_nmt_boot_wait_prog_on_up_con(
			boot, CO_SDO_AC_TIMEOUT, NULL, 0);

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_wait_prog_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

	// Retry the SDO request on timeout (this includes the first attempt).
	if (ac == CO_SDO_AC_TIMEOUT && boot->retry--) {
		// Read the program control of the slave (sub-object 1F51:01).
		if (co_nmt_boot_up(boot, 0x1f51, 0x01) == -1)
			return co_nmt_boot_abort_state;
		return NULL;
	} else if (ac) {
#if !LELY_NO_STDIO
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" received on upload request of sub-object 1F51:01 (Program control) to node %02X: %s",
				ac, boot->id, co_sdo_ac2str(ac));
#endif
		return co_nmt_boot_abort_state;
	}

	// If the program control differs from 'Program started', try again.
	co_unsigned8_t val = 0;
	const uint_least8_t *const begin = ptr;
	const uint_least8_t *const end = begin != NULL ? begin + n : NULL;
	// clang-format off
	if (!co_val_read(CO_DEFTYPE_UNSIGNED8, &val, begin, end) || val != 1)
		// clang-format on
		return co_nmt_boot_wait_prog_state;

	return co_nmt_boot_chk_device_type_state;
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

	// The configuration check may follow an NMT 'reset communication'
	// command (if the 'check software version' step was skipped), in which
	// case we may have to give the slave some time to complete the state
	// change. Start the first SDO request by simulating a timeout.
	boot->retry = LELY_CO_NMT_BOOT_SDO_RETRY + 1;
	return co_nmt_boot_chk_cfg_date_on_up_con(
			boot, CO_SDO_AC_TIMEOUT, NULL, 0);
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_cfg_date_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

	// Retry the SDO request on timeout (this includes the first attempt).
	if (ac == CO_SDO_AC_TIMEOUT && boot->retry--) {
		// Read the configuration date of the slave (sub-object
		// 1020:01).
		if (co_nmt_boot_up(boot, 0x1020, 0x01) == -1)
			return co_nmt_boot_abort_state;
		return NULL;
#if !LELY_NO_STDIO
	} else if (ac) {
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" received on upload request of sub-object 1020:01 (Configuration date) to node %02X: %s",
				ac, boot->id, co_sdo_ac2str(ac));
#endif
	}

	// If the configuration date does not match the expected value, skip
	// checking the time and proceed to 'update configuration'.
	if (ac || !co_nmt_boot_chk(boot, 0x1f26, boot->id, ptr, n))
		return co_nmt_boot_up_cfg_state;

	// Read the configuration time of the slave (sub-object 1020:02).
	if (co_nmt_boot_up(boot, 0x1020, 0x02) == -1)
		return co_nmt_boot_abort_state;

	return co_nmt_boot_chk_cfg_time_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_chk_cfg_time_on_up_con(co_nmt_boot_t *boot, co_unsigned32_t ac,
		const void *ptr, size_t n)
{
	assert(boot);

#if !LELY_NO_STDIO
	if (ac)
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" received on upload request of sub-object 1020:02 (Configuration time) to node %02X: %s",
				ac, boot->id, co_sdo_ac2str(ac));
#endif

	// If the configuration time does not match the expected value, proceed
	// to 'update configuration'.
	if (ac || !co_nmt_boot_chk(boot, 0x1f27, boot->id, ptr, n))
		return co_nmt_boot_up_cfg_state;

	return co_nmt_boot_ec_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_up_cfg_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	boot->es = 'J';

	// clang-format off
	if (co_nmt_cfg_req(boot->nmt, boot->id, boot->timeout,
			&co_nmt_boot_cfg_con, boot) == -1)
		// clang-format on
		return co_nmt_boot_abort_state;

	return NULL;
}

static co_nmt_boot_state_t *
co_nmt_boot_up_cfg_on_cfg_con(co_nmt_boot_t *boot, co_unsigned32_t ac)
{
#if LELY_NO_STDIO
	(void)boot;
#else
	assert(boot);
#endif

	if (ac) {
#if !LELY_NO_STDIO
		diag(DIAG_ERROR, 0,
				"SDO abort code %08" PRIX32
				" received while updating the configuration of node %02X: %s",
				ac, boot->id, co_sdo_ac2str(ac));
#endif
		return co_nmt_boot_abort_state;
	}

	return co_nmt_boot_ec_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_ec_on_enter(co_nmt_boot_t *boot)
{
	assert(boot);

	if (boot->ms) {
		boot->es = 'K';
		// Start the CAN frame receiver for heartbeat messages.
		can_recv_start(boot->recv, boot->net, CO_NMT_EC_CANID(boot->id),
				0);
		// Wait for the first heartbeat indication.
		can_timer_timeout(boot->timer, boot->net, boot->ms);
		return NULL;
#if !LELY_NO_CO_NG
	} else if (boot->assignment & 0x01) {
		// If the guard time is non-zero, start node guarding by sending
		// the first RTR, but do not wait for the response.
		co_unsigned16_t gt = (boot->assignment >> 16) & 0xffff;
		if (gt)
			co_nmt_boot_send_rtr(boot);
#endif
	}

	boot->es = 0;
	return co_nmt_boot_abort_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_ec_on_recv(co_nmt_boot_t *boot, const struct can_msg *msg)
{
	assert(boot);
	assert(msg);
	assert(msg->id == (uint_least32_t)CO_NMT_EC_CANID(boot->id));

	if (msg->len >= 1) {
		co_unsigned8_t st = msg->data[0];
		// Do not consider a boot-up message to be a heartbeat message.
		if (st == CO_NMT_ST_BOOTUP)
			return NULL;
		boot->st = st;
		boot->es = 0;
	}

	return co_nmt_boot_abort_state;
}

static co_nmt_boot_state_t *
co_nmt_boot_ec_on_time(co_nmt_boot_t *boot, const struct timespec *tp)
{
	(void)boot;
	(void)tp;

	return co_nmt_boot_abort_state;
}

static int
co_nmt_boot_dn(co_nmt_boot_t *boot, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned16_t type, const void *val)
{
	assert(boot);

	return co_csdo_dn_val_req(boot->sdo, idx, subidx, type, val,
			&co_nmt_boot_dn_con, boot);
}

static int
co_nmt_boot_up(co_nmt_boot_t *boot, co_unsigned16_t idx, co_unsigned8_t subidx)
{
	assert(boot);

	return co_csdo_up_req(
			boot->sdo, idx, subidx, &co_nmt_boot_up_con, boot);
}

static int
co_nmt_boot_chk(co_nmt_boot_t *boot, co_unsigned16_t idx, co_unsigned8_t subidx,
		const void *ptr, size_t n)
{
	assert(boot);

	co_sub_t *sub = co_dev_find_sub(boot->dev, idx, subidx);
	if (!sub)
		return 0;
	co_unsigned16_t type = co_sub_get_type(sub);
	assert(!co_type_is_array(type));

	union co_val val;
	if (!co_val_read(type, &val, ptr, (const uint_least8_t *)ptr + n))
		return 0;

	return !co_val_cmp(type, &val, co_sub_get_val(sub));
}

#if !LELY_NO_CO_NG
static int
co_nmt_boot_send_rtr(co_nmt_boot_t *boot)
{
	assert(boot);

	struct can_msg msg = CAN_MSG_INIT;
	msg.id = CO_NMT_EC_CANID(boot->id);
	msg.flags |= CAN_FLAG_RTR;

	return can_net_send(boot->net, &msg);
}
#endif

#endif // !LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT
