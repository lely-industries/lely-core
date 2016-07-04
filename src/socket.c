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

#include <lely/util/errnum.h>
#include <lely/can/socket.h>

#include <assert.h>
#include <string.h>

#include <linux/can.h>

LELY_CAN_EXPORT int
can_frame2can_msg(const struct can_frame *src, struct can_msg *dst)
{
	assert(src);
	assert(dst);

	if (__unlikely(src->can_id & CAN_ERR_FLAG)) {
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
	if (!(dst->flags & CAN_FLAG_RTR))
		memcpy(dst->data, src->data, dst->len);

	return 0;
}

LELY_CAN_EXPORT int
can_msg2can_frame(const struct can_msg *src, struct can_frame *dst)
{
	assert(src);
	assert(dst);

#ifndef LELY_NO_CANFD
	if (__unlikely(src->flags & CAN_FLAG_EDL)) {
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

#if !defined(LELY_NO_CANFD) && defined(CANFD_MTU)

LELY_CAN_EXPORT int
canfd_frame2can_msg(const struct canfd_frame *src, struct can_msg *dst)
{
	assert(src);
	assert(dst);

	if (__unlikely(src->can_id & CAN_ERR_FLAG)) {
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

LELY_CAN_EXPORT int
can_msg2canfd_frame(const struct can_msg *src, struct canfd_frame *dst)
{
	assert(src);
	assert(dst);

	if (__unlikely(!(src->flags & CAN_FLAG_EDL))) {
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

#endif // LELY_HAVE_SOCKET_CAN

