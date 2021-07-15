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

#include "co-csdo-up-con.hpp"

co_csdo_t* CoCsdoUpCon::sdo = nullptr;
co_unsigned16_t CoCsdoUpCon::idx = 0;
co_unsigned8_t CoCsdoUpCon::subidx = 0;
co_unsigned32_t CoCsdoUpCon::ac = 0;
const void* CoCsdoUpCon::ptr = nullptr;
size_t CoCsdoUpCon::n = 0;
void* CoCsdoUpCon::data = nullptr;
size_t CoCsdoUpCon::num_called = 0;
uint_least8_t CoCsdoUpCon::buf[CoCsdoUpCon::BUFSIZE] = {0};

void
CoCsdoUpCon::func(co_csdo_t* const sdo_, const co_unsigned16_t idx_,
                  const co_unsigned8_t subidx_, const co_unsigned32_t ac_,
                  const void* const ptr_, const size_t n_, void* const data_) {
  sdo = sdo_;
  idx = idx_;
  subidx = subidx_;
  ac = ac_;
  ptr = ptr_;
  n = n_;
  data = data_;
  if (ptr_ != nullptr) memcpy(buf, static_cast<const uint_least8_t*>(ptr_), n_);
  num_called++;
}

void
CoCsdoUpCon::Clear() {
  sdo = nullptr;
  idx = 0;
  subidx = 0;
  ac = 0;
  ptr = nullptr;
  n = 0;
  data = nullptr;
  memset(buf, 0, BUFSIZE);

  num_called = 0;
}

void
CoCsdoUpCon::Check(const co_csdo_t* const sdo_, const co_unsigned16_t idx_,
                   const co_unsigned8_t subidx_, const co_unsigned32_t ac_,
                   const void* const ptr_, const size_t n_,
                   const void* const data_) {
  POINTERS_EQUAL(sdo_, sdo);
  CHECK_EQUAL(idx_, idx);
  CHECK_EQUAL(subidx_, subidx);
  CHECK_EQUAL(ac_, ac);
  POINTERS_EQUAL(ptr_, ptr);
  CHECK_EQUAL(n_, n);
  POINTERS_EQUAL(data_, data);
}

void
CoCsdoUpCon::CheckNonempty(const co_csdo_t* sdo_, const co_unsigned16_t idx_,
                           const co_unsigned8_t subidx_,
                           const co_unsigned32_t ac_, const size_t n_,
                           const void* data_) {
  POINTERS_EQUAL(sdo_, sdo);
  CHECK_EQUAL(idx_, idx);
  CHECK_EQUAL(subidx_, subidx);
  CHECK_EQUAL(ac_, ac);
  CHECK(ptr != nullptr);
  CHECK_EQUAL(n_, n);
  POINTERS_EQUAL(data_, data);
}
