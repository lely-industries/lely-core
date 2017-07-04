/*!\file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<stdio.h>` and defines any missing functionality.
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

#ifndef LELY_LIBC_STDIO_H
#define LELY_LIBC_STDIO_H

#include <lely/libc/libc.h>

#ifndef LELY_HAVE_SNPRINTF
#define LELY_HAVE_SNPRINTF	1
// Microsoft Visual C++ 2013 and earlier do not have snprintf().
#if defined(_MSC_VER) && _MSC_VER < 1900
#undef LELY_HAVE_SNPRINTF
#endif
#endif

#if !LELY_HAVE_SNPRINTF
// Hide existing (and non-conformant) definitions of snprintf() and vsnprintf().
#undef snprintf
#define snprintf	__no_snprintf
#undef vsnprintf
#define vsnprintf	__no_vsnprintf
#endif

#include <lely/libc/sys/types.h>

#include <stdarg.h>
#include <stdio.h>

#if !LELY_HAVE_SNPRINTF
#undef vsnprintf
#undef snprintf
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !(_POSIX_C_SOURCE >= 200809L)

/*!
 * Reads a delimited record from a stream.
 *
 * \param lineptr the address of a pointer to the buffer used for storing the
 *                characters read from \a stream. If *\a lineptr is NULL or the
 *                buffer is too small, a new buffer will be allocated (with
 *                `malloc()` or `realloc()`, respectively) and the address will
 *                be stored in *\a lineptr.
 * \param n       the address of a value containing the size of the buffer at
 *                *\a lineptr. On exit, *\a n contains the actual size of the
 *                buffer.
 * \param delim   the delimiter character.
 * \param stream  a pointer to the input stream.
 *
 * \returns the number of characters read (including the delimiter but excluding
 * the terminating null byte), or -1 on error or end-of-file.
 */
LELY_LIBC_EXTERN ssize_t getdelim(char **lineptr, size_t *n, int delim,
		FILE *stream);

//! Equivalent to #getdelim(lineptr, n, '\\n', stream).
LELY_LIBC_EXTERN ssize_t getline(char **lineptr, size_t *n, FILE *stream);

#endif // !(_POSIX_C_SOURCE >= 200809L)

#if !defined(_GNU_SOURCE) || defined(__MINGW32__)

#if !LELY_HAVE_SNPRINTF

/*!
 * Equivalent to `printf()`, except that the output is written to a string
 * buffer rather than a stream.
 *
 * \param s      the address of the output buffer. If \a s is not NULL, at most
 *               `n - 1` characters are written, plus a terminating null byte.
 * \param n      the size (in bytes) of the buffer at \a s. If \a n is zero,
 *               nothing is written.
 * \param format a printf-style format string.
 * \param ...    an optional list of arguments to be printed according to
 *               \a format.
 *
 * \returns the number of characters that would have been written had the
 * buffer been sufficiently large, not counting the terminating null byte, or a
 * negative number on error.
 *
 * \see vsnprintf()
 */
LELY_LIBC_EXTERN int __cdecl snprintf(char *s, size_t n, const char *format,
		...);

/*!
 * Equivalent to `vprintf()`, except that the output is written to a string
 * buffer rather than a stream.
 *
 * \param s      the address of the output buffer. If \a s is not NULL, at most
 *               `n - 1` characters are written, plus a terminating null byte.
 * \param n      the size (in bytes) of the buffer at \a s. If \a n is zero,
 *               nothing is written.
 * \param format a printf-style format string.
 * \param arg    the list with arguments to be printed according to \a format.
 *
 * \returns the number of characters that would have been written had the
 * buffer been sufficiently large, not counting the terminating null byte, or a
 * negative number on error.
 *
 * \see snprintf()
 */
LELY_LIBC_EXTERN int __cdecl vsnprintf(char *s, size_t n, const char *format,
		va_list arg);

#endif // !LELY_HAVE_SNPRINTF

/*!
 * Equivalent to `sprintf()`, except that it allocates a string large enough to
 * hold the output, including the terminating null byte.
 *
 * \param strp the address of a value which, on success, contains a pointer to
 *             the allocated string. This pointer SHOULD be passed to `free()`
 *             to release the allocated storage.
 * \param fmt  a printf-style format string.
 * \param ...  an optional list of arguments to be printed according to \a fmt.
 *
 * \returns the number of characters written, not counting the terminating null
 * byte, or a negative number on error.
 *
 * \see vasprintf()
 */
LELY_LIBC_EXTERN int __cdecl asprintf(char **strp, const char *fmt, ...);

/*!
 * Equivalent to `vsprintf()`, except that it allocates a string large enough to
 * hold the output, including the terminating null byte.
 *
 * \param strp the address of a value which, on success, contains a pointer to
 *             the allocated string. This pointer SHOULD be passed to `free()`
 *             to release the allocated storage.
 * \param fmt  a printf-style format string.
 * \param ap   the list with arguments to be printed according to \a fmt.
 *
 * \returns the number of characters written, not counting the terminating null
 * byte, or a negative number on error.
 *
 * \see asprintf()
 */
LELY_LIBC_EXTERN int __cdecl vasprintf(char **strp, const char *fmt,
		va_list ap);

#endif // !_GNU_SOURCE || __MINGW32__

#ifdef __cplusplus
}
#endif

#endif

