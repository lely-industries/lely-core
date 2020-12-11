/**@file
 * This file is part of the CAN library; it contains the implementation of the
 * SocketCAN interface.
 *
 * @see lely/can/socket.h
 *
 * @copyright 2015-2020 Lely Industries N.V.
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

#include "can.h"

#if !LELY_NO_STDIO && LELY_HAVE_SOCKET_CAN

#include <lely/can/socket.h>
#include <lely/util/errnum.h>
#include <lely/util/util.h>

#include <assert.h>
#include <string.h>

#ifdef HAVE_LINUX_CAN_H
#include <linux/can.h>
#endif
#ifdef HAVE_LINUX_CAN_ERROR_H
#include <linux/can/error.h>
#endif

int
can_frame_is_error(const struct can_frame *frame, enum can_state *pstate,
		enum can_error *perror)
{
	assert(frame);

	if (!(frame->can_id & CAN_ERR_FLAG))
		return 0;

	enum can_state state = pstate ? *pstate : CAN_STATE_ACTIVE;
	enum can_error error = perror ? *perror : 0;

#ifdef HAVE_LINUX_CAN_ERROR_H
	if (frame->can_dlc != CAN_ERR_DLC) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	if (frame->can_id & CAN_ERR_RESTARTED)
		state = CAN_STATE_ACTIVE;

	if (frame->can_id & CAN_ERR_TX_TIMEOUT)
		error |= CAN_ERROR_OTHER;

	if (frame->can_id & CAN_ERR_CRTL) {
#ifdef CAN_ERR_CRTL_ACTIVE
		if (frame->data[1] & CAN_ERR_CRTL_ACTIVE)
			state = CAN_STATE_ACTIVE;
#endif
		// clang-format off
		if (frame->data[1] & (CAN_ERR_CRTL_RX_PASSIVE
				| CAN_ERR_CRTL_TX_PASSIVE))
			// clang-format on
			state = CAN_STATE_PASSIVE;
	}

	if (frame->can_id & CAN_ERR_PROT) {
		if (frame->data[2] & CAN_ERR_PROT_BIT)
			error |= CAN_ERROR_BIT;
		if (frame->data[2] & CAN_ERR_PROT_FORM)
			error |= CAN_ERROR_FORM;
		if (frame->data[2] & CAN_ERR_PROT_STUFF)
			error |= CAN_ERROR_STUFF;
		// clang-format off
		if (frame->data[2] & (CAN_ERR_PROT_BIT0 | CAN_ERR_PROT_BIT1
				| CAN_ERR_PROT_OVERLOAD))
			// clang-format on
			error |= CAN_ERROR_OTHER;
		if (frame->data[2] & CAN_ERR_PROT_ACTIVE)
			state = CAN_STATE_ACTIVE;
		if (frame->data[3] & CAN_ERR_PROT_LOC_CRC_SEQ)
			error |= CAN_ERROR_CRC;
	}

	if ((frame->can_id & CAN_ERR_TRX) && frame->data[4])
		error |= CAN_ERROR_OTHER;

	if (frame->can_id & CAN_ERR_ACK)
		error |= CAN_ERROR_ACK;

	if (frame->can_id & CAN_ERR_BUSOFF)
		state = CAN_STATE_BUSOFF;
#endif // HAVE_LINUX_CAN_ERROR_H

	if (pstate)
		*pstate = state;

	if (perror)
		*perror = error;

	return 1;
}

int
can_frame2can_msg(const struct can_frame *src, struct can_msg *dst)
{
	assert(src);
	assert(dst);

	if (src->can_id & CAN_ERR_FLAG) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	memset(dst, 0, sizeof(*dst));
	dst->flags = 0;
	if (src->can_id & CAN_EFF_FLAG) {
		dst->id = src->can_id & CAN_EFF_MASK;
		dst->flags |= CAN_FLAG_IDE;
	} else {
		dst->id = src->can_id & CAN_SFF_MASK;
	}
	if (src->can_id & CAN_RTR_FLAG)
		dst->flags |= CAN_FLAG_RTR;
	dst->len = MIN(src->can_dlc, CAN_MAX_LEN);
	// cppcheck-suppress knownConditionTrueFalse
	if (!(dst->flags & CAN_FLAG_RTR))
		memcpy(dst->data, src->data, dst->len);

	return 0;
}

int
can_msg2can_frame(const struct can_msg *src, struct can_frame *dst)
{
	assert(src);
	assert(dst);

#if !LELY_NO_CANFD
	if (src->flags & CAN_FLAG_EDL) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
#endif

	memset(dst, 0, sizeof(*dst));
	dst->can_id = src->id;
	if (src->flags & CAN_FLAG_IDE) {
		dst->can_id &= CAN_EFF_MASK;
		dst->can_id |= CAN_EFF_FLAG;
	} else {
		dst->can_id &= CAN_SFF_MASK;
	}
	dst->can_dlc = MIN(src->len, CAN_MAX_LEN);
	if (src->flags & CAN_FLAG_RTR)
		dst->can_id |= CAN_RTR_FLAG;
	else
		memcpy(dst->data, src->data, dst->can_dlc);

	return 0;
}

#if !LELY_NO_CANFD && defined(CANFD_MTU)

int
canfd_frame2can_msg(const struct canfd_frame *src, struct can_msg *dst)
{
	assert(src);
	assert(dst);

	if (src->can_id & CAN_ERR_FLAG) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	memset(dst, 0, sizeof(*dst));
	dst->flags = CAN_FLAG_EDL;
	if (src->can_id & CAN_EFF_FLAG) {
		dst->id = src->can_id & CAN_EFF_MASK;
		dst->flags |= CAN_FLAG_IDE;
	} else {
		dst->id = src->can_id & CAN_SFF_MASK;
	}
	if (src->flags & CANFD_BRS)
		dst->flags |= CAN_FLAG_BRS;
	if (src->flags & CANFD_ESI)
		dst->flags |= CAN_FLAG_ESI;
	dst->len = MIN(src->len, CANFD_MAX_LEN);
	memcpy(dst->data, src->data, dst->len);

	return 0;
}

int
can_msg2canfd_frame(const struct can_msg *src, struct canfd_frame *dst)
{
	assert(src);
	assert(dst);

	if (!(src->flags & CAN_FLAG_EDL)) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	memset(dst, 0, sizeof(*dst));
	dst->can_id = src->id;
	if (src->flags & CAN_FLAG_IDE) {
		dst->can_id &= CAN_EFF_MASK;
		dst->can_id |= CAN_EFF_FLAG;
	} else {
		dst->can_id &= CAN_SFF_MASK;
	}
	dst->flags = 0;
	if (src->flags & CAN_FLAG_BRS)
		dst->flags |= CANFD_BRS;
	if (src->flags & CAN_FLAG_ESI)
		dst->flags |= CANFD_ESI;
	dst->len = MIN(src->len, CANFD_MAX_LEN);
	memcpy(dst->data, src->data, dst->len);

	return 0;
}

#endif // !LELY_NO_CANFD && CANFD_MTU

#endif // !LELY_NO_STDIO && LELY_HAVE_SOCKET_CAN
