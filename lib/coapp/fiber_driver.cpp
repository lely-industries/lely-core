/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the remote node driver which runs its tasks and callbacks
 * in fibers.
 *
 * @see lely/coapp/fiber_driver.hpp
 *
 * @copyright 2019-2021 Lely Industries N.V.
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

#include <lely/coapp/fiber_driver.hpp>

#include <cassert>

namespace lely {

namespace canopen {

namespace detail {

FiberDriverBase::FiberDriverBase(ev_exec_t* exec_)
#if !_WIN32 && _POSIX_MAPPED_FILES
    : thrd(ev::FiberFlag::SAVE_ERROR | ev::FiberFlag::GUARD_STACK),
#else
    : thrd(ev::FiberFlag::SAVE_ERROR),
#endif
      exec(exec_),
      strand(exec) {
}

}  // namespace detail

FiberDriver::FiberDriver(ev_exec_t* exec, AsyncMaster& master, uint8_t id)
    : FiberDriverBase(exec ? exec
                           : static_cast<ev_exec_t*>(master.GetExecutor())),
      BasicDriver(FiberDriverBase::exec, master, id) {}

void
FiberDriver::Wait(SdoFuture<void> f, ::std::error_code& ec) {
  fiber_await(f);
  try {
    f.get().value();
  } catch (const ::std::system_error& e) {
    ec = e.code();
  } catch (const ev::future_not_ready& e) {
    ec = ::std::make_error_code(::std::errc::operation_canceled);
  }
}

void
FiberDriver::USleep(uint_least64_t usec) {
  ::std::error_code ec;
  USleep(usec, ec);
  if (ec) throw ::std::system_error(ec, "USleep");
}

void
FiberDriver::USleep(uint_least64_t usec, ::std::error_code& ec) {
  io_tqueue_wait* wait = nullptr;
  auto f = master.AsyncWait(::std::chrono::microseconds(usec), &wait);
  assert(wait);
  Wait(f, ec);
  if (!f.is_ready()) master.CancelWait(*wait);
}

}  // namespace canopen

}  // namespace lely

#endif  // !LELY_NO_COAPP_MASTER
