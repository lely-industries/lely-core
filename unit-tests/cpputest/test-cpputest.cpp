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

/*** Unit Tests Structure Description ***/

/* Test structure described below is to be considered as a strong suggestion,
 * hence frequent usage of the word SHOULD. It can be altered or adapted
 * if that is necessary to improve the test readability or to reduce the amount
 * of negligible code. This usually applies to a very complicated or trivial
 * test cases.
 */

/* Headers SHOULD be included in the following order with each group separated
 * by a blank line. This rule can be bent if project setup entails it.
 */

/* Related header */
// #include "test-cpputest.h"

/* C system headers */
#include <sys/types.h>
#include <unistd.h>

/* C/C++ standard library headers */
#include <cstring>
#include <vector>

/* Other libraries' headers starting with CppUTest (if possible) */
#include <CppUTest/TestHarness.h>

/* Project's headers */
#include <lely/util/error.h>

/* --------------- sample test code --------------- */
struct param_struct {
  int param1;
  const char* param2;
};

ssize_t
sample_func(const struct param_struct* pc, const bool flag) {
  if (!pc || !pc->param2) return -1;

  if (flag)
    return strlen(pc->param2) * pc->param1;
  else
    return strlen(pc->param2) / pc->param1;
}
/* ------------ end of sample test code ----------- */

/* Test groups SHOULD pertain to a single function or data structure in a given
 * module. Group name format is defined as:
 *
 *      ModuleName_FunctionName
 *      ModuleName_StructureName
 *
 * Both phrases SHOULD be written in PascalCase (convert names if necessary).
 * If function/structure name is prefixed with a module name it SHOULDN'T be
 * repeated, e.g. 'can_msg_bits()' function in CAN module will become
 * 'CAN_MsgBits'.
 *
 * Arguments that are non-trivially constructed SHOULD be declared in group
 * body and initialized with default values in TEST_SETUP(). When needed,
 * TEST_TEARDOWN() SHOULD be used to free resources.
 */
TEST_GROUP(Module_SampleFunc) {
  /* constants */
  static const size_t STR_LEN = 13;

  /* member variables */
  param_struct param;
  char test_str[STR_LEN] = "testtesttest";

  /* helper functions */
  void SampleHelperFunction(const int idx, const char c) { test_str[idx] = c; }

  /* test setup/teardown */
  TEST_SETUP() {
    param.param1 = 0;
    param.param2 = nullptr;
  }

  TEST_TEARDOWN(){};
};

/* Tests SHOULD examine one aspect of a function. Each test will call
 * a function once and analyse its output. Test name SHOULD describe what
 * is being tested and SHOULD be written with PascalCase, optionally with
 * underscores to improve readability.
 *
 *      TestAspectOne
 *      TestAspectTwo_TestMode
 *      TestAspectTwo_TestOtherMode
 *
 * Test code structure must follow AAA/GWT (Arrange-Act-Assert/Given-When-Then)
 * pattern with each section separated by an empty line.
 */
TEST(Module_SampleFunc, InvalidArgs) {
  /* arrange */
  param.param2 = nullptr;

  /* act */
  auto ret = sample_func(&param, true);

  /* assert */
  CHECK_EQUAL(-1, ret);
}

TEST(Module_SampleFunc, Test_FlagTrue_1) {
  /* given */
  param.param1 = 5;
  param.param2 = test_str;
  SampleHelperFunction(10, '\0');

  /* when */
  auto ret = sample_func(&param, true);

  /* then */
  CHECK_EQUAL(50, ret);
}

/*** CppUTest self-check ***/

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

/* --------------- CppUTest main() ---------------- */
#include <CppUTest/CommandLineTestRunner.h>

int
main(int ac, char** av) {
  return RUN_ALL_TESTS(ac, av);
}
/* ------------ end of CppUTest main() ------------ */
