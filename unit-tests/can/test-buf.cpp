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

#include <algorithm>
#include <string>

#include <CppUTest/TestHarness.h>

#include <lely/can/buf.h>
#include <lely/util/error.h>

TEST_GROUP(CAN_BufInit) {
  can_buf buf_;  // default-initialized (indeterminate value)
  can_buf* buf = &buf_;

  TEST_TEARDOWN() { can_buf_fini(buf); }
};

/// @name CAN_BUF_INIT
///@{

/// \Given a pointer to an uninitialized CAN frame buffer (can_buf)
///
/// \When CAN_BUF_INIT is used to initialize the buffer
///
/// \Then the buffer is initialized, has a null pointer for frame storage,
///       zeroed size and offsets
TEST(CAN_BufInit, StaticInitializer) {
  *buf = CAN_BUF_INIT;

  POINTERS_EQUAL(nullptr, buf->ptr);
  CHECK_EQUAL(0, buf->size);
  CHECK_EQUAL(0, buf->begin);
  CHECK_EQUAL(0, buf->end);
}

///@}

/// @name can_buf_init()
///@{

/// \Given a pointer to an uninitialized CAN frame buffer (can_buf)
///
/// \When can_buf_init() is called with a null pointer to memory region and zero
///       size
///
/// \Then the buffer is initialized, has a null pointer for frame storage,
///       zeroed size and offsets
TEST(CAN_BufInit, CanBufInit_NullAndZero) {
  can_buf_init(buf, nullptr, 0);

  POINTERS_EQUAL(nullptr, buf->ptr);
  CHECK_EQUAL(0, buf->size);
  CHECK_EQUAL(0, buf->begin);
  CHECK_EQUAL(0, buf->end);
}

#if LELY_NO_MALLOC
/// \Given a pointer to an uninitialized CAN frame buffer (can_buf) and some
///        memory region for storing frames
///
/// \When can_buf_init() is called with a pointer to the memory region and its
///       size
///
/// \Then the buffer is initialized, has size equal to the memory region's size
///       minus 1, pointer to requested region and is empty
TEST(CAN_BufInit, CanBufInit) {
  const size_t BUFFER_SIZE = 32;
  can_msg memory[BUFFER_SIZE];

  can_buf_init(buf, memory, BUFFER_SIZE);

  CHECK_EQUAL(BUFFER_SIZE - 1, buf->size);
  CHECK_EQUAL(memory, buf->ptr);
  CHECK_EQUAL(0, buf->begin);
  CHECK_EQUAL(0, buf->end);
}
#endif  // LELY_NO_MALLOC

///@}

TEST_GROUP(CAN_Buf) {
  static const size_t BUF_SIZE = 15;
#if LELY_NO_MALLOC
  can_msg memory[BUF_SIZE + 1];
#endif
  can_buf buf = CAN_BUF_INIT;

  TEST_SETUP() {
#if LELY_NO_MALLOC
    can_buf_init(&buf, memory, BUF_SIZE + 1);  // +1 to make it power of 2
#else
    can_buf_init(&buf, nullptr, 0);
    CHECK_EQUAL(BUF_SIZE, can_buf_reserve(&buf, BUF_SIZE));
#endif
  }

  TEST_TEARDOWN() { can_buf_fini(&buf); }

  static void FillCanMsg(can_msg & msg, const uint_least32_t id,
                         const uint_least8_t len, const uint_least8_t val) {
    msg.id = id;
    msg.len = len;
    for (int i = 0; i < len; ++i) msg.data[i] = val;
  }

  static void CheckCanMsgTabs(const can_msg msg_arr1[],
                              const can_msg msg_arr2[], const size_t n) {
    for (size_t j = 0; j < n; ++j) {
      const std::string msg_str =
          "comparing msg_arr[" + std::to_string(j) + "]";
      CHECK_EQUAL_TEXT(msg_arr1[j].id, msg_arr2[j].id, msg_str.c_str());
      CHECK_EQUAL_TEXT(msg_arr1[j].len, msg_arr2[j].len, msg_str.c_str());
      MEMCMP_EQUAL_TEXT(msg_arr1[j].data, msg_arr2[j].data, CAN_MSG_MAX_LEN,
                        msg_str.c_str());
    }
  }
};

/// @name can_buf_write()
///@{

