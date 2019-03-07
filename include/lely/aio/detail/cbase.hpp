/**@file
 * This header file is part of the C++ asynchronous I/O library; it contains ...
 *
 * @copyright 2018-2019 Lely Industries N.V.
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

#ifndef LELY_AIO_DETAIL_CBASE_HPP_
#define LELY_AIO_DETAIL_CBASE_HPP_

#include <lely/aio/aio.hpp>

#include <cstddef>

namespace lely {

namespace aio {

namespace detail {

template <class T>
class CBase {
 public:
  // cppcheck-suppress noExplicitConstructor
  CBase(T* c_ptr_) noexcept  // NOLINT(runtime/explicit)
      : c_ptr(c_ptr_) {}

  operator T*() const noexcept { return c_ptr; }

  explicit operator bool() const noexcept { return !!c_ptr; }

  friend bool
  operator==(const CBase& lhs, const CBase& rhs) noexcept {
    return lhs.c_ptr == rhs.c_ptr;
  }

  friend bool
  operator==(const CBase& lhs, ::std::nullptr_t) noexcept {
    return !lhs;
  }

  friend bool
  operator==(::std::nullptr_t, const CBase& rhs) noexcept {
    return !rhs;
  }

  friend bool
  operator!=(const CBase& lhs, const CBase& rhs) noexcept {
    return !(lhs == rhs);
  }

  friend bool
  operator!=(const CBase& lhs, ::std::nullptr_t) noexcept {
    return lhs;
  }

  friend bool
  operator!=(::std::nullptr_t, const CBase& rhs) noexcept {
    return rhs;
  }

 protected:
  T* c_ptr{nullptr};
};

}  // namespace detail

}  // namespace aio

}  // namespace lely

#endif  // LELY_AIO_DETAIL_CBASE_HPP_
