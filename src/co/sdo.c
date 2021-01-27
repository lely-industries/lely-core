/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the Service Data Object (SDO) functions.
 *
 * @see lely/co/sdo.h
 *
 * @copyright 2020 Lely Industries N.V.
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

#include "co.h"
#define LELY_CO_SDO_INLINE extern inline
#include <lely/util/diag.h>
#if !LELY_NO_CO_OBJ_FILE
#include <lely/util/frbuf.h>
#include <lely/util/fwbuf.h>
#endif
#include <lely/co/sdo.h>
#include <lely/co/val.h>

#include <assert.h>

/**
 * Copies the next segment of the specified CANopen SDO download request to the
 * internal buffer.
 *
 * @param req    a pointer to a CANopen SDO download request.
 * @param pptr   the address of a pointer which, on success, points to the first
 *               byte in the buffer (can be NULL).
 * @param pnbyte the address of a value which, on success, contains the total
 *               number of bytes in the buffer (can be NULL).
 *
 * @returns 1 if all segments have been copied, 0 if one or more segments
 * remain, or -1 on error. In the latter case, the error number can be obtained
 * with get_errc().
 */
static int co_sdo_req_dn_buf(
		struct co_sdo_req *req, const void **pptr, size_t *pnbyte);

/// Constructs a CANopen SDO upload request from its internal buffer.
static void co_sdo_req_up_buf(struct co_sdo_req *req);

const char *
co_sdo_ac2str(co_unsigned32_t ac)
{
	switch (ac) {
	case 0: return "Success";
	case CO_SDO_AC_TOGGLE: return "Toggle bit not altered";
	case CO_SDO_AC_TIMEOUT: return "SDO protocol timed out";
	case CO_SDO_AC_NO_CS:
		return "Client/server command specifier not valid or unknown";
	case CO_SDO_AC_BLK_SIZE: return "Invalid block size";
	case CO_SDO_AC_BLK_SEQ: return "Invalid sequence number";
	case CO_SDO_AC_BLK_CRC: return "CRC error";
	case CO_SDO_AC_NO_MEM: return "Out of memory";
	case CO_SDO_AC_NO_ACCESS: return "Unsupported access to an object";
	case CO_SDO_AC_NO_READ: return "Attempt to read a write only object";
	case CO_SDO_AC_NO_WRITE: return "Attempt to write a read only object";
	case CO_SDO_AC_NO_OBJ:
		return "Object does not exist in the object dictionary";
	case CO_SDO_AC_NO_PDO: return "Object cannot be mapped to the PDO";
	case CO_SDO_AC_PDO_LEN:
		return "The number and length of the objects to be mapped would exceed the PDO length";
	case CO_SDO_AC_PARAM: return "General parameter incompatibility reason";
	case CO_SDO_AC_COMPAT:
		return "General internal incompatibility in the device";
	case CO_SDO_AC_HARDWARE: return "Access failed due to a hardware error";
	case CO_SDO_AC_TYPE_LEN:
		return "Data type does not match, length of service parameter does not match";
	case CO_SDO_AC_TYPE_LEN_HI:
		return "Data type does not match, length of service parameter too high";
	case CO_SDO_AC_TYPE_LEN_LO:
		return "Data type does not match, length of service parameter too low";
	case CO_SDO_AC_NO_SUB: return "Sub-index does not exist";
	case CO_SDO_AC_PARAM_VAL: return "Invalid value for parameter";
	case CO_SDO_AC_PARAM_HI: return "Value of parameter written too high";
	case CO_SDO_AC_PARAM_LO: return "Value of parameter written too low";
	case CO_SDO_AC_PARAM_RANGE:
		return "Maximum value is less than minimum value";
	case CO_SDO_AC_NO_SDO: return "Resource not available: SDO connection";
	case CO_SDO_AC_ERROR: return "General error";
	case CO_SDO_AC_DATA:
		return "Data cannot be transferred or stored to the application";
	case CO_SDO_AC_DATA_CTL:
		return "Data cannot be transferred or stored to the application because of local control";
	case CO_SDO_AC_DATA_DEV:
		return "Data cannot be transferred or stored to the application because of the present device state";
	case CO_SDO_AC_NO_OD:
		return "Object dictionary dynamic generation fails or no object dictionary is present";
	case CO_SDO_AC_NO_DATA: return "No data available";
	default: return "Unknown abort code";
	}
}

