/**@file
 * This header file is part of the I/O library; it contains the C++ interface
 * for the abstract timer.
 *
 * @see lely/io2/timer.h
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

#ifndef LELY_IO2_TIMER_HPP_
#define LELY_IO2_TIMER_HPP_

#include <lely/ev/future.hpp>
#include <lely/io2/dev.hpp>
#include <lely/io2/clock.hpp>
#include <lely/io2/timer.h>

#include <utility>

namespace lely {
namespace io {

namespace detail {

template <class F>
class TimerWaitWrapper : public io_timer_wait {
 public:
  TimerWaitWrapper(ev_exec_t* exec, F&& f)
      : io_timer_wait IO_TIMER_WAIT_INIT(
            exec,
            [](ev_task* task) noexcept {
              auto wait = io_timer_wait_from_task(task);
              auto overrun = wait->r.result;
              ::std::error_code ec;
              if (overrun == -1) ec = util::make_error_code(wait->r.errc);
              auto self = static_cast<TimerWaitWrapper*>(wait);
              compat::invoke(::std::move(self->func_), overrun, ec);
              delete self;
            }),
        func_(::std::forward<F>(f)) {}

  TimerWaitWrapper(const TimerWaitWrapper&) = delete;

  TimerWaitWrapper& operator=(const TimerWaitWrapper&) = delete;

 private:
  typename ::std::decay<F>::type func_;
};

}  // namespace detail

/**
 * Creates a wait operation with a completion task. The operation deletes
 * itself after it is completed, so it MUST NOT be deleted once it is submitted
 * to a timer.
 */
template <class F>
inline typename ::std::enable_if<
    compat::is_invocable<F, int, ::std::error_code>::value,
    detail::TimerWaitWrapper<F>*>::type
make_timer_wait_wrapper(ev_exec_t* exec, F&& f) {
  return new detail::TimerWaitWrapper<F>(exec, ::std::forward<F>(f));
}

/**
 * A wait operation suitable for use with an I/O timer. This class stores a
 * callable object with signature `void(int overrun, std::error_code ec)`,
 * which is invoked upon completion (or cancellation) of the wait operation.
 */
class TimerWait : public io_timer_wait {
 public:
  using Signature = void(int, ::std::error_code);

  /// Constructs a wait operation with a completion task.
  template <class F>
  TimerWait(ev_exec_t* exec, F&& f)
      : io_timer_wait
        IO_TIMER_WAIT_INIT(exec,
                           [](ev_task* task) noexcept {
                             auto wait = io_timer_wait_from_task(task);
                             auto self = static_cast<TimerWait*>(wait);
                             if (self->func_) {
                               auto overrun = wait->r.result;
                               ::std::error_code ec;
                               if (overrun == -1)
                                 ec = util::make_error_code(wait->r.errc);
                               self->func_(overrun, ec);
                             }
                           }),
        func_(::std::forward<F>(f)) {}

  /// Constructs a wait operation with a completion task.
  template <class F>
  explicit TimerWait(F&& f) : TimerWait(nullptr, ::std::forward<F>(f)) {}

  TimerWait(const TimerWait&) = delete;

  TimerWait& operator=(const TimerWait&) = delete;

  operator ev_task&() & noexcept { return task; }

  /// Returns the executor to which the completion task is (to be) submitted.
  ev::Executor
  get_executor() const noexcept {
    return ev::Executor(task.exec);
  }

 private:
  ::std::function<Signature> func_;
};

/**
 * A reference to an abstract timer. This class is a wrapper around
 * `#io_timer_t*`.
 */
class TimerBase : public Device {
 public:
  using Device::operator io_dev_t*;

  using duration = Clock::duration;
  using time_point = Clock::time_point;

  explicit TimerBase(io_timer_t* timer_) noexcept
      : Device(timer_ ? io_timer_get_dev(timer_) : nullptr), timer(timer_) {}

  operator io_timer_t*() const noexcept { return timer; }

  /// @see io_timer_get_clock()
  Clock
  get_clock() const noexcept {
    return Clock(io_timer_get_clock(*this));
  }

  /// @see io_timer_getoverrun()
  int
  getoverrun(::std::error_code& ec) const noexcept {
    int errsv = get_errc();
    set_errc(0);
    int overrun = io_timer_getoverrun(*this);
    if (overrun >= 0)
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
    return overrun;
  }

