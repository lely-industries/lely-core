/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * rtnetlink functions.
 *
 * @see lely/io/rtnl.h
 *
 * @copyright 2017-2019 Lely Industries N.V.
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

#include "io.h"

#if !LELY_NO_STDIO && defined(HAVE_LINUX_RTNETLINK_H)

#include "rtnl.h"

#include <assert.h>
#include <errno.h>
#include <string.h>

#ifndef RTNL_BUFSIZE
#define RTNL_BUFSIZE 8192
#endif

static int io_rtnl_getattr_func(struct ifinfomsg *ifi, struct rtattr *rta,
		unsigned short rtalen, void *data);

static int io_rtnl_recv(int fd, int (*func)(struct nlmsghdr *nlh, void *data),
		void *data);
static int io_rtnl_recv_ack(int fd);
static int io_rtnl_recv_newlink(
		int fd, io_rtnl_newlink_func_t *func, void *data);
static int io_rtnl_recv_newlink_func(struct nlmsghdr *nlh, void *data);

static ssize_t io_rtnl_send(int fd, struct iovec *iov, int iovlen);
static ssize_t io_rtnl_send_getlink(int fd, __u32 seq, __u32 pid);
static ssize_t io_rtnl_send_newlink(int fd, __u32 seq, __u32 pid, int ifi_index,
		unsigned int ifi_flags, struct rtattr *rta,
		unsigned int rtalen);

int
io_rtnl_socket(__u32 pid, __u32 groups)
{
	int errsv = 0;

	int fd = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);
	if (fd == -1) {
		errsv = errno;
		goto error_socket;
	}

	struct sockaddr_nl addr = {
		.nl_family = AF_NETLINK, .nl_pid = pid, .nl_groups = groups
	};

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		errsv = errno;
		goto error_bind;
	}

	return fd;

error_bind:
	close(fd);
error_socket:
	errno = errsv;
	return -1;
}

int
io_rtnl_newlink(int fd, __u32 seq, __u32 pid, int ifi_index,
		unsigned int ifi_flags, struct rtattr *rta,
		unsigned short rtalen)
{
	// clang-format off
	if (io_rtnl_send_newlink(fd, seq, pid, ifi_index, ifi_flags, rta,
			rtalen) == -1)
		// clang-format on
		return -1;
	return io_rtnl_recv_ack(fd);
}

int
io_rtnl_getlink(int fd, __u32 seq, __u32 pid, io_rtnl_newlink_func_t *func,
		void *data)
{
	if (io_rtnl_send_getlink(fd, seq, pid) == -1)
		return -1;
	return io_rtnl_recv_newlink(fd, func, data);
}

int
io_rtnl_getattr(int fd, __u32 seq, __u32 pid, int ifi_index,
		unsigned int *pifi_flags, unsigned short type, void *data,
		unsigned short payload)
{
	assert(data || !payload);

	if (ifi_index <= 0) {
		errno = ENODEV;
		return -1;
	}

	struct {
		int ifi_index;
		unsigned int *pifi_flags;
		unsigned short type;
		void *data;
		unsigned short payload;
	} args = { ifi_index, pifi_flags, type, data, payload };

	if (io_rtnl_getlink(fd, seq, pid, &io_rtnl_getattr_func, &args) == -1)
		return -1;

	// On success, rtnl_getattr_func() sets ifi_index to 0.
	if (args.ifi_index) {
		errno = ENODEV;
		return -1;
	}

	// Return the actual size of the attribute payload.
	return args.payload;
}

static int
io_rtnl_getattr_func(struct ifinfomsg *ifi, struct rtattr *rta,
		unsigned short rtalen, void *data)
{
	assert(ifi);
	assert(rta);
	struct {
		int ifi_index;
		unsigned int *pifi_flags;
		unsigned short type;
		void *data;
		unsigned short payload;
	} *pargs = data;
	assert(pargs);

	// Set ifi_index to 0 once we've found the interface. This ensures we
	// only copy the attribute once, even if the interface occurs multiple
	// times.
	if (!pargs->ifi_index || pargs->ifi_index != ifi->ifi_index)
		return 0;
	pargs->ifi_index = 0;

	if (pargs->pifi_flags)
		*pargs->pifi_flags = ifi->ifi_flags;

	rta = io_rta_find(rta, rtalen, pargs->type);
	if (!rta) {
		errno = EOPNOTSUPP;
		return -1;
	}

	unsigned short payload = RTA_PAYLOAD(rta);
	if (pargs->data)
		memcpy(pargs->data, RTA_DATA(rta),
				MIN(pargs->payload, payload));
	pargs->payload = payload;

	return 0;
}

int
io_rtnl_setattr(int fd, __u32 seq, __u32 pid, int ifi_index,
		unsigned int ifi_flags, unsigned short type, const void *data,
		unsigned short payload)
{
	assert(data || !payload);

	char buf[RTA_SPACE(payload)];

	struct rtattr *rta = (struct rtattr *)buf;
	*rta = (struct rtattr){ .rta_len = RTA_LENGTH(payload),
		.rta_type = type };
	if (data)
		memcpy(RTA_DATA(rta), data, payload);

	return io_rtnl_newlink(
			fd, seq, pid, ifi_index, ifi_flags, rta, rta->rta_len);
}

