/**@file
 * This header file is part of the utilities library; it contains the (atomic)
 * write file buffer declarations.
 *
 * The write file buffer is atomic because all file operations are performed on
 * a temporary file. Only once all file operations have successfully completed
 * and fwbuf_commit() is invoked without error is the final file created (or
 * replaced). If any error occurs, the temporary file is discarded. This
 * guarantees that either the previous version or the complete new version of
 * the file is present on disk, even in the case of abnormal termination. Note
 * that this can only be guaranteed on Windows and POSIX platforms. On other
 * platforms we can only make a best effort.
 *
 * The file buffer allows mapping (part of) the contents of the file to memory.
 * This makes it possible to use pointer manipulation and functions like
 * `memcpy()` instead of writing to the file explicitly. The implementation uses
 * `CreateFileMapping()`/`MapViewOfFile()` or `mmap()` if available.
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

#ifndef LELY_UTIL_FWBUF_H_
#define LELY_UTIL_FWBUF_H_

#include <lely/libc/sys/types.h>
#include <lely/util/util.h>

#include <stddef.h>
#include <stdint.h>

struct __fwbuf;
#ifndef __cplusplus
/// An opaque (atomic) write file buffer type.
typedef struct __fwbuf fwbuf_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

void *__fwbuf_alloc(void);
void __fwbuf_free(void *ptr);
struct __fwbuf *__fwbuf_init(struct __fwbuf *buf, const char *filename);
void __fwbuf_fini(struct __fwbuf *buf);

/**
 * Creates a new (atomic) write file buffer.
 *
 * @param filename a pointer to the name of the file to be created or replaced
 *                 once all file operations have completed successfully.
 *
 * @returns a pointer to a new file buffer, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see fwbuf_destroy()
 */
fwbuf_t *fwbuf_create(const char *filename);

/**
 * Destroys a write file buffer. fwbuf_unmap() is invoked, if necessary, before
 * the file is closed. If fwbuf_commit() was invoked and did not return an
 * error, the file is created (or replaced), otherwise it is discarded.
 *
 * @see fwbuf_create()
 */
void fwbuf_destroy(fwbuf_t *buf);

/**
 * Returns the current size (in bytes) of the a write file buffer, or -1 on
 * error. In the latter case, the error number can be obtained with
 * get_errc().
 *
 * @see fwbuf_set_size()
 */
intmax_t fwbuf_get_size(fwbuf_t *buf);

/**
 * Sets the new size (in bytes) of the a write file buffer. This function
 * invokes fwbuf_unmap() before changing the size. Depending on the platform, it
 * may not be possible to discard bytes already written to the file, putting a
 * lower limit on the file size.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see fwbuf_get_size()
 */
int fwbuf_set_size(fwbuf_t *buf, intmax_t size);

/**
 * Returns the current offset (in bytes) of a write file buffer with respect to
 * the beginning of the file, or -1 on error. In the latter case, the error
 * number can be obtained with get_errc().
 *
 * The position indicates the location at which the next call to fwbuf_write()
 * will start writing, and it is only updated by that function or
 * fwbuf_set_pos().
 */
intmax_t fwbuf_get_pos(fwbuf_t *buf);

/**
 * Sets the current offset (in bytes) of a write file buffer with respect to the
 * beginning of the file. The new position may be larger than the current size
 * of the file. In that case the file is extended with zeros the moment data is
 * written to the new position.
 *
 * @returns the new position, or -1 on error. In the latter case, the error
 * number can be obtained with get_errc().
 *
 * @see fwbuf_get_pos()
 */
intmax_t fwbuf_set_pos(fwbuf_t *buf, intmax_t pos);

/**
 * Writes bytes to the current position in a write file buffer. Note that this
 * function updates the current position (and the memory map, if it exists) on
 * success.
 *
 * @param buf  a pointer to a write file buffer.
 * @param ptr  ta pointer to the bytes to write.
 * @param size the number of bytes to write.
 *
 * @returns the number of bytes written, or -1 on error. In the latter case, the
 * error number can be obtained with get_errc().
 *
 * @see fwbuf_pwrite()
 */
ssize_t fwbuf_write(fwbuf_t *buf, const void *ptr, size_t size);

/**
 * Writes bytes to the specified position in a write file buffer. This function
 * does not modify the current position. It does update the memory map, if
 * it exists (see fwbuf_map()).
 *
 * @param buf  a pointer to a write file buffer.
 * @param ptr  ta pointer to the bytes to write.
 * @param size the number of bytes to write.
 * @param pos  the offset (in bytes, with respect to the beginning of the file)
 *             at which to start writing.
 *
 * @returns the number of bytes written, or -1 on error. In the latter case, the
 * error number can be obtained with get_errc().
 *
 * @see fwbuf_write()
 */
ssize_t fwbuf_pwrite(fwbuf_t *buf, const void *ptr, size_t size, intmax_t pos);

/**
 * Maps (part of) the contents of a write file buffer to memory. Only a single
 * memory map can exists at a time. This function invokes fwbuf_unmap() before
 * creating the map.
 *
 * @param buf   a pointer to a write file buffer.
 * @param pos   the offset (in bytes, with respect to the beginning of the file)
 *              at which to the memory map.
 * @param psize the address of a value containing the desired size (in bytes) of
 *              the memory map. If <b>psize</b> or *<b>psize</b> is zero, all
 *              bytes from <b>pos</b> until the (current) end of the file are
 *              mapped. It is not possible to extend the map beyond the current
 *              end of the file. On success, *<b>psize</b> contains the size of
 *              the map.
 *
 * @returns a pointer to the first byte in the memory map, or NULL on error. In
 * the latter case, the error number can be obtained with get_errc().
 */
void *fwbuf_map(fwbuf_t *buf, intmax_t pos, size_t *psize);

/**
 * Unmaps the current memory map of a write file buffer, if it exists, and
 * writes the changes to disk.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see fwbuf_map()
 */
int fwbuf_unmap(fwbuf_t *buf);

/**
 * Clears the error indicator of a write file buffer, allowing fwbuf_commit() to
 * write the file to disk.
 *
 * @see fwbuf_error()
 */
void fwbuf_clearerr(fwbuf_t *buf);

/**
 * Returns 1 if the error indicator of a write file buffer is set, and 0 if not.
 * In the former case, the error number of the first error encountered during a
 * file operation can be obtained with get_errc().
 *
 * @see fwbuf_clearerr()
 */
int fwbuf_error(fwbuf_t *buf);

/**
 * Cancels any further file operations by setting the error indicator of a write
 * file buffer to #ERRNUM_CANCELED (if it was not already set).
 */
void fwbuf_cancel(fwbuf_t *buf);

/**
 * Commits all changes to a write file buffer to disk if all previous file
 * operations were successful, or discards them if not. This function does not
 * return until all changes are physically written to disk. Any further file
 * operations will fail.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * of the first error encountered during a file operation can be obtained with
 * get_errc().
 */
int fwbuf_commit(fwbuf_t *buf);

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_FWBUF_H_
