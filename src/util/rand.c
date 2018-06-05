/*!\file
 * This file is part of the utilities library; it contains the implementation of
 * the random number generator.
 *
 * \see lely/util/rand.h
 *
 * \copyright 2017 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
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
#include <lely/util/rand.h>

#include <assert.h>

LELY_UTIL_EXPORT void
rand64_seed(struct rand64 *r, uint64_t seed)
{
	assert(r);

	r->u = seed;
	r->v = UINT64_C(4101842887655102017);
	r->w = 1;

	r->u ^= r->v;
	rand64_discard(r, 1);
	r->v = r->u;
	rand64_discard(r, 1);
	r->w = r->v;
	rand64_discard(r, 1);
}

LELY_UTIL_EXPORT uint64_t
rand64_get(struct rand64 *r)
{
	rand64_discard(r, 1);

	uint64_t x = r->u ^ (r->u << 21);
	x ^= x >> 35;
	x ^= x << 4;
	return (x + r->v) ^ r->w;
}

LELY_UTIL_EXPORT void
rand64_discard(struct rand64 *r, uint64_t z)
{
	assert(r);

	while (z--) {
		r->u *= UINT64_C(2862933555777941757);
		r->u += UINT64_C(7046029254386353087);
		r->v ^= r->v >> 17;
		r->v ^= r->v << 31;
		r->v ^= r->v >> 8;
		r->w = UINT32_C(4294957665) * (r->w & UINT32_C(0xffffffff))
				+ (r->w >> 32);
	}
}

#define LELY_UTIL_DEFINE_RAND(b) \
	LELY_UTIL_EXPORT void \
	rand##b##_seed(struct rand##b *r, uint64_t seed) \
	{ \
		assert(r); \
	\
		rand64_seed(&r->r, seed); \
		r->x = 0; \
		r->n = 0; \
	} \
	\
	LELY_UTIL_EXPORT uint##b##_t \
	rand##b##_get(struct rand##b *r) \
	{ \
		if (r->n < b) { \
			r->x = rand64_get(&r->r); \
			r->n = 64; \
		} \
	\
		r->n -= b; \
		return (uint##b##_t)(r->x >>= b); \
	} \
	\
	LELY_UTIL_EXPORT void \
	rand##b##_discard(struct rand##b *r, uint64_t z) \
	{ \
		if (z > r->n / b) { \
			z -= r->n / b; \
	\
			if (z >= (64 / b)) { \
				rand64_discard(&r->r, z / (64 / b)); \
				z %= (64 / b); \
			} \
	\
			r->x = rand64_get(&r->r); \
			r->n = 64; \
		} \
	\
		if (z) { \
			r->x >>= z * b; \
			r->n -= (unsigned int)(z * b); \
		} \
	}

LELY_UTIL_DEFINE_RAND(32)
LELY_UTIL_DEFINE_RAND(16)
LELY_UTIL_DEFINE_RAND(8)

#undef LELY_UTIL_DEFINE_RAND

