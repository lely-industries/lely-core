/**@file
 * This header file is part of the C++ asynchronous I/O library; it contains ...
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

#ifndef LELY_AIO_REACTOR_HPP_
#define LELY_AIO_REACTOR_HPP_

#include <lely/aio/poll.hpp>

#include <lely/aio/reactor.h>

#include <system_error>
#include <utility>

namespace lely {

namespace aio {

class ReactorBase : public detail::CBase<aio_reactor_t> {
 public:
  aio_context_t* GetContext() const noexcept;

  PollBase GetPoll() const noexcept;

#if LELY_AIO_WITH_IOCP
  void Watch(aio_watch& watch, aio_handle_t handle);
  void Watch(aio_watch& watch, aio_handle_t handle, ::std::error_code& ec);
#else
  void Watch(aio_watch& watch, aio_handle_t handle, int events);
  void Watch(aio_watch& watch, aio_handle_t handle, int events,
             ::std::error_code& ec);
#endif

 protected:
  using CBase::CBase;
};

class Reactor : public ReactorBase {
 public:
  explicit Reactor(ContextBase& ctx);

  Reactor(const Reactor&) = delete;

  Reactor(Reactor&& other) : ReactorBase(other.c_ptr) { other.c_ptr = nullptr; }

  Reactor& operator=(const Reactor&) = delete;

  Reactor&
  operator=(Reactor&& other) {
    ::std::swap(c_ptr, other.c_ptr);
    return *this;
  }

  ~Reactor();
};

}  // namespace aio

}  // namespace lely

#endif  // LELY_AIO_REACTOR_HPP_
