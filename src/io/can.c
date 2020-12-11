/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * Controller Area Network (CAN) functions.
 *
 * @see lely/io/can.h
 *
 * @copyright 2017-2020 Lely Industries N.V.
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

#if !LELY_NO_STDIO

#include <lely/util/errnum.h>
#if !LELY_NO_CAN && defined(__linux__) && HAVE_LINUX_CAN_H
#include <lely/can/socket.h>
#endif
#include "handle.h"
#include <lely/io/can.h>
#ifdef __linux
#include "rtnl.h"
#endif

#include <assert.h>
#include <string.h>

#if defined(__linux__) && HAVE_LINUX_CAN_H

#ifdef HAVE_LINUX_CAN_ERROR_H
#include <linux/can/error.h>
#endif
#ifdef HAVE_LINUX_CAN_NETLINK_H
#define can_state can_state_
#define CAN_STATE_STOPPED CAN_STATE_STOPPED_
#define CAN_STATE_SLEEPING CAN_STATE_SLEEPING_
#include <linux/can/netlink.h>
#undef CAN_STATE_SLEEPING
#undef CAN_STATE_STOPPED
#undef can_state
#endif
#ifdef HAVE_LINUX_CAN_RAW_H
#include <linux/can/raw.h>
#endif

/// A CAN device.
struct can {
	/// The I/O device base handle.
	struct io_handle base;
#if !LELY_NO_CANFD && defined(CANFD_MTU)
	/// A flag indicating the device supports sending CAN FD frames.
	int canfd;
#endif
	/// The interface index.
	unsigned int ifindex;
	/// The active flag word of the device.
	unsigned int ifflags;
	/**
	 * The state of the CAN controller (one of `CAN_STATE_ACTIVE`,
	 * `CAN_STATE_PASSIVE` or `CAN_STATE_BUSOFF`).
	 */
	int state;
	/** The last error (any combination of `CAN_ERROR_BIT`,
	 * `CAN_ERROR_STUFF`, `CAN_ERROR_CRC`, `CAN_ERROR_FORM` and
	 * `CAN_ERROR_ACK`).
	 */
	int error;
};

static void can_fini(struct io_handle *handle);
static int can_flags(struct io_handle *handle, int flags);
static ssize_t can_read(struct io_handle *handle, void *buf, size_t nbytes);
static ssize_t can_write(
		struct io_handle *handle, const void *buf, size_t nbytes);

static const struct io_handle_vtab can_vtab = { .type = IO_TYPE_CAN,
	.size = sizeof(struct can),
	.fini = &can_fini,
	.flags = &can_flags,
	.read = &can_read,
	.write = &can_write };

static int can_err(struct can *can, const struct can_frame *frame);

#if defined(HAVE_LINUX_CAN_NETLINK_H) && defined(HAVE_LINUX_RTNETLINK_H)

static int can_getattr(int fd, __u32 seq, __u32 pid, int ifi_index,
		unsigned int *pifi_flags, unsigned short type, void *data,
		unsigned short payload);

static int can_setattr(int fd, __u32 seq, __u32 pid, int ifi_index,
		unsigned int ifi_flags, unsigned short type, const void *data,
		unsigned short payload);

#endif

io_handle_t
io_open_can(const char *path)
{
	int errsv = 0;

	int s = socket(AF_CAN, SOCK_RAW | SOCK_CLOEXEC, CAN_RAW);
	if (s == -1) {
		errsv = errno;
		goto error_socket;
	}

#if !LELY_NO_CANFD && defined(CANFD_MTU)
	int canfd = 0;
#ifdef HAVE_CAN_RAW_FD_FRAMES
	errsv = errno;
	// clang-format off
	if (!setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &(int){ 1 },
			sizeof(int)))
		// clang-format on
		canfd = 1;
	else
		errno = errsv;
#endif
#endif

#ifdef HAVE_LINUX_CAN_RAW_H
	// clang-format off
	if (setsockopt(s, SOL_CAN_RAW, CAN_RAW_ERR_FILTER,
			&(can_err_mask_t){ CAN_ERR_MASK },
			sizeof(can_err_mask_t)) == -1) {
		// clang-format on
		errsv = errno;
		goto error_setsockopt;
	}
