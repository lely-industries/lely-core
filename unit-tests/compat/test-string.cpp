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

#include <algorithm>
#include <numeric>
#include <array>

#include <CppUTest/TestHarness.h>

#include <lely/compat/string.h>

TEST_GROUP(Compat_String) {
  static const size_t totalSize = 10;

  std::array<uint8_t, totalSize> memory;
  std::array<uint8_t, totalSize> expected;

  TEST_SETUP() {
    memory.fill(0u);
    expected.fill(0u);
  }

  void CheckExpected() {
    MEMCMP_EQUAL(expected.data(), memory.data(), totalSize);  // nice message
    CHECK(expected == memory);                                // sanity check
  }
};

#if LELY_NO_HOSTED

/// @name lely_compat_memcpy
///@{

/// \Given memory area
///
/// \When calling lely_compat_memcpy() with the area, source data pointer
//        and data count
///
/// \Then count bytes from memory area is overwritten with new data;
///       memory area address is returned
TEST(Compat_String, LelyCompatMemcpy_NonZeroCount) {
  const char data[] = "abcde";

  const void* result = lely_compat_memcpy(memory.data(), data, sizeof(data));

  POINTERS_EQUAL(memory.data(), result);
  std::copy(std::begin(data), std::end(data), std::begin(expected));
  CheckExpected();
}

/// \Given memory area
///
/// \When calling lely_compat_memcpy() for the area with zero count
///
/// \Then memory area is not modified; memory area address is returned
TEST(Compat_String, LelyCompatMemcpy_ZeroCount) {
  const char data[] = "abcde";

  const void* result = lely_compat_memcpy(memory.data(), data, 0u);

  POINTERS_EQUAL(memory.data(), result);
  CheckExpected();
}

///@}

/// @name lely_compat_memmove
///@{

/// \Given memory area
///
/// \When calling lely_compat_memmove() with the area, source data pointer
///       and data count
///
/// \Then count bytes from memory area is overwritten with new data;
///       memory area address is returned
TEST(Compat_String, LelyCompatMemmove_NonZeroCount) {
  const char data[] = "abcde";

  const void* result = lely_compat_memmove(memory.data(), data, sizeof(data));

  POINTERS_EQUAL(memory.data(), result);
  std::copy(std::begin(data), std::end(data), std::begin(expected));
  CheckExpected();
}

/// \Given memory area
///
/// \When calling lely_compat_memmove() with the area with zero count
///
/// \Then memory area is not modified; memory area address is returned
TEST(Compat_String, LelyCompatMemmove_ZeroCount) {
  const char data[] = "abcde";

  const void* result = lely_compat_memmove(memory.data(), data, 0u);

  POINTERS_EQUAL(memory.data(), result);
  CheckExpected();
}

/// \Given memory area
///
/// \When calling lely_compat_memmove() with source pointer value larger
///       than target pointer value
///
/// \Then count bytes in memory area is overwritten;
///       memory area address is returned
TEST(Compat_String, LelyCompatMemmove_SourceLarger) {
  std::iota(std::begin(memory), std::end(memory), 0);

  const size_t offset = (totalSize / 2) - 1;  // overlapping regions
  const size_t count = (totalSize / 2) + 1;   // (moving > half bytes)
  const void* result =
      lely_compat_memmove(memory.data(), memory.data() + offset, count);

  POINTERS_EQUAL(memory.data(), result);
  std::iota(std::begin(expected), std::end(expected), 0);
  std::iota(std::begin(expected), std::begin(expected) + count, offset);
  CheckExpected();
}

