/**@file
 * This header file is part of the utilities library; it contains the C++
 * exception declarations.
 *
 * @copyright 2017-2018 Lely Industries N.V.
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

#ifndef LELY_UTIL_EXCEPTION_HPP
#define LELY_UTIL_EXCEPTION_HPP

#include <lely/util/errnum.h>

#include <stdexcept>
#if __cplusplus >= 201103L
#include <system_error>
#endif

#if !defined(noexcept) && !(__cplusplus >= 201103L && (__GNUC_PREREQ(4, 6) \
		|| __has_feature(cxx_noexcept))) && !(_MSC_VER >= 1900)
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
/**
 * If exceptions are disabled, aborts the process instead of throwing an
 * exception.
 */
#if __cpp_exceptions
#define throw_or_abort(e)	__throw_or_abort((e).what())
#else
#define throw_or_abort(e)	throw(e)
#endif
#endif

extern "C" {

/// Aborts the process instead of throwing an exception.
_Noreturn void __throw_or_abort(const char *what) noexcept;

}

namespace lely {

/**
 * The type of objects thrown as exceptions to report a system error with an
 * associated error code.
 */
#if __cplusplus >= 201103L
class error: public ::std::system_error {
#else
class error: public ::std::runtime_error {
#endif
public:
	error(errc_t errc = get_errc())
#if __cplusplus >= 201103L
	: ::std::system_error(errc, ::std::system_category())
#else
	: ::std::runtime_error(errc2str(errc))
#endif
	, m_errc(errc)
	{}

	errc_t errc() const noexcept { return m_errc; }
	errnum_t errnum() const noexcept { return errc2num(errc()); }

private:
	errc_t m_errc;
};

} // lely

#endif

