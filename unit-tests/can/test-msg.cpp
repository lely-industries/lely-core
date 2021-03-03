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

#include <cstring>

#include <CppUTest/TestHarness.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lely/can/msg.h>
#include <lely/util/error.h>

#include "override/libc-stdio.hpp"

/* lely/can/can_msg_bits() */

TEST_GROUP(CAN_MsgBits) { can_msg msg = CAN_MSG_INIT; };

TEST(CAN_MsgBits, CanMsg_StaticInitializer) {
  CHECK_EQUAL(0, msg.id);
  CHECK_EQUAL(0, msg.flags);
  CHECK_EQUAL(0, msg.len);
  for (const auto byte : msg.data) CHECK_EQUAL(0, byte);
}

#if !LELY_NO_CANFD
TEST(CAN_MsgBits, InvalidFDFFlag) {
  msg.flags |= CAN_FLAG_FDF;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT);

  CHECK_EQUAL(-1, frameSize);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}
#endif

TEST(CAN_MsgBits, InvalidMode) {
  const auto frameSize = can_msg_bits(&msg, (can_msg_bits_mode)(-1));

  CHECK_EQUAL(-1, frameSize);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CAN_MsgBits, InvalidMsgLength) {
  msg.len = CAN_MAX_LEN + 1;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT);

  CHECK_EQUAL(-1, frameSize);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CAN_MsgBits, CANBasic_ModeNoStuff_RTR) {
  msg.len = 0;
  msg.flags |= CAN_FLAG_RTR;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_NO_STUFF);

  CHECK_EQUAL(47, frameSize);  // min frame length
}

TEST(CAN_MsgBits, CANBasic_ModeNoStuff_NoData) {
  msg.len = 0;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_NO_STUFF);

  CHECK_EQUAL(47, frameSize);  // min frame length
}

TEST(CAN_MsgBits, CANBasic_ModeNoStuff_MaxLength) {
  msg.len = CAN_MAX_LEN;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_NO_STUFF);

  CHECK_EQUAL(111, frameSize);  // max frame length
}

TEST(CAN_MsgBits, CANBasic_ModeWorst_RTR) {
  msg.len = 0;
  msg.flags |= CAN_FLAG_RTR;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_WORST);

  CHECK_EQUAL(47 + 8, frameSize);  // frame (min) + stuffing
}

TEST(CAN_MsgBits, CANBasic_ModeWorst_NoData) {
  msg.len = 0;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_WORST);

  CHECK_EQUAL(47 + 8, frameSize);  // frame (min) + stuffing
}

TEST(CAN_MsgBits, CANBasic_ModeWorst_MaxLength) {
  msg.len = CAN_MAX_LEN;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_WORST);

  CHECK_EQUAL(111 + 24, frameSize);  // frame (max) + stuffing
}

TEST(CAN_MsgBits, CANBasic_ModeExact_RTR) {
  msg.id = 0x95;
  msg.len = 0;
  msg.flags |= CAN_FLAG_RTR;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT);

  CHECK_EQUAL(47 + 2, frameSize);  // frame (min) + stuffing
}

TEST(CAN_MsgBits, CANBasic_ModeExact_NoData) {
  msg.id = 0xa4;
  msg.len = 0;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT);

  CHECK_EQUAL(47 + 1, frameSize);  // frame (min) + stuffing
}

TEST(CAN_MsgBits, CANBasic_ModeExact_1) {
  msg.id = 0x78;
  msg.len = 8;
  for (int i = 0; i < msg.len; ++i) msg.data[i] = 0x3c;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT);

  CHECK_EQUAL(47 + 64 + 20, frameSize);  // control + data + stuffing
}

TEST(CAN_MsgBits, CANBasic_ModeExact_2) {
  msg.id = 0xfb;
  msg.len = 7;
  msg.data[0] = 0x8f;
  msg.data[1] = 0x26;
  msg.data[2] = 0x4d;
  msg.data[3] = 0x84;
  msg.data[4] = 0xcc;
  msg.data[5] = 0xa6;
  msg.data[6] = 0x9a;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT);

  CHECK_EQUAL(47 + 56 + 2, frameSize);  // control + data + stuffing
}

TEST(CAN_MsgBits, CANBasic_ModeExact_3) {
  msg.id = 0x1df;
  msg.len = 3;
  msg.data[0] = 0x81;
  msg.data[1] = 0x99;
  msg.data[2] = 0x1d;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT);

  CHECK_EQUAL(47 + 24 + 3, frameSize);  // control + data + stuffing
}

