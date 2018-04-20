/*!\file
 * This file is part of the C++ asynchronous I/O library; it contains ...
 *
 * \see lely/aio/exec.hpp
 *
 * \copyright 2018 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#include "aio.hpp"

#if !LELY_NO_CXX

#include <lely/aio/exec.hpp>
#include <lely/aio/loop.hpp>

namespace lely {

namespace aio {

LELY_AIO_EXPORT void
ExecutorBase::Run(aio_task& task) { aio_exec_run(*this, &task); }

LELY_AIO_EXPORT void
ExecutorBase::Dispatch(aio_task& task) { aio_exec_dispatch(*this, &task); }

LELY_AIO_EXPORT void
ExecutorBase::Post(aio_task& task) { aio_exec_post(*this, &task); }

LELY_AIO_EXPORT void
ExecutorBase::Defer(aio_task& task) { aio_exec_defer(*this, &task); }

LELY_AIO_EXPORT void
ExecutorBase::OnTaskStarted() { aio_exec_on_task_started(*this); }

LELY_AIO_EXPORT void
ExecutorBase::OnTaskFinished() { aio_exec_on_task_finished(*this); }

LELY_AIO_EXPORT
Executor::Executor(LoopBase& loop)
    : ExecutorBase(InvokeC("Executor", aio_exec_create, loop)) {}

LELY_AIO_EXPORT Executor::~Executor() { aio_exec_destroy(*this); }

LELY_AIO_EXPORT void
Task::Func_(aio_task* task) noexcept {
  auto self = static_cast<Task*>(task);
  if (self->func) {
    auto ec = ::std::error_code(task->errc, ::std::system_category());
    self->func_(ec);
  }
}

LELY_AIO_EXPORT void
TaskWrapper::Func_(aio_task* task) noexcept {
  auto self = static_cast<TaskWrapper*>(task);
  ::std::function<Signature> func(::std::move(self->func_));
  auto ec = ::std::error_code(task->errc, ::std::system_category());
  delete self;
  if (func)
    func(ec);
}

}  // namespace aio

}  // namespace lely

#endif  // !LELY_NO_CXX
