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
#include <lely/can/msg.h>
#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/pdo.h>
#include <lely/co/sdo.h>

#include <assert.h>

#ifndef LELY_NO_CO_RPDO
LELY_CO_EXPORT co_unsigned32_t
co_pdo_read(const struct co_pdo_map_par *par, co_dev_t *dev,
		struct co_sdo_req *req, const uint8_t *buf, size_t n)
{
	assert(par);
	assert(dev);
	assert(req);
	assert(buf);

	co_unsigned32_t ac = 0;

	const uint8_t *bp = buf;
	const uint8_t *end = buf + n;

	for (size_t i = 1; i <= MIN(par->n, 0x40u); i++) {
		co_unsigned32_t map = par->map[i - 1];
		co_unsigned16_t idx = (map >> 16) & 0xffff;
		co_unsigned8_t subidx = (map >> 8) & 0xff;
		co_unsigned8_t len = map & 0xff;

		// Check whether the sub-object exists and can be mapped into a
		// PDO (or is a valid dummy entry).
		size_t size = 0;
		ac = co_dev_check_rpdo(dev, idx, subidx, &size);
		if (__unlikely(ac))
			return ac;

		// Check the PDO length.
		if (__unlikely(len != size * 8 || bp + len / 8 > end))
			return CO_SDO_AC_PDO_LEN;

		co_sub_t *sub = co_dev_find_sub(dev, idx, subidx);
		if (sub) {
			// Download the value from the sub-object.
			co_sdo_req_clear(req);
			req->size = len / 8;
			req->buf = bp;
			req->nbyte = req->size;
			ac = co_sub_dn_ind(sub, req);
			if (__unlikely(ac))
				return ac;
		}

		bp += len / 8;
	}

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

	uint8_t *bp = buf;
	uint8_t *end = buf + (buf && pn ? *pn : 0);

	for (size_t i = 1; i <= MIN(par->n, 0x40u); i++) {
		co_unsigned32_t map = par->map[i - 1];
		co_unsigned16_t idx = (map >> 16) & 0xffff;
		co_unsigned8_t subidx = (map >> 8) & 0xff;
		co_unsigned8_t len = map & 0xff;

		// Check whether the sub-object exists and can be mapped into a
		// PDO.
		size_t size = 0;
		ac = co_dev_check_tpdo(dev, idx, subidx, &size);
		if (__unlikely(ac))
			return ac;

		// Check the PDO length.
		if (__unlikely(len != size * 8
				|| bp + len / 8 > buf + CAN_MAX_LEN))
			return CO_SDO_AC_PDO_LEN;

		// Upload the value of the sub-object.
		co_sdo_req_clear(req);
		ac = co_sub_up_ind(co_dev_find_sub(dev, idx, subidx), req);
		if (__unlikely(ac))
			return ac;

		// Check the PDO length and copy the value.
		if (__unlikely(len != req->size * 8 || !co_sdo_req_first(req)
				|| !co_sdo_req_last(req)))
			return CO_SDO_AC_PDO_LEN;
		if (buf && pn && bp < end)
			memcpy(bp, req->buf, MIN(len / 8, end - bp));
		bp += len / 8;
	}

	if (pn)
		*pn = bp - buf;

	return ac;
}
#endif // !LELY_NO_CO_TPDO

