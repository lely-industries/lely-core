/*!\file
 * This file is part of the I/O library; it contains the implementation of the
 * regular file functions.
 *
 * \see lely/io/file.h
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

#ifdef __linux__
// This needs to be defined before any files are included to make lseek64()
// available.
#define _LARGEFILE64_SOURCE	1
#endif

#include "io.h"
#include <lely/util/errnum.h>
#include <lely/io/file.h>
#include "handle.h"

#include <assert.h>

#if defined(_WIN32) || _POSIX_C_SOURCE >= 200112L

//! A regular file handle.
struct file {
	//! The I/O device base handle.
	struct io_handle base;
	/*!
	 * The file flags (any combination of #IO_FILE_READ, #IO_FILE_WRITE,
	 * #IO_FILE_APPEND, #IO_FILE_CREATE, #IO_FILE_NO_EXIST and
	 * #IO_FILE_TRUNCATE).
	 */
	int flags;
};

static void file_fini(struct io_handle *handle);
static ssize_t file_read(struct io_handle *handle, void *buf, size_t nbytes);
static ssize_t file_write(struct io_handle *handle, const void *buf,
		size_t nbytes);
static int file_flush(struct io_handle *handle);
io_off_t file_seek(struct io_handle *handle, io_off_t offset, int whence);
static ssize_t file_pread(struct io_handle *handle, void *buf, size_t nbytes,
		io_off_t offset);
static ssize_t file_pwrite(struct io_handle *handle, const void *buf,
		size_t nbytes, io_off_t offset);

static const struct io_handle_vtab file_vtab = {
	.size = sizeof(struct file),
	.fini = &file_fini,
	.read = &file_read,
	.write = &file_write,
	.flush = &file_flush,
	.pread = &file_pread,
	.pwrite = &file_pwrite
};

#ifdef _WIN32
static ssize_t _file_read(struct io_handle *handle, void *buf, size_t nbytes,
		io_off_t offset);
static ssize_t _file_write(struct io_handle *handle, const void *buf,
		size_t nbytes, io_off_t offset);
#endif

LELY_IO_EXPORT io_handle_t
io_open_file(const char *path, int flags)
{
	assert(path);

	errc_t errc = 0;

	if (__unlikely(!(flags & (IO_FILE_READ | IO_FILE_WRITE)))) {
		errc = errnum2c(ERRNUM_INVAL);
		goto error_param;
	}

	if (!(flags & IO_FILE_WRITE))
		flags &= ~(IO_FILE_APPEND | IO_FILE_CREATE | IO_FILE_NO_EXIST
				| IO_FILE_TRUNCATE);
	if (!(flags & IO_FILE_CREATE))
		flags &= ~IO_FILE_NO_EXIST;
	if (flags & IO_FILE_NO_EXIST)
		flags &= ~IO_FILE_TRUNCATE;

#ifdef _WIN32
	DWORD dwDesiredAccess = 0;
	if (flags & IO_FILE_READ)
		dwDesiredAccess |= FILE_READ_DATA;
	if (flags & IO_FILE_APPEND) {
		dwDesiredAccess |= FILE_APPEND_DATA;
	} else if (flags & IO_FILE_WRITE) {
		dwDesiredAccess |= FILE_WRITE_DATA;
	}

	DWORD dwCreationDisposition
			= (flags & IO_FILE_CREATE) && (flags & IO_FILE_NO_EXIST)
					? CREATE_NEW
			: (flags & IO_FILE_CREATE) && (flags & IO_FILE_TRUNCATE)
					? CREATE_ALWAYS
			: (flags & IO_FILE_CREATE) ? OPEN_ALWAYS
			: (flags & IO_FILE_TRUNCATE) ? TRUNCATE_EXISTING
			: OPEN_EXISTING;

	HANDLE fd = CreateFileA(path, dwDesiredAccess, 0, NULL,
			dwCreationDisposition, FILE_FLAG_OVERLAPPED, NULL);
#else
	int oflag = (flags & IO_FILE_READ) && (flags & IO_FILE_WRITE) ? O_RDWR
			: flags & IO_FILE_READ ? O_RDONLY
			: flags & IO_FILE_WRITE ? O_WRONLY
			: 0;
	if (flags & IO_FILE_APPEND)
		oflag |= O_APPEND;
	if (flags & IO_FILE_CREATE) {
		oflag |= O_CREAT;
		if (flags & IO_FILE_NO_EXIST)
			oflag |= O_EXCL;
	}
	if (flags & IO_FILE_TRUNCATE)
		oflag |= O_TRUNC;
	oflag |= O_CLOEXEC;

	// When creating a new file, enable read permission for all, and write
	// permission for the owner and the group (664). Note that this value is
	// modified by the process' umask.
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

	int fd;
	do fd = open(path, oflag, mode);
	while (__unlikely(fd == -1 && errno == EINTR));
#endif

	if (__unlikely(fd == INVALID_HANDLE_VALUE)) {
		errc = get_errc();
		goto error_open;
	}

	struct io_handle *handle = io_handle_alloc(&file_vtab);
	if (__unlikely(!handle)) {
		errc = get_errc();
		goto error_alloc_handle;
	}

	handle->fd = fd;
	((struct file *)handle)->flags = flags;

	return handle;

error_alloc_handle:
#ifdef _WIN32
	CloseHandle(fd);
#else
	close(fd);
#endif
error_open:
error_param:
	set_errc(errc);
	return IO_HANDLE_ERROR;
}

