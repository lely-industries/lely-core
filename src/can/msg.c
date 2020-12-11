/**@file
 * This file is part of the CAN library; it contains the implementation of the
 * CAN frame functions.
 *
 * @see lely/can/msg.h
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

#include "can.h"
#include <lely/can/msg.h>
#include <lely/util/bits.h>
#include <lely/util/errnum.h>
#include <lely/util/util.h>

#include <assert.h>
#if !LELY_NO_STDIO
#include <stdio.h>
// Include inttypes.h after stdio.h to enforce declarations of format specifiers
// in Newlib.
#include <inttypes.h>
#include <stdlib.h>
#endif

/// Computes a bitwise CRC-15-CAN checksum of a single byte. @see can_crc()
static uint_least16_t can_crc_bits(
		uint_least16_t crc, uint_least8_t byte, int off, int bits);

/// Computes a CRC-15-CAN checksum. @see can_crc()
static uint_least16_t can_crc_bytes(
		uint_least16_t crc, const unsigned char *bp, size_t n);

int
can_msg_bits(const struct can_msg *msg, enum can_msg_bits_mode mode)
{
	assert(msg);

#if !LELY_NO_CANFD
	if (msg->flags & CAN_FLAG_FDF) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}
#endif

	if (msg->len > CAN_MAX_LEN) {
		set_errnum(ERRNUM_INVAL);
		return -1;
	}

	switch (mode) {
	case CAN_MSG_BITS_MODE_NO_STUFF: {
		int bits = (msg->flags & CAN_FLAG_IDE) ? 67 : 47;
		if (!(msg->flags & CAN_FLAG_RTR))
			bits += msg->len * 8;
		return bits;
	}
	case CAN_MSG_BITS_MODE_WORST: {
		int bits = (msg->flags & CAN_FLAG_IDE) ? 80 : 55;
		if (!(msg->flags & CAN_FLAG_RTR))
			bits += msg->len * 10;
		return bits;
	}
	case CAN_MSG_BITS_MODE_EXACT: break;
	default: set_errnum(ERRNUM_INVAL); return -1;
	}

	uint_least8_t data[16] = { 0 };
	uint_least8_t *bp = data;
	int off = 0;
	int bits = 0;

	if (msg->flags & CAN_FLAG_IDE) {
		// s = SOF, B = (base) Identifier, S = SRR, I = IDE,
		// E = Identifier (extension), R = RTR = 1 = R1, 0 = R0,
		// DLC4 = DLC, 0-7 = Data, C = CRC
		// data[0-3]   |.sBBBBBB BBBBBSIE EEEEEEEE EEEEEEEE|
		// data[4-7]   |ER10DLC4 00000000 11111111 22222222|
		// data[8-11]  |33333333 44444444 55555555 66666666|
		// data[12-14] |77777777 CCCCCCCC CCCCCCC. ........|
		uint_least32_t id = msg->id & CAN_MASK_EID;
		off = 1;
		*bp++ = (id >> 23) & 0x3f; // SOF = 0, base (Indentifier)
		bits += 8 - off;

		*bp++ = ((id >> 15) & 0xf8) // base (Indentifier)
				| (0x03 << 1) // SRR, IDE
				| ((id >> 17) & 0x01); // Identifier (extension)
		bits += 8;

		*bp++ = (id >> 9) & 0xff; // Identifier (extension)
		bits += 8;

		*bp++ = (id >> 1) & 0xff; // Identifier (extension)
		bits += 8;

		*bp++ = ((id << 7) & 0x80) // Identifier (extension)
				| (!!(msg->flags & CAN_FLAG_RTR) << 6) // RTR
				| (msg->len & 0x0f); // R1 = 0, R0 = 0, DLC
		bits += 8;
	} else {
		// s = SOF, B = (base) Identifier, R = RTR, I = IDE, 0 = R0,
		// DLC4 = DLC, 0-7 = Data, C = CRC
		// data[0-3]   |.....sBB BBBBBBBB BRI0DLC4 00000000|
		// data[4-7]   |11111111 22222222 33333333 44444444|
		// data[8-11]  |55555555 66666666 77777777 CCCCCCCC|
		// data[12-14] |CCCCCCC. ........ ........ ........|
		uint_least32_t id = msg->id & CAN_MASK_BID;
		off = 5;
		*bp++ = (id >> 9) & 0x03; // SOF = 0, base (Indentifier)
		bits += 8 - off;

		*bp++ = (id >> 1) & 0xff; // base (Indentifier)
		bits += 8;

		*bp++ = ((id << 7) & 0x80) // base (Indentifier)
				| (!!(msg->flags & CAN_FLAG_RTR) << 6) // RTR
				| (msg->len & 0x0f); // IDE = 0, R0 = 0, DLC
		bits += 8;
	}

	if (!(msg->flags & CAN_FLAG_RTR) && msg->len) {
		for (uint_least8_t i = 0; i < msg->len; i++, bits += 8)
			*bp++ = msg->data[i] & 0xff;
	}

	uint_least16_t crc = can_crc(0, data, off, bits);
	assert(!((off + bits) % 8));
	*bp++ = (crc >> 7) & 0xff;
	*bp++ = (crc << 1) & 0xff;
	bits += 15;

	// Count the stuffed bits.
	int stuff = 0;
	uint_least8_t mask = 0x1f;
	uint_least8_t same = mask;
	for (int i = off; i < off + bits;) {
		// Alternate between looking for a series of zeros and ones.
		same = same ? 0 : mask;
		// Extract 5 bits at i at look for a bit flip.
		// clang-format off
		uint_least8_t five = (((uint_least16_t)data[i / 8] << 8)
				| data[i / 8 + 1]) >> (16 - 5 - i % 8);
		// clang-format on
		int n = clz8((five & mask) ^ same) - 3;
		i += n;
		if (n < 5) {
			// No bit stuffing needed. Check the next 5 bits.
			mask = 0x1f;
		} else {
			// Insert a stuffed bit and look for the next 4 bits
			// (the 5th bit is the stuffed one).
			if (mask == 0x1e)
				i--;
			else
				mask = 0x1e;
			if (i <= off + bits)
				stuff++;
		}
	}
	bits += stuff;

	bits += 3; // CRC delimiter, ACK slot, ACK delimiter
	bits += 7; // EOF sequence
	bits += 3; // intermission

	return bits;
}

#if !LELY_NO_STDIO

int
snprintf_can_msg(char *s, size_t n, const struct can_msg *msg)
{
	if (!s)
		n = 0;

	if (!msg)
		return 0;

	uint_least8_t len = msg->len;
#if !LELY_NO_CANFD
	if (msg->flags & CAN_FLAG_FDF)
		len = MIN(len, CANFD_MAX_LEN);
	else
#endif
		len = MIN(len, CAN_MAX_LEN);

	int r, t = 0;

	if (msg->flags & CAN_FLAG_IDE)
		// cppcheck-suppress nullPointer symbolName=s
		r = snprintf(s, n, "%08" PRIX32, msg->id & CAN_MASK_EID);
	else
		// cppcheck-suppress nullPointer symbolName=s
		r = snprintf(s, n, "%03" PRIX32, msg->id & CAN_MASK_BID);
	if (r < 0)
		return r;
	t += r;
	r = MIN((size_t)r, n);
	s += r;
	n -= r;

#if !LELY_NO_CANFD
	if (msg->flags & CAN_FLAG_FDF)
		r = snprintf(s, n, "  [%02d] ", len);
	else
#endif
		r = snprintf(s, n, "   [%d] ", len);
	if (r < 0)
		return r;
	t += r;
	r = MIN((size_t)r, n);
	s += r;
	n -= r;

	if (msg->flags & CAN_FLAG_RTR) {
		r = snprintf(s, n, " remote request");
		if (r < 0)
			return r;
		t += r;
	} else {
		for (uint_least8_t i = 0; i < len; i++) {
			int r = snprintf(s, n, " %02X", msg->data[i]);
			if (r < 0)
				return r;
			t += r;
			r = MIN((size_t)r, n);
			s += r;
			n -= r;
		}
	}

	return t;
}

#if !LELY_NO_MALLOC
int
asprintf_can_msg(char **ps, const struct can_msg *msg)
{
	int n = snprintf_can_msg(NULL, 0, msg);
	if (n < 0)
		return n;

	char *s = malloc(n + 1);
	if (!s)
		return -1;

	n = snprintf_can_msg(s, n + 1, msg);
	if (n < 0) {
		int errsv = errno;
		free(s);
		errno = errsv;
		return n;
	}

	*ps = s;
	return n;
}
#endif // !LELY_NO_MALLOC

#endif // !LELY_NO_STDIO

uint_least16_t
can_crc(uint_least16_t crc, const void *ptr, int off, size_t bits)
{
	assert(ptr || !bits);

	const uint_least8_t *bp = ptr;
	bp += off / 8;
	off %= 8;
	if (off < 0) {
		bp--;
		off += 8;
	}

	if (off && bits) {
		int n = MIN((size_t)(8 - off), bits);
		crc = can_crc_bits(crc, *bp++, off, n);
		bits -= n;
	}

	size_t n = bits / 8;
	crc = can_crc_bytes(crc, bp, n);
	bp += n;
	bits -= n * 8;

	if (bits)
		crc = can_crc_bits(crc, *bp, 0, bits);

	return crc;
}

static uint_least16_t
can_crc_bits(uint_least16_t crc, uint_least8_t byte, int off, int bits)
{
	assert(off >= 0);
	assert(bits >= 0);
	assert(off + bits <= 8);

	for (byte <<= off; bits--; byte <<= 1) {
		if ((byte ^ (crc >> 7)) & 0x80)
			crc = (crc << 1) ^ 0x4599;
		else
			crc <<= 1;
	}
	return crc & 0x7fff;
}

static uint_least16_t
can_crc_bytes(uint_least16_t crc, const unsigned char *bp, size_t n)
{
	assert(bp || !n);

	// This table contains precomputed CRC-15-CAN checksums for each of the
	// 256 bytes. The table was computed with the following code:
	/*
	uint_least16_t tab[256];
	for (int n = 0; n < 256; n++) {
		uint_least16_t crc = n << 7;
		for (int k = 0; k < 8; k++) {
			if (crc & 0x4000)
				crc = (crc << 1) ^ 0x4599;
			else
				crc <<= 1;
		}
		tab[n] = crc & 0x7fff;
	}
	*/
	// clang-format off
	static const uint_least16_t tab[] = {
		0x0000, 0x4599, 0x4eab, 0x0b32, 0x58cf, 0x1d56, 0x1664, 0x53fd,
		0x7407, 0x319e, 0x3aac, 0x7f35, 0x2cc8, 0x6951, 0x6263, 0x27fa,
		0x2d97, 0x680e, 0x633c, 0x26a5, 0x7558, 0x30c1, 0x3bf3, 0x7e6a,
		0x5990, 0x1c09, 0x173b, 0x52a2, 0x015f, 0x44c6, 0x4ff4, 0x0a6d,
		0x5b2e, 0x1eb7, 0x1585, 0x501c, 0x03e1, 0x4678, 0x4d4a, 0x08d3,
		0x2f29, 0x6ab0, 0x6182, 0x241b, 0x77e6, 0x327f, 0x394d, 0x7cd4,
		0x76b9, 0x3320, 0x3812, 0x7d8b, 0x2e76, 0x6bef, 0x60dd, 0x2544,
		0x02be, 0x4727, 0x4c15, 0x098c, 0x5a71, 0x1fe8, 0x14da, 0x5143,
		0x73c5, 0x365c, 0x3d6e, 0x78f7, 0x2b0a, 0x6e93, 0x65a1, 0x2038,
		0x07c2, 0x425b, 0x4969, 0x0cf0, 0x5f0d, 0x1a94, 0x11a6, 0x543f,
		0x5e52, 0x1bcb, 0x10f9, 0x5560, 0x069d, 0x4304, 0x4836, 0x0daf,
		0x2a55, 0x6fcc, 0x64fe, 0x2167, 0x729a, 0x3703, 0x3c31, 0x79a8,
		0x28eb, 0x6d72, 0x6640, 0x23d9, 0x7024, 0x35bd, 0x3e8f, 0x7b16,
		0x5cec, 0x1975, 0x1247, 0x57de, 0x0423, 0x41ba, 0x4a88, 0x0f11,
		0x057c, 0x40e5, 0x4bd7, 0x0e4e, 0x5db3, 0x182a, 0x1318, 0x5681,
		0x717b, 0x34e2, 0x3fd0, 0x7a49, 0x29b4, 0x6c2d, 0x671f, 0x2286,
		0x2213, 0x678a, 0x6cb8, 0x2921, 0x7adc, 0x3f45, 0x3477, 0x71ee,
		0x5614, 0x138d, 0x18bf, 0x5d26, 0x0edb, 0x4b42, 0x4070, 0x05e9,
		0x0f84, 0x4a1d, 0x412f, 0x04b6, 0x574b, 0x12d2, 0x19e0, 0x5c79,
		0x7b83, 0x3e1a, 0x3528, 0x70b1, 0x234c, 0x66d5, 0x6de7, 0x287e,
		0x793d, 0x3ca4, 0x3796, 0x720f, 0x21f2, 0x646b, 0x6f59, 0x2ac0,
		0x0d3a, 0x48a3, 0x4391, 0x0608, 0x55f5, 0x106c, 0x1b5e, 0x5ec7,
		0x54aa, 0x1133, 0x1a01, 0x5f98, 0x0c65, 0x49fc, 0x42ce, 0x0757,
		0x20ad, 0x6534, 0x6e06, 0x2b9f, 0x7862, 0x3dfb, 0x36c9, 0x7350,
		0x51d6, 0x144f, 0x1f7d, 0x5ae4, 0x0919, 0x4c80, 0x47b2, 0x022b,
		0x25d1, 0x6048, 0x6b7a, 0x2ee3, 0x7d1e, 0x3887, 0x33b5, 0x762c,
		0x7c41, 0x39d8, 0x32ea, 0x7773, 0x248e, 0x6117, 0x6a25, 0x2fbc,
		0x0846, 0x4ddf, 0x46ed, 0x0374, 0x5089, 0x1510, 0x1e22, 0x5bbb,
		0x0af8, 0x4f61, 0x4453, 0x01ca, 0x5237, 0x17ae, 0x1c9c, 0x5905,
		0x7eff, 0x3b66, 0x3054, 0x75cd, 0x2630, 0x63a9, 0x689b, 0x2d02,
		0x276f, 0x62f6, 0x69c4, 0x2c5d, 0x7fa0, 0x3a39, 0x310b, 0x7492,
		0x5368, 0x16f1, 0x1dc3, 0x585a, 0x0ba7, 0x4e3e, 0x450c, 0x0095
	};
	// clang-format on

	while (n--)
		crc = (tab[(*bp++ ^ (crc >> 7)) & 0xff] ^ (crc << 8)) & 0x7fff;
	return crc;
}
