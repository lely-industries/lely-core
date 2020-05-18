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

#include <CppUTest/TestHarness.h>
#include <lely/util/bitset.h>
#include <limits.h>
#include <string>

TEST_GROUP(UtilBitset) {
  bitset set;
  const unsigned int set_size = 43;

  static void CheckAllStates(const bitset* const set,
                             const int expected_state) {
    for (int i = 0; i < bitset_size(set); i++) {
      const std::string msg_str =
          "testing bitset_test(set, " + std::to_string(i) + ")";
      CHECK_EQUAL_TEXT(expected_state, bitset_test(set, i), msg_str.c_str());
    }
  }

  TEST_SETUP() {
    bitset_init(&set, set_size);
    bitset_clr_all(&set);
  }
  TEST_TEARDOWN() { bitset_fini(&set); }
};

IGNORE_TEST(UtilBitset, BitsetInit) {
  // TODO(N7S)
}

IGNORE_TEST(UtilBitset, BitsetFini) {
  // TODO(N7S)
}

IGNORE_TEST(UtilBitset, BitsetResize) {
  // TODO(N7S)
}

TEST(UtilBitset, BitsetSize) {
  int size_in_bits = set.size * sizeof(int) * CHAR_BIT;

  CHECK_EQUAL(size_in_bits, bitset_size(&set));
}

TEST(UtilBitset, BitsetTest) {
  bitset_set(&set, 1);
  bitset_set(&set, 5);

  CHECK_EQUAL(1, bitset_test(&set, 1));
  CHECK_EQUAL(1, bitset_test(&set, 5));
}

TEST(UtilBitset, BitsetTest_OutOfBoundsReturnsZero) {
  bitset_set_all(&set);

  CHECK_EQUAL(0, bitset_test(&set, -1));
  CHECK_EQUAL(0, bitset_test(&set, bitset_size(&set)));
  CHECK_EQUAL(0, bitset_test(&set, bitset_size(&set) + 1));
}

TEST(UtilBitset, BitsetSet_CorrectIndex) {
  const int first_idx = 0;
  const int mid_idx = 42;
  bitset_set(&set, first_idx);
  bitset_set(&set, mid_idx);

  CHECK_EQUAL(1, bitset_test(&set, first_idx));
  CHECK_EQUAL(1, bitset_test(&set, mid_idx));
}

TEST(UtilBitset, BitsetSet_OutOfBoundsIndex) {
  bitset_set(&set, -1);
  bitset_set(&set, bitset_size(&set));
  bitset_set(&set, bitset_size(&set) + 1);

  CheckAllStates(&set, 0);
}

TEST(UtilBitset, BitsetSetAll) {
  bitset_set_all(&set);

  CheckAllStates(&set, 1);
}

TEST(UtilBitset, BitsetClr_CorrectIndex) {
  const int last_idx = bitset_size(&set) - 1;

  bitset_set_all(&set);
  bitset_clr(&set, 0);
  bitset_clr(&set, 1);
  bitset_clr(&set, last_idx);

  CHECK_EQUAL(0, bitset_test(&set, 0));
  CHECK_EQUAL(0, bitset_test(&set, 1));
  CHECK_EQUAL(0, bitset_test(&set, last_idx));
}

TEST(UtilBitset, BitsetClr_OutOfBoundsIndex) {
  bitset_set_all(&set);
  bitset_clr(&set, -1);
  bitset_clr(&set, bitset_size(&set));
  bitset_clr(&set, bitset_size(&set) + 1);

  CheckAllStates(&set, 1);
}

TEST(UtilBitset, BitsetClrAll) {
  bitset_clr_all(&set);

  CheckAllStates(&set, 0);
}

TEST(UtilBitset, BitsetCompl) {
  bitset_clr_all(&set);
  bitset_set(&set, 0);
  bitset_set(&set, 1);

  bitset_compl(&set);

  CHECK_EQUAL(0, bitset_test(&set, 0));
  CHECK_EQUAL(0, bitset_test(&set, 1));
  CHECK_EQUAL(1, bitset_test(&set, 2));
  CHECK_EQUAL(1, bitset_test(&set, bitset_size(&set) - 3));
  CHECK_EQUAL(1, bitset_test(&set, bitset_size(&set) - 2));
  CHECK_EQUAL(1, bitset_test(&set, bitset_size(&set) - 1));
}

TEST(UtilBitset, BitsetFfs_AllZero) {
  bitset_clr_all(&set);

  CHECK_EQUAL(0, bitset_ffs(&set));
}