/// \Given memory area
///
/// \When calling lely_compat_memmove() with source pointer value smaller
//        than target pointer value
///
/// \Then count bytes in memory area is overwritten;
///       memory area address is returned
TEST(Compat_String, LelyCompatMemmove_TargetLarger) {
  std::iota(std::begin(memory), std::end(memory), 0);

  const size_t offset = (totalSize / 2) - 1;  // overlapping regions
  const size_t count = (totalSize / 2) + 1;   // (moving > half bytes)
  const void* result =
      lely_compat_memmove(memory.data() + offset, memory.data(), count);

  POINTERS_EQUAL(memory.data() + offset, result);
  std::iota(std::begin(expected), std::end(expected), 0);
  std::iota(std::begin(expected) + offset,
            std::begin(expected) + offset + count, 0);
  CheckExpected();
}

/// \Given memory area
///
/// \When calling lely_compat_memmove() with source and target poiner equal
///
/// \Then memory is left unmodified; memory area address is returned
TEST(Compat_String, LelyCompatMemmove_SourceEqualTarget) {
  std::iota(std::begin(memory), std::end(memory), 0);

  const void* result =
      lely_compat_memmove(memory.data(), memory.data(), totalSize / 2);

  POINTERS_EQUAL(memory.data(), result);
  std::iota(std::begin(expected), std::end(expected), 0);
  CheckExpected();
}

///@}

/// @name lely_compat_memcmp
///@{

/// \Given memory areas to compare
///
/// \When calling lely_compat_memcmp() with zero byte count
///
/// \Then 0 is returned
TEST(Compat_String, LelyCompatMemcmp_ZeroCount) {
  const char area1[] = "XYZ";
  const char area2[] = "ABC";

  const int result = lely_compat_memcmp(area1, area2, 0);

  CHECK_EQUAL(0, result);
}

/// \Given memory areas to compare
///
/// \When calling lely_compat_memcmp() with 'greater' area as the first argument
///
/// \Then value greater than 0 is returned
TEST(Compat_String, LelyCompatMemcmp_LeftGreater) {
  const uint8_t area1[] = {0u, 0u, 11u};
  const uint8_t area2[] = {0u, 0u, 10u};

  const int result = lely_compat_memcmp(area1, area2, sizeof(area1));

  CHECK_COMPARE(result, >, 0);
}

/// \Given memory areas to compare
///
/// \When calling lely_compat_memcmp() with 'greater' area
///       as the second argument
///
/// \Then value less than 0 is returned
TEST(Compat_String, LelyCompatMemcmp_RightGreater) {
  const uint8_t area1[] = {0u, 0u, 11u};
  const uint8_t area2[] = {0u, 0u, 15u};

  const int result = lely_compat_memcmp(area1, area2, sizeof(area1));

  CHECK_COMPARE(result, <, 0);
}

/// \Given memory areas to compare
///
/// \When calling lely_compat_memcmp() with equal areas
///
/// \Then 0 is returned
TEST(Compat_String, LelyCompatMemcmp_AreasEqual) {
  const uint8_t area1[] = {0u, 0u, 11u};
  const uint8_t area2[] = {0u, 0u, 11u};

  const int result = lely_compat_memcmp(area1, area2, sizeof(area1));

  CHECK_EQUAL(0, result);
}

///@}

/// @name lely_compat_strcmp
///@{

/// \Given strings to compare
///
/// \When calling lely_compat_strcmp() with 'greater' string
///       as the first argument
///
/// \Then value greater than 0 is returned
TEST(Compat_String, LelyCompatStrcmp_LeftGreater) {
  const char str1[] = "XYZ";
  const char str2[] = "ABC";

  const int result = lely_compat_strcmp(str1, str2);

  CHECK_COMPARE(result, >, 0);
}

/// \Given strings to compare
///
/// \When calling lely_compat_strcmp() with 'greater' string
///       as the second argument
///
/// \Then value less than 0 is returned
TEST(Compat_String, LelyCompatStrcmp_RightGreater) {
  const char str1[] = "ABC";
  const char str2[] = "XYZ";

  const int result = lely_compat_strcmp(str1, str2);

  CHECK_COMPARE(result, <, 0);
}