#endif // _WIN32 || _POSIX_C_SOURCE >= 200112L

LELY_IO_EXPORT io_off_t
io_seek(io_handle_t handle, io_off_t offset, int whence)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	assert(handle->vtab);
	if (__unlikely(!handle->vtab->seek)) {
		set_errnum(ERRNUM_SPIPE);
		return -1;
	}

	return handle->vtab->seek(handle, offset, whence);
}

LELY_IO_EXPORT ssize_t
io_pread(io_handle_t handle, void *buf, size_t nbytes, io_off_t offset)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	assert(handle->vtab);
	if (__unlikely(!handle->vtab->pread)) {
		set_errnum(ERRNUM_SPIPE);
		return -1;
	}

	return handle->vtab->pread(handle, buf, nbytes, offset);
}

LELY_IO_EXPORT ssize_t
io_pwrite(io_handle_t handle, const void *buf, size_t nbytes, io_off_t offset)
{
	if (__unlikely(handle == IO_HANDLE_ERROR)) {
		set_errnum(ERRNUM_BADF);
		return -1;
	}

	assert(handle->vtab);
	if (__unlikely(!handle->vtab->pwrite)) {
		set_errnum(ERRNUM_SPIPE);
		return -1;
	}

	return handle->vtab->pwrite(handle, buf, nbytes, offset);
}

#if defined(_WIN32) || _POSIX_C_SOURCE >= 200112L

static void
file_fini(struct io_handle *handle)
{
	assert(handle);

#ifdef _WIN32
	CloseHandle(handle->fd) ? 0 : -1;
#else
	close(handle->fd);
#endif
}

static ssize_t
file_read(struct io_handle *handle, void *buf, size_t nbytes)
{
	assert(handle);

#ifdef _WIN32
	io_off_t current = file_seek(handle, 0, IO_SEEK_CURRENT);
	if (__unlikely(current == -1))
		return -1;
	return _file_read(handle, buf, nbytes, current);
#else
	ssize_t result;
	do result = read(handle->fd, buf, nbytes);
	while (__unlikely(result == -1 && errno == EINTR));
	return result;
#endif
}

static ssize_t
file_write(struct io_handle *handle, const void *buf, size_t nbytes)
{
	assert(handle);

#ifdef _WIN32
	io_off_t current;
	if (((struct file *)handle)->flags & IO_FILE_APPEND) {
		// This value of the offset causes WriteFile() to write to the
		// end of file.
		current = UINT64_MAX;
	} else {
		current = file_seek(handle, 0, IO_SEEK_CURRENT);
		if (__unlikely(current == -1))
			return -1;
	}
	return _file_write(handle, buf, nbytes, current);
#else
	ssize_t result;
	do result = write(handle->fd, buf, nbytes);
	while (__unlikely(result == -1 && errno == EINTR));
	return result;
#endif
}

static int
file_flush(struct io_handle *handle)
{
	assert(handle);

#ifdef _WIN32
	return FlushFileBuffers(handle->fd) ? 0 : -1;
#else
	int result;
	do result = fsync(handle->fd);
	while (__unlikely(result == -1 && errno == EINTR));
	return result;
#endif
}

io_off_t
file_seek(struct io_handle *handle, io_off_t offset, int whence)
{
	assert(handle);

#ifdef _WIN32
	DWORD dwMoveMethod;
	switch (whence) {
	case IO_SEEK_BEGIN:
		dwMoveMethod = FILE_BEGIN;
		break;
	case IO_SEEK_CURRENT:
		dwMoveMethod = FILE_CURRENT;
		break;
	case IO_SEEK_END:
		dwMoveMethod = FILE_END;
		break;
	default:
		SetLastError(ERROR_INVALID_PARAMETER);
		return -1;
	}

	LARGE_INTEGER li;
	li.QuadPart = offset;
	li.LowPart = SetFilePointer(handle->fd, li.LowPart, &li.HighPart,
			dwMoveMethod);
	if (__unlikely(li.LowPart == INVALID_SET_FILE_POINTER))
		return -1;
	return li.QuadPart;
#else
	switch (whence) {
	case IO_SEEK_BEGIN:
		whence = SEEK_SET;
		break;
	case IO_SEEK_CURRENT:
		whence = SEEK_CUR;
		break;
	case IO_SEEK_END:
		whence = SEEK_END;
		break;
	default:
		errno = EINVAL;
		return -1;
	}

#if defined(__linux__)
	return lseek64(handle->fd, offset, whence);
#else
	return lseek(handle->fd, offset, whence);
#endif
#endif
}

