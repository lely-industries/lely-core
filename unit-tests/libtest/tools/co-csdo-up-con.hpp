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

#ifndef LELY_UNIT_TEST_CO_CSDO_UP_CON_HPP_
#define LELY_UNIT_TEST_CO_CSDO_UP_CON_HPP_

#include <lely/co/type.h>

struct CoCsdoUpCon {
  static co_csdo_t* sdo;
  static co_unsigned16_t idx;
  static co_unsigned8_t subidx;
  static co_unsigned32_t ac;
  static const void* ptr;
  static size_t n;
  static void* data;
  static size_t num_called;
  static constexpr size_t BUFSIZE = 4u;
  static uint_least8_t buf[BUFSIZE];

  static void func(co_csdo_t* sdo_, co_unsigned16_t idx_,
                   co_unsigned8_t subidx_, co_unsigned32_t ac_,
                   const void* ptr_, size_t n_, void* data_);
  static void Check(const co_csdo_t* sdo_, co_unsigned16_t idx_,
                    co_unsigned8_t subidx_, co_unsigned32_t ac_,
                    const void* ptr_, size_t n_, const void* data_);
  static void CheckNonempty(const co_csdo_t* sdo_, co_unsigned16_t idx_,
                            co_unsigned8_t subidx_, co_unsigned32_t ac_,
                            size_t n_, const void* data_);
  static void Clear();

  static inline bool
  Called() {
    return num_called > 0;
  }
};

#endif  // LELY_UNIT_TEST_CO_CSDO_UP_CON_HPP_
