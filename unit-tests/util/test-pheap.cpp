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

#include <cassert>
#include <set>

#include <CppUTest/TestHarness.h>

#include <lely/util/pheap.h>

static int
pheap_cmp_ints(const void* p1, const void* p2) noexcept {
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

TEST_GROUP(Util_PheapCmpInts){};

TEST(Util_PheapCmpInts, PheapCmpInts) {
  int a = 2;
  int b = 3;
  int c = 2;

  CHECK_EQUAL(0, pheap_cmp_ints(&a, &c));
  CHECK_COMPARE(0, >, pheap_cmp_ints(&a, &b));
  CHECK_COMPARE(0, <, pheap_cmp_ints(&b, &a));
}

TEST_GROUP(Util_Pheap) {
  pheap heap;
  static const size_t NODES_NUM = 10;

  pnode nodes[NODES_NUM];
  int keys[NODES_NUM] = {-32454, -2431, 0,     273,   332,
                         3244,   4444,  13444, 17895, 21995};

  void FillHeap(const int how_many) {
    for (int i = 0; i < how_many; i++) {
      pheap_insert(&heap, &nodes[i]);
    }
  }

  TEST_SETUP() {
    pheap_init(&heap, pheap_cmp_ints);

    for (size_t i = 0; i < NODES_NUM; i++) {
      pnode_init(&nodes[i], &keys[i]);
    }
  }
};

TEST(Util_Pheap, PheapInit_EmptyWhenInitialized) {
  CHECK_EQUAL(0, pheap_size(&heap));
  POINTERS_EQUAL(nullptr, pheap_first(&heap));
}

TEST(Util_Pheap, PnodeInit) {
  CHECK_EQUAL(keys[0], *static_cast<const int*>(nodes[0].key));
  CHECK_EQUAL(keys[1], *static_cast<const int*>(nodes[1].key));
  CHECK_EQUAL(keys[NODES_NUM - 1],
              *static_cast<const int*>(nodes[NODES_NUM - 1].key));
}

TEST(Util_Pheap, PnodeNext_Null) {
  pheap_insert(&heap, &nodes[1]);

  POINTERS_EQUAL(nullptr, pnode_next(&nodes[1]));
}

TEST(Util_Pheap, PnodeNext_Child) {
  pheap_insert(&heap, &nodes[0]);
  pheap_insert(&heap, &nodes[2]);
  pheap_insert(&heap, &nodes[1]);

  POINTERS_EQUAL(&nodes[1], pnode_next(&nodes[0]));
  POINTERS_EQUAL(&nodes[2], pnode_next(&nodes[1]));
  POINTERS_EQUAL(nullptr, pnode_next(&nodes[2]));
}

TEST(Util_Pheap, PnodeNext_ReplaceRoot) {
  pheap_insert(&heap, &nodes[1]);
  pheap_insert(&heap, &nodes[0]);
  pheap_insert(&heap, &nodes[2]);

  POINTERS_EQUAL(&nodes[2], pnode_next(&nodes[0]));
  POINTERS_EQUAL(nullptr, pnode_next(&nodes[1]));
  POINTERS_EQUAL(&nodes[1], pnode_next(&nodes[2]));
}

TEST(Util_Pheap, PheapEmpty_IsEmpty) { CHECK_EQUAL(1, pheap_empty(&heap)); }

TEST(Util_Pheap, PheapEmpty_IsNotEmpty) {
  pheap_insert(&heap, &nodes[0]);

  CHECK_EQUAL(0, pheap_empty(&heap));
}

TEST(Util_Pheap, PheapSize_IsEmpty) { CHECK_EQUAL(0, pheap_size(&heap)); }

TEST(Util_Pheap, PheapSize_HasOneElement) {
  pheap_insert(&heap, &nodes[0]);

  CHECK_EQUAL(1UL, pheap_size(&heap));
}

TEST(Util_Pheap, PheapSize_HasMultipleElements) {
  pheap_insert(&heap, &nodes[0]);
  pheap_insert(&heap, &nodes[1]);

  CHECK_EQUAL(2UL, pheap_size(&heap));
}

TEST(Util_Pheap, PheapInsert_WhenEmpty) {
  pheap_insert(&heap, &nodes[0]);

  CHECK_EQUAL(&nodes[0], pheap_first(&heap));
}

TEST(Util_Pheap, PheapRemove_NodeIsParentAndIsOnlyElement) {
  pheap_insert(&heap, &nodes[0]);

  pheap_remove(&heap, &nodes[0]);

  CHECK_EQUAL(0UL, pheap_size(&heap));
}

TEST(Util_Pheap, PheapRemove_NodeIsNotParentAndIsNotOnlyElement) {
  pheap_insert(&heap, &nodes[0]);
  pheap_insert(&heap, &nodes[1]);

  pheap_remove(&heap, &nodes[0]);

  CHECK_EQUAL(1UL, pheap_size(&heap));
}

TEST(Util_Pheap, PheapRemove_NodeIsParentAndIsNotOnlyElement) {
  pheap_insert(&heap, &nodes[0]);
  pheap_insert(&heap, &nodes[1]);

  pheap_remove(&heap, &nodes[1]);

  CHECK_EQUAL(1UL, pheap_size(&heap));
}

TEST(Util_Pheap, PheapRemove_NodeFromTheMiddleThatHasNext) {
  pheap_insert(&heap, &nodes[3]);
  pheap_insert(&heap, &nodes[2]);
  pheap_insert(&heap, &nodes[4]);
  pheap_insert(&heap, &nodes[1]);

  pheap_remove(&heap, &nodes[2]);
  CHECK_EQUAL(3UL, pheap_size(&heap));
}

TEST(Util_Pheap, PheapFind_WhenEmpty) {
  POINTERS_EQUAL(nullptr, pheap_find(&heap, &keys[0]));
}

TEST(Util_Pheap, PheapFind_WhenNotEmpty) {
  pheap_insert(&heap, &nodes[1]);
  pheap_insert(&heap, &nodes[2]);
  pheap_insert(&heap, &nodes[3]);
  pheap_insert(&heap, &nodes[0]);

  POINTERS_EQUAL(&nodes[0], pheap_find(&heap, &keys[0]));
  POINTERS_EQUAL(&nodes[1], pheap_find(&heap, &keys[1]));
  POINTERS_EQUAL(&nodes[2], pheap_find(&heap, &keys[2]));
  POINTERS_EQUAL(&nodes[3], pheap_find(&heap, &keys[3]));
}

TEST(Util_Pheap, PheapFind_FindNotPresentWhenMultipleElements) {
  FillHeap(3);

  POINTERS_EQUAL(nullptr, pheap_find(&heap, &keys[3]));
}

TEST(Util_Pheap, PheapFirst) {
  pheap_insert(&heap, &nodes[0]);

  POINTERS_EQUAL(&nodes[0], pheap_first(&heap));
}

TEST(Util_Pheap, PnodeForeach_EmptyHeap) {
  int node_counter = 0;

  pnode_foreach(pheap_first(&heap), node) node_counter++;

  CHECK_EQUAL(0, node_counter);
}

TEST(Util_Pheap, PnodeForeach_OnlyHead) {
  int node_counter = 0;
  FillHeap(1);

  pnode_foreach(pheap_first(&heap), node) {
    POINTERS_EQUAL(pheap_first(&heap), node);
    node_counter++;
  }

  CHECK_EQUAL(1, node_counter);
}

TEST(Util_Pheap, PnodeForeach_MultipleElements) {
  int node_counter = 0;
  FillHeap(NODES_NUM);
  std::set<int> visited_keys;

  pnode_foreach(pheap_first(&heap), node) {
    visited_keys.insert(*static_cast<const int*>(node->key));
    node_counter++;
  }

  CHECK_EQUAL(NODES_NUM, node_counter);
  CHECK_EQUAL(NODES_NUM, visited_keys.size());
}

TEST(Util_Pheap, PheapForeach_EmptyHeap) {
  int node_counter = 0;

  pheap_foreach(&heap, node) node_counter++;

  CHECK_EQUAL(0, node_counter);
}

TEST(Util_Pheap, PheapForeach_OnlyHead) {
  int node_counter = 0;
  FillHeap(1);

  pheap_foreach(&heap, node) {
    POINTERS_EQUAL(pheap_first(&heap), node);
    node_counter++;
  }

  CHECK_EQUAL(1, node_counter);
}

TEST(Util_Pheap, PheapForeach_MultipleElements) {
  int node_counter = 0;
  FillHeap(NODES_NUM);
  std::set<int> visited_keys;

  pheap_foreach(&heap, node) {
    visited_keys.insert(*static_cast<const int*>(node->key));
    node_counter++;
  }

  CHECK_EQUAL(NODES_NUM, node_counter);
  CHECK_EQUAL(NODES_NUM, visited_keys.size());
}

TEST(Util_Pheap, PheapForeach_MultiElementsRemoveCurrent) {
  int node_counter = 0;
  FillHeap(NODES_NUM);
  std::set<int> visited_keys;

  pheap_foreach(&heap, node) {
    if (node_counter != 2) {
      visited_keys.insert(*static_cast<const int*>(node->key));
    } else {
      pheap_remove(&heap, node);
    }
    node_counter++;
  }

  CHECK_EQUAL(NODES_NUM, node_counter);
  CHECK_EQUAL(NODES_NUM - 1, visited_keys.size());
}

TEST(Util_Pheap, PheapContains_HeapEmpty) {
  CHECK_EQUAL(0, pheap_contains(&heap, &nodes[0]));
  CHECK_EQUAL(0, pheap_contains(&heap, &nodes[1]));
  CHECK_EQUAL(0, pheap_contains(&heap, &nodes[2]));
}

TEST(Util_Pheap, PheapContains_ContainsAsRoot) {
  pheap_insert(&heap, &nodes[0]);

  CHECK_EQUAL(1, pheap_contains(&heap, &nodes[0]));
  CHECK_EQUAL(0, pheap_contains(&heap, &nodes[1]));
  CHECK_EQUAL(0, pheap_contains(&heap, &nodes[2]));
}

TEST(Util_Pheap, PheapContains_ContainsAsParentsChild) {
  pheap_insert(&heap, &nodes[0]);
  pheap_insert(&heap, &nodes[1]);

  CHECK_EQUAL(1, pheap_contains(&heap, &nodes[0]));
  CHECK_EQUAL(1, pheap_contains(&heap, &nodes[1]));
  CHECK_EQUAL(0, pheap_contains(&heap, &nodes[2]));
}

TEST(Util_Pheap, PheapContains_ContainsMany) {
  pheap_insert(&heap, &nodes[3]);
  pheap_insert(&heap, &nodes[2]);
  pheap_insert(&heap, &nodes[1]);

  CHECK_EQUAL(0, pheap_contains(&heap, &nodes[0]));
  CHECK_EQUAL(1, pheap_contains(&heap, &nodes[1]));
  CHECK_EQUAL(1, pheap_contains(&heap, &nodes[2]));
  CHECK_EQUAL(1, pheap_contains(&heap, &nodes[3]));
  CHECK_EQUAL(0, pheap_contains(&heap, &nodes[4]));
}
