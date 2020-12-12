/**@file
 * This file is part of the utilities library; it contains the implementation of
 * the bitset functions.
 *
 * @see lely/util/bitset.h
 *
 * @copyright 2017-2019 Lely Industries N.V.
 *
 * @author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#include "util.h"

#if !LELY_NO_MALLOC

#include <lely/libc/strings.h>
#include <lely/util/bitset.h>
#include <lely/util/errnum.h>

#include <assert.h>
#include <stdlib.h>

#undef INT_BIT
#define INT_BIT (sizeof(int) * CHAR_BIT)

int
bitset_init(struct bitset *set, int size)
{
	assert(set);

	size = MAX(0, (size + INT_BIT - 1) / INT_BIT);

	unsigned int *bits = malloc(size * sizeof(unsigned int));
	if (!bits && size) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return -1;
	}

	set->size = size;
	set->bits = bits;

	return 0;
}

void
bitset_fini(struct bitset *set)
{
	assert(set);

	set->size = 0;
	free(set->bits);
	set->bits = NULL;
}

int
bitset_size(const struct bitset *set)
{
	return set->size * INT_BIT;
}

int
bitset_resize(struct bitset *set, int size)
{
	assert(set);
	assert(set->size >= 0);

	size = MAX(0, (size + INT_BIT - 1) / INT_BIT);

	unsigned int *bits = realloc(set->bits, size * sizeof(unsigned int));
	if (!bits && size) {
#if !LELY_NO_ERRNO
		set_errc(errno2c(errno));
#endif
		return 0;
	}

	// If the size increased, clear the new bits.
	for (int i = set->size; i < size; i++)
		bits[i] = 0;

	set->size = size;
	set->bits = bits;

	return bitset_size(set);
}

int
bitset_get_size(const struct bitset *set)
{
	return set->size * INT_BIT;
}

int
bitset_test(const struct bitset *set, int n)
{
	if (n < 0 || n >= bitset_size(set))
		return 0;
	return (set->bits[n / INT_BIT] >> (n & (INT_BIT - 1))) & 1u;
}

void
bitset_set(struct bitset *set, int n)
{
	if (n < 0 || n >= bitset_size(set))
		return;
	set->bits[n / INT_BIT] |= 1u << (n & (INT_BIT - 1));
}

void
bitset_set_all(struct bitset *set)
{
	for (int i = 0; i < set->size; i++)
		set->bits[i] = ~0u;
}

void
bitset_clr(struct bitset *set, int n)
{
	if (n < 0 || n >= bitset_size(set))
		return;
	set->bits[n / INT_BIT] &= ~(1u << (n & (INT_BIT - 1)));
}

void
bitset_clr_all(struct bitset *set)
{
	for (int i = 0; i < set->size; i++)
		set->bits[i] = 0;
}

void
bitset_compl(struct bitset *set)
{
	for (int i = 0; i < set->size; i++)
		set->bits[i] = ~set->bits[i];
}

int
bitset_ffs(const struct bitset *set)
{
	const unsigned int *bits = set->bits;
	int offset = 0;
	for (int i = 0; i < set->size; i++) {
		unsigned int x = *bits++;
		if (x)
			return offset + ffs(x);
		offset += INT_BIT;
	}
	return 0;
}

int
bitset_ffz(const struct bitset *set)
{
	const unsigned int *bits = set->bits;
	int offset = 0;
	for (int i = 0; i < set->size; i++) {
		unsigned int x = *bits++;
		if (~x)
			return offset + ffs(~x);
		offset += INT_BIT;
	}
	return 0;
}

int
bitset_fns(const struct bitset *set, int n)
{
	if (n < 0)
		n = 0;
	if (n >= bitset_size(set))
		return 0;

	int size = set->size - n / INT_BIT;
	const unsigned int *bits = set->bits + n / INT_BIT;
	int offset = n & ~(INT_BIT - 1);
	n -= offset;
	while (size) {
		unsigned int x = *bits++;
		if (n) {
			// Clear the bits smaller than n.
			x &= ~0u << n;
			n = 0;
		}
		if (x)
			return offset + ffs(x);
		size--;
		offset += INT_BIT;
	}
	return 0;
}

int
bitset_fnz(const struct bitset *set, int n)
{
	if (n < 0)
		n = 0;
	if (n >= bitset_size(set))
		return 0;

	int size = set->size - n / INT_BIT;
	const unsigned int *bits = set->bits + n / INT_BIT;
	int offset = n & ~(INT_BIT - 1);
	n -= offset;
	while (size) {
		unsigned int x = *bits++;
		if (n) {
			// Set the bits smaller than n.
			x |= ~0u >> (INT_BIT - n);
			n = 0;
		}
		if (~x)
			return offset + ffs(~x);
		size--;
		offset += INT_BIT;
	}
	return 0;
}

#endif // !LELY_NO_MALLOC
