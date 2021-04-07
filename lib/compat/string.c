/**@file
 * This file is part of the C11 and POSIX compatibility library.
 *
 * @see lely/compat/string.h
 *
 * @copyright 2015-2020 Lely Industries N.V.
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

#include "compat.h"
#include <lely/compat/string.h>

#if LELY_NO_HOSTED

void *
lely_compat_memcpy(void *s1, const void *s2, size_t n)
{
	unsigned char *bp1 = s1;
	const unsigned char *bp2 = s2;
	while (n--)
		*bp1++ = *bp2++;
	return s1;
}

void *
lely_compat_memmove(void *s1, const void *s2, size_t n)
{
	unsigned char *bp1 = s1;
	const unsigned char *bp2 = s2;
	if (s1 < s2) {
		while (n--)
			*bp1++ = *bp2++;
	} else if (s1 > s2) {
		bp1 += n;
		bp2 += n;
		while (n--)
			*--bp1 = *--bp2;
	}
	return s1;
}

int
lely_compat_memcmp(const void *s1, const void *s2, size_t n)
{
	for (const unsigned char *bp1 = s1, *bp2 = s2; n; n--, bp1++, bp2++) {
		if (*bp1 < *bp2)
			return -1;
		if (*bp1 > *bp2)
			return 1;
	}
	return 0;
}

int
lely_compat_strcmp(const char *s1, const char *s2)
{
	const unsigned char *bp1 = (const unsigned char *)s1;
	const unsigned char *bp2 = (const unsigned char *)s2;
	for (;; bp1++, bp2++) {
		if (*bp1 != *bp2)
			return *bp1 < *bp2 ? -1 : 1;
		if (!*bp1)
			return 0;
	}
}

int
lely_compat_strncmp(const char *s1, const char *s2, size_t n)
{
	const unsigned char *bp1 = (const unsigned char *)s1;
	const unsigned char *bp2 = (const unsigned char *)s2;
	for (; n; n--, bp1++, bp2++) {
		if (*bp1 != *bp2)
			return *bp1 < *bp2 ? -1 : 1;
		if (!*bp1)
			return 0;
	}
	return 0;
}

void *
lely_compat_memset(void *s, int c, size_t n)
{
	unsigned char *bp = s;
	while (n--)
		*bp++ = (unsigned char)c;
	return s;
}

size_t
lely_compat_strlen(const char *s)
{
	size_t n = 0;
	while (*s++)
		n++;
	return n;
}

#endif // LELY_NO_HOSTED

#if !LELY_NO_MALLOC

#include <stdlib.h>

#if !(_MSC_VER >= 1400) && !(_POSIX_C_SOURCE >= 200809L) \
		&& !defined(__MINGW32__)
char *
strdup(const char *s)
{
	size_t size = strlen(s) + 1;
	char *dup = malloc(size);
	if (!dup)
		return NULL;
	return memcpy(dup, s, size);
}
#endif

#if !(_POSIX_C_SOURCE >= 200809L)
char *
strndup(const char *s, size_t size)
{
	size = strnlen(s, size);
	char *dup = malloc(size + 1);
	if (!dup)
		return NULL;
	dup[size] = '\0';
	return memcpy(dup, s, size);
}
#endif

#endif // !LELY_NO_MALLOC

#if LELY_NO_HOSTED \
		|| (!(_MSC_VER >= 1400) && !(_POSIX_C_SOURCE >= 200809L) \
				&& !defined(__MINGW32__))
size_t
lely_compat_strnlen(const char *s, size_t maxlen)
{
	size_t size = 0;
	while (size < maxlen && *s++)
		size++;
	return size;
}
#endif
