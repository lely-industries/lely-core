/*!\file
 * This file is part of the C++ asynchronous I/O library; it contains ...
 *
 * \see lely/aio/can_bus.hpp
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

#include <lely/aio/can_bus.hpp>
#include <lely/aio/loop.hpp>
#include <lely/aio/reactor.hpp>

namespace lely {

namespace aio {

LELY_AIO_EXPORT void
CanBusBase::ReadOperation::Func_(aio_task* task) noexcept {
  auto op = structof(task, aio_can_bus_read_op, task);
  auto self = static_cast<ReadOperation*>(op);
  if (self->func_) {
    auto ec = ::std::error_code(task->errc, ::std::system_category());
    self->func_(ec, op->result);
  }
}

LELY_AIO_EXPORT void
CanBusBase::WriteOperation::Func_(aio_task* task) noexcept {
  auto op = structof(task, aio_can_bus_write_op, task);
  auto self = static_cast<WriteOperation*>(op);
  if (self->func_) {
    auto ec = ::std::error_code(task->errc, ::std::system_category());
    self->func_(ec, op->result);
  }
}

LELY_AIO_EXPORT void
CanBusBase::ReadOperationWrapper::Func_(aio_task* task) noexcept {
  auto op = structof(task, aio_can_bus_read_op, task);
  auto self = static_cast<ReadOperationWrapper*>(op);
  auto ec = ::std::error_code(task->errc, ::std::system_category());
  auto result = op->result;
  ::std::function<ReadSignature> func(::std::move(self->func_));
  delete self;
  if (func)
    func(ec, result);
}

LELY_AIO_EXPORT void
CanBusBase::WriteOperationWrapper::Func_(aio_task* task) noexcept {
  auto op = structof(task, aio_can_bus_write_op, task);
  auto self = static_cast<WriteOperationWrapper*>(op);
  auto ec = ::std::error_code(task->errc, ::std::system_category());
  auto result = op->result;
  ::std::function<WriteSignature> func(::std::move(self->func_));
  delete self;
  if (func)
    func(ec, result);
}

LELY_AIO_EXPORT ExecutorBase
CanBusBase::GetExecutor() const noexcept { return aio_can_bus_get_exec(*this); }

LELY_AIO_EXPORT int
CanBusBase::Read(Frame* msg, Info* info) {
  return InvokeC("Read", aio_can_bus_read, *this, msg, info);
}

LELY_AIO_EXPORT int
CanBusBase::Read(Frame* msg, Info* info, ::std::error_code& ec) {
  return InvokeC(ec, aio_can_bus_read, *this, msg, info);
}

LELY_AIO_EXPORT void
CanBusBase::SubmitRead(aio_can_bus_read_op& op) {
  if (aio_can_bus_submit_read(*this, &op) == -1)
    throw_errc("SubmitRead");
}

LELY_AIO_EXPORT ::std::size_t
CanBusBase::CancelRead() { return aio_can_bus_cancel_read(*this, nullptr); }

LELY_AIO_EXPORT ::std::size_t
CanBusBase::CancelRead(aio_can_bus_read_op& op) {
  return aio_can_bus_cancel_read(*this, &op);
}

LELY_AIO_EXPORT int
CanBusBase::Write(const Frame& msg) {
  return InvokeC("Write", aio_can_bus_write, *this, &msg);
}

LELY_AIO_EXPORT int
CanBusBase::Write(const Frame& msg, ::std::error_code& ec) {
  return InvokeC(ec, aio_can_bus_write, *this, &msg);
}

LELY_AIO_EXPORT void
CanBusBase::SubmitWrite(aio_can_bus_write_op& op) {
  if (aio_can_bus_submit_write(*this, &op) == -1)
    throw_errc("SubmitWrite");
}

LELY_AIO_EXPORT ::std::size_t
CanBusBase::CancelWrite() { return aio_can_bus_cancel_write(*this, nullptr); }

LELY_AIO_EXPORT ::std::size_t
CanBusBase::CancelWrite(aio_can_bus_write_op& op) {
  return aio_can_bus_cancel_write(*this, &op);
}

LELY_AIO_EXPORT ::std::size_t
CanBusBase::Cancel() { return aio_can_bus_cancel(*this); }

LELY_AIO_EXPORT Future<aio_can_bus_read_op>
CanBusBase::AsyncRead(LoopBase& loop, Frame* msg, Info* info,
                      aio_can_bus_read_op** pop) {
  return Future<aio_can_bus_read_op>(InvokeC("AsyncRead",
      aio_can_bus_async_read, *this, loop, msg, info, pop));
}

LELY_AIO_EXPORT int
CanBusBase::RunRead(LoopBase& loop, Frame* msg, Info* info) {
  return InvokeC("RunRead", aio_can_bus_run_read, *this, loop, msg, info);
}

LELY_AIO_EXPORT int
CanBusBase::RunRead(LoopBase& loop, Frame* msg, Info* info,
                    ::std::error_code& ec) {
  return InvokeC(ec, aio_can_bus_run_read, *this, loop, msg, info);
}

LELY_AIO_EXPORT Future<aio_can_bus_write_op>
CanBusBase::AsyncWrite(LoopBase& loop, const Frame* msg,
                       aio_can_bus_write_op** pop) {
  return Future<aio_can_bus_write_op>(InvokeC("AsyncWrite",
      aio_can_bus_async_write, *this, loop, msg, pop));
}

LELY_AIO_EXPORT int
CanBusBase::RunWrite(LoopBase& loop, const Frame& msg) {
  return InvokeC("RunWrite", aio_can_bus_run_write, *this, loop, &msg);
}

LELY_AIO_EXPORT int
CanBusBase::RunWrite(LoopBase& loop, const Frame& msg, ::std::error_code& ec) {
  return InvokeC(ec, aio_can_bus_run_write, *this, loop, &msg);
}

LELY_AIO_EXPORT int
CanBusBase::RunReadUntil(LoopBase& loop, Frame* msg, Info* info,
                         const timespec* tp) {
  return InvokeC("RunReadUntil", aio_can_bus_run_read_until, *this, loop, msg,
                 info, tp);
}

LELY_AIO_EXPORT int
CanBusBase::RunReadUntil(LoopBase& loop, Frame* msg, Info* info,
                         const timespec* tp, ::std::error_code& ec) {
  return InvokeC(ec, aio_can_bus_run_read_until, *this, loop, msg, info, tp);
}

LELY_AIO_EXPORT int
CanBusBase::RunWriteUntil(LoopBase& loop, const Frame& msg,
                          const timespec* tp) {
  return InvokeC("RunWriteUntil", aio_can_bus_run_write_until, *this, loop,
                 &msg, tp);
}

LELY_AIO_EXPORT int
CanBusBase::RunWriteUntil(LoopBase& loop, const Frame& msg, const timespec* tp,
                          ::std::error_code& ec) {
  return InvokeC(ec, aio_can_bus_run_write_until, *this, loop, &msg, tp);
}

#if LELY_AIO_WITH_CAN_BUS

template <class> struct CanBusOptionTraits;

template <>
struct CanBusOptionTraits<CanBus::FdFrames> {
  static constexpr int name() { return AIO_CAN_BUS_FD_FRAMES; }
};

template <>
struct CanBusOptionTraits<CanBus::ErrorFrames> {
  static constexpr int name() { return AIO_CAN_BUS_ERROR_FRAMES; }
};

LELY_AIO_EXPORT
CanBus::CanBus(ExecutorBase& exec, ReactorBase& reactor)
    : CanBusBase(InvokeC("CanBus", aio_can_bus_create, exec, reactor)) {}

LELY_AIO_EXPORT CanBus::~CanBus() { aio_can_bus_destroy(*this); }

LELY_AIO_EXPORT aio_handle_t
CanBus::GetHandle() const noexcept { return aio_can_bus_get_handle(*this); }

LELY_AIO_EXPORT void
CanBus::Open(const ::std::string& ifname) {
  ::std::error_code ec;
  Open(ifname, ec);
  if (ec)
    throw ::std::system_error(ec, "Open");
}

LELY_AIO_EXPORT void
CanBus::Open(const ::std::string& ifname, ::std::error_code& ec) {
  ec.clear();
  if (aio_can_bus_open(*this, ifname.c_str()) == -1)
    ec = ::std::error_code(get_errc(), ::std::system_category());
}

LELY_AIO_EXPORT void
CanBus::Assign(aio_handle_t handle) {
  ::std::error_code ec;
  Assign(handle, ec);
  if (ec)
    throw ::std::system_error(ec, "Assign");
}

LELY_AIO_EXPORT void
CanBus::Assign(aio_handle_t handle, ::std::error_code& ec) {
  ec.clear();
  if (aio_can_bus_assign(*this, handle) == -1)
    ec = ::std::error_code(get_errc(), ::std::system_category());
}

LELY_AIO_EXPORT aio_handle_t
CanBus::Release() {
  ::std::error_code ec;
  auto r = Release(ec);
  if (ec)
    throw ::std::system_error(ec, "Release");
  return r;
}

LELY_AIO_EXPORT aio_handle_t
CanBus::Release(::std::error_code& ec) {
  ec.clear();
  auto handle = aio_can_bus_release(*this);
  if (handle == AIO_INVALID_HANDLE)
    ec = ::std::error_code(get_errc(), ::std::system_category());
  return handle;
}

LELY_AIO_EXPORT bool
CanBus::IsOpen() const noexcept { return !!aio_can_bus_is_open(*this); }

LELY_AIO_EXPORT void
CanBus::Close() {
  ::std::error_code ec;
  Close(ec);
  if (ec)
    throw ::std::system_error(ec, "Close");
}

LELY_AIO_EXPORT void
CanBus::Close(::std::error_code& ec) {
  ec.clear();
  if (aio_can_bus_close(*this) == -1)
    ec = ::std::error_code(get_errc(), ::std::system_category());
}

template <class GettableCanBusOption>
LELY_AIO_EXPORT void
CanBus::GetOption(GettableCanBusOption& option) const {
  ::std::error_code ec;
  GetOption(option, ec);
  if (__unlikely(ec))
    throw ::std::system_error(ec, "GetOption");
}

template <class GettableCanBusOption>
LELY_AIO_EXPORT void
CanBus::GetOption(GettableCanBusOption& option, ::std::error_code& ec) const {
  ec.clear();
  auto name = CanBusOptionTraits<GettableCanBusOption>::name();
  auto len = sizeof(option.value_);
  if (aio_can_bus_get_option(*this, name, &option.value_, &len) == -1)
    ec = ::std::error_code(get_errc(), ::std::system_category());
}

template <class SettableCanBusOption>
LELY_AIO_EXPORT void
CanBus::SetOption(const SettableCanBusOption& option) {
  ::std::error_code ec;
  SetOption(option, ec);
  if (__unlikely(ec))
    throw ::std::system_error(ec, "SetOption");
}

template <class SettableCanBusOption>
LELY_AIO_EXPORT void
CanBus::SetOption(const SettableCanBusOption& option, ::std::error_code& ec) {
  ec.clear();
  auto name = CanBusOptionTraits<SettableCanBusOption>::name();
  auto len = sizeof(option.value_);
  if (aio_can_bus_set_option(*this, name, &option.value_, len) == -1)
    ec = ::std::error_code(get_errc(), ::std::system_category());
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

template LELY_AIO_EXPORT void CanBus::GetOption(FdFrames&) const;
template LELY_AIO_EXPORT void
CanBus::GetOption(FdFrames&, ::std::error_code&) const;

template LELY_AIO_EXPORT void CanBus::SetOption(const FdFrames&);
template LELY_AIO_EXPORT void
CanBus::SetOption(const FdFrames&, ::std::error_code&);

template LELY_AIO_EXPORT void CanBus::GetOption(ErrorFrames&) const;
template LELY_AIO_EXPORT void
CanBus::GetOption(ErrorFrames&, ::std::error_code&) const;

template LELY_AIO_EXPORT void CanBus::SetOption(const ErrorFrames&);
template LELY_AIO_EXPORT void
CanBus::SetOption(const ErrorFrames&, ::std::error_code&);

#endif  // !DOXYGEN_SHOULD_SKIP_THIS

#endif  // LELY_AIO_WITH_CAN_BUS

}  // namespace aio

}  // namespace lely

#endif  // !LELY_NO_CXX
