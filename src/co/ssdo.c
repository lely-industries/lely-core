/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the Server-SDO functions.
 *
 * @see lely/co/ssdo.h, src/sdo.h
 *
 * @copyright 2016-2020 Lely Industries N.V.
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
#include "sdo.h"
#include <lely/co/crc.h>
#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/ssdo.h>
#include <lely/co/val.h>
#include <lely/util/endian.h>
#include <lely/util/errnum.h>

#include <assert.h>
#if !LELY_NO_STDIO
#include <inttypes.h>
#endif
#include <stdlib.h>
#if LELY_NO_MALLOC
#include <string.h>
#endif

#if LELY_NO_MALLOC
#ifndef CO_SSDO_MEMBUF_SIZE
/**
 * The default size (in bytes) of a Server-SDO memory buffer in the absence of
 * dynamic memory allocation. The default size is large enough to accomodate the
 * maximum block size used by SDO block transfer.
 */
#define CO_SSDO_MEMBUF_SIZE (CO_SDO_MAX_SEQNO * 7)
#endif
#endif

#if LELY_NO_MALLOC
#define CO_SSDO_MAX_SEQNO MIN(CO_SDO_MAX_SEQNO, (CO_SSDO_MEMBUF_SIZE / 7))
#else
#define CO_SSDO_MAX_SEQNO CO_SDO_MAX_SEQNO
#endif

struct __co_ssdo_state;
/// An opaque CANopen Server-SDO state type.
typedef const struct __co_ssdo_state co_ssdo_state_t;

/// A CANopen Server-SDO.
struct __co_ssdo {
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
	co_ssdo_state_t *state;
	/// The current object index.
	co_unsigned16_t idx;
	/// The current object sub-index.
	co_unsigned8_t subidx;
	/// The current value of the toggle bit.
	co_unsigned8_t toggle;
	/// The number of segments per block.
	co_unsigned8_t blksize;
	/// The sequence number of the last successfully received segment.
	co_unsigned8_t ackseq;
	/// A flag indicating whether a CRC should be generated.
	unsigned gencrc : 1;
	/// The generated CRC.
	co_unsigned16_t crc;
	/// The SDO request.
	struct co_sdo_req req;
	/// The buffer.
	struct membuf buf;
	/// The number of bytes in #req already copied to #buf.
	size_t nbyte;
#if LELY_NO_MALLOC
	/**
	 * The static memory buffer used by #buf in the absence of dynamic
	 * memory allocation.
	 */
	char begin[CO_SSDO_MEMBUF_SIZE];
#endif
};

/**
 * Updates and (de)activates a Server-SDO service. This function is invoked when
 * one of the SDO server parameters (objects 1200..127F) is updated.
 *
 * @returns 0 on success, or -1 on error.
 */
static int co_ssdo_update(co_ssdo_t *sdo);

/**
 * The download indication function for (all sub-objects of) CANopen objects
 * 1200..127F (SDO server parameter).
 *
 * @see co_sub_dn_ind_t
 */
static co_unsigned32_t co_1200_dn_ind(
		co_sub_t *sub, struct co_sdo_req *req, void *data);

/**
 * The CAN receive callback function for a Server-SDO service.
 *
 * @see can_recv_func_t
 */
static int co_ssdo_recv(const struct can_msg *msg, void *data);

/**
 * The CAN timer callback function for a Server-SDO service.
 *
 * @see can_timer_func_t
 */
static int co_ssdo_timer(const struct timespec *tp, void *data);

/// Enters the specified state of a Server-SDO service.
static inline void co_ssdo_enter(co_ssdo_t *sdo, co_ssdo_state_t *next);

/**
 * Invokes the 'abort' transition function of the current state of a Server-SDO
 * service.
 *
 * @param sdo a pointer to a Server-SDO service.
 * @param ac  the abort code.
 */
static inline void co_ssdo_emit_abort(co_ssdo_t *sdo, co_unsigned32_t ac);

/**
 * Invokes the 'timeout' transition function of the current state of a
 * Server-SDO service.
 *
 * @param sdo a pointer to a Server-SDO service.
 * @param tp  a pointer to the current time.
 */
static inline void co_ssdo_emit_time(co_ssdo_t *sdo, const struct timespec *tp);

/**
 * Invokes the 'CAN frame received' transition function of the current state of
 * a Server-SDO service.
 *
 * @param sdo a pointer to a Server-SDO service.
 * @param msg a pointer to the received CAN frame.
 */
static inline void co_ssdo_emit_recv(co_ssdo_t *sdo, const struct can_msg *msg);

/// A CANopen Server-SDO state.
struct __co_ssdo_state {
	/**
	 * A pointer to the transition function invoked when an abort code has
	 * been received.
	 *
	 * @param sdo a pointer to a Server-SDO service.
	 * @param ac  the abort code.
	 *
	 * @returns a pointer to the next state.
	 */
	co_ssdo_state_t *(*on_abort)(co_ssdo_t *sdo, co_unsigned32_t ac);
	/**
	 * A pointer to the transition function invoked when a timeout occurs.
	 *
	 * @param sdo a pointer to a Server-SDO service.
	 * @param tp  a pointer to the current time.
	 *
	 * @returns a pointer to the next state.
	 */
	co_ssdo_state_t *(*on_time)(co_ssdo_t *sdo, const struct timespec *tp);
	/**
	 * A pointer to the transition function invoked when a CAN frame has
	 * been received.
	 *
	 * @param sdo a pointer to a Server-SDO service.
	 * @param msg a pointer to the received CAN frame.
	 *
	 * @returns a pointer to the next state.
	 */
	co_ssdo_state_t *(*on_recv)(co_ssdo_t *sdo, const struct can_msg *msg);
};

#define LELY_CO_DEFINE_STATE(name, ...) \
	static co_ssdo_state_t *const name = &(co_ssdo_state_t){ __VA_ARGS__ };

/// The 'stopped' state.
LELY_CO_DEFINE_STATE(co_ssdo_stopped_state, NULL)

/// The 'abort' transition function of the 'waiting' state.
static co_ssdo_state_t *co_ssdo_wait_on_abort(
		co_ssdo_t *sdo, co_unsigned32_t ac);

/// The 'CAN frame received' transition function of the 'waiting' state.
static co_ssdo_state_t *co_ssdo_wait_on_recv(
		co_ssdo_t *sdo, const struct can_msg *msg);

/// The 'waiting' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_ssdo_wait_state,
	.on_abort = &co_ssdo_wait_on_abort,
	.on_recv = &co_ssdo_wait_on_recv
)
// clang-format on

/**
 * The 'CAN frame received' transition function of the 'download initiate'
 * state.
 */
static co_ssdo_state_t *co_ssdo_dn_ini_on_recv(
		co_ssdo_t *sdo, const struct can_msg *msg);

/// The 'download initiate' state.
// LELY_CO_DEFINE_STATE(co_ssdo_dn_ini_state,
//	.on_recv = &co_ssdo_dn_ini_on_recv
//)

/// The 'abort' transition function of the 'download segment' state.
static co_ssdo_state_t *co_ssdo_dn_seg_on_abort(
		co_ssdo_t *sdo, co_unsigned32_t ac);

/// The 'timeout' transition function of the 'download segment' state.
static co_ssdo_state_t *co_ssdo_dn_seg_on_time(
		co_ssdo_t *sdo, const struct timespec *tp);

