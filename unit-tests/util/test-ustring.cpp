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

#include <lely/util/ustring.h>
#include <lely/util/util.h>

TEST_GROUP(UTIL_UString) {
  const char16_t* const TEST_STR = u"abcdefg";
  static constexpr size_t TEST_LEN = 7u;
};

TEST(UTIL_UString, Str16Len_Empty) { CHECK_EQUAL(0, str16len(u"")); }

TEST(UTIL_UString, Str16Len_OneLength) { CHECK_EQUAL(1, str16len(u"a")); }

TEST(UTIL_UString, Str16Len_FullString) {
  CHECK_EQUAL(TEST_LEN, str16len(TEST_STR));
}

TEST(UTIL_UString, Str16NCpy_Zero) {
  POINTERS_EQUAL(nullptr, str16ncpy(nullptr, nullptr, 0));
}

TEST(UTIL_UString, Str16NCpy_EmptySrcZeroBuffer) {
  const auto* const ret = str16ncpy(nullptr, u"", 0);

  POINTERS_EQUAL(nullptr, ret);
}

TEST(UTIL_UString, Str16NCpy_EmptySrcExactBuffer) {
  char16_t buf = u'A';

  const auto* const ret = str16ncpy(&buf, u"", 1u);

  POINTERS_EQUAL(ret, &buf);
  CHECK_EQUAL(u'\0', buf);
}

TEST(UTIL_UString, Str16NCpy_EmptySrcBigBuffer) {
  const size_t BUF_LEN = 5u;
  char16_t buf[BUF_LEN];
  memset(buf, u'A', BUF_LEN * sizeof(char16_t));

  const auto* const ret = str16ncpy(buf, u"", BUF_LEN);

  POINTERS_EQUAL(&buf, ret);
  for (size_t i = 0; i < BUF_LEN; ++i) CHECK_EQUAL(u'\0', buf[i]);
}

TEST(UTIL_UString, Str16NCpy_TooBigBuffer) {
  const size_t BUF_LEN = 10u;
  char16_t buf[BUF_LEN];
  memset(buf, u'A', BUF_LEN * sizeof(char16_t));

  const auto* const ret = str16ncpy(buf, TEST_STR, BUF_LEN);

  POINTERS_EQUAL(buf, ret);
  for (size_t i = 0; i < BUF_LEN; ++i) {
    if (i < TEST_LEN)
      CHECK_EQUAL(TEST_STR[i], buf[i]);
    else
      CHECK_EQUAL(u'\0', buf[i]);
  }
}

TEST(UTIL_UString, Str16NCpy_TooSmallBuffer) {
  const size_t BUF_LEN = 5u;
  char16_t buf[BUF_LEN];
  memset(buf, u'A', BUF_LEN * sizeof(char16_t));

  const auto* const ret = str16ncpy(buf, TEST_STR, BUF_LEN);

  POINTERS_EQUAL(buf, ret);
  for (size_t i = 0; i < BUF_LEN; ++i) CHECK_EQUAL(TEST_STR[i], buf[i]);
}

TEST(UTIL_UString, Str16NCpy_ExactBuffer) {
  char16_t buf[TEST_LEN];
  memset(buf, u'A', TEST_LEN * sizeof(char16_t));

  const auto* const ret = str16ncpy(buf, TEST_STR, TEST_LEN);

  POINTERS_EQUAL(buf, ret);
  for (size_t i = 0; i < TEST_LEN; ++i) CHECK_EQUAL(TEST_STR[i], buf[i]);
}

TEST(UTIL_UString, Str16NCmp_Zero) {
  CHECK_EQUAL(0, str16ncmp(nullptr, nullptr, 0));
}

TEST(UTIL_UString, Str16NCmp_EmptyStr1) {
  CHECK_COMPARE(0, >, str16ncmp(u"", TEST_STR, 1u));
}

TEST(UTIL_UString, Str16NCmp_EmptyStr2) {
  CHECK_COMPARE(0, <, str16ncmp(TEST_STR, u"", 1u));
}

TEST(UTIL_UString, Str16NCmp_Equal) {
  CHECK_EQUAL(0, str16ncmp(TEST_STR, TEST_STR, TEST_LEN));
}

TEST(UTIL_UString, Str16NCmp_EqualSmallN) {
  CHECK_EQUAL(0, str16ncmp(TEST_STR, TEST_STR, 3u));
}

TEST(UTIL_UString, Str16NCmpEqualEmpty) {
  CHECK_EQUAL(0, str16ncmp(u"", u"", 1u));
}

TEST(UTIL_UString, Str16NCmp_NotEqual1) {
  const char16_t test_str[] = u"xyz";

  CHECK_COMPARE(0, >, str16ncmp(TEST_STR, test_str, TEST_LEN));
}

TEST(UTIL_UString, Str16NCmp_NotEqual2) {
  const char16_t test_str[] = u"xyz";

  CHECK_COMPARE(0, <, str16ncmp(test_str, TEST_STR, TEST_LEN));
}

TEST(UTIL_UString, Str16NCmp_NotEqualPart1) {
  char16_t test_str[] = u"xyz";
  str16ncpy(test_str, TEST_STR, 2u);

  CHECK_COMPARE(0, <, str16ncmp(test_str, TEST_STR, TEST_LEN));
}

TEST(UTIL_UString, Str16NCmp_NotEqualPart2) {
  char16_t test_str[] = u"xyz";
  str16ncpy(test_str, TEST_STR, 2u);

  CHECK_COMPARE(0, >, str16ncmp(TEST_STR, test_str, TEST_LEN));
}

TEST(UTIL_UString, Str16NCmp_NotEqualPart1_ExactN) {
  char16_t test_str[] = u"xyz";
  str16ncpy(test_str, TEST_STR, 2u);

  CHECK_COMPARE(0, <, str16ncmp(test_str, TEST_STR, 3u));
}

TEST(UTIL_UString, Str16NCmp_NotEqualPart2_ExactN) {
  char16_t test_str[] = u"xyz";
  str16ncpy(test_str, TEST_STR, 2u);

  CHECK_COMPARE(0, >, str16ncmp(TEST_STR, test_str, 3u));
}

TEST(UTIL_UString, Str16NCmp_ShorterStr1) {
  char16_t test_str[TEST_LEN - 2] = {u'\0'};
  str16ncpy(test_str, TEST_STR, TEST_LEN - 3);

  CHECK_COMPARE(0, >, str16ncmp(test_str, TEST_STR, TEST_LEN));
}

TEST(UTIL_UString, Str16NCmp_ShorterStr2) {
  char16_t test_str[TEST_LEN - 2] = {u'\0'};
  str16ncpy(test_str, TEST_STR, TEST_LEN - 3);

  CHECK_COMPARE(0, <, str16ncmp(TEST_STR, test_str, TEST_LEN));
}
