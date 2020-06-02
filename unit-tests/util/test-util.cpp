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

#include <cfloat>
#include <climits>

#include <CppUTest/TestHarness.h>

#include <lely/util/util.h>

TEST_GROUP(Util_Util){};

TEST(Util_Util, Abs_Ints) {
  CHECK_EQUAL(0, ABS(0));
  CHECK_EQUAL(0, ABS(-0));
  CHECK_EQUAL(1, ABS(1));
  CHECK_EQUAL(1, ABS(-1));
  CHECK_EQUAL(CHAR_MAX, ABS(CHAR_MAX));
  CHECK_EQUAL(CHAR_MAX, ABS(-CHAR_MAX));
  CHECK_EQUAL(SHRT_MAX, ABS(SHRT_MAX));
  CHECK_EQUAL(SHRT_MAX, ABS(-SHRT_MAX));
  CHECK_EQUAL(INT_MAX, ABS(INT_MAX));
  CHECK_EQUAL(INT_MAX, ABS(-INT_MAX));
}

TEST(Util_Util, Abs_LongInts) {
  CHECK_EQUAL(0L, ABS(0L));
  CHECK_EQUAL(0L, ABS(-0L));
  CHECK_EQUAL(1L, ABS(1L));
  CHECK_EQUAL(1L, ABS(-1L));
  CHECK_EQUAL(LONG_MAX, ABS(LONG_MAX));
  CHECK_EQUAL(LONG_MAX, ABS(-LONG_MAX));
}

TEST(Util_Util, Abs_LongLongInts) {
  CHECK_EQUAL(0LL, ABS(0LL));
  CHECK_EQUAL(0LL, ABS(-0LL));
  CHECK_EQUAL(1LL, ABS(1LL));
  CHECK_EQUAL(1LL, ABS(-1LL));
  CHECK_EQUAL(LONG_LONG_MAX, ABS(LONG_LONG_MAX));
  CHECK_EQUAL(LONG_LONG_MAX, ABS(-LONG_LONG_MAX));
}

TEST(Util_Util, Abs_Doubles) {
  CHECK_EQUAL(DBL_MIN, ABS(-DBL_MIN));
  CHECK_EQUAL(DBL_MIN, ABS(DBL_MIN));
  CHECK_EQUAL(0.0, ABS(0.0));
  CHECK_EQUAL(0.0, ABS(-0.0));
  CHECK_EQUAL(1.0, ABS(1.0));
  CHECK_EQUAL(1.0, ABS(-1.0));
  CHECK_EQUAL(DBL_MAX, ABS(DBL_MAX));
  CHECK_EQUAL(DBL_MAX, ABS(-DBL_MAX));
}

TEST(Util_Util, Abs_Floats) {
  CHECK_EQUAL(FLT_MIN, ABS(FLT_MIN));
  CHECK_EQUAL(FLT_MIN, ABS(-FLT_MIN));
  CHECK_EQUAL(0.0f, ABS(0.0f));
  CHECK_EQUAL(0.0f, ABS(-0.0f));
  CHECK_EQUAL(1.0f, ABS(1.0f));
  CHECK_EQUAL(1.0f, ABS(-1.0f));
  CHECK_EQUAL(FLT_MAX, ABS(FLT_MAX));
  CHECK_EQUAL(FLT_MAX, ABS(-FLT_MAX));
}

TEST(Util_Util, Align_Base2BigNegatives) {
  int base = 2;

  CHECK_EQUAL(-LONG_LONG_MAX + 1, ALIGN(-LONG_LONG_MAX, base));
  CHECK_EQUAL(-LONG_MAX + 1, ALIGN(-LONG_MAX, base));
  CHECK_EQUAL(-INT_MAX + 1, ALIGN(-INT_MAX, base));
  CHECK_EQUAL(-SHRT_MAX + 1, ALIGN(-SHRT_MAX, base));
  CHECK_EQUAL(-CHAR_MAX + 1, ALIGN(-CHAR_MAX, base));
  CHECK_EQUAL(-2133254542ULL, ALIGN(-2133254543ULL, base));
  CHECK_EQUAL(-21332544UL, ALIGN(-21332545UL, base));
  CHECK_EQUAL(-21332524LL, ALIGN(-21332525LL, base));
  CHECK_EQUAL(-213324L, ALIGN(-213325L, base));
  CHECK_EQUAL(-31536, ALIGN(-31536, base));
}

