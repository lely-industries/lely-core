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

#ifndef LELY_AIO_EXEC_HPP_
#define LELY_AIO_EXEC_HPP_

#include <lely/aio/detail/cbase.hpp>

#include <lely/aio/exec.h>

#include <functional>
#include <system_error>
#include <utility>

namespace lely {

namespace aio {

class ExecutorBase : public detail::CBase<aio_exec_t> {
 public:
  using CBase::CBase;

  void Run(aio_task& task);

  void Dispatch(aio_task& task);

  void Post(aio_task& task);

  void Defer(aio_task& task);

  void OnTaskStarted();

  void OnTaskFinished();
};

class Executor : public ExecutorBase {
 public:
  explicit Executor(LoopBase& loop);

  Executor(const Executor&) = delete;

  Executor(Executor&& other) : ExecutorBase(other.c_ptr) {
    other.c_ptr = nullptr;
  }

  Executor& operator=(const Executor&) = delete;

  Executor&
  operator=(Executor&& other) {
    ::std::swap(c_ptr, other.c_ptr);
    return *this;
  }

  ~Executor();
};

class Task : public aio_task {
 public:
  using Signature = void(::std::error_code ec);

  template <class F>
  Task(F&& f)
      : aio_task AIO_TASK_INIT(nullptr, &Func_), func_(::std::forward<F>(f)) {}

  Task(const Task&) = delete;

  Task& operator=(const Task&) = delete;

  ExecutorBase
  GetExecutor() const noexcept {
    return ExecutorBase(exec);
  }

 private:
  static void Func_(aio_task* task) noexcept;

  ::std::function<Signature> func_;
};

class TaskWrapper : public aio_task {
 public:
  using Signature = void(::std::error_code ec);

  template <class F>
  TaskWrapper(F&& f)
      : aio_task AIO_TASK_INIT(nullptr, &Func_), func_(::std::forward<F>(f)) {}

  TaskWrapper(const Task&) = delete;

  TaskWrapper& operator=(const Task&) = delete;

 private:
  ~TaskWrapper() = default;

  static void Func_(aio_task* task) noexcept;

  ::std::function<Signature> func_;
};

}  // namespace aio

}  // namespace lely

#endif  // LELY_AIO_EXEC_HPP_