TEST(CAN_MsgBits, CANExtended_ModeNoStuff_RTR) {
  msg.len = 0;
  msg.flags |= CAN_FLAG_IDE;
  msg.flags |= CAN_FLAG_RTR;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_NO_STUFF);

  CHECK_EQUAL(67, frameSize);  // min frame length
}

TEST(CAN_MsgBits, CANExtended_ModeNoStuff_NoData) {
  msg.len = 0;
  msg.flags |= CAN_FLAG_IDE;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_NO_STUFF);

  CHECK_EQUAL(67, frameSize);  // min frame length
}

TEST(CAN_MsgBits, CANExtended_ModeNoStuff_MaxLength) {
  msg.len = CAN_MAX_LEN;
  msg.flags |= CAN_FLAG_IDE;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_NO_STUFF);

  CHECK_EQUAL(131, frameSize);  // max frame length
}

TEST(CAN_MsgBits, CANExtended_ModeWorst_RTR) {
  msg.len = 0;
  msg.flags |= CAN_FLAG_IDE;
  msg.flags |= CAN_FLAG_RTR;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_WORST);

  CHECK_EQUAL(67 + 13, frameSize);  // frame (min) + stuffing
}

TEST(CAN_MsgBits, CANExtended_ModeWorst_NoData) {
  msg.len = 0;
  msg.flags |= CAN_FLAG_IDE;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_WORST);

  CHECK_EQUAL(67 + 13, frameSize);  // frame (min) + stuffing
}

TEST(CAN_MsgBits, CANExtended_ModeWorst_MaxLength) {
  msg.len = CAN_MAX_LEN;
  msg.flags |= CAN_FLAG_IDE;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_WORST);

  CHECK_EQUAL(131 + 29, frameSize);  // frame (max) + stuffing
}

TEST(CAN_MsgBits, CANExtended_ModeExact_RTR) {
  msg.id = 0xfce1f1;
  msg.len = 0;
  msg.flags |= CAN_FLAG_IDE;
  msg.flags |= CAN_FLAG_RTR;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_WORST);

  CHECK_EQUAL(67 + 13, frameSize);  // frame (min) + stuffing
}

TEST(CAN_MsgBits, CANExtended_ModeExact_NoData) {
  msg.id = 0x1371e0d;
  msg.len = 0;
  msg.flags |= CAN_FLAG_IDE;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_WORST);

  CHECK_EQUAL(67 + 13, frameSize);  // frame (min) + stuffing
}

TEST(CAN_MsgBits, CANExtended_ModeExact_1) {
  msg.id = 0x1e38787;
  msg.len = 8;
  msg.flags |= CAN_FLAG_IDE;
  for (int i = 0; i < msg.len; ++i) msg.data[i] = 0x3c;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT);

  CHECK_EQUAL(67 + 64 + 23, frameSize);  // control + data + stuffing
}

TEST(CAN_MsgBits, CANExtended_ModeExact_2) {
  msg.id = 0x3885ff0a;
  msg.len = 2;
  msg.flags |= CAN_FLAG_IDE;
  msg.data[0] = 0x6e;
  msg.data[1] = 0x84;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT);

  CHECK_EQUAL(67 + 16 + 2, frameSize);  // control + data + stuffing
}

TEST(CAN_MsgBits, CANExtended_ModeExact_3) {
  msg.id = 0x1ca0c017;
  msg.len = 6;
  msg.flags |= CAN_FLAG_IDE;
  msg.data[0] = 0xb9;
  msg.data[1] = 0x75;
  msg.data[2] = 0x27;
  msg.data[3] = 0xad;
  msg.data[4] = 0x30;
  msg.data[5] = 0x2e;

  const auto frameSize =
      can_msg_bits(&msg, can_msg_bits_mode::CAN_MSG_BITS_MODE_EXACT);

  CHECK_EQUAL(67 + 36 + 14, frameSize);  // control + data + stuffing
}

/* util/bits/snprintf_can_msg() */
#if !LELY_NO_STDIO

TEST_GROUP(CAN_SNPrintfCanMsg) {
  static const size_t STRLEN = 256;

  can_msg msg = CAN_MSG_INIT;
  char output_str[STRLEN];

  TEST_SETUP() {
#if HAVE_SNPRINTF_OVERRIDE
    LibCOverride::snprintf_vc = Override::AllCallsValid;
#endif
    memset(output_str, 0, STRLEN);
  }
};

