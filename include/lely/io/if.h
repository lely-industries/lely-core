/**@file
 * This header file is part of the I/O library; it contains network interface
 * declarations.
 *
 * @copyright 2016-2020 Lely Industries N.V.
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

#ifndef LELY_IO_IF_H_
#define LELY_IO_IF_H_

#include <lely/io/addr.h>

/**
 * The maximum number of bytes required to hold the name of a network interface,
 * including the terminating null byte.
 */
#if _WIN32
#define IO_IF_NAME_STRLEN 256
#else
#define IO_IF_NAME_STRLEN 16
#endif

enum {
	/// The interface is running.
	IO_IF_UP = 1 << 0,
	/// A valid broadcast address is set.
	IO_IF_BROADCAST = 1 << 1,
	/// The interface is a loopback interface.
	IO_IF_LOOPBACK = 1 << 2,
	/// The interface is a point-to-point link.
	IO_IF_POINTTOPOINT = 1 << 3,
	/// The interface supports multicast.
	IO_IF_MULTICAST = 1 << 4
};

/// A structure describing a network interface.
struct io_ifinfo {
	/// The interface index.
	unsigned int index;
	/// The interface name.
	char name[IO_IF_NAME_STRLEN];
	/**
	 * The domain of the interface (one of #IO_SOCK_BTH, #IO_SOCK_IPV4,
	 * #IO_SOCK_IPV6 or #IO_SOCK_UNIX).
	 */
	int domain;
	/**
	 * The status of the interface (any combination of #IO_IF_UP,
	 * #IO_IF_BROADCAST, #IO_IF_LOOPBACK, #IO_IF_POINTTOPOINT and
	 * #IO_IF_MULTICAST).
	 */
	int flags;
	/// The address of the interface.
	io_addr_t addr;
	/// The netmask used by the interface.
	io_addr_t netmask;
	/// The broadcast address of the interface.
	io_addr_t broadaddr;
};

/// The static initializer for struct #io_ifinfo.
#define IO_IFINFO_INIT \
	{ \
		0, { '\0' }, 0, 0, IO_ADDR_INIT, IO_ADDR_INIT, IO_ADDR_INIT \
	}

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Obtains a list of network interfaces.
 *
 * @param maxinfo the maximum number of #io_ifinfo structs to return.
 * @param info    an array of at least <b>maxinfo</b> #io_ifinfo structs (can be
 *                NULL). On success, *<b>info</b> contains at most
 *                <b>maxinfo</b> structures describing the network interfaces.
 *
 * @returns the total number of interfaces (which may be different from
 * <b>maxinfo</b>), or -1 on error. In the latter case, the error number can be
 * obtained with get_errc().
 */
int io_get_ifinfo(int maxinfo, struct io_ifinfo *info);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO_IF_H_
