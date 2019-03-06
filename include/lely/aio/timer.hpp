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

#ifndef LELY_AIO_TIMER_HPP_
#define LELY_AIO_TIMER_HPP_

#include <lely/aio/detail/timespec.hpp>
#include <lely/aio/exec.hpp>

#include <lely/aio/timer.h>

#include <tuple>

namespace lely {

namespace aio {

class ClockBase : public detail::CBase<aio_clock_t> {
 public:
  using CBase::CBase;

  using duration = ::std::chrono::nanoseconds;
  using time_point = ::std::chrono::time_point<ClockBase, duration>;

  duration GetResolution() const;

  time_point GetTime() const;

  void SetTime(const time_point& t);
};

template <class Clock>
class BasicClock : public ClockBase {
 public:
  BasicClock() noexcept;
};

using SystemClock = BasicClock<::std::chrono::system_clock>;
using SteadyClock = BasicClock<::std::chrono::steady_clock>;

class TimerBase : public detail::CBase<aio_timer_t> {
 public:
  using CBase::CBase;

  using duration = ClockBase::duration;
  using time_point = ClockBase::time_point;

  using WaitOperation = Task;

  ClockBase GetClock() const noexcept;

  int GetOverrun() const;

  ::std::tuple<duration, duration> GetTime() const;

  ::std::tuple<duration, duration> SetTime(const duration& expiry,
                                           const duration& period = {});

  ::std::tuple<duration, duration> SetTime(const time_point& expiry,
                                           const duration& period = {});

  ExecutorBase GetExecutor() const noexcept;

  void SubmitWait(aio_task& task);

  void
  SubmitWait(WaitOperation& op) {
    SubmitWait(static_cast<aio_task&>(op));
  }

  template <class F>
  void
  SubmitWait(F&& f) {
    aio_task* task = new WaitOperationWrapper(::std::forward<F>(f));
    SubmitWait(*task);
  }

  ::std::size_t Cancel(aio_task& task);

  ::std::size_t Cancel();

  Future<aio_task> AsyncWait(LoopBase& loop);

  Future<aio_task> AsyncWait(LoopBase& loop, aio_task*& ptask);

  void RunWait(LoopBase& loop);

  void RunWait(LoopBase& loop, ::std::error_code& ec);

  void
  RunWaitFor(LoopBase& loop) {
    RunWaitUntil(loop, nullptr);
  }

  void
  RunWaitFor(LoopBase& loop, ::std::error_code& ec) {
    RunWaitUntil(loop, nullptr, ec);
  }

  template <class Rep, class Period>
  void
  RunWaitFor(LoopBase& loop,
             const ::std::chrono::duration<Rep, Period>& rel_time) {
    auto ts = detail::AbsTime(rel_time);
    RunWaitUntil(loop, &ts);
  }

  template <class Rep, class Period>
  void
  RunWaitFor(LoopBase& loop,
             const ::std::chrono::duration<Rep, Period>& rel_time,
             ::std::error_code& ec) {
    auto ts = detail::AbsTime(rel_time);
    RunWaitUntil(loop, &ts, ec);
  }

  template <class Clock, class Duration>
  void
  RunWaitUntil(LoopBase& loop,
               const ::std::chrono::time_point<Clock, Duration>& abs_time) {
    auto ts = detail::AbsTime(abs_time);
    RunWaitUntil(loop, &ts);
  }

  template <class Clock, class Duration>
  void
  RunWaitUntil(LoopBase& loop,
               const ::std::chrono::time_point<Clock, Duration>& abs_time,
               ::std::error_code& ec) {
    auto ts = detail::AbsTime(abs_time);
    RunWaitUntil(loop, &ts, ec);
  }

 protected:
  using WaitOperationWrapper = TaskWrapper;

  void RunWaitUntil(LoopBase& loop, const timespec* tp);

  void RunWaitUntil(LoopBase& loop, const timespec* tp, ::std::error_code& ec);
};

class Timer : public TimerBase {
 public:
  Timer(clockid_t clockid, ExecutorBase& exec, ReactorBase& reactor);

  Timer(const Timer&) = delete;

  Timer(Timer&& other) : TimerBase(other.c_ptr) { other.c_ptr = nullptr; }

  Timer& operator=(const Timer&) = delete;

  Timer&
  operator=(Timer&& other) {
    ::std::swap(c_ptr, other.c_ptr);
    return *this;
  }

  ~Timer();
};

template <class Clock>
class BasicTimer : public Timer {
 public:
  BasicTimer(ExecutorBase& exec, ReactorBase& reactor);
};

using SystemTimer = BasicTimer<::std::chrono::system_clock>;
using SteadyTimer = BasicTimer<::std::chrono::steady_clock>;

}  // namespace aio

}  // namespace lely

#endif  // LELY_AIO_TIMER_HPP_
