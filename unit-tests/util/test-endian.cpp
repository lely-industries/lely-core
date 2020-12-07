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

#include <lely/util/endian.h>

TEST_GROUP(Util_Endian_Bcpy) {
  uint_least8_t dst[1u] = {0xffu};
  const uint_least8_t src[1u] = {0xccu};
};

TEST(Util_Endian_Bcpy, Bcpybe_CopyZero) {
  const int dstbit = 0;
  const int srcbit = 0;
  const size_t n = 0u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xffu, *dst);
}

TEST(Util_Endian_Bcpy, Bcpybe_DstbitPlusNMoreThan8) {
  const int dstbit = 3;
  const int srcbit = 2;
  const size_t n = 6u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xe6u, *dst);
}

TEST(Util_Endian_Bcpy, Bcpybe_LastIsZero) {
  const int dstbit = 3;
  const int srcbit = 2;
  const size_t n = 5u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xe6u, *dst);
}

TEST(Util_Endian_Bcpy, Bcpyle_LastIsZero) {
  const int dstbit = 3;
  const int srcbit = 2;
  const size_t n = 5u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0x9fu, *dst);
}

TEST(Util_Endian_Bcpy, Bcpyle_NoShiftDstbitPlusNMoreThan8) {
  const int dstbit = 2;
  const int srcbit = 2;
  const size_t n = 17u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xcfu, *dst);
}

TEST(Util_Endian_Bcpy, Bcpyle_NoShiftDstbitPlusNMoreThan8LastZero) {
  const int dstbit = 2;
  const int srcbit = 2;
  const size_t n = 22u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xcfu, *dst);
}

TEST(Util_Endian_Bcpy, Bcpyle_NoShiftDstbitPlusNLessThan8LastNotZero) {
  const int dstbit = 6;
  const int srcbit = 6;
  const size_t n = 1u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xffu, *dst);
}

TEST(Util_Endian_Bcpy, Bcpyle_CopyAll) {
  const int dstbit = 0;
  const int srcbit = 0;
  const size_t n = 8u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(*src, *dst);
}

TEST_GROUP(Util_Endian_BcpyOutOfRange) {
  uint_least8_t dst_array[4u] = {0xffu, 0xffu, 0xffu, 0xffu};
  const uint_least8_t src_array[4u] = {0x00u, 0xccu, 0x00u, 0x00u};
  uint_least8_t* const dst = &dst_array[1u];
  const uint_least8_t* const src = &src_array[1u];
};

TEST(Util_Endian_BcpyOutOfRange, Bcpybe_DstbitIsZeroCopyMoreThan8) {
  const int dstbit = 0;
  const int srcbit = 0;
  const size_t n = 9u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xccu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpybe_PositiveShiftSrcbitPlusNMoreThan8) {
  const int dstbit = 3;
  const int srcbit = 4;
  const size_t n = 5u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xf8u, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpybe_NegativeShiftSrcbitPlusNMoreThan8) {
  const int dstbit = 1;
  const int srcbit = 2;
  const size_t n = 7u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0x98u, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpybe_NegativeShiftDstbitPlusNMoreThan8) {
  const int dstbit = 2;
  const int srcbit = 3;
  const size_t n = 17u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xd8u, *dst);
}

TEST(Util_Endian_BcpyOutOfRange,
     Bcpybe_NegativeShiftDstbitPlusNMoreThan8LastIsZero) {
  const int dstbit = 2;
  const int srcbit = 3;
  const size_t n = 22u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xd8u, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpybe_NoShiftDstbitPlusNMoreThan8) {
  const int dstbit = 2;
  const int srcbit = 2;
  const size_t n = 17u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xccu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpybe_NoShiftDstbitPlusNMoreThan8LastZero) {
  const int dstbit = 2;
  const int srcbit = 2;
  const size_t n = 22u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xccu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange,
     Bcpybe_NoShiftDstbitPlusNLessThan8LastNotZero) {
  const int dstbit = 6;
  const int srcbit = 6;
  const size_t n = 1u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xfdu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpybe_CopyAll) {
  const int dstbit = 0;
  const int srcbit = 0;
  const size_t n = 8u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(*src, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpyle_DstbitIsZeroCopyMoreThan8) {
  const int dstbit = 0;
  const int srcbit = 0;
  const size_t n = 9u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xccu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpyle_PositiveShift) {
  const int dstbit = 3;
  const int srcbit = 4;
  const size_t n = 5u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0x67u, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpyle_NegativeShiftSrcbitPlusNMoreThan8) {
  const int dstbit = 1;
  const int srcbit = 2;
  const size_t n = 7u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0x67u, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpyle_NegativeShiftDstbitPlusNMoreThan8) {
  const int dstbit = 2;
  const int srcbit = 3;
  const size_t n = 17u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0x67u, *dst);
}

