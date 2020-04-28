
#include <CppUTest/TestHarness.h>
#include <lely/util/pheap.h>

static int pheap_cmp_ints(const void* p1, const void* p2) {
	if ( nullptr == p1 || nullptr == p2)
		return -2;
	if ((int)(*(int*)p1) > (int)(*(int*)p2))
		return 1;
	else if ((int)(*(int*)p1) == (int)(*(int*)p2))
		return 0;
	else
		return -1;
}

TEST_GROUP(StaticPheapCmpIntsGroup) {};

TEST(StaticPheapCmpIntsGroup, compareEqual) {
	int a = 2;
	int b = 2;

	LONGS_EQUAL(0, pheap_cmp_ints(&a, &b));
}

TEST(StaticPheapCmpIntsGroup, compareFirstLess) {
	int a = 1;
	int b = 2;
	
	LONGS_EQUAL(-1, pheap_cmp_ints(&a, &b));
}

TEST(StaticPheapCmpIntsGroup, compareFirstGreater) {
	int a = 3;
	int b = 2;
	
	LONGS_EQUAL(1, pheap_cmp_ints(&a, &b));
}

TEST(StaticPheapCmpIntsGroup, compareNullptr) {
	int a = 4;

	LONGS_EQUAL(-2, pheap_cmp_ints(&a, nullptr));
}

TEST_GROUP(UtilPheapInitGroup) {};

TEST(UtilPheapInitGroup, Pheap_emptyWhenInitialized) {
	struct pheap Pheap = PHEAP_INIT;

	LONGS_EQUAL(0, Pheap.num_nodes);
}

static void pnode_nullify(struct pnode* node) {
	*node = PNODE_INIT;
}

#define NUM_OF_NODES 10
TEST_GROUP(UtilPheapGroup){
	struct pheap Pheap;

	struct pnode node;
	int key = 0x0000FF;

	struct pnode nodes[NUM_OF_NODES];
	int keys[NUM_OF_NODES] = { 0x0000001,
				   0x0000011,
				   0x0001111,
				   0x0000111,
				   0x0011111,
				   0x0111111,
				   0x1111111,
				   0x111111F,
				   0x11111FF,
				   0x1111FFF };

	void setup() {
		for(int i = 0; i < NUM_OF_NODES; i++) {
			pnode_nullify(&(nodes[i]));
		}
		pheap_init(&Pheap, pheap_cmp_ints);
	}
};

static struct pnode* get_node_ptr(const int i, const struct pnode* nodes) {
	return (struct pnode*)(nodes + i);
}

static void* get_key_ptr(const int i, const void* keys) {
	return (void*)((long long int)keys + i);
}

static void node_init_many(int num_of_nodes_to_init, const struct pnode* nodes, const void* keys) {
	for(int i = 0; i < num_of_nodes_to_init; i++) {
		struct pnode* node_ptr = get_node_ptr(i, nodes);
		void* key_ptr = get_key_ptr(i, keys);		
		pnode_init(node_ptr, key_ptr);
	}
}

TEST(UtilPheapGroup, Pnode_init) {
	struct pnode* node_ptr = get_node_ptr(0, nodes);
	void* key_ptr = get_key_ptr(0, keys);
	
	pnode_init(node_ptr, key_ptr);

	int value_of_node = *((int*)node_ptr->key);
	LONGS_EQUAL(keys[0], value_of_node);
}

TEST(UtilPheapGroup, Pnode_next) {
	struct pnode* node_ptr = get_node_ptr(0, nodes);
	struct pnode* next_node_ptr = get_node_ptr(1, nodes);
	node_init_many(2, nodes, keys);

	node_ptr->next = next_node_ptr;

	POINTERS_EQUAL(next_node_ptr, pnode_next(node_ptr));
}

TEST(UtilPheapGroup, Pheap_init) {
	FUNCTIONPOINTERS_EQUAL(pheap_cmp_ints, Pheap.cmp);
	POINTERS_EQUAL(nullptr, Pheap.root);
	LONGS_EQUAL(0, Pheap.num_nodes);
}

TEST(UtilPheapGroup, Pheap_empty_isEmpty) {
	LONGS_EQUAL(1, pheap_empty(&Pheap));
}

TEST(UtilPheapGroup, Pheap_empty_isNotEmpty) {
	pnode_init(&node, (void*)&key);
	pheap_insert(&Pheap, &node);

	LONGS_EQUAL(0, pheap_empty(&Pheap));
}

TEST(UtilPheapGroup, Pheap_size_isEmpty) {
	LONGS_EQUAL(0, pheap_size(&Pheap));
}

TEST(UtilPheapGroup, Pheap_size_hasOneElement) {
	pnode_init(&node, (void*)&key);
	pheap_insert(&Pheap, &node);

	LONGS_EQUAL(1, pheap_size(&Pheap));
}

TEST(UtilPheapGroup, Pheap_size_hasMultipleElements) {
	struct pnode* node_ptrs[] = {
		get_node_ptr(0, nodes),
		get_node_ptr(1, nodes)
	};

	pheap_insert(&Pheap, node_ptrs[0]);
	pheap_insert(&Pheap, node_ptrs[1]);
	
	LONGS_EQUAL(2, pheap_size(&Pheap));
}

