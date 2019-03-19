/**@file
 * This header file is part of the event library; it contains the C++ interface
 * for the futures and promises.
 *
 * @see lely/ev/future.h
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

#ifndef LELY_EV_FUTURE_HPP_
#define LELY_EV_FUTURE_HPP_

#include <lely/ev/exec.hpp>
#include <lely/ev/future.h>
#include <lely/util/result.hpp>

#include <utility>
#include <vector>

namespace lely {
namespace ev {

template <class, class = ::std::error_code>
class Future;

/**
 * A promise. <b>T</b> and <b>E</b> should be nothrow default-constructible. Any
 * exceptions thrown during the default construction of the shared state will
 * result in a call to std::terminate().
 *
 * @see ev_promise_t
 */
template <class T, class E = ::std::error_code>
class Promise {
 public:
  /// The type of the result stored in the shared state.
  using result_type = util::Result<T, E>;

  /**
   * Constructs a promise with (a reference to) an empty shared state.
   *
   * @see ev_promise_create()
   */
  Promise()
      : promise_(ev_promise_create(sizeof(result_type), [](void* ptr) noexcept {
          static_cast<result_type*>(ptr)->~result_type();
        })) {
    if (!promise_) util::throw_errc("Promise");
    // Since we already registered the destructor for result_type(), we cannot
    // handle an exception from the default constructor. Wrapping the placement
    // new in a noexcept lambda prevents undefined behavior.
    [](void* ptr) noexcept { new (ptr) result_type(); }
    (ev_promise_data(promise_));
  }

  explicit Promise(ev_promise_t* promise) noexcept : promise_(promise) {}

  /// Constructs a promise with a reference to the shared state of <b>other</b>.
  Promise(const Promise& other) noexcept
      : promise_(ev_promise_acquire(other.promise_)) {}

  /**
   * Moves the shared state of <b>other</b> to `*this`.
   *
   * @post <b>other</b> has no shared state (#operator bool() returns false).
   */
  Promise(Promise&& other) noexcept : promise_(other.promise_) {
    other.promise_ = nullptr;
  }

  /**
   * Abandons the shared state of `*this` as if by #~Promise(), then creates a
   * reference to the shared state of <b>other</b>.
   *
   * @returns `*this`.
   */
  Promise&
  operator=(const Promise& other) noexcept {
    if (promise_ != other.promise_) {
      ev_promise_release(promise_);
      promise_ = ev_promise_acquire(other.promise_);
    }
    return *this;
  }

  /**
   * Abandons the shared state of `*this` as if by #~Promise(), then moves the
   * shared state of <b>other</b> to `*this`.
   *
   * @post <b>other</b> has no shared state (#operator bool() returns false).
   *
   * @returns `*this`;
   */
  Promise&
  operator=(Promise&& other) noexcept {
    using ::std::swap;
    swap(promise_, other.promise_);
    return *this;
  }

  /**
   * Abandons the shared state. If no references remain to the shared state, it
   * is destroyed.
   *
   * @see ev_promise_release()
   */
  ~Promise() { ev_promise_release(promise_); }

  operator ev_promise_t*() const noexcept { return promise_; }

  /// Checks whether `*this` contains (a reference to) a shared state.
  explicit operator bool() const noexcept { return promise_ != nullptr; }

  /**
   * Checks whether `*this` contains a unique reference to its shared state.
   *
   * @pre #operator bool() returns true.
   *
   * @see ev_promise_is_unique()
   */
  bool
  is_unique() const noexcept {
    return ev_promise_is_unique(*this) != 0;
  }

  /**
   * Satisfies a promise, if it was not aready satisfied, and stores the
   * specified value as the result in the shared state.
   *
   * @pre #operator bool() returns true.
   *
   * @returns true if the promise is successfully satisfied, and false if it was
   * already satisfied.
   *
   * @see ev_promise_set()
   */
  template <class U>
  bool
  set(U&& u) {
    if (ev_promise_set_acquire(*this)) {
      auto p = reinterpret_cast<result_type*>(ev_promise_data(*this));
      try {
        *p = result_type{::std::forward<U>(u)};
        ev_promise_set_release(*this, static_cast<void*>(p));
      } catch (...) {
        ev_promise_set_release(*this, nullptr);
        throw;
      }
      return true;
    }
    return false;
  }

  /**
   * Returns a #lely::ev::Future with (a reference to) the same shared state as
   * `*this`.
   *
   * @pre #operator bool() returns true.
   */
  Future<T, E>
  get_future() const noexcept {
    return Future<T, E>(ev_promise_get_future(*this));
  }

 private:
  ev_promise_t* promise_{nullptr};
};

/// A future. @see ev_future_t
template <class T, class E>
class Future {
 public:
  /// The type of the result stored in the shared state.
  using result_type = util::Result<T, E>;

  /**
   * Constructs future without (a reference to) a shared state.
   *
   * @post #operator bool() returns false.
   */
  Future() noexcept = default;

  explicit Future(ev_future_t* future) noexcept : future_(future) {}

  /// Constructs a future with a reference to the shared state of <b>other</b>.
  Future(const Future& other) noexcept
      : future_(ev_future_acquire(other.future_)) {}

