/*!\file
 * This file is part of the CANopen library; it contains the implementation of
 * the Service Data Object (SDO) functions.
 *
 * \see lely/co/sdo.h
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

#include "co.h"
#include <lely/util/errnum.h>
#include <lely/co/sdo.h>
#include <lely/co/val.h>

#include <assert.h>

/*!
 * Copies the next segment of the specified CANopen SDO download request to the
 * internal buffer.
 *
 * \param req    a pointer to a CANopen SDO download request.
 * \param pptr   the address of a pointer which, on success, points to the first
 *               byte in the buffer (can be NULL).
 * \param pnbyte the address of a value which, on success, contains the total
 *               number of bytes in the buffer (can be NULL).
 *
 * \returns 1 if all segments have been copied, 0 if one or more segments
 * remain, or -1 on error. In the latter case, the error number can be obtained
 * with `get_errnum()`.
 *
 * \see co_sdo_req_dn()
 */
static int co_sdo_req_dn_buf(struct co_sdo_req *req, const void **pptr,
		size_t *pnbyte);

/*!
 * Constructs a CANopen SDO upload request from its internal buffer.
 *
 * \see co_sdo_req_up()
 */
static void co_sdo_req_up_buf(struct co_sdo_req *req);

LELY_CO_EXPORT const char *
co_sdo_ac2str(co_unsigned32_t ac)
{
	switch (ac) {
	case CO_SDO_AC_TOGGLE:
		return "Toggle bit not altered";
	case CO_SDO_AC_TIMEOUT:
		return "SDO protocol timed out";
	case CO_SDO_AC_NO_CS:
		return "Client/server command specifier not valid or unknown";
	case CO_SDO_AC_BLK_SIZE:
		return "Invalid block size";
	case CO_SDO_AC_BLK_SEQ:
		return "Invalid sequence number";
	case CO_SDO_AC_BLK_CRC:
		return "CRC error";
	case CO_SDO_AC_NO_MEM:
		return "Out of memory";
	case CO_SDO_AC_NO_ACCES:
		return "Unsupported access to an object";
	case CO_SDO_AC_NO_RO:
		return "Attempt to read a write only object";
	case CO_SDO_AC_NO_WO:
		return "Attempt to write a read only object";
	case CO_SDO_AC_NO_OBJ:
		return "Object does not exist in the object dictionary";
	case CO_SDO_AC_NO_PDO:
		return "Object cannot be mapped to the PDO";
	case CO_SDO_AC_PDO_LEN:
		return "The number and length of the objects to be mapped would exceed the PDO length";
	case CO_SDO_AC_PARAM:
		return "General parameter incompatibility reason";
	case CO_SDO_AC_COMPAT:
		return "General internal incompatibility in the device";
	case CO_SDO_AC_HARDWARE:
		return "Access failed due to a hardware error";
	case CO_SDO_AC_TYPE_LEN:
		return "Data type does not match, length of service parameter does not match";
	case CO_SDO_AC_TYPE_LEN_HI:
		return "Data type does not match, length of service parameter too high";
	case CO_SDO_AC_TYPE_LEN_LO:
		return "Data type does not match, length of service parameter too low";
	case CO_SDO_AC_NO_SUB:
		return "Sub-index does not exist";
	case CO_SDO_AC_PARAM_VAL:
		return "Invalid value for parameter";
	case CO_SDO_AC_PARAM_HI:
		return "Value of parameter written too high";
	case CO_SDO_AC_PARAM_LO:
		return "Value of parameter written too low";
	case CO_SDO_AC_PARAM_RANGE:
		return "Maximum value is less than minimum value";
	case CO_SDO_AC_NO_SDO:
		return "Resource not available: SDO connection";
	case CO_SDO_AC_ERROR:
		return "General error";
	case CO_SDO_AC_DATA:
		return "Data cannot be transferred or stored to the application";
	case CO_SDO_AC_DATA_CTL:
		return "Data cannot be transferred or stored to the application because of local control";
	case CO_SDO_AC_DATA_DEV:
		return "Data cannot be transferred or stored to the application because of the present device state";
	case CO_SDO_AC_NO_OD:
		return "Object dictionary dynamic generation fails or no object dictionary is present";
	case CO_SDO_AC_NO_DATA:
		return "No data available";
	default:
		return "Unknown abort code";
	}
}

LELY_CO_EXPORT uint16_t
co_sdo_crc(uint16_t crc, const void *ptr, size_t n)
{
	// This table contains precomputed CRC-16 checksums for each of the 256
	// bytes. The table was computed with the following code:
	/*
	uint16_t tab[256];
	for (int n = 0; n < 256; n++) {
		uint16_t crc = n << 8;
		for (int k = 0; k < 8; k++) {
			if (crc & 0x8000)
				crc = (crc << 1) ^ 0x1021;
			else
				crc <<= 1;
		}
		tab[n] = crc;
	}
	*/
	static const uint16_t tab[] = {
		0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
		0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
		0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
		0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
		0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
		0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
		0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
		0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
		0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
		0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
		0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
		0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
		0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
		0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
		0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
		0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
		0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
		0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
		0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
		0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
		0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
		0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
		0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
		0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
		0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
		0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
		0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
		0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
		0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
		0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
		0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
		0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
	};

	if (__likely(ptr && n)) {
		const uint8_t *bp = ptr;
		while (n--)
			crc = tab[*bp++ ^ (crc >> 8)] ^ (crc << 8);
	}
	return crc;
}