TEST(Util_Endian_BcpyOutOfRange,
     Bcpyle_NegativeShiftDstbitPlusNMoreThan8LastIsZero) {
  const int dstbit = 2;
  const int srcbit = 3;
  const size_t n = 22u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0x67u, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpyle_CopyZero) {
  const int dstbit = 0;
  const int srcbit = 0;
  const size_t n = 0u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xffu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpyle_DstbitPlusNMoreThan8) {
  const int dstbit = 3;
  const int srcbit = 2;
  const size_t n = 6u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0x9fu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpybe_OffsetsLowerThan0) {
  const int dstbit = -1;
  const int srcbit = -1;
  const size_t n = 4u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xdfu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpybe_SrcbitOffsetLowerThan0) {
  const int dstbit = 0;
  const int srcbit = -1;
  const size_t n = 1u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0x7fu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpyle_OffsetsLowerThan0) {
  const int dstbit = -1;
  const int srcbit = -1;
  const size_t n = 4u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xfcu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpyle_SrcbitOffsetLowerThan0) {
  const int dstbit = 0;
  const int srcbit = -1;
  const size_t n = 1u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xfeu, *dst);
}

TEST_GROUP(Util_Endian){};

TEST(Util_Endian, Htobe16) {
  CHECK_EQUAL(0x0000u, htobe16(0x0000u));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x3412u, htobe16(0x1234u));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x1234u, htobe16(0x1234u));
#endif
}

TEST(Util_Endian, Betoh16) {
  CHECK_EQUAL(0x0000u, betoh16(0x0000u));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x3412u, betoh16(0x1234u));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x1234u, betoh16(0x1234u));
#endif
}

TEST(Util_Endian, Htole16) {
  CHECK_EQUAL(0x0000u, htole16(0x0000u));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x1234u, htole16(0x1234u));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x3412u, htole16(0x1234u));
#endif
}

TEST(Util_Endian, Letoh16) {
  CHECK_EQUAL(0x0000u, letoh16(0x0000u));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x1234u, letoh16(0x1234u));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x3412u, letoh16(0x1234u));
#endif
}

TEST(Util_Endian, Htobe32) {
  CHECK_EQUAL(0x00000000u, htobe32(0x00000000u));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x78563412u, htobe32(0x12345678u));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x12345678u, htobe32(0x12345678u));
#endif
}

TEST(Util_Endian, Betoh32) {
  CHECK_EQUAL(0x00000000u, betoh32(0x00000000u));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x78563412u, betoh32(0x12345678u));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x12345678u, betoh32(0x12345678u));
#endif
}

TEST(Util_Endian, Htole32) {
  CHECK_EQUAL(0x00000000u, htole32(0x00000000u));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x12345678u, htole32(0x12345678u));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x78563412u, htole32(0x12345678u));
#endif
}

TEST(Util_Endian, Letoh32) {
  CHECK_EQUAL(0x00000000u, letoh32(0x00000000u));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x12345678u, letoh32(0x12345678u));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x78563412u, letoh32(0x12345678u));
#endif
}

TEST(Util_Endian, Htobe64) {
  CHECK_EQUAL(0x0000000000000000u, htobe64(0x0000000000000000u));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0xefcdab8967452301u, htobe64(0x0123456789abcdefu));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x0123456789abcdefu, htobe64(0x0123456789abcdefu));
#endif
}

TEST(Util_Endian, Betoh64) {
  CHECK_EQUAL(0x0000000000000000u, betoh64(0x0000000000000000u));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0xefcdab8967452301u, betoh64(0x0123456789abcdefu));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x0123456789abcdefu, betoh64(0x0123456789abcdefu));
#endif
}

TEST(Util_Endian, Htole64) {
  CHECK_EQUAL(0x0000000000000000u, htole64(0x0000000000000000u));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x0123456789abcdefu, htole64(0x0123456789abcdefu));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0xefcdab8967452301u, htole64(0x0123456789abcdefu));
#endif
}

