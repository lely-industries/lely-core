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

#include <CppUTest/TestHarness.h>

#include <lely/util/dllist.h>

TEST_GROUP(Util_DllistInit){};

/// @name dllist_init()
///@{

/// \Given an uninitialized instance of a doubly-linked list (dllist)
///
/// \When dllist_init() is called
///
/// \Then the list is initialized, has null pointers to the first and the last
///       nodes
TEST(Util_DllistInit, DllistInit_Nominal) {
  dllist list;

  dllist_init(&list);

  POINTERS_EQUAL(nullptr, dllist_first(&list));
  POINTERS_EQUAL(nullptr, dllist_last(&list));
}

///@}

/// @name dlnode_init()
///@{

/// \Given an uninitialized instance of a doubly-linked list node (dlnode)
///
/// \When dlnode_init() is called
///
/// \Then the node is initialized, has null pointers to the next and the
///       previous nodes
TEST(Util_DllistInit, DlnodeInit_Nominal) {
  dlnode node;

  dlnode_init(&node);

  POINTERS_EQUAL(nullptr, node.next);
  POINTERS_EQUAL(nullptr, node.prev);
}

///@}

TEST_GROUP(Util_Dllist) {
  static const size_t NODES_NUM = 10u;
  dlnode nodes[NODES_NUM];
  dllist list;

  void AreNodesLinked(const dlnode* const node, const dlnode* const next_node) {
    POINTERS_EQUAL(node->next, next_node);
    POINTERS_EQUAL(next_node->prev, node);
  }

  TEST_SETUP() {
    dllist_init(&list);
    for (size_t i = 0u; i < NODES_NUM; i++) {
      dlnode_init(&nodes[i]);
    }
  }
};

/// @name dllist_empty()
///@{

/// \Given an empty doubly-linked list
///
/// \When dllist_empty() is called
///
/// \Then 1 is returned
TEST(Util_Dllist, DllistEmpty_IsEmpty) { CHECK_EQUAL(1, dllist_empty(&list)); }

/// \Given a doubly-linked list with at least one node
///
/// \When dllist_empty() is called
///
/// \Then 0 is returned
TEST(Util_Dllist, DllistEmpty_NotEmpty) {
  dllist_push_front(&list, &nodes[0]);

  CHECK_EQUAL(0u, dllist_empty(&list));
}

///@}

/// @name dllist_size()
///@{

/// \Given an empty doubly-linked list
///
/// \When dllist_size() is called
///
/// \Then 0 is returned
TEST(Util_Dllist, DllistSize_IsEmpty) { CHECK_EQUAL(0u, dllist_size(&list)); }

/// \Given a doubly-linked list with one node
///
/// \When dllist_size() is called
///
/// \Then 1 is returned
TEST(Util_Dllist, DllistSize_OneAdded) {
  dllist_push_front(&list, &nodes[0]);

  CHECK_EQUAL(1u, dllist_size(&list));
}

/// \Given a doubly-linked list with two nodes
///
/// \When dllist_size() is called
///
/// \Then 2 is returned
TEST(Util_Dllist, DllistSize_ManyAdded) {
  dllist_push_front(&list, &nodes[0]);
  dllist_push_front(&list, &nodes[1]);

  CHECK_EQUAL(2u, dllist_size(&list));
}

///@}

/// @name dllist_push_front()
///@{

/// \Given an empty doubly-linked list and an initialized node
///
/// \When dllist_push_front() is called with a pointer to the node
///
/// \Then the node is added to the list, the list is no longer empty
TEST(Util_Dllist, DllistPushFront_Empty) {
  dllist_push_front(&list, &nodes[0]);

  CHECK_EQUAL(1u, dllist_size(&list));
  POINTERS_EQUAL(&nodes[0], dllist_first(&list));
}

/// \Given a doubly-linked list with multiple nodes and an initialized node
///
/// \When dllist_push_front() is called with a pointer to the node
///
/// \Then the node is added to the list as the new first node, size of the list
///       is increased by 1
TEST(Util_Dllist, DllistPushFront_ManyAdded) {
  dllist_push_front(&list, &nodes[0]);
  dllist_push_front(&list, &nodes[1]);
  dllist_push_front(&list, &nodes[2]);
  const size_t size = dllist_size(&list);

  dllist_push_front(&list, &nodes[5]);

  POINTERS_EQUAL(&nodes[5], dllist_first(&list));
  AreNodesLinked(&nodes[5], &nodes[2]);
  CHECK_EQUAL(size + 1u, dllist_size(&list));
}

