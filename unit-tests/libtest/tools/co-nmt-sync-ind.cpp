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

#include "co-nmt-sync-ind.hpp"

size_t CoNmtSyncInd::num_called_ = 0;
co_nmt_t* CoNmtSyncInd::nmt_ = nullptr;
co_unsigned8_t CoNmtSyncInd::cnt_ = CO_UNSIGNED8_MAX;
void* CoNmtSyncInd::data_ = nullptr;

void
CoNmtSyncInd::Func(co_nmt_t* const nmt, const co_unsigned8_t cnt,
                   void* const data) {
  num_called_++;

  nmt_ = nmt;
  cnt_ = cnt;
  data_ = data;
}

void
CoNmtSyncInd::Clear() {
  num_called_ = 0;

  nmt_ = nullptr;
  cnt_ = CO_UNSIGNED8_MAX;
  data_ = nullptr;
}

void
CoNmtSyncInd::Check(const co_nmt_t* const nmt, const co_unsigned8_t cnt,
                    const void* const data) {
  CHECK_COMPARE(num_called_, >, 0);
  POINTERS_EQUAL(nmt, nmt_);
  CHECK_EQUAL(cnt, cnt_);
  POINTERS_EQUAL(data, data_);
}
