/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the remote node driver which runs its tasks and callbacks
 * in fibers.
 *
 * @see lely/coapp/fiber_driver.hpp
 *
 * @copyright 2019 Lely Industries N.V.
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

namespace lely {

namespace canopen {

namespace detail {

FiberDriverBase::FiberDriverBase(ev_exec_t* exec_)
#if !_WIN32 && _POSIX_MAPPED_FILES
    : thrd(ev::FiberFlag::GUARD_STACK),
#else
    : thrd(),
#endif
      exec(exec_),
      strand(exec) {
}

}  // namespace detail

FiberDriver::FiberDriver(ev_exec_t* exec, BasicMaster& master, uint8_t id)
    : FiberDriverBase(exec), BasicDriver(FiberDriverBase::exec, master, id) {}

void
FiberDriver::Wait(ev::Future<void, ::std::exception_ptr> f,
                  ::std::error_code& ec) {
  fiber_await(f);
  if (!f.is_ready()) {
    ec = ::std::make_error_code(::std::errc::operation_canceled);
    return;
  }
  auto& result = f.get();
  if (result.has_value()) {
    ec.clear();
  } else {
    try {
      ::std::rethrow_exception(result.error());
    } catch (const ::std::system_error& e) {
      ec = e.code();
    }
  }
}

}  // namespace canopen

}  // namespace lely

#endif  // !LELY_NO_COAPP_MASTER
