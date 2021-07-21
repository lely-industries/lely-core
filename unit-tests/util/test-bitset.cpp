/**
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <climits>
#include <set>
#include <string>

#include <CppUTest/TestHarness.h>

#include <lely/util/bitset.h>

#if !LELY_NO_MALLOC

TEST_GROUP(Util_BitsetInit) {
  bitset set;
  int GetSizeForBits(const int requested_size) {
    bitset_init(&set, requested_size);
    int result = bitset_size(&set);
    bitset_fini(&set);
    return result;
  }
};

TEST(Util_BitsetInit, BitsetSize) {
  const int INT_BIT = sizeof(int) * CHAR_BIT;

  CHECK_EQUAL(0, GetSizeForBits(0));
  CHECK_EQUAL(INT_BIT, GetSizeForBits(1));
  CHECK_EQUAL(INT_BIT, GetSizeForBits(INT_BIT - 1));
  CHECK_EQUAL(INT_BIT, GetSizeForBits(INT_BIT));
  CHECK_EQUAL(2 * INT_BIT, GetSizeForBits(INT_BIT + 1));
  CHECK_EQUAL(2 * INT_BIT, GetSizeForBits(2 * INT_BIT));
  CHECK_EQUAL(3 * INT_BIT, GetSizeForBits(2 * INT_BIT + 1));
}

TEST_GROUP(Util_Bitset) {
  bitset set;
  const int SET_SIZE = 64;

  void CheckAllStates(const int expected_state,
                      const std::set<int>& negated_indexes = std::set<int>()) {
    for (int i = 0; i < SET_SIZE; i++) {
      const int check_state =
          (negated_indexes.count(i) == 0) ? expected_state : !expected_state;
      const std::string msg_str =
          "testing bitset_test(set, " + std::to_string(i) + ")";
      CHECK_EQUAL_TEXT(check_state, bitset_test(&set, i), msg_str.c_str());
    }
  }

  TEST_SETUP() {
    bitset_init(&set, SET_SIZE);
    CHECK_EQUAL(SET_SIZE, bitset_size(&set));
    bitset_clr_all(&set);
  }
  TEST_TEARDOWN() { bitset_fini(&set); }
};

TEST(Util_Bitset, BitsetTest) {
  bitset_set(&set, 0);
  bitset_set(&set, 1);
  bitset_set(&set, 5);
  bitset_set(&set, SET_SIZE - 1);

  CHECK_EQUAL(1, bitset_test(&set, 0));
  CHECK_EQUAL(1, bitset_test(&set, 1));
  CHECK_EQUAL(1, bitset_test(&set, 5));
  CHECK_EQUAL(1, bitset_test(&set, SET_SIZE - 1));
}

TEST(Util_Bitset, BitsetTest_OutOfBoundsReturnsZero) {
  bitset_set_all(&set);

  CHECK_EQUAL(0, bitset_test(&set, -1));
  CHECK_EQUAL(0, bitset_test(&set, SET_SIZE));
  CHECK_EQUAL(0, bitset_test(&set, SET_SIZE + 1));
}

TEST(Util_Bitset, BitsetSet_OutOfBoundsIndex) {
  bitset_set(&set, -1);
  bitset_set(&set, SET_SIZE);
  bitset_set(&set, SET_SIZE + 1);

  CheckAllStates(0);
}

TEST(Util_Bitset, BitsetSetAll) {
  bitset_set_all(&set);

  CheckAllStates(1);
}

TEST(Util_Bitset, BitsetClr_CorrectIndex) {
  bitset_set_all(&set);
  bitset_clr(&set, 0);
  bitset_clr(&set, 1);
  bitset_clr(&set, SET_SIZE - 1);

  CheckAllStates(1, {0, 1, SET_SIZE - 1});
}

TEST(Util_Bitset, BitsetClr_OutOfBoundsIndex) {
  bitset_set_all(&set);
  bitset_clr(&set, -1);
  bitset_clr(&set, SET_SIZE);
  bitset_clr(&set, SET_SIZE + 1);

  CheckAllStates(1);
}

TEST(Util_Bitset, BitsetClrAll) {
  bitset_set_all(&set);
  bitset_clr_all(&set);

  CheckAllStates(0);
}

TEST(Util_Bitset, BitsetCompl) {
  bitset_set(&set, 0);
  bitset_set(&set, 1);

  bitset_compl(&set);

  CheckAllStates(1, {0, 1});
}

TEST(Util_Bitset, BitsetFfs_AllZero) { CHECK_EQUAL(0, bitset_ffs(&set)); }

TEST(Util_Bitset, BitsetFfs_FirstSet) {
  bitset_set(&set, 0);

  CHECK_EQUAL(1, bitset_ffs(&set));
}

TEST(Util_Bitset, BitsetFfs_LastSet) {
  bitset_set(&set, SET_SIZE - 1);

  CHECK_EQUAL(SET_SIZE, bitset_ffs(&set));
}

TEST(Util_Bitset, BitsetFfs_MiddleSet) {
  bitset_set(&set, 23);
  bitset_set(&set, 25);

  CHECK_EQUAL(24, bitset_ffs(&set));
}

TEST(Util_Bitset, BitsetFfz_AllSet) {
  bitset_set_all(&set);

  CHECK_EQUAL(0, bitset_ffz(&set));
}

TEST(Util_Bitset, BitsetFfz_FirstZero) {
  bitset_set_all(&set);
  bitset_clr(&set, 0);

  CHECK_EQUAL(1, bitset_ffz(&set));
}

TEST(Util_Bitset, BitsetFfz_LastZero) {
  bitset_set_all(&set);

  bitset_clr(&set, SET_SIZE - 1);

  CHECK_EQUAL(SET_SIZE, bitset_ffz(&set));
}

TEST(Util_Bitset, BitsetFfz_MiddleZero) {
  bitset_set_all(&set);

  bitset_clr(&set, 10);
  bitset_clr(&set, 16);

  CHECK_EQUAL(11, bitset_ffz(&set));
}

TEST(Util_Bitset, BitsetFns_AllZeros) {
  bitset_set(&set, 10);

  CHECK_EQUAL(0, bitset_fns(&set, 11));
}

TEST(Util_Bitset, BitsetFns_NextIsSet) {
  bitset_set(&set, 10);
  bitset_set(&set, 11);

  CHECK_EQUAL(11, bitset_fns(&set, 10));
}

TEST(Util_Bitset, BitsetFns_LastIsSet) {
  bitset_set(&set, 10);
  bitset_set(&set, SET_SIZE - 1);

  CHECK_EQUAL(SET_SIZE, bitset_fns(&set, 11));
}

TEST(Util_Bitset, BitsetFns_OutOfBoundsIdx) {
  bitset_set(&set, 0);

  CHECK_EQUAL(0, bitset_fns(&set, SET_SIZE));
  CHECK_EQUAL(1, bitset_fns(&set, -1));
}

TEST(Util_Bitset, BitsetFnz_AllSet) {
  bitset_set_all(&set);
  bitset_clr(&set, 10);

  CHECK_EQUAL(0, bitset_fnz(&set, 11));
}

TEST(Util_Bitset, BitsetFnz_NextIsZero) {
  bitset_set_all(&set);

  bitset_clr(&set, 10);
  bitset_clr(&set, 11);

  CHECK_EQUAL(11, bitset_fnz(&set, 10));
}

TEST(Util_Bitset, BitsetFnz_LastIsZero) {
  bitset_set_all(&set);

  bitset_clr(&set, 10);
  bitset_clr(&set, SET_SIZE - 1);

  CHECK_EQUAL(SET_SIZE, bitset_fnz(&set, 11));
}

TEST(Util_Bitset, BitsetFnz_OutOfBoundsIdx) {
  bitset_set_all(&set);
  bitset_clr(&set, 0);

  CHECK_EQUAL(0, bitset_fnz(&set, SET_SIZE));
  CHECK_EQUAL(1, bitset_fnz(&set, -1));
}

#endif  // !LELY_NO_MALLOC