TEST(Util_Endian, Letoh64) {
  CHECK_EQUAL(0x0000000000000000u, letoh64(0x0000000000000000u));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x0123456789abcdefu, letoh64(0x0123456789abcdefu));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0xefcdab8967452301u, letoh64(0x0123456789abcdefu));
#endif
}

TEST(Util_Endian, StbeI16_Zero) {
  uint_least8_t dst[] = {0x12u, 0x34u};
  const int_least16_t x = 0x0000;

  stbe_i16(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
}

TEST(Util_Endian, StbeI16_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u};
  const int_least16_t x = 0x1234;

  stbe_i16(dst, x);

  CHECK_EQUAL(0x12u, dst[0]);
  CHECK_EQUAL(0x34u, dst[1]);
}

TEST(Util_Endian, LdbeI16_Zero) {
  const uint_least8_t src[] = {0x00u, 0x00u};

  CHECK_EQUAL(0x0000, ldbe_i16(src));
}

TEST(Util_Endian, LdbeI16_Nonzero) {
  const uint_least8_t src[] = {0x12u, 0x34u};

  CHECK_EQUAL(0x1234, ldbe_i16(src));
}

TEST(Util_Endian, StbeU16_Zero) {
  uint_least8_t dst[] = {0x12u, 0x34u};
  const uint_least16_t x = 0x0000u;

  stbe_u16(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
}

TEST(Util_Endian, StbeU16_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u};
  const uint_least16_t x = 0x1234u;

  stbe_u16(dst, x);

  CHECK_EQUAL(0x12u, dst[0]);
  CHECK_EQUAL(0x34u, dst[1]);
}

TEST(Util_Endian, LdbeU16_Zero) {
  const uint_least8_t src[] = {0x00u, 0x00u};

  CHECK_EQUAL(0x0000u, ldbe_u16(src));
}

TEST(Util_Endian, LdbeU16_Nonzero) {
  const uint_least8_t src[] = {0x12u, 0x34u};

  CHECK_EQUAL(0x1234u, ldbe_u16(src));
}

TEST(Util_Endian, StleI16_Zero) {
  uint_least8_t dst[] = {0x34u, 0x12u};
  const int_least16_t x = 0x0000;

  stle_i16(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
}

TEST(Util_Endian, StleI16_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u};
  const int_least16_t x = 0x1234;

  stle_i16(dst, x);

  CHECK_EQUAL(0x34u, dst[0]);
  CHECK_EQUAL(0x12u, dst[1]);
}

TEST(Util_Endian, LdleI16_Zero) {
  const uint_least8_t src[] = {0x00, 0x00};

  CHECK_EQUAL(0x0000, ldle_i16(src));
}

TEST(Util_Endian, LdleI16_Nonzero) {
  const uint_least8_t src[] = {0x12u, 0x34u};

  CHECK_EQUAL(0x3412, ldle_i16(src));
}

TEST(Util_Endian, StleU16_Zero) {
  uint_least8_t dst[] = {0x12u, 0x34u};
  const uint_least16_t x = 0x0000u;

  stle_u16(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
}

TEST(Util_Endian, StleU16_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u};
  const uint_least16_t x = 0x1234u;

  stle_u16(dst, x);

  CHECK_EQUAL(0x34u, dst[0]);
  CHECK_EQUAL(0x12u, dst[1]);
}

TEST(Util_Endian, LdleU16_Zero) {
  const uint_least8_t src[] = {0x00u, 0x00u};

  CHECK_EQUAL(0x0000u, ldle_u16(src));
}

TEST(Util_Endian, LdleU16_Nonzero) {
  const uint_least8_t src[] = {0x12u, 0x34u};

  CHECK_EQUAL(0x3412u, ldle_u16(src));
}

TEST(Util_Endian, StbeI32_Zero) {
  uint_least8_t dst[] = {0x12u, 0x34u, 0x56u, 0x78u};
  const int_least32_t x = 0x0000000000;

  stbe_i32(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0x00u, dst[3]);
}

TEST(Util_Endian, StbeI32_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u, 0x00u, 0x00u};
  const int_least32_t x = 0x12345678;

  stbe_i32(dst, x);

  CHECK_EQUAL(0x12u, dst[0]);
  CHECK_EQUAL(0x34u, dst[1]);
  CHECK_EQUAL(0x56u, dst[2]);
  CHECK_EQUAL(0x78u, dst[3]);
}

