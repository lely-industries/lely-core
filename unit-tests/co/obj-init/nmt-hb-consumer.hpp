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

#ifndef LELY_UNIT_TEST_NMT_HB_CONSUMER_HPP_
#define LELY_UNIT_TEST_NMT_HB_CONSUMER_HPP_

#include <cassert>
#include <memory>

#include <lely/co/type.h>

#include "holder/obj.hpp"

// Consumer heartbeat time
namespace Obj1016ConsumerHb {
void
Set00HighestSubidxSupported(CoObjTHolder& obj_holder,
                            const co_unsigned8_t max_subidx) {
  assert(co_obj_get_idx(obj_holder.Get()) == 0x1016u);

  obj_holder.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, max_subidx);
}

void
SetNthConsumerHbTime(CoObjTHolder& obj_holder, const co_unsigned8_t subidx,
                     const co_unsigned32_t value) {
  assert(co_obj_get_idx(obj_holder.Get()) == 0x1016u);
  assert(subidx > 0);

  obj_holder.InsertAndSetSub(subidx, CO_DEFTYPE_UNSIGNED32, value);
}

co_unsigned32_t
MakeHbConsumerEntry(const co_unsigned8_t node_id, const co_unsigned16_t ms) {
  return (co_unsigned32_t(node_id) << 16u) | ms;
}
}  // namespace Obj1016ConsumerHb

#endif  // LELY_UNIT_TEST_NMT_HB_CONSUMER_HPP_
