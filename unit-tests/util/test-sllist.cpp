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

TEST_GROUP(UtilSllistInit){};

TEST(UtilSllistInit, SllistInit) {
  sllist list;

  sllist_init(&list);

  CHECK(sllist_first(&list) == nullptr);
  CHECK(sllist_last(&list) == nullptr);
}

TEST(UtilSllistInit, SlnodeInit) {
  slnode node;

  slnode_init(&node);

  POINTERS_EQUAL(nullptr, node.next);
}

TEST_GROUP(UtilSllist) {
  static const size_t NODES_NUMBER = 10;
  sllist list;
  slnode nodes[NODES_NUMBER];

  void FillList(sllist * list, const int how_many) {
    for (int i = 0; i < how_many; i++) {
      slnode* node_ptr = &nodes[i];
      slnode_init(node_ptr);
      sllist_push_back(list, node_ptr);
    }
  }

  TEST_SETUP() {
    sllist_init(&list);

    for (size_t i = 0; i < NODES_NUMBER; i++) {
      slnode_init(&nodes[i]);
    }
  }
};

TEST(UtilSllist, SllistEmpty_AfterCreation) {
  CHECK_EQUAL(1, sllist_empty(&list));
}

TEST(UtilSllist, SllistEmpty_NotEmptyWhenElementAdded) {
  sllist_push_front(&list, &nodes[0]);

  CHECK_EQUAL(0, sllist_empty(&list));
}

TEST(UtilSllist, SllistEmpty_NotEmptyWhenManyElementsAdded) {
  FillList(&list, 3);

  CHECK_EQUAL(0, sllist_empty(&list));
}

