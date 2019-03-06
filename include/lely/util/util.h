/**@file
 * This is the public header file of the utilities library.
 *
 * @copyright 2013-2018 Lely Industries N.V.
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

#ifndef LELY_UTIL_UTIL_H_
#define LELY_UTIL_UTIL_H_

#include <lely/features.h>

#include <stddef.h>

#ifndef ABS
/// Returns the absolute value of <b>a</b>.
#define ABS(a) ((a) < 0 ? -(a) : (a))
#endif

#ifndef ALIGN
/**
 * Rounds <b>x</b> up to the nearest multiple of <b>a</b>.
 *
 * Since the rounding is performed with a bitmask, <b>a</b> MUST be a power of
 * two.
 */
#if defined(__clang__) || defined(__GNUC__)
#define ALIGN(x, a) ALIGN_MASK((x), (__typeof__(x))(a)-1)
#else
#define ALIGN(x, a) ALIGN_MASK((x), (a)-1)
#endif
#endif

#ifndef ALIGN_MASK
#define ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#endif

#ifndef MIN
/**
 * Returns the minimum of <b>a</b> and <b>b</b>. Guaranteed to return the
 * opposite of MAX(), i.e., if MAX() returns <b>a</b> then MIN() returns
 * <b>b</b> and vice versa.
 */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
/**
 * Returns the maximum of <b>a</b> and <b>b</b>. Guaranteed to return the
 * opposite of MIN().
 */
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#endif

#ifndef countof
/// Returns the number of array members in <b>a</b>.
#define countof(a) (sizeof(a) / sizeof(0 [a]))
#endif

#ifndef powerof2
/**
 * Returns 1 if <b>x</b> is a power of two, and 0 otherwise. Note that zero is
 * considered to be a power of two.
 */
#define powerof2(x) (!((x) & ((x)-1)))
#endif

#ifndef structof
/**
 * Obtains the address of a structure from the address of one of its members.
 * This macro only works for plain old data structures (PODSs) and performs no
 * type or NULL checking.
 *
 * @param ptr    a pointer to the member.
 * @param type   the type of the structure.
 * @param member the name of member.
 */
// clang-format off
#if defined(__clang__) || defined(__GNUC__)
#define structof(ptr, type, member) \
	__extension__({ \
		const __typeof__(((type *)NULL)->member) *_tmp_ = (ptr); \
		((type *)((char *)(_tmp_) - offsetof(type, member))); \
	})
#else
#define structof(ptr, type, member) \
	((type *)((char *)(ptr) - offsetof(type, member)))
#endif
// clang-format on
#endif

#endif // !LELY_UTIL_UTIL_H_
