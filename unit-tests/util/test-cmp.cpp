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

#include <CppUTest/TestHarness.h>

#include <lely/util/cmp.h>

TEST_GROUP(Util_Cmp){};

/// @name ptr_cmp()
///@{

/// \Given N/A
///
/// \When ptr_cmp() is called with two comparable pointers
///
/// \Then if first argument precedes the second one then -1 is returned; if both
///       arguments point to the same entity then 0 is returned; if second
///       argument precedes the first one then 1 is returned
TEST(Util_Cmp, PtrCmp) {
  const int tab[2] = {0};
  const int* const p1 = &tab[0];
  const int* const p2 = &tab[1];

  CHECK_EQUAL(-1, ptr_cmp(p1, p2));
  CHECK_EQUAL(0, ptr_cmp(p1, p1));
  CHECK_EQUAL(1, ptr_cmp(p2, p1));
}

///@}

/// @name str_cmp()
///@{

/// \Given N/A
///
/// \When str_cmp() is called with exactly one of the arguments being a null
///       pointer
///
/// \Then 1 is returned if the second argument is a null pointer; -1 is returned
///       if the first argument is a null pointer
TEST(Util_Cmp, StrCmp_NullPointers) {
  const char p[] = "arhgesv";

  CHECK_EQUAL(1, str_cmp(p, nullptr));
  CHECK_EQUAL(-1, str_cmp(nullptr, p));
}

/// \Given N/A
///
/// \When str_cmp() is called with two pointers pointing to the same character
///       array
///
/// \Then 0 is returned
TEST(Util_Cmp, StrCmp_PointersEqual) {
  const char p1[] = "arhgesv";
  const char* const p2 = p1;

  CHECK_EQUAL(0, str_cmp(p1, p2));
}

/// \Given N/A
///
/// \When str_cmp() is called with two pointers to different character arrays
///       with identical contents
///
/// \Then 0 is returned
TEST(Util_Cmp, StrCmp_StringsEqual) {
  const char p1[] = "%arhgesvdfg45-";
  const char p2[] = "%arhgesvdfg45-";

  CHECK_EQUAL(0, str_cmp(p1, p2));
}

/// \Given N/A
///
/// \When str_cmp() is called with two pointers to different character arrays of
///       same length and the first character of the first array is greater than
///       the first character of the second array
///
/// \Then a value greater than zero is returned
TEST(Util_Cmp, StrCmp_StrMoreChar) {
  const char p1[] = "brhgesv";
  const char p2[] = "arhgesv";

  CHECK_COMPARE(0, <, str_cmp(p1, p2));
}

/// \Given N/A
///
/// \When str_cmp() is called with two pointers to different character arrays of
///       different lengths, the shorter array is a prefix of the longer one and
///       the longer array is passed as first argument
///
/// \Then a value greater than zero is returned
TEST(Util_Cmp, StrCmp_StrMoreNull) {
  const char p1[] = "arhgesvv";
  const char p2[] = "arhgesv";

  CHECK_COMPARE(0, <, str_cmp(p1, p2));
}

/// \Given N/A
///
/// \When str_cmp() is called with two pointers to different character arrays of
///       same length and the first character of the first array is less than
///       the first character of the second array
///
/// \Then a value less than zero is returned
TEST(Util_Cmp, StrCmp_StrLessChar) {
  const char p1[] = "brhgesv";
  const char p2[] = "hrhgesv";

  CHECK_COMPARE(0, >, str_cmp(p1, p2));
}

/// \Given N/A
///
/// \When str_cmp() is called with two pointers to different character arrays of
///       different lengths, the shorter array is a prefix of the longer one and
///       the shorter array is passed as first argument
///
/// \Then a value less than zero is returned
TEST(Util_Cmp, StrCmp_StrLessNull) {
  const char p1[] = "arhgesv";
  const char p2[] = "arhgesvddd";

  CHECK_COMPARE(0, >, str_cmp(p1, p2));
}

///@}

/// @name str_case_cmp()
///@{

