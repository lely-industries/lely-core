/**@file
 * This file is part of the C++ asynchronous I/O library; it contains ...
 *
 * @see lely/aio/loop.hpp
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

#include "aio.hpp"

#if !LELY_NO_CXX

#include <lely/aio/detail/timespec.hpp>
#include <lely/aio/loop.hpp>

namespace lely {

namespace aio {

PollBase
LoopBase::GetPoll() const noexcept {
  return aio_loop_get_poll(*this);
}

PromiseBase
LoopBase::GetPromise(ExecutorBase& exec, aio_dtor_t* dtor, void* arg) {
  return PromiseBase(*this, exec, dtor, arg);
}

void
LoopBase::Post(aio_task& task) {
  aio_loop_post(*this, &task);
}

void
LoopBase::OnTaskStarted() {
  aio_loop_on_task_started(*this);
}

void
LoopBase::OnTaskFinished() {
  aio_loop_on_task_finished(*this);
}

::std::size_t
LoopBase::Run() {
  return InvokeC("Run", aio_loop_run, *this, nullptr, 0);
}

::std::size_t
LoopBase::Run(::std::error_code& ec) {
  return InvokeC(ec, aio_loop_run, *this, nullptr, 0);
}

void
LoopBase::Stop() {
  aio_loop_stop(*this);
}

bool
LoopBase::Stopped() const noexcept {
  return !!aio_loop_stopped(*this);
}

void
LoopBase::Restart() {
  aio_loop_restart(*this);
}

::std::size_t
LoopBase::RunUntil(const timespec* tp) {
  return InvokeC("RunUntil", aio_loop_run_until, *this, nullptr, 0, tp);
}

::std::size_t
LoopBase::RunUntil(const timespec* tp, ::std::error_code& ec) {
  return InvokeC(ec, aio_loop_run_until, *this, nullptr, 0, tp);
}

Loop::Loop() : LoopBase(InvokeC("Loop", aio_loop_create, nullptr)) {}

Loop::Loop(const PollBase& poll)
    : LoopBase(InvokeC("Loop", aio_loop_create, poll)) {}

Loop::~Loop() { aio_loop_destroy(*this); }

}  // namespace aio

}  // namespace lely

#endif  // !LELY_NO_CXX
