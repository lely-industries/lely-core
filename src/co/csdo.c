/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the Client-SDO functions.
 *
 * @see lely/co/csdo.h, src/sdo.h
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

#if !LELY_NO_CO_CSDO

#include "sdo.h"
#include <lely/co/crc.h>
#include <lely/co/csdo.h>
#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/val.h>
#include <lely/util/endian.h>
#include <lely/util/errnum.h>

#include <assert.h>
#include <stdlib.h>

#if LELY_NO_MALLOC
#ifndef CO_CSDO_MEMBUF_SIZE
/**
 * The default size (in bytes) of a Client-SDO memory buffer in the absence of
 * dynamic memory allocation. The default size is large enough to accomodate all
 * basic data types.
 */
#define CO_CSDO_MEMBUF_SIZE 8
#endif
#endif

struct __co_csdo_state;
/// An opaque CANopen Client-SDO state type.
typedef const struct __co_csdo_state co_csdo_state_t;

/// The state of a concise DCF download request.
struct co_csdo_dn_dcf {
	/// The number of remaining entries in the concise DCF.
	co_unsigned32_t n;
	/// A pointer to the next byte in the concise DCF.
	const uint_least8_t *begin;
	/// A pointer to one past the last byte in the concise DCF.
	const uint_least8_t *end;
	/// A pointer to the download confirmation function.
	co_csdo_dn_con_t *con;
	/// A pointer to user-specified data for #con.
	void *data;
};

/// A CANopen Client-SDO.
struct __co_csdo {
	/// A pointer to a CAN network interface.
	can_net_t *net;
	/// A pointer to a CANopen device.
	co_dev_t *dev;
	/// The SDO number.
	co_unsigned8_t num;
	/// The SDO parameter record.
	struct co_sdo_par par;
	/// A pointer to the CAN frame receiver.
	can_recv_t *recv;
	/// The SDO timeout (in milliseconds).
	int timeout;
	/// A pointer to the CAN timer.
	can_timer_t *timer;
	/// A pointer to the current state.
	co_csdo_state_t *state;
	/// The current abort code.
	co_unsigned32_t ac;
	/// The current object index.
	co_unsigned16_t idx;
	/// The current object sub-index.
	co_unsigned8_t subidx;
	/// The data set size (in bytes).
	co_unsigned32_t size;
	/// The current value of the toggle bit.
	co_unsigned8_t toggle;
	/// The number of segments per block.
	co_unsigned8_t blksize;
	/// The sequence number of the last successfully received segment.
	co_unsigned8_t ackseq;
	/// A flag indicating whether a CRC should be generated.
	unsigned crc : 1;
	/// The memory buffer used for download requests.
	struct membuf dn_buf;
	/// A pointer to the memory buffer used for upload requests.
	struct membuf *up_buf;
	/**
	 * The memory buffer used for storing serialized values in the absence
	 * of a user-specified buffer.
	 */
	struct membuf buf;
#if LELY_NO_MALLOC
	/**
	 * The static memory buffer used by #buf in the absence of dynamic
	 * memory allocation.
	 */
	char begin[CO_CSDO_MEMBUF_SIZE];
#endif
	/// A pointer to the download confirmation function.
	co_csdo_dn_con_t *dn_con;
	/// A pointer to user-specified data for #dn_con.
	void *dn_con_data;
	/// A pointer to the download progress indication function.
	co_csdo_ind_t *dn_ind;
	/// A pointer to user-specified data for #dn_ind.
	void *dn_ind_data;
	/// A pointer to the upload confirmation function.
	co_csdo_up_con_t *up_con;
	/// A pointer to user-specified data for #up_con.
	void *up_con_data;
	/// A pointer to the upload progress indication function.
	co_csdo_ind_t *up_ind;
	/// A pointer to user-specified data for #up_ind.
	void *up_ind_data;
	/// The state of the concise DCF download request.
	struct co_csdo_dn_dcf dn_dcf;
};

/**
 * Updates and (de)activates a Client-SDO service. This function is invoked when
 * one of the SDO client parameters (objects 1280..12FF) is updated.
 *
 * @returns 0 on success, or -1 on error.
 */
static int co_csdo_update(co_csdo_t *sdo);

/**
 * The download indication function for (all sub-objects of) CANopen objects
 * 1280..12FF (SDO client parameter).
 *
 * @see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1280_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);

/**
 * The CAN receive callback function for a Client-SDO service.
 *
 * @see can_recv_func_t
 */
static int co_csdo_recv(const struct can_msg *msg, void *data);

/**
 * The CAN timer callback function for a Client-SDO service.
 *
 * @see can_timer_func_t
 */
static int co_csdo_timer(const struct timespec *tp, void *data);

/**
 * Enters the specified state of a Client-SDO service and invokes the exit and
 * entry functions.
 */
static inline void co_csdo_enter(co_csdo_t *sdo, co_csdo_state_t *next);

/**
 * Invokes the 'abort' transition function of the current state of a Client-SDO
 * service.
 *
 * @param sdo a pointer to a Client-SDO service.
 * @param ac  the abort code.
 */
static inline void co_csdo_emit_abort(co_csdo_t *sdo, co_unsigned32_t ac);

/**
 * Invokes the 'timeout' transition function of the current state of a
 * Client-SDO service.
 *
 * @param sdo a pointer to a Client-SDO service.
 * @param tp  a pointer to the current time.
 */
static inline void co_csdo_emit_time(co_csdo_t *sdo, const struct timespec *tp);

/**
 * Invokes the 'CAN frame received' transition function of the current state of
 * a Client-SDO service.
 *
 * @param sdo a pointer to a Client-SDO service.
 * @param msg a pointer to the received CAN frame.
 */
static inline void co_csdo_emit_recv(co_csdo_t *sdo, const struct can_msg *msg);

/// A CANopen Client-SDO state.
struct __co_csdo_state {
	/// A pointer to the function invoked when a new state is entered.
	co_csdo_state_t *(*on_enter)(co_csdo_t *sdo);
	/**
	 * A pointer to the transition function invoked when an abort code has
	 * been received.
	 *
	 * @param sdo a pointer to a Client-SDO service.
	 * @param ac  the abort code.
	 *
	 * @returns a pointer to the next state.
	 */
	co_csdo_state_t *(*on_abort)(co_csdo_t *sdo, co_unsigned32_t ac);
	/**
	 * A pointer to the transition function invoked when a timeout occurs.
	 *
	 * @param sdo a pointer to a Client-SDO service.
	 * @param tp  a pointer to the current time.
	 *
	 * @returns a pointer to the next state.
	 */
	co_csdo_state_t *(*on_time)(co_csdo_t *sdo, const struct timespec *tp);
	/**
	 * A pointer to the transition function invoked when a CAN frame has
	 * been received.
	 *
	 * @param sdo a pointer to a Client-SDO service.
	 * @param msg a pointer to the received CAN frame.
	 *
	 * @returns a pointer to the next state.
	 */
	co_csdo_state_t *(*on_recv)(co_csdo_t *sdo, const struct can_msg *msg);
	/// A pointer to the function invoked when the current state is left.
	void (*on_leave)(co_csdo_t *sdo);
};

#define LELY_CO_DEFINE_STATE(name, ...) \
	static co_csdo_state_t *const name = &(co_csdo_state_t){ __VA_ARGS__ };

/// The 'abort' transition function of the 'stopped' state.
static co_csdo_state_t *co_csdo_stopped_on_abort(
		co_csdo_t *sdo, co_unsigned32_t ac);

/// The 'stopped' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_csdo_stopped_state,
	.on_abort = &co_csdo_stopped_on_abort
)
// clang-format on

/// The 'abort' transition function of the 'waiting' state.
static co_csdo_state_t *co_csdo_wait_on_abort(
		co_csdo_t *sdo, co_unsigned32_t ac);

/// The 'CAN frame received' transition function of the 'waiting' state.
static co_csdo_state_t *co_csdo_wait_on_recv(
		co_csdo_t *sdo, const struct can_msg *msg);

/// The 'waiting' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_csdo_wait_state,
	.on_abort = &co_csdo_wait_on_abort,
	.on_recv = &co_csdo_wait_on_recv
)
// clang-format on

/// The entry function of the 'abort transfer' state.
static co_csdo_state_t *co_csdo_abort_on_enter(co_csdo_t *sdo);

/// The exit function of the 'abort transfer' state.
static void co_csdo_abort_on_leave(co_csdo_t *sdo);

/// The 'abort transfer' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_csdo_abort_state,
	.on_enter = &co_csdo_abort_on_enter,
	.on_leave = &co_csdo_abort_on_leave
)
// clang-format on

/// The 'abort' transition function of the 'download initiate' state.
static co_csdo_state_t *co_csdo_dn_ini_on_abort(
		co_csdo_t *sdo, co_unsigned32_t ac);

/// The 'timeout' transition function of the 'download initiate' state.
static co_csdo_state_t *co_csdo_dn_ini_on_time(
		co_csdo_t *sdo, const struct timespec *tp);

/**
 * The 'CAN frame received' transition function of the 'download initiate'
 * state.
 */
static co_csdo_state_t *co_csdo_dn_ini_on_recv(
		co_csdo_t *sdo, const struct can_msg *msg);

/// The 'download initiate' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_csdo_dn_ini_state,
	.on_abort = &co_csdo_dn_ini_on_abort,
	.on_time = &co_csdo_dn_ini_on_time,
	.on_recv = &co_csdo_dn_ini_on_recv
)
// clang-format on

/// The entry function of the 'download segment' state.
static co_csdo_state_t *co_csdo_dn_seg_on_enter(co_csdo_t *sdo);

/// The 'abort' transition function of the 'download segment' state.
static co_csdo_state_t *co_csdo_dn_seg_on_abort(
		co_csdo_t *sdo, co_unsigned32_t ac);

/// The 'timeout' transition function of the 'download segment' state.
static co_csdo_state_t *co_csdo_dn_seg_on_time(
		co_csdo_t *sdo, const struct timespec *tp);

/**
 * The 'CAN frame received' transition function of the 'download segment' state.
 */
static co_csdo_state_t *co_csdo_dn_seg_on_recv(
		co_csdo_t *sdo, const struct can_msg *msg);

/// The 'download segment' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_csdo_dn_seg_state,
	.on_enter = &co_csdo_dn_seg_on_enter,
	.on_abort = &co_csdo_dn_seg_on_abort,
	.on_time = &co_csdo_dn_seg_on_time,
	.on_recv = &co_csdo_dn_seg_on_recv
)
// clang-format on