///@}

/// @name dllist_push_back()
///@{

/// \Given an empty doubly-linked list and an initialized node
///
/// \When dllist_push_back() is called with a pointer to the node
///
/// \Then the node is added to the list, the list is no longer empty
TEST(Util_Dllist, DllistPushBack_Empty) {
  dllist_push_back(&list, &nodes[0]);

  CHECK_EQUAL(1u, dllist_size(&list));
  POINTERS_EQUAL(&nodes[0], dllist_last(&list));
}

/// \Given a doubly-linked list with multiple nodes and an initialized node
///
/// \When dllist_push_back() is called with a pointer to the node
///
/// \Then the node is added to the list as the new last node, size of the list
///       is increased by 1
TEST(Util_Dllist, DllistPushBack_ManyAdded) {
  dllist_push_back(&list, &nodes[0]);
  dllist_push_back(&list, &nodes[1]);
  dllist_push_back(&list, &nodes[2]);
  const size_t size = dllist_size(&list);

  dllist_push_back(&list, &nodes[5]);

  POINTERS_EQUAL(&nodes[5], dllist_last(&list));
  AreNodesLinked(&nodes[2], &nodes[5]);
  CHECK_EQUAL(size + 1u, dllist_size(&list));
}

///@}

/// @name dllist_pop_front()
///@{

/// \Given an empty doubly-linked list
///
/// \When dllist_pop_front() is called
///
/// \Then a null pointer is returned, nothing is changed
TEST(Util_Dllist, DllistPopFront_Empty) {
  POINTERS_EQUAL(nullptr, dllist_pop_front(&list));
  CHECK_EQUAL(0u, dllist_size(&list));
}

/// \Given a doubly-linked list with one node
///
/// \When dllist_pop_front() is called
///
/// \Then a pointer to the only node is returned, this node is removed from the
///       list and the list is empty
TEST(Util_Dllist, DllistPopFront_OneAdded) {
  dllist_push_front(&list, &nodes[0]);

  POINTERS_EQUAL(&nodes[0], dllist_pop_front(&list));
  CHECK_EQUAL(0u, dllist_size(&list));
}

/// \Given a doubly-linked list with multiple nodes
///
/// \When dllist_pop_front() is called
///
/// \Then a pointer to the first node is returned, this node is removed from the
///       list and size of the list is decreased by 1
TEST(Util_Dllist, DllistPopFront_ManyAdded) {
  dllist_push_front(&list, &nodes[0]);
  dllist_push_front(&list, &nodes[1]);
  dllist_push_front(&list, &nodes[2]);

  POINTERS_EQUAL(&nodes[2], dllist_pop_front(&list));
  CHECK_EQUAL(2u, dllist_size(&list));
}

///@}

/// @name dllist_pop_back()
///@{

/// \Given an empty doubly-linked list
///
/// \When dllist_pop_back() is called
///
/// \Then a null pointer is returned, nothing is changed
TEST(Util_Dllist, DllistPopBack_Empty) {
  POINTERS_EQUAL(nullptr, dllist_pop_back(&list));
  CHECK_EQUAL(0u, dllist_size(&list));
}

/// \Given a doubly-linked list with one node
///
/// \When dllist_pop_back() is called
///
/// \Then a pointer to the only node is returned, this node is removed from the
///       list and the list is empty
TEST(Util_Dllist, DllistPopBack_OneAdded) {
  dllist_push_front(&list, &nodes[0]);

  POINTERS_EQUAL(&nodes[0], dllist_pop_back(&list));
  CHECK_EQUAL(0u, dllist_size(&list));
}

/// \Given a doubly-linked list with multiple nodes
///
/// \When dllist_pop_back() is called
///
/// \Then a pointer to the last node is returned, this node is removed from the
///       list and size of the list is decreased by 1
TEST(Util_Dllist, DllistPopBack_ManyAdded) {
  dllist_push_front(&list, &nodes[0]);
  dllist_push_front(&list, &nodes[1]);
  dllist_push_front(&list, &nodes[2]);

  POINTERS_EQUAL(&nodes[0], dllist_pop_back(&list));
  CHECK_EQUAL(2u, dllist_size(&list));
}

///@}

/// @name dllist_insert_after()
///@{

