/**@file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<strings.h>`, if it exists, and defines any missing functionality.
 *
 * @copyright 2014-2018 Lely Industries N.V.
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

#ifndef LELY_LIBC_STRINGS_H_
#define LELY_LIBC_STRINGS_H_

#include <lely/features.h>

#ifndef LELY_HAVE_STRINGS_H
#if (_POSIX_C_SOURCE >= 200112L || defined(__NEWLIB__))
#define LELY_HAVE_STRINGS_H 1
#endif
#endif

#if LELY_HAVE_STRINGS_H
#include <strings.h>
#else // !LELY_HAVE_STRINGS_H

#include <stddef.h>

#ifndef LELY_LIBC_STRINGS_INLINE
#define LELY_LIBC_STRINGS_INLINE static inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Finds the index of the first (least significant) bit set in <b>i</b>. Bits
 * are numbered counting from 1.
 *
 * @returns the index of the first bit set, or 0 if <b>i</b> is 0.
 */
#if (defined(__GNUC__) || __has_builtin(__builtin_ffs)) \
		&& !defined(__BSD_VISIBLE)
LELY_LIBC_STRINGS_INLINE int ffs(int i);

inline int
ffs(int i)
{
	return __builtin_ffs(i);
}
#else
int ffs(int i);
#endif

/**
 * Compares the string at <b>s1</b> to the string at <b>s2</b>, ignoring
 * differences in case.
 *
 * @returns an integer greater than, equal to, or less than 0 if the string at
 * <b>s1</b> is greater than, equal to, or less than the string at <b>s2</b>.
 */
int strcasecmp(const char *s1, const char *s2);

/**
 * Compares at most <b>n</b> characters from the the string at <b>s1</b> to the
 * string at <b>s2</b>, ignoring differences in case.
 *
 * @returns an integer greater than, equal to, or less than 0 if the string at
 * <b>s1</b> is greater than, equal to, or less than the string at <b>s2</b>.
 */
int strncasecmp(const char *s1, const char *s2, size_t n);

#ifdef __cplusplus
}
#endif

#endif // !LELY_HAVE_STRINGS_H

#endif // !LELY_LIBC_STRINGS_H_
