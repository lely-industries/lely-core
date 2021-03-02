/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the remote node driver containing an event loop.
 *
 * @see lely/coapp/loop_driver.hpp
 *
 * @copyright 2018-2021 Lely Industries N.V.
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

#if !LELY_NO_COAPP_MASTER && !LELY_NO_THREADS

#include <lely/coapp/loop_driver.hpp>

#include <atomic>
#ifdef __MINGW32__
#include <lely/libc/threads.h>
#else
#include <thread>
#endif

#include <cassert>

namespace lely {

namespace canopen {

/// The internal implementation of #lely::canopen::LoopDriver.
struct LoopDriver::Impl_ : io_svc {
  explicit Impl_(LoopDriver* self, io::ContextBase ctx);
  Impl_(const Impl_&) = delete;
  Impl_& operator=(const Impl_&) = delete;
  ~Impl_();

  void Start();
  void Shutdown();
  void Join();

  static const io_svc_vtbl svc_vtbl;

  LoopDriver* self{nullptr};
  io::ContextBase ctx{nullptr};
  ::std::atomic_flag shutdown{false};
  ev::Promise<void, void> stopped;
#ifdef __MINGW32__
  thrd_t thr;
#else
  ::std::thread thread;
#endif
  ::std::atomic_flag joined{false};
};

// clang-format off
const io_svc_vtbl LoopDriver::Impl_::svc_vtbl = {
    nullptr,
    [](io_svc* svc) noexcept {
      static_cast<LoopDriver::Impl_*>(svc)->Shutdown();
    }
};
// clang-format on

LoopDriver::LoopDriver(AsyncMaster& master, uint8_t id)
    : BasicDriver(strand.get_inner_executor(), master, id),
      impl_(new Impl_(this, master.GetContext())) {}

LoopDriver::~LoopDriver() = default;

void
LoopDriver::Join() {
  impl_->Join();
}

ev::Future<void, void>
LoopDriver::AsyncStoppped() noexcept {
  return impl_->stopped.get_future();
}

void
LoopDriver::Wait(SdoFuture<void> f, ::std::error_code& ec) {
  GetLoop().wait(f, ec);
  try {
    f.get().value();
  } catch (const ::std::system_error& e) {
    ec = e.code();
  } catch (const ev::future_not_ready& e) {
    ec = ::std::make_error_code(::std::errc::operation_canceled);
  }
}

void
LoopDriver::USleep(uint_least64_t usec) {
  ::std::error_code ec;
  USleep(usec, ec);
  if (ec) throw ::std::system_error(ec, "USleep");
}

void
LoopDriver::USleep(uint_least64_t usec, ::std::error_code& ec) {
  io_tqueue_wait* wait = nullptr;
  auto f = master.AsyncWait(::std::chrono::microseconds(usec), &wait);
  assert(wait);
  Wait(f, ec);
  if (!f.is_ready()) master.CancelWait(*wait);
}

LoopDriver::Impl_::Impl_(LoopDriver* self_, io::ContextBase ctx_)
    : io_svc IO_SVC_INIT(&svc_vtbl),
      self(self_),
      ctx(ctx_)
#ifndef __MINGW32__
      ,
      thread(&Impl_::Start, this)
#endif
{
#ifdef __MINGW32__
  if (thrd_create(
          &thr,
          [](void* arg) noexcept {
            static_cast<LoopDriver::Impl_*>(arg)->Start();
            return 0;
          },
          this) != thrd_success)
    util::throw_errc("thrd_create");
#endif
  ctx.insert(*this);
}

LoopDriver::Impl_::~Impl_() {
  Join();
  ctx.remove(*this);
}

void
LoopDriver::Impl_::Start() {
  auto& loop = self->GetLoop();
  auto exec = self->GetExecutor();

  // Start the event loop. Signal the existence of a fake task to prevent the
  // loop for stopping early.
  exec.on_task_init();
  loop.run();
  exec.on_task_fini();

  // Deregister the driver to prevent the master from queueing new events. This
  // also cancels any outstanding SDO requests.
  self->master.Erase(*self);

  // Finish remaining tasks, but do not block.
  loop.restart();
  loop.poll();

  // Satisfy the promise to signal that the thread is about to terminate and it
  // a call to the destructor will not block.
  stopped.set(0);
}

void
LoopDriver::Impl_::Shutdown() {
  if (!shutdown.test_and_set()) {
    // Stop receiving CANopen events.
    self->master.Erase(*self);
    // Stop the blocking run of the event loop.
    self->GetLoop().stop();
  }
}

void
LoopDriver::Impl_::Join() {
  if (!joined.test_and_set()) {
    Shutdown();
#ifdef __MINGW32__
    thrd_join(thr, nullptr);
#else
    thread.join();
#endif
  }
}

}  // namespace canopen

}  // namespace lely

#endif  // !LELY_NO_COAPP_MASTER && !LELY_NO_THREADS