/**
 * The 'CAN frame received' transition function of the 'download segment' state.
 */
static co_ssdo_state_t *co_ssdo_dn_seg_on_recv(
		co_ssdo_t *sdo, const struct can_msg *msg);

/// The 'download segment' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_ssdo_dn_seg_state,
	.on_abort = &co_ssdo_dn_seg_on_abort,
	.on_time = &co_ssdo_dn_seg_on_time,
	.on_recv = &co_ssdo_dn_seg_on_recv
)
// clang-format on

/// The 'CAN frame received' transition function of the 'upload initiate' state.
static co_ssdo_state_t *co_ssdo_up_ini_on_recv(
		co_ssdo_t *sdo, const struct can_msg *msg);

/// The 'upload initiate' state.
// LELY_CO_DEFINE_STATE(co_ssdo_up_ini_state,
//	.on_recv = &co_ssdo_up_ini_on_recv
//)

/// The 'abort' transition function of the 'upload segment' state.
static co_ssdo_state_t *co_ssdo_up_seg_on_abort(
		co_ssdo_t *sdo, co_unsigned32_t ac);

/// The 'timeout' transition function of the 'upload segment' state.
static co_ssdo_state_t *co_ssdo_up_seg_on_time(
		co_ssdo_t *sdo, const struct timespec *tp);

/// The 'CAN frame received' transition function of the 'upload segment' state.
static co_ssdo_state_t *co_ssdo_up_seg_on_recv(
		co_ssdo_t *sdo, const struct can_msg *msg);

/// The 'upload segment' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_ssdo_up_seg_state,
	.on_abort = &co_ssdo_up_seg_on_abort,
	.on_time = &co_ssdo_up_seg_on_time,
	.on_recv = &co_ssdo_up_seg_on_recv
)
// clang-format on

/**
 * The 'CAN frame received' transition function of the 'block download initiate'
 * state.
 */
static co_ssdo_state_t *co_ssdo_blk_dn_ini_on_recv(
		co_ssdo_t *sdo, const struct can_msg *msg);

/// The 'block download initiate' state.
// LELY_CO_DEFINE_STATE(co_ssdo_blk_dn_ini_state,
//	.on_recv = &co_ssdo_blk_dn_ini_on_recv
//)

/// The 'abort' transition function of the 'block download sub-block' state.
static co_ssdo_state_t *co_ssdo_blk_dn_sub_on_abort(
		co_ssdo_t *sdo, co_unsigned32_t ac);

/// The 'timeout' transition function of the 'block download sub-block' state.
static co_ssdo_state_t *co_ssdo_blk_dn_sub_on_time(
		co_ssdo_t *sdo, const struct timespec *tp);

/**
 * The 'CAN frame received' transition function of the 'block download
 * sub-block' state.
 */
static co_ssdo_state_t *co_ssdo_blk_dn_sub_on_recv(
		co_ssdo_t *sdo, const struct can_msg *msg);

/// The 'block download sub-block' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_ssdo_blk_dn_sub_state,
	.on_abort = &co_ssdo_blk_dn_sub_on_abort,
	.on_time = &co_ssdo_blk_dn_sub_on_time,
	.on_recv = &co_ssdo_blk_dn_sub_on_recv
)
// clang-format on

/// The 'abort' transition function of the 'block download end' state.
static co_ssdo_state_t *co_ssdo_blk_dn_end_on_abort(
		co_ssdo_t *sdo, co_unsigned32_t ac);

/// The 'timeout' transition function of the 'block download end' state.
static co_ssdo_state_t *co_ssdo_blk_dn_end_on_time(
		co_ssdo_t *sdo, const struct timespec *tp);

/**
 * The 'CAN frame received' transition function of the 'block download end'
 * state.
 */
static co_ssdo_state_t *co_ssdo_blk_dn_end_on_recv(
		co_ssdo_t *sdo, const struct can_msg *msg);

/// The 'block download end' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_ssdo_blk_dn_end_state,
	.on_abort = &co_ssdo_blk_dn_end_on_abort,
	.on_time = &co_ssdo_blk_dn_end_on_time,
	.on_recv = &co_ssdo_blk_dn_end_on_recv
)
// clang-format on

/**
 * The 'CAN frame received' transition function of the 'block upload initiate'
 * state.
 */
static co_ssdo_state_t *co_ssdo_blk_up_ini_on_recv(
		co_ssdo_t *sdo, const struct can_msg *msg);

/// The 'block upload initiate' state.
// LELY_CO_DEFINE_STATE(co_ssdo_blk_up_ini_state,
//	.on_recv = &co_ssdo_blk_up_ini_on_recv
//)

/// The 'abort' transition function of the 'block upload sub-block' state.
static co_ssdo_state_t *co_ssdo_blk_up_sub_on_abort(
		co_ssdo_t *sdo, co_unsigned32_t ac);

/// The 'timeout' transition function of the 'block upload sub-block' state.
static co_ssdo_state_t *co_ssdo_blk_up_sub_on_time(
		co_ssdo_t *sdo, const struct timespec *tp);

/**
 * The 'CAN frame received' transition function of the 'block upload sub-block'
 * state.
 */
static co_ssdo_state_t *co_ssdo_blk_up_sub_on_recv(
		co_ssdo_t *sdo, const struct can_msg *msg);

/// The 'block upload sub-block' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_ssdo_blk_up_sub_state,
	.on_abort = &co_ssdo_blk_up_sub_on_abort,
	.on_time = &co_ssdo_blk_up_sub_on_time,
	.on_recv = &co_ssdo_blk_up_sub_on_recv
)
// clang-format on

/// The 'abort' transition function of the 'block upload end' state.
static co_ssdo_state_t *co_ssdo_blk_up_end_on_abort(
		co_ssdo_t *sdo, co_unsigned32_t ac);

/// The 'timeout' transition function of the 'block upload end' state.
static co_ssdo_state_t *co_ssdo_blk_up_end_on_time(
		co_ssdo_t *sdo, const struct timespec *tp);

/**
 * The 'CAN frame received' transition function of the 'block upload end' state.
 */
static co_ssdo_state_t *co_ssdo_blk_up_end_on_recv(
		co_ssdo_t *sdo, const struct can_msg *msg);

/// The 'block upload end' state.
// clang-format off
LELY_CO_DEFINE_STATE(co_ssdo_blk_up_end_state,
	.on_abort = &co_ssdo_blk_up_end_on_abort,
	.on_time = &co_ssdo_blk_up_end_on_time,
	.on_recv = &co_ssdo_blk_up_end_on_recv
)
// clang-format on

#undef LELY_CO_DEFINE_STATE

/**
 * Processes an abort transfer indication by aborting any ongoing transfer of a
 * Server-SDO and returning it to the waiting state.
 *
 * @returns #co_ssdo_wait_state
 */
static co_ssdo_state_t *co_ssdo_abort_ind(co_ssdo_t *sdo);

/**
 * Sends an abort transfer request and aborts any ongoing transfer by invoking
 * co_ssdo_abort_ind().
 *
 * @param sdo a pointer to a Server-SDO service.
 * @param ac  the SDO abort code.
 *
 * @returns #co_ssdo_wait_state
 *
 * @see co_ssdo_send_abort()
 */
static co_ssdo_state_t *co_ssdo_abort_res(co_ssdo_t *sdo, co_unsigned32_t ac);

