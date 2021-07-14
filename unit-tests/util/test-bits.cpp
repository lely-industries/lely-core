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

#include <lely/util/bits.h>

TEST_GROUP(Util_Bits){};

/// @name bswap16()
///@{

/// \Given N/A
///
/// \When bswap16() is called with a 16-bit unsigned integer value
///
/// \Then a copy of the input value with its bytes in reversed order is returned
TEST(Util_Bits, BSwap16) {
  // clang-format off
  const uint_least16_t test16[] = {
    0x0000u, 0x0000u,
    0xffffu, 0xffffu,
    0xafafu, 0xafafu,
    0x00ffu, 0xff00u,
    0x0001u, 0x0100u,
    0x1000u, 0x0010u,
    0x1234u, 0x3412u,
    0xbbddu, 0xddbbu,
    0x4a3du, 0x3d4au,
    0x8758u, 0x5887u,
    0xa486u, 0x86a4u,
    0x28eau, 0xea28u,
    0xe00du, 0x0de0u,
    0x6222u, 0x2262u,
    0xadd7u, 0xd7adu,
    0xfe57u, 0x57feu,
  };
  // clang-format on

  const size_t tsize = sizeof(test16) / sizeof(uint_least16_t);

  for (size_t i = 0; i < tsize; i += 2) {
    CHECK_EQUAL(test16[i], bswap16(test16[i + 1]));
    CHECK_EQUAL(test16[i + 1], bswap16(test16[i]));
  }
}

///@}

/// @name bswap32()
///@{

/// \Given N/A
///
/// \When bswap32() is called with a 32-bit unsigned integer value
///
/// \Then a copy of the input value with its bytes in reversed order is returned
TEST(Util_Bits, BSwap32) {
  // clang-format off
  const uint_least32_t test32[] = {
    0x00000000u, 0x00000000u,
    0xffffffffu, 0xffffffffu,
    0xabcdcdabu, 0xabcdcdabu,
    0x0000ffffu, 0xffff0000u,
    0x00000001u, 0x01000000u,
    0x10000000u, 0x00000010u,
    0x12345678u, 0x78563412u,
    0xabde1379u, 0x7913deabu,
    0xbcde5c2cu, 0x2c5cdebcu,
    0x11c61f9bu, 0x9b1fc611u,
    0x62978ffau, 0xfa8f9762u,
    0xd0b2fb90u, 0x90fbb2d0u,
    0x80d2b6a8u, 0xa8b6d280u,
    0xec14ef9eu, 0x9eef14ecu,
    0x7c8c8529u, 0x29858c7cu,
    0x7f5b330fu, 0x0f335b7fu,
  };
  // clang-format on

  const size_t tsize = sizeof(test32) / sizeof(uint_least32_t);

  for (size_t i = 0; i < tsize; i += 2) {
    CHECK_EQUAL(test32[i], bswap32(test32[i + 1]));
    CHECK_EQUAL(test32[i + 1], bswap32(test32[i]));
  }
}

///@}

/// @name bswap64()
///@{

/// \Given N/A
///
/// \When bswap64() is called with a 64-bit unsigned integer value
///
/// \Then a copy of the input value with its bytes in reversed order is returned
TEST(Util_Bits, BSwap64) {
  // clang-format off
  const uint_least64_t test64[] = {
    0x0000000000000000uL, 0x0000000000000000uL,
    0xffffffffffffffffuL, 0xffffffffffffffffuL,
    0xabcdef1212efcdabuL, 0xabcdef1212efcdabuL,
    0x00000000ffffffffuL, 0xffffffff00000000uL,
    0x0000000000000001uL, 0x0100000000000000uL,
    0x1000000000000000uL, 0x0000000000000010uL,
    0x0123456789abcdefuL, 0xefcdab8967452301uL,
    0xa8a43d00f3e67b7cuL, 0x7c7be6f3003da4a8uL,
    0xd519e9912a041e87uL, 0x871e042a91e919d5uL,
    0x31b0cd63c0b2a4c5uL, 0xc5a4b2c063cdb031uL,
    0xbcb66618fa9aaf22uL, 0x22af9afa1866b6bcuL,
    0xa617a18293bbd3f9uL, 0xf9d3bb9382a117a6uL,
    0xf42fe1ce2ee3bdbbuL, 0xbbbde32ecee12ff4uL,
    0xae3fbd91d8d4e911uL, 0x11e9d4d891bd3faeuL,
    0xf2ff58e8800a7ac9uL, 0xc97a0a80e858fff2uL,
    0xfb027d92f19239c2uL, 0xc23992f1927d02fbuL,
  };
  // clang-format on

  const size_t tsize = sizeof(test64) / sizeof(uint_least64_t);

  for (size_t i = 0; i < tsize; i += 2) {
    CHECK_EQUAL(test64[i], bswap64(test64[i + 1]));
    CHECK_EQUAL(test64[i + 1], bswap64(test64[i]));
  }
}

///@}

/// @name cls8() and clz8()
///@{

/// \Given N/A
///
/// \When cls8() and clz8() are called with an 8-bit unsigned integer value
///
/// \Then a call to cls8() returns the number of leading set bits and a call to
///       clz8() returns the number of leading zero bits in the requested value
TEST(Util_Bits, ClsClz8) {
  // clang-format off
  const uint_least8_t test8[] = {
    0x00u, 0x80u, 0xc0u, 0xe0u, 0xf0u, 0xf8u, 0xfcu, 0xfeu, 0xffu,
    0x16u, 0x9cu, 0xd8u, 0xedu, 0xf5u, 0xf9u, 0xfdu, 0xfeu, 0xffu,
    0x32u, 0xa1u, 0xc9u, 0xe1u, 0xf3u, 0xfau, 0xfcu, 0xfeu, 0xffu,
    0x79u, 0xb3u, 0xd5u, 0xe8u, 0xf7u, 0xfbu, 0xfdu, 0xfeu, 0xffu,
  };
  // clang-format on

  const ssize_t tsize = sizeof(test8) / sizeof(uint_least8_t);
  const int max_s = cls8(0xffu) + 1;
  const int max_z = clz8(0x00u) + 1;

  CHECK_EQUAL(max_s, max_z);

  for (int i = 0; i < tsize; ++i) {
    CHECK_EQUAL(i % max_s, cls8(test8[i]));
    CHECK_EQUAL(i % max_z, clz8(~test8[i]));
  }
}

///@}

/// @name cls16() and clz16()
///@{

/// \Given N/A
///
/// \When cls16() and clz16() are called with a 16-bit unsigned integer value
///
/// \Then a call to cls16() returns the number of leading set bits and a call to
///       clz16() returns the number of leading zero bits in the requested value
TEST(Util_Bits, ClsClz16) {
  // clang-format off
  const uint_least16_t test16[] = {
    0x0000u,
    0x8000u, 0xc000u, 0xe000u, 0xf000u,
    0xf800u, 0xfc00u, 0xfe00u, 0xff00u,
    0xff80u, 0xffc0u, 0xffe0u, 0xfff0u,
    0xfff8u, 0xfffcu, 0xfffeu, 0xffffu,

    0x741eu,
    0x9d45u, 0xcab3u, 0xe0ccu, 0xf4afu,
    0xf9adu, 0xfd53u, 0xfed2u, 0xff68u,
    0xffa0u, 0xffd3u, 0xffeau, 0xfff5u,
    0xfffbu, 0xfffdu, 0xfffeu, 0xffffu,
  };
  // clang-format on

  const ssize_t tsize = sizeof(test16) / sizeof(uint_least16_t);
  const int max_s = cls16(0xffffu) + 1;
  const int max_z = clz16(0x0u) + 1;

  CHECK_EQUAL(max_s, max_z);

  for (int i = 0; i < tsize; ++i) {
    CHECK_EQUAL(i % max_s, cls16(test16[i]));
    CHECK_EQUAL(i % max_z, clz16(~test16[i]));
  }
}