/// The 'abort' transition function of the 'upload initiate' state.
static co_csdo_state_t *co_csdo_up_ini_on_abort(
		co_csdo_t *sdo, co_unsigned32_t ac);

/// The 'timeout' transition function of the 'upload initiate' state.
static co_csdo_state_t *co_csdo_up_ini_on_time(
		co_csdo_t *sdo, const struct timespec *tp);

/// The 'CAN frame received' transition function of the 'upload initiate' state.
static co_csdo_state_t *co_csdo_up_ini_on_recv(
		co_csdo_t *sdo, const struct can_msg *msg);

/// The 'upload initiate' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_csdo_up_ini_state,
	.on_abort = &co_csdo_up_ini_on_abort,
	.on_time = &co_csdo_up_ini_on_time,
	.on_recv = &co_csdo_up_ini_on_recv
)
// clang-format on

/// The 'abort' transition function of the 'upload segment' state.
static co_csdo_state_t *co_csdo_up_seg_on_abort(
		co_csdo_t *sdo, co_unsigned32_t ac);

/// The 'timeout' transition function of the 'upload segment' state.
static co_csdo_state_t *co_csdo_up_seg_on_time(
		co_csdo_t *sdo, const struct timespec *tp);

/// The 'CAN frame received' transition function of the 'upload segment' state.
static co_csdo_state_t *co_csdo_up_seg_on_recv(
		co_csdo_t *sdo, const struct can_msg *msg);

/// The 'upload segment' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_csdo_up_seg_state,
	.on_abort = &co_csdo_up_seg_on_abort,
	.on_time = &co_csdo_up_seg_on_time,
	.on_recv = &co_csdo_up_seg_on_recv
)
// clang-format on

/// The 'abort' transition function of the 'block download initiate' state.
static co_csdo_state_t *co_csdo_blk_dn_ini_on_abort(
		co_csdo_t *sdo, co_unsigned32_t ac);

/// The 'timeout' transition function of the 'block download initiate' state.
static co_csdo_state_t *co_csdo_blk_dn_ini_on_time(
		co_csdo_t *sdo, const struct timespec *tp);

/**
 * The 'CAN frame received' transition function of the 'block download initiate'
 * state.
 */
static co_csdo_state_t *co_csdo_blk_dn_ini_on_recv(
		co_csdo_t *sdo, const struct can_msg *msg);

/// The 'block download initiate' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_csdo_blk_dn_ini_state,
	.on_abort = &co_csdo_blk_dn_ini_on_abort,
	.on_time = &co_csdo_blk_dn_ini_on_time,
	.on_recv = &co_csdo_blk_dn_ini_on_recv
)
// clang-format on

/// The entry function of the 'block download sub-block' state.
static co_csdo_state_t *co_csdo_blk_dn_sub_on_enter(co_csdo_t *sdo);

/// The 'abort' transition function of the 'block download sub-block' state.
static co_csdo_state_t *co_csdo_blk_dn_sub_on_abort(
		co_csdo_t *sdo, co_unsigned32_t ac);

/// The 'timeout' transition function of the 'block download sub-block' state.
static co_csdo_state_t *co_csdo_blk_dn_sub_on_time(
		co_csdo_t *sdo, const struct timespec *tp);

/**
 * The 'CAN frame received' transition function of the 'block download
 * sub-block' state.
 */
static co_csdo_state_t *co_csdo_blk_dn_sub_on_recv(
		co_csdo_t *sdo, const struct can_msg *msg);

/// The 'block download sub-block' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_csdo_blk_dn_sub_state,
	.on_enter = &co_csdo_blk_dn_sub_on_enter,
	.on_abort = &co_csdo_blk_dn_sub_on_abort,
	.on_time = &co_csdo_blk_dn_sub_on_time,
	.on_recv = &co_csdo_blk_dn_sub_on_recv
)
// clang-format on

/// The 'abort' transition function of the 'block download end' state.
static co_csdo_state_t *co_csdo_blk_dn_end_on_abort(
		co_csdo_t *sdo, co_unsigned32_t ac);

/// The 'timeout' transition function of the 'block download end' state.
static co_csdo_state_t *co_csdo_blk_dn_end_on_time(
		co_csdo_t *sdo, const struct timespec *tp);

/**
 * The 'CAN frame received' transition function of the 'block download end'
 * state.
 */
static co_csdo_state_t *co_csdo_blk_dn_end_on_recv(
		co_csdo_t *sdo, const struct can_msg *msg);

/// The 'block download end' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_csdo_blk_dn_end_state,
	.on_abort = &co_csdo_blk_dn_end_on_abort,
	.on_time = &co_csdo_blk_dn_end_on_time,
	.on_recv = &co_csdo_blk_dn_end_on_recv
)
// clang-format on

/// The 'abort' transition function of the 'block upload initiate' state.
static co_csdo_state_t *co_csdo_blk_up_ini_on_abort(
		co_csdo_t *sdo, co_unsigned32_t ac);

/// The 'timeout' transition function of the 'block upload initiate' state.
static co_csdo_state_t *co_csdo_blk_up_ini_on_time(
		co_csdo_t *sdo, const struct timespec *tp);

/**
 * The 'CAN frame received' transition function of the 'block upload initiate'
 * state.
 */
static co_csdo_state_t *co_csdo_blk_up_ini_on_recv(
		co_csdo_t *sdo, const struct can_msg *msg);

/// The 'block upload initiate' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_csdo_blk_up_ini_state,
	.on_abort = &co_csdo_blk_up_ini_on_abort,
	.on_time = &co_csdo_blk_up_ini_on_time,
	.on_recv = &co_csdo_blk_up_ini_on_recv
)
// clang-format on

/// The 'abort' transition function of the 'block upload sub-block' state.
static co_csdo_state_t *co_csdo_blk_up_sub_on_abort(
		co_csdo_t *sdo, co_unsigned32_t ac);

/// The 'timeout' transition function of the 'block upload sub-block' state.
static co_csdo_state_t *co_csdo_blk_up_sub_on_time(
		co_csdo_t *sdo, const struct timespec *tp);

/**
 * The 'CAN frame received' transition function of the 'block upload sub-block'
 * state.
 */
static co_csdo_state_t *co_csdo_blk_up_sub_on_recv(
		co_csdo_t *sdo, const struct can_msg *msg);

/// The 'block upload sub-block' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_csdo_blk_up_sub_state,
	.on_abort = &co_csdo_blk_up_sub_on_abort,
	.on_time = &co_csdo_blk_up_sub_on_time,
	.on_recv = &co_csdo_blk_up_sub_on_recv
)
// clang-format on

/// The 'abort' transition function of the 'block upload end' state.
static co_csdo_state_t *co_csdo_blk_up_end_on_abort(
		co_csdo_t *sdo, co_unsigned32_t ac);

/// The 'timeout' transition function of the 'block upload end' state.
static co_csdo_state_t *co_csdo_blk_up_end_on_time(
		co_csdo_t *sdo, const struct timespec *tp);

/**
 * The 'CAN frame received' transition function of the 'block upload end' state.
 */
static co_csdo_state_t *co_csdo_blk_up_end_on_recv(
		co_csdo_t *sdo, const struct can_msg *msg);

/// The 'block upload end' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_csdo_blk_up_end_state,
	.on_abort = &co_csdo_blk_up_end_on_abort,
	.on_time = &co_csdo_blk_up_end_on_time,
	.on_recv = &co_csdo_blk_up_end_on_recv
)
// clang-format on

#undef LELY_CO_DEFINE_STATE

/**
 * Processes an abort transfer indication by aborting any ongoing transfer of a
 * Client-SDO and returning it to the waiting state after notifying the user.
 *
 * @returns #co_csdo_wait_state
 */
static co_csdo_state_t *co_csdo_abort_ind(co_csdo_t *sdo, co_unsigned32_t ac);

/**
 * Sends an abort transfer request and aborts any ongoing transfer by invoking
 * co_csdo_abort_ind().
 *
 * @param sdo a pointer to a Client-SDO service.
 * @param ac  the SDO abort code.
 *
 * @returns #co_csdo_wait_state
 *
 * @see co_csdo_send_abort()
 */
static co_csdo_state_t *co_csdo_abort_res(co_csdo_t *sdo, co_unsigned32_t ac);

/**
 * Processes a download request from a Client-SDO by checking and updating the
 * state and copying the value to the internal buffer.
 *
 * @returns 0 on success, or -1 on error.
 *
 * @see co_csdo_dn_req(), co_csdo_blk_dn_req()
 */
static int co_csdo_dn_ind(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, const void *ptr, size_t n,
		co_csdo_dn_con_t *con, void *data);

/**
 * Processes an upload request from a Client-SDO by checking and updating the
 * state.
 *
 * @returns 0 on success, or -1 on error.
 *
 * @see co_csdo_up_req(), co_csdo_blk_up_req()
 */
static int co_csdo_up_ind(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, struct membuf *buf,
		co_csdo_up_con_t *con, void *data);

/**
 * Sends an abort transfer request.
 *
 * @param sdo a pointer to a Server-SDO service.
 * @param ac  the SDO abort code.
 */
static void co_csdo_send_abort(co_csdo_t *sdo, co_unsigned32_t ac);

/// Sends a Client-SDO 'download initiate' (expedited) request.
static void co_csdo_send_dn_exp_req(co_csdo_t *sdo);

/// Sends a Client-SDO 'download initiate' request.
static void co_csdo_send_dn_ini_req(co_csdo_t *sdo);

/**
 * Sends a Client-SDO 'download segment' request.
 *
 * @param sdo  a pointer to a Client-SDO service.
 * @param n    the number of bytes to be sent in this segment (in the range
 *             [1..7]).
 * @param last a flag indicating whether this is the last segment.
 */
static void co_csdo_send_dn_seg_req(
		co_csdo_t *sdo, co_unsigned32_t n, int last);

/// Sends a Client-SDO 'upload initiate' request.
static void co_csdo_send_up_ini_req(co_csdo_t *sdo);

/// Sends a Client-SDO 'upload segment' request.
static void co_csdo_send_up_seg_req(co_csdo_t *sdo);

/// Sends a Client-SDO 'block download initiate' request.
static void co_csdo_send_blk_dn_ini_req(co_csdo_t *sdo);