/// \Given a pointer to a CAN frame buffer (can_buf)
///
/// \When can_buf_write() is called with a null pointer to storage and 0 frames
///       to write
///
/// \Then 0 is returned, nothing is changed
TEST(CAN_Buf, CanBufWrite_ZeroFrames) {
  const auto frames_written = can_buf_write(&buf, nullptr, 0);

  CHECK_EQUAL(0, frames_written);
  CHECK_EQUAL(0, can_buf_size(&buf));
  CHECK_EQUAL(BUF_SIZE, can_buf_capacity(&buf));
}

/// \Given a pointer to an empty CAN frame buffer (can_buf)
///
/// \When can_buf_write() is called with a pointer to a single CAN frame and
///       number of frames to write equal to 1
///
/// \Then 1 is returned, the frame was written to the buffer, buffer's remaining
///       capacity got decreased by 1 and size increased by 1
TEST(CAN_Buf, CanBufWrite_OneFrame) {
  can_msg msg = CAN_MSG_INIT;
  FillCanMsg(msg, 0x77, 5, 0xaa);

  const auto frames_written = can_buf_write(&buf, &msg, 1);

  CHECK_EQUAL(1, frames_written);
  CHECK_EQUAL(1, can_buf_size(&buf));
  CHECK_EQUAL(BUF_SIZE - 1, can_buf_capacity(&buf));

  can_msg out_tab[BUF_SIZE + 1];
  std::fill_n(out_tab, BUF_SIZE + 1, can_msg(CAN_MSG_INIT));
  CHECK_EQUAL(1, can_buf_peek(&buf, out_tab, BUF_SIZE + 1));
  CheckCanMsgTabs(&msg, out_tab, 1);
}

/// \Given a pointer to an empty CAN frame buffer (can_buf) and an array of CAN
///        frames of size less than or equal to the buffer's capacity
///
/// \When can_buf_write() is called with the array and its size
///
/// \Then size of the array is returned, all requested frames were written to
///       the buffer, buffer's remaining capacity got decreased by number of
///       frames written and size increased by the same value
TEST(CAN_Buf, CanBufWrite_ManyFrames) {
  const size_t MSG_NUM = 3;
  can_msg msg_arr[MSG_NUM];

  std::fill_n(msg_arr, MSG_NUM, can_msg(CAN_MSG_INIT));
  FillCanMsg(msg_arr[0], 0x1d, 6, 0xa2);
  FillCanMsg(msg_arr[1], 0x2c, 3, 0xb4);
  FillCanMsg(msg_arr[2], 0x3b, 1, 0xc8);

  const auto frames_written = can_buf_write(&buf, msg_arr, MSG_NUM);

  CHECK_EQUAL(MSG_NUM, frames_written);
  CHECK_EQUAL(MSG_NUM, can_buf_size(&buf));
  CHECK_EQUAL(BUF_SIZE - MSG_NUM, can_buf_capacity(&buf));

  can_msg out_tab[BUF_SIZE + 1];
  std::fill_n(out_tab, BUF_SIZE + 1, can_msg(CAN_MSG_INIT));
  CHECK_EQUAL(MSG_NUM, can_buf_peek(&buf, out_tab, BUF_SIZE + 1));
  CheckCanMsgTabs(msg_arr, out_tab, MSG_NUM);
}

/// \Given a pointer to a CAN frame buffer (can_buf) and an array of CAN frames
///        of size greater than the buffer's capacity
///
/// \When can_buf_write() is called with the array and its size
///
/// \Then the buffer's capacity from before the call is returned, same number of
///       frames were written to the buffer, there is no space left in the
///       buffer
TEST(CAN_Buf, CanBufWrite_TooManyFrames) {
  const size_t MSG_NUM = BUF_SIZE + 1;
  can_msg msg_arr[MSG_NUM];

  std::fill_n(msg_arr, MSG_NUM, can_msg(CAN_MSG_INIT));
  FillCanMsg(msg_arr[0], 0x4d, 2, 0xc2);
  FillCanMsg(msg_arr[14], 0x26, 7, 0xb0);
  FillCanMsg(msg_arr[15], 0x81, 4, 0x08);

  const auto frames_written = can_buf_write(&buf, msg_arr, MSG_NUM);

  CHECK_EQUAL(BUF_SIZE, frames_written);
  CHECK_EQUAL(BUF_SIZE, can_buf_size(&buf));
  CHECK_EQUAL(0, can_buf_capacity(&buf));

  can_msg out_tab[BUF_SIZE + 1];
  std::fill_n(out_tab, BUF_SIZE + 1, can_msg(CAN_MSG_INIT));
  CHECK_EQUAL(BUF_SIZE, can_buf_peek(&buf, out_tab, BUF_SIZE + 1));
  CheckCanMsgTabs(msg_arr, out_tab, 15);
}

