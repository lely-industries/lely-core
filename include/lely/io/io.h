/*!\file
 * This is the public header file of the I/O library.
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

#ifndef LELY_IO_IO_H
#define LELY_IO_IO_H

#include <lely/libc/stdint.h>
#include <lely/libc/sys/types.h>
#include <lely/util/util.h>

#ifndef LELY_IO_EXTERN
#ifdef DLL_EXPORT
#ifdef LELY_IO_INTERN
#define LELY_IO_EXTERN	extern __dllexport
#else
#define LELY_IO_EXTERN	extern __dllimport
#endif
#else
#define LELY_IO_EXTERN	extern
#endif
#endif

//! An opaque I/O device handle type.
typedef struct io_handle *io_handle_t;

//! The value of an invalid I/O device handle.
#define IO_HANDLE_ERROR	((io_handle_t)NULL)

//! A file offset type.
typedef int64_t io_off_t;

//! An opaque network address type.
typedef struct __io_addr io_addr_t;

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Initializes the I/O library and makes the I/O functions available for use.
 * On Windows this function invokes `WSAStartup()`. This function is not thread
 * safe, but can be invoked multiple times, as long as it is matched by an equal
 * number of calls to lely_io_fini().
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_IO_EXTERN int lely_io_init(void);

/*!
 * Finalizes the I/O library and terminates the availability of the I/O
 * functions. On Windows this function invokes `WSACleanup()`. Note that this
 * function MUST be invoked once for each corresponding call to lely_io_init().
 * Only the last invocation will finalize the library.
 */
LELY_IO_EXTERN void lely_io_fini(void);

/*!
 * Closes an I/O device.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_IO_EXTERN int io_close(io_handle_t handle);

/*!
 * Performs a read operation. For regular files, this function updates the file
 * pointer on success.
 *
 * \param handle a valid I/O device id.
 * \param buf    a pointer to the destination buffer.
 * \param nbytes the number of bytes to read.
 *
 * \returns the number of bytes read on success, or -1 on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 */
LELY_IO_EXTERN ssize_t io_read(io_handle_t handle, void *buf, size_t nbytes);

/*!
 * Performs a write operation. For regular files, this function updates the file
 * pointer on success.
 *
 * \param handle a valid I/O device id.
 * \param buf    a pointer to the source buffer.
 * \param nbytes the number of bytes to write.
 *
 * \returns the number of bytes written on success, or -1 on error. In the
 * latter case, the error number can be obtained with `get_errnum()`.
 */
LELY_IO_EXTERN ssize_t io_write(io_handle_t handle, const void *buf,
		size_t nbytes);

/*!
 * Flushes the write buffer of a an I/O device. This function waits until all
 * buffered data has been written.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with `get_errnum()`.
 */
LELY_IO_EXTERN int io_flush(io_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif

