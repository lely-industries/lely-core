/**@file
 * This header file is part of the utilities library; it contains a generic type
 * that can represent both the result of a successful operation or the reason
 * for failure.
 *
 * #lely::util::Result is a simplified version of the <b>result</b> type from
 * the upcoming <a href="http://ned14.github.io/outcome/">Boost.Outcome</a>
 * library.
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

#ifndef LELY_UTIL_RESULT_HPP_
#define LELY_UTIL_RESULT_HPP_

#include <lely/util/error.hpp>

#include <type_traits>
#include <utility>

namespace lely {
namespace util {

namespace detail {

template <class T>
class Success {
 public:
  using value_type = T;

  Success() = default;

  template <class U, class = typename ::std::enable_if<!::std::is_same<
                         typename ::std::decay<U>::type, Success>::value>::type>
  Success(U&& u) : value_(::std::forward<U>(u)) {}

  value_type&
  value() & {
    return value_;
  }

  const value_type&
  value() const& {
    return value_;
  }

  value_type&&
  value() && {
    return ::std::move(value_);
  }

  const value_type&&
  value() const&& {
    return ::std::move(value_);
  }

 private:
  value_type value_;
};

template <>
class Success<void> {
 public:
  using value_type = void;
};

template <class E>
class Failure {
 public:
  using error_type = E;

  Failure() = delete;

  template <class U, class = typename ::std::enable_if<!::std::is_same<
                         typename ::std::decay<U>::type, Failure>::value>::type>
  Failure(U&& u) : error_(::std::forward<U>(u)) {}

  error_type&
  error() & {
    return error_;
  }

  const error_type&
  error() const& {
    return error_;
  }

  error_type&&
  error() && {
    return ::std::move(error_);
  }

  const error_type&&
  error() const&& {
    return ::std::move(error_);
  }

 private:
  error_type error_;
};

template <class>
struct error_traits;

template <>
struct error_traits<int> {
  using error_type = int;

  static constexpr bool
  is_error(error_type error) noexcept {
    return error != 0;
  }

  static void
  throw_error(error_type error, const char* what_arg) {
    throw_errc(what_arg, error);
  }
};

template <>
struct error_traits<::std::error_code> {
  using error_type = ::std::error_code;

  static bool
  is_error(error_type error) noexcept {
    return error.value() != 0;
  }

  static void
  throw_error(error_type error, const char* what_arg) {
    throw ::std::system_error(error, what_arg);
  }
};

template <>
struct error_traits<::std::exception_ptr> {
  using error_type = ::std::exception_ptr;

  static bool
  is_error(const error_type& error) noexcept {
    return error != nullptr;
  }

  static void
  throw_error(error_type error, const char* /*what_arg*/) {
    ::std::rethrow_exception(::std::move(error));
  }
};

}  // namespace detail

/**
 * Returns an object that can be used to implicitly construct a successful
 * #lely::util::Result with a default-constructed value.
 */
inline detail::Success<void>
success() noexcept {
  return {};
}

/**
 * Returns an object that can be used to implicitly construct a successful
 * #lely::util::Result with the specified value.
 */
template <class T>
inline detail::Success<typename ::std::decay<T>::type>
success(T&& t) noexcept {
  return {::std::forward<T>(t)};
}

/**
 * Returns an object that can be used to implicitly construct a failure
 * #lely::util::Result with the specified error.
 */
template <class E>
inline detail::Failure<typename ::std::decay<E>::type>
failure(E&& e) noexcept {
  return {::std::forward<E>(e)};
}

/**
 * A type capable of representing both the successful and failure result of an
 * operation.
 */
template <class T, class E = ::std::error_code>
class Result {
  using error_traits = detail::error_traits<E>;

 public:
  /// The value type on success.
  using value_type = T;

  /// The error type on failure.
  using error_type = typename error_traits::error_type;

  /// Constructs a successful result with an empty value.
  Result() = default;

  /// Constructs a successful result with an empty value. @see success()
  Result(const detail::Success<void>& /*s*/) {}

  /// Constructs a successful result with the specified value. @see success(T&&)
  template <class U>
  Result(const detail::Success<U>& s) : value_{s.value()} {}

  /// Constructs a successful result with the specified value. @see success(T&&)
  template <class U>
  Result(detail::Success<U>&& s) : value_{::std::move(s).value()} {}

  /// Constructs a failure result with the specified error. @see failure(E&&)
  template <class U>
  Result(const detail::Failure<U>& f) : error_{f.error()} {}

  /// Constructs a failure result with the specified error. @see failure(E&&)
  template <class U>
  Result(detail::Failure<U>&& f) : error_{::std::move(f).error()} {}

