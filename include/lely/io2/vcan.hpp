/**@file
 * This header file is part of the I/O library; it contains the C++ interface
 * for the virtual CAN bus.
 *
 * @see lely/io2/vcan.h
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

#ifndef LELY_IO2_VCAN_HPP_
#define LELY_IO2_VCAN_HPP_

#include <lely/io2/can.hpp>
#include <lely/io2/clock.hpp>
#include <lely/io2/vcan.h>

#include <utility>

namespace lely {
namespace io {

/// A virtual CAN controller.
class VirtualCanController : public CanControllerBase {
 public:
  /// @see io_vcan_ctrl_create()
  VirtualCanController(io_clock_t* clock, CanBusFlag flags = CanBusFlag::MASK,
                       int nominal = 0, int data = 0,
                       CanState state = CanState::ACTIVE)
      : CanControllerBase(io_vcan_ctrl_create(clock, static_cast<int>(flags),
                                              nominal, data,
                                              static_cast<int>(state))) {
    if (!ctrl) util::throw_errc("VirtualCanController");
  }

  VirtualCanController(const VirtualCanController&) = delete;

  VirtualCanController(VirtualCanController&& other) noexcept
      : CanControllerBase(other.ctrl) {
    other.ctrl = nullptr;
  }

  VirtualCanController& operator=(const VirtualCanController&) = delete;

  VirtualCanController&
  operator=(VirtualCanController&& other) noexcept {
    using ::std::swap;
    swap(ctrl, other.ctrl);
    return *this;
  }

  /// @see io_vcan_ctrl_destroy()
  ~VirtualCanController() { io_vcan_ctrl_destroy(*this); }

  /// @see io_vcan_ctrl_set_state()
  void
  set_state(CanState state) {
    io_vcan_ctrl_set_state(*this, static_cast<int>(state));
  }

  /// @see io_vcan_ctrl_write_msg()
  void
  write(const can_msg& msg, int timeout, ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    if (!io_vcan_ctrl_write_msg(*this, &msg, timeout))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_vcan_ctrl_write_msg()
  void
  write(const can_msg& msg, int timeout = -1) {
    ::std::error_code ec;
    write(msg, timeout, ec);
    if (ec) throw ::std::system_error(ec, "write");
  }

  /// @see io_vcan_ctrl_write_err()
  void
  write(const can_err& err, int timeout, ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    if (!io_vcan_ctrl_write_err(*this, &err, timeout))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_vcan_ctrl_write_msg()
  void
  write(const can_err& err, int timeout = -1) {
    ::std::error_code ec;
    write(err, timeout, ec);
    if (ec) throw ::std::system_error(ec, "write");
  }
};

/// A virtual CAN channel.
class VirtualCanChannel : public CanChannelBase {
 public:
  /// @see io_vcan_chan_create()
  VirtualCanChannel(io_ctx_t* ctx, ev_exec_t* exec, size_t rxlen = 0)
      : CanChannelBase(io_vcan_chan_create(ctx, exec, rxlen)) {
    if (!chan) util::throw_errc("VirtualCanChannel");
  }

  VirtualCanChannel(const VirtualCanChannel&) = delete;

  VirtualCanChannel(VirtualCanChannel&& other) noexcept
      : CanChannelBase(other.chan) {
    other.chan = nullptr;
    other.dev = nullptr;
  }

  VirtualCanChannel& operator=(const VirtualCanChannel&) = delete;

  VirtualCanChannel&
  operator=(VirtualCanChannel&& other) noexcept {
    using ::std::swap;
    swap(chan, other.chan);
    swap(dev, other.dev);
    return *this;
  }

  /// @see io_vcan_chan_destroy()
  ~VirtualCanChannel() { io_vcan_chan_destroy(*this); }

  /// @see io_vcan_chan_get_ctrl()
  CanControllerBase
  get_ctrl() const noexcept {
    return CanControllerBase(io_vcan_chan_get_ctrl(*this));
  }

  /// @see io_vcan_chan_open()
  void
  open(const io_can_ctrl_t* ctrl) noexcept {
    io_vcan_chan_open(*this, ctrl);
  }

  /// @see io_vcan_chan_is_open()
  bool
  is_open() const noexcept {
    return io_vcan_chan_is_open(*this) != 0;
  }

  /// @see io_vcan_chan_close()
  void
  close() noexcept {
    io_vcan_chan_close(*this);
  }
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_VCAN_HPP_
