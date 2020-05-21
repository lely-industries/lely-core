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

#include <lely/util/sllist.h>

TEST_GROUP(Util_SllistInit){};

TEST(Util_SllistInit, SllistInit) {
  sllist list;

  sllist_init(&list);

  POINTERS_EQUAL(nullptr, sllist_first(&list));
  POINTERS_EQUAL(nullptr, sllist_last(&list));
}

TEST(Util_SllistInit, SlnodeInit) {
  slnode node;

  slnode_init(&node);

  POINTERS_EQUAL(nullptr, node.next);
}

TEST_GROUP(Util_Sllist) {
  static const size_t NODES_NUMBER = 10;
  sllist list;
  slnode nodes[NODES_NUMBER];

  void FillList(sllist * list, const int how_many) {
    for (int i = 0; i < how_many; i++) {
      sllist_push_back(list, &nodes[i]);
    }
  }

  TEST_SETUP() {
    sllist_init(&list);
    for (size_t i = 0; i < NODES_NUMBER; i++) {
      slnode_init(&nodes[i]);
    }
  }
};

TEST(Util_Sllist, SllistEmpty_AfterCreation) {
  CHECK_EQUAL(1, sllist_empty(&list));
}

TEST(Util_Sllist, SllistEmpty_NotEmptyWhenElementAdded) {
  sllist_push_front(&list, &nodes[0]);

  CHECK_EQUAL(0, sllist_empty(&list));
}

TEST(Util_Sllist, SllistEmpty_NotEmptyWhenManyElementsAdded) {
  FillList(&list, 3);

  CHECK_EQUAL(0, sllist_empty(&list));
}