TEST(Util_Util, Align_Base2SmallInts) {
  int base = 2;

  CHECK_EQUAL(-31534, ALIGN(-31535, base));
  CHECK_EQUAL(-6, ALIGN(-7, base));
  CHECK_EQUAL(-6, ALIGN(-6, base));
  CHECK_EQUAL(-4, ALIGN(-5, base));
  CHECK_EQUAL(-4, ALIGN(-4, base));
  CHECK_EQUAL(-2, ALIGN(-3, base));
  CHECK_EQUAL(-2, ALIGN(-2, base));
  CHECK_EQUAL(0, ALIGN(-1, base));
  CHECK_EQUAL(0, ALIGN(0, base));
  CHECK_EQUAL(2, ALIGN(1, base));
  CHECK_EQUAL(2, ALIGN(2, base));
  CHECK_EQUAL(4, ALIGN(3, base));
  CHECK_EQUAL(4, ALIGN(4, base));
  CHECK_EQUAL(6, ALIGN(5, base));
  CHECK_EQUAL(6, ALIGN(6, base));
  CHECK_EQUAL(31536, ALIGN(31536, base));
}

TEST(Util_Util, Align_Base2BigPositives) {
  int base = 2;

  CHECK_EQUAL(213326L, ALIGN(213325L, base));
  CHECK_EQUAL(21332526LL, ALIGN(21332525LL, base));
  CHECK_EQUAL(21332546UL, ALIGN(21332545UL, base));
  CHECK_EQUAL(2133254544ULL, ALIGN(2133254543ULL, base));
  CHECK_EQUAL(LONG_LONG_MAX - 1, ALIGN(LONG_LONG_MAX - 1, base));
  CHECK_EQUAL(ULONG_LONG_MAX - 1, ALIGN(ULONG_LONG_MAX - 1, base));
  CHECK_EQUAL(LONG_MAX - 1, ALIGN(LONG_MAX - 1, base));
  CHECK_EQUAL(ULONG_MAX - 1, ALIGN(ULONG_MAX - 1, base));
  CHECK_EQUAL(INT_MAX - 1, ALIGN(INT_MAX - 1, base));
  CHECK_EQUAL(0, ALIGN(UINT_MAX, base));
  CHECK_EQUAL(static_cast<int16_t>(SHRT_MAX - 1),
              ALIGN(static_cast<int16_t>(SHRT_MAX - 2), base));
  CHECK_EQUAL(static_cast<char>(CHAR_MAX - 1),
              ALIGN(static_cast<char>(CHAR_MAX - 2), base));
}

TEST(Util_Util, Align_Base0BigNegatives) {
  int base = 0;

  CHECK_EQUAL(0LL, ALIGN(-LONG_LONG_MAX, base));
  CHECK_EQUAL(0L, ALIGN(-LONG_MAX, base));
  CHECK_EQUAL(0, ALIGN(-INT_MAX, base));
  CHECK_EQUAL(0, ALIGN(-SHRT_MAX, base));
  CHECK_EQUAL(0, ALIGN(-CHAR_MAX, base));
  CHECK_EQUAL(0ULL, ALIGN(-2133254543ULL, base));
  CHECK_EQUAL(0UL, ALIGN(-21332545UL, base));
  CHECK_EQUAL(0LL, ALIGN(-21332525LL, base));
  CHECK_EQUAL(0L, ALIGN(-213325L, base));
}

TEST(Util_Util, Align_Base0SmallInts) {
  int base = 0;

  CHECK_EQUAL(0, ALIGN(-31535, base));
  CHECK_EQUAL(0, ALIGN(-1, base));
  CHECK_EQUAL(0, ALIGN(0, base));
  CHECK_EQUAL(0, ALIGN(1, base));
  CHECK_EQUAL(0, ALIGN(31536, base));
}

