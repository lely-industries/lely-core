/**@file
 * This header file is part of the I/O library; it contains the serial I/O
 * attributes declarations.
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

#ifndef LELY_IO_ATTR_H_
#define LELY_IO_ATTR_H_

#include <lely/io/io.h>

/// An opaque serial I/O device attributes type.
#if _WIN32
union __io_attr {
	char __size[48];
	int __align;
};
#else
union __io_attr {
	char __size[60];
	int __align;
};
#endif

/// The static initializer for #io_attr_t.
#define IO_ATTR_INIT \
	{ \
		{ \
			0 \
		} \
	}

enum {
	/// No parity.
	IO_PARITY_NONE,
	/// Odd parity.
	IO_PARITY_ODD,
	/// Even parity.
	IO_PARITY_EVEN
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the baud rate from the attributes of a serial I/O device, or -1 on
 * error.
 *
 * @see io_attr_set_speed()
 */
int io_attr_get_speed(const io_attr_t *attr);

/**
 * Sets the baud rate of a serial I/O device. Note that the new device
 * attributes will not take effect until a call to io_serial_set_attr().
 *
 * @param attr  a pointer to the current device attributes.
 * @param speed the input and output baud rate. Note that not all possible
 *              values are supported.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_attr_get_speed()
 */
int io_attr_set_speed(io_attr_t *attr, int speed);

/**
 * Checks if flow control is enabled in the attributes of a serial I/O device.
 *
 * @returns 1 if flow control is enabled and 0 if not, or -1 on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see io_attr_set_flow_control()
 */
int io_attr_get_flow_control(const io_attr_t *attr);

/**
 * Disables flow control for a serial I/O device if <b>flow_control</b> is zero,
 * and enables it otherwise. Note that the new device attributes will not take
 * effect until a call to io_serial_set_attr().
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_attr_get_flow_control()
 */
int io_attr_set_flow_control(io_attr_t *attr, int flow_control);

/**
 * Obtains the parity scheme from the attributes of a serial I/O device.
 *
 * @returns #IO_PARITY_NONE, #IO_PARITY_ODD or #IO_PARITY_EVEN, or -1 on error.
 * In the latter case, the error number can be obtained with get_errc().
 *
 * @see io_attr_set_parity()
 */
int io_attr_get_parity(const io_attr_t *attr);

/**
 * Sets the parity scheme of a serial I/O device. Note that the new device
 * attributes will not take effect until a call to io_serial_set_attr().
 *
 * @param attr   a pointer to the current device attributes.
 * @param parity the parity scheme to be used (one of #IO_PARITY_NONE,
 *               #IO_PARITY_ODD or #IO_PARITY_EVEN).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_attr_get_parity()
 */
int io_attr_set_parity(io_attr_t *attr, int parity);

/**
 * Obtains the number of stop bits used from the attributes of a serial I/O
 * device.
 *
 * @returns 1 is two stop bits are used and 0 if one bit is used, or -1 on
 * error. In the latter case, the error number can be obtained with
 * get_errc().
 *
 * @see io_attr_set_stop_bits()
 */
int io_attr_get_stop_bits(const io_attr_t *attr);

/**
 * Sets the number of stop bits used in a serial I/O device to one if
 * <b>stop_bits</b> is zero, and two otherwise. Note that the new device
 * attributes will not take effect until a call to io_serial_set_attr().
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_attr_get_stop_bits()
 */
int io_attr_set_stop_bits(io_attr_t *attr, int stop_bits);

/**
 * Obtains the character size (in bits) from the attributes of a serial I/O
 * device.
 *
 * @returns the character size, or -1 on error. In the latter case, the error
 * number can be obtained with get_errc().
 *
 * @see io_attr_set_char_size()
 */
int io_attr_get_char_size(const io_attr_t *attr);

/**
 * Sets the character size (in bits) of a serial I/O device. Note that the new
 * device attributes will not take effect until a call to io_serial_set_attr().
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see io_attr_get_char_size()
 */
int io_attr_set_char_size(io_attr_t *attr, int char_size);

#ifdef __cplusplus
}
#endif

#endif // !LELY_IO_ATTR_H_
