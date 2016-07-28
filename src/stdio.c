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

#if !defined(_GNU_SOURCE) || defined(__MINGW32__)

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>

#ifndef LELY_HAVE_SNPRINTF

LELY_LIBC_EXPORT int __cdecl
snprintf(char *s, size_t n, const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	int result = vsnprintf(s, n, format, arg);
	va_end(arg);
	return result;
}

LELY_LIBC_EXPORT int __cdecl
vsnprintf(char *s, size_t n, const char *format, va_list arg)
{
	int result = -1;
	if (n) {
		va_list ap;
		va_copy(ap, arg);
		result = _vsnprintf_s(s, n, _TRUNCATE, format, ap);
		va_end(ap);
	}
	return result == -1 ? _vscprintf(format, arg) : result;
}

#endif // !LELY_HAVE_SNPRINTF

LELY_LIBC_EXPORT int __cdecl
asprintf(char **strp, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int result = vasprintf(strp, fmt, ap);
	va_end(ap);
	return result;
}

LELY_LIBC_EXPORT int __cdecl
vasprintf(char **strp, const char *fmt, va_list ap)
{
	assert(strp);

	va_list aq;
	va_copy(aq, ap);
	int n = vsnprintf(NULL, 0, fmt, aq);
	va_end(aq);
	if (__unlikely(n < 0))
		return n;

	char *s = malloc(n + 1);
	if (__unlikely(!s))
		return -1;

	n = vsnprintf(s, n + 1, fmt, ap);
	if (__unlikely(n < 0)) {
		int errsv = errno;
		free(s);
		errno = errsv;
		return n;
	}

	*strp = s;
	return n;
}

#endif // !_GNU_SOURCE || __MINGW32__