// \Given strings to compare
///
/// \When calling lely_compat_strcmp() with equal strings
///
/// \Then 0 is returned
TEST(Compat_String, LelyCompatStrcmp_EqualStrings) {
  const char str1[] = "ABCD";
  const char str2[] = "ABCD";

  const int result = lely_compat_strcmp(str1, str2);

  CHECK_EQUAL(0, result);
}

// \Given strings to compare
///
/// \When calling lely_compat_strcmp() with shorter string
///       as the first argument
///
/// \Then value less than 0 is returned
TEST(Compat_String, LelyCompatStrcmp_LeftShorter) {
  const char str1[] = "ABC";
  const char str2[] = "ABCD";

  const int result = lely_compat_strcmp(str1, str2);

  CHECK_COMPARE(result, <, 0);
}

// \Given strings to compare
///
/// \When calling lely_compat_strcmp() with shorter string
///       as the second argument
///
/// \Then value greater than 0 is returned
TEST(Compat_String, LelyCompatStrcmp_RightShorter) {
  const char str1[] = "ABCD";
  const char str2[] = "ABC";

  const int result = lely_compat_strcmp(str1, str2);

  CHECK_COMPARE(result, >, 0);
}

///@}

/// @name lely_compat_strncmp
///@{

/// \Given strings to compare
///
/// \When calling lely_compat_strncmp() with zero byte count
///
/// \Then 0 is returned
TEST(Compat_String, LelyCompatStrncmp_ZeroCount) {
  const char str1[] = "XYZ";
  const char str2[] = "ABC";

  const int result = lely_compat_strncmp(str1, str2, 0);

  CHECK_EQUAL(0, result);
}

/// \Given strings to compare
///
/// \When calling lely_compat_strncmp() with 'greater' string
///       as the first argument
///
/// \Then value greater than 0 is returned
TEST(Compat_String, LelyCompatStrncmp_LeftGreater) {
  const char str1[] = "XYZ";
  const char str2[] = "ABC";

  const int result = lely_compat_strncmp(str1, str2, sizeof(str1));

  CHECK_COMPARE(result, >, 0);
}

/// \Given strings to compare
///
/// \When calling lely_compat_strncmp() with 'greater' string
///       as the second argument
///
/// \Then value less than 0 is returned
TEST(Compat_String, LelyCompatStrncmp_RightGreater) {
  const char str1[] = "ABC";
  const char str2[] = "XYZ";

  const int result = lely_compat_strncmp(str1, str2, sizeof(str1));

  CHECK_COMPARE(result, <, 0);
}

/// \Given strings to compare
///
/// \When calling lely_compat_strncmp() with strings equal
///       on the first count of bytes
///
/// \Then 0 is returned
TEST(Compat_String, LelyCompatStrncmp_EqualCountBytes) {
  const char str1[] = "ABCx";
  const char str2[] = "ABCz";

  const int result = lely_compat_strncmp(str1, str2, 3);

  CHECK_EQUAL(0, result);
}

// \Given strings to compare
///
/// \When calling lely_compat_strncmp() with equal strings
///
/// \Then 0 is returned
TEST(Compat_String, LelyCompatStrncmp_EqualStrings) {
  const char str1[] = "ABCD";
  const char str2[] = "ABCD";

  const int result = lely_compat_strncmp(str1, str2, sizeof(str1));

  CHECK_EQUAL(0, result);
}

// \Given strings to compare
///
/// \When calling lely_compat_strncmp() with shorter string
///       as the first argument
///
/// \Then value less than 0 is returned
TEST(Compat_String, LelyCompatStrncmp_LeftShorter) {
  const char str1[] = "ABC";
  const char str2[] = "ABCD";

  const int result = lely_compat_strncmp(str1, str2, sizeof(str2));

  CHECK_COMPARE(result, <, 0);
}

// \Given strings to compare
///
/// \When calling lely_compat_strncmp() with shorter string
///       as the second argument
///
/// \Then value greater than 0 is returned
TEST(Compat_String, LelyCompatStrncmp_RightShorter) {
  const char str1[] = "ABCD";
  const char str2[] = "ABC";

  const int result = lely_compat_strncmp(str1, str2, sizeof(str1));

  CHECK_COMPARE(result, >, 0);
}

