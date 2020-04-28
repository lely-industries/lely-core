
#include <CppUTest/TestHarness.h>
#include <lely/util/bitset.h>

#undef INT_BIT
#undef CHAR_BIT
#define CHAR_BIT __CHAR_BIT__
#define INT_BIT (sizeof(int) * CHAR_BIT)

typedef enum {
	Bitset_state_ZERO = 0,
	Bitset_state_SET = 1,
} Bitset_state;

TEST_GROUP(UtilBitsetGroup){
	static const unsigned int bitsize = 287;
	static const unsigned int bytes = MAX(0, (bitsize + INT_BIT - 1) / INT_BIT)  * sizeof(unsigned int);
	unsigned int bits[bytes];
	struct bitset Bitset = {MAX(0, (bitsize + INT_BIT - 1) / INT_BIT), bits};
};

IGNORE_TEST(UtilBitsetGroup, Bitset_init) {
	// TODO
}

IGNORE_TEST(UtilBitsetGroup, Bitset_fini) {
	// TODO
}

TEST(UtilBitsetGroup, Bitset_size_inBits) {
	LONGS_EQUAL(Bitset.size * INT_BIT, bitset_size(&Bitset));
}

IGNORE_TEST(UtilBitsetGroup, Bitset_resize) {
	// TODO
}

static void add_set_bit(unsigned int* byte, unsigned int idx) {
	*byte = (*byte) | (0x1 << idx);
}

TEST(UtilBitsetGroup, Bitset_test_nonemptyReturnsState) {
	add_set_bit(&(Bitset.bits[0]), 1);
	add_set_bit(&(Bitset.bits[0]), 5);

	LONGS_EQUAL(Bitset_state_SET ,bitset_test(&Bitset, 1));
	LONGS_EQUAL(Bitset_state_SET, bitset_test(&Bitset, 5));
}

static void helper_set_bitset(unsigned int* const bitset, const unsigned int size) {
	for(unsigned int i = 0; i < size; i++) {
		bitset[i] = Bitset_state_SET;
	}
}

TEST(UtilBitsetGroup, Bitset_test_outOfBoundsReturnsZero) {
	helper_set_bitset(Bitset.bits, Bitset.size);
	LONGS_EQUAL(Bitset_state_SET, bitset_test(&Bitset, 0));

	LONGS_EQUAL(Bitset_state_ZERO, bitset_test(&Bitset, -1));
	LONGS_EQUAL(Bitset_state_ZERO, bitset_test(&Bitset, bitset_size(&Bitset)));
	LONGS_EQUAL(Bitset_state_ZERO, bitset_test(&Bitset, bitset_size(&Bitset) + 1));
}

TEST(UtilBitsetGroup, Bitset_set_correctIndex) {
	bitset_set(&Bitset, 0);
	int idx_in_mid = bitset_size(&Bitset)/2;
	bitset_set(&Bitset, idx_in_mid);

	LONGS_EQUAL(Bitset_state_SET, bitset_test(&Bitset, 0));
	LONGS_EQUAL(Bitset_state_SET, bitset_test(&Bitset, idx_in_mid));
}

static void check_all_states(struct bitset* set, Bitset_state expected_state) {
	LONGS_EQUAL(expected_state, bitset_test(set, 0));
	int last_idx = bitset_size(set) - 1;
	for(int i = 1; i < last_idx; i++) {
		LONGS_EQUAL(expected_state, bitset_test(set, i));
	}
	LONGS_EQUAL(expected_state, bitset_test(set, last_idx));
}

TEST(UtilBitsetGroup, Bitset_set_outOfBoundsIndex) {
	bitset_set(&Bitset, -1);
	bitset_set(&Bitset, bitset_size(&Bitset));
	bitset_set(&Bitset, bitset_size(&Bitset) + 1);

	check_all_states(&Bitset, Bitset_state_ZERO);
}

