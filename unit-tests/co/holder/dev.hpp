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

#ifndef LELY_UNIT_TESTS_CO_DEV_HOLDER_HPP_
#define LELY_UNIT_TESTS_CO_DEV_HOLDER_HPP_

#include <lely/co/dev.h>

#if LELY_NO_MALLOC
#include <lely/co/detail/dev.h>
#endif

#include "holder.hpp"

class CoDevTHolder : public Holder<co_dev_t> {
#if LELY_NO_MALLOC
 public:
  explicit CoDevTHolder(co_unsigned8_t id) { __co_dev_init(Get(), id); }
#else   // !LELY_NO_MALLOC
 public:
  explicit CoDevTHolder(co_unsigned8_t id)
      : Holder<co_dev_t>(co_dev_create(id)) {}

  ~CoDevTHolder() {
    if (!taken) co_dev_destroy(Get());
  }
#endif  // LELY_NO_MALLOC
};      // class CoDevTHolder

#endif  // LELY_UNIT_TESTS_CO_DEV_HOLDER_HPP_