  /**
   * Moves the shared state of <b>other</b> to `*this`.
   *
   * @post <b>other</b> has no shared state (#operator bool() returns false).
   */
  Future(Future&& other) noexcept : future_(other.future_) {
    other.future_ = nullptr;
  }

  /**
   * Abandons the shared state of `*this` as if by #~Future(), then creates a
   * reference to the shared state of <b>other</b>.
   *
   * @returns `*this`.
   */
  Future&
  operator=(const Future& other) noexcept {
    if (future_ != other.future_) {
      ev_future_release(future_);
      future_ = ev_future_acquire(other.future_);
    }
    return *this;
  }

  /**
   * Abandons the shared state of `*this` as if by #~Future(), then moves the
   * shared state of <b>other</b> to `*this`.
   *
   * @post <b>other</b> has no shared state (#operator bool() returns false).
   *
   * @returns `*this`;
   */
  Future&
  operator=(Future&& other) noexcept {
    using ::std::swap;
    swap(future_, other.future_);
    return *this;
  }

  /**
   * Abandons the shared state. If no references remain to the shared state, it
   * is destroyed.
   *
   * @see ev_future_release()
   */
  ~Future() { ev_future_release(future_); }

  operator ev_future_t*() const noexcept { return future_; }

  /// Checks whether `*this` contains (a reference to) a shared state.
  explicit operator bool() const noexcept { return future_ != nullptr; }

  /**
   * Checks whether `*this` contains a unique reference to its shared state.
   *
   * @pre #operator bool() returns true.
   *
   * @see ev_future_is_unique()
   */
  bool
  is_unique() const noexcept {
    return ev_future_is_unique(*this) != 0;
  }

  /**
   * Checks whether the future is ready, i.e., its associated promise has been
   * satisfied and a result has been stored in the shared state.
   *
   * @pre #operator bool() returns true.
   *
   * @see ev_future_is_ready()
   */
  bool
  is_ready() const noexcept {
    return ev_future_is_ready(*this);
  }

  /**
   * Returns the result of a ready future.
   *
   * @pre #is_ready() returns true.
   *
   * @returns a reference to the result stored in the shared state.
   *
   * @see ev_future_get()
   */
  result_type&
  get() noexcept {
    return *reinterpret_cast<result_type*>(ev_future_get(*this));
  }

  /**
   * Returns the result of a ready future.
   *
   * @pre #is_ready() returns true.
   *
   * @returns a reference to the result stored in the shared state.
   *
   * @see ev_future_get()
   */
  const result_type&
  get() const noexcept {
    return *reinterpret_cast<result_type*>(ev_future_get(*this));
  }

  /// @see ev_future_submit()
  void
  submit(ev_task& task) noexcept {
    ev_future_submit(*this, &task);
  }

  /// @see ev_future_submit()
  template <class F>
  void
  submit(ev_exec_t* exec, F&& f) {
    submit(*make_task_wrapper(exec, ::std::forward<F>(f)));
  }

  /// @see ev_future_cancel()
  bool
  cancel(ev_task& task) noexcept {
    return !!ev_future_cancel(*this, &task);
  }

  /// @see ev_future_cancel()
  ::std::size_t
  cancel_all() noexcept {
    return ev_future_cancel(*this, nullptr);
  }

  /// @see ev_future_abort()
  bool
  abort(ev_task& task) noexcept {
    return !!ev_future_abort(*this, &task);
  }

  /// @see ev_future_abort()
  ::std::size_t
  abort_all() noexcept {
    return ev_future_abort(*this, nullptr);
  }

 private:
  ev_future_t* future_{nullptr};
};

/// @see ev_future_when_all_n()
template <class InputIt>
inline Future<::std::size_t, void>
when_all(ev_exec_t* exec, InputIt first, InputIt last) {
  ::std::vector<ev_future_t*> futures(first, last);
  auto f = ev_future_when_all_n(exec, futures.size(), futures.data());
  if (!f) util::throw_errc("when_all");
  return Future<::std::size_t, void>(f);
}

/// @see ev_future_when_all_n()
template <class... Futures>
inline Future<::std::size_t, void>
when_all(ev_exec_t* exec, Futures&&... futures) {
  auto f =
      ev_future_when_all(exec, static_cast<ev_future_t*>(futures)..., nullptr);
  if (!f) util::throw_errc("when_all");
  return Future<::std::size_t, void>(f);
}

/// @see ev_future_when_any_n()
template <class InputIt>
inline Future<::std::size_t, void>
when_any(ev_exec_t* exec, InputIt first, InputIt last) {
  ::std::vector<ev_future_t*> futures(first, last);
  auto f = ev_future_when_any_n(exec, futures.size(), futures.data());
  if (!f) util::throw_errc("when_any");
  return Future<::std::size_t, void>(f);
}

/// @see ev_future_when_any_n()
template <class... Futures>
inline Future<::std::size_t, void>
when_any(ev_exec_t* exec, Futures&&... futures) {
  auto f =
      ev_future_when_any(exec, static_cast<ev_future_t*>(futures)..., nullptr);
  if (!f) util::throw_errc("when_any");
  return Future<::std::size_t, void>(f);
}

}  // namespace ev
}  // namespace lely

#endif  // !LELY_EV_FUTURE_HPP_
