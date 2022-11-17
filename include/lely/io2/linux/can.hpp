/**@file
 * This header file is part of the  I/O library; it contains the C++ interface
 * for the CAN bus interface for Linux.
 *
 * @see lely/io2/linux/can.h
 *
 * @copyright 2018-2022 Lely Industries N.V.
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

#ifndef LELY_IO2_LINUX_CAN_HPP_
#define LELY_IO2_LINUX_CAN_HPP_

#include <lely/io2/linux/can.h>
#include <lely/io2/can.hpp>

#include <utility>

namespace lely {
namespace io {

/// A CAN controller.
class CanController : public CanControllerBase {
 public:
  /// @see io_can_ctrl_create_from_name()
  CanController(const char* name, ::std::size_t txlen = 0)
      : CanControllerBase(io_can_ctrl_create_from_name(name, txlen)) {
    if (!ctrl) util::throw_errc("CanController");
  }

  /// @see io_can_ctrl_create_from_index()
  CanController(unsigned int index, ::std::size_t txlen = 0)
      : CanControllerBase(io_can_ctrl_create_from_index(index, txlen)) {
    if (!ctrl) util::throw_errc("CanController");
  }

  CanController(const CanController&) = delete;

  CanController(CanController&& other) noexcept
      : CanControllerBase(other.ctrl) {
    other.ctrl = nullptr;
  }

  CanController& operator=(const CanController&) = delete;

  CanController&
  operator=(CanController&& other) noexcept {
    using ::std::swap;
    swap(ctrl, other.ctrl);
    return *this;
  }

  /// @see io_can_ctrl_destroy()
  ~CanController() { io_can_ctrl_destroy(*this); }

  // @see io_can_ctrl_get_name()
  const char*
  get_name() const noexcept {
    return io_can_ctrl_get_name(*this);
  }

  // @see io_can_ctrl_get_index()
  unsigned int
  get_index() const noexcept {
    return io_can_ctrl_get_index(*this);
  }

  // @see io_can_ctrl_get_flags
  CanBusFlag
  get_flags() const noexcept {
    return static_cast<CanBusFlag>(io_can_ctrl_get_flags(*this));
  }
};

/// A CAN channel.
class CanChannel : public CanChannelBase {
 public:
  /// @see io_can_chan_create()
  CanChannel(io_poll_t* poll, ev_exec_t* exec, ::std::size_t rxlen = 0,
             bool txwait = true)
      : CanChannelBase(io_can_chan_create(poll, exec, rxlen, txwait)) {
    if (!chan) util::throw_errc("CanChannel");
  }

  CanChannel(const CanChannel&) = delete;

  CanChannel(CanChannel&& other) noexcept : CanChannelBase(other.chan) {
    other.chan = nullptr;
    other.dev = nullptr;
  }

  CanChannel& operator=(const CanChannel&) = delete;

  CanChannel&
  operator=(CanChannel&& other) noexcept {
    using ::std::swap;
    swap(chan, other.chan);
    swap(dev, other.dev);
    return *this;
  }

  /// @see io_can_chan_destroy()
  ~CanChannel() { io_can_chan_destroy(*this); }

  /// @see io_can_chan_get_handle()
  int
  get_handle() const noexcept {
    return io_can_chan_get_handle(*this);
  }

  /// @see io_can_chan_open()
  void
  open(const io_can_ctrl_t* ctrl, CanBusFlag flags,
       ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    if (!io_can_chan_open(*this, ctrl, static_cast<int>(flags)))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_can_chan_open()
  void
  open(const io_can_ctrl_t* ctrl, CanBusFlag flags = CanBusFlag::NONE) {
    ::std::error_code ec;
    open(ctrl, flags, ec);
    if (ec) throw ::std::system_error(ec, "open");
  }

  /// @see io_can_chan_assign()
  void
  assign(int fd, ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    if (!io_can_chan_assign(*this, fd))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_can_chan_assign()
  void
  assign(int fd) {
    ::std::error_code ec;
    assign(fd, ec);
    if (ec) throw ::std::system_error(ec, "assign");
  }

  /// @see io_can_chan_release()
  int
  release() noexcept {
    return io_can_chan_release(*this);
  }

  /// @see io_can_chan_is_open()
  bool
  is_open() const noexcept {
    return io_can_chan_is_open(*this) != 0;
  }

  /// @see io_can_chan_close()
  void
  close(::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    if (!io_can_chan_close(*this))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_can_chan_close()
  void
  close() {
    ::std::error_code ec;
    close(ec);
    if (ec) throw ::std::system_error(ec, "close");
  }
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_LINUX_CAN_HPP_
