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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <CppUTest/TestHarness.h>

#include "co-csdo-dn-con.hpp"

co_csdo_t* CoCsdoDnCon::sdo = nullptr;
co_unsigned16_t CoCsdoDnCon::idx = 0;
co_unsigned8_t CoCsdoDnCon::subidx = 0;
co_unsigned32_t CoCsdoDnCon::ac = 0;
void* CoCsdoDnCon::data = nullptr;
size_t CoCsdoDnCon::num_called = 0;

void
CoCsdoDnCon::Func(co_csdo_t* const sdo_, const co_unsigned16_t idx_,
                  const co_unsigned8_t subidx_, const co_unsigned32_t ac_,
                  void* const data_) {
  sdo = sdo_;
  idx = idx_;
  subidx = subidx_;
  ac = ac_;
  data = data_;
  num_called++;
}

void
CoCsdoDnCon::Clear() {
  sdo = nullptr;
  idx = 0;
  subidx = 0;
  ac = 0;
  data = nullptr;

  num_called = 0;
}

void
CoCsdoDnCon::Check(const co_csdo_t* const sdo_, const co_unsigned16_t idx_,
                   const co_unsigned8_t subidx_, const co_unsigned32_t ac_,
                   const void* const data_) {
  CHECK_COMPARE(num_called, >, 0);
  POINTERS_EQUAL(sdo_, sdo);
  CHECK_EQUAL(idx_, idx);
  CHECK_EQUAL(subidx_, subidx);
  CHECK_EQUAL(ac_, ac);
  POINTERS_EQUAL(data_, data);
}
