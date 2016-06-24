/*!\file
 * This is the public header file of the utilities library.
 *
 * \copyright 2016 Lely Industries N.V.
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

#ifndef LELY_UTIL_UTIL_H
#define LELY_UTIL_UTIL_H

#include <lely/libc/libc.h>

#ifndef LELY_UTIL_EXTERN
#ifdef DLL_EXPORT
#ifdef LELY_UTIL_INTERN
#define LELY_UTIL_EXTERN	extern __dllexport
#else
#define LELY_UTIL_EXTERN	extern __dllimport
#endif
#else
#define LELY_UTIL_EXTERN	extern
#endif
#endif

#ifndef ABS
//! Returns the absolute value of \a a.
#define ABS(a)	((a) < 0 ? -(a) : (a))
#endif

#ifndef ALIGN
/*!
 * Rounds \a x up to the nearest multiple of \a a.
 *
 * Since the rounding is performed with a bitmask, \a a MUST be a power of two.
 */
#ifdef __GNUC__
#define ALIGN(x, a)	__ALIGN_MASK((x), (__typeof__(x))(a) - 1)
#else
#define ALIGN(x, a)	__ALIGN_MASK((x), (a) - 1)
#endif
#endif

#ifndef __ALIGN_MASK
#define __ALIGN_MASK(x, mask)	(((x) + (mask)) & ~(mask))
#endif

#ifndef MIN
/*!
 * Returns the minimum of \a a and \a b. Guaranteed to return the opposite of
 * MAX(), i.e., if MAX() returns \a a then MIN() returns \a b and vice versa.
 */
#define MIN(a, b)	((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
/*!
 * Returns the maximum of \a a and \a b. Guaranteed to return the opposite of
 * MIN().
 */
#define MAX(a, b)	((a) < (b) ? (b) : (a))
#endif

#ifndef powerof2
/*!
 * Returns 1 if \a x is a power of two, and 0 otherwise. Note that zero is
 * considered to be a power of two.
 */
#define powerof2(x)	(!((x) & ((x) - 1)))
#endif

#ifndef STRINGIFY
//! Expands and stringifies \a x. \see QUOTE()
#define STRINGIFY(x)	QUOTE(x)
#endif

#ifndef structof
/*!
 * Obtains the address of a structure from the address of one of its members.
 * This macro only works for plain old data structures (PODSs) and performs no
 * type or NULL checking.
 *
 * \param ptr    a pointer to the member.
 * \param type   the type of the structure.
 * \param member the name of member.
 */
#define structof(ptr, type, member) \
	((type *)((char *)(ptr) - offsetof(type, member)))
#endif

#ifndef QUOTE
//! Stringifies \a x without expanding. \see STRINGIFY()
#define QUOTE(x)	#x
#endif

#endif