void
co_sdo_req_init(struct co_sdo_req *req)
{
	assert(req);

	req->size = 0;
	req->buf = NULL;
	req->nbyte = 0;
	req->offset = 0;
	membuf_init(&req->membuf, NULL, 0);
}

void
co_sdo_req_fini(struct co_sdo_req *req)
{
	assert(req);

	membuf_fini(&req->membuf);
}

void
co_sdo_req_clear(struct co_sdo_req *req)
{
	req->size = 0;
	req->buf = NULL;
	req->nbyte = 0;
	req->offset = 0;
	membuf_clear(&req->membuf);
}

int
co_sdo_req_dn(struct co_sdo_req *req, const void **pptr, size_t *pnbyte,
		co_unsigned32_t *pac)
{
	co_unsigned32_t ac = 0;

	int errc = get_errc();
	switch (co_sdo_req_dn_buf(req, pptr, pnbyte)) {
	default:
		// Convert the error number to an SDO abort code.
#if LELY_NO_ERRNO
		ac = CO_SDO_AC_ERROR;
#else
		// clang-format off
		ac = get_errnum() == ERRNUM_NOMEM
				? CO_SDO_AC_NO_MEM
				: CO_SDO_AC_ERROR;
		// clang-format on
#endif
		set_errc(errc);
	// ...falls through ...
	case 0:
		// Return without an abort code if not all data is present. This
		// is not an error.
		if (pac)
			*pac = ac;
		return -1;
	case 1: return 0;
	}
}

int
co_sdo_req_dn_val(struct co_sdo_req *req, co_unsigned16_t type, void *val,
		co_unsigned32_t *pac)
{
	co_unsigned32_t ac = 0;

	const void *ptr = NULL;
	size_t nbyte = 0;
	if (co_sdo_req_dn(req, &ptr, &nbyte, pac) == -1)
		return -1;

	// Read the value.
	co_val_init(type, val);
	size_t size = co_val_read(
			type, val, ptr, (const uint_least8_t *)ptr + nbyte);

	// Check the size of the value.
	if (co_type_is_array(type)) {
		if (size != nbyte) {
			ac = CO_SDO_AC_NO_MEM;
			goto error_read;
		}
	} else {
		if (!size) {
			ac = CO_SDO_AC_TYPE_LEN_LO;
			goto error_read;
		} else if (size < nbyte) {
			ac = CO_SDO_AC_TYPE_LEN_HI;
			goto error_read;
		}
	}

	return 0;

error_read:
	co_val_fini(type, val);
	if (pac)
		*pac = ac;
	return -1;
}

#if !LELY_NO_CO_OBJ_FILE
int
co_sdo_req_dn_file(struct co_sdo_req *req, const char *filename,
		co_unsigned32_t *pac)
{
	int errc = get_errc();
	co_unsigned32_t ac = 0;

	const void *ptr = NULL;
	size_t nbyte = 0;
	if (co_sdo_req_dn(req, &ptr, &nbyte, pac) == -1)
		return -1;

	fwbuf_t *fbuf = fwbuf_create(filename);
	if (!fbuf) {
		diag(DIAG_ERROR, get_errc(), "%s", filename);
		ac = CO_SDO_AC_DATA;
		goto error_create_fbuf;
	}

	if (fwbuf_write(fbuf, ptr, nbyte) != (ssize_t)nbyte) {
		diag(DIAG_ERROR, get_errc(), "%s", filename);
		ac = CO_SDO_AC_DATA;
		goto error_write;
	}

	if (fwbuf_commit(fbuf) == -1) {
		diag(DIAG_ERROR, get_errc(), "%s", filename);
		ac = CO_SDO_AC_DATA;
		goto error_commit;
	}

	fwbuf_destroy(fbuf);

	return 0;

error_commit:
error_write:
	fwbuf_destroy(fbuf);
error_create_fbuf:
	if (pac)
		*pac = ac;
	set_errc(errc);
	return -1;
}
#endif // !LELY_NO_CO_OBJ_FILE