TEST(CAN_SNPrintfCanMsg, ZeroStr) {
  const auto slen = snprintf_can_msg(NULL, 0, &msg);

  CHECK_EQUAL(10, slen);
}

TEST(CAN_SNPrintfCanMsg, InvalidStr) {
  const auto slen = snprintf_can_msg(NULL, STRLEN, &msg);

  CHECK_EQUAL(10, slen);
}

TEST(CAN_SNPrintfCanMsg, NullCANMsg) {
  const auto slen = snprintf_can_msg(output_str, STRLEN, NULL);

  CHECK_EQUAL(0, slen);
  STRCMP_EQUAL("", output_str);
}

TEST(CAN_SNPrintfCanMsg, CANZeroMsg) {
  const auto slen = snprintf_can_msg(output_str, STRLEN, &msg);

  CHECK_EQUAL(10, slen);
  STRCMP_EQUAL("000   [0] ", output_str);
}

TEST(CAN_SNPrintfCanMsg, CANBasicMsg) {
  msg.id = 0x45d;
  msg.len = 8;
  for (int i = 0; i < msg.len; ++i) msg.data[i] = 0xc3;

  const auto slen = snprintf_can_msg(output_str, STRLEN, &msg);

  CHECK_EQUAL(34, slen);
  STRCMP_EQUAL("45D   [8]  C3 C3 C3 C3 C3 C3 C3 C3", output_str);
}

TEST(CAN_SNPrintfCanMsg, CANBasicRTRMsg) {
  msg.id = 0xe6;
  msg.flags |= CAN_FLAG_RTR;

  const auto slen = snprintf_can_msg(output_str, STRLEN, &msg);

  CHECK_EQUAL(25, slen);
  STRCMP_EQUAL("0E6   [0]  remote request", output_str);
}

TEST(CAN_SNPrintfCanMsg, CANExtendedMsg) {
  msg.id = 0xc38b35;
  msg.len = 6;
  msg.flags |= CAN_FLAG_IDE;
  for (int i = 0; i < msg.len; ++i) msg.data[i] = 0x67;

  const auto slen = snprintf_can_msg(output_str, STRLEN, &msg);

  CHECK_EQUAL(33, slen);
  STRCMP_EQUAL("00C38B35   [6]  67 67 67 67 67 67", output_str);
}

TEST(CAN_SNPrintfCanMsg, CANExtendedRTRMsg) {
  msg.id = 0x1ff0f03;
  msg.flags |= CAN_FLAG_IDE;
  msg.flags |= CAN_FLAG_RTR;

  const auto slen = snprintf_can_msg(output_str, STRLEN, &msg);

  CHECK_EQUAL(30, slen);
  STRCMP_EQUAL("01FF0F03   [0]  remote request", output_str);
}

#if !LELY_NO_CANFD
TEST(CAN_SNPrintfCanMsg, CANFDZeroMsg) {
  const auto slen = snprintf_can_msg(output_str, STRLEN, &msg);

  CHECK_EQUAL(10, slen);
  STRCMP_EQUAL("000   [0] ", output_str);
}

TEST(CAN_SNPrintfCanMsg, CANFDBasicMsg) {
  msg.id = 0x03;
  msg.len = 31;
  msg.flags |= CAN_FLAG_FDF;
  for (int i = 0; i < msg.len; ++i) msg.data[i] = 0x9d;

  const auto slen = snprintf_can_msg(output_str, STRLEN, &msg);

  CHECK_EQUAL(103, slen);
  STRCMP_EQUAL(
      "003  [31]  9D 9D 9D 9D 9D 9D 9D 9D 9D 9D 9D 9D 9D 9D 9D 9D 9D 9D 9D 9D "
      "9D 9D 9D 9D 9D 9D 9D 9D 9D 9D 9D",
      output_str);
}

TEST(CAN_SNPrintfCanMsg, CANFDExtendedMsg) {
  msg.id = 0x516083;
  msg.len = 64;
  msg.flags |= CAN_FLAG_FDF;
  msg.flags |= CAN_FLAG_IDE;
  for (int i = 0; i < msg.len; ++i) msg.data[i] = 0xa6;

  const auto slen = snprintf_can_msg(output_str, STRLEN, &msg);

  CHECK_EQUAL(207, slen);
  STRCMP_EQUAL(
      "00516083  [64]  A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 "
      "A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 "
      "A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6 A6",
      output_str);
}
#endif

