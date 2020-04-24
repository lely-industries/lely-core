/**@file
 * This is the internal header file of the SocketCAN rtnetlink attributes
 * functions.
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

#ifndef LELY_IO2_INTERN_LINUX_CAN_ATTR_H_
#define LELY_IO2_INTERN_LINUX_CAN_ATTR_H_

#include "io.h"

#ifdef __linux__

#include <lely/io2/can.h>

#include <errno.h>

#include <net/if.h>

#include <linux/can.h>
#if !LELY_NO_CANFD && !defined(CANFD_MTU)
#error CAN FD not supported.
#endif
#define can_state can_state_
#define CAN_STATE_STOPPED CAN_STATE_STOPPED_
#define CAN_STATE_SLEEPING CAN_STATE_SLEEPING_
#include <linux/can/netlink.h>
#undef CAN_STATE_SLEEPING
#undef CAN_STATE_STOPPED
#undef can_state
#include <linux/if_arp.h>

#include "rtnl.h"

struct io_can_attr {
	int state;
	int flags;
	int nominal;
#if !LELY_NO_CANFD
	int data;
#endif
};

#if LELY_NO_CANFD
#define IO_CAN_ATTR_INIT \
	{ \
		CAN_STATE_ACTIVE, 0, 0 \
	}
#else
#define IO_CAN_ATTR_INIT \
	{ \
		CAN_STATE_ACTIVE, 0, 0, 0 \
	}
#endif

#ifdef __cplusplus
extern "C" {
#endif

static int io_can_attr_get(struct io_can_attr *attr, unsigned int ifindex);
static int io_can_attr_parse(struct nlmsghdr *nlh, size_t len, void *arg);

static inline int
io_can_attr_get(struct io_can_attr *attr, unsigned int ifindex)
{
	int errsv = 0;

	struct rtnl_handle rth = { -1, 0, 0 };
	if (rtnl_open(&rth) == -1) {
		errsv = errno;
		goto error_rtnl_open;
	}

	if (rtnl_send_getlink_request(&rth, AF_UNSPEC, ARPHRD_CAN, ifindex)
			== -1) {
		errsv = errno;
		goto error_rtnl_send_getlink_request;
	}

	if (rtnl_recv_type(&rth, RTM_NEWLINK, &io_can_attr_parse, attr) == -1) {
		errsv = errno;
		goto error_rtnl_recv_type;
	}

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
io_can_attr_parse(struct nlmsghdr *nlh, size_t len, void *arg)
{
	assert(nlh);
	assert(NLMSG_OK(nlh, len));
	assert(nlh->nlmsg_type == RTM_NEWLINK);
	(void)len;
	struct io_can_attr *attr = arg;
	assert(attr);

	struct ifinfomsg *ifi = NLMSG_DATA(nlh);
	if (ifi->ifi_type != ARPHRD_CAN) {
		errno = ENODEV;
		return -1;
	}

#if !LELY_NO_CANFD
	struct rtattr *mtu =
			rta_find(IFLA_RTA(ifi), IFLA_PAYLOAD(nlh), IFLA_MTU);
	if (mtu && RTA_PAYLOAD(mtu) >= sizeof(unsigned int)) {
		unsigned int *data = RTA_DATA(mtu);
		if (*data == CANFD_MTU)
			attr->flags |= IO_CAN_BUS_FLAG_FDF;
		else
			attr->flags &= ~IO_CAN_BUS_FLAG_FDF;
	}
#endif

	struct rtattr *linkinfo = rta_find(
			IFLA_RTA(ifi), IFLA_PAYLOAD(nlh), IFLA_LINKINFO);
	if (!linkinfo) {
		errno = EOPNOTSUPP;
		return -1;
	}

	struct rtattr *info_data = rta_find(RTA_DATA(linkinfo),
			RTA_PAYLOAD(linkinfo), IFLA_INFO_DATA);
	if (info_data) {
		struct rtattr *rta;

		rta = rta_find(RTA_DATA(info_data), RTA_PAYLOAD(info_data),
				IFLA_CAN_STATE);
		if (rta && RTA_PAYLOAD(rta) >= sizeof(int)) {
			int *data = RTA_DATA(rta);
			switch (*data) {
			case CAN_STATE_ERROR_ACTIVE:
			case CAN_STATE_ERROR_WARNING:
				attr->state = CAN_STATE_ACTIVE;
				break;
			case CAN_STATE_ERROR_PASSIVE:
				attr->state = CAN_STATE_PASSIVE;
				break;
			case CAN_STATE_BUS_OFF:
				attr->state = CAN_STATE_BUSOFF;
				break;
			case CAN_STATE_STOPPED_:
				attr->state = CAN_STATE_STOPPED;
				break;
			case CAN_STATE_SLEEPING_:
				attr->state = CAN_STATE_SLEEPING;
				break;
			}
		}

#if !LELY_NO_CANFD
		rta = rta_find(RTA_DATA(info_data), RTA_PAYLOAD(info_data),
				IFLA_CAN_CTRLMODE);
		if (rta && RTA_PAYLOAD(rta) >= sizeof(struct can_ctrlmode)) {
			struct can_ctrlmode *data = RTA_DATA(rta);
			if (data->flags & CAN_CTRLMODE_FD)
				attr->flags |= IO_CAN_BUS_FLAG_BRS;
			else
				attr->flags &= ~IO_CAN_BUS_FLAG_BRS;
		}
#endif

		rta = rta_find(RTA_DATA(info_data), RTA_PAYLOAD(info_data),
				IFLA_CAN_BITTIMING);
		if (rta && RTA_PAYLOAD(rta) >= sizeof(struct can_bittiming)) {
			struct can_bittiming *data = RTA_DATA(rta);
			attr->nominal = data->bitrate;
		}

#if !LELY_NO_CANFD
		rta = rta_find(RTA_DATA(info_data), RTA_PAYLOAD(info_data),
				IFLA_CAN_DATA_BITTIMING);
		if (rta && RTA_PAYLOAD(rta) >= sizeof(struct can_bittiming)) {
			struct can_bittiming *data = RTA_DATA(rta);
			attr->data = data->bitrate;
		}
#endif
	}

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif // __linux__

#endif // !LELY_IO2_INTERN_LINUX_CAN_ATTR_H_
