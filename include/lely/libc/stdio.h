/**@file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<stdio.h>` and defines any missing functionality.
 *
 * @copyright 2016-2018 Lely Industries N.V.
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

#ifndef LELY_LIBC_STDIO_H_
#define LELY_LIBC_STDIO_H_

#include <lely/features.h>

#if !LELY_NO_STDIO

#include <lely/libc/sys/types.h>

#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !(_POSIX_C_SOURCE >= 200809L)

/**
 * Reads a delimited record from a stream.
 *
 * @param lineptr the address of a pointer to the buffer used for storing the
 *                characters read from <b>stream</b>. If *<b>lineptr</b> is NULL
 *                or the buffer is too small, a new buffer will be allocated
 *                (with `malloc()` or `realloc()`, respectively) and the address
 *                will be stored in *<b>lineptr</b>.
 * @param n       the address of a value containing the size of the buffer at
 *                *<b>lineptr</b>. On exit, *<b>n</b> contains the actual size
 *                of the buffer.
 * @param delim   the delimiter character.
 * @param stream  a pointer to the input stream.
 *
 * @returns the number of characters read (including the delimiter but
 * excluding the terminating null byte), or -1 on error or end-of-file.
 */
ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream);

/// Equivalent to #getdelim(lineptr, n, '\\n', stream).
ssize_t getline(char **lineptr, size_t *n, FILE *stream);

#endif // !(_POSIX_C_SOURCE >= 200809L)

#if !defined(_GNU_SOURCE)

/**
 * Equivalent to `sprintf()`, except that it allocates a string large enough to
 * hold the output, including the terminating null byte.
 *
 * @param strp the address of a value which, on success, contains a pointer to
 *             the allocated string. This pointer SHOULD be passed to `free()`
 *             to release the allocated storage.
 * @param fmt  a printf-style format string.
 * @param ...  an optional list of arguments to be printed according to
 *             <b>fmt</b>.
 *
 * @returns the number of characters written, not counting the terminating null
 * byte, or a negative number on error.
 *
 * @see vasprintf()
 */
int asprintf(char **strp, const char *fmt, ...);

/**
 * Equivalent to `vsprintf()`, except that it allocates a string large enough to
 * hold the output, including the terminating null byte.
 *
 * @param strp the address of a value which, on success, contains a pointer to
 *             the allocated string. This pointer SHOULD be passed to `free()`
 *             to release the allocated storage.
 * @param fmt  a printf-style format string.
 * @param ap   the list with arguments to be printed according to <b>fmt</b>.
 *
 * @returns the number of characters written, not counting the terminating null
 * byte, or a negative number on error.
 *
 * @see asprintf()
 */
int vasprintf(char **strp, const char *fmt, va_list ap);

#endif // !_GNU_SOURCE

#ifdef __cplusplus
}
#endif

#endif // !LELY_NO_STDIO

#endif // !LELY_LIBC_STDIO_H_
