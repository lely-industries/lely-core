/*!\file
 * This header file is part of the Lely libraries; it contains the compiler
 * feature definitions.
 *
 * \copyright 2013-2018 Lely Industries N.V.
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

#ifndef LELY_FEATURES_H_
#define LELY_FEATURES_H_

#if defined(__STDC_VERSION__) && __STDC_VERSION__ < 199901L
#error This file requires compiler and library support for the ISO C99 standard.
#endif

#if defined(__cplusplus) && __cplusplus < 199711L
#error This file requires compiler and library support for the ISO C++98 standard.
#endif

#ifdef _MSC_VER
#if _MSC_VER < 1800
#error This file requires Microsoft Visual C++ 2013 or later.
#endif
// Disable warnings about deprecated POSIX functions.
#pragma warning(disable: 4996)
#endif

#ifdef _WIN32
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

#ifndef __CLANG_PREREQ
#if defined(__clang__) && defined(__clang_major__) && defined(__clang_minor__)
#define __CLANG_PREREQ(major, minor) \
	((__clang_major__ << 16) + __clang_minor__ >= ((major) << 16) + (minor))
#else
#define __CLANG_PREREQ(major, minor)	0
#endif
#endif

#ifndef __GNUC_PREREQ
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#define __GNUC_PREREQ(major, minor) \
	((__GNUC__ << 16) + __GNUC_MINOR__ >= ((major) << 16) + (minor))
#else
#define __GNUC_PREREQ(major, minor)	0
#endif
#endif

#ifndef __has_attribute
#define __has_attribute(x)	0
#endif

#ifndef __has_builtin
#define __has_builtin(x)	0
#endif

#ifndef __has_declspec_attribute
#define __has_declspec_attribute(x)	0
#endif

#ifndef __has_extension
#define __has_extension	__has_feature
#endif

#ifndef __has_feature
#define __has_feature(x)	0
#endif

#ifndef __has_include
#define __has_include(x)	1
#endif

#ifndef __STDC_HOSTED__
#if defined(_MSC_VER) && _MSC_VER < 1900
#define __STDC_HOSTED__	1
#endif
#endif

#ifndef __STDC_NO_ATOMICS__
// GCC versions older than 4.9 do not properly advertise the absence of
// <stdatomic.h>.
#if defined(__cplusplus) || defined(_MSC_VER) \
		|| (defined(__GNUC__) && !__GNUC_PREREQ(4, 9))
#define __STDC_NO_ATOMICS__	1
#endif
#endif

#ifndef __STDC_NO_THREADS__
// Although recent versions of Cygwin do provide <threads.h>, it requires
// <machine/_threads.h>, which is missing.
#if defined(__cplusplus) || defined(_MSC_VER) || defined(__CYGWIN__)
#define __STDC_NO_THREADS__	1
#endif
#endif

#ifndef __STDC_NO_VLA__
#if defined(__cplusplus) || defined(_MSC_VER)
#define __STDC_NO_VLA__	1
#endif
#endif

#ifdef __cplusplus

#ifndef __cpp_exceptions
#if (defined(_MSC_VER) && _HAS_EXCEPTIONS) \
		|| (defined(__GNUC__) && defined(__EXCEPTIONS)) \
		|| (defined(__clang__) && __has_feature(cxx_exceptions))
#define __cpp_exceptions	__cplusplus
#endif
#endif

#ifndef __cpp_rtti
#if (defined(_MSC_VER) && _CPPRTTI) \
		|| (defined(__GNUC__) && defined(__GXX_RTTI)) \
		|| (defined(__clang__) && __has_feature(cxx_rtti))
#define __cpp_rtti	__cplusplus
#endif
#endif

#endif // __cplusplus

//! Specifies the alignment requirement of the declared object or member.
#if !defined(_Alignas) && !(__STDC_VERSION__ >= 201112L \
		&& (__GNUC_PREREQ(4, 7) || __has_feature(c_alignas)))
#if __cplusplus >= 201103L \
		&& (__GNUC_PREREQ(4, 8) || __has_feature(cxx_alignas))
#define _Alignas	alignas
#elif defined(__GNUC__) || __has_attribute(__aligned__)
#define _Alignas(x)	__attribute__((__aligned__(x)))
#elif defined(_MSC_VER) || defined(__declspec) \
		|| __has_declspec_attribute(align)
#define _Alignas(x)	__declspec(align(x))
#else
#define _Alignas(x)
#endif
#endif

//! Yields the alignment requirement of its operand type.
#if !defined(_Alignof) && !(__STDC_VERSION__ >= 201112L \
		&& (__GNUC_PREREQ(4, 7) || __has_feature(c_alignof)))
#if __cplusplus >= 201103L \
		&& (__GNUC_PREREQ(4, 8) || __has_feature(cxx_alignof))
#define _Alignof	alignof
#elif defined(__GNUC__)
#define _Alignof(x)	__alignof__(x)
#elif defined(_MSC_VER)
#define _Alignof(x)	__alignof(x)
#else
#include <stddef.h>
#define _Alignof(type)	(offsetof(struct { char c; type x; }, x))
#endif
#endif

/*!
 * A function declared with a `_Noreturn` function specifier SHALL not return to
 * its caller.
 */
