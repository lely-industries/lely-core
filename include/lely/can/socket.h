/*!\file
 * This header file is part of the CAN library; it contains the SocketCAN
 * interface declarations.
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

#ifndef LELY_CAN_SOCKET_H
#define LELY_CAN_SOCKET_H

#include <lely/can/msg.h>

// The SocketCAN CAN frame struct from <linux/can.h>
struct can_frame;

#ifndef LELY_NO_CANFD
// The SocketCAN CAN FD frame struct from <linux/can.h>
struct canfd_frame;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Converts a SocketCAN CAN frame to a #can_msg frame.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see can_msg2can_frame()
 */
LELY_CAN_EXTERN int can_frame2can_msg(const struct can_frame *src,
		struct can_msg *dst);

/*!
 * Converts a #can_msg frame to a SocketCAN CAN frame.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see can_frame2can_msg()
 */
LELY_CAN_EXTERN int can_msg2can_frame(const struct can_msg *src,
		struct can_frame *dst);

#ifndef LELY_NO_CANFD

/*!
 * Converts a SocketCAN CAN FD frame to a #can_msg frame.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see can_msg2canfd_frame()
 */
LELY_CAN_EXTERN int canfd_frame2can_msg(const struct canfd_frame *src,
		struct can_msg *dst);

/*!
 * Converts a #can_msg frame to a SocketCAN CAN FD frame.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see canfd_frame2can_msg()
 */
LELY_CAN_EXTERN int can_msg2canfd_frame(const struct can_msg *src,
		struct canfd_frame *dst);

#endif // !LELY_NO_CANFD

#ifdef __cplusplus
}
#endif

#endif

