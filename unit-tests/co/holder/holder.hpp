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

#ifndef LELY_UNIT_TESTS_CO_HOLDER_HPP_
#define LELY_UNIT_TESTS_CO_HOLDER_HPP_

#include <cassert>

template <typename Item>
class Holder {
#if LELY_NO_MALLOC

 protected:
  Holder() : item({}) {}

 public:
  Item*
  Get() {
    return &item;
  }

  Item*
  Take() {
    return Get();
  }

 private:
  Item item;
#else   // !LELY_NO_MALLOC

 protected:
  explicit Holder(Item* item_) : item(item_) {}

 public:
  Item*
  Get() {
    return item;
  }

  Item*
  Take() {
    assert(taken == false);
    taken = true;
    return item;
  }

 protected:
  bool taken = false;

 private:
  Item* item = nullptr;
#endif  // LELY_NO_MALLOC
};      // class Holder

#endif  // LELY_UNIT_TESTS_CO_HOLDER_HPP_
