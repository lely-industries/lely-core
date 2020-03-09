/**@file
 * This header file is part of the event library; it contains the C++ interface
 * for the polling event loop.
 *
 * @see lely/ev/loop.h
 *
 * @copyright 2018-2020 Lely Industries N.V.
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

#ifndef LELY_EV_LOOP_HPP_
#define LELY_EV_LOOP_HPP_

#include <lely/ev/exec.hpp>
#include <lely/ev/future.hpp>
#include <lely/ev/loop.h>
#include <lely/ev/poll.hpp>
#include <lely/util/chrono.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace lely {
namespace ev {

/// A polling event loop.
class Loop {
 public:
  /// @see ev_loop_create()
#if _WIN32
  Loop(ev_poll_t* poll = nullptr, ::std::size_t npoll = 0,
#else
  Loop(ev_poll_t* poll = nullptr, ::std::size_t npoll = 1,
#endif
       bool poll_task = false)
      : loop_(ev_loop_create(poll, npoll, poll_task)) {
    if (!loop_) util::throw_errc("Loop");
  }

  Loop(const Loop&) = delete;

  Loop(Loop&& other) noexcept : loop_(other.loop_) { other.loop_ = nullptr; }

  Loop& operator=(const Loop&) = delete;

  Loop&
  operator=(Loop&& other) noexcept {
    using ::std::swap;
    swap(loop_, other.loop_);
    return *this;
  }

  /// @see ev_loop_destroy()
  ~Loop() { ev_loop_destroy(*this); }

  operator ev_loop_t*() const noexcept { return loop_; }

  /// @see ev_loop_get_poll()
  Poll
  get_poll() const noexcept {
    return Poll(ev_loop_get_poll(*this));
  }

  /// @see ev_loop_get_exec()
  Executor
  get_executor() const noexcept {
    return Executor(ev_loop_get_exec(*this));
  }

  /// @see ev_loop_stop()
  void
  stop() noexcept {
    ev_loop_stop(*this);
  }

  ///@ see ev_loop_stopped()
  bool
  stopped() const noexcept {
    return ev_loop_stopped(*this) != 0;
  }

  /// @see ev_loop_stop()
  void
  restart() noexcept {
    ev_loop_restart(*this);
  }

  /// @see ev_loop_wait()
  ::std::size_t
  wait(ev_future_t* future) {
    ::std::error_code ec;
    auto result = wait(future, ec);
    if (ec) throw ::std::system_error(ec, "wait");
    return result;
  }

  /// @see ev_loop_wait()
  ::std::size_t
  wait(ev_future_t* future, ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    auto result = ev_loop_wait(*this, future);
    ec = util::make_error_code();
    set_errc(errsv);
    return result;
  }

  /// @see ev_loop_wait_until()
  template <class Rep, class Period>
  ::std::size_t
  wait_for(ev_future_t* future, const ::std::chrono::duration<Rep, Period>& d) {
    return wait_until(future, ::std::chrono::system_clock::now() + d);
  }

  /// @see ev_loop_wait_until()
  template <class Rep, class Period>
  ::std::size_t
  wait_for(ev_future_t* future, const ::std::chrono::duration<Rep, Period>& d,
           ::std::error_code& ec) noexcept {
    return wait_until(future, ::std::chrono::system_clock::now() + d, ec);
  }

  /// @see ev_loop_wait_until()
  template <class Clock, class Duration>
  ::std::size_t
  wait_until(ev_future_t* future,
             const ::std::chrono::time_point<Clock, Duration>& t) {
    auto abs_time =
        util::to_timespec(compat::clock_cast<::std::chrono::system_clock>(t));
    return wait_until(future, &abs_time);
  }

  /// @see ev_loop_wait_until()
  template <class Clock, class Duration>
  ::std::size_t
  wait_until(ev_future_t* future,
             const ::std::chrono::time_point<Clock, Duration>& t,
             ::std::error_code& ec) noexcept {
    auto abs_time =
        util::to_timespec(compat::clock_cast<::std::chrono::system_clock>(t));
    return wait_until(future, &abs_time, ec);
  }

  /// @see ev_loop_wait_one()
  ::std::size_t
  wait_one(ev_future_t* future) {
    ::std::error_code ec;
    auto result = wait_one(future, ec);
    if (ec) throw ::std::system_error(ec, "wait_one");
    return result;
  }

  /// @see ev_loop_wait_one()
  ::std::size_t
  wait_one(ev_future_t* future, ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    auto result = ev_loop_wait_one(*this, future);
    ec = util::make_error_code();
    set_errc(errsv);
    return result;
  }

  /// @see ev_loop_wait_one_until()
  template <class Rep, class Period>
  ::std::size_t
  wait_one_for(ev_future_t* future,
               const ::std::chrono::duration<Rep, Period>& d) {
    return wait_one_until(future, ::std::chrono::system_clock::now() + d);
  }

  /// @see ev_loop_wait_one_until()
  template <class Rep, class Period>
  ::std::size_t
  wait_one_for(ev_future_t* future,
               const ::std::chrono::duration<Rep, Period>& d,
               ::std::error_code& ec) noexcept {
    return wait_one_until(future, ::std::chrono::system_clock::now() + d, ec);
  }

  /// @see ev_loop_wait_one_until()
  template <class Clock, class Duration>
  ::std::size_t
  wait_one_until(ev_future_t* future,
                 const ::std::chrono::time_point<Clock, Duration>& t) {
    auto abs_time =
        util::to_timespec(compat::clock_cast<::std::chrono::system_clock>(t));
    return wait_one_until(future, &abs_time);
  }

  /// @see ev_loop_wait_one_until()
  template <class Clock, class Duration>
  ::std::size_t
  wait_one_until(ev_future_t* future,
                 const ::std::chrono::time_point<Clock, Duration>& t,
                 ::std::error_code& ec) noexcept {
    auto abs_time =
        util::to_timespec(compat::clock_cast<::std::chrono::system_clock>(t));
    return wait_one_until(future, &abs_time, ec);
  }

  /// @see ev_loop_run()
  ::std::size_t
  run() {
    ::std::error_code ec;
    auto result = run(ec);
    if (ec) throw ::std::system_error(ec, "run");
    return result;
  }

  /// @see ev_loop_run()
  ::std::size_t
  run(::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    auto result = ev_loop_run(*this);
    ec = util::make_error_code();
    set_errc(errsv);
    return result;
  }

  /// @see ev_loop_run_until()
  template <class Rep, class Period>
  ::std::size_t
  run_for(const ::std::chrono::duration<Rep, Period>& d) {
    return run_until(::std::chrono::system_clock::now() + d);
  }

  /// @see ev_loop_run_until()
  template <class Rep, class Period>
  ::std::size_t
  run_for(const ::std::chrono::duration<Rep, Period>& d,
          ::std::error_code& ec) noexcept {
    return run_until(::std::chrono::system_clock::now() + d, ec);
  }

  /// @see ev_loop_run_until()
  template <class Clock, class Duration>
  ::std::size_t
  run_until(const ::std::chrono::time_point<Clock, Duration>& t) {
    auto abs_time =
        util::to_timespec(compat::clock_cast<::std::chrono::system_clock>(t));
    return run_until(&abs_time);
  }

  /// @see ev_loop_run_until()
  template <class Clock, class Duration>
  ::std::size_t
  run_until(const ::std::chrono::time_point<Clock, Duration>& t,
            ::std::error_code& ec) noexcept {
    auto abs_time =
        util::to_timespec(compat::clock_cast<::std::chrono::system_clock>(t));
    return run_until(&abs_time, ec);
  }

  /// @see ev_loop_run_one()
  ::std::size_t
  run_one() {
    ::std::error_code ec;
    auto result = run_one(ec);
    if (ec) throw ::std::system_error(ec, "run_one");
    return result;
  }

  /// @see ev_loop_run_one()
  ::std::size_t
  run_one(::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    auto result = ev_loop_run_one(*this);
    ec = util::make_error_code();
    set_errc(errsv);
    return result;
  }

  /// @see ev_loop_run_one_until()
  template <class Rep, class Period>
  ::std::size_t
  run_one_for(const ::std::chrono::duration<Rep, Period>& d) {
    return run_one_until(::std::chrono::system_clock::now() + d);
  }

  /// @see ev_loop_run_one_until()
  template <class Rep, class Period>
  ::std::size_t
  run_one_for(const ::std::chrono::duration<Rep, Period>& d,
              ::std::error_code& ec) noexcept {
    return run_one_until(::std::chrono::system_clock::now() + d, ec);
  }

  /// @see ev_loop_run_one_until()
  template <class Clock, class Duration>
  ::std::size_t
  run_one_until(const ::std::chrono::time_point<Clock, Duration>& t) {
    auto abs_time =
        util::to_timespec(compat::clock_cast<::std::chrono::system_clock>(t));
    return run_one_until(&abs_time);
  }

  /// @see ev_loop_run_one_until()
  template <class Clock, class Duration>
  ::std::size_t
  run_one_until(const ::std::chrono::time_point<Clock, Duration>& t,
                ::std::error_code& ec) noexcept {
    auto abs_time =
        util::to_timespec(compat::clock_cast<::std::chrono::system_clock>(t));
    return run_one_until(&abs_time, ec);
  }

  /// @see ev_loop_run_until()
  ::std::size_t
  poll() {
    ::std::error_code ec;
    auto result = poll(ec);
    if (ec) throw ::std::system_error(ec, "poll");
    return result;
  }

  /// @see ev_loop_run_until()
  ::std::size_t
  poll(::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    auto result = ev_loop_poll(*this);
    ec = util::make_error_code();
    set_errc(errsv);
    return result;
  }

  /// @see ev_loop_run_one_until()
  ::std::size_t
  poll_one() {
    ::std::error_code ec;
    auto result = poll_one(ec);
    if (ec) throw ::std::system_error(ec, "poll_one");
    return result;
  }

  /// @see ev_loop_run_one_until()
  ::std::size_t
  poll_one(::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    auto result = ev_loop_poll_one(*this);
    ec = util::make_error_code();
    set_errc(errsv);
    return result;
  }

  /// @see ev_loop_self()
  static void*
  self() noexcept {
    return ev_loop_self();
  }

  /// @see ev_loop_kill()
  void
  kill(void* thr) {
    if (ev_loop_kill(*this, thr) == -1) util::throw_errc("kill");
  }

 protected:
  ::std::size_t
  wait_until(ev_future_t* future, const timespec* abs_time) {
    ::std::error_code ec;
    auto result = wait_until(future, abs_time, ec);
    if (ec && errc2num(ec.value()) != ERRNUM_TIMEDOUT)
      throw ::std::system_error(ec, "wait_until");
    return result;
  }

  ::std::size_t
  wait_until(ev_future_t* future, const timespec* abs_time,
             ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    auto result = ev_loop_wait_until(*this, future, abs_time);
    ec = util::make_error_code();
    set_errc(errsv);
    return result;
  }

  ::std::size_t
  wait_one_until(ev_future_t* future, const timespec* abs_time) {
    ::std::error_code ec;
    auto result = wait_one_until(future, abs_time, ec);
    if (ec && errc2num(ec.value()) != ERRNUM_TIMEDOUT)
      throw ::std::system_error(ec, "wait_one_until");
    return result;
  }

  ::std::size_t
  wait_one_until(ev_future_t* future, const timespec* abs_time,
                 ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    auto result = ev_loop_wait_one_until(*this, future, abs_time);
    ec = util::make_error_code();
    set_errc(errsv);
    return result;
  }

  ::std::size_t
  run_until(const timespec* abs_time) {
    ::std::error_code ec;
    auto result = run_until(abs_time, ec);
    if (ec && errc2num(ec.value()) != ERRNUM_TIMEDOUT)
      throw ::std::system_error(ec, "run_until");
    return result;
  }

  ::std::size_t
  run_until(const timespec* abs_time, ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    auto result = ev_loop_run_until(*this, abs_time);
    ec = util::make_error_code();
    set_errc(errsv);
    return result;
  }

  ::std::size_t
  run_one_until(const timespec* abs_time) {
    ::std::error_code ec;
    auto result = run_one_until(abs_time, ec);
    if (ec && errc2num(ec.value()) != ERRNUM_TIMEDOUT)
      throw ::std::system_error(ec, "run_one_until");
    return result;
  }

  ::std::size_t
  run_one_until(const timespec* abs_time, ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    auto result = ev_loop_run_one_until(*this, abs_time);
    ec = util::make_error_code();
    set_errc(errsv);
    return result;
  }

 private:
  ev_loop_t* loop_{nullptr};
};

}  // namespace ev
}  // namespace lely

#endif  // !LELY_EV_LOOP_HPP_
