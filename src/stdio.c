/*!\file
 * This file is part of the C11 and POSIX compatibility library.
 *
 * \see lely/libc/stdio.h
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
#include <lely/libc/stdio.h>

#ifndef _GNU_SOURCE

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>

LELY_LIBC_EXPORT int
asprintf(char **strp, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int result = vasprintf(strp, fmt, ap);
	va_end(ap);
	return result;
}

LELY_LIBC_EXPORT int
vasprintf(char **strp, const char *fmt, va_list ap)
{
	assert(strp);

	va_list aq;
	va_copy(aq, ap);
	int n = vsnprintf(NULL, 0, fmt, aq);
	va_end(aq);
	if (__unlikely(n < 0))
		return n;

	void *ptr = malloc(n + 1);
	if (__unlikely(!ptr && n > 0))
		return -1;

	*strp = ptr;
	return vsnprintf(*strp, n + 1, fmt, ap);
}

#endif // !_GNU_SOURCE

