
#include <CppUTest/TestHarness.h>
#include <lely/util/sllist.h>

TEST_GROUP(UtilSllistInitGroup) {};

TEST(UtilSllistInitGroup, init) {
	struct sllist List;

	sllist_init(&List);

	CHECK(List.first == nullptr);
}

static void slnode_init_many(const int how_many, struct slnode* nodes) {
	for(int i = 0; i < how_many; i++) {
		slnode_init(nodes + i);
	}
}

#define NUM_OF_NODES 10
TEST_GROUP(UtilSllistGroup) {
	struct sllist List;
	struct slnode nodes[NUM_OF_NODES];

	void setup() {
		sllist_init(&List);
		slnode_init_many(NUM_OF_NODES, nodes);
	}
};

static struct slnode* get_node(const int i, const struct slnode* nodes) {
	return (struct slnode*)(nodes + i);
}

static void fill_list(struct sllist* list, struct slnode* nodes, const int how_many) {
	for(int i = 0; i < how_many; i++) {
		struct slnode* node_ptr = get_node(i, nodes);
		slnode_init(node_ptr);
		sllist_push_back(list, node_ptr);
	}
}

TEST(UtilSllistGroup, Slnode_init) {
	struct slnode node;
	slnode_init(&node);
	POINTERS_EQUAL(nullptr, node.next);
}

TEST(UtilSllistGroup, Sllist_empty_afterCreation) {
	LONGS_EQUAL(1, sllist_empty(&List));
}

TEST(UtilSllistGroup, Sllist_empty_notEmptyWhenElementAdded) {
	struct slnode* node_ptr = get_node(0, nodes);
	sllist_push_front(&List, node_ptr);

	LONGS_EQUAL(0, sllist_empty(&List));
}

TEST(UtilSllistGroup, Sllist_empty_notEmptyWhenManyElementsAdded) {
	fill_list(&List, nodes, 3);

	LONGS_EQUAL(0, sllist_empty(&List));
}

TEST(UtilSllistGroup, Sllist_size_zeroWhenCreated) {
	LONGS_EQUAL(0, sllist_size(&List));
}

TEST(UtilSllistGroup, Sllist_size_oneElementAdded) {
	struct slnode* node_ptr = get_node(0, nodes);
	sllist_push_front(&List, node_ptr);

	LONGS_EQUAL(1, sllist_size(&List));
}

TEST(UtilSllistGroup, Sllist_size_manyAdded) {
	fill_list(&List, nodes, 4);

	LONGS_EQUAL(4, sllist_size(&List));
}

TEST(UtilSllistGroup, Sllist_push_front_whenEmpty) {
	struct slnode* node_ptr = get_node(0, nodes);

	sllist_push_front(&List, node_ptr);

	POINTERS_EQUAL(node_ptr, List.first);
}

TEST(UtilSllistGroup, Sllist_push_front_addMany) {
	struct slnode* node_ptrs[] = {
		get_node(0, nodes),
		get_node(1, nodes)
	};

	sllist_push_front(&List, node_ptrs[0]);
	sllist_push_front(&List, node_ptrs[1]);

	POINTERS_EQUAL(node_ptrs[1], List.first);
}

TEST(UtilSllistGroup, Sllist_push_front_sameAddedFewTimes) {
	struct slnode* node_ptr = get_node(0, nodes);

	sllist_push_front(&List, node_ptr);
	sllist_push_front(&List, node_ptr);
	sllist_push_front(&List, node_ptr);

	POINTERS_EQUAL(node_ptr, List.first);
}

TEST(UtilSllistGroup, Sllist_push_back_whenEmpty) {
	struct slnode node;
	slnode_init(&node);

	sllist_push_back(&List, &node);

	POINTERS_EQUAL(&node, List.first);
}

TEST(UtilSllistGroup, Sllist_push_back_addMany) {
	struct slnode* node_ptrs[] = {
		get_node(0, nodes),
		get_node(1, nodes),
	};

	sllist_push_back(&List, node_ptrs[0]);
	sllist_push_back(&List, node_ptrs[1]);

	POINTERS_EQUAL(node_ptrs[0], List.first);
	POINTERS_EQUAL(node_ptrs[1], List.first->next);
}

TEST(UtilSllistGroup, Sllist_pop_front_whenEmpty) {
	POINTERS_EQUAL(nullptr, sllist_pop_front(&List));
}

TEST(UtilSllistGroup, Sllist_pop_front_oneAdded) {
	fill_list(&List, nodes, 1);

	POINTERS_EQUAL(get_node(0, nodes), sllist_pop_front(&List));
}

TEST(UtilSllistGroup, Sllist_pop_front_manyAdded) {
	fill_list(&List, nodes, 8);

	POINTERS_EQUAL(get_node(0, nodes), sllist_pop_front(&List));
	POINTERS_EQUAL(get_node(1, nodes), sllist_pop_front(&List));
}

TEST(UtilSllistGroup, Sllist_pop_back_whenEmpty) {
	POINTERS_EQUAL(nullptr, sllist_pop_back(&List));
}

