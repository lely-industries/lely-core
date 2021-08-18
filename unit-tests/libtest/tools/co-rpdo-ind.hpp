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

#ifndef LELY_UNIT_TEST_CO_RPDO_IND_HPP_
#define LELY_UNIT_TEST_CO_RPDO_IND_HPP_

#include <lely/co/rpdo.h>

/// #co_rpdo_ind_t mock.
class CoRpdoInd {
 public:
  static void Func(co_rpdo_t* pdo, co_unsigned32_t ac, const void* ptr,
                   size_t n, void* data);
  static void Clear();
  static void Check(const co_rpdo_t* pdo, co_unsigned32_t ac, const void* ptr,
                    size_t n, const void* data);
  static void CheckPtrNotNull(const co_rpdo_t* pdo, co_unsigned32_t ac,
                              size_t n, const void* data);
  static inline size_t
  GetNumCalled() {
    return num_called_;
  }

 private:
  static size_t num_called_;

  static co_rpdo_t* pdo_;
  static co_unsigned32_t ac_;
  static const void* ptr_;
  static size_t n_;
  static void* data_;
};

#endif  // LELY_UNIT_TEST_CO_RPDO_IND_HPP_