TEST(Util_Util, Align_Base0BigPositives) {
  int base = 0;

  CHECK_EQUAL(0L, ALIGN(213325L, base));
  CHECK_EQUAL(0LL, ALIGN(21332525LL, base));
  CHECK_EQUAL(0UL, ALIGN(21332545UL, base));
  CHECK_EQUAL(0ULL, ALIGN(2133254543ULL, base));
  CHECK_EQUAL(0LL, ALIGN(LONG_LONG_MAX - 1, base));
  CHECK_EQUAL(0ULL, ALIGN(ULONG_LONG_MAX - 1, base));
  CHECK_EQUAL(0L, ALIGN(LONG_MAX - 1, base));
  CHECK_EQUAL(0L, ALIGN(ULONG_MAX - 1, base));
  CHECK_EQUAL(0, ALIGN(INT_MAX - 1, base));
  CHECK_EQUAL(0U, ALIGN(UINT_MAX, base));
  CHECK_EQUAL(static_cast<int16_t>(0),
              ALIGN(static_cast<int16_t>(SHRT_MAX - 2), base));
  CHECK_EQUAL(static_cast<char>(0),
              ALIGN(static_cast<char>(CHAR_MAX - 2), base));
}

TEST(Util_Util, Align_Base4BigNegatives) {
  int base = 4;

  CHECK_EQUAL(-LONG_LONG_MAX + 3, ALIGN(-LONG_LONG_MAX, base));
  CHECK_EQUAL(-LONG_MAX + 3, ALIGN(-LONG_MAX, base));
  CHECK_EQUAL(-INT_MAX + 3, ALIGN(-INT_MAX, base));
  CHECK_EQUAL(-SHRT_MAX + 3, ALIGN(-SHRT_MAX, base));
  CHECK_EQUAL(-CHAR_MAX + 3, ALIGN(-CHAR_MAX, base));
  CHECK_EQUAL(-2133254540ULL, ALIGN(-2133254543ULL, base));
  CHECK_EQUAL(-21332544UL, ALIGN(-21332545UL, base));
  CHECK_EQUAL(-21332524LL, ALIGN(-21332525LL, base));
  CHECK_EQUAL(-213324L, ALIGN(-213325L, base));
  CHECK_EQUAL(-31536, ALIGN(-31536, base));
}

TEST(Util_Util, Align_Base4SmallInts) {
  int base = 4;

  CHECK_EQUAL(-31532, ALIGN(-31535, base));
  CHECK_EQUAL(-4, ALIGN(-7, base));
  CHECK_EQUAL(-4, ALIGN(-6, base));
  CHECK_EQUAL(-4, ALIGN(-5, base));
  CHECK_EQUAL(-4, ALIGN(-4, base));
  CHECK_EQUAL(0, ALIGN(-3, base));
  CHECK_EQUAL(0, ALIGN(-2, base));
  CHECK_EQUAL(0, ALIGN(-1, base));
  CHECK_EQUAL(0, ALIGN(0, base));
  CHECK_EQUAL(4, ALIGN(1, base));
  CHECK_EQUAL(4, ALIGN(2, base));
  CHECK_EQUAL(4, ALIGN(3, base));
  CHECK_EQUAL(4, ALIGN(4, base));
  CHECK_EQUAL(8, ALIGN(5, base));
  CHECK_EQUAL(8, ALIGN(6, base));
  CHECK_EQUAL(31536, ALIGN(31536, base));
}

