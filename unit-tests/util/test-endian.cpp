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

TEST_GROUP(Util_Endian){};

TEST(Util_Endian, Htobe16) {
  CHECK_EQUAL(0x0000U, htobe16(0x0000U));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x3412U, htobe16(0x1234U));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x1234U, htobe16(0x1234U));
#endif
}

TEST(Util_Endian, Betoh16) {
  CHECK_EQUAL(0x0000U, betoh16(0x0000U));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x3412U, betoh16(0x1234U));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x1234U, betoh16(0x1234U));
#endif
}

TEST(Util_Endian, Htole16) {
  CHECK_EQUAL(0x0000U, htobe16(0x0000U));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x3412U, htobe16(0x1234U));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x1234U, htobe16(0x1234U));
#endif
}

TEST(Util_Endian, Letoh16) {
  CHECK_EQUAL(0x0000U, letoh16(0x0000U));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x1234U, letoh16(0x1234U));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x3412U, letoh16(0x1234U));
#endif
}

TEST(Util_Endian, Htobe32) {
  CHECK_EQUAL(0x00000000U, htobe32(0x00000000U));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x78563412U, htobe32(0x12345678U));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x12345678U, htobe32(0x12345678U));
#endif
}

TEST(Util_Endian, Betoh32) {
  CHECK_EQUAL(0x00000000U, betoh32(0x00000000U));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x78563412U, betoh32(0x12345678U));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x12345678U, betoh32(0x12345678U));
#endif
}

TEST(Util_Endian, Htole32) {
  CHECK_EQUAL(0x00000000U, htole32(0x00000000U));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x12345678U, htole32(0x12345678U));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x78563412U, htole32(0x12345678U));
#endif
}

TEST(Util_Endian, Letoh32) {
  CHECK_EQUAL(0x00000000U, letoh32(0x00000000U));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x12345678U, letoh32(0x12345678U));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x78563412U, letoh32(0x12345678U));
#endif
}

TEST(Util_Endian, Htobe64) {
  CHECK_EQUAL(0x0000000000000000U, htobe64(0x0000000000000000U));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0xEFCDAB8967452301U, htobe64(0x0123456789ABCDEFU));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x0123456789ABCDEFU, htobe64(0x0123456789ABCDEFU));
#endif
}

TEST(Util_Endian, Betoh64) {
  CHECK_EQUAL(0x0000000000000000U, betoh64(0x0000000000000000U));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0xEFCDAB8967452301U, betoh64(0x0123456789ABCDEFU));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x0123456789ABCDEFU, betoh64(0x0123456789ABCDEFU));
#endif
}

TEST(Util_Endian, Htole64) {
  CHECK_EQUAL(0x0000000000000000U, htole64(0x0000000000000000U));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x0123456789ABCDEFU, htole64(0x0123456789ABCDEFU));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0x0EFCDAB8967452301U, htole64(0x0123456789ABCDEFU));
#endif
}

TEST(Util_Endian, Letoh64) {
  CHECK_EQUAL(0x0000000000000000U, letoh64(0x0000000000000000U));
#if LELY_LITTLE_ENDIAN
  CHECK_EQUAL(0x0123456789ABCDEFU, letoh64(0x0123456789ABCDEFU));
#elif LELY_BIG_ENDIAN
  CHECK_EQUAL(0xEFCDAB8967452301U, letoh64(0x0123456789ABCDEFU));
#endif
}

TEST(Util_Endian, StbeI16_Zero) {
  uint_least8_t dst[] = {0x12U, 0x34U};
  int_least16_t x = 0x0000;

  stbe_i16(dst, x);

  CHECK_EQUAL(0x00U, dst[0]);
  CHECK_EQUAL(0x00U, dst[1]);
}

TEST(Util_Endian, StbeI16_Nonzero) {
  uint_least8_t dst[] = {0x00U, 0x00U};
  int_least16_t x = 0x1234;

  stbe_i16(dst, x);

  CHECK_EQUAL(0x12U, dst[0]);
  CHECK_EQUAL(0x34U, dst[1]);
}

TEST(Util_Endian, LdbeI16) {
  uint_least8_t src[] = {0x00U, 0x00U};
  CHECK_EQUAL(0x0000, ldbe_i16(src));
  src[0] = 0x12U;
  src[1] = 0x34U;
  CHECK_EQUAL(0x1234, ldbe_i16(src));
}

