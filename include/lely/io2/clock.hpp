/**@file
 * This header file is part of the I/O library; it contains the C++ interface
 * for the abstract clock.
 *
 * @see lely/io2/clock.h
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

#ifndef LELY_IO2_CLOCK_HPP_
#define LELY_IO2_CLOCK_HPP_

#include <lely/io2/clock.h>
#include <lely/util/chrono.hpp>
#include <lely/util/error.hpp>

namespace lely {
namespace io {

/// An abstract clock. This class is a wrapper around `#io_clock_t*`.
class Clock {
 public:
  using duration = ::std::chrono::nanoseconds;
  using time_point = ::std::chrono::time_point<Clock, duration>;

  explicit constexpr Clock(io_clock_t* clock) noexcept : clock_(clock) {}

  operator io_clock_t*() const noexcept { return clock_; }

  /// @see io_clock_getres()
  duration
  getres(::std::error_code& ec) const noexcept {
    int errsv = get_errc();
    set_errc(0);
    timespec res = {0, 0};
    if (!io_clock_getres(*this, &res))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
    return util::from_timespec(res);
  }

  /// @see io_clock_getres()
  duration
  getres() const {
    ::std::error_code ec;
    auto res = getres(ec);
    if (ec) throw ::std::system_error(ec, "getres");
    return res;
  }

  /// @see io_clock_gettime()
  time_point
  gettime(::std::error_code& ec) const noexcept {
    int errsv = get_errc();
    set_errc(0);
    timespec ts = {0, 0};
    if (!io_clock_gettime(*this, &ts))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
    return time_point(util::from_timespec(ts));
  }

  /// @see io_clock_gettime()
  time_point
  gettime() const {
    ::std::error_code ec;
    auto t = gettime(ec);
    if (ec) throw ::std::system_error(ec, "gettime");
    return t;
  }

  /// @see io_clock_settime()
  void
  settime(const time_point& t, ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    auto ts = util::to_timespec(t);
    if (!io_clock_settime(*this, &ts))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_clock_settime()
  void
  settime(const time_point& t) {
    ::std::error_code ec;
    settime(t, ec);
    if (ec) throw ::std::system_error(ec, "settime");
  }

 private:
  io_clock_t* clock_{nullptr};
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_CLOCK_HPP_
