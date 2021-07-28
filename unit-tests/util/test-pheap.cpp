/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2020-2021 N7 Space Sp. z o.o.
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

#include <cassert>
#include <set>

#include <CppUTest/TestHarness.h>

#include <lely/util/pheap.h>

static int
pheap_cmp_ints(const void* const p1, const void* const p2) noexcept {
  assert(p1);
  assert(p2);

  const auto val1 = *static_cast<const int32_t*>(p1);
  const auto val2 = *static_cast<const int32_t*>(p2);

  if (val1 > val2)
    return 1;
  else if (val1 == val2)
    return 0;
  else
    return -1;
}

TEST_GROUP(PheapCmpInts){};

TEST(PheapCmpInts, PheapCmpInts) {
  const int32_t a = 2;
  const int32_t b = 3;
  const int32_t c = 2;

  CHECK_EQUAL(0, pheap_cmp_ints(&a, &c));
  CHECK_COMPARE(0, >, pheap_cmp_ints(&a, &b));
  CHECK_COMPARE(0, <, pheap_cmp_ints(&b, &a));
}

TEST_BASE(Util_PheapBase) {
  pheap heap;
  static const size_t NODES_NUM = 10;

  pnode nodes[NODES_NUM];
  const int32_t keys[NODES_NUM] = {-32454, -2431, 0,     273,   332,
                                   3244,   4444,  13444, 17895, 21995};

  void FillHeap(const size_t how_many) {
    assert(how_many <= NODES_NUM);

    for (size_t i = 0; i < how_many; i++) {
      pheap_insert(&heap, &nodes[i]);
    }
  }
};

TEST_GROUP_BASE(Util_PheapInit, Util_PheapBase){};

/// @name pheap_init()
///@{

/// \Given an uninitialized pairing heap (pheap)
///
/// \When pheap_init() is called with a pointer to the integer comparison
///       function
///
/// \Then the heap size is zero, the first node pointer is null and
///       the comparison function is set
TEST(Util_PheapInit, EmptyWhenInitialized) {
  pheap_init(&heap, pheap_cmp_ints);

  CHECK_EQUAL(0, pheap_size(&heap));
  POINTERS_EQUAL(nullptr, pheap_first(&heap));
  FUNCTIONPOINTERS_EQUAL(pheap_cmp_ints, heap.cmp);
}

///@}

/// @name pnode_init()
///@{

/// \Given a pointer to the uninitialized node (pnode)
///
/// \When pnode_init() is called with a pointer to the key value
///
/// \Then the node is initialized with the given value
TEST(Util_PheapInit, PnodeInit) {
  pnode_init(&nodes[0], &keys[0]);

  CHECK_EQUAL(keys[0], *static_cast<const int*>(nodes[0].key));
}

///@}

TEST_GROUP_BASE(Util_Pheap, Util_PheapBase) {
  int32_t data = 0;  // dummy line to workaround clang-format issue

  TEST_SETUP() {
    pheap_init(&heap, pheap_cmp_ints);

    for (size_t i = 0; i < NODES_NUM; i++) {
      pnode_init(&nodes[i], &keys[i]);
    }
  }
};

/// @name pnode_next()
///@{

/// \Given a pairing heap (pheap) with a single node (pnode) inserted
///
/// \When pnode_next() is called
///
/// \Then a null pointer is returned
TEST(Util_Pheap, PnodeNext_Null) {
  pheap_insert(&heap, &nodes[1]);

  const auto* const ret = pnode_next(&nodes[1]);

  POINTERS_EQUAL(nullptr, ret);
}

/// \Given a pairing heap (pheap) with nodes inserted (node 1 is a child
///        of node 0)
///
/// \When pnode_next() is called for the node 0
///
/// \Then the address of the node 1 is returned
TEST(Util_Pheap, PnodeNext_Child) {
  pheap_insert(&heap, &nodes[0]);
  pheap_insert(&heap, &nodes[2]);
  pheap_insert(&heap, &nodes[1]);

  POINTERS_EQUAL(&nodes[1], pnode_next(&nodes[0]));
}

