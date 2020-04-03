/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the I/O context.
 *
 * @see lely/coapp/io_context.hpp
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

#include "coapp.hpp"
#include <lely/coapp/io_context.hpp>

namespace lely {

namespace canopen {

IoContext::IoContext(io::TimerBase& timer, io::CanChannelBase& chan)
    : io::CanNet(timer, chan, 0, 0) {}

IoContext::~IoContext() = default;

ev::Executor
IoContext::GetExecutor() const noexcept {
  return get_executor();
}

io::ContextBase
IoContext::GetContext() const noexcept {
  return get_ctx();
}

io::Clock
IoContext::GetClock() const noexcept {
  return get_clock();
}

void
IoContext::SubmitWait(const time_point& t, io_tqueue_wait& wait) {
  wait.value = util::to_timespec(t);
  io_tqueue_submit_wait(*this, &wait);
}

void
IoContext::SubmitWait(const duration& d, io_tqueue_wait& wait) {
  SubmitWait(GetClock().gettime() + d, wait);
}

ev::Future<void, ::std::exception_ptr>
IoContext::AsyncWait(ev_exec_t* exec, const time_point& t,
                     io_tqueue_wait** pwait) {
  if (!exec) exec = GetExecutor();
  auto value = util::to_timespec(t);
  ev::Future<void, int> f{io_tqueue_async_wait(*this, exec, &value, pwait)};
  if (!f) util::throw_errc("AsyncWait");
  return f.then(exec, [](ev::Future<void, int> f) {
    // Convert the error code into an exception pointer.
    int errc = f.get().error();
    if (errc) util::throw_errc("AsyncWait", errc);
  });
}

ev::Future<void, ::std::exception_ptr>
IoContext::AsyncWait(ev_exec_t* exec, const duration& d,
                     io_tqueue_wait** pwait) {
  return AsyncWait(exec, GetClock().gettime() + d, pwait);
}

bool
IoContext::CancelWait(io_tqueue_wait& wait) noexcept {
  return io_tqueue_cancel_wait(*this, &wait) != 0;
}

bool
IoContext::AbortWait(io_tqueue_wait& wait) noexcept {
  return io_tqueue_abort_wait(*this, &wait) != 0;
}

void
IoContext::OnCanState(
    ::std::function<void(io::CanState, io::CanState)> on_can_state) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  on_can_state_ = on_can_state;
}

void
IoContext::OnCanError(::std::function<void(io::CanError)> on_can_error) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  on_can_error_ = on_can_error;
}

CANNet*
IoContext::net() const noexcept {
  return *this;
}

void
IoContext::SetTime() {
  set_time();
}

void
IoContext::on_can_state(io::CanState new_state,
                        io::CanState old_state) noexcept {
  OnCanState(new_state, old_state);
  if (on_can_state_) {
    auto f = on_can_state_;
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    f(new_state, old_state);
  }
}

void
IoContext::on_can_error(io::CanError error) noexcept {
  OnCanError(error);
  if (on_can_error_) {
    auto f = on_can_error_;
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    f(error);
  }
}

}  // namespace canopen

}  // namespace lely
