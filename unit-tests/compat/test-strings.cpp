/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2021 N7 Space Sp. z o.o.
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

#include <lely/compat/strings.h>

TEST_GROUP(Compat_Strings){};

/// @name ffs
///@{

/// \Given set of integers with various first (least) significant bit
///
/// \When calling ffs() for those integers
///
/// \Then first significant bit number is returned for each integer
TEST(Compat_Strings, Fss) {
  CHECK_EQUAL(0, ffs(0x00000000));

  CHECK_EQUAL(0x01, ffs(0x00000001));
  CHECK_EQUAL(0x02, ffs(0x00000002));
  CHECK_EQUAL(0x03, ffs(0x00000004));
  CHECK_EQUAL(0x04, ffs(0x00000008));

  CHECK_EQUAL(0x05, ffs(0x00000010));
  CHECK_EQUAL(0x06, ffs(0x00000020));
  CHECK_EQUAL(0x07, ffs(0x00000040));
  CHECK_EQUAL(0x08, ffs(0x00000080));

  CHECK_EQUAL(0x09, ffs(0x00000100));
  CHECK_EQUAL(0x0A, ffs(0x00000200));
  CHECK_EQUAL(0x0B, ffs(0x00000400));
  CHECK_EQUAL(0x0C, ffs(0x00000800));

  CHECK_EQUAL(0x0D, ffs(0x00001000));
  CHECK_EQUAL(0x0E, ffs(0x00002000));
  CHECK_EQUAL(0x0F, ffs(0x00004000));
  CHECK_EQUAL(0x10, ffs(0x00008000));

  CHECK_EQUAL(0x11, ffs(0x00010000));
  CHECK_EQUAL(0x12, ffs(0x00020000));
  CHECK_EQUAL(0x13, ffs(0x00040000));
  CHECK_EQUAL(0x14, ffs(0x00080000));

  CHECK_EQUAL(0x15, ffs(0x00100000));
  CHECK_EQUAL(0x16, ffs(0x00200000));
  CHECK_EQUAL(0x17, ffs(0x00400000));
  CHECK_EQUAL(0x18, ffs(0x00800000));

  CHECK_EQUAL(0x19, ffs(0x01000000));
  CHECK_EQUAL(0x1A, ffs(0x02000000));
  CHECK_EQUAL(0x1B, ffs(0x04000000));
  CHECK_EQUAL(0x1C, ffs(0x08000000));

  CHECK_EQUAL(0x1D, ffs(0x10000000));
  CHECK_EQUAL(0x1E, ffs(0x20000000));
  CHECK_EQUAL(0x1F, ffs(0x40000000));
  CHECK_EQUAL(0x20, ffs(0x80000000));

  CHECK_EQUAL(0x01, ffs(0xFFFFFFFF));
  CHECK_EQUAL(0x02, ffs(0xFFFFFFFE));
  CHECK_EQUAL(0x03, ffs(0xFFFFFFFC));
  CHECK_EQUAL(0x04, ffs(0xFFFFFFF8));

  CHECK_EQUAL(0x05, ffs(0xFFFFFFF0));
  CHECK_EQUAL(0x06, ffs(0xFFFFFFE0));
  CHECK_EQUAL(0x07, ffs(0xFFFFFFC0));
  CHECK_EQUAL(0x08, ffs(0xFFFFFF80));

  CHECK_EQUAL(0x09, ffs(0xFFFFFF00));
  CHECK_EQUAL(0x0A, ffs(0xFFFFFE00));
  CHECK_EQUAL(0x0B, ffs(0xFFFFFC00));
  CHECK_EQUAL(0x0C, ffs(0xFFFFF800));

  CHECK_EQUAL(0x0D, ffs(0xFFFFF000));
  CHECK_EQUAL(0x0E, ffs(0xFFFFE000));
  CHECK_EQUAL(0x0F, ffs(0xFFFFC000));
  CHECK_EQUAL(0x10, ffs(0xFFFF8000));

  CHECK_EQUAL(0x11, ffs(0xFFFF0000));
  CHECK_EQUAL(0x12, ffs(0xFFFE0000));
  CHECK_EQUAL(0x13, ffs(0xFFFC0000));
  CHECK_EQUAL(0x14, ffs(0xFFF80000));

  CHECK_EQUAL(0x15, ffs(0xFFF00000));
  CHECK_EQUAL(0x16, ffs(0xFFE00000));
  CHECK_EQUAL(0x17, ffs(0xFFC00000));
  CHECK_EQUAL(0x18, ffs(0xFF800000));

  CHECK_EQUAL(0x19, ffs(0xFF000000));
  CHECK_EQUAL(0x1A, ffs(0xFE000000));
  CHECK_EQUAL(0x1B, ffs(0xFC000000));
  CHECK_EQUAL(0x1C, ffs(0xF8000000));

  CHECK_EQUAL(0x1D, ffs(0xF0000000));
  CHECK_EQUAL(0x1E, ffs(0xE0000000));
  CHECK_EQUAL(0x1F, ffs(0xC0000000));
  CHECK_EQUAL(0x20, ffs(0x80000000));
}

///@}

#if LELY_NO_HOSTED

/// @name lely_compat_strcasecmp
///@{

/// \Given a string
///
/// \When calling lely_compat_strcasecmp() to compare it with itself
///
/// \Then 0 is returned
TEST(Compat_Strings, LelyCompatStrcasecmp_SameObject) {
  const char str[] = "aAbB";

  const int result = lely_compat_strcasecmp(str, str);

  CHECK_EQUAL(0, result);
}

