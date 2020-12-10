/**@file
 * This header file is part of the Lely libraries; it contains the compiler
 * feature definitions.
 *
 * @copyright 2013-2020 Lely Industries N.V.
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

#ifndef LELY_FEATURES_H_
#define LELY_FEATURES_H_

#if defined(__STDC_VERSION__) && __STDC_VERSION__ < 199901L
#error This file requires compiler and library support for the ISO C99 standard.
#endif

#if defined(__cplusplus) && __cplusplus < 201103L
#error This file requires compiler and library support for the ISO C++11 standard.
#endif

#ifdef _MSC_VER
#if _MSC_VER < 1900
#error This file requires Microsoft Visual C++ 2015 or later.
#endif
// Disable warnings about deprecated POSIX functions.
#pragma warning(disable : 4996)
#endif

#if _WIN32
// Windows 7 is the minimum supported version.
#if !defined(NTDDI_VERSION) || (NTDDI_VERSION < NTDDI_WIN7)
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN7
#endif
#if !defined(_WIN32_WINNT) || _WIN32_WINNT < _WIN32_WINNT_WIN7
#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#endif
#if !defined(WINVER) || WINVER < _WIN32_WINNT
#undef WINVER
#define WINVER _WIN32_WINNT
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windef.h>
#endif

// <limits.h> is guaranteed to be present, even in freestanding environments,
// and will typically include <features.h>, which we need but cannot portably
// include directly.
#include <limits.h>

// Include a (platform-specific) header which defines the POSIX feature test
// macros.
#ifdef __GLIBC__
#include <bits/posix_opt.h>
#elif defined(__NEWLIB__)
#include <sys/features.h>
#endif

#ifndef CLANG_PREREQ
#if defined(__clang__) && defined(__clang_major__) && defined(__clang_minor__)
#define CLANG_PREREQ(major, minor) \
	((__clang_major__ << 16) + __clang_minor__ >= ((major) << 16) + (minor))
#else
#define CLANG_PREREQ(major, minor) 0
#endif
#endif

#ifndef GNUC_PREREQ
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#define GNUC_PREREQ(major, minor) \
	((__GNUC__ << 16) + __GNUC_MINOR__ >= ((major) << 16) + (minor))
#else
#define GNUC_PREREQ(major, minor) 0
#endif
#endif

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#ifndef __has_declspec_attribute
#define __has_declspec_attribute(x) 0
#endif

#ifndef __has_extension
#define __has_extension __has_feature
#endif

#ifndef __has_feature
#define __has_feature(x) 0
#endif

#ifndef __has_include
#define __has_include(x) 1
#endif

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS 1
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS 1
#endif

#ifndef __cplusplus

#ifndef __STDC_NO_ATOMICS__
// GCC versions older than 4.9 do not properly advertise the absence of
// <stdatomic.h>.
// clang-format off
#if defined(_MSC_VER) \
		|| (defined(__GNUC__) && !GNUC_PREREQ(4, 9) \
				&& !defined(__clang__)) \
		|| (defined(__clang__) && !__has_extension(c_atomic))
// clang-format on
#define __STDC_NO_ATOMICS__ 1
#endif
#endif // !__STDC_NO_ATOMICS__

#ifndef __STDC_NO_THREADS__
// Although recent versions of Cygwin do provide <threads.h>, it requires
// <machine/_threads.h>, which is missing.
#if defined(_MSC_VER) || defined(__CYGWIN__)
#define __STDC_NO_THREADS__ 1
#endif
#endif // !__STDC_NO_THREADS__

#ifndef __STDC_NO_VLA__
#if defined(_MSC_VER)
#define __STDC_NO_VLA__ 1
#endif
#endif // !__STDC_NO_VLA__

#endif // !__cplusplus

#ifdef __cplusplus

#ifndef __cpp_exceptions
#if (defined(_MSC_VER) && _HAS_EXCEPTIONS) \
		|| (defined(__GNUC__) && defined(__EXCEPTIONS)) \
		|| (defined(__clang__) && __has_feature(cxx_exceptions))
#define __cpp_exceptions __cplusplus
#endif
#endif

#ifndef __cpp_rtti
#if (defined(_MSC_VER) && _CPPRTTI) \
		|| (defined(__GNUC__) && defined(__GXX_RTTI)) \
		|| (defined(__clang__) && __has_feature(cxx_rtti))
#define __cpp_rtti __cplusplus
#endif
#endif

#endif // __cplusplus

/// Specifies the alignment requirement of the declared object or member.
// clang-format off
#if !defined(_Alignas) && !(__STDC_VERSION__ >= 201112L \
		&& (GNUC_PREREQ(4, 7) || __has_feature(c_alignas)))
// clang-format on
#if __cplusplus >= 201103L && (GNUC_PREREQ(4, 8) || __has_feature(cxx_alignas))
#define _Alignas alignas
#elif defined(__GNUC__) || __has_attribute(__aligned__)
#define _Alignas(x) __attribute__((__aligned__(x)))
#elif defined(_MSC_VER) || defined(__declspec) \
		|| __has_declspec_attribute(align)
#define _Alignas(x) __declspec(align(x))
#else
#define _Alignas(x)
#endif
#endif

/// Yields the alignment requirement of its operand type.
// clang-format off
#if !defined(_Alignof) && !(__STDC_VERSION__ >= 201112L \
		&& (GNUC_PREREQ(4, 7) || __has_feature(c_alignof)))
// clang-format on
#if __cplusplus >= 201103L && (GNUC_PREREQ(4, 8) || __has_feature(cxx_alignof))
#define _Alignof alignof
#elif defined(__GNUC__)
#define _Alignof(x) __alignof__(x)
#elif defined(_MSC_VER)
#define _Alignof(x) __alignof(x)
#else
#include <stddef.h>
#define _Alignof(type) \
	(offsetof( \
			struct { \
				char c; \
				type x; \
			}, \
			x))
#endif
#endif

/**
 * A function declared with a `_Noreturn` function specifier SHALL not return to
 * its caller.
 */
