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

#include "obj-init/obj-init.hpp"
#include "holder/obj.hpp"

// 0x1400-0x15ff: RPDO communication parameter
struct Obj1400RpdoCommPar : ObjInitT<0x1400u, 0x1400u, 0x15ffu> {
  struct Sub00HighestSubidxSupported
      : SubT<0x00u, CO_DEFTYPE_UNSIGNED8, 0x02u> {};
  struct Sub01CobId : SubT<0x01u, CO_DEFTYPE_UNSIGNED32> {};
  struct Sub02TransmissionType : SubT<0x02u, CO_DEFTYPE_UNSIGNED8> {};
  struct Sub03InhibitTime : SubT<0x03u, CO_DEFTYPE_UNSIGNED16> {};
  struct Sub04Reserved : SubT<0x04u, CO_DEFTYPE_UNSIGNED8, 0> {};
  struct Sub05EventTimer : SubT<0x05u, CO_DEFTYPE_UNSIGNED16> {};
  struct Sub06SyncStartValue : SubT<0x04u, CO_DEFTYPE_UNSIGNED8> {};
};

#endif  // LELY_UNIT_TEST_RPDO_COMM_PAR_HPP_
