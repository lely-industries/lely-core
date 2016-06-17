/*!\file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<stdio.h>` and defines any missing functionality.
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

#ifndef LELY_LIBC_STDIO_H
#define LELY_LIBC_STDIO_H

#include <lely/libc/libc.h>

#include <stdio.h>

#ifndef _GNU_SOURCE

#ifdef __cplusplus
extern "C" {
#endif

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
 * byte, or a* negative number on error.
 *
 * \see vasprintf()
 */
LELY_LIBC_EXTERN int asprintf(char **strp, const char *fmt, ...);

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
 * byte, or a* negative number on error.
 *
 * \see asprintf()
 */
LELY_LIBC_EXTERN int vasprintf(char **strp, const char *fmt, va_list ap);

#ifdef __cplusplus
}
#endif

#endif // !_GNU_SOURCE

#endif

