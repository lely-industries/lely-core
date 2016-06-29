/*!\file
 * This file is part of the I/O library; it contains the implementation of the
 * Controller Area Network (CAN) functions.
 *
 * \see lely/io/can.h
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

#include "io.h"
#include <lely/util/errnum.h>
#if !defined(LELY_NO_CAN) && defined(__linux__) && defined(HAVE_LINUX_CAN_H)
#include <lely/can/socket.h>
#endif
#include <lely/io/can.h>
#include "handle.h"

#include <assert.h>

#if defined(__linux__) && defined(HAVE_LINUX_CAN_H)

//! A CAN device.
struct can {
	//! The I/O device base handle.
	struct io_handle base;
#if !defined(LELY_NO_CANFD) && defined(CANFD_MTU)
	//! A flag indicating the device supports sending CAN FD frames.
	int canfd;
#endif
};

static void can_fini(struct io_handle *handle);
static int can_flags(struct io_handle *handle, int flags);
static ssize_t can_read(struct io_handle *handle, void *buf, size_t nbytes);
static ssize_t can_write(struct io_handle *handle, const void *buf,
		size_t nbytes);

static const struct io_handle_vtab can_vtab = {
	.size = sizeof(struct can),
	.fini = &can_fini,
	.flags = &can_flags,
	.read = &can_read,
	.write = &can_write
};

LELY_IO_EXPORT io_handle_t
io_open_can(const char *path)
{
	int errsv = 0;

	int s = socket(AF_CAN, SOCK_RAW | SOCK_CLOEXEC, CAN_RAW);
	if (__unlikely(s == -1)) {
		errsv = errno;
		goto error_socket;
	}

#if !defined(LELY_NO_CANFD) && defined(CANFD_MTU)
	int canfd = 0;
#ifdef HAVE_CAN_RAW_FD_FRAMES
	errsv = errno;
	if (__likely(!setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &(int){ 1 },
			sizeof(int))))
		canfd = 1;
	else
		errsv = errno;
#endif
#endif

	unsigned int ifindex = if_nametoindex(path);
	if (__unlikely(!ifindex)) {
		errsv = errno;
		goto error_if_nametoindex;
	}

	struct sockaddr_can addr;
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifindex;

	if (__unlikely(bind(s, (struct sockaddr *)&addr, sizeof(addr)) == -1)) {
		errsv = errno;
		goto error_bind;
	}

#if !defined(LELY_NO_CANFD) && defined(CANFD_MTU) && defined(HAVE_SYS_IOCTL_H)
	if (canfd) {
		struct ifreq ifr;
		if (__unlikely(ioctl(s, SIOCGIFMTU, &ifr) == -1)) {
			errsv = errno;
			goto error_ioctl;
		}
		canfd = ifr.ifr_mtu == CANFD_MTU;
	}
#endif

	struct io_handle *handle = io_handle_alloc(&can_vtab);
	if (__unlikely(!handle)) {
		errsv = errno;
		goto error_alloc_handle;
	}

	handle->fd = s;
#if !defined(LELY_NO_CANFD) && defined(CANFD_MTU)
	((struct can *)handle)->canfd = canfd;
#endif

	return handle;

error_alloc_handle:
#if !defined(LELY_NO_CANFD) && defined(CANFD_MTU) && defined(HAVE_SYS_IOCTL_H)
error_ioctl:
#endif
error_bind:
error_if_nametoindex:
	close(s);
error_socket:
	errno = errsv;
	return IO_HANDLE_ERROR;
}

#ifndef LELY_NO_CAN

LELY_IO_EXPORT int
io_can_read(io_handle_t handle, struct can_msg *msg)
{
	assert(msg);

	if (__unlikely(!handle)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	if (__unlikely(handle->vtab != &can_vtab)) {
		set_errnum(ERRNUM_NXIO);
		return -1;
	}

#if !defined(LELY_NO_CANFD) && defined(CANFD_MTU)
	if (((struct can *)handle)->canfd) {
		struct canfd_frame frame;
		ssize_t nbytes = can_read(handle, &frame, sizeof(frame));
		if (__unlikely(nbytes < 0))
			return -1;

		if (nbytes == CANFD_MTU) {
			if (__unlikely(frame.can_id & CAN_ERR_FLAG))
				return 0;
			canfd_frame2can_msg(&frame, msg);
			return 1;
		} else if (nbytes == CAN_MTU) {
			if (__unlikely(frame.can_id & CAN_ERR_FLAG))
				return 0;
			can_frame2can_msg((struct can_frame *)&frame, msg);
			return 1;
		}

		return 0;
	}
#endif

	struct can_frame frame;
	ssize_t nbytes = can_read(handle, &frame, sizeof(frame));
	if (__unlikely(nbytes < 0))
		return -1;

	if (nbytes == CAN_MTU) {
		if (__unlikely(frame.can_id & CAN_ERR_FLAG))
			return 0;
		can_frame2can_msg(&frame, msg);
		return 1;
	}

	return 0;
}

LELY_IO_EXPORT int
io_can_write(io_handle_t handle, const struct can_msg *msg)
{
	assert(msg);

	if (__unlikely(!handle)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	if (__unlikely(handle->vtab != &can_vtab)) {
		set_errnum(ERRNUM_NXIO);
		return -1;
	}

#if !defined(LELY_NO_CANFD) && defined(CANFD_MTU)
	if (msg->flags & CAN_FLAG_EDL) {
		if (__unlikely(!((struct can *)handle)->canfd)) {
			errno = EINVAL;
			return -1;
		}
		struct canfd_frame frame;
		can_msg2canfd_frame(msg, &frame);
		return can_write(handle, &frame, sizeof(frame)) == sizeof(frame)
				? 0 : -1;
	}
#endif
	struct can_frame frame;
	can_msg2can_frame(msg, &frame);
	return can_write(handle, &frame, sizeof(frame)) == sizeof(frame)
			? 0 : -1;
}

#endif // !LELY_NO_CAN

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
	if (__unlikely(arg == -1))
		return -1;

	if (flags & IO_FLAG_NONBLOCK)
		arg |= O_NONBLOCK;
	else
		arg &= ~O_NONBLOCK;

	return fcntl(handle->fd, F_SETFL, arg);
}

static ssize_t
can_read(struct io_handle *handle, void *buf, size_t nbytes)
{
	assert(handle);

	ssize_t result;
	do result = read(handle->fd, buf, nbytes);
	while (__unlikely(result == -1 && errno == EINTR));
	return result;
}

static ssize_t
can_write(struct io_handle *handle, const void *buf, size_t nbytes)
{
	assert(handle);

	ssize_t result;
	do result = write(handle->fd, buf, nbytes);
	while (__unlikely(result == -1 && errno == EINTR));
	return result;
}

#endif // __linux__ && HAVE_LINUX_CAN_H

