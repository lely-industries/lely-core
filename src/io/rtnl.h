/**@file
 * This is the internal header file of the rtnetlink declarations.
 *
 * @copyright 2016-2018 Lely Industries N.V.
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

#ifndef LELY_IO_INTERN_RTNL_H_
#define LELY_IO_INTERN_RTNL_H_

#include "io.h"

#ifdef HAVE_LINUX_RTNETLINK_H

#include <linux/rtnetlink.h>

/**
 * Returns the address of the next attribute. This macro is useful for
 * constructing a list of attributes, since `RTA_NEXT()` only works for complete
 * lists.
 */
#define RTA_TAIL(rta) \
	(struct rtattr *)((char *)(rta) + RTA_ALIGN((rta)->rta_len))

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a callback function invoked when an RTM_NEWLINK response is
 * received.
 *
 * @param ifi    a pointer to an `ifinfomsg` struct describing the network
 *               interface.
 * @param rta    a pointer to a list of attributes.
 * @param rtalen the length (in bytes) of the attribute list.
 * @param data   a pointer to used-specified data.
 *
 * @returns 0 on success, or -1 on error.
 */
typedef int io_rtnl_newlink_func_t(struct ifinfomsg *ifi, struct rtattr *rta,
		unsigned short rtalen, void *data);

/**
 * Opens an rtnetlink socket.
 *
 * @param pid    a unique id identifying the netlink socket. This can be equal
 *               to the process id only for at most one socket. If <b>pid</b> is
 *               0, the kernel assigns a unique id.
 * @param groups a bitmask specifying to which of the 32 multicast groups the
 *               socket should listen. Sending or receiving multicast messages
 *               requires the CAP_NET_ADMIN capability.
 *
 * @returns a socket file descriptor, or -1 on error. In the latter case, the
 * error number is stored in `errno`.
 */
int io_rtnl_socket(__u32 pid, __u32 groups);

/**
 * Sends an RTM_NEWLINK request and waits until the acknowledgment is received.
 * This operation requires the CAP_NET_ADMIN capability.
 *
 * @param fd        an rtnetlink socket file descriptor.
 * @param seq       the sequence number identifying the request (can be 0).
 * @param pid       the (process) ID identifying the sender (can be 0).
 * @param ifi_index the interface index.
 * @param ifi_flags the active flag word of the device.
 * @param rta       a pointer to a list of attributes (can be NULL).
 * @param rtalen    the length of the attribute list (in bytes).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * is stored in `errno`.
 */
int io_rtnl_newlink(int fd, __u32 seq, __u32 pid, int ifi_index,
		unsigned int ifi_flags, struct rtattr *rta,
		unsigned short rtalen);

/**
 * Sends an RTM_GETLINK request and invokes the specified callback function for
 * each received network interface.
 *
 * @param fd   an rtnetlink socket file descriptor.
 * @param seq  the sequence number identifying the request (can be 0).
 * @param pid  the (process) ID identifying the sender (can be 0).
 * @param func a pointer to the function to be invoked for each network
 *             interface.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>func</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * is stored in `errno`.
 */
int io_rtnl_getlink(int fd, __u32 seq, __u32 pid, io_rtnl_newlink_func_t *func,
		void *data);

/**
 * Invokes io_rtnl_getlink() and retrieves a single attribute of the specified
 * network interface.
 *
 * @param fd         an rtnetlink socket file descriptor.
 * @param seq        the sequence number identifying the request (can be 0).
 * @param pid        the (process) ID identifying the sender (can be 0).
 * @param ifi_index  the interface index.
 * @param pifi_flags the address at which to store the active flag word of the
 *                   device (can be NULL).
 * @param type       the type of the attribute.
 * @param data       the address at which to store the payload of the attribute.
 * @param payload    the size of the buffer at <b>data</b> (in bytes).
 *
 * @returns length of the payload (in bytes) of the attribute on success, or -1
 * on error. In the latter case, the error number is stored in `errno`. Note
 * that the result can be larger than <b>payload</b>, but at most <b>payload</b>
 * bytes will be copied to <b>data</b>.
 *
 * @see io_rtnl_setattr()
 */
int io_rtnl_getattr(int fd, __u32 seq, __u32 pid, int ifi_index,
		unsigned int *pifi_flags, unsigned short type, void *data,
		unsigned short payload);

/**
 * Invokes io_rtnl_newlink() to set at most one attribute of the specified
 * network interface. This operation requires the CAP_NET_ADMIN capability.
 *
 * @param fd        an rtnetlink socket file descriptor.
 * @param seq       the sequence number identifying the request (can be 0).
 * @param pid       the (process) ID identifying the sender (can be 0).
 * @param ifi_index the interface index.
 * @param ifi_flags the active flag word of the device.
 * @param type      the type of the attribute.
 * @param data      a pointer to the payload of the attribute (can be NULL).
 * @param payload   the length of the payload (in bytes).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * is stored in `errno`.
 *
 * @see io_rtnl_getattr()
 */
int io_rtnl_setattr(int fd, __u32 seq, __u32 pid, int ifi_index,
		unsigned int ifi_flags, unsigned short type, const void *data,
		unsigned short payload);

/**
 * Finds an attribute in a list of attributes.
 *
 * @param rta  a pointer to a list of attributes.
 * @param len  the length of the attribute list (in bytes).
 * @param type the type of the attribute to be found.
 *
 * @returns a pointer to the attribute if found, or NULL if not.
 */
static inline struct rtattr *io_rta_find(
		struct rtattr *rta, unsigned short len, unsigned short type);

static inline struct rtattr *
io_rta_find(struct rtattr *rta, unsigned short len, unsigned short type)
{
	for (; RTA_OK(rta, len); rta = RTA_NEXT(rta, len)) {
		if (rta->rta_type == type)
			return rta;
	}
	return NULL;
}

#ifdef __cplusplus
}
#endif

#endif // HAVE_LINUX_RTNETLINK_H

#endif // !LELY_IO_INTERN_RTNL_H_
