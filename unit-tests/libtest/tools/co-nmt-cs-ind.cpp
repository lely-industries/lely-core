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

#include <lely/co/val.h>

#include "co-nmt-cs-ind.hpp"

size_t CoNmtCsInd::num_called_ = 0;
const co_nmt_t* CoNmtCsInd::nmt_ = nullptr;
co_unsigned8_t CoNmtCsInd::cs_ = CO_UNSIGNED8_MAX;
const void* CoNmtCsInd::data_ = nullptr;
std::function<co_nmt_cs_ind_t> CoNmtCsInd::checkFunc;

void
CoNmtCsInd::Func(co_nmt_t* const nmt, const co_unsigned8_t cs,
                 void* const data) {
  if (checkFunc != nullptr) checkFunc(nmt, cs, data);

  ++num_called_;

  nmt_ = nmt;
  cs_ = cs;
  data_ = data;
}

void
CoNmtCsInd::Clear() {
  num_called_ = 0;

  nmt_ = nullptr;
  cs_ = CO_UNSIGNED8_MAX;
  data_ = nullptr;

  checkFunc = nullptr;
}

void
CoNmtCsInd::Check(const co_nmt_t* const nmt, const co_unsigned8_t cs,
                  const void* const data) {
  POINTERS_EQUAL(nmt, nmt_);
  CHECK_EQUAL(cs, cs_);
  POINTERS_EQUAL(data, data_);
}

void
CoNmtCsInd::SetCheckSeq(const co_nmt_t* const nmt, const Seq& csSeq) {
  Clear();

  checkFunc = [nmt, csSeq](const co_nmt_t* const service,
                           const co_unsigned8_t cs, const void* const data) {
    CHECK(num_called_ < csSeq.size());
    POINTERS_EQUAL(nmt, service);
    CHECK_EQUAL(csSeq[num_called_], cs);
    POINTERS_EQUAL(nullptr, data);
  };
}
