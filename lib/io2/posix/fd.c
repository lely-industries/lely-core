/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * common file descriptor functions.
 *
 * @copyright 2018-2020 Lely Industries N.V.
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

#include "fd.h"

#if !LELY_NO_STDIO && _POSIX_C_SOURCE >= 200112L

#include <assert.h>
#include <errno.h>

#include <fcntl.h>
#include <poll.h>

int
io_fd_set_cloexec(int fd)
{
	int arg = fcntl(fd, F_GETFD);
	if (arg == -1)
		return -1;
	if (!(arg & FD_CLOEXEC) && fcntl(fd, F_SETFD, arg | FD_CLOEXEC) == -1)
		return -1;
	return 0;
}

int
io_fd_set_nonblock(int fd)
{
	int arg = fcntl(fd, F_GETFL);
	if (arg == -1)
		return -1;
	if (!(arg & O_NONBLOCK) && fcntl(fd, F_SETFL, arg | O_NONBLOCK) == -1)
		return -1;
	return 0;
}

int
io_fd_wait(int fd, int *events, int timeout)
{
	assert(events);

	int result;
	struct pollfd fds[1] = { { .fd = fd, .events = *events } };
	do
		result = poll(fds, 1, timeout);
	// clang-format off
	while (result == -1 && ((timeout < 0 && errno == EINTR)
			|| errno == EAGAIN));
	// clang-format on
	*events = 0;
	if (result == -1)
		return -1;
	if (!result && timeout >= 0) {
		errno = EAGAIN;
		return -1;
	}
	assert(result == 1);
	*events = fds[0].revents;
	return 0;
}

ssize_t
io_fd_recvmsg(int fd, struct msghdr *msg, int flags, int timeout)
{
#ifdef MSG_DONTWAIT
	if (timeout >= 0)
		flags |= MSG_DONTWAIT;
#endif

	ssize_t result = 0;
	int errsv = errno;
	for (;;) {
		errno = errsv;
		// Try to receive a message.
		result = recvmsg(fd, msg, flags);
		if (result >= 0)
			break;
		if (errno == EINTR)
			continue;
		if (!timeout || (errno != EAGAIN && errno != EWOULDBLOCK))
			return -1;
		// Wait for a message to arrive.
		// clang-format off
		int events = (flags & MSG_OOB)
				? (POLLRDBAND | POLLPRI) : POLLRDNORM;
		// clang-format on
		if (io_fd_wait(fd, &events, timeout) == -1)
			return -1;
		// Since the timeout is relative, we can only use a positive
		// value once.
		if (timeout > 0)
			timeout = 0;
	}

	return result;
}

ssize_t
io_fd_sendmsg(int fd, const struct msghdr *msg, int flags, int timeout)
{
	flags |= MSG_NOSIGNAL;
#ifdef MSG_DONTWAIT
	if (timeout >= 0)
		flags |= MSG_DONTWAIT;
#endif

	ssize_t result = 0;
	int errsv = errno;
	for (;;) {
		errno = errsv;
		// Try to send a message.
		result = sendmsg(fd, msg, flags);
		if (result >= 0)
			break;
		if (errno == EINTR)
			continue;
		if (!timeout || (errno != EAGAIN && errno != EWOULDBLOCK))
			return -1;
		// Wait for the socket to become ready.
		int events = (flags & MSG_OOB) ? POLLWRBAND : POLLWRNORM;
		if (io_fd_wait(fd, &events, timeout) == -1)
			return -1;
		// Since the timeout is relative, we can only use a positive
		// value once.
		if (timeout > 0)
			timeout = 0;
	}
	return result;
}

#endif // !LELY_NO_STDIO && _POSIX_C_SOURCE >= 200112L
