/*!\file
 * This header file is part of the CAN library; it contains the IXXAT VCI V4
 * interface declarations.
 *
 * \copyright 2017 Lely Industries N.V.
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

#ifndef LELY_CAN_VCI_H
#define LELY_CAN_VCI_H

#include <lely/can/msg.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Converts an IXXAT VCI CAN message to a #can_msg frame.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see can_msg2CANMSG()
 */
LELY_CAN_EXTERN int CANMSG2can_msg(const void *src, struct can_msg *dst);

/*!
 * Converts a #can_msg frame to an IXXAT VCI CAN message.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 *
 * \see CANMSG2can_msg()
 */
LELY_CAN_EXTERN int can_msg2CANMSG(const struct can_msg *src, void *dst);

#ifdef __cplusplus
}
#endif

#endif