/// \Given a pairing heap (pheap) with nodes inserted (node 1 has no child
///        and no next node)
///
/// \When pnode_next() is called for the node 1
///
/// \Then the address of the parent's next node (null pointer) is returned
TEST(Util_Pheap, PnodeNext_Parent) {
  pheap_insert(&heap, &nodes[1]);
  pheap_insert(&heap, &nodes[0]);
  pheap_insert(&heap, &nodes[2]);

  POINTERS_EQUAL(nullptr, pnode_next(&nodes[1]));
}

///@}

/// @name pheap_empty()
///@{

/// \Given an empty pairing heap (pheap)
///
/// \When pheap_empty() is called
///
/// \Then 1 is returned
///       \Calls pheap_size()
TEST(Util_Pheap, PheapEmpty_IsEmpty) { CHECK_EQUAL(1, pheap_empty(&heap)); }

/// \Given a non-empty pairing heap (pheap)
///
/// \When pheap_empty() is called
///
/// \Then 0 is returned
///       \Calls pheap_size()
TEST(Util_Pheap, PheapEmpty_IsNotEmpty) {
  pheap_insert(&heap, &nodes[0]);

  CHECK_EQUAL(0, pheap_empty(&heap));
}

///@}

/// @name pheap_size()
///@{

/// \Given an empty pairing heap (pheap)
///
/// \When pheap_size() is called
///
/// \Then 0 is returned
TEST(Util_Pheap, PheapSize_IsEmpty) { CHECK_EQUAL(0, pheap_size(&heap)); }

/// \Given a pairing heap (pheap) with one node (pnode) inserted
///
/// \When pheap_size() is called
///
/// \Then 1 is returned
TEST(Util_Pheap, PheapSize_HasOneElement) {
  pheap_insert(&heap, &nodes[0]);

  CHECK_EQUAL(1uL, pheap_size(&heap));
}

/// \Given a pairing heap (pheap) with multiple nodes inserted
///
/// \When pheap_size() is called
///
/// \Then the number of nodes is returned
TEST(Util_Pheap, PheapSize_HasMultipleElements) {
  pheap_insert(&heap, &nodes[0]);
  pheap_insert(&heap, &nodes[1]);

  CHECK_EQUAL(2uL, pheap_size(&heap));
}

///@}

/// @name pheap_insert()
///@{

/// \Given an empty pairing heap (pheap)
///
/// \When pheap_insert() is called with a node (pnode) address
///
/// \Then the node is inserted, heap size is one
TEST(Util_Pheap, PheapInsert_WhenEmpty) {
  pheap_insert(&heap, &nodes[0]);

  CHECK_EQUAL(&nodes[0], pheap_first(&heap));
  CHECK_EQUAL(1u, pheap_size(&heap));
}

/// \Given a pairing heap (pheap) with one node (pnode) inserted
///
/// \When pheap_insert() is called with a node address
///
/// \Then the node is inserted, heap size is two
TEST(Util_Pheap, PheapInsert_OneInserted) {
  pheap_insert(&heap, &nodes[0]);

  pheap_insert(&heap, &nodes[1]);

  CHECK_EQUAL(&nodes[1], pheap_find(&heap, &keys[1]));
  CHECK_EQUAL(2u, pheap_size(&heap));
}

/// \Given a pairing heap (pheap) with multiple nodes (pnode) inserted
///
/// \When pheap_insert() is called with a node address
///
/// \Then the node is inserted, heap size has increased by one
TEST(Util_Pheap, PheapInsert_MultipleInserted) {
  pheap_insert(&heap, &nodes[0]);
  pheap_insert(&heap, &nodes[1]);

  pheap_insert(&heap, &nodes[2]);

  CHECK_EQUAL(&nodes[2], pheap_find(&heap, &keys[2]));
  CHECK_EQUAL(3u, pheap_size(&heap));
}

