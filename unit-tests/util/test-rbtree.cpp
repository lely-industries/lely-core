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

#include <cassert>

#include <CppUTest/TestHarness.h>

#include <lely/util/rbtree.h>

static int
rbtree_cmp_ints(const void* p1, const void* p2) {
  assert(p1);
  assert(p2);

  const auto val1 = *static_cast<const int*>(p1);
  const auto val2 = *static_cast<const int*>(p2);

  if (val1 > val2)
    return 1;
  else if (val1 == val2)
    return 0;
  else
    return -1;
}

TEST_GROUP(Util_RbtreeCmpInts){};

TEST(Util_RbtreeCmpInts, RbtreeCmpInts) {
  int a = 2;
  int b = 3;
  int c = 2;

  CHECK_EQUAL(0, rbtree_cmp_ints(&a, &c));
  CHECK_COMPARE(0, >, rbtree_cmp_ints(&a, &b));
  CHECK_COMPARE(0, <, rbtree_cmp_ints(&b, &a));
}

TEST_GROUP(Util_Rbtree) {
  rbtree tree;
  static const size_t NODES_NUMBER = 10;
  rbnode nodes[NODES_NUMBER];
  const int keys[NODES_NUMBER] = {-123, 233,  342,  343,  500,
                                  543,  1000, 3255, 4352, 5142};

  TEST_SETUP() {
    rbtree_init(&tree, rbtree_cmp_ints);
    for (size_t i = 0; i < NODES_NUMBER; i++) {
      rbnode_init(&nodes[i], &keys[i]);
    }
  }
};

TEST(Util_Rbtree, RbtreeInit) {
  CHECK_EQUAL(0, rbtree_size(&tree));
  POINTERS_EQUAL(nullptr, rbtree_root(&tree));
}

TEST(Util_Rbtree, RbtreeEmpty_IsEmpty) { CHECK_EQUAL(1, rbtree_empty(&tree)); }

TEST(Util_Rbtree, RbtreeEmpty_RootAdded) {
  rbtree_insert(&tree, &nodes[0]);

  CHECK_EQUAL(0, rbtree_empty(&tree));
}

TEST(Util_Rbtree, RbtreeEmpty_LeaveAdded) {
  rbtree_insert(&tree, &nodes[0]);
  rbtree_insert(&tree, &nodes[1]);

  CHECK_EQUAL(0, rbtree_empty(&tree));
}

TEST(Util_Rbtree, RbtreeInsert_Empty) {
  rbtree_insert(&tree, &nodes[0]);

  rbnode* root_ptr = rbtree_root(&tree);
  POINTERS_EQUAL(&nodes[0], root_ptr);
  POINTERS_EQUAL(nullptr, rbnode_next(root_ptr));
}

TEST(Util_Rbtree, RbnodeInsert_OneAdded) {
  rbtree_insert(&tree, &nodes[0]);

  rbtree_insert(&tree, &nodes[1]);

  rbnode* root_ptr = rbtree_root(&tree);
  POINTERS_EQUAL(&nodes[1], rbnode_next(root_ptr));
}

TEST(Util_Rbtree, RbnodeInsert_ManyAdded) {
  rbtree_insert(&tree, &nodes[0]);
  rbtree_insert(&tree, &nodes[1]);

  rbtree_insert(&tree, &nodes[2]);

  rbnode* root_ptr = rbtree_root(&tree);
  POINTERS_EQUAL(&nodes[2], rbnode_next(root_ptr));
}

TEST(Util_Rbtree, RbnodeInsert_ManyAddedRedAndBlackNodes) {
  rbtree_insert(&tree, &nodes[0]);
  rbtree_insert(&tree, &nodes[1]);
  rbtree_insert(&tree, &nodes[2]);
  rbtree_insert(&tree, &nodes[3]);
  rbtree_insert(&tree, &nodes[4]);

  rbtree_insert(&tree, &nodes[5]);

  POINTERS_EQUAL(&nodes[1], rbtree_root(&tree));
  CHECK_EQUAL(6U, rbtree_size(&tree));
}

TEST(Util_Rbtree, RbnodeInsert_NodeHasRedUncle) {
  rbtree_insert(&tree, &nodes[3]);
  rbtree_insert(&tree, &nodes[2]);
  rbtree_insert(&tree, &nodes[1]);
  rbtree_insert(&tree, &nodes[0]);

  POINTERS_EQUAL(&nodes[2], rbtree_root(&tree));
  CHECK_EQUAL(4U, rbtree_size(&tree));
}

TEST(Util_Rbtree, RbnodeInsert_RightRotateAtGrandparent) {
  rbtree_insert(&tree, &nodes[0]);
  rbtree_insert(&tree, &nodes[1]);
  rbtree_insert(&tree, &nodes[2]);
  rbtree_insert(&tree, &nodes[3]);
  rbtree_insert(&tree, &nodes[4]);
  rbtree_insert(&tree, &nodes[6]);
  rbtree_insert(&tree, &nodes[5]);

  POINTERS_EQUAL(&nodes[1], rbtree_root(&tree));
  CHECK_EQUAL(7U, rbtree_size(&tree));
}

TEST(Util_Rbtree, RbnodePrev_NodeIsRoot) {
  rbtree_insert(&tree, &nodes[0]);

  POINTERS_EQUAL(nullptr, rbnode_prev(tree.root));
}

