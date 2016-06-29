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
#include <lely/io/can.h>
#include "handle.h"

#include <assert.h>

#if defined(__linux__) && defined(HAVE_LINUX_CAN_H)

static void can_fini(struct io_handle *handle);
static int can_flags(struct io_handle *handle, int flags);
static ssize_t can_read(struct io_handle *handle, void *buf, size_t nbytes);
static ssize_t can_write(struct io_handle *handle, const void *buf,
		size_t nbytes);

static const struct io_handle_vtab can_vtab = {
	.size = sizeof(struct io_handle),
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

#ifdef HAVE_CAN_RAW_FD_FRAMES
	if (__unlikely(setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES,
			&(int){ 1 }, sizeof(int)) == -1)) {
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

	struct io_handle *handle = io_handle_alloc(&can_vtab);
	if (__unlikely(!handle)) {
		errsv = errno;
		goto error_alloc_handle;
	}

	handle->fd = s;

	return handle;

error_alloc_handle:
error_bind:
error_if_nametoindex:
#ifdef HAVE_CAN_RAW_FD_FRAMES
error_setsockopt:
#endif
	close(s);
error_socket:
	errno = errsv;
	return IO_HANDLE_ERROR;
}

static void
can_fini(struct io_handle *handle)
{
	assert(handle);

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

