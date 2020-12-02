/**@file
 * This is the public header file of the I/O library.
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

#ifndef LELY_IO_IO_H_
#define LELY_IO_IO_H_

#include <lely/libc/sys/types.h>
#include <lely/util/util.h>

#include <stddef.h>
#include <stdint.h>

/// An opaque I/O device handle type.
typedef struct io_handle *io_handle_t;

/// The value of an invalid I/O device handle.
#define IO_HANDLE_ERROR ((io_handle_t)NULL)

/// A file offset type.
typedef int64_t io_off_t;

/// An opaque serial I/O device attributes type.
typedef union __io_attr io_attr_t;

/// An opaque network address type.
typedef struct __io_addr io_addr_t;

enum {
	/// A CAN device.
	IO_TYPE_CAN = 1,
	/// A regular file.
	IO_TYPE_FILE,
	/// A pipe.
	IO_TYPE_PIPE,
	/// A serial I/O device.
	IO_TYPE_SERIAL,
	/// A network socket.
	IO_TYPE_SOCK
};

enum {
	/// Do not close the native file descriptor when closing an I/O device.
	IO_FLAG_NO_CLOSE = 1 << 0,
	/// Perform I/O operations in non-blocking mode.
	IO_FLAG_NONBLOCK = 1 << 1,
	/// Receive own messages (i.e., sent by the same device).
	IO_FLAG_LOOPBACK = 1 << 2
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the I/O library and makes the I/O functions available for use.
 * On Windows this function invokes `WSAStartup()`. This function is not thread
 * safe, but can be invoked multiple times, as long as it is matched by an equal
 * number of calls to lely_io_fini().
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int lely_io_init(void);

/**
 * Finalizes the I/O library and terminates the availability of the I/O
 * functions. On Windows this function invokes `WSACleanup()`. Note that this
 * function MUST be invoked once for each corresponding call to lely_io_init().
 * Only the last invocation will finalize the library.
 */
void lely_io_fini(void);

/**
 * Increments the reference count of an I/O device handle.
 *
 * @returns <b>handle</b>.
 *
 * @see io_handle_release()
 */
io_handle_t io_handle_acquire(io_handle_t handle);

/**
 * Decrements the reference count of an I/O device handle. If the count reaches
 * zero, the handle is destroyed.
 *
 * @see io_handle_acquire()
 */
void io_handle_release(io_handle_t handle);

/**
 * Returns 1 if there is only a single reference to the specified I/O device
 * handle, and 0 otherwise.
 */
int io_handle_unique(io_handle_t handle);

/**
 * Closes an I/O device.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_close(io_handle_t handle);

/**
 * Returns the type of an I/O device (one of #IO_TYPE_CAN, #IO_TYPE_FILE,
 * #IO_TYPE_PIPE, #IO_TYPE_SERIAL or #IO_TYPE_SOCK), or -1 on error. In the
 * latter case, the error number can be obtained with get_errc().
 */
int io_get_type(io_handle_t handle);

/// Returns the native file descriptor of an I/O device.
#if _WIN32
HANDLE io_get_fd(io_handle_t handle);
#else
int io_get_fd(io_handle_t handle);
#endif

/**
 * Obtains the flags of an I/O device.
 *
 * @returns the active flags (any combination of #IO_FLAG_NO_CLOSE,
 * #IO_FLAG_NONBLOCK and #IO_FLAG_LOOPBACK), or -1 on error. In the latter case,
 * the error number can be obtained with get_errc().
 *
 * @see io_set_flags()
 */
int io_get_flags(io_handle_t handle);

/**
 * Sets the flags of an I/O device.
 *
 * @param handle a valid I/O device handle.
 * @param flags  the I/O device flags (any combination of #IO_FLAG_NO_CLOSE,
 *               #IO_FLAG_NONBLOCK and #IO_FLAG_LOOPBACK).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_get_flags()
 */
int io_set_flags(io_handle_t handle, int flags);

/**
 * Performs a read operation. For regular files, this function updates the file
 * pointer on success.
 *
 * @param handle a valid I/O device handle.
 * @param buf    a pointer to the destination buffer.
 * @param nbytes the number of bytes to read.
 *
 * @returns the number of bytes read on success, or -1 on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
ssize_t io_read(io_handle_t handle, void *buf, size_t nbytes);

/**
 * Performs a write operation. For regular files, this function updates the file
 * pointer on success.
 *
 * @param handle a valid I/O device handle.
 * @param buf    a pointer to the source buffer.
 * @param nbytes the number of bytes to write.
 *
 * @returns the number of bytes written on success, or -1 on error. In the
 * latter case, the error number can be obtained with get_errc().
 */
ssize_t io_write(io_handle_t handle, const void *buf, size_t nbytes);

/**
 * Flushes the write buffer of a an I/O device. This function waits until all
 * buffered data has been written.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_flush(io_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO_IO_H_