static int
io_rtnl_recv(int fd, int (*func)(struct nlmsghdr *nlh, void *data), void *data)
{
	char buf[RTNL_BUFSIZE] = { 0 };

	int result = 0;
	int errsv = errno;

	int done = 0;
	while (!done) {
		ssize_t len;
		do {
			errno = errsv;
			len = recv(fd, buf, sizeof(buf), 0);
		} while (len == -1 && errno == EINTR);
		if (len <= 0) {
			if (len < 0) {
				result = -1;
				errsv = errno;
			}
			break;
		}

		struct nlmsghdr *nlh = (struct nlmsghdr *)buf;
		for (; NLMSG_OK(nlh, len); nlh = NLMSG_NEXT(nlh, len)) {
			if (nlh->nlmsg_type == NLMSG_DONE
					|| !(nlh->nlmsg_flags & NLM_F_MULTI))
				done = 1;
			if (nlh->nlmsg_type == NLMSG_ERROR) {
				struct nlmsgerr *err = NLMSG_DATA(nlh);
				if (err->error < 0 && !result) {
					result = -1;
					errsv = -err->error;
				}
			}
			if (nlh->nlmsg_type < NLMSG_MIN_TYPE)
				continue;
			if (func && func(nlh, data) && !result) {
				result = -1;
				errsv = errno;
			}
		}
	}

	errno = errsv;
	return result;
}

static int
io_rtnl_recv_ack(int fd)
{
	return io_rtnl_recv(fd, NULL, NULL);
}

static int
io_rtnl_recv_newlink(int fd, io_rtnl_newlink_func_t *func, void *data)
{
	struct {
		io_rtnl_newlink_func_t *func;
		void *data;
	} args = { func, data };

	return io_rtnl_recv(fd, &io_rtnl_recv_newlink_func, &args);
}

static int
io_rtnl_recv_newlink_func(struct nlmsghdr *nlh, void *data)
{
	assert(nlh);
	struct {
		io_rtnl_newlink_func_t *func;
		void *data;
	} *pargs = data;
	assert(pargs);

	if (nlh->nlmsg_type != RTM_NEWLINK)
		return 0;

	if (!pargs->func)
		return 0;

	return pargs->func(NLMSG_DATA(nlh), IFLA_RTA(NLMSG_DATA(nlh)),
			IFLA_PAYLOAD(nlh), pargs->data);
}

static ssize_t
io_rtnl_send(int fd, struct iovec *iov, int iovlen)
{
	struct sockaddr_nl addr = { .nl_family = AF_NETLINK };

	struct msghdr msg = { .msg_name = &addr,
		.msg_namelen = sizeof(addr),
		.msg_iov = iov,
		.msg_iovlen = iovlen };

	ssize_t result;
	int errsv = errno;
	do {
		errno = errsv;
		result = sendmsg(fd, &msg, 0);
	} while (result == -1 && errno == EINTR);
	return result;
}

static ssize_t
io_rtnl_send_getlink(int fd, __u32 seq, __u32 pid)
{
	char buf[NLMSG_SPACE(sizeof(struct rtgenmsg))] = { 0 };

	struct nlmsghdr *nlh = (struct nlmsghdr *)buf;
	// clang-format off
	*nlh = (struct nlmsghdr){
		.nlmsg_len = NLMSG_SPACE(sizeof(struct rtgenmsg)),
		.nlmsg_type = RTM_GETLINK,
		.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP,
		.nlmsg_seq = seq,
		.nlmsg_pid = pid };
	// clang-format on

	struct rtgenmsg *rtgen = NLMSG_DATA(nlh);
	*rtgen = (struct rtgenmsg){ .rtgen_family = AF_UNSPEC };

	struct iovec iov[] = { { buf, sizeof(buf) } };
	return io_rtnl_send(fd, iov, sizeof(iov) / sizeof(*iov));
}

static ssize_t
io_rtnl_send_newlink(int fd, __u32 seq, __u32 pid, int ifi_index,
		unsigned int ifi_flags, struct rtattr *rta, unsigned int rtalen)
{
	assert(rta || !rtalen);

	char buf[NLMSG_SPACE(sizeof(struct ifinfomsg))] = { 0 };

	struct nlmsghdr *nlh = (struct nlmsghdr *)buf;
	*nlh = (struct nlmsghdr){
		.nlmsg_len = NLMSG_SPACE(sizeof(struct ifinfomsg)) + rtalen,
		.nlmsg_type = RTM_NEWLINK,
		.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK,
		.nlmsg_seq = seq,
		.nlmsg_pid = pid
	};

	struct ifinfomsg *ifi = NLMSG_DATA(nlh);
	*ifi = (struct ifinfomsg){ .ifi_family = AF_UNSPEC,
		.ifi_index = ifi_index,
		.ifi_flags = ifi_flags,
		.ifi_change = 0xffffffffu };

	struct iovec iov[] = { { buf, sizeof(buf) }, { rta, rtalen } };
	return io_rtnl_send(fd, iov, sizeof(iov) / sizeof(*iov));
}

#endif // !LELY_NO_STDIO && HAVE_LINUX_RTNETLINK_H
