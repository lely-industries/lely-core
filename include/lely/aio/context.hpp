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

#ifndef LELY_AIO_CONTEXT_HPP_
#define LELY_AIO_CONTEXT_HPP_

#include <lely/aio/detail/cbase.hpp>

#include <lely/aio/context.h>

#include <utility>
#include <system_error>

namespace lely {

namespace aio {

enum class ForkEvent { PREPARE, PARENT, CHILD };

class ServiceBase;

class ContextBase : public detail::CBase<aio_context_t> {
 public:
  using CBase::CBase;

  void Insert(ServiceBase& srv);

  void Remove(ServiceBase& srv);

  void NotifyFork(ForkEvent e);

  void NotifyFork(ForkEvent e, ::std::error_code& ec);

  void Shutdown() noexcept;
};

class Context : public ContextBase {
 public:
  Context();

  Context(const Context&) = delete;

  Context(Context&& other) : ContextBase(other.c_ptr) { other.c_ptr = nullptr; }

  Context& operator=(const Context&) = delete;

  Context&
  operator=(Context&& other) {
    ::std::swap(c_ptr, other.c_ptr);
    return *this;
  }

  ~Context();
};

class ServiceBase : public detail::CBase<aio_service> {
 public:
  using CBase::CBase;
};

class BasicService : private aio_service, public ServiceBase {
 public:
  explicit BasicService(const ContextBase& ctx);

  BasicService(const BasicService&) = delete;
  BasicService& operator=(const BasicService&) = delete;

 protected:
  ~BasicService();

  virtual void
  NotifyFork(ForkEvent e) {
    (void)e;
  }

  virtual void Shutdown() noexcept {};

 private:
  static const aio_service_vtbl* Vptr_() noexcept;

  static int NotifyFork_(aio_service* srv, aio_fork_event e) noexcept;

  static void Shutdown_(aio_service* srv) noexcept;

  aio_context_t* ctx_;
};

}  // namespace aio

}  // namespace lely

#endif  // LELY_AIO_CONTEXT_HPP_