///@}

/// @name pheap_remove()
///@{

/// \Given a pairing heap (pheap) with one node (pnode) inserted
///
/// \When pheap_remove() is called with the node address
///
/// \Then the node is removed and the pairing heap is empty
TEST(Util_Pheap, PheapRemove_NodeIsOnlyElement) {
  pheap_insert(&heap, &nodes[0]);

  pheap_remove(&heap, &nodes[0]);

  CHECK_EQUAL(0uL, pheap_size(&heap));
  CHECK_EQUAL(1, pheap_empty(&heap));
}

/// \Given a pairing heap (pheap) with multiple nodes (pnode) inserted
///
/// \When pheap_remove() is called with the non-parent node address
///
/// \Then the node is removed
TEST(Util_Pheap, PheapRemove_NodeIsNotParentAndIsNotOnlyElement) {
  pheap_insert(&heap, &nodes[0]);
  pheap_insert(&heap, &nodes[1]);

  pheap_remove(&heap, &nodes[0]);

  CHECK_EQUAL(1uL, pheap_size(&heap));
}

/// \Given a pairing heap (pheap) with multiple nodes (pnode) inserted
///
/// \When pheap_remove() is called with the parent node address
///
/// \Then the node is removed
TEST(Util_Pheap, PheapRemove_NodeIsParentAndIsNotOnlyElement) {
  pheap_insert(&heap, &nodes[0]);
  pheap_insert(&heap, &nodes[1]);

  pheap_remove(&heap, &nodes[1]);

  CHECK_EQUAL(1uL, pheap_size(&heap));
}

/// \Given a pairing heap (pheap) with multiple nodes (pnode) inserted
///
/// \When pheap_remove() is called with the address of the node which has
///       a child node
///
/// \Then the node is removed
TEST(Util_Pheap, PheapRemove_NodeFromTheMiddleThatHasChild) {
  pheap_insert(&heap, &nodes[3]);
  pheap_insert(&heap, &nodes[2]);
  pheap_insert(&heap, &nodes[4]);
  pheap_insert(&heap, &nodes[1]);

  pheap_remove(&heap, &nodes[2]);

  CHECK_EQUAL(3uL, pheap_size(&heap));
}

/// \Given a pairing heap (pheap) with multiple nodes (pnode) inserted
///
/// \When pheap_remove() is called with the address of the node which has
///       both parent and sibling nodes
///
/// \Then the node is removed
TEST(Util_Pheap, PheapRemove_NodeWithParentAndSibling) {
  pheap_insert(&heap, &nodes[3]);
  pheap_insert(&heap, &nodes[2]);
  pheap_insert(&heap, &nodes[4]);
  pheap_insert(&heap, &nodes[1]);

  pheap_remove(&heap, &nodes[3]);

  CHECK_EQUAL(3uL, pheap_size(&heap));
}

///@}

/// @name pheap_find()
///@{

/// \Given an empty pairing heap (pheap)
///
/// \When pheap_find() is called with an address of any key
///
/// \Then null pointer is returned
TEST(Util_Pheap, PheapFind_Empty) {
  POINTERS_EQUAL(nullptr, pheap_find(&heap, &keys[0]));
}

/// \Given a pairing heap (pheap) with multiple nodes (pnode) inserted
///
/// \When pheap_find() is called with an address of key of one of the inserted
//        nodes
///
/// \Then an address of the node with the key is returned
TEST(Util_Pheap, PheapFind_NotEmpty) {
  pheap_insert(&heap, &nodes[1]);
  pheap_insert(&heap, &nodes[2]);
  pheap_insert(&heap, &nodes[3]);
  pheap_insert(&heap, &nodes[0]);

  POINTERS_EQUAL(&nodes[2], pheap_find(&heap, &keys[2]));
}

