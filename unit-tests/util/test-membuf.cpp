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

#include <lely/util/membuf.h>
#include <lely/util/error.h>

TEST_GROUP(Util_MemBuf_Init) {
  membuf buf_;
  membuf* buf;

  TEST_SETUP() { buf = &buf_; }

  TEST_TEARDOWN() { membuf_fini(buf); }
};

TEST(Util_MemBuf_Init, MemBuf_Init) {
  membuf_init(buf, nullptr, 0);

  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_EQUAL(0u, membuf_capacity(buf));
  POINTERS_EQUAL(nullptr, membuf_begin(buf));
}

TEST(Util_MemBuf_Init, MemBuf_Init_Macro) {
  *buf = MEMBUF_INIT;

  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_EQUAL(0u, membuf_capacity(buf));
  POINTERS_EQUAL(nullptr, membuf_begin(buf));
}

TEST(Util_MemBuf_Init, MemBuf_Init_ExistingMemory) {
  const size_t CAPACITY = 5u;
  uint8_t memory[CAPACITY] = {0};

  membuf_init(buf, &memory, CAPACITY);

  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_EQUAL(CAPACITY, membuf_capacity(buf));
  POINTERS_EQUAL(memory, membuf_begin(buf));

  // membuf_fini() will try to free the memory. Re-initialize the buffer to
  // prevent a segfault.
  membuf_init(buf, nullptr, 0);
}

#if !LELY_NO_MALLOC
TEST(Util_MemBuf_Init, MemBuf_Reserve_InitialReserve) {
  membuf_init(buf, nullptr, 0);

  const auto ret = membuf_reserve(buf, 1);

  CHECK_COMPARE(ret, >, 0);
  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_COMPARE(0u, <, membuf_capacity(buf));
  CHECK(membuf_begin(buf) != nullptr);
}
#endif

TEST_GROUP(Util_MemBuf) {
  membuf buf_;
  membuf* buf;
  static const size_t CAPACITY = 16u;  // default capacity in membuf.c
#if LELY_NO_MALLOC
  char memory[CAPACITY];
#endif

  TEST_SETUP() {
    buf = &buf_;
#if LELY_NO_MALLOC
    membuf_init(buf, &memory, CAPACITY);
#else
    membuf_init(buf, nullptr, 0);
    membuf_reserve(buf, CAPACITY);
#endif
    memset(membuf_begin(buf), 0xddu, CAPACITY);
  }

  TEST_TEARDOWN() { membuf_fini(buf); }
};

TEST(Util_MemBuf, MemBuf_Alloc_InEmpty) {
  const size_t REQUIRED_SIZE = CAPACITY / 4u;
  size_t size = REQUIRED_SIZE;

  const auto ret = membuf_alloc(buf, &size);

  POINTERS_EQUAL(membuf_begin(buf), ret);
  CHECK_EQUAL(REQUIRED_SIZE, size);
  CHECK_EQUAL(REQUIRED_SIZE, membuf_size(buf));
  CHECK_EQUAL(CAPACITY - REQUIRED_SIZE, membuf_capacity(buf));
}

TEST(Util_MemBuf, MemBuf_Alloc_InNotEmpty) {
  const size_t REQUIRED_SIZE = CAPACITY / 4u;
  size_t size = REQUIRED_SIZE;

  const auto ret1 = membuf_alloc(buf, &size);
  const auto ret2 = membuf_alloc(buf, &size);

  POINTERS_EQUAL(membuf_begin(buf), ret1);
  POINTERS_EQUAL(reinterpret_cast<char*>(membuf_begin(buf)) + REQUIRED_SIZE,
                 ret2);
  CHECK_EQUAL(REQUIRED_SIZE, size);
  CHECK_EQUAL(2 * REQUIRED_SIZE, membuf_size(buf));
  CHECK_EQUAL(CAPACITY - 2 * REQUIRED_SIZE, membuf_capacity(buf));
}

