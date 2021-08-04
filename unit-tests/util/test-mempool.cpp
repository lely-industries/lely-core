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

#include <cstring>

#include <lely/util/mempool.h>
#include <lely/util/error.h>

TEST_GROUP(Util_MemPoolInit){};

/// @name mempool_init()
///@{

/// \Given an uninitialized memory pool and a memory buffer
///
/// \When mempool_init() is called with a pointer to the pool, the memory buffer
///       and its size
///
/// \Then a pointer to an abstract allocator (alloc_t) is returned with non-null
///       function pointers, memory pool is initialized with the memory buffer
///       and points at it beginning
TEST(Util_MemPoolInit, MemPool_Init) {
  mempool pool;
  const size_t size = 10;
  char buffer[size] = {0};

  const auto* const alloc = mempool_init(&pool, buffer, size);

  CHECK((*alloc)->alloc != nullptr);
  CHECK((*alloc)->free != nullptr);
  CHECK((*alloc)->size != nullptr);
  CHECK((*alloc)->capacity != nullptr);

  POINTERS_EQUAL(pool.beg, buffer);
  POINTERS_EQUAL(pool.end, buffer + size);
  POINTERS_EQUAL(pool.cur, buffer);
}

///@}

TEST_GROUP(Util_MemPool) {
  static const size_t POOL_SIZE = 1024u;
  mempool pool;
  alloc_t* alloc = nullptr;
  char memory[POOL_SIZE] = {0};

  TEST_SETUP() { alloc = mempool_init(&pool, memory, POOL_SIZE); }
};

/// @name mem_size()
///@{

/// \Given a pointer to an allocator (alloc_t) based on memory pool, with no
///        prior allocations
///
/// \When mem_size() is called
///
/// \Then zero is returned
TEST(Util_MemPool, MemPool_Size) { CHECK_EQUAL(0, mem_size(alloc)); }

///@}

/// @name mem_capacity()
///@{

/// \Given a pointer to an allocator (alloc_t) based on memory pool
///
/// \When mem_capacity() is called
///
/// \Then total memory pool size is returned
TEST(Util_MemPool, MemPool_Capacity) {
  CHECK_EQUAL(POOL_SIZE, mem_capacity(alloc));
}

///@}

/// @name mem_alloc()

/// \Given a pointer to an allocator (alloc_t) based on memory pool, with no
///        prior allocations
///
/// \When mem_alloc() is called with zero alignment and non-zero allocation size
///
/// \Then a pointer pointing to memory pool's buffer first byte is returned;
///       mem_size() returns requested allocation size, remaining capacity
///       returned by mem_capacity() is lower by requested size
TEST(Util_MemPool, MemPool_Alloc) {
  const size_t allocationSize = 10u;

  const auto result = mem_alloc(alloc, 0, allocationSize);

  POINTERS_EQUAL(memory, result);
  CHECK_EQUAL(allocationSize, mem_size(alloc));
  CHECK_EQUAL(POOL_SIZE - allocationSize, mem_capacity(alloc));
}

/// \Given a pointer to an allocator (alloc_t) based on memory pool, with no
///        prior allocations
///
/// \When mem_alloc() is called twice with non-zero alignment and non-zero size
///       that's not a multiple of alignment's value
///
/// \Then both calls return non-null pointers that point into memory pool's
///       buffer and are apart by requested allocation size rounded up to a
///       multiple of requested alignment; mem_size() returns twice the
///       requested allocation size plus required padding, remaining capacity is
///       lowered by same value
TEST(Util_MemPool, MemPool_Alloc_RespectsAlignment) {
  const size_t allocationSize = 11u;
  const size_t alignment = 2u;

  const auto result1 = mem_alloc(alloc, alignment, allocationSize);
  const auto result2 = mem_alloc(alloc, alignment, allocationSize);

  POINTERS_EQUAL(memory, result1);
  POINTERS_EQUAL(memory + allocationSize + 1u, result2);
  CHECK_EQUAL(2u * allocationSize + 1u, mem_size(alloc));
  CHECK_EQUAL(POOL_SIZE - mem_size(alloc), mem_capacity(alloc));
}

/// \Given a pointer to an allocator (alloc_t) based on memory pool, with no
///        prior allocations
///
/// \When mem_alloc() is called with non-zero alignment that's not a power of 2
///       and non-zero size
///
/// \Then a null pointer is returned, nothing is changed,
///       ERRNUM_INVAL error number is set
TEST(Util_MemPool, MemPool_Alloc_IncorrectAlignment) {
  const size_t allocationSize = 10u;

  const auto result = mem_alloc(alloc, 3u, allocationSize);

  POINTERS_EQUAL(nullptr, result);
  CHECK_EQUAL(0, mem_size(alloc));
  CHECK_EQUAL(POOL_SIZE, mem_capacity(alloc));
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given a pointer to an allocator (alloc_t) based on memory pool, with no
///        prior allocations
///
/// \When mem_alloc() is called with zero alignment and size that's greater than
///       memory pool's buffer size
///
/// \Then a null pointer is returned, nothing is changed,
///       ERRNUM_NOMEM error number is set
TEST(Util_MemPool, MemPool_Alloc_OutOfMemory) {
  const size_t allocationSize = POOL_SIZE + 1u;

  const auto result = mem_alloc(alloc, 0, allocationSize);

  POINTERS_EQUAL(nullptr, result);
  CHECK_EQUAL(0, mem_size(alloc));
  CHECK_EQUAL(POOL_SIZE, mem_capacity(alloc));
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}

/// \Given a pointer to an allocator (alloc_t) based on memory pool, with no
///        prior allocations
///
/// \When mem_alloc() is called with zero alignment and zero size
///
/// \Then a null pointer is returned, nothing is changed;
///       no error is reported
TEST(Util_MemPool, MemPool_Alloc_SizeZero) {
  set_errnum(ERRNUM_SUCCESS);

  const auto result = mem_alloc(alloc, 0, 0);

  POINTERS_EQUAL(nullptr, result);
  CHECK_EQUAL(0, mem_size(alloc));
  CHECK_EQUAL(POOL_SIZE, mem_capacity(alloc));
  CHECK_EQUAL(0, get_errnum());
}

///@}

/// @name mem_free()

/// \Given pointers to an allocator (alloc_t) based on memory pool and some
///        allocated memory
///
/// \When mem_free() is called with a pointer to allocated memory
///
/// \Then nothing is changed
TEST(Util_MemPool, MemPool_Free_DoesNothing) {
  const size_t allocationSize = 10u;
  const auto result = mem_alloc(alloc, 0, allocationSize);

  mem_free(alloc, result);

  POINTERS_EQUAL(memory, result);
  CHECK_EQUAL(allocationSize, mem_size(alloc));
  CHECK_EQUAL(POOL_SIZE - allocationSize, mem_capacity(alloc));
}

///@}