/// \Given a pairing heap (pheap) with multiple nodes (pnode) inserted
///
/// \When pheap_find() is called with an address of a key which is not
///       present in any of the nodes stored in the heap
///
/// \Then null pointer is returned
TEST(Util_Pheap, PheapFind_FindNotPresentWhenMultipleElements) {
  FillHeap(3);

  POINTERS_EQUAL(nullptr, pheap_find(&heap, &keys[3]));
}

///@}

/// @name pheap_first()
///@{

/// \Given a pairing heap (pheap) with one node (pnode) inserted
///
/// \When pheap_first() is called
///
/// \Then the node address is returned
TEST(Util_Pheap, PheapFirst_OneInserted) {
  pheap_insert(&heap, &nodes[0]);

  POINTERS_EQUAL(&nodes[0], pheap_first(&heap));
}

/// \Given a pairing heap (pheap) with multiple nodes (pnode) inserted
///
/// \When pheap_first() is called
///
/// \Then the address of the node with a minimum key value is returned
TEST(Util_Pheap, PheapFirst_MultipleInserted) {
  FillHeap(6u);
  pheap_remove(&heap, &nodes[0]);

  POINTERS_EQUAL(&nodes[1], pheap_first(&heap));
}

///@}

/// @name pnode_foreach()

/// \Given an empty pairing heap (pheap)
///
/// \When pnode_foreach() is used with heap's root node
///
/// \Then body of the loop was not executed
TEST(Util_Pheap, PnodeForeach_EmptyHeap) {
  size_t node_counter = 0;

  pnode_foreach(pheap_first(&heap), node) node_counter++;

  CHECK_EQUAL(0, node_counter);
}

/// \Given a pairing heap (pheap) with one node (pnode) inserted
///
/// \When pnode_foreach() is used with heap's root node
///
/// \Then body of the loop is executed once
TEST(Util_Pheap, PnodeForeach_OnlyHead) {
  size_t node_counter = 0;
  FillHeap(1);

  pnode_foreach(pheap_first(&heap), node) {
    POINTERS_EQUAL(pheap_first(&heap), node);
    node_counter++;
  }

  CHECK_EQUAL(1, node_counter);
}

/// \Given a pairing heap (pheap) with multiple nodes (pnode) inserted
///
/// \When pnode_foreach() is used with heap's root node
///
/// \Then body of the loop is executed once for each node
///       \Calls pnode_next()
TEST(Util_Pheap, PnodeForeach_MultipleElements) {
  size_t node_counter = 0;
  FillHeap(NODES_NUM);
  std::set<int32_t> visited_keys;

  pnode_foreach(pheap_first(&heap), node) {
    visited_keys.insert(*static_cast<const int32_t*>(node->key));
    node_counter++;
  }

  CHECK_EQUAL(NODES_NUM, node_counter);
  CHECK_EQUAL(NODES_NUM, visited_keys.size());
}

///@}

/// @name pheap_foreach()
///@{

/// \Given an empty pairing heap (pheap)
///
/// \When pheap_foreach() is used
///
/// \Then body of the loop was not executed
///       \Calls pheap_first()
TEST(Util_Pheap, PheapForeach_EmptyHeap) {
  size_t node_counter = 0;

  pheap_foreach(&heap, node) node_counter++;

  CHECK_EQUAL(0, node_counter);
}

/// \Given a pairing heap (pheap) with one node (pnode) inserted
///
/// \When pheap_foreach() is used
///
/// \Then body of the loop is executed once
///       \Calls pheap_first()
TEST(Util_Pheap, PheapForeach_OnlyHead) {
  size_t node_counter = 0;
  FillHeap(1);

  pheap_foreach(&heap, node) {
    POINTERS_EQUAL(pheap_first(&heap), node);
    node_counter++;
  }

  CHECK_EQUAL(1, node_counter);
}

