/**@file
 * This is the internal header file of the default implementation of the I/O
 * device handle methods.
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

#ifndef LELY_IO_INTERN_DEFAULT_H_
#define LELY_IO_INTERN_DEFAULT_H_

#include "handle.h"
#include "io.h"
#include <lely/util/errnum.h>

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#if _WIN32 || _POSIX_C_SOURCE >= 200112L

static inline void default_fini(struct io_handle *handle);
static inline int default_flags(struct io_handle *handle, int flags);
static inline ssize_t default_read(
		struct io_handle *handle, void *buf, size_t nbytes);
static inline ssize_t default_write(
		struct io_handle *handle, const void *buf, size_t nbytes);

static inline void
default_fini(struct io_handle *handle)
{
	assert(handle);

	if (!(handle->flags & IO_FLAG_NO_CLOSE))
#if _WIN32
		CloseHandle(handle->fd);
#else
		close(handle->fd);
#endif
}

static inline int
default_flags(struct io_handle *handle, int flags)
{
	assert(handle);

#if _WIN32
	(void)handle;
	(void)flags;

	return 0;
#else
	int arg = fcntl(handle->fd, F_GETFL, 0);
	if (arg == -1)
		return -1;

	if ((flags & IO_FLAG_NONBLOCK) && !(arg & O_NONBLOCK))
		return fcntl(handle->fd, F_SETFL, arg | O_NONBLOCK);
	else if (!(flags & IO_FLAG_NONBLOCK) && (arg & O_NONBLOCK))
		return fcntl(handle->fd, F_SETFL, arg & ~O_NONBLOCK);
	return 0;
#endif
}

static inline ssize_t
default_read(struct io_handle *handle, void *buf, size_t nbytes)
{
	assert(handle);

#if _WIN32
	DWORD dwErrCode = GetLastError();

	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!overlapped.hEvent) {
		dwErrCode = GetLastError();
		goto error_CreateEvent;
	}

	DWORD dwNumberOfBytesRead = 0;

retry:
	io_handle_lock(handle);
	int flags = handle->flags;
	io_handle_unlock(handle);

	// clang-format off
	if (ReadFile(handle->fd, buf, nbytes, &dwNumberOfBytesRead,
			&overlapped))
		// clang-format on
		goto done;

	switch (GetLastError()) {
	case ERROR_IO_PENDING: break;
	case ERROR_OPERATION_ABORTED:
		if (ClearCommError(handle->fd, NULL, NULL))
			goto retry;
		// ... falls through ...
	default: dwErrCode = GetLastError(); goto error_ReadFile;
	}

	if ((flags & IO_FLAG_NONBLOCK) && !CancelIoEx(handle->fd, &overlapped)
			&& GetLastError() == ERROR_NOT_FOUND) {
		dwErrCode = GetLastError();
		goto error_CancelIoEx;
	}

	// clang-format off
	if (!GetOverlappedResult(handle->fd, &overlapped, &dwNumberOfBytesRead,
			TRUE)) {
		// clang-format on
		dwErrCode = GetLastError();
		goto error_GetOverlappedResult;
	}

	if (nbytes && !dwNumberOfBytesRead) {
		if (!(flags & IO_FLAG_NONBLOCK))
			goto retry;
		dwErrCode = errnum2c(ERRNUM_AGAIN);
		goto error_dwNumberOfBytesRead;
	}

done:
	CloseHandle(overlapped.hEvent);
	SetLastError(dwErrCode);
	return dwNumberOfBytesRead;

error_dwNumberOfBytesRead:
error_GetOverlappedResult:
error_CancelIoEx:
error_ReadFile:
	CloseHandle(overlapped.hEvent);
error_CreateEvent:
	SetLastError(dwErrCode);
	return -1;
#else
	ssize_t result;
	int errsv = errno;
	do {
		errno = errsv;
		result = read(handle->fd, buf, nbytes);
	} while (result == -1 && errno == EINTR);
	return result;
#endif
}

static inline ssize_t
default_write(struct io_handle *handle, const void *buf, size_t nbytes)
{
	assert(handle);

#if _WIN32
	DWORD dwErrCode = GetLastError();

	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!overlapped.hEvent) {
		dwErrCode = GetLastError();
		goto error_CreateEvent;
	}

	DWORD dwNumberOfBytesWritten = 0;

retry:
	io_handle_lock(handle);
	int flags = handle->flags;
	io_handle_unlock(handle);

	// clang-format off
	if (WriteFile(handle->fd, buf, nbytes, &dwNumberOfBytesWritten,
			&overlapped))
		// clang-format on
		goto done;

	switch (GetLastError()) {
	case ERROR_IO_PENDING: break;
	case ERROR_OPERATION_ABORTED:
		if (ClearCommError(handle->fd, NULL, NULL))
			goto retry;
		// ... falls through ...
	default: dwErrCode = GetLastError(); goto error_WriteFile;
	}

	if ((flags & IO_FLAG_NONBLOCK) && !CancelIoEx(handle->fd, &overlapped)
			&& GetLastError() == ERROR_NOT_FOUND) {
		dwErrCode = GetLastError();
		goto error_CancelIoEx;
	}

	// clang-format off
	if (!GetOverlappedResult(handle->fd, &overlapped,
			&dwNumberOfBytesWritten, TRUE)) {
		// clang-format on
		dwErrCode = GetLastError();
		goto error_GetOverlappedResult;
	}

	if (nbytes && !dwNumberOfBytesWritten) {
		if (!(flags & IO_FLAG_NONBLOCK))
			goto retry;
		dwErrCode = errnum2c(ERRNUM_AGAIN);
		goto error_dwNumberOfBytesWritten;
	}

done:
	CloseHandle(overlapped.hEvent);
	SetLastError(dwErrCode);
	return dwNumberOfBytesWritten;

error_dwNumberOfBytesWritten:
error_GetOverlappedResult:
error_CancelIoEx:
error_WriteFile:
	CloseHandle(overlapped.hEvent);
error_CreateEvent:
	SetLastError(dwErrCode);
	return -1;
#else
	ssize_t result;
	int errsv = errno;
	do {
		errno = errsv;
		result = write(handle->fd, buf, nbytes);
	} while (result == -1 && errno == EINTR);
	return result;
#endif
}

#endif // _WIN32 || _POSIX_C_SOURCE >= 200112L

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO_INTERN_DEFAULT_H_
