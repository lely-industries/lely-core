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

#include <lely/util/membuf.h>
#include <lely/util/error.h>

TEST_GROUP(Util_MemBuf_Init) {
  membuf buf_;
  membuf* buf;

  TEST_SETUP() { buf = &buf_; }

  TEST_TEARDOWN() { membuf_fini(buf); }
};

/// @name membuf_init()
///@{

/// \Given a pointer to uninitialized memory buffer (membuf)
/// \When membuf_init() is called with a null pointer to memory region and zero
///       size
/// \Then buffer is initialized, has zero size and capacity and its pointer to
///       first byte of the buffer is null
TEST(Util_MemBuf_Init, MemBuf_Init) {
  membuf_init(buf, nullptr, 0);

  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_EQUAL(0u, membuf_capacity(buf));
  POINTERS_EQUAL(nullptr, membuf_begin(buf));
}

///@}

/// @name MEMBUF_INIT
///@{

/// \Given a pointer to uninitialized memory buffer (membuf)
/// \When MEMBUF_INIT is used to initialize buffer
/// \Then buffer is initialized, has zero size and capacity and its pointer to
///       first byte of the buffer is null
TEST(Util_MemBuf_Init, MemBuf_InitMacro_Properties) {
  *buf = MEMBUF_INIT;

  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_EQUAL(0u, membuf_capacity(buf));
  POINTERS_EQUAL(nullptr, membuf_begin(buf));
}

/// \Given a pointer to uninitialized memory buffer (membuf)
/// \When MEMBUF_INIT is used to initialize buffer
/// \Then all buffer fields are initialized to null pointers
TEST(Util_MemBuf_Init, MemBuf_InitMacro_Fields) {
  *buf = MEMBUF_INIT;

  POINTERS_EQUAL(nullptr, buf->cur);
  POINTERS_EQUAL(nullptr, buf->begin);
  POINTERS_EQUAL(nullptr, buf->end);
}

///@}

/// @name membuf_init()
///@{

/// \Given a pointer to uninitialized memory buffer (membuf) and some memory
///        region
/// \When membuf_init() is called with a pointer to the region and its size
/// \Then buffer is initialized, has zero size, capacity equal to memory region
///       size and its pointer to first byte of the buffer points to provided
///       memory region
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

///@}

/// @name membuf_reserve()
///@{

#if !LELY_NO_MALLOC
/// \Given LElY_NO_MALLOC disabled; a pointer to memory buffer (membuf)
///        initialized with null pointer to memory region and size zero
/// \When membuf_reserve() is called with a non zero capacity
/// \Then new capacity larger than zero is returned, memory area the buffer is
///       based on is no longer null
TEST(Util_MemBuf_Init, MemBuf_Reserve_InitialReserve) {
  membuf_init(buf, nullptr, 0);

  const auto ret = membuf_reserve(buf, 1);

  CHECK_COMPARE(ret, >, 0);
  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_COMPARE(0u, <, membuf_capacity(buf));
  CHECK(membuf_begin(buf) != nullptr);
}
#endif

///@}

TEST_GROUP(Util_MemBuf) {
  membuf buf_;
  membuf* buf;
  static const size_t CAPACITY = 16u;  // default capacity in membuf.c
#if LELY_NO_MALLOC
  char memory[CAPACITY];
#else
  void* memory;
#endif

  TEST_SETUP() {
    buf = &buf_;
#if LELY_NO_MALLOC
    membuf_init(buf, &memory, CAPACITY);
#else
    membuf_init(buf, nullptr, 0);
    membuf_reserve(buf, CAPACITY);
    memory = membuf_begin(buf);
#endif
    memset(memory, 0xddu, CAPACITY);
  }

  TEST_TEARDOWN() { membuf_fini(buf); }
};

/// @name membuf_alloc()
///@{

