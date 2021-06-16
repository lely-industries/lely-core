/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2021 N7 Space Sp. z o.o.
 *
 * Unit Test Suite was developed under a programme of,
 * and funded by, the European Space Agency.
 *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cassert>

#include <lely/util/endian.h>

#include "sdo-defines.hpp"
#include "sdo-create-message.hpp"

can_msg
SdoCreateMsg::Abort(const co_unsigned16_t idx, const co_unsigned8_t subidx,
                    const uint_least32_t recipient_id,
                    const co_unsigned32_t ac) {
  can_msg msg = SdoCreateMsg::Default(idx, subidx, recipient_id);
  msg.data[0] = CO_SDO_CS_ABORT;
  stle_u32(msg.data + 4u, ac);

  return msg;
}

can_msg
SdoCreateMsg::Default(const co_unsigned16_t idx, const co_unsigned8_t subidx,
                      const uint_least32_t recipient_id) {
  can_msg msg = CAN_MSG_INIT;
  msg.id = recipient_id;
  stle_u16(msg.data + 1u, idx);
  msg.data[3] = subidx;
  msg.len = CO_SDO_MSG_SIZE;

  return msg;
}

can_msg
SdoCreateMsg::BlkDnIniReq(const co_unsigned16_t idx,
                          const co_unsigned8_t subidx,
                          const uint_least32_t recipient_id,
                          const co_unsigned8_t cs_flags, const size_t size) {
  can_msg msg = SdoCreateMsg::Default(idx, subidx, recipient_id);
  co_unsigned8_t cs = CO_SDO_CCS_BLK_DN_REQ | cs_flags;
  if (size > 0) {
    cs |= CO_SDO_BLK_SIZE_IND;
    stle_u32(msg.data + 4u, size);
  }
  msg.data[0] = cs;

  return msg;
}

can_msg
SdoCreateMsg::BlkDnSubReq(const co_unsigned16_t idx,
                          const co_unsigned8_t subidx,
                          const uint_least32_t recipient_id,
                          const co_unsigned8_t seqno,
                          const co_unsigned8_t last) {
  can_msg msg = SdoCreateMsg::Default(idx, subidx, recipient_id);
  msg.data[0] = last | seqno;

  return msg;
}

can_msg
SdoCreateMsg::BlkDnEnd(const co_unsigned16_t idx, const co_unsigned8_t subidx,
                       const uint_least32_t recipient_id,
                       const co_unsigned16_t crc,
                       const co_unsigned8_t cs_flags) {
  can_msg msg = SdoCreateMsg::Default(idx, subidx, recipient_id);
  msg.data[0] = CO_SDO_CCS_BLK_DN_REQ | CO_SDO_SC_END_BLK | cs_flags;
  stle_u16(msg.data + 1u, crc);

  return msg;
}

can_msg
SdoCreateMsg::DnIniReq(const co_unsigned16_t idx, const co_unsigned8_t subidx,
                       const uint_least32_t recipient_id,
                       uint_least8_t buffer[CO_SDO_INI_DATA_SIZE],
                       uint_least8_t cs_flags) {
  can_msg msg = SdoCreateMsg::Default(idx, subidx, recipient_id);
  msg.data[0] |= CO_SDO_CCS_DN_INI_REQ;
  msg.data[0] |= cs_flags;
  if (buffer != nullptr) memcpy(msg.data + 4u, buffer, CO_SDO_INI_DATA_SIZE);

  return msg;
}

can_msg
SdoCreateMsg::DnSeg(const co_unsigned16_t idx, const co_unsigned8_t subidx,
                    const uint_least32_t recipient_id,
                    const uint_least8_t buf[], const size_t size,
                    const uint_least8_t cs_flags) {
  assert(size <= 7u);

  can_msg msg = SdoCreateMsg::Default(idx, subidx, recipient_id);
  msg.data[0] = CO_SDO_CCS_DN_SEG_REQ | CO_SDO_SEG_SIZE_SET(size) | cs_flags;
  memcpy(msg.data + 1u, buf, size);

  return msg;
}

can_msg
SdoCreateMsg::UpIniReq(const co_unsigned16_t idx, const co_unsigned8_t subidx,
                       const uint_least32_t recipient_id) {
  can_msg msg = SdoCreateMsg::Default(idx, subidx, recipient_id);
  msg.data[0] = CO_SDO_CCS_UP_INI_REQ;
  stle_u16(msg.data + 1u, idx);
  msg.data[3] = subidx;

  return msg;
}

can_msg
SdoCreateMsg::BlkUpIniReq(const co_unsigned16_t idx,
                          const co_unsigned8_t subidx,
                          const uint_least32_t recipient_id,
                          const co_unsigned8_t blksize) {
  can_msg msg = SdoCreateMsg::Default(idx, subidx, recipient_id);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ;
  msg.data[3] = subidx;
  msg.data[4] = blksize;

  return msg;
}
