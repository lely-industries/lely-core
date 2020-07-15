/**@file
 * This header file is part of the utilities library; it contains the C++
 * exception declarations.
 *
 * @copyright 2017-2019 Lely Industries N.V.
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

#ifndef LELY_UTIL_EXCEPTION_HPP_
#define LELY_UTIL_EXCEPTION_HPP_

#include <lely/util/errnum.h>

#include <stdexcept>
#include <system_error>

#ifndef throw_or_abort
/**
 * If exceptions are disabled, aborts the process instead of throwing an
 * exception.
 */
#if __cpp_exceptions
#define throw_or_abort(e) throw(e)
#else
#define throw_or_abort(e) __throw_or_abort((e).what())
#endif
#endif

extern "C" {

/// Aborts the process instead of throwing an exception.
_Noreturn void __throw_or_abort(const char* what) noexcept;
}

namespace lely {

/**
 * The type of objects thrown as exceptions to report a system error with an
 * associated error code.
 */
class error : public ::std::system_error {
 public:
  error(int errc = get_errc())
      : ::std::system_error(errc, ::std::system_category()), m_errc(errc) {}

  int
  errc() const noexcept {
    return m_errc;
  }

  errnum_t
  errnum() const noexcept {
    return errc2num(errc());
  }

 private:
  int m_errc;
};

}  // namespace lely

#endif  // !LELY_UTIL_EXCEPTION_HPP_
