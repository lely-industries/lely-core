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
#include <lely/util/cmp.h>

TEST_GROUP(UtilCmp){};

TEST(UtilCmp, StrCmp_NullsAndEqual) {
  const char p1[] = "arhgesv";
  const char* const p2 = p1;

  CHECK_EQUAL(1, str_cmp(p1, nullptr));
  CHECK_EQUAL(-1, str_cmp(nullptr, p1));
  CHECK_EQUAL(0, str_cmp(p1, p2));
}

TEST(UtilCmp, StrCmp_StringsEqual) {
  const char p1[] = "%arhgesvdfg45-";
  const char p2[] = "%arhgesvdfg45-";

  CHECK_EQUAL(0, str_cmp(p1, p2));
}

TEST(UtilCmp, StrCmp_StrMoreChar) {
  const char p1[] = "brhgesv";
  const char p2[] = "arhgesv";

  CHECK_COMPARE(0, <, str_cmp(p1, p2));
}

TEST(UtilCmp, StrCmp_StrMoreNull) {
  const char p1[] = "arhgesvv";
  const char p2[] = "arhgesv";

  CHECK_COMPARE(0, <, str_cmp(p1, p2));
}

TEST(UtilCmp, StrCmp_StrLessChar) {
  const char p1[] = "brhgesv";
  const char p2[] = "hrhgesv";

  CHECK_COMPARE(0, >, str_cmp(p1, p2));
}

TEST(UtilCmp, StrCmp_StrLessNull) {
  const char p1[] = "arhgesv";
  const char p2[] = "arhgesvddd";

  CHECK_COMPARE(0, >, str_cmp(p1, p2));
}

TEST(UtilCmp, StrCaseCmp_NullsAndEqual) {
  const char p1[] = "arhgesv";
  const char* const p2 = p1;

  CHECK_EQUAL(1, str_case_cmp(p1, nullptr));
  CHECK_EQUAL(-1, str_case_cmp(nullptr, p2));
  CHECK_EQUAL(0, str_case_cmp(p1, p2));
}

TEST(UtilCmp, StrCaseCmp_Equal) {
  const char p1[] = "arhgesv";
  const char p2[] = "ArhGesv";

  CHECK_EQUAL(0, str_case_cmp(p1, p2));
}

TEST(UtilCmp, StrCaseCmp_StrMoreChar) {
  const char p1[] = "arhgesverh";
  const char p2[] = "ArhGesvaaa";

  CHECK_COMPARE(0, <, str_case_cmp(p1, p2));
}

TEST(UtilCmp, StrCaseCmp_StrMoreNull) {
  const char p1[] = "arhgesverh";
  const char p2[] = "ArhGesv";

  CHECK_COMPARE(0, <, str_case_cmp(p1, p2));
}

TEST(UtilCmp, StrCaseCmp_StrLessChar) {
  const char p1[] = "arhgesvaaa";
  const char p2[] = "ArhGesvegr";

  CHECK_COMPARE(0, >, str_case_cmp(p1, p2));
}

TEST(UtilCmp, StrCaseCmp_StrLessNull) {
  const char p1[] = "arhgesv";
  const char p2[] = "ArhGesvegr";

  CHECK_COMPARE(0, >, str_case_cmp(p1, p2));
}