  /**
   * Constructs a successful result with value <b>u</b> if <b>U</b> is
   * constructible to #value_type and _not_ constructible to #error_type.
   */
  template <class U>
  Result(U&& u, typename ::std::enable_if<
                    ::std::is_constructible<value_type, U>::value &&
                        !::std::is_constructible<error_type, U>::value,
                    bool>::type /*SFINAE*/
                = false)
      : value_{::std::forward<U>(u)} {}

  /**
   * Constructs a failure result with error <b>u</b> if <b>U</b> is
   * constructible to #error_type and _not_ constructible to #value_type.
   */
  template <class U>
  Result(U&& u, typename ::std::enable_if<
                    !::std::is_constructible<value_type, U>::value &&
                        ::std::is_constructible<error_type, U>::value,
                    bool>::type /*SFINAE*/
                = false)
      : error_{::std::forward<U>(u)} {}

  /// Check whether `*this` contains a value (and not a non-zero error).
  explicit operator bool() const noexcept { return has_value(); }

  /**
   * Returns true if `*this` contains a value (and not a non-zero error).
   *
   * @see has_error()
   */
  bool
  has_value() const noexcept {
    return !has_error();
  }

  /**
   * Returns a reference to the value if `*this` contains a value, and throws an
   * exception if `*this` contains a non-zero error.
   */
  value_type&
  value() {
    if (has_error()) error_traits::throw_error(error_, "value");
    return value_;
  }

  /**
   * Returns a reference to the value if `*this` contains a value, and throws an
   * exception if `*this` contains a non-zero error.
   */
  const value_type&
  value() const {
    if (has_error()) error_traits::throw_error(error_, "value");
    return value_;
  }

  /// Returns true if `*this` contains a non-zero error. @see has_value()
  bool
  has_error() const noexcept {
    return error_traits::is_error(error_);
  }

  /// Returns a reference to the error, if any.
  error_type&
  error() noexcept {
    return error_;
  }

  /// Returns a reference to the error, if any.
  const error_type&
  error() const noexcept {
    return error_;
  }

 private:
  value_type value_{};
  error_type error_{};
};

template <class T>
class Result<T, typename ::std::enable_if<!::std::is_void<T>::value>::type> {
 public:
  using value_type = T;
  using error_type = void;

  Result() = default;

  Result(const detail::Success<void>& /*s*/) {}

  template <class U>
  Result(const detail::Success<U>& s) : value_{s.value()} {}

  template <class U>
  Result(detail::Success<U>&& s) : value_{::std::move(s).value()} {}

  template <class U, typename = typename ::std::enable_if<
                         ::std::is_constructible<value_type, U>::value>::type>
  // NOLINTNEXTLINE(readability/nolint)
  // NOLINTNEXTLINE(misc-forwarding-reference-overload)
  Result(U&& u) : value_{::std::forward<U>(u)} {}

  explicit operator bool() const noexcept { return has_value(); }

  bool
  has_value() const noexcept {
    return !has_error();
  }

  value_type&
  value() noexcept {
    return value_;
  }

  const value_type&
  value() const noexcept {
    return value_;
  }

  bool
  has_error() const noexcept {
    return false;
  }

  void
  error() const noexcept {}

 private:
  value_type value_{};
};

template <class E>
class Result<void, E> {
  using error_traits = detail::error_traits<E>;

 public:
  using value_type = void;
  using error_type = typename error_traits::error_type;

  Result() = default;

  Result(const detail::Success<void>&) {}

  Result(detail::Success<void>&&) {}

  template <class U>
  Result(const detail::Failure<U>& f) : error_{f.error()} {}

  template <class U>
  Result(detail::Failure<U>&& f) : error_{::std::move(f).error()} {}

  template <class U, typename = typename ::std::enable_if<
                         ::std::is_constructible<error_type, U>::value>::type>
  // NOLINTNEXTLINE(readability/nolint)
  // NOLINTNEXTLINE(misc-forwarding-reference-overload)
  Result(U&& u) : error_{::std::forward<U>(u)} {}

  explicit operator bool() const noexcept { return has_value(); }

  bool
  has_value() const noexcept {
    return !has_error();
  }

  void
  value() const {
    if (has_error()) error_traits::throw_error(error_, "value");
  }

  bool
  has_error() const noexcept {
    return error_traits::is_error(error_);
  }

  error_type&
  error() noexcept {
    return error_;
  }

  const error_type&
  error() const noexcept {
    return error_;
  }

 private:
  error_type error_{};
};

template <>
class Result<void, void> {
 public:
  using value_type = void;
  using error_type = void;

  Result() = default;

  template <class U>
  Result(U&&) noexcept {}

  explicit operator bool() const noexcept { return has_value(); }

  bool
  has_value() const noexcept {
    return !has_error();
  }

  void
  value() const noexcept {}

  bool
  has_error() const noexcept {
    return false;
  }

  void
  error() const noexcept {}
};

}  // namespace util
}  // namespace lely

#endif  // !LELY_UTIL_RESULT_HPP_
