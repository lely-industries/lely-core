/**@file
 * This header file is part of the CAN library; it contains the CAN frame
 * declarations.
 *
 * @copyright 2015-2020 Lely Industries N.V.
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

#ifndef LELY_CAN_MSG_H_
#define LELY_CAN_MSG_H_

#include <lely/features.h>

#include <stddef.h>
#include <stdint.h>

/// The mask used to extract the 11-bit Base Identifier from a CAN frame.
#define CAN_MASK_BID UINT32_C(0x000007ff)

/// The mask used to extract the 29-bit Extended Identifier from a CAN frame.
#define CAN_MASK_EID UINT32_C(0x1fffffff)

/// The flags of a CAN or CAN FD format frame.
enum {
	/**
	 * The Identifier Extension (IDE) flag. If this flag is set, the CAN
	 * Extended Format (with a 29-bit identifier) is used, otherwise the
	 * CAN Base Format (with an 11-bit identifier) is used.
	 */
	CAN_FLAG_IDE = 1u << 0,
	/**
	 * The Remote Transmission Request (RTR) flag (unavailable in CAN FD
	 * format frames). If this flag is set, the frame has no payload.
	 */
	CAN_FLAG_RTR = 1u << 1,
#if !LELY_NO_CANFD
	/**
	 * The FD Format (FDF) flag, formerly known as Extended Data Length
	 * (EDL). This flag is set for CAN FD format frames.
	 */
	CAN_FLAG_FDF = 1u << 2,
	CAN_FLAG_EDL = CAN_FLAG_FDF,
	/**
	 * The Bit Rate Switch (BRS) flag (only available in CAN FD format
	 * frames). If this flag is set, the bit rate is switched from the
	 * standard bit rate of the arbitration phase to the preconfigured
	 * alternate bit rate of the data phase.
	 */
	CAN_FLAG_BRS = 1u << 3,
	/**
	 * The Error State Indicator (ESI) flag (only available in CAN FD format
	 * frames).
	 */
	CAN_FLAG_ESI = 1u << 4
#endif // !LELY_NO_CANFD
};

/// The maximum number of bytes in the payload of a CAN format frame.
#define CAN_MAX_LEN 8

#if !LELY_NO_CANFD
/// The maximum number of bytes in the payload of a CAN FD format frame.
#define CANFD_MAX_LEN 64
#endif

/// The maximum number of bytes in the payload of a #can_msg struct.
#if LELY_NO_CANFD
#define CAN_MSG_MAX_LEN CAN_MAX_LEN
#else
#define CAN_MSG_MAX_LEN CANFD_MAX_LEN
#endif

/// A CAN or CAN FD format frame.
struct can_msg {
	/// The identifier (11 or 29 bits, depending on the #CAN_FLAG_IDE flag).
	uint_least32_t id;
	/**
	 * The flags (any combination of #CAN_FLAG_IDE, #CAN_FLAG_RTR,
	 * #CAN_FLAG_FDF, #CAN_FLAG_BRS and #CAN_FLAG_ESI).
	 */
	uint_least8_t flags;
	/**
	 * The number of bytes in #data (or the requested number of bytes in
	 * case of a remote frame). The maximum value is 8 for CAN format frames
	 * and 64 for CAN FD format frames.
	 */
	uint_least8_t len;
	/// The frame payload (in case of a data frame).
	uint_least8_t data[CAN_MSG_MAX_LEN];
};

/// The static initializer for #can_msg.
#if LELY_NO_CANFD
#define CAN_MSG_INIT \
	{ \
		0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } \
	}
#else
// clang-format off
#define CAN_MSG_INIT \
	{ \
		0, 0, 0, \
		{ \
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 \
		} \
	}
// clang-format on
#endif

/// The method used to compute te size (in bits) of a CAN frame.
enum can_msg_bits_mode {
	/// Simple calculation assuming no bit stuffing.
	CAN_MSG_BITS_MODE_NO_STUFF,
	/// Simple worst case estimate.
	CAN_MSG_BITS_MODE_WORST,
	/// Exact calculation based of frame content and CRC.
	CAN_MSG_BITS_MODE_EXACT
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Computes the size (in bits) of the specified CAN format frame on the CAN bus.
 *
 * @param msg  a pointer to a CAN frame format frame.
 * @param mode the method used to compute the size (one of
 *             #CAN_MSG_BITS_MODE_NO_STUFF, #CAN_MSG_BITS_MODE_WORST or
 *             #CAN_MSG_BITS_MODE_EXACT).
 *
 * @returns the number of bits on success, or -1 on error. In the latter case,
 * the error number can be obtained with get_errc().
 */
int can_msg_bits(const struct can_msg *msg, enum can_msg_bits_mode mode);

/**
 * Prints the contents of a CAN or CAN FD format frame to a string buffer. The
 * output mimics that of candump.
 *
 * @param s   the address of the output buffer. If <b>s</b> is not NULL, at most
 *            `n - 1` characters are written, plus a terminating null byte.
 * @param n   the size (in bytes) of the buffer at <b>s</b>. If <b>n</b> is
 *            zero, nothing is written.
 * @param msg a pointer to the CAN frame to be printed.
 *
 * @returns the number of characters that would have been written had the
 * buffer been sufficiently large, not counting the terminating null byte, or a
 * negative number on error. In the latter case, the error number is stored in
 * `errno`.
 */
int snprintf_can_msg(char *s, size_t n, const struct can_msg *msg);

#if !LELY_NO_MALLOC
/**
 * Equivalent to snprintf_can_msg(), except that it allocates a string large
 * enough to hold the output, including the terminating null byte.
 *
 * @param ps  the address of a value which, on success, contains a pointer to
 *            the allocated string. This pointer SHOULD be passed to `free()` to
 *            release the allocated storage.
 * @param msg a pointer to the CAN frame to be printed.
 *
 * @returns the number of characters written, not counting the terminating null
 * byte, or a negative number on error. In the latter case, the error number is
 * stored in `errno`.
 */
int asprintf_can_msg(char **ps, const struct can_msg *msg);
#endif // !LELY_NO_MALLOC

/**
 * Computes a bitwise CRC-15-CAN checksum, based on the 0x4599 generator
 * polynomial. The implementation uses a table with precomputed values for
 * efficiency.
 *
 * @param crc  the initial value.
 * @param ptr  a pointer to the bits to be hashed.
 * @param off  the offset (in bits) with respect to <b>ptr</b> of the first bit
 *             to be hashed.
 * @param bits the number of bits to hash.
 *
 * @returns the updated CRC.
 */
uint_least16_t can_crc(
		uint_least16_t crc, const void *ptr, int off, size_t bits);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CAN_MSG_H_
