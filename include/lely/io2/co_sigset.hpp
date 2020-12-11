/**@file
 * This header file is part of the I/O library; it contains the base class for
 * signal wait operations with a stackless coroutine as the completion task.
 *
 * @see lely/util/coroutine.hpp, lely/io2/sigset.hpp
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

#ifndef LELY_IO2_CO_SIGSET_HPP_
#define LELY_IO2_CO_SIGSET_HPP_

#include <lely/io2/sigset.hpp>
#include <lely/util/coroutine.hpp>

namespace lely {
namespace io {

/**
 * A wait operation, suitable for use with a signal handler, with a stackless
 * coroutine as the completion task.
 */
class CoSignalSetWait : public io_sigset_wait, public util::Coroutine {
 public:
  /// Constructs a wait operation.
  explicit CoSignalSetWait(ev_exec_t* exec) noexcept
      : io_sigset_wait IO_SIGSET_WAIT_INIT(exec, [](ev_task* task) noexcept {
          auto wait = io_sigset_wait_from_task(task);
          auto signo = wait->signo;
          auto self = static_cast<CoSignalSetWait*>(wait);
          (*self)(signo);
        }) {}

  /// Constructs a wait operation.
  CoSignalSetWait() noexcept : CoSignalSetWait(nullptr) {}

  virtual ~CoSignalSetWait() = default;

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
   * @param signo the signal number, or -1 if the operation is canceled.
   */
  virtual void operator()(int signo) noexcept = 0;
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_CO_SIGSET_HPP_
