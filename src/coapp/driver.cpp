/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the remote node driver interface.
 *
 * @see lely/coapp/driver.hpp
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

#if !LELY_NO_COAPP_MASTER

#include <lely/coapp/driver.hpp>

#if !LELY_NO_THREADS
#ifdef __MINGW32__
#include <lely/libc/threads.h>
#else
#include <thread>
#endif
#endif

namespace lely {

namespace canopen {

#if !LELY_NO_THREADS

/// The internal implementation of #lely::canopen::LoopDriver.
struct LoopDriver::Impl_ : public io_svc {
  explicit Impl_(LoopDriver* self, io::ContextBase ctx);
  ~Impl_();

  void Start();
  void Stop() noexcept;

  static const io_svc_vtbl svc_vtbl;

  LoopDriver* self{nullptr};
  io::ContextBase ctx{nullptr};
#ifdef __MINGW32__
  thrd_t thr;
#else
  ::std::thread thread;
#endif
};

// clang-format on
const io_svc_vtbl LoopDriver::Impl_::svc_vtbl = {
    nullptr,
    [](io_svc* svc) noexcept {
      static_cast<LoopDriver::Impl_*>(svc)->Stop();
    }
};
// clang-format on

#endif  // !LELY_NO_THREADS

BasicDriver::BasicDriver(ev_exec_t* exec, BasicMaster& master_, uint8_t id)
    : master(master_), exec_(exec), id_(id) {
  master.Insert(*this);
}

BasicDriver::~BasicDriver() { master.Erase(*this); }

uint8_t
BasicDriver::netid() const noexcept {
  return master.netid();
}

#if !LELY_NO_THREADS

LoopDriver::LoopDriver(BasicMaster& master, uint8_t id)
    : BasicDriver(strand.get_inner_executor(), master, id),
      impl_(new Impl_(this, master.GetContext())) {}

LoopDriver::~LoopDriver() = default;

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
  if (thrd_create(&thr,
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
  Stop();
#ifdef __MINGW32__
  thrd_join(thr, nullptr);
#else
  thread.join();
#endif
  ctx.remove(*this);
}

void
LoopDriver::Impl_::Start() {
  auto& loop = self->GetLoop();
  auto exec = self->GetExecutor();

  // Start the event loop. Signal the existence of a fake task to
  // prevent the loop for stopping early.
  exec.on_task_init();
  loop.run();
  exec.on_task_fini();

  // Deregister the driver to prevent the master from queueing new events. This
  // also cancels any outstanding SDO requests.
  self->master.Erase(*self);

  // Finish remaining tasks, but do not block.
  loop.restart();
  loop.poll();
}

void
LoopDriver::Impl_::Stop() noexcept {
  self->GetLoop().stop();
}

#endif  // !LELY_NO_THREADS

}  // namespace canopen

}  // namespace lely

#endif  // !LELY_NO_COAPP_MASTER
