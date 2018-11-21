/**@file
 * This header file is part of the utilities library; it contains the random
 * number generator definitions.
 *
 * The implementation of the random number generator is based on Numerical
 * Recipes (3rd edition), paragraph 7.1. It generates 64-bit uniformly
 * distributed random numbers with a period of more than 3 * 10^57.
 *
 * @copyright 2017-2018 Lely Industries N.V.
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

#ifndef LELY_UTIL_RAND_H_
#define LELY_UTIL_RAND_H_

#include <lely/libc/stdint.h>
#include <lely/util/util.h>

/// A 64-bit uniformly distributed unsigned random number generator.
struct rand64 {
	/// The first state value of the generator.
	uint64_t u;
	/// The second state value of the generator.
	uint64_t v;
	/// The third state value of the generator.
	uint64_t w;
};

/**
 * A 32-bit uniformly distributed unsigned random number generator. This
 * generator uses all bits of the 64-bit base generator, instead of discarding
 * the higher bits.
 */
struct rand32 {
	/// The 64-bit base generator.
	struct rand64 r;
	/// The current set of random numbers.
	uint64_t x;
	/// The number of random bits left in #x.
	unsigned int n;
};

/**
 * A 16-bit uniformly distributed unsigned random number generator. This
 * generator uses all bits of the 64-bit base generator, instead of discarding
 * the higher bits.
 */
struct rand16 {
	/// The 64-bit base generator.
	struct rand64 r;
	/// The current set of random numbers.
	uint64_t x;
	/// The number of random bits left in #x.
	unsigned int n;
};

/**
 * An 8-bit uniformly distributed unsigned random number generator. This
 * generator uses all bits of the 64-bit base generator, instead of discarding
 * the higher bits.
 */
struct rand8 {
	/// The 64-bit base generator.
	struct rand64 r;
	/// The current set of random numbers.
	uint64_t x;
	/// The number of random bits left in #x.
	unsigned int n;
};

#ifdef __cplusplus
extern "C" {
#endif

/// Initializes a 64-bit random number generator with a seed.
void rand64_seed(struct rand64 *r, uint64_t seed);

/// Generates an unsigned 64-bit random number.
uint64_t rand64_get(struct rand64 *r);

/// Discards the next <b>z</b> random numbers from the sequence.
void rand64_discard(struct rand64 *r, uint64_t z);

/// Initializes a 32-bit random number generator with a seed.
void rand32_seed(struct rand32 *r, uint64_t seed);

/// Generates an unsigned 32-bit random number.
uint32_t rand32_get(struct rand32 *r);

/// Discards the next <b>z</b> random numbers from the sequence.
void rand32_discard(struct rand32 *r, uint64_t z);

/// Initializes a 16-bit random number generator with a seed.
void rand16_seed(struct rand16 *r, uint64_t seed);

/// Generates an unsigned 16-bit random number.
uint16_t rand16_get(struct rand16 *r);

/// Discards the next <b>z</b> random numbers from the sequence.
void rand16_discard(struct rand16 *r, uint64_t z);

/// Initializes a 8-bit random number generator with a seed.
void rand8_seed(struct rand8 *r, uint64_t seed);

/// Generates an unsigned 8-bit random number.
uint8_t rand8_get(struct rand8 *r);

/// Discards the next <b>z</b> random numbers from the sequence.
void rand8_discard(struct rand8 *r, uint64_t z);

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_RAND_H_
