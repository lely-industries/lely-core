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

#ifndef LELY_UNIT_TEST_SDO_INIT_EXPECTED_DATA_
#define LELY_UNIT_TEST_SDO_INIT_EXPECTED_DATA_

#include <cstdint>
#include <vector>

#include <lely/co/type.h>

namespace SdoInitExpectedData {
std::vector<uint_least8_t> Empty(uint_least8_t cs, co_unsigned16_t idx = 0,
                                 co_unsigned8_t subidx = 0);
std::vector<uint_least8_t> U16(uint_least8_t cs, co_unsigned16_t idx = 0,
                               co_unsigned8_t subidx = 0,
                               co_unsigned16_t val = 0);
std::vector<uint_least8_t> U32(uint_least8_t cs, co_unsigned16_t idx = 0,
                               co_unsigned8_t subidx = 0,
                               co_unsigned32_t val = 0);
std::vector<uint_least8_t> Segment(uint_least8_t seqno,
                                   const std::vector<uint_least8_t>& data);
}  // namespace SdoInitExpectedData

#endif  // LELY_UNIT_TEST_SDO_INIT_EXPECTED_DATA_