int
co_sdo_req_up(struct co_sdo_req *req, const void *ptr, size_t n,
		co_unsigned32_t *pac)
{
	assert(req);
	struct membuf *buf = &req->membuf;

	co_unsigned32_t ac = 0;

	membuf_clear(buf);
	int errc = get_errc();
	if (n && !membuf_reserve(buf, n)) {
		ac = CO_SDO_AC_NO_MEM;
		set_errc(errc);
		goto error_reserve;
	}

	if (ptr)
		membuf_write(buf, ptr, n);

	co_sdo_req_up_buf(req);
	return 0;

error_reserve:
	if (pac)
		*pac = ac;
	return -1;
}

int
co_sdo_req_up_val(struct co_sdo_req *req, co_unsigned16_t type, const void *val,
		co_unsigned32_t *pac)
{
	assert(req);
	struct membuf *buf = &req->membuf;

	co_unsigned32_t ac = 0;

	size_t size = co_val_write(type, val, NULL, NULL);

	membuf_clear(buf);
	int errc = get_errc();
	if (size && !membuf_reserve(buf, size)) {
		ac = CO_SDO_AC_NO_MEM;
		set_errc(errc);
		goto error_reserve;
	}

	uint_least8_t *begin = membuf_alloc(buf, &size);
	if (co_val_write(type, val, begin, begin + size) != size) {
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

#if !LELY_NO_CO_OBJ_FILE
int
co_sdo_req_up_file(struct co_sdo_req *req, const char *filename,
		co_unsigned32_t *pac)
{
	assert(req);
	struct membuf *buf = &req->membuf;

	int errc = get_errc();
	co_unsigned32_t ac = 0;

	frbuf_t *fbuf = frbuf_create(filename);
	if (!fbuf) {
		diag(DIAG_ERROR, get_errc(), "%s", filename);
		ac = CO_SDO_AC_DATA;
		goto error_create_fbuf;
	}

	intmax_t size = frbuf_get_size(fbuf);
	if (size == -1) {
		diag(DIAG_ERROR, get_errc(), "%s", filename);
		ac = CO_SDO_AC_DATA;
		goto error_get_size;
	}
	size_t nbyte = (size_t)size;

	membuf_clear(buf);
	if (size && !membuf_reserve(buf, nbyte)) {
		ac = CO_SDO_AC_NO_MEM;
		goto error_reserve;
	}

	void *ptr = membuf_alloc(buf, &nbyte);
	if (frbuf_read(fbuf, ptr, nbyte) != (ssize_t)nbyte) {
		diag(DIAG_ERROR, get_errc(), "%s", filename);
		ac = CO_SDO_AC_DATA;
		goto error_read;
	}

	frbuf_destroy(fbuf);

	co_sdo_req_up_buf(req);
	return 0;

error_read:
error_reserve:
error_get_size:
	frbuf_destroy(fbuf);
error_create_fbuf:
	if (pac)
		*pac = ac;
	set_errc(errc);
	return -1;
}
#endif // !LELY_NO_CO_OBJ_FILE

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
			assert(req->size);
			if (!membuf_reserve(buf, req->size))
				goto error_reserve;
		} else {
			// Adjust the offset if necessary. Only backtracking is
			// allowed.
			offset += req->offset;
			if (offset) {
				if (offset > 0) {
					set_errnum(ERRNUM_INVAL);
					goto error_offset;
				}
				membuf_seek(buf, offset);
			}
		}

		if (req->nbyte) {
			if (req->nbyte > membuf_capacity(buf)) {
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