TEST(UtilBitsetGroup, Bitset_set_all) {
	bitset_set_all(&Bitset);

	check_all_states(&Bitset, Bitset_state_SET);
}

TEST(UtilBitsetGroup, Bitset_clr_correctIndex) {
	int last_idx = bitset_size(&Bitset) - 1;

	bitset_set_all(&Bitset);
	bitset_clr(&Bitset, 0);
	bitset_clr(&Bitset, 1);
	bitset_clr(&Bitset, last_idx);

	LONGS_EQUAL(Bitset_state_ZERO, bitset_test(&Bitset, 0));
	LONGS_EQUAL(Bitset_state_ZERO, bitset_test(&Bitset, 1));
	LONGS_EQUAL(Bitset_state_ZERO, bitset_test(&Bitset, last_idx));
}

TEST(UtilBitsetGroup, Bitset_clr_outOfBoundsIndex) {
	bitset_set_all(&Bitset);
	bitset_clr(&Bitset, -1);
	bitset_clr(&Bitset, bitset_size(&Bitset));
	bitset_clr(&Bitset, bitset_size(&Bitset) + 1);

	check_all_states(&Bitset, Bitset_state_SET);
}

TEST(UtilBitsetGroup, Bitset_clr_all) {
	bitset_clr_all(&Bitset);

	check_all_states(&Bitset, Bitset_state_ZERO);
}

TEST(UtilBitsetGroup, Bitset_compl) {
	bitset_clr_all(&Bitset);
	bitset_set(&Bitset, 0);
	bitset_set(&Bitset, 1);
	
	bitset_compl(&Bitset);

	LONGS_EQUAL(Bitset_state_ZERO, bitset_test(&Bitset, 0));
	LONGS_EQUAL(Bitset_state_ZERO, bitset_test(&Bitset, 1));
	LONGS_EQUAL(Bitset_state_SET, bitset_test(&Bitset, 2));
	LONGS_EQUAL(Bitset_state_SET, bitset_test(&Bitset, bitset_size(&Bitset) - 3));
	LONGS_EQUAL(Bitset_state_SET, bitset_test(&Bitset, bitset_size(&Bitset) - 2));
	LONGS_EQUAL(Bitset_state_SET, bitset_test(&Bitset, bitset_size(&Bitset) - 1));
}

TEST(UtilBitsetGroup, Bitset_ffs_allZero) {
	bitset_clr_all(&Bitset);
	LONGS_EQUAL(0, bitset_ffs(&Bitset));
}

TEST(UtilBitsetGroup, Bitset_ffs_firstSet) {
	bitset_clr_all(&Bitset);
	bitset_set(&Bitset, 0);

	LONGS_EQUAL(1, bitset_ffs(&Bitset));
}

TEST(UtilBitsetGroup, Bitset_ffs_lastSet) {
	int last_idx = bitset_size(&Bitset) - 1;
	bitset_clr_all(&Bitset);
	bitset_set(&Bitset, last_idx);

	LONGS_EQUAL(last_idx + 1, bitset_ffs(&Bitset));
}

TEST(UtilBitsetGroup, Bitset_ffs_middleSet) {
	int middle_idx = bitset_size(&Bitset)/2;
	bitset_clr_all(&Bitset);
	bitset_set(&Bitset, middle_idx);
	if(middle_idx + 4 < bitset_size(&Bitset))
		bitset_set(&Bitset, middle_idx + 4);

	LONGS_EQUAL(middle_idx + 1, bitset_ffs(&Bitset));
}

TEST(UtilBitsetGroup, Bitset_ffz_allSet) {
	bitset_set_all(&Bitset);
	LONGS_EQUAL(0, bitset_ffz(&Bitset));
}

TEST(UtilBitsetGroup, Bitset_ffz_firstZero) {
	bitset_set_all(&Bitset);
	bitset_clr(&Bitset, 0);

	LONGS_EQUAL(1, bitset_ffz(&Bitset));
}

