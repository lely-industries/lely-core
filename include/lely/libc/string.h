/**@file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<string.h>` and defines any missing functionality.
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

#ifndef LELY_LIBC_STRING_H_
#define LELY_LIBC_STRING_H_

#include <lely/features.h>

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !LELY_NO_MALLOC

#if !(_MSC_VER >= 1400) && !(_POSIX_C_SOURCE >= 200809L) \
		&& !defined(__MINGW32__)
/**
 * Duplicates the string at <b>s</b>.
 *
 * @returns a pointer to a new string, or NULL on error. The returned pointer
 * can be passed to free().
 */
char *strdup(const char *s);
#endif

#if !(_POSIX_C_SOURCE >= 200809L)
/**
 * Duplicates at most <b>size</b> characters (excluding the terminating null
 * byte) from the string at <b>s</b>. The resulting string is null-terminated.
 *
 * @returns a pointer to a new string, or NULL on error. The returned pointer
 * can be passed to free().
 */
char *strndup(const char *s, size_t size);
#endif

#endif // !LELY_NO_MALLOC

#if !(_MSC_VER >= 1400) && !(_POSIX_C_SOURCE >= 200809L) \
		&& !defined(__MINGW32__)
/**
 * Computes the length of the string at <b>s</b>, not including the terminating
 * null byte. At most <b>maxlen</b> characters are examined.
 *
 * @returns the smaller of the length of the string at <b>s</b> or
 * <b>maxlen</b>.
 */
size_t strnlen(const char *s, size_t maxlen);
#endif

#ifdef __cplusplus
}
#endif

#endif // !LELY_LIBC_STRING_H_
