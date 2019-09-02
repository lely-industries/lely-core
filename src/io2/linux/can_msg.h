/**@file
 * This is the internal header file of the SocketCAN CAN frame conversion
 * functions.
 *
 * @copyright 2015-2019 Lely Industries N.V.
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

#ifndef LELY_IO2_INTERN_LINUX_CAN_MSG_H_
#define LELY_IO2_INTERN_LINUX_CAN_MSG_H_

#include "io.h"

#ifdef __linux__

#include <lely/io2/can/msg.h>

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <linux/can.h>

#ifdef __cplusplus
extern "C" {
#endif

static int can_frame2can_msg(const struct can_frame *src, struct can_msg *dst);
static int can_msg2can_frame(const struct can_msg *src, struct can_frame *dst);
#if !LELY_NO_CANFD
static int canfd_frame2can_msg(
		const struct canfd_frame *src, struct can_msg *dst);
static int can_msg2canfd_frame(
		const struct can_msg *src, struct canfd_frame *dst);
#endif

static inline int
can_frame2can_msg(const struct can_frame *src, struct can_msg *dst)
{
	assert(src);
	assert(dst);

	if (src->can_id & CAN_ERR_FLAG) {
		errno = EINVAL;
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

static inline int
can_msg2can_frame(const struct can_msg *src, struct can_frame *dst)
{
	assert(src);
	assert(dst);

#if !LELY_NO_CANFD
	if (src->flags & CAN_FLAG_FDF) {
		errno = EINVAL;
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

#if !LELY_NO_CANFD

static inline int
canfd_frame2can_msg(const struct canfd_frame *src, struct can_msg *dst)
{
	assert(src);
	assert(dst);

	if (src->can_id & CAN_ERR_FLAG) {
		errno = EINVAL;
		return -1;
	}

	memset(dst, 0, sizeof(*dst));
	dst->flags = CAN_FLAG_FDF;
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

static inline int
can_msg2canfd_frame(const struct can_msg *src, struct canfd_frame *dst)
{
	assert(src);
	assert(dst);

	if (!(src->flags & CAN_FLAG_FDF)) {
		errno = EINVAL;
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

#endif // !LELY_NO_CANFD

#ifdef __cplusplus
}
#endif

#endif // __linux__

#endif // !LELY_IO2_INTERN_LINUX_CAN_MSG_H_