///@}

/// @name cls32() and clz32()
///@{

/// \Given N/A
///
/// \When cls32() and clz32() are called with a 32-bit unsigned integer value
///
/// \Then a call to cls32() returns the number of leading set bits and a call to
///       clz32() returns the number of leading zero bits in the requested value
TEST(Util_Bits, ClsClz32) {
  // clang-format off
  const uint_least32_t test32[] = {
    0x00000000u,
    0x80000000u, 0xc0000000u, 0xe0000000u, 0xf0000000u,
    0xf8000000u, 0xfc000000u, 0xfe000000u, 0xff000000u,
    0xff800000u, 0xffc00000u, 0xffe00000u, 0xfff00000u,
    0xfff80000u, 0xfffc0000u, 0xfffe0000u, 0xffff0000u,
    0xffff8000u, 0xffffc000u, 0xffffe000u, 0xfffff000u,
    0xfffff800u, 0xfffffc00u, 0xfffffe00u, 0xffffff00u,
    0xffffff80u, 0xffffffc0u, 0xffffffe0u, 0xfffffff0u,
    0xfffffff8u, 0xfffffffcu, 0xfffffffeu, 0xffffffffu,

    0x314ffe89u,
    0xb5300dfcu, 0xca78c934u, 0xed9b3ecfu, 0xf3b0843bu,
    0xf8438ffdu, 0xfdf6c7b9u, 0xfefff835u, 0xff3ef887u,
    0xffaea43au, 0xffd8d510u, 0xffeb2240u, 0xfff40b94u,
    0xfffb960eu, 0xfffdeb71u, 0xfffef349u, 0xffff4bfdu,
    0xffffad7au, 0xffffd203u, 0xffffe2ccu, 0xfffff6b6u,
    0xfffff8f5u, 0xfffffdeau, 0xfffffe01u, 0xffffff14u,
    0xffffffa2u, 0xffffffd0u, 0xffffffe7u, 0xfffffff5u,
    0xfffffffau, 0xfffffffdu, 0xfffffffeu, 0xffffffffu,
  };

  // clang-format on

  const ssize_t tsize = sizeof(test32) / sizeof(uint_least32_t);
  const int max_s = cls32(0xffffffffu) + 1;
  const int max_z = clz32(0x0u) + 1;

  CHECK_EQUAL(max_s, max_z);

  for (int i = 0; i < tsize; ++i) {
    CHECK_EQUAL(i % max_s, cls32(test32[i]));
    CHECK_EQUAL(i % max_z, clz32(~test32[i]));
  }
}

///@}

/// @name cls64() and clz64()
///@{

/// \Given N/A
///
/// \When cls64() and clz64() are called with a 64-bit unsigned integer value
///
/// \Then a call to cls64() returns the number of leading set bits and a call to
///       clz64() returns the number of leading zero bits in the requested value
TEST(Util_Bits, ClsClz64) {
  // clang-format off
  const uint_least64_t test64[] = {
    0x0000000000000000uL,
    0x8000000000000000uL, 0xc000000000000000uL, 0xe000000000000000uL, 0xf000000000000000uL,  // NOLINT
    0xf800000000000000uL, 0xfc00000000000000uL, 0xfe00000000000000uL, 0xff00000000000000uL,  // NOLINT
    0xff80000000000000uL, 0xffc0000000000000uL, 0xffe0000000000000uL, 0xfff0000000000000uL,  // NOLINT
    0xfff8000000000000uL, 0xfffc000000000000uL, 0xfffe000000000000uL, 0xffff000000000000uL,  // NOLINT
    0xffff800000000000uL, 0xffffc00000000000uL, 0xffffe00000000000uL, 0xfffff00000000000uL,  // NOLINT
    0xfffff80000000000uL, 0xfffffc0000000000uL, 0xfffffe0000000000uL, 0xffffff0000000000uL,  // NOLINT
    0xffffff8000000000uL, 0xffffffc000000000uL, 0xffffffe000000000uL, 0xfffffff000000000uL,  // NOLINT
    0xfffffff800000000uL, 0xfffffffc00000000uL, 0xfffffffe00000000uL, 0xffffffff00000000uL,  // NOLINT
    0xffffffff80000000uL, 0xffffffffc0000000uL, 0xffffffffe0000000uL, 0xfffffffff0000000uL,  // NOLINT
    0xfffffffff8000000uL, 0xfffffffffc000000uL, 0xfffffffffe000000uL, 0xffffffffff000000uL,  // NOLINT
    0xffffffffff800000uL, 0xffffffffffc00000uL, 0xffffffffffe00000uL, 0xfffffffffff00000uL,  // NOLINT
    0xfffffffffff80000uL, 0xfffffffffffc0000uL, 0xfffffffffffe0000uL, 0xffffffffffff0000uL,  // NOLINT
    0xffffffffffff8000uL, 0xffffffffffffc000uL, 0xffffffffffffe000uL, 0xfffffffffffff000uL,  // NOLINT
    0xfffffffffffff800uL, 0xfffffffffffffc00uL, 0xfffffffffffffe00uL, 0xffffffffffffff00uL,  // NOLINT
    0xffffffffffffff80uL, 0xffffffffffffffc0uL, 0xffffffffffffffe0uL, 0xfffffffffffffff0uL,  // NOLINT
    0xfffffffffffffff8uL, 0xfffffffffffffffcuL, 0xfffffffffffffffeuL, 0xffffffffffffffffuL,  // NOLINT

    0x42b139630a0b0596uL,
    0xacaa634ea2d357abuL, 0xcab7e5abf85f3afcuL, 0xe4f7d1195acad6bfuL, 0xf019bcd064df1b62uL,  // NOLINT
    0xfbb88dd93589318buL, 0xfccd0e9dbca29c56uL, 0xfe31bdeb1878a15duL, 0xff71e71ae41c7cb6uL,  // NOLINT
    0xff819f2bfedaf46fuL, 0xffd53d744135992cuL, 0xffe18474b591b9c4uL, 0xfff3354a561b1bdauL,  // NOLINT
    0xfffbc68b13c69247uL, 0xfffd2f3db32cf9d0uL, 0xfffe94cd581a9bcbuL, 0xffff63a0e7c8cf46uL,  // NOLINT
    0xffff8d451a297fc2uL, 0xffffdf2affe13ddeuL, 0xffffe0e9f450868buL, 0xfffff33a9cb37a47uL,  // NOLINT
    0xfffffbef74ad5c91uL, 0xfffffd652a8b5c36uL, 0xfffffe5dc90b792cuL, 0xffffff0cdbfac333uL,  // NOLINT
    0xffffff9b23749aabuL, 0xffffffc8f3973014uL, 0xffffffe73617d2c9uL, 0xfffffff03d3ef0fbuL,  // NOLINT
    0xfffffff9a6888689uL, 0xfffffffc1c0e9717uL, 0xfffffffef1d8acbcuL, 0xffffffff7e391679uL,  // NOLINT
    0xffffffff8788908buL, 0xffffffffd73e1ec9uL, 0xffffffffe35953e8uL, 0xfffffffff411752buL,  // NOLINT
    0xfffffffff846f930uL, 0xfffffffffd89d7ccuL, 0xfffffffffe45ebcfuL, 0xffffffffff2e00eauL,  // NOLINT
    0xffffffffffa5943buL, 0xffffffffffc0774cuL, 0xffffffffffe7f965uL, 0xfffffffffff3474auL,  // NOLINT
    0xfffffffffff8fe93uL, 0xfffffffffffc50cfuL, 0xfffffffffffea621uL, 0xffffffffffff08f0uL,  // NOLINT
    0xffffffffffff9d14uL, 0xffffffffffffd2cduL, 0xffffffffffffed4buL, 0xfffffffffffff555uL,  // NOLINT
    0xfffffffffffffa18uL, 0xfffffffffffffd22uL, 0xfffffffffffffe07uL, 0xffffffffffffff61uL,  // NOLINT
    0xffffffffffffffbauL, 0xffffffffffffffd9uL, 0xffffffffffffffebuL, 0xfffffffffffffff5uL,  // NOLINT
    0xfffffffffffffffbuL, 0xfffffffffffffffduL, 0xfffffffffffffffeuL, 0xffffffffffffffffuL,  // NOLINT
  };
  // clang-format on

  const ssize_t tsize = sizeof(test64) / sizeof(uint_least64_t);
  const int max_s = cls64(0xffffffffffffffffuL) + 1;
  const int max_z = clz64(0x0uL) + 1;

  CHECK_EQUAL(max_s, max_z);

  for (int i = 0; i < tsize; ++i) {
    CHECK_EQUAL(i % max_s, cls64(test64[i]));
    CHECK_EQUAL(i % max_z, clz64(~test64[i]));
  }
}