/**
 * Sends a Client-SDO 'block download sub-block' request.
 *
 * @param sdo   a pointer to a Client-SDO service.
 * @param seqno the sequence number (in the range [1..127]).
 */
static void co_csdo_send_blk_dn_sub_req(co_csdo_t *sdo, co_unsigned8_t seqno);

/// Sends a Client-SDO 'block download end' request.
static void co_csdo_send_blk_dn_end_req(co_csdo_t *sdo);

/**
 * Sends a Client-SDO 'block upload initiate' request.
 *
 * @param sdo a pointer to a Client-SDO service.
 * @param pst the protocol switch threshold.
 */
static void co_csdo_send_blk_up_ini_req(co_csdo_t *sdo, co_unsigned8_t pst);

/// Sends a Client-SDO 'start upload' request.
static void co_csdo_send_start_up_req(co_csdo_t *sdo);

/// Sends a Client-SDO 'block upload sub-block' response.
static void co_csdo_send_blk_up_sub_res(co_csdo_t *sdo);

/// Sends a Client-SDO 'block upload end' response.
static void co_csdo_send_blk_up_end_res(co_csdo_t *sdo);

/**
 * Initializes a Client-SDO download/upload initiate request CAN frame.
 *
 * @param sdo a pointer to a Client-SDO service.
 * @param msg a pointer to the CAN frame to be initialized.
 * @param cs  the command specifier.
 */
static void co_csdo_init_ini_req(
		co_csdo_t *sdo, struct can_msg *msg, co_unsigned8_t cs);

/**
 * Initializes a Client-SDO download/upload segment request CAN frame.
 *
 * @param sdo a pointer to a Client-SDO service.
 * @param msg a pointer to the CAN frame to be initialized.
 * @param cs  the command specifier.
 */
static void co_csdo_init_seg_req(
		co_csdo_t *sdo, struct can_msg *msg, co_unsigned8_t cs);

/**
 * The confirmation function of a single SDO download request during a concise
 * DCF download.
 *
 * @see co_csdo_dn_dcf_req()
 */
static void co_csdo_dn_dcf_dn_con(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac, void *data);

int
co_dev_dn_req(co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx,
		const void *ptr, size_t n, co_csdo_dn_con_t *con, void *data)
{
	assert(dev);

	int errc = get_errc();
	struct co_sdo_req req = CO_SDO_REQ_INIT;

	co_unsigned32_t ac = 0;

	co_obj_t *obj = co_dev_find_obj(dev, idx);
	if (!obj) {
		ac = CO_SDO_AC_NO_OBJ;
		goto done;
	}

	co_sub_t *sub = co_obj_find_sub(obj, subidx);
	if (!sub) {
		ac = CO_SDO_AC_NO_SUB;
		goto done;
	}

	if (co_sdo_req_up(&req, ptr, n, &ac) == -1)
		goto done;

	ac = co_sub_dn_ind(sub, &req);

done:
	if (con)
		con(NULL, idx, subidx, ac, data);

	co_sdo_req_fini(&req);
	set_errc(errc);
	return 0;
}

int
co_dev_dn_val_req(co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned16_t type, const void *val, co_csdo_dn_con_t *con,
		void *data)
{
	assert(dev);

	int errc = get_errc();
	struct co_sdo_req req = CO_SDO_REQ_INIT;

	co_unsigned32_t ac = 0;

	co_obj_t *obj = co_dev_find_obj(dev, idx);
	if (!obj) {
		ac = CO_SDO_AC_NO_OBJ;
		goto done;
	}

	co_sub_t *sub = co_obj_find_sub(obj, subidx);
	if (!sub) {
		ac = CO_SDO_AC_NO_SUB;
		goto done;
	}

	if (co_sdo_req_up_val(&req, type, val, &ac) == -1)
		goto done;

	ac = co_sub_dn_ind(sub, &req);

done:
	if (con)
		con(NULL, idx, subidx, ac, data);

	co_sdo_req_fini(&req);
	set_errc(errc);
	return 0;
}

int
co_dev_dn_dcf_req(co_dev_t *dev, const uint_least8_t *begin,
		const uint_least8_t *end, co_csdo_dn_con_t *con, void *data)
{
	assert(dev);
	assert(begin);
	assert(end >= begin);

	int errc = get_errc();
	struct co_sdo_req req = CO_SDO_REQ_INIT;

	co_unsigned16_t idx = 0;
	co_unsigned8_t subidx = 0;
	co_unsigned32_t ac = 0;

	// Read the total number of sub-indices.
	co_unsigned32_t n;
	if (co_val_read(CO_DEFTYPE_UNSIGNED32, &n, begin, end) != 4) {
		ac = CO_SDO_AC_TYPE_LEN_LO;
		goto done;
	}
	begin += 4;

	for (size_t i = 0; i < n && !ac; i++) {
		idx = 0;
		subidx = 0;
		ac = CO_SDO_AC_TYPE_LEN_LO;
		// Read the object index.
		if (co_val_read(CO_DEFTYPE_UNSIGNED16, &idx, begin, end) != 2)
			break;
		begin += 2;
		// Read the object sub-index.
		if (co_val_read(CO_DEFTYPE_UNSIGNED8, &subidx, begin, end) != 1)
			break;
		begin += 1;
		// Read the value size (in bytes).
		co_unsigned32_t size;
		if (co_val_read(CO_DEFTYPE_UNSIGNED32, &size, begin, end) != 4)
			break;
		begin += 4;
		if (end - begin < (ptrdiff_t)size)
			break;
		co_obj_t *obj = co_dev_find_obj(dev, idx);

		if (!obj) {
			ac = CO_SDO_AC_NO_OBJ;
			break;
		}
		co_sub_t *sub = co_obj_find_sub(obj, subidx);
		if (!sub) {
			ac = CO_SDO_AC_NO_SUB;
			break;
		}

		// Write the value to the object dictionary.
		co_sdo_req_clear(&req);
		// cppcheck-suppress redundantAssignment
		ac = 0;
		if (!co_sdo_req_up(&req, begin, size, &ac))
			ac = co_sub_dn_ind(sub, &req);

		begin += size;
	}

done:
	if (con)
		con(NULL, idx, subidx, ac, data);

	co_sdo_req_fini(&req);
	set_errc(errc);
	return 0;
}

int
co_dev_up_req(const co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_csdo_up_con_t *con, void *data)
{
	assert(dev);

	int errc = get_errc();
	struct membuf buf = MEMBUF_INIT;
	co_unsigned32_t ac = 0;

	const co_obj_t *obj = co_dev_find_obj(dev, idx);
	if (!obj) {
		ac = CO_SDO_AC_NO_OBJ;
		goto done;
	}

	const co_sub_t *sub = co_obj_find_sub(obj, subidx);
	if (!sub) {
		ac = CO_SDO_AC_NO_SUB;
		goto done;
	}

	// If the object is an array, check whether the element exists.
	if (co_obj_get_code(obj) == CO_OBJECT_ARRAY
			&& subidx > co_obj_get_val_u8(obj, 0)) {
		ac = CO_SDO_AC_NO_DATA;
		goto done;
	}

	struct co_sdo_req req = CO_SDO_REQ_INIT;

	ac = co_sub_up_ind(sub, &req);
	if (!ac && req.size && !membuf_reserve(&buf, req.size))
		ac = CO_SDO_AC_NO_MEM;

	while (!ac && membuf_size(&buf) < req.size) {
		membuf_write(&buf, req.buf, req.nbyte);
		if (!co_sdo_req_last(&req))
			ac = co_sub_up_ind(sub, &req);
	}

	co_sdo_req_fini(&req);

done:
	if (con)
		con(NULL, idx, subidx, ac, ac ? NULL : buf.begin,
				ac ? 0 : membuf_size(&buf), data);

	membuf_fini(&buf);
	set_errc(errc);
	return 0;
}

