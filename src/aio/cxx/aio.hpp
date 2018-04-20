/*!\file
 * This is the internal header file of the C++ asynchronous I/O library.
 *
 * \copyright 2018 Lely Industries N.V.
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

#ifndef LELY_AIO_INTERN_AIO_HPP_
#define LELY_AIO_INTERN_AIO_HPP_

#include "../aio.h"

#if !LELY_NO_CXX

#include <lely/util/errnum.h>
#include <lely/aio/aio.hpp>

#include <system_error>
#include <type_traits>
#include <utility>

namespace lely {

namespace aio {

namespace {

_Noreturn inline void
throw_errc(const char *what, int errc = get_errc()) {
  throw ::std::system_error(errc, ::std::system_category(), what);
}

template <class F, class... Args>
inline auto
InvokeC(::std::error_code& ec, F&& f, Args&&... args)
    -> typename ::std::enable_if<!::std::is_same<
        decltype(f(::std::forward<Args>(args)...)), void
    >::value, decltype(f(::std::forward<Args>(args)...))>::type {
  ec.clear();
  int errsv = get_errc();
  set_errc(0);
  auto r = f(::std::forward<Args>(args)...);
  int errc = get_errc();
  if (errc)
    ec = ::std::error_code(errc, ::std::system_category());
  set_errc(errsv);
  return r;
}

template <class F, class... Args>
inline auto
InvokeC(const char* what, F&& f, Args&&... args)
    -> typename ::std::enable_if<!::std::is_same<
        decltype(f(::std::forward<Args>(args)...)), void
    >::value, decltype(f(::std::forward<Args>(args)...))>::type {
  ::std::error_code ec;
  auto r = InvokeC(ec, ::std::forward<F>(f), ::std::forward<Args>(args)...);
  if (ec)
    throw ::std::system_error(ec, what);
  return r;
}

template <class F, class... Args>
inline auto
InvokeC(::std::error_code& ec, F&& f, Args&&... args)
    -> typename ::std::enable_if<::std::is_same<
        decltype(f(::std::forward<Args>(args)...)), void
    >::value, decltype(f(::std::forward<Args>(args)...))>::type {
  int errsv = get_errc();
  set_errc(0);
  f(::std::forward<Args>(args)...);
  int errc = get_errc();
  if (errc)
    ec = ::std::error_code(errc, ::std::system_category());
  set_errc(errsv);
}

template <class F, class... Args>
inline auto
InvokeC(const char* what, F&& f, Args&&... args)
    -> typename ::std::enable_if<::std::is_same<
        decltype(f(::std::forward<Args>(args)...)), void
    >::value, decltype(f(::std::forward<Args>(args)...))>::type {
  ::std::error_code ec;
  InvokeC(ec, ::std::forward<F>(f), ::std::forward<Args>(args)...);
  if (ec)
    throw ::std::system_error(ec, what);
}

}  // namespace

}  // namespace aio

}  // namespace lely

#endif  // !LELY_NO_CXX

#endif  // LELY_AIO_INTERN_AIO_HPP_
