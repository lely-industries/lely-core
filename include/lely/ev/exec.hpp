/**@file
 * This header file is part of the event library; it contains the C++ interface
 * for the abstract task executor.
 *
 * @see lely/ev/exec.h
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

#ifndef LELY_EV_EXEC_HPP_
#define LELY_EV_EXEC_HPP_

#include <lely/ev/exec.h>
#include <lely/ev/task.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace lely {
namespace ev {

/// An abstract task executor. This class is a wrapper around `#ev_exec_t*`.
class Executor {
 public:
  Executor(ev_exec_t* exec) noexcept : exec_(exec) {}

  operator ev_exec_t*() const noexcept { return exec_; }

  /// @see ev_exec_on_task_init()
  void
  on_task_init() noexcept {
    ev_exec_on_task_init(*this);
  }

  /// @see ev_exec_on_task_fini()
  void
  on_task_fini() noexcept {
    ev_exec_on_task_fini(*this);
  }

  /// @see ev_exec_dispatch()
  bool
  dispatch(ev_task& task) noexcept {
    return ev_exec_dispatch(*this, &task) != 0;
  }

  /// @see ev_exec_dispatch()
  template <class F, class... Args>
  typename ::std::enable_if<
      !::std::is_base_of<ev_task, typename ::std::decay<F>::type>::value,
      bool>::type
  dispatch(F&& f, Args&&... args) {
    auto task = make_task_wrapper(*this, ::std::forward<F>(f),
                                  ::std::forward<Args>(args)...);
    dispatch(*task);
  }

  /// @see ev_exec_post()
  void
  post(ev_task& task) noexcept {
    ev_exec_post(*this, &task);
  }

  /// @see ev_exec_post()
  template <class F, class... Args>
  typename ::std::enable_if<
      !::std::is_base_of<ev_task, typename ::std::decay<F>::type>::value>::type
  post(F&& f, Args&&... args) {
    auto task = make_task_wrapper(*this, ::std::forward<F>(f),
                                  ::std::forward<Args>(args)...);
    post(*task);
  }

  /// @see ev_exec_defer()
  void
  defer(ev_task& task) noexcept {
    ev_exec_defer(*this, &task);
  }

  /// @see ev_exec_defer()
  template <class F, class... Args>
  typename ::std::enable_if<
      !::std::is_base_of<ev_task, typename ::std::decay<F>::type>::value>::type
  defer(F&& f, Args&&... args) {
    auto task = make_task_wrapper(*this, ::std::forward<F>(f),
                                  ::std::forward<Args>(args)...);
    defer(*task);
  }

  /// @see ev_exec_abort()
  bool
  abort(ev_task& task) noexcept {
    return ev_exec_abort(*this, &task) != 0;
  }

  /// @see ev_exec_abort()
  ::std::size_t
  abort_all() noexcept {
    return ev_exec_abort(*this, nullptr);
  }

  /// @see ev_exec_run()
  void
  run(ev_task& task) noexcept {
    ev_exec_run(*this, &task);
  }

 protected:
  ev_exec_t* exec_{nullptr};
};

inline Executor
Task::get_executor() const noexcept {
  return Executor(exec);
}

}  // namespace ev
}  // namespace lely

#endif  // !LELY_EV_EXEC_HPP_