///@}

/// @name can_buf_clear()
///@{

/// \Given a pointer to an empty CAN frame buffer (can_buf)
///
/// \When can_buf_clear() is called
///
/// \Then nothing is changed
TEST(CAN_Buf, CanBufClear_Empty) {
  can_buf_clear(&buf);

  CHECK_EQUAL(0, can_buf_size(&buf));
  CHECK_EQUAL(BUF_SIZE, can_buf_capacity(&buf));
}

/// \Given a pointer to a CAN frame buffer (can_buf) with some frames stored
///
/// \When can_buf_clear() is called
///
/// \Then the buffer is empty and at full capacity
TEST(CAN_Buf, CanBufClear_NonEmpty) {
  const size_t MSG_NUM = 5;
  can_msg msg_arr[MSG_NUM];

  std::fill_n(msg_arr, MSG_NUM, can_msg(CAN_MSG_INIT));
  can_buf_write(&buf, msg_arr, MSG_NUM);

  can_buf_clear(&buf);

  CHECK_EQUAL(0, can_buf_size(&buf));
  CHECK_EQUAL(BUF_SIZE, can_buf_capacity(&buf));
}

///@}

/// @name can_buf_peek()
///@{

/// \Given a pointer to a CAN frame buffer (can_buf) with some frames stored
///
/// \When can_buf_peek() is called with a null pointer to storage and a number
///       of frames requested less than or equal to the number of frames stored
///
/// \Then number of requested frames is returned, nothing is changed
TEST(CAN_Buf, CanBufPeek_NullPtr) {
  const size_t MSG_NUM = 4;
  can_msg msg_arr[MSG_NUM];

  std::fill_n(msg_arr, MSG_NUM, can_msg(CAN_MSG_INIT));
  can_buf_write(&buf, msg_arr, MSG_NUM);

  const auto frames_read = can_buf_peek(&buf, nullptr, 3);

  CHECK_EQUAL(3, frames_read);
  CHECK_EQUAL(MSG_NUM, can_buf_size(&buf));
}

/// \Given a pointer to a CAN frame buffer (can_buf) with some frames stored
///
/// \When can_buf_peek() is called with a null pointer to storage and a number
///       of frames requested greater than the number of frames stored
///
/// \Then number of stored frames is returned, nothing is changed
TEST(CAN_Buf, CanBufPeek_NullPtr_ManyFrames) {
  const size_t MSG_NUM = 4;
  can_msg msg_arr[MSG_NUM];

  std::fill_n(msg_arr, MSG_NUM, can_msg(CAN_MSG_INIT));
  can_buf_write(&buf, msg_arr, MSG_NUM);

  const auto frames_read = can_buf_peek(&buf, nullptr, MSG_NUM + 1);

  CHECK_EQUAL(MSG_NUM, frames_read);
  CHECK_EQUAL(MSG_NUM, can_buf_size(&buf));
}

/// \Given a pointer to a CAN frame buffer (can_buf) with some frames stored and
///        a memory region of size greater than or equal to the frame buffer's
///        size
///
/// \When can_buf_peek() is called with a pointer to the memory region and its
///       size
///
/// \Then number of stored frames is returned, the memory region contains all
///       read frames, the buffer's state is not changed
TEST(CAN_Buf, CanBufPeek_ManyFrames) {
  const size_t MSG_NUM = 3;
  can_msg msg_arr[MSG_NUM];
  std::fill_n(msg_arr, MSG_NUM, can_msg(CAN_MSG_INIT));
  FillCanMsg(msg_arr[0], 0x1d, 6, 0xa2);
  FillCanMsg(msg_arr[1], 0x2c, 3, 0xb4);
  FillCanMsg(msg_arr[2], 0x3b, 1, 0xc8);
  CHECK_EQUAL(MSG_NUM, can_buf_write(&buf, msg_arr, MSG_NUM));

  can_msg out_tab[BUF_SIZE + 1];
  std::fill_n(out_tab, BUF_SIZE + 1, can_msg(CAN_MSG_INIT));
  const auto frames_read = can_buf_peek(&buf, out_tab, BUF_SIZE + 1);

  CHECK_EQUAL(MSG_NUM, frames_read);
  CHECK_EQUAL(MSG_NUM, can_buf_size(&buf));
  CHECK_EQUAL(BUF_SIZE - MSG_NUM, can_buf_capacity(&buf));
  CheckCanMsgTabs(msg_arr, out_tab, MSG_NUM);
}

