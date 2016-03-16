/*!\file
 * This file is part of the CAN library; it contains the implementation of the
 * SocketCAN interface.
 *
 * \see lely/can/socket.h
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

#include "can.h"

#ifdef LELY_HAVE_SOCKET_CAN

#include <lely/can/socket.h>

#include <assert.h>
#include <string.h>

#include <linux/can.h>

LELY_CAN_EXPORT void
can_frame2can_msg(const struct can_frame *frame, struct can_msg *msg)
{
	assert(frame);
	assert(!(frame->can_id & CAN_ERR_FLAG));
	assert(msg);

	memset(msg, 0, sizeof(*msg));
	msg->flags = 0;
	if (frame->can_id & CAN_EFF_FLAG) {
		msg->id = frame->can_id & CAN_EFF_MASK;
		msg->flags |= CAN_FLAG_IDE;
	} else {
		msg->id = frame->can_id & CAN_SFF_MASK;
	}
	if (frame->can_id & CAN_RTR_FLAG)
		msg->flags |= CAN_FLAG_RTR;
	msg->len = MIN(frame->can_dlc, CAN_MAX_LEN);
	if (!(msg->flags & CAN_FLAG_RTR))
		memcpy(msg->data, frame->data, msg->len);
}

LELY_CAN_EXPORT void
can_msg2can_frame(const struct can_msg *msg, struct can_frame *frame)
{
	assert(msg);
#ifndef LELY_NO_CANFD
	assert(!(msg->flags & CAN_FLAG_EDL));
#endif
	assert(frame);

	memset(frame, 0, sizeof(*frame));
	frame->can_id = msg->id;
	if (msg->flags & CAN_FLAG_IDE) {
		frame->can_id &= CAN_EFF_MASK;
		frame->can_id |= CAN_EFF_FLAG;
	} else {
		frame->can_id &= CAN_SFF_MASK;
	}
	frame->can_dlc = MIN(msg->len, CAN_MAX_LEN);
	if (msg->flags & CAN_FLAG_RTR)
		frame->can_id |= CAN_RTR_FLAG;
	else
		memcpy(frame->data, msg->data, frame->can_dlc);
}

#if !defined(LELY_NO_CANFD) && defined(CANFD_MTU)

LELY_CAN_EXPORT void
canfd_frame2can_msg(const struct canfd_frame *frame, struct can_msg *msg)
{
	assert(frame);
	assert(!(frame->can_id & CAN_ERR_FLAG));
	assert(msg);

	memset(msg, 0, sizeof(*msg));
	msg->flags = CAN_FLAG_EDL;
	if (frame->can_id & CAN_EFF_FLAG) {
		msg->id = frame->can_id & CAN_EFF_MASK;
		msg->flags |= CAN_FLAG_IDE;
	} else {
		msg->id = frame->can_id & CAN_SFF_MASK;
	}
	if (frame->flags & CANFD_BRS)
		msg->flags |= CAN_FLAG_BRS;
	if (frame->flags & CANFD_ESI)
		msg->flags |= CAN_FLAG_ESI;
	msg->len = MIN(frame->len, CANFD_MAX_LEN);
	memcpy(msg->data, frame->data, msg->len);
}

LELY_CAN_EXPORT void
can_msg2canfd_frame(const struct can_msg *msg, struct canfd_frame *frame)
{
	assert(msg);
	assert(msg->flags & CAN_FLAG_EDL);
	assert(frame);

	memset(frame, 0, sizeof(*frame));
	frame->can_id = msg->id;
	if (msg->flags & CAN_FLAG_IDE) {
		frame->can_id &= CAN_EFF_MASK;
		frame->can_id |= CAN_EFF_FLAG;
	} else {
		frame->can_id &= CAN_SFF_MASK;
	}
	frame->flags = 0;
	if (msg->flags & CAN_FLAG_BRS)
		frame->flags |= CANFD_BRS;
	if (msg->flags & CAN_FLAG_ESI)
		frame->flags |= CANFD_ESI;
	frame->len = MIN(msg->len, CANFD_MAX_LEN);
	memcpy(frame->data, msg->data, frame->len);
}

#endif // !LELY_NO_CANFD && CANFD_MTU

#endif // LELY_HAVE_SOCKET_CAN

