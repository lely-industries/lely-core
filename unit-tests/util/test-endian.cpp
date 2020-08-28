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

  CHECK_EQUAL(0xFFu, *dst);
}

TEST(Util_Endian_Bcpy, Bcpybe_DstbitPlusNMoreThan8) {
  const int dstbit = 3;
  const int srcbit = 2;
  const size_t n = 6u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xE6u, *dst);
}

TEST(Util_Endian_Bcpy, Bcpybe_LastIsZero) {
  const int dstbit = 3;
  const int srcbit = 2;
  const size_t n = 5u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xE6u, *dst);
}

TEST(Util_Endian_Bcpy, Bcpyle_LastIsZero) {
  const int dstbit = 3;
  const int srcbit = 2;
  const size_t n = 5u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0x9Fu, *dst);
}

TEST(Util_Endian_Bcpy, Bcpyle_NoShiftDstbitPlusNMoreThan8) {
  const int dstbit = 2;
  const int srcbit = 2;
  const size_t n = 17u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xCFu, *dst);
}

TEST(Util_Endian_Bcpy, Bcpyle_NoShiftDstbitPlusNMoreThan8LastZero) {
  const int dstbit = 2;
  const int srcbit = 2;
  const size_t n = 22u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xCFu, *dst);
}

TEST(Util_Endian_Bcpy, Bcpyle_NoShiftDstbitPlusNLessThan8LastNotZero) {
  const int dstbit = 6;
  const int srcbit = 6;
  const size_t n = 1u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xFFu, *dst);
}

TEST(Util_Endian_Bcpy, Bcpyle_CopyAll) {
  const int dstbit = 0;
  const int srcbit = 0;
  const size_t n = 8u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(*src, *dst);
}

TEST_GROUP(Util_Endian_BcpyOutOfRange) {
  uint_least8_t dst_array[4u] = {0xFFu, 0xFFu, 0xFFu, 0xFFu};
  const uint_least8_t src_array[4u] = {0x00u, 0xCCu, 0x00u, 0x00u};
  uint_least8_t* const dst = &dst_array[1u];
  const uint_least8_t* const src = &src_array[1u];
};

TEST(Util_Endian_BcpyOutOfRange, Bcpybe_DstbitIsZeroCopyMoreThan8) {
  const int dstbit = 0;
  const int srcbit = 0;
  const size_t n = 9u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xCCu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpybe_PositiveShiftSrcbitPlusNMoreThan8) {
  const int dstbit = 3;
  const int srcbit = 4;
  const size_t n = 5u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xF8u, *dst);
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

  CHECK_EQUAL(0xD8u, *dst);
}

TEST(Util_Endian_BcpyOutOfRange,
     Bcpybe_NegativeShiftDstbitPlusNMoreThan8LastIsZero) {
  const int dstbit = 2;
  const int srcbit = 3;
  const size_t n = 22u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xD8u, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpybe_NoShiftDstbitPlusNMoreThan8) {
  const int dstbit = 2;
  const int srcbit = 2;
  const size_t n = 17u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xCCu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpybe_NoShiftDstbitPlusNMoreThan8LastZero) {
  const int dstbit = 2;
  const int srcbit = 2;
  const size_t n = 22u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xCCu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange,
     Bcpybe_NoShiftDstbitPlusNLessThan8LastNotZero) {
  const int dstbit = 6;
  const int srcbit = 6;
  const size_t n = 1u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xFDu, *dst);
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

  CHECK_EQUAL(0xCCu, *dst);
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

  CHECK_EQUAL(0xFFu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpyle_DstbitPlusNMoreThan8) {
  const int dstbit = 3;
  const int srcbit = 2;
  const size_t n = 6u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0x9Fu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpybe_OffsetsLowerThan0) {
  const int dstbit = -1;
  const int srcbit = -1;
  const size_t n = 4u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xDFu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpybe_SrcbitOffsetLowerThan0) {
  const int dstbit = 0;
  const int srcbit = -1;
  const size_t n = 1u;

  bcpybe(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0x7Fu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpyle_OffsetsLowerThan0) {
  const int dstbit = -1;
  const int srcbit = -1;
  const size_t n = 4u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xFCu, *dst);
}

TEST(Util_Endian_BcpyOutOfRange, Bcpyle_SrcbitOffsetLowerThan0) {
  const int dstbit = 0;
  const int srcbit = -1;
  const size_t n = 1u;

  bcpyle(dst, dstbit, src, srcbit, n);

  CHECK_EQUAL(0xFEu, *dst);
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
  CHECK_EQUAL(0x0000u, htobe16(0x0000u));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x3412u, htobe16(0x1234u));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x1234u, htobe16(0x1234u));
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
  CHECK_EQUAL(0xEFCDAB8967452301u, htobe64(0x0123456789ABCDEFu));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x0123456789ABCDEFu, htobe64(0x0123456789ABCDEFu));
