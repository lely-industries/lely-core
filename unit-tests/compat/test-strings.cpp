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
