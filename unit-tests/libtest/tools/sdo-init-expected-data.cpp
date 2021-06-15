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

#include <vector>

#include <lely/util/endian.h>

#include "sdo-defines.hpp"

#include "sdo-init-expected-data.hpp"

std::vector<uint_least8_t>
SdoInitExpectedData::Empty(const uint_least8_t cs, const co_unsigned16_t idx,
                           const co_unsigned8_t subidx) {
  std::vector<uint_least8_t> buffer(CO_SDO_MSG_SIZE, 0);
  buffer[0] = cs;
  stle_u16(&buffer[1u], idx);
  buffer[3] = subidx;

  return buffer;
}

std::vector<uint_least8_t>
SdoInitExpectedData::U16(const uint_least8_t cs, const co_unsigned16_t idx,
                         const co_unsigned8_t subidx,
                         const co_unsigned16_t val) {
  std::vector<uint_least8_t> buffer = Empty(cs, idx, subidx);
  stle_u16(&buffer[4u], val);

  return buffer;
}

std::vector<uint_least8_t>
SdoInitExpectedData::U32(const uint_least8_t cs, const co_unsigned16_t idx,
                         const co_unsigned8_t subidx,
                         const co_unsigned32_t val) {
  std::vector<uint_least8_t> buffer = Empty(cs, idx, subidx);
  stle_u32(&buffer[4u], val);

  return buffer;
}
