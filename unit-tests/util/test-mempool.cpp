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

#include <cstring>

#include <lely/util/mempool.h>
#include <lely/util/errnum.h>

TEST_GROUP(Util_MemPool) {
  static const size_t POOL_SIZE = 1024u;
  mempool pool;
  alloc_t* alloc = nullptr;
  char memory[POOL_SIZE] = {0};

  TEST_SETUP() { alloc = mempool_init(&pool, memory, POOL_SIZE); }
};

TEST(Util_MemPool, MemPool_Size) { CHECK_EQUAL(0, mem_size(alloc)); }

TEST(Util_MemPool, MemPool_Capacity) {
  CHECK_EQUAL(POOL_SIZE, mem_capacity(alloc));
}

TEST(Util_MemPool, MemPool_Alloc) {
  const size_t allocationSize = 10u;

  const auto result = mem_alloc(alloc, 0, allocationSize);

  POINTERS_EQUAL(memory, result);
  CHECK_EQUAL(allocationSize, mem_size(alloc));
  CHECK_EQUAL(POOL_SIZE - allocationSize, mem_capacity(alloc));
}

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

TEST(Util_MemPool, MemPool_Alloc_IncorrectAlignment) {
  const size_t allocationSize = 10u;

  const auto result = mem_alloc(alloc, 3u, allocationSize);

  POINTERS_EQUAL(NULL, result);
  CHECK_EQUAL(0, mem_size(alloc));
  CHECK_EQUAL(POOL_SIZE, mem_capacity(alloc));
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(Util_MemPool, MemPool_Alloc_OutOfMemory) {
  const size_t allocationSize = POOL_SIZE + 1u;

  const auto result = mem_alloc(alloc, 0, allocationSize);

  POINTERS_EQUAL(NULL, result);
  CHECK_EQUAL(0, mem_size(alloc));
  CHECK_EQUAL(POOL_SIZE, mem_capacity(alloc));
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}

TEST(Util_MemPool, MemPool_Alloc_SizeZero) {
  set_errnum(0);

  const auto result = mem_alloc(alloc, 0, 0);

  POINTERS_EQUAL(NULL, result);
  CHECK_EQUAL(0, mem_size(alloc));
  CHECK_EQUAL(POOL_SIZE, mem_capacity(alloc));
  CHECK_EQUAL(0, get_errnum());
}

TEST(Util_MemPool, MemPool_Free_DoeaNothing) {
  const size_t allocationSize = 10u;
  const auto result = mem_alloc(alloc, 0, allocationSize);

  mem_free(alloc, result);

  POINTERS_EQUAL(memory, result);
  CHECK_EQUAL(allocationSize, mem_size(alloc));
  CHECK_EQUAL(POOL_SIZE - allocationSize, mem_capacity(alloc));
}
