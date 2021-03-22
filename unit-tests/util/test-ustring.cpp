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

#include <lely/util/ustring.h>
#include <lely/util/util.h>

TEST_GROUP(UTIL_UString) {
  const char16_t* const TEST_STR = u"abcdefg";
  static constexpr size_t TEST_LEN = 7u;
};

/// @name str16len()
///@{

/// \Given N/A
///
/// \When str16len() is called with a pointer to an empty 16-bit Unicode string
///
/// \Then 0 is returned
TEST(UTIL_UString, Str16Len_Empty) { CHECK_EQUAL(0, str16len(u"")); }

/// \Given N/A
///
/// \When str16len() is called with a pointer to a 16-bit Unicode string with
///       one character
///
/// \Then 1 is returned
TEST(UTIL_UString, Str16Len_OneLength) { CHECK_EQUAL(1, str16len(u"a")); }

/// \Given N/A
///
/// \When str16len() is called with a pointer to a 16-bit Unicode string with
///       multiple characters
///
/// \Then the number of characters in the string is returned
TEST(UTIL_UString, Str16Len_FullString) {
  CHECK_EQUAL(TEST_LEN, str16len(TEST_STR));
}

///@}

/// @name str16ncpy()
///@{

/// \Given N/A
///
/// \When str16ncpy() is called with two null pointers and zero characters to
///       copy
///
/// \Then a null pointer is returned
TEST(UTIL_UString, Str16NCpy_Zero) {
  POINTERS_EQUAL(nullptr, str16ncpy(nullptr, nullptr, 0));
}

/// \Given N/A
///
/// \When str16ncpy() is called with a null pointer as destination, a pointer to
///       an empty 16-bit Unicode string as source and zero characters to copy
///
/// \Then a null pointer is returned
TEST(UTIL_UString, Str16NCpy_EmptySrcZeroBuffer) {
  const auto* const ret = str16ncpy(nullptr, u"", 0);

  POINTERS_EQUAL(nullptr, ret);
}

/// \Given a single 16-bit character destination buffer
///
/// \When str16ncpy() is called with a pointer to the destination buffer, a
///       pointer to an empty 16-bit Unicode string as source and 1 character to
///       copy
///
/// \Then a pointer to the buffer is returned, the buffer contains the null
///       character
TEST(UTIL_UString, Str16NCpy_EmptySrcExactBuffer) {
  char16_t buf = u'A';

  const auto* const ret = str16ncpy(&buf, u"", 1u);

  POINTERS_EQUAL(ret, &buf);
  CHECK_EQUAL(u'\0', buf);
}

/// \Given a destination buffer for 16-bit characters
///
/// \When str16ncpy() is called with a pointer to the destination buffer, a
///       pointer to an empty 16-bit Unicode string as source and a number of
///       characters to copy equal to the length of the buffer
///
/// \Then a pointer to the buffer is returned, the buffer is zeroed
TEST(UTIL_UString, Str16NCpy_EmptySrcBigBuffer) {
  const size_t BUF_LEN = 5u;
  char16_t buf[BUF_LEN];
  memset(buf, u'A', BUF_LEN * sizeof(char16_t));

  const auto* const ret = str16ncpy(buf, u"", BUF_LEN);

  POINTERS_EQUAL(&buf, ret);
  for (size_t i = 0; i < BUF_LEN; ++i) CHECK_EQUAL(u'\0', buf[i]);
}

/// \Given a destination buffer for 16-bit characters
///
/// \When str16ncpy() is called with a pointer to the destination buffer, a
///       pointer to a non-empty 16-bit Unicode string shorter than the buffer
///       as source and a number of characters to copy equal to the length of
///       the buffer
///
/// \Then a pointer to the buffer is returned, the buffer contains the string as
///       prefix and remaining characters are zeroed
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

/// \Given a destination buffer for 16-bit characters
///
/// \When str16ncpy() is called with a pointer to the destination buffer, a
///       pointer to a non-empty 16-bit Unicode string longer than the buffer as
///       source and a number of characters to copy equal to the length of the
///       buffer
///
/// \Then a pointer to the buffer is returned, the buffer is filled until its
///       end with prefix characters of the string
TEST(UTIL_UString, Str16NCpy_TooSmallBuffer) {
  const size_t BUF_LEN = 5u;
  char16_t buf[BUF_LEN];
  memset(buf, u'A', BUF_LEN * sizeof(char16_t));

  const auto* const ret = str16ncpy(buf, TEST_STR, BUF_LEN);

  POINTERS_EQUAL(buf, ret);
  for (size_t i = 0; i < BUF_LEN; ++i) CHECK_EQUAL(TEST_STR[i], buf[i]);
}