// clang-format off
#if !defined(_Noreturn) && !(__STDC_VERSION__ >= 201112L \
		&& (GNUC_PREREQ(4, 7) || CLANG_PREREQ(3, 3)))
// clang-format on
#if defined(__GNUC__) || __has_attribute(__noreturn__)
#define _Noreturn __attribute__((__noreturn__))
#elif defined(_MSC_VER) || defined(__declspec) \
		|| __has_declspec_attribute(noreturn)
#define _Noreturn __declspec(noreturn)
#else
#define _Noreturn
#endif
#endif

/**
 * An object whose identifier is declared with the storage-class specifier
 * `_Thread_local` has thread storage duration. Its lifetime is the entire
 * execution of the thread for which it is created, and its stored value is
 * initialized when the thread is started. There is a distinct object per
 * thread, and use of the declared name in an expression refers to the object
 * associated with the thread evaluating the expression.
 */
// clang-format off
#if !defined(_Thread_local) && !(__STDC_VERSION__ >= 201112L \
		&& (GNUC_PREREQ(4, 7) || __has_feature(c_thread_local)))
// clang-format on
#if __cplusplus >= 201103L \
		&& (GNUC_PREREQ(4, 8) || __has_feature(cxx_thread_local))
#define _Thread_local thread_local
#elif defined(__GNUC__)
#define _Thread_local __thread
#elif defined(_MSC_VER) || defined(__declspec) \
		|| __has_declspec_attribute(thread)
#define _Thread_local __declspec(thread)
#else
#define _Thread_local
#endif
#endif

#ifndef format_printf__
#if defined(__GNUC__) || __has_attribute(__format__)
#ifdef __MINGW32__
#define format_printf__(i, j) \
	__attribute__((__format__(__gnu_printf__, (i), (j))))
#else
#define format_printf__(i, j) __attribute__((__format__(__printf__, (i), (j))))
#endif
#else
#define format_printf__(i, j)
#endif
#endif

#ifndef __WORDSIZE
/// The native word size (in bits).
// clang-format off
#if !defined(__ILP32__) && (defined(__LP64__) || _WIN64 || defined(_M_AMD64) \
		|| defined(__amd64__) || defined(_M_IA64) || defined(__ia64__) \
		|| defined(_M_X64) || defined(__x86_64__) \
		|| defined(__aarch64__))
// clang-format on
#define __WORDSIZE 64
#else
#define __WORDSIZE 32
#endif
#endif

#ifndef LONG_BIT
/// The number of bits in a long.
#if _WIN32
// long remains 32-bits on 64-bit Windows.
#define LONG_BIT 32
#else
#define LONG_BIT __WORDSIZE
#endif
#endif

#ifndef LEVEL1_DCACHE_LINESIZE
/**
 * The presumed size (in bytes) of a line in the L1 data cache. This value can
 * be used with _Alignas() to prevent variables from sharing a cache line, which
 * may increase the performance of some data structures in a multithreaded
 * environment.
 */
#define LEVEL1_DCACHE_LINESIZE 64
#endif

#if !LELY_NO_THREADS
#if defined(__cplusplus)
// <thread> (C++11 thread support library) is available.
#elif __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
// <threads.h> (C11 thread support library) is available.
#elif _POSIX_THREADS >= 200112L || defined(__MINGW32__)
// <pthread.h> (POSIX threads) is available.
#elif _WIN32
// Windows threads are available.
#else
#define LELY_NO_THREADS 1
#endif
#endif // !LELY_NO_THREADS

#if !LELY_NO_ATOMICS
#if LELY_NO_THREADS
// Disable atomic operations if threads are disabled.
#define LELY_NO_ATOMICS 1
#elif __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
// <stdatomic.h> (C11 atomic operations library) is available.
#elif defined(__clang__) && __has_extension(c_atomic)
// Clang C11 atomic operations are available.
#elif GNUC_PREREQ(4, 1)
// GCC __sync or __atomic builtins are available.
#else
#define LELY_NO_ATOMICS 1
#endif
#endif // !LELY_NO_ATOMICS

#if LELY_NO_ERRNO || LELY_NO_MALLOC
// Disable standard I/O if errno and/or dynamic memory allocation are disabled.
#undef LELY_NO_STDIO
#define LELY_NO_STDIO 1
#endif

#if _WIN32
#if LELY_NO_ERRNO
#error Windows requires errno.
#endif
#if LELY_NO_MALLOC
#error Windows requires dynamic memory allocation.
#endif
#endif // _WIN32

#define LELY_INGORE_EMPTY_TRANSLATION_UNIT \
	typedef int lely_ignore_empty_translation_unit__;

#ifndef LELY_VLA_SIZE_MAX
/// The maximum size (in bytes) of stack-allocated arrays.
#define LELY_VLA_SIZE_MAX 256
#endif

#endif // LELY_FEATURES_H_
