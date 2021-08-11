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

#ifndef LELY_UNIT_TEST_NMT_SLAVE_ASSIGNMENT_HPP_
#define LELY_UNIT_TEST_NMT_SLAVE_ASSIGNMENT_HPP_

#include "obj-init/obj-init.hpp"

#include <lely/co/dev.h>

/// 0x1f81: NMT Slave assignment
struct Obj1f81NmtSlaveAssignment : ObjInitT<0x1f81u> {
  struct Sub00HighestSubidxSupported : SubT<0x00u, CO_DEFTYPE_UNSIGNED8> {};
  struct SubNthSlaveEntry : SubT<0x01u, CO_DEFTYPE_UNSIGNED32, 0, 0x01> {};

  static const co_unsigned32_t ASSIGNMENT_BIT = 0x01u;
  static const co_unsigned32_t BOOTED_BIT = 0x04u;
  static const co_unsigned32_t MANDATORY_BIT = 0x08u;
  static const co_unsigned32_t KEEP_ALIVE_BIT = 0x10u;
};

#endif  // LELY_UNIT_TEST_NMT_SLAVE_ASSIGNMENT_HPP_