void *
__co_csdo_alloc(void)
{
	void *ptr = malloc(sizeof(struct __co_csdo));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
__co_csdo_free(void *ptr)
{
	free(ptr);
}

struct __co_csdo *
__co_csdo_init(struct __co_csdo *sdo, can_net_t *net, co_dev_t *dev,
		co_unsigned8_t num)
{
	assert(sdo);
	assert(net);

	int errc = 0;

	if (!num || num > (dev ? CO_NUM_SDOS : CO_NUM_NODES)) {
		errc = errnum2c(ERRNUM_INVAL);
		goto error_param;
	}

	// Find the SDO client parameter in the object dictionary.
	co_obj_t *obj_1280 =
			dev ? co_dev_find_obj(dev, 0x1280 + num - 1) : NULL;
	if (dev && !obj_1280) {
		errc = errnum2c(ERRNUM_INVAL);
		goto error_param;
	}

	sdo->net = net;
	sdo->dev = dev;
	sdo->num = num;

	// Initialize the SDO parameter record with the default values.
	sdo->par.n = 3;
	sdo->par.id = num;
	sdo->par.cobid_req = 0x600 + sdo->par.id;
	sdo->par.cobid_res = 0x580 + sdo->par.id;

	sdo->recv = can_recv_create();
	if (!sdo->recv) {
		errc = get_errc();
		goto error_create_recv;
	}
	can_recv_set_func(sdo->recv, &co_csdo_recv, sdo);

	sdo->timeout = 0;

	sdo->timer = can_timer_create();
	if (!sdo->timer) {
		errc = get_errc();
		goto error_create_timer;
	}
	can_timer_set_func(sdo->timer, &co_csdo_timer, sdo);

	sdo->state = co_csdo_stopped_state;

	sdo->ac = 0;
	sdo->idx = 0;
	sdo->subidx = 0;
	sdo->size = 0;

	sdo->toggle = 0;
	sdo->blksize = 0;
	sdo->ackseq = 0;
	sdo->crc = 0;

	membuf_init(&sdo->dn_buf, NULL, 0);
	sdo->up_buf = NULL;
#if LELY_NO_MALLOC
	membuf_init(&sdo->buf, sdo->begin, CO_CSDO_MEMBUF_SIZE);
	memset(sdo->begin, 0, CO_CSDO_MEMBUF_SIZE);
#else
	membuf_init(&sdo->buf, NULL, 0);
#endif

	sdo->dn_con = NULL;
	sdo->dn_con_data = NULL;

	sdo->dn_ind = NULL;
	sdo->dn_ind_data = NULL;

	sdo->up_con = NULL;
	sdo->up_con_data = NULL;

	sdo->up_ind = NULL;
	sdo->up_ind_data = NULL;

	sdo->dn_dcf = (struct co_csdo_dn_dcf){ 0 };

	if (co_csdo_start(sdo) == -1) {
		errc = get_errc();
		goto error_start;
	}

	return sdo;

	// co_csdo_stop(sdo);
error_start:
	can_timer_destroy(sdo->timer);
error_create_timer:
	can_recv_destroy(sdo->recv);
error_create_recv:
error_param:
	set_errc(errc);
	return NULL;
}

void
__co_csdo_fini(struct __co_csdo *sdo)
{
	assert(sdo);
	assert(sdo->num >= 1 && sdo->num <= CO_NUM_SDOS);

	co_csdo_stop(sdo);

	membuf_fini(&sdo->buf);

	can_timer_destroy(sdo->timer);
	can_recv_destroy(sdo->recv);
}

co_csdo_t *
co_csdo_create(can_net_t *net, co_dev_t *dev, co_unsigned8_t num)
{
	trace("creating Client-SDO %d", num);

	int errc = 0;

	co_csdo_t *sdo = __co_csdo_alloc();
	if (!sdo) {
		errc = get_errc();
		goto error_alloc_sdo;
	}

	if (!__co_csdo_init(sdo, net, dev, num)) {
		errc = get_errc();
		goto error_init_sdo;
	}

	return sdo;

error_init_sdo:
	__co_csdo_free(sdo);
error_alloc_sdo:
	set_errc(errc);
	return NULL;
}

void
co_csdo_destroy(co_csdo_t *csdo)
{
	if (csdo) {
		trace("destroying Client-SDO %d", csdo->num);
		__co_csdo_fini(csdo);
		__co_csdo_free(csdo);
	}
}

int
co_csdo_start(co_csdo_t *sdo)
{
	assert(sdo);

	if (!co_csdo_is_stopped(sdo))
		return 0;

	if (sdo->dev) {
		co_obj_t *obj_1280 = co_dev_find_obj(
				sdo->dev, 0x1280 + sdo->num - 1);
		// Copy the SDO parameter record.
		size_t size = co_obj_sizeof_val(obj_1280);
		memcpy(&sdo->par, co_obj_addressof_val(obj_1280),
				MIN(size, sizeof(sdo->par)));
		// Set the download indication function for the SDO parameter
		// record.
		co_obj_set_dn_ind(obj_1280, &co_1280_dn_ind, sdo);
	}

	co_csdo_enter(sdo, co_csdo_wait_state);

	if (co_csdo_update(sdo) == -1)
		goto error_update;

	return 0;

error_update:
	co_csdo_stop(sdo);
	return -1;
}

void
co_csdo_stop(co_csdo_t *sdo)
{
	assert(sdo);

	if (co_csdo_is_stopped(sdo))
		return;

	// Abort any ongoing transfer.
	co_csdo_abort_req(sdo, CO_SDO_AC_NO_SDO);

	can_timer_stop(sdo->timer);
	can_recv_stop(sdo->recv);

	co_obj_t *obj_1280 = NULL;
	if (sdo->dev) {
		obj_1280 = co_dev_find_obj(sdo->dev, 0x1280 + sdo->num - 1);
		// Remove the download indication functions for the SDO
		// parameter record.
		co_obj_set_dn_ind(obj_1280, NULL, NULL);
	}

	co_csdo_enter(sdo, co_csdo_stopped_state);
}

int
co_csdo_is_stopped(const co_csdo_t *sdo)
{
	assert(sdo);

	return sdo->state == co_csdo_stopped_state;
}

can_net_t *
co_csdo_get_net(const co_csdo_t *sdo)
{
	assert(sdo);

	return sdo->net;
}

co_dev_t *
co_csdo_get_dev(const co_csdo_t *sdo)
{
	assert(sdo);

	return sdo->dev;
}

co_unsigned8_t
co_csdo_get_num(const co_csdo_t *sdo)
{
	assert(sdo);

	return sdo->num;
}

const struct co_sdo_par *
co_csdo_get_par(const co_csdo_t *sdo)
{
	assert(sdo);

	return &sdo->par;
}

int
co_csdo_get_timeout(const co_csdo_t *sdo)
{
	assert(sdo);

	return sdo->timeout;
}

void
co_csdo_set_timeout(co_csdo_t *sdo, int timeout)
{
	assert(sdo);

	if (sdo->timeout && timeout <= 0)
		can_timer_stop(sdo->timer);

	sdo->timeout = MAX(0, timeout);
}

void
co_csdo_get_dn_ind(const co_csdo_t *sdo, co_csdo_ind_t **pind, void **pdata)
{
	assert(sdo);

	if (pind)
		*pind = sdo->dn_ind;
	if (pdata)
		*pdata = sdo->dn_ind_data;
}

void
co_csdo_set_dn_ind(co_csdo_t *sdo, co_csdo_ind_t *ind, void *data)
{
	assert(sdo);

	sdo->dn_ind = ind;
	sdo->dn_ind_data = data;
}

void
co_csdo_get_up_ind(const co_csdo_t *sdo, co_csdo_ind_t **pind, void **pdata)
{
	assert(sdo);

	if (pind)
		*pind = sdo->up_ind;
	if (pdata)
		*pdata = sdo->up_ind_data;
}

void
co_csdo_set_up_ind(co_csdo_t *sdo, co_csdo_ind_t *ind, void *data)
{
	assert(sdo);

	sdo->up_ind = ind;
	sdo->up_ind_data = data;
}

int
co_csdo_is_valid(const co_csdo_t *sdo)
{
	assert(sdo);

	int valid_req = !(sdo->par.cobid_req & CO_SDO_COBID_VALID);
	int valid_res = !(sdo->par.cobid_res & CO_SDO_COBID_VALID);
	return valid_req && valid_res;
}

int
co_csdo_is_idle(const co_csdo_t *sdo)
{
	assert(sdo);

	return sdo->state == co_csdo_wait_state;
}

void
co_csdo_abort_req(co_csdo_t *sdo, co_unsigned32_t ac)
{
	assert(sdo);

	co_csdo_emit_abort(sdo, ac);
}

int
co_csdo_dn_req(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		const void *ptr, size_t n, co_csdo_dn_con_t *con, void *data)
{
	assert(sdo);

	if (co_csdo_dn_ind(sdo, idx, subidx, ptr, n, con, data) == -1)
		return -1;

	trace("CSDO: %04X:%02X: initiate download", idx, subidx);

	if (sdo->timeout)
		can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);
	if (sdo->size && sdo->size <= 4)
		co_csdo_send_dn_exp_req(sdo);
	else
		co_csdo_send_dn_ini_req(sdo);
	co_csdo_enter(sdo, co_csdo_dn_ini_state);

	return 0;
}

int
co_csdo_dn_val_req(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned16_t type, const void *val, co_csdo_dn_con_t *con,
		void *data)
{
	assert(sdo);
	struct membuf *buf = &sdo->buf;

	// Obtain the size of the serialized value (which may be 0 for arrays).
	size_t n = co_val_write(type, val, NULL, NULL);
	if (!n && co_val_sizeof(type, val))
		return -1;

	membuf_clear(buf);
	if (!membuf_reserve(buf, n))
		return -1;
	void *ptr = membuf_alloc(buf, &n);

	if (co_val_write(type, val, ptr, (uint_least8_t *)ptr + n) != n)
		return -1;

	return co_csdo_dn_req(sdo, idx, subidx, ptr, n, con, data);
}

int
co_csdo_dn_dcf_req(co_csdo_t *sdo, const uint_least8_t *begin,
		const uint_least8_t *end, co_csdo_dn_con_t *con, void *data)
{
	assert(sdo);
	assert(begin);
	assert(end >= begin);

	// Check whether the SDO exists, is valid and is in the waiting state.
	if (!co_csdo_is_valid(sdo) || !co_csdo_is_idle(sdo)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	co_unsigned32_t ac = 0;

	// Read the total number of sub-indices.
	co_unsigned32_t n;
	if (co_val_read(CO_DEFTYPE_UNSIGNED32, &n, begin, end) != 4)
		ac = CO_SDO_AC_TYPE_LEN_LO;
	begin += 4;

	// Start the first SDO request.
	sdo->dn_dcf = (struct co_csdo_dn_dcf){ n, begin, end, con, data };
	co_csdo_dn_dcf_dn_con(sdo, 0, 0, ac, NULL);

	return 0;
}

int
co_csdo_up_req(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_csdo_up_con_t *con, void *data)
{
	assert(sdo);

	if (co_csdo_up_ind(sdo, idx, subidx, NULL, con, data) == -1)
		return -1;

	trace("CSDO: %04X:%02X: initiate upload", idx, subidx);

	if (sdo->timeout)
		can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);
	co_csdo_send_up_ini_req(sdo);
	co_csdo_enter(sdo, co_csdo_up_ini_state);

	return 0;
}

int
co_csdo_blk_dn_req(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		const void *ptr, size_t n, co_csdo_dn_con_t *con, void *data)
{
	assert(sdo);

	if (co_csdo_dn_ind(sdo, idx, subidx, ptr, n, con, data) == -1)
		return -1;

	trace("CSDO: %04X:%02X: initiate block download", idx, subidx);

	if (sdo->timeout)
		can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);
	co_csdo_send_blk_dn_ini_req(sdo);
	co_csdo_enter(sdo, co_csdo_blk_dn_ini_state);

	return 0;
}

int
co_csdo_blk_dn_val_req(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned16_t type, const void *val,
		co_csdo_dn_con_t *con, void *data)
{
	assert(sdo);
	struct membuf *buf = &sdo->buf;

	// Obtain the size of the serialized value (which may be 0 for arrays).
	size_t n = co_val_write(type, val, NULL, NULL);
	if (!n && co_val_sizeof(type, val))
		return -1;

	membuf_clear(buf);
	if (!membuf_reserve(buf, n))
		return -1;
	void *ptr = membuf_alloc(buf, &n);

	if (co_val_write(type, val, ptr, (uint_least8_t *)ptr + n) != n)
		return -1;

	return co_csdo_blk_dn_req(sdo, idx, subidx, ptr, n, con, data);
}

