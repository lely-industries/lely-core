/**@file
 * This file is part of the asynchronous I/O library; it contains ...
 *
 * @see lely/aio/self_pipe.h
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

#include "../aio.h"

#ifdef __linux__

#include <lely/aio/self_pipe.h>

#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include <sys/eventfd.h>

int
aio_self_pipe_open(struct aio_self_pipe *pipe)
{
	assert(pipe);

	int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	if (efd == -1)
		return -1;

	pipe->handles[0] = pipe->handles[1] = efd;

	return 0;
}

int
aio_self_pipe_is_open(struct aio_self_pipe *pipe)
{
	assert(pipe);
	assert(pipe->handles[0] == pipe->handles[1]);
	int efd = pipe->handles[0];

	return efd != -1;
}

int
aio_self_pipe_close(struct aio_self_pipe *pipe)
{
	assert(pipe);
	assert(pipe->handles[0] == pipe->handles[1]);
	int efd = pipe->handles[0];

	pipe->handles[0] = pipe->handles[1] = -1;
	return close(efd);
}

ssize_t
aio_self_pipe_read(struct aio_self_pipe *pipe)
{
	assert(pipe);
	assert(pipe->handles[0] == pipe->handles[1]);
	int efd = pipe->handles[0];

	ssize_t events = 0;

	int errsv = errno;
	ssize_t result;
	do {
		errno = errsv;
		uint64_t value = 0;
		result = read(efd, &value, sizeof(value));

		if (result == sizeof(value)) {
			if (value > SSIZE_MAX)
				value = SSIZE_MAX;
			if (events > (ssize_t)(SSIZE_MAX - value))
				events = SSIZE_MAX;
			else
				events += value;
		}
	} while (result == sizeof(uint64_t)
			|| (result == -1 && errno == EINTR));

	if (result == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		errno = errsv;
		result = 0;
	}

	return result == -1 ? -1 : events;
}

ssize_t
aio_self_pipe_write(struct aio_self_pipe *pipe)
{
	assert(pipe);
	assert(pipe->handles[0] == pipe->handles[1]);
	int efd = pipe->handles[1];

	int errsv = errno;
	ssize_t result;
	do {
		errno = errsv;
		uint64_t value = 1;
		result = write(efd, &value, sizeof(value));
	} while (result == -1 && errno == EINTR);

	if (result == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		errno = errsv;
		result = 0;
	}

	return result == sizeof(uint64_t) ? 1 : result;
}

#endif // __linux__
