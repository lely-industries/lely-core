/**@file
 * This header file is part of the utilities library; it contains the time
 * function declarations.
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

#ifndef LELY_UTIL_CHRONO_HPP_
#define LELY_UTIL_CHRONO_HPP_

#include <lely/libc/chrono.hpp>
#include <lely/libc/time.h>

#include <limits>

namespace lely {
namespace util {

/// Converts a C11 time interval to a C++11 duration.
inline ::std::chrono::nanoseconds
from_timespec(const timespec& ts) noexcept {
  using ::std::chrono::nanoseconds;
  using ::std::chrono::seconds;
  return seconds(ts.tv_sec) + nanoseconds(ts.tv_nsec);
}

/// Converts a C++11 duration to a C11 time interval.
template <class Rep, class Period>
inline timespec
to_timespec(const ::std::chrono::duration<Rep, Period>& d) noexcept {
  using ::std::chrono::duration_cast;
  using ::std::chrono::nanoseconds;
  using ::std::chrono::seconds;
  auto sec = duration_cast<seconds>(d);
  if (sec.count() < ::std::numeric_limits<time_t>::min())
    return timespec{::std::numeric_limits<time_t>::min(), 0};
  if (sec.count() > ::std::numeric_limits<time_t>::max())
    return timespec{::std::numeric_limits<time_t>::max(), 0};
  auto nsec = duration_cast<nanoseconds>(d - sec);
  return timespec{static_cast<time_t>(sec.count()),
                  // NOLINTNEXTLINE(runtime/int)
                  static_cast<long>(nsec.count())};
}

/**
 * Converts a C++11 time point to a C11 time interval with respect to
 * <b>Clock</b>'s epoch.
 */
template <class Clock, class Duration>
inline timespec
to_timespec(const ::std::chrono::time_point<Clock, Duration>& t) noexcept {
  return to_timespec(t.time_since_epoch());
}

}  // namespace util
}  // namespace lely

#endif  // !LELY_UTIL_CHRONO_HPP_
