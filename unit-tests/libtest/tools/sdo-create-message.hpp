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

#ifndef LELY_UNIT_TEST_SDO_CREATE_MESSAGE_HPP_
#define LELY_UNIT_TEST_SDO_CREATE_MESSAGE_HPP_

#include <vector>

#include <lely/can/msg.h>
#include <lely/co/type.h>

#include "sdo-consts.hpp"

namespace SdoCreateMsg {
can_msg Abort(co_unsigned16_t idx, co_unsigned8_t subidx,
              uint_least32_t recipient_id, co_unsigned32_t ac = 0);
can_msg Default(co_unsigned16_t idx, co_unsigned8_t subidx,
                uint_least32_t recipient_id);
// block download initiate request
can_msg BlkDnIniReq(co_unsigned16_t idx, co_unsigned8_t subidx,
                    uint_least32_t recipient_id, co_unsigned8_t cs_flags = 0,
                    size_t size = 0);
// block download sub-block request
can_msg BlkDnSubReq(co_unsigned16_t idx, co_unsigned8_t subidx,
                    uint_least32_t recipient_id, co_unsigned8_t seqno,
                    co_unsigned8_t last = 0);
// block download sub-block response
can_msg BlkDnSubRes(co_unsigned16_t idx, co_unsigned8_t subidx,
                    uint_least32_t recipient_id, co_unsigned8_t seqno,
                    co_unsigned8_t cs_flags = 0, co_unsigned32_t blksize = 0);
// block download end
can_msg BlkDnEnd(co_unsigned16_t idx, co_unsigned8_t subidx,
                 uint_least32_t recipient_id, co_unsigned16_t crc,
                 co_unsigned8_t cs_flags = 0);
// download initiate request
can_msg DnIniReq(co_unsigned16_t idx, co_unsigned8_t subidx,
                 uint_least32_t recipient_id, const uint_least8_t buf[],
                 uint_least8_t cs_flags = 0);
// download segment request
can_msg DnSegReq(co_unsigned16_t idx, co_unsigned8_t subidx,
                 uint_least32_t recipient_id, const uint_least8_t buf[],
                 uint8_t size, uint_least8_t cs_flags = 0);
// upload initiate request
can_msg UpIniReq(co_unsigned16_t idx, co_unsigned8_t subidx,
                 uint_least32_t recipient_id);
// upload initiate response - generic
can_msg UpIniRes(co_unsigned16_t idx, co_unsigned8_t subidx,
                 uint_least32_t recipient_id, uint_least8_t cs_flags = 0,
                 const std::vector<uint_least8_t>& data = {});
// upload initiate response - with optional size indication
can_msg UpIniResWithSize(co_unsigned16_t idx, co_unsigned8_t subidx,
                         uint_least32_t recipient_id, size_t size);
// upload segment request
can_msg UpSeg(uint_least32_t recipient_id, uint_least8_t seqno,
              const std::vector<uint_least8_t>& data,
              co_unsigned8_t cs_flags = 0);
// upload segment response
can_msg UpSegRes(uint_least32_t recipient_id,
                 const std::vector<uint_least8_t>& data,
                 co_unsigned8_t cs_flags = 0);
// block upload initiate request
can_msg BlkUpIniReq(co_unsigned16_t idx, co_unsigned8_t subidx,
                    uint_least32_t recipient_id,
                    co_unsigned8_t blksize = CO_SDO_MAX_SEQNO);
// block upload request
can_msg BlkUpReq(uint_least32_t recipient_id, co_unsigned8_t cs_flags = 0);
// block upload response
can_msg BlkUpRes(uint_least32_t recipient_id, co_unsigned8_t size = 0,
                 co_unsigned8_t cs_flags = 0);
// block upload initiate response
can_msg BlkUpIniRes(co_unsigned16_t idx, co_unsigned8_t subidx,
                    uint_least32_t recipient_id, co_unsigned32_t size = 0);
}  // namespace SdoCreateMsg

#endif  // LELY_UNIT_TEST_SDO_CREATE_MESSAGE_HPP_