TEST(UtilSllistGroup, Sllist_pop_back_oneAdded) {
	fill_list(&List, nodes, 1);

	POINTERS_EQUAL(get_node(0, nodes), sllist_pop_back(&List));
}

TEST(UtilSllistGroup, Sllist_pop_back_manyAdded) {
	fill_list(&List, nodes, 8);

	POINTERS_EQUAL(get_node(7, nodes), sllist_pop_back(&List));
	POINTERS_EQUAL(get_node(6, nodes), sllist_pop_back(&List));
}

TEST(UtilSllistGroup, Sllist_pop_remove_nullptr) {
	POINTERS_EQUAL(nullptr, sllist_remove(&List, nullptr));
}

TEST(UtilSllistGroup, Sllist_pop_remove_empty) {
	struct slnode* node_ptr = get_node(0, nodes);

	POINTERS_EQUAL(nullptr, sllist_remove(&List, node_ptr));
}

TEST(UtilSllistGroup, Sllist_pop_remove_oneAdded) {
	fill_list(&List, nodes, 1);

	POINTERS_EQUAL(get_node(0, nodes), sllist_remove(&List, get_node(0, nodes)));
}

TEST(UtilSllistGroup, Sllist_pop_remove_manyAdded) {
	fill_list(&List, nodes, 2);

	POINTERS_EQUAL(get_node(0, nodes), sllist_remove(&List, get_node(0, nodes)));
	POINTERS_EQUAL(get_node(1, nodes), sllist_remove(&List, get_node(1, nodes)));
}

TEST(UtilSllistGroup, Sllist_append_bothEmpty) {
	struct sllist List2;
	sllist_init(&List2);
	
	POINTERS_EQUAL(&List, sllist_append(&List, &List2));
	LONGS_EQUAL(0, sllist_size(&List2));
}

TEST(UtilSllistGroup, Sllist_append_dstEmpty) {
	struct sllist List2;
	sllist_init(&List2);
	fill_list(&List2, nodes, 1);
	
	POINTERS_EQUAL(&List, sllist_append(&List, &List2));
	LONGS_EQUAL(0, sllist_size(&List2));
	LONGS_EQUAL(1, sllist_size(&List));
}

TEST(UtilSllistGroup, Sllist_append_srcEmpty) {
	struct sllist List2;
	sllist_init(&List2);
	fill_list(&List, nodes, 1);
	
	POINTERS_EQUAL(&List, sllist_append(&List, &List2));
	LONGS_EQUAL(0, sllist_size(&List2));
	LONGS_EQUAL(1, sllist_size(&List));
}

TEST(UtilSllistGroup, Sllist_append_srcMany) {
	struct sllist List2;
	sllist_init(&List2);
	fill_list(&List2, nodes, 2);
	
	POINTERS_EQUAL(&List, sllist_append(&List, &List2));
	LONGS_EQUAL(0, sllist_size(&List2));
	LONGS_EQUAL(2, sllist_size(&List));
}

TEST(UtilSllistGroup, Sllist_first_empty) {
	POINTERS_EQUAL(nullptr, sllist_first(&List));
}

TEST(UtilSllistGroup, Sllist_first_oneAdded) {
	fill_list(&List, nodes, 1);

	POINTERS_EQUAL(get_node(0, nodes), sllist_first(&List));
}

TEST(UtilSllistGroup, Sllist_first_manyAdded) {
	fill_list(&List, nodes, 2);

	POINTERS_EQUAL(get_node(0, nodes), sllist_first(&List));
}

TEST(UtilSllistGroup, Sllist_last_empty) {
	POINTERS_EQUAL(nullptr, sllist_last(&List));
}

TEST(UtilSllistGroup, Sllist_last_oneAdded) {
	fill_list(&List, nodes, 1);

	POINTERS_EQUAL(get_node(0, nodes), sllist_last(&List));
}

TEST(UtilSllistGroup, Sllist_last_manyAdded) {
	fill_list(&List, nodes, 2);

	POINTERS_EQUAL(get_node(1, nodes), sllist_last(&List));
}

TEST(UtilSllistGroup, Sllist_foreach_empty) {
	struct slnode* node_ptr = get_node(0, nodes);
	sllist_foreach(&List, node)
		node_ptr = node;
	POINTERS_EQUAL(get_node(0, nodes), node_ptr);
}

TEST(UtilSllistGroup, Sllist_foreach_onlyHead) {
	fill_list(&List, nodes, 1);

	struct slnode* node_ptr = nullptr;
	sllist_foreach(&List, node)
		node_ptr = node;

	POINTERS_EQUAL(get_node(0, nodes), node_ptr);
}

TEST(UtilSllistGroup, Sllist_foreach_multipleElements) {
	fill_list(&List, nodes, 2);

	struct slnode* node_ptr = nullptr;
	sllist_foreach(&List, node)
		node_ptr = node;

	POINTERS_EQUAL(get_node(1, nodes), node_ptr);
}
