/**@file
 * This header file is part of the CAN library; it contains CAN bus error
 * definitions.
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

#ifndef LELY_CAN_ERR_H_
#define LELY_CAN_ERR_H_

#include <lely/features.h>

/// The states of a CAN node, depending on the TX/RX error count.
enum can_state {
	/// The error active state (TX/RX error count < 128).
	CAN_STATE_ACTIVE,
	/// The error passive state (TX/RX error count < 256).
	CAN_STATE_PASSIVE,
	/// The bus off state (TX/RX error count >= 256).
	CAN_STATE_BUSOFF,
	/// The device is in sleep mode.
	CAN_STATE_SLEEPING,
	/// The device is stopped.
	CAN_STATE_STOPPED
};

/// The error flags of a CAN bus, which are not mutually exclusive.
enum can_error {
	/// A single bit error.
	CAN_ERROR_BIT = 1u << 0,
	/// A bit stuffing error.
	CAN_ERROR_STUFF = 1u << 1,
	/// A CRC sequence error.
	CAN_ERROR_CRC = 1u << 2,
	/// A form error.
	CAN_ERROR_FORM = 1u << 3,
	/// An acknowledgment error.
	CAN_ERROR_ACK = 1u << 4,
	/// One or more other errors.
	CAN_ERROR_OTHER = 1u << 5
};

#endif // !LELY_CAN_ERR_H_