TEST(Util_Rbtree, RbnodePrev_NodeHasLeftSubtree) {
  rbtree_insert(&tree, &nodes[3]);
  rbtree_insert(&tree, &nodes[2]);
  rbtree_insert(&tree, &nodes[1]);

  POINTERS_EQUAL(&nodes[1], rbnode_prev(&nodes[2]));
  POINTERS_EQUAL(&nodes[2], rbnode_prev(&nodes[3]));
}

TEST(Util_Rbtree, RbnodePrev_NodeHasRightSubtree) {
  rbtree_insert(&tree, &nodes[4]);
  rbtree_insert(&tree, &nodes[2]);
  rbtree_insert(&tree, &nodes[3]);

  POINTERS_EQUAL(nullptr, rbnode_prev(&nodes[2]));
  POINTERS_EQUAL(&nodes[2], rbnode_prev(&nodes[3]));
}

TEST(Util_Rbtree, RbtreeRemove_OnlyHead) {
  rbtree_insert(&tree, &nodes[0]);

  rbtree_remove(&tree, &nodes[0]);

  POINTERS_EQUAL(nullptr, rbtree_first(&tree));
}

TEST(Util_Rbtree, RbtreeRemove_HeadOneAdded) {
  rbtree_insert(&tree, &nodes[0]);
  rbtree_insert(&tree, &nodes[1]);

  rbtree_remove(&tree, &nodes[0]);

  POINTERS_EQUAL(&nodes[1], rbtree_first(&tree));
}

TEST(Util_Rbtree, RbtreeRemove_ElementOneAdded) {
  rbtree_insert(&tree, &nodes[0]);
  rbtree_insert(&tree, &nodes[1]);

  rbtree_remove(&tree, &nodes[1]);

  POINTERS_EQUAL(&nodes[0], rbtree_first(&tree));
}

TEST(Util_Rbtree, RbtreeRemove_BothLeftAndRightSubtree) {
  rbtree_insert(&tree, &nodes[0]);
  rbtree_insert(&tree, &nodes[1]);
  rbtree_insert(&tree, &nodes[2]);
  rbtree_insert(&tree, &nodes[3]);
  rbtree_insert(&tree, &nodes[4]);
  rbtree_insert(&tree, &nodes[6]);
  rbtree_insert(&tree, &nodes[5]);

  rbtree_remove(&tree, &nodes[3]);
  rbtree_remove(&tree, &nodes[5]);
  rbtree_remove(&tree, &nodes[0]);
  rbtree_remove(&tree, &nodes[6]);
  rbtree_remove(&tree, &nodes[4]);
  rbtree_remove(&tree, &nodes[2]);
  rbtree_remove(&tree, &nodes[1]);

  POINTERS_EQUAL(nullptr, rbtree_root(&tree));
  CHECK_EQUAL(0U, rbtree_size(&tree));
}

TEST(Util_Rbtree, RbtreeRemove_NextIsNode) {
  rbtree_insert(&tree, &nodes[0]);
  rbtree_insert(&tree, &nodes[1]);
  rbtree_insert(&tree, &nodes[2]);

  rbtree_remove(&tree, &nodes[1]);

  POINTERS_EQUAL(&nodes[2], rbtree_root(&tree));
  CHECK_EQUAL(2U, rbtree_size(&tree));
}

TEST(Util_Rbtree, RbtreeRemove_FixViolations) {
  rbtree_insert(&tree, &nodes[1]);
  rbtree_insert(&tree, &nodes[0]);
  rbtree_insert(&tree, &nodes[9]);
  rbtree_insert(&tree, &nodes[8]);
  rbtree_insert(&tree, &nodes[2]);
  rbtree_insert(&tree, &nodes[4]);
  rbtree_insert(&tree, &nodes[3]);
  rbtree_insert(&tree, &nodes[5]);
  rbtree_insert(&tree, &nodes[7]);
  rbtree_insert(&tree, &nodes[6]);

  rbtree_remove(&tree, &nodes[9]);

  POINTERS_EQUAL(&nodes[3], rbtree_root(&tree));
  CHECK_EQUAL(9U, rbtree_size(&tree));
}

TEST(Util_Rbtree, RbtreeLast_EmptyTree) {
  POINTERS_EQUAL(nullptr, rbtree_last(&tree));
}

TEST(Util_Rbtree, RbtreeLast_OnlyRoot) {
  rbtree_insert(&tree, &nodes[0]);

  POINTERS_EQUAL(&nodes[0], rbtree_last(&tree));
}

TEST(Util_Rbtree, RbtreeLast_ElementAddedRightSubtree) {
  rbtree_insert(&tree, &nodes[0]);
  rbtree_insert(&tree, &nodes[1]);

  POINTERS_EQUAL(&nodes[1], rbtree_last(&tree));
}

TEST(Util_Rbtree, RbtreeLast_ElementAddedLeftSubtree) {
  rbtree_insert(&tree, &nodes[1]);
  rbtree_insert(&tree, &nodes[0]);

  POINTERS_EQUAL(&nodes[1], rbtree_last(&tree));
}

TEST(Util_Rbtree, RbtreeLast_RemovedHead) {
  rbtree_insert(&tree, &nodes[1]);
  rbtree_insert(&tree, &nodes[0]);

  rbtree_remove(&tree, &nodes[1]);

  POINTERS_EQUAL(&nodes[0], rbtree_last(&tree));
}

TEST(Util_Rbtree, RbtreeLast_RemovedLastAddedHead) {
  rbtree_insert(&tree, &nodes[1]);
  rbtree_insert(&tree, &nodes[0]);

  rbtree_remove(&tree, &nodes[0]);

  POINTERS_EQUAL(&nodes[1], rbtree_last(&tree));
}