TEST(Util_Sllist, SllistSize_ZeroWhenCreated) {
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(Util_Sllist, SllistSize_OneElementAdded) {
  sllist_push_front(&list, &nodes[0]);

  CHECK_EQUAL(1, sllist_size(&list));
}

TEST(Util_Sllist, SllistSize_ManyAdded) {
  FillList(&list, 4);

  CHECK_EQUAL(4, sllist_size(&list));
}

TEST(Util_Sllist, SllistPushFront_WhenEmpty) {
  sllist_push_front(&list, &nodes[0]);

  POINTERS_EQUAL(&nodes[0], sllist_first(&list));
}

TEST(Util_Sllist, SllistPushFront_AddMany) {
  sllist_push_front(&list, &nodes[0]);
  sllist_push_front(&list, &nodes[1]);

  POINTERS_EQUAL(&nodes[1], sllist_first(&list));
}

TEST(Util_Sllist, SllistPushBack_WhenEmpty) {
  sllist_push_back(&list, &nodes[0]);

  POINTERS_EQUAL(&nodes[0], sllist_first(&list));
  CHECK_EQUAL(1, sllist_size(&list));
}

TEST(Util_Sllist, SllistPushBack_AddMany) {
  sllist_push_back(&list, &nodes[0]);
  sllist_push_back(&list, &nodes[1]);

  POINTERS_EQUAL(&nodes[0], sllist_first(&list));
  POINTERS_EQUAL(&nodes[1], sllist_pop_back(&list));
  CHECK_EQUAL(1, sllist_size(&list));
}

TEST(Util_Sllist, SllistPopFront_WhenEmpty) {
  POINTERS_EQUAL(nullptr, sllist_pop_front(&list));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(Util_Sllist, SllistPopFront_OneAdded) {
  FillList(&list, 1);

  POINTERS_EQUAL(&nodes[0], sllist_pop_front(&list));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(Util_Sllist, SllistPopFront_ManyAdded) {
  FillList(&list, 8);

  POINTERS_EQUAL(&nodes[0], sllist_pop_front(&list));
  POINTERS_EQUAL(&nodes[1], sllist_pop_front(&list));
  CHECK_EQUAL(6, sllist_size(&list));
}

TEST(Util_Sllist, SllistPopBack_WhenEmpty) {
  POINTERS_EQUAL(nullptr, sllist_pop_back(&list));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(Util_Sllist, SllistPopBack_OneAdded) {
  FillList(&list, 1);

  POINTERS_EQUAL(&nodes[0], sllist_pop_back(&list));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(Util_Sllist, SllistPopBack_ManyAdded) {
  FillList(&list, 8);

  POINTERS_EQUAL(&nodes[7], sllist_pop_back(&list));
  POINTERS_EQUAL(&nodes[6], sllist_pop_back(&list));
  CHECK_EQUAL(6, sllist_size(&list));
}

TEST(Util_Sllist, SllistRemove_Nullptr) {
  POINTERS_EQUAL(nullptr, sllist_remove(&list, nullptr));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(Util_Sllist, SllistRemove_Empty) {
  POINTERS_EQUAL(nullptr, sllist_remove(&list, &nodes[0]));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(Util_Sllist, SllistRemove_OneAdded) {
  FillList(&list, 1);

  POINTERS_EQUAL(&nodes[0], sllist_remove(&list, &nodes[0]));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(Util_Sllist, SllistRemove_OneAddedRemovedTwice) {
  FillList(&list, 1);

  sllist_remove(&list, &nodes[0]);

  POINTERS_EQUAL(nullptr, sllist_remove(&list, &nodes[0]));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(Util_Sllist, SllistRemove_OneAddedRemovedNullptr) {
  FillList(&list, 1);

  POINTERS_EQUAL(nullptr, sllist_remove(&list, nullptr));
  CHECK_EQUAL(1, sllist_size(&list));
}

TEST(Util_Sllist, SllistRemove_ManyAdded) {
  FillList(&list, 2);

  POINTERS_EQUAL(&nodes[0], sllist_remove(&list, &nodes[0]));
  POINTERS_EQUAL(&nodes[1], sllist_remove(&list, &nodes[1]));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(Util_Sllist, SllistAppend_BothEmpty) {
  sllist source_list;
  sllist_init(&source_list);

  POINTERS_EQUAL(&list, sllist_append(&list, &source_list));
  CHECK_EQUAL(0, sllist_size(&source_list));
}

TEST(Util_Sllist, SllistAppend_DstEmpty) {
  sllist source_list;
  sllist_init(&source_list);
  FillList(&source_list, 1);

  POINTERS_EQUAL(&list, sllist_append(&list, &source_list));
  CHECK_EQUAL(0, sllist_size(&source_list));
  CHECK_EQUAL(1, sllist_size(&list));
}

TEST(Util_Sllist, SllistAppend_SrcEmpty) {
  sllist source_list;
  sllist_init(&source_list);
  FillList(&list, 1);

  POINTERS_EQUAL(&list, sllist_append(&list, &source_list));
  CHECK_EQUAL(0, sllist_size(&source_list));
  CHECK_EQUAL(1, sllist_size(&list));
}

TEST(Util_Sllist, SllistAppend_SrcMany) {
  sllist source_list;
  sllist_init(&source_list);
  FillList(&source_list, 2);

  POINTERS_EQUAL(&list, sllist_append(&list, &source_list));
  CHECK_EQUAL(0, sllist_size(&source_list));
  CHECK_EQUAL(2, sllist_size(&list));
}

TEST(Util_Sllist, SllistFirst_Empty) {
  POINTERS_EQUAL(nullptr, sllist_first(&list));
}

TEST(Util_Sllist, SllistFirst_OneAdded) {
  FillList(&list, 1);

  POINTERS_EQUAL(&nodes[0], sllist_first(&list));
}

TEST(Util_Sllist, SllistFirst_ManyAdded) {
  FillList(&list, 2);

  POINTERS_EQUAL(&nodes[0], sllist_first(&list));
}

TEST(Util_Sllist, SllistLast_Empty) {
  POINTERS_EQUAL(nullptr, sllist_last(&list));
}

TEST(Util_Sllist, SllistLast_OneAdded) {
  FillList(&list, 1);

  POINTERS_EQUAL(&nodes[0], sllist_last(&list));
}

TEST(Util_Sllist, SllistLast_ManyAdded) {
  FillList(&list, 2);

  POINTERS_EQUAL(&nodes[1], sllist_last(&list));
}

TEST(Util_Sllist, SllistForeach_Empty) {
  slnode* node_ptr = &nodes[0];

  sllist_foreach(&list, node) node_ptr = node;

  POINTERS_EQUAL(&nodes[0], node_ptr);
}

TEST(Util_Sllist, SllistForeach_OnlyHead) {
  FillList(&list, 1);
  slnode* node_ptr = nullptr;

  sllist_foreach(&list, node) node_ptr = node;

  POINTERS_EQUAL(&nodes[0], node_ptr);
}

TEST(Util_Sllist, SllistForeach_MultipleElements) {
  FillList(&list, 2);
  slnode* node_ptr = nullptr;

  sllist_foreach(&list, node) node_ptr = node;

  POINTERS_EQUAL(&nodes[1], node_ptr);
}