TEST(Util_Util, Align_Base4BigPositives) {
  int base = 4;

  CHECK_EQUAL(213328L, ALIGN(213325L, base));
  CHECK_EQUAL(21332528LL, ALIGN(21332525LL, base));
  CHECK_EQUAL(21332548UL, ALIGN(21332545UL, base));
  CHECK_EQUAL(2133254544ULL, ALIGN(2133254543ULL, base));
  CHECK_EQUAL(LONG_LONG_MAX - 3, ALIGN(LONG_LONG_MAX - 4, base));
  CHECK_EQUAL(ULONG_LONG_MAX - 3, ALIGN(ULONG_LONG_MAX - 4, base));
  CHECK_EQUAL(LONG_MAX - 3, ALIGN(LONG_MAX - 4, base));
  CHECK_EQUAL(ULONG_MAX - 3, ALIGN(ULONG_MAX - 4, base));
  CHECK_EQUAL(INT_MAX - 3, ALIGN(INT_MAX - 4, base));
  CHECK_EQUAL(0, ALIGN(UINT_MAX, base));
  CHECK_EQUAL(static_cast<int16_t>(SHRT_MAX - 3),
              ALIGN(static_cast<int16_t>(SHRT_MAX - 5), base));
  CHECK_EQUAL(static_cast<char>(CHAR_MAX - 3),
              ALIGN(static_cast<char>(CHAR_MAX - 5), base));
}

TEST(Util_Util, Align_BigBaseBigNegatives) {
  int base = 4096;

  CHECK_EQUAL(-LONG_LONG_MAX + 4095, ALIGN(-LONG_LONG_MAX, base));
  CHECK_EQUAL(-LONG_MAX + 4095, ALIGN(-LONG_MAX, base));
  CHECK_EQUAL(-INT_MAX + 4095, ALIGN(-INT_MAX, base));
  CHECK_EQUAL(-SHRT_MAX + 4095, ALIGN(-SHRT_MAX, base));
  CHECK_EQUAL(0, ALIGN(-CHAR_MAX, base));
  CHECK_EQUAL(-21331968LL, ALIGN(-21332525LL, base));
  CHECK_EQUAL(-212992L, ALIGN(-213325L, base));
  CHECK_EQUAL(-28672, ALIGN(-31536, base));
}

TEST(Util_Util, Align_BigBaseSmallInts) {
  int base = 4096;

  CHECK_EQUAL(-28672, ALIGN(-31535, base));
  CHECK_EQUAL(0, ALIGN(-7, base));
  CHECK_EQUAL(0, ALIGN(-2, base));
  CHECK_EQUAL(0, ALIGN(-1, base));
  CHECK_EQUAL(0, ALIGN(0, base));
  CHECK_EQUAL(4096, ALIGN(1, base));
  CHECK_EQUAL(4096, ALIGN(2, base));
  CHECK_EQUAL(32768, ALIGN(31536, base));
}

TEST(Util_Util, Align_BigBaseBigPositives) {
  int base = 4096;

  CHECK_EQUAL(217088L, ALIGN(213325L, base));
  CHECK_EQUAL(21336064LL, ALIGN(21332525LL, base));
  CHECK_EQUAL(21336064UL, ALIGN(21332545UL, base));
  CHECK_EQUAL(2133258240ULL, ALIGN(2133254543ULL, base));
  CHECK_EQUAL(LONG_LONG_MAX - 4095, ALIGN(LONG_LONG_MAX - 4097, base));
  CHECK_EQUAL(ULONG_LONG_MAX - 4095, ALIGN(ULONG_LONG_MAX - 4097, base));
  CHECK_EQUAL(LONG_MAX - 4095, ALIGN(LONG_MAX - 4097, base));
  CHECK_EQUAL(ULONG_MAX - 4095, ALIGN(ULONG_MAX - 4097, base));
  CHECK_EQUAL(INT_MAX - 4095, ALIGN(INT_MAX - 4097, base));
  CHECK_EQUAL(0, ALIGN(UINT_MAX, base));
  CHECK_EQUAL(static_cast<int16_t>(SHRT_MAX - 4095),
              ALIGN(static_cast<int16_t>(SHRT_MAX - 4097), base));
}

