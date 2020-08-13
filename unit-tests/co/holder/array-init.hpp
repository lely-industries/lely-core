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

#ifndef LELY_UNIT_TESTS_CO_ARRAY_INIT_HPP_
#define LELY_UNIT_TESTS_CO_ARRAY_INIT_HPP_

#include <list>
#include <cstring>

#include <lely/co/val.h>

class CoArrays {
 public:
  template <typename T>
  T
  Init() {
#if LELY_NO_MALLOC
    const co_array array = CO_ARRAY_INIT;
    return Make<T>(array);
#else
    return nullptr;
#endif
  }

  template <typename T>
  T
  DeadBeef() {
#if LELY_NO_MALLOC
    co_array array;
    array.hdr.capacity = CO_ARRAY_CAPACITY;
    array.hdr.size = 42;
    memset(array.u.data, 0xDD, CO_ARRAY_CAPACITY);
    return Make<T>(array);
#else
    return reinterpret_cast<T>(0xdeadbeefu);
#endif
  }
  void
  Clear() {
#if LELY_NO_MALLOC
    arrays.clear();
#endif
  }

  bool
  IsEmptyInitialized(void* arr) {
#if LELY_NO_MALLOC
    char test_buf[CO_ARRAY_CAPACITY] = {0};
    return memcmp(arr, test_buf, CO_ARRAY_CAPACITY) == 0;
#else
    return arr == nullptr;
#endif
  }

 private:
#if LELY_NO_MALLOC
  co_array*
  Push(const co_array& arr) {
    arrays.emplace_back(arr);
    return &arrays.back();
  }

  template <typename T>
  T
  Make(const co_array& arr) {
    T ret;
    co_val_init_array(&ret, Push(arr));
    return ret;
  }

  std::list<co_array> arrays;
#endif
};

#endif  // LELY_UNIT_TESTS_CO_ARRAY_INIT_HPP_
