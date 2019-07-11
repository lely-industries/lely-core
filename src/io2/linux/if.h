/**@file
 * This is the internal header file of the rtnetlink network interface
 * functions.
 *
 * @copyright 2018-2019 Lely Industries N.V.
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

#ifndef LELY_IO2_INTERN_LINUX_IF_H_
#define LELY_IO2_INTERN_LINUX_IF_H_

#include "io.h"

#ifdef __linux__

#include "rtnl.h"

#ifdef __cplusplus
extern "C" {
#endif

static int io_if_get_txqlen(
		unsigned short ifi_type, int ifi_index, __u32 *ptxqlen);
static int io_if_set_txqlen(
		unsigned short ifi_type, int ifi_index, __u32 txqlen);
static int io_if_get_txqlen_parse(struct nlmsghdr *nlh, size_t len, void *arg);

static inline int
io_if_get_txqlen(unsigned short ifi_type, int ifi_index, __u32 *ptxqlen)
{
	int errsv = 0;

	struct rtnl_handle rth = { -1, 0, 0 };
	if (rtnl_open(&rth) == -1) {
		errsv = errno;
		goto error_rtnl_open;
	}

	if (rtnl_send_getlink_request(&rth, AF_UNSPEC, ifi_type, ifi_index)
			== -1) {
		errsv = errno;
		goto error_rtnl_send_getlink_request;
	}

	struct {
		unsigned short ifi_type;
		unsigned int ifi_flags;
		__u32 txqlen;
	} arg = { ifi_type, 0, 0 };
	if (rtnl_recv_type(&rth, RTM_NEWLINK, &io_if_get_txqlen_parse, &arg)
			== -1) {
		errsv = errno;
		goto error_rtnl_recv_type;
	}

	if (ptxqlen)
		*ptxqlen = arg.txqlen;

	rtnl_close(&rth);
	return 0;

error_rtnl_recv_type:
error_rtnl_send_getlink_request:
	rtnl_close(&rth);
error_rtnl_open:
	errno = errsv;
	return -1;
}

static inline int
io_if_set_txqlen(unsigned short ifi_type, int ifi_index, __u32 txqlen)
{
	int errsv = 0;

	struct rtnl_handle rth = { -1, 0, 0 };
	if (rtnl_open(&rth) == -1) {
		errsv = errno;
		goto error_rtnl_open;
	}

	if (rtnl_send_getlink_request(&rth, AF_UNSPEC, ifi_type, ifi_index)
			== -1) {
		errsv = errno;
		goto error_rtnl_send_getlink_request;
	}

	struct {
		unsigned short ifi_type;
		unsigned int ifi_flags;
		__u32 txqlen;
	} arg = { ifi_type, 0, 0 };
	if (rtnl_recv_type(&rth, RTM_NEWLINK, &io_if_get_txqlen_parse, &arg)
			== -1) {
		errsv = errno;
		goto error_rtnl_recv_type;
	}

	if (arg.txqlen < txqlen) {
		char buf[8];
		struct rtattr *rta = (struct rtattr *)buf;
		*rta = (struct rtattr){ .rta_len = RTA_LENGTH(sizeof(__u32)),
			.rta_type = IFLA_TXQLEN };
		*(__u32 *)RTA_DATA(rta) = txqlen;

		// clang-format off
		if (rtnl_send_newlink_request(&rth, AF_UNSPEC, ifi_type,
				ifi_index, arg.ifi_flags, rta,
				RTA_ALIGN(rta->rta_len)) == -1) {
			// clang-format on
			errsv = errno;
			goto error_rtnl_send_newlink_request;
		}

		if (rtnl_recv_ack(&rth) == -1) {
			errsv = errno;
			goto error_rtnl_recv_ack;
		}
	}

	rtnl_close(&rth);
	return 0;

error_rtnl_recv_ack:
error_rtnl_send_newlink_request:
error_rtnl_recv_type:
error_rtnl_send_getlink_request:
	rtnl_close(&rth);
error_rtnl_open:
	errno = errsv;
	return -1;
}

static inline int
io_if_get_txqlen_parse(struct nlmsghdr *nlh, size_t len, void *arg_)
{
	assert(nlh);
	assert(NLMSG_OK(nlh, len));
	assert(nlh->nlmsg_type == RTM_NEWLINK);
	struct {
		unsigned short ifi_type;
		unsigned int ifi_flags;
		__u32 txqlen;
	} *arg = arg_;
	assert(arg);

	struct ifinfomsg *ifi = NLMSG_DATA(nlh);
	if (ifi->ifi_type != arg->ifi_type) {
		errno = ENODEV;
		return -1;
	}

	arg->ifi_flags = ifi->ifi_flags;

	struct rtattr *rta =
			rta_find(IFLA_RTA(ifi), IFLA_PAYLOAD(nlh), IFLA_TXQLEN);
	if (!rta || RTA_PAYLOAD(rta) < sizeof(__u32)) {
		errno = EOPNOTSUPP;
		return -1;
	}

	arg->txqlen = *(__u32 *)RTA_DATA(rta);

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif // __linux__

#endif // !LELY_IO2_INTERN_LINUX_IF_H_
