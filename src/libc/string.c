/**@file
 * This file is part of the C11 and POSIX compatibility library.
 *
 * @see lely/libc/string.h
 *
 * @copyright 2015-2018 Lely Industries N.V.
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
#include <lely/libc/string.h>

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

#if !(_MSC_VER >= 1400) && !(_POSIX_C_SOURCE >= 200809L) \
		&& !defined(__MINGW32__)
size_t
strnlen(const char *s, size_t maxlen)
{
	size_t size = 0;
	while (size < maxlen && *s++)
		size++;
	return size;
}
#endif
