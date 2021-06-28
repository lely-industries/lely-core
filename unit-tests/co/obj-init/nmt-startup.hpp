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

#ifndef LELY_UNIT_TEST_NMT_CONFIG_HPP_
#define LELY_UNIT_TEST_NMT_CONFIG_HPP_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cassert>
#include <memory>

#include <lely/co/type.h>

#include "holder/obj.hpp"

namespace Obj1f80NmtStartup {
static const co_unsigned32_t MASTER_BIT = 0x01u;

void
SetVal(CoObjTHolder& obj_holder, const co_unsigned32_t startup) {
  assert(co_obj_get_idx(obj_holder.Get()) == 0x1f80u);

  obj_holder.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32, startup);
}
}  // namespace Obj1f80NmtStartup

#endif  // LELY_UNIT_TEST_NMT_CONFIG_HPP_
