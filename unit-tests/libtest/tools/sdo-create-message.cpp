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

#include <algorithm>
#include <cassert>
#include <vector>

#include <lely/util/endian.h>

#include "sdo-consts.hpp"
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
    stle_u32(msg.data + 4u, static_cast<uint32_t>(size));
  }
  msg.data[0] = cs;

  return msg;
}

can_msg
SdoCreateMsg::BlkDnSubReq(const uint_least32_t recipient_id,
                          const co_unsigned8_t seqno,
                          const co_unsigned8_t cs_flags,
                          const std::vector<uint_least8_t>& data) {
  assert(data.size() <= 7u);

  can_msg msg = SdoCreateMsg::Default(0, 0, recipient_id);
  msg.data[0] = cs_flags | seqno;
  std::copy(std::begin(data), std::end(data), msg.data + 1u);

  return msg;
}

can_msg
SdoCreateMsg::BlkDnIniRes(const co_unsigned16_t idx,
                          const co_unsigned8_t subidx,
                          const uint_least32_t recipient_id,
                          const co_unsigned8_t cs_flags,
                          const co_unsigned8_t blksize) {
  can_msg msg = SdoCreateMsg::Default(idx, subidx, recipient_id);
  msg.data[0] = CO_SDO_SCS_BLK_DN_RES;
  msg.data[0] |= cs_flags;
  msg.data[4] = blksize;

  return msg;
}

can_msg
SdoCreateMsg::BlkDnEnd(const co_unsigned16_t idx, const co_unsigned8_t subidx,
                       const uint_least32_t recipient_id,
                       const co_unsigned16_t crc,
                       const co_unsigned8_t cs_flags) {
  can_msg msg = SdoCreateMsg::Default(idx, subidx, recipient_id);
  msg.data[0] = CO_SDO_CCS_BLK_DN_REQ;
  msg.data[0] |= CO_SDO_SC_END_BLK;
  msg.data[0] |= cs_flags;
  stle_u16(msg.data + 1u, crc);

  return msg;
}

can_msg
SdoCreateMsg::DnIniReq(const co_unsigned16_t idx, const co_unsigned8_t subidx,
                       const uint_least32_t recipient_id,
                       const uint_least8_t buf[CO_SDO_INI_DATA_SIZE],
                       const uint_least8_t cs_flags) {
  can_msg msg = SdoCreateMsg::Default(idx, subidx, recipient_id);
  msg.data[0] |= CO_SDO_CCS_DN_INI_REQ;
  msg.data[0] |= cs_flags;
  if (buf != nullptr) memcpy(msg.data + 4u, buf, CO_SDO_INI_DATA_SIZE);

  return msg;
}

can_msg
SdoCreateMsg::DnIniRes(const co_unsigned16_t idx, const co_unsigned8_t subidx,
                       const uint_least32_t recipient_id) {
  can_msg msg = SdoCreateMsg::Default(idx, subidx, recipient_id);
  msg.data[0] = CO_SDO_SCS_DN_INI_RES;

  return msg;
}

can_msg
SdoCreateMsg::DnSegReq(const co_unsigned16_t idx, const co_unsigned8_t subidx,
                       const uint_least32_t recipient_id,
                       const uint_least8_t buf[], const uint8_t size,
                       const uint_least8_t cs_flags) {
  assert(size <= 7u);

  can_msg msg = SdoCreateMsg::Default(idx, subidx, recipient_id);
  msg.data[0] = CO_SDO_CCS_DN_SEG_REQ | CO_SDO_SEG_SIZE_SET(size) | cs_flags;
  memcpy(msg.data + 1u, buf, size);

  return msg;
}

can_msg
SdoCreateMsg::DnSegRes(const uint_least32_t recipient_id,
                       const co_unsigned8_t cs_flags) {
  can_msg msg = SdoCreateMsg::Default(0u, 0u, recipient_id);
  msg.data[0] = CO_SDO_SCS_DN_SEG_RES | cs_flags;

  return msg;
}

can_msg
SdoCreateMsg::UpIniReq(const co_unsigned16_t idx, const co_unsigned8_t subidx,
                       const uint_least32_t recipient_id) {
  can_msg msg = SdoCreateMsg::Default(idx, subidx, recipient_id);
  msg.data[0] = CO_SDO_CCS_UP_INI_REQ;

  return msg;
}