TEST(Util_Endian, StbeU16_Zero) {
  uint_least8_t dst[] = {0x12U, 0x34U};
  uint_least16_t x = 0x0000U;

  stbe_u16(dst, x);

  CHECK_EQUAL(0x00U, dst[0]);
  CHECK_EQUAL(0x00U, dst[1]);
}

TEST(Util_Endian, StbeU16_Nonzero) {
  uint_least8_t dst[] = {0x00U, 0x00U};
  uint_least16_t x = 0x1234U;

  stbe_u16(dst, x);

  CHECK_EQUAL(0x12U, dst[0]);
  CHECK_EQUAL(0x34U, dst[1]);
}

TEST(Util_Endian, LdbeU16) {
  uint_least8_t src[] = {0x00U, 0x00U};
  CHECK_EQUAL(0x0000U, ldbe_u16(src));
  src[0] = 0x12U;
  src[1] = 0x34U;
  CHECK_EQUAL(0x1234U, ldbe_u16(src));
}

TEST(Util_Endian, StleI16_Zero) {
  uint_least8_t dst[] = {0x34U, 0x12U};
  int_least16_t x = 0x0000;

  stle_i16(dst, x);

  CHECK_EQUAL(0x00U, dst[0]);
  CHECK_EQUAL(0x00U, dst[1]);
}

TEST(Util_Endian, StleI16_Nonzero) {
  uint_least8_t dst[] = {0x00U, 0x00U};
  int_least16_t x = 0x1234;

  stle_i16(dst, x);

  CHECK_EQUAL(0x34U, dst[0]);
  CHECK_EQUAL(0x12U, dst[1]);
}

TEST(Util_Endian, LdleI16) {
  uint_least8_t src[] = {0x00, 0x00};
  CHECK_EQUAL(0x0000, ldle_i16(src));
  src[0] = 0x12;
  src[1] = 0x34;
  CHECK_EQUAL(0x3412, ldle_i16(src));
}

TEST(Util_Endian, StleU16_Zero) {
  uint_least8_t dst[] = {0x12U, 0x34U};
  uint_least16_t x = 0x0000U;

  stle_u16(dst, x);

  CHECK_EQUAL(0x00U, dst[0]);
  CHECK_EQUAL(0x00U, dst[1]);
}

TEST(Util_Endian, StleU16_Nonzero) {
  uint_least8_t dst[] = {0x00U, 0x00U};
  uint_least16_t x = 0x1234U;

  stle_u16(dst, x);

  CHECK_EQUAL(0x34U, dst[0]);
  CHECK_EQUAL(0x12U, dst[1]);
}

TEST(Util_Endian, LdleU16) {
  uint_least8_t src[] = {0x00U, 0x00U};
  CHECK_EQUAL(0x0000U, ldle_u16(src));
  src[0] = 0x12U;
  src[1] = 0x34U;
  CHECK_EQUAL(0x3412U, ldle_u16(src));
}

TEST(Util_Endian, StbeI32_Zero) {
  uint_least8_t dst[] = {0x12U, 0x34U, 0x56U, 0x78U};
  int_least32_t x = 0x0000000000;

  stbe_i32(dst, x);

  CHECK_EQUAL(0x00U, dst[0]);
  CHECK_EQUAL(0x00U, dst[1]);
  CHECK_EQUAL(0x00U, dst[2]);
  CHECK_EQUAL(0x00U, dst[3]);
}

TEST(Util_Endian, StbeI32_Nonzero) {
  uint_least8_t dst[] = {0x00U, 0x00U, 0x00U, 0x00U};
  int_least32_t x = 0x12345678;

  stbe_i32(dst, x);

  CHECK_EQUAL(0x12U, dst[0]);
  CHECK_EQUAL(0x34U, dst[1]);
  CHECK_EQUAL(0x56U, dst[2]);
  CHECK_EQUAL(0x78U, dst[3]);
}

TEST(Util_Endian, LdbeI32) {
  uint_least8_t src[] = {0x00U, 0x00U, 0x00U, 0x00U};
  CHECK_EQUAL(0x00000000, ldbe_i32(src));
  src[0] = 0x12U;
  src[1] = 0x34U;
  src[2] = 0x56U;
  src[3] = 0x78U;
  CHECK_EQUAL(0x12345678, ldbe_i32(src));
}