static ssize_t
file_pread(struct io_handle *handle, void *buf, size_t nbytes, io_off_t offset)
{
	assert(handle);

#ifdef _WIN32
	io_off_t current = file_seek(handle, 0, IO_SEEK_CURRENT);
	if (__unlikely(current == -1))
		return -1;
	ssize_t result = _file_read(handle, buf, nbytes, offset);
	if (__unlikely(result == -1))
		return -1;
	// pread() does not change the file pointer.
	if (__unlikely(file_seek(handle, current, IO_SEEK_BEGIN) == -1))
		return current;
	return result;
#else
	ssize_t result;
	do result = pread(handle->fd, buf, nbytes, offset);
	while (__unlikely(result == -1 && errno == EINTR));
	return result;
#endif
}

static ssize_t
file_pwrite(struct io_handle *handle, const void *buf, size_t nbytes,
		io_off_t offset)
{
	assert(handle);

#ifdef _WIN32
	io_off_t current = file_seek(handle, 0, IO_SEEK_CURRENT);
	if (__unlikely(current == -1))
		return -1;
	ssize_t result = _file_write(handle, buf, nbytes, offset);
	if (__unlikely(result == -1))
		return -1;
	// pwrite() does not change the file pointer.
	if (__unlikely(file_seek(handle, current, IO_SEEK_BEGIN) == -1))
		return -1;
	return result;
#else
	ssize_t result;
	do result = pwrite(handle->fd, buf, nbytes, offset);
	while (__unlikely(result == -1 && errno == EINTR));
	return result;
#endif
}

#ifdef _WIN32

static ssize_t
_file_read(struct io_handle *handle, void *buf, size_t nbytes, io_off_t offset)
{
	assert(handle);

	DWORD dwErrCode = GetLastError();

	LARGE_INTEGER li;
	li.QuadPart = offset;
	OVERLAPPED overlapped = { 0 };
	overlapped.Offset = li.LowPart;
	overlapped.OffsetHigh = li.HighPart;
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (__unlikely(!overlapped.hEvent)) {
		dwErrCode = GetLastError();
		goto error_CreateEvent;
	}

	DWORD dwNumberOfBytesRead = 0;

	if (ReadFile(handle->fd, buf, nbytes, &dwNumberOfBytesRead,
			&overlapped))
		goto done;

	if (__unlikely(GetLastError() != ERROR_IO_PENDING)) {
		dwErrCode = GetLastError();
		goto error_ReadFile;
	}

	if (__unlikely(!GetOverlappedResult(handle->fd, &overlapped,
			&dwNumberOfBytesRead, TRUE))) {
		dwErrCode = GetLastError();
		goto error_GetOverlappedResult;
	}

done:
	CloseHandle(overlapped.hEvent);
	SetLastError(dwErrCode);
	return dwNumberOfBytesRead;

error_GetOverlappedResult:
error_ReadFile:
	CloseHandle(overlapped.hEvent);
error_CreateEvent:
	SetLastError(dwErrCode);
	return -1;
}

static ssize_t
_file_write(struct io_handle *handle, const void *buf, size_t nbytes,
		io_off_t offset)
{
	assert(handle);

	DWORD dwErrCode = GetLastError();

	LARGE_INTEGER li;
	li.QuadPart = offset;
	OVERLAPPED overlapped = { 0 };
	overlapped.Offset = li.LowPart;
	overlapped.OffsetHigh = li.HighPart;
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (__unlikely(!overlapped.hEvent)) {
		dwErrCode = GetLastError();
		goto error_CreateEvent;
	}

	DWORD dwNumberOfBytesWritten = 0;

	if (WriteFile(handle->fd, buf, nbytes, &dwNumberOfBytesWritten,
			&overlapped))
		goto done;

	if (__unlikely(GetLastError() != ERROR_IO_PENDING)) {
		dwErrCode = GetLastError();
		goto error_WriteFile;
	}

	if (__unlikely(!GetOverlappedResult(handle->fd, &overlapped,
			&dwNumberOfBytesWritten, TRUE))) {
		dwErrCode = GetLastError();
		goto error_GetOverlappedResult;
	}

done:
	CloseHandle(overlapped.hEvent);
	SetLastError(dwErrCode);
	return dwNumberOfBytesWritten;

error_GetOverlappedResult:
error_WriteFile:
	CloseHandle(overlapped.hEvent);
error_CreateEvent:
	SetLastError(dwErrCode);
	return -1;
}

#endif // _WIN32

#endif // _WIN32 || _POSIX_C_SOURCE >= 200112L

