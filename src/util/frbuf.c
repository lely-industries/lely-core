/**@file
 * This file is part of the utilities library; it contains the implementation of
 * the read file buffer.
 *
 * @see lely/util/frbuf.h
 *
 * @copyright 2016-2020 Lely Industries N.V.
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

#if !_WIN32
// This needs to be defined before any files are included to make fstat64(),
// lseek64(), mmap64() and pread64() available.
#define _LARGEFILE64_SOURCE 1
#endif

#include "util.h"

#if !LELY_NO_STDIO

#include <lely/util/errnum.h>
#include <lely/util/frbuf.h>

#include <assert.h>
#include <stdlib.h>

#if !_WIN32
#include <stdio.h>
#if _POSIX_C_SOURCE >= 200112L
#include <fcntl.h>
#if _POSIX_MAPPED_FILES >= 200112L
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#endif
#endif

/// An read file buffer struct.
struct __frbuf {
#if _WIN32
	/// The file handle.
	HANDLE hFile;
	/// The handle of the file mapping.
	HANDLE hFileMappingObject;
	/// The base address of the file mapping.
	LPVOID lpBaseAddress;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
	/// The file descriptor.
	int fd;
	/// The base address of the current file mapping.
	void *addr;
	/// The length (in bytes) of the mapping at <b>addr</b>.
	size_t len;
#else
	/// The file stream.
	FILE *stream;
	/// The address of the file mapping.
	void *map;
#endif
};

void *
__frbuf_alloc(void)
{
	void *ptr = malloc(sizeof(struct __frbuf));
	if (!ptr)
		set_errc(errno2c(errno));
	return ptr;
}

void
__frbuf_free(void *ptr)
{
	free(ptr);
}

struct __frbuf *
__frbuf_init(struct __frbuf *buf, const char *filename)
{
	assert(buf);
	assert(filename);

#if _WIN32
	buf->hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_READONLY | FILE_FLAG_NO_BUFFERING, NULL);
	if (buf->hFile == INVALID_HANDLE_VALUE)
		return NULL;

	buf->hFileMappingObject = INVALID_HANDLE_VALUE;
	buf->lpBaseAddress = NULL;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
	buf->fd = open(filename, O_RDONLY | O_CLOEXEC);
	if (buf->fd == -1)
		return NULL;

	buf->addr = MAP_FAILED;
	buf->len = 0;
#else
	buf->stream = fopen(filename, "rb");
	if (!buf->stream)
		return NULL;
#endif

	return buf;
}

void
__frbuf_fini(struct __frbuf *buf)
{
	frbuf_unmap(buf);

#if _WIN32
	CloseHandle(buf->hFile);
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
	close(buf->fd);
#else
	fclose(buf->stream);
#endif
}

frbuf_t *
frbuf_create(const char *filename)
{
	int errc = 0;

	frbuf_t *buf = __frbuf_alloc();
	if (!buf) {
		errc = get_errc();
		goto error_alloc_buf;
	}

	if (!__frbuf_init(buf, filename)) {
		errc = get_errc();
		goto error_init_buf;
	}

	return buf;

error_init_buf:
	__frbuf_free(buf);
error_alloc_buf:
	set_errc(errc);
	return NULL;
}

void
frbuf_destroy(frbuf_t *buf)
{
	if (buf) {
		__frbuf_fini(buf);
		__frbuf_free(buf);
	}
}

intmax_t
frbuf_get_size(frbuf_t *buf)
{
	assert(buf);

#if _WIN32
	LARGE_INTEGER FileSize;
	if (!GetFileSizeEx(buf->hFile, &FileSize))
		return -1;
	return FileSize.QuadPart;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
#ifdef __linux__
	struct stat64 stat;
	if (fstat64(buf->fd, &stat) == -1)
		return -1;
#else
	struct stat stat;
	if (fstat(buf->fd, &stat) == -1)
		return -1;
#endif
	return stat.st_size;
#else
	long offset = ftell(buf->stream);
	if (offset == -1) {
		set_errc(errno2c(errno));
		return -1;
	}

	// WARNING: This is not guaranteed to work, but there exists no standard
	// C alternative.
	if (fseek(buf->stream, 0, SEEK_END)) {
		set_errc(errno2c(errno));
		return -1;
	}

	long size = ftell(buf->stream);

	int errc = get_errc();
	fseek(buf->stream, offset, SEEK_SET);
	set_errc(errc);

	return size;
#endif
}

intmax_t
frbuf_get_pos(frbuf_t *buf)
{
	assert(buf);

#if _WIN32
	LARGE_INTEGER li = { .QuadPart = 0 };
	if (!SetFilePointerEx(buf->hFile, li, &li, FILE_CURRENT))
		return -1;
	return li.QuadPart;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
#ifdef __linux__
	return lseek64(buf->fd, 0, SEEK_CUR);
#else
	return lseek(buf->fd, 0, SEEK_CUR);
#endif
#else
	long pos = ftell(buf->stream);
	if (pos == -1)
		set_errc(errno2c(errno));
	return pos;
#endif
}

intmax_t
frbuf_set_pos(frbuf_t *buf, intmax_t pos)
{
	assert(buf);

	if (pos < 0) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

#if _WIN32
	LARGE_INTEGER li = { .QuadPart = pos };
	if (!SetFilePointerEx(buf->hFile, li, &li, FILE_BEGIN))
		return -1;
	return li.QuadPart;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
#ifdef __linux__
	return lseek64(buf->fd, pos, SEEK_SET);
#else
	return lseek(buf->fd, pos, SEEK_SET);
#endif
#else
	if (pos > LONG_MAX) {
		set_errnum(ERRNUM_OVERFLOW);
		return -1;
	}
	if (fseek(buf->stream, pos, SEEK_SET)) {
		set_errc(errno2c(errno));
		return -1;
	}
	return frbuf_get_pos(buf);
#endif
}

ssize_t
frbuf_read(frbuf_t *buf, void *ptr, size_t size)
{
	assert(buf);
	assert(ptr || !size);

	if (!size)
		return 0;

#if _WIN32
	DWORD nNumberOfBytesRead;
	if (!ReadFile(buf->hFile, ptr, size, &nNumberOfBytesRead, NULL))
		return -1;
	return nNumberOfBytesRead;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
	ssize_t result;
	do
		result = read(buf->fd, ptr, size);
	while (result == -1 && errno == EINTR);
	return result;
#else
	size_t result = fread(ptr, 1, size, buf->stream);
	if (result != size && ferror(buf->stream)) {
		set_errc(errno2c(errno));
		if (!result)
			return -1;
	}
	return result;
#endif
}

ssize_t
frbuf_pread(frbuf_t *buf, void *ptr, size_t size, intmax_t pos)
{
	assert(buf);
	assert(ptr || !size);

	if (!size)
		return 0;

	if (pos < 0) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

#if _WIN32
	ssize_t result = 0;
	DWORD dwErrCode = GetLastError();

	intmax_t oldpos = frbuf_get_pos(buf);
	if (oldpos == -1) {
		result = -1;
		dwErrCode = GetLastError();
		goto error_get_pos;
	}

	DWORD nNumberOfBytesRead;
	OVERLAPPED Overlapped = { 0 };
	ULARGE_INTEGER uli = { .QuadPart = pos };
	Overlapped.Offset = uli.LowPart;
	Overlapped.OffsetHigh = uli.HighPart;
	// clang-format off
	if (!ReadFile(buf->hFile, ptr, size, &nNumberOfBytesRead,
			&Overlapped)) {
		// clang-format on
		result = -1;
		dwErrCode = GetLastError();
		goto error_ReadFile;
	}

	result = nNumberOfBytesRead;

error_ReadFile:
	if (frbuf_set_pos(buf, oldpos) == -1 && !dwErrCode)
		dwErrCode = GetLastError();
error_get_pos:
	SetLastError(dwErrCode);
	return result;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
	ssize_t result;
#ifdef __linux__
	do
		result = pread64(buf->fd, ptr, size, pos);
#else
	do
		result = pread(buf->fd, ptr, size, pos);
#endif
	while (result == -1 && errno == EINTR);
	return result;
#else
	ssize_t result = 0;
	int errc = get_errc();

	intmax_t oldpos = frbuf_get_pos(buf);
	if (oldpos == -1) {
		result = -1;
		errc = get_errc();
		goto error_get_pos;
	}

	if (frbuf_set_pos(buf, pos) != pos) {
		result = -1;
		errc = get_errc();
		goto error_set_pos;
	}

	result = frbuf_read(buf, ptr, size);
	if (result == -1 || (size_t)result != size)
		errc = get_errc();

error_set_pos:
	if (frbuf_set_pos(buf, oldpos) == -1 && !errc)
		errc = get_errc();
error_get_pos:
	set_errc(errc);
	return result;
#endif
}

const void *
frbuf_map(frbuf_t *buf, intmax_t pos, size_t *psize)
{
	frbuf_unmap(buf);

	intmax_t size = frbuf_get_size(buf);
	if (size < 0)
		return NULL;
	if (pos < 0) {
		set_errnum(ERRNUM_INVAL);
		return NULL;
	}
	if (pos > (intmax_t)size) {
		set_errnum(ERRNUM_OVERFLOW);
		return NULL;
	}
	size -= pos;

	if (psize && *psize)
		size = MIN((uintmax_t)size, *psize);

#if _WIN32
	DWORD dwErrCode = 0;

	SYSTEM_INFO SystemInfo;
	// cppcheck-suppress uninitvar
	GetSystemInfo(&SystemInfo);
	DWORD off = pos % SystemInfo.dwAllocationGranularity;
	if ((uintmax_t)size > (uintmax_t)(SIZE_MAX - off)) {
		dwErrCode = ERROR_INVALID_PARAMETER;
		goto error_size;
	}

	ULARGE_INTEGER MaximumSize = { .QuadPart = pos + size };
	buf->hFileMappingObject = CreateFileMapping(buf->hFile, NULL,
			PAGE_READONLY, MaximumSize.HighPart,
			MaximumSize.LowPart, NULL);
	if (buf->hFileMappingObject == INVALID_HANDLE_VALUE) {
		dwErrCode = GetLastError();
		goto error_CreateFileMapping;
	}

	ULARGE_INTEGER FileOffset = { .QuadPart = pos - off };
	buf->lpBaseAddress = MapViewOfFile(buf->hFileMappingObject,
			FILE_MAP_READ, FileOffset.HighPart, FileOffset.LowPart,
			(SIZE_T)(off + size));
	if (!buf->lpBaseAddress) {
		dwErrCode = GetLastError();
		goto error_MapViewOfFile;
	}

	if (psize)
		*psize = (size_t)size;

	return (char *)buf->lpBaseAddress + off;

error_MapViewOfFile:
	CloseHandle(buf->hFileMappingObject);
	buf->hFileMappingObject = INVALID_HANDLE_VALUE;
error_CreateFileMapping:
error_size:
	SetLastError(dwErrCode);
	return NULL;
#elif _POSIX_MAPPED_FILES >= 200112L
	long page_size = sysconf(_SC_PAGE_SIZE);
	if (page_size <= 0)
		return NULL;
	intmax_t off = pos % page_size;
	if ((uintmax_t)size > (uintmax_t)(SIZE_MAX - off)) {
		errno = EOVERFLOW;
		return NULL;
	}

#ifdef __linux__
	buf->addr = mmap64(NULL, off + size, PROT_READ, MAP_SHARED, buf->fd,
			pos - off);
#else
	// TODO: Check if `pos - off` does not overflow the range of off_t.
	buf->addr = mmap(NULL, off + size, PROT_READ, MAP_SHARED, buf->fd,
			pos - off);
#endif
	if (buf->addr == MAP_FAILED)
		return NULL;
	buf->len = off + size;

	if (psize)
		*psize = size;

	return (char *)buf->addr + off;
#else
	int errc = get_errc();

	buf->map = malloc(size);
	if (!buf->map) {
		errc = errno2c(errno);
		goto error_malloc_map;
	}

	ssize_t result = frbuf_pread(buf, buf->map, size, pos);
	if (result == -1) {
		errc = get_errc();
		goto error_pread;
	}
	size = result;

	if (psize)
		*psize = size;

	return buf->map;

error_pread:
	free(buf->map);
	buf->map = NULL;
error_malloc_map:
	set_errc(errc);
	return NULL;
#endif
}

int
frbuf_unmap(frbuf_t *buf)
{
	assert(buf);

	int result = 0;

#if _WIN32
	if (buf->hFileMappingObject != INVALID_HANDLE_VALUE) {
		DWORD dwErrCode = GetLastError();
		if (!UnmapViewOfFile(buf->lpBaseAddress)) {
			result = -1;
			dwErrCode = GetLastError();
		}
		if (!CloseHandle(buf->hFileMappingObject) && !result) {
			result = -1;
			dwErrCode = GetLastError();
		}
		SetLastError(dwErrCode);

		buf->hFileMappingObject = INVALID_HANDLE_VALUE;
		buf->lpBaseAddress = NULL;
	}
#elif _POSIX_MAPPED_FILES >= 200112L
	if (buf->addr != MAP_FAILED) {
		result = munmap(buf->addr, buf->len);

		buf->addr = MAP_FAILED;
		buf->len = 0;
	}
#else
	if (buf->map) {
		free(buf->map);

		buf->map = NULL;
	}
#endif

	return result;
}

#endif // !LELY_NO_STDIO