can_msg
SdoCreateMsg::UpIniRes(const co_unsigned16_t idx, const co_unsigned8_t subidx,
                       const uint_least32_t recipient_id,
                       const uint_least8_t cs_flags,
                       const std::vector<uint_least8_t>& data) {
  assert(data.size() <= 4u);

  can_msg msg = SdoCreateMsg::Default(idx, subidx, recipient_id);
  msg.data[0] = CO_SDO_SCS_UP_INI_RES | cs_flags;
  std::copy(std::begin(data), std::end(data), msg.data + 4u);

  return msg;
}

can_msg
SdoCreateMsg::UpIniResWithSize(const co_unsigned16_t idx,
                               const co_unsigned8_t subidx,
                               const uint_least32_t recipient_id,
                               const size_t size) {
  if (size == 0) return UpIniRes(idx, subidx, recipient_id);

  std::vector<uint_least8_t> buf(4u);
  stle_u32(buf.data(), static_cast<co_unsigned32_t>(size));

  return UpIniRes(idx, subidx, recipient_id, CO_SDO_INI_SIZE_IND, buf);
}

can_msg
SdoCreateMsg::BlkUpSegReq(const uint_least32_t recipient_id,
                          const uint_least8_t seqno,
                          const std::vector<uint_least8_t>& data,
                          const co_unsigned8_t cs_flags) {
  assert(data.size() <= CO_SDO_SEG_MAX_DATA_SIZE);

  can_msg msg = SdoCreateMsg::Default(0, 0, recipient_id);
  msg.data[0] = seqno;
  msg.data[0] |= cs_flags;
  std::copy(data.begin(), data.end(), msg.data + 1u);

  return msg;
}

can_msg
SdoCreateMsg::UpSegRes(const uint_least32_t recipient_id,
                       const std::vector<uint_least8_t>& data,
                       const co_unsigned8_t cs_flags) {
  assert(data.size() <= CO_SDO_SEG_MAX_DATA_SIZE);

  can_msg msg = SdoCreateMsg::Default(0u, 0u, recipient_id);
  msg.data[0] = CO_SDO_SCS_UP_SEG_RES;
  msg.data[0] |= cs_flags;
  std::copy(data.begin(), data.end(), msg.data + 1u);

  return msg;
}

can_msg
SdoCreateMsg::BlkUpIniReq(const co_unsigned16_t idx,
                          const co_unsigned8_t subidx,
                          const uint_least32_t recipient_id,
                          const co_unsigned8_t blksize) {
  can_msg msg = SdoCreateMsg::Default(idx, subidx, recipient_id);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ;
  msg.data[4] = blksize;

  return msg;
}

can_msg
SdoCreateMsg::BlkUpReq(const uint_least32_t recipient_id,
                       const co_unsigned8_t cs_flags) {
  can_msg msg = SdoCreateMsg::Default(0, 0, recipient_id);
  msg.data[0] = CO_SDO_CCS_BLK_UP_REQ | cs_flags;

  return msg;
}

can_msg
SdoCreateMsg::BlkUpRes(const uint_least32_t recipient_id,
                       const co_unsigned8_t size,
                       const co_unsigned8_t cs_flags) {
  auto msg = SdoCreateMsg::Default(0, 0, recipient_id);
  msg.data[0] = CO_SDO_SCS_BLK_UP_RES;
  msg.data[0] |= cs_flags;
  msg.data[0] |= CO_SDO_BLK_SIZE_SET(size);

  return msg;
}

can_msg
SdoCreateMsg::BlkUpIniRes(const co_unsigned16_t idx,
                          const co_unsigned8_t subidx,
                          const uint_least32_t recipient_id,
                          const co_unsigned32_t size) {
  can_msg msg = SdoCreateMsg::Default(idx, subidx, recipient_id);
  uint_least8_t cs = CO_SDO_SCS_BLK_UP_RES | CO_SDO_SC_INI_BLK;
  if (size > 0) {
    cs |= CO_SDO_BLK_SIZE_IND;
    stle_u32(msg.data + 4u, size);
  }
  msg.data[0] = cs;

  return msg;
}
