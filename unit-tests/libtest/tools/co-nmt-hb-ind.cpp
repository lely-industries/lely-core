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
#include <CppUTestExt/MockSupport.h>

#include <lely/co/nmt.h>
#include <lely/co/type.h>

#include "co-nmt-hb-ind.hpp"

size_t CoNmtHbInd::num_called = 0;
co_nmt_t* CoNmtHbInd::nmt_ = nullptr;
co_unsigned8_t CoNmtHbInd::id_ = 255u;
int CoNmtHbInd::state_ = -1;
int CoNmtHbInd::reason_ = -1;
void* CoNmtHbInd::data_ = nullptr;
bool CoNmtHbInd::skipCallToDefaultInd = false;

void
CoNmtHbInd::Func(co_nmt_t* const nmt, const co_unsigned8_t id, const int state,
                 const int reason, void* const data) {
  num_called++;

  nmt_ = nmt;
  id_ = id;
  state_ = state;
  reason_ = reason;
  data_ = data;

  if (!skipCallToDefaultInd) co_nmt_on_hb(nmt, id, state, reason);
}

void
CoNmtHbInd::Clear() {
  num_called = 0;

  nmt_ = nullptr;
  id_ = 255u;
  state_ = -1;
  reason_ = -1;
  data_ = nullptr;

  skipCallToDefaultInd = false;
}

void
CoNmtHbInd::Check(const co_nmt_t* const nmt, const co_unsigned8_t id,
                  const int state, const int reason, const void* const data) {
  POINTERS_EQUAL(nmt, nmt_);
  CHECK_EQUAL(id, id_);
  CHECK_EQUAL(state, state_);
  CHECK_EQUAL(reason, reason_);
  POINTERS_EQUAL(data, data_);
}

void
CoNmtHbInd::SkipCallToDefaultInd() {
  skipCallToDefaultInd = true;
}

co_nmt_hb_ind_t*
CoNmtHbIndMock::GetFunc() {
  mock("ConNmtHbIndMock").strictOrder();
  return [](co_nmt_t* const nmt, const co_unsigned8_t id, const int state,
            const int reason, void* const data) {
    mock("CoNmtHbIndMock")
        .actualCall("co_nmt_hb_ind_t")
        .withParameter("nmt", nmt)
        .withParameter("id", id)
        .withParameter("state", state)
        .withParameter("reason", reason)
        .withParameter("data", data);

    co_nmt_on_hb(nmt, id, state, reason);
  };
}

void*
CoNmtHbIndMock::GetData() {
  return &indData;
}

void
CoNmtHbIndMock::Expect(co_nmt_t* const nmt, const co_unsigned8_t id,
                       const int state, const int reason) {
  mock("CoNmtHbIndMock")
      .expectOneCall("co_nmt_hb_ind_t")
      .withParameter("nmt", nmt)
      .withParameter("id", id)
      .withParameter("state", state)
      .withParameter("reason", reason)
      .withParameter("data", &indData);
}
