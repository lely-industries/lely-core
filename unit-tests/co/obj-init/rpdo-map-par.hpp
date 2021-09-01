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

#ifndef LELY_UNIT_TEST_RPDO_MAP_PAR_HPP_
#define LELY_UNIT_TEST_RPDO_MAP_PAR_HPP_

#include "obj-init/obj-init.hpp"
#include "holder/obj.hpp"

/// 0x1600-0x17ff: RPDO mapping parameter
struct Obj1600RpdoMapPar : ObjInitT<0x1600u, 0x1600u, 0x17ffu> {
  struct Sub00NumOfMappedObjs : SubT<0x00, CO_DEFTYPE_UNSIGNED8> {};
  struct SubNthAppObject : SubT<0x01, CO_DEFTYPE_UNSIGNED32, 0, 0x01> {};

  static SubNthAppObject::sub_type
  MakeMappingParam(const co_unsigned16_t idx, const co_unsigned8_t subidx,
                   const co_unsigned8_t bits_len) {
    return (co_unsigned32_t{idx} << 16u) | (co_unsigned32_t{subidx} << 8u) |
           bits_len;
  }
};

#endif  // LELY_UNIT_TEST_RPDO_MAP_PAR_HPP_