TEST(Util_MemBuf, MemBuf_Alloc_AllCapacity) {
  const size_t REQUIRED_SIZE = CAPACITY / 2u;
  size_t size = REQUIRED_SIZE;

  const auto ret1 = membuf_alloc(buf, &size);
  const auto ret2 = membuf_alloc(buf, &size);

  CHECK_EQUAL(REQUIRED_SIZE, size);
  CHECK_EQUAL(CAPACITY, membuf_size(buf));
  CHECK_EQUAL(0, membuf_capacity(buf));
  CHECK(ret1 != nullptr);
  CHECK(ret2 != nullptr);
}

TEST(Util_MemBuf, MemBuf_Alloc_NotEnoughSpace) {
  const size_t LEFT_OVER_SPACE = 2u;
  const size_t REQUIRED_SIZE = CAPACITY - LEFT_OVER_SPACE;
  size_t size = REQUIRED_SIZE;

  const auto ret1 = membuf_alloc(buf, &size);
  const auto ret2 = membuf_alloc(buf, &size);

  CHECK_COMPARE(ret1, !=, ret2);
  CHECK_EQUAL(LEFT_OVER_SPACE, size);
  CHECK_EQUAL(CAPACITY, membuf_size(buf));
  CHECK_EQUAL(0, membuf_capacity(buf));
  CHECK(ret1 != nullptr);
  CHECK(ret2 != nullptr);
}

TEST(Util_MemBuf, MemBuf_Clear_Empty) {
  membuf_clear(buf);

  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_EQUAL(CAPACITY, membuf_capacity(buf));
}

TEST(Util_MemBuf, MemBuf_Clear_NotEmpty) {
  size_t size = 10u;
  membuf_alloc(buf, &size);

  membuf_clear(buf);

  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_EQUAL(CAPACITY, membuf_capacity(buf));
}

TEST(Util_MemBuf, MemBuf_Seek_Forward) {
  const ptrdiff_t OFFSET = 5;

  const auto ret = membuf_seek(buf, OFFSET);

  CHECK_EQUAL(OFFSET, ret);
  CHECK_EQUAL(OFFSET, membuf_size(buf));
  CHECK_EQUAL(CAPACITY - OFFSET, membuf_capacity(buf));
}

TEST(Util_MemBuf, MemBuf_Seek_Backward) {
  const ptrdiff_t OFFSET = 5;
  size_t size = OFFSET;
  membuf_alloc(buf, &size);

  const auto ret = membuf_seek(buf, -OFFSET);

  CHECK_EQUAL(-OFFSET, ret);
  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_EQUAL(CAPACITY, membuf_capacity(buf));
}

TEST(Util_MemBuf, MemBuf_Seek_ForwardTooFar) {
  size_t size = 5u;
  membuf_alloc(buf, &size);

  const auto ret = membuf_seek(buf, CAPACITY);

  CHECK_EQUAL(CAPACITY - size, static_cast<size_t>(ret));
  CHECK_EQUAL(CAPACITY, membuf_size(buf));
  CHECK_EQUAL(0u, membuf_capacity(buf));
}

TEST(Util_MemBuf, MemBuf_Seek_BackwardTooFar) {
  size_t size = 5u;
  membuf_alloc(buf, &size);

  const auto ret = membuf_seek(buf, -size - 1);

  CHECK_EQUAL(size, static_cast<size_t>(-ret));
  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_EQUAL(CAPACITY, membuf_capacity(buf));
}

TEST(Util_MemBuf, MemBuf_Write_Zero) {
  const char DUMMY[] =
      "GCC produces false positive for null argument - nonnull + memcpy";

  const auto ret = membuf_write(buf, DUMMY, 0);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_EQUAL(CAPACITY, membuf_capacity(buf));
}

TEST(Util_MemBuf, MemBuf_Write_Few) {
  const char TEST[] = "test string";

  const auto ret = membuf_write(buf, TEST, sizeof(TEST));

  CHECK_EQUAL(sizeof(TEST), ret);
  CHECK_EQUAL(sizeof(TEST), membuf_size(buf));
  CHECK_EQUAL(CAPACITY - sizeof(TEST), membuf_capacity(buf));
  STRCMP_EQUAL(TEST, reinterpret_cast<char*>(membuf_begin(buf)));
}

