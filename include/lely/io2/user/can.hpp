/**@file
 * This header file is part of the I/O library; it contains the C++ interface
 * for the user-defined CAN channel.
 *
 * @see lely/io2/user/can.h
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

#ifndef LELY_IO2_USER_CAN_HPP_
#define LELY_IO2_USER_CAN_HPP_

#include <lely/io2/user/can.h>
#include <lely/io2/can.hpp>
#include <lely/util/chrono.hpp>

#include <utility>

namespace lely {
namespace io {

/// A user-defined CAN channel.
class UserCanChannel : public CanChannelBase {
 public:
  /// @see io_user_can_chan_create()
  UserCanChannel(io_ctx_t* ctx, ev_exec_t* exec,
                 CanBusFlag flags = CanBusFlag::NONE, size_t rxlen = 0,
                 int txtime = 0, io_user_can_chan_write_t* func = nullptr,
                 void* arg = nullptr)
      : CanChannelBase(io_user_can_chan_create(
            ctx, exec, static_cast<int>(flags), rxlen, txtime, func, arg)) {
    if (!chan) util::throw_errc("UserCanChannel");
  }

  UserCanChannel(const UserCanChannel&) = delete;

  UserCanChannel(UserCanChannel&& other) noexcept : CanChannelBase(other.chan) {
    other.chan = nullptr;
    other.dev = nullptr;
  }

  UserCanChannel& operator=(const UserCanChannel&) = delete;

  UserCanChannel&
  operator=(UserCanChannel&& other) noexcept {
    using ::std::swap;
    swap(chan, other.chan);
    swap(dev, other.dev);
    return *this;
  }

  /// @see io_user_can_chan_destroy()
  ~UserCanChannel() { io_user_can_chan_destroy(*this); }

  /// @see io_user_can_chan_on_msg()
  void
  on_read(const can_msg* msg, const ::std::chrono::nanoseconds& d, int timeout,
          ::std::error_code& ec) noexcept {
    auto ts = util::to_timespec(d);
    int errsv = get_errc();
    set_errc(0);
    if (!io_user_can_chan_on_msg(*this, msg, &ts, timeout))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_user_can_chan_on_msg()
  void
  on_read(const can_msg* msg, const ::std::chrono::nanoseconds& d,
          int timeout = -1) {
    ::std::error_code ec;
    on_read(msg, d, timeout, ec);
    if (ec) throw ::std::system_error(ec, "on_read");
  }

  /// @see io_user_can_chan_on_msg()
  void
  on_read(const can_msg* msg, int timeout, ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    if (!io_user_can_chan_on_msg(*this, msg, nullptr, timeout))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_user_can_chan_on_msg()
  void
  on_read(const can_msg* msg, int timeout = -1) {
    ::std::error_code ec;
    on_read(msg, timeout, ec);
    if (ec) throw ::std::system_error(ec, "on_read");
  }

  /// @see io_user_can_chan_on_err()
  void
  on_read(const can_err* err, const ::std::chrono::nanoseconds& d, int timeout,
          ::std::error_code& ec) noexcept {
    auto ts = util::to_timespec(d);
    int errsv = get_errc();
    set_errc(0);
    if (!io_user_can_chan_on_err(*this, err, &ts, timeout))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_user_can_chan_on_err()
  void
  on_read(const can_err* err, const ::std::chrono::nanoseconds& d,
          int timeout = -1) {
    ::std::error_code ec;
    on_read(err, d, timeout, ec);
    if (ec) throw ::std::system_error(ec, "on_read");
  }

  /// @see io_user_can_chan_on_err()
  void
  on_read(const can_err* err, int timeout, ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    if (!io_user_can_chan_on_err(*this, err, nullptr, timeout))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_user_can_chan_on_err()
  void
  on_read(const can_err* err, int timeout = -1) {
    ::std::error_code ec;
    on_read(err, timeout, ec);
    if (ec) throw ::std::system_error(ec, "on_read");
  }
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_USER_CAN_HPP_
