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

#include <functional>

#include <CppUTest/TestHarness.h>

#include <lely/co/dev.h>
#include <lely/co/detail/obj.h>
#include <lely/co/obj.h>

#include "co-sub-dn-ind.hpp"

unsigned int CoSubDnInd::num_called = 0u;
co_sub_t* CoSubDnInd::sub = nullptr;
co_sdo_req* CoSubDnInd::req = nullptr;
co_unsigned32_t CoSubDnInd::ac = 0u;
void* CoSubDnInd::data = nullptr;

co_unsigned32_t
CoSubDnInd::func(co_sub_t* sub_, co_sdo_req* req_, co_unsigned32_t ac_,
                 void* data_) {
  num_called++;

  sub = sub_;
  req = req_;
  ac = ac_;
  data = data_;

  return 0;
}

void
CoSubDnInd::Clear() {
  num_called = 0;

  sub = nullptr;
  req = nullptr;
  ac = 0;
  data = nullptr;
}

static void CheckSubDnInd(
    const co_dev_t* const dev, const co_unsigned16_t idx,
    std::function<void(co_sub_dn_ind_t*, void* data)> pred);

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

co_unsigned32_t
LelyUnitTest::CallDnIndWithAbortCode(const co_dev_t* const dev,
                                     const co_unsigned16_t idx,
                                     const co_unsigned8_t subidx,
                                     const co_unsigned32_t ac) {
  co_sub_t* const sub = co_dev_find_sub(dev, idx, subidx);
  CHECK(sub != nullptr);

  co_sub_dn_ind_t* ind = nullptr;
  void* data = nullptr;
  co_sub_get_dn_ind(sub, &ind, &data);
  CHECK(ind != nullptr);
  CHECK(data != nullptr);

  co_sdo_req req;
  co_sdo_req_init(&req, nullptr);

  return ind(sub, &req, ac, data);
}
