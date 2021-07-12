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

#ifndef LELY_UNIT_TEST_CO_CSDO_DN_CON_HPP_
#define LELY_UNIT_TEST_CO_CSDO_DN_CON_HPP_

#include <lely/co/type.h>

class CoCsdoDnCon {
 public:
  static co_csdo_t* sdo;
  static co_unsigned16_t idx;
  static co_unsigned8_t subidx;
  static co_unsigned32_t ac;
  static void* data;

  static void Func(co_csdo_t* sdo_, co_unsigned16_t idx_,
                   co_unsigned8_t subidx_, co_unsigned32_t ac_, void* data_);
  static void Check(const co_csdo_t* sdo_, co_unsigned16_t idx_,
                    co_unsigned8_t subidx_, co_unsigned32_t ac_,
                    const void* data_);
  static void Clear();

  static inline bool
  Called() {
    return num_called > 0;
  }
  static inline size_t
  GetNumCalled() {
    return num_called;
  }

 private:
  static size_t num_called;
};

#endif  // LELY_UNIT_TEST_CO_CSDO_DN_CON_HPP_
