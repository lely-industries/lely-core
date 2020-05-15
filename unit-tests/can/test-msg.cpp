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

#include <CppUTest/TestHarness.h>
#include <config.h>
#include <lely/can/msg.h>

#include <lely/util/errnum.h>

TEST_GROUP(UtilBitsCanMsgBits) {
  struct can_msg msg;
  int expected;

  void setup() override {
    msg = CAN_MSG_INIT;
    expected = -1;
  }

  void expect(const int value) { expected = value; }

  void given(const int frame_size) { CHECK_EQUAL(expected, frame_size); }
};

#if !LELY_NO_CANFD
TEST(UtilBitsCanMsgBits, InvalidFDFFlag) {
  expect(-1);
  msg.flags |= CAN_FLAG_FDF;
  given(can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT));
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}
#endif

TEST(UtilBitsCanMsgBits, InvalidMsgLength) {
  expect(-1);
  msg.len = CAN_MAX_LEN + 1;
  given(can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT));
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(UtilBitsCanMsgBits, CANBasic_ModeNoStuff_RTR) {
  expect(47);  // min frame length
  msg.len = 0;
  msg.flags |= CAN_FLAG_RTR;
  given(can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_NO_STUFF));
}

TEST(UtilBitsCanMsgBits, CANBasic_ModeNoStuff_MaxLength) {
  expect(111);  // max frame length
  msg.len = CAN_MAX_LEN;
  given(can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_NO_STUFF));
}

TEST(UtilBitsCanMsgBits, CANBasic_ModeWorst_RTR) {
  expect(47 + 8);  // frame (min) + stuff
  msg.len = 0;
  msg.flags |= CAN_FLAG_RTR;
  given(can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_WORST));
}

TEST(UtilBitsCanMsgBits, CANBasic_ModeWorst_MaxLength) {
  expect(111 + 24);  // frame (max) + stuff
  msg.len = CAN_MAX_LEN;
  given(can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_WORST));
}

TEST(UtilBitsCanMsgBits, CANBasic_ModeExact_1) {
  expect(47 + 64 + 20);  // control + data + stuff
  msg.id = 0x78;
  msg.len = 8;
  for (int i = 0; i < msg.len; ++i) msg.data[i] = 0x3c;
  given(can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT));
}

TEST(UtilBitsCanMsgBits, CANBasic_ModeExact_2) {
  expect(47 + 56 + 2);  // control + data + stuff
  msg.id = 0xfb;
  msg.len = 7;
  msg.data[0] = 0x8f;
  msg.data[1] = 0x26;
  msg.data[2] = 0x4d;
  msg.data[3] = 0x84;
  msg.data[4] = 0xcc;
  msg.data[5] = 0xa6;
  msg.data[6] = 0x9a;
  given(can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT));
}

TEST(UtilBitsCanMsgBits, CANBasic_ModeExact_3) {
  expect(47 + 24 + 5);  // control + data + stuff
  msg.id = 0x01df;
  msg.len = 3;
  msg.data[0] = 0x81;
  msg.data[2] = 0x99;
  msg.data[3] = 0x1d;
  given(can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT));
}

TEST(UtilBitsCanMsgBits, CANExtended_ModeNoStuff_RTR) {
  expect(67);  // min frame length
  msg.len = 0;
  msg.flags |= CAN_FLAG_IDE;
  msg.flags |= CAN_FLAG_RTR;
  given(can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_NO_STUFF));
}

TEST(UtilBitsCanMsgBits, CANExtended_ModeNoStuff_MaxLength) {
  expect(131);  // max frame length
  msg.len = CAN_MAX_LEN;
  msg.flags |= CAN_FLAG_IDE;
  given(can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_NO_STUFF));
}

TEST(UtilBitsCanMsgBits, CANExtended_ModeWorst_RTR) {
  expect(67 + 13);  // frame (min) + stuff
  msg.len = 0;
  msg.flags |= CAN_FLAG_IDE;
  msg.flags |= CAN_FLAG_RTR;
  given(can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_WORST));
}

TEST(UtilBitsCanMsgBits, CANExtended_ModeWorst_MaxLength) {
  expect(131 + 29);  // frame (max) + stuff
  msg.len = CAN_MAX_LEN;
  msg.flags |= CAN_FLAG_IDE;
  given(can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_WORST));
}

TEST(UtilBitsCanMsgBits, CANExtended_ModeExact_1) {
  expect(67 + 64 + 23);  // control + data + stuff
  msg.id = 0x1e38787;
  msg.len = 8;
  msg.flags |= CAN_FLAG_IDE;
  for (int i = 0; i < msg.len; ++i) msg.data[i] = 0x3c;
  given(can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT));
}

TEST(UtilBitsCanMsgBits, CANExtended_ModeExact_2) {
  expect(67 + 16 + 2);  // control + data + stuff
  msg.id = 0x3885ff0a;
  msg.len = 2;
  msg.flags |= CAN_FLAG_IDE;
  msg.data[0] = 0x6e;
  msg.data[1] = 0x84;
  given(can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT));
}

TEST(UtilBitsCanMsgBits, CANExtended_ModeExact_3) {
  expect(67 + 36 + 14);  // control + data + stuff
  msg.id = 0x1ca0c017;
  msg.len = 6;
  msg.flags |= CAN_FLAG_IDE;
  msg.data[0] = 0xb9;
  msg.data[1] = 0x75;
  msg.data[2] = 0x27;
  msg.data[3] = 0xad;
  msg.data[4] = 0x30;
  msg.data[5] = 0x2e;
  given(can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT));
}
