/*!\file
 * This file is part of the C11 and POSIX compatibility library.
 *
 * \see lely/libc/stdio.h
 *
 * \copyright 2017 Lely Industries N.V.
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

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>

#if !(_POSIX_C_SOURCE >= 200809L)

#ifndef LELY_GETDELIM_SIZE
//! The initial size (in bytes) of the buffer allocated by getdelim().
#define LELY_GETDELIM_SIZE	64
#endif

LELY_LIBC_EXPORT ssize_t
getdelim(char **lineptr, size_t *n, int delim, FILE *stream)
{
	if (__unlikely(!lineptr || !n)) {
#ifdef EINVAL
		errno = EINVAL;
#endif
		return -1;
	}
	if (!*n)
		*lineptr = NULL;
	else if (!*lineptr)
		*n = 0;
	size_t size = LELY_GETDELIM_SIZE;

	char *cp = *lineptr;
	for (;;) {
		// Make sure we have enough space for the next character and the
		// terminating null byte.
		if (*n - (cp - *lineptr) < 2) {
			int errsv = errno;
			// First try to double the buffer, to minimize the
			// number of reallocations.
			while (size < (size_t)(cp - *lineptr + 2))
				size *= 2;
			char *tmp = realloc(*lineptr, size);
			if (__unlikely(!tmp)) {
				errno = errsv;
				// If doubling the buffer fails, try to allocate
				// just enough memory for the next character
				// (including the terminating null byte).
				size = cp - *lineptr + 2;
				tmp = realloc(*lineptr, size);
				if (__unlikely(!tmp))
					return -1;
			}
			cp = tmp + (cp - *lineptr);
			*lineptr = tmp;
			*n = size;
		}
		int c = fgetc(stream);
		// On end-of-file, if any characters have been written to the
		// buffer and no error has occurred, do not return -1. This
		// allows the caller to retrieve the last (undelimited) record.
		if (c == EOF && (ferror(stream) || !(cp - *lineptr)))
			return -1;
		if (c == EOF || (*cp++ = c) == delim)
			break;
	}
	*cp = '\0';

	return cp - *lineptr;
}

LELY_LIBC_EXPORT ssize_t
getline(char **lineptr, size_t *n, FILE *stream)
{
	return getdelim(lineptr, n, '\n', stream);
}

#endif // !(_POSIX_C_SOURCE >= 200809L)

#if !defined(_GNU_SOURCE) || defined(__MINGW32__)

#if !LELY_HAVE_SNPRINTF

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

