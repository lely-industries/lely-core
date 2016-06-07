/*!\file
 * This header file is part of the utilities library; it contains the C++
 * exception declarations.
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

#ifndef LELY_UTIL_EXCEPTION_HPP
#define LELY_UTIL_EXCEPTION_HPP

#include <lely/util/util.h>

#include <exception>

#if !defined(noexcept) && !(__cplusplus >= 201103L && (__GNUC_PREREQ(4, 6) \
		|| __has_feature(cxx_noexcept)))
#define noexcept
#endif

#ifndef nothrow_or_noexcept
#if __cplusplus >= 201103L
#define nothrow_or_noexcept	noexcept
#else
#define nothrow_or_noexcept	throw()
#endif
#endif

#ifndef throw_or_abort
/*!
 * If exceptions are disabled, aborts the process instead of throwing an
 * exception.
 */
#if defined(__EXCEPTIONS) || __has_feature(cxx_exceptions)
#define throw_or_abort(e)	throw(e)
#else
#define throw_or_abort(e)	__throw_or_abort((e).what())
#endif
#endif

//! Aborts the process instead of throwing an exception.
LELY_UTIL_EXTERN _Noreturn void __throw_or_abort(const char *what) noexcept;

#endif

