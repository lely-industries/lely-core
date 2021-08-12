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

#ifndef LELY_ALLOCATORS_LIMITED_H
#define LELY_ALLOCATORS_LIMITED_H

#include <cassert>
#include <cstddef>
#include <limits>

#include <lely/util/error.h>

#include "default.hpp"

namespace Allocators {

/// Memory allocator that allows control over available memory left.
/// See #alloc_t.
class Limited {
 public:
  explicit Limited() : alloc(&vtbl) {
    vtbl.alloc = Alloc;
    vtbl.free = Free;
    vtbl.size = Size;
    vtbl.capacity = Capacity;
  }

  void
  LimitAllocationTo(const size_t limit) {
#if LELY_NO_MALLOC
    assert(limit <= POOL_SIZE);
#endif
    allocationLimit = limit;
  }

  size_t
  GetAllocationLimit() const {
    return allocationLimit;
  }

  alloc_t*
  ToAllocT() const {
    return &alloc;
  }

 private:
  static void*
  Alloc(alloc_t* const alloc, const size_t alignment, const size_t size) {
    auto* const this_ = Cast(alloc);
    if (this_->allocationLimit < size) {
      set_errnum(ERRNUM_NOMEM);
      return nullptr;
    }

    auto* const ret = mem_alloc(Inner(alloc), alignment, size);
    if (ret != nullptr) this_->allocationLimit -= size;

    return ret;
  }

  static void
  Free(alloc_t* const alloc, void* const ptr) {
    mem_free(Inner(alloc), ptr);
  }

  static size_t
  Size(const alloc_t* const alloc) {
    return mem_size(Inner(alloc));
  }

  static size_t
  Capacity(const alloc_t* const alloc) {
    return mem_capacity(Inner(alloc));
  }

  static Limited*
  Cast(alloc_t* const alloc) {
    static_assert(offsetof(Limited, alloc) == 0,
                  "alloc must remain first member");
    return const_cast<Limited*>(reinterpret_cast<const Limited*>(alloc));
  }

  static alloc_t*
  Inner(alloc_t* const alloc) {
    // might return nullptr
    return Cast(alloc)->inner.ToAllocT();
  }

  alloc_t alloc;
  alloc_vtbl vtbl;
  Default inner;
  size_t allocationLimit = std::numeric_limits<size_t>::max();
};

}  // namespace Allocators

#endif  // LELY_ALLOCATORS_LIMITED_H
