/*!\file
 * This file is part of the C++ asynchronous I/O library; it contains ...
 *
 * \see lely/aio/exec.hpp
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

#include "aio.hpp"

#if !LELY_NO_CXX

#include <lely/aio/context.hpp>

namespace lely {

namespace aio {

LELY_AIO_EXPORT void
ContextBase::Insert(ServiceBase& srv) { aio_context_insert(*this, srv); }

LELY_AIO_EXPORT void
ContextBase::Remove(ServiceBase& srv) { aio_context_remove(*this, srv); }

LELY_AIO_EXPORT void
ContextBase::NotifyFork(ForkEvent e) {
  ::std::error_code ec;
  NotifyFork(e, ec);
  if (ec)
    throw ::std::system_error(ec, "NotifyFork");
}

LELY_AIO_EXPORT void
ContextBase::NotifyFork(ForkEvent e, ::std::error_code& ec) {
  ec.clear();
  if (aio_context_notify_fork(*this, static_cast<aio_fork_event>(e)) == -1)
    ec = ::std::error_code(get_errc(), ::std::system_category());
}

LELY_AIO_EXPORT void
ContextBase::Shutdown() noexcept { aio_context_shutdown(*this); }

LELY_AIO_EXPORT Context::Context()
    : ContextBase(InvokeC("Context", aio_context_create)) {}

LELY_AIO_EXPORT Context::~Context() { aio_context_destroy(*this); }

LELY_AIO_EXPORT BasicService::BasicService(const ContextBase& ctx)
    : aio_service AIO_SERVICE_INIT(Vptr_()), ServiceBase(this), ctx_(ctx) {
  ContextBase(ctx_).Insert(*this);
}

LELY_AIO_EXPORT BasicService::~BasicService() {
  ContextBase(ctx_).Remove(*this);
}

const aio_service_vtbl*
BasicService::Vptr_() noexcept {
  static const aio_service_vtbl vtbl = {
    &BasicService::NotifyFork_,
    &BasicService::Shutdown_
  };
  return &vtbl;
}

LELY_AIO_EXPORT int
BasicService::NotifyFork_(aio_service* srv, aio_fork_event e) noexcept {
  auto self = static_cast<BasicService*>(srv);
  try {
    self->NotifyFork(static_cast<ForkEvent>(e));
  } catch (::std::system_error& e) {
    set_errc(e.code().value());
    return -1;
  } catch (...) {
    return 0;
  }
  return 0;
}

LELY_AIO_EXPORT void
BasicService::Shutdown_(aio_service* srv) noexcept {
  auto self = static_cast<BasicService*>(srv);
  self->Shutdown();
}

}  // namespace aio

}  // namespace lely

#endif  // !LELY_NO_CXX
