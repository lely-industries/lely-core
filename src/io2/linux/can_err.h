/**@file
 * This is the internal header file of the SocketCAN error frame conversion
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

#ifndef LELY_IO2_INTERN_LINUX_CAN_ERR_H_
#define LELY_IO2_INTERN_LINUX_CAN_ERR_H_

#include "io.h"

#ifdef __linux__

#include <lely/io2/can/err.h>
#include <lely/util/util.h>

#include <assert.h>
#include <errno.h>

#include <linux/can.h>
#include <linux/can/error.h>

#ifdef __cplusplus
extern "C" {
#endif

static int can_frame2can_err(
		const struct can_frame *frame, struct can_err *err);

static inline int
can_frame2can_err(const struct can_frame *frame, struct can_err *err)
{
	assert(frame);

	if (!(frame->can_id & CAN_ERR_FLAG))
		return 0;

	if (frame->can_dlc != CAN_ERR_DLC) {
		errno = EINVAL;
		return -1;
	}

	enum can_state state = err ? err->state : CAN_STATE_ACTIVE;
	enum can_error error = err ? err->error : 0;

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

	if (err) {
		err->state = state;
		err->error = error;
	}

	return 1;
}

#ifdef __cplusplus
}
#endif

#endif // __linux__

#endif // !LELY_IO2_INTERN_LINUX_CAN_ERR_H_