///@}

/// @name lely_compat_memset
///@{

/// \Given memory area
///
/// \When calling lely_compat_memset() for the area with pattern and size
///
/// \Then pattern is set for specifed count of bytes;
///       memory area address is returned
TEST(Compat_String, LelyCompatMemset_NonZeroCount) {
  const uint8_t pattern = 0xDAu;
  const auto count = totalSize / 2;

  const void* result = lely_compat_memset(memory.data(), pattern, count);

  POINTERS_EQUAL(memory.data(), result);
  std::fill_n(std::begin(expected), count, pattern);
  CheckExpected();
}

/// \Given memory area
///
/// \When calling lely_compat_memset() for the area with pattern and zero size
///
/// \Then memory area is not modified; memory area address is returned
TEST(Compat_String, LelyCompatMemset_Count) {
  const uint8_t pattern = 0xDAu;

  const void* result = lely_compat_memset(memory.data(), pattern, 0u);

  POINTERS_EQUAL(memory.data(), result);
  CheckExpected();
}

///@}

/// @name lely_compat_strlen
///@{

/// \Given empty null-terminated string
///
/// \When calling lely_compat_strlen() with the string
///
/// \Then 0 is returned
TEST(Compat_String, LelyCompatStrlen_EmptyString) {
  const char str[] = "";

  const size_t result = lely_compat_strlen(str);

  CHECK_EQUAL(0, result);
}

/// \Given non-empty null-terminated string
///
/// \When calling lely_compat_strlen() with the string
///
/// \Then string length is returned
TEST(Compat_String, LelyCompatStrlen_NonEmptyString) {
  const char str[] = "abcdef";

  const size_t result = lely_compat_strlen(str);

  CHECK_EQUAL(sizeof(str) - 1, result);
}

///@}

#endif

#if LELY_NO_HOSTED || (!(_MSC_VER >= 1400) && !(_POSIX_C_SOURCE >= 200809L) && \
                       !defined(__MINGW32__))

/// @name lely_compat_strnlen
///@{

/// \Given empty null-terminated string
///
/// \When calling lely_compat_strlen() with the string and larger maximum length
///
/// \Then 0  is returned
TEST(Compat_String, LelyCompatStrnlen_EmptyString) {
  const char str[] = "";
  const size_t maxlen = sizeof(str) + 1;

  const size_t result = lely_compat_strnlen(str, maxlen);

  CHECK_EQUAL(0, result);
}

/// \Given non-empty null-terminated string
///
/// \When calling lely_compat_strlen() with the string and larger maximum length
///
/// \Then string length is returned
TEST(Compat_String, LelyCompatStrnlen_StringShorterThanMaximumLength) {
  const char str[] = "abcdef";
  const size_t maxlen = sizeof(str) + 1;

  const size_t result = lely_compat_strnlen(str, maxlen);

  CHECK_EQUAL(sizeof(str) - 1, result);
}

/// \Given non-empty null-terminated string
///
/// \When calling lely_compat_strlen() with the string
///       and smaller maximum length
///
/// \Then the maximum length is returned
TEST(Compat_String, LelyCompatStrnlen_StringLongerThanMaximumLength) {
  const char str[] = "abcdef";
  const size_t maxlen = sizeof(str) / 2;

  const size_t result = lely_compat_strnlen(str, maxlen);

  CHECK_EQUAL(maxlen, result);
}

/// \Given non-empty null-terminated string
///
/// \When calling lely_compat_strlen() with the string and zero maximum length
///
/// \Then 0 is returned
TEST(Compat_String, LelyCompatStrnlen_ZeroMaximumLength) {
  const char str[] = "abcdef";
  const size_t maxlen = 0;

  const size_t result = lely_compat_strnlen(str, maxlen);

  CHECK_EQUAL(0, result);
}

///@}

#endif