TEST(Util_Endian, LdbeI32_Zero) {
  const uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u};

  CHECK_EQUAL(0x00000000, ldbe_i32(src));
}

TEST(Util_Endian, LdbeI32_Nonzero) {
  const uint_least8_t src[] = {0x12u, 0x34u, 0x56u, 0x78u};

  CHECK_EQUAL(0x12345678, ldbe_i32(src));
}

TEST(Util_Endian, StbeU32_Zero) {
  uint_least8_t dst[] = {0x12u, 0x34u, 0x56u, 0x78u};
  const uint_least32_t x = 0x0000000000u;

  stbe_u32(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0x00u, dst[3]);
}

TEST(Util_Endian, StbeU32_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u, 0x00u, 0x00u};
  const uint_least32_t x = 0x12345678u;

  stbe_u32(dst, x);

  CHECK_EQUAL(0x12u, dst[0]);
  CHECK_EQUAL(0x34u, dst[1]);
  CHECK_EQUAL(0x56u, dst[2]);
  CHECK_EQUAL(0x78u, dst[3]);
}

TEST(Util_Endian, LdbeU32_Zero) {
  const uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u};

  CHECK_EQUAL(0x00000000u, ldbe_u32(src));
}

TEST(Util_Endian, LdbeU32_Nonzero) {
  const uint_least8_t src[] = {0x12u, 0x34u, 0x56u, 0x78u};

  CHECK_EQUAL(0x12345678u, ldbe_u32(src));
}

TEST(Util_Endian, StleI32_Zero) {
  uint_least8_t dst[] = {0x12u, 0x34u, 0x56u, 0x78u};
  const int_least32_t x = 0x0000000000;

  stle_i32(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0x00u, dst[3]);
}

TEST(Util_Endian, StleI32_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u, 0x00u, 0x00u};
  const int_least32_t x = 0x12345678;

  stle_i32(dst, x);

  CHECK_EQUAL(0x78u, dst[0]);
  CHECK_EQUAL(0x56u, dst[1]);
  CHECK_EQUAL(0x34u, dst[2]);
  CHECK_EQUAL(0x12u, dst[3]);
}

TEST(Util_Endian, LdleI32_Zero) {
  const uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u};

  CHECK_EQUAL(0x00000000, ldle_i32(src));
}

TEST(Util_Endian, LdleI32_Nonzero) {
  const uint_least8_t src[] = {0x12u, 0x34u, 0x56u, 0x78u};

  CHECK_EQUAL(0x78563412, ldle_i32(src));
}

TEST(Util_Endian, StleU32_Zero) {
  uint_least8_t dst[] = {0x12u, 0x34u, 0x56u, 0x78u};
  const uint_least32_t x = 0x0000000000u;

  stle_u32(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0x00u, dst[3]);
}

TEST(Util_Endian, StleU32_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u, 0x00u, 0x00u};
  const uint_least32_t x = 0x12345678u;

  stle_u32(dst, x);

  CHECK_EQUAL(0x78u, dst[0]);
  CHECK_EQUAL(0x56u, dst[1]);
  CHECK_EQUAL(0x34u, dst[2]);
  CHECK_EQUAL(0x12u, dst[3]);
}

TEST(Util_Endian, LdleU32_Zero) {
  const uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u};

  CHECK_EQUAL(0x00000000u, ldle_u32(src));
}

TEST(Util_Endian, LdleU32_Nonzero) {
  const uint_least8_t src[] = {0x12u, 0x34u, 0x56u, 0x78u};

  CHECK_EQUAL(0x78563412u, ldle_u32(src));
}

TEST(Util_Endian, StbeI64_Zero) {
  uint_least8_t dst[] = {0x01u, 0x23u, 0x45u, 0x67u,
                         0x89u, 0xabu, 0xcdu, 0xefu};
  const int_least64_t x = 0x00000000000000000000L;

  stbe_i64(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0x00u, dst[3]);
  CHECK_EQUAL(0x00u, dst[4]);
  CHECK_EQUAL(0x00u, dst[5]);
  CHECK_EQUAL(0x00u, dst[6]);
  CHECK_EQUAL(0x00u, dst[7]);
}