///@}

/// @name can_buf_reserve()
///@{

#if !LELY_NO_MALLOC
/// \Given LELY_NO_MALLOC disabled; a pointer to a CAN frame buffer (can_buf)
///        with some frames stored
///
/// \When can_buf_reserve() is called with a value greater than the buffer's
///       remaining capacity
///
/// \Then a value greater than original capacity is returned, new space in the
///       buffer is allocated, remaining capacity is increased and already
///       stored frames are preserved
TEST(CAN_Buf, CanBufReserve_Enlarge_MallocMode) {
  const size_t MSG_NUM = 8;
  can_msg msg_arr[MSG_NUM];

  std::fill_n(msg_arr, MSG_NUM, can_msg(CAN_MSG_INIT));
  can_buf_write(&buf, msg_arr, MSG_NUM);

  const auto capacity = can_buf_reserve(&buf, BUF_SIZE - MSG_NUM + 1);

  CHECK_EQUAL(31 - MSG_NUM, capacity);  // (new_buffer_size - MSG_NUM)
  CHECK_EQUAL(31 - MSG_NUM, can_buf_capacity(&buf));
  CHECK_EQUAL(MSG_NUM, can_buf_size(&buf));
}
#endif

#if LELY_NO_MALLOC
/// \Given LELY_NO_MALLOC enabled; a pointer to a CAN frame buffer (can_buf)
///        with some frames stored
///
/// \When can_buf_reserve() is called with a value greater than the buffer's
///       remaining capacity
///
/// \Then 0 is returned, nothing is changed;
///       if !LELY_NO_ERRNO no memory error is reported
TEST(CAN_Buf, CanBufReserve_Enlarge_NoMallocMode) {
  const size_t MSG_NUM = 8;
  can_msg msg_arr[MSG_NUM];

  std::fill_n(msg_arr, MSG_NUM, can_msg(CAN_MSG_INIT));
  can_buf_write(&buf, msg_arr, MSG_NUM);

  const auto capacity = can_buf_reserve(&buf, BUF_SIZE - MSG_NUM + 1);

  CHECK_EQUAL(0, capacity);
  CHECK_EQUAL(BUF_SIZE - MSG_NUM, can_buf_capacity(&buf));
  CHECK_EQUAL(MSG_NUM, can_buf_size(&buf));
#if !LELY_NO_ERRNO
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
#endif
}
#endif

/// \Given a pointer to a CAN frame buffer (can_buf) with some frames stored
///
/// \When can_buf_reserve() is called with a value less than or equal to the
///       buffer's remaining capacity
///
/// \Then buffer capacity is returned, nothing is changed
TEST(CAN_Buf, CanBufReserve_BigEnough) {
  const size_t MSG_NUM = 8;
  can_msg msg_arr[MSG_NUM];

  std::fill_n(msg_arr, MSG_NUM, can_msg(CAN_MSG_INIT));
  can_buf_write(&buf, msg_arr, MSG_NUM);

  const auto capacity = can_buf_reserve(&buf, BUF_SIZE - MSG_NUM);

  CHECK_EQUAL(BUF_SIZE - MSG_NUM, capacity);
  CHECK_EQUAL(BUF_SIZE - MSG_NUM, can_buf_capacity(&buf));
  CHECK_EQUAL(MSG_NUM, can_buf_size(&buf));
}