LELY_CO_EXPORT void
co_sdo_req_init(struct co_sdo_req *req)
{
	assert(req);

	req->size = 0;
	req->buf = NULL;
	req->nbyte = 0;
	req->offset = 0;
	membuf_init(&req->membuf);
}

LELY_CO_EXPORT void
co_sdo_req_fini(struct co_sdo_req *req)
{
	assert(req);

	membuf_fini(&req->membuf);
}

LELY_CO_EXPORT void
co_sdo_req_clear(struct co_sdo_req *req)
{
	req->size = 0;
	req->buf = NULL;
	req->nbyte = 0;
	req->offset = 0;
	membuf_clear(&req->membuf);
}

LELY_CO_EXPORT int
co_sdo_req_dn(struct co_sdo_req *req, co_unsigned16_t type, void *val,
		co_unsigned32_t *pac)
{
	errc_t errc = get_errnum();
	co_unsigned32_t ac = 0;

	const void *ptr = NULL;
	size_t nbyte = 0;
	switch (co_sdo_req_dn_buf(req, &ptr, &nbyte)) {
	default:
		// Convert the error number to an SDO abort code.
		ac = get_errnum() == ERRNUM_NOMEM
				? CO_SDO_AC_NO_MEM : CO_SDO_AC_ERROR;
		set_errc(errc);
	case 0:
		// Return without an abort code if not all data is present. This
		// is not an error.
		goto error_done;
	case 1:
		break;
	}

	// Read the value.
	co_val_init(type, val);
	size_t size = co_val_read(type, val, ptr, (const uint8_t *)ptr + nbyte);

	// Check the size of the value.
	if (co_type_is_array(type)) {
		if (__unlikely(size != nbyte)) {
			ac = CO_SDO_AC_NO_MEM;
			goto error_read;
		}
	} else {
		if (__unlikely(!size)) {
			ac = CO_SDO_AC_TYPE_LEN_LO;
			goto error_read;
		} else if (__unlikely(size < nbyte)) {
			ac = CO_SDO_AC_TYPE_LEN_HI;
			goto error_read;
		}
	}

	return 0;

error_read:
	co_val_fini(type, val);
error_done:
	if (pac)
		*pac = ac;
	return -1;
}

LELY_CO_EXPORT int
co_sdo_req_up(struct co_sdo_req *req, co_unsigned16_t type, const void *val,
		co_unsigned32_t *pac)
{
	assert(req);
	struct membuf *buf = &req->membuf;

	co_unsigned32_t ac = 0;

	size_t size = co_val_write(type, val, NULL, NULL);

	membuf_clear(buf);
	if (__unlikely(size && !membuf_reserve(buf, size))) {
		ac = CO_SDO_AC_NO_MEM;
		goto error_reserve;
	}

	uint8_t *begin = membuf_alloc(buf, &size);
	if (__unlikely(co_val_write(type, val, begin, begin + size) != size)) {
		ac = CO_SDO_AC_ERROR;
		goto error_write;
	}

	co_sdo_req_up_buf(req);
	return 0;

error_write:
error_reserve:
	if (pac)
		*pac = ac;
	return -1;
}

static int
co_sdo_req_dn_buf(struct co_sdo_req *req, const void **pptr, size_t *pnbyte)
{
	assert(req);
	struct membuf *buf = &req->membuf;

	// In case of an error, keep track of the offset with respect to the
	// position indicator of the buffer.
	ptrdiff_t offset = -(ptrdiff_t)membuf_size(buf);

	const void *ptr;
	if (co_sdo_req_first(req) && co_sdo_req_last(req)) {
		// If the entire value is available right away, skip copying the
		// data to the buffer.
		ptr = req->buf;
	} else {
		if (co_sdo_req_first(req)) {
			membuf_clear(buf);
			if (__unlikely(req->size
					&& !membuf_reserve(buf, req->size)))
				goto error_reserve;
		} else {
			// Adjust the offset if necessary. Only backtracking is
			// allowed.
			offset += req->offset;
			if (offset) {
				if (__unlikely(offset > 0)) {
					set_errnum(ERRNUM_INVAL);
					goto error_offset;
				}
				membuf_seek(buf, offset);
			}
		}

		if (req->nbyte) {
			if (__unlikely(req->nbyte > membuf_capacity(buf))) {
				set_errnum(ERRNUM_INVAL);
				goto error_nbyte;
			}
			membuf_write(buf, req->buf, req->nbyte);
		}

		if (!co_sdo_req_last(req))
			return 0;

		ptr = membuf_begin(buf);
	}

	if (pptr)
		*pptr = ptr;
	if (pnbyte)
		*pnbyte = req->size;

	return 1;

error_nbyte:
error_reserve:
	// Restore the position indicator of the buffer.
	membuf_seek(buf, -offset);
error_offset:
	return -1;
}

static void
co_sdo_req_up_buf(struct co_sdo_req *req)
{
	assert(req);
	struct membuf *buf = &req->membuf;

	req->size = membuf_size(buf);
	req->buf = membuf_begin(buf);
	req->nbyte = req->size;
	req->offset = 0;
}

