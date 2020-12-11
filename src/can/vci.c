/**@file
 * This file is part of the CAN library; it contains the implementation of the
 * IXXAT VCI V4 interface.
 *
 * @see lely/can/vci.h
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

// Rename error flags to avoid conflicts with definitions in <cantype.h>.
#define CAN_ERROR_BIT _CAN_ERROR_BIT
#define CAN_ERROR_STUFF _CAN_ERROR_STUFF
#define CAN_ERROR_CRC _CAN_ERROR_CRC
#define CAN_ERROR_FORM _CAN_ERROR_FORM
#define CAN_ERROR_ACK _CAN_ERROR_ACK
#define CAN_ERROR_OTHER _CAN_ERROR_OTHER

#include "can.h"

#undef CAN_ERROR_OTHER
#undef CAN_ERROR_ACK
#undef CAN_ERROR_FORM
#undef CAN_ERROR_CRC
#undef CAN_ERROR_STUFF
#undef CAN_ERROR_BIT

#if !LELY_NO_STDIO && LELY_HAVE_VCI

#include <lely/can/vci.h>
#include <lely/util/endian.h>
#include <lely/util/errnum.h>

#include <assert.h>
#include <string.h>

#ifdef HAVE_CANTYPE_H
#include <cantype.h>
#endif

int
CANMSG_is_error(const void *msg, enum can_state *pstate, enum can_error *perror)
{
	const CANMSG *msg_ = msg;
	assert(msg);

	if (msg_->uMsgInfo.Bits.type != CAN_MSGTYPE_ERROR)
		return 0;

	enum can_state state = pstate ? *pstate : 0;
	enum can_error error = perror ? *perror : 0;

	switch (msg_->abData[0]) {
	case CAN_ERROR_STUFF: error |= _CAN_ERROR_STUFF; break;
	case CAN_ERROR_FORM: error |= _CAN_ERROR_FORM; break;
	case CAN_ERROR_ACK: error |= _CAN_ERROR_ACK; break;
	case CAN_ERROR_BIT: error |= _CAN_ERROR_BIT; break;
	case CAN_ERROR_CRC: error |= _CAN_ERROR_CRC; break;
	case CAN_ERROR_OTHER: error |= _CAN_ERROR_OTHER; break;
	}

	if (msg_->abData[1] & CAN_STATUS_BUSOFF) {
		state = CAN_STATE_BUSOFF;
	} else if (msg_->abData[1] & CAN_STATUS_ERRLIM) {
		state = CAN_STATE_PASSIVE;
	} else {
		state = CAN_STATE_ACTIVE;
	}

	if (pstate)
		*pstate = state;

	if (perror)
		*perror = error;

	return 1;
}

int
CANMSG2can_msg(const void *src, struct can_msg *dst)
{
	const CANMSG *msg = src;
	assert(msg);
	assert(dst);

	if (msg->uMsgInfo.Bits.type != CAN_MSGTYPE_DATA) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	memset(dst, 0, sizeof(*dst));
	dst->flags = 0;
	if (msg->uMsgInfo.Bits.ext) {
		dst->id = letoh32(msg->dwMsgId) & CAN_MASK_EID;
		dst->flags |= CAN_FLAG_IDE;
	} else {
		dst->id = letoh32(msg->dwMsgId) & CAN_MASK_BID;
	}
	if (msg->uMsgInfo.Bits.rtr)
		dst->flags |= CAN_FLAG_RTR;
	dst->len = MIN(msg->uMsgInfo.Bits.dlc, CAN_MAX_LEN);
	if (!(dst->flags & CAN_FLAG_RTR))
		memcpy(dst->data, msg->abData, dst->len);

	return 0;
}

int
can_msg2CANMSG(const struct can_msg *src, void *dst)
{
	assert(src);
	CANMSG *msg = dst;
	assert(msg);

#if !LELY_NO_CANFD
	if (src->flags & CAN_FLAG_EDL) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
#endif

	memset(msg, 0, sizeof(*msg));
	msg->dwTime = 0;
	msg->uMsgInfo.Bits.type = CAN_MSGTYPE_DATA;
	if (src->flags & CAN_FLAG_IDE) {
		msg->dwMsgId = htole32(src->id & CAN_MASK_EID);
		msg->uMsgInfo.Bits.ext = 1;
	} else {
		msg->dwMsgId = htole32(src->id & CAN_MASK_BID);
	}
	msg->uMsgInfo.Bits.dlc = MIN(src->len, CAN_MAX_LEN);
	if (src->flags & CAN_FLAG_RTR)
		msg->uMsgInfo.Bits.rtr = 1;
	else
		memcpy(msg->abData, src->data, msg->uMsgInfo.Bits.dlc);

	return 0;
}

#endif // !LELY_NO_STDIO && LELY_HAVE_VCI
