/**@file
 * This is the internal header file of the rtnetlink functions.
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

#ifndef LELY_IO2_INTERN_LINUX_RTNL_H_
#define LELY_IO2_INTERN_LINUX_RTNL_H_

#include "io.h"

#ifdef __linux__

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/socket.h>
#include <unistd.h>

#include <linux/rtnetlink.h>

/**
 * Returns the address of the next attribute. This macro is useful for
 * constructing a list of attributes, since `RTA_NEXT()` only works for complete
 * lists.
 */
#define RTA_TAIL(rta) \
	(struct rtattr *)((char *)(rta) + RTA_ALIGN((rta)->rta_len))

struct rtnl_handle {
	int fd;
	__u32 pid;
	__u32 seq;
};

#ifdef __cplusplus
extern "C" {
#endif

typedef int rtnl_recv_func_t(struct nlmsghdr *nlh, size_t len, void *arg);

static int rtnl_open(struct rtnl_handle *rth);
static int rtnl_close(struct rtnl_handle *rth);

static ssize_t rtnl_send(const struct rtnl_handle *rth, struct nlmsghdr *nlh,
		void *data, __u32 len);
static ssize_t rtnl_recv(const struct rtnl_handle *rth, void **pbuf);

static int rtnl_recv_ack(const struct rtnl_handle *rth);

static int rtnl_recv_type(const struct rtnl_handle *rth, __u16 type,
		rtnl_recv_func_t *func, void *arg);

static int rtnl_send_newlink_request(struct rtnl_handle *rth,
		unsigned char ifi_family, unsigned short ifi_type,
		int ifi_index, unsigned int ifi_flags, void *data, __u32 len);

static int rtnl_send_getlink_request(struct rtnl_handle *rth,
		unsigned char ifi_family, unsigned short ifi_type,
		int ifi_index);

static struct rtattr *rta_find(
		struct rtattr *rta, unsigned int len, unsigned short type);

static inline int
rtnl_open(struct rtnl_handle *rth)
{
	assert(rth);

	int errsv;

	rth->fd = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);
	if (rth->fd == -1) {
		errsv = errno;
		goto error_socket;
	}

	struct sockaddr_nl addr = { .nl_family = AF_NETLINK };
	socklen_t addrlen = sizeof(addr);
	if (bind(rth->fd, (struct sockaddr *)&addr, addrlen) == -1) {
		errsv = errno;
		goto error_bind;
	}
	if (getsockname(rth->fd, (struct sockaddr *)&addr, &addrlen) == -1) {
		errsv = errno;
		goto error_getsockname;
	}
	assert(addrlen == sizeof(addr));
	assert(addr.nl_family == AF_NETLINK);
	rth->pid = addr.nl_pid;

	rth->seq = time(NULL);

	return 0;

error_getsockname:
error_bind:
	close(rth->fd);
	rth->fd = -1;
error_socket:
	errno = errsv;
	return -1;
}

static inline int
rtnl_close(struct rtnl_handle *rth)
{
	assert(rth);

	int fd = rth->fd;
	rth->fd = -1;
	return close(fd);
}

static inline ssize_t
rtnl_send(const struct rtnl_handle *rth, struct nlmsghdr *nlh, void *data,
		__u32 len)
{
	assert(rth);
	assert(nlh);

	nlh->nlmsg_pid = rth->pid;

	struct sockaddr_nl addr = { .nl_family = AF_NETLINK };
	struct iovec iov[2] = {
		{ .iov_base = nlh, .iov_len = nlh->nlmsg_len - len },
		{ .iov_base = data, .iov_len = len }
	};
	struct msghdr msg = { .msg_name = &addr,
		.msg_namelen = sizeof(addr),
		.msg_iov = iov,
		.msg_iovlen = data ? 2 : 1 };

	ssize_t result;
	int errsv = errno;
	do {
		errno = errsv;
		result = sendmsg(rth->fd, &msg, 0);
	} while (result == -1 && errno == EINTR);
	return result;
}

static inline ssize_t
rtnl_recv(const struct rtnl_handle *rth, void **pbuf)
{
	assert(rth);

	void *buf = NULL;
	for (;; free(buf), buf = NULL) {
		ssize_t result;
		int errsv = errno;

		do {
			errno = errsv;
			result = recv(rth->fd, NULL, 0, MSG_PEEK | MSG_TRUNC);
		} while (result == -1 && errno == EINTR);
		if (result <= 0)
			break;
		size_t len = result;

		buf = malloc(len);
		if (!buf)
			break;

		struct sockaddr_nl addr = { .nl_family = AF_NETLINK };
		socklen_t addrlen = sizeof(addr);
		do {
			errno = errsv;
			result = recvfrom(rth->fd, buf, len, 0,
					(struct sockaddr *)&addr, &addrlen);
		} while (result == -1 && errno == EINTR);
		if (result < 0)
			break;
		assert(addrlen == sizeof(addr));
		assert(addr.nl_family == AF_NETLINK);

		struct nlmsghdr *nlh = buf;
		if (result != (ssize_t)len || !NLMSG_OK(nlh, len)) {
			errno = ENOBUFS;
			break;
		}

		if (addr.nl_pid || nlh->nlmsg_pid != rth->pid)
			continue;

		if (pbuf)
			*pbuf = buf;
		else
			free(buf);
		return result;
	}
	free(buf);

	return -1;
}

