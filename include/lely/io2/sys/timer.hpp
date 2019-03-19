/**@file
 * This header file is part of the I/O library; it contains the C++ interface
 * for the I/O system timer.
 *
 * @see lely/io2/sys/timer.h
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

#ifndef LEYL_IO2_SYS_TIMER_HPP_
#define LEYL_IO2_SYS_TIMER_HPP_

#include <lely/io2/sys/timer.h>
#include <lely/io2/timer.hpp>

#include <utility>

namespace lely {
namespace io {

/// An I/O system timer.
class Timer : public TimerBase {
 public:
  /// @see io_timer_create()
  Timer(io_poll_t* poll, ev_exec_t* exec, clockid_t clockid)
      : TimerBase(io_timer_create(poll, exec, clockid)) {
    if (!timer) util::throw_errc("Timer");
  }

  Timer(const Timer&) = delete;

  Timer(Timer&& other) noexcept : TimerBase(other.timer) {
    other.timer = nullptr;
    other.dev = nullptr;
  }

  Timer& operator=(const Timer&) = delete;

  Timer&
  operator=(Timer&& other) noexcept {
    using ::std::swap;
    swap(timer, other.timer);
    swap(dev, other.dev);
    return *this;
  }

  /// @see io_timer_destroy()
  ~Timer() { io_timer_destroy(*this); }
};

}  // namespace io
}  // namespace lely

#endif  // !LEYL_IO2_SYS_TIMER_HPP_
