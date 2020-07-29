/**@file
 * This header file is part of the utilities library; it contains (16-bit)
 * Unicode string functions.
 *
 * @copyright 2020 Lely Industries N.V.
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

#ifndef LELY_UTIL_USTRING_H_
#define LELY_UTIL_USTRING_H_

#include <lely/libc/uchar.h>

#include <assert.h>
#include <stddef.h>

#ifndef LELY_UTIL_USTRING_INLINE
#define LELY_UTIL_USTRING_INLINE static inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the number of (16-bit) Unicode characters, excluding the terminating
 * null bytes, in the string at <b>s</b>.
 *
 * @param s a pointer to the (null-terminated) string.
 *
 * @returns the number of characters in the string pointed to by <b>s</b>.
 */
LELY_UTIL_USTRING_INLINE size_t str16len(const char16_t *s);

/**
 * Copies <b>n</b> (16-bit) Unicode characters from the string at <b>src</b> to
 * <b>dst</b>. If the string at <b>src</b> is shorter than <b>n</b> characters,
 * NUL characters are appended to the string at <b>dst</b> until <b>n</b>
 * characters haven been written.
 *
 * @param dst the destination address, which MUST be large enough to hold the
 *            string.
 * @param src a pointer to the string to be copied.
 * @param n   the number of (16-bit) Unicode characters to copy.
 *
 * @returns <b>dst</b>.
 */
LELY_UTIL_USTRING_INLINE char16_t *str16ncpy(
		char16_t *dst, const char16_t *src, size_t n);

/**
 * Compares two (16-bit) Unicode strings.
 *
 * @param s1 a pointer to the first string.
 * @param s2 a pointer to the second string.
 * @param n  the maximum number of characters to compare.
 *
 * @returns an integer greater than, equal to, or less than 0 if the string at
 * <b>s1</b> is greater than, equal to, or less than the string at <b>s2</b>.
 */
LELY_UTIL_USTRING_INLINE int str16ncmp(
		const char16_t *s1, const char16_t *s2, size_t n);

LELY_UTIL_USTRING_INLINE size_t
str16len(const char16_t *s)
{
	const char16_t *cp = s;
	while (*cp != 0)
		cp++;

	return cp - s;
}

LELY_UTIL_USTRING_INLINE char16_t *
str16ncpy(char16_t *dst, const char16_t *src, size_t n)
{
	char16_t *cp = dst;
	for (; (n != 0) && (*src != 0); n--, cp++, src++)
		*cp = *src;
	for (; n != 0; n--)
		*cp++ = 0;

	return dst;
}

LELY_UTIL_USTRING_INLINE int
str16ncmp(const char16_t *s1, const char16_t *s2, size_t n)
{
	for (; n != 0; n--, s1++, s2++) {
		const int cmp = *s1 - *s2;
		if ((cmp != 0) || (*s1 == 0))
			return cmp;
	}
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif // !LELY_UTIL_USTRING_H_
