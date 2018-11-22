/**@file
 * This header file is part of the utilities library; it contains the read file
 * buffer declarations.
 *
 * The file buffer allows mapping (part of) the contents of the file to memory.
 * This makes it possible to use pointer manipulation and functions like
 * `memcpy()` instead of reading from the file explicitly. The implementation
 * uses `CreateFileMapping()`/`MapViewOfFile()` or `mmap()` if available.
 *
 * @copyright 2016-2018 Lely Industries N.V.
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

#ifndef LELY_UTIL_FRBUF_H_
#define LELY_UTIL_FRBUF_H_

#include <lely/libc/sys/types.h>
#include <lely/util/util.h>

#include <stddef.h>
#include <stdint.h>

struct __frbuf;
#ifndef __cplusplus
/// An opaque read file buffer type.
typedef struct __frbuf frbuf_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

void *__frbuf_alloc(void);
void __frbuf_free(void *ptr);
struct __frbuf *__frbuf_init(struct __frbuf *buf, const char *filename);
void __frbuf_fini(struct __frbuf *buf);

/**
 * Creates a new read file buffer.
 *
 * @param filename a pointer to the name of the file to be read into a buffer.
 *
 * @returns a pointer to a new file buffer, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see frbuf_destroy()
 */
frbuf_t *frbuf_create(const char *filename);

/**
 * Destroys a read file buffer. frbuf_unmap() is invoked, if necessary, before
 * the file is closed.
 *
 * @see frbuf_create()
 */
void frbuf_destroy(frbuf_t *buf);

/**
 * Returns the size (in bytes) of the a read file buffer, or -1 on error. In the
 * latter case, the error number can be obtained with get_errc().
 */
intmax_t frbuf_get_size(frbuf_t *buf);

/**
 * Returns the current offset (in bytes) of a read file buffer with respect to
 * the beginning of the file, or -1 on error. In the latter case, the error
 * number can be obtained with get_errc().
 *
 * The position indicates the location at which the next call to frbuf_read()
 * will starting reading, and it is only updated by that function or
 * frbuf_set_pos().
 */
intmax_t frbuf_get_pos(frbuf_t *buf);

/**
 * Sets the current offset (in bytes) of a read file buffer with respect to the
 * beginning of the file. The new position cannot be larger that the size of the
 * file.
 *
 * @returns the new position, or -1 on error. In the latter case, the error
 * number can be obtained with get_errc().
 *
 * @see frbuf_get_pos()
 */
intmax_t frbuf_set_pos(frbuf_t *buf, intmax_t pos);

/**
 * Reads bytes from the current position in a read file buffer. Note that this
 * function updates the current position on success.
 *
 * @param buf  a pointer to a read file buffer.
 * @param ptr  the address at which to store the bytes read.
 * @param size the number of bytes to read.
 *
 * @returns the number of bytes read, or -1 on error. In the latter case, the
 * error number can be obtained with get_errc().
 *
 * @see frbuf_pread()
 */
ssize_t frbuf_read(frbuf_t *buf, void *ptr, size_t size);

/**
 * Reads bytes from the specified position in a read file buffer. This function
 * does not modify the current position.
 *
 * @param buf  a pointer to a read file buffer.
 * @param ptr  the address at which to store the bytes read.
 * @param size the number of bytes to read.
 * @param pos  the offset (in bytes, with respect to the beginning of the file)
 *             at which to start reading.
 *
 * @returns the number of bytes read, or -1 on error. In the latter case, the
 * error number can be obtained with get_errc().
 *
 * @see frbuf_read()
 */
ssize_t frbuf_pread(frbuf_t *buf, void *ptr, size_t size, intmax_t pos);

/**
 * Maps (part of) the contents of a read file buffer to memory. Only a single
 * memory map can exists at a time. This function invokes frbuf_unmap() before
 * creating the map.
 *
 * @param buf   a pointer to a read file buffer.
 * @param pos   the offset (in bytes, with respect to the beginning of the file)
 *              at which to the memory map.
 * @param psize the address of a value containing the desired size (in bytes) of
 *              the memory map. If <b>psize</b> or *<b>psize</b> is zero, all
 *              bytes from <b>pos</b> until the end of the file are mapped. On
 *              success, *<b>psize</b> contains the size of the map.
 *
 * @returns a pointer to the first byte in the memory map, or NULL on error. In
 * the latter case, the error number can be obtained with get_errc(). Note that
 * it is an error to modify bytes in the memory map which may lead to a
 * segmentation fault.
 */
const void *frbuf_map(frbuf_t *buf, intmax_t pos, size_t *psize);

/**
 * Unmaps the current memory map of a read file buffer, if it exists.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see frbuf_map()
 */
int frbuf_unmap(frbuf_t *buf);

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_FRBUF_H_
