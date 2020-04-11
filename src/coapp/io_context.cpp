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
#include <lely/can/net.hpp>
#include <lely/coapp/io_context.hpp>
#include <lely/libc/stdlib.h>
#include <lely/util/diag.h>
#include <lely/util/time.h>
#include <lely/util/spscring.h>

#include <vector>
#include <utility>

#include <cassert>

#ifndef LELY_COAPP_IO_CONTEXT_TXLEN
/**
 * The default transmit queue length (in number of CAN frames) of the I/O
 * context.
 */
#define LELY_COAPP_IO_CONTEXT_TXLEN 1000
#endif

#ifndef LELY_COAPP_IO_CONTEXT_TXTIMEO
/**
 * The default timeout (in milliseconds) when waiting for a CAN frame write
 * confirmation.
 */
#define LELY_COAPP_IO_CONTEXT_TXTIMEO 100
#endif

namespace lely {

namespace canopen {

/// The internal implementation of the I/O context.
struct IoContext::Impl_ : util::BasicLockable, io_svc {
  Impl_(IoContext* self, io::TimerBase& timer, io::CanChannelBase& chan,
        util::BasicLockable* mutex);
  virtual ~Impl_();

  void*
  operator new(::std::size_t size) {
    // This is necessary because of the alignment requirements of spscring.
    return aligned_alloc(alignof(Impl_), size);
  }

  void
  operator delete(void* p) {
    aligned_free(p);
  }

  void
  lock() override {
    if (mutex) mutex->lock();
  }

  void
  unlock() override {
    if (mutex) mutex->unlock();
  }

  void OnShutdown() noexcept;

  void OnWaitNext(::std::error_code ec) noexcept;
  void OnWaitConfirm(::std::error_code ec) noexcept;

  void OnRead(int result, ::std::error_code ec) noexcept;
  void OnWrite(::std::error_code ec) noexcept;

  int OnNext(const timespec* tp) noexcept;
  int OnSend(const can_msg* msg) noexcept;

  bool Wait();
  void Write();

  static const io_svc_vtbl svc_vtbl;

  IoContext* self{nullptr};

  util::BasicLockable* mutex{nullptr};

  io::ContextBase ctx{nullptr};
  bool shutdown{false};

  io::TimerBase& timer;
  io::TimerQueue tq;
  io::TimerQueueWait wait_next;
  io::TimerQueueWait wait_confirm;

  io::CanChannelBase& chan;

  can_msg read_msg CAN_MSG_INIT;
  can_err read_err CAN_ERR_INIT;
  io::CanChannelRead read;
  ::std::error_code read_ec;
  ::std::size_t read_errcnt{0};

  int state{CAN_STATE_ACTIVE};
  ::std::function<void(io::CanState, io::CanState)> on_can_state;

  int error{0};
  ::std::function<void(io::CanError)> on_can_error;

  can_msg write_msg CAN_MSG_INIT;
  io::CanChannelWrite write;
  ::std::error_code write_ec;
  ::std::size_t write_errcnt{0};

  spscring tx_ring;
  ::std::vector<can_msg> tx_buf;
  ::std::size_t tx_errcnt{0};

