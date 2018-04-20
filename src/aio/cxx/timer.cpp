/*!\file
 * This file is part of the C++ asynchronous I/O library; it contains ...
 *
 * \see lely/aio/timer.hpp
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

#include "aio.hpp"

#if !LELY_NO_CXX

#include <lely/aio/loop.hpp>
#include <lely/aio/reactor.hpp>
#include <lely/aio/timer.hpp>

namespace lely {

namespace aio {

namespace {

inline ::std::tuple<::std::chrono::nanoseconds, ::std::chrono::nanoseconds>
FromItimerspec(const itimerspec& value) {
  auto period = detail::FromTimespec(value.it_interval);
  auto expiry = detail::FromTimespec(value.it_value);
  return ::std::make_tuple(expiry, period);
}

}  // namespace

LELY_AIO_EXPORT ClockBase::duration
ClockBase::GetResolution() const {
  timespec ts = { 0, 0 };
  if (aio_clock_getres(*this, &ts) == -1)
    throw_errc("GetResolution");
  return detail::FromTimespec(ts);
}

LELY_AIO_EXPORT ClockBase::time_point
ClockBase::GetTime() const {
  timespec ts = { 0, 0 };
  if (aio_clock_gettime(*this, &ts) == -1)
    throw_errc("GetTime");
  return time_point(detail::FromTimespec(ts));
}

LELY_AIO_EXPORT void
ClockBase::SetTime(const time_point& t) {
  auto ts = detail::ToTimespec(t);
  if (aio_clock_settime(*this, &ts) == -1)
    throw_errc("SetTime");
}

#if LELY_AIO_WITH_CLOCK

template <class> struct BasicClockTraits;

template <>
struct BasicClockTraits<::std::chrono::system_clock> {
  static constexpr aio_clock_t* clock() noexcept { return AIO_CLOCK_REALTIME; }
};

template <>
struct BasicClockTraits<::std::chrono::steady_clock> {
  static constexpr aio_clock_t* clock() noexcept { return AIO_CLOCK_MONOTONIC; }
};

template <class Clock>
LELY_AIO_EXPORT BasicClock<Clock>::BasicClock() noexcept
    : ClockBase(BasicClockTraits<Clock>::clock()) {}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
template class LELY_AIO_EXPORT BasicClock<::std::chrono::system_clock>;
template class LELY_AIO_EXPORT BasicClock<::std::chrono::steady_clock>;
#endif

#endif  // LELY_AIO_WITH_CLOCK

LELY_AIO_EXPORT ClockBase
TimerBase::GetClock() const noexcept { return aio_timer_get_clock(*this); }

LELY_AIO_EXPORT int
TimerBase::GetOverrun() const {
  int overrun = aio_timer_getoverrun(*this);
  if (overrun == -1)
    throw_errc("GetOverrun");
  return overrun;
}

LELY_AIO_EXPORT ::std::tuple<TimerBase::duration, TimerBase::duration>
TimerBase::GetTime() const {
  itimerspec value = { { 0, 0 }, { 0, 0 } };
  if (aio_timer_gettime(*this, &value) == -1)
    throw_errc("GetTime");
  return FromItimerspec(value);
}

LELY_AIO_EXPORT ExecutorBase
TimerBase::GetExecutor() const noexcept { return aio_timer_get_exec(*this); }

LELY_AIO_EXPORT ::std::tuple<TimerBase::duration, TimerBase::duration>
TimerBase::SetTime(const duration& expiry, const duration& period) {
  itimerspec value = { detail::ToTimespec(period), detail::ToTimespec(expiry) };
  itimerspec ovalue = { { 0, 0 }, { 0, 0 } };
  if (aio_timer_settime(*this, 0, &value, &ovalue) == -1)
    throw_errc("SetTime");
  return FromItimerspec(ovalue);
}

LELY_AIO_EXPORT ::std::tuple<TimerBase::duration, TimerBase::duration>
TimerBase::SetTime(const time_point& expiry, const duration& period) {
  itimerspec value = { detail::ToTimespec(period), detail::ToTimespec(expiry) };
  itimerspec ovalue = { { 0, 0 }, { 0, 0 } };
  if (aio_timer_settime(*this, TIMER_ABSTIME, &value, &ovalue) == -1)
    throw_errc("SetTime");
  return FromItimerspec(ovalue);
}

LELY_AIO_EXPORT void
TimerBase::SubmitWait(aio_task& task) { aio_timer_submit_wait(*this, &task); }

LELY_AIO_EXPORT ::std::size_t
TimerBase::Cancel(aio_task& task) { return aio_timer_cancel(*this, &task); }

LELY_AIO_EXPORT ::std::size_t
TimerBase::Cancel() { return aio_timer_cancel(*this, nullptr); }

LELY_AIO_EXPORT Future<aio_task>
TimerBase::AsyncWait(LoopBase& loop) {
  return Future<aio_task>(InvokeC("AsyncWait", aio_timer_async_wait, *this,
                                  loop, nullptr));
}

LELY_AIO_EXPORT Future<aio_task>
TimerBase::AsyncWait(LoopBase& loop, aio_task*& ptask) {
  return Future<aio_task>(InvokeC("AsyncWait", aio_timer_async_wait, *this,
                                  loop, &ptask));
}

LELY_AIO_EXPORT void
TimerBase::RunWait(LoopBase& loop) {
  InvokeC("RunWait", aio_timer_run_wait, *this, loop);
}

LELY_AIO_EXPORT void
TimerBase::RunWait(LoopBase& loop, ::std::error_code& ec) {
  InvokeC(ec, aio_timer_run_wait, *this, loop);
}

LELY_AIO_EXPORT void
TimerBase::RunWaitUntil(LoopBase& loop, const timespec* tp) {
  InvokeC("RunWaitUntil", aio_timer_run_wait_until, *this, loop, tp);
}

LELY_AIO_EXPORT void
TimerBase::RunWaitUntil(LoopBase& loop, const timespec* tp,
                        ::std::error_code& ec) {
  InvokeC(ec, aio_timer_run_wait_until, *this, loop, tp);
}

#if LELY_AIO_WITH_TIMER

LELY_AIO_EXPORT
Timer::Timer(clockid_t clockid, ExecutorBase& exec, ReactorBase& reactor)
    : TimerBase(InvokeC("Timer", aio_timer_create, clockid, exec, reactor)) {}

LELY_AIO_EXPORT Timer::~Timer() { aio_timer_destroy(*this); }

#if LELY_AIO_WITH_CLOCK

template <class> struct BasicTimerTraits;

template <>
struct BasicTimerTraits<::std::chrono::system_clock> {
  static constexpr clockid_t clockid() noexcept { return CLOCK_REALTIME; }
};

template <>
struct BasicTimerTraits<::std::chrono::steady_clock> {
  static constexpr clockid_t clockid() noexcept { return CLOCK_MONOTONIC; }
};

template <class Clock> LELY_AIO_EXPORT
BasicTimer<Clock>::BasicTimer(ExecutorBase& exec, ReactorBase& reactor)
    : Timer(BasicTimerTraits<Clock>::clockid(), exec, reactor) {}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
template class LELY_AIO_EXPORT BasicTimer<::std::chrono::system_clock>;
template class LELY_AIO_EXPORT BasicTimer<::std::chrono::steady_clock>;
#endif

#endif  // LELY_AIO_WITH_CLOCK

#endif  // LELY_AIO_WITH_TIMER

}  // namespace aio

}  // namespace lely

#endif  // !LELY_NO_CXX