/// \Given N/A
///
/// \When str_case_cmp() is called with exactly one of the arguments being a
///       null pointer
///
/// \Then 1 is returned if the second argument is a null pointer; -1 is returned
///       if the first argument is a null pointer
TEST(Util_Cmp, StrCaseCmp_NullPointers) {
  const char p1[] = "arhgesv";
  const char* const p2 = p1;

  CHECK_EQUAL(1, str_case_cmp(p1, nullptr));
  CHECK_EQUAL(-1, str_case_cmp(nullptr, p2));
}

/// \Given N/A
///
/// \When str_case_cmp() is called with two pointers pointing to the same
///       character array
///
/// \Then 0 is returned
TEST(Util_Cmp, StrCaseCmp_PointersEqual) {
  const char p1[] = "arhgesv";
  const char* const p2 = p1;

  CHECK_EQUAL(0, str_case_cmp(p1, p2));
}

/// \Given N/A
///
/// \When str_case_cmp() is called with two pointers to different character
///       arrays of same length with same characters but of different case
///
/// \Then 0 is returned
TEST(Util_Cmp, StrCaseCmp_Equal) {
  const char p1[] = "arhgesv";
  const char p2[] = "ArhGesv";

  CHECK_EQUAL(0, str_case_cmp(p1, p2));
}

/// \Given N/A
///
/// \When str_case_cmp() is called with two pointers to different character
///       arrays of same length with common case insensitive prefixes and first
///       different character being greater in the first array
///
/// \Then a value greater than zero is returned
TEST(Util_Cmp, StrCaseCmp_StrMoreChar) {
  const char p1[] = "arhgesverh";
  const char p2[] = "ArhGesvaaa";

  CHECK_COMPARE(0, <, str_case_cmp(p1, p2));
}

/// \Given N/A
///
/// \When str_case_cmp() is called with two pointers to different character
///       arrays of different lengths, the shorter array is a case insensitive
///       prefix of the longer one and the longer array is passed as first
///       argument
///
/// \Then a value greater than zero is returned
TEST(Util_Cmp, StrCaseCmp_StrMoreNull) {
  const char p1[] = "arhgesverh";
  const char p2[] = "ArhGesv";

  CHECK_COMPARE(0, <, str_case_cmp(p1, p2));
}

/// \Given N/A
///
/// \When str_case_cmp() is called with two pointers to different character
///       arrays of same length with common case insensitive prefixes and first
///       different character being greater in the second array
///
/// \Then a value less than zero is returned
TEST(Util_Cmp, StrCaseCmp_StrLessChar) {
  const char p1[] = "arhgesvaaa";
  const char p2[] = "ArhGesvegr";

  CHECK_COMPARE(0, >, str_case_cmp(p1, p2));
}

/// \Given N/A
///
/// \When str_case_cmp() is called with two pointers to different character
///       arrays of different lengths, the shorter array is a case insensitive
///       prefix of the longer one and the longer array is passed as second
///       argument
///
/// \Then a value less than zero is returned
TEST(Util_Cmp, StrCaseCmp_StrLessNull) {
  const char p1[] = "arhgesv";
  const char p2[] = "ArhGesvegr";

  CHECK_COMPARE(0, >, str_case_cmp(p1, p2));
}

///@}

/// @name basic type comparison functions
///@{

