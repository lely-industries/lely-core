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

#ifndef LELY_UNIT_TEST_RPDO_COMM_PAR_HPP_
#define LELY_UNIT_TEST_RPDO_COMM_PAR_HPP_

#include <cassert>
#include <memory>

#include <lely/co/dev.h>
#include <lely/co/type.h>

#include "holder/obj.hpp"

// RPDO communication parameter record (0x1400-0x15ff)
namespace Obj1400RpdoCommPar {
// sub 0x00 - highest sub-index supported
void
Set00HighestSubidxSupported(std::unique_ptr<CoObjTHolder>& obj_holder,
                            const co_unsigned8_t max_subidx) {
  assert(co_obj_get_idx(obj_holder->Get()) >= 0x1400u &&
         co_obj_get_idx(obj_holder->Get()) <= 0x15ffu);
  CHECK(obj_holder != nullptr);

  co_sub_t* const sub = co_obj_find_sub(obj_holder->Get(), 0x00u);
  if (sub == nullptr)
    obj_holder->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, max_subidx);
  else
    co_sub_set_val_u8(sub, max_subidx);
}

// sub 0x01 - COB-ID used by RPDO
void
Set01CobId(std::unique_ptr<CoObjTHolder>& obj_holder,
           const co_unsigned32_t cobid) {
  assert(co_obj_get_idx(obj_holder->Get()) >= 0x1400u &&
         co_obj_get_idx(obj_holder->Get()) <= 0x15ffu);
  CHECK(obj_holder != nullptr);

  co_sub_t* const sub = co_obj_find_sub(obj_holder->Get(), 0x01u);
  if (sub == nullptr)
    obj_holder->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32, cobid);
  else
    co_sub_set_val_u32(sub, cobid);
}

// sub 0x02 - transmission type
void
Set02TransmissionType(std::unique_ptr<CoObjTHolder>& obj_holder,
                      const co_unsigned8_t type) {
  assert(co_obj_get_idx(obj_holder->Get()) >= 0x1400u &&
         co_obj_get_idx(obj_holder->Get()) <= 0x15ffu);
  CHECK(obj_holder != nullptr);

  co_sub_t* const sub = co_obj_find_sub(obj_holder->Get(), 0x02u);
  if (sub == nullptr)
    obj_holder->InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8, type);
  else
    co_sub_set_val_u8(sub, type);
}

// sub 0x03 - inhibit time, in multiples of 100 microseconds
void
Set03InhibitTime(std::unique_ptr<CoObjTHolder>& obj_holder,
                 const co_unsigned16_t inhibit_time) {
  assert(co_obj_get_idx(obj_holder->Get()) >= 0x1400u &&
         co_obj_get_idx(obj_holder->Get()) <= 0x15ffu);
  CHECK(obj_holder != nullptr);

  co_sub_t* const sub = co_obj_find_sub(obj_holder->Get(), 0x03u);
  if (sub == nullptr)
    obj_holder->InsertAndSetSub(0x03u, CO_DEFTYPE_UNSIGNED16, inhibit_time);
  else
    co_sub_set_val_u16(sub, inhibit_time);
}

// sub 0x04 - compatibility entry, reserved and unused
void
Set04CompatibilityEntry(std::unique_ptr<CoObjTHolder>& obj_holder) {
  assert(co_obj_get_idx(obj_holder->Get()) >= 0x1400u &&
         co_obj_get_idx(obj_holder->Get()) <= 0x15ffu);
  CHECK(obj_holder != nullptr);

  co_sub_t* const sub = co_obj_find_sub(obj_holder->Get(), 0x04u);
  if (sub == nullptr)
    obj_holder->InsertAndSetSub(0x04u, CO_DEFTYPE_UNSIGNED8, 0);
}

// sub 0x05 - event-timer, in milliseconds
void
Set05EventTimer(std::unique_ptr<CoObjTHolder>& obj_holder,
                const co_unsigned16_t timer) {
  assert(co_obj_get_idx(obj_holder->Get()) >= 0x1400u &&
         co_obj_get_idx(obj_holder->Get()) <= 0x15ffu);
  CHECK(obj_holder != nullptr);

  co_sub_t* const sub = co_obj_find_sub(obj_holder->Get(), 0x05u);
  if (sub == nullptr)
    obj_holder->InsertAndSetSub(0x05u, CO_DEFTYPE_UNSIGNED16, timer);
  else
    co_sub_set_val_u16(sub, timer);
}

// sub 0x06 - SYNC start value, not used
void
Set06SyncStartValue(std::unique_ptr<CoObjTHolder>& obj_holder,
                    const co_unsigned8_t sync_start) {
  assert(co_obj_get_idx(obj_holder->Get()) >= 0x1400u &&
         co_obj_get_idx(obj_holder->Get()) <= 0x15ffu);
  CHECK(obj_holder != nullptr);

  co_sub_t* const sub = co_obj_find_sub(obj_holder->Get(), 0x06u);
  if (sub == nullptr)
    obj_holder->InsertAndSetSub(0x06u, CO_DEFTYPE_UNSIGNED8, sync_start);
  else
    co_sub_set_val_u8(sub, sync_start);
}

void
SetDefaultValues(std::unique_ptr<CoObjTHolder>& obj_holder) {
  assert(co_obj_get_idx(obj_holder->Get()) >= 0x1400u &&
         co_obj_get_idx(obj_holder->Get()) <= 0x15ffu);
  CHECK(obj_holder != nullptr);

  Set00HighestSubidxSupported(obj_holder, 0x02u);
  Set01CobId(obj_holder, 0);
  Set02TransmissionType(obj_holder, 0xfeu);
}
}  // namespace Obj1400RpdoCommPar

#endif  // LELY_UNIT_TEST_RPDO_COMM_PAR_HPP_
