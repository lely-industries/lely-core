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

#include <cfloat>
#include <climits>

#include <CppUTest/TestHarness.h>

#include <lely/util/util.h>

TEST_GROUP(Util_Util){};

/// @name ABS()
///@{

/// \Given N/A
///
/// \When ABS() is used with an integer
///
/// \Then macro evaluates to the absolute value of the integer
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

/// \Given N/A
///
/// \When ABS() is used with a long integer
///
/// \Then macro evaluates to the absolute value of the integer
TEST(Util_Util, Abs_LongInts) {
  CHECK_EQUAL(0L, ABS(0L));
  CHECK_EQUAL(0L, ABS(-0L));
  CHECK_EQUAL(1L, ABS(1L));
  CHECK_EQUAL(1L, ABS(-1L));
  CHECK_EQUAL(LONG_MAX, ABS(LONG_MAX));
  CHECK_EQUAL(LONG_MAX, ABS(-LONG_MAX));
}

/// \Given N/A
///
/// \When ABS() is used with a long long integer
///
/// \Then macro evaluates to the absolute value of the integer
TEST(Util_Util, Abs_LongLongInts) {
  CHECK_EQUAL(0LL, ABS(0LL));
  CHECK_EQUAL(0LL, ABS(-0LL));
  CHECK_EQUAL(1LL, ABS(1LL));
  CHECK_EQUAL(1LL, ABS(-1LL));
  CHECK_EQUAL(LONG_LONG_MAX, ABS(LONG_LONG_MAX));
  CHECK_EQUAL(LONG_LONG_MAX, ABS(-LONG_LONG_MAX));
}

/// \Given N/A
///
/// \When ABS() is used with a double-precision floating-point number
///
/// \Then the macro evaluates to the absolute value of the number
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

/// \Given N/A
///
/// \When ABS() is used with a single-precision floating-point number
///
/// \Then the macro evaluates to the absolute value of the number
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

///@}

/// @name ALIGN()
///@{

/// \Given N/A
///
/// \When ALIGN() is used with a small integer and a base 2
///
/// \Then the macro evaluates to the nearest multiple of base greater or equal
///       to the number
TEST(Util_Util, Align_Base2SmallInts) {
  int base = 2;

  CHECK_EQUAL(0, ALIGN(0, base));
  CHECK_EQUAL(2, ALIGN(1, base));
  CHECK_EQUAL(2, ALIGN(2, base));
  CHECK_EQUAL(4, ALIGN(3, base));
  CHECK_EQUAL(4, ALIGN(4, base));
  CHECK_EQUAL(6, ALIGN(5, base));
  CHECK_EQUAL(6, ALIGN(6, base));
  CHECK_EQUAL(31536, ALIGN(31536, base));
}

