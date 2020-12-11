/**@file
 * This file is part of the utilities library; it contains the implementation of
 * the (atomic) write file buffer.
 *
 * @see lely/util/fwbuf.h
 *
 * @copyright 2016-2019 Lely Industries N.V.
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

#include <lely/libc/string.h>
#include <lely/util/errnum.h>
#include <lely/util/fwbuf.h>

#include <assert.h>
#include <stdlib.h>

#if _WIN32
#ifdef _MSC_VER
#pragma comment(lib, "shlwapi.lib")
#endif
#include <shlwapi.h>
#else
#include <stdio.h>
#if _POSIX_C_SOURCE >= 200112L
#include <fcntl.h>
#include <libgen.h>
#if _POSIX_MAPPED_FILES >= 200112L
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#endif
#endif

/// An (atomic) write file buffer struct.
struct __fwbuf {
	/// A pointer to the name of the file.
	char *filename;
#if _WIN32
	/// The name of the temporary file.
	char tmpname[MAX_PATH];
	/// The file handle.
	HANDLE hFile;
	/// The number of the first error that occurred during a file operation.
	DWORD dwErrCode;
	/// The handle of the file mapping.
	HANDLE hFileMappingObject;
	/// The base address of the file mapping.
	LPVOID lpBaseAddress;
	/// The number of bytes mapped at <b>lpBaseAddress</b>.
	SIZE_T dwNumberOfBytesToMap;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
	/// A pointer to the name of the temporary file.
	char *tmpname;
	/// The file descriptor of the directory containing the temporary file.
	int dirfd;
	/// The file descriptor.
	int fd;
	/// The number of the first error that occurred during a file operation.
	int errsv;
	/// The base address of the current file mapping.
	void *addr;
	/// The length (in bytes) of the mapping at <b>addr</b>.
	size_t len;
#else
	/// The name of the temporary file.
	char tmpname[L_tmpnam];
	/// The file stream.
	FILE *stream;
	/// The number of the first error that occurred during a file operation.
	int errnum;
	/// The size (in bytes) of the file.
	intmax_t size;
	/// One past the position of the last byte written to the file.
	intmax_t last;
	/// The address of the file mapping.
	void *map;
	/**
	 * The offset (in bytes) with respect to the beginning of the file of
	 * the mapping at <b>map</b>.
	 */
	intmax_t pos;
	/// The length (in bytes) of the mapping at <b>map</b>.
	size_t len;
#endif
};

void *
__fwbuf_alloc(void)
{
	void *ptr = malloc(sizeof(struct __fwbuf));
	if (!ptr)
		set_errc(errno2c(errno));
	return ptr;
}

void
__fwbuf_free(void *ptr)
{
	free(ptr);
}

