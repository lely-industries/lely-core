/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2020 N7 Space Sp. z o.o.
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

#ifndef LELY_UNIT_TESTS_CO_SUB_HOLDER_HPP_
#define LELY_UNIT_TESTS_CO_SUB_HOLDER_HPP_

#include <lely/co/obj.h>

#if LELY_NO_MALLOC
#include <lely/co/detail/obj.h>
#endif

#include "holder.hpp"

class CoSubTHolder : public Holder<co_sub_t> {
#if LELY_NO_MALLOC

 public:
  explicit CoSubTHolder(co_unsigned8_t subidx, co_unsigned16_t type)
      : value({}) {
    if (co_type_is_array(type)) {
      co_val_init_array(&value, &array);
      __co_sub_init(Get(), subidx, type, &value);
    } else {
      __co_sub_init(Get(), subidx, type, nullptr);
    }
  }

 private:
  co_val value;
  co_array array = CO_ARRAY_INIT;
#else   // !LELY_NO_MALLOC

 public:
  explicit CoSubTHolder(co_unsigned8_t subidx, co_unsigned16_t type)
      : Holder<co_sub_t>(co_sub_create(subidx, type)) {}

  ~CoSubTHolder() {
    if (!taken) co_sub_destroy(Get());
  }
#endif  // LELY_NO_MALLOC
};      // class CoSubTHolder

#endif  // LELY_UNIT_TESTS_CO_SUB_HOLDER_HPP_
