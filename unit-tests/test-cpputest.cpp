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

class ClassName {};

TEST_GROUP(ClassNameGroup) {
  ClassName* className;
  TEST_SETUP() { className = new ClassName(); }
  TEST_TEARDOWN() { delete className; }
};

TEST(ClassNameGroup, Create) {
  CHECK(true);
  CHECK(nullptr != className);
  CHECK_TEXT(true, "Failure text");
  CHECK_TRUE(true);
  CHECK_FALSE(false);

  CHECK_EQUAL(1, 1);

  STRCMP_EQUAL("hello", "hello");
  STRCMP_NOCASE_EQUAL("hello", "HELLO");
  STRCMP_CONTAINS("hello", "xyzhelloxyz");

  LONGS_EQUAL(1, 1);
  BYTES_EQUAL(0xFF, 0xFF);
  POINTERS_EQUAL(nullptr, nullptr);
  DOUBLES_EQUAL(1.000, 1.001, .01);
}

IGNORE_TEST(ClassNameGroup, Fail) { FAIL("Failed test"); }
