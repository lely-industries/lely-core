/**@file
 * This header file is part of the CAN library; it contains the IXXAT VCI V4
 * interface declarations.
 *
 * @copyright 2016-2019 Lely Industries N.V.
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

#ifndef LELY_CAN_VCI_H_
#define LELY_CAN_VCI_H_

#include <lely/can/msg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Checks if an IXXAT VCI CAN message is an error message and parses the bus
 * state and error flags if it is.
 *
 * @param msg    a pointer to an IXXAT VCI CAN message.
 * @param pstate the address at which to store the updated CAN node state (can
 *               be NULL). The value is only updated if the error frame
 *               indicates a state change.
 * @param perror the address at which to store the updated CAN bus errors (can
 *               be NULL). Any new error flags indicated by by the error frame
 *               are set in *<b>perror</b>, but existing flags are not cleared.
 *
 * @returns 1 if the CAN message is an error message, 0 if not, and -1 on error.
 * In the latter case, the error number can be obtained with get_errc().
 */
int CANMSG_is_error(const void *msg, enum can_state *pstate,
		enum can_error *perror);

/**
 * Converts an IXXAT VCI CAN message to a #can_msg frame.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see can_msg2CANMSG()
 */
int CANMSG2can_msg(const void *src, struct can_msg *dst);

/**
 * Converts a #can_msg frame to an IXXAT VCI CAN message.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see CANMSG2can_msg()
 */
int can_msg2CANMSG(const struct can_msg *src, void *dst);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CAN_VCI_H_
