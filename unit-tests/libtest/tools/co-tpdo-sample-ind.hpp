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

#ifndef LELY_UNIT_TEST_CO_TPDO_SAMPLE_IND_HPP_
#define LELY_UNIT_TEST_CO_TPDO_SAMPLE_IND_HPP_

#include <lely/co/tpdo.h>

/// #co_tpdo_sample_ind_t mock.
class CoTpdoSampleInd {
 public:
  static int Func(co_tpdo_t* pdo, void* data);
  static void Clear();
  static void Check(const co_tpdo_t* pdo, const void* data);
  static inline size_t
  GetNumCalled() {
    return num_called_;
  }
  static void
  SetSkipSampleResCall(const bool skip) {
    skipSampleResCall_ = skip;
  }

 private:
  static size_t num_called_;

  static co_tpdo_t* pdo_;
  static void* data_;

  static bool skipSampleResCall_;
};

#endif  // LELY_UNIT_TEST_CO_TPDO_SAMPLE_IND_HPP_