///@}

/// @name cts8() and ctz8()
///@{

/// \Given N/A
///
/// \When cts8() and ctz8() are called with an 8-bit unsigned integer value
///
/// \Then a call to cts8() returns the number of trailing set bits and a call to
///       ctz8() returns the number of trailing zero bits in the requested value
TEST(Util_Bits, CtsCtz8) {
  // clang-format off
  const uint_least8_t test8[] = {
    0x00u, 0x01u, 0x03u, 0x07u, 0x0fu, 0x1fu, 0x3fu, 0x7fu, 0xffu,
    0xaau, 0x11u, 0x8bu, 0x47u, 0x6fu, 0x5fu, 0xbfu, 0x7fu, 0xffu,
    0x40u, 0xbdu, 0xdbu, 0x97u, 0xefu, 0xdfu, 0x3fu, 0x7fu, 0xffu,
    0x10u, 0x61u, 0xe3u, 0xc7u, 0xcfu, 0x9fu, 0xbfu, 0x7fu, 0xffu,
  };
  // clang-format on

  const ssize_t tsize = sizeof(test8) / sizeof(uint_least8_t);
  const int max_s = cts8(0xffu) + 1;
  const int max_z = ctz8(0x0u) + 1;

  CHECK_EQUAL(max_s, max_z);

  for (int i = 0; i < tsize; ++i) {
    CHECK_EQUAL(i % max_s, cts8(test8[i]));
    CHECK_EQUAL(i % max_z, ctz8(~test8[i]));
  }
}

///@}

/// @name cts16() and ctz16()
///@{

/// \Given N/A
///
/// \When cts16() and ctz16() are called with a 16-bit unsigned integer value
///
/// \Then a call to cts16() returns the number of trailing set bits and a call
///       to ctz16() returns the number of trailing zero bits in the requested
///       value
TEST(Util_Bits, CtsCtz16) {
  // clang-format off
  const uint_least16_t test16[] = {
    0x0000u,
    0x0001u, 0x0003u, 0x0007u, 0x000fu,
    0x001fu, 0x003fu, 0x007fu, 0x00ffu,
    0x01ffu, 0x03ffu, 0x07ffu, 0x0fffu,
    0x1fffu, 0x3fffu, 0x7fffu, 0xffffu,

    0x5406u,
    0x01e9u, 0xa093u, 0x11e7u, 0x97cfu,
    0xd01fu, 0x21bfu, 0x367fu, 0x72ffu,
    0xadffu, 0xcbffu, 0x57ffu, 0x6fffu,
    0xdfffu, 0x3fffu, 0x7fffu, 0xffffu,
  };
  // clang-format on

  const ssize_t tsize = sizeof(test16) / sizeof(uint_least16_t);
  const int max_s = cts16(0xffffu) + 1;
  const int max_z = ctz16(0x0u) + 1;

  CHECK_EQUAL(max_s, max_z);

  for (int i = 0; i < tsize; ++i) {
    CHECK_EQUAL(i % max_s, cts16(test16[i]));
    CHECK_EQUAL(i % max_z, ctz16(~test16[i]));
  }
}

///@}

/// @name cts32() and ctz32()
///@{

/// \Given N/A
///
/// \When cts32() and ctz32() are called with a 32-bit unsigned integer value
///
/// \Then a call to cts32() returns the number of trailing set bits and a call
///       to ctz32() returns the number of trailing zero bits in the requested
///       value
TEST(Util_Bits, CtsCtz32) {
  // clang-format off
  const uint_least32_t test32[] = {
    0x00000000u,
    0x00000001u, 0x00000003u, 0x00000007u, 0x0000000fu,
    0x0000001fu, 0x0000003fu, 0x0000007fu, 0x000000ffu,
    0x000001ffu, 0x000003ffu, 0x000007ffu, 0x00000fffu,
    0x00001fffu, 0x00003fffu, 0x00007fffu, 0x0000ffffu,
    0x0001ffffu, 0x0003ffffu, 0x0007ffffu, 0x000fffffu,
    0x001fffffu, 0x003fffffu, 0x007fffffu, 0x00ffffffu,
    0x01ffffffu, 0x03ffffffu, 0x07ffffffu, 0x0fffffffu,
    0x1fffffffu, 0x3fffffffu, 0x7fffffffu, 0xffffffffu,

    0xf4d17e3cu,
    0xdd5882ddu, 0x198de57bu, 0xc72748c7u, 0x2285326fu,
    0x7e00c31fu, 0x17d64b3fu, 0xd5a7e97fu, 0x9f3f30ffu,
    0xc116e5ffu, 0x8f497bffu, 0x136bd7ffu, 0xbe510fffu,
    0x14831fffu, 0xbbbd3fffu, 0xa6297fffu, 0xd1eeffffu,
    0x4e5dffffu, 0x7e5bffffu, 0xfb27ffffu, 0xd52fffffu,
    0xaa1fffffu, 0xd7bfffffu, 0xbb7fffffu, 0x44ffffffu,
    0xc5ffffffu, 0x8bffffffu, 0x67ffffffu, 0x4fffffffu,
    0x9fffffffu, 0xbfffffffu, 0x7fffffffu, 0xffffffffu,
  };
  // clang-format on

  const ssize_t tsize = sizeof(test32) / sizeof(uint_least32_t);
  const int max_s = cts32(0xffffffffu) + 1;
  const int max_z = ctz32(0x0u) + 1;

  CHECK_EQUAL(max_s, max_z);

  for (int i = 0; i < tsize; ++i) {
    CHECK_EQUAL(i % max_s, cts32(test32[i]));
    CHECK_EQUAL(i % max_z, ctz32(~test32[i]));
  }
}

