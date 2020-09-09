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

#include <algorithm>
#include <string>

#include <CppUTest/TestHarness.h>

#include <lely/can/buf.h>
#include <lely/util/errnum.h>

TEST_GROUP(CAN_BufInit) {
  can_buf buf = CAN_BUF_INIT;

  TEST_TEARDOWN() { can_buf_fini(&buf); }
};

TEST(CAN_BufInit, StaticInitializer) {
  POINTERS_EQUAL(nullptr, buf.ptr);
  CHECK_EQUAL(0, buf.size);
  CHECK_EQUAL(0, buf.begin);
  CHECK_EQUAL(0, buf.end);
}

#if LELY_NO_MALLOC
TEST(CAN_BufInit, CanBufInit) {
  const size_t BUFFER_SIZE = 32;
  can_msg memory[BUFFER_SIZE];

  can_buf_init(&buf, memory, BUFFER_SIZE);

  CHECK_EQUAL(BUFFER_SIZE - 1, buf.size);
}
#else
TEST(CAN_BufInit, CanBufInit) {
  can_buf_init(&buf, nullptr, 0);

  CHECK_EQUAL(0, buf.size);
}
#endif  // !LELY_NO_MALLOC

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

  static void CheckCanMsgTabs(const can_msg msg_tab1[],
                              const can_msg msg_tab2[], const size_t n) {
    for (size_t j = 0; j < n; ++j) {
      const std::string msg_str =
          "comparing msg_tab[" + std::to_string(j) + "]";
      CHECK_EQUAL_TEXT(msg_tab1[j].id, msg_tab2[j].id, msg_str.c_str());
      CHECK_EQUAL_TEXT(msg_tab1[j].len, msg_tab2[j].len, msg_str.c_str());
      MEMCMP_EQUAL_TEXT(msg_tab1[j].data, msg_tab2[j].data, CAN_MSG_MAX_LEN,
                        msg_str.c_str());
    }
  }
};

TEST(CAN_Buf, CanBufWrite_ZeroFrames) {
  const auto frames_written = can_buf_write(&buf, nullptr, 0);

  CHECK_EQUAL(0, frames_written);
  CHECK_EQUAL(0, can_buf_size(&buf));
  CHECK_EQUAL(BUF_SIZE, can_buf_capacity(&buf));
}

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

TEST(CAN_Buf, CanBufWrite_ManyFrames) {
  const size_t MSG_SIZE = 3;
  can_msg msg_tab[MSG_SIZE];

  std::fill_n(msg_tab, MSG_SIZE, can_msg(CAN_MSG_INIT));
  FillCanMsg(msg_tab[0], 0x1d, 6, 0xa2);
  FillCanMsg(msg_tab[1], 0x2c, 3, 0xb4);
  FillCanMsg(msg_tab[2], 0x3b, 1, 0xc8);

  const auto frames_written = can_buf_write(&buf, msg_tab, MSG_SIZE);

  CHECK_EQUAL(MSG_SIZE, frames_written);
  CHECK_EQUAL(MSG_SIZE, can_buf_size(&buf));
  CHECK_EQUAL(BUF_SIZE - MSG_SIZE, can_buf_capacity(&buf));

  can_msg out_tab[BUF_SIZE + 1];
  std::fill_n(out_tab, BUF_SIZE + 1, can_msg(CAN_MSG_INIT));
  CHECK_EQUAL(MSG_SIZE, can_buf_peek(&buf, out_tab, BUF_SIZE + 1));
  CheckCanMsgTabs(msg_tab, out_tab, MSG_SIZE);
}

TEST(CAN_Buf, CanBufWrite_TooManyFrames) {
  const size_t MSG_SIZE = BUF_SIZE + 1;
  can_msg msg_tab[MSG_SIZE];

  std::fill_n(msg_tab, MSG_SIZE, can_msg(CAN_MSG_INIT));
  FillCanMsg(msg_tab[0], 0x4d, 2, 0xc2);
  FillCanMsg(msg_tab[14], 0x26, 7, 0xb0);
  FillCanMsg(msg_tab[15], 0x81, 4, 0x08);

  const auto frames_written = can_buf_write(&buf, msg_tab, MSG_SIZE);

  CHECK_EQUAL(BUF_SIZE, frames_written);
  CHECK_EQUAL(BUF_SIZE, can_buf_size(&buf));
  CHECK_EQUAL(0, can_buf_capacity(&buf));

  can_msg out_tab[BUF_SIZE + 1];
  std::fill_n(out_tab, BUF_SIZE + 1, can_msg(CAN_MSG_INIT));
  CHECK_EQUAL(BUF_SIZE, can_buf_peek(&buf, out_tab, BUF_SIZE + 1));
  CheckCanMsgTabs(msg_tab, out_tab, 15);
}