#if LELY_NO_MALLOC
/// \Given LELY_NO_MALLOC enabled; a pointer to an empty CAN frame buffer
///        (can_buf)
///
/// \When can_buf_reserve() is called with a value greater than the buffer's
///       remaining capacity
///
/// \Then 0 is returned, nothing is changed;
///       if !LELY_NO_ERRNO no memory error is reported
TEST(CAN_Buf, CanBufReserve_NoMemory) {
  const auto capacity = can_buf_reserve(&buf, 2 * BUF_SIZE);

  CHECK_EQUAL(0, capacity);
#if !LELY_NO_ERRNO
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
#endif
  CHECK_EQUAL(BUF_SIZE, can_buf_capacity(&buf));
  CHECK_EQUAL(0, can_buf_size(&buf));
}
#else
/// \Given LELY_NO_MALLOC disabled; a pointer to a CAN frame buffer (can_buf)
///        with some frames stored and some already read
///
/// \When can_buf_reserve() is called with a value greater than the buffer's
///       remaining capacity
///
/// \Then a value greater than original capacity is returned, new space in the
///       buffer is allocated, remaining capacity is increased, already stored
///       frames are preserved and all unread frames can be read in order as
///       they were originally written
TEST(CAN_Buf, CanBufReserve_Wrapping) {
  const size_t MSG_NUM = 15;
  can_msg msg_arr[MSG_NUM];

  std::fill_n(msg_arr, MSG_NUM, can_msg(CAN_MSG_INIT));
  FillCanMsg(msg_arr[0], 0xdd, 8, 0x01);
  FillCanMsg(msg_arr[5], 0x8a, 5, 0xf8);
  FillCanMsg(msg_arr[10], 0xef, 7, 0x34);

  can_buf_write(&buf, msg_arr, MSG_NUM);
  can_buf_read(&buf, nullptr, 10);
  can_buf_write(&buf, msg_arr, 6);
  const size_t NEW_MSG_NUM = MSG_NUM - 10 + 6;

  const auto capacity = can_buf_reserve(&buf, BUF_SIZE - NEW_MSG_NUM + 1);

  CHECK_EQUAL(31 - NEW_MSG_NUM, capacity);  // (new_buffer_size - NEW_MSG_NUM)
  CHECK_EQUAL(31 - NEW_MSG_NUM, can_buf_capacity(&buf));
  CHECK_EQUAL(NEW_MSG_NUM, can_buf_size(&buf));

  can_msg out_tab[BUF_SIZE + 1];
  std::fill_n(out_tab, BUF_SIZE + 1, can_msg(CAN_MSG_INIT));
  CHECK_EQUAL(NEW_MSG_NUM, can_buf_read(&buf, out_tab, BUF_SIZE + 1));
  CheckCanMsgTabs(msg_arr + 10, out_tab, 5);
  CheckCanMsgTabs(msg_arr, out_tab + 5, 6);
}
#endif  // LELY_NO_MALLOC

///@}

/// @name can_buf_read()
///@{

/// \Given a pointer to a CAN frame buffer (can_buf)
///
/// \When can_buf_read() is called with a null pointer to storage and zero
///       frames requested
///
/// \Then 0 is returned, nothing is changed
TEST(CAN_Buf, CanBufRead_ZeroFrames) {
  const auto frames_read = can_buf_read(&buf, nullptr, 0);

  CHECK_EQUAL(0, frames_read);
}

/// \Given a pointer to a CAN frame buffer (can_buf) with one frame stored and a
///        memory region to store read frames
///
/// \When can_buf_read() is called with a pointer to the memory region and its
///       size
///
/// \Then 1 is returned, there are no more frames in the buffer to be read, the
///       memory region contains the read frame
TEST(CAN_Buf, CanBufRead_OneFrame) {
  can_msg msg = CAN_MSG_INIT;
  FillCanMsg(msg, 0x77, 5, 0xaa);
  CHECK_EQUAL(1, can_buf_write(&buf, &msg, 1));

  can_msg out_tab[BUF_SIZE + 1];
  std::fill_n(out_tab, BUF_SIZE + 1, can_msg(CAN_MSG_INIT));
  const auto frames_read = can_buf_read(&buf, out_tab, BUF_SIZE + 1);

  CHECK_EQUAL(1, frames_read);
  CHECK_EQUAL(0, can_buf_size(&buf));
  CheckCanMsgTabs(&msg, out_tab, 1);
}

/// \Given a pointer to a CAN frame buffer (can_buf) with some frames stored and
///        a memory region of size greater than or equal to the frame buffer's
///        size
///
/// \When can_buf_read() is called with a pointer to the memory region and its
///       size
///
/// \Then number of stored frames is returned, there are no more frames in the
///       buffer to be read, buffer is at full capacity, the memory region
///       contains all read frames
TEST(CAN_Buf, CanBufRead_ManyFrames) {
  const size_t MSG_NUM = 3;
  can_msg msg_arr[MSG_NUM];
  std::fill_n(msg_arr, MSG_NUM, can_msg(CAN_MSG_INIT));
  FillCanMsg(msg_arr[0], 0x1d, 6, 0xa2);
  FillCanMsg(msg_arr[1], 0x2c, 3, 0xb4);
  FillCanMsg(msg_arr[2], 0x3b, 1, 0xc8);
  CHECK_EQUAL(MSG_NUM, can_buf_write(&buf, msg_arr, MSG_NUM));

  can_msg out_tab[BUF_SIZE + 1];
  std::fill_n(out_tab, BUF_SIZE + 1, can_msg(CAN_MSG_INIT));
  const auto frames_read = can_buf_read(&buf, out_tab, BUF_SIZE + 1);

  CHECK_EQUAL(MSG_NUM, frames_read);
  CHECK_EQUAL(0, can_buf_size(&buf));
  CHECK_EQUAL(BUF_SIZE, can_buf_capacity(&buf));
  CheckCanMsgTabs(msg_arr, out_tab, MSG_NUM);
}

