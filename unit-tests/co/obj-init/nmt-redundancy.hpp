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

#include <lely/co/type.h>
#include <lib/co/nmt_rdn.h>

#include "obj-init/obj-init.hpp"

// 0x2000 (default): Redundnacy object
struct ObjNmtRedundancy : ObjInitT<CO_NMT_RDN_REDUNDANCY_OBJ_IDX> {
  struct Sub00HighestSubidxSupported
      : ObjInitT::SubT<0x00u, CO_DEFTYPE_UNSIGNED8, 0x04u> {};
  struct Sub01Bdefault : ObjInitT::SubT<0x01u, CO_DEFTYPE_UNSIGNED8> {};
  struct Sub02Ttoggle : ObjInitT::SubT<0x02u, CO_DEFTYPE_UNSIGNED8> {};
  struct Sub03Ntoggle : ObjInitT::SubT<0x03u, CO_DEFTYPE_UNSIGNED8> {};
  struct Sub04Ctoggle : ObjInitT::SubT<0x04u, CO_DEFTYPE_UNSIGNED8> {};
};

#endif  // LELY_UNIT_TEST_NMT_REDUNDANCY_HPP_