/// \Given a pointer to memory buffer (membuf)
/// \When membuf_alloc() is called with a pointer to non-zero size variable
/// \Then a pointer to reserved memory at buffer's beginning is returned, passed
///       size variable is not modified, the buffer's current position indicator
///       is moved by requested size and capacity is reduced by the same value
TEST(Util_MemBuf, MemBuf_Alloc_InEmpty) {
  const size_t REQUIRED_SIZE = CAPACITY / 4u;
  size_t size = REQUIRED_SIZE;

  const auto ret = membuf_alloc(buf, &size);

  POINTERS_EQUAL(membuf_begin(buf), ret);
  CHECK_EQUAL(REQUIRED_SIZE, size);
  CHECK_EQUAL(REQUIRED_SIZE, membuf_size(buf));
  CHECK_EQUAL(CAPACITY - REQUIRED_SIZE, membuf_capacity(buf));
}

/// \Given a pointer to memory buffer (membuf) with some memory already used
/// \When membuf_alloc() is called with a pointer to non-zero size variable
/// \Then a pointer to reserved memory in the buffer directly after end of
///       previous allocation is returned, passed size variable is not modified,
///       the buffer's current position indicator is moved by requested size and
///       capacity is reduced by the same value
TEST(Util_MemBuf, MemBuf_Alloc_InNotEmpty) {
  const size_t FIRST_ALLOC_SIZE = CAPACITY / 8u;
  const size_t REQUIRED_SIZE = CAPACITY / 4u;
  size_t size = FIRST_ALLOC_SIZE;
  membuf_alloc(buf, &size);
  size = REQUIRED_SIZE;

  const auto ret = membuf_alloc(buf, &size);

  POINTERS_EQUAL(reinterpret_cast<char*>(membuf_begin(buf)) + FIRST_ALLOC_SIZE,
                 ret);
  CHECK_EQUAL(REQUIRED_SIZE, size);
  CHECK_EQUAL(FIRST_ALLOC_SIZE + REQUIRED_SIZE, membuf_size(buf));
  CHECK_EQUAL(CAPACITY - FIRST_ALLOC_SIZE - REQUIRED_SIZE,
              membuf_capacity(buf));
}

/// \Given a pointer to memory buffer (membuf) with half memory already used
/// \When membuf_alloc() is called with a pointer to size variable equal to
///       remaining buffer capacity
/// \Then a pointer to reserved memory in the buffer directly after end of
///       previous allocation is returned, passed size variable is not modified,
///       the buffer's current position indicator is moved to the end of buffer
///       and there is no space left in the buffer
TEST(Util_MemBuf, MemBuf_Alloc_AllCapacity) {
  const size_t REQUIRED_SIZE = CAPACITY / 2u;
  size_t size = REQUIRED_SIZE;
  membuf_alloc(buf, &size);

  const auto ret = membuf_alloc(buf, &size);

  POINTERS_EQUAL(reinterpret_cast<char*>(membuf_begin(buf)) + REQUIRED_SIZE,
                 ret);
  CHECK_EQUAL(REQUIRED_SIZE, size);
  CHECK_EQUAL(CAPACITY, membuf_size(buf));
  CHECK_EQUAL(0, membuf_capacity(buf));
}

/// \Given a pointer to memory buffer (membuf) with most memory already used
/// \When membuf_alloc() is called with a pointer to size variable greater than
///       remaining buffer capacity
/// \Then a pointer to reserved memory in the buffer directly after end of
///       previous allocation is returned, passed size variable is changed to
///       space size available before the call, the buffer's current position
///       indicator is moved to the end of buffer and there is no space left in
///       the buffer
TEST(Util_MemBuf, MemBuf_Alloc_NotEnoughSpace) {
  const size_t LEFT_OVER_SPACE = 2u;
  const size_t REQUIRED_SIZE = CAPACITY - LEFT_OVER_SPACE;
  size_t size = REQUIRED_SIZE;
  membuf_alloc(buf, &size);

  const auto ret = membuf_alloc(buf, &size);

  POINTERS_EQUAL(reinterpret_cast<char*>(membuf_begin(buf)) + REQUIRED_SIZE,
                 ret);
  CHECK_EQUAL(LEFT_OVER_SPACE, size);
  CHECK_EQUAL(CAPACITY, membuf_size(buf));
  CHECK_EQUAL(0, membuf_capacity(buf));
}

