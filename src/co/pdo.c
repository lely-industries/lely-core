/**@file
 * This file is part of the CANopen library; it contains the implementation of
 * the Process Data Object (PDO) functions.
 *
 * @see lely/co/pdo.h
 *
 * @copyright 2016-2021 Lely Industries N.V.
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

#if !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO

#include <lely/can/msg.h>
#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/pdo.h>
#include <lely/co/sdo.h>
#include <lely/util/endian.h>

#include <assert.h>

static co_unsigned32_t co_dev_cfg_pdo_comm(const co_dev_t *dev,
		co_unsigned16_t idx, const struct co_pdo_comm_par *par);

static co_unsigned32_t co_dev_cfg_pdo_map(const co_dev_t *dev,
		co_unsigned16_t num, const struct co_pdo_map_par *par);

#if !LELY_NO_CO_RPDO && !LELY_NO_CO_MPDO
co_unsigned32_t co_mpdo_dn(const struct co_pdo_map_par *par, co_dev_t *dev,
		struct co_sdo_req *req, const uint_least8_t *buf, size_t n);
#endif

#if !LELY_NO_CO_RPDO

co_unsigned32_t
co_dev_chk_rpdo(const co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx)
{
	assert(dev);

	if (co_type_is_basic(idx) && !subidx) {
		// If the object is a dummy entry, check if it is enabled.
		if (!(co_dev_get_dummy(dev) & (1 << idx)))
			return CO_SDO_AC_NO_OBJ;
	} else {
		co_obj_t *obj = co_dev_find_obj(dev, idx);
		if (!obj)
			return CO_SDO_AC_NO_OBJ;

		co_sub_t *sub = co_obj_find_sub(obj, subidx);
		if (!sub)
			return CO_SDO_AC_NO_SUB;

		unsigned int access = co_sub_get_access(sub);
		if (!(access & CO_ACCESS_WRITE))
			return CO_SDO_AC_NO_WRITE;

		if (!co_sub_get_pdo_mapping(sub) || !(access & CO_ACCESS_RPDO))
			return CO_SDO_AC_NO_PDO;
	}

	return 0;
}

co_unsigned32_t
co_dev_cfg_rpdo(const co_dev_t *dev, co_unsigned16_t num,
		const struct co_pdo_comm_par *comm,
		const struct co_pdo_map_par *map)
{
	assert(comm);

	co_unsigned32_t ac = 0;

	struct co_pdo_comm_par par = *comm;
	// Disable the RPDO service before configuring any parameters.
	par.cobid |= CO_PDO_COBID_VALID;
	ac = co_dev_cfg_rpdo_comm(dev, num, &par);
	if (ac)
		return ac;

	ac = co_dev_cfg_rpdo_map(dev, num, map);
	if (ac)
		return ac;

	// Re-enable the RPDO service if necessary.
	if (!(comm->cobid & CO_PDO_COBID_VALID))
		ac = co_dev_cfg_rpdo_comm(dev, num, comm);
	return ac;
}

co_unsigned32_t
co_dev_cfg_rpdo_comm(const co_dev_t *dev, co_unsigned16_t num,
		const struct co_pdo_comm_par *par)
{
	if (!num || num > CO_NUM_PDOS)
		return CO_SDO_AC_NO_OBJ;

	return co_dev_cfg_pdo_comm(dev, 0x1400 + num - 1, par);
}

co_unsigned32_t
co_dev_cfg_rpdo_map(const co_dev_t *dev, co_unsigned16_t num,
		const struct co_pdo_map_par *par)
{
	if (!num || num > CO_NUM_PDOS)
		return CO_SDO_AC_NO_OBJ;

	return co_dev_cfg_pdo_map(dev, 0x1600 + num - 1, par);
}

#endif // !LELY_NO_CO_RPDO

#if !LELY_NO_CO_TPDO

co_unsigned32_t
co_dev_chk_tpdo(const co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx)
{
	assert(dev);

	co_obj_t *obj = co_dev_find_obj(dev, idx);
	if (!obj)
		return CO_SDO_AC_NO_OBJ;

	co_sub_t *sub = co_obj_find_sub(obj, subidx);
	if (!sub)
		return CO_SDO_AC_NO_SUB;

	unsigned int access = co_sub_get_access(sub);
	if (!(access & CO_ACCESS_READ))
		return CO_SDO_AC_NO_READ;

	if (!co_sub_get_pdo_mapping(sub) || !(access & CO_ACCESS_TPDO))
		return CO_SDO_AC_NO_PDO;

	return 0;
}

co_unsigned32_t
co_dev_cfg_tpdo(const co_dev_t *dev, co_unsigned16_t num,
		const struct co_pdo_comm_par *comm,
		const struct co_pdo_map_par *map)
{
	assert(comm);

	co_unsigned32_t ac = 0;

	struct co_pdo_comm_par par = *comm;
	// Disable the TPDO service before configuring any parameters.
	par.cobid |= CO_PDO_COBID_VALID;
	ac = co_dev_cfg_tpdo_comm(dev, num, &par);
	if (ac)
		return ac;

	ac = co_dev_cfg_tpdo_map(dev, num, map);
	if (ac)
		return ac;

	// Re-enable the TPDO service if necessary.
	if (!(comm->cobid & CO_PDO_COBID_VALID))
		ac = co_dev_cfg_tpdo_comm(dev, num, comm);
	return ac;
}

co_unsigned32_t
co_dev_cfg_tpdo_comm(const co_dev_t *dev, co_unsigned16_t num,
		const struct co_pdo_comm_par *par)
{
	if (!num || num > CO_NUM_PDOS)
		return CO_SDO_AC_NO_OBJ;

	return co_dev_cfg_pdo_comm(dev, 0x1800 + num - 1, par);
}

co_unsigned32_t
co_dev_cfg_tpdo_map(const co_dev_t *dev, co_unsigned16_t num,
		const struct co_pdo_map_par *par)
{
	if (!num || num > CO_NUM_PDOS)
		return CO_SDO_AC_NO_OBJ;

	return co_dev_cfg_pdo_map(dev, 0x1a00 + num - 1, par);
}

#endif // !LELY_NO_CO_TPDO

#if !LELY_NO_CO_MPDO

int
co_dev_chk_sam_mpdo(
		const co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx)
{
	assert(dev);

	// Loop over all sub-objects in the object scanner list (1FA0..1FCF).
	co_obj_t *obj = NULL;
	for (co_unsigned16_t i = 0x1fa0; !obj && i <= 0x1fcf; i++)
		obj = co_dev_find_obj(dev, i);
	for (; obj && co_obj_get_idx(obj) <= 0x1fcf; obj = co_obj_next(obj)) {
		co_unsigned8_t n = co_obj_get_val_u8(obj, 0);
		if (!n)
			continue;
		co_sub_t *sub = co_sub_next(co_obj_first_sub(obj));
		for (; sub && co_sub_get_subidx(sub) <= n;
				sub = co_sub_next(sub)) {
			co_unsigned32_t val = co_sub_get_val_u32(sub);
			if (!val)
				continue;
			// Check if the object index matches.
			if (((val >> 8) & 0xffff) != idx)
				continue;
			// Check if the object sub-index is part of the
			// specified block.
			co_unsigned8_t min = val & 0xff;
			if (subidx < min)
				continue;
			co_unsigned8_t max = min;
			co_unsigned8_t blk = (val >> 24) & 0xff;
			if (blk)
				max += MIN(blk - 1, 0xff - min);
			if (subidx > max)
				continue;

			return 1;
		}
	}

	return 0;
}

int
co_dev_map_sam_mpdo(const co_dev_t *dev, co_unsigned8_t id, co_unsigned16_t idx,
		co_unsigned8_t subidx, co_unsigned16_t *pidx,
		co_unsigned8_t *psubidx)
{
	assert(dev);

	// Loop over all sub-objects in the object dispatching list
	// (1FD0..1FFF).
	co_obj_t *obj = NULL;
	for (co_unsigned16_t i = 0x1fd0; !obj && i <= 0x1fff; i++)
		obj = co_dev_find_obj(dev, i);
	for (; obj && co_obj_get_idx(obj) <= 0x1fff; obj = co_obj_next(obj)) {
		co_unsigned8_t n = co_obj_get_val_u8(obj, 0);
		if (!n)
			continue;
		co_sub_t *sub = co_sub_next(co_obj_first_sub(obj));
		for (; sub && co_sub_get_subidx(sub) <= n;
				sub = co_sub_next(sub)) {
			co_unsigned64_t val = co_sub_get_val_u64(sub);
			if (!val)
				continue;
			// Check if the remote node-ID and object index match.
			if ((val & 0xff) != id)
				continue;
			if (((val >> 16) & 0xffff) != idx)
				continue;
			// Check if the remote object sub-index is part of the
			// specified block.
			co_unsigned8_t min = (val >> 8) & 0xff;
			if (subidx < min)
				continue;
			co_unsigned8_t max = min;
			co_unsigned8_t blk = (val >> 54) & 0xff;
			if (blk)
				max += MIN(blk - 1, 0xff - min);
			if (subidx > max)
				continue;

			if (pidx)
				*pidx = (val >> 40) & 0xffff;
			if (psubidx) {
				*psubidx = (val >> 32) & 0xff;
				*psubidx += subidx - min;
			}
			return 1;
		}
	}

	return 0;
}

#endif // !LELY_NO_CO_MPDO

co_unsigned32_t
co_pdo_map(const struct co_pdo_map_par *par, const co_unsigned64_t *val,
		co_unsigned8_t n, uint_least8_t *buf, size_t *pn)
{
	assert(par);
	assert(val);

	if (par->n > CO_PDO_NUM_MAPS || n != par->n)
		return CO_SDO_AC_PDO_LEN;

	size_t offset = 0;
	for (size_t i = 0; i < par->n; i++) {
		co_unsigned32_t map = par->map[i];
		if (!map)
			continue;

		co_unsigned8_t len = map & 0xff;
		if (offset + len > CAN_MAX_LEN * 8)
			return CO_SDO_AC_PDO_LEN;

		uint_least8_t tmp[sizeof(co_unsigned64_t)] = { 0 };
		stle_u64(tmp, val[i]);
		if (buf && pn && offset + len <= *pn * 8)
			bcpyle(buf, offset, tmp, 0, len);

		offset += len;
	}

	if (pn)
		*pn = (offset + 7) / 8;

	return 0;
}

co_unsigned32_t
co_pdo_unmap(const struct co_pdo_map_par *par, const uint_least8_t *buf,
		size_t n, co_unsigned64_t *val, co_unsigned8_t *pn)
{
	assert(par);
	assert(buf);

	if (par->n > CO_PDO_NUM_MAPS)
		return CO_SDO_AC_PDO_LEN;

	size_t offset = 0;
	for (size_t i = 0; i < par->n; i++) {
		co_unsigned32_t map = par->map[i];
		if (!map)
			continue;

		co_unsigned8_t len = map & 0xff;
		if (offset + len > n * 8)
			return CO_SDO_AC_PDO_LEN;

		uint_least8_t tmp[sizeof(co_unsigned64_t)] = { 0 };
		bcpyle(tmp, 0, buf, offset, len);
		if (val && pn && i < *pn)
			val[i] = ldle_u64(tmp);

		offset += len;
	}

	if (pn)
		*pn = par->n;

	return 0;
}

#if !LELY_NO_CO_RPDO
co_unsigned32_t
co_pdo_dn(const struct co_pdo_map_par *par, co_dev_t *dev,
		struct co_sdo_req *req, const uint_least8_t *buf, size_t n)
{
	assert(par);
	assert(dev);
	assert(req);
	assert(buf);

	if (par->n > CO_PDO_NUM_MAPS) {
#if LELY_NO_CO_MPDO
		return CO_SDO_AC_PDO_LEN;
#else
		return co_mpdo_dn(par, dev, req, buf, n);
#endif // !LELY_NO_CO_MPDO
	}

	if (n > CAN_MAX_LEN)
		return CO_SDO_AC_PDO_LEN;

	size_t offset = 0;
	for (size_t i = 0; i < par->n; i++) {
		co_unsigned32_t map = par->map[i];
		if (!map)
			continue;

		co_unsigned16_t idx = (map >> 16) & 0xffff;
		co_unsigned8_t subidx = (map >> 8) & 0xff;
		co_unsigned8_t len = map & 0xff;

		// Check the PDO length.
		if (offset + len > n * 8)
			return CO_SDO_AC_PDO_LEN;

		// Check whether the sub-object exists and can be mapped into an
		// RPDO (or is a valid dummy entry).
		co_unsigned32_t ac = co_dev_chk_rpdo(dev, idx, subidx);
		if (ac)
			return ac;

		co_sub_t *sub = co_dev_find_sub(dev, idx, subidx);
		if (sub) {
			// Copy the value and download it into the sub-object.
			uint_least8_t tmp[CAN_MAX_LEN] = { 0 };
			bcpyle(tmp, 0, buf, offset, len);
			co_sdo_req_clear(req);
			req->size = (len + 7) / 8;
			req->buf = tmp;
			req->nbyte = req->size;
			ac = co_sub_dn_ind(sub, req);
			if (ac)
				return ac;
		}

		offset += len;
	}

	return 0;
}
#endif // !LELY_NO_CO_RPDO

#if !LELY_NO_CO_TPDO

co_unsigned32_t
co_pdo_up(const struct co_pdo_map_par *par, const co_dev_t *dev,
		struct co_sdo_req *req, uint_least8_t *buf, size_t *pn)
{
	assert(par);
	assert(dev);
	assert(req);

	if (par->n > CO_PDO_NUM_MAPS)
		return CO_SDO_AC_PDO_LEN;

	size_t offset = 0;
	for (size_t i = 0; i < par->n; i++) {
		co_unsigned32_t map = par->map[i];
		if (!map)
			continue;

		co_unsigned16_t idx = (map >> 16) & 0xffff;
		co_unsigned8_t subidx = (map >> 8) & 0xff;
		co_unsigned8_t len = map & 0xff;

		// Check the PDO length.
		if (offset + len > CAN_MAX_LEN * 8)
			return CO_SDO_AC_PDO_LEN;

		// Check whether the sub-object exists and can be mapped into a
		// TPDO.
		co_unsigned32_t ac = co_dev_chk_tpdo(dev, idx, subidx);
		if (ac)
			return ac;

		// Upload the value of the sub-object and copy the value.
		co_sdo_req_clear(req);
		ac = co_sub_up_ind(co_dev_find_sub(dev, idx, subidx), req);
		if (ac)
			return ac;
		if (!co_sdo_req_first(req) || !co_sdo_req_last(req))
			return CO_SDO_AC_PDO_LEN;
		if (buf && pn && offset + len <= *pn * 8)
			bcpyle(buf, offset, req->buf, 0, len);

		offset += len;
	}

	if (pn)
		*pn = (offset + 7) / 8;

	return 0;
}

#if !LELY_NO_CO_MPDO
co_unsigned32_t
co_sam_mpdo_up(const co_dev_t *dev, co_unsigned16_t idx, co_unsigned8_t subidx,
		struct co_sdo_req *req, uint_least8_t buf[4])
{
	assert(dev);
	assert(req);

	// Check whether the sub-object can be mapped into a SAM-MPDO.
	if (!co_dev_chk_sam_mpdo(dev, idx, subidx))
		return CO_SDO_AC_NO_PDO;

	// Check whether the sub-object exists and can be mapped into a TPDO.
	co_unsigned32_t ac = co_dev_chk_tpdo(dev, idx, subidx);
	if (ac)
		return ac;

	// Upload the value of the sub-object.
	co_sdo_req_clear(req);
	ac = co_sub_up_ind(co_dev_find_sub(dev, idx, subidx), req);
	if (ac)
		return ac;
	// Check if the value is complete and fits in the PDO.
	if (req->size > 4 || !co_sdo_req_first(req) || !co_sdo_req_last(req))
		return CO_SDO_AC_PDO_LEN;
	if (buf) {
		// Copy the value and pad it with zeros, if necessary.
		memcpy(buf, req->buf, req->size);
		for (size_t i = req->size; i < 4; i++)
			buf[i] = 0;
	}

	return 0;
}
#endif // !LELY_NO_CO_MPDO

#endif // !LELY_NO_CO_TPDO

static co_unsigned32_t
co_dev_cfg_pdo_comm(const co_dev_t *dev, co_unsigned16_t idx,
		const struct co_pdo_comm_par *par)
{
	assert(dev);
	assert(par);

	co_unsigned32_t ac = 0;

	co_obj_t *obj = co_dev_find_obj(dev, idx);
	if (!obj)
		return CO_SDO_AC_NO_OBJ;

	// Check if all sub-objects are available.
	co_unsigned8_t n = co_obj_get_val_u8(obj, 0x00);
	if (par->n > n)
		return CO_SDO_AC_NO_SUB;

	// Configure the COB-ID.
	if (par->n >= 1) {
		co_sub_t *sub = co_obj_find_sub(obj, 0x01);
		if (!sub)
			return CO_SDO_AC_NO_SUB;
		ac = co_sub_dn_ind_val(sub, CO_DEFTYPE_UNSIGNED32, &par->cobid);
	}

	// Configure the transmission type.
	if (par->n >= 2 && !ac) {
		co_sub_t *sub = co_obj_find_sub(obj, 0x02);
		if (!sub)
			return CO_SDO_AC_NO_SUB;
		ac = co_sub_dn_ind_val(sub, CO_DEFTYPE_UNSIGNED8, &par->trans);
	}

	// Configure the inhibit time.
	if (par->n >= 3 && !ac) {
		co_sub_t *sub = co_obj_find_sub(obj, 0x03);
		if (!sub)
			return CO_SDO_AC_NO_SUB;
		ac = co_sub_dn_ind_val(
				sub, CO_DEFTYPE_UNSIGNED16, &par->inhibit);
	}

	// Configure the event timer.
	if (par->n >= 5 && !ac) {
		co_sub_t *sub = co_obj_find_sub(obj, 0x05);
		if (!sub)
			return CO_SDO_AC_NO_SUB;
		ac = co_sub_dn_ind_val(sub, CO_DEFTYPE_UNSIGNED16, &par->event);
	}

	// Configure the SYNC start value.
	if (par->n >= 6 && !ac) {
		co_sub_t *sub = co_obj_find_sub(obj, 0x06);
		if (!sub)
			return CO_SDO_AC_NO_SUB;
		ac = co_sub_dn_ind_val(sub, CO_DEFTYPE_UNSIGNED8, &par->sync);
	}

	return ac;
}

static co_unsigned32_t
co_dev_cfg_pdo_map(const co_dev_t *dev, co_unsigned16_t idx,
		const struct co_pdo_map_par *par)
{
	assert(dev);
	assert(par);

	co_unsigned32_t ac = 0;

	co_obj_t *obj = co_dev_find_obj(dev, idx);
	if (!obj)
		return CO_SDO_AC_NO_OBJ;

	co_sub_t *sub_00 = co_obj_find_sub(obj, 0x00);
	if (!sub_00)
		return CO_SDO_AC_NO_SUB;

	// Disable mapping by setting subindex 0x00 to zero.
	ac = co_sub_dn_ind_val(
			sub_00, CO_DEFTYPE_UNSIGNED8, &(co_unsigned8_t){ 0 });
	if (ac)
		return ac;

	if (par->n <= CO_PDO_NUM_MAPS) {
		// Copy the mapping parameters.
		for (co_unsigned8_t i = 1; i <= par->n; i++) {
			co_sub_t *sub = co_obj_find_sub(obj, i);
			if (!sub)
				return CO_SDO_AC_NO_SUB;
			ac = co_sub_dn_ind_val(sub, CO_DEFTYPE_UNSIGNED32,
					&par->map[i - 1]);
			if (ac)
				return ac;
		}
	}

	// Enable mapping.
	return co_sub_dn_ind_val(sub_00, CO_DEFTYPE_UNSIGNED8, &par->n);
}

#if !LELY_NO_CO_RPDO && !LELY_NO_CO_MPDO
co_unsigned32_t
co_mpdo_dn(const struct co_pdo_map_par *par, co_dev_t *dev,
		struct co_sdo_req *req, const uint_least8_t *buf, size_t n)
{
	assert(par);
	assert(dev);
	assert(req);
	assert(buf);

	if ((par->n != CO_PDO_MAP_SAM_MPDO && par->n != CO_PDO_MAP_DAM_MPDO)
			|| n != CAN_MAX_LEN)
		return CO_SDO_AC_PDO_LEN;

	co_unsigned8_t id = buf[0];
	co_unsigned16_t idx = ldle_u16(buf + 1);
	co_unsigned8_t subidx = buf[3];
	if (id & 0x80) {
		// Ignore an unexpected DAM-MPDO.
		if (par->n != CO_PDO_MAP_DAM_MPDO)
			return 0;
		// Check if the node-ID matches.
		id &= 0x7f;
		if (id && id != co_dev_get_id(dev))
			return 0;
	} else {
		// Ignore an unexpected SAM-MPDO.
		if (par->n != CO_PDO_MAP_SAM_MPDO)
			return 0;
		// Find the local object index and sub-index in the object
		// dispatching list.
		if (!co_dev_map_sam_mpdo(dev, id, idx, subidx, &idx, &subidx))
			return 0;
	}

	// Check whether the sub-object exists and can be mapped into a PDO (or
	// is a valid dummy entry).
	co_unsigned32_t ac = co_dev_chk_rpdo(dev, idx, subidx);
	if (ac)
		return ac;

	co_sub_t *sub = co_dev_find_sub(dev, idx, subidx);
	if (sub) {
		// Download the value to the sub-object.
		co_sdo_req_clear(req);
		req->size = 4;
		req->buf = buf + 4;
		req->nbyte = req->size;
		ac = co_sub_dn_ind(sub, req);
		if (ac)
			return ac;
	}

	return 0;
}
#endif // !LELY_NO_CO_RPDO && !LELY_NO_CO_MPDO

#endif // !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO
