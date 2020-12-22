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

#ifndef LELY_ALLOCATORS_FAILING_H
#define LELY_ALLOCATORS_FAILING_H

#include <cstddef>

#include "default.hpp"

namespace Allocators {

class Failing {
 public:
  explicit Failing() : alloc(&vtbl), allowedAllocsCount(0) {
    vtbl.alloc = Alloc;
    vtbl.free = Free;
    vtbl.size = Size;
    vtbl.capacity = Capacity;
  }

  void
  FailOnNthAllocation(unsigned n) {
    allowedAllocsCount = n - 1;
  }

  alloc_t*
  ToAllocT() const {
    return &alloc;
  }

 private:
  static void*
  Alloc(alloc_t* alloc, size_t alignment, size_t size) {
    auto this_ = Cast(alloc);
    if (this_->allowedAllocsCount == 0) return nullptr;
    this_->allowedAllocsCount--;
    return mem_alloc(Inner(alloc), alignment, size);
  }

  static void
  Free(alloc_t* alloc, void* ptr) {
    mem_free(Inner(alloc), ptr);
  }

  static size_t
  Size(const alloc_t* alloc) {
    return mem_size(Inner(alloc));
  }

  static size_t
  Capacity(const alloc_t* alloc) {
    return mem_capacity(Inner(alloc));
  }

  static Failing*
  Cast(alloc_t* alloc) {
    static_assert(offsetof(Failing, alloc) == 0,
                  "alloc must remain first member");
    return const_cast<Failing*>(reinterpret_cast<const Failing*>(alloc));
  }

  static alloc_t*
  Inner(alloc_t* alloc) {
    // might return nullptr
    return Cast(alloc)->inner.ToAllocT();
  }

  alloc_t alloc;
  alloc_vtbl vtbl;
  Default inner;
  unsigned allowedAllocsCount;
};

}  // namespace Allocators

#endif  // LELY_ALLOCATORS_FAILING_H