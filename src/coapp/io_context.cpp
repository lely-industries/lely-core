/*!\file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the I/O context.
 *
 * \see lely/coapp/io_context.hpp
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

#include "coapp.hpp"
#include <lely/can/net.hpp>
#include <lely/aio/detail/timespec.hpp>
#include <lely/coapp/io_context.hpp>

#include <mutex>

namespace lely {

namespace canopen {

using namespace aio;

//! The internal implementation of the I/O context.
struct IoContext::Impl_ : public BasicLockable {
  Impl_(IoContext* self, TimerBase& timer, CanBusBase& bus,
        BasicLockable* mutex);

  void lock() override { if (mutex) mutex->lock(); }
  void unlock() override { if (mutex) mutex->unlock(); }

  void operator()(::std::error_code ec) noexcept;
  void operator()(::std::error_code ec, int result) noexcept;

  int OnNext(const timespec* tp) noexcept;
  int OnSend(const can_msg* msg) noexcept;

  IoContext* self { nullptr };

  TimerBase& timer;
  TimerBase::WaitOperation wait_op;

  CanBusBase& bus;
  CanBusBase::Frame msg CAN_MSG_INIT;
  CanBusBase::Info info CAN_MSG_INFO_INIT;
  int state { 0 };
  int error { 0 };
  CanBusBase::ReadOperation read_op;

  BasicLockable* mutex { nullptr };

  unique_c_ptr<CANNet> net;
};

LELY_COAPP_EXPORT
IoContext::IoContext(aio::TimerBase& timer, aio::CanBusBase& bus,
                     BasicLockable* mutex)
    : impl_(new Impl_(this, timer, bus, mutex)) {}

LELY_COAPP_EXPORT IoContext::~IoContext() = default;

LELY_COAPP_EXPORT aio::ExecutorBase
IoContext::GetExecutor() const noexcept {
  return impl_->timer.GetExecutor();
}

LELY_COAPP_EXPORT CANNet*
IoContext::net() const noexcept { return impl_->net.get(); }

LELY_COAPP_EXPORT void
IoContext::SetTime() {
    net()->setTime(aio::detail::ToTimespec(impl_->timer.GetClock().GetTime()));
}

IoContext::Impl_::Impl_(IoContext* self_, TimerBase& timer_, CanBusBase& bus_,
                        BasicLockable* mutex_)
    : self(self_), timer(timer_), wait_op(::std::ref(*this)), bus(bus_),
      read_op(&msg, &info, ::std::ref(*this)), mutex(mutex_),
      net(make_unique_c<CANNet>()) {
  // Initialize the CAN network clock with the current time.
  net->setTime(aio::detail::ToTimespec(timer.GetClock().GetTime()));

  // Register the OnNext() member function as the function to be invoked when
  // the time at which the next timer triggers is updated.
  net->setNextFunc<Impl_, &Impl_::OnNext>(this);
  // Register the OnSend() member function as the function to be invoked when a
  // CAN frame needs to be sent.
  net->setSendFunc<Impl_, &Impl_::OnSend>(this);

  // Start waiting for timeouts.
  timer.SubmitWait(wait_op);

  // Start receiving CAN frames.
  bus.SubmitRead(read_op);
}

void
IoContext::Impl_::operator()(::std::error_code ec) noexcept {
  if (!ec) {
    ::std::lock_guard<Impl_> lock(*this);
    self->SetTime();
  }
  timer.SubmitWait(wait_op);
}

void
IoContext::Impl_::operator()(::std::error_code ec, int result) noexcept {
  if (!ec) {
    ::std::lock_guard<Impl_> lock(*this);
    if (result == 1) {
      net->recv(msg);
    } else if (!result) {
      if (info.state != state) {
        ::std::swap(info.state, state);
        self->OnCanState(static_cast<CanState>(state),
                         static_cast<CanState>(info.state));
      }
      if (info.error != error) {
        error = info.error;
        self->OnCanError(static_cast<CanError>(error));
      }
    }
  }
  bus.SubmitRead(read_op);
}

int
IoContext::Impl_::OnNext(const timespec* tp) noexcept {
  timer.SetTime(TimerBase::time_point(aio::detail::FromTimespec(*tp)));
  return 0;
}

int
IoContext::Impl_::OnSend(const can_msg* msg) noexcept {
  // The CAN network interface does not support asynchronous writes, so we try a
  // non-blocking synchronous write.
  ::std::error_code ec;
  if (!bus.Write(*msg, ec)) {
    if (ec)
      set_errc(ec.value());
    return -1;
  }
  return 0;
}

}  // namespace canopen

}  // namespace lely
