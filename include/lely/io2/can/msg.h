/**@file
 * This header file is part of the I/O library; it contains the CAN frame
 * declarations.
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

#ifndef LELY_IO2_CAN_MSG_H_
#define LELY_IO2_CAN_MSG_H_

#include <lely/can/msg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compares two CAN or CAN FD format frames.
 *
 * @returns an integer greater than, equal to, or less than 0 if the frame at
 * <b>p1</b> is greater than, equal to, or less than the frame at <b>p2</b>.
 */
int can_msg_cmp(const void *p1, const void *p2);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO2_CAN_MSG_H_