#endif

	unsigned int ifindex = if_nametoindex(path);
	if (!ifindex) {
		errsv = errno;
		goto error_if_nametoindex;
	}

	struct sockaddr_can addr = { .can_family = AF_CAN,
		.can_ifindex = ifindex };

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		errsv = errno;
		goto error_bind;
	}

#ifdef HAVE_SYS_IOCTL_H
#if !LELY_NO_CANFD && defined(CANFD_MTU)
	if (canfd) {
		struct ifreq ifr;
		if_indextoname(ifindex, ifr.ifr_name);
		if (ioctl(s, SIOCGIFMTU, &ifr) == -1) {
			errsv = errno;
			goto error_ioctl;
		}
		canfd = ifr.ifr_mtu == CANFD_MTU;
	}
#endif

	int ifflags = 0;
	struct ifreq ifr;
	if_indextoname(ifindex, ifr.ifr_name);
	if (ioctl(s, SIOCGIFFLAGS, &ifr) == -1) {
		errsv = errno;
		goto error_ioctl;
	}
	ifflags = ifr.ifr_flags;
#endif

	struct io_handle *handle = io_handle_alloc(&can_vtab);
	if (!handle) {
		errsv = errno;
		goto error_alloc_handle;
	}

	handle->fd = s;
#if !LELY_NO_CANFD && defined(CANFD_MTU)
	((struct can *)handle)->canfd = canfd;
#endif
	((struct can *)handle)->ifindex = ifindex;
	((struct can *)handle)->ifflags = ifflags;
	((struct can *)handle)->state = CAN_STATE_ACTIVE;
	((struct can *)handle)->error = 0;

	return io_handle_acquire(handle);

error_alloc_handle:
#ifdef HAVE_SYS_IOCTL_H
error_ioctl:
#endif
error_bind:
error_if_nametoindex:
#ifdef HAVE_LINUX_CAN_RAW_H
error_setsockopt:
#endif
	close(s);
error_socket:
	errno = errsv;
	return IO_HANDLE_ERROR;
}

#if !LELY_NO_CAN

int
io_can_read(io_handle_t handle, struct can_msg *msg)
{
	assert(msg);

	if (!handle) {
		errno = EBADF;
		return -1;
	}

	if (handle->vtab != &can_vtab) {
		errno = ENXIO;
		return -1;
	}
	struct can *can = (struct can *)handle;

#if !LELY_NO_CANFD && defined(CANFD_MTU)
	if (((struct can *)handle)->canfd) {
		struct canfd_frame frame = { .can_id = 0 };
		ssize_t nbytes = can_read(handle, &frame, sizeof(frame));
		if (nbytes < 0)
			return -1;

		if (nbytes == CANFD_MTU) {
			if (frame.can_id & CAN_ERR_FLAG)
				return can_err(can, (struct can_frame *)&frame);
			if (canfd_frame2can_msg(&frame, msg) == -1)
				return 0;
			return 1;
		} else if (nbytes == CAN_MTU) {
			if (frame.can_id & CAN_ERR_FLAG)
				return can_err(can, (struct can_frame *)&frame);
			if (can_frame2can_msg((struct can_frame *)&frame, msg)
					== -1)
				return 0;
			return 1;
		}

		return 0;
	}
#endif

	struct can_frame frame = { .can_id = 0 };
	ssize_t nbytes = can_read(handle, &frame, sizeof(frame));
	if (nbytes < 0)
		return -1;

	if (nbytes == CAN_MTU) {
		if (frame.can_id & CAN_ERR_FLAG)
			return can_err(can, &frame);
		if (can_frame2can_msg(&frame, msg) == -1)
			return 0;
		return 1;
	}

	return 0;
}

