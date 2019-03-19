/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * common file descriptor functions.
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

#include "fd.h"

#if _POSIX_C_SOURCE >= 200112L

#include <fcntl.h>

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

#endif // _POSIX_C_SOURCE >= 200112L
