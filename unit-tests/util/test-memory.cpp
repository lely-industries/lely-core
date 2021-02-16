/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2020-2021 N7 Space Sp. z o.o.
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

/// @name mem_alloc()
///@{

/// \Given N/A
/// \When mem_alloc() is called with null pointer to allocator, any alignment
///       value and zero size
/// \Then null pointer is returned
TEST(Util_MemoryDefaultAllocator, MemAlloc_ZeroSize) {
  ptr = mem_alloc(nullptr, 0, 0);

  POINTERS_EQUAL(nullptr, ptr);
}

#if LELY_NO_MALLOC

/// \Given LELY_NO_MALLOC enabled
/// \When mem_alloc() is called with null pointer to allocator, non-zero
///       alignment that's not a power of 2 and non-zero size
/// \Then null pointer is returned;
///       if !LELY_NO_ERRNO invalid argument error is reported
TEST(Util_MemoryDefaultAllocator, MemAlloc_BadAlignment) {
  const size_t alignment = 3u;

  ptr = mem_alloc(nullptr, alignment, sizeof(int));

  POINTERS_EQUAL(nullptr, ptr);
#if !LELY_NO_ERRNO
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
#endif
}

/// \Given LELY_NO_MALLOC enabled
/// \When mem_alloc() is called with null pointer to allocator, zero (default)
///       alignment and non-zero size
/// \Then null pointer is returned;
///       if !LELY_NO_ERRNO no memory error is reported
TEST(Util_MemoryDefaultAllocator, MemAlloc_ZeroAlignment) {
  ptr = mem_alloc(nullptr, 0, sizeof(int));

  POINTERS_EQUAL(nullptr, ptr);
#if !LELY_NO_ERRNO
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
#endif
}

/// \Given LELY_NO_MALLOC enabled
/// \When mem_alloc() is called with null pointer to allocator, non-zero
///       alignment and non-zero size
/// \Then null pointer is returned;
///       if !LELY_NO_ERRNO no memory error is reported
TEST(Util_MemoryDefaultAllocator, MemAlloc_AnyAllocationFails) {
  ptr = mem_alloc(nullptr, alignof(int), sizeof(int));

  POINTERS_EQUAL(nullptr, ptr);
#if !LELY_NO_ERRNO
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
#endif
}

///@}

/// @name mem_size()
///@{

/// \Given LELY_NO_MALLOC enabled
/// \When mem_size() is called with null pointer to allocator
/// \Then zero is returned
TEST(Util_MemoryDefaultAllocator, MemSize) {
  const auto ret = mem_size(nullptr);

  CHECK_EQUAL(0, ret);
}

///@}

/// @name mem_capacity()
///@{

/// \Given LELY_NO_MALLOC enabled
/// \When mem_capacity() is called with null pointer to allocator
/// \Then zero is returned
TEST(Util_MemoryDefaultAllocator, MemCapacity) {
  const auto ret = mem_capacity(nullptr);

  CHECK_EQUAL(0, ret);
}

///@}

TEST_GROUP(Util_MemoryDefaultAllocatorFree){};

/// @name mem_free()
///@{

/// \Given LELY_NO_MALLOC enabled
/// \When mem_free() is called with null pointer to allocator and any pointer to
///       be freed
/// \Then nothing happens
TEST(Util_MemoryDefaultAllocatorFree, MemFree) {
  int data = 42;
  mem_free(nullptr, &data);
}

///@}

#endif  // LELY_NO_MALLOC
