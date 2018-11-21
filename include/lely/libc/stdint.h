/**@file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<stdint.h>`, if it exists, and defines any missing functionality.
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

#ifndef LELY_LIBC_STDINT_H_
#define LELY_LIBC_STDINT_H_

#include <lely/features.h>

#ifndef LELY_HAVE_STDINT_H
#if (__STDC_VERSION__ >= 199901L || __cplusplus >= 201103L \
		|| _MSC_VER >= 1600) \
		&& __has_include(<stdint.h>)
#define LELY_HAVE_STDINT_H 1
#endif
#endif

#if LELY_HAVE_STDINT_H
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS 1
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS 1
#endif
#include <stdint.h>
#else

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wlong-long"
#endif

#ifdef __INT8_TYPE__
typedef __INT8_TYPE__ int8_t;
#else
typedef signed char int8_t;
#endif

#ifdef __INT16_TYPE__
typedef __INT16_TYPE__ int16_t;
#else
typedef short int16_t;
#endif

#ifdef __INT32_TYPE__
typedef __INT32_TYPE__ int32_t;
#else
typedef int int32_t;
#endif

#ifdef __INT64_TYPE__
__extension__ typedef __INT64_TYPE__ int64_t;
#elif LONG_BIT == 64
typedef long int64_t;
#else
__extension__ typedef long long int64_t;
#endif

#ifdef __UINT8_TYPE__
typedef __UINT8_TYPE__ uint8_t;
#else
typedef unsigned char uint8_t;
#endif

#ifdef __UINT16_TYPE__
typedef __UINT16_TYPE__ uint16_t;
#else
typedef unsigned short uint16_t;
#endif

#ifdef __UINT32_TYPE__
typedef __UINT32_TYPE__ uint32_t;
#else
typedef unsigned int uint32_t;
#endif

#ifdef __UINT64_TYPE__
__extension__ typedef __UINT64_TYPE__ uint64_t;
#elif LONG_BIT == 64
typedef unsigned long uint64_t;
#else
__extension__ typedef unsigned long long uint64_t;
#endif

#ifdef __INT_LEAST8_TYPE__
typedef __INT_LEAST8_TYPE__ int_least8_t;
#else
typedef int8_t int_least8_t;
#endif

#ifdef __INT_LEAST16_TYPE__
typedef __INT_LEAST16_TYPE__ int_least16_t;
#else
typedef int16_t int_least16_t;
#endif

#ifdef __INT_LEAST32_TYPE__
typedef __INT_LEAST32_TYPE__ int_least32_t;
#else
typedef int32_t int_least32_t;
#endif

#ifdef __INT_LEAST64_TYPE__
__extension__ typedef __INT_LEAST64_TYPE__ int_least64_t;
#else
__extension__ typedef int64_t int_least64_t;
#endif

#ifdef __UINT_LEAST8_TYPE__
typedef __UINT_LEAST8_TYPE__ uint_least8_t;
#else
typedef uint8_t uint_least8_t;
#endif

#ifdef __UINT_LEAST16_TYPE__
typedef __UINT_LEAST16_TYPE__ uint_least16_t;
#else
typedef uint16_t uint_least16_t;
#endif

#ifdef __UINT_LEAST32_TYPE__
typedef __UINT_LEAST32_TYPE__ uint_least32_t;
#else
typedef uint32_t uint_least32_t;
#endif

#ifdef __UINT_LEAST64_TYPE__
__extension__ typedef __UINT_LEAST64_TYPE__ uint_least64_t;
#else
__extension__ typedef uint64_t uint_least64_t;
#endif

#ifdef __INT_FAST8_TYPE__
typedef __INT_FAST8_TYPE__ int_fast8_t;
#else
typedef int8_t int_fast8_t;
#endif

#ifdef __INT_FAST16_TYPE__
typedef __INT_FAST16_TYPE__ int_fast16_t;
#elif __WORDSIZE == 64 && LONG_BIT == 64
typedef int64_t int_fast16_t;
#else
typedef int32_t int_fast16_t;
#endif

#ifdef __INT_FAST32_TYPE__
typedef __INT_FAST32_TYPE__ int_fast32_t;
#elif __WORDSIZE == 64
typedef int64_t int_fast32_t;
#else
typedef int32_t int_fast32_t;
#endif

#ifdef __INT_FAST64_TYPE__
__extension__ typedef __INT_FAST64_TYPE__ int_fast64_t;
#else
__extension__ typedef int64_t int_fast64_t;
#endif

#ifdef __UINT_FAST8_TYPE__
typedef __UINT_FAST8_TYPE__ uint_fast8_t;
#else
typedef uint8_t uint_fast8_t;
#endif

#ifdef __UINT_FAST16_TYPE__
typedef __UINT_FAST16_TYPE__ uint_fast16_t;
#elif __WORDSIZE == 64
typedef uint64_t uint_fast16_t;
#else
typedef uint32_t uint_fast16_t;
#endif

#ifdef __UINT_FAST32_TYPE__
typedef __UINT_FAST32_TYPE__ uint_fast32_t;
#elif __WORDSIZE == 64
typedef uint64_t uint_fast32_t;
#else
typedef uint32_t uint_fast32_t;
#endif

#ifdef __UINT_FAST64_TYPE__
__extension__ typedef __UINT_FAST64_TYPE__ uint_fast64_t;
#else
__extension__ typedef uint64_t uint_fast64_t;
#endif

#ifdef __INTPTR_TYPE__
__extension__ typedef __INTPTR_TYPE__ intptr_t;
#elif __WORDSIZE == 64
__extension__ typedef int64_t intptr_t;
#elif __WORDSIZE == 32
typedef int32_t intptr_t;
#endif

#ifdef __UINTPTR_TYPE__
__extension__ typedef __UINTPTR_TYPE__ uintptr_t;
#elif __WORDSIZE == 64
__extension__ typedef uint64_t uintptr_t;
#else
typedef uint32_t uintptr_t;
#endif

#ifdef __INTMAX_TYPE__
__extension__ typedef __INTMAX_TYPE__ intmax_t;
#else
__extension__ typedef int64_t intmax_t;
#endif

#ifdef __UINTMAX_TYPE__
__extension__ typedef __UINTMAX_TYPE__ uintmax_t;
#else
__extension__ typedef uint64_t uintmax_t;
#endif

#endif // LELY_HAVE_STDINT_H

#ifndef INT8_C
#ifdef __INT8_C
#define INT8_C __INT8_C
#else
#define INT8_C(c) c
#endif
#endif

#ifndef INT8_MAX
#ifdef __INT8_MAX__
#define INT8_MAX __INT8_MAX__
#else
#define INT8_MAX INT8_C(127)
#endif
#endif

#ifndef INT8_MIN
#define INT8_MIN (-INT8_MAX - 1)
#endif

#ifndef INT16_C
#ifdef __INT16_C
#define INT16_C __INT16_C
#else
#define INT16_C(c) c
#endif
#endif

#ifndef INT16_MAX
#ifdef __INT16_MAX__
#define INT16_MAX __INT16_MAX__
#else
#define INT16_MAX INT16_C(32767)
#endif
#endif

#ifndef INT16_MIN
#define INT16_MIN (-INT16_MAX - 1)
#endif

#ifndef INT32_C
#ifdef __INT32_C
#define INT32_C __INT32_C
#else
#define INT32_C(c) c
#endif
#endif

#ifndef INT32_MAX
#ifdef __INT32_MAX__
#define INT32_MAX __INT32_MAX__
#else
#define INT32_MAX INT32_C(2147483647)
#endif
#endif

#ifndef INT32_MIN
#define INT32_MIN (-INT32_MAX - 1)
#endif

#ifndef INT64_C
#ifdef __INT64_C
#define INT64_C __INT32_C
#elif LONG_BIT == 64
#define INT64_C(c) c##L
#else
#define INT64_C(c) c##LL
#endif
#endif

#ifndef INT64_MAX
#ifdef __INT64_MAX__
#define INT64_MAX __INT64_MAX__
#else
#define INT64_MAX INT64_C(9223372036854775807)
#endif
#endif

#ifndef INT64_MIN
#define INT64_MIN (-INT64_MAX - 1)
#endif

#ifndef UINT8_C
#ifdef __UINT8_C
#define UINT8_C __UINT8_C
#else
#define UINT8_C(c) c
#endif
#endif

#ifndef UINT8_MAX
#ifdef __UINT8_MAX__
#define UINT8_MAX __UINT8_MAX__
#else
#define UINT8_MAX UINT8_C(255)
#endif
#endif

#ifndef UINT16_C
#ifdef __UINT16_C
#define UINT16_C __UINT16_C
#else
#define UINT16_C(c) c
#endif
#endif

#ifndef UINT16_MAX
#ifdef __UINT16_MAX__
#define UINT16_MAX __UINT16_MAX__
#else
#define UINT16_MAX UINT16_C(65535)
#endif
#endif

#ifndef UINT32_C
#ifdef __UINT32_C
#define UINT32_C __UINT32_C
#else
#define UINT32_C(c) c##U
#endif
#endif

#ifndef UINT32_MAX
#ifdef __UINT32_MAX__
#define UINT32_MAX __UINT32_MAX__
#else
#define UINT32_MAX UINT32_C(4294967295)
#endif
#endif

#ifndef UINT64_C
#ifdef __UINT64_C
#define UINT64_C __UINT64_C
#elif LONG_BIT == 64
#define UINT64_C(c) c##UL
#else
#define UINT64_C(c) c##ULL
#endif
#endif

#ifndef UINT64_MAX
#ifdef __UINT64_MAX__
#define UINT64_MAX __UINT64_MAX__
#else
#define UINT64_MAX UINT64_C(18446744073709551615)
#endif
#endif

#ifndef INT_LEAST8_MAX
#ifdef __INT_LEAST8_MAX__
#define INT_LEAST8_MAX __INT_LEAST8_MAX__
#else
#define INT_LEAST8_MAX INT8_MAX
#endif
#endif

#ifndef INT_LEAST8_MIN
#define INT_LEAST8_MIN (-INT_LEAST8_MAX - 1)
#endif

#ifndef INT_LEAST16_MAX
#ifdef __INT_LEAST16_MAX__
#define INT_LEAST16_MAX __INT_LEAST16_MAX__
#else
#define INT_LEAST16_MAX INT16_MAX
#endif
#endif

#ifndef INT_LEAST16_MIN
#define INT_LEAST16_MIN (-INT_LEAST16_MAX - 1)
#endif

#ifndef INT_LEAST32_MAX
#ifdef __INT_LEAST32_MAX__
#define INT_LEAST32_MAX __INT_LEAST32_MAX__
#else
#define INT_LEAST32_MAX INT32_MAX
#endif
#endif

#ifndef INT_LEAST32_MIN
#define INT_LEAST32_MIN (-INT_LEAST32_MAX - 1)
#endif

#ifndef INT_LEAST64_MAX
#ifdef __INT_LEAST64_MAX__
#define INT_LEAST64_MAX __INT_LEAST64_MAX__
#else
#define INT_LEAST64_MAX INT64_MAX
#endif
#endif

#ifndef INT_LEAST64_MIN
#define INT_LEAST64_MIN (-INT_LEAST64_MAX - 1)
#endif

#ifndef UINT_LEAST8_MAX
#ifdef __UINT_LEAST8_MAX__
#define UINT_LEAST8_MAX __UINT_LEAST8_MAX__
#else
#define UINT_LEAST8_MAX UINT8_MAX
#endif
#endif

#ifndef UINT_LEAST16_MAX
#ifdef __UINT_LEAST16_MAX__
#define UINT_LEAST16_MAX __UINT_LEAST16_MAX__
#else
#define UINT_LEAST16_MAX UINT16_MAX
#endif
#endif

#ifndef UINT_LEAST32_MAX
#ifdef __UINT_LEAST32_MAX__
#define UINT_LEAST32_MAX __UINT_LEAST32_MAX__
#else
#define UINT_LEAST32_MAX UINT32_MAX
#endif
#endif

#ifndef UINT_LEAST64_MAX
#ifdef __UINT_LEAST64_MAX__
#define UINT_LEAST64_MAX __UINT_LEAST64_MAX__
#else
#define UINT_LEAST64_MAX UINT64_MAX
#endif
#endif

#ifndef INT_FAST8_MAX
#ifdef __INT_FAST8_MAX__
#define INT_FAST8_MAX __INT_FAST8_MAX__
#else
#define INT_FAST8_MAX INT8_MAX
#endif
#endif

#ifndef INT_FAST8_MIN
#define INT_FAST8_MIN (-INT_FAST8_MAX - 1)
#endif

#ifndef INT_FAST16_MAX
#ifdef __INT_FAST16_MAX__
#define INT_FAST16_MAX __INT_FAST16_MAX__
#elif __WORDSIZE == 64
#define INT_FAST16_MAX INT64_MAX
#else
#define INT_FAST16_MAX INT32_MAX
#endif
#endif

#ifndef INT_FAST16_MIN
#define INT_FAST16_MIN (-INT_FAST16_MAX - 1)
#endif

#ifndef INT_FAST32_MAX
#ifdef __INT_FAST32_MAX__
#define INT_FAST32_MAX __INT_FAST32_MAX__
#elif __WORDSIZE == 64
#define INT_FAST32_MAX INT64_MAX
#else
#define INT_FAST32_MAX INT32_MAX
#endif
#endif

#ifndef INT_FAST32_MIN
#define INT_FAST32_MIN (-INT_FAST32_MAX - 1)
#endif

#ifndef INT_FAST64_MAX
#ifdef __INT_FAST64_MAX__
#define INT_FAST64_MAX __INT_FAST64_MAX__
#else
#define INT_FAST64_MAX INT64_MAX
#endif
#endif

#ifndef INT_FAST64_MIN
#define INT_FAST64_MIN (-INT_FAST64_MAX - 1)
#endif

#ifndef UINT_FAST8_MAX
#ifdef __UINT_FAST8_MAX__
#define UINT_FAST8_MAX __UINT_FAST8_MAX__
#else
#define UINT_FAST8_MAX UINT8_MAX
#endif
#endif

#ifndef UINT_FAST16_MAX
#ifdef __UINT_FAST16_MAX__
#define UINT_FAST16_MAX __UINT_FAST16_MAX__
#elif __WORDSIZE == 64
#define UINT_FAST64_MAX UINT16_MAX
#else
#define UINT_FAST32_MAX UINT16_MAX
#endif
#endif

#ifndef UINT_FAST32_MAX
#ifdef __UINT_FAST32_MAX__
#define UINT_FAST32_MAX __UINT_FAST32_MAX__
#elif __WORDSIZE == 64
#define UINT_FAST64_MAX UINT32_MAX
#else
#define UINT_FAST32_MAX UINT32_MAX
#endif
#endif

#ifndef UINT_FAST64_MAX
#ifdef __UINT_FAST64_MAX__
#define UINT_FAST64_MAX __UINT_FAST64_MAX__
#else
#define UINT_FAST64_MAX UINT64_MAX
#endif
#endif

#ifndef INTPTR_MAX
#ifdef __INTPTR_MAX__
#define INTPTR_MAX __INTPTR_MAX__
#elif __WORDSIZE == 64
#define INTPTR_MAX INT64_MAX
#else
#define INTPTR_MAX INT32_MAX
#endif
#endif

#ifndef INTPTR_MIN
#define INTPTR_MIN (-INTPTR_MAX - 1)
#endif

#ifndef UINTPTR_MAX
#ifdef __UINTPTR_MAX__
#define UINTPTR_MAX __UINTPTR_MAX__
#elif __WORDSIZE == 64
#define UINTPTR_MAX UINT64_MAX
#else
#define UINTPTR_MAX UINT32_MAX
#endif
#endif

#ifndef INTMAX_C
#ifdef __INTMAX_C
#define INTMAX_C __INTMAX_C
#else
#define INTMAX_C INT64_C
#endif
#endif

#ifndef INTMAX_MAX
#ifdef __INTMAX_MAX__
#define INTMAX_MAX __INTMAX_MAX__
#else
#define INTMAX_MAX INT64_MAX
#endif
#endif

#ifndef INTMAX_MIN
#define INTMAX_MIN (-INTMAX_MAX - 1)
#endif

#ifndef UINTMAX_C
#ifdef __UINTMAX_C
#define UINTMAX_C __UINTMAX_C
#else
#define UINTMAX_C UINT64_C
#endif
#endif

#ifndef UINTMAX_MAX
#ifdef __UINTMAX_MAX__
#define UINTMAX_MAX __UINTMAX_MAX__
#else
#define UINTMAX_MAX UINT64_MAX
#endif
#endif

#ifndef PTRDIFF_MAX
#ifdef __PTRDIFF_MAX__
#define PTRDIFF_MAX __PTRDIFF_MAX__
#else
#define PTRDIFF_MAX INTPTR_MAX
#endif
#endif

#ifndef PTRDIFF_MIN
#define PTRDIFF_MIN (-PTRDIFF_MAX - 1)
#endif

#ifndef SIG_ATOMIC_MAX
#ifdef __SIG_ATOMIC_MAX__
#define SIG_ATOMIC_MAX __SIG_ATOMIC_MAX__
#else
#define SIG_ATOMIC_MAX 2147483647
#endif
#endif

#ifndef SIG_ATOMIC_MIN
#ifdef __SIG_ATOMIC_MIN__
#define SIG_ATOMIC_MIN __SIG_ATOMIC_MIN__
#else
#define SIG_ATOMIC_MIN (-SIG_ATOMIC_MAX - 1)
#endif
#endif

#ifndef SIZE_MAX
#ifdef __SIZE_MAX__
#define SIZE_MAX __SIZE_MAX__
#else
#define SIZE_MAX UINTPTR_MAX
#endif
#endif

#ifndef WCHAR_MAX
#ifdef __WCHAR_MAX__
#define WCHAR_MAX __WCHAR_MAX__
#elif defined(_WIN32)
#define WCHAR_MAX 65535
#elif __WORDSIZE == 64
#define WCHAR_MAX 2147483647
#else
#define WCHAR_MAX 2147483647L
#endif
#endif

#ifndef WCHAR_MIN
#ifdef __WCHAR_MIN__
#define WCHAR_MIN __WCHAR_MIN__
#elif defined(_WIN32)
#define WCHAR_MIN 0
#else
#define WCHAR_MIN (-WCHAR_MAX - 1)
#endif
#endif

#ifndef WINT_MAX
#ifdef __WINT_MAX__
#define WINT_MAX __WINT_MAX__
#elif defined(_WIN32)
#define WINT_MAX 65535
#else
#define WINT_MAX 4294967295U
#endif
#endif

#ifndef WINT_MIN
#ifdef __WINT_MIN__
#define WINT_MIN __WINT_MIN__
#else
#define WINT_MIN 0
#endif
#endif

#endif // !LELY_LIBC_STDINT_H_
