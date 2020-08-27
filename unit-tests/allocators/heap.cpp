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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include <lely/util/errnum.h>
#include <lely/util/util.h>

#include "heap.hpp"

using Allocators::HeapAllocator;

namespace {

void*
default_mem_alloc(alloc_t*, size_t alignment, size_t size) {
  if (!size) return NULL;

  if (!alignment) alignment = _Alignof(max_align_t);

  if (!powerof2(alignment) || alignment > _Alignof(max_align_t)) {
    set_errnum(ERRNUM_INVAL);
    return NULL;
  }

  return malloc(size);
}

void
default_mem_free(alloc_t*, void* ptr) {
  free(ptr);
}

size_t
default_size(const alloc_t*) {
  return 0;
}

size_t
default_capacity(const alloc_t*) {
  return 0;
}

}  // namespace

HeapAllocator::HeapAllocator() : alloc(&alloc_f) {
  alloc_f.alloc = default_mem_alloc;
  alloc_f.free = default_mem_free;
  alloc_f.size = default_size;
  alloc_f.capacity = default_capacity;
}

alloc_t*
HeapAllocator::ToAllocT() const {
#ifdef LELY_NO_MALLOC
  return &alloc;
#else
  return nullptr;
#endif
}