TEST(Util_Endian, StbeI64_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u, 0x00u, 0x00u,
                         0x00u, 0x00u, 0x00u, 0x00u};
  const int_least64_t x = 0x0123456789abcdefL;

  stbe_i64(dst, x);

  CHECK_EQUAL(0x01u, dst[0]);
  CHECK_EQUAL(0x23u, dst[1]);
  CHECK_EQUAL(0x45u, dst[2]);
  CHECK_EQUAL(0x67u, dst[3]);
  CHECK_EQUAL(0x89u, dst[4]);
  CHECK_EQUAL(0xabu, dst[5]);
  CHECK_EQUAL(0xcdu, dst[6]);
  CHECK_EQUAL(0xefu, dst[7]);
}

TEST(Util_Endian, LdleI64_Zero) {
  uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u,
                         0x00u, 0x00u, 0x00u, 0x00u};

  CHECK_EQUAL(0x0000000000000000L, ldle_i64(src));
}

TEST(Util_Endian, LdleI64_Nonzero) {
  const uint_least8_t src[] = {0xefu, 0xcdu, 0xabu, 0x89u,
                               0x67u, 0x45u, 0x23u, 0x01u};

  CHECK_EQUAL(0x0123456789abcdefL, ldle_i64(src));
}

TEST(Util_Endian, StbeU64_Zero) {
  uint_least8_t dst[] = {0x01u, 0x23u, 0x45u, 0x67u,
                         0x89u, 0xabu, 0xcdu, 0xefu};
  const uint_least64_t x = 0x00000000000000000000uL;

  stbe_u64(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0x00u, dst[3]);
  CHECK_EQUAL(0x00u, dst[4]);
  CHECK_EQUAL(0x00u, dst[5]);
  CHECK_EQUAL(0x00u, dst[6]);
  CHECK_EQUAL(0x00u, dst[7]);
}

TEST(Util_Endian, StbeU64_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u, 0x00u, 0x00u,
                         0x00u, 0x00u, 0x00u, 0x00u};
  const uint_least64_t x = 0x0123456789abcdefuL;

  stbe_u64(dst, x);

  CHECK_EQUAL(0x01u, dst[0]);
  CHECK_EQUAL(0x23u, dst[1]);
  CHECK_EQUAL(0x45u, dst[2]);
  CHECK_EQUAL(0x67u, dst[3]);
  CHECK_EQUAL(0x89u, dst[4]);
  CHECK_EQUAL(0xabu, dst[5]);
  CHECK_EQUAL(0xcdu, dst[6]);
  CHECK_EQUAL(0xefu, dst[7]);
}

TEST(Util_Endian, LdbeU64_Zero) {
  const uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u,
                               0x00u, 0x00u, 0x00u, 0x00u};

  CHECK_EQUAL(0x0000000000000000uL, ldbe_u64(src));
}

TEST(Util_Endian, LdbeU64_Nonzero) {
  const uint_least8_t src[] = {0x01u, 0x23u, 0x45u, 0x67u,
                               0x89u, 0xabu, 0xcdu, 0xefu};

  CHECK_EQUAL(0x0123456789abcdefuL, ldbe_u64(src));
}

TEST(Util_Endian, StleI64_Zero) {
  uint_least8_t dst[] = {0x01u, 0x23u, 0x45u, 0x67u,
                         0x89u, 0xabu, 0xcdu, 0xefu};
  const int_least64_t x = 0x00000000000000000000L;

  stle_i64(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0x00u, dst[3]);
  CHECK_EQUAL(0x00u, dst[4]);
  CHECK_EQUAL(0x00u, dst[5]);
  CHECK_EQUAL(0x00u, dst[6]);
  CHECK_EQUAL(0x00u, dst[7]);
}

TEST(Util_Endian, StleI64_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u, 0x00u, 0x00u,
                         0x00u, 0x00u, 0x00u, 0x00u};
  const int_least64_t x = 0x0123456789abcdefL;

  stle_i64(dst, x);

  CHECK_EQUAL(0xefu, dst[0]);
  CHECK_EQUAL(0xcdu, dst[1]);
  CHECK_EQUAL(0xabu, dst[2]);
  CHECK_EQUAL(0x89u, dst[3]);
  CHECK_EQUAL(0x67u, dst[4]);
  CHECK_EQUAL(0x45u, dst[5]);
  CHECK_EQUAL(0x23u, dst[6]);
  CHECK_EQUAL(0x01u, dst[7]);
}

TEST(Util_Endian, LdbeI64_Zero) {
  const uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u,
                               0x00u, 0x00u, 0x00u, 0x00u};

  CHECK_EQUAL(0x0000000000000000L, ldbe_i64(src));
}