int
co_csdo_blk_up_req(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		co_unsigned8_t pst, co_csdo_up_con_t *con, void *data)
{
	assert(sdo);

	if (co_csdo_up_ind(sdo, idx, subidx, NULL, con, data) == -1)
		return -1;

	trace("CSDO: %04X:%02X: initiate block upload", idx, subidx);

	// Use the maximum block size by default.
	sdo->blksize = CO_SDO_MAX_SEQNO;

	if (sdo->timeout)
		can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);
	co_csdo_send_blk_up_ini_req(sdo, pst);
	co_csdo_enter(sdo, co_csdo_blk_up_ini_state);

	return 0;
}

static int
co_csdo_update(co_csdo_t *sdo)
{
	assert(sdo);

	// Abort any ongoing transfer.
	co_csdo_abort_req(sdo, CO_SDO_AC_NO_SDO);

	if (co_csdo_is_valid(sdo)) {
		uint_least32_t id = sdo->par.cobid_res;
		uint_least8_t flags = 0;
		if (id & CO_SDO_COBID_FRAME) {
			id &= CAN_MASK_EID;
			flags |= CAN_FLAG_IDE;
		} else {
			id &= CAN_MASK_BID;
		}
		can_recv_start(sdo->recv, sdo->net, id, flags);
	} else {
		can_recv_stop(sdo->recv);
	}

	return 0;
}

static co_unsigned32_t
co_1280_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(req);
	co_csdo_t *sdo = data;
	assert(sdo);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1280 + sdo->num - 1);

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

		// The CAN-ID cannot be changed when the SDO is and remains
		// valid.
		int valid = !(cobid & CO_SDO_COBID_VALID);
		int valid_old = !(cobid_old & CO_SDO_COBID_VALID);
		uint_least32_t canid = cobid & CAN_MASK_EID;
		uint_least32_t canid_old = cobid_old & CAN_MASK_EID;
		if (valid && valid_old && canid != canid_old)
			return CO_SDO_AC_PARAM_VAL;

		// A 29-bit CAN-ID is only valid if the frame bit is set.
		if (!(cobid & CO_SDO_COBID_FRAME)
				&& (cobid & (CAN_MASK_EID ^ CAN_MASK_BID)))
			return CO_SDO_AC_PARAM_VAL;

		sdo->par.cobid_req = cobid;
		break;
	}
	case 2: {
		assert(type == CO_DEFTYPE_UNSIGNED32);
		co_unsigned32_t cobid = val.u32;
		co_unsigned32_t cobid_old = co_sub_get_val_u32(sub);
		if (cobid == cobid_old)
			return 0;

		// The CAN-ID cannot be changed when the SDO is and remains
		// valid.
		int valid = !(cobid & CO_SDO_COBID_VALID);
		int valid_old = !(cobid_old & CO_SDO_COBID_VALID);
		uint_least32_t canid = cobid & CAN_MASK_EID;
		uint_least32_t canid_old = cobid_old & CAN_MASK_EID;
		if (valid && valid_old && canid != canid_old)
			return CO_SDO_AC_PARAM_VAL;

		// A 29-bit CAN-ID is only valid if the frame bit is set.
		if (!(cobid & CO_SDO_COBID_FRAME)
				&& (cobid & (CAN_MASK_EID ^ CAN_MASK_BID)))
			return CO_SDO_AC_PARAM_VAL;

		sdo->par.cobid_res = cobid;
		break;
	}
	case 3: {
		assert(type == CO_DEFTYPE_UNSIGNED8);
		co_unsigned8_t id = val.u8;
		co_unsigned8_t id_old = co_sub_get_val_u8(sub);
		if (id == id_old)
			return 0;

		sdo->par.id = id;
		break;
	}
	default: return CO_SDO_AC_NO_SUB;
	}

	co_sub_dn(sub, &val);

	co_csdo_update(sdo);
	return 0;
}

static int
co_csdo_recv(const struct can_msg *msg, void *data)
{
	assert(msg);
	co_csdo_t *sdo = data;
	assert(sdo);

	// Ignore remote frames.
	if (msg->flags & CAN_FLAG_RTR)
		return 0;

#if !LELY_NO_CANFD
	// Ignore CAN FD format frames.
	if (msg->flags & CAN_FLAG_EDL)
		return 0;
#endif

	co_csdo_emit_recv(sdo, msg);

	return 0;
}

static int
co_csdo_timer(const struct timespec *tp, void *data)
{
	assert(tp);
	co_csdo_t *sdo = data;
	assert(sdo);

	co_csdo_emit_time(sdo, tp);

	return 0;
}

static inline void
co_csdo_enter(co_csdo_t *sdo, co_csdo_state_t *next)
{
	assert(sdo);
	assert(sdo->state);

	while (next) {
		co_csdo_state_t *prev = sdo->state;
		sdo->state = next;

		if (prev->on_leave)
			prev->on_leave(sdo);

		next = next->on_enter ? next->on_enter(sdo) : NULL;
	}
}

static inline void
co_csdo_emit_abort(co_csdo_t *sdo, co_unsigned32_t ac)
{
	assert(sdo);
	assert(sdo->state);
	assert(sdo->state->on_abort);

	co_csdo_enter(sdo, sdo->state->on_abort(sdo, ac));
}

static inline void
co_csdo_emit_time(co_csdo_t *sdo, const struct timespec *tp)
{
	assert(sdo);
	assert(sdo->state);
	assert(sdo->state->on_time);

	co_csdo_enter(sdo, sdo->state->on_time(sdo, tp));
}

static inline void
co_csdo_emit_recv(co_csdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(sdo->state);
	assert(sdo->state->on_recv);

	co_csdo_enter(sdo, sdo->state->on_recv(sdo, msg));
}

static co_csdo_state_t *
co_csdo_stopped_on_abort(co_csdo_t *sdo, co_unsigned32_t ac)
{
	(void)sdo;
	(void)ac;

	return NULL;
}

static co_csdo_state_t *
co_csdo_wait_on_abort(co_csdo_t *sdo, co_unsigned32_t ac)
{
	(void)sdo;
	(void)ac;

	return NULL;
}

static co_csdo_state_t *
co_csdo_wait_on_recv(co_csdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);

	if (msg->len < 1)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	co_unsigned32_t ac;
	switch (cs & CO_SDO_CS_MASK) {
	case CO_SDO_CS_ABORT:
		ac = msg->len < 8 ? 0 : ldle_u32(msg->data + 4);
		return co_csdo_abort_ind(sdo, ac ? ac : CO_SDO_AC_ERROR);
	default: return NULL;
	}
}

static co_csdo_state_t *
co_csdo_abort_on_enter(co_csdo_t *sdo)
{
	(void)sdo;

	can_timer_stop(sdo->timer);

	return co_csdo_wait_state;
}

static void
co_csdo_abort_on_leave(co_csdo_t *sdo)
{
	assert(sdo);

	co_csdo_dn_con_t *dn_con = sdo->dn_con;
	sdo->dn_con = NULL;
	void *dn_con_data = sdo->dn_con_data;
	sdo->dn_con_data = NULL;

	co_csdo_up_con_t *up_con = sdo->up_con;
	sdo->up_con = NULL;
	void *up_con_data = sdo->up_con_data;
	sdo->up_con_data = NULL;

	if (dn_con) {
		dn_con(sdo, sdo->idx, sdo->subidx, sdo->ac, dn_con_data);
	} else if (up_con) {
		struct membuf *buf = sdo->up_buf;
		assert(buf);

		up_con(sdo, sdo->idx, sdo->subidx, sdo->ac,
				sdo->ac ? NULL : buf->begin,
				sdo->ac ? 0 : membuf_size(buf), up_con_data);
	}
}

static co_csdo_state_t *
co_csdo_dn_ini_on_abort(co_csdo_t *sdo, co_unsigned32_t ac)
{
	return co_csdo_abort_res(sdo, ac);
}

static co_csdo_state_t *
co_csdo_dn_ini_on_time(co_csdo_t *sdo, const struct timespec *tp)
{
	(void)tp;

	return co_csdo_abort_res(sdo, CO_SDO_AC_TIMEOUT);
}

static co_csdo_state_t *
co_csdo_dn_ini_on_recv(co_csdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);

	if (msg->len < 1)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	// Check the server command specifier.
	co_unsigned32_t ac;
	switch (cs & CO_SDO_CS_MASK) {
	case CO_SDO_SCS_DN_INI_RES: break;
	case CO_SDO_CS_ABORT:
		ac = msg->len < 8 ? 0 : ldle_u32(msg->data + 4);
		return co_csdo_abort_ind(sdo, ac ? ac : CO_SDO_AC_ERROR);
	default: return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	}

	// Check the object index and sub-index.
	if (msg->len < 4)
		return co_csdo_abort_res(sdo, CO_SDO_AC_ERROR);
	co_unsigned16_t idx = ldle_u16(msg->data + 1);
	co_unsigned8_t subidx = msg->data[3];
	if (idx != sdo->idx || subidx != sdo->subidx)
		return co_csdo_abort_res(sdo, CO_SDO_AC_ERROR);

	return co_csdo_dn_seg_state;
}

static co_csdo_state_t *
co_csdo_dn_seg_on_enter(co_csdo_t *sdo)
{
	assert(sdo);
	struct membuf *buf = &sdo->dn_buf;

	size_t n = sdo->size - membuf_size(buf);
	// 0-byte values cannot be sent using expedited transfer, so we need to
	// send one empty segment. We use the toggle bit to check if it was
	// sent.
	if (n || (!sdo->size && !sdo->toggle)) {
		if (sdo->timeout)
			can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);
		co_csdo_send_dn_seg_req(sdo, MIN(n, 7), n <= 7);
		return NULL;
	} else {
		return co_csdo_abort_ind(sdo, 0);
	}
}

static co_csdo_state_t *
co_csdo_dn_seg_on_abort(co_csdo_t *sdo, co_unsigned32_t ac)
{
	return co_csdo_abort_res(sdo, ac);
}

static co_csdo_state_t *
co_csdo_dn_seg_on_time(co_csdo_t *sdo, const struct timespec *tp)
{
	(void)tp;

	return co_csdo_abort_res(sdo, CO_SDO_AC_TIMEOUT);
}

static co_csdo_state_t *
co_csdo_dn_seg_on_recv(co_csdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);

	if (msg->len < 1)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	// Check the server command specifier.
	co_unsigned32_t ac;
	switch (cs & CO_SDO_CS_MASK) {
	case CO_SDO_SCS_DN_SEG_RES: break;
	case CO_SDO_CS_ABORT:
		ac = msg->len < 8 ? 0 : ldle_u32(msg->data + 4);
		return co_csdo_abort_ind(sdo, ac ? ac : CO_SDO_AC_ERROR);
	default: return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	}

	// Check the value of the toggle bit.
	if ((cs & CO_SDO_SEG_TOGGLE) == sdo->toggle)
		return co_csdo_abort_res(sdo, CO_SDO_AC_TOGGLE);

	return co_csdo_dn_seg_state;
}