/**
 * Processes a download indication of a Server-SDO by checking access to the
 * requested sub-object and reading the data from the frame.
 *
 * @returns 0 on success, or an SDO abort code on error.
 */
static co_unsigned32_t co_ssdo_dn_ind(co_ssdo_t *sdo);

/**
 * Processes an upload indication of a Server-SDO by checking access to the
 * requested sub-object and writing the data to the internal buffer.
 *
 * @returns 0 on success, or an SDO abort code on error.
 */
static co_unsigned32_t co_ssdo_up_ind(co_ssdo_t *sdo);

/**
 * Copies at most <b>nbyte</b> bytes from a CANopen SDO upload request,
 * obtaining more bytes with co_ssdo_up_ind() as necessary.
 *
 * @returns 0 on success, or an SDO abort code on error.
 */
static co_unsigned32_t co_ssdo_up_buf(co_ssdo_t *sdo, size_t nbyte);

/**
 * Sends an abort transfer request.
 *
 * @param sdo a pointer to a Server-SDO service.
 * @param ac  the SDO abort code.
 */
static void co_ssdo_send_abort(co_ssdo_t *sdo, co_unsigned32_t ac);

/// Sends a Server-SDO 'download initiate' response.
static void co_ssdo_send_dn_ini_res(co_ssdo_t *sdo);

/// Sends a Server-SDO 'download segment' response.
static void co_ssdo_send_dn_seg_res(co_ssdo_t *sdo);

/// Sends a Server-SDO 'upload initiate' (expedited) response.
static void co_ssdo_send_up_exp_res(co_ssdo_t *sdo);

/// Sends a Server-SDO 'upload initiate' response.
static void co_ssdo_send_up_ini_res(co_ssdo_t *sdo);

/**
 * Sends a Server-SDO 'upload segment' response.
 *
 * @param sdo  a pointer to a Server-SDO service.
 * @param last a flag indicating whether this is the last segment.
 */
static void co_ssdo_send_up_seg_res(co_ssdo_t *sdo, int last);

/// Sends a Server-SDO 'block download initiate' response.
static void co_ssdo_send_blk_dn_ini_res(co_ssdo_t *sdo);

/// Sends a Server-SDO 'block upload sub-block' response.
static void co_ssdo_send_blk_dn_sub_res(co_ssdo_t *sdo);

/// Sends a Server-SDO 'block download end' response.
static void co_ssdo_send_blk_dn_end_res(co_ssdo_t *sdo);

/// Sends a Server-SDO 'block upload initiate' response.
static void co_ssdo_send_blk_up_ini_res(co_ssdo_t *sdo);

/**
 * Sends a Server-SDO 'block upload sub-block' response.
 *
 * @param sdo  a pointer to a Server-SDO service.
 * @param last a flag indicating whether this block contains the last segment.
 */
static void co_ssdo_send_blk_up_sub_res(co_ssdo_t *sdo, int last);

/// Sends a Server-SDO 'block upload end' response.
static void co_ssdo_send_blk_up_end_res(co_ssdo_t *sdo);

/**
 * Initializes a Server-SDO download/upload initiate response CAN frame.
 *
 * @param sdo a pointer to a Server-SDO service.
 * @param msg a pointer to the CAN frame to be initialized.
 * @param cs  the command specifier.
 */
static void co_ssdo_init_ini_res(
		co_ssdo_t *sdo, struct can_msg *msg, co_unsigned8_t cs);

/**
 * Initializes a Server-SDO download/upload segment response CAN frame.
 *
 * @param sdo a pointer to a Server-SDO service.
 * @param msg a pointer to the CAN frame to be initialized.
 * @param cs  the command specifier.
 */
static void co_ssdo_init_seg_res(
		co_ssdo_t *sdo, struct can_msg *msg, co_unsigned8_t cs);

void *
__co_ssdo_alloc(void)
{
	void *ptr = malloc(sizeof(struct __co_ssdo));
#if !LELY_NO_ERRNO
	if (!ptr)
		set_errc(errno2c(errno));
#endif
	return ptr;
}

void
__co_ssdo_free(void *ptr)
{
	free(ptr);
}

