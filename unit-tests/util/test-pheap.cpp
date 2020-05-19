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
#include <lely/util/pheap.h>
#include <assert.h>

static int
pheap_cmp_ints(const void* p1, const void* p2) {
  assert(p1);
  assert(p2);

  if (*static_cast<const int*>(p1) > *static_cast<const int*>(p2))
    return 1;
  else if (*static_cast<const int*>(p1) == *static_cast<const int*>(p2))
    return 0;
  else
    return -1;
}

TEST_GROUP(UtilPheapInit){};

TEST(UtilPheapInit, Pheap_EmptyWhenInitialized) {
  pheap heap;
  pheap_init(&heap, pheap_cmp_ints);

  CHECK_EQUAL(0, heap.num_nodes);
  FUNCTIONPOINTERS_EQUAL(pheap_cmp_ints, heap.cmp);
  POINTERS_EQUAL(nullptr, heap.root);
}

TEST_GROUP(UtilPheap) {
  pheap heap;
  static const size_t NODES_NUMBER = 10;

  pnode nodes[NODES_NUMBER];
  // clang-format off
  int keys[NODES_NUMBER] = {0x0000001,
                            0x0000011,
                            0x0001111,
                            0x0000111,
                            0x0011111,
                            0x0111111,
                            0x1111111,
                            0x111111F,
                            0x11111FF,
                            0x1111FFF};
  // clang-format on

  void FillHeap(const int how_many) {
    for (int i = 0; i < how_many; i++) {
      pheap_insert(&heap, &nodes[i]);
    }
  }

  TEST_SETUP() {
    pheap_init(&heap, pheap_cmp_ints);

    for (size_t i = 0; i < NODES_NUMBER; i++) {
      pnode* node_ptr = &nodes[i];
      int* key_ptr = &keys[i];
      pnode_init(node_ptr, key_ptr);
    }
  }
};

TEST(UtilPheap, PheapCmpInts) {
  int a = 2;
  int b = 3;
  int c = 2;

  CHECK_EQUAL(0, pheap_cmp_ints(&a, &c));
  CHECK_COMPARE(0, >, pheap_cmp_ints(&a, &b));
  CHECK_COMPARE(0, <, pheap_cmp_ints(&b, &a));
}

TEST(UtilPheap, PnodeInit) {
  pnode* node_ptr = &nodes[0];
  void* key_ptr = &keys[0];

  pnode_init(node_ptr, key_ptr);

  const int value_of_node = *(static_cast<const int*>(node_ptr->key));
  CHECK_EQUAL(keys[0], value_of_node);
}

TEST(UtilPheap, PnodeNext) {
  pnode* node_ptr = &nodes[0];
  pnode* next_node_ptr = &nodes[1];

  node_ptr->next = next_node_ptr;

  POINTERS_EQUAL(next_node_ptr, pnode_next(node_ptr));
}

TEST(UtilPheap, PheapEmpty_IsEmpty) { CHECK_EQUAL(1, pheap_empty(&heap)); }

TEST(UtilPheap, PheapEmpty_IsNotEmpty) {
  pheap_insert(&heap, &nodes[0]);

  CHECK_EQUAL(0, pheap_empty(&heap));
}

TEST(UtilPheap, PheapSize_IsEmpty) { CHECK_EQUAL(0, pheap_size(&heap)); }

TEST(UtilPheap, PheapSize_HasOneElement) {
  pheap_insert(&heap, &nodes[0]);

  CHECK_EQUAL(1UL, pheap_size(&heap));
}

TEST(UtilPheap, PheapSize_HasMultipleElements) {
  pheap_insert(&heap, &nodes[0]);
  pheap_insert(&heap, &nodes[1]);

  CHECK_EQUAL(2UL, pheap_size(&heap));
}

TEST(UtilPheap, PheapInsert_WhenEmpty) {
  pnode* node_ptr = &nodes[0];

  pheap_insert(&heap, node_ptr);

  CHECK_EQUAL(node_ptr, heap.root);
}

TEST(UtilPheap, PheapRemove_NodeIsParentAndIsOnlyElement) {
  pnode* node_ptr = &nodes[0];

  pheap_insert(&heap, node_ptr);
  pheap_remove(&heap, node_ptr);

  CHECK_EQUAL(0UL, pheap_size(&heap));
}

TEST(UtilPheap, PheapRemove_NodeIsNotParentAndIsNotOnlyElement) {
  pnode* node_ptr = &nodes[0];
  pnode* next_node_ptr = &nodes[1];

  pheap_insert(&heap, node_ptr);
  pheap_insert(&heap, next_node_ptr);
  pheap_remove(&heap, node_ptr);

  CHECK_EQUAL(1UL, pheap_size(&heap));
}

TEST(UtilPheap, PheapRemove_NodeIsParentAndIsNotOnlyElement) {
  pnode* node_ptr = &nodes[0];
  pnode* next_node_ptr = &nodes[1];

  pheap_insert(&heap, node_ptr);
  pheap_insert(&heap, next_node_ptr);
  pheap_remove(&heap, next_node_ptr);

  CHECK_EQUAL(1UL, pheap_size(&heap));
}

TEST(UtilPheap, PheapFind_WhenEmpty) {
  void* key_ptr = &keys[0];

  POINTERS_EQUAL(nullptr, pheap_find(&heap, key_ptr));
}

TEST(UtilPheap, PheapFind_WhenNotEmpty) {
  FillHeap(2);

  POINTERS_EQUAL(&nodes[0], pheap_find(&heap, &keys[0]));
  POINTERS_EQUAL(&nodes[1], pheap_find(&heap, &keys[1]));
}

TEST(UtilPheap, PheapFind_FindNotPresentWhenMultipleElements) {
  FillHeap(3);

  POINTERS_EQUAL(nullptr, pheap_find(&heap, &keys[3]));
}

TEST(UtilPheap, PheapFirst) { POINTERS_EQUAL(heap.root, pheap_first(&heap)); }

TEST(UtilPheap, PnodeForeach_EmptyHeap) {
  int node_counter = 0;

  pnode_foreach(heap.root, node) node_counter++;

  CHECK_EQUAL(0, node_counter);
}

TEST(UtilPheap, PnodeForeach_OnlyHead) {
  int node_counter = 0;
  FillHeap(1);

  pnode_foreach(heap.root, node) node_counter++;

  CHECK_EQUAL(1, node_counter);
}

TEST(UtilPheap, PnodeForeach_MultipleElements) {
  int node_counter = 0;
  FillHeap(2);

  pnode_foreach(heap.root, node) node_counter++;

  CHECK_EQUAL(2, node_counter);
}

TEST(UtilPheap, PheapForeach_EmptyHeap) {
  int node_counter = 0;

  pheap_foreach(&heap, node) node_counter++;

  CHECK_EQUAL(0, node_counter);
}

TEST(UtilPheap, PheapForeach_OnlyHead) {
  int node_counter = 0;
  FillHeap(1);

  pheap_foreach(&heap, node) node_counter++;

  CHECK_EQUAL(1, node_counter);
}

TEST(UtilPheap, PheapForeach_MultipleElements) {
  int node_counter = 0;
  FillHeap(2);

  pheap_foreach(&heap, node) node_counter++;

  CHECK_EQUAL(2, node_counter);
}