/// \Given N/A
///
/// \When ALIGN() is used with a big positive number and a base 2
///
/// \Then the macro evaluates to the nearest multiple of base greater or equal
///       to the number
TEST(Util_Util, Align_Base2BigInts) {
  const int base = 2;

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

/// \Given N/A
///
/// \When ALIGN() is used with a small integer and a base 0
///
/// \Then the number is 0
TEST(Util_Util, Align_Base0SmallInts) {
  const int base = 0;

  CHECK_EQUAL(0, ALIGN(0, base));
  CHECK_EQUAL(0, ALIGN(1, base));
  CHECK_EQUAL(0, ALIGN(31536, base));
}

/// \Given N/A
///
/// \When ALIGN() is used with a big positive number and a base 0
///
/// \Then the number is 0
TEST(Util_Util, Align_Base0BigInts) {
  const int base = 0;

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

/// \Given N/A
///
/// \When ALIGN() is used with a small integer and a base 4
///
/// \Then the macro evaluates to the nearest multiple of base greater or equal
///       to the number
TEST(Util_Util, Align_Base4SmallInts) {
  const int base = 4;

  CHECK_EQUAL(0, ALIGN(0, base));
  CHECK_EQUAL(4, ALIGN(1, base));
  CHECK_EQUAL(4, ALIGN(2, base));
  CHECK_EQUAL(4, ALIGN(3, base));
  CHECK_EQUAL(4, ALIGN(4, base));
  CHECK_EQUAL(8, ALIGN(5, base));
  CHECK_EQUAL(8, ALIGN(6, base));
  CHECK_EQUAL(31536, ALIGN(31536, base));
}

/// \Given N/A
///
/// \When ALIGN() is used with a big positive number and a base 4
///
/// \Then the macro evaluates to the nearest multiple of base greater or equal
///       to the number
TEST(Util_Util, Align_Base4BigInts) {
  const int base = 4;

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

/// \Given N/A
///
/// \When ALIGN() is used with a small integer and base 4096
///
/// \Then the macro evaluates to the nearest multiple of base greater or equal
///       to the number
TEST(Util_Util, Align_BigBaseSmallInts) {
  const int base = 4096;

  CHECK_EQUAL(0, ALIGN(0, base));
  CHECK_EQUAL(4096, ALIGN(1, base));
  CHECK_EQUAL(4096, ALIGN(2, base));
  CHECK_EQUAL(32768, ALIGN(31536, base));
}

/// \Given N/A
///
/// \When ALIGN() is used with a big positive number and a base 4096
///
/// \Then the macro evaluates to the nearest multiple of base greater or equal
///       to the number
TEST(Util_Util, Align_BigBaseBigInts) {
  const int base = 4096;

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

///@}

/// @name MIN()
///@{

/// \Given N/A
///
/// \When MIN() is used with two numbers
///
/// \Then macro evaluates to the lower of the numbers
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

///@}

/// @name MAX()
///@{

/// \Given N/A
///
/// \When MAX() is used with two numbers
///
/// \Then macro evaluates to the greater of the numbers
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

/// \Given N/A
///
/// \When MAX() is used with two equal numbers, MIN() is used with same
///       numbers
///
/// \Then both macros evaluate to different variables
TEST(Util_Util, MinMaxDifferentAddresses) {
  const int32_t a = 42;
  const int32_t b = 42;

  const int32_t* const min = &(MIN(a, b));
  const int32_t* const max = &(MAX(a, b));

  CHECK(min != max);
}

///@}

/// @name countof()
///@{

/// \Given N/A
///
/// \When countof() is used with an array
///
/// \Then macro evaluates to the number of elements in the array
TEST(Util_Util, Countof) {
  const int32_t a[1] = {0};
  CHECK_EQUAL(1, countof(a));
  const int32_t b[2] = {0};
  CHECK_EQUAL(2, countof(b));
  const int32_t c[42] = {0};
  CHECK_EQUAL(42, countof(c));
}

///@}

/// @name powerof2
///@{

/// \Given N/A
///
/// \When powerof2() is used with an integer
///
/// \Then if the integer is a power of 2: macro evaluates to 1, else to 0
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
  CHECK_EQUAL(0, powerof2(-INT_MAX));
  CHECK_EQUAL(0, powerof2(-SHRT_MAX - 1));
  CHECK_EQUAL(1, powerof2(SHRT_MAX + 1));
  CHECK_EQUAL(0, powerof2(INT_MAX));
  CHECK_EQUAL(1, powerof2(static_cast<int64_t>(INT_MAX) + 1L));
}

///@}

TEST(Util_Util, Structof_Example) {
  struct TestNode {
    int32_t x;
  };

  struct TestObject {
    TestNode node;
    int32_t val;
  };

  const TestNode test_node_init = {0};
  const TestObject test_object = {test_node_init, 0};
  const TestNode* const test_node_ptr = &test_object.node;

  POINTERS_EQUAL(&test_object, structof(test_node_ptr, TestObject, node));
}

/// @name structof()
///@{

/// \Given N/A
///
/// \When structof() is used with a pointer to member, struct name and member
///       name
///
/// \Then the macro evaluates to the address of the member's parent structure
TEST(Util_Util, Structof_General) {
  struct TestStruct {
    int32_t a;
    int32_t b;
  };

  const TestStruct test_instance = {0, 0};
  const int32_t* const a_ptr = &test_instance.a;
  const int32_t* const b_ptr = &test_instance.b;

  POINTERS_EQUAL(&test_instance, structof(a_ptr, TestStruct, a));
  POINTERS_EQUAL(&test_instance, structof(b_ptr, TestStruct, b));
}

///@}