///@}

/// @name cts64() and ctz64()
///@{

/// \Given N/A
///
/// \When cts64() and ctz64() are called with a 64-bit unsigned integer value
///
/// \Then a call to cts64() returns the number of trailing set bits and a call
///       to ctz64() returns the number of trailing zero bits in the requested
///       value
TEST(Util_Bits, CtsCtz64) {
  // clang-format off
  const uint_least64_t test64[] = {
    0x0000000000000000uL,
    0x0000000000000001uL, 0x0000000000000003uL, 0x0000000000000007uL, 0x000000000000000fuL,  // NOLINT
    0x000000000000001fuL, 0x000000000000003fuL, 0x000000000000007fuL, 0x00000000000000ffuL,  // NOLINT
    0x00000000000001ffuL, 0x00000000000003ffuL, 0x00000000000007ffuL, 0x0000000000000fffuL,  // NOLINT
    0x0000000000001fffuL, 0x0000000000003fffuL, 0x0000000000007fffuL, 0x000000000000ffffuL,  // NOLINT
    0x000000000001ffffuL, 0x000000000003ffffuL, 0x000000000007ffffuL, 0x00000000000fffffuL,  // NOLINT
    0x00000000001fffffuL, 0x00000000003fffffuL, 0x00000000007fffffuL, 0x0000000000ffffffuL,  // NOLINT
    0x0000000001ffffffuL, 0x0000000003ffffffuL, 0x0000000007ffffffuL, 0x000000000fffffffuL,  // NOLINT
    0x000000001fffffffuL, 0x000000003fffffffuL, 0x000000007fffffffuL, 0x00000000ffffffffuL,  // NOLINT
    0x00000001ffffffffuL, 0x00000003ffffffffuL, 0x00000007ffffffffuL, 0x0000000fffffffffuL,  // NOLINT
    0x0000001fffffffffuL, 0x0000003fffffffffuL, 0x0000007fffffffffuL, 0x000000ffffffffffuL,  // NOLINT
    0x000001ffffffffffuL, 0x000003ffffffffffuL, 0x000007ffffffffffuL, 0x00000fffffffffffuL,  // NOLINT
    0x00001fffffffffffuL, 0x00003fffffffffffuL, 0x00007fffffffffffuL, 0x0000ffffffffffffuL,  // NOLINT
    0x0001ffffffffffffuL, 0x0003ffffffffffffuL, 0x0007ffffffffffffuL, 0x000fffffffffffffuL,  // NOLINT
    0x001fffffffffffffuL, 0x003fffffffffffffuL, 0x007fffffffffffffuL, 0x00ffffffffffffffuL,  // NOLINT
    0x01ffffffffffffffuL, 0x03ffffffffffffffuL, 0x07ffffffffffffffuL, 0x0fffffffffffffffuL,  // NOLINT
    0x1fffffffffffffffuL, 0x3fffffffffffffffuL, 0x7fffffffffffffffuL, 0xffffffffffffffffuL,  // NOLINT

    0xd082b0c953361364uL,
    0xdeab2ecd7f0fc25duL, 0x699974bc5f7a7edbuL, 0x03208b5b39ff70f7uL, 0x9872283a5cc6394fuL,  // NOLINT
    0xba15af046d8a40dfuL, 0x307b97335e8b113fuL, 0xacdc6b17044e917fuL, 0xd809c2f04835feffuL,  // NOLINT
    0xe02d29f55c904dffuL, 0x9dc69947208e03ffuL, 0xf537f67b3ca327ffuL, 0xbca9ff91f1f00fffuL,  // NOLINT
    0x9be3307ef517dfffuL, 0xde78d15b2f6ebfffuL, 0x808d2641990f7fffuL, 0xf93652c29832ffffuL,  // NOLINT
    0x80a6c80a142dffffuL, 0xab7a9d938f5bffffuL, 0xb15167d0d567ffffuL, 0xbd55891a80efffffuL,  // NOLINT
    0xce54e9cd35dfffffuL, 0x585249a6053fffffuL, 0x98d0a95d4f7fffffuL, 0x78095b96aeffffffuL,  // NOLINT
    0x54290dc925ffffffuL, 0x00dc5f72dbffffffuL, 0x535ee84597ffffffuL, 0xbaa4490eefffffffuL,  // NOLINT
    0x5e52603c5fffffffuL, 0x13d7e585bfffffffuL, 0x999b13257fffffffuL, 0x70335fc0ffffffffuL,  // NOLINT
    0x9cde0951ffffffffuL, 0x2fa4d78bffffffffuL, 0x1d19db47ffffffffuL, 0xb672a4efffffffffuL,  // NOLINT
    0x1ff5d8dfffffffffuL, 0x9bdff23fffffffffuL, 0xb90eec7fffffffffuL, 0xc0de8cffffffffffuL,  // NOLINT
    0x735265ffffffffffuL, 0x23018bffffffffffuL, 0xa0e1e7ffffffffffuL, 0x096a8fffffffffffuL,  // NOLINT
    0x82e61fffffffffffuL, 0x60a0bfffffffffffuL, 0x67e97fffffffffffuL, 0x0bd8ffffffffffffuL,  // NOLINT
    0xbf11ffffffffffffuL, 0x13e3ffffffffffffuL, 0x7377ffffffffffffuL, 0x0eafffffffffffffuL,  // NOLINT
    0x631fffffffffffffuL, 0xa73fffffffffffffuL, 0xfb7fffffffffffffuL, 0x92ffffffffffffffuL,  // NOLINT
    0x65ffffffffffffffuL, 0xabffffffffffffffuL, 0x47ffffffffffffffuL, 0x8fffffffffffffffuL,  // NOLINT
    0xdfffffffffffffffuL, 0xbfffffffffffffffuL, 0x7fffffffffffffffuL, 0xffffffffffffffffuL,  // NOLINT
  };
  // clang-format on

  const ssize_t tsize = sizeof(test64) / sizeof(uint_least64_t);
  const int max_s = cts64(0xffffffffffffffffuL) + 1;
  const int max_z = ctz64(0x0uL) + 1;

  CHECK_EQUAL(max_s, max_z);

  for (int i = 0; i < tsize; ++i) {
    CHECK_EQUAL(i % max_s, cts64(test64[i]));
    CHECK_EQUAL(i % max_z, ctz64(~test64[i]));
  }
}

///@}

/// @name ffs8()
///@{

/// \Given N/A
///
/// \When ffs8() is called with a zero 8-bit unsigned integer value
///
/// \Then 0 is returned
TEST(Util_Bits, Ffs8_Zero) { CHECK_EQUAL(0, ffs8(0x00u)); }

/// \Given N/A
///
/// \When ffs8() is called with a non-zero 8-bit unsigned integer value
///
/// \Then the index of the first set bit in the requested value is returned
TEST(Util_Bits, Ffs8_NonZero) {
  CHECK_EQUAL(1, ffs8(0x01u));
  CHECK_EQUAL(2, ffs8(0xf2u));
  CHECK_EQUAL(8, ffs8(0x80u));
}

///@}

/// @name ffz8()
///@{

/// \Given N/A
///
/// \When ffz8() is called with an 8-bit unsigned integer value with all bits
///       set
///
/// \Then 0 is returned
TEST(Util_Bits, Ffz8_AllBitsSet) { CHECK_EQUAL(0, ffz8(0xffu)); }

