/**@file
 * This header file is part of the utilities library; it contains the C++
 * interface for the stop token.
 *
 * @see lely/util/stop.h
 *
 * @copyright 2020-2021 Lely Industries N.V.
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

#ifndef LELY_UTIL_STOP_HPP_
#define LELY_UTIL_STOP_HPP_

#include <lely/util/stop.h>
#include <lely/util/error.hpp>

#include <utility>

namespace lely {
namespace util {

/**
 * An object providing the means to check if a stop request has been made for
 * its associated #lely::util::StopSource object. It is essentially a
 * thread-safe "view" of the associated stop-state.
 */
class StopToken {
 public:
  /**
   * Constructs an empty stop token with no assiociated stop-state.
   *
   * @post stop_possible() returns `false`.
   * @post stop_requested() returns `false`.
   */
  StopToken() noexcept = default;

  explicit StopToken(stop_token_t* token) noexcept : token_(token) {}

  /**
   * Constructs a stop token whose associated stop-state is the same as that
   * if <b>other</b>.
   *
   * @post `*this` and <b>other</b> compare equal.
   *
   * @see stop_token_acquire()
   */
  StopToken(const StopToken& other) noexcept
      : token_(stop_token_acquire(other.token_)) {}

  /**
   * Constructs a stop token whose associated stop-state is the same as that
   * if <b>other</b>; <b>other</b> is left as valid stop token with no
   * associated stop-state.
   *
   * @post `other.stop_possible()` returns `false`.
   */
  StopToken(StopToken&& other) noexcept : token_(other.token_) {
    other.token_ = nullptr;
  }

  /**
   * Copy-assigns the stop-state of <b>other</b> to `*this`.
   *
   * @post `*this` and <b>other</b> compare equal.
   *
   * @see stop_token_release(), stop_token_acquire().
   */
  StopToken&
  operator=(const StopToken& other) noexcept {
    if (token_ != other.token_) {
      stop_token_release(token_);
      token_ = stop_token_acquire(other.token_);
    }
    return *this;
  }

  /**
   * Move-assigns the stop-state of <b>other</b> to `*this`. After the
   * assignment, `*this` contains the previous stop-state of <b>other</b>, and
   * <b>other</b> has no stop-state.
   *
   * @post `other.stop_possible()` returns `false`.
   */
  StopToken&
  operator=(StopToken&& other) noexcept {
    using ::std::swap;
    swap(token_, other.token_);
    return *this;
  }

  /**
   * Destroys a stop token. If `*this` has a stop-state, its ownership is
   * released.
   *
   * @see stop_token_release()
   */
  ~StopToken() { stop_token_release(token_); }

  operator stop_token_t*() const noexcept { return token_; }

  /// Returns `true` if `*this` has a stop-state, and `false` otherwise.
  explicit operator bool() const noexcept { return token_ != nullptr; }

  /**
   * Returns `true` if `*this` has a stop-state and it has received a stop
   * request, and `false` otherwise.
   *
   * @see stop_token_stop_requested()
   */
  bool
  stop_requested() const noexcept {
    return *this && stop_token_stop_requested(*this) != 0;
  }

  /**
   * Returns `true` if `*this` has a stop-state and it has received a stop
   * request, or if it has an associated stop source that can still issue such a
   * request, and `false` otherwise.
   *
   * @see stop_token_stop_possible()
   */
  bool
  stop_possible() const noexcept {
    return *this && stop_token_stop_possible(*this) != 0;
  }

  /// Exchanges the stop-state of `*this` and <b>other</b>/
  void
  swap(StopToken& other) noexcept {
    ::std::swap(token_, other.token_);
  }

  /**
   * Returns `true` if <b>lhs</b> and <b>rhs</b> have the same stop-state, and
   * `false` otherwise.
   */
  friend bool
  operator==(const StopToken& lhs, const StopToken& rhs) noexcept {
    return lhs.token_ == rhs.token_;
  }