#endif
}

TEST(Util_Endian, Betoh64) {
  CHECK_EQUAL(0x0000000000000000u, betoh64(0x0000000000000000u));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0xEFCDAB8967452301u, betoh64(0x0123456789ABCDEFu));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x0123456789ABCDEFu, betoh64(0x0123456789ABCDEFu));
#endif
}

TEST(Util_Endian, Htole64) {
  CHECK_EQUAL(0x0000000000000000u, htole64(0x0000000000000000u));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x0123456789ABCDEFu, htole64(0x0123456789ABCDEFu));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x0EFCDAB8967452301u, htole64(0x0123456789ABCDEFu));
#endif
}

TEST(Util_Endian, Letoh64) {
  CHECK_EQUAL(0x0000000000000000u, letoh64(0x0000000000000000u));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x0123456789ABCDEFu, letoh64(0x0123456789ABCDEFu));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0xEFCDAB8967452301u, letoh64(0x0123456789ABCDEFu));
#endif
}

TEST(Util_Endian, StbeI16_Zero) {
  uint_least8_t dst[] = {0x12u, 0x34u};
  int_least16_t x = 0x0000;

  stbe_i16(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
}

TEST(Util_Endian, StbeI16_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u};
  int_least16_t x = 0x1234;

  stbe_i16(dst, x);

  CHECK_EQUAL(0x12u, dst[0]);
  CHECK_EQUAL(0x34u, dst[1]);
}

TEST(Util_Endian, LdbeI16) {
  uint_least8_t src[] = {0x00u, 0x00u};
  CHECK_EQUAL(0x0000, ldbe_i16(src));
  src[0] = 0x12u;
  src[1] = 0x34u;
  CHECK_EQUAL(0x1234, ldbe_i16(src));
}

TEST(Util_Endian, StbeU16_Zero) {
  uint_least8_t dst[] = {0x12u, 0x34u};
  uint_least16_t x = 0x0000u;

  stbe_u16(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
}

TEST(Util_Endian, StbeU16_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u};
  uint_least16_t x = 0x1234u;

  stbe_u16(dst, x);

  CHECK_EQUAL(0x12u, dst[0]);
  CHECK_EQUAL(0x34u, dst[1]);
}

TEST(Util_Endian, LdbeU16) {
  uint_least8_t src[] = {0x00u, 0x00u};
  CHECK_EQUAL(0x0000u, ldbe_u16(src));
  src[0] = 0x12u;
  src[1] = 0x34u;
  CHECK_EQUAL(0x1234u, ldbe_u16(src));
}

TEST(Util_Endian, StleI16_Zero) {
  uint_least8_t dst[] = {0x34u, 0x12u};
  int_least16_t x = 0x0000;

  stle_i16(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
}

TEST(Util_Endian, StleI16_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u};
  int_least16_t x = 0x1234;

  stle_i16(dst, x);

  CHECK_EQUAL(0x34u, dst[0]);
  CHECK_EQUAL(0x12u, dst[1]);
}

TEST(Util_Endian, LdleI16) {
  uint_least8_t src[] = {0x00, 0x00};
  CHECK_EQUAL(0x0000, ldle_i16(src));
  src[0] = 0x12;
  src[1] = 0x34;
  CHECK_EQUAL(0x3412, ldle_i16(src));
}

TEST(Util_Endian, StleU16_Zero) {
  uint_least8_t dst[] = {0x12u, 0x34u};
  uint_least16_t x = 0x0000u;

  stle_u16(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
}

TEST(Util_Endian, StleU16_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u};
  uint_least16_t x = 0x1234u;

  stle_u16(dst, x);

  CHECK_EQUAL(0x34u, dst[0]);
  CHECK_EQUAL(0x12u, dst[1]);
}

TEST(Util_Endian, LdleU16) {
  uint_least8_t src[] = {0x00u, 0x00u};
  CHECK_EQUAL(0x0000u, ldle_u16(src));
  src[0] = 0x12u;
  src[1] = 0x34u;
  CHECK_EQUAL(0x3412u, ldle_u16(src));
}

TEST(Util_Endian, StbeI32_Zero) {
  uint_least8_t dst[] = {0x12u, 0x34u, 0x56u, 0x78u};
  int_least32_t x = 0x0000000000;

  stbe_i32(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0x00u, dst[3]);
}

TEST(Util_Endian, StbeI32_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u, 0x00u, 0x00u};
  int_least32_t x = 0x12345678;

  stbe_i32(dst, x);

  CHECK_EQUAL(0x12u, dst[0]);
  CHECK_EQUAL(0x34u, dst[1]);
  CHECK_EQUAL(0x56u, dst[2]);
  CHECK_EQUAL(0x78u, dst[3]);
}