/// \Given N/A
///
/// \When ffz8() is called with an 8-bit unsigned integer value with at least
///       one zero bit
///
/// \Then the index of the first zero bit in the requested value is returned
TEST(Util_Bits, Ffz8_WithZeroBits) {
  CHECK_EQUAL(1, ffz8(0x00u));
  CHECK_EQUAL(2, ffz8(0xe1u));
  CHECK_EQUAL(8, ffz8(0x7fu));
}

///@}

/// @name ffs16()
///@{

/// \Given N/A
///
/// \When ffs16() is called with a zero 16-bit unsigned integer value
///
/// \Then 0 is returned
TEST(Util_Bits, Ffs16_Zero) { CHECK_EQUAL(0, ffs16(0x0000u)); }

/// \Given N/A
///
/// \When ffs16() is called with a non-zero 16-bit unsigned integer value
///
/// \Then the index of the first set bit in the requested value is returned
TEST(Util_Bits, Ffs16_NonZero) {
  CHECK_EQUAL(1, ffs16(0x0001u));
  CHECK_EQUAL(2, ffs16(0x0002u));
  CHECK_EQUAL(8, ffs16(0x0080u));

  CHECK_EQUAL(9, ffs16(0x0100u));
  CHECK_EQUAL(16, ffs16(0x8000u));
}

///@}

/// @name ffz16()
///@{

/// \Given N/A
///
/// \When ffz16() is called with a 16-bit unsigned integer value with all bits
///       set
///
/// \Then 0 is returned
TEST(Util_Bits, Ffz16_AllBitsSet) { CHECK_EQUAL(0, ffz16(0xffffu)); }

/// \Given N/A
///
/// \When ffz16() is called with a 16-bit unsigned integer value with at least
///       one zero bit
///
/// \Then the index of the first zero bit in the requested value is returned
TEST(Util_Bits, Ffz16_WithZeroBits) {
  CHECK_EQUAL(1, ffz16(0x0000u));
  CHECK_EQUAL(2, ffz16(0x0001u));
  CHECK_EQUAL(8, ffz16(0x007fu));

  CHECK_EQUAL(9, ffz16(0x00ffu));
  CHECK_EQUAL(16, ffz16(0x7fffu));
}

///@}

/// @name ffs32()
///@{

/// \Given N/A
///
/// \When ffs32() is called with a zero 32-bit unsigned integer value
///
/// \Then 0 is returned
TEST(Util_Bits, Ffs32_Zero) { CHECK_EQUAL(0, ffs32(0x00000000u)); }

/// \Given N/A
///
/// \When ffs32() is called with a non-zero 32-bit unsigned integer value
///
/// \Then the index of the first set bit in the requested value is returned
TEST(Util_Bits, Ffs32_NonZero) {
  CHECK_EQUAL(1, ffs32(0x00000001u));
  CHECK_EQUAL(2, ffs32(0x00000002u));
  CHECK_EQUAL(8, ffs32(0x00000080u));

  CHECK_EQUAL(9, ffs32(0x00000100u));
  CHECK_EQUAL(16, ffs32(0x00008000u));

  CHECK_EQUAL(17, ffs32(0x00010000u));
  CHECK_EQUAL(24, ffs32(0x00800000u));

  CHECK_EQUAL(25, ffs32(0x01000000u));
  CHECK_EQUAL(32, ffs32(0x80000000u));
}

///@}

/// @name ffz32()
///@{

/// \Given N/A
///
/// \When ffz32() is called with a 32-bit unsigned integer value with all bits
///       set
///
/// \Then 0 is returned
TEST(Util_Bits, Ffz32_AllBitsSet) { CHECK_EQUAL(0, ffz32(0xffffffffu)); }

/// \Given N/A
///
/// \When ffz32() is called with a 32-bit unsigned integer value with at least
///       one zero bit
///
/// \Then the index of the first zero bit in the requested value is returned
TEST(Util_Bits, Ffz32_WithZeroBits) {
  CHECK_EQUAL(1, ffz32(0x00000002u));
  CHECK_EQUAL(2, ffz32(0x00000001u));
  CHECK_EQUAL(8, ffz32(0x0000007fu));

  CHECK_EQUAL(9, ffz32(0x000000ffu));
  CHECK_EQUAL(16, ffz32(0x00007fffu));

  CHECK_EQUAL(17, ffz32(0x0000ffffu));
  CHECK_EQUAL(24, ffz32(0x007fffffu));

  CHECK_EQUAL(25, ffz32(0x00ffffffu));
  CHECK_EQUAL(32, ffz32(0x7fffffffu));
}

///@}

/// @name ffs64()
///@{

/// \Given N/A
///
/// \When ffs64() is called with a zero 64-bit unsigned integer value
///
/// \Then 0 is returned
TEST(Util_Bits, Ffs64_Zero) { CHECK_EQUAL(0, ffs64(0x0000000000000000uL)); }

/// \Given N/A
///
/// \When ffs64() is called with a non-zero 64-bit unsigned integer value
///
/// \Then the index of the first set bit in the requested value is returned
TEST(Util_Bits, Ffs64_NonZero) {
  CHECK_EQUAL(1, ffs64(0x0000000000000001uL));
  CHECK_EQUAL(2, ffs64(0x0000000000000002uL));
  CHECK_EQUAL(8, ffs64(0x0000000000000080uL));

  CHECK_EQUAL(9, ffs64(0x0000000000000100uL));
  CHECK_EQUAL(16, ffs64(0x0000000000008000uL));

  CHECK_EQUAL(17, ffs64(0x0000000000010000uL));
  CHECK_EQUAL(24, ffs64(0x0000000000800000uL));

  CHECK_EQUAL(25, ffs64(0x0000000001000000uL));
  CHECK_EQUAL(32, ffs64(0x0000000080000000uL));

  CHECK_EQUAL(33, ffs64(0x0000000100000000uL));
  CHECK_EQUAL(40, ffs64(0x0000008000000000uL));

  CHECK_EQUAL(41, ffs64(0x0000010000000000uL));
  CHECK_EQUAL(48, ffs64(0x0000800000000000uL));

  CHECK_EQUAL(49, ffs64(0x0001000000000000uL));
  CHECK_EQUAL(56, ffs64(0x0080000000000000uL));

  CHECK_EQUAL(57, ffs64(0x0100000000000000uL));
  CHECK_EQUAL(64, ffs64(0x8000000000000000uL));
}

///@}

/// @name ffz64()
///@{

/// \Given N/A
///
/// \When ffz64() is called with a 64-bit unsigned integer value with all bits
///       set
///
/// \Then 0 is returned
TEST(Util_Bits, Ffz64_AllBitsSet) {
  CHECK_EQUAL(0, ffz64(0xffffffffffffffffuL));
}

