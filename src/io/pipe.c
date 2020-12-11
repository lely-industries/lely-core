/**@file
 * This file is part of the I/O library; it contains the implementation of the
 * pipe functions.
 *
 * @see lely/io/pipe.h
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

#include <lely/io/pipe.h>

#include "default.h"

#if _WIN32 || _POSIX_C_SOURCE >= 200112L

#if _WIN32
#include <lely/libc/stdio.h>

#ifdef __MINGW32__
// Ignore complaints that "I64" is not a valid ISO C length modifier.
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"
#endif

static int pipe(HANDLE fildes[2]);
#endif

static const struct io_handle_vtab pipe_vtab = { .type = IO_TYPE_PIPE,
	.size = sizeof(struct io_handle),
	.fini = &default_fini,
	.flags = &default_flags,
	.read = &default_read,
	.write = &default_write };

int
io_open_pipe(io_handle_t handle_vector[2])
{
	assert(handle_vector);
	handle_vector[0] = handle_vector[1] = IO_HANDLE_ERROR;

	int errc = 0;

#if _WIN32
	HANDLE fd[2];
#else
	int fd[2];
#endif
#if (defined(__CYGWIN__) || defined(__linux__)) && defined(_GNU_SOURCE)
	if (pipe2(fd, O_CLOEXEC) == -1) {
#else
	if (pipe(fd) == -1) {
#endif
		errc = get_errc();
		goto error_pipe;
	}

#if _POSIX_C_SOURCE >= 200112L && !defined(__CYGINW__) && !defined(__linux__)
	if (fcntl(fd[0], F_SETFD, FD_CLOEXEC) == -1) {
		errc = get_errc();
		goto error_fcntl;
	}
	if (fcntl(fd[1], F_SETFD, FD_CLOEXEC) == -1) {
		errc = get_errc();
		goto error_fcntl;
	}
#endif

	handle_vector[0] = io_handle_alloc(&pipe_vtab);
	if (!handle_vector[0]) {
		errc = get_errc();
		goto error_alloc_handle_vector_0;
	}
	handle_vector[0]->fd = fd[0];

	handle_vector[1] = io_handle_alloc(&pipe_vtab);
	if (!handle_vector[1]) {
		errc = get_errc();
		goto error_alloc_handle_vector_1;
	}
	handle_vector[1]->fd = fd[1];

	io_handle_acquire(handle_vector[0]);
	io_handle_acquire(handle_vector[1]);

	return 0;

error_alloc_handle_vector_1:
	handle_vector[1] = IO_HANDLE_ERROR;
	io_handle_free(handle_vector[0]);
error_alloc_handle_vector_0:
	handle_vector[0] = IO_HANDLE_ERROR;
#if _POSIX_C_SOURCE >= 200112L && !defined(__CYGINW__) && !defined(__linux__)
error_fcntl:
#endif
#if _WIN32
	CloseHandle(fd[1]);
	CloseHandle(fd[0]);
#else
	close(fd[1]);
	close(fd[0]);
#endif
error_pipe:
	set_errc(errc);
	return -1;
}

#if _WIN32
static int
pipe(HANDLE fildes[2])
{
	assert(fildes);
	fildes[0] = fildes[1] = INVALID_HANDLE_VALUE;

	DWORD dwErrCode = 0;

	CHAR Name[MAX_PATH] = { 0 };
	static LONGLONG cnt;
	snprintf(Name, sizeof(Name) - 1,
			"\\\\.\\pipe\\lely-io-pipe-%04lx-%08I64x",
			GetCurrentProcessId(), InterlockedIncrement64(&cnt));

	fildes[0] = CreateNamedPipeA(Name,
			PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_BYTE | PIPE_WAIT, 1, 1, 1, 0, NULL);
	if (fildes[0] == INVALID_HANDLE_VALUE) {
		dwErrCode = GetLastError();
		goto error_CreateNamedPipeA;
	}

	fildes[1] = CreateFileA(Name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	if (fildes[1] == INVALID_HANDLE_VALUE) {
		dwErrCode = GetLastError();
		goto error_CreateFileA;
	}

	return 0;

error_CreateFileA:
	CloseHandle(fildes[0]);
	fildes[0] = INVALID_HANDLE_VALUE;
error_CreateNamedPipeA:
	SetLastError(dwErrCode);
	return -1;
}
#endif

#endif // _WIN32 || _POSIX_C_SOURCE >= 200112L

#endif // !LELY_NO_STDIO