TEST(Util_Endian, LdbeI32) {
  uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u};
  CHECK_EQUAL(0x00000000, ldbe_i32(src));
  src[0] = 0x12u;
  src[1] = 0x34u;
  src[2] = 0x56u;
  src[3] = 0x78u;
  CHECK_EQUAL(0x12345678, ldbe_i32(src));
}

TEST(Util_Endian, StbeU32_Zero) {
  uint_least8_t dst[] = {0x12u, 0x34u, 0x56u, 0x78u};
  uint_least32_t x = 0x0000000000u;

  stbe_u32(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0x00u, dst[3]);
}

TEST(Util_Endian, StbeU32_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u, 0x00u, 0x00u};
  uint_least32_t x = 0x12345678u;

  stbe_u32(dst, x);

  CHECK_EQUAL(0x12u, dst[0]);
  CHECK_EQUAL(0x34u, dst[1]);
  CHECK_EQUAL(0x56u, dst[2]);
  CHECK_EQUAL(0x78u, dst[3]);
}

TEST(Util_Endian, LdbeU32) {
  uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u};
  CHECK_EQUAL(0x00000000u, ldbe_u32(src));
  src[0] = 0x12u;
  src[1] = 0x34u;
  src[2] = 0x56u;
  src[3] = 0x78u;
  CHECK_EQUAL(0x12345678u, ldbe_u32(src));
}

TEST(Util_Endian, StleI32_Zero) {
  uint_least8_t dst[] = {0x12u, 0x34u, 0x56u, 0x78u};
  int_least32_t x = 0x0000000000;

  stle_i32(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0x00u, dst[3]);
}

TEST(Util_Endian, StleI32_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u, 0x00u, 0x00u};
  int_least32_t x = 0x12345678;

  stle_i32(dst, x);

  CHECK_EQUAL(0x78u, dst[0]);
  CHECK_EQUAL(0x56u, dst[1]);
  CHECK_EQUAL(0x34u, dst[2]);
  CHECK_EQUAL(0x12u, dst[3]);
}

TEST(Util_Endian, LdleI32) {
  uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u};
  CHECK_EQUAL(0x00000000, ldle_i32(src));
  src[0] = 0x12u;
  src[1] = 0x34u;
  src[2] = 0x56u;
  src[3] = 0x78u;
  CHECK_EQUAL(0x78563412, ldle_i32(src));
}

TEST(Util_Endian, StleU32_Zero) {
  uint_least8_t dst[] = {0x12u, 0x34u, 0x56u, 0x78u};
  uint_least32_t x = 0x0000000000u;

  stle_u32(dst, x);

  CHECK_EQUAL(0x00u, dst[0]);
  CHECK_EQUAL(0x00u, dst[1]);
  CHECK_EQUAL(0x00u, dst[2]);
  CHECK_EQUAL(0x00u, dst[3]);
}

TEST(Util_Endian, StleU32_Nonzero) {
  uint_least8_t dst[] = {0x00u, 0x00u, 0x00u, 0x00u};
  uint_least32_t x = 0x12345678u;

  stle_u32(dst, x);

  CHECK_EQUAL(0x78u, dst[0]);
  CHECK_EQUAL(0x56u, dst[1]);
  CHECK_EQUAL(0x34u, dst[2]);
  CHECK_EQUAL(0x12u, dst[3]);
}

TEST(Util_Endian, LdleU32) {
  uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u};
  CHECK_EQUAL(0x00000000u, ldle_u32(src));
  src[0] = 0x12u;
  src[1] = 0x34u;
  src[2] = 0x56u;
  src[3] = 0x78u;
  CHECK_EQUAL(0x78563412u, ldle_u32(src));
}

TEST(Util_Endian, StbeI64_Zero) {
  uint_least8_t dst[] = {0x01u, 0x23u, 0x45u, 0x67u,
                         0x89u, 0xABu, 0xCDu, 0xEFu};
  int_least64_t x = 0x00000000000000000000L;

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
  int_least64_t x = 0x0123456789ABCDEFL;

  stbe_i64(dst, x);

  CHECK_EQUAL(0x01u, dst[0]);
  CHECK_EQUAL(0x23u, dst[1]);
  CHECK_EQUAL(0x45u, dst[2]);
  CHECK_EQUAL(0x67u, dst[3]);
  CHECK_EQUAL(0x89u, dst[4]);
  CHECK_EQUAL(0xABu, dst[5]);
  CHECK_EQUAL(0xCDu, dst[6]);
  CHECK_EQUAL(0xEFu, dst[7]);
}