TEST(UtilPheapGroup, Pheap_insert_whenEmpty) {
	pnode_init(&node, (void*)&key);

	pheap_insert(&Pheap, &node);

	LONGS_EQUAL(1, pheap_size(&Pheap));
}

TEST(UtilPheapGroup, Pheap_remove_whenEmpty) {
	struct pnode* node_ptr = get_node_ptr(0, nodes);

	// note: when pheap_remove is called, 
	// heap->num_nodes is decremented even 
	// when there were no elements removed
	pheap_remove(&Pheap, node_ptr);

	LONGS_EQUAL(-1, pheap_size(&Pheap));
}

TEST(UtilPheapGroup, Pheap_remove_nodeIsParentAndIsOnlyElement) {
	struct pnode* node_ptr = get_node_ptr(0, nodes);

	pheap_insert(&Pheap, node_ptr);
	pheap_remove(&Pheap, node_ptr);

	LONGS_EQUAL(0, pheap_size(&Pheap));
}

TEST(UtilPheapGroup, Pheap_remove_nodeIsNotParentAndIsNotOnlyElement) {
	struct pnode* node_ptr = get_node_ptr(0, nodes);
	struct pnode* next_node_ptr = get_node_ptr(1, nodes);

	pheap_insert(&Pheap, node_ptr);
	pheap_insert(&Pheap, next_node_ptr);
	pheap_remove(&Pheap, node_ptr);

	LONGS_EQUAL(1, pheap_size(&Pheap));
}

TEST(UtilPheapGroup, Pheap_remove_nodeIsParentAndIsNotOnlyElement) {
	struct pnode* node_ptr = get_node_ptr(0, nodes);
	struct pnode* next_node_ptr = get_node_ptr(1, nodes);

	pheap_insert(&Pheap, node_ptr);
	pheap_insert(&Pheap, next_node_ptr);
	pheap_remove(&Pheap, next_node_ptr);

	LONGS_EQUAL(1, pheap_size(&Pheap));
}

static void fill_heap(struct pheap* heap, const int how_many, struct pnode* nodes, void* keys) {
	node_init_many(how_many, nodes, keys);
	for(int i = 0; i < how_many; i++) {
		pheap_insert(heap, get_node_ptr(i, nodes));
	}
}

TEST(UtilPheapGroup, Pheap_find_whenEmpty) {
	void* key_ptr = get_key_ptr(0, keys);

	POINTERS_EQUAL(nullptr, pheap_find(&Pheap, key_ptr));
}

TEST(UtilPheapGroup, Pheap_find_whenNotEmpty) {
	fill_heap(&Pheap, 2, nodes, keys);
	void* key_ptrs[] = {
		get_key_ptr(0, keys),
		get_key_ptr(1, keys)
	};

	POINTERS_EQUAL(&(nodes[0]), pheap_find(&Pheap, key_ptrs[0]));
	POINTERS_EQUAL(&(nodes[1]), pheap_find(&Pheap, key_ptrs[1]));
}

TEST(UtilPheapGroup, Pheap_find_findNotPresentWhenMultipleElements) {
	fill_heap(&Pheap, 3, nodes, keys);
	
	POINTERS_EQUAL(nullptr, pheap_find(&Pheap, get_key_ptr(3, keys)));
}

TEST(UtilPheapGroup, Pheap_first) {
	POINTERS_EQUAL(Pheap.root, pheap_first(&Pheap));
}

TEST(UtilPheapGroup, Pnode_foreach_emptyHeap) {
	int node_counter = 0;

	pnode_foreach(Pheap.root, node)
		node_counter++;
	
	LONGS_EQUAL(0, node_counter);
}

TEST(UtilPheapGroup, Pnode_foreach_onlyHead) {
	int node_counter = 0;
	fill_heap(&Pheap, 1, nodes, keys);

	pnode_foreach(Pheap.root, node)
		node_counter++;
	
	LONGS_EQUAL(1, node_counter);
}

TEST(UtilPheapGroup, Pnode_foreach_multipleElements) {
	int node_counter = 0;
	fill_heap(&Pheap, 2, nodes, keys);
	
	pnode_foreach(Pheap.root, node)
		node_counter++;

	LONGS_EQUAL(2, node_counter);
}

TEST(UtilPheapGroup, Pheap_foreach_emptyHeap) {
	int node_counter = 0;

	pheap_foreach(&Pheap, node)
		node_counter++;
	
	LONGS_EQUAL(0, node_counter);
}

TEST(UtilPheapGroup, Pheap_foreach_onlyHead) {
	int node_counter = 0;
	fill_heap(&Pheap, 1, nodes, keys);

	pheap_foreach(&Pheap, node)
		node_counter++;
	
	LONGS_EQUAL(1, node_counter);
}

TEST(UtilPheapGroup, Pheap_foreach_multipleElements) {
	int node_counter = 0;
	fill_heap(&Pheap, 2, nodes, keys);

	pheap_foreach(&Pheap, node)
		node_counter++;

	LONGS_EQUAL(2, node_counter);
}