/// \Given a pointer to a CAN frame buffer (can_buf) with some frames stored
///
/// \When can_buf_read() is called with a null pointer to storage and a number
///       of frames requested less than or equal to the number of frames stored
///
/// \Then number of requested frames is returned, those frames are removed from
///       the buffer
TEST(CAN_Buf, CanBufRead_NullPtr) {
  const size_t MSG_NUM = 4;
  can_msg msg_arr[MSG_NUM];
  std::fill_n(msg_arr, MSG_NUM, can_msg(CAN_MSG_INIT));
  can_buf_write(&buf, msg_arr, MSG_NUM);

  const auto frames_read = can_buf_read(&buf, nullptr, 3);

  CHECK_EQUAL(3, frames_read);
  CHECK_EQUAL(MSG_NUM - frames_read, can_buf_size(&buf));
}

///@}

/// @name can_buf_size()
///@{

/// \Given a pointer to an empty CAN frame buffer (can_buf)
///
/// \When can_buf_size() is called
///
/// \Then 0 is returned
TEST(CAN_Buf, CanBufSize_Empty) { CHECK_EQUAL(0, can_buf_size(&buf)); }

/// \Given a pointer to a CAN frame buffer (can_buf) with some frames stored
///
/// \When can_buf_size() is called
///
/// \Then number of frames stored is returned
TEST(CAN_Buf, CanBufSize_ManyFrames) {
  const size_t MSG_NUM = 4u;
  can_msg msg_arr[MSG_NUM];
  std::fill_n(msg_arr, MSG_NUM, can_msg(CAN_MSG_INIT));
  can_buf_write(&buf, msg_arr, MSG_NUM);

  CHECK_EQUAL(4u, can_buf_size(&buf));
}

/// \Given a pointer to a CAN frame buffer (can_buf) with no space left
///
/// \When can_buf_size() is called
///
/// \Then full initial buffer capacity is returned
TEST(CAN_Buf, CanBufSize_Full) {
  const size_t MSG_NUM = BUF_SIZE + 1;
  can_msg msg_arr[MSG_NUM];
  std::fill_n(msg_arr, MSG_NUM, can_msg(CAN_MSG_INIT));
  can_buf_write(&buf, msg_arr, MSG_NUM);

  CHECK_EQUAL(BUF_SIZE, can_buf_size(&buf));
}

///@}

/// @name can_buf_capacity()
///@{

/// \Given a pointer to an empty CAN frame buffer (can_buf)
///
/// \When can_buf_capacity() is called
///
/// \Then full buffer capacity is returned
TEST(CAN_Buf, CanBufCapacity_Empty) {
  CHECK_EQUAL(BUF_SIZE, can_buf_capacity(&buf));
}

/// \Given a pointer to a CAN frame buffer (can_buf) with some frames stored
///
/// \When can_buf_capacity() is called
///
/// \Then full buffer capacity decreased by number of frames stored is returned
TEST(CAN_Buf, CanBufCapacity_ManyFrames) {
  const size_t MSG_NUM = 4u;
  can_msg msg_arr[MSG_NUM];
  std::fill_n(msg_arr, MSG_NUM, can_msg(CAN_MSG_INIT));
  can_buf_write(&buf, msg_arr, MSG_NUM);

  CHECK_EQUAL(BUF_SIZE - 4u, can_buf_capacity(&buf));
}

/// \Given a pointer to a CAN frame buffer (can_buf) with no space left
///
/// \When can_buf_capacity() is called
///
/// \Then 0 is returned
TEST(CAN_Buf, CanBufCapacity_Full) {
  const size_t MSG_NUM = BUF_SIZE + 1;
  can_msg msg_arr[MSG_NUM];
  std::fill_n(msg_arr, MSG_NUM, can_msg(CAN_MSG_INIT));
  can_buf_write(&buf, msg_arr, MSG_NUM);

  CHECK_EQUAL(0u, can_buf_capacity(&buf));
}

///@}
