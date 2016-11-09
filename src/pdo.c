/*!\file
 * This file is part of the CANopen library; it contains the implementation of
 * the Process Data Object (PDO) functions.
 *
 * \see lely/co/pdo.h
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

#if !defined(LELY_NO_CO_RPDO) || !defined(LELY_NO_CO_TPDO)

#include <lely/util/endian.h>
#include <lely/can/msg.h>
#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/pdo.h>
#include <lely/co/sdo.h>

#include <assert.h>

#ifndef LELY_NO_CO_RPDO
LELY_CO_EXPORT co_unsigned32_t
co_dev_chk_rpdo(const co_dev_t *dev, co_unsigned16_t idx,
		co_unsigned8_t subidx)
{
	assert(dev);

	if (co_type_is_basic(idx) && !subidx) {
		// If the object is a dummy entry, check if it is enabled.
		if (__unlikely(!(co_dev_get_dummy(dev) & (1 << idx))))
			return CO_SDO_AC_NO_OBJ;
	} else {
		co_obj_t *obj = co_dev_find_obj(dev, idx);
		if (__unlikely(!obj))
			return CO_SDO_AC_NO_OBJ;

		co_sub_t *sub = co_obj_find_sub(obj, subidx);
		if (__unlikely(!sub))
			return CO_SDO_AC_NO_SUB;

		unsigned int access = co_sub_get_access(sub);
		if (__unlikely(!(access & CO_ACCESS_WRITE)))
			return CO_SDO_AC_NO_WRITE;

		if (__unlikely(!co_sub_get_pdo_mapping(sub)
				|| !(access & CO_ACCESS_RPDO)))
			return CO_SDO_AC_NO_PDO;
	}

	return 0;
}
#endif

#ifndef LELY_NO_CO_TPDO
LELY_CO_EXPORT co_unsigned32_t
co_dev_chk_tpdo(const co_dev_t *dev, co_unsigned16_t idx,
		co_unsigned8_t subidx)
{
	assert(dev);

	co_obj_t *obj = co_dev_find_obj(dev, idx);
	if (__unlikely(!obj))
		return CO_SDO_AC_NO_OBJ;

	co_sub_t *sub = co_obj_find_sub(obj, subidx);
	if (__unlikely(!sub))
		return CO_SDO_AC_NO_SUB;

	unsigned int access = co_sub_get_access(sub);
	if (__unlikely(!(access & CO_ACCESS_READ)))
		return CO_SDO_AC_NO_WRITE;

	if (__unlikely(!co_sub_get_pdo_mapping(sub)
			|| !(access & CO_ACCESS_TPDO)))
		return CO_SDO_AC_NO_PDO;

	return 0;
}
#endif

LELY_CO_EXPORT co_unsigned32_t
co_pdo_map(const struct co_pdo_map_par *par, const co_unsigned64_t *val,
		size_t n, uint8_t *buf, size_t *pn)
{
	assert(par);
	assert(val);

	if (__unlikely(par->n > 0x40 || n != par->n))
		return CO_SDO_AC_PDO_LEN;

	size_t offset = 0;
	for (size_t i = 0; i < par->n; i++) {
		co_unsigned8_t len = par->map[i] & 0xff;

		if (__unlikely(offset + len > CAN_MAX_LEN * 8))
			return CO_SDO_AC_PDO_LEN;

		uint8_t tmp[sizeof(co_unsigned64_t)] = { 0 };
		stle_u64(tmp, val[i]);
		if (buf && pn && offset + len <= *pn * 8)
			bcpyle(buf, offset, val, 0, len);

		offset += len;
	}

	if (pn)
		*pn = (offset + 7) / 8;

	return 0;
}

LELY_CO_EXPORT co_unsigned32_t
co_pdo_unmap(const struct co_pdo_map_par *par, const uint8_t *buf, size_t n,
		co_unsigned64_t *val, size_t *pn)
{
	assert(par);
	assert(buf);

	if (__unlikely(par->n > 0x40))
		return CO_SDO_AC_PDO_LEN;

	size_t offset = 0;
	for (size_t i = 0; i < par->n; i++) {
		co_unsigned8_t len = par->map[i] & 0xff;

		if (__unlikely(offset + len > n * 8))
			return CO_SDO_AC_PDO_LEN;

		uint8_t tmp[sizeof(co_unsigned64_t)] = { 0 };
		bcpyle(tmp, 0, buf, offset, len);
		if (val && pn && i < *pn)
			val[i] = ldle_u64(tmp);

		offset += len;
	}

	if (pn)
		*pn = par->n;

	return 0;
}

#ifndef LELY_NO_CO_RPDO
LELY_CO_EXPORT co_unsigned32_t
co_pdo_read(const struct co_pdo_map_par *par, co_dev_t *dev,
		struct co_sdo_req *req, const uint8_t *buf, size_t n)
{
	assert(par);
	assert(dev);
	assert(req);
	assert(buf);

	if (__unlikely(n > CAN_MAX_LEN))
		return CO_SDO_AC_PDO_LEN;

	co_unsigned32_t ac = 0;

	size_t offset = 0;
	for (size_t i = 0; i < MIN(par->n, 0x40u); i++) {
		co_unsigned32_t map = par->map[i];
		co_unsigned16_t idx = (map >> 16) & 0xffff;
		co_unsigned8_t subidx = (map >> 8) & 0xff;
		co_unsigned8_t len = map & 0xff;

		// Check the PDO length.
		if (__unlikely(offset + len > n * 8))
			return CO_SDO_AC_PDO_LEN;

		// Check whether the sub-object exists and can be mapped into a
		// PDO (or is a valid dummy entry).
		ac = co_dev_chk_rpdo(dev, idx, subidx);
		if (__unlikely(ac))
			return ac;

		co_sub_t *sub = co_dev_find_sub(dev, idx, subidx);
		if (sub) {
			// Copy the value and download it into the sub-object.
			uint8_t tmp[CAN_MAX_LEN] = { 0 };
			bcpyle(tmp, 0, buf, offset, len);
			co_sdo_req_clear(req);
			req->size = (len + 7) / 8;
			req->buf = tmp;
			req->nbyte = req->size;
			ac = co_sub_dn_ind(sub, req);
			if (__unlikely(ac))
				return ac;
		}

		offset += len;
	}

	// Also return an error if we received too many bytes.
	if (__unlikely((offset + 7) / 8 < n))
		return CO_SDO_AC_PDO_LEN;

	return ac;
}
#endif // !LELY_NO_CO_RPDO

#ifndef LELY_NO_CO_TPDO
LELY_CO_EXPORT co_unsigned32_t
co_pdo_write(const struct co_pdo_map_par *par, const co_dev_t *dev,
		struct co_sdo_req *req, uint8_t *buf, size_t *pn)
{
	assert(par);
	assert(dev);
	assert(req);

	co_unsigned32_t ac = 0;

	size_t offset = 0;
	for (size_t i = 0; i < MIN(par->n, 0x40u); i++) {
		co_unsigned32_t map = par->map[i];
		co_unsigned16_t idx = (map >> 16) & 0xffff;
		co_unsigned8_t subidx = (map >> 8) & 0xff;
		co_unsigned8_t len = map & 0xff;

		// Check the PDO length.
		if (__unlikely(offset + len > CAN_MAX_LEN * 8))
			return CO_SDO_AC_PDO_LEN;

		// Check whether the sub-object exists and can be mapped into a
		// PDO.
		ac = co_dev_chk_tpdo(dev, idx, subidx);
		if (__unlikely(ac))
			return ac;

		// Upload the value of the sub-object and copy the value.
		co_sdo_req_clear(req);
		ac = co_sub_up_ind(co_dev_find_sub(dev, idx, subidx), req);
		if (__unlikely(ac))
			return ac;
		if (__unlikely(!co_sdo_req_first(req) || !co_sdo_req_last(req)))
			return CO_SDO_AC_PDO_LEN;
		if (buf && pn && offset + len <= *pn * 8)
			bcpyle(buf, offset, req->buf, 0, len);

		offset += len;
	}

	if (pn)
		*pn = (offset + 7) / 8;

	return ac;
}
#endif // !LELY_NO_CO_TPDO

#endif // !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO

