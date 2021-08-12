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

#ifndef LELY_ALLOCATORS_POOL_H
#define LELY_ALLOCATORS_POOL_H

#include <stdint.h>

#include <lely/util/mempool.h>

namespace Allocators {

/// Memory pool based allocator (see #alloc_t).
template <size_t PoolSize>
class PoolAllocator {
 public:
  explicit PoolAllocator() : alloc(mempool_init(&pool, memory, PoolSize)) {}

  alloc_t*
  ToAllocT() const {
    return alloc;
  }

 private:
  uint8_t memory[PoolSize];
  mempool pool;
  alloc_t* const alloc;
};

}  // namespace Allocators

#endif  // LELY_ALLOCATORS_POOL_H
