/**@file
 * This header file is part of the I/O library; it contains the regular file
 * declarations.
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

#ifndef LELY_IO_FILE_H_
#define LELY_IO_FILE_H_

#include <lely/io/io.h>

enum {
	/// Open a file for reading.
	IO_FILE_READ = 1 << 0,
	/// Open a file for writing
	IO_FILE_WRITE = 1 << 1,
	/// Append data to the end of the file.
	IO_FILE_APPEND = 1 << 2,
	/// Create a new file if it does not exists.
	IO_FILE_CREATE = 1 << 3,
	/**
	 * Fail if the file already exists (ignored unless #IO_FILE_CREATE is
	 * set).
	 */
	IO_FILE_NO_EXIST = 1 << 4,
	/// Truncate an existing file (ignored if #IO_FILE_NO_EXIST is set).
	IO_FILE_TRUNCATE = 1 << 5
};

enum {
	/// A seek operation with respect to the beginning of a file.
	IO_SEEK_BEGIN,
	/// A seek operation with respect to the current offset in a file.
	IO_SEEK_CURRENT,
	/// A seek operation with respect to the end of a file.
	IO_SEEK_END
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opens a regular file.
 *
 * @param path  the path name of the file.
 * @param flags any combination of #IO_FILE_READ, #IO_FILE_WRITE,
 *              #IO_FILE_APPEND, #IO_FILE_CREATE, #IO_FILE_NO_EXIST and
 *              #IO_FILE_TRUNCATE.
 *
 * @returns a I/O device handle, or #IO_HANDLE_ERROR on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
io_handle_t io_open_file(const char *path, int flags);

/**
 * Moves the current read/write offset of an open file.
 *
 * @param handle a valid file device handle.
 * @param offset the desired offset (in bytes) with respect to the beginning of
 *               the file (if `whence == IO_SEEK_BEGIN`), the current offset (if
 *               `whence == IO_SEEK_CURRENT`) or the end of the file (if
 *               `whence == IO_SEEK_END`).
 * @param whence one of #IO_SEEK_BEGIN, #IO_SEEK_CURRENT or #IO_SEEK_END.
 *
 * @returns the current offset with respect to the beginning of the file, or -1
 * on error. In the latter case, the error number can be obtained with
 * get_errc().
 */
io_off_t io_seek(io_handle_t handle, io_off_t offset, int whence);

/**
 * Performs a read operation at the specified offset, without updating the file
 * pointer.
 *
 * @param handle a valid file device handle.
 * @param buf    a pointer to the destination buffer.
 * @param nbytes the number of bytes to read.
 * @param offset the offset (in bytes) at which to start reading.
 *
 * @returns the number of bytes read on success, or -1 on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
ssize_t io_pread(io_handle_t handle, void *buf, size_t nbytes, io_off_t offset);

/**
 * Performs a write operation at the specified offset, without updating the file
 * pointer.
 *
 * @param handle a valid file device handle.
 * @param buf    a pointer to the source buffer.
 * @param nbytes the number of bytes to write.
 * @param offset the offset (in bytes) at which to start writing.
 *
 * @returns the number of bytes written on success, or -1 on error. In the
 * latter case, the error number can be obtained with get_errc().
 */
ssize_t io_pwrite(io_handle_t handle, const void *buf, size_t nbytes,
		io_off_t offset);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO_FILE_H_
