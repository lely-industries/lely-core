/*!\file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<string.h>` and defines any missing functionality.
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

#ifndef LELY_LIBC_STRING_H
#define LELY_LIBC_STRING_H

#include <lely/libc/libc.h>

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !(_POSIX_C_SOURCE >= 200809L) && !defined(__MINGW32__)
/*!
 * Duplicates the string at \a s.
 *
 * \returns a pointer to a new string, or NULL on error. The returned pointer
 * can be passed to free().
 */
LELY_LIBC_EXTERN char * __cdecl strdup(const char *s);
#endif

#if !(_POSIX_C_SOURCE >= 200809L)
/*!
 * Duplicates at most \a size characters (excluding the terminating null byte)
 * from the string at \a s. The resulting string is null-terminated.
 *
 * \returns a pointer to a new string, or NULL on error. The returned pointer
 * can be passed to free().
 */
LELY_LIBC_EXTERN char * __cdecl strndup(const char *s, size_t size);
#endif

#if !(_POSIX_C_SOURCE >= 200809L) && !defined(__MINGW32__)
/*!
 * Computes the length of the string at \a s, not including the terminating null
 * byte. At most \a maxlen characters are examined.
 *
 * \returns the smaller of the length of the string at \a s or \a maxlen.
 */
LELY_LIBC_EXTERN  size_t __cdecl strnlen(const char *s, size_t maxlen);
#endif

#ifdef __cplusplus
}
#endif

#endif