///@}

/// @name membuf_clear()
///@{

/// \Given a pointer to memory buffer (membuf)
/// \When membuf_clear() is called
/// \Then nothing is changed
TEST(Util_MemBuf, MemBuf_Clear_Empty) {
  membuf_clear(buf);

  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_EQUAL(CAPACITY, membuf_capacity(buf));
}

/// \Given a pointer to memory buffer (membuf) with some memory already used
/// \When membuf_clear() is called()
/// \Then the buffer's current position indicator is moved to the beginning and
///       full capacity is available
TEST(Util_MemBuf, MemBuf_Clear_NotEmpty) {
  size_t size = 10u;
  membuf_alloc(buf, &size);

  membuf_clear(buf);

  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_EQUAL(CAPACITY, membuf_capacity(buf));
}

///@}

/// @name membuf_seek()
///@{

/// \Given a pointer to memory buffer (membuf)
/// \When membuf_seek() is called with some positive offset less than
///       remaining buffer's capacity
/// \Then requested offset value is returned; the buffer's current position
///       indicator is moved by requested offset and capacity is reduced by the
///       same value
TEST(Util_MemBuf, MemBuf_Seek_Forward) {
  const ptrdiff_t OFFSET = 5;

  const auto ret = membuf_seek(buf, OFFSET);

  CHECK_EQUAL(OFFSET, ret);
  CHECK_EQUAL(OFFSET, membuf_size(buf));
  CHECK_EQUAL(CAPACITY - OFFSET, membuf_capacity(buf));
}

/// \Given a pointer to memory buffer (membuf) with some memory already used
/// \When membuf_seek() is called with a negative offset of absolute value equal
///       to already allocated size
/// \Then requested negative offset value is returned; the buffer is rewound to
///       its beginning and full capacity is available
TEST(Util_MemBuf, MemBuf_Seek_Backward) {
  const ptrdiff_t OFFSET = 5;
  size_t size = OFFSET;
  membuf_alloc(buf, &size);

  const auto ret = membuf_seek(buf, -OFFSET);

  CHECK_EQUAL(-OFFSET, ret);
  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_EQUAL(CAPACITY, membuf_capacity(buf));
}

/// \Given a pointer to memory buffer (membuf) with some memory already used
/// \When membuf_seek() is called with some positive offset value greater than
///       left buffer capacity
/// \Then remaining capacity from before the call is returned; the buffer's
///       current position indicator is moved to the end of buffer and there is
///       no more capacity left
TEST(Util_MemBuf, MemBuf_Seek_ForwardTooFar) {
  size_t size = 5u;
  membuf_alloc(buf, &size);

  const auto ret = membuf_seek(buf, CAPACITY);

  CHECK_EQUAL(CAPACITY - size, static_cast<size_t>(ret));
  CHECK_EQUAL(CAPACITY, membuf_size(buf));
  CHECK_EQUAL(0u, membuf_capacity(buf));
}

/// \Given a pointer to memory buffer (membuf) with some memory already used
/// \When membuf_seek() is called with a negative offset of absolute value
///                     greater than already allocated size
/// \Then allocated size from before the call is returned; buffer is rewound to
///       its beginning and full capacity is available
TEST(Util_MemBuf, MemBuf_Seek_BackwardTooFar) {
  size_t size = 5u;
  membuf_alloc(buf, &size);

  const auto ret = membuf_seek(buf, -size - 1);

  CHECK_EQUAL(size, static_cast<size_t>(-ret));
  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_EQUAL(CAPACITY, membuf_capacity(buf));
}

///@}

/// @name membuf_write()
///@{

/// \Given a pointer to memory buffer (membuf)
/// \When membuf_write() is called with a non-null pointer to some data and size
///       equal zero
/// \Then zero is returned, nothing is changed
TEST(Util_MemBuf, MemBuf_Write_Zero) {
  const char DUMMY[] =
      "GCC produces false positive for null argument - nonnull + memcpy";

  const auto ret = membuf_write(buf, DUMMY, 0);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_EQUAL(CAPACITY, membuf_capacity(buf));
}