/// \Given N/A
///
/// \When ffz64() is called with a 64-bit unsigned integer value with at least
///       one zero bit
///
/// \Then the index of the first zero bit in the requested value is returned
TEST(Util_Bits, Ffz64_WithZeroBits) {
  CHECK_EQUAL(1, ffz64(0xffffffff00000002uL));
  CHECK_EQUAL(2, ffz64(0xffffffff00000001uL));
  CHECK_EQUAL(8, ffz64(0xffffffff0000007fuL));

  CHECK_EQUAL(9, ffz64(0xffffffff000000ffuL));
  CHECK_EQUAL(16, ffz64(0xffffffff00007fffuL));

  CHECK_EQUAL(17, ffz64(0xffffffff0000ffffuL));
  CHECK_EQUAL(24, ffz64(0xfffffff0ff7fffffuL));

  CHECK_EQUAL(25, ffz64(0xffffffff70ffffffuL));
  CHECK_EQUAL(32, ffz64(0xffffffff7fffffffuL));

  CHECK_EQUAL(33, ffz64(0xfffffff0ffffffffuL));
  CHECK_EQUAL(40, ffz64(0xffffff7fffffffffuL));

  CHECK_EQUAL(41, ffz64(0xfffff0ffffffffffuL));
  CHECK_EQUAL(48, ffz64(0xffff7fffffffffffuL));

  CHECK_EQUAL(49, ffz64(0xfff0ffffffffffffuL));
  CHECK_EQUAL(56, ffz64(0xff7fffffffffffffuL));

  CHECK_EQUAL(57, ffz64(0xf0ffffffffffffffuL));
  CHECK_EQUAL(64, ffz64(0x7fffffffffffffffuL));
}

///@}

/// @name parity8()
///@{

/// \Given N/A
///
/// \When parity8() is called with an 8-bit unsigned integer value
///
/// \Then 0 is returned if the number of set bits in the requested value is
///       even, 1 is returned otherwise
TEST(Util_Bits, Parity8) {
  CHECK_EQUAL(0, parity8(0x00u));
  CHECK_EQUAL(1, parity8(0x01u));

  CHECK_EQUAL(0, parity8(0xf0u));
  CHECK_EQUAL(1, parity8(0xf1u));
}

///@}

/// @name parity16()
///@{

/// \Given N/A
///
/// \When parity16() is called with a 16-bit unsigned integer value
///
/// \Then 0 is returned if the number of set bits in the requested value is
///       even, 1 is returned otherwise
TEST(Util_Bits, Parity16) {
  CHECK_EQUAL(0, parity16(0x0000u));
  CHECK_EQUAL(1, parity16(0x0001u));

  CHECK_EQUAL(0, parity16(0x00f0u));
  CHECK_EQUAL(1, parity16(0x00f1u));

  CHECK_EQUAL(0, parity16(0xf000u));
  CHECK_EQUAL(1, parity16(0x0ff1u));
}

///@}

/// @name parity32()
///@{

/// \Given N/A
///
/// \When parity32() is called with a 32-bit unsigned integer value
///
/// \Then 0 is returned if the number of set bits in the requested value is
///       even, 1 is returned otherwise
TEST(Util_Bits, Parity32) {
  CHECK_EQUAL(0, parity32(0x00000000u));
  CHECK_EQUAL(1, parity32(0x00000001u));

  CHECK_EQUAL(0, parity32(0x000000f0u));
  CHECK_EQUAL(1, parity32(0x000000f1u));

  CHECK_EQUAL(0, parity32(0x0000f000u));
  CHECK_EQUAL(1, parity32(0x00000ff1u));

  CHECK_EQUAL(0, parity32(0xfffffff0u));
  CHECK_EQUAL(1, parity32(0xf0ffcff1u));
}

///@}

/// @name parity64()
///@{

/// \Given N/A
///
/// \When parity64() is called with a 64-bit unsigned integer value
///
/// \Then 0 is returned if the number of set bits in the requested value is
///       even, 1 is returned otherwise
TEST(Util_Bits, Parity64) {
  CHECK_EQUAL(0, parity64(0x0000000000000000uL));
  CHECK_EQUAL(1, parity64(0x0000000000000001uL));

  CHECK_EQUAL(0, parity64(0x00000000000000f0uL));
  CHECK_EQUAL(1, parity64(0x00000000000000f1uL));

  CHECK_EQUAL(0, parity64(0x000000000000f000uL));
  CHECK_EQUAL(1, parity64(0x0000000000000ff1uL));

  CHECK_EQUAL(0, parity64(0x00000000fffffff0uL));
  CHECK_EQUAL(1, parity64(0x00000000f0ffcff1uL));

  CHECK_EQUAL(0, parity64(0xfffffffffffffff0uL));
  CHECK_EQUAL(1, parity64(0x0ffffffffffffff1uL));
}

///@}

/// @name popcount8()
///@{

/// \Given N/A
///
/// \When popcount8() is called with an 8-bit unsigned integer value
///
/// \Then the number of set bits in the requested value is returned
TEST(Util_Bits, Popcount8) {
  CHECK_EQUAL(0, popcount8(0x00u));
  CHECK_EQUAL(1, popcount8(0x01u));
  CHECK_EQUAL(4, popcount8(0xe1u));
  CHECK_EQUAL(8, popcount8(0xffu));
}

///@}

/// @name popcount16()
///@{

/// \Given N/A
///
/// \When popcount16() is called with a 16-bit unsigned integer value
///
/// \Then the number of set bits in the requested value is returned
TEST(Util_Bits, Popcount16) {
  CHECK_EQUAL(0, popcount16(0x0000u));
  CHECK_EQUAL(1, popcount16(0x0001u));
  CHECK_EQUAL(9, popcount16(0x01ffu));
  CHECK_EQUAL(8, popcount16(0xf00fu));
}

///@}

/// @name popcount32()
///@{

/// \Given N/A
///
/// \When popcount32() is called with a 32-bit unsigned integer value
///
/// \Then the number of set bits in the requested value is returned
TEST(Util_Bits, Popcount32) {
  CHECK_EQUAL(0, popcount32(0x00000000u));
  CHECK_EQUAL(1, popcount32(0x00000001u));
  CHECK_EQUAL(9, popcount32(0x000001ffu));
  CHECK_EQUAL(8, popcount32(0x0000f00fu));
  CHECK_EQUAL(11, popcount32(0x00e0f00fu));
  CHECK_EQUAL(15, popcount32(0x33e0f00fu));
}

///@}

/// @name popcount64()
///@{

/// \Given N/A
///
/// \When popcount64() is called with a 64-bit unsigned integer value
///
/// \Then the number of set bits in the requested value is returned
TEST(Util_Bits, Popcount64) {
  CHECK_EQUAL(0, popcount64(0x0000000000000000uL));
  CHECK_EQUAL(1, popcount64(0x0000000000000001uL));
  CHECK_EQUAL(8, popcount64(0x00000000000000ffuL));
  CHECK_EQUAL(9, popcount64(0x00000000000001ffuL));
  CHECK_EQUAL(8, popcount64(0x000000000000f00fuL));
  CHECK_EQUAL(8, popcount64(0x000000000000f00fuL));
  CHECK_EQUAL(10, popcount64(0x000000000044f00fuL));
  CHECK_EQUAL(12, popcount64(0x000000002211f00fuL));
  CHECK_EQUAL(12, popcount64(0x000000560000f00fuL));
  CHECK_EQUAL(13, popcount64(0x000010560000f00fuL));
  CHECK_EQUAL(16, popcount64(0x007010560000f00fuL));
  CHECK_EQUAL(16, popcount64(0x550000560000f00fuL));
}

///@}

/// @name rol8()
///@{