TEST(Util_Util, AlignMask_Mask0x01BigNegatives) {
  int mask = 0x01;

  CHECK_EQUAL(-LONG_LONG_MAX + 1, ALIGN_MASK(-LONG_LONG_MAX, mask));
  CHECK_EQUAL(-LONG_MAX + 1, ALIGN_MASK(-LONG_MAX, mask));
  CHECK_EQUAL(-INT_MAX + 1, ALIGN_MASK(-INT_MAX, mask));
  CHECK_EQUAL(-SHRT_MAX + 1, ALIGN_MASK(-SHRT_MAX, mask));
  CHECK_EQUAL(-CHAR_MAX + 1, ALIGN_MASK(-CHAR_MAX, mask));
  CHECK_EQUAL(-21332524LL, ALIGN_MASK(-21332525LL, mask));
  CHECK_EQUAL(-213324L, ALIGN_MASK(-213325L, mask));
  CHECK_EQUAL(-31536, ALIGN_MASK(-31536, mask));
}

TEST(Util_Util, AlignMask_Mask0x01SmallInts) {
  int mask = 0x01;

  CHECK_EQUAL(-31534, ALIGN_MASK(-31535, mask));
  CHECK_EQUAL(-6, ALIGN_MASK(-7, mask));
  CHECK_EQUAL(-6, ALIGN_MASK(-6, mask));
  CHECK_EQUAL(-4, ALIGN_MASK(-5, mask));
  CHECK_EQUAL(-4, ALIGN_MASK(-4, mask));
  CHECK_EQUAL(-2, ALIGN_MASK(-3, mask));
  CHECK_EQUAL(-2, ALIGN_MASK(-2, mask));
  CHECK_EQUAL(0, ALIGN_MASK(-1, mask));
  CHECK_EQUAL(0, ALIGN_MASK(0, mask));
  CHECK_EQUAL(2, ALIGN_MASK(1, mask));
  CHECK_EQUAL(2, ALIGN_MASK(2, mask));
  CHECK_EQUAL(4, ALIGN_MASK(3, mask));
  CHECK_EQUAL(4, ALIGN_MASK(4, mask));
  CHECK_EQUAL(6, ALIGN_MASK(5, mask));
  CHECK_EQUAL(6, ALIGN_MASK(6, mask));
  CHECK_EQUAL(31536, ALIGN_MASK(31536, mask));
}

TEST(Util_Util, AlignMask_Mask0x01BigPositives) {
  int mask = 0x01;

  CHECK_EQUAL(213326L, ALIGN_MASK(213325L, mask));
  CHECK_EQUAL(21332526LL, ALIGN_MASK(21332525LL, mask));
  CHECK_EQUAL(21332546UL, ALIGN_MASK(21332545UL, mask));
  CHECK_EQUAL(2133254544ULL, ALIGN_MASK(2133254543ULL, mask));
  CHECK_EQUAL(LONG_LONG_MAX - 3, ALIGN_MASK(LONG_LONG_MAX - 4, mask));
  CHECK_EQUAL(ULONG_LONG_MAX - 3, ALIGN_MASK(ULONG_LONG_MAX - 4, mask));
  CHECK_EQUAL(LONG_MAX - 3, ALIGN_MASK(LONG_MAX - 4, mask));
  CHECK_EQUAL(ULONG_MAX - 3, ALIGN_MASK(ULONG_MAX - 4, mask));
  CHECK_EQUAL(INT_MAX - 3, ALIGN_MASK(INT_MAX - 4, mask));
  CHECK_EQUAL(0, ALIGN_MASK(UINT_MAX, mask));
  CHECK_EQUAL(static_cast<int16_t>(SHRT_MAX - 5),
              ALIGN_MASK(static_cast<int16_t>(SHRT_MAX - 5), mask));
  CHECK_EQUAL(static_cast<char>(CHAR_MAX - 5),
              ALIGN_MASK(static_cast<char>(CHAR_MAX - 5), mask));
}

