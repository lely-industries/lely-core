/**@file
 * This header file is part of the event library; it contains the C++ interface
 * for the thread-local event loop.
 *
 * @see lely/ev/thrd_loop.h
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

#ifndef LELY_EV_THRD_LOOP_HPP_
#define LELY_EV_THRD_LOOP_HPP_

#include <lely/ev/exec.hpp>
#include <lely/ev/thrd_loop.h>

namespace lely {
namespace ev {

/// The thread-local event loop.
class ThreadLoop : public Executor {
 public:
  /// @see ev_thrd_loop_get_exec()
  static Executor
  get_executor() noexcept {
    return Executor(ev_thrd_loop_get_exec());
  }

  /// @see ev_thrd_loop_stop()
  static void
  stop() noexcept {
    ev_thrd_loop_stop();
  }

  /// @see ev_thrd_loop_stopped()
  static bool
  stopped() noexcept {
    return ev_thrd_loop_stopped() != 0;
  }

  /// @see ev_thrd_loop_restart()
  static void
  restart() noexcept {
    ev_thrd_loop_restart();
  }

  /// @see ev_thrd_loop_run()
  static ::std::size_t
  run() noexcept {
    return ev_thrd_loop_run();
  }

  /// @see ev_thrd_loop_run_one()
  static ::std::size_t
  run_one() noexcept {
    return ev_thrd_loop_run_one();
  }
};

}  // namespace ev
}  // namespace lely

#endif  // !LELY_EV_THRD_LOOP_HPP_