/// \Given a pairing heap (pheap) with multiple nodes (pnode) inserted
///
/// \When pheap_foreach() is used
///
/// \Then body of the loop is executed once for each node
///       \Calls pheap_first()
///       \Calls pnode_next()
TEST(Util_Pheap, PheapForeach_MultipleElements) {
  size_t node_counter = 0;
  FillHeap(NODES_NUM);
  std::set<int32_t> visited_keys;

  pheap_foreach(&heap, node) {
    visited_keys.insert(*static_cast<const int32_t*>(node->key));
    node_counter++;
  }

  CHECK_EQUAL(NODES_NUM, node_counter);
  CHECK_EQUAL(NODES_NUM, visited_keys.size());
}

/// \Given a pairing heap (pheap) with multiple nodes (pnode) inserted
///
/// \When pheap_foreach() is used and one of the nodes is removed
///
/// \Then body of the loop is executed once for each node and the removed node
///       is not present in the heap
///       \Calls pheap_first()
///       \Calls pnode_next()
TEST(Util_Pheap, PheapForeach_MultiElementsRemoveCurrent) {
  size_t iteration_counter = 0;
  FillHeap(NODES_NUM);

  pheap_foreach(&heap, node) {
    if (iteration_counter == 2) pheap_remove(&heap, node);
    iteration_counter++;
  }

  CHECK_EQUAL(NODES_NUM, iteration_counter);
  CHECK_EQUAL(NODES_NUM - 1, pheap_size(&heap));
}

///@}

/// @name pheap_contains()
///@{

/// \Given an empty pairing heap (pheap)
///
/// \When pheap_contains() is called with a node address
///
/// \Then 0 is returned
TEST(Util_Pheap, PheapContains_HeapEmpty) {
  CHECK_FALSE(pheap_contains(&heap, &nodes[0]));
  CHECK_FALSE(pheap_contains(&heap, &nodes[1]));
  CHECK_FALSE(pheap_contains(&heap, &nodes[2]));
}

/// \Given a pairing heap (pheap) with one node (pnode) inserted
///
/// \When pheap_contains() is called with the node address
///
/// \Then 1 is returned
TEST(Util_Pheap, PheapContains_OneInserted_Contains) {
  pheap_insert(&heap, &nodes[0]);

  CHECK(pheap_contains(&heap, &nodes[0]));
}

/// \Given a pairing heap (pheap) with one node (pnode) inserted
///
/// \When pheap_contains() is called with address of the node which is not
///       present in the heap
///
/// \Then 0 is returned
TEST(Util_Pheap, PheapContains_OneInserted_NotContain) {
  pheap_insert(&heap, &nodes[0]);

  CHECK_FALSE(pheap_contains(&heap, &nodes[1]));
}

/// \Given a pairing heap (pheap) with multiple nodes (pnode) inserted
///
/// \When pheap_contains() is called with address of the node which is
///       present in the heap
///
/// \Then 1 is returned
TEST(Util_Pheap, PheapContains_Many_Contains) {
  pheap_insert(&heap, &nodes[3]);
  pheap_insert(&heap, &nodes[2]);
  pheap_insert(&heap, &nodes[1]);

  CHECK(pheap_contains(&heap, &nodes[1]));
  CHECK(pheap_contains(&heap, &nodes[2]));
  CHECK(pheap_contains(&heap, &nodes[3]));
}

/// \Given a pairing heap (pheap) with multiple nodes (pnode) inserted
///
/// \When pheap_contains() is called with address of the node which is not
///       present in the heap
///
/// \Then 0 is returned
TEST(Util_Pheap, PheapContains_Many_NotContain) {
  pheap_insert(&heap, &nodes[3]);
  pheap_insert(&heap, &nodes[2]);
  pheap_insert(&heap, &nodes[1]);

  CHECK_FALSE(pheap_contains(&heap, &nodes[0]));
  CHECK_FALSE(pheap_contains(&heap, &nodes[4]));
}

///@}