struct __co_ssdo *
__co_ssdo_init(struct __co_ssdo *sdo, can_net_t *net, co_dev_t *dev,
		co_unsigned8_t num)
{
	assert(sdo);
	assert(net);
	assert(dev);

	int errc = 0;

	if (!num || num > CO_NUM_SDOS) {
		errc = errnum2c(ERRNUM_INVAL);
		goto error_param;
	}

	// Find the SDO server parameter in the object dictionary. The default
	// SDO (1200) is optional.
	co_obj_t *obj_1200 = co_dev_find_obj(dev, 0x1200 + num - 1);
	if (num != 1 && !obj_1200) {
		errc = errnum2c(ERRNUM_INVAL);
		goto error_param;
	}

	sdo->net = net;
	sdo->dev = dev;
	sdo->num = num;

	// Initialize the SDO parameter record with the default values.
	sdo->par.n = 3;
	sdo->par.id = co_dev_get_id(dev);
	sdo->par.cobid_req = 0x600 + sdo->par.id;
	sdo->par.cobid_res = 0x580 + sdo->par.id;

	sdo->recv = can_recv_create();
	if (!sdo->recv) {
		errc = get_errc();
		goto error_create_recv;
	}
	can_recv_set_func(sdo->recv, &co_ssdo_recv, sdo);

	sdo->timeout = 0;

	sdo->timer = can_timer_create();
	if (!sdo->timer) {
		errc = get_errc();
		goto error_create_timer;
	}
	can_timer_set_func(sdo->timer, &co_ssdo_timer, sdo);

	sdo->state = co_ssdo_stopped_state;

	sdo->idx = 0;
	sdo->subidx = 0;

	sdo->toggle = 0;
	sdo->blksize = 0;
	sdo->ackseq = 0;
	sdo->gencrc = 0;
	sdo->crc = 0;

	co_sdo_req_init(&sdo->req);
#if LELY_NO_MALLOC
	membuf_init(&sdo->buf, sdo->begin, CO_SSDO_MEMBUF_SIZE);
#else
	membuf_init(&sdo->buf, NULL, 0);
#endif
	sdo->nbyte = 0;
#if LELY_NO_MALLOC
	memset(sdo->begin, 0, CO_SSDO_MEMBUF_SIZE);
#endif

	if (co_ssdo_start(sdo) == -1) {
		errc = get_errc();
		goto error_start;
	}

	return sdo;

	// co_ssdo_stop(sdo);
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
__co_ssdo_fini(struct __co_ssdo *sdo)
{
	assert(sdo);
	assert(sdo->num >= 1 && sdo->num <= CO_NUM_SDOS);

	co_ssdo_stop(sdo);

	membuf_fini(&sdo->buf);
	co_sdo_req_fini(&sdo->req);

	can_timer_destroy(sdo->timer);

	can_recv_destroy(sdo->recv);
}

co_ssdo_t *
co_ssdo_create(can_net_t *net, co_dev_t *dev, co_unsigned8_t num)
{
	trace("creating Server-SDO %d", num);

	int errc = 0;

	co_ssdo_t *sdo = __co_ssdo_alloc();
	if (!sdo) {
		errc = get_errc();
		goto error_alloc_sdo;
	}

	if (!__co_ssdo_init(sdo, net, dev, num)) {
		errc = get_errc();
		goto error_init_sdo;
	}

	return sdo;

error_init_sdo:
	__co_ssdo_free(sdo);
error_alloc_sdo:
	set_errc(errc);
	return NULL;
}

void
co_ssdo_destroy(co_ssdo_t *ssdo)
{
	if (ssdo) {
		trace("destroying Server-SDO %d", ssdo->num);
		__co_ssdo_fini(ssdo);
		__co_ssdo_free(ssdo);
	}
}

int
co_ssdo_start(co_ssdo_t *sdo)
{
	assert(sdo);

	if (!co_ssdo_is_stopped(sdo))
		return 0;

	co_ssdo_enter(sdo, co_ssdo_wait_state);

	co_obj_t *obj_1200 = co_dev_find_obj(sdo->dev, 0x1200 + sdo->num - 1);
	if (obj_1200) {
		// Copy the SDO parameter record.
		size_t size = co_obj_sizeof_val(obj_1200);
		memcpy(&sdo->par, co_obj_addressof_val(obj_1200),
				MIN(size, sizeof(sdo->par)));
		// Set the download indication function for the SDO parameter
		// record.
		co_obj_set_dn_ind(obj_1200, &co_1200_dn_ind, sdo);
	}

	if (co_ssdo_update(sdo) == -1)
		goto error_update;

	return 0;

error_update:
	co_ssdo_stop(sdo);
	return -1;
}

void
co_ssdo_stop(co_ssdo_t *sdo)
{
	assert(sdo);

	if (co_ssdo_is_stopped(sdo))
		return;

	// Abort any ongoing transfer.
	co_ssdo_emit_abort(sdo, CO_SDO_AC_NO_SDO);

	can_timer_stop(sdo->timer);
	can_recv_stop(sdo->recv);

	// Remove the download indication functions for the SDO parameter
	// record.
	co_obj_t *obj_1200 = co_dev_find_obj(sdo->dev, 0x1200 + sdo->num - 1);
	if (obj_1200)
		co_obj_set_dn_ind(obj_1200, NULL, NULL);

	co_ssdo_enter(sdo, co_ssdo_stopped_state);
}

int
co_ssdo_is_stopped(const co_ssdo_t *sdo)
{
	assert(sdo);

	return sdo->state == co_ssdo_stopped_state;
}

can_net_t *
co_ssdo_get_net(const co_ssdo_t *sdo)
{
	assert(sdo);

	return sdo->net;
}

co_dev_t *
co_ssdo_get_dev(const co_ssdo_t *sdo)
{
	assert(sdo);

	return sdo->dev;
}

co_unsigned8_t
co_ssdo_get_num(const co_ssdo_t *sdo)
{
	assert(sdo);

	return sdo->num;
}

const struct co_sdo_par *
co_ssdo_get_par(const co_ssdo_t *sdo)
{
	assert(sdo);

	return &sdo->par;
}

int
co_ssdo_get_timeout(const co_ssdo_t *sdo)
{
	assert(sdo);

	return sdo->timeout;
}

void
co_ssdo_set_timeout(co_ssdo_t *sdo, int timeout)
{
	assert(sdo);

	if (sdo->timeout && timeout <= 0)
		can_timer_stop(sdo->timer);

	sdo->timeout = MAX(0, timeout);
}

static int
co_ssdo_update(co_ssdo_t *sdo)
{
	assert(sdo);

	// Abort any ongoing transfer.
	co_ssdo_emit_abort(sdo, CO_SDO_AC_NO_SDO);

	int valid_req = !(sdo->par.cobid_req & CO_SDO_COBID_VALID);
	int valid_res = !(sdo->par.cobid_res & CO_SDO_COBID_VALID);
	if (valid_req && valid_res) {
		uint_least32_t id = sdo->par.cobid_req;
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
co_1200_dn_ind(co_sub_t *sub, struct co_sdo_req *req, void *data)
{
	assert(sub);
	assert(req);
	co_ssdo_t *sdo = data;
	assert(sdo);
	assert(co_obj_get_idx(co_sub_get_obj(sub)) == 0x1200 + sdo->num - 1);

	co_unsigned16_t type = co_sub_get_type(sub);
	assert(!co_type_is_array(type));

	co_unsigned32_t ac = 0;
	union co_val val;
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

	co_ssdo_update(sdo);
	return 0;
}

static int
co_ssdo_recv(const struct can_msg *msg, void *data)
{
	assert(msg);
	co_ssdo_t *sdo = data;
	assert(sdo);

	// Ignore remote frames.
	if (msg->flags & CAN_FLAG_RTR)
		return 0;

#if !LELY_NO_CANFD
	// Ignore CAN FD format frames.
	if (msg->flags & CAN_FLAG_EDL)
		return 0;
#endif

	co_ssdo_emit_recv(sdo, msg);

	return 0;
}

static int
co_ssdo_timer(const struct timespec *tp, void *data)
{
	assert(tp);
	co_ssdo_t *sdo = data;
	assert(sdo);

	co_ssdo_emit_time(sdo, tp);

	return 0;
}

static inline void
co_ssdo_enter(co_ssdo_t *sdo, co_ssdo_state_t *next)
{
	assert(sdo);

	if (next)
		sdo->state = next;
}

static inline void
co_ssdo_emit_abort(co_ssdo_t *sdo, co_unsigned32_t ac)
{
	assert(sdo);
	assert(sdo->state);
	assert(sdo->state->on_recv);

	co_ssdo_enter(sdo, sdo->state->on_abort(sdo, ac));
}

static inline void
co_ssdo_emit_time(co_ssdo_t *sdo, const struct timespec *tp)
{
	assert(sdo);
	assert(sdo->state);
	assert(sdo->state->on_time);

	co_ssdo_enter(sdo, sdo->state->on_time(sdo, tp));
}

static inline void
co_ssdo_emit_recv(co_ssdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(sdo->state);
	assert(sdo->state->on_recv);

	co_ssdo_enter(sdo, sdo->state->on_recv(sdo, msg));
}

static co_ssdo_state_t *
co_ssdo_wait_on_abort(co_ssdo_t *sdo, co_unsigned32_t ac)
{
	(void)sdo;
	(void)ac;

	return NULL;
}

static co_ssdo_state_t *
co_ssdo_wait_on_recv(co_ssdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);

	if (msg->len < 1)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	switch (cs & CO_SDO_CS_MASK) {
	case CO_SDO_CCS_DN_INI_REQ: return co_ssdo_dn_ini_on_recv(sdo, msg);
	case CO_SDO_CCS_UP_INI_REQ: return co_ssdo_up_ini_on_recv(sdo, msg);
	case CO_SDO_CCS_BLK_DN_REQ: return co_ssdo_blk_dn_ini_on_recv(sdo, msg);
	case CO_SDO_CCS_BLK_UP_REQ: return co_ssdo_blk_up_ini_on_recv(sdo, msg);
	case CO_SDO_CS_ABORT: return NULL;
	default: return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	}
}

static co_ssdo_state_t *
co_ssdo_dn_ini_on_recv(co_ssdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);

	assert(msg->len > 0);
	co_unsigned8_t cs = msg->data[0];

	// Load the object index and sub-index from the CAN frame.
	if (msg->len < 3)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_OBJ);
	sdo->idx = ldle_u16(msg->data + 1);
	if (msg->len < 4)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_SUB);
	sdo->subidx = msg->data[3];

	trace("SSDO: %04X:%02X: received download request", sdo->idx,
			sdo->subidx);

	// Obtain the size from the command specifier.
	co_sdo_req_clear(&sdo->req);
	int exp = !!(cs & CO_SDO_INI_SIZE_EXP);
	if (exp) {
		if (cs & CO_SDO_INI_SIZE_IND)
			sdo->req.size = CO_SDO_INI_SIZE_EXP_GET(cs);
		else
			sdo->req.size = msg->len - 4;
	} else if (cs & CO_SDO_INI_SIZE_IND) {
		// 0-pad the data bytes to handle clients which send CAN frames
		// less than 8 bytes.
		uint_least8_t data[4] = { 0 };
		memcpy(data, msg->data + 4, msg->len - 4);
		sdo->req.size = ldle_u32(data);
	}

	if (exp) {
		// Perform an expedited transfer.
		sdo->req.buf = msg->data + 4;
		sdo->req.nbyte = sdo->req.size;
		co_unsigned32_t ac = co_ssdo_dn_ind(sdo);
		if (ac)
			return co_ssdo_abort_res(sdo, ac);
		// Finalize the transfer.
		co_ssdo_send_dn_ini_res(sdo);
		return co_ssdo_abort_ind(sdo);
	} else {
		co_ssdo_send_dn_ini_res(sdo);
		if (sdo->timeout)
			can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);
		return co_ssdo_dn_seg_state;
	}
}

