/**@file
 * This header file is part of the event library; it contains the basic C++
 * task interface.
 *
 * @see lely/ev/task.h
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

#ifndef LELY_EV_TASK_HPP_
#define LELY_EV_TASK_HPP_

#include <lely/ev/task.h>
#include <lely/util/invoker.hpp>

#include <functional>
#include <utility>

namespace lely {
namespace ev {

class Executor;

namespace detail {

template <class Invoker>
class TaskWrapper : public ev_task {
 public:
  template <class F, class... Args>
  TaskWrapper(ev_exec_t* exec, F&& f, Args&&... args)
      : ev_task EV_TASK_INIT(exec,
                             [](ev_task* task) noexcept {
                               auto self = static_cast<TaskWrapper*>(task);
                               self->invoker_();
                               delete self;
                             }),
        invoker_(::std::forward<F>(f), ::std::forward<Args>(args)...) {}

  TaskWrapper(const TaskWrapper&) = delete;

  TaskWrapper& operator=(const TaskWrapper&) = delete;

 private:
  Invoker invoker_;
};

}  // namespace detail

/**
 * A helper alias template for the result of
 * #lely::ev::make_task_wrapper<F, Args...>().
 */
template <class F, class... Args>
using TaskWrapper = detail::TaskWrapper<util::invoker_t<F, Args...>>;

/**
 * Creates a temporary task from a callable object with an associated executor
 * (can be nullptr). The task deletes itself after it is run, so it MUST NOT be
 * deleted once it is submitted to an executor.
 */
template <class F, class... Args>
inline TaskWrapper<F, Args...>*
make_task_wrapper(ev_exec_t* exec, F&& f, Args&&... args) {
  return new TaskWrapper<F, Args...>(exec, ::std::forward<F>(f),
                                     ::std::forward<Args>(args)...);
}

/**
 * A basic task. This class turns a callable object with signature `void()` into
 * a task which can be submitted to an executor.
 */
class Task : public ev_task {
 public:
  using Signature = void();

  /**
   * Constructs a task from a callable object with an associated executor (can
   * be nullptr).
   */
  template <class F>
  Task(ev_exec_t* exec, F&& f)
      : ev_task EV_TASK_INIT(exec,
                             [](ev_task* task) noexcept {
                               auto self = static_cast<Task*>(task);
                               if (self->func_) self->func_();
                             }),
        func_(::std::forward<F>(f)) {}

  /// Constructs a task from a callable object.
  template <class F>
  explicit Task(F&& f) : Task(nullptr, ::std::forward<F>(f)) {}

  Task(const Task&) = delete;

  Task& operator=(const Task&) = delete;

  /// Returns the executor to which the task is (to be) submitted.
  Executor get_executor() const noexcept;

 private:
  ::std::function<Signature> func_;
};

}  // namespace ev
}  // namespace lely

#endif  // !LELY_EV_TASK_HPP_