/// \Given two strings
///
/// \When calling lely_compat_strcasecmp() with 'greater (case insensitive)'
///       as the first argument
///
/// \Then value less than 0 is returned
TEST(Compat_Strings, LelyCompatStrcasecmp_LeftGreater) {
  const char str1[] = "abcdeX";
  const char str2[] = "ABCDEz";

  const int result = lely_compat_strcasecmp(str1, str2);

  CHECK_COMPARE(result, <, 0);
}

/// \Given two strings
///
/// \When calling lely_compat_strcasecmp() with 'greater (case insensitive)'
///       as the second argument
///
/// \Then value greater than 0 is returned
TEST(Compat_Strings, LelyCompatStrcasecmp_RightGreater) {
  const char str1[] = "abcdeZ";
  const char str2[] = "ABCDEx";

  const int result = lely_compat_strcasecmp(str1, str2);

  CHECK_COMPARE(result, >, 0);
}

/// \Given two equal (case insensitively) strings
///
/// \When calling lely_compat_strcasecmp() to compare those strings
///
/// \Then 0 is returned
TEST(Compat_Strings, LelyCompatStrcasecmp_Equal) {
  const char str1[] = "abcdeX";
  const char str2[] = "ABCDEx";

  const int result = lely_compat_strcasecmp(str1, str2);

  CHECK_EQUAL(0, result);
}

/// \Given two strings
///
/// \When calling lely_compat_strcasecmp() with shorter
///       as the first argument
///
/// \Then value less than 0 is returned
TEST(Compat_Strings, LelyCompatStrcasecmp_LeftShorter) {
  const char str1[] = "abcd";
  const char str2[] = "ABCDE";

  const int result = lely_compat_strcasecmp(str1, str2);

  CHECK_COMPARE(result, <, 0);
}

/// \Given two strings
///
/// \When calling lely_compat_strcasecmp() with shorter
///       as the second argument
///
/// \Then value greater than 0 is returned
TEST(Compat_Strings, LelyCompatStrcasecmp_RightShorter) {
  const char str1[] = "abcde";
  const char str2[] = "ABCD";

  const int result = lely_compat_strcasecmp(str1, str2);

  CHECK_COMPARE(result, >, 0);
}

///@}

/// @name lely_compat_strncasecmp
///@{

/// \Given a string
///
/// \When calling lely_compat_strncasecmp() to compare it with itself
///
/// \Then 0 is returned
TEST(Compat_Strings, LelyCompatStrncasecmp_SameObject) {
  const char str[] = "aAbB";

  const int result = lely_compat_strncasecmp(str, str, sizeof(str));

  CHECK_EQUAL(0, result);
}

/// \Given two strings
///
/// \When calling lely_compat_strncasecmp() to compare 0 bytes of those strings
///
/// \Then 0 is returned
TEST(Compat_Strings, LelyCompatStrncasecmp_Zero) {
  const char str1[] = "abcdeX";
  const char str2[] = "ABCDEz";

  const int result = lely_compat_strncasecmp(str1, str2, 0);

  CHECK_EQUAL(0, result);
}

/// \Given two strings
///
/// \When calling lely_compat_strncasecmp() with 'greater (case insensitive)'
///       as the first argument
///
/// \Then value less than 0 is returned
TEST(Compat_Strings, LelyCompatStrncasecmp_LeftGreater) {
  const char str1[] = "abcdeX";
  const char str2[] = "ABCDEz";

  const int result = lely_compat_strncasecmp(str1, str2, sizeof(str1));

  CHECK_COMPARE(result, <, 0);
}

/// \Given two strings
///
/// \When calling lely_compat_strncasecmp() with 'greater (case insensitive)'
///       as the second argument
///
/// \Then value greater than 0 is returned
TEST(Compat_Strings, LelyCompatStrncasecmp_RightGreater) {
  const char str1[] = "abcdeZ";
  const char str2[] = "ABCDEx";

  const int result = lely_compat_strncasecmp(str1, str2, sizeof(str1));

  CHECK_COMPARE(result, >, 0);
}

/// \Given two equal (case insensitively) strings on first n bytes
///
/// \When calling lely_compat_strncasecmp() to compare those strings
///
/// \Then 0 is returned
TEST(Compat_Strings, LelyCompatStrncasecmp_Equal) {
  const char str1[] = "abczzzzzzz";
  const char str2[] = "ABCxxxxxxxx";

  const int result = lely_compat_strncasecmp(str1, str2, 3);

  CHECK_EQUAL(0, result);
}

/// \Given two strings
///
/// \When calling lely_compat_strncasecmp() with shorter
///       as the first argument
///
/// \Then value less than 0 is returned
TEST(Compat_Strings, LelyCompatStrncasecmp_LeftShorter) {
  const char str1[] = "abcd";
  const char str2[] = "ABCDE";

  const int result = lely_compat_strncasecmp(str1, str2, sizeof(str2));

  CHECK_COMPARE(result, <, 0);
}

/// \Given two strings
///
/// \When calling lely_compat_strncasecmp() with shorter
///       as the second argument
///
/// \Then value greater than 0 is returned
TEST(Compat_Strings, LelyCompatStrncasecmp_RightShorter) {
  const char str1[] = "abcde";
  const char str2[] = "ABCD";

  const int result = lely_compat_strncasecmp(str1, str2, sizeof(str1));

  CHECK_COMPARE(result, >, 0);
}

///@}

#endif