/// \Given a destination buffer for 16-bit characters
///
/// \When str16ncpy() is called with a pointer to the destination buffer, a
///       pointer to a non-empty 16-bit Unicode string of equal length to the
///       buffer as source and a number of characters to copy equal to the
///       length of the string
///
/// \Then a pointer to the buffer is returned, the buffer contains the same
///       characters as the string but is not null-terminated
TEST(UTIL_UString, Str16NCpy_ExactBuffer) {
  char16_t buf[TEST_LEN];
  memset(buf, u'A', TEST_LEN * sizeof(char16_t));

  const auto* const ret = str16ncpy(buf, TEST_STR, TEST_LEN);

  POINTERS_EQUAL(buf, ret);
  for (size_t i = 0; i < TEST_LEN; ++i) CHECK_EQUAL(TEST_STR[i], buf[i]);
}

///@}

/// @name str16ncmp()
///@{

/// \Given N/A
///
/// \When str16ncmp() is called with two null pointers and zero characters to
///       compare
///
/// \Then 0 is returned
TEST(UTIL_UString, Str16NCmp_Zero) {
  CHECK_EQUAL(0, str16ncmp(nullptr, nullptr, 0));
}

/// \Given N/A
///
/// \When str16ncmp() is called with two pointers to different 16-bit Unicode
///       strings and one character to compare, the first string is empty, the
///       second is not
///
/// \Then a value less than 0 is returned
TEST(UTIL_UString, Str16NCmp_EmptyStrFirst) {
  CHECK_COMPARE(0, >, str16ncmp(u"", TEST_STR, 1u));
}

/// \Given N/A
///
/// \When str16ncmp() is called with two pointers to different 16-bit Unicode
///       strings and one character to compare, the second string is empty, the
///       first is not
///
/// \Then a value greater than 0 is returned
TEST(UTIL_UString, Str16NCmp_EmptyStrSecond) {
  CHECK_COMPARE(0, <, str16ncmp(TEST_STR, u"", 1u));
}

/// \Given N/A
///
/// \When str16ncmp() is called with two pointers to the same 16-bit Unicode
///       string and full string length to compare
///
/// \Then 0 is returned
TEST(UTIL_UString, Str16NCmp_Equal) {
  CHECK_EQUAL(0, str16ncmp(TEST_STR, TEST_STR, TEST_LEN));
}

/// \Given N/A
///
/// \When str16ncmp() is called with two pointers to the same 16-bit Unicode
///       string and a number of characters to compare less than the length of
///       the string
///
/// \Then 0 is returned
TEST(UTIL_UString, Str16NCmp_EqualSmallN) {
  CHECK_EQUAL(0, str16ncmp(TEST_STR, TEST_STR, 3u));
}

/// \Given N/A
///
/// \When str16ncmp() is called with two pointers to empty 16-bit Unicode
///       strings and one character to compare
///
/// \Then 0 is returned
TEST(UTIL_UString, Str16NCmp_EqualEmpty) {
  CHECK_EQUAL(0, str16ncmp(u"", u"", 1u));
}

/// \Given N/A
///
/// \When str16ncmp() is called with two pointers to different non-empty 16-bit
///       Unicode strings of different lengths and the length of the longer one,
///       the longer string is passed as first argument, the shorter string
///       starts with a character that's greater than the first character of the
///       longer string
///
/// \Then a value less than 0 is returned
TEST(UTIL_UString, Str16NCmp_LessThan) {
  const char16_t test_str[] = u"xyz";

  CHECK_COMPARE(0, >, str16ncmp(TEST_STR, test_str, TEST_LEN));
}

