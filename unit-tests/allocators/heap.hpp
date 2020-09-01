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

#ifndef LELY_ALLOCATORS_HEAP_H
#define LELY_ALLOCATORS_HEAP_H

#include <lely/util/memory.h>

namespace Allocators {

class HeapAllocator {
 public:
  explicit HeapAllocator();

  alloc_t* ToAllocT() const;

 private:
  alloc_t alloc;
  alloc_vtbl alloc_f;
};

}  // namespace Allocators

#endif  // LELY_ALLOCATORS_HEAP_H