static inline int
rtnl_recv_ack(const struct rtnl_handle *rth)
{
	void *buf = NULL;
	for (;; free(buf), buf = NULL) {
		ssize_t len = rtnl_recv(rth, &buf);
		if (len <= 0)
			break;

		for (struct nlmsghdr *nlh = buf; NLMSG_OK(nlh, len);
				NLMSG_NEXT(nlh, len)) {
			if (nlh->nlmsg_seq != rth->seq)
				continue;

			int error = 0;
			if (nlh->nlmsg_type == NLMSG_ERROR) {
				struct nlmsgerr *err = NLMSG_DATA(nlh);
				if ((error = -err->error))
					errno = error;
			} else {
				errno = error = EPROTO;
			}

			free(buf);
			return error ? -1 : 0;
		}
	}
	free(buf);

	return -1;
}

static inline int
rtnl_recv_type(const struct rtnl_handle *rth, __u16 type,
		rtnl_recv_func_t *func, void *arg)
{
	void *buf = NULL;
	for (;; free(buf), buf = NULL) {
		ssize_t len = rtnl_recv(rth, &buf);
		if (len <= 0)
			break;

		for (struct nlmsghdr *nlh = buf; NLMSG_OK(nlh, len);
				NLMSG_NEXT(nlh, len)) {
			if (nlh->nlmsg_seq != rth->seq)
				continue;

			int error = 0;
			if (nlh->nlmsg_type == type) {
				if (func)
					error = func(nlh, len, arg);
			} else if (nlh->nlmsg_type == NLMSG_ERROR) {
				struct nlmsgerr *err = NLMSG_DATA(nlh);
				if ((error = -err->error))
					errno = error;
				else
					errno = error = EPROTO;
			} else {
				errno = error = EPROTO;
			}

			free(buf);
			return error ? -1 : 0;
		}
	}
	free(buf);

	return -1;
}

static inline int
rtnl_send_newlink_request(struct rtnl_handle *rth, unsigned char ifi_family,
		unsigned short ifi_type, int ifi_index, unsigned int ifi_flags,
		void *data, __u32 len)
{
	assert(rth);

	char buf[NLMSG_SPACE(sizeof(struct ifinfomsg))] = { 0 };

	struct nlmsghdr *nlh = (struct nlmsghdr *)buf;
	*nlh = (struct nlmsghdr){ .nlmsg_len = sizeof(buf) + len,
		.nlmsg_type = RTM_NEWLINK,
		.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK,
		.nlmsg_seq = ++rth->seq };

	struct ifinfomsg *ifi = NLMSG_DATA(nlh);
	*ifi = (struct ifinfomsg){ .ifi_family = ifi_family,
		.ifi_type = ifi_type,
		.ifi_index = ifi_index,
		.ifi_flags = ifi_flags,
		.ifi_change = 0xffffffffu };

	return rtnl_send(rth, nlh, data, len) == -1 ? -1 : 0;
}

static inline int
rtnl_send_getlink_request(struct rtnl_handle *rth, unsigned char ifi_family,
		unsigned short ifi_type, int ifi_index)
{
	assert(rth);

	char buf[NLMSG_SPACE(sizeof(struct ifinfomsg))] = { 0 };

	struct nlmsghdr *nlh = (struct nlmsghdr *)buf;
	*nlh = (struct nlmsghdr){ .nlmsg_len = sizeof(buf),
		.nlmsg_type = RTM_GETLINK,
		.nlmsg_flags = NLM_F_REQUEST,
		.nlmsg_seq = ++rth->seq };

	struct ifinfomsg *ifi = NLMSG_DATA(nlh);
	*ifi = (struct ifinfomsg){ .ifi_family = ifi_family,
		.ifi_type = ifi_type,
		.ifi_index = ifi_index,
		.ifi_change = 0xffffffffu };

	return rtnl_send(rth, nlh, NULL, 0) == -1 ? -1 : 0;
}

static inline struct rtattr *
rta_find(struct rtattr *rta, unsigned int len, unsigned short type)
{
	assert(rta);

	for (; RTA_OK(rta, len); rta = RTA_NEXT(rta, len)) {
		if (rta->rta_type == type)
			return rta;
	}
	return NULL;
}

#ifdef __cplusplus
}
#endif

#endif // __linux__

#endif // !LELY_IO2_INTERN_LINUX_RTNL_H_