int
io_can_write(io_handle_t handle, const struct can_msg *msg)
{
	assert(msg);

	if (!handle) {
		errno = EBADF;
		return -1;
	}

	if (handle->vtab != &can_vtab) {
		errno = ENXIO;
		return -1;
	}

#if !LELY_NO_CANFD && defined(CANFD_MTU)
	if (msg->flags & CAN_FLAG_EDL) {
		if (!((struct can *)handle)->canfd) {
			errno = EINVAL;
			return -1;
		}

		struct canfd_frame frame;
		if (can_msg2canfd_frame(msg, &frame) == -1)
			return -1;
		ssize_t nbytes = can_write(handle, &frame, sizeof(frame));
		if (nbytes < 0)
			return -1;

		return nbytes == CANFD_MTU;
	}
#endif
	struct can_frame frame;
	if (can_msg2can_frame(msg, &frame) == -1)
		return -1;
	ssize_t nbytes = can_write(handle, &frame, sizeof(frame));
	if (nbytes < 0)
		return -1;

	return nbytes == CAN_MTU;
}

#endif // !LELY_NO_CAN

#if defined(HAVE_LINUX_CAN_NETLINK_H) && defined(HAVE_LINUX_RTNETLINK_H)

int
io_can_start(io_handle_t handle)
{
	if (handle == IO_HANDLE_ERROR) {
		errno = EBADF;
		return -1;
	}

	if (handle->vtab != &can_vtab) {
		errno = ENXIO;
		return -1;
	}
	struct can *can = (struct can *)handle;

	int fd = io_rtnl_socket(0, 0);
	if (fd == -1)
		return -1;

	// clang-format off
	if (io_rtnl_newlink(fd, 0, 0, can->ifindex, can->ifflags | IFF_UP, NULL,
			0) == -1) {
		// clang-format on
		int errsv = errno;
		close(fd);
		errno = errsv;
		return -1;
	}
	can->ifflags |= IFF_UP;

	return close(fd);
}

int
io_can_stop(io_handle_t handle)
{
	if (handle == IO_HANDLE_ERROR) {
		errno = EBADF;
		return -1;
	}

	if (handle->vtab != &can_vtab) {
		errno = ENXIO;
		return -1;
	}
	struct can *can = (struct can *)handle;

	int fd = io_rtnl_socket(0, 0);
	if (fd == -1)
		return -1;

	// clang-format off
	if (io_rtnl_newlink(fd, 0, 0, can->ifindex, can->ifflags & ~IFF_UP,
			NULL, 0) == -1) {
		// clang-format on
		int errsv = errno;
		close(fd);
		errno = errsv;
		return -1;
	}
	can->ifflags &= ~IFF_UP;

	return close(fd);
}

#endif // HAVE_LINUX_CAN_NETLINK_H && HAVE_LINUX_RTNETLINK_H

int
io_can_get_state(io_handle_t handle)
{
	if (handle == IO_HANDLE_ERROR) {
		errno = EBADF;
		return -1;
	}

	if (handle->vtab != &can_vtab) {
		errno = ENXIO;
		return -1;
	}
	struct can *can = (struct can *)handle;

#if defined(HAVE_LINUX_CAN_NETLINK_H) && defined(HAVE_LINUX_RTNETLINK_H)
	int errsv = errno;

	int fd = io_rtnl_socket(0, 0);
	if (fd == -1)
		goto error;

	__u32 attr = 0;
	// clang-format off
	if (can_getattr(fd, 0, 0, can->ifindex, &can->ifflags, IFLA_CAN_STATE,
			&attr, sizeof(attr)) < (int)sizeof(attr)) {
		// clang-format on
		close(fd);
		goto error;
	}

	close(fd);

	switch (attr) {
	case CAN_STATE_ERROR_ACTIVE:
	case CAN_STATE_ERROR_WARNING: can->state = CAN_STATE_ACTIVE; break;
	case CAN_STATE_ERROR_PASSIVE: can->state = CAN_STATE_PASSIVE; break;
	case CAN_STATE_BUS_OFF: can->state = CAN_STATE_BUSOFF; break;
	}

error:
	// The virtual CAN driver does not provide the state through rtnetlink.
	// This is not an error, so return the stored state.
	errno = errsv;
#endif

	return can->state;
}