TEST(Util_Endian, LdbeI64_Nonzero) {
  const uint_least8_t src[] = {0x01u, 0x23u, 0x45u, 0x67u,
                               0x89u, 0xabu, 0xcdu, 0xefu};

  CHECK_EQUAL(0x0123456789abcdefL, ldbe_i64(src));
}

TEST(Util_Endian, StleU64_Zero) {
  uint_least8_t dst[] = {0x01u, 0x23u, 0x45u, 0x67u,
                         0x89u, 0xabu, 0xcdu, 0xefu};
  const uint_least64_t x = 0x00000000000000000000u;

  stle_u64(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0x00u, dst[3]);
  CHECK_EQUAL(0x00u, dst[4]);
  CHECK_EQUAL(0x00u, dst[5]);
  CHECK_EQUAL(0x00u, dst[6]);
  CHECK_EQUAL(0x00u, dst[7]);
}

TEST(Util_Endian, StleU64_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u, 0x00u, 0x00u,
                         0x00u, 0x00u, 0x00u, 0x00u};
  const uint_least64_t x = 0x0123456789abcdefuL;

  stle_u64(dst, x);

  CHECK_EQUAL(0xefu, dst[0]);
  CHECK_EQUAL(0xcdu, dst[1]);
  CHECK_EQUAL(0xabu, dst[2]);
  CHECK_EQUAL(0x89u, dst[3]);
  CHECK_EQUAL(0x67u, dst[4]);
  CHECK_EQUAL(0x45u, dst[5]);
  CHECK_EQUAL(0x23u, dst[6]);
  CHECK_EQUAL(0x01u, dst[7]);
}

TEST(Util_Endian, LdleU64_Zero) {
  const uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u,
                               0x00u, 0x00u, 0x00u, 0x00u};

  CHECK_EQUAL(0x0000000000000000uL, ldle_u64(src));
}

TEST(Util_Endian, LdleU64_Nonzero) {
  const uint_least8_t src[] = {0x01u, 0x23u, 0x45u, 0x67u,
                               0x89u, 0xabu, 0xcdu, 0xefu};

  CHECK_EQUAL(0xefcdab8967452301uL, ldle_u64(src));
}

TEST(Util_Endian, StleU24_Zero) {
  uint_least8_t dst[] = {0x12u, 0x34u, 0x56u};
  const uint_least32_t x = 0;

  stle_u24(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
}

TEST(Util_Endian, StleU24_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u, 0x00u};
  const uint_least32_t x = 0x123456u;

  stle_u24(dst, x);

  CHECK_EQUAL(0x56u, dst[0]);
  CHECK_EQUAL(0x34u, dst[1]);
  CHECK_EQUAL(0x12u, dst[2]);
}

TEST(Util_Endian, LdleU24_Zero) {
  const uint_least8_t src[] = {0x00u, 0x00u, 0x00u};

  CHECK_EQUAL(0, ldle_u24(src));
}

TEST(Util_Endian, LdleU24_Nonzero) {
  const uint_least8_t src[] = {0x12u, 0x34u, 0x56u};

  CHECK_EQUAL(0x563412u, ldle_u24(src));
}

TEST(Util_Endian, StbeFlt32_Zero) {
  const flt32_t x = 0.0;
  uint_least8_t dst[] = {0xffu, 0xffu, 0xffu, 0xffu};

  stbe_flt32(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0x00u, dst[3]);
}

TEST(Util_Endian, LdbeFlt32_Zero) {
  const uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u};

  const flt32_t x = ldbe_flt32(src);

  LONGS_EQUAL(0.0, x);
}

TEST(Util_Endian, StbeFlt32) {
  const flt32_t x = 128.5;
  uint_least8_t dst[] = {0x00u, 0x00u, 0x00u, 0x00u};

  stbe_flt32(dst, x);

  CHECK_EQUAL(0x43u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x80u, dst[2]);
  CHECK_EQUAL(0x00u, dst[3]);
}

TEST(Util_Endian, LdbeFlt32) {
  const uint_least8_t src[] = {0x43u, 0x00u, 0x80u, 0x00u};

  const flt32_t x = ldbe_flt32(src);

  LONGS_EQUAL(128.5, x);
}