static co_ssdo_state_t *
co_ssdo_dn_seg_on_abort(co_ssdo_t *sdo, co_unsigned32_t ac)
{
	return co_ssdo_abort_res(sdo, ac);
}

static co_ssdo_state_t *
co_ssdo_dn_seg_on_time(co_ssdo_t *sdo, const struct timespec *tp)
{
	(void)tp;

	return co_ssdo_abort_res(sdo, CO_SDO_AC_TIMEOUT);
}

static co_ssdo_state_t *
co_ssdo_dn_seg_on_recv(co_ssdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);

	if (msg->len < 1)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	// Check the client command specifier.
	switch (cs & CO_SDO_CS_MASK) {
	case CO_SDO_CCS_DN_SEG_REQ: break;
	case CO_SDO_CS_ABORT: return co_ssdo_abort_ind(sdo);
	default: return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	}

	// Check the value of the toggle bit.
	if ((cs & CO_SDO_SEG_TOGGLE) != sdo->toggle)
		return co_ssdo_dn_seg_state;

	// Obtain the size of the segment.
	size_t n = CO_SDO_SEG_SIZE_GET(cs);
	if (msg->len < 1 + n)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	int last = !!(cs & CO_SDO_SEG_LAST);

	if (sdo->req.offset + sdo->req.nbyte + n > sdo->req.size)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_TYPE_LEN_HI);

	sdo->req.buf = msg->data + 1;
	sdo->req.offset += sdo->req.nbyte;
	sdo->req.nbyte = n;

	if (last && !co_sdo_req_last(&sdo->req))
		return co_ssdo_abort_res(sdo, CO_SDO_AC_TYPE_LEN_LO);

	co_unsigned32_t ac = co_ssdo_dn_ind(sdo);
	if (ac)
		return co_ssdo_abort_res(sdo, ac);

	co_ssdo_send_dn_seg_res(sdo);

	if (last) {
		return co_ssdo_abort_ind(sdo);
	} else {
		if (sdo->timeout)
			can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);
		return co_ssdo_dn_seg_state;
	}
}

static co_ssdo_state_t *
co_ssdo_up_ini_on_recv(co_ssdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);

	// Load the object index and sub-index from the CAN frame.
	if (msg->len < 3)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_OBJ);
	sdo->idx = ldle_u16(msg->data + 1);
	if (msg->len < 4)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_SUB);
	sdo->subidx = msg->data[3];

	trace("SSDO: %04X:%02X: received upload request", sdo->idx,
			sdo->subidx);

	// Perform access checks and start serializing the value.
	co_sdo_req_clear(&sdo->req);
	co_unsigned32_t ac = co_ssdo_up_ind(sdo);
	if (ac)
		return co_ssdo_abort_res(sdo, ac);

	if (sdo->req.size && sdo->req.size <= 4) {
		// Perform an expedited transfer.
		if ((ac = co_ssdo_up_buf(sdo, sdo->req.size)) != 0)
			return co_ssdo_abort_res(sdo, ac);
		co_ssdo_send_up_exp_res(sdo);
		return co_ssdo_abort_ind(sdo);
	} else {
		co_ssdo_send_up_ini_res(sdo);
		if (sdo->timeout)
			can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);
		return co_ssdo_up_seg_state;
	}
}

static co_ssdo_state_t *
co_ssdo_up_seg_on_abort(co_ssdo_t *sdo, co_unsigned32_t ac)
{
	return co_ssdo_abort_res(sdo, ac);
}

static co_ssdo_state_t *
co_ssdo_up_seg_on_time(co_ssdo_t *sdo, const struct timespec *tp)
{
	(void)tp;

	return co_ssdo_abort_res(sdo, CO_SDO_AC_TIMEOUT);
}

static co_ssdo_state_t *
co_ssdo_up_seg_on_recv(co_ssdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);

	if (msg->len < 1)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	// Check the client command specifier.
	switch (cs & CO_SDO_CS_MASK) {
	case CO_SDO_CCS_UP_SEG_REQ: break;
	case CO_SDO_CS_ABORT: return co_ssdo_abort_ind(sdo);
	default: return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	}

	// Check the value of the toggle bit.
	if ((cs & CO_SDO_SEG_TOGGLE) != sdo->toggle)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_TOGGLE);

	membuf_clear(&sdo->buf);
	co_unsigned32_t ac = co_ssdo_up_buf(sdo, 7);
	if (ac)
		return co_ssdo_abort_res(sdo, ac);

	int last = co_sdo_req_last(&sdo->req) && sdo->nbyte == sdo->req.nbyte;
	co_ssdo_send_up_seg_res(sdo, last);

	if (last) {
		// Finalize the transfer.
		return co_ssdo_abort_ind(sdo);
	} else {
		if (sdo->timeout)
			can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);
		return co_ssdo_up_seg_state;
	}
}

static co_ssdo_state_t *
co_ssdo_blk_dn_ini_on_recv(co_ssdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);

	assert(msg->len > 0);
	co_unsigned8_t cs = msg->data[0];

	// Check the client subcommand.
	if ((cs & 0x01) != CO_SDO_SC_INI_BLK)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);

	// Check if the client supports generating a CRC.
	sdo->gencrc = !!(cs & CO_SDO_BLK_CRC);

	// Load the object index and sub-index from the CAN frame.
	if (msg->len < 3)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_OBJ);
	sdo->idx = ldle_u16(msg->data + 1);
	if (msg->len < 4)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_SUB);
	sdo->subidx = msg->data[3];

	trace("SSDO: %04X:%02X: received block download request", sdo->idx,
			sdo->subidx);

	// Obtain the data set size.
	co_sdo_req_clear(&sdo->req);
	if (cs & CO_SDO_BLK_SIZE_IND) {
		// 0-pad the data bytes to handle clients which send CAN frames
		// less than 8 bytes.
		uint_least8_t data[4] = { 0 };
		memcpy(data, msg->data + 4, msg->len - 4);
		sdo->req.size = ldle_u32(data);
	}

	// Use the maximum block size by default.
	sdo->blksize = CO_SSDO_MAX_SEQNO;
	sdo->ackseq = 0;

	co_ssdo_send_blk_dn_ini_res(sdo);

	if (sdo->timeout)
		can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);
	return co_ssdo_blk_dn_sub_state;
}