TEST(Util_Endian, StbeU32_Zero) {
  uint_least8_t dst[] = {0x12U, 0x34U, 0x56U, 0x78U};
  uint_least32_t x = 0x0000000000U;

  stbe_u32(dst, x);

  CHECK_EQUAL(0x00U, dst[0]);
  CHECK_EQUAL(0x00U, dst[1]);
  CHECK_EQUAL(0x00U, dst[2]);
  CHECK_EQUAL(0x00U, dst[3]);
}

TEST(Util_Endian, StbeU32_Nonzero) {
  uint_least8_t dst[] = {0x00U, 0x00U, 0x00U, 0x00U};
  uint_least32_t x = 0x12345678U;

  stbe_u32(dst, x);

  CHECK_EQUAL(0x12U, dst[0]);
  CHECK_EQUAL(0x34U, dst[1]);
  CHECK_EQUAL(0x56U, dst[2]);
  CHECK_EQUAL(0x78U, dst[3]);
}

TEST(Util_Endian, LdbeU32) {
  uint_least8_t src[] = {0x00U, 0x00U, 0x00U, 0x00U};
  CHECK_EQUAL(0x00000000U, ldbe_u32(src));
  src[0] = 0x12U;
  src[1] = 0x34U;
  src[2] = 0x56U;
  src[3] = 0x78U;
  CHECK_EQUAL(0x12345678U, ldbe_u32(src));
}

TEST(Util_Endian, StleI32_Zero) {
  uint_least8_t dst[] = {0x12U, 0x34U, 0x56U, 0x78U};
  int_least32_t x = 0x0000000000;

  stle_i32(dst, x);

  CHECK_EQUAL(0x00U, dst[0]);
  CHECK_EQUAL(0x00U, dst[1]);
  CHECK_EQUAL(0x00U, dst[2]);
  CHECK_EQUAL(0x00U, dst[3]);
}

TEST(Util_Endian, StleI32_Nonzero) {
  uint_least8_t dst[] = {0x00U, 0x00U, 0x00U, 0x00U};
  int_least32_t x = 0x12345678;

  stle_i32(dst, x);

  CHECK_EQUAL(0x78U, dst[0]);
  CHECK_EQUAL(0x56U, dst[1]);
  CHECK_EQUAL(0x34U, dst[2]);
  CHECK_EQUAL(0x12U, dst[3]);
}

TEST(Util_Endian, LdleI32) {
  uint_least8_t src[] = {0x00U, 0x00U, 0x00U, 0x00U};
  CHECK_EQUAL(0x00000000, ldle_i32(src));
  src[0] = 0x12U;
  src[1] = 0x34U;
  src[2] = 0x56U;
  src[3] = 0x78U;
  CHECK_EQUAL(0x78563412, ldle_i32(src));
}

TEST(Util_Endian, StleU32_Zero) {
  uint_least8_t dst[] = {0x12U, 0x34U, 0x56U, 0x78U};
  uint_least32_t x = 0x0000000000U;

  stle_u32(dst, x);

  CHECK_EQUAL(0x00U, dst[0]);
  CHECK_EQUAL(0x00U, dst[1]);
  CHECK_EQUAL(0x00U, dst[2]);
  CHECK_EQUAL(0x00U, dst[3]);
}

TEST(Util_Endian, StleU32_Nonzero) {
  uint_least8_t dst[] = {0x00U, 0x00U, 0x00U, 0x00U};
  uint_least32_t x = 0x12345678U;

  stle_u32(dst, x);

  CHECK_EQUAL(0x78U, dst[0]);
  CHECK_EQUAL(0x56U, dst[1]);
  CHECK_EQUAL(0x34U, dst[2]);
  CHECK_EQUAL(0x12U, dst[3]);
}

TEST(Util_Endian, LdleU32) {
  uint_least8_t src[] = {0x00U, 0x00U, 0x00U, 0x00U};
  CHECK_EQUAL(0x00000000U, ldle_u32(src));
  src[0] = 0x12U;
  src[1] = 0x34U;
  src[2] = 0x56U;
  src[3] = 0x78U;
  CHECK_EQUAL(0x78563412U, ldle_u32(src));
}