  /// @see io_timer_getoverrun()
  int
  getoverrun() const {
    ::std::error_code ec;
    int overrun = getoverrun(ec);
    if (overrun < 0) throw ::std::system_error(ec, "getoverrun");
    return overrun;
  }

  /**
   * @see io_timer_gettime()
   *
   * @returns a pair of time intervals. The first interval is the amount of time
   * until the next expiration of the time. The second interval is the reload
   * value of the timer.
   */
  ::std::pair<duration, duration>
  gettime(::std::error_code& ec) const noexcept {
    int errsv = get_errc();
    set_errc(0);
    itimerspec value = {{0, 0}, {0, 0}};
    if (!io_timer_gettime(*this, &value))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
    return {util::from_timespec(value.it_value),
            util::from_timespec(value.it_interval)};
  }

  /**
   * @see io_timer_gettime()
   *
   * @returns a pair of time intervals. The first interval is the amount of time
   * until the next expiration of the time. The second interval is the reload
   * value of the timer.
   */
  ::std::pair<duration, duration>
  gettime() const {
    ::std::error_code ec;
    auto value = gettime(ec);
    if (ec) throw ::std::system_error(ec, "gettime");
    return value;
  }

  /// @see io_timer_settime(), gettime()
  ::std::pair<duration, duration>
  settime(const duration& expiry, const duration& period,
          ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    itimerspec value = {util::to_timespec(period), util::to_timespec(expiry)};
    itimerspec ovalue = {{0, 0}, {0, 0}};
    if (!io_timer_settime(*this, 0, &value, &ovalue))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
    return {util::from_timespec(ovalue.it_value),
            util::from_timespec(ovalue.it_interval)};
  }

  /// @see io_timer_settime(), gettime()
  ::std::pair<duration, duration>
  settime(const duration& expiry, const duration& period = {}) {
    ::std::error_code ec;
    auto ovalue = settime(expiry, period, ec);
    if (ec) throw ::std::system_error(ec, "settime");
    return ovalue;
  }

  /// @see io_timer_settime(), gettime()
  ::std::pair<duration, duration>
  settime(const time_point& expiry, const duration& period,
          ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    itimerspec value = {util::to_timespec(period), util::to_timespec(expiry)};
    itimerspec ovalue = {{0, 0}, {0, 0}};
    if (!io_timer_settime(*this, TIMER_ABSTIME, &value, &ovalue))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
    return {util::from_timespec(ovalue.it_value),
            util::from_timespec(ovalue.it_interval)};
  }

  /// @see io_timer_settime(), gettime()
  ::std::pair<duration, duration>
  settime(const time_point& expiry, const duration& period = {}) {
    ::std::error_code ec;
    auto ovalue = settime(expiry, period, ec);
    if (ec) throw ::std::system_error(ec, "settime");
    return ovalue;
  }

  /// @see io_timer_submit_wait()
  void
  submit_wait(io_timer_wait& wait) noexcept {
    io_timer_submit_wait(*this, &wait);
  }

  /// @see io_timer_submit_wait()
  template <class F>
  void
  submit_wait(ev_exec_t* exec, F&& f) {
    submit_wait(*make_timer_wait_wrapper(exec, ::std::forward<F>(f)));
  }

  /// @see io_timer_submit_wait()
  template <class F>
  typename ::std::enable_if<!::std::is_base_of<
      io_timer_wait, typename ::std::decay<F>::type>::value>::type
  submit_wait(F&& f) {
    submit_wait(nullptr, ::std::forward<F>(f));
  }

  /// @see io_timer_cancel_wait()
  bool
  cancel_wait(struct io_timer_wait& wait) noexcept {
    return io_timer_cancel_wait(*this, &wait) != 0;
  }

  /// @see io_timer_abort_wait()
  bool
  abort_wait(struct io_timer_wait& wait) noexcept {
    return io_timer_abort_wait(*this, &wait) != 0;
  }

  /// @see io_timer_async_wait()
  ev::Future<int, int>
  async_wait(ev_exec_t* exec, struct io_timer_wait** pwait = nullptr) {
    auto future = io_timer_async_wait(*this, exec, pwait);
    if (!future) util::throw_errc("async_wait");
    return ev::Future<int, int>(future);
  }

  /// @see io_timer_async_wait()
  ev::Future<int, int>
  async_wait(struct io_timer_wait** pwait = nullptr) {
    return async_wait(nullptr, pwait);
  }

 protected:
  io_timer_t* timer{nullptr};
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_TIMER_HPP_
