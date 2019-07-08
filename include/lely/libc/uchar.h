/**@file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<uchar.h>`, if it exists, and defines any missing functionality.
 *
 * @copyright 2014-2019 Lely Industries N.V.
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

#ifndef LELY_LIBC_UCHAR_H_
#define LELY_LIBC_UCHAR_H_

#include <lely/features.h>

#ifndef LELY_HAVE_UCHAR_H
#if (__STDC_VERSION__ >= 201112L || __cplusplus >= 201103L) \
		&& !defined(__NEWLIB__)
#define LELY_HAVE_UCHAR_H 1
#endif
#endif

#if LELY_HAVE_UCHAR_H
#include <uchar.h>
#else

#include <wchar.h>

#if __cplusplus >= 201103L
// char16_t and char32_t are built-in types in C++11 and later.
#else

#include <stdint.h>

typedef uint_least16_t char16_t;
typedef uint_least32_t char32_t;

#endif

#endif // !LELY_HAVE_UCHAR_H

#endif // !LELY_LIBC_UCHAR_H_