static co_ssdo_state_t *
co_ssdo_blk_dn_sub_on_abort(co_ssdo_t *sdo, co_unsigned32_t ac)
{
	return co_ssdo_abort_res(sdo, ac);
}

static co_ssdo_state_t *
co_ssdo_blk_dn_sub_on_time(co_ssdo_t *sdo, const struct timespec *tp)
{
	(void)tp;

	return co_ssdo_abort_res(sdo, CO_SDO_AC_TIMEOUT);
}

static co_ssdo_state_t *
co_ssdo_blk_dn_sub_on_recv(co_ssdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);

	if (msg->len < 1)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	if (cs == CO_SDO_CS_ABORT)
		return co_ssdo_abort_ind(sdo);

	co_unsigned8_t seqno = cs & ~CO_SDO_SEQ_LAST;
	int last = !!(cs & CO_SDO_SEQ_LAST);

	if (!seqno || seqno > sdo->blksize)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_BLK_SEQ);

	// Only accept sequential segments. Dropped segments will be resent
	// after the confirmation message.
	if (seqno == sdo->ackseq + 1) {
		sdo->ackseq++;
		// Update the CRC.
		if (sdo->gencrc)
			sdo->crc = co_crc(
					sdo->crc, sdo->req.buf, sdo->req.nbyte);
		// Pass the previous frame to the download indication function.
		co_unsigned32_t ac = co_ssdo_dn_ind(sdo);
		if (ac)
			return co_ssdo_abort_res(sdo, ac);
		// Determine the number of bytes to copy.
		assert(sdo->req.size >= sdo->req.offset + sdo->req.nbyte);
		size_t n = MIN(sdo->req.size - sdo->req.offset - sdo->req.nbyte,
				7);
		if (!last && n < 7)
			return co_ssdo_abort_res(sdo, CO_SDO_AC_TYPE_LEN_HI);
		// Copy the new frame to the SDO request.
		membuf_clear(&sdo->buf);
		if (!membuf_reserve(&sdo->buf, n))
			return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_MEM);
		membuf_write(&sdo->buf, msg->data + 1, n);
		sdo->req.buf = membuf_begin(&sdo->buf);
		sdo->req.offset += sdo->req.nbyte;
		sdo->req.nbyte = membuf_size(&sdo->buf);
	}

	// If this is the last segment in the block, send a confirmation.
	if (seqno == sdo->blksize || last) {
		co_ssdo_send_blk_dn_sub_res(sdo);
		sdo->ackseq = 0;
	}

	if (sdo->timeout)
		can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);
	return last ? co_ssdo_blk_dn_end_state : co_ssdo_blk_dn_sub_state;
}

static co_ssdo_state_t *
co_ssdo_blk_dn_end_on_abort(co_ssdo_t *sdo, co_unsigned32_t ac)
{
	return co_ssdo_abort_res(sdo, ac);
}

static co_ssdo_state_t *
co_ssdo_blk_dn_end_on_time(co_ssdo_t *sdo, const struct timespec *tp)
{
	(void)tp;

	return co_ssdo_abort_res(sdo, CO_SDO_AC_TIMEOUT);
}

static co_ssdo_state_t *
co_ssdo_blk_dn_end_on_recv(co_ssdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);

	if (msg->len < 1)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	// Check the client command specifier.
	switch (cs & CO_SDO_CS_MASK) {
	case CO_SDO_CCS_BLK_DN_REQ: break;
	case CO_SDO_CS_ABORT: return co_ssdo_abort_ind(sdo);
	default: return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	}

	// Check the client subcommand.
	if ((cs & CO_SDO_SC_MASK) != CO_SDO_SC_END_BLK)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);

	// Check the total length.
	if (sdo->req.size != sdo->req.offset + sdo->req.nbyte)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_TYPE_LEN_LO);

	// Check the number of bytes in the last segment.
	co_unsigned8_t n = sdo->req.size ? (sdo->req.size - 1) % 7 + 1 : 0;
	if (CO_SDO_BLK_SIZE_GET(cs) != n)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);

	// Check the CRC.
	if (sdo->gencrc) {
		sdo->crc = co_crc(sdo->crc, sdo->req.buf, sdo->req.nbyte);
		co_unsigned16_t crc = ldle_u16(msg->data + 1);
		if (sdo->crc != crc)
			return co_ssdo_abort_res(sdo, CO_SDO_AC_BLK_CRC);
	}

	co_unsigned32_t ac = co_ssdo_dn_ind(sdo);
	if (ac)
		return co_ssdo_abort_res(sdo, ac);

	// Finalize the transfer.
	co_ssdo_send_blk_dn_end_res(sdo);
	return co_ssdo_abort_ind(sdo);
}

static co_ssdo_state_t *
co_ssdo_blk_up_ini_on_recv(co_ssdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);

	assert(msg->len > 0);
	co_unsigned8_t cs = msg->data[0];

	// Check the client subcommand.
	if ((cs & CO_SDO_SC_MASK) != CO_SDO_SC_INI_BLK)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);

	// Check if the client supports generating a CRC.
	sdo->gencrc = !!(cs & CO_SDO_BLK_CRC);

	// Load the object index and sub-index from the CAN frame.
	if (msg->len < 3)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_OBJ);
	sdo->idx = ldle_u16(msg->data + 1);
	if (msg->len < 4)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_SUB);
	sdo->subidx = msg->data[3];

	trace("SSDO: %04X:%02X: received block upload request", sdo->idx,
			sdo->subidx);

	// Load the number of segments per block.
	if (msg->len < 5)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_BLK_SIZE);
	sdo->blksize = msg->data[4];
	if (!sdo->blksize || sdo->blksize > CO_SSDO_MAX_SEQNO)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_BLK_SIZE);

	// Load the protocol switch threshold (PST).
	co_unsigned8_t pst = msg->len > 5 ? msg->data[5] : 0;

	// Perform access checks and start serializing the value.
	co_sdo_req_clear(&sdo->req);
	co_unsigned32_t ac = co_ssdo_up_ind(sdo);
	if (ac)
		return co_ssdo_abort_res(sdo, ac);

	if (pst && sdo->req.size <= pst) {
		// If the PST is non-zero, and the number of bytes is smaller
		// than or equal to the PST, switch to the SDO upload protocol.
		if (sdo->req.size <= 4) {
			// Perform an expedited transfer.
			if ((ac = co_ssdo_up_buf(sdo, sdo->req.size)) != 0)
				return co_ssdo_abort_res(sdo, ac);
			co_ssdo_send_up_exp_res(sdo);
			return co_ssdo_abort_ind(sdo);
		} else {
			co_ssdo_send_up_ini_res(sdo);
			if (sdo->timeout)
				can_timer_timeout(sdo->timer, sdo->net,
						sdo->timeout);
			return co_ssdo_up_seg_state;
		}
	} else {
		co_ssdo_send_blk_up_ini_res(sdo);
		if (sdo->timeout)
			can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);
		return co_ssdo_blk_up_sub_state;
	}
}

static co_ssdo_state_t *
co_ssdo_blk_up_sub_on_abort(co_ssdo_t *sdo, co_unsigned32_t ac)
{
	return co_ssdo_abort_res(sdo, ac);
}

