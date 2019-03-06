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

#ifndef LELY_AIO_DETAIL_TIMESPEC_HPP_
#define LELY_AIO_DETAIL_TIMESPEC_HPP_

#include <lely/aio/aio.hpp>

#include <lely/libc/time.h>

#include <chrono>
#include <type_traits>

namespace lely {

namespace aio {

namespace detail {

using ClockType = ::std::chrono::system_clock;

inline ::std::chrono::nanoseconds
FromTimespec(const timespec& ts) {
  ::std::chrono::seconds sec(ts.tv_sec);
  ::std::chrono::nanoseconds nsec(ts.tv_nsec);
  return sec + nsec;
}

template <class Rep, class Period>
inline timespec
ToTimespec(const ::std::chrono::duration<Rep, Period>& d) {
  auto sec = ::std::chrono::duration_cast<::std::chrono::seconds>(d);
  auto nsec = ::std::chrono::duration_cast<::std::chrono::nanoseconds>(d - sec);
  return timespec{sec.count(), nsec.count()};
}

template <class Clock, class Duration>
inline timespec
ToTimespec(const ::std::chrono::time_point<Clock, Duration>& t) {
  return ToTimespec(t.time_since_epoch());
}

inline timespec
AbsTime(const ClockType::time_point& abs_time) {
  return ToTimespec(abs_time);
}

template <class Duration>
inline typename ::std::enable_if<
    !::std::is_same<Duration, ClockType::duration>::value, timespec>::type
AbsTime(const ::std::chrono::time_point<ClockType, Duration>& abs_time) {
  return AbsTime(
      ::std::chrono::time_point_cast<ClockType::time_point>(abs_time));
}

template <class Rep, class Period>
inline timespec
AbsTime(const ::std::chrono::duration<Rep, Period>& rel_time) {
  return AbsTime(ClockType::now() + rel_time);
}

template <class Clock, class Duration>
inline typename ::std::enable_if<!::std::is_same<Clock, ClockType>::value,
                                 timespec>::type
AbsTime(const ::std::chrono::time_point<Clock, Duration>& abs_time) {
  return AbsTime(abs_time - Clock::now());
}

}  // namespace detail

}  // namespace aio

}  // namespace lely

#endif  // LELY_AIO_DETAIL_TIMESPEC_HPP_
