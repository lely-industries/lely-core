/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the I/O context.
 *
 * @see lely/coapp/io_context.hpp
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

#include "coapp.hpp"
#include <lely/can/net.hpp>
#include <lely/coapp/io_context.hpp>

#if !LELY_NO_THREADS
#include <atomic>
#endif
#include <utility>

namespace lely {

namespace canopen {

/// The internal implementation of the I/O context.
struct IoContext::Impl_ : util::BasicLockable, io_svc {
  Impl_(IoContext* self, io::TimerBase& timer, io::CanChannelBase& chan,
        util::BasicLockable* mutex);
  virtual ~Impl_();

  void
  lock() override {
    if (mutex) mutex->lock();
  }

  void
  unlock() override {
    if (mutex) mutex->unlock();
  }

  void OnShutdown() noexcept;

  void OnWait(int overrun, ::std::error_code ec) noexcept;

  void OnRead(int result, ::std::error_code ec) noexcept;

  int OnNext(const timespec* tp) noexcept;
  int OnSend(const can_msg* msg) noexcept;

  static const io_svc_vtbl svc_vtbl;

  IoContext* self{nullptr};

  io::ContextBase ctx{nullptr};
#if LELY_NO_THREADS
  bool shutdown{false};
#else
  ::std::atomic_bool shutdown{false};
#endif

  io::TimerBase& timer;
  io::TimerWait wait;

  io::CanChannelBase& chan;
  io::CanChannelRead read;
  can_msg msg CAN_MSG_INIT;
  can_err err CAN_ERR_INIT;
  int state{0};
  int error{0};

  util::BasicLockable* mutex{nullptr};

  unique_c_ptr<CANNet> net;
};

// clang-format off
const io_svc_vtbl IoContext::Impl_::svc_vtbl = {
    nullptr, [](io_svc* svc) noexcept {
      static_cast<IoContext::Impl_*>(svc)->OnShutdown();
    }
};
// clang-format on

IoContext::IoContext(io::TimerBase& timer, io::CanChannelBase& chan,
                     util::BasicLockable* mutex)
    : impl_(new Impl_(this, timer, chan, mutex)) {}

IoContext::~IoContext() = default;

ev::Executor
IoContext::GetExecutor() const noexcept {
  return impl_->chan.get_executor();
}

io::ContextBase
IoContext::GetContext() const noexcept {
  return impl_->ctx;
}

CANNet*
IoContext::net() const noexcept {
  return impl_->net.get();
}

void
IoContext::SetTime() {
  net()->setTime(util::to_timespec(impl_->timer.get_clock().gettime()));
}

IoContext::Impl_::Impl_(IoContext* self_, io::TimerBase& timer_,
                        io::CanChannelBase& chan_, util::BasicLockable* mutex_)
    : io_svc IO_SVC_INIT(&svc_vtbl),
      self(self_),
      ctx(timer_.get_ctx()),
      timer(timer_),
      wait([this](int overrun, ::std::error_code ec) { OnWait(overrun, ec); }),
      chan(chan_),
      read(&msg, &err, nullptr,
           [this](int result, ::std::error_code ec) { OnRead(result, ec); }),
      mutex(mutex_),
      net(make_unique_c<CANNet>()) {
  // Initialize the CAN network clock with the current time.
  net->setTime(util::to_timespec(timer.get_clock().gettime()));

  // Register the OnNext() member function as the function to be invoked when
  // the time at which the next timer triggers is updated.
  net->setNextFunc<Impl_, &Impl_::OnNext>(this);
  // Register the OnSend() member function as the function to be invoked when a
  // CAN frame needs to be sent.
  net->setSendFunc<Impl_, &Impl_::OnSend>(this);

  // Start waiting for timeouts.
  timer.submit_wait(wait);

  // Start receiving CAN frames.
  chan.submit_read(read);

  ctx.insert(*this);
}

IoContext::Impl_::~Impl_() { ctx.remove(*this); }

void
IoContext::Impl_::OnShutdown() noexcept {
#if LELY_NO_THREADS
  shutdown = true;
#else
  shutdown.store(true, ::std::memory_order_release);
#endif
  timer.cancel(wait);
  chan.cancel_read(read);
}

void
IoContext::Impl_::OnWait(int, ::std::error_code ec) noexcept {
  if (!ec) {
    ::std::lock_guard<Impl_> lock(*this);
    self->SetTime();
  }
#if LELY_NO_THREADS
  if (!shutdown)
#else
  if (!shutdown.load(::std::memory_order_acquire))
#endif
    timer.submit_wait(wait);
}

void
IoContext::Impl_::OnRead(int result, ::std::error_code) noexcept {
  {
    ::std::lock_guard<Impl_> lock(*this);
    // Update the internal clock before processing the incoming CAN (error)
    // frame.
    self->SetTime();
    if (result == 1) {
      net->recv(msg);
    } else if (!result) {
      if (err.state != state) {
        ::std::swap(err.state, state);
        self->OnCanState(static_cast<io::CanState>(state),
                         static_cast<io::CanState>(err.state));
      }
      if (err.error != error) {
        error = err.error;
        self->OnCanError(static_cast<io::CanError>(error));
      }
    }
  }
#if LELY_NO_THREADS
  if (!shutdown)
#else
  if (!shutdown.load(::std::memory_order_acquire))
#endif
    chan.submit_read(read);
}

int
IoContext::Impl_::OnNext(const timespec* tp) noexcept {
  timer.settime(io::TimerBase::time_point(util::from_timespec(*tp)));
  return 0;
}

int
IoContext::Impl_::OnSend(const can_msg* msg) noexcept {
  // The CAN network interface does not support asynchronous writes, so we try a
  // non-blocking synchronous write.
  ::std::error_code ec;
  chan.write(*msg, 0, ec);
  if (ec) {
    set_errc(ec.value());
    return -1;
  }
  return 0;
}

}  // namespace canopen

}  // namespace lely
