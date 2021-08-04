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

#include "co-nmt-st-ind.hpp"

size_t CoNmtStInd::num_called = 0;
co_nmt_t* CoNmtStInd::nmt_ = nullptr;
co_unsigned8_t CoNmtStInd::id_ = CO_UNSIGNED8_MAX;
co_unsigned8_t CoNmtStInd::st_ = CO_UNSIGNED8_MAX;
void* CoNmtStInd::data_ = nullptr;
std::function<co_nmt_st_ind_t> CoNmtStInd::checkFunc;

void
CoNmtStInd::Func(co_nmt_t* const nmt, const co_unsigned8_t id,
                 const co_unsigned8_t st, void* const data) {
  if (checkFunc != nullptr) checkFunc(nmt, id, st, data);

  num_called++;

  nmt_ = nmt;
  id_ = id;
  st_ = st;
  data_ = data;

  co_nmt_on_st(nmt, id, st);
}

void
CoNmtStInd::Clear() {
  num_called = 0u;

  nmt_ = nullptr;
  id_ = CO_UNSIGNED8_MAX;
  st_ = CO_UNSIGNED8_MAX;
  data_ = nullptr;
  checkFunc = nullptr;
}

void
CoNmtStInd::Check(const co_nmt_t* const nmt, const co_unsigned8_t id,
                  const co_unsigned8_t st, const void* const data) {
  POINTERS_EQUAL(nmt, nmt_);
  CHECK_EQUAL(id, id_);
  CHECK_EQUAL(st, st_);
  POINTERS_EQUAL(data, data_);
}

void
CoNmtStInd::SetCheckSeq(const co_nmt_t* const nmt, const co_unsigned8_t id,
                        const Seq& stSeq) {
  checkFunc = [nmt, id, stSeq](
                  const co_nmt_t* const service, const co_unsigned8_t seqId,
                  const co_unsigned8_t st, const void* const data) {
    CHECK_COMPARE(num_called, <, stSeq.size());
    POINTERS_EQUAL(nmt, service);
    CHECK_EQUAL(id, seqId);
    CHECK_EQUAL(stSeq[num_called], st);
    POINTERS_EQUAL(data_, data);
  };
}