static co_ssdo_state_t *
co_ssdo_blk_up_sub_on_time(co_ssdo_t *sdo, const struct timespec *tp)
{
	(void)tp;

	return co_ssdo_abort_res(sdo, CO_SDO_AC_TIMEOUT);
}

static co_ssdo_state_t *
co_ssdo_blk_up_sub_on_recv(co_ssdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);

	if (msg->len < 1)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	// Check the client command specifier.
	switch (cs & CO_SDO_CS_MASK) {
	case CO_SDO_CCS_BLK_UP_REQ: break;
	case CO_SDO_CS_ABORT: return co_ssdo_abort_ind(sdo);
	default: return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	}

	// Check the client subcommand.
	switch (cs & CO_SDO_SC_MASK) {
	case CO_SDO_SC_BLK_RES:
		if (co_sdo_req_first(&sdo->req) && !sdo->nbyte)
			return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);

		if (msg->len < 3)
			return co_ssdo_abort_res(sdo, CO_SDO_AC_BLK_SEQ);

		// Flush the successfully sent segments from the buffer.
		co_unsigned8_t ackseq = msg->data[1];
		membuf_flush(&sdo->buf, ackseq * 7);

		// Read the number of segments in the next block.
		sdo->blksize = msg->data[2];
		if (!sdo->blksize || sdo->blksize > CO_SSDO_MAX_SEQNO)
			return co_ssdo_abort_res(sdo, CO_SDO_AC_BLK_SIZE);

		break;
	case CO_SDO_SC_START_UP:
		if (!(co_sdo_req_first(&sdo->req) && !sdo->nbyte))
			return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);
		break;
	default: return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	}

	ptrdiff_t n = sdo->blksize * 7 - membuf_size(&sdo->buf);
	if (n > 0) {
		if (!membuf_reserve(&sdo->buf, n))
			return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_MEM);
		co_unsigned32_t ac = co_ssdo_up_buf(sdo, n);
		if (ac)
			return co_ssdo_abort_res(sdo, ac);
		sdo->blksize = (co_unsigned8_t)(
				(membuf_size(&sdo->buf) + 6) / 7);
	}
	int last = co_sdo_req_last(&sdo->req) && sdo->nbyte == sdo->req.nbyte;

	if (sdo->timeout)
		can_timer_timeout(sdo->timer, sdo->net, sdo->timeout);

	if (sdo->blksize) {
		// Send all segments in the current block.
		co_ssdo_send_blk_up_sub_res(sdo, last);
		return co_ssdo_blk_up_sub_state;
	} else {
		co_ssdo_send_blk_up_end_res(sdo);
		return co_ssdo_blk_up_end_state;
	}
}

static co_ssdo_state_t *
co_ssdo_blk_up_end_on_abort(co_ssdo_t *sdo, co_unsigned32_t ac)
{
	return co_ssdo_abort_res(sdo, ac);
}

static co_ssdo_state_t *
co_ssdo_blk_up_end_on_time(co_ssdo_t *sdo, const struct timespec *tp)
{
	(void)tp;

	return co_ssdo_abort_res(sdo, CO_SDO_AC_TIMEOUT);
}

static co_ssdo_state_t *
co_ssdo_blk_up_end_on_recv(co_ssdo_t *sdo, const struct can_msg *msg)
{
	assert(sdo);
	assert(msg);

	if (msg->len < 1)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	co_unsigned8_t cs = msg->data[0];

	// Check the client command specifier.
	switch (cs & CO_SDO_CS_MASK) {
	case CO_SDO_CCS_BLK_UP_REQ: break;
	case CO_SDO_CS_ABORT: return co_ssdo_abort_ind(sdo);
	default: return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);
	}

	// Check the client subcommand.
	if ((cs & CO_SDO_SC_MASK) != CO_SDO_SC_END_BLK)
		return co_ssdo_abort_res(sdo, CO_SDO_AC_NO_CS);

	return co_ssdo_abort_ind(sdo);
}

static co_ssdo_state_t *
co_ssdo_abort_ind(co_ssdo_t *sdo)
{
	assert(sdo);

	if (sdo->timeout)
		can_timer_stop(sdo->timer);

	sdo->idx = 0;
	sdo->subidx = 0;

	sdo->toggle = 0;
	sdo->blksize = 0;
	sdo->ackseq = 0;
	sdo->gencrc = 0;
	sdo->crc = 0;

	co_sdo_req_clear(&sdo->req);
	membuf_clear(&sdo->buf);
	sdo->nbyte = 0;

	return co_ssdo_wait_state;
}

static co_ssdo_state_t *
co_ssdo_abort_res(co_ssdo_t *sdo, co_unsigned32_t ac)
{
#if !LELY_NO_STDIO
	trace("SSDO: abort code %08" PRIX32 " (%s)", ac, co_sdo_ac2str(ac));
#endif
	co_ssdo_send_abort(sdo, ac);
	return co_ssdo_abort_ind(sdo);
}

static co_unsigned32_t
co_ssdo_dn_ind(co_ssdo_t *sdo)
{
	assert(sdo);

	// Find the object in the object dictionary.
	co_obj_t *obj = co_dev_find_obj(sdo->dev, sdo->idx);
	if (!obj)
		return CO_SDO_AC_NO_OBJ;

	// Find the sub-object.
	co_sub_t *sub = co_obj_find_sub(obj, sdo->subidx);
	if (!sub)
		return CO_SDO_AC_NO_SUB;

	return co_sub_dn_ind(sub, &sdo->req);
}

static co_unsigned32_t
co_ssdo_up_ind(co_ssdo_t *sdo)
{
	assert(sdo);

	// Find the object in the object dictionary.
	const co_obj_t *obj = co_dev_find_obj(sdo->dev, sdo->idx);
	if (!obj)
		return CO_SDO_AC_NO_OBJ;

	// Find the sub-object.
	const co_sub_t *sub = co_obj_find_sub(obj, sdo->subidx);
	if (!sub)
		return CO_SDO_AC_NO_SUB;

	// If the object is an array, check whether the element exists.
	if (co_obj_get_code(obj) == CO_OBJECT_ARRAY
			&& sdo->subidx > co_obj_get_val_u8(obj, 0x00))
		return CO_SDO_AC_NO_DATA;

	sdo->nbyte = 0;
	return co_sub_up_ind(sub, &sdo->req);
}

static co_unsigned32_t
co_ssdo_up_buf(co_ssdo_t *sdo, size_t nbyte)
{
	co_unsigned32_t ac = 0;

	if (nbyte && !membuf_reserve(&sdo->buf, nbyte))
		return CO_SDO_AC_NO_MEM;

	while (nbyte) {
		if (sdo->nbyte >= sdo->req.nbyte) {
			if (co_sdo_req_last(&sdo->req)
					|| (ac = co_ssdo_up_ind(sdo)))
				break;
			sdo->nbyte = 0;
		}
		const uint_least8_t *src = (const uint_least8_t *)sdo->req.buf
				+ sdo->nbyte;
		size_t n = MIN(nbyte, sdo->req.nbyte - sdo->nbyte);

		if (sdo->gencrc)
			sdo->crc = co_crc(sdo->crc, src, n);

		membuf_write(&sdo->buf, src, n);
		nbyte -= n;
		sdo->nbyte += n;
	}

	return ac;
}

