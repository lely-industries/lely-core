/**@file
 * This header file is part of the  I/O library; it contains the C++ interface
 * for the IXXAT CAN bus interface for Windows.
 *
 * @see lely/io2/win32/ixxat.h
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

#ifndef LELY_IO2_WIN32_IXXAT_HPP_
#define LELY_IO2_WIN32_IXXAT_HPP_

#include <lely/io2/win32/ixxat.h>
#include <lely/io2/can.hpp>

#include <utility>

#include <windows.h>

namespace lely {
namespace io {

/// A RAII-style wrapper around io_ixxat_init() and io_ixxat_fini().
class IxxatGuard {
 public:
  IxxatGuard() {
    if (io_ixxat_init() == -1) util::throw_errc("IxxatGuard");
  }

  IxxatGuard(const IxxatGuard&) = delete;
  IxxatGuard& operator=(const IxxatGuard&) = delete;

  ~IxxatGuard() noexcept { io_ixxat_fini(); }
};

/// An IXXAT CAN controller.
class IxxatController : public CanControllerBase {
 public:
  /// @see io_ixxat_ctrl_create_from_index()
  IxxatController(UINT32 dwIndex, UINT32 dwCanNo, CanBusFlag flags, int nominal,
                  int data = 0)
      : CanControllerBase(io_ixxat_ctrl_create_from_index(
            dwIndex, dwCanNo, static_cast<int>(flags), nominal, data)) {
    if (!ctrl) util::throw_errc("IxxatController");
  }

  /// @see io_ixxat_ctrl_create_from_luid()
  IxxatController(const LUID& rLuid, UINT32 dwCanNo, CanBusFlag flags,
                  int nominal, int data = 0)
      : CanControllerBase(io_ixxat_ctrl_create_from_luid(
            &rLuid, dwCanNo, static_cast<int>(flags), nominal, data)) {
    if (!ctrl) util::throw_errc("IxxatController");
  }

  IxxatController(const IxxatController&) = delete;

  IxxatController(IxxatController&& other) noexcept
      : CanControllerBase(other.ctrl) {
    other.ctrl = nullptr;
  }

  IxxatController& operator=(const IxxatController&) = delete;

  IxxatController&
  operator=(IxxatController&& other) noexcept {
    using ::std::swap;
    swap(ctrl, other.ctrl);
    return *this;
  }

  /// @see io_ixxat_ctrl_destroy()
  ~IxxatController() { io_ixxat_ctrl_destroy(*this); }

  /// @see io_ixxat_ctrl_get_handle()
  HANDLE
  get_handle() const noexcept { return io_ixxat_ctrl_get_handle(*this); }
};

/// An IXXAT CAN channel.
class IxxatChannel : public CanChannelBase {
 public:
  /// @see io_ixxat_chan_create()
  IxxatChannel(io_ctx_t* ctx, ev_exec_t* exec, int rxtimeo = 0, int txtimeo = 0)
      : CanChannelBase(io_ixxat_chan_create(ctx, exec, rxtimeo, txtimeo)) {
    if (!chan) util::throw_errc("IxxatChannel");
  }

  IxxatChannel(const IxxatChannel&) = delete;

  IxxatChannel(IxxatChannel&& other) noexcept : CanChannelBase(other.chan) {
    other.chan = nullptr;
    other.dev = nullptr;
  }

  IxxatChannel& operator=(const IxxatChannel&) = delete;

  IxxatChannel&
  operator=(IxxatChannel&& other) noexcept {
    using ::std::swap;
    swap(chan, other.chan);
    swap(dev, other.dev);
    return *this;
  }

  /// @see io_ixxat_chan_destroy()
  ~IxxatChannel() { io_ixxat_chan_destroy(*this); }

  /// @see io_ixxat_chan_get_handle()
  HANDLE
  get_handle() const noexcept { return io_ixxat_chan_get_handle(*this); }

  /// @see io_ixxat_chan_open()
  void
  open(const io_can_ctrl_t* ctrl, UINT16 wRxFifoSize, UINT16 wTxFifoSize,
       ::std::error_code& ec) noexcept {
    DWORD dwErrCode = GetLastError();
    SetLastError(0);
    if (!io_ixxat_chan_open(*this, ctrl, wRxFifoSize, wTxFifoSize))
      ec.clear();
    else
      ec = util::make_error_code();
    SetLastError(dwErrCode);
  }

  /// @see io_ixxat_chan_open()
  void
  open(const io_can_ctrl_t* ctrl, UINT16 wRxFifoSize = 0,
       UINT16 wTxFifoSize = 0) {
    ::std::error_code ec;
    open(ctrl, wRxFifoSize, wTxFifoSize, ec);
    if (ec) throw ::std::system_error(ec, "open");
  }

  /// @see io_ixxat_chan_assign()
  void
  assign(HANDLE hCanChn, UINT32 dwTscClkFreq, UINT32 dwTscDivisor,
         ::std::error_code& ec) noexcept {
    DWORD dwErrCode = GetLastError();
    SetLastError(0);
    if (!io_ixxat_chan_assign(*this, hCanChn, dwTscClkFreq, dwTscDivisor))
      ec.clear();
    else
      ec = util::make_error_code();
    SetLastError(dwErrCode);
  }

  /// @see io_ixxat_chan_assign()
  void
  assign(HANDLE hCanChn, UINT32 dwTscClkFreq = 0, UINT32 dwTscDivisor = 0) {
    ::std::error_code ec;
    assign(hCanChn, dwTscClkFreq, dwTscDivisor, ec);
    if (ec) throw ::std::system_error(ec, "assign");
  }

  /// @see io_ixxat_chan_release()
  HANDLE
  release() noexcept { return io_ixxat_chan_release(*this); }

  /// @see io_ixxat_chan_is_open()
  bool
  is_open() const noexcept {
    return io_ixxat_chan_is_open(*this) != 0;
  }

  /// @see io_ixxat_chan_close()
  void
  close(::std::error_code& ec) noexcept {
    DWORD dwErrCode = GetLastError();
    SetLastError(0);
    if (!io_ixxat_chan_close(*this))
      ec.clear();
    else
      ec = util::make_error_code();
    SetLastError(dwErrCode);
  }

  /// @see io_ixxat_chan_close()
  void
  close() {
    ::std::error_code ec;
    close(ec);
    if (ec) throw ::std::system_error(ec, "close");
  }
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_WIN32_IXXAT_HPP_