/// \Given a pointer to memory buffer (membuf)
/// \When membuf_write() is called with a non-null pointer to some data and
///       data's size
/// \Then requested data size is returned; requested data was written to the
///       buffer and the buffer's current position indicator is moved by
///       requested size and capacity is reduced by the same value
TEST(Util_MemBuf, MemBuf_Write_Few) {
  const char TEST[] = "test string";

  const auto ret = membuf_write(buf, TEST, sizeof(TEST));

  CHECK_EQUAL(sizeof(TEST), ret);
  CHECK_EQUAL(sizeof(TEST), membuf_size(buf));
  CHECK_EQUAL(CAPACITY - sizeof(TEST), membuf_capacity(buf));
  STRCMP_EQUAL(TEST, reinterpret_cast<char*>(membuf_begin(buf)));
}

/// \Given a pointer to memory buffer (membuf) with some data already written
/// \When membuf_write() is called with a non-null pointer to some data and
///       data's size
/// \Then requested data size is returned; requested data was written to the
///       buffer directly after previously written data; buffer's current
///       position indicator points at one past just written data
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

/// \Given a pointer to memory buffer (membuf)
/// \When membuf_write() is called with a non-null pointer to some data and its
///       size, both exceeding the buffer's capacity
/// \Then the buffer's remaining capacity from before the call is returned, data
///       was written into the buffer until its end, rest was discarded and
///       there is no space left in the buffer
TEST(Util_MemBuf, MemBuf_Write_TooBig) {
  const char TEST[] = "0123456789ABCDEF__________________";

  const auto ret = membuf_write(buf, TEST, sizeof(TEST));

  CHECK_EQUAL(CAPACITY, ret);
  CHECK_EQUAL(CAPACITY, membuf_size(buf));
  CHECK_EQUAL(0u, membuf_capacity(buf));
  STRNCMP_EQUAL(TEST, reinterpret_cast<char*>(membuf_begin(buf)), CAPACITY);
}

///@}

/// @name membuf_reserve()
///@{

/// \Given a pointer to memory buffer (membuf) with some data already written
/// \When membuf_reserve() is called with a non zero capacity less than or equal
///       to remaining buffer capacity
/// \Then buffer's initial capacity is returned, nothing is changed
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

#if !LELY_NO_MALLOC
/// \Given LELY_NO_MALLOC disabled; a pointer to memory buffer (membuf) with no
///        remaining capacity
/// \When membuf_reserve() is called with some non-zero capacity
/// \Then a value greater than or equal to requested additional capacity is
///       returned, same value is the new buffer's capacity, buffer's current
///       position indicator was not changed and data already written is left
///       intact
TEST(Util_MemBuf, MemBuf_Reserve_AddNew_MallocMode) {
  const char TEST[] = "0123456789ABCDEF__________________";
  membuf_write(buf, TEST, sizeof(TEST));
  const size_t REQUIRED_ADDITIONAL_SIZE = 1u;

  const auto ret = membuf_reserve(buf, REQUIRED_ADDITIONAL_SIZE);

  CHECK_COMPARE(ret, >=, REQUIRED_ADDITIONAL_SIZE);
  CHECK_EQUAL(ret, membuf_capacity(buf));
  CHECK_EQUAL(CAPACITY, membuf_size(buf));
  STRNCMP_EQUAL(TEST, reinterpret_cast<char*>(membuf_begin(buf)), CAPACITY);
}
#endif

#if LELY_NO_MALLOC
/// \Given LELY_NO_MALLOC enabled; a pointer to memory buffer (membuf) with no
///        remaining capacity
/// \When membuf_reserve() is called with some non-zero capacity
/// \Then 0 is returned, nothing is changed;
///       if !LELY_NO_ERRNO no memory error is reported
TEST(Util_MemBuf, MemBuf_Reserve_AddNew_NoMallocMode) {
  const char TEST[] = "0123456789ABCDEF__________________";
  membuf_write(buf, TEST, sizeof(TEST));
  const size_t REQUIRED_ADDITIONAL_SIZE = 1u;

  const auto ret = membuf_reserve(buf, REQUIRED_ADDITIONAL_SIZE);

  CHECK_EQUAL(0, ret);
#if !LELY_NO_ERRNO
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
#endif
  CHECK_EQUAL(0, membuf_capacity(buf));
  CHECK_EQUAL(CAPACITY, membuf_size(buf));
  STRNCMP_EQUAL(TEST, reinterpret_cast<char*>(membuf_begin(buf)), CAPACITY);
}
#endif