TEST(UtilBitset, BitsetFfs_FirstSet) {
  bitset_clr_all(&set);

  bitset_set(&set, 0);

  CHECK_EQUAL(1, bitset_ffs(&set));
}

TEST(UtilBitset, BitsetFfs_LastSet) {
  const int last_idx = bitset_size(&set) - 1;
  bitset_clr_all(&set);

  bitset_set(&set, last_idx);

  CHECK_EQUAL(last_idx + 1, bitset_ffs(&set));
}

TEST(UtilBitset, BitsetFfs_MiddleSet) {
  const int middle_idx = 42;
  bitset_clr_all(&set);

  bitset_set(&set, middle_idx);
  if (middle_idx + 4 < bitset_size(&set)) bitset_set(&set, middle_idx + 4);

  CHECK_EQUAL(middle_idx + 1, bitset_ffs(&set));
}

TEST(UtilBitset, BitsetFfz_AllSet) {
  bitset_set_all(&set);

  CHECK_EQUAL(0, bitset_ffz(&set));
}

TEST(UtilBitset, BitsetFfz_FirstZero) {
  bitset_set_all(&set);
  bitset_clr(&set, 0);

  CHECK_EQUAL(1, bitset_ffz(&set));
}

TEST(UtilBitset, BitsetFfz_LastZero) {
  const int last_idx = bitset_size(&set) - 1;
  bitset_set_all(&set);

  bitset_clr(&set, last_idx);

  CHECK_EQUAL(last_idx + 1, bitset_ffz(&set));
}

TEST(UtilBitset, BitsetFfz_MiddleZero) {
  const int middle_idx = 42;
  bitset_set_all(&set);

  bitset_clr(&set, middle_idx);
  if (middle_idx + 4 < bitset_size(&set)) bitset_clr(&set, middle_idx + 4);

  CHECK_EQUAL(middle_idx + 1, bitset_ffz(&set));
}

TEST(UtilBitset, BitsetFns_AllZeros) {
  const int set_idx = 10;

  bitset_clr_all(&set);
  bitset_set(&set, set_idx);

  CHECK_EQUAL(0, bitset_fns(&set, set_idx + 1));
}

TEST(UtilBitset, BitsetFns_NextIsSet) {
  const int set_idx = 10;
  bitset_clr_all(&set);

  bitset_set(&set, set_idx);
  bitset_set(&set, set_idx + 1);

  CHECK_EQUAL(set_idx + 2, bitset_fns(&set, set_idx + 1));
}

TEST(UtilBitset, BitsetFns_LastIsSet) {
  const int set_idx = 10;
  const int last_idx = bitset_size(&set) - 1;
  bitset_clr_all(&set);

  bitset_set(&set, set_idx);
  bitset_set(&set, last_idx);

  CHECK_EQUAL(last_idx + 1, bitset_fns(&set, set_idx + 1));
}

TEST(UtilBitset, BitsetFns_OutOfBoundsIdx) {
  const int last_idx = bitset_size(&set) - 1;
  const int idx_to_set = 0;
  bitset_set(&set, idx_to_set);

  CHECK_EQUAL(0, bitset_fns(&set, last_idx + 1));
  CHECK_EQUAL(idx_to_set + 1, bitset_fns(&set, -1));
}

TEST(UtilBitset, BitsetFnz_AllSet) {
  const int set_idx = 10;
  bitset_set_all(&set);
  bitset_clr(&set, set_idx);

  CHECK_EQUAL(0, bitset_fnz(&set, set_idx + 1));
}

TEST(UtilBitset, BitsetFnz_NextIsZero) {
  const int set_idx = 10;
  bitset_set_all(&set);

  bitset_clr(&set, set_idx);
  bitset_clr(&set, set_idx + 1);

  CHECK_EQUAL(set_idx + 2, bitset_fnz(&set, set_idx + 1));
}

TEST(UtilBitset, BitsetFnz_LastIsZero) {
  const int set_idx = 10;
  const int last_idx = bitset_size(&set) - 1;
  bitset_set_all(&set);

  bitset_clr(&set, set_idx);
  bitset_clr(&set, last_idx);

  CHECK_EQUAL(last_idx + 1, bitset_fnz(&set, set_idx + 1));
}

TEST(UtilBitset, BitsetFnz_OutOfBoundsIdx) {
  const int last_idx = bitset_size(&set) - 1;
  const int idx_to_clr = 0;

  bitset_set_all(&set);
  bitset_clr(&set, idx_to_clr);

  CHECK_EQUAL(0, bitset_fnz(&set, last_idx + 1));
  CHECK_EQUAL(idx_to_clr + 1, bitset_fnz(&set, -1));
}