TEST(Util_Util, AlignMask_Mask0x04BigNegatives) {
  int mask = 0x04;

  CHECK_EQUAL(-LONG_LONG_MAX, ALIGN_MASK(-LONG_LONG_MAX, mask));
  CHECK_EQUAL(-LONG_MAX, ALIGN_MASK(-LONG_MAX, mask));
  CHECK_EQUAL(-INT_MAX, ALIGN_MASK(-INT_MAX, mask));
  CHECK_EQUAL(-SHRT_MAX, ALIGN_MASK(-SHRT_MAX, mask));
  CHECK_EQUAL(-CHAR_MAX, ALIGN_MASK(-CHAR_MAX, mask));
  CHECK_EQUAL(-21332525LL, ALIGN_MASK(-21332525LL, mask));
  CHECK_EQUAL(-213325L, ALIGN_MASK(-213325L, mask));
  CHECK_EQUAL(-31536, ALIGN_MASK(-31536, mask));
}

TEST(Util_Util, AlignMask_Mask0x04SmallInts) {
  int mask = 0x04;

  CHECK_EQUAL(-31535, ALIGN_MASK(-31535, mask));
  CHECK_EQUAL(-7, ALIGN_MASK(-7, mask));
  CHECK_EQUAL(-6, ALIGN_MASK(-6, mask));
  CHECK_EQUAL(-5, ALIGN_MASK(-5, mask));
  CHECK_EQUAL(0, ALIGN_MASK(-4, mask));
  CHECK_EQUAL(1, ALIGN_MASK(-3, mask));
  CHECK_EQUAL(2, ALIGN_MASK(-2, mask));
  CHECK_EQUAL(3, ALIGN_MASK(-1, mask));
  CHECK_EQUAL(0, ALIGN_MASK(0, mask));
  CHECK_EQUAL(1, ALIGN_MASK(1, mask));
  CHECK_EQUAL(2, ALIGN_MASK(2, mask));
  CHECK_EQUAL(3, ALIGN_MASK(3, mask));
  CHECK_EQUAL(8, ALIGN_MASK(4, mask));
  CHECK_EQUAL(9, ALIGN_MASK(5, mask));
  CHECK_EQUAL(10, ALIGN_MASK(6, mask));
  CHECK_EQUAL(31536, ALIGN_MASK(31536, mask));
}

TEST(Util_Util, AlignMask_Mask0x04BigPositives) {
  int mask = 0x04;

  CHECK_EQUAL(213329L, ALIGN_MASK(213325L, mask));
  CHECK_EQUAL(21332529LL, ALIGN_MASK(21332525LL, mask));
  CHECK_EQUAL(21332545UL, ALIGN_MASK(21332545UL, mask));
  CHECK_EQUAL(2133254547ULL, ALIGN_MASK(2133254543ULL, mask));
  CHECK_EQUAL(LONG_LONG_MAX - 4, ALIGN_MASK(LONG_LONG_MAX - 4, mask));
  CHECK_EQUAL(ULONG_LONG_MAX - 4, ALIGN_MASK(ULONG_LONG_MAX - 4, mask));
  CHECK_EQUAL(LONG_MAX - 4, ALIGN_MASK(LONG_MAX - 4, mask));
  CHECK_EQUAL(ULONG_MAX - 4, ALIGN_MASK(ULONG_MAX - 4, mask));
  CHECK_EQUAL(INT_MAX - 4, ALIGN_MASK(INT_MAX - 4, mask));
  CHECK_EQUAL(3, ALIGN_MASK(UINT_MAX, mask));
  CHECK_EQUAL(static_cast<int16_t>(SHRT_MAX - 5),
              ALIGN_MASK(static_cast<int16_t>(SHRT_MAX - 5), mask));
  CHECK_EQUAL(static_cast<char>(CHAR_MAX - 5),
              ALIGN_MASK(static_cast<char>(CHAR_MAX - 5), mask));
}