TEST(CAN_Buf, CanBufClear) {
  const size_t MSG_SIZE = 5;
  can_msg msg_tab[MSG_SIZE];

  std::fill_n(msg_tab, MSG_SIZE, can_msg(CAN_MSG_INIT));
  can_buf_write(&buf, msg_tab, MSG_SIZE);

  can_buf_clear(&buf);

  CHECK_EQUAL(0, can_buf_size(&buf));
  CHECK_EQUAL(BUF_SIZE, can_buf_capacity(&buf));
}

TEST(CAN_Buf, CanBufPeek_NullPtr) {
  const size_t MSG_SIZE = 4;
  can_msg msg_tab[MSG_SIZE];

  std::fill_n(msg_tab, MSG_SIZE, can_msg(CAN_MSG_INIT));
  can_buf_write(&buf, msg_tab, MSG_SIZE);

  const auto frames_read = can_buf_peek(&buf, nullptr, 3);

  CHECK_EQUAL(3, frames_read);
  CHECK_EQUAL(MSG_SIZE, can_buf_size(&buf));
}

TEST(CAN_Buf, CanBufReserve_Enlarge) {
  const size_t MSG_SIZE = 8;
  can_msg msg_tab[MSG_SIZE];

  std::fill_n(msg_tab, MSG_SIZE, can_msg(CAN_MSG_INIT));
  can_buf_write(&buf, msg_tab, MSG_SIZE);

  const auto capacity = can_buf_reserve(&buf, BUF_SIZE - MSG_SIZE + 1);

#if LELY_NO_MALLOC
  CHECK_EQUAL(0, capacity);
  CHECK_EQUAL(BUF_SIZE - MSG_SIZE, can_buf_capacity(&buf));
  CHECK_EQUAL(MSG_SIZE, can_buf_size(&buf));
#else
  CHECK_EQUAL(31 - MSG_SIZE, capacity);  // (new_buffer_size - MSG_SIZE)
  CHECK_EQUAL(31 - MSG_SIZE, can_buf_capacity(&buf));
  CHECK_EQUAL(MSG_SIZE, can_buf_size(&buf));
#endif
}

TEST(CAN_Buf, CanBufReserve_BigEnough) {
  const size_t MSG_SIZE = 8;
  can_msg msg_tab[MSG_SIZE];

  std::fill_n(msg_tab, MSG_SIZE, can_msg(CAN_MSG_INIT));
  can_buf_write(&buf, msg_tab, MSG_SIZE);

  const auto capacity = can_buf_reserve(&buf, BUF_SIZE - MSG_SIZE);

  CHECK_EQUAL(BUF_SIZE - MSG_SIZE, capacity);
  CHECK_EQUAL(BUF_SIZE - MSG_SIZE, can_buf_capacity(&buf));
  CHECK_EQUAL(MSG_SIZE, can_buf_size(&buf));
}

#if LELY_NO_MALLOC
TEST(CAN_Buf, CanBufReserve_NoMemory) {
  const auto capacity = can_buf_reserve(&buf, 2 * BUF_SIZE);

  CHECK_EQUAL(0, capacity);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(BUF_SIZE, can_buf_capacity(&buf));
  CHECK_EQUAL(0, can_buf_size(&buf));
}
#else
TEST(CAN_Buf, CanBufReserve_Wrapping) {
  const size_t MSG_SIZE = 15;
  can_msg msg_tab[MSG_SIZE];

  std::fill_n(msg_tab, MSG_SIZE, can_msg(CAN_MSG_INIT));
  FillCanMsg(msg_tab[0], 0xdd, 8, 0x01);
  FillCanMsg(msg_tab[5], 0x8a, 5, 0xf8);
  FillCanMsg(msg_tab[10], 0xef, 7, 0x34);

  can_buf_write(&buf, msg_tab, MSG_SIZE);
  can_buf_read(&buf, nullptr, 10);
  can_buf_write(&buf, msg_tab, 6);
  const size_t NEW_MSG_SIZE = MSG_SIZE - 10 + 6;

  const auto capacity = can_buf_reserve(&buf, BUF_SIZE - NEW_MSG_SIZE + 1);

  CHECK_EQUAL(31 - NEW_MSG_SIZE, capacity);  // (new_buffer_size - NEW_MSG_SIZE)
  CHECK_EQUAL(31 - NEW_MSG_SIZE, can_buf_capacity(&buf));
  CHECK_EQUAL(NEW_MSG_SIZE, can_buf_size(&buf));

  can_msg out_tab[BUF_SIZE + 1];
  std::fill_n(out_tab, BUF_SIZE + 1, can_msg(CAN_MSG_INIT));
  CHECK_EQUAL(NEW_MSG_SIZE, can_buf_read(&buf, out_tab, BUF_SIZE + 1));
  CheckCanMsgTabs(msg_tab + 10, out_tab, 5);
  CheckCanMsgTabs(msg_tab, out_tab + 5, 6);
}
#endif  // !LELY_NO_MALLOC