/// \Given N/A
///
/// \When str16ncmp() is called with two pointers to different non-empty 16-bit
///       Unicode strings of different lengths and the length of the longer one,
///       the shorter string is passed as first argument, the shorter string
///       starts with a character that's greater than the first character of the
///       longer string
///
/// \Then a value greater than 0 is returned
TEST(UTIL_UString, Str16NCmp_GreaterThan) {
  const char16_t test_str[] = u"xyz";

  CHECK_COMPARE(0, <, str16ncmp(test_str, TEST_STR, TEST_LEN));
}

/// \Given N/A
///
/// \When str16ncmp() is called with two pointers to different non-empty 16-bit
///       Unicode strings of different lengths and the length of the longer one,
///       the shorter string is passed as first argument, both strings have a
///       common prefix until the shorter string's last character that's greater
///       than the corresponding character of the longer string
///
/// \Then a value greater than 0 is returned
TEST(UTIL_UString, Str16NCmp_CommonPrefixGreaterThan) {
  char16_t test_str[] = u"xyz";
  str16ncpy(test_str, TEST_STR, 2u);

  CHECK_COMPARE(0, <, str16ncmp(test_str, TEST_STR, TEST_LEN));
}

/// \Given N/A
///
/// \When str16ncmp() is called with two pointers to different non-empty 16-bit
///       Unicode strings of different lengths and the length of the longer one,
///       the longer string is passed as first argument, both strings have a
///       common prefix until the shorter string's last character that's greater
///       than the corresponding character of the longer string
///
/// \Then a value less than 0 is returned
TEST(UTIL_UString, Str16NCmp_CommonPrefixLessThan) {
  char16_t test_str[] = u"xyz";
  str16ncpy(test_str, TEST_STR, 2u);

  CHECK_COMPARE(0, >, str16ncmp(TEST_STR, test_str, TEST_LEN));
}

/// \Given N/A
///
/// \When str16ncmp() is called with two pointers to different non-empty 16-bit
///       Unicode strings of different lengths and the length of the shorter
///       one, the shorter string is passed as first argument, both strings have
///       a common prefix until the shorter string's last character that's
///       greater than the corresponding character of the longer string
///
/// \Then a value greater than 0 is returned
TEST(UTIL_UString, Str16NCmp_CommonPrefixGreaterThan_ExactN) {
  char16_t test_str[] = u"xyz";
  str16ncpy(test_str, TEST_STR, 2u);

  CHECK_COMPARE(0, <, str16ncmp(test_str, TEST_STR, 3u));
}

/// \Given N/A
///
/// \When str16ncmp() is called with two pointers to different non-empty 16-bit
///       Unicode strings of different lengths and the length of the shorter
///       one, the longer string is passed as first argument, both strings have
///       a common prefix until the shorter string's last character that's
///       greater than the corresponding character of the longer string
///
/// \Then a value less than 0 is returned
TEST(UTIL_UString, Str16NCmp_CommonPrefixLessThan_ExactN) {
  char16_t test_str[] = u"xyz";
  str16ncpy(test_str, TEST_STR, 2u);

  CHECK_COMPARE(0, >, str16ncmp(TEST_STR, test_str, 3u));
}

/// \Given N/A
///
/// \When str16ncmp() is called with two pointers to different non-empty 16-bit
///       Unicode strings of different lengths and the length of the longer one,
///       the shorter string is passed as first argument, the shorter string is
///       a prefix of the longer string
///
/// \Then a value less than 0 is returned
TEST(UTIL_UString, Str16NCmp_ShorterStrFirst) {
  char16_t test_str[TEST_LEN - 2] = {u'\0'};
  str16ncpy(test_str, TEST_STR, TEST_LEN - 3);

  CHECK_COMPARE(0, >, str16ncmp(test_str, TEST_STR, TEST_LEN));
}

/// \Given N/A
///
/// \When str16ncmp() is called with two pointers to different non-empty 16-bit
///       Unicode strings of different lengths and the length of the longer one,
///       the longer string is passed as first argument, the shorter string is a
///       prefix of the longer string
///
/// \Then a value greater than 0 is returned
TEST(UTIL_UString, Str16NCmp_ShorterStrSecond) {
  char16_t test_str[TEST_LEN - 2] = {u'\0'};
  str16ncpy(test_str, TEST_STR, TEST_LEN - 3);

  CHECK_COMPARE(0, <, str16ncmp(TEST_STR, test_str, TEST_LEN));
}

///@}