/// \Given N/A
///
/// \When rol8() is called with an 8-bit unsigned integer value and a number of
///       bits
///
/// \Then a copy of the input value rotated left by the requested number of bits
///       is returned
TEST(Util_Bits, Rol8) {
  CHECK_EQUAL(0x00u, rol8(0x00u, 0));
  CHECK_EQUAL(0x00u, rol8(0x00u, 1));
  CHECK_EQUAL(0x00u, rol8(0x00u, 8));
  CHECK_EQUAL(0x00u, rol8(0x00u, 42));

  CHECK_EQUAL(0xc0u, rol8(0x30u, 2));
  CHECK_EQUAL(0x26u, rol8(0x89u, 2));
  CHECK_EQUAL(0x4Cu, rol8(0x89u, 3));
  CHECK_EQUAL(0x98u, rol8(0x89u, 4));

  CHECK_EQUAL(0x89u, rol8(0x89u, 8));

  CHECK_EQUAL(0x26u, rol8(0x89u, 66));
  CHECK_EQUAL(0x98u, rol8(0x89u, 68));
}

///@}

/// @name ror8()
///@{

/// \Given N/A
///
/// \When ror8() is called with an 8-bit unsigned integer value and a number of
///       bits
///
/// \Then a copy of the input value rotated right by the requested number of
///       bits is returned
TEST(Util_Bits, Ror8) {
  CHECK_EQUAL(0x00u, ror8(0x00u, 0));
  CHECK_EQUAL(0x00u, ror8(0x00u, 1));
  CHECK_EQUAL(0x00u, ror8(0x00u, 8));
  CHECK_EQUAL(0x00u, ror8(0x00u, 42));

  CHECK_EQUAL(0x0Cu, ror8(0x30u, 2));
  CHECK_EQUAL(0x62u, ror8(0x89u, 2));
  CHECK_EQUAL(0x31u, ror8(0x89u, 3));
  CHECK_EQUAL(0x98u, ror8(0x89u, 4));

  CHECK_EQUAL(0x89u, ror8(0x89u, 8));

  CHECK_EQUAL(0x62u, ror8(0x89u, 66));
  CHECK_EQUAL(0x98u, ror8(0x89u, 68));
}

///@}

/// @name rol16()
///@{

/// \Given N/A
///
/// \When rol16() is called with a 16-bit unsigned integer value and a number of
///       bits
///
/// \Then a copy of the input value rotated left by the requested number of bits
///       is returned
TEST(Util_Bits, Rol16) {
  CHECK_EQUAL(0x0000u, rol16(0x0000u, 0));
  CHECK_EQUAL(0x0000u, rol16(0x0000u, 1));
  CHECK_EQUAL(0x0000u, rol16(0x0000u, 16));
  CHECK_EQUAL(0x0000u, rol16(0x0000u, 42));

  CHECK_EQUAL(0x04c0u, rol16(0x0130u, 2));
  CHECK_EQUAL(0x9e25u, rol16(0x6789u, 2));
  CHECK_EQUAL(0x3c4bu, rol16(0x6789u, 3));
  CHECK_EQUAL(0x7896u, rol16(0x6789u, 4));

  CHECK_EQUAL(0x8967u, rol16(0x6789u, 8));
  CHECK_EQUAL(0x12cfu, rol16(0x6789u, 9));

  CHECK_EQUAL(0xb3C4u, rol16(0x6789u, 15));

  CHECK_EQUAL(0x6789u, rol16(0x6789u, 16));

  CHECK_EQUAL(0x9e25u, rol16(0x6789u, 66));
  CHECK_EQUAL(0x7896u, rol16(0x6789u, 68));
}

///@}

/// @name ror16()
///@{

/// \Given N/A
///
/// \When ror16() is called with a 16-bit unsigned integer value and a number of
///       bits
///
/// \Then a copy of the input value rotated right by the requested number of
///       bits is returned
TEST(Util_Bits, Ror16) {
  CHECK_EQUAL(0x0000u, ror16(0x0000u, 0));
  CHECK_EQUAL(0x0000u, ror16(0x0000u, 1));
  CHECK_EQUAL(0x0000u, ror16(0x0000u, 16));
  CHECK_EQUAL(0x0000u, ror16(0x0000u, 42));

  CHECK_EQUAL(0x004Cu, ror16(0x0130u, 2));
  CHECK_EQUAL(0x59e2u, ror16(0x6789u, 2));
  CHECK_EQUAL(0x2cf1u, ror16(0x6789u, 3));
  CHECK_EQUAL(0x9678u, ror16(0x6789u, 4));

  CHECK_EQUAL(0x8967u, ror16(0x6789u, 8));
  CHECK_EQUAL(0xc4b3u, ror16(0x6789u, 9));

  CHECK_EQUAL(0xcf12u, ror16(0x6789u, 15));

  CHECK_EQUAL(0x6789u, ror16(0x6789u, 16));

  CHECK_EQUAL(0x59e2u, ror16(0x6789u, 66));
  CHECK_EQUAL(0x9678u, ror16(0x6789u, 68));
}

///@}

/// @name rol32()
///@{

/// \Given N/A
///
/// \When rol32() is called with a 32-bit unsigned integer value and a number of
///       bits
///
/// \Then a copy of the input value rotated left by the requested number of bits
///       is returned
TEST(Util_Bits, Rol32) {
  CHECK_EQUAL(0x00000000u, rol32(0x00000000u, 0));
  CHECK_EQUAL(0x00000000u, rol32(0x00000000u, 1));
  CHECK_EQUAL(0x00000000u, rol32(0x00000000u, 16));
  CHECK_EQUAL(0x00000000u, rol32(0x00000000u, 42));

  CHECK_EQUAL(0x4c00000u, rol32(0x01300000u, 2));
  CHECK_EQUAL(0x8d159e24u, rol32(0x23456789u, 2));
  CHECK_EQUAL(0x1a2b3c49u, rol32(0x23456789u, 3));
  CHECK_EQUAL(0x34567892u, rol32(0x23456789u, 4));

  CHECK_EQUAL(0x45678923u, rol32(0x23456789u, 8));
  CHECK_EQUAL(0x8acf1246u, rol32(0x23456789u, 9));

  CHECK_EQUAL(0xb3c491a2u, rol32(0x23456789u, 15));
  CHECK_EQUAL(0x67892345u, rol32(0x23456789u, 16));

  CHECK_EQUAL(0x89234567u, rol32(0x23456789u, 24));
  CHECK_EQUAL(0x12468acfu, rol32(0x23456789u, 25));

  CHECK_EQUAL(0x23456789u, rol32(0x23456789u, 32));

  CHECK_EQUAL(0x8d159e24u, rol32(0x23456789u, 66));
  CHECK_EQUAL(0x34567892u, rol32(0x23456789u, 68));
}

///@}

/// @name ror32()
///@{

