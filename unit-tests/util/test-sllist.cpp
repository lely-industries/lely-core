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

#include <vector>

#include <CppUTest/TestHarness.h>

#include <lely/util/sllist.h>

TEST_GROUP(Util_Sllist) {
  static const size_t NODES_NUM = 10;
  sllist list;
  slnode nodes[NODES_NUM];

  void FillList(sllist * list, const int how_many) {
    for (int i = 0; i < how_many; i++) {
      sllist_push_back(list, &nodes[i]);
    }
  }

  TEST_SETUP() {
    sllist_init(&list);
    for (size_t i = 0; i < NODES_NUM; i++) {
      slnode_init(&nodes[i]);
    }
  }
};

TEST(Util_Sllist, SllistInit) {
  POINTERS_EQUAL(nullptr, sllist_first(&list));
  POINTERS_EQUAL(nullptr, sllist_last(&list));
}

TEST(Util_Sllist, SlnodeInit) {
  POINTERS_EQUAL(nullptr, nodes[0].next);
  POINTERS_EQUAL(nullptr, nodes[1].next);
  POINTERS_EQUAL(nullptr, nodes[NODES_NUM - 1].next);
}

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
  FillList(&list, NODES_NUM);

  POINTERS_EQUAL(&nodes[0], sllist_pop_front(&list));
  POINTERS_EQUAL(&nodes[1], sllist_pop_front(&list));
  CHECK_EQUAL(NODES_NUM - 2, sllist_size(&list));
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

TEST(Util_Sllist, SllistAppend_SrcOneDstEmpty) {
  sllist source_list;
  sllist_init(&source_list);
  FillList(&source_list, 1);

  POINTERS_EQUAL(&list, sllist_append(&list, &source_list));
  CHECK_EQUAL(0, sllist_size(&source_list));
  CHECK_EQUAL(1, sllist_size(&list));
}

TEST(Util_Sllist, SllistAppend_SrcEmptyDstOne) {
  sllist source_list;
  sllist_init(&source_list);
  FillList(&list, 1);

  POINTERS_EQUAL(&list, sllist_append(&list, &source_list));
  CHECK_EQUAL(0, sllist_size(&source_list));
  CHECK_EQUAL(1, sllist_size(&list));
}

TEST(Util_Sllist, SllistAppend_SrcManyDstEmpty) {
  sllist source_list;
  sllist_init(&source_list);
  FillList(&source_list, 2);

  POINTERS_EQUAL(&list, sllist_append(&list, &source_list));
  CHECK_EQUAL(0, sllist_size(&source_list));
  CHECK_EQUAL(2, sllist_size(&list));
}

TEST(Util_Sllist, SllistAppend_SrcManyDstMany) {
  sllist source_list;
  sllist_init(&source_list);
  FillList(&source_list, 2);
  sllist_push_front(&list, &nodes[NODES_NUM - 1]);
  sllist_push_front(&list, &nodes[NODES_NUM - 2]);

  POINTERS_EQUAL(&list, sllist_append(&list, &source_list));
  CHECK_EQUAL(0, sllist_size(&source_list));
  CHECK_EQUAL(4, sllist_size(&list));
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
  bool visited = false;

  sllist_foreach(&list, node) visited = true;

  CHECK_EQUAL(false, visited);
}

TEST(Util_Sllist, SllistForeach_OnlyHead) {
  FillList(&list, 1);
  slnode* node_ptr = nullptr;
  int visited_nodes_counter = 0;

  sllist_foreach(&list, node) {
    node_ptr = node;
    visited_nodes_counter++;
  }

  POINTERS_EQUAL(&nodes[0], node_ptr);
  CHECK_EQUAL(1, visited_nodes_counter);
}

TEST(Util_Sllist, SllistForeach_MultipleElements) {
  FillList(&list, NODES_NUM);
  int visited_nodes_counter = 0;
  std::vector<void*> visited_nodes;

  sllist_foreach(&list, node) {
    visited_nodes.push_back(node);
    visited_nodes_counter++;
  }

  CHECK_EQUAL(NODES_NUM, visited_nodes_counter);
  CHECK_EQUAL(NODES_NUM, visited_nodes.size());
}

TEST(Util_Sllist, SllistForeach_MultiElementsRemoveCurrent) {
  FillList(&list, NODES_NUM);
  int visited_nodes_counter = 0;
  std::vector<void*> visited_nodes;

  sllist_foreach(&list, node) {
    if (visited_nodes_counter != 3) {
      visited_nodes.push_back(node);
    } else {
      sllist_remove(&list, node);
    }
    visited_nodes_counter++;
  }

  CHECK_EQUAL(NODES_NUM, visited_nodes_counter);
  CHECK_EQUAL(NODES_NUM - 1, visited_nodes.size());
}
