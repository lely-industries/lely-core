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

#include <memory>

#include <lely/co/dev.h>
#include <lely/co/type.h>

#include "holder/obj.hpp"

// RPDO mapping parameter record (0x1600-0x17ff)
namespace Obj1600RpdoMapPar {
// sub 0x00 - number of mapped application objects in RPDO
void
Set00NumOfMappedAppObjs(std::unique_ptr<CoObjTHolder>& obj_holder,
                        const co_unsigned8_t number) {
  assert(co_obj_get_idx(obj_holder->Get()) >= 0x1600u &&
         co_obj_get_idx(obj_holder->Get()) <= 0x17ffu);
  CHECK(obj_holder != nullptr);

  co_sub_t* const sub = co_obj_find_sub(obj_holder->Get(), 0x00u);
  if (sub == nullptr)
    obj_holder->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, number);
  else
    co_sub_set_val_u8(sub, number);
}

// sub 0xNN - N-th application object; N must be > 0x00
void
SetNthAppObject(std::unique_ptr<CoObjTHolder>& obj_holder,
                const co_unsigned8_t subidx, const co_unsigned32_t mapping) {
  assert(co_obj_get_idx(obj_holder->Get()) >= 0x1600u &&
         co_obj_get_idx(obj_holder->Get()) <= 0x17ffu);
  assert(subidx > 0x00u);
  assert(subidx <= 0x40u);
  CHECK(obj_holder != nullptr);

  co_sub_t* const sub = co_obj_find_sub(obj_holder->Get(), subidx);
  if (sub == nullptr)
    obj_holder->InsertAndSetSub(subidx, CO_DEFTYPE_UNSIGNED32, mapping);
  else
    co_sub_set_val_u32(sub, mapping);
}

void
SetDefaultValues(std::unique_ptr<CoObjTHolder>& obj_holder) {
  assert(co_obj_get_idx(obj_holder->Get()) >= 0x1600u &&
         co_obj_get_idx(obj_holder->Get()) <= 0x17ffu);
  CHECK(obj_holder != nullptr);

  Set00NumOfMappedAppObjs(obj_holder, 1u);
  SetNthAppObject(obj_holder, 0x01u, 0x00000000u);
}
}  // namespace Obj1600RpdoMapPar

#endif  // LELY_UNIT_TEST_RPDO_MAP_PAR_HPP_