TEST(Util_Endian, StbeI64_Zero) {
  uint_least8_t dst[] = {0x01U, 0x23U, 0x45U, 0x67U,
                         0x89U, 0xABU, 0xCDU, 0xEFU};
  int_least64_t x = 0x00000000000000000000L;

  stbe_i64(dst, x);

  CHECK_EQUAL(0x00U, dst[0]);
  CHECK_EQUAL(0x00U, dst[1]);
  CHECK_EQUAL(0x00U, dst[2]);
  CHECK_EQUAL(0x00U, dst[3]);
  CHECK_EQUAL(0x00U, dst[4]);
  CHECK_EQUAL(0x00U, dst[5]);
  CHECK_EQUAL(0x00U, dst[6]);
  CHECK_EQUAL(0x00U, dst[7]);
}

TEST(Util_Endian, StbeI64_Nonzero) {
  uint_least8_t dst[] = {0x00U, 0x00U, 0x00U, 0x00U,
                         0x00U, 0x00U, 0x00U, 0x00U};
  int_least64_t x = 0x0123456789ABCDEFL;

  stbe_i64(dst, x);

  CHECK_EQUAL(0x01U, dst[0]);
  CHECK_EQUAL(0x23U, dst[1]);
  CHECK_EQUAL(0x45U, dst[2]);
  CHECK_EQUAL(0x67U, dst[3]);
  CHECK_EQUAL(0x89U, dst[4]);
  CHECK_EQUAL(0xABU, dst[5]);
  CHECK_EQUAL(0xCDU, dst[6]);
  CHECK_EQUAL(0xEFU, dst[7]);
}

TEST(Util_Endian, LdleI64) {
  uint_least8_t src[] = {0x00U, 0x00U, 0x00U, 0x00U,
                         0x00U, 0x00U, 0x00U, 0x00U};
  CHECK_EQUAL(0x0000000000000000L, ldle_i64(src));
  src[0] = 0xEFU;
  src[1] = 0xCDU;
  src[2] = 0xABU;
  src[3] = 0x89U;
  src[4] = 0x67U;
  src[5] = 0x45U;
  src[6] = 0x23U;
  src[7] = 0x01U;
  CHECK_EQUAL(0x0123456789ABCDEFL, ldle_i64(src));
}

TEST(Util_Endian, StbeU64_Zero) {
  uint_least8_t dst[] = {0x01U, 0x23U, 0x45U, 0x67U,
                         0x89U, 0xABU, 0xCDU, 0xEFU};
  uint_least64_t x = 0x00000000000000000000UL;

  stbe_u64(dst, x);

  CHECK_EQUAL(0x00U, dst[0]);
  CHECK_EQUAL(0x00U, dst[1]);
  CHECK_EQUAL(0x00U, dst[2]);
  CHECK_EQUAL(0x00U, dst[3]);
  CHECK_EQUAL(0x00U, dst[4]);
  CHECK_EQUAL(0x00U, dst[5]);
  CHECK_EQUAL(0x00U, dst[6]);
  CHECK_EQUAL(0x00U, dst[7]);
}

TEST(Util_Endian, StbeU64_Nonzero) {
  uint_least8_t dst[] = {0x00U, 0x00U, 0x00U, 0x00U,
                         0x00U, 0x00U, 0x00U, 0x00U};
  uint_least64_t x = 0x0123456789ABCDEFUL;

  stbe_u64(dst, x);

  CHECK_EQUAL(0x01U, dst[0]);
  CHECK_EQUAL(0x23U, dst[1]);
  CHECK_EQUAL(0x45U, dst[2]);
  CHECK_EQUAL(0x67U, dst[3]);
  CHECK_EQUAL(0x89U, dst[4]);
  CHECK_EQUAL(0xABU, dst[5]);
  CHECK_EQUAL(0xCDU, dst[6]);
  CHECK_EQUAL(0xEFU, dst[7]);
}

TEST(Util_Endian, LdbeU64) {
  uint_least8_t src[] = {0x00U, 0x00U, 0x00U, 0x00U,
                         0x00U, 0x00U, 0x00U, 0x00U};
  CHECK_EQUAL(0x0000000000000000UL, ldbe_u64(src));
  src[0] = 0x01U;
  src[1] = 0x23U;
  src[2] = 0x45U;
  src[3] = 0x67U;
  src[4] = 0x89U;
  src[5] = 0xABU;
  src[6] = 0xCDU;
  src[7] = 0xEFU;
  CHECK_EQUAL(0x0123456789ABCDEFUL, ldbe_u64(src));
}