TEST(Util_Util, AlignMask_BigMaskBigNegatives) {
  int mask = 0xABD9;

  CHECK_EQUAL(-LONG_LONG_MAX + 1, ALIGN_MASK(-LONG_LONG_MAX, mask));
  CHECK_EQUAL(-LONG_MAX + 1, ALIGN_MASK(-LONG_MAX, mask));
  CHECK_EQUAL(-INT_MAX + 1, ALIGN_MASK(-INT_MAX, mask));
  CHECK_EQUAL(2, ALIGN_MASK(-SHRT_MAX, mask));
  CHECK_EQUAL(2, ALIGN_MASK(-CHAR_MAX, mask));
  CHECK_EQUAL(-21299164LL, ALIGN_MASK(-21332525LL, mask));
  CHECK_EQUAL(-180221L, ALIGN(-213325L, mask));
  CHECK_EQUAL(4128, ALIGN(-31536, mask));
}

TEST(Util_Util, AlignMask_BigMaskSmallInts) {
  int mask = 0xABD9;

  CHECK_EQUAL(4130, ALIGN_MASK(-31535, mask));
  CHECK_EQUAL(2, ALIGN_MASK(-7, mask));
  CHECK_EQUAL(6, ALIGN_MASK(-2, mask));
  CHECK_EQUAL(0, ALIGN_MASK(-1, mask));
  CHECK_EQUAL(0, ALIGN_MASK(0, mask));
  CHECK_EQUAL(2, ALIGN_MASK(1, mask));
  CHECK_EQUAL(2, ALIGN_MASK(2, mask));
  CHECK_EQUAL(66560, ALIGN_MASK(31536, mask));
}

TEST(Util_Util, AlignMask_BigMaskBigPositives) {
  int mask = 0xABD9;

  CHECK_EQUAL(214054L, ALIGN_MASK(213325L, mask));
  CHECK_EQUAL(21365766LL, ALIGN_MASK(21332525LL, mask));
  CHECK_EQUAL(21365762UL, ALIGN_MASK(21332545UL, mask));
  CHECK_EQUAL(2133263392ULL, ALIGN_MASK(2133254543ULL, mask));
  CHECK_EQUAL(-LONG_LONG_MAX + 4101, ALIGN_MASK(LONG_LONG_MAX - 4097, mask));
  CHECK_EQUAL(4102, ALIGN_MASK(ULONG_LONG_MAX - 4097, mask));
  CHECK_EQUAL(-LONG_MAX + 4101, ALIGN_MASK(LONG_MAX - 4097, mask));
  CHECK_EQUAL(4102, ALIGN_MASK(ULONG_MAX - 4097, mask));
  CHECK_EQUAL(-INT_MAX + 4101, ALIGN_MASK(INT_MAX - 4097, mask));
  CHECK_EQUAL(0, ALIGN_MASK(UINT_MAX, mask));
}

TEST(Util_Util, Min) {
  CHECK_EQUAL(0, MIN(0, 0));
  CHECK_EQUAL(0, MIN(-0, 0));
  CHECK_EQUAL(1, MIN(1, 1));
  CHECK_EQUAL(1, MIN(1, 2));
  CHECK_EQUAL(1, MIN(2, 1));
  CHECK_EQUAL(0, MIN(CHAR_MAX, 0));
  CHECK_EQUAL(CHAR_MAX - 1, MIN(CHAR_MAX, CHAR_MAX - 1));
  CHECK_EQUAL(0, MIN(0, SHRT_MAX));
  CHECK_EQUAL(1.0, MIN(1.0, 1.1));
  CHECK_EQUAL(LONG_MAX - 1, MIN(LONG_MAX - 1, LONG_MAX));
  CHECK_EQUAL(LONG_LONG_MAX, MIN(LONG_LONG_MAX, LONG_LONG_MAX));
  CHECK_EQUAL(0.001, MIN(0.001, DBL_MAX));
  CHECK_EQUAL(DBL_MIN, MIN(0.001, DBL_MIN));

  CHECK_EQUAL(-1.0, MIN(-1.0, -0.5));
  CHECK_EQUAL(-5, MIN(-1, -5));
  CHECK_EQUAL(-1, MIN(-1, 1));
  CHECK_EQUAL(-1, MIN(1, -1));
  CHECK_EQUAL(0.0f, MIN(0.0f, 0.0001f));
  CHECK_EQUAL(-FLT_MAX, MIN(-FLT_MAX, FLT_MAX));
}