  /**
   * Returns `false` if <b>lhs</b> and <b>rhs</b> have the same stop-state, and
   * `true` otherwise.
   */
  friend bool
  operator!=(const StopToken& lhs, const StopToken& rhs) noexcept {
    return !(lhs == rhs);
  }

  /// Exchanges the stop-state of <b>lhs</b> with <b>rhs</b>.
  friend void
  swap(StopToken& lhs, StopToken& rhs) noexcept {
    lhs.swap(rhs);
  }

 private:
  stop_token_t* token_{nullptr};
};

/**
 * An object providing the means to issue a stop request. A stop request is
 * visible to all #lely::util::StopSource and #lely::util::StopToken objects of
 * the same associated stop-state.
 */
class StopSource {
 public:
  /**
   * Constructs a stop source with a new stop-state.
   *
   * @see stop_source_create()
   */
  StopSource() : source_(stop_source_create()) {
    if (!source_) util::throw_errc("StopSource");
  }

  /**
   * Constructs an empty stop source with no assiociated stop-state.
   *
   * @post stop_possible() returns `false`.
   */
  explicit StopSource(stop_source_t* source) noexcept : source_(source) {}

  /**
   * Constructs a stop source whose associated stop-state is the same as that
   * if <b>other</b>.
   *
   * @post `*this` and <b>other</b> compare equal.
   *
   * @see stop_source_acquire()
   */
  StopSource(const StopSource& other) noexcept
      : source_(stop_source_acquire(other.source_)) {}

  /**
   * Constructs a stop source whose associated stop-state is the same as that
   * if <b>other</b>; <b>other</b> is left as valid stop source with no
   * associated stop-state.
   *
   * @post `other.stop_possible()` returns `false`.
   */
  StopSource(StopSource&& other) noexcept : source_(other.source_) {
    other.source_ = nullptr;
  }

  /**
   * Copy-assigns the stop-state of <b>other</b> to `*this`.
   *
   * @post `*this` and <b>other</b> compare equal.
   *
   * @see stop_source_release(), stop_source_acquire().
   */
  StopSource&
  operator=(const StopSource& other) noexcept {
    if (source_ != other.source_) {
      stop_source_release(source_);
      source_ = stop_source_acquire(other.source_);
    }
    return *this;
  }

  /**
   * Move-assigns the stop-state of <b>other</b> to `*this`. After the
   * assignment, `*this` contains the previous stop-state of <b>other</b>, and
   * <b>other</b> has no stop-state.
   *
   * @post `other.stop_possible()` returns `false`.
   */
  StopSource&
  operator=(StopSource&& other) noexcept {
    StopSource(::std::move(other)).swap(*this);
    return *this;
  }

  /**
   * Destroys a stop source. If `*this` has a stop-state, its ownership is
   * released.
   *
   * @see stop_source_release()
   */
  ~StopSource() { stop_source_release(source_); }

  operator stop_source_t*() const noexcept { return source_; }

  /// Returns `true` if `*this` has a stop-state, and `false` otherwise.
  explicit operator bool() const noexcept { return source_ != nullptr; }

  /**
   * Issues a stop requests to the stop-state, if `*this` has a stop-state` and
   * it has not already received a stop request. Once a stop is requested, it
   * cannot be withdrawn. If a stop request is issued, any
   * #lely::util::StopCallback callbacks registered with a
   * #lely::util::StopToken with the same associated stop-state are invoked
   * synchronously on the calling thread.
   *
   * @returns `true` if a stop request was issued, and `false` if not. In the
   * latter case, another thread MAY still be in the middle of invoking a
   * #lely::util::StopCallback callback.
   *
   * @post stop_possible() returns `false` or stop_requested() returns `true`.
   *
   * @see stop_source_request_stop()
   */
  bool
  request_stop() noexcept {
    return *this && stop_source_request_stop(*this) != 0;
  }

  /**
   * Returns `true` if `*this` has a stop-state and it has received a stop
   * request, and `false` otherwise.
   *
   * @see stop_token_stop_requested()
   */
  bool
  stop_requested() const noexcept {
    return *this && stop_source_stop_requested(*this) != 0;
  }

