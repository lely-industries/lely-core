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

#include <CppUTest/TestHarness.h>

#include <lely/util/memory.h>
#include <lely/util/error.h>

TEST_GROUP(Util_MemoryDefaultAllocator) {
  void* ptr = nullptr;

  TEST_TEARDOWN() { mem_free(nullptr, ptr); }
};

// given: default memory allocator
// when: trying to allocate 0 bytes
// then: null pointer is returned
TEST(Util_MemoryDefaultAllocator, MemAlloc_ZeroSize) {
  ptr = mem_alloc(nullptr, 0, 0);

  POINTERS_EQUAL(nullptr, ptr);
}

#if LELY_NO_MALLOC

// given: default memory allocator
// when: trying to allocate miss-aligned bytes
// then: null pointer is returned and error is reported.
TEST(Util_MemoryDefaultAllocator, MemAlloc_BadAlignment) {
  const size_t alignment = 3u;

  ptr = mem_alloc(nullptr, alignment, sizeof(int));

  POINTERS_EQUAL(nullptr, ptr);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

// given: default memory allocator in 'no malloc' mode
// when: trying to allocate any memory
// then: null pointer is returned and error is reported.
TEST(Util_MemoryDefaultAllocator, MemAlloc_AnyAllocationFails) {
  ptr = mem_alloc(nullptr, alignof(int), sizeof(int));

  POINTERS_EQUAL(nullptr, ptr);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}

// given: default memory allocator in 'no malloc' mode
// when: trying to retrieve allocated memory size
// then: zero is returned
TEST(Util_MemoryDefaultAllocator, MemSize) {
  const auto ret = mem_size(nullptr);

  CHECK_EQUAL(0, ret);
}

// given: default memory allocator in 'no malloc' mode
// when: trying to retrieve memory capacity
// then: zero is returned
TEST(Util_MemoryDefaultAllocator, MemCapacity) {
  const auto ret = mem_capacity(nullptr);

  CHECK_EQUAL(0, ret);
}

#endif  // LELY_NO_MALLOC
