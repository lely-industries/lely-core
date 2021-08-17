/**@file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<strings.h>`, if it exists, and defines any missing functionality.
 *
 * @copyright 2014-2020 Lely Industries N.V.
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

#ifndef LELY_COMPAT_STRINGS_H_
#define LELY_COMPAT_STRINGS_H_

#include <lely/compat/features.h>

#ifndef LELY_HAVE_STRINGS_H
#if !LELY_NO_HOSTED && (_POSIX_C_SOURCE >= 200112L || defined(__NEWLIB__))
#define LELY_HAVE_STRINGS_H 1
#endif
#endif

#if LELY_HAVE_STRINGS_H
#include <strings.h>
#else // !LELY_HAVE_STRINGS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compares the string at <b>s1</b> to the string at <b>s2</b>, ignoring
 * differences in case.
 *
 * @returns an integer greater than, equal to, or less than 0 if the string at
 * <b>s1</b> is greater than, equal to, or less than the string at <b>s2</b>.
 */
int lely_compat_strcasecmp(const char *s1, const char *s2);

#ifndef strcasecmp
#define strcasecmp lely_compat_strcasecmp
#endif

/**
 * Compares at most <b>n</b> characters from the the string at <b>s1</b> to the
 * string at <b>s2</b>, ignoring differences in case.
 *
 * @returns an integer greater than, equal to, or less than 0 if the string at
 * <b>s1</b> is greater than, equal to, or less than the string at <b>s2</b>.
 */
int lely_compat_strncasecmp(const char *s1, const char *s2, size_t n);

#ifndef strncasecmp
#define strncasecmp lely_compat_strncasecmp
#endif

#ifdef __cplusplus
}
#endif

#endif // !LELY_HAVE_STRINGS_H

#endif // !LELY_COMPAT_STRINGS_H_