/// \Given a doubly-linked list with two nodes and an initialized node
///
/// \When dllist_insert_after() is called with a pointer to the first node in
///       the list and a pointer to the initialized node
///
/// \Then the initialized node is inserted into the list between the first and
///       the second nodes of the list, size of the list is increased by 1
TEST(Util_Dllist, DllistInsertAfter_Middle) {
  dllist_push_back(&list, &nodes[0]);
  dllist_push_back(&list, &nodes[2]);

  dllist_insert_after(&list, &nodes[0], &nodes[1]);

  AreNodesLinked(&nodes[0], &nodes[1]);
  AreNodesLinked(&nodes[1], &nodes[2]);
  CHECK_EQUAL(3u, dllist_size(&list));
}

/// \Given a doubly-linked list with multiple nodes and an initialized node
///
/// \When dllist_insert_after() is called with a pointer to the last node in
///       the list and a pointer to the initialized node
///
/// \Then the initialized node is inserted into the list after the last node of
///       the list, becoming the new last node of the list, size of the list
///       is increased by 1
TEST(Util_Dllist, DllistInsertAfter_Last) {
  dllist_push_back(&list, &nodes[0]);
  dllist_push_back(&list, &nodes[1]);

  dllist_insert_after(&list, &nodes[1], &nodes[2]);

  AreNodesLinked(&nodes[1], &nodes[2]);
  CHECK_EQUAL(3u, dllist_size(&list));
  POINTERS_EQUAL(&nodes[2], dllist_last(&list));
}

///@}

/// @name dllist_insert_before()
///@{

/// \Given a doubly-linked list with two nodes and an initialized node
///
/// \When dllist_insert_before() is called with a pointer to the second node in
///       the list and a pointer to the initialized node
///
/// \Then the initialized node is inserted into the list between the first and
///       the second nodes of the list, size of the list is increased by 1
TEST(Util_Dllist, DllistInsertBefore_Middle) {
  dllist_push_back(&list, &nodes[0]);
  dllist_push_back(&list, &nodes[2]);

  dllist_insert_before(&list, &nodes[2], &nodes[1]);

  AreNodesLinked(&nodes[0], &nodes[1]);
  AreNodesLinked(&nodes[1], &nodes[2]);
  CHECK_EQUAL(3u, dllist_size(&list));
}

/// \Given a doubly-linked list with multiple nodes and an initialized node
///
/// \When dllist_insert_before() is called with a pointer to the first node in
///       the list and a pointer to the initialized node
///
/// \Then the initialized node is inserted into the list before the first node
///       of the list, becoming the new first node of the list, size of the list
///       is increased by 1
TEST(Util_Dllist, DllistInsertBefore_FirstNode) {
  dllist_push_back(&list, &nodes[1]);
  dllist_push_back(&list, &nodes[2]);

  dllist_insert_before(&list, &nodes[1], &nodes[0]);

  AreNodesLinked(&nodes[0], &nodes[1]);
  CHECK_EQUAL(3u, dllist_size(&list));
}

///@}

/// @name dllist_remove()
///@{

/// \Given a doubly-linked list with multiple nodes
///
/// \When dllist_remove() is called with a pointer to the first node
///
/// \Then the original first node is removed from the list, its successor is the
///       new first node of the list, size of the list is decreased by 1
TEST(Util_Dllist, DllistRemove_First) {
  dllist_push_back(&list, &nodes[0]);
  dllist_push_back(&list, &nodes[1]);

  dllist_remove(&list, &nodes[0]);

  POINTERS_EQUAL(&nodes[1], dllist_first(&list));
  CHECK_EQUAL(1u, dllist_size(&list));
}

/// \Given a doubly-linked list with multiple nodes
///
/// \When dllist_remove() is called with a pointer to the last node
///
/// \Then the original last node is removed from the list, its predecessor is
///       the new last node of the list, size of the list is decreased by 1
TEST(Util_Dllist, DllistRemove_Last) {
  dllist_push_back(&list, &nodes[0]);
  dllist_push_back(&list, &nodes[1]);

  dllist_remove(&list, &nodes[1]);

  POINTERS_EQUAL(&nodes[0], dllist_first(&list));
  CHECK_EQUAL(1u, dllist_size(&list));
}

/// \Given a doubly-linked list with multiple nodes
///
/// \When dllist_remove() is called with a pointer to one of the middle nodes
///
/// \Then the requested node is removed from the list, its predecessor and
///       successor are linked directly, size of the list is decreased by 1
TEST(Util_Dllist, DllistRemove_Middle) {
  dllist_push_back(&list, &nodes[0]);
  dllist_push_back(&list, &nodes[1]);
  dllist_push_back(&list, &nodes[2]);

  dllist_remove(&list, &nodes[1]);

  AreNodesLinked(&nodes[0], &nodes[2]);
  POINTERS_EQUAL(&nodes[0], dllist_first(&list));
  POINTERS_EQUAL(&nodes[2], dllist_last(&list));
  CHECK_EQUAL(2u, dllist_size(&list));
}

