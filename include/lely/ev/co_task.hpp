/**@file
 * This header file is part of the event library; it contains the base class for
 * stackless coroutine tasks.
 *
 * @see lely/util/coroutine.hpp, lely/ev/task.h
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

#ifndef LELY_EV_CO_TASK_HPP_
#define LELY_EV_CO_TASK_HPP_

#include <lely/ev/exec.hpp>
#include <lely/util/coroutine.hpp>

namespace lely {
namespace ev {

/// A stackless coroutine which can be submitted to an executor as a task.
class CoTask : public ev_task, public util::Coroutine {
 public:
  /// Constructs a coroutine task with an associated executor (can be nullptr).
  explicit CoTask(ev_exec_t* exec) noexcept
      : ev_task EV_TASK_INIT(exec, [](ev_task* task) noexcept {
          auto self = static_cast<CoTask*>(task);
          (*self)();
        }) {}

  /// Constructs a coroutine task.
  CoTask() noexcept : CoTask(nullptr) {}

  virtual ~CoTask() = default;

  /// Returns the executor to which the task is (to be) submitted.
  Executor
  get_executor() const noexcept {
    return Executor(exec);
  }

  /// The coroutine to be executed when the task is run.
  virtual void operator()() noexcept = 0;
};

}  // namespace ev
}  // namespace lely

#endif  // !LELY_EV_CO_TASK_HPP_