TEST(Util_Util, Max) {
  CHECK_EQUAL(0, MAX(0, 0));
  CHECK_EQUAL(0, MAX(-0, 0));
  CHECK_EQUAL(1, MAX(1, 1));
  CHECK_EQUAL(2, MAX(1, 2));
  CHECK_EQUAL(2, MAX(2, 1));
  CHECK_EQUAL(CHAR_MAX, MAX(CHAR_MAX, 0));
  CHECK_EQUAL(CHAR_MAX, MAX(CHAR_MAX, CHAR_MAX - 1));
  CHECK_EQUAL(SHRT_MAX, MAX(0, SHRT_MAX));
  CHECK_EQUAL(1.1, MAX(1.0, 1.1));
  CHECK_EQUAL(LONG_MAX, MAX(LONG_MAX - 1, LONG_MAX));
  CHECK_EQUAL(LONG_LONG_MAX, MAX(LONG_LONG_MAX, LONG_LONG_MAX));
  CHECK_EQUAL(DBL_MAX, MAX(0.001, DBL_MAX));
  CHECK_EQUAL(0.001, MAX(0.001, DBL_MIN));

  CHECK_EQUAL(-0.5, MAX(-1.0, -0.5));
  CHECK_EQUAL(-1, MAX(-1, -5));
  CHECK_EQUAL(1, MAX(-1, 1));
  CHECK_EQUAL(1, MAX(1, -1));
  CHECK_EQUAL(0.0001f, MAX(0.0f, 0.0001f));
  CHECK_EQUAL(FLT_MAX, MAX(-FLT_MAX, FLT_MAX));
}

TEST(Util_Util, Countof) {
  int a[1];
  CHECK_EQUAL(1, countof(a));
  int b[2];
  CHECK_EQUAL(2, countof(b));
  int c[128];
  CHECK_EQUAL(128, countof(c));
}

TEST(Util_Util, Powerof2) {
  CHECK_EQUAL(0, powerof2(-3342));
  CHECK_EQUAL(0, powerof2(-2));
  CHECK_EQUAL(0, powerof2(-1));
  CHECK_EQUAL(1, powerof2(0));
  CHECK_EQUAL(1, powerof2(1));
  CHECK_EQUAL(1, powerof2(2));
  CHECK_EQUAL(0, powerof2(3));
  CHECK_EQUAL(1, powerof2(4));
  CHECK_EQUAL(0, powerof2(5));
  CHECK_EQUAL(0, powerof2(6));
  CHECK_EQUAL(1, powerof2(8));
  CHECK_EQUAL(0, powerof2(3243));
}

TEST(Util_Util, Powerof2_NearBounds) {
  CHECK_EQUAL(0, powerof2(-INT_MAX));
  CHECK_EQUAL(0, powerof2(-SHRT_MAX - 1));

  CHECK_EQUAL(1, powerof2(SHRT_MAX + 1));
  CHECK_EQUAL(0, powerof2(INT_MAX));
  CHECK_EQUAL(1, powerof2(static_cast<int64_t>(INT_MAX) + 1L));
}

TEST(Util_Util, Structof_Example) {
  struct TestNode {
    int x;
  };

  struct TestObject {
    TestNode node;
    int val;
  };

  const TestNode test_node_init = {0};
  const TestObject test_object = {test_node_init, 0};
  const TestNode* const test_node_ptr = &test_object.node;

  POINTERS_EQUAL(&test_object, structof(test_node_ptr, TestObject, node));
}

TEST(Util_Util, Structof_General) {
  struct TestStruct {
    int a;
    int b;
  };

  const TestStruct test_instance = {0, 0};
  const int* const a_ptr = &test_instance.a;
  const int* const b_ptr = &test_instance.b;

  POINTERS_EQUAL(&test_instance, structof(a_ptr, TestStruct, a));
  POINTERS_EQUAL(&test_instance, structof(b_ptr, TestStruct, b));
}
