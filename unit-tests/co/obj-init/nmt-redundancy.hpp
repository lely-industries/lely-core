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

#ifndef LELY_UNIT_TEST_NMT_REDUNDANCY_HPP_
#define LELY_UNIT_TEST_NMT_REDUNDANCY_HPP_

#include <memory>
#include <cassert>

#include <lib/co/nmt_rdn.h>

#include "holder/obj.hpp"

namespace ObjNmtRedundancy {
void
InsertAndSetAllSubValues(CoObjTHolder& obj_holder,
                         const co_unsigned8_t bdefault,
                         const co_unsigned8_t ttoggle,
                         const co_unsigned8_t ntoggle,
                         const co_unsigned8_t ctoggle) {
  assert(co_obj_get_idx(obj_holder.Get()) == CO_NMT_RDN_REDUNDANCY_OBJ_IDX);

  obj_holder.InsertAndSetSub(0x00, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t(CO_NMT_REDUNDANCY_OBJ_MAX_IDX));
  obj_holder.InsertAndSetSub(CO_NMT_RDN_BDEFAULT_SUBIDX, CO_DEFTYPE_UNSIGNED8,
                             bdefault);
  obj_holder.InsertAndSetSub(CO_NMT_RDN_TTOGGLE_SUBIDX, CO_DEFTYPE_UNSIGNED8,
                             ttoggle);
  obj_holder.InsertAndSetSub(CO_NMT_RDN_NTOGGLE_SUBIDX, CO_DEFTYPE_UNSIGNED8,
                             ntoggle);
  obj_holder.InsertAndSetSub(CO_NMT_RDN_CTOGGLE_SUBIDX, CO_DEFTYPE_UNSIGNED8,
                             ctoggle);
}
}  // namespace ObjNmtRedundancy

#endif  // LELY_UNIT_TEST_NMT_REDUNDANCY_HPP_