static co_csdo_state_t *
co_csdo_up_ini_on_abort(co_csdo_t *sdo, co_unsigned32_t ac)
{
	return co_csdo_abort_res(sdo, ac);
}

static co_csdo_state_t *
co_csdo_up_ini_on_time(co_csdo_t *sdo, const struct timespec *tp)
{
	(void)tp;

	return co_csdo_abort_res(sdo, CO_SDO_AC_TIMEOUT);
}

static co_csdo_state_t *
co_csdo_up_ini_on_recv(co_csdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);
	struct membuf *buf = sdo->up_buf;
	assert(buf);

	if (msg->len < 1)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	// Check the server command specifier.
	co_unsigned32_t ac;
	switch (cs & CO_SDO_CS_MASK) {
	case CO_SDO_SCS_UP_INI_RES: break;
	case CO_SDO_CS_ABORT:
		ac = msg->len < 8 ? 0 : ldle_u32(msg->data + 4);
		return co_csdo_abort_ind(sdo, ac ? ac : CO_SDO_AC_ERROR);
	default: return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	}

	// Check the object index and sub-index.
	if (msg->len < 4)
		return co_csdo_abort_res(sdo, CO_SDO_AC_ERROR);
	co_unsigned16_t idx = ldle_u16(msg->data + 1);
	co_unsigned8_t subidx = msg->data[3];
	if (idx != sdo->idx || subidx != sdo->subidx)
		return co_csdo_abort_res(sdo, CO_SDO_AC_ERROR);

	// 0-pad the data bytes to handle servers which send CAN frames less
	// than 8 bytes.
	uint_least8_t data[4] = { 0 };
	memcpy(data, msg->data + 4, msg->len - 4);

	// Obtain the size from the command specifier.
	int exp = !!(cs & CO_SDO_INI_SIZE_EXP);
	sdo->size = 0;
	if (exp) {
		if (cs & CO_SDO_INI_SIZE_IND)
			sdo->size = CO_SDO_INI_SIZE_EXP_GET(cs);
		else
			sdo->size = msg->len - 4;
	} else if (cs & CO_SDO_INI_SIZE_IND) {
		sdo->size = ldle_u32(data);
	}

	// Allocate the buffer.
	if (sdo->size && !membuf_reserve(buf, sdo->size))
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_MEM);

	if (exp) {
		// Perform an expedited transfer.
		membuf_write(buf, data, sdo->size);

		return co_csdo_abort_ind(sdo, 0);
	} else {
		if (sdo->size && sdo->up_ind)
			sdo->up_ind(sdo, sdo->idx, sdo->subidx, sdo->size, 0,
					sdo->up_ind_data);
		if (sdo->timeout)
			can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);
		co_csdo_send_up_seg_req(sdo);
		return co_csdo_up_seg_state;
	}
}

static co_csdo_state_t *
co_csdo_up_seg_on_abort(co_csdo_t *sdo, co_unsigned32_t ac)
{
	return co_csdo_abort_res(sdo, ac);
}

static co_csdo_state_t *
co_csdo_up_seg_on_time(co_csdo_t *sdo, const struct timespec *tp)
{
	(void)tp;

	return co_csdo_abort_res(sdo, CO_SDO_AC_TIMEOUT);
}

static co_csdo_state_t *
co_csdo_up_seg_on_recv(co_csdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);
	struct membuf *buf = sdo->up_buf;
	assert(buf);

	if (msg->len < 1)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	// Check the server command specifier.
	co_unsigned32_t ac;
	switch (cs & CO_SDO_CS_MASK) {
	case CO_SDO_SCS_UP_SEG_RES: break;
	case CO_SDO_CS_ABORT:
		ac = msg->len < 8 ? 0 : ldle_u32(msg->data + 4);
		return co_csdo_abort_ind(sdo, ac ? ac : CO_SDO_AC_ERROR);
	default: return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	}

	// Check the value of the toggle bit.
	if ((cs & CO_SDO_SEG_TOGGLE) == sdo->toggle)
		return co_csdo_up_seg_state;

	// Obtain the size of the segment.
	size_t n = CO_SDO_SEG_SIZE_GET(cs);
	if (msg->len < 1 + n)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	int last = !!(cs & CO_SDO_SEG_LAST);

	if (membuf_size(buf) + n > sdo->size)
		return co_csdo_abort_res(sdo, CO_SDO_AC_TYPE_LEN_HI);

	// Copy the data to the buffer.
	assert(membuf_capacity(buf) >= n);
	membuf_write(buf, msg->data + 1, n);

	if ((last || !(membuf_size(buf) % (CO_SDO_MAX_SEQNO * 7))) && sdo->size
			&& sdo->up_ind)
		sdo->up_ind(sdo, sdo->idx, sdo->subidx, sdo->size,
				membuf_size(buf), sdo->up_ind_data);
	if (last) {
		if (sdo->size && membuf_size(buf) != sdo->size)
			return co_csdo_abort_res(sdo, CO_SDO_AC_TYPE_LEN_LO);
		return co_csdo_abort_ind(sdo, 0);
	} else {
		if (sdo->timeout)
			can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);
		co_csdo_send_up_seg_req(sdo);
		return co_csdo_up_seg_state;
	}
}

static co_csdo_state_t *
co_csdo_blk_dn_ini_on_abort(co_csdo_t *sdo, co_unsigned32_t ac)
{
	return co_csdo_abort_res(sdo, ac);
}

static co_csdo_state_t *
co_csdo_blk_dn_ini_on_time(co_csdo_t *sdo, const struct timespec *tp)
{
	(void)tp;

	return co_csdo_abort_res(sdo, CO_SDO_AC_TIMEOUT);
}

static co_csdo_state_t *
co_csdo_blk_dn_ini_on_recv(co_csdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);

	if (msg->len < 1)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	// Check the server command specifier.
	co_unsigned32_t ac;
	switch (cs & CO_SDO_CS_MASK) {
	case CO_SDO_SCS_BLK_DN_RES: break;
	case CO_SDO_CS_ABORT:
		ac = msg->len < 8 ? 0 : ldle_u32(msg->data + 4);
		return co_csdo_abort_ind(sdo, ac ? ac : CO_SDO_AC_ERROR);
	default: return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	}

	// Check the server subcommand.
	if ((cs & CO_SDO_SC_MASK) != CO_SDO_SC_INI_BLK)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);

	// Check if the server supports generating a CRC.
	sdo->crc = !!(cs & CO_SDO_BLK_CRC);

	// Check the object index and sub-index.
	if (msg->len < 4)
		return co_csdo_abort_res(sdo, CO_SDO_AC_ERROR);
	co_unsigned16_t idx = ldle_u16(msg->data + 1);
	co_unsigned8_t subidx = msg->data[3];
	if (idx != sdo->idx || subidx != sdo->subidx)
		return co_csdo_abort_res(sdo, CO_SDO_AC_ERROR);

	// Load the number of segments per block.
	if (msg->len < 5)
		return co_csdo_abort_res(sdo, CO_SDO_AC_BLK_SIZE);
	sdo->blksize = msg->data[4];

	return co_csdo_blk_dn_sub_state;
}

static co_csdo_state_t *
co_csdo_blk_dn_sub_on_enter(co_csdo_t *sdo)
{
	assert(sdo);
	struct membuf *buf = &sdo->dn_buf;

	size_t n = sdo->size - membuf_size(buf);
	if ((n > 0 && !sdo->blksize) || sdo->blksize > CO_SDO_MAX_SEQNO)
		return co_csdo_abort_res(sdo, CO_SDO_AC_BLK_SIZE);
	sdo->blksize = (co_unsigned8_t)MIN((n + 6) / 7, sdo->blksize);

	if (sdo->size && sdo->dn_ind)
		sdo->dn_ind(sdo, sdo->idx, sdo->subidx, sdo->size,
				membuf_size(buf), sdo->dn_ind_data);
	if (sdo->timeout)
		can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);
	if (n) {
		// Send all segments in the current block.
		for (co_unsigned8_t seqno = 1; seqno <= sdo->blksize; seqno++)
			co_csdo_send_blk_dn_sub_req(sdo, seqno);
		return NULL;
	} else {
		co_csdo_send_blk_dn_end_req(sdo);
		return co_csdo_blk_dn_end_state;
	}
}

static co_csdo_state_t *
co_csdo_blk_dn_sub_on_abort(co_csdo_t *sdo, co_unsigned32_t ac)
{
	return co_csdo_abort_res(sdo, ac);
}

static co_csdo_state_t *
co_csdo_blk_dn_sub_on_time(co_csdo_t *sdo, const struct timespec *tp)
{
	(void)tp;

	return co_csdo_abort_res(sdo, CO_SDO_AC_TIMEOUT);
}

static co_csdo_state_t *
co_csdo_blk_dn_sub_on_recv(co_csdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);
	struct membuf *buf = &sdo->dn_buf;

	if (msg->len < 1)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	// Check the server command specifier.
	co_unsigned32_t ac;
	switch (cs & CO_SDO_CS_MASK) {
	case CO_SDO_SCS_BLK_DN_RES: break;
	case CO_SDO_CS_ABORT:
		ac = msg->len < 8 ? 0 : ldle_u32(msg->data + 4);
		return co_csdo_abort_ind(sdo, ac ? ac : CO_SDO_AC_ERROR);
	default: return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	}

	// Check the server subcommand.
	if ((cs & CO_SDO_SC_MASK) != CO_SDO_SC_BLK_RES)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);

	if (msg->len < 2)
		return co_csdo_abort_res(sdo, CO_SDO_AC_BLK_SEQ);
	co_unsigned8_t ackseq = msg->data[1];
	if (ackseq < sdo->blksize) {
		// If the sequence number of the last segment that was
		// successfully received is smaller than the number of segments
		// in the block, resend the missing segments.
		size_t n = (membuf_size(buf) + 6) / 7;
		assert(n >= sdo->blksize);
		n -= sdo->blksize - ackseq;
		buf->cur = buf->begin + n * 7;
	}

	// Read the number of segments in the next block.
	if (msg->len < 3)
		return co_csdo_abort_res(sdo, CO_SDO_AC_BLK_SIZE);
	sdo->blksize = msg->data[2];

	return co_csdo_blk_dn_sub_state;
}