/// \Given N/A
///
/// \When ror32() is called with a 32-bit unsigned integer value and a number of
///       bits
///
/// \Then a copy of the input value rotated right by the requested number of
///       bits is returned
TEST(Util_Bits, Ror32) {
  CHECK_EQUAL(0x00000000u, ror32(0x00000000u, 0));
  CHECK_EQUAL(0x00000000u, ror32(0x00000000u, 1));
  CHECK_EQUAL(0x00000000u, ror32(0x00000000u, 16));
  CHECK_EQUAL(0x00000000u, ror32(0x00000000u, 42));

  CHECK_EQUAL(0x004c0000u, ror32(0x01300000u, 2));
  CHECK_EQUAL(0x48d159e2u, ror32(0x23456789u, 2));
  CHECK_EQUAL(0x2468acf1u, ror32(0x23456789u, 3));
  CHECK_EQUAL(0x92345678u, ror32(0x23456789u, 4));

  CHECK_EQUAL(0x89234567u, ror32(0x23456789u, 8));
  CHECK_EQUAL(0xc491a2b3u, ror32(0x23456789u, 9));

  CHECK_EQUAL(0xcf12468au, ror32(0x23456789u, 15));
  CHECK_EQUAL(0x67892345u, ror32(0x23456789u, 16));

  CHECK_EQUAL(0x45678923u, ror32(0x23456789u, 24));
  CHECK_EQUAL(0xa2b3c491u, ror32(0x23456789u, 25));

  CHECK_EQUAL(0x23456789u, ror32(0x23456789u, 32));

  CHECK_EQUAL(0x48d159e2u, ror32(0x23456789u, 66));
  CHECK_EQUAL(0x92345678u, ror32(0x23456789u, 68));
}

///@}

/// @name rol64()
///@{

/// \Given N/A
///
/// \When rol64() is called with a 64-bit unsigned integer value and a number of
///       bits
///
/// \Then a copy of the input value rotated left by the requested number of bits
///       is returned
TEST(Util_Bits, Rol64) {
  CHECK_EQUAL(0x0000000000000000uL, rol64(0x0000000000000000uL, 0));
  CHECK_EQUAL(0x0000000000000000uL, rol64(0x0000000000000000uL, 1));
  CHECK_EQUAL(0x0000000000000000uL, rol64(0x0000000000000000uL, 16));
  CHECK_EQUAL(0x0000000000000000uL, rol64(0x0000000000000000uL, 42));

  CHECK_EQUAL(0x0003fc0004c00000uL, rol64(0x0000ff0001300000uL, 2));
  CHECK_EQUAL(0xffffc3c48d159e27uL, rol64(0xfffff0f123456789uL, 2));
  CHECK_EQUAL(0xffff87891a2b3c4fuL, rol64(0xfffff0f123456789uL, 3));
  CHECK_EQUAL(0xffff0f123456789fuL, rol64(0xfffff0f123456789uL, 4));

  CHECK_EQUAL(0xfff0f123456789ffuL, rol64(0xfffff0f123456789uL, 8));
  CHECK_EQUAL(0xffe1e2468acf13ffuL, rol64(0xfffff0f123456789uL, 9));

  CHECK_EQUAL(0xf87891a2b3c4ffffuL, rol64(0xfffff0f123456789uL, 15));
  CHECK_EQUAL(0xf0f123456789ffffuL, rol64(0xfffff0f123456789uL, 16));

  CHECK_EQUAL(0xf123456789fffff0uL, rol64(0xfffff0f123456789uL, 24));
  CHECK_EQUAL(0xe2468acf13ffffe1uL, rol64(0xfffff0f123456789uL, 25));

  CHECK_EQUAL(0x23456789fffff0f1uL, rol64(0xfffff0f123456789uL, 32));
  CHECK_EQUAL(0x468acf13ffffe1e2uL, rol64(0xfffff0f123456789uL, 33));

  CHECK_EQUAL(0x456789fffff0f123uL, rol64(0xfffff0f123456789uL, 40));
  CHECK_EQUAL(0x8acf13ffffe1e246uL, rol64(0xfffff0f123456789uL, 41));

  CHECK_EQUAL(0x6789fffff0f12345uL, rol64(0xfffff0f123456789uL, 48));
  CHECK_EQUAL(0xcf13ffffe1e2468auL, rol64(0xfffff0f123456789uL, 49));

  CHECK_EQUAL(0xe27ffffc3c48d159uL, rol64(0xfffff0f123456789uL, 54));
  CHECK_EQUAL(0xc4fffff87891a2b3uL, rol64(0xfffff0f123456789uL, 55));

  CHECK_EQUAL(0xfffff0f123456789uL, rol64(0xfffff0f123456789uL, 64));

  CHECK_EQUAL(0xffffc3c48d159e27uL, rol64(0xfffff0f123456789uL, 66));
  CHECK_EQUAL(0xffff0f123456789fuL, rol64(0xfffff0f123456789uL, 68));
}

///@}

/// @name ror64()
///@{

/// \Given N/A
///
/// \When ror64() is called with a 64-bit unsigned integer value and a number of
///       bits
///
/// \Then a copy of the input value rotated right by the requested number of
///       bits is returned
TEST(Util_Bits, Ror64) {
  CHECK_EQUAL(0x0000000000000000uL, ror64(0x0000000000000000uL, 0));
  CHECK_EQUAL(0x0000000000000000uL, ror64(0x0000000000000000uL, 1));
  CHECK_EQUAL(0x0000000000000000uL, ror64(0x0000000000000000uL, 16));
  CHECK_EQUAL(0x0000000000000000uL, ror64(0x0000000000000000uL, 42));

  CHECK_EQUAL(0x00003fc0004c0000uL, ror64(0x0000ff0001300000uL, 2));
  CHECK_EQUAL(0x7ffffc3c48d159e2uL, ror64(0xfffff0f123456789uL, 2));
  CHECK_EQUAL(0x3ffffe1e2468acf1uL, ror64(0xfffff0f123456789uL, 3));
  CHECK_EQUAL(0x9fffff0f12345678uL, ror64(0xfffff0f123456789uL, 4));

  CHECK_EQUAL(0x89fffff0f1234567uL, ror64(0xfffff0f123456789uL, 8));
  CHECK_EQUAL(0xc4fffff87891a2b3uL, ror64(0xfffff0f123456789uL, 9));

  CHECK_EQUAL(0xcf13ffffe1e2468auL, ror64(0xfffff0f123456789uL, 15));
  CHECK_EQUAL(0x6789fffff0f12345uL, ror64(0xfffff0f123456789uL, 16));

  CHECK_EQUAL(0x456789fffff0f123uL, ror64(0xfffff0f123456789uL, 24));
  CHECK_EQUAL(0xa2b3c4fffff87891uL, ror64(0xfffff0f123456789uL, 25));

  CHECK_EQUAL(0x23456789fffff0f1uL, ror64(0xfffff0f123456789uL, 32));
  CHECK_EQUAL(0x91a2b3c4fffff878uL, ror64(0xfffff0f123456789uL, 33));

  CHECK_EQUAL(0xf123456789fffff0uL, ror64(0xfffff0f123456789uL, 40));
  CHECK_EQUAL(0x7891a2b3c4fffff8uL, ror64(0xfffff0f123456789uL, 41));

  CHECK_EQUAL(0xf0f123456789ffffuL, ror64(0xfffff0f123456789uL, 48));
  CHECK_EQUAL(0xf87891a2b3c4ffffuL, ror64(0xfffff0f123456789uL, 49));

  CHECK_EQUAL(0xffc3c48d159e27ffuL, ror64(0xfffff0f123456789uL, 54));
  CHECK_EQUAL(0xffe1e2468acf13ffuL, ror64(0xfffff0f123456789uL, 55));

  CHECK_EQUAL(0xfffff0f123456789uL, ror64(0xfffff0f123456789uL, 64));

  CHECK_EQUAL(0x7ffffc3c48d159e2uL, ror64(0xfffff0f123456789uL, 66));
  CHECK_EQUAL(0x9fffff0f12345678uL, ror64(0xfffff0f123456789uL, 68));
}

///@}