TEST(Util_Endian, LdleI64) {
  uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u,
                         0x00u, 0x00u, 0x00u, 0x00u};
  CHECK_EQUAL(0x0000000000000000L, ldle_i64(src));
  src[0] = 0xEFu;
  src[1] = 0xCDu;
  src[2] = 0xABu;
  src[3] = 0x89u;
  src[4] = 0x67u;
  src[5] = 0x45u;
  src[6] = 0x23u;
  src[7] = 0x01u;
  CHECK_EQUAL(0x0123456789ABCDEFL, ldle_i64(src));
}

TEST(Util_Endian, StbeU64_Zero) {
  uint_least8_t dst[] = {0x01u, 0x23u, 0x45u, 0x67u,
                         0x89u, 0xABu, 0xCDu, 0xEFu};
  uint_least64_t x = 0x00000000000000000000UL;

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
  uint_least64_t x = 0x0123456789ABCDEFUL;

  stbe_u64(dst, x);

  CHECK_EQUAL(0x01u, dst[0]);
  CHECK_EQUAL(0x23u, dst[1]);
  CHECK_EQUAL(0x45u, dst[2]);
  CHECK_EQUAL(0x67u, dst[3]);
  CHECK_EQUAL(0x89u, dst[4]);
  CHECK_EQUAL(0xABu, dst[5]);
  CHECK_EQUAL(0xCDu, dst[6]);
  CHECK_EQUAL(0xEFu, dst[7]);
}

TEST(Util_Endian, LdbeU64) {
  uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u,
                         0x00u, 0x00u, 0x00u, 0x00u};
  CHECK_EQUAL(0x0000000000000000UL, ldbe_u64(src));
  src[0] = 0x01u;
  src[1] = 0x23u;
  src[2] = 0x45u;
  src[3] = 0x67u;
  src[4] = 0x89u;
  src[5] = 0xABu;
  src[6] = 0xCDu;
  src[7] = 0xEFu;
  CHECK_EQUAL(0x0123456789ABCDEFUL, ldbe_u64(src));
}

TEST(Util_Endian, StleI64_Zero) {
  uint_least8_t dst[] = {0x01u, 0x23u, 0x45u, 0x67u,
                         0x89u, 0xABu, 0xCDu, 0xEFu};
  int_least64_t x = 0x00000000000000000000L;

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
  int_least64_t x = 0x0123456789ABCDEFL;

  stle_i64(dst, x);

  CHECK_EQUAL(0xEFu, dst[0]);
  CHECK_EQUAL(0xCDu, dst[1]);
  CHECK_EQUAL(0xABu, dst[2]);
  CHECK_EQUAL(0x89u, dst[3]);
  CHECK_EQUAL(0x67u, dst[4]);
  CHECK_EQUAL(0x45u, dst[5]);
  CHECK_EQUAL(0x23u, dst[6]);
  CHECK_EQUAL(0x01u, dst[7]);
}

TEST(Util_Endian, LdbeI64) {
  uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u,
                         0x00u, 0x00u, 0x00u, 0x00u};
  CHECK_EQUAL(0x0000000000000000L, ldbe_i64(src));
  src[0] = 0x01u;
  src[1] = 0x23u;
  src[2] = 0x45u;
  src[3] = 0x67u;
  src[4] = 0x89u;
  src[5] = 0xABu;
  src[6] = 0xCDu;
  src[7] = 0xEFu;
  CHECK_EQUAL(0x0123456789ABCDEFL, ldbe_i64(src));
}

TEST(Util_Endian, StleU64_Zero) {
  uint_least8_t dst[] = {0x01u, 0x23u, 0x45u, 0x67u,
                         0x89u, 0xABu, 0xCDu, 0xEFu};
  uint_least64_t x = 0x00000000000000000000u;

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
  uint_least64_t x = 0x0123456789ABCDEFUL;

  stle_u64(dst, x);

  CHECK_EQUAL(0xEFu, dst[0]);
  CHECK_EQUAL(0xCDu, dst[1]);
  CHECK_EQUAL(0xABu, dst[2]);
  CHECK_EQUAL(0x89u, dst[3]);
  CHECK_EQUAL(0x67u, dst[4]);
  CHECK_EQUAL(0x45u, dst[5]);
  CHECK_EQUAL(0x23u, dst[6]);
  CHECK_EQUAL(0x01u, dst[7]);
}

TEST(Util_Endian, LdleU64) {
  uint_least8_t src[] = {0x00u, 0x00u, 0x00u, 0x00u,
                         0x00u, 0x00u, 0x00u, 0x00u};
  CHECK_EQUAL(0x0000000000000000UL, ldle_u64(src));
  src[0] = 0x01u;
  src[1] = 0x23u;
  src[2] = 0x45u;
  src[3] = 0x67u;
  src[4] = 0x89u;
  src[5] = 0xABu;
  src[6] = 0xCDu;
  src[7] = 0xEFu;
  CHECK_EQUAL(0xEFCDAB8967452301UL, ldle_u64(src));
}
