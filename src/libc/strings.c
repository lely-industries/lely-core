/**@file
 * This file is part of the C11 and POSIX compatibility library.
 *
 * @see lely/libc/strings.h
 *
 * @copyright 2014-2019 Lely Industries N.V.
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

#include "libc.h"
#define LELY_LIBC_STRINGS_INLINE extern inline
#include <lely/libc/strings.h>

#if !LELY_HAVE_STRINGS_H

#include <ctype.h>

#if !(defined(__GNUC__) || __has_builtin(__builtin_ffs))

#include <stdint.h>

// clang-format off
static const int ffs_tab[] = {
	0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};
// clang-format on

int
ffs(int i)
{
	// cppcheck-suppress oppositeExpression
	unsigned int x = i & -i;
	// clang-format off
	unsigned int n = x > UINT32_C(0x00ffffff)
			? 24 : (x > UINT16_C(0xffffu)
			? 16 : (x > UINT8_C(0xff) ? 8 : 0));
	// clang-format on
	return n + ffs_tab[(x >> n) & UINT8_C(0xff)];
}

#endif

int
strcasecmp(const char *s1, const char *s2)
{
	if (s1 == s2)
		return 0;

	int result;
	// clang-format off
	while ((result = tolower((unsigned char)*s1)
			- tolower((unsigned char)*s2++)) == 0 && *s1++)
		// clang-format on
		;
	return result;
}

int
strncasecmp(const char *s1, const char *s2, size_t n)
{
	if (s1 == s2 || !n)
		return 0;

	int result;
	// clang-format off
	while ((result = tolower((unsigned char)*s1)
			- tolower((unsigned char)*s2++)) == 0 && --n && *s1++)
		// clang-format on
		;
	return result;
}

#endif // !LELY_HAVE_STRINGS_H