TEST(Util_MemBuf, MemBuf_Write_Append) {
  const char TEST1[] = "test";
  const char TEST2[] = "other";
  membuf_write(buf, TEST1, sizeof(TEST1));

  const auto ret = membuf_write(buf, TEST2, sizeof(TEST2));

  CHECK_EQUAL(sizeof(TEST2), ret);
  CHECK_EQUAL(sizeof(TEST1) + sizeof(TEST2), membuf_size(buf));
  STRCMP_EQUAL(TEST2,
               reinterpret_cast<char*>(membuf_begin(buf)) + sizeof(TEST1));
}

TEST(Util_MemBuf, MemBuf_Write_TooBig) {
  const char TEST[] = "0123456789ABCDEF__________________";

  const auto ret = membuf_write(buf, TEST, sizeof(TEST));

  CHECK_EQUAL(CAPACITY, ret);
  CHECK_EQUAL(CAPACITY, membuf_size(buf));
  CHECK_EQUAL(0u, membuf_capacity(buf));
  STRNCMP_EQUAL(TEST, reinterpret_cast<char*>(membuf_begin(buf)), CAPACITY);
}

TEST(Util_MemBuf, MemBuf_Reserve_EnoughAlready) {
  const char TEST[] = "01234";
  membuf_write(buf, TEST, sizeof(TEST));
  const size_t REQUIRED_ADDITIONAL_SIZE = 5u;

  const auto ret = membuf_reserve(buf, REQUIRED_ADDITIONAL_SIZE);

  CHECK_EQUAL(CAPACITY - sizeof(TEST), ret);
  CHECK_EQUAL(ret, membuf_capacity(buf));
  CHECK_EQUAL(sizeof(TEST), membuf_size(buf));
  STRCMP_EQUAL(TEST, reinterpret_cast<char*>(membuf_begin(buf)));
}

TEST(Util_MemBuf, MemBuf_Reserve_AddNew) {
  const char TEST[] = "0123456789ABCDEF__________________";
  membuf_write(buf, TEST, sizeof(TEST));
  const size_t REQUIRED_ADDITIONAL_SIZE = 1u;

  const auto ret = membuf_reserve(buf, REQUIRED_ADDITIONAL_SIZE);

#if LELY_NO_MALLOC
  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, membuf_capacity(buf));
#else
  CHECK_COMPARE(ret, >=, REQUIRED_ADDITIONAL_SIZE);
  CHECK_EQUAL(ret, membuf_capacity(buf));
#endif
  CHECK_EQUAL(CAPACITY, membuf_size(buf));
  STRNCMP_EQUAL(TEST, reinterpret_cast<char*>(membuf_begin(buf)), CAPACITY);
}

TEST(Util_MemBuf, MemBuf_Flush_Empty) {
  membuf_flush(buf, CAPACITY);

  CHECK_EQUAL(0u, membuf_size(buf));
}

TEST(Util_MemBuf, MemBuf_Flush_All) {
  const char TEST[] = "test";
  membuf_write(buf, TEST, sizeof(TEST));

  membuf_flush(buf, CAPACITY);

  CHECK_EQUAL(0u, membuf_size(buf));
}

TEST(Util_MemBuf, MemBuf_Flush_Part) {
  const char TEST[] = "test";
  membuf_write(buf, TEST, sizeof(TEST));
  const size_t FLUSH_SIZE = sizeof(TEST) / 2u;

  membuf_flush(buf, FLUSH_SIZE);

  CHECK_EQUAL(sizeof(TEST) - FLUSH_SIZE, membuf_size(buf));
  STRCMP_EQUAL(TEST + FLUSH_SIZE, reinterpret_cast<char*>(membuf_begin(buf)));
}
