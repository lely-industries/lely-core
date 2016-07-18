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
	/*!
	 * The state of the CAN controller (one of `CAN_STATE_ACTIVE`,
	 * `CAN_STATE_PASSIVE` or `CAN_STATE_BUSOFF`).
	 */
	int state;
	/*! The last error (any combination of `CAN_ERROR_BIT`,
	* `CAN_ERROR_STUFF`, `CAN_ERROR_CRC`, `CAN_ERROR_FORM` and
	* `CAN_ERROR_ACK`).
	 */
	int error;
};

static void can_fini(struct io_handle *handle);
static int can_flags(struct io_handle *handle, int flags);
static ssize_t can_read(struct io_handle *handle, void *buf, size_t nbytes);
static ssize_t can_write(struct io_handle *handle, const void *buf,
		size_t nbytes);

static const struct io_handle_vtab can_vtab = {
	.type = IO_TYPE_CAN,
	.size = sizeof(struct can),
	.fini = &can_fini,
	.flags = &can_flags,
	.read = &can_read,
	.write = &can_write
};

static int can_err(io_handle_t handle, const struct can_frame *frame);

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
		errno = errsv;
#endif
#endif

#ifdef HAVE_LINUX_CAN_RAW_H
	if (__unlikely(setsockopt(s, SOL_CAN_RAW, CAN_RAW_ERR_FILTER,
			&(can_err_mask_t){ CAN_ERR_MASK },
			sizeof(can_err_mask_t)) == -1)) {
		errsv = errno;
		goto error_setsockopt;
	}
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
		if_indextoname(ifindex, ifr.ifr_name);
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
	((struct can *)handle)->state = CAN_STATE_ACTIVE;
	((struct can *)handle)->error = 0;

	return io_handle_acquire(handle);

error_alloc_handle:
#if !defined(LELY_NO_CANFD) && defined(CANFD_MTU) && defined(HAVE_SYS_IOCTL_H)
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
				return can_err(handle,
						(struct can_frame *)&frame);
			if (__unlikely(canfd_frame2can_msg(&frame, msg) == -1))
				return 0;
			return 1;
		} else if (nbytes == CAN_MTU) {
			if (__unlikely(frame.can_id & CAN_ERR_FLAG))
				return can_err(handle,
						(struct can_frame *)&frame);
			if (__unlikely(can_frame2can_msg(
					(struct can_frame *)&frame, msg) == -1))
				return 0;
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
			return can_err(handle, &frame);
		if (__unlikely(can_frame2can_msg(&frame, msg) == -1))
			return 0;
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
		if (__unlikely(can_msg2canfd_frame(msg, &frame) == -1))
			return -1;
		ssize_t nbytes = can_write(handle, &frame, sizeof(frame));
		if (__unlikely(nbytes < 0))
			return -1;

		return nbytes == CANFD_MTU;
	}
#endif
	struct can_frame frame;
	if (__unlikely(can_msg2can_frame(msg, &frame) == -1))
		return -1;
	ssize_t nbytes = can_write(handle, &frame, sizeof(frame));
	if (__unlikely(nbytes < 0))
		return -1;

	return nbytes == CAN_MTU;
}

#endif // !LELY_NO_CAN

LELY_IO_EXPORT int
io_can_get_state(io_handle_t handle)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	if (__unlikely(handle->vtab != &can_vtab)) {
		set_errnum(ERRNUM_NXIO);
		return -1;
	}

	return ((struct can *)handle)->state;
}

LELY_IO_EXPORT int
io_can_get_error(io_handle_t handle, int *perror)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	if (__unlikely(handle->vtab != &can_vtab)) {
		set_errnum(ERRNUM_NXIO);
		return -1;
	}

	if (perror)
		*perror = ((struct can *)handle)->error;
	((struct can *)handle)->error = 0;

	return 0;
}

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

static int
can_err(io_handle_t handle, const struct can_frame *frame)
{
	assert(handle);
	assert(handle->vtab == &can_vtab);
	assert(frame);
	assert(frame->can_id & CAN_ERR_FLAG);

#ifdef HAVE_LINUX_CAN_ERROR_H
	if (__unlikely(frame->can_dlc != CAN_ERR_DLC))
		return 0;
#endif

	int state = ((struct can *)handle)->state;
	int error = 0;

#ifdef HAVE_LINUX_CAN_ERROR_H
	if (frame->can_id & CAN_ERR_RESTARTED)
		state = CAN_STATE_ACTIVE;

	if (frame->can_id & CAN_ERR_CRTL) {
#ifdef CAN_ERR_CRTL_ACTIVE
		if (frame->data[1] & CAN_ERR_CRTL_ACTIVE)
			state = CAN_STATE_ACTIVE;
#endif
		if (frame->data[1] & (CAN_ERR_CRTL_RX_PASSIVE
				| CAN_ERR_CRTL_TX_PASSIVE))
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

	((struct can *)handle)->state = state;
	((struct can *)handle)->state = error;

	if (state != CAN_STATE_ACTIVE || error) {
		errno = EIO;
		return -1;
	}

	return 0;
}

#endif // __linux__ && HAVE_LINUX_CAN_H

