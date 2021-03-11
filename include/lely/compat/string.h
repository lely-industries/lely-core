/**@file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<string.h>` and defines any missing functionality.
 *
 * @copyright 2015-2020 Lely Industries N.V.
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

#ifndef LELY_COMPAT_STRING_H_
#define LELY_COMPAT_STRING_H_

#include <lely/compat/features.h>

#if LELY_NO_HOSTED
#include <stddef.h>
#else
#include <string.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if LELY_NO_HOSTED

#ifndef memcpy
#define memcpy lely_compat_memcpy
#endif

#ifndef memmove
#define memmove lely_compat_memmove
#endif

#ifndef memcmp
#define memcmp lely_compat_memcmp
#endif

#ifndef strcmp
#define strcmp lely_compat_strcmp
#endif

#ifndef strncmp
#define strncmp lely_compat_strncmp
#endif

#ifndef memset
#define memset lely_compat_memset
#endif

#ifndef strlen
#define strlen lely_compat_strlen
#endif

/**
 * Copies <b>n</b> bytes from the object pointed to by <b>s2</b> into the object
 * pointed to by <b>s1</b>. If copying takes place between objects that overlap,
 * the behavior is undefined.
 *
 * @returns <b>s1</b>.
 */
void *lely_compat_memcpy(void *s1, const void *s2, size_t n);

/**
 * Copies <b>n</b> bytes from the object pointed to by <b>s2</b> into the object
 * pointed to by <b>s1</b>. Copying takes place as if the <b>n</b> bytes from
 * the object pointed to by <b>s2</b> are first copied into a temporary array of
 * <b>n</b> bytes that does not overlap the objects pointed to by <b>s1</b> and
 * <b>s2</b>, and then the <b>n</b> bytes from the temporary array are copied
 * into the object pointed to by <b>s1</b>.
 *
 * @returns <b>s1</b>.
 */
void *lely_compat_memmove(void *s1, const void *s2, size_t n);

/**
 * Compares the first <b>n</b> bytes (each interpreted as an `unsigned char`) of
 * the object pointed to by <b>s1</b> to the first <b>n</b> btyes of the object
 * pointed to by <b>s2</b>.
 *
 * The sign of a non-zero return value shall be determined by the sign of the
 * difference between the values of the first pair of bytes (both interpreted as
 * type `unsigned char`) that differ in the objects being compared.
 *
 * @returns an integer greater than, equal to, or less than 0, if the object
 * pointed to by <b>s1</b> is greater than, equal to, or less than the object
 * pointed to by <b>s2</b>.
 */
int lely_compat_memcmp(const void *s1, const void *s2, size_t n);

/**
 * Compares the string pointed to by <b>s1</b> to the string pointed to by
 * <b>s2</b>.
 *
 * The sign of a non-zero return value shall be determined by the sign of the
 * difference between the values of the first pair of bytes (both interpreted as
 * type `unsigned char`) that differ in the strings being compared.
 *
 * @returns an integer greater than, equal to, or less than 0, if the string
 * pointed to by <b>s1</b> is greater than, equal to, or less than the string
 * pointed to by <b>s2</b>.
 */
int lely_compat_strcmp(const char *s1, const char *s2);

/**
 * Compares not more than <b>n</n> bytes (bytes that follow a null character are
 * not compared) from the array pointed to by <b>s1</b> to the array pointed to
 * by <b>s2</b>.
 *
 * The sign of a non-zero return value shall be determined by the sign of the
 * difference between the values of the first pair of bytes (both interpreted as
 * type `unsigned char`) that differ in the strings being compared.
 *
 * @returns an integer greater than, equal to, or less than 0, if the possible
 * null-terminated array pointed to by <b>s1</b> is greater than, equal to, or
 * less than the possibly null-terminated array pointed to by <b>s2</b>.
 */
int lely_compat_strncmp(const char *s1, const char *s2, size_t n);

/**
 * Copies <b>c</b> (converted to an `unsigned char`) into each of the first
 * <b>n</b> bytes of the object pointed to by <b>s</b>.
 *
 * @returns <b>s</b>.
 */
void *lely_compat_memset(void *s, int c, size_t n);

/**
 * Computes the length of the string pointed to by <b>s</b>.
 *
 * @returns the number of bytes that precede the terminating null character.
 */
size_t lely_compat_strlen(const char *s);

#endif // LELY_NO_HOSTED

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

#if LELY_NO_HOSTED \
		|| (!(_MSC_VER >= 1400) && !(_POSIX_C_SOURCE >= 200809L) \
				&& !defined(__MINGW32__))

#ifndef strnlen
#define strnlen lely_compat_strnlen
#endif

/**
 * Computes the length of the string at <b>s</b>, not including the terminating
 * null byte. At most <b>maxlen</b> characters are examined.
 *
 * @returns the smaller of the length of the string at <b>s</b> or
 * <b>maxlen</b>.
 */
size_t lely_compat_strnlen(const char *s, size_t maxlen);
#endif

#ifdef __cplusplus
}
#endif

#endif // !LELY_COMPAT_STRING_H_