int
io_can_get_error(io_handle_t handle, int *perror)
{
	if (handle == IO_HANDLE_ERROR) {
		errno = EBADF;
		return -1;
	}

	if (handle->vtab != &can_vtab) {
		errno = ENXIO;
		return -1;
	}

	if (perror)
		*perror = ((struct can *)handle)->error;
	((struct can *)handle)->error = 0;

	return 0;
}

#if defined(HAVE_LINUX_CAN_NETLINK_H) && defined(HAVE_LINUX_RTNETLINK_H)

int
io_can_get_ec(io_handle_t handle, uint16_t *ptxec, uint16_t *prxec)
{
	if (handle == IO_HANDLE_ERROR) {
		errno = EBADF;
		return -1;
	}

	if (handle->vtab != &can_vtab) {
		errno = ENXIO;
		return -1;
	}
	struct can *can = (struct can *)handle;

	int fd = io_rtnl_socket(0, 0);
	if (fd == -1)
		return -1;

	struct can_berr_counter attr = { 0 };
	// clang-format off
	if (can_getattr(fd, 0, 0, can->ifindex, &can->ifflags,
			IFLA_CAN_BERR_COUNTER, &attr, sizeof(attr))
			< (int)sizeof(attr)) {
		// clang-format on
		int errsv = errno;
		close(fd);
		errno = errsv;
		return -1;
	}

	close(fd);

	if (ptxec)
		*ptxec = attr.txerr;
	if (prxec)
		*prxec = attr.rxerr;

	return 0;
}

int
io_can_get_bitrate(io_handle_t handle, uint32_t *pbitrate)
{
	if (handle == IO_HANDLE_ERROR) {
		errno = EBADF;
		return -1;
	}

	if (handle->vtab != &can_vtab) {
		errno = ENXIO;
		return -1;
	}
	struct can *can = (struct can *)handle;

	int fd = io_rtnl_socket(0, 0);
	if (fd == -1)
		return -1;

	struct can_bittiming attr = { 0 };
	// clang-format off
	if (can_getattr(fd, 0, 0, can->ifindex, &can->ifflags,
			IFLA_CAN_BITTIMING, &attr, sizeof(attr))
			< (int)sizeof(attr)) {
		// clang-format on
		int errsv = errno;
		close(fd);
		errno = errsv;
		return -1;
	}

	close(fd);

	if (pbitrate)
		*pbitrate = attr.bitrate;

	return 0;
}

int
io_can_set_bitrate(io_handle_t handle, uint32_t bitrate)
{
	int result = -1;
	int errsv = errno;

	if (handle == IO_HANDLE_ERROR) {
		errsv = EBADF;
		goto error_param;
	}

	if (handle->vtab != &can_vtab) {
		errsv = ENXIO;
		goto error_param;
	}
	struct can *can = (struct can *)handle;

	int fd = io_rtnl_socket(0, 0);
	if (fd == -1) {
		errsv = errno;
		goto error_socket;
	}

	// Deactivate the network interface, if necessary, before changing the
	// bitrate.
	// clang-format off
	if ((can->ifflags & IFF_UP) && io_rtnl_newlink(fd, 0, 0, can->ifindex,
			can->ifflags & ~IFF_UP, NULL, 0) == -1) {
		// clang-format on
		errsv = errno;
		goto error_newlink;
	}

	struct can_bittiming attr = { .bitrate = bitrate };
	// clang-format off
	if (can_setattr(fd, 0, 0, can->ifindex, can->ifflags,
			IFLA_CAN_BITTIMING, &attr, sizeof(attr)) == -1) {
		// clang-format on
		errsv = errno;
		goto error_setattr;
	}

	result = 0;

error_setattr:
error_newlink:
	close(fd);
error_socket:
error_param:
	errno = errsv;
	return result;
}

