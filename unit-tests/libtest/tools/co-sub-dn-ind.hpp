
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

#ifndef LELY_UNIT_TEST_CO_SUB_DN_IND_HPP_
#define LELY_UNIT_TEST_CO_SUB_DN_IND_HPP_

#include <lely/co/type.h>
#include <lely/co/sdo.h>

struct CoSubDnInd {
  static unsigned int num_called;
  static co_sub_t* sub;
  static co_sdo_req* req;
  static co_unsigned32_t ac;
  static void* data;

  static co_unsigned32_t func(co_sub_t* sub_, co_sdo_req* req_,
                              co_unsigned32_t ac_, void* data_);
  static void Clear();

  static inline bool
  Called() {
    return num_called > 0;
  }
};

namespace LelyUnitTest {
/**
 * Checks if a download indication function is set (not null) for
 * a sub-object with the given user-specifed data pointer.
 */
void CheckSubDnIndIsSet(const co_dev_t* dev, co_unsigned16_t idx,
                        const void* data);
/**
 * Checks if sub-object has a default download indication function and
 * user-specified data set.
 */
void CheckSubDnIndIsDefault(const co_dev_t* dev, co_unsigned16_t idx);

/**
  * Calls the download indication function for the sub-object with the given
  * abort code.
  */
co_unsigned32_t CallDnIndWithAbortCode(const co_dev_t* dev, co_unsigned16_t idx,
                                       co_unsigned8_t subidx,
                                       co_unsigned32_t ac);

}  // namespace LelyUnitTest

#endif  // LELY_UNIT_TEST_CO_SUB_DN_IND_HPP_