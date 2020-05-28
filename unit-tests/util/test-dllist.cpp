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

#include <lely/util/dllist.h>

TEST_GROUP(Util_Dllist) {
  static const size_t NODES_NUM = 10;
  dlnode nodes[NODES_NUM];
  dllist list;

  void AreNodesConnected(const dlnode* const node,
                         const dlnode* const next_node) {
    POINTERS_EQUAL(node->next, next_node);
    POINTERS_EQUAL(next_node->prev, node);
  }

  TEST_SETUP() {
    dllist_init(&list);
    for (size_t i = 0; i < NODES_NUM; i++) {
      dlnode_init(&nodes[i]);
    }
  }
};

TEST(Util_Dllist, DllistInit) {
  POINTERS_EQUAL(nullptr, dllist_first(&list));
  POINTERS_EQUAL(nullptr, dllist_last(&list));
}

TEST(Util_Dllist, DlnodeInit) {
  POINTERS_EQUAL(nullptr, nodes[0].next);
  POINTERS_EQUAL(nullptr, nodes[0].prev);
  POINTERS_EQUAL(nullptr, nodes[1].next);
  POINTERS_EQUAL(nullptr, nodes[1].prev);
  POINTERS_EQUAL(nullptr, nodes[NODES_NUM - 1].next);
  POINTERS_EQUAL(nullptr, nodes[NODES_NUM - 1].prev);
}

TEST(Util_Dllist, DllistEmpty_IsEmpty) { CHECK_EQUAL(1, dllist_empty(&list)); }

TEST(Util_Dllist, DllistEmpty_NotEmpty) {
  dllist_push_front(&list, &nodes[0]);

  CHECK_EQUAL(0, dllist_empty(&list));
}

TEST(Util_Dllist, DllistSize_IsEmpty) { CHECK_EQUAL(0, dllist_size(&list)); }

TEST(Util_Dllist, DllistSize_OneAdded) {
  dllist_push_front(&list, &nodes[0]);

  CHECK_EQUAL(1, dllist_size(&list));
}

TEST(Util_Dllist, DllistSize_ManyAdded) {
  dllist_push_front(&list, &nodes[0]);
  dllist_push_front(&list, &nodes[1]);

  CHECK_EQUAL(2, dllist_size(&list));
}

TEST(Util_Dllist, DllistPopFront_Empty) {
  POINTERS_EQUAL(nullptr, dllist_pop_front(&list));
  CHECK_EQUAL(0, dllist_size(&list));
}

TEST(Util_Dllist, DllistPopFront_OneAdded) {
  dllist_push_front(&list, &nodes[0]);

  POINTERS_EQUAL(&nodes[0], dllist_pop_front(&list));
  CHECK_EQUAL(0, dllist_size(&list));
}

TEST(Util_Dllist, DllistPopFront_ManyAdded) {
  dllist_push_front(&list, &nodes[0]);
  dllist_push_front(&list, &nodes[1]);

  POINTERS_EQUAL(&nodes[1], dllist_pop_front(&list));
  CHECK_EQUAL(1, dllist_size(&list));
}

TEST(Util_Dllist, DllistPopBack_Empty) {
  POINTERS_EQUAL(nullptr, dllist_pop_back(&list));
  CHECK_EQUAL(0, dllist_size(&list));
}

TEST(Util_Dllist, DllistPopBack_OneAdded) {
  dllist_push_front(&list, &nodes[0]);

  POINTERS_EQUAL(&nodes[0], dllist_pop_back(&list));
  CHECK_EQUAL(0, dllist_size(&list));
}

TEST(Util_Dllist, DllistPopBack_ManyAdded) {
  dllist_push_front(&list, &nodes[0]);
  dllist_push_front(&list, &nodes[1]);

  POINTERS_EQUAL(&nodes[0], dllist_pop_back(&list));
  CHECK_EQUAL(1, dllist_size(&list));
}

TEST(Util_Dllist, DllistInsertAfter_Middle) {
  dllist_push_back(&list, &nodes[0]);
  dllist_push_back(&list, &nodes[2]);

  dllist_insert_after(&list, &nodes[0], &nodes[1]);

  AreNodesConnected(&nodes[0], &nodes[1]);
  AreNodesConnected(&nodes[1], &nodes[2]);
  CHECK_EQUAL(3, dllist_size(&list));
}

TEST(Util_Dllist, DllistInsertAfter_Last) {
  dllist_push_back(&list, &nodes[0]);
  dllist_push_back(&list, &nodes[1]);

  dllist_insert_after(&list, &nodes[1], &nodes[2]);

  AreNodesConnected(&nodes[1], &nodes[2]);
  CHECK_EQUAL(3, dllist_size(&list));
}

TEST(Util_Dllist, DllistInsertBefore_Middle) {
  dllist_push_back(&list, &nodes[0]);
  dllist_push_back(&list, &nodes[2]);

  dllist_insert_before(&list, &nodes[2], &nodes[1]);

  AreNodesConnected(&nodes[0], &nodes[1]);
  AreNodesConnected(&nodes[1], &nodes[2]);
  CHECK_EQUAL(3, dllist_size(&list));
}

TEST(Util_Dllist, DllistInsertBefore_FirstNode) {
  dllist_push_back(&list, &nodes[1]);
  dllist_push_back(&list, &nodes[2]);

  dllist_insert_before(&list, &nodes[1], &nodes[0]);

  AreNodesConnected(&nodes[0], &nodes[1]);
  CHECK_EQUAL(3, dllist_size(&list));
}

TEST(Util_Dllist, DllistRemove) {
  dllist_push_back(&list, &nodes[0]);
  dllist_push_back(&list, &nodes[1]);

  dllist_remove(&list, &nodes[0]);

  POINTERS_EQUAL(&nodes[1], dllist_first(&list));
  CHECK_EQUAL(1, dllist_size(&list));
}

TEST(Util_Dllist, DllistAppend_SrcEmptyDstEmpty) {
  dllist src;
  dllist_init(&src);

  POINTERS_EQUAL(&list, dllist_append(&list, &src));

  CHECK_EQUAL(0, dllist_size(&list));
}

TEST(Util_Dllist, DllistAppend_SrcOneDstEmpty) {
  dllist src;
  dllist_init(&src);
  dllist_push_back(&src, &nodes[0]);

  POINTERS_EQUAL(&list, dllist_append(&list, &src));

  CHECK_EQUAL(1, dllist_size(&list));
  POINTERS_EQUAL(&nodes[0], dllist_first(&list));
}

TEST(Util_Dllist, DllistAppend_SrcManyDstEmpty) {
  dllist src;
  dllist_init(&src);
  dllist_push_back(&src, &nodes[0]);
  dllist_push_back(&src, &nodes[1]);

  POINTERS_EQUAL(&list, dllist_append(&list, &src));

  CHECK_EQUAL(2, dllist_size(&list));
  POINTERS_EQUAL(&nodes[0], dllist_first(&list));
}

TEST(Util_Dllist, DllistAppend_SrcManyDstMany) {
  dllist src;
  dllist_init(&src);

  dllist_push_back(&src, &nodes[0]);
  dllist_push_back(&src, &nodes[1]);
  dllist_push_back(&list, &nodes[2]);
  dllist_push_back(&list, &nodes[3]);

  POINTERS_EQUAL(&list, dllist_append(&list, &src));

  CHECK_EQUAL(4, dllist_size(&list));
  POINTERS_EQUAL(&nodes[2], dllist_first(&list));
}
