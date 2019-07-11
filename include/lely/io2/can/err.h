/**@file
 * This header file is part of the I/O library; it contains CAN bus error
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

#ifndef LELY_IO2_CAN_ERR_H_
#define LELY_IO2_CAN_ERR_H_

#include <lely/can/err.h>

/// A CAN error frame.
struct can_err {
	/**
	 * The state of the CAN node (one of #CAN_STATE_ACTIVE,
	 * #CAN_STATE_PASSIVE or #CAN_STATE_BUSOFF).
	 */
	int state;
	/**
	 * The error flags of the CAN bus (any combination of #CAN_ERROR_BIT,
	 * #CAN_ERROR_STUFF, #CAN_ERROR_CRC, #CAN_ERROR_FORM, #CAN_ERROR_ACK and
	 * #CAN_ERROR_OTHER).
	 */
	int error;
};

/// The static initializer for a #can_err struct.
#define CAN_ERR_INIT \
	{ \
		0, 0 \
	}

#endif // !LELY_IO2_CAN_ERR_H_
