/*!\file
 * This header file is part of the I/O library; it contains network socket
 * declarations.
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

#ifndef LELY_IO_SOCK_H
#define LELY_IO_SOCK_H

#include <lely/io/io.h>

enum {
	//! A Bluetooth socket.
	IO_SOCK_BTH = 1,
	//! A CAN (Controller Area Network) socket (only supported on Linux).
	IO_SOCK_CAN,
	//! An IPv4 socket.
	IO_SOCK_IPV4,
	//! An IPv6 socket.
	IO_SOCK_IPV6,
	//! A UNIX domain socket (only supported on POSIX platforms).
	IO_SOCK_UNIX
};

enum {
	/*!
	 * A stream-oriented connection-mode socket type. This corresponds to
	 * TCP for IPv4 or IPv6 sockets and RFCOMM for Bluetooth.
	 */
	IO_SOCK_STREAM = 1,
	/*!
	 * A datagram-oriented, typically connectionless-mode, socket type. This
	 * corresponds to UDP for IPv4 or IPv6 sockets.
	 */
	IO_SOCK_DGRAM
};

#endif

