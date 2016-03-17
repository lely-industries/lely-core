/*!\file
 * This file is part of the utilities library; it contains the implementation of
 * the file buffer.
 *
 * \see lely/util/fbuf.h
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

#include "util.h"
#include <lely/util/errnum.h>
#include <lely/util/fbuf.h>

#include <assert.h>
#include <stdlib.h>

#ifdef _WIN32
#elif _POSIX_C_SOURCE >= 200112L && defined(_POSIX_MAPPED_FILES)
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#else
#include <stdio.h>
#endif

//! A file buffer struct.
struct __fbuf {
	//! The size (in bytes) of the buffer.
	size_t size;
	//! A pointer to the first byte in the buffer.
	void *ptr;
};

LELY_UTIL_EXPORT void *
__fbuf_alloc(void)
{
	return malloc(sizeof(struct __fbuf));
}

LELY_UTIL_EXPORT void
__fbuf_free(void *ptr)
{
	free(ptr);
}

LELY_UTIL_EXPORT struct __fbuf *
__fbuf_init(struct __fbuf *buf, const char *filename)
{
	assert(buf);
	assert(filename);

#ifdef _WIN32
	DWORD iError = 0;

	HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ,
			NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_READONLY | FILE_FLAG_NO_BUFFERING, NULL);
	if (__unlikely(hFile == INVALID_HANDLE_VALUE)) {
		iError = GetLastError();
		goto error_CreateFile;
	}

	ULARGE_INTEGER FileSize;
	FileSize.LowPart = GetFileSize(hFile, &FileSize.HighPart);
	if (FileSize.LowPart == INVALID_FILE_SIZE) {
		iError = GetLastError();
		if (__unlikely(iError))
			goto error_GetFileSize;
	}
	buf->size = FileSize.QuadPart;

	HANDLE hFileMappingObject = CreateFileMapping(hFile, NULL,
			PAGE_READONLY, 0, 0, NULL);
	if (__unlikely(!hFileMappingObject)) {
		iError = GetLastError();
		goto error_CreateFileMapping;
	}

	buf->ptr = MapViewOfFile(hFileMappingObject, FILE_MAP_READ, 0, 0,
			buf->size);
	if (__unlikely(!buf->ptr)) {
		iError = GetLastError();
		goto error_MapViewOfFile;
	}

	CloseHandle(hFileMappingObject);
	CloseHandle(hFile);

	return buf;

error_MapViewOfFile:
	CloseHandle(hFileMappingObject);
error_CreateFileMapping:
error_GetFileSize:
	CloseHandle(hFile);
error_CreateFile:
	SetLastError(iError);
	return NULL;
#elif _POSIX_C_SOURCE >= 200112L && defined(_POSIX_MAPPED_FILES)
	int errsv = 0;

	int fd = open(filename, O_RDONLY);
	if (__unlikely(fd == -1)) {
		errsv = errno;
		goto error_open;
	}

	struct stat stat;
	if (__unlikely(fstat(fd, &stat) == -1)) {
		errsv = errno;
		goto error_fstat;
	}

	buf->size = stat.st_size;
	buf->ptr = mmap(NULL, buf->size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (__unlikely(buf->ptr == MAP_FAILED)) {
		errsv = errno;
		goto error_mmap;
	}

	close(fd);

	return buf;

error_mmap:
error_fstat:
	close(fd);
error_open:
	errno = errsv;
	return NULL;
#else
	int errsv = 0;

	FILE *stream = fopen(filename, "rb");
	if (__unlikely(!stream)) {
		errsv = errno;
		goto error_fopen;
	}

	if (__unlikely(fseek(stream, 0, SEEK_END) == -1)) {
		errsv = errno;
		goto error_fseek;
	}
	long offset = ftell(stream);
	if (__unlikely(offset == -1)) {
		errsv = errno;
		goto error_ftell;
	}
	rewind(stream);

	buf->size = offset;
	buf->ptr = malloc(buf->size);
	if (__unlikely(!buf->ptr)) {
		errsv = errno;
		goto error_alloc_ptr;
	}

	if (__unlikely(fread(buf->ptr, 1, buf->size, stream) != buf->size)) {
		errsv = errno;
		goto error_fread;
	}

	fclose(stream);

	return buf;

error_fread:
	free(buf->ptr);
error_alloc_ptr:
error_ftell:
error_fseek:
	fclose(stream);
error_fopen:
	errno = errsv;
	return NULL;
#endif
}

LELY_UTIL_EXPORT void
__fbuf_fini(struct __fbuf *buf)
{
	assert(buf);

#ifdef _WIN32
	UnmapViewOfFile(buf->ptr);
#elif _POSIX_C_SOURCE >= 200112L && defined(_POSIX_MAPPED_FILES)
	munmap(buf->ptr, buf->size);
#else
	free(buf->ptr);
#endif
}

LELY_UTIL_EXPORT fbuf_t *
fbuf_create(const char *filename)
{
	errc_t errc = 0;

	fbuf_t *buf = __fbuf_alloc();
	if (__unlikely(!buf)) {
		errc = get_errc();
		goto error_alloc_buf;
	}

	if (__unlikely(!__fbuf_init(buf, filename))) {
		errc = get_errc();
		goto error_init_buf;
	}

	return buf;

error_init_buf:
	__fbuf_free(buf);
error_alloc_buf:
	set_errc(errc);
	return NULL;
}

LELY_UTIL_EXPORT void
fbuf_destroy(fbuf_t *buf)
{
	if (buf) {
		__fbuf_fini(buf);
		__fbuf_free(buf);
	}
}

LELY_UTIL_EXPORT void *
fbuf_begin(const fbuf_t *buf)
{
	assert(buf);

	return buf->ptr;
}

LELY_UTIL_EXPORT size_t
fbuf_size(const fbuf_t *buf)
{
	assert(buf);

	return buf->size;
}

