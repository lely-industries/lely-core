/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2020 N7 Space Sp. z o.o.
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

#include <cassert>
#include <functional>

#include <CppUTest/TestHarness.h>

#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/detail/obj.h>
#include <lely/util/diag.h>

#include "lely-unit-test.hpp"

co_csdo_t* CoCsdoDnCon::sdo = nullptr;
co_unsigned16_t CoCsdoDnCon::idx = 0;
co_unsigned8_t CoCsdoDnCon::subidx = 0;
co_unsigned32_t CoCsdoDnCon::ac = 0;
void* CoCsdoDnCon::data = nullptr;
unsigned int CoCsdoDnCon::num_called = 0;

int CanSend::ret = 0;
void* CanSend::data = nullptr;
unsigned int CanSend::num_called = 0;
can_msg CanSend::msg = CAN_MSG_INIT;
can_msg* CanSend::msg_buf = &CanSend::msg;
size_t CanSend::buf_size = 1u;

static void CheckSubDnInd(
    const co_dev_t* const dev, const co_unsigned16_t idx,
    std::function<void(co_sub_dn_ind_t*, void* data)> pred);

void
CoCsdoDnCon::func(co_csdo_t* sdo_, co_unsigned16_t idx_, co_unsigned8_t subidx_,
                  co_unsigned32_t ac_, void* data_) {
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
CoCsdoDnCon::Check(const co_csdo_t* sdo_, const co_unsigned16_t idx_,
                   const co_unsigned8_t subidx_, const co_unsigned32_t ac_,
                   const void* data_) {
  POINTERS_EQUAL(sdo_, sdo);
  CHECK_EQUAL(idx_, idx);
  CHECK_EQUAL(subidx_, subidx);
  CHECK_EQUAL(ac_, ac);
  POINTERS_EQUAL(data_, data);
}

int
CanSend::func(const can_msg* msg_, void* data_) {
  assert(msg_);
  assert(num_called < buf_size);

  msg = *msg_;
  data = data_;

  if (num_called < buf_size) {
    msg_buf[num_called] = *msg_;
  }
  num_called++;

  return ret;
}

void
CanSend::Clear() {
  msg = CAN_MSG_INIT;
  data = nullptr;

  ret = 0;
  num_called = 0;

  buf_size = 1u;
  msg_buf = &msg;
}

void
LelyUnitTest::DisableDiagnosticMessages() {
#if LELY_NO_DIAG
  // enforce coverage in NO_DIAG mode
  diag(DIAG_DEBUG, 0, "Message suppressed");
  diag_if(DIAG_DEBUG, 0, nullptr, "Message suppressed");
#else
  diag_set_handler(nullptr, nullptr);
  diag_at_set_handler(nullptr, nullptr);
#endif
}

static void
CheckSubDnInd(const co_dev_t* const dev, const co_unsigned16_t idx,
              std::function<void(co_sub_dn_ind_t*, void* data)> pred) {
  co_sub_t* const sub = co_dev_find_sub(dev, idx, 0x00u);
  CHECK(sub != nullptr);

  co_sub_dn_ind_t* ind = nullptr;
  void* data = nullptr;

  co_sub_get_dn_ind(sub, &ind, &data);

  pred(ind, data);
}

void
LelyUnitTest::CheckSubDnIndIsSet(const co_dev_t* const dev,
                                 const co_unsigned16_t idx,
                                 const void* const data) {
  CheckSubDnInd(dev, idx,
                [=](co_sub_dn_ind_t* const ind, const void* const ind_data) {
                  CHECK(ind != &co_sub_default_dn_ind);
                  CHECK(ind != nullptr);

                  POINTERS_EQUAL(data, ind_data);
                });
}

void
LelyUnitTest::CheckSubDnIndIsDefault(const co_dev_t* const dev,
                                     co_unsigned16_t const idx) {
  CheckSubDnInd(dev, idx,
                [=](co_sub_dn_ind_t* const ind, const void* const data) {
                  FUNCTIONPOINTERS_EQUAL(ind, &co_sub_default_dn_ind);

                  POINTERS_EQUAL(nullptr, data);
                });
}