/// \Given N/A
///
/// \When <typename>_cmp() (e.g. uint32_cmp()) is called with pointers to
///       variables with same value
///
/// \Then 0 is returned
TEST(Util_Cmp, TypeCmp_Equal) {
  int clangformat_fix = 0;
  (void)clangformat_fix;
#define LELY_UTIL_DEFINE_TYPE(name, type) \
  { \
    const type a = static_cast<type>(1); \
    const type b = static_cast<type>(1); \
\
    const auto cmp = name##_cmp(&a, &b); \
\
    CHECK_EQUAL_TEXT(0, cmp, "checked type: <" #name ">"); \
  }
#include <lely/util/def/type.def>
#undef LELY_UTIL_DEFINE_TYPE
}

/// \Given N/A
///
/// \When <typename>_cmp() (e.g. uint32_cmp()) is called with two pointers
///       where the first pointer points to the greater variable
///
/// \Then 1 is returned
TEST(Util_Cmp, TypeCmp_FirstGreater) {
  int clangformat_fix = 0;
  (void)clangformat_fix;
#define LELY_UTIL_DEFINE_TYPE(name, type) \
  { \
    const type a = static_cast<type>(1); \
    const type b = static_cast<type>(0); \
\
    const auto cmp = name##_cmp(&a, &b); \
\
    CHECK_EQUAL_TEXT(1, cmp, "checked type: <" #name ">"); \
  }
#include <lely/util/def/type.def>  // NOLINT(build/include)
#undef LELY_UTIL_DEFINE_TYPE
}

/// \Given N/A
///
/// \When <typename>_cmp() (e.g. uint32_cmp()) is called with two pointers where
///       the second pointer points to the greater variable
///
/// \Then -1 is returned
TEST(Util_Cmp, TypeCmp_SecondGreater) {
  int clangformat_fix = 0;
  (void)clangformat_fix;
#define LELY_UTIL_DEFINE_TYPE(name, type) \
  { \
    const type a = static_cast<type>(0); \
    const type b = static_cast<type>(1); \
\
    const auto cmp = name##_cmp(&a, &b); \
\
    CHECK_EQUAL_TEXT(-1, cmp, "checked type: <" #name ">"); \
  }
#include <lely/util/def/type.def>  // NOLINT(build/include)
#undef LELY_UTIL_DEFINE_TYPE
}

/// \Given N/A
///
/// \When <typename>_cmp() (e.g. uint32_cmp()) is called with two pointers to
///       same variable
///
/// \Then 0 is returned
TEST(Util_Cmp, TypeCmp_PtrEqual) {
  int clangformat_fix = 0;
  (void)clangformat_fix;
#define LELY_UTIL_DEFINE_TYPE(name, type) \
  { \
    const type a{}; \
\
    const auto cmp = name##_cmp(&a, &a); \
\
    CHECK_EQUAL_TEXT(0, cmp, "checked type: <" #name ">"); \
  }
#include <lely/util/def/type.def>  // NOLINT(build/include)
#undef LELY_UTIL_DEFINE_TYPE
}

/// \Given N/A
///
/// \When <typename>_cmp() (e.g. uint32_cmp()) is called with a null pointer as
///       first argument and a non-null pointer as second argument
///
/// \Then -1 is returned
TEST(Util_Cmp, TypeCmp_FirstPtrNull) {
  int clangformat_fix = 0;
  (void)clangformat_fix;
#define LELY_UTIL_DEFINE_TYPE(name, type) \
  { \
    const type a{}; \
\
    const auto cmp = name##_cmp(nullptr, &a); \
\
    CHECK_EQUAL_TEXT(-1, cmp, "checked type: <" #name ">"); \
  }
#include <lely/util/def/type.def>  // NOLINT(build/include)
#undef LELY_UTIL_DEFINE_TYPE
}

/// \Given N/A
///
/// \When <typename>_cmp() (e.g. uint32_cmp()) is called with a non-null pointer
///       as first argument and a null pointer as second argument
///
/// \Then 1 is returned
TEST(Util_Cmp, TypeCmp_SecondPtrNull) {
  int clangformat_fix = 0;
  (void)clangformat_fix;
#define LELY_UTIL_DEFINE_TYPE(name, type) \
  { \
    const type a{}; \
\
    const auto cmp = name##_cmp(&a, nullptr); \
\
    CHECK_EQUAL_TEXT(1, cmp, "checked type: <" #name ">"); \
  }
#include <lely/util/def/type.def>  // NOLINT(build/include)
#undef LELY_UTIL_DEFINE_TYPE
}

/// \Given N/A
///
/// \When <typename>_cmp() (e.g. uint32_cmp()) is called with two null pointers
///
/// \Then 0 is returned
TEST(Util_Cmp, TypeCmp_BothPtrNull) {
  int clangformat_fix = 0;
  (void)clangformat_fix;
#define LELY_UTIL_DEFINE_TYPE(name, type) \
  { \
    const auto cmp = name##_cmp(nullptr, nullptr); \
\
    CHECK_EQUAL_TEXT(0, cmp, "checked type: <" #name ">"); \
  }
#include <lely/util/def/type.def>  // NOLINT(build/include)
#undef LELY_UTIL_DEFINE_TYPE
}

///@}