TEST(UtilBitsetGroup, Bitset_ffz_lastZero) {
	int last_idx = bitset_size(&Bitset) - 1;
	bitset_set_all(&Bitset);
	bitset_clr(&Bitset, last_idx);

	LONGS_EQUAL(last_idx + 1, bitset_ffz(&Bitset));
}

TEST(UtilBitsetGroup, Bitset_ffz_middleZero) {
	int middle_idx = bitset_size(&Bitset)/2;
	bitset_set_all(&Bitset);
	bitset_clr(&Bitset, middle_idx);
	if(middle_idx + 4 < bitset_size(&Bitset))
		bitset_clr(&Bitset, middle_idx + 4);

	LONGS_EQUAL(middle_idx + 1, bitset_ffz(&Bitset));
}

TEST(UtilBitsetGroup, Bitset_fns_allZeros) {
	int set_idx = 10;
	bitset_clr_all(&Bitset);
	bitset_set(&Bitset, set_idx);

	LONGS_EQUAL(0, bitset_fns(&Bitset, set_idx + 1));
}

TEST(UtilBitsetGroup, Bitset_fns_nextIsSet) {
	int set_idx = 10;
	bitset_clr_all(&Bitset);
	bitset_set(&Bitset, set_idx);
	bitset_set(&Bitset, set_idx + 1);

	LONGS_EQUAL(set_idx + 2, bitset_fns(&Bitset, set_idx + 1));
}

TEST(UtilBitsetGroup, Bitset_fns_lastIsSet) {
	int set_idx = 10;
	int last_idx = bitset_size(&Bitset) - 1;
	bitset_clr_all(&Bitset);
	bitset_set(&Bitset, set_idx);
	bitset_set(&Bitset, last_idx);

	LONGS_EQUAL(last_idx + 1, bitset_fns(&Bitset, set_idx + 1));
}

TEST(UtilBitsetGroup, Bitset_fns_outOfBoundsIdx) {
	int last_idx = bitset_size(&Bitset) - 1;
	int idx_to_set = 0;
	bitset_set(&Bitset, idx_to_set);

	LONGS_EQUAL(0, bitset_fns(&Bitset, last_idx + 1));
	LONGS_EQUAL(idx_to_set + 1, bitset_fns(&Bitset, -1));
}

TEST(UtilBitsetGroup, Bitset_fnz_allSet) {
	int set_idx = 10;
	bitset_set_all(&Bitset);
	bitset_clr(&Bitset, set_idx);

	LONGS_EQUAL(0, bitset_fnz(&Bitset, set_idx + 1));
}

TEST(UtilBitsetGroup, Bitset_fnz_nextIsZero) {
	int set_idx = 10;
	bitset_set_all(&Bitset);
	bitset_clr(&Bitset, set_idx);
	bitset_clr(&Bitset, set_idx + 1);

	LONGS_EQUAL(set_idx + 2, bitset_fnz(&Bitset, set_idx + 1));
}

TEST(UtilBitsetGroup, Bitset_fnz_lastIsZero) {
	int set_idx = 10;
	int last_idx = bitset_size(&Bitset) - 1;
	bitset_set_all(&Bitset);
	bitset_clr(&Bitset, set_idx);
	bitset_clr(&Bitset, last_idx);

	LONGS_EQUAL(last_idx + 1, bitset_fnz(&Bitset, set_idx + 1));
}

TEST(UtilBitsetGroup, Bitset_fnz_outOfBoundsIdx) {
	int last_idx = bitset_size(&Bitset) - 1;
	int idx_to_clr = 0;
	bitset_clr(&Bitset, idx_to_clr);

	LONGS_EQUAL(0, bitset_fnz(&Bitset, last_idx + 1));
	LONGS_EQUAL(idx_to_clr + 1, bitset_fnz(&Bitset, -1));
}