  unique_c_ptr<CANNet> net;
  timespec next{0, 0};
};

// clang-format off
const io_svc_vtbl IoContext::Impl_::svc_vtbl = {
    nullptr,
    [](io_svc* svc) noexcept {
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

io::Clock
IoContext::GetClock() const noexcept {
  return impl_->timer.get_clock();
}

void
IoContext::SubmitWait(const time_point& t, io_tqueue_wait& wait) {
  impl_->tq.submit_wait(t, wait);
}

void
IoContext::SubmitWait(const duration& d, io_tqueue_wait& wait) {
  impl_->tq.submit_wait(d, wait);
}

ev::Future<void, ::std::exception_ptr>
IoContext::AsyncWait(ev_exec_t* exec, const time_point& t,
                     io_tqueue_wait** pwait) {
  if (!exec) exec = GetExecutor();
  return impl_->tq.async_wait(exec, t, pwait)
      .then(exec, [](ev::Future<void, int> f) {
        // Convert the error code into an exception pointer.
        int errc = f.get().error();
        if (errc) util::throw_errc("AsyncWait", errc);
      });
}

ev::Future<void, ::std::exception_ptr>
IoContext::AsyncWait(ev_exec_t* exec, const duration& d,
                     io_tqueue_wait** pwait) {
  if (!exec) exec = GetExecutor();
  return impl_->tq.async_wait(exec, d, pwait)
      .then(exec, [](ev::Future<void, int> f) {
        // Convert the error code into an exception pointer.
        int errc = f.get().error();
        if (errc) util::throw_errc("AsyncWait", errc);
      });
}

bool
IoContext::CancelWait(io_tqueue_wait& wait) noexcept {
  return impl_->tq.cancel_wait(wait);
}

bool
IoContext::AbortWait(io_tqueue_wait& wait) noexcept {
  return impl_->tq.abort_wait(wait);
}

void
IoContext::OnCanState(
    ::std::function<void(io::CanState, io::CanState)> on_can_state) {
  ::std::lock_guard<Impl_> lock(*impl_);
  impl_->on_can_state = on_can_state;
}

void
IoContext::OnCanError(::std::function<void(io::CanError)> on_can_error) {
  ::std::lock_guard<Impl_> lock(*impl_);
  impl_->on_can_error = on_can_error;
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
      mutex(mutex_),
      ctx(timer_.get_ctx()),
      timer(timer_),
      tq(timer),
      wait_next([this](::std::error_code ec) { OnWaitNext(ec); }),
      wait_confirm([this](::std::error_code ec) { OnWaitConfirm(ec); }),
      chan(chan_),
      read(&read_msg, &read_err, nullptr,
           [this](int result, ::std::error_code ec) { OnRead(result, ec); }),
      write(write_msg, nullptr, [this](::std::error_code ec) { OnWrite(ec); }),
      tx_buf(LELY_COAPP_IO_CONTEXT_TXLEN, CAN_MSG_INIT),
      net(make_unique_c<CANNet>()) {
  spscring_init(&tx_ring, LELY_COAPP_IO_CONTEXT_TXLEN);

  // Initialize the CAN network clock with the current time.
  net->setTime(util::to_timespec(timer.get_clock().gettime()));

  // Register the OnNext() member function as the function to be invoked when
  // the time at which the next timer triggers is updated.
  net->setNextFunc<Impl_, &Impl_::OnNext>(this);
  // Register the OnSend() member function as the function to be invoked when a
  // CAN frame needs to be sent.
  net->setSendFunc<Impl_, &Impl_::OnSend>(this);

  ctx.insert(*this);

  // Start waiting for CAN frames to be put into the transmit queue.
  Wait();

  // Submit a wait operation with a near-infinite timeout. Knowing that a wait
  // operation is always submitted simplifies the interaction between
  // OnWaitNext() and OnNext().
  tq.submit_wait(time_point::max(), wait_next);
  // Subit a similarly near-infinite write confirmation timeout wait operation
  // to simplify the interaction between Write() and OnWrite().
  tq.submit_wait(time_point::max(), wait_confirm);

  // Start receiving CAN frames.
  chan.submit_read(read);
}

IoContext::Impl_::~Impl_() { ctx.remove(*this); }

void
IoContext::Impl_::OnShutdown() noexcept {
  {
    ::std::lock_guard<Impl_> lock(*this);
    if (shutdown) return;
    shutdown = true;
  }
  spscring_c_abort_wait(&tx_ring);
}

void
IoContext::Impl_::OnWaitNext(::std::error_code) noexcept {
  time_point t = time_point::max();
  {
    ::std::lock_guard<Impl_> lock(*this);
    // Update the time of the CAN network interface.
    self->SetTime();
    if (shutdown) return;
    // If the next timeout is in the past, use a near-infinite timeout instead.
    // This prevents busy-waiting when there is no timer active.
    timespec now{0, 0};
    net->getTime(&now);
    if (timespec_cmp(&now, &next) < 0)
      t = time_point(util::from_timespec(next));
  }
  tq.submit_wait(t, wait_next);
}

void
IoContext::Impl_::OnWaitConfirm(::std::error_code) noexcept {
  // No confirmation message was received; cancel the ongoing write operation.
  chan.cancel_write(write);
}

void
IoContext::Impl_::OnRead(int result, ::std::error_code ec) noexcept {
  if (ec && ec != ::std::errc::operation_canceled) {
    if (ec != read_ec)
      // Only print a diagnostic for unique read errors.
      diag(DIAG_WARNING, ec.value(), "error reading CAN frame");
    read_ec = ec;
    read_errcnt++;
  } else if (!ec && read_ec) {
    diag(DIAG_INFO, 0, "CAN frame successfully read after %zu read error(s)",
         read_errcnt);
    read_ec = {};
    read_errcnt = 0;
  }

  if (result == 1) {
    ::std::lock_guard<Impl_> lock(*this);
    // Update the internal clock before processing the incoming CAN frame.
    self->SetTime();
    net->recv(read_msg);
  } else if (result == 0) {
    if (read_err.state != state) {
      if (static_cast<io::CanState>(state) == io::CanState::BUSOFF)
        // Cancel the ongoing write operation if we just recovered from bus off.
        chan.cancel_write(write);

      ::std::swap(read_err.state, state);
      switch (static_cast<io::CanState>(state)) {
        case io::CanState::ACTIVE:
          diag(DIAG_INFO, 0, "CAN bus is in the error active state");
          break;
        case io::CanState::PASSIVE:
          diag(DIAG_INFO, 0, "CAN bus is in the error passive state");
          break;
        case io::CanState::BUSOFF:
          diag(DIAG_WARNING, 0, "CAN bus is in the bus off state");
          break;
        case io::CanState::SLEEPING:
          diag(DIAG_INFO, 0, "CAN interface is in sleep mode");
          break;
        case io::CanState::STOPPED:
          diag(DIAG_WARNING, 0, "CAN interface is stopped");
          break;
      }

      // Invoke the registered callback, if any.
      ::std::function<void(io::CanState, io::CanState)> f;
      {
        ::std::lock_guard<Impl_> lock(*this);
        f = on_can_state;
      }
      if (f)
        f(static_cast<io::CanState>(state),
          static_cast<io::CanState>(read_err.state));
    }

    if (read_err.error != error) {
      error = read_err.error;
      if (error & CAN_ERROR_BIT)
        diag(DIAG_WARNING, 0, "single bit error detected on CAN bus");
      if (error & CAN_ERROR_STUFF)
        diag(DIAG_WARNING, 0, "bit stuffing error detected on CAN bus");
      if (error & CAN_ERROR_CRC)
        diag(DIAG_WARNING, 0, "CRC sequence error detected on CAN bus");
      if (error & CAN_ERROR_FORM)
        diag(DIAG_WARNING, 0, "form error detected on CAN bus");
      if (error & CAN_ERROR_ACK)
        diag(DIAG_WARNING, 0, "acknowledgment error detected on CAN bus");
      if (error & CAN_ERROR_OTHER)
        diag(DIAG_WARNING, 0, "one or more unknown errors detected on CAN bus");

      // Invoke the registered callback, if any.
      ::std::function<void(io::CanError)> f;
      {
        ::std::lock_guard<Impl_> lock(*this);
        f = on_can_error;
      }
      if (f) f(static_cast<io::CanError>(error));
    }
  }

  {
    ::std::lock_guard<Impl_> lock(*this);
    if (shutdown) return;
  }

  chan.submit_read(read);
}

void
IoContext::Impl_::OnWrite(::std::error_code ec) noexcept {
  if (ec) {
    if (ec != write_ec)
      // Only print a diagnostic for unique write errors.
      diag(DIAG_WARNING, ec.value(), "error writing CAN frame");
    write_ec = ec;
    write_errcnt++;
  } else if (write_ec) {
    diag(DIAG_INFO, 0,
         "CAN frame successfully written after %zu write error(s)",
         write_errcnt);
    write_ec = {};
    write_errcnt = 0;
  }

  // Stop the timeout after receiving a write confirmation (or write error).
  if (tq.abort_wait(wait_confirm))
    // Register a near-infinite timeout to simplify the interaction with
    // Write().
    tq.submit_wait(time_point::max(), wait_confirm);

  // Remove the frame from the transmit queue, unless the write operation was
  // canceled, in which we discard the entire queue.
  assert(spscring_c_capacity(&tx_ring) >= 1);
  size_t n = 1;
  if (ec == ::std::errc::operation_canceled) {
    n = spscring_c_capacity(&tx_ring);
    // Track the number of dropped frames. The first frame has already been
    // accounted for.
    write_errcnt += n - 1;
  }
  spscring_c_commit(&tx_ring, n);

  ::std::lock_guard<Impl_> lock(*this);
  if (shutdown) return;
  // Write the next frame, if available.
  if (!Wait()) Write();
}

int
IoContext::Impl_::OnNext(const timespec* tp) noexcept {
  assert(tp);

  // In case OnWaitNext() is currently running, store the time for the next
  // earliest timeout so OnWaitNext() can re-submit the wait operation.
  next = *tp;

  if (!shutdown) {
    // Re-submit the wait operation with the new timeout, but only if we can be
    // sure OnWaitNext() is not currently running.
    if (tq.abort_wait(wait_next))
      tq.submit_wait(time_point(util::from_timespec(*tp)), wait_next);
  }

  return 0;
}

int
IoContext::Impl_::OnSend(const can_msg* msg) noexcept {
  assert(msg);

  ::std::size_t n = 1;
  auto i = spscring_p_alloc(&tx_ring, &n);
  if (n) {
    tx_buf[i] = *msg;
    spscring_p_commit(&tx_ring, n);
    if (tx_errcnt) {
      diag(DIAG_INFO, 0,
           "CAN frame successfully queued, after dropping %zu frame(s)",
           tx_errcnt);
      tx_errcnt = 0;
    }
    return 0;
  } else {
    set_errnum(ERRNUM_AGAIN);
    if (!tx_errcnt)
      diag(DIAG_WARNING, get_errc(), "CAN transmit queue full; dropping frame");
    tx_errcnt++;
    return -1;
  }
}

bool
IoContext::Impl_::Wait() {
  // Wait for next frame to become available.
  if (spscring_c_submit_wait(&tx_ring, 1,
                             [](spscring*, void* arg) noexcept {
                               auto self = static_cast<Impl_*>(arg);
                               // A frame was just added to the transmit queue;
                               // try to send it.
                               if (!self->Wait()) self->Write();
                             },
                             this))
    return true;

  // Extract the frame from the transmit queue.
  ::std::size_t n = 1;
  auto i = spscring_c_alloc(&tx_ring, &n);
  assert(n == 1);
  write_msg = tx_buf[i];

  return false;
}

void
IoContext::Impl_::Write() {
  assert(spscring_c_capacity(&tx_ring) >= 1);

  // Send the frame.
  chan.submit_write(write);

  // Register a timeout for the write confirmation.
  if (tq.abort_wait(wait_confirm))
    tq.submit_wait(::std::chrono::milliseconds(LELY_COAPP_IO_CONTEXT_TXTIMEO),
                   wait_confirm);
}

}  // namespace canopen

}  // namespace lely