///@}

/// @name membuf_flush()
///@{

/// \Given a pointer to memory buffer (membuf)
/// \When membuf_flush() is called with some size
/// \Then nothing is changed
TEST(Util_MemBuf, MemBuf_Flush_Empty) {
  membuf_flush(buf, CAPACITY);

  CHECK_EQUAL(0u, membuf_size(buf));
}

/// \Given a pointer to memory buffer (membuf) with some data already written
/// \When membuf_flush() is called with size greater than or equal to the size
///       of already written data
/// \Then already written data is removed, the buffer is empty and at full
///       capacity
TEST(Util_MemBuf, MemBuf_Flush_All) {
  const char TEST[] = "test";
  membuf_write(buf, TEST, sizeof(TEST));

  membuf_flush(buf, CAPACITY);

  CHECK_EQUAL(0u, membuf_size(buf));
  CHECK_EQUAL(CAPACITY, membuf_capacity(buf));
}

/// \Given a pointer to memory buffer (membuf) with some data already written
/// \When membuf_flush() is called with non-zero size less than the size of
///       already written data
/// \Then part of already written data after requested flush size is moved to
///       the beginning of the buffer, remaining capacity is increased by
///       requested flush size
TEST(Util_MemBuf, MemBuf_Flush_Part) {
  const char TEST[] = "test";
  membuf_write(buf, TEST, sizeof(TEST));
  const size_t FLUSH_SIZE = sizeof(TEST) / 2u;

  membuf_flush(buf, FLUSH_SIZE);

  CHECK_EQUAL(sizeof(TEST) - FLUSH_SIZE, membuf_size(buf));
  STRCMP_EQUAL(TEST + FLUSH_SIZE, reinterpret_cast<char*>(membuf_begin(buf)));
  CHECK_EQUAL(CAPACITY - (sizeof(TEST) - FLUSH_SIZE), membuf_capacity(buf));
}

///@}

/// @name membuf_begin()
///@{

/// \Given a pointer to memory buffer (membuf)
/// \When membuf_begin() is called before and after writing some data to it
/// \Then a pointer to the buffer's beginning is returned from both calls
TEST(Util_MemBuf, MemBuf_Begin) {
  const void* const expected_begin = memory;

  POINTERS_EQUAL(expected_begin, membuf_begin(buf));

  const char data[] = "data";
  membuf_write(buf, data, sizeof(data));

  POINTERS_EQUAL(expected_begin, membuf_begin(buf));
}

///@}

/// @name membuf_size()
///@{

/// \Given a pointer to memory buffer (membuf)
/// \When membuf_size() is called before and after writing some data to it
/// \Then zero is returned from the first call and written data size from the
///       second call
TEST(Util_MemBuf, MemBuf_Size) {
  CHECK_EQUAL(0u, membuf_size(buf));

  const char data[] = "data";
  membuf_write(buf, data, sizeof(data));

  CHECK_EQUAL(sizeof(data), membuf_size(buf));
}

///@}

/// @name membuf_capacity()
///@{

/// \Given a pointer to memory buffer (membuf)
/// \When membuf_capacity() is called before and after writing some data to it
/// \Then the buffer's full capacity is returned from the first call and then
///       reduced by written data size from the second call
TEST(Util_MemBuf, MemBuf_Capacity) {
  CHECK_EQUAL(CAPACITY, membuf_capacity(buf));

  const char data[] = "data";
  membuf_write(buf, data, sizeof(data));

  CHECK_EQUAL(CAPACITY - sizeof(data), membuf_capacity(buf));
}

///@}