TEST(Util_Endian, StleI64_Zero) {
  uint_least8_t dst[] = {0x01U, 0x23U, 0x45U, 0x67U,
                         0x89U, 0xABU, 0xCDU, 0xEFU};
  int_least64_t x = 0x00000000000000000000L;

  stle_i64(dst, x);

  CHECK_EQUAL(0x00U, dst[0]);
  CHECK_EQUAL(0x00U, dst[1]);
  CHECK_EQUAL(0x00U, dst[2]);
  CHECK_EQUAL(0x00U, dst[3]);
  CHECK_EQUAL(0x00U, dst[4]);
  CHECK_EQUAL(0x00U, dst[5]);
  CHECK_EQUAL(0x00U, dst[6]);
  CHECK_EQUAL(0x00U, dst[7]);
}

TEST(Util_Endian, StleI64_Nonzero) {
  uint_least8_t dst[] = {0x00U, 0x00U, 0x00U, 0x00U,
                         0x00U, 0x00U, 0x00U, 0x00U};
  int_least64_t x = 0x0123456789ABCDEFL;

  stle_i64(dst, x);

  CHECK_EQUAL(0xEFU, dst[0]);
  CHECK_EQUAL(0xCDU, dst[1]);
  CHECK_EQUAL(0xABU, dst[2]);
  CHECK_EQUAL(0x89U, dst[3]);
  CHECK_EQUAL(0x67U, dst[4]);
  CHECK_EQUAL(0x45U, dst[5]);
  CHECK_EQUAL(0x23U, dst[6]);
  CHECK_EQUAL(0x01U, dst[7]);
}

TEST(Util_Endian, LdbeI64) {
  uint_least8_t src[] = {0x00U, 0x00U, 0x00U, 0x00U,
                         0x00U, 0x00U, 0x00U, 0x00U};
  CHECK_EQUAL(0x0000000000000000L, ldbe_i64(src));
  src[0] = 0x01U;
  src[1] = 0x23U;
  src[2] = 0x45U;
  src[3] = 0x67U;
  src[4] = 0x89U;
  src[5] = 0xABU;
  src[6] = 0xCDU;
  src[7] = 0xEFU;
  CHECK_EQUAL(0x0123456789ABCDEFL, ldbe_i64(src));
}

TEST(Util_Endian, StleU64_Zero) {
  uint_least8_t dst[] = {0x01U, 0x23U, 0x45U, 0x67U,
                         0x89U, 0xABU, 0xCDU, 0xEFU};
  uint_least64_t x = 0x00000000000000000000U;

  stle_u64(dst, x);

  CHECK_EQUAL(0x00U, dst[0]);
  CHECK_EQUAL(0x00U, dst[1]);
  CHECK_EQUAL(0x00U, dst[2]);
  CHECK_EQUAL(0x00U, dst[3]);
  CHECK_EQUAL(0x00U, dst[4]);
  CHECK_EQUAL(0x00U, dst[5]);
  CHECK_EQUAL(0x00U, dst[6]);
  CHECK_EQUAL(0x00U, dst[7]);
}

TEST(Util_Endian, StleU64_Nonzero) {
  uint_least8_t dst[] = {0x00U, 0x00U, 0x00U, 0x00U,
                         0x00U, 0x00U, 0x00U, 0x00U};
  uint_least64_t x = 0x0123456789ABCDEFUL;

  stle_u64(dst, x);

  CHECK_EQUAL(0xEFU, dst[0]);
  CHECK_EQUAL(0xCDU, dst[1]);
  CHECK_EQUAL(0xABU, dst[2]);
  CHECK_EQUAL(0x89U, dst[3]);
  CHECK_EQUAL(0x67U, dst[4]);
  CHECK_EQUAL(0x45U, dst[5]);
  CHECK_EQUAL(0x23U, dst[6]);
  CHECK_EQUAL(0x01U, dst[7]);
}

TEST(Util_Endian, LdleU64) {
  uint_least8_t src[] = {0x00U, 0x00U, 0x00U, 0x00U,
                         0x00U, 0x00U, 0x00U, 0x00U};
  CHECK_EQUAL(0x0000000000000000UL, ldle_u64(src));
  src[0] = 0x01U;
  src[1] = 0x23U;
  src[2] = 0x45U;
  src[3] = 0x67U;
  src[4] = 0x89U;
  src[5] = 0xABU;
  src[6] = 0xCDU;
  src[7] = 0xEFU;
  CHECK_EQUAL(0xEFCDAB8967452301UL, ldle_u64(src));
}
