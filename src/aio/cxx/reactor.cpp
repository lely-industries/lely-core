/**@file
 * This file is part of the C++ asynchronous I/O library; it contains ...
 *
 * @see lely/aio/reactor.hpp
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

#include "aio.hpp"

#if !LELY_NO_CXX

#include <lely/aio/context.hpp>
#include <lely/aio/reactor.hpp>

namespace lely {

namespace aio {

aio_context_t*
ReactorBase::GetContext() const noexcept {
  return aio_reactor_get_context(*this);
}

PollBase
ReactorBase::GetPoll() const noexcept {
  return aio_reactor_get_poll(*this);
}

#if LELY_AIO_WITH_IOCP

void
ReactorBase::Watch(aio_watch& watch, aio_handle_t handle) {
  ::std::error_code ec;
  Watch(watch, handle, ec);
  if (ec) throw ::std::system_error(ec, "Watch");
}

void
ReactorBase::Watch(aio_watch& watch, aio_handle_t handle,
                   ::std::error_code& ec) {
  ec.clear();
  if (aio_reactor_watch(*this, &watch, handle) == -1)
    ec = ::std::error_code(get_errc(), ::std::system_category());
}

#else

void
ReactorBase::Watch(aio_watch& watch, aio_handle_t handle, int events) {
  ::std::error_code ec;
  Watch(watch, handle, events, ec);
  if (ec) throw ::std::system_error(ec, "Watch");
}

void
ReactorBase::Watch(aio_watch& watch, aio_handle_t handle, int events,
                   ::std::error_code& ec) {
  ec.clear();
  if (aio_reactor_watch(*this, &watch, handle, events) == -1)
    ec = ::std::error_code(get_errc(), ::std::system_category());
}

#endif  // LELY_AIO_WITH_IOCP

#if LELY_AIO_WITH_REACTOR

Reactor::Reactor(ContextBase& ctx)
    : ReactorBase(InvokeC("Reactor", aio_reactor_create, ctx)) {}

Reactor::~Reactor() { aio_reactor_destroy(*this); }

#endif  // LELY_AIO_WITH_REACTOR

}  // namespace aio

}  // namespace lely

#endif  // !LELY_NO_CXX