static void
co_ssdo_send_abort(co_ssdo_t *sdo, co_unsigned32_t ac)
{
	assert(sdo);

	struct can_msg msg;
	co_ssdo_init_ini_res(sdo, &msg, CO_SDO_CS_ABORT);
	stle_u32(msg.data + 4, ac);
	can_net_send(sdo->net, &msg);
}

static void
co_ssdo_send_dn_ini_res(co_ssdo_t *sdo)
{
	assert(sdo);

	co_unsigned8_t cs = CO_SDO_SCS_DN_INI_RES;

	struct can_msg msg;
	co_ssdo_init_ini_res(sdo, &msg, cs);
	can_net_send(sdo->net, &msg);
}

static void
co_ssdo_send_dn_seg_res(co_ssdo_t *sdo)
{
	assert(sdo);

	co_unsigned8_t cs = CO_SDO_SCS_DN_SEG_RES | sdo->toggle;
	sdo->toggle ^= CO_SDO_SEG_TOGGLE;

	struct can_msg msg;
	co_ssdo_init_seg_res(sdo, &msg, cs);
	can_net_send(sdo->net, &msg);
}

static void
co_ssdo_send_up_exp_res(co_ssdo_t *sdo)
{
	assert(sdo);
	assert(sdo->req.size && sdo->req.size <= 4);

	const char *buf = membuf_begin(&sdo->buf);
	size_t nbyte = membuf_size(&sdo->buf);
	assert(nbyte == sdo->req.size);

	co_unsigned8_t cs =
			CO_SDO_SCS_UP_INI_RES | CO_SDO_INI_SIZE_EXP_SET(nbyte);

	struct can_msg msg;
	co_ssdo_init_ini_res(sdo, &msg, cs);
	memcpy(msg.data + 4, buf, nbyte);
	can_net_send(sdo->net, &msg);
}

static void
co_ssdo_send_up_ini_res(co_ssdo_t *sdo)
{
	assert(sdo);
	assert(!sdo->req.size || sdo->req.size > 4);

	co_unsigned8_t cs = CO_SDO_SCS_UP_INI_RES | CO_SDO_INI_SIZE_IND;

	struct can_msg msg;
	co_ssdo_init_ini_res(sdo, &msg, cs);
	stle_u32(msg.data + 4, sdo->req.size);
	can_net_send(sdo->net, &msg);
}

static void
co_ssdo_send_up_seg_res(co_ssdo_t *sdo, int last)
{
	assert(sdo);
	assert(!sdo->req.size || sdo->req.size > 4);

	const char *buf = membuf_begin(&sdo->buf);
	size_t nbyte = membuf_size(&sdo->buf);
	assert(nbyte <= 7);

	co_unsigned8_t cs = CO_SDO_SCS_UP_SEG_RES | sdo->toggle
			| CO_SDO_SEG_SIZE_SET(nbyte);
	sdo->toggle ^= CO_SDO_SEG_TOGGLE;
	if (last)
		cs |= CO_SDO_SEG_LAST;

	struct can_msg msg;
	co_ssdo_init_seg_res(sdo, &msg, cs);
	memcpy(msg.data + 1, buf, nbyte);
	can_net_send(sdo->net, &msg);
}

static void
co_ssdo_send_blk_dn_ini_res(co_ssdo_t *sdo)
{
	assert(sdo);

	co_unsigned8_t cs = CO_SDO_SCS_BLK_DN_RES | CO_SDO_BLK_CRC
			| CO_SDO_SC_INI_BLK;

	struct can_msg msg;
	co_ssdo_init_ini_res(sdo, &msg, cs);
	msg.data[4] = sdo->blksize;
	can_net_send(sdo->net, &msg);
}

static void
co_ssdo_send_blk_dn_sub_res(co_ssdo_t *sdo)
{
	assert(sdo);

	co_unsigned8_t cs = CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_BLK_RES;

	struct can_msg msg;
	co_ssdo_init_seg_res(sdo, &msg, cs);
	msg.data[1] = sdo->ackseq;
	msg.data[2] = sdo->blksize;
	can_net_send(sdo->net, &msg);
}

static void
co_ssdo_send_blk_dn_end_res(co_ssdo_t *sdo)
{
	assert(sdo);

	co_unsigned8_t cs = CO_SDO_SCS_BLK_DN_RES | CO_SDO_SC_END_BLK;

	struct can_msg msg;
	co_ssdo_init_seg_res(sdo, &msg, cs);
	can_net_send(sdo->net, &msg);
}

static void
co_ssdo_send_blk_up_ini_res(co_ssdo_t *sdo)
{
	assert(sdo);

	co_unsigned8_t cs = CO_SDO_SCS_BLK_UP_RES | CO_SDO_BLK_CRC
			| CO_SDO_BLK_SIZE_IND | CO_SDO_SC_INI_BLK;

	struct can_msg msg;
	co_ssdo_init_ini_res(sdo, &msg, cs);
	stle_u32(msg.data + 4, sdo->req.size);
	can_net_send(sdo->net, &msg);
}

static void
co_ssdo_send_blk_up_sub_res(co_ssdo_t *sdo, int last)
{
	assert(sdo);

	const char *buf = membuf_begin(&sdo->buf);
	size_t nbyte = membuf_size(&sdo->buf);

	for (co_unsigned8_t seqno = 1; seqno <= sdo->blksize;
			seqno++, buf += 7, nbyte -= 7) {
		co_unsigned8_t cs = seqno;
		if (last && nbyte <= 7)
			cs |= CO_SDO_SEQ_LAST;

		struct can_msg msg;
		co_ssdo_init_seg_res(sdo, &msg, cs);
		memcpy(msg.data + 1, buf, MIN(nbyte, 7));
		can_net_send(sdo->net, &msg);
	}
}

static void
co_ssdo_send_blk_up_end_res(co_ssdo_t *sdo)
{
	assert(sdo);

	// Compute the number of bytes in the last segment containing data.
	co_unsigned8_t n = sdo->req.size ? (sdo->req.size - 1) % 7 + 1 : 0;

	co_unsigned8_t cs = CO_SDO_SCS_BLK_UP_RES | CO_SDO_SC_END_BLK
			| CO_SDO_BLK_SIZE_SET(n);

	struct can_msg msg;
	co_ssdo_init_seg_res(sdo, &msg, cs);
	stle_u16(msg.data + 1, sdo->crc);
	can_net_send(sdo->net, &msg);
}

static void
co_ssdo_init_ini_res(co_ssdo_t *sdo, struct can_msg *msg, co_unsigned8_t cs)
{
	assert(sdo);
	assert(msg);

	*msg = (struct can_msg)CAN_MSG_INIT;
	msg->id = sdo->par.cobid_res;
	if (sdo->par.cobid_res & CO_SDO_COBID_FRAME) {
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
co_ssdo_init_seg_res(co_ssdo_t *sdo, struct can_msg *msg, co_unsigned8_t cs)
{
	assert(sdo);
	assert(msg);

	*msg = (struct can_msg)CAN_MSG_INIT;
	msg->id = sdo->par.cobid_res;
	if (sdo->par.cobid_res & CO_SDO_COBID_FRAME) {
		msg->id &= CAN_MASK_EID;
		msg->flags |= CAN_FLAG_IDE;
	} else {
		msg->id &= CAN_MASK_BID;
	}
	msg->len = CAN_MAX_LEN;
	msg->data[0] = cs;
}