  /// Returns `true` if `*this` has a stop-state, and `false` otherwise.
  bool
  stop_possible() const noexcept {
    return *this;
  }

  /// Exchanges the stop-state of `*this` and <b>other</b>/
  void
  swap(StopSource& other) noexcept {
    ::std::swap(source_, other.source_);
  }

  /**
   * Returns a stop token associated with the stop-state of `*this`, if it has
   * any, and a default-constructed (empty) stop token otherwise.
   */
  StopToken
  get_token() const noexcept {
    return StopToken(stop_source_get_token(*this));
  }

  /**
   * Returns `true` if <b>lhs</b> and <b>rhs</b> have the same stop-state, and
   * `false` otherwise.
   */
  friend bool
  operator==(const StopSource& lhs, const StopSource& rhs) noexcept {
    return lhs.source_ == rhs.source_;
  }

  /**
   * Returns `false` if <b>lhs</b> and <b>rhs</b> have the same stop-state, and
   * `true` otherwise.
   */
  friend bool
  operator!=(const StopSource& lhs, const StopSource& rhs) noexcept {
    return !(lhs == rhs);
  }

  /// Exchanges the stop-state of <b>lhs</b> with <b>rhs</b>.
  friend void
  swap(StopSource& lhs, StopSource& rhs) noexcept {
    lhs.swap(rhs);
  }

 private:
  stop_source_t* source_{nullptr};
};

/**
 * A RAII object type that registers a callback function with a
 * #lely::util::StopToken object. The callback function will be invoked when the
 * #lely::util::StopSource object associated with the stop token is requested to
 * stop.
 */
template <class Callback>
class StopCallback : public stop_func {
 public:
  using callback_type = Callback;

  /**
   * Copies the <b>token</b> stop token, saves the <b>cb</b> callback function
   * and registers with with <b>token</b>. If a stop request has already been
   * issued for the associated #lely::util::StopSource object, the callback
   * function is invoked in the calling thread before the constructor returns.
   *
   * @see stop_token_insert()
   */
  template <class C>
  explicit StopCallback(const StopToken& token, C&& cb)
      : stop_func STOP_FUNC_INIT(&func_), cb_(::std::forward<C>(cb)) {
    if (token && !stop_token_insert(token, this)) token_ = token;
  }

  /**
   * Moves the <b>token</b> stop token, saves the <b>cb</b> callback function
   * and registers with with <b>token</b>. If a stop request has already been
   * issued for the associated #lely::util::StopSource object, the callback
   * function is invoked in the calling thread before the constructor returns.
   *
   * @see stop_token_insert()
   */
  template <class C>
  explicit StopCallback(StopToken&& token, C&& cb)
      : stop_func STOP_FUNC_INIT(&func_), cb_(::std::forward<C>(cb)) {
    if (token && !stop_token_insert(token, this)) token_ = ::std::move(token);
  }

  StopCallback(const StopCallback&) = delete;
  StopCallback(StopCallback&&) = delete;

  StopCallback& operator=(const StopCallback&) = delete;
  StopCallback& operator=(StopCallback&&) = delete;

  /**
   * If `*this` has a #lely::util::StopToken with associated stop-state,
   * deregisters the saved callback function from it. If the callback function
   * is being invoked concurrently on another thread, the destructor does not
   * return until the callback function invocation is complete. If the callback
   * function is being invoked on the calling thread, the destructor does not
   * wait until the invocation is complete. Hence it is safe to invoke the
   * destructor from the callback function.
   *
   * @see stop_token_remove()
   */
  ~StopCallback() {
    if (token_) stop_token_remove(token_, this);
  }

 private:
  static void
  func_(stop_func* func) noexcept {
    Callback& cb_ = static_cast<StopCallback*>(func)->cb_;
    ::std::forward<Callback>(cb_)();
  }

  StopToken token_;
  Callback cb_;
};

}  // namespace util
}  // namespace lely

#endif  // !LELY_UTIL_STOP_HPP_
