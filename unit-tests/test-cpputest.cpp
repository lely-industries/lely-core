
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
