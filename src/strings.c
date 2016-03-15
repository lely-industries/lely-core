/*!\file
 * This file is part of the C11 and POSIX compatibility library.
 *
 * \see lely/libc/strings.h
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

#include "libc.h"
#include <lely/libc/strings.h>

#ifndef LELY_HAVE_STRINGS_H

#include <ctype.h>

LELY_LIBC_EXPORT int __cdecl
strcasecmp(const char *s1, const char *s2)
{
	if (s1 == s2)
		return 0;

	int result;
	while (!(result = tolower((unsigned char)*s1)
			- tolower((unsigned char)*s2++)) && *s1++);
	return result;
}

LELY_LIBC_EXPORT int __cdecl
strncasecmp(const char *s1, const char *s2, size_t n)
{
	if (s1 == s2 || !n)
		return 0;

	int result;
	while (!(result = tolower((unsigned char)*s1)
			- tolower((unsigned char)*s2++)) && --n && *s1++);
	return result;
}

#endif // !LELY_HAVE_STRINGS_H