#if HAVE_SNPRINTF_OVERRIDE
TEST(CAN_SNPrintfCanMsg, CANZeroMsg_SNPrintfErr_1) {
  LibCOverride::snprintf_vc = Override::NoneCallsValid;

  const auto slen = snprintf_can_msg(output_str, STRLEN, &msg);

  CHECK_EQUAL(-1, slen);
}

TEST(CAN_SNPrintfCanMsg, CANZeroMsg_SNPrintfErr_2) {
  LibCOverride::snprintf_vc = 1;

  const auto slen = snprintf_can_msg(output_str, STRLEN, &msg);

  CHECK_EQUAL(-1, slen);
}

TEST(CAN_SNPrintfCanMsg, CANBasicRTRMsg_SNPrintfErr) {
  LibCOverride::snprintf_vc = 2;
  msg.flags |= CAN_FLAG_RTR;

  const auto slen = snprintf_can_msg(output_str, STRLEN, &msg);

  CHECK_EQUAL(-1, slen);
}

TEST(CAN_SNPrintfCanMsg, CANBasicMsg_SNPrintfErr) {
  LibCOverride::snprintf_vc = 2;
  msg.len = 5;
  for (int i = 0; i < msg.len; ++i) msg.data[i] = 0xdd;

  const auto slen = snprintf_can_msg(output_str, STRLEN, &msg);

  CHECK_EQUAL(-1, slen);
}
#endif

/* util/bits/asprintf_can_msg() */
#if !LELY_NO_MALLOC

TEST_GROUP(CAN_ASPrintfCanMsg) {
  char* output_ps = nullptr;
  can_msg msg = CAN_MSG_INIT;

#if HAVE_SNPRINTF_OVERRIDE
  TEST_SETUP() { LibCOverride::snprintf_vc = Override::AllCallsValid; }
#endif
};

TEST(CAN_ASPrintfCanMsg, CANBasicMsg) {
  msg.id = 0x11d;
  msg.len = 5;
  for (int i = 0; i < msg.len; ++i) msg.data[i] = 0xaa;

  const auto slen = asprintf_can_msg(&output_ps, &msg);

  CHECK_EQUAL(25, slen);
  STRCMP_EQUAL("11D   [5]  AA AA AA AA AA", output_ps);

  free(output_ps);
}

TEST(CAN_ASPrintfCanMsg, CANExtendedMsg) {
  msg.id = 0xa3f500;
  msg.len = 2;
  msg.flags |= CAN_FLAG_IDE;
  for (int i = 0; i < msg.len; ++i) msg.data[i] = 0x01;

  const auto slen = asprintf_can_msg(&output_ps, &msg);

  CHECK_EQUAL(21, slen);
  STRCMP_EQUAL("00A3F500   [2]  01 01", output_ps);

  free(output_ps);
}

#if HAVE_SNPRINTF_OVERRIDE
TEST(CAN_ASPrintfCanMsg, CANZeroMsg_SNPrintfErr_1) {
  LibCOverride::snprintf_vc = Override::NoneCallsValid;

  const auto slen = asprintf_can_msg(&output_ps, &msg);

  CHECK_EQUAL(-1, slen);
}

TEST(CAN_ASPrintfCanMsg, CANZeroMsg_SNPrintfErr_2) {
  LibCOverride::snprintf_vc = 3;

  const auto slen = asprintf_can_msg(&output_ps, &msg);

  CHECK_EQUAL(-1, slen);
}
#endif  // HAVE_SNPRINTF_OVERRIDE

#endif  // !LELY_NO_MALLOC

#endif  // !LELY_NO_STDIO

/* util/bits/can_crc() */

TEST_GROUP(CAN_Crc) {
  uint_least8_t data[8] = {0xa4, 0x6f, 0xff, 0xe2, 0x11, 0x6a, 0xb5, 0xa3};
};

TEST(CAN_Crc, AllZeros) {
  auto ret = can_crc(0, NULL, 0, 0);

  CHECK_EQUAL(0x0, ret);
}

TEST(CAN_Crc, BitsZero) {
  auto ret = can_crc(0, data, 4, 0);

  CHECK_EQUAL(0, ret);
}

TEST(CAN_Crc, NegetiveOff) {
  auto ret = can_crc(0, data + 3, -11, 46);

  CHECK_EQUAL(0x3754, ret);
}