TEST(Util_Endian, StleFlt32_Zero) {
  const flt32_t x = 0.0;
  uint_least8_t dst[] = {0xffu, 0xffu, 0xffu, 0xffu};

  stle_flt32(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0x00u, dst[3]);
}

TEST(Util_Endian, LdleFlt32_Zero) {
  const uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u};

  const flt32_t x = ldle_flt32(src);

  LONGS_EQUAL(0.0, x);
}

TEST(Util_Endian, StleFlt32) {
  const flt32_t x = 128.5;
  uint_least8_t dst[] = {0x00u, 0x00u, 0x00u, 0x00u};

  stle_flt32(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x80u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0x43u, dst[3]);
}

TEST(Util_Endian, LdleFlt32) {
  const uint_least8_t src[] = {0x00u, 0x80u, 0x00u, 0x43u};

  const flt32_t x = ldle_flt32(src);

  LONGS_EQUAL(128.5, x);
}

TEST(Util_Endian, StleFlt64_Zero) {
  const flt64_t x = 0.0;
  uint_least8_t dst[] = {0xffu, 0xffu, 0xffu, 0xffu,
                         0xffu, 0xffu, 0xffu, 0xffu};

  stle_flt64(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0x00u, dst[3]);
  CHECK_EQUAL(0x00u, dst[4]);
  CHECK_EQUAL(0x00u, dst[5]);
  CHECK_EQUAL(0x00u, dst[6]);
  CHECK_EQUAL(0x00u, dst[7]);
}

TEST(Util_Endian, LdleFlt64_Zero) {
  const uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u,
                               0x00u, 0x00u, 0x00u, 0x00u};

  const flt64_t x = ldle_flt64(src);

  LONGS_EQUAL(0.0, x);
}

TEST(Util_Endian, StleFlt64) {
  const flt32_t x = 128.6780905;
  uint_least8_t dst[] = {0x00u, 0x00u, 0x00u, 0x00u,
                         0x00u, 0x00u, 0x00u, 0x00u};

  stle_flt64(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0xe0u, dst[3]);
  CHECK_EQUAL(0xb2u, dst[4]);
  CHECK_EQUAL(0x15u, dst[5]);
  CHECK_EQUAL(0x60u, dst[6]);
  CHECK_EQUAL(0x40u, dst[7]);
}

TEST(Util_Endian, LdleFlt64) {
  const uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0xe0u,
                               0xb2u, 0x15u, 0x60u, 0x40u};

  const flt64_t x = ldle_flt64(src);

  LONGS_EQUAL(128.6780905, x);
}

TEST(Util_Endian, StbeFlt64_Zero) {
  const flt64_t x = 0.0;
  uint_least8_t dst[] = {0xffu, 0xffu, 0xffu, 0xffu,
                         0xffu, 0xffu, 0xffu, 0xffu};

  stbe_flt64(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0x00u, dst[3]);
  CHECK_EQUAL(0x00u, dst[4]);
  CHECK_EQUAL(0x00u, dst[5]);
  CHECK_EQUAL(0x00u, dst[6]);
  CHECK_EQUAL(0x00u, dst[7]);
}

TEST(Util_Endian, LdbeFlt64_Zero) {
  const uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u,
                               0x00u, 0x00u, 0x00u, 0x00u};

  const flt64_t x = ldbe_flt64(src);

  LONGS_EQUAL(0.0, x);
}

TEST(Util_Endian, StbeFlt64) {
  const flt32_t x = 128.6780905;
  uint_least8_t dst[] = {0x00u, 0x00u, 0x00u, 0x00u,
                         0x00u, 0x00u, 0x00u, 0x00u};

  stbe_flt64(dst, x);

  CHECK_EQUAL(0x40u, dst[0]);
  CHECK_EQUAL(0x60u, dst[1]);
  CHECK_EQUAL(0x15u, dst[2]);
  CHECK_EQUAL(0xb2u, dst[3]);
  CHECK_EQUAL(0xe0u, dst[4]);
  CHECK_EQUAL(0x00u, dst[5]);
  CHECK_EQUAL(0x00u, dst[6]);
  CHECK_EQUAL(0x00u, dst[7]);
}

TEST(Util_Endian, LdbeFlt64) {
  const uint_least8_t src[] = {0x40u, 0x60u, 0x15u, 0xb2u,
                               0xe0u, 0x00u, 0x00u, 0x00u};

  const flt64_t x = ldbe_flt64(src);

  LONGS_EQUAL(128.6780905, x);
}