TEST(UtilSllist, SllistSize_ZeroWhenCreated) {
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(UtilSllist, SllistSize_OneElementAdded) {
  sllist_push_front(&list, &nodes[0]);

  CHECK_EQUAL(1, sllist_size(&list));
}

TEST(UtilSllist, SllistSize_ManyAdded) {
  FillList(&list, 4);

  CHECK_EQUAL(4, sllist_size(&list));
}

TEST(UtilSllist, SllistPushFront_WhenEmpty) {
  slnode* node_ptr = &nodes[0];

  sllist_push_front(&list, node_ptr);

  POINTERS_EQUAL(node_ptr, sllist_first(&list));
}

TEST(UtilSllist, SllistPushFront_AddMany) {
  sllist_push_front(&list, &nodes[0]);
  sllist_push_front(&list, &nodes[1]);

  POINTERS_EQUAL(&nodes[1], sllist_first(&list));
}

TEST(UtilSllist, SllistPushBack_WhenEmpty) {
  sllist_push_back(&list, &nodes[0]);

  POINTERS_EQUAL(&nodes[0], list.first);
  CHECK_EQUAL(1, sllist_size(&list));
}

TEST(UtilSllist, SllistPushBack_AddMany) {
  sllist_push_back(&list, &nodes[0]);
  sllist_push_back(&list, &nodes[1]);

  POINTERS_EQUAL(&nodes[0], sllist_first(&list));
  POINTERS_EQUAL(&nodes[1], sllist_pop_back(&list));
  CHECK_EQUAL(1, sllist_size(&list));
}

TEST(UtilSllist, SllistPopFront_WhenEmpty) {
  POINTERS_EQUAL(nullptr, sllist_pop_front(&list));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(UtilSllist, SllistPopFront_OneAdded) {
  FillList(&list, 1);

  POINTERS_EQUAL(&nodes[0], sllist_pop_front(&list));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(UtilSllist, SllistPopFront_ManyAdded) {
  FillList(&list, 8);

  POINTERS_EQUAL(&nodes[0], sllist_pop_front(&list));
  POINTERS_EQUAL(&nodes[1], sllist_pop_front(&list));
  CHECK_EQUAL(6, sllist_size(&list));
}

TEST(UtilSllist, SllistPopBack_WhenEmpty) {
  POINTERS_EQUAL(nullptr, sllist_pop_back(&list));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(UtilSllist, SllistPopBack_OneAdded) {
  FillList(&list, 1);

  POINTERS_EQUAL(&nodes[0], sllist_pop_back(&list));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(UtilSllist, SllistPopBack_ManyAdded) {
  FillList(&list, 8);

  POINTERS_EQUAL(&nodes[7], sllist_pop_back(&list));
  POINTERS_EQUAL(&nodes[6], sllist_pop_back(&list));
  CHECK_EQUAL(6, sllist_size(&list));
}

TEST(UtilSllist, SllistRemove_Nullptr) {
  POINTERS_EQUAL(nullptr, sllist_remove(&list, nullptr));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(UtilSllist, SllistRemove_Empty) {
  POINTERS_EQUAL(nullptr, sllist_remove(&list, &nodes[0]));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(UtilSllist, SllistRemove_OneAdded) {
  FillList(&list, 1);

  POINTERS_EQUAL(&nodes[0], sllist_remove(&list, &nodes[0]));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(UtilSllist, SllistRemove_OneAddedRemovedTwice) {
  FillList(&list, 1);

  sllist_remove(&list, &nodes[0]);

  POINTERS_EQUAL(nullptr, sllist_remove(&list, &nodes[0]));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(UtilSllist, SllistRemove_OneAddedRemovedNullptr) {
  FillList(&list, 1);

  POINTERS_EQUAL(nullptr, sllist_remove(&list, nullptr));
  CHECK_EQUAL(1, sllist_size(&list));
}

TEST(UtilSllist, SllistRemove_ManyAdded) {
  FillList(&list, 2);

  POINTERS_EQUAL(&nodes[0], sllist_remove(&list, &nodes[0]));
  POINTERS_EQUAL(&nodes[1], sllist_remove(&list, &nodes[1]));
  CHECK_EQUAL(0, sllist_size(&list));
}

TEST(UtilSllist, SllistAppend_BothEmpty) {
  sllist source_list;
  sllist_init(&source_list);

  POINTERS_EQUAL(&list, sllist_append(&list, &source_list));
  CHECK_EQUAL(0, sllist_size(&source_list));
}

TEST(UtilSllist, SllistAppend_DstEmpty) {
  sllist source_list;
  sllist_init(&source_list);
  FillList(&source_list, 1);

  POINTERS_EQUAL(&list, sllist_append(&list, &source_list));
  CHECK_EQUAL(0, sllist_size(&source_list));
  CHECK_EQUAL(1, sllist_size(&list));
}

TEST(UtilSllist, SllistAppend_SrcEmpty) {
  sllist source_list;
  sllist_init(&source_list);
  FillList(&list, 1);

  POINTERS_EQUAL(&list, sllist_append(&list, &source_list));
  CHECK_EQUAL(0, sllist_size(&source_list));
  CHECK_EQUAL(1, sllist_size(&list));
}

TEST(UtilSllist, SllistAppend_SrcMany) {
  sllist source_list;
  sllist_init(&source_list);
  FillList(&source_list, 2);

  POINTERS_EQUAL(&list, sllist_append(&list, &source_list));
  CHECK_EQUAL(0, sllist_size(&source_list));
  CHECK_EQUAL(2, sllist_size(&list));
}

TEST(UtilSllist, SllistFirst_Empty) {
  POINTERS_EQUAL(nullptr, sllist_first(&list));
}

TEST(UtilSllist, SllistFirst_OneAdded) {
  FillList(&list, 1);

  POINTERS_EQUAL(&nodes[0], sllist_first(&list));
}

TEST(UtilSllist, SllistFirst_ManyAdded) {
  FillList(&list, 2);

  POINTERS_EQUAL(&nodes[0], sllist_first(&list));
}

TEST(UtilSllist, SllistLast_Empty) {
  POINTERS_EQUAL(nullptr, sllist_last(&list));
}

TEST(UtilSllist, SllistLast_OneAdded) {
  FillList(&list, 1);

  POINTERS_EQUAL(&nodes[0], sllist_last(&list));
}

TEST(UtilSllist, SllistLast_ManyAdded) {
  FillList(&list, 2);

  POINTERS_EQUAL(&nodes[1], sllist_last(&list));
}

TEST(UtilSllist, SllistForeach_Empty) {
  slnode* node_ptr = &nodes[0];

  sllist_foreach(&list, node) node_ptr = node;

  POINTERS_EQUAL(&nodes[0], node_ptr);
}

TEST(UtilSllist, SllistForeach_OnlyHead) {
  FillList(&list, 1);

  slnode* node_ptr = nullptr;
  sllist_foreach(&list, node) node_ptr = node;

  POINTERS_EQUAL(&nodes[0], node_ptr);
}

TEST(UtilSllist, SllistForeach_MultipleElements) {
  FillList(&list, 2);

  slnode* node_ptr = nullptr;
  sllist_foreach(&list, node) node_ptr = node;

  POINTERS_EQUAL(&nodes[1], node_ptr);
}
