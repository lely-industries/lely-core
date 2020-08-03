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

#ifndef LELY_UNIT_TESTS_CO_OBJ_HOLDER_HPP_
#define LELY_UNIT_TESTS_CO_OBJ_HOLDER_HPP_

#include <lely/co/obj.h>

#if LELY_NO_MALLOC
#include <lely/co/detail/obj.h>
#endif

#include "holder.hpp"

class CoObjTHolder : public Holder<co_obj_t> {
#if LELY_NO_MALLOC
 public:
  explicit CoObjTHolder(co_unsigned16_t idx) {
    __co_obj_init(Get(), idx, array.u.data, PREALOCATED_OBJ_SIZE);
  }

  static const size_t PREALOCATED_OBJ_SIZE = CO_ARRAY_CAPACITY;

 private:
  co_array array = CO_ARRAY_INIT;
#else   // !LELY_NO_MALLOC
 public:
  explicit CoObjTHolder(co_unsigned16_t idx)
      : Holder<co_obj_t>(co_obj_create(idx)) {}

  ~CoObjTHolder() { co_obj_destroy(Get()); }
#endif  // LELY_NO_MALLOC
};      // class CoObjTHolder

#endif  // LELY_UNIT_TESTS_CO_OBJ_HOLDER_HPP_
