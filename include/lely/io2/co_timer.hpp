/**@file
 * This header file is part of the I/O library; it contains the base class for
 * I/O timer wait operations with a stackless coroutine as the completion task.
 *
 * @see lely/util/coroutine.hpp, lely/io2/timer.hpp
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

#ifndef LELY_IO2_CO_TIMER_HPP_
#define LELY_IO2_CO_TIMER_HPP_

#include <lely/io2/timer.hpp>
#include <lely/util/coroutine.hpp>

namespace lely {
namespace io {

/**
 * A wait operation, suitable for use with an I/O timer, with a stackless
 * coroutine as the completion task.
 */
class CoTimerWait : public io_timer_wait, public util::Coroutine {
 public:
  /// Constructs a wait operation.
  explicit CoTimerWait(ev_exec_t* exec) noexcept
      : io_timer_wait IO_TIMER_WAIT_INIT(exec, [](ev_task* task) noexcept {
          auto wait = io_timer_wait_from_task(task);
          auto overrun = wait->r.result;
          ::std::error_code ec;
          if (overrun == -1) ec = util::make_error_code(wait->r.errc);
          auto self = static_cast<CoTimerWait*>(wait);
          (*self)(overrun, ec);
        }) {}

  /// Constructs a wait operation.
  CoTimerWait() noexcept : CoTimerWait(nullptr) {}

  virtual ~CoTimerWait() = default;

  operator ev_task&() & noexcept { return task; }

  /// Returns the executor to which the completion task is (to be) submitted.
  ev::Executor
  get_executor() const noexcept {
    return ev::Executor(task.exec);
  }

  /**
   * The coroutine to be executed once the wait operation completes (or is
   * canceled).
   *
   * @param overrun the expiration overrun count (see io_timer_getoverrun()) on
   *                success, or -1 on error (or if the operation is canceled).
   * @param ec      the error code if <b>overrun</b> is -1.
   */
  virtual void operator()(int overrun, ::std::error_code ec) noexcept = 0;
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_CO_TIMER_HPP_
