/**@file
 * This header file is part of the I/O library; it contains the serial I/O
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

#ifndef LELY_IO_SERIAL_H_
#define LELY_IO_SERIAL_H_

#include <lely/io/io.h>

enum {
	/// Purge the receive buffer of a serial I/O device.
	IO_PURGE_RX = 1 << 0,
	/// Purge the transmit buffer of a serial I/O device.
	IO_PURGE_TX = 1 << 1
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opens a serial I/O device.
 *
 * @param path the (platform dependent) path name of the device.
 * @param attr the address at which to store the original device attributes (can
 *             be NULL).
 *
 * @returns a I/O device handle, or #IO_HANDLE_ERROR on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
io_handle_t io_open_serial(const char *path, io_attr_t *attr);

/**
 * Purges the receive and/or transmit buffers of a serial I/O device.
 *
 * @param handle a valid serial device handle.
 * @param flags  a flag specifying which of the buffers is to be discarded (any
 *               combination of #IO_PURGE_RX and #IO_PURGE_TX).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_serial_flush()
 */
int io_purge(io_handle_t handle, int flags);

/**
 * Retrieves the current attributes of a serial I/O device and stores them in
 * *<b>attr</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_serial_set_attr()
 */
int io_serial_get_attr(io_handle_t handle, io_attr_t *attr);

/**
 * Sets the attributes of a serial I/O device to those in *<b>attr</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_serial_get_attr()
 */
int io_serial_set_attr(io_handle_t handle, const io_attr_t *attr);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO_SERIAL_H_