static co_csdo_state_t *
co_csdo_blk_dn_end_on_abort(co_csdo_t *sdo, co_unsigned32_t ac)
{
	return co_csdo_abort_res(sdo, ac);
}

static co_csdo_state_t *
co_csdo_blk_dn_end_on_time(co_csdo_t *sdo, const struct timespec *tp)
{
	(void)tp;

	return co_csdo_abort_res(sdo, CO_SDO_AC_TIMEOUT);
}

static co_csdo_state_t *
co_csdo_blk_dn_end_on_recv(co_csdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);

	if (msg->len < 1)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	// Check the server command specifier.
	co_unsigned32_t ac;
	switch (cs & CO_SDO_CS_MASK) {
	case CO_SDO_SCS_BLK_DN_RES: break;
	case CO_SDO_CS_ABORT:
		ac = msg->len < 8 ? 0 : ldle_u32(msg->data + 4);
		return co_csdo_abort_ind(sdo, ac ? ac : CO_SDO_AC_ERROR);
	default: return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	}

	// Check the server subcommand.
	if ((cs & CO_SDO_SC_MASK) != CO_SDO_SC_END_BLK)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);

	return co_csdo_abort_ind(sdo, 0);
}

static co_csdo_state_t *
co_csdo_blk_up_ini_on_abort(co_csdo_t *sdo, co_unsigned32_t ac)
{
	return co_csdo_abort_res(sdo, ac);
}

static co_csdo_state_t *
co_csdo_blk_up_ini_on_time(co_csdo_t *sdo, const struct timespec *tp)
{
	(void)tp;

	return co_csdo_abort_res(sdo, CO_SDO_AC_TIMEOUT);
}

static co_csdo_state_t *
co_csdo_blk_up_ini_on_recv(co_csdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);
	struct membuf *buf = sdo->up_buf;
	assert(buf);

	if (msg->len < 1)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	// Check the server command specifier.
	co_unsigned32_t ac;
	switch (cs & CO_SDO_CS_MASK) {
	case CO_SDO_SCS_UP_INI_RES:
		// In case of a server-induced protocol switch, fall back to the
		// SDO upload protocol.
		return co_csdo_up_ini_on_recv(sdo, msg);
	case CO_SDO_SCS_BLK_UP_RES: break;
	case CO_SDO_CS_ABORT:
		ac = msg->len < 8 ? 0 : ldle_u32(msg->data + 4);
		return co_csdo_abort_ind(sdo, ac ? ac : CO_SDO_AC_ERROR);
	default: return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	}

	// Check the server subcommand.
	if ((cs & 0x01) != CO_SDO_SC_INI_BLK)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);

	// Check if the server supports generating a CRC.
	sdo->crc = !!(cs & CO_SDO_BLK_CRC);

	// Check the object index and sub-index.
	if (msg->len < 4)
		return co_csdo_abort_res(sdo, CO_SDO_AC_ERROR);
	co_unsigned16_t idx = ldle_u16(msg->data + 1);
	co_unsigned8_t subidx = msg->data[3];
	if (idx != sdo->idx || subidx != sdo->subidx)
		return co_csdo_abort_res(sdo, CO_SDO_AC_ERROR);

	// Obtain the data set size.
	sdo->size = 0;
	if (cs & CO_SDO_BLK_SIZE_IND) {
		// 0-pad the data bytes to handle servers which send CAN frames
		// less than 8 bytes.
		uint_least8_t data[4] = { 0 };
		memcpy(data, msg->data + 4, msg->len - 4);
		sdo->size = ldle_u32(data);
	}

	// Allocate the buffer.
	if (sdo->size && !membuf_reserve(buf, sdo->size))
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_MEM);

	sdo->ackseq = 0;

	if (sdo->timeout)
		can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);
	co_csdo_send_start_up_req(sdo);
	return co_csdo_blk_up_sub_state;
}

static co_csdo_state_t *
co_csdo_blk_up_sub_on_abort(co_csdo_t *sdo, co_unsigned32_t ac)
{
	return co_csdo_abort_res(sdo, ac);
}

static co_csdo_state_t *
co_csdo_blk_up_sub_on_time(co_csdo_t *sdo, const struct timespec *tp)
{
	(void)tp;

	return co_csdo_abort_res(sdo, CO_SDO_AC_TIMEOUT);
}

static co_csdo_state_t *
co_csdo_blk_up_sub_on_recv(co_csdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);
	struct membuf *buf = sdo->up_buf;
	assert(buf);

	if (msg->len < 1)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	if (cs == CO_SDO_CS_ABORT) {
		co_unsigned32_t ac = msg->len < 8 ? 0 : ldle_u32(msg->data + 4);
		return co_csdo_abort_ind(sdo, ac ? ac : CO_SDO_AC_ERROR);
	}

	co_unsigned8_t seqno = cs & ~CO_SDO_SEQ_LAST;
	int last = !!(cs & CO_SDO_SEQ_LAST);

	// Only accept sequential segments. Dropped segments will be resent
	// after the confirmation message.
	if (seqno == sdo->ackseq + 1) {
		sdo->ackseq++;

		// Determine the number of bytes to copy.
		assert(sdo->size >= membuf_size(buf));
		size_t n = MIN(sdo->size - membuf_size(buf), 7);
		if (!last && n < 7)
			return co_csdo_abort_res(sdo, CO_SDO_AC_TYPE_LEN_HI);

		// Copy the data to the buffer.
		assert(membuf_capacity(buf) >= n);
		membuf_write(buf, msg->data + 1, n);
	}

	// If this is the last segment in the block, send a confirmation.
	if (seqno == sdo->blksize || last) {
		co_csdo_send_blk_up_sub_res(sdo);
		sdo->ackseq = 0;
	}

	if (sdo->timeout)
		can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);
	return last ? co_csdo_blk_up_end_state : co_csdo_blk_up_sub_state;
}

static co_csdo_state_t *
co_csdo_blk_up_end_on_abort(co_csdo_t *sdo, co_unsigned32_t ac)
{
	return co_csdo_abort_res(sdo, ac);
}

static co_csdo_state_t *
co_csdo_blk_up_end_on_time(co_csdo_t *sdo, const struct timespec *tp)
{
	(void)tp;

	return co_csdo_abort_res(sdo, CO_SDO_AC_TIMEOUT);
}

static co_csdo_state_t *
co_csdo_blk_up_end_on_recv(co_csdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);
	struct membuf *buf = sdo->up_buf;
	assert(buf);

	if (msg->len < 1)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	// Check the server command specifier.
	co_unsigned32_t ac;
	switch (cs & CO_SDO_CS_MASK) {
	case CO_SDO_SCS_BLK_UP_RES: break;
	case CO_SDO_CS_ABORT:
		ac = msg->len < 8 ? 0 : ldle_u32(msg->data + 4);
		return co_csdo_abort_ind(sdo, ac ? ac : CO_SDO_AC_ERROR);
	default: return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	}

	// Check the server subcommand.
	if ((cs & CO_SDO_SC_MASK) != CO_SDO_SC_END_BLK)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);

	// Check the total length.
	if (sdo->size && membuf_size(buf) != sdo->size)
		return co_csdo_abort_res(sdo, CO_SDO_AC_TYPE_LEN_LO);

	// Check the number of bytes in the last segment.
	co_unsigned8_t n = sdo->size ? (sdo->size - 1) % 7 + 1 : 0;
	if (CO_SDO_BLK_SIZE_GET(cs) != n)
		return co_csdo_abort_res(sdo, CO_SDO_AC_NO_CS);

	// Check the CRC.
	if (sdo->crc) {
		co_unsigned16_t crc = ldle_u16(msg->data + 1);
		if (crc != co_crc(0, (uint_least8_t *)buf->begin, sdo->size))
			return co_csdo_abort_res(sdo, CO_SDO_AC_BLK_CRC);
	}

	co_csdo_send_blk_up_end_res(sdo);
	return co_csdo_abort_ind(sdo, 0);
}

static co_csdo_state_t *
co_csdo_abort_ind(co_csdo_t *sdo, co_unsigned32_t ac)
{
	assert(sdo);

	sdo->ac = ac;
	return co_csdo_abort_state;
}

static co_csdo_state_t *
co_csdo_abort_res(co_csdo_t *sdo, co_unsigned32_t ac)
{
	co_csdo_send_abort(sdo, ac);
	return co_csdo_abort_ind(sdo, ac);
}

static int
co_csdo_dn_ind(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		const void *ptr, size_t n, co_csdo_dn_con_t *con, void *data)
{
	assert(sdo);

	// Check whether the SDO exists, is valid and is in the waiting state.
	if (!co_csdo_is_valid(sdo) || !co_csdo_is_idle(sdo)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	sdo->ac = 0;
	sdo->idx = idx;
	sdo->subidx = subidx;
	sdo->size = ptr ? n : 0;

	sdo->toggle = 0;
	sdo->blksize = 0;
	sdo->ackseq = 0;
	sdo->crc = 0;

	// Casting away const is safe here since a download (write) request only
	// reads from the provided buffer.
	membuf_init(&sdo->dn_buf, (void *)ptr, n);

	sdo->dn_con = con;
	sdo->dn_con_data = data;

	sdo->up_con = NULL;
	sdo->up_con_data = NULL;

	return 0;
}

static int
co_csdo_up_ind(co_csdo_t *sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
		struct membuf *buf, co_csdo_up_con_t *con, void *data)
{
	assert(sdo);

	// Check whether the SDO exists, is valid and is in the waiting state.
	if (!co_csdo_is_valid(sdo) || !co_csdo_is_idle(sdo)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	sdo->ac = 0;
	sdo->idx = idx;
	sdo->subidx = subidx;
	sdo->size = 0;

	sdo->toggle = 0;
	sdo->blksize = 0;
	sdo->ackseq = 0;
	sdo->crc = 0;

	sdo->up_buf = buf ? buf : &sdo->buf;
	membuf_clear(sdo->up_buf);

	sdo->dn_con = NULL;
	sdo->dn_con_data = NULL;

	sdo->up_con = con;
	sdo->up_con_data = data;

	return 0;
}

static void
co_csdo_send_abort(co_csdo_t *sdo, co_unsigned32_t ac)
{
	assert(sdo);

	struct can_msg msg;
	co_csdo_init_ini_req(sdo, &msg, CO_SDO_CS_ABORT);
	stle_u32(msg.data + 4, ac);
	can_net_send(sdo->net, &msg);
}

static void
co_csdo_send_dn_exp_req(co_csdo_t *sdo)
{
	assert(sdo);
	assert(sdo->size && sdo->size <= 4);
	struct membuf *buf = &sdo->dn_buf;

	co_unsigned8_t cs = CO_SDO_CCS_DN_INI_REQ
			| CO_SDO_INI_SIZE_EXP_SET(sdo->size);

	struct can_msg msg;
	co_csdo_init_ini_req(sdo, &msg, cs);
	memcpy(msg.data + 4, buf->cur, sdo->size);
	buf->cur += sdo->size;
	can_net_send(sdo->net, &msg);
}

static void
co_csdo_send_dn_ini_req(co_csdo_t *sdo)
{
	assert(sdo);
	assert(!sdo->size || sdo->size > 4);

	co_unsigned8_t cs = CO_SDO_CCS_DN_INI_REQ | CO_SDO_INI_SIZE_IND;

	struct can_msg msg;
	co_csdo_init_ini_req(sdo, &msg, cs);
	stle_u32(msg.data + 4, sdo->size);
	can_net_send(sdo->net, &msg);

	if (sdo->size && sdo->dn_ind)
		sdo->dn_ind(sdo, sdo->idx, sdo->subidx, sdo->size, 0,
				sdo->dn_ind_data);
}

static void
co_csdo_send_dn_seg_req(co_csdo_t *sdo, co_unsigned32_t n, int last)
{
	assert(sdo);
	assert(n <= 7);
	struct membuf *buf = &sdo->dn_buf;

	co_unsigned8_t cs = CO_SDO_CCS_DN_SEG_REQ | sdo->toggle
			| CO_SDO_SEG_SIZE_SET(n);
	sdo->toggle ^= CO_SDO_SEG_TOGGLE;
	if (last)
		cs |= CO_SDO_SEG_LAST;

	struct can_msg msg;
	co_csdo_init_seg_req(sdo, &msg, cs);
	memcpy(msg.data + 1, buf->cur, n);
	buf->cur += n;
	can_net_send(sdo->net, &msg);

	if ((last || !(membuf_size(buf) % (CO_SDO_MAX_SEQNO * 7))) && sdo->size
			&& sdo->dn_ind)
		sdo->dn_ind(sdo, sdo->idx, sdo->subidx, sdo->size,
				membuf_size(buf), sdo->dn_ind_data);
}

static void
co_csdo_send_up_ini_req(co_csdo_t *sdo)
{
	assert(sdo);

	co_unsigned8_t cs = CO_SDO_CCS_UP_INI_REQ;

	struct can_msg msg;
	co_csdo_init_ini_req(sdo, &msg, cs);
	can_net_send(sdo->net, &msg);
}

static void
co_csdo_send_up_seg_req(co_csdo_t *sdo)
{
	assert(sdo);

	co_unsigned8_t cs = CO_SDO_CCS_UP_SEG_REQ | sdo->toggle;
	sdo->toggle ^= CO_SDO_SEG_TOGGLE;

	struct can_msg msg;
	co_csdo_init_seg_req(sdo, &msg, cs);
	can_net_send(sdo->net, &msg);
}

static void
co_csdo_send_blk_dn_ini_req(co_csdo_t *sdo)
{
	assert(sdo);

	co_unsigned8_t cs = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_BLK_CRC
			| CO_SDO_BLK_SIZE_IND | CO_SDO_SC_INI_BLK;

	struct can_msg msg;
	co_csdo_init_ini_req(sdo, &msg, cs);
	stle_u32(msg.data + 4, sdo->size);
	can_net_send(sdo->net, &msg);
}

static void
co_csdo_send_blk_dn_sub_req(co_csdo_t *sdo, co_unsigned8_t seqno)
{
	assert(sdo);
	assert(seqno && seqno <= CO_SDO_MAX_SEQNO);
	struct membuf *buf = &sdo->dn_buf;

	size_t n = sdo->size - membuf_size(buf);
	int last = n <= 7;
	n = MIN(n, 7);

	co_unsigned8_t cs = seqno;
	if (last)
		cs |= CO_SDO_SEQ_LAST;

	struct can_msg msg;
	co_csdo_init_seg_req(sdo, &msg, cs);
	memcpy(msg.data + 1, buf->cur, n);
	buf->cur += n;
	can_net_send(sdo->net, &msg);
}

static void
co_csdo_send_blk_dn_end_req(co_csdo_t *sdo)
{
	assert(sdo);
	struct membuf *buf = &sdo->dn_buf;

	// Compute the number of bytes in the last segment containing data.
	co_unsigned8_t n = sdo->size ? (sdo->size - 1) % 7 + 1 : 0;

	co_unsigned8_t cs = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_SC_END_BLK
			| CO_SDO_BLK_SIZE_SET(n);

	co_unsigned16_t crc = sdo->crc
			? co_crc(0, (uint_least8_t *)buf->begin, sdo->size)
			: 0;

	struct can_msg msg;
	co_csdo_init_seg_req(sdo, &msg, cs);
	stle_u16(msg.data + 1, crc);
	can_net_send(sdo->net, &msg);
}

static void
co_csdo_send_blk_up_ini_req(co_csdo_t *sdo, co_unsigned8_t pst)
{
	assert(sdo);

	co_unsigned8_t cs = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_BLK_CRC
			| CO_SDO_SC_INI_BLK;

	struct can_msg msg;
	co_csdo_init_ini_req(sdo, &msg, cs);
	msg.data[4] = sdo->blksize;
	msg.data[5] = pst;
	can_net_send(sdo->net, &msg);
}

static void
co_csdo_send_start_up_req(co_csdo_t *sdo)
{
	assert(sdo);

	co_unsigned8_t cs = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_START_UP;

	struct can_msg msg;
	co_csdo_init_seg_req(sdo, &msg, cs);
	can_net_send(sdo->net, &msg);

	if (sdo->size && sdo->up_ind)
		sdo->up_ind(sdo, sdo->idx, sdo->subidx, sdo->size, 0,
				sdo->up_ind_data);
}

static void
co_csdo_send_blk_up_sub_res(co_csdo_t *sdo)
{
	assert(sdo);
	struct membuf *buf = sdo->up_buf;
	assert(buf);

	co_unsigned8_t cs = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_BLK_RES;

	struct can_msg msg;
	co_csdo_init_seg_req(sdo, &msg, cs);
	msg.data[1] = sdo->ackseq;
	msg.data[2] = sdo->blksize;
	can_net_send(sdo->net, &msg);

	if (sdo->size && sdo->up_ind)
		sdo->up_ind(sdo, sdo->idx, sdo->subidx, sdo->size,
				membuf_size(buf), sdo->up_ind_data);
}

static void
co_csdo_send_blk_up_end_res(co_csdo_t *sdo)
{
	assert(sdo);

	co_unsigned8_t cs = CO_SDO_CCS_BLK_UP_REQ | CO_SDO_SC_END_BLK;

	struct can_msg msg;
	co_csdo_init_seg_req(sdo, &msg, cs);
	can_net_send(sdo->net, &msg);
}

static void
co_csdo_init_ini_req(co_csdo_t *sdo, struct can_msg *msg, co_unsigned8_t cs)
{
	assert(sdo);
	assert(msg);

	*msg = (struct can_msg)CAN_MSG_INIT;
	msg->id = sdo->par.cobid_req;
	if (sdo->par.cobid_req & CO_SDO_COBID_FRAME) {
		msg->id &= CAN_MASK_EID;
		msg->flags |= CAN_FLAG_IDE;
	} else {
		msg->id &= CAN_MASK_BID;
	}
	msg->len = CAN_MAX_LEN;
	msg->data[0] = cs;
	stle_u16(msg->data + 1, sdo->idx);
	msg->data[3] = sdo->subidx;
}

static void
co_csdo_init_seg_req(co_csdo_t *sdo, struct can_msg *msg, co_unsigned8_t cs)
{
	assert(sdo);
	assert(msg);

	*msg = (struct can_msg)CAN_MSG_INIT;
	msg->id = sdo->par.cobid_req;
	if (sdo->par.cobid_req & CO_SDO_COBID_FRAME) {
		msg->id &= CAN_MASK_EID;
		msg->flags |= CAN_FLAG_IDE;
	} else {
		msg->id &= CAN_MASK_BID;
	}
	msg->len = CAN_MAX_LEN;
	msg->data[0] = cs;
}

static void
co_csdo_dn_dcf_dn_con(co_csdo_t *sdo, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned32_t ac, void *data)
{
	assert(sdo);
	assert(co_csdo_is_valid(sdo));
	assert(co_csdo_is_idle(sdo));
	struct co_csdo_dn_dcf *dcf = &sdo->dn_dcf;

	if (!ac && dcf->n--) {
		idx = 0;
		subidx = 0;
		ac = CO_SDO_AC_TYPE_LEN_LO;
		// Read the object index.
		// clang-format off
		if (co_val_read(CO_DEFTYPE_UNSIGNED16, &idx, dcf->begin,
				dcf->end) != 2)
			// clang-format on
			goto done;
		dcf->begin += 2;
		// Read the object sub-index.
		// clang-format off
		if (co_val_read(CO_DEFTYPE_UNSIGNED8, &subidx, dcf->begin,
				dcf->end) != 1)
			// clang-format on
			goto done;
		dcf->begin += 1;
		// Read the value size (in bytes).
		co_unsigned32_t size;
		// clang-format off
		if (co_val_read(CO_DEFTYPE_UNSIGNED32, &size, dcf->begin,
				dcf->end) != 4)
			// clang-format on
			goto done;
		dcf->begin += 4;
		if (dcf->end - dcf->begin < (ptrdiff_t)size)
			goto done;
		const void *ptr = dcf->begin;
		dcf->begin += size;
		// Submit the SDO download request. This cannot fail since we
		// already checked that the SDO exists, is valid and is idle.
		co_csdo_dn_req(sdo, idx, subidx, ptr, size,
				&co_csdo_dn_dcf_dn_con, NULL);
		return;
	}

done:;
	co_csdo_dn_con_t *con = dcf->con;
	data = dcf->data;

	*dcf = (struct co_csdo_dn_dcf){ 0 };

	if (con)
		con(sdo, idx, subidx, ac, data);
}

#endif // !LELY_NO_CO_CSDO