#if !defined(_Noreturn) && !(__STDC_VERSION__ >= 201112L \
		&& (__GNUC_PREREQ(4, 7) || __CLANG_PREREQ(3, 3)))
#if defined(__GNUC__) || __has_attribute(__noreturn__)
#define _Noreturn	__attribute__((__noreturn__))
#elif defined(_MSC_VER) || defined(__declspec) \
		|| __has_declspec_attribute(noreturn)
#define _Noreturn	__declspec(noreturn)
#else
#define _Noreturn
#endif
#endif

/*!
 * If the value of \a expr compares unequal to 0, the declaration has no effect.
 * Otherwise, the constraint is violated and a diagnostic message is produced
 * which may, depending on the implementation, include the text of \a msg.
 */
#if !defined(_Static_assert) && !(__STDC_VERSION__ >= 201112L \
		&& (__GNUC_PREREQ(4, 6) || __has_feature(c_static_assert)))
#if __cplusplus >= 201103L \
		&& (__GNUC_PREREQ(4, 3) || __has_feature(cxx_static_assert))
#define _Static_assert	static_assert
#else
#ifdef __COUNTER__
#define _Static_assert(expr, msg) \
	__Static_assert(expr, __COUNTER__)
#else
#define _Static_assert(expr, msg) \
	__Static_assert(expr, __LINE__)
#endif
#define __Static_assert(expr, n) \
	___Static_assert(expr, n)
#define ___Static_assert(expr, n) \
	typedef char ___Static_assert_##n[(expr) ? 1 : -1]
#endif
#endif

/*!
 * An object whose identifier is declared with the storage-class specifier
 * `_Thread_local` has thread storage duration. Its lifetime is the entire
 * execution of the thread for which it is created, and its stored value is
 * initialized when the thread is started. There is a distinct object per
 * thread, and use of the declared name in an expression refers to the object
 * associated with the thread evaluating the expression.
 */
#if !defined(_Thread_local) && !(__STDC_VERSION__ >= 201112L \
		&& (__GNUC_PREREQ(4, 7) || __has_feature(c_thread_local)))
#if __cplusplus >= 201103L \
		&& (__GNUC_PREREQ(4, 8) || __has_feature(cxx_thread_local))
#define _Thread_local	thread_local
#elif defined(__GNUC__)
#define _Thread_local	__thread
#elif defined(_MSC_VER) || defined(__declspec) \
		|| __has_declspec_attribute(thread)
#define _Thread_local	__declspec(thread)
#else
#define _Thread_local
#endif
#endif

#ifndef __builtin_expect
#if defined(__GNUC__) || __has_builtin(__builtin_expect)
#else
#define __builtin_expect(exp, c)	(exp)
#endif
#endif

