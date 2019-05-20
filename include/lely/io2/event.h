/**@file
 * This header file is part of the I/O library; it contains the I/O event
 * declarations.
 *
 * @copyright 2019 Lely Industries N.V.
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

#ifndef LELY_IO2_EVENT_H_
#define LELY_IO2_EVENT_H_

#include <lely/io2/io2.h>

/// The type of I/O events that can be monitored and/or reported.
enum io_event {
	/**
	 * Data (other than priority data) MAY be read without blocking. For
	 * connection-oriented socket, this event is also reported when the peer
	 * closed the connection. For listening sockets, this event indicates
	 * that there are pending connections waiting to be accepted.
	 */
	IO_EVENT_IN = 1u << 0,
	/**
	 * Priority data MAY be read without blocking. For sockets, this event
	 * typically indicates the presence of out-of-band data.
	 */
	IO_EVENT_PRI = 1u << 1,
	/**
	 * Data (bot normal and priority data) MAY be written without blocking.
	 * For connection-oriented sockets, this event is also reported when a
	 * connection attempt completes (with success or failure).
	 */
	IO_EVENT_OUT = 1u << 2,
	/// An error has occurred. This event is always reported.
	IO_EVENT_ERR = 1u << 3,
	/**
	 * The device has been disconnected. For connection-oriented sockets,
	 * this event is reported when a connection is shut down (by
	 * `closesocket()` on Windows or `shutdown(socket, SHUT_RDWR)` on POSIX
	 * plafforms) or when a connection attempt fails. This event is always
	 * reported.
	 */
	IO_EVENT_HUP = 1u << 4,
	IO_EVENT_NONE = 0,
	IO_EVENT_MASK = IO_EVENT_IN | IO_EVENT_PRI | IO_EVENT_OUT | IO_EVENT_ERR
			| IO_EVENT_HUP
};

#endif // !LELY_IO2_EVENT_H_
