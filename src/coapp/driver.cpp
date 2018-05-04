/*!\file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the remote node driver interface.
 *
 * \see lely/coapp/driver.hpp
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

#if !LELY_NO_COAPP_MASTER

#include <lely/coapp/driver.hpp>

#if !LELY_NO_THREADS
#include <thread>
#endif

namespace lely {

namespace canopen {

#if !LELY_NO_THREADS
//! The internal implementation of #lely::canopen::LoopDriver.
struct LoopDriver::Impl_ {
  explicit Impl_(LoopDriver* self);
  ~Impl_();

  void Start();

  LoopDriver* self { nullptr };
  ::std::thread thread;
};
#endif

LELY_COAPP_EXPORT
BasicDriver::BasicDriver(aio::LoopBase& loop, aio::ExecutorBase& exec,
                         BasicMaster& master_, uint8_t id)
    : master(master_), loop_(loop), exec_(exec), id_(id) {
  master.Insert(*this);
}

LELY_COAPP_EXPORT
BasicDriver::~BasicDriver() { master.Erase(*this); }

LELY_COAPP_EXPORT uint8_t
BasicDriver::netid() const noexcept { return master.netid(); }

#if !LELY_NO_THREADS

LELY_COAPP_EXPORT
LoopDriver::LoopDriver(BasicMaster& master, uint8_t id)
    : BasicDriver(loop, exec, master, id), impl_(new Impl_(this)) {}

LELY_COAPP_EXPORT LoopDriver::~LoopDriver() = default;

LoopDriver::Impl_::Impl_(LoopDriver* self_)
    : self(self_), thread(&Impl_::Start, this) {}

LoopDriver::Impl_::~Impl_() {
  self->GetLoop().Stop();
  thread.join();
}

void
LoopDriver::Impl_::Start() {
  auto loop = self->GetLoop();
  auto exec = self->GetExecutor();

  // Start the event loop. Signal the existance of a fake task to
  // prevent the loop for stopping early.
  exec.OnTaskStarted();
  loop.Run();
  exec.OnTaskFinished();

  // Finish remaining tasks, but do not block.
  loop.Restart();
  loop.RunFor();
}

#endif  // !LELY_NO_THREADS

}  // namespace canopen

}  // namespace lely

#endif  // !LELY_NO_COAPP_MASTER