///@}

/// @name dllist_append()
///@{

/// \Given two empty doubly-linked lists
///
/// \When dllist_append() is called with pointers to both lists
///
/// \Then first pointer to list is returned, nothing is changed
TEST(Util_Dllist, DllistAppend_SrcEmptyDstEmpty) {
  dllist src;
  dllist_init(&src);

  POINTERS_EQUAL(&list, dllist_append(&list, &src));

  CHECK_EQUAL(0u, dllist_size(&list));
}

/// \Given two doubly-linked lists, one empty, the other with a single node
///
/// \When dllist_remove() is called with two pointers, first to the empty list,
///       second to the non-empty one
///
/// \Then first pointer to list is returned, the originally empty list contains
///       the only node from the other list, the other list is empty
TEST(Util_Dllist, DllistAppend_SrcOneDstEmpty) {
  dllist src;
  dllist_init(&src);
  dllist_push_back(&src, &nodes[0]);

  POINTERS_EQUAL(&list, dllist_append(&list, &src));

  CHECK_EQUAL(1u, dllist_size(&list));
  POINTERS_EQUAL(&nodes[0], dllist_first(&list));
  CHECK_EQUAL(1, dllist_empty(&src));
}

/// \Given two doubly-linked lists, one empty, the other with multiple nodes
///
/// \When dllist_append() is called with two pointers, first to the empty list,
///       second to the non-empty one
///
/// \Then first pointer to list is returned, the originally empty list contains
///       all nodes from the other list in the same order, the other list is
///       empty
TEST(Util_Dllist, DllistAppend_SrcManyDstEmpty) {
  dllist src;
  dllist_init(&src);
  dllist_push_back(&src, &nodes[0]);
  dllist_push_back(&src, &nodes[1]);

  POINTERS_EQUAL(&list, dllist_append(&list, &src));

  CHECK_EQUAL(2u, dllist_size(&list));
  POINTERS_EQUAL(&nodes[0], dllist_first(&list));
  CHECK_EQUAL(1, dllist_empty(&src));
}

/// \Given two doubly-linked lists, both with multiple nodes
///
/// \When dllist_append() is called with pointers to both lists
///
/// \Then first pointer to list is returned, this list contains all nodes from
///       the other list in the same order placed right after the original nodes
///       of this list, the other list is empty
TEST(Util_Dllist, DllistAppend_SrcManyDstMany) {
  dllist src;
  dllist_init(&src);

  dllist_push_back(&src, &nodes[0]);
  dllist_push_back(&src, &nodes[1]);
  dllist_push_back(&list, &nodes[2]);
  dllist_push_back(&list, &nodes[3]);

  POINTERS_EQUAL(&list, dllist_append(&list, &src));

  CHECK_EQUAL(4u, dllist_size(&list));
  CHECK_EQUAL(1, dllist_empty(&src));
  POINTERS_EQUAL(&nodes[2], dllist_first(&list));
  POINTERS_EQUAL(&nodes[1], dllist_last(&list));
  AreNodesLinked(&nodes[2], &nodes[3]);
  AreNodesLinked(&nodes[3], &nodes[0]);
  AreNodesLinked(&nodes[0], &nodes[1]);
}

///@}

/// @name dllist_contains()
///@{

/// \Given a doubly-linked list
///
/// \When dllist_contains() is called with a null pointer
///
/// \Then 0 is returned
TEST(Util_Dllist, DllistContains_EmptyListContainsNull) {
  CHECK_EQUAL(0, dllist_contains(&list, nullptr));
}

/// \Given an empty doubly-linked list
///
/// \When dllist_contains() is called with a pointer to a node not present in
///       the list
///
/// \Then 0 is returned
TEST(Util_Dllist, DllistContains_EmptyListContainsNotNull) {
  CHECK_EQUAL(0, dllist_contains(&list, &nodes[0]));
}

/// \Given a doubly-linked list with one node
///
/// \When dllist_contains() is called with a pointer to the node in the list
///
/// \Then 1 is returned
TEST(Util_Dllist, DllistContains_ListWithOneContains) {
  dllist_push_back(&list, &nodes[0]);

  CHECK_EQUAL(1, dllist_contains(&list, &nodes[0]));
}