#if !defined(__cdecl) && !defined(_MSC_VER)
#define __cdecl
#endif

#ifndef __deprecated
//! Mark a function as deprecated.
#if defined(__GNUC__) || __has_attribute(__deprecated__)
#define __deprecated	__attribute__((__deprecated__))
#else
#define __deprecated
#endif
#endif

#ifndef __dllexport
#if defined(_WIN32) && (defined(_MSC_VER) || defined(__declspec) \
		|| __has_declspec_attribute(dllexport))
#define __dllexport	__declspec(dllexport)
#elif defined(__GNUC__)
#define __dllexport	__attribute__((visibility("default")))
#else
#define __dllexport
#endif
#endif

#ifndef __dllimport
#if defined(_WIN32) && (defined(_MSC_VER) || defined(__declspec) \
		|| __has_declspec_attribute(dllimport))
#define __dllimport	__declspec(dllimport)
#else
#define __dllimport
#endif
#endif

#if !defined(__extension__) && !defined(__GNUC__)
#define __extension__
#endif

#ifndef __format_printf
#if defined(__GNUC__) || __has_attribute(__format__)
#ifdef __MINGW32__
#define __format_printf(i, j) \
	__attribute__((__format__(__gnu_printf__, (i), (j))))
#else
#define __format_printf(i, j) \
	__attribute__((__format__(__printf__, (i), (j))))
#endif
#else
#define	__format_printf(i, j)
#endif
#endif

#if !defined(__cplusplus) && defined(_MSC_VER) && _MSC_VER < 1900
// Microsoft Visual C++ 2013 and earlier do not support the C99 inline keyword.
#ifndef inline
#define inline	__inline
#endif
#endif

#ifndef __likely
/*!
 * Indicates to the compiler that the expression is most-likely true. Subject to
 * the same considerations as __unlikely().
 */
#define __likely(x)	__builtin_expect(!!(x), 1)
#endif

#ifndef __unlikely
/*!
 * Indicates to the compiler that the expression is most-likely false. This
 * should only be used in performance-critical sections as compilers are very
 * good at branch prediction nowadays. The only exception to this rule is error
 * and exception checking, e.g. `if (__unlikely(!ptr)) { handle NULL pointer }`,
 * since we never optimize exceptions.
 *
 * \see __likely()
 */
#define __unlikely(x)	__builtin_expect(!!(x), 0)
#endif

#ifndef __unused_arg
//! Suppresses a compiler warning about an unused function argument.
#ifdef _MSC_VER
#define __unused_arg	__pragma(warning(suppress: 4100))
#elif defined(__GNUC__) || __has_attribute(__unused__)
#define __unused_arg	__attribute__((__unused__))
#else
#define __unused_arg
#endif
#endif

#ifndef __unused_var
//! Suppresses a compiler warning about an unused variable.
#ifdef _MSC_VER
#define __unused_var(x)	__pragma(warning(suppress: 4100 4101)) x
#else
#define __unused_var(x)	((void)(0 ? ((void)(x), 0) : 0))
#endif
#endif

#ifndef __WORDSIZE
//! The native word size (in bits).
#if !defined(__ILP32__) && (defined(__LP64__) || defined(_WIN64) \
		|| defined(_M_AMD64) || defined(__amd64__) || defined(_M_IA64) \
		|| defined(__ia64__) || defined(_M_X64) || defined(__x86_64__) \
		|| defined(__aarch64__))
#define __WORDSIZE	64
#else
#define __WORDSIZE	32
#endif
#endif

#ifndef LONG_BIT
//! The number of bits in a long.
#ifdef _WIN32
// long remains 32-bits on 64-bit Windows.
#define LONG_BIT	32
#else
#define LONG_BIT	__WORDSIZE
#endif
#endif

#ifndef LEVEL1_DCACHE_LINESIZE
//! The size (in bytes) of the level 1 (L1) data cache.
#define LEVEL1_DCACHE_LINESIZE	64
#endif

#endif // LELY_FEATURES_H_
