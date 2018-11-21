/**@file
 * This header file is part of the I/O library; it contains the Controller Area
 * Network (CAN) declarations.
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

#ifndef LELY_IO_CAN_H_
#define LELY_IO_CAN_H_

#include <lely/io/io.h>

// The CAN or CAN FD format frame struct from <lely/can/msg.h>.
struct can_msg;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opens a CAN device.
 *
 * @param path the (platform dependent) path (or interface) name of the device.
 *
 * @returns a new I/O device handle, or #IO_HANDLE_ERROR on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
io_handle_t io_open_can(const char *path);

/**
 * Reads a single CAN or CAN FD frame.
 *
 * @param handle a valid CAN device handle.
 * @param msg    the address at which to store the received frame.
 *
 * @returns the number of frames received (at most 1), or -1 on error. In the
 * latter case, the error number can be obtained with get_errc(). In case of
 * an I/O error (`ERRNUM_IO`), the CAN device state and error can be obtained
 * with io_can_get_state() and io_can_get_error(), respectively.
 */
int io_can_read(io_handle_t handle, struct can_msg *msg);

/**
 * Writes a single CAN or CAN FD frame.
 *
 * @param handle a valid CAN device handle.
 * @param msg    a pointer to the frame to be written.
 *
 * @returns the number of frames sent (at most 1), or -1 on error. In the latter
 * case, the error number can be obtained with get_errc().
 */
int io_can_write(io_handle_t handle, const struct can_msg *msg);

/**
 * Starts transmission and reception on a CAN device. On Linux, this operation
 * requires the process to have the CAP_NET_ADMIN capability.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_can_stop()
 */
int io_can_start(io_handle_t handle);

/**
 * Stops transmission and reception on a CAN device. On Linux, this operation
 * requires the process to have the CAP_NET_ADMIN capability.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_can_start()
 */
int io_can_stop(io_handle_t handle);

/**
 * Obtains the state of a CAN device.
 *
 * @returns `CAN_STATE_ACTIVE`, `CAN_STATE_PASSIVE`, `CAN_STATE_BUSOFF`, or -1
 * on error. In the latter case, the error number can be obtained with
 * get_errc().
 */
int io_can_get_state(io_handle_t handle);

/**
 * Obtains and clears the current error number of a CAN device, and stores the
 * value (any combination of `CAN_ERROR_BIT`, `CAN_ERROR_STUFF`,
 * `CAN_ERROR_CRC`, `CAN_ERROR_FORM` and `CAN_ERROR_ACK`) in *<b>perror</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_can_get_error(io_handle_t handle, int *perror);

/**
 * Obtains the transmit and/or receive error count of a CAN device and stores
 * the value in *<b>ptxec</b> and/or *<b>prxec</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
int io_can_get_ec(io_handle_t handle, uint16_t *ptxec, uint16_t *prxec);

/**
 * Obtains the bitrate (in bit/s) of a CAN device and stores the value in
 * *<b>pbitrate</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_can_set_bitrate()
 */
int io_can_get_bitrate(io_handle_t handle, uint32_t *pbitrate);

/**
 * Sets the bitrate (in bit/s) of a CAN device. Note that not all bitrates are
 * supported on every CAN controller. Standard bitrates are 10 kbit/s,
 * 20 kbit/s, 50 kbit/s, 125 kbit/s, 250 kbit/s, 500 kbit/s, 800 kbit/s and 1
 * Mbit/s. On Linux, this operation requires the process to have the
 * CAP_NET_ADMIN capability.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_can_get_bitrate()
 */
int io_can_set_bitrate(io_handle_t handle, uint32_t bitrate);

/**
 * Obtains the length of the transmission queue (in number of CAN frames) of a
 * CAN device and stores the value in *<b>ptxqlen</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_can_set_txqlen()
 */
int io_can_get_txqlen(io_handle_t handle, size_t *ptxqlen);

/**
 * Sets the length of the transmission queue (in number of CAN frames) of a CAN
 * device. On Linux, this operation requires the process to have the
 * CAP_NET_ADMIN capability.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_can_get_txqlen()
 */
int io_can_set_txqlen(io_handle_t handle, size_t txqlen);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO_CAN_H_