int
io_can_get_txqlen(io_handle_t handle, size_t *ptxqlen)
{
	if (handle == IO_HANDLE_ERROR) {
		errno = EBADF;
		return -1;
	}

	if (handle->vtab != &can_vtab) {
		errno = ENXIO;
		return -1;
	}
	struct can *can = (struct can *)handle;

	int fd = io_rtnl_socket(0, 0);
	if (fd == -1)
		return -1;

	__u32 attr = 0;
	// clang-format off
	if (io_rtnl_getattr(fd, 0, 0, can->ifindex, &can->ifflags, IFLA_TXQLEN,
			&attr, sizeof(attr)) < (int)sizeof(attr)) {
		// clang-format on
		int errsv = errno;
		close(fd);
		errno = errsv;
		return -1;
	}

	close(fd);

	if (ptxqlen)
		*ptxqlen = attr;

	return 0;
}

int
io_can_set_txqlen(io_handle_t handle, size_t txqlen)
{
	if (handle == IO_HANDLE_ERROR) {
		errno = EBADF;
		return -1;
	}

	if (handle->vtab != &can_vtab) {
		errno = ENXIO;
		return -1;
	}
	struct can *can = (struct can *)handle;

	int fd = io_rtnl_socket(0, 0);
	if (fd == -1)
		return -1;

	__u32 attr = txqlen;
	// clang-format off
	if (io_rtnl_setattr(fd, 0, 0, can->ifindex, can->ifflags, IFLA_TXQLEN,
			&attr, sizeof(attr)) == -1) {
		// clang-format on
		int errsv = errno;
		close(fd);
		errno = errsv;
		return -1;
	}

	return close(fd);
}

#endif // HAVE_LINUX_CAN_NETLINK_H && HAVE_LINUX_RTNETLINK_H

static void
can_fini(struct io_handle *handle)
{
	assert(handle);

	if (!(handle->flags & IO_FLAG_NO_CLOSE))
		close(handle->fd);
}

static int
can_flags(struct io_handle *handle, int flags)
{
	assert(handle);

	int arg = fcntl(handle->fd, F_GETFL, 0);
	if (arg == -1)
		return -1;

	int errsv = 0;

	if ((flags & IO_FLAG_NONBLOCK) && !(arg & O_NONBLOCK)) {
		if (fcntl(handle->fd, F_SETFL, arg | O_NONBLOCK) == -1) {
			errsv = errno;
			goto error_fcntl;
		}
	} else if (!(flags & IO_FLAG_NONBLOCK) && (arg & O_NONBLOCK)) {
		if (fcntl(handle->fd, F_SETFL, arg & ~O_NONBLOCK) == -1) {
			errsv = errno;
			goto error_fcntl;
		}
	}

	int optval = !!(flags & IO_FLAG_LOOPBACK);
	// clang-format off
	if (setsockopt(handle->fd, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS,
			(const char *)&optval, sizeof(optval)) == -1) {
		// clang-format on
		errsv = errno;
		goto error_setsockopt;
	}

	return 0;

error_setsockopt:
	fcntl(handle->fd, F_SETFL, arg);
error_fcntl:
	errno = errsv;
	return -1;
}

static ssize_t
can_read(struct io_handle *handle, void *buf, size_t nbytes)
{
	assert(handle);

	ssize_t result;
	int errsv = errno;
	do {
		errno = errsv;
		result = read(handle->fd, buf, nbytes);
	} while (result == -1 && errno == EINTR);
	return result;
}

static ssize_t
can_write(struct io_handle *handle, const void *buf, size_t nbytes)
{
	assert(handle);

	ssize_t result;
	int errsv = errno;
	do {
		errno = errsv;
		result = write(handle->fd, buf, nbytes);
	} while (result == -1 && errno == EINTR);
	return result;
}

static int
can_err(struct can *can, const struct can_frame *frame)
{
	assert(can);
	assert(frame);
	assert(frame->can_id & CAN_ERR_FLAG);

#ifdef HAVE_LINUX_CAN_ERROR_H
	if (frame->can_dlc != CAN_ERR_DLC)
		return 0;
#endif

	int state = can->state;
	int error = 0;

#ifdef HAVE_LINUX_CAN_ERROR_H
	if (frame->can_id & CAN_ERR_RESTARTED)
		state = CAN_STATE_ACTIVE;

	if (frame->can_id & CAN_ERR_CRTL) {
#ifdef CAN_ERR_CRTL_ACTIVE
		if (frame->data[1] & CAN_ERR_CRTL_ACTIVE)
			state = CAN_STATE_ACTIVE;
#endif
		// clang-format off
		if (frame->data[1] & (CAN_ERR_CRTL_RX_PASSIVE
				| CAN_ERR_CRTL_TX_PASSIVE))
			// clang-format on
			state = CAN_STATE_PASSIVE;
	}

	if (frame->can_id & CAN_ERR_PROT) {
		if (frame->data[2] & CAN_ERR_PROT_BIT)
			error |= CAN_ERROR_BIT;
		if (frame->data[2] & CAN_ERR_PROT_FORM)
			error |= CAN_ERROR_FORM;
		if (frame->data[2] & CAN_ERR_PROT_STUFF)
			error |= CAN_ERROR_STUFF;
		if (frame->data[3] & CAN_ERR_PROT_LOC_CRC_SEQ)
			error |= CAN_ERROR_CRC;
	}

	if (frame->can_id & CAN_ERR_ACK)
		error |= CAN_ERROR_ACK;

	if (frame->can_id & CAN_ERR_BUSOFF)
		state = CAN_STATE_BUSOFF;
#endif

	can->state = state;
	can->error = error;

	// cppcheck-suppress knownConditionTrueFalse
	if (state != CAN_STATE_ACTIVE || error) {
		errno = EIO;
		return -1;
	}

	return 0;
}

#if defined(HAVE_LINUX_CAN_NETLINK_H) && defined(HAVE_LINUX_RTNETLINK_H)

static int
can_getattr(int fd, __u32 seq, __u32 pid, int ifi_index,
		unsigned int *pifi_flags, unsigned short type, void *data,
		unsigned short payload)
{
	assert(data || !payload);

	char buf[1024];

	int len = io_rtnl_getattr(fd, seq, pid, ifi_index, pifi_flags,
			IFLA_LINKINFO, buf, sizeof(buf));
	if (len == -1)
		return -1;

	struct rtattr *rta =
			io_rta_find((struct rtattr *)buf, len, IFLA_INFO_DATA);
	if (rta)
		rta = io_rta_find(RTA_DATA(rta), RTA_PAYLOAD(rta), type);
	if (!rta) {
		errno = EOPNOTSUPP;
		return -1;
	}

	len = RTA_PAYLOAD(rta);
	if (data)
		memcpy(data, RTA_DATA(rta), MIN(payload, len));
	return len;
}

static int
can_setattr(int fd, __u32 seq, __u32 pid, int ifi_index, unsigned int ifi_flags,
		unsigned short type, const void *data, unsigned short payload)
{
	assert(data || !payload);

	char buf[1024];
	int len = 0;

	const char kind[] = "can";
	struct rtattr *info_kind = (struct rtattr *)buf;
	*info_kind = (struct rtattr){ .rta_len = RTA_LENGTH(sizeof(kind)),
		.rta_type = IFLA_INFO_KIND };
	memcpy(RTA_DATA(info_kind), kind, strlen(kind));

	len += RTA_ALIGN(info_kind->rta_len);

	struct rtattr *info_data = RTA_TAIL(info_kind);
	*info_data = (struct rtattr){ .rta_len = RTA_LENGTH(0),
		.rta_type = IFLA_INFO_DATA };

	if (data) {
		struct rtattr *rta = RTA_DATA(info_data);
		*rta = (struct rtattr){ .rta_len = RTA_LENGTH(payload),
			.rta_type = type };
		memcpy(RTA_DATA(rta), data, payload);

		info_data->rta_len += RTA_ALIGN(rta->rta_len);
	}

	len += RTA_ALIGN(info_data->rta_len);

	return io_rtnl_setattr(fd, seq, pid, ifi_index, ifi_flags,
			IFLA_LINKINFO, buf, len);
}

#endif // HAVE_LINUX_CAN_NETLINK_H && HAVE_LINUX_RTNETLINK_H

#endif // __linux__ && HAVE_LINUX_CAN_H

#endif // !LELY_NO_STDIO