struct __fwbuf *
__fwbuf_init(struct __fwbuf *buf, const char *filename)
{
	assert(buf);
	assert(filename);

#if _WIN32
	DWORD dwErrCode = 0;

	buf->filename = _strdup(filename);
	if (!buf->filename) {
		dwErrCode = errno2c(errno);
		goto error_strdup;
	}

	// Obtain the directory name.
	char dir[MAX_PATH - 14];
	strncpy(dir, buf->filename, MAX_PATH - 14 - 1);
	dir[MAX_PATH - 14 - 1] = '\0';
	PathRemoveFileSpecA(dir);
	if (!*dir) {
		dir[0] = '.';
		dir[1] = '\0';
	}

	if (!GetTempFileNameA(dir, "tmp", 0, buf->tmpname)) {
		dwErrCode = GetLastError();
		goto error_GetTempFileNameA;
	}

	buf->hFile = CreateFileA(buf->tmpname, GENERIC_READ | GENERIC_WRITE, 0,
			NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (buf->hFile == INVALID_HANDLE_VALUE) {
		dwErrCode = GetLastError();
		goto error_CreateFileA;
	}

	buf->dwErrCode = 0;

	buf->hFileMappingObject = INVALID_HANDLE_VALUE;
	buf->lpBaseAddress = NULL;
	buf->dwNumberOfBytesToMap = 0;

	return buf;

error_CreateFileA:
	DeleteFileA(buf->tmpname);
error_GetTempFileNameA:
	free(buf->filename);
error_strdup:
	SetLastError(dwErrCode);
	return NULL;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
	int errsv = 0;

	buf->filename = strdup(filename);
	if (!buf->filename) {
		errsv = errno;
		goto error_strdup_filename;
	}

	char *tmp = strdup(buf->filename);
	if (!tmp) {
		errsv = errno;
		goto error_strdup_tmp;
	}
	char *dir = dirname(tmp);
	size_t n = strlen(dir);

	buf->dirfd = open(dir, O_RDONLY | O_CLOEXEC | O_DIRECTORY);
	if (buf->dirfd == -1) {
		errsv = errno;
		goto error_open_dirfd;
	}

	buf->tmpname = malloc(n + 13);
	if (!buf->tmpname) {
		errsv = errno;
		goto error_malloc_tmpname;
	}

	strcpy(buf->tmpname, dir);
	if (!n || buf->tmpname[n - 1] != '/')
		strcat(buf->tmpname, "/.tmp-XXXXXX");
	else
		strcat(buf->tmpname, ".tmp-XXXXXX");

	free(tmp);
	tmp = NULL;

#ifdef _GNU_SOURCE
	buf->fd = mkostemp(buf->tmpname, O_CLOEXEC);
#else
	buf->fd = mkstemp(buf->tmpname);
#endif
	if (buf->fd == -1) {
		errsv = errno;
		goto error_open_fd;
	}

#ifndef _GNU_SOURCE
	if (fcntl(buf->fd, F_SETFD, FD_CLOEXEC) == -1) {
		errsv = errno;
		goto error_fcntl;
	}
#endif

	buf->errsv = 0;

	buf->addr = MAP_FAILED;
	buf->len = 0;

	return buf;

#ifndef _GNU_SOURCE
error_fcntl:
#endif
error_open_fd:
	free(buf->tmpname);
error_malloc_tmpname:
	close(buf->dirfd);
error_open_dirfd:
	if (tmp)
		free(tmp);
error_strdup_tmp:
	free(buf->filename);
error_strdup_filename:
	errno = errsv;
	return NULL;
#else
	int errc = 0;

	buf->filename = strdup(filename);
	if (!buf->filename) {
		errc = errno2c(errno);
		goto error_strdup;
	}

	// cppcheck-suppress tmpnamCalled
	buf->stream = fopen(tmpnam(buf->tmpname), "w+b");
	if (!buf->stream) {
		errc = errno2c(errno);
		goto error_fopen;
	}

	buf->errnum = 0;

	buf->size = 0;
	buf->last = 0;

	buf->map = NULL;
	buf->pos = 0;
	buf->len = 0;

	return buf;

error_fopen:
	free(buf->filename);
error_strdup:
	set_errc(errc);
	return NULL;
#endif
}

void
__fwbuf_fini(struct __fwbuf *buf)
{
	int errc = get_errc();
	fwbuf_cancel(buf);
	fwbuf_commit(buf);
	set_errc(errc);

#if _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
	free(buf->tmpname);
#endif
	free(buf->filename);
}

fwbuf_t *
fwbuf_create(const char *filename)
{
	int errc = 0;

	fwbuf_t *buf = __fwbuf_alloc();
	if (!buf) {
		errc = get_errc();
		goto error_alloc_buf;
	}

	if (!__fwbuf_init(buf, filename)) {
		errc = get_errc();
		goto error_init_buf;
	}

	return buf;

error_init_buf:
	__fwbuf_free(buf);
error_alloc_buf:
	set_errc(errc);
	return NULL;
}

void
fwbuf_destroy(fwbuf_t *buf)
{
	if (buf) {
		__fwbuf_fini(buf);
		__fwbuf_free(buf);
	}
}

intmax_t
fwbuf_get_size(fwbuf_t *buf)
{
	if (fwbuf_error(buf))
		return -1;

#if _WIN32
	LARGE_INTEGER FileSize;
	if (!GetFileSizeEx(buf->hFile, &FileSize)) {
		buf->dwErrCode = GetLastError();
		return -1;
	}
	return FileSize.QuadPart;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
#ifdef __linux__
	struct stat64 stat;
	if (fstat64(buf->fd, &stat) == -1) {
#else
	struct stat stat;
	if (fstat(buf->fd, &stat) == -1) {
#endif
		buf->errsv = errno;
		return -1;
	}
	return stat.st_size;
#else
	return buf->size;
#endif
}

int
fwbuf_set_size(fwbuf_t *buf, intmax_t size)
{
	if (fwbuf_unmap(buf) == -1)
		return -1;

#if _WIN32
	intmax_t pos = fwbuf_get_pos(buf);
	if (pos == -1)
		return -1;

	if (fwbuf_set_pos(buf, size) == -1)
		return -1;

	if (!SetEndOfFile(buf->hFile)) {
		buf->dwErrCode = GetLastError();
		return -1;
	}

	if (fwbuf_set_pos(buf, pos) == -1)
		return -1;

	return 0;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
#ifdef __linux__
	if (ftruncate64(buf->fd, size) == -1) {
#else
	// TODO: Check if size does not overflow the range of off_t.
	if (ftruncate(buf->fd, size) == -1) {
#endif
		buf->errsv = errno;
		return -1;
	}
	return 0;
#else
	if (size < buf->last) {
		set_errnum(buf->errnum = ERRNUM_INVAL);
		return -1;
	}

	buf->size = size;

	return 0;
#endif
}

intmax_t
fwbuf_get_pos(fwbuf_t *buf)
{
	if (fwbuf_error(buf))
		return -1;

#if _WIN32
	LARGE_INTEGER li = { .QuadPart = 0 };
	if (!SetFilePointerEx(buf->hFile, li, &li, FILE_CURRENT)) {
		buf->dwErrCode = GetLastError();
		return -1;
	}
	return li.QuadPart;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
#ifdef __linux__
	intmax_t pos = lseek64(buf->fd, 0, SEEK_CUR);
#else
	intmax_t pos = lseek(buf->fd, 0, SEEK_CUR);
#endif
	if (pos == -1)
		buf->errsv = errno;
	return pos;
#else
	long pos = ftell(buf->stream);
	if (pos == -1) {
		buf->errnum = errno2num(errno);
		set_errc(errno2c(errno));
	}
	return pos;
#endif
}

intmax_t
fwbuf_set_pos(fwbuf_t *buf, intmax_t pos)
{
	if (fwbuf_error(buf))
		return -1;

#if _WIN32
	LARGE_INTEGER li = { .QuadPart = pos };
	if (!SetFilePointerEx(buf->hFile, li, &li, FILE_BEGIN)) {
		buf->dwErrCode = GetLastError();
		return -1;
	}
	return li.QuadPart;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
#ifdef __linux__
	pos = lseek64(buf->fd, pos, SEEK_SET);
#else
	pos = lseek(buf->fd, pos, SEEK_SET);
#endif
	if (pos == -1)
		buf->errsv = errno;
	return pos;
#else
	if (pos < 0) {
		set_errnum(buf->errnum = ERRNUM_INVAL);
		return -1;
	}
	if (pos > LONG_MAX) {
		set_errnum(buf->errnum = ERRNUM_OVERFLOW);
		return -1;
	}

	if (fseek(buf->stream, pos, SEEK_SET)) {
		buf->errnum = errno2num(errno);
		set_errc(errno2c(errno));
		return -1;
	}

	return fwbuf_get_pos(buf);
#endif
}

ssize_t
fwbuf_write(fwbuf_t *buf, const void *ptr, size_t size)
{
	assert(ptr || !size);

	if (fwbuf_error(buf))
		return -1;

	if (!size)
		return 0;

#if _WIN32
	DWORD nNumberOfBytesWritten;
	if (!WriteFile(buf->hFile, ptr, size, &nNumberOfBytesWritten, NULL)) {
		buf->dwErrCode = GetLastError();
		return -1;
	}
	return nNumberOfBytesWritten;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
	ssize_t result;
	do
		result = write(buf->fd, ptr, size);
	while (result == -1 && errno == EINTR);
	if (result == -1)
		buf->errsv = errno;
	return result;
#else
	intmax_t pos = fwbuf_get_pos(buf);
	if (pos < 0)
		return -1;

	size_t result = fwrite(ptr, 1, size, buf->stream);
	if (result != size && ferror(buf->stream)) {
		buf->errnum = errno2num(errno);
		set_errc(errno2c(errno));
		if (!result)
			return -1;
	}

	// Update the memory map, if necessary.
	if (buf->map && pos < buf->pos + (intmax_t)buf->len
			&& pos + (intmax_t)size > buf->pos) {
		size_t begin = MAX(pos - buf->pos, 0);
		size_t end = MIN(pos + size - buf->pos, buf->len);
		memmove((char *)buf->map + begin, ptr, end - begin);
	}

	buf->last = MAX(buf->last, pos + (intmax_t)result);
	buf->size = MAX(buf->size, buf->last);

	return result;
#endif
}

ssize_t
fwbuf_pwrite(fwbuf_t *buf, const void *ptr, size_t size, intmax_t pos)
{
	assert(ptr || !size);

	if (fwbuf_error(buf))
		return -1;

	if (!size)
		return 0;

#if _WIN32
	ssize_t result = 0;

	if (pos < 0) {
		result = -1;
		buf->dwErrCode = ERROR_INVALID_PARAMETER;
		goto error_pos;
	}

	intmax_t oldpos = fwbuf_get_pos(buf);
	if (oldpos == -1) {
		result = -1;
		goto error_get_pos;
	}

	DWORD nNumberOfBytesWritten;
	OVERLAPPED Overlapped = { 0 };
	ULARGE_INTEGER uli = { .QuadPart = pos };
	Overlapped.Offset = uli.LowPart;
	Overlapped.OffsetHigh = uli.HighPart;
	// clang-format off
	if (!WriteFile(buf->hFile, ptr, size, &nNumberOfBytesWritten,
			&Overlapped)) {
		// clang-format on
		result = -1;
		buf->dwErrCode = GetLastError();
		goto error_WriteFile;
	}

	result = nNumberOfBytesWritten;

error_WriteFile:
	fwbuf_set_pos(buf, oldpos);
error_get_pos:
error_pos:
	SetLastError(buf->dwErrCode);
	return result;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
	ssize_t result;
#ifdef __linux__
	do
		result = pwrite64(buf->fd, ptr, size, pos);
#else
	do
		result = pwrite(buf->fd, ptr, size, pos);
#endif
	while (result == -1 && errno == EINTR);
	if (result == -1)
		buf->errsv = errno;
	return result;
#else
	ssize_t result = 0;
	int errc = get_errc();

	if (pos < 0) {
		result = -1;
		errc = errnum2c(buf->errnum = ERRNUM_INVAL);
		goto error_pos;
	}

	intmax_t oldpos = fwbuf_get_pos(buf);
	if (oldpos == -1) {
		result = -1;
		errc = get_errc();
		goto error_get_pos;
	}

	// Move to the requested position and fill any gap with zeros.
	if (buf->last < pos) {
		if (fwbuf_set_pos(buf, buf->last) != buf->last) {
			errc = get_errc();
			goto error_set_pos;
		}
		while (buf->last < pos) {
			if (fputc(0, buf->stream) == EOF) {
				buf->errnum = errno2num(errno);
				errc = errno2c(errno);
				goto error_fputc;
			}
			buf->last++;
			buf->size = MAX(buf->size, buf->last);
		}
	} else {
		if (fwbuf_set_pos(buf, pos) != pos) {
			errc = get_errc();
			goto error_set_pos;
		}
	}

	result = fwbuf_write(buf, ptr, size);
	if (result == -1 || (size_t)result != size)
		errc = get_errc();

error_fputc:
error_set_pos:
	if (fwbuf_set_pos(buf, oldpos) == -1 && !errc)
		errc = get_errc();
error_get_pos:
error_pos:
	set_errc(errc);
	return result;
#endif
}

void *
fwbuf_map(fwbuf_t *buf, intmax_t pos, size_t *psize)
{
	if (fwbuf_unmap(buf) == -1)
		return NULL;

	intmax_t size = fwbuf_get_size(buf);
	if (size < 0)
		return NULL;
	if (pos < 0) {
#if _WIN32
		SetLastError(buf->dwErrCode = ERROR_INVALID_PARAMETER);
#elif _POSIX_MAPPED_FILES >= 200112L
		errno = buf->errsv = EINVAL;
#else
		set_errnum(buf->errnum = ERRNUM_INVAL);
#endif
		return NULL;
	}
	if (pos > (intmax_t)size) {
#if _WIN32
		SetLastError(buf->dwErrCode = ERROR_INVALID_PARAMETER);
#elif _POSIX_MAPPED_FILES >= 200112L
		errno = buf->errsv = EOVERFLOW;
#else
		set_errnum(buf->errnum = ERRNUM_OVERFLOW);
#endif
		return NULL;
	}
	size -= pos;

	if (psize && *psize)
		size = MIN((uintmax_t)size, *psize);

#if _WIN32
	SYSTEM_INFO SystemInfo;
	// cppcheck-suppress uninitvar
	GetSystemInfo(&SystemInfo);
	DWORD off = pos % SystemInfo.dwAllocationGranularity;
	if ((uintmax_t)size > (uintmax_t)(SIZE_MAX - off)) {
		buf->dwErrCode = ERROR_INVALID_PARAMETER;
		goto error_size;
	}

	ULARGE_INTEGER MaximumSize = { .QuadPart = pos + size };
	buf->hFileMappingObject = CreateFileMapping(buf->hFile, NULL,
			PAGE_READWRITE, MaximumSize.HighPart,
			MaximumSize.LowPart, NULL);
	if (buf->hFileMappingObject == INVALID_HANDLE_VALUE) {
		buf->dwErrCode = GetLastError();
		goto error_CreateFileMapping;
	}

	ULARGE_INTEGER FileOffset = { .QuadPart = pos - off };
	buf->lpBaseAddress = MapViewOfFile(buf->hFileMappingObject,
			FILE_MAP_WRITE, FileOffset.HighPart, FileOffset.LowPart,
			(SIZE_T)(off + size));
	if (!buf->lpBaseAddress) {
		buf->dwErrCode = GetLastError();
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
	SetLastError(buf->dwErrCode);
	return NULL;
#elif _POSIX_MAPPED_FILES >= 200112L
	long page_size = sysconf(_SC_PAGE_SIZE);
	if (page_size <= 0) {
		buf->errsv = errno;
		return NULL;
	}
	intmax_t off = pos % page_size;
	if ((uintmax_t)size > (uintmax_t)(SIZE_MAX - off)) {
		errno = buf->errsv = EOVERFLOW;
		return NULL;
	}

#ifdef __linux__
	buf->addr = mmap64(NULL, off + size, PROT_READ | PROT_WRITE, MAP_SHARED,
			buf->fd, pos - off);
#else
	// TODO: Check if `pos - off` does not overflow the range of off_t.
	buf->addr = mmap(NULL, off + size, PROT_READ | PROT_WRITE, MAP_SHARED,
			buf->fd, pos - off);
#endif
	if (buf->addr == MAP_FAILED) {
		buf->errsv = errno;
		return NULL;
	}
	buf->len = off + size;

	if (psize)
		*psize = size;

	return (char *)buf->addr + off;
#else
	int errc = 0;

	if ((uintmax_t)size > SIZE_MAX) {
		set_errnum(buf->errnum = ERRNUM_OVERFLOW);
		return NULL;
	}

	buf->map = calloc(size, 1);
	if (!buf->map) {
		buf->errnum = errno2num(errno);
		errc = errno2c(errno);
		goto error_malloc_map;
	}

	// Copy bytes that have been written to the file to the mapped memory
	// region.
	intmax_t oldpos = 0;
	if (pos < buf->last) {
		oldpos = fwbuf_get_pos(buf);
		if (oldpos == -1) {
			errc = get_errc();
			goto error_get_pos;
		}

		if (fwbuf_set_pos(buf, pos) != pos) {
			errc = get_errc();
			goto error_set_pos;
		}

		size_t nitems = MIN(size, buf->last - pos);
		if (fread(buf->map, 1, nitems, buf->stream) != nitems
				&& ferror(buf->stream)) {
			buf->errnum = errno2num(errno);
			errc = errno2c(errno);
			goto error_fread;
		}

		if (fwbuf_set_pos(buf, oldpos) == oldpos) {
			errc = get_errc();
			goto error_set_pos;
		}
	}

	buf->pos = pos;
	buf->len = size;

	if (psize)
		*psize = size;

	return buf->map;

error_fread:
error_set_pos:
	if (fwbuf_set_pos(buf, oldpos) == -1 && !errc)
		errc = get_errc();
error_get_pos:
	free(buf->map);
	buf->map = NULL;
error_malloc_map:
	set_errc(errc);
	return NULL;
#endif
}

int
fwbuf_unmap(fwbuf_t *buf)
{
	assert(buf);

	int result = 0;
#if _WIN32
	DWORD dwErrCode = 0;
	if (buf->dwErrCode) {
		result = -1;
		dwErrCode = buf->dwErrCode;
	}

	if (buf->hFileMappingObject != INVALID_HANDLE_VALUE) {
		// clang-format off
		if (!FlushViewOfFile(buf->lpBaseAddress,
				buf->dwNumberOfBytesToMap) && !result) {
			// clang-format on
			result = -1;
			dwErrCode = GetLastError();
		}
		if (!UnmapViewOfFile(buf->lpBaseAddress) && !result) {
			result = -1;
			dwErrCode = GetLastError();
		}
		if (!CloseHandle(buf->hFileMappingObject) && !result) {
			result = -1;
			dwErrCode = GetLastError();
		}

		buf->hFileMappingObject = INVALID_HANDLE_VALUE;
		buf->lpBaseAddress = NULL;
		buf->dwNumberOfBytesToMap = 0;
	}

	if (dwErrCode) {
		if (!buf->dwErrCode)
			buf->dwErrCode = dwErrCode;
		SetLastError(dwErrCode);
	}
#elif _POSIX_MAPPED_FILES >= 200112L
	int errsv = 0;
	if (buf->errsv) {
		result = -1;
		errsv = buf->errsv;
	}

	if (buf->addr != MAP_FAILED) {
		if (msync(buf->addr, buf->len, MS_SYNC) == -1 && !result) {
			result = -1;
			errsv = errno;
		}
		if (munmap(buf->addr, buf->len) == -1 && !result) {
			result = -1;
			errsv = errno;
		}

		buf->addr = MAP_FAILED;
		buf->len = 0;
	}

	if (errsv) {
		if (!buf->errsv)
			buf->errsv = errsv;
		errno = errsv;
	}
#else
	int errc = 0;
	if (buf->errnum) {
		result = -1;
		errc = errnum2c(buf->errnum);
	}

	if (buf->map) {
		// Write the memory map to the file. We set the map pointer to
		// NULL before writing to prevent an unnecessary update of the
		// memory map by fwbuf_write().
		void *map = buf->map;
		buf->map = NULL;
		if (fwbuf_pwrite(buf, map, buf->len, buf->pos)
						!= (ssize_t)buf->len
				&& !result) {
			result = -1;
			errc = get_errc();
		}
		free(map);

		buf->pos = 0;
		buf->len = 0;
	}

	if (errc) {
		if (!buf->errnum)
			buf->errnum = errc2num(errc);
		set_errc(errc);
	}
#endif

	return result;
}

void
fwbuf_clearerr(fwbuf_t *buf)
{
	assert(buf);

#if _WIN32
	buf->dwErrCode = 0;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
	buf->errsv = 0;
#else
	buf->errnum = 0;
#endif
}

int
fwbuf_error(fwbuf_t *buf)
{
	assert(buf);

#if _WIN32
	if (buf->dwErrCode)
		SetLastError(buf->dwErrCode);
	return !!buf->dwErrCode;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
	if (buf->errsv)
		errno = buf->errsv;
	return !!buf->errsv;
#else
	if (buf->errnum)
		set_errnum(buf->errnum);
	return !!buf->errnum;
#endif
}

void
fwbuf_cancel(fwbuf_t *buf)
{
	assert(buf);

#if _WIN32
	if (!buf->dwErrCode)
		buf->dwErrCode = ERROR_OPERATION_ABORTED;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
	if (!buf->errsv)
		buf->errsv = ECANCELED;
#else
	if (!buf->errnum)
		buf->errnum = ERRNUM_CANCELED;
#endif
}

int
fwbuf_commit(fwbuf_t *buf)
{
	fwbuf_unmap(buf);

	int result = fwbuf_error(buf) ? -1 : 0;
#if _WIN32
	DWORD dwErrCode = GetLastError();

	if (buf->hFile == INVALID_HANDLE_VALUE)
		goto done;

	// Only invoke FlushFileBuffers() if no error occurred.
	if (!result && !FlushFileBuffers(buf->hFile)) {
		result = -1;
		dwErrCode = GetLastError();
	}

	if (!CloseHandle(buf->hFile) && !result) {
		result = -1;
		dwErrCode = GetLastError();
	}
	buf->hFile = INVALID_HANDLE_VALUE;

	// clang-format off
	if (result || !MoveFileExA(buf->tmpname, buf->filename,
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
		// clang-format on
		if (!result) {
			result = -1;
			dwErrCode = GetLastError();
		}
		DeleteFileA(buf->tmpname);
	}

done:
	SetLastError(buf->dwErrCode = dwErrCode);
	return result;
#elif _POSIX_C_SOURCE >= 200112L && !defined(__NEWLIB__)
	int errsv = errno;

	if (buf->fd == -1)
		goto done;

	// Only invoke fsync() if no error occurred.
	if (!result && fsync(buf->fd) == -1) {
		result = -1;
		errsv = errno;
	}

	if (close(buf->fd) == -1 && !result) {
		result = -1;
		errsv = errno;
	}
	buf->fd = -1;

	if (result || rename(buf->tmpname, buf->filename) == -1) {
		if (!result) {
			result = -1;
			errsv = errno;
		}
		remove(buf->tmpname);
	}

	if (!result && fsync(buf->dirfd) == -1) {
		result = -1;
		errsv = errno;
	}

	if (close(buf->dirfd) == -1 && !result) {
		result = -1;
		errsv = errno;
	}
	buf->dirfd = -1;

done:
	errno = buf->errsv = errsv;
	return result;
#else
	int errc = get_errc();

	if (!buf->stream)
		goto done;

	// Add zeros to the end of the file to reach the requested size.
	if (!result && buf->last < buf->size) {
		if (fwbuf_set_pos(buf, buf->last) != buf->last) {
			result = -1;
			errc = errno2c(errno);
		}
		while (!result && buf->last < buf->size) {
			if (fputc(0, buf->stream) == EOF) {
				result = -1;
				buf->errnum = errno2num(errno);
				errc = errno2c(errno);
			}
			buf->last++;
		}
	}

	// Only invoke fflush() if no error occurred.
	if (!result && fflush(buf->stream) == EOF) {
		result = -1;
		buf->errnum = errno2num(errno);
		errc = errno2c(errno);
	}

	if (fclose(buf->stream) == EOF && !result) {
		result = -1;
		buf->errnum = errno2num(errno);
		errc = errno2c(errno);
	}
	buf->stream = NULL;

	// WARNING: rename() may fail if the file already exists. Unfortunately,
	// if we remove the old file, we cannot guarantee that we won't lose
	// data.
	if (result || rename(buf->tmpname, buf->filename)) {
		if (!result) {
			result = -1;
			buf->errnum = errno2num(errno);
			errc = errno2c(errno);
		}
		remove(buf->tmpname);
	}

done:
	set_errc(errc);
	return result;
#endif
}

#endif // !LELY_NO_STDIO