/// \Given a doubly-linked list with one node
///
/// \When dllist_contains() is called with a pointer to a node not present in
///       the list
///
/// \Then 0 is returned
TEST(Util_Dllist, DllistContains_ListWithOneDoesNotContain) {
  dllist_push_back(&list, &nodes[0]);

  CHECK_EQUAL(0, dllist_contains(&list, &nodes[1]));
}

/// \Given a doubly-linked list with two nodes
///
/// \When dllist_contains() is called with a pointer to one of the nodes in the
///       list
///
/// \Then 1 is returned
TEST(Util_Dllist, DllistContains_ListWithManyContains) {
  dllist_push_back(&list, &nodes[0]);
  dllist_push_back(&list, &nodes[1]);

  CHECK_EQUAL(1, dllist_contains(&list, &nodes[1]));
}

/// \Given a doubly-linked list with two nodes
///
/// \When dllist_contains() is called with a pointer to a node not present in
///       the list
///
/// \Then 0 is returned
TEST(Util_Dllist, DllistContains_ListWithManyDoesNotContain) {
  dllist_push_back(&list, &nodes[0]);
  dllist_push_back(&list, &nodes[1]);

  CHECK_EQUAL(0, dllist_contains(&list, &nodes[3]));
}

///@}

/// @name dlnode_foreach()
///@{

/// \Given N/A
///
/// \When dlnode_foreach() is used with a null pointer
///
/// \Then no loop iterations are performed
TEST(Util_Dllist, DlnodeForeach_Null) {
  unsigned node_counter = 0;

  dlnode_foreach(nullptr, current_node) { ++node_counter; }

  CHECK_EQUAL(0u, node_counter);
}

/// \Given a doubly-linked list with multiple nodes
///
/// \When dlnode_foreach() is used with a pointer to a middle node of the list
///
/// \Then body of the loop is executed for requested node and all its successors
TEST(Util_Dllist, DlnodeForeach_MiddleNode) {
  dllist_push_back(&list, &nodes[0]);
  dllist_push_back(&list, &nodes[1]);
  dllist_push_back(&list, &nodes[2]);
  dllist_push_back(&list, &nodes[3]);
  dllist_push_back(&list, &nodes[4]);
  unsigned node_counter = 0;

  unsigned i = 2;
  dlnode_foreach(&nodes[2], current_node) {
    POINTERS_EQUAL(&nodes[i], current_node);

    ++node_counter;
    ++i;
  }

  CHECK_EQUAL(3u, node_counter);
}

///@}

/// @name dllist_foreach()
///@{

/// \Given an empty doubly-linked list
///
/// \When dllist_foreach() is used
///
/// \Then no loop iterations are performed
TEST(Util_Dllist, DllistForeach_Empty) {
  unsigned node_counter = 0;

  dllist_foreach(&list, current_node) { ++node_counter; }

  CHECK_EQUAL(0u, node_counter);
}

/// \Given a doubly-linked list with multiple nodes
///
/// \When dllist_foreach() is used
///
/// \Then body of the loop is executed for all nodes from the list in order from
///       the first to the last
TEST(Util_Dllist, DllistForeach_ManyAdded) {
  dllist_push_back(&list, &nodes[0]);
  dllist_push_back(&list, &nodes[1]);
  dllist_push_back(&list, &nodes[2]);
  dllist_push_back(&list, &nodes[3]);
  dllist_push_back(&list, &nodes[4]);
  unsigned node_counter = 0;

  dllist_foreach(&list, current_node) {
    POINTERS_EQUAL(&nodes[node_counter], current_node);
    ++node_counter;
  }

  CHECK_EQUAL(dllist_size(&list), node_counter);
}

/// \Given a doubly-linked list with multiple nodes
///
/// \When dllist_foreach() is used and one of the nodes is removed
///
/// \Then body of the loop is executed for all nodes from the list and the
///       removed node is not present in the list
TEST(Util_Dllist, DllistForeach_ManyAddedRemoveCurrent) {
  dllist_push_back(&list, &nodes[0]);
  dllist_push_back(&list, &nodes[1]);
  dllist_push_back(&list, &nodes[2]);
  dllist_push_back(&list, &nodes[3]);

  unsigned iteration_counter = 0;
  dllist_foreach(&list, current_node) {
    if (current_node == &nodes[1]) dllist_remove(&list, current_node);

    ++iteration_counter;
  }

  CHECK_EQUAL(4u, iteration_counter);
  CHECK_EQUAL(3u, dllist_size(&list));
}

///@}
