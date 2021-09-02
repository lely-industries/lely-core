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

#include "co-tpdo-sample-ind.hpp"

size_t CoTpdoSampleInd::num_called_ = 0;

co_tpdo_t* CoTpdoSampleInd::pdo_ = nullptr;
void* CoTpdoSampleInd::data_ = nullptr;
bool CoTpdoSampleInd::skipSampleResCall_ = false;

int
CoTpdoSampleInd::Func(co_tpdo_t* const pdo, void* const data) {
  ++num_called_;

  pdo_ = pdo;
  data_ = data;

  if (skipSampleResCall_)
    return 0;
  else
    return co_tpdo_sample_res(pdo, 0);
}

void
CoTpdoSampleInd::Clear() {
  num_called_ = 0u;

  pdo_ = nullptr;
  data_ = nullptr;

  skipSampleResCall_ = false;
}

void
CoTpdoSampleInd::Check(const co_tpdo_t* const pdo, const void* const data) {
  CHECK_COMPARE(num_called_, >, 0);
  POINTERS_EQUAL(pdo, pdo_);
  POINTERS_EQUAL(data, data_);
}
