/**@file
 * This header file is part of the I/O library; it contains the C++ interface
 * for the abstract CAN bus.
 *
 * @see lely/io2/can.h
 *
 * @copyright 2018-2020 Lely Industries N.V.
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

#ifndef LELY_IO2_CAN_HPP_
#define LELY_IO2_CAN_HPP_

#include <lely/ev/future.hpp>
#include <lely/io2/can/err.hpp>
#include <lely/io2/can/msg.hpp>
#include <lely/io2/can.h>
#include <lely/io2/dev.hpp>
#include <lely/util/chrono.hpp>

#include <utility>

namespace lely {
namespace io {

/// The CAN bus flags.
enum class CanBusFlag : int {
  /// Reception of error frames is enabled.
  ERR = IO_CAN_BUS_FLAG_ERR,
#if !LELY_NO_CANFD
  /// FD Format (formerly Extended Data Length) support is enabled.
  FDF = IO_CAN_BUS_FLAG_FDF,
  /// Bit Rate Switch support is enabled.
  BRS = IO_CAN_BUS_FLAG_BRS,
#endif
  NONE = IO_CAN_BUS_FLAG_NONE,
  MASK = IO_CAN_BUS_FLAG_MASK
};

constexpr CanBusFlag
operator~(CanBusFlag rhs) {
  return static_cast<CanBusFlag>(~static_cast<int>(rhs));
}

constexpr CanBusFlag
operator&(CanBusFlag lhs, CanBusFlag rhs) {
  return static_cast<CanBusFlag>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

constexpr CanBusFlag
operator^(CanBusFlag lhs, CanBusFlag rhs) {
  return static_cast<CanBusFlag>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
}

constexpr CanBusFlag
operator|(CanBusFlag lhs, CanBusFlag rhs) {
  return static_cast<CanBusFlag>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline CanBusFlag&
operator&=(CanBusFlag& lhs, CanBusFlag rhs) {
  return lhs = lhs & rhs;
}

inline CanBusFlag&
operator^=(CanBusFlag& lhs, CanBusFlag rhs) {
  return lhs = lhs ^ rhs;
}

inline CanBusFlag&
operator|=(CanBusFlag& lhs, CanBusFlag rhs) {
  return lhs = lhs | rhs;
}

namespace detail {

template <class F>
class CanChannelReadWrapper : public io_can_chan_read {
 public:
  CanChannelReadWrapper(can_msg* msg, can_err* err,
                        ::std::chrono::nanoseconds* dp, ev_exec_t* exec, F&& f)
      : io_can_chan_read IO_CAN_CHAN_READ_INIT(
            msg, err, dp ? &ts_ : nullptr, exec,
            [](ev_task* task) noexcept {
              auto read = io_can_chan_read_from_task(task);
              auto result = read->r.result;
              ::std::error_code ec;
              if (result == -1) ec = util::make_error_code(read->r.errc);
              auto self = static_cast<CanChannelReadWrapper*>(read);
              if (self->dp_) *self->dp_ = util::from_timespec(self->ts_);
              compat::invoke(::std::move(self->func_), result, ec);
              delete self;
            }),
        dp_(dp),
        func_(::std::forward<F>(f)) {}

  CanChannelReadWrapper(const CanChannelReadWrapper&) = delete;

  CanChannelReadWrapper& operator=(const CanChannelReadWrapper&) = delete;

  operator ev_task&() & noexcept { return task; }

 private:
  timespec ts_{0, 0};
  ::std::chrono::nanoseconds* dp_{nullptr};
  typename ::std::decay<F>::type func_;
};

}  // namespace detail

/**
 * Creates a CAN channel read operation with a completion task. The operation
 * deletes itself after it is completed, so it MUST NOT be deleted once it is
 * submitted to a CAN channel.
 */
template <class F>
inline typename ::std::enable_if<
    compat::is_invocable<F, int, ::std::error_code>::value,
    detail::CanChannelReadWrapper<F>*>::type
make_can_channel_read_wrapper(can_msg* msg, can_err* err,
                              ::std::chrono::nanoseconds* dp, ev_exec_t* exec,
                              F&& f) {
  return new detail::CanChannelReadWrapper<F>(msg, err, dp, exec,
                                              ::std::forward<F>(f));
}

/**
 * A read operation suitable for use with a CAN channel. This class stores a
 * callable object with signature `void(int result, std::error_code ec)`,
 * which is invoked upon completion (or cancellation) of the read operation.
 */
class CanChannelRead : public io_can_chan_read {
 public:
  using Signature = void(int, ::std::error_code);

  /// Constructs a read operation with a completion task.
  template <class F>
  CanChannelRead(can_msg* msg, can_err* err, ::std::chrono::nanoseconds* dp,
                 ev_exec_t* exec, F&& f)
      : io_can_chan_read
        IO_CAN_CHAN_READ_INIT(msg, err, dp ? &ts_ : nullptr, exec,
                              [](ev_task* task) noexcept {
                                auto read = io_can_chan_read_from_task(task);
                                auto self = static_cast<CanChannelRead*>(read);
                                if (self->dp_)
                                  *self->dp_ = util::from_timespec(self->ts_);
                                if (self->func_) {
                                  auto result = read->r.result;
                                  ::std::error_code ec;
                                  if (result == -1)
                                    ec = util::make_error_code(read->r.errc);
                                  self->func_(result, ec);
                                }
                              }),
        dp_(dp),
        func_(::std::forward<F>(f)) {}

  /// Constructs a read operation with a completion task.
  template <class F>
  CanChannelRead(can_msg* msg, can_err* err, ::std::chrono::nanoseconds* dp,
                 F&& f)
      : CanChannelRead(msg, err, dp, nullptr, ::std::forward<F>(f)) {}

  CanChannelRead(const CanChannelRead&) = delete;

  CanChannelRead& operator=(const CanChannelRead&) = delete;

  operator ev_task&() & noexcept { return task; }

  /// Returns the executor to which the completion task is (to be) submitted.
  ev::Executor
  get_executor() const noexcept {
    return ev::Executor(task.exec);
  }

 private:
  timespec ts_{0, 0};
  ::std::chrono::nanoseconds* dp_{nullptr};
  ::std::function<Signature> func_;
};

namespace detail {

template <class F>
class CanChannelWriteWrapper : public io_can_chan_write {
 public:
  CanChannelWriteWrapper(const can_msg& msg, ev_exec_t* exec, F&& f)
      : io_can_chan_write IO_CAN_CHAN_WRITE_INIT(
            &msg, exec,
            [](ev_task* task) noexcept {
              auto write = io_can_chan_write_from_task(task);
              ::std::error_code ec = util::make_error_code(write->errc);
              auto self = static_cast<CanChannelWriteWrapper*>(write);
              compat::invoke(::std::move(self->func_), ec);
              delete self;
            }),
        func_(::std::forward<F>(f)) {}

  CanChannelWriteWrapper(const CanChannelWriteWrapper&) = delete;

  CanChannelWriteWrapper& operator=(const CanChannelWriteWrapper&) = delete;

 private:
  typename ::std::decay<F>::type func_;
};

}  // namespace detail

/**
 * Creates a CAN channel write operation with a completion task. The operation
 * deletes itself after it is completed, so it MUST NOT be deleted once it is
 * submitted to a CAN channel.
 */
template <class F>
// clang-format off
inline typename ::std::enable_if<
    compat::is_invocable<F, ::std::error_code>::value,
    detail::CanChannelWriteWrapper<F>*>::type
make_can_channel_write_wrapper(const can_msg& msg, ev_exec_t* exec, F&& f) {
  // clang-format on
  return new detail::CanChannelWriteWrapper<F>(msg, exec, ::std::forward<F>(f));
}

/**
 * A write operation suitable for use with a CAN channel. This class stores a
 * callable object with signature `void(std::error_code ec)`, which is invoked
 * upon completion (or cancellation) of the write operation.
 */
class CanChannelWrite : public io_can_chan_write {
 public:
  using Signature = void(::std::error_code);

  /// Constructs a write operation with a completion task.
  template <class F>
  CanChannelWrite(const can_msg& msg, ev_exec_t* exec, F&& f)
      : io_can_chan_write IO_CAN_CHAN_WRITE_INIT(
            &msg, exec,
            [](ev_task* task) noexcept {
              auto write = io_can_chan_write_from_task(task);
              auto self = static_cast<CanChannelWrite*>(write);
              if (self->func_) {
                ::std::error_code ec = util::make_error_code(write->errc);
                self->func_(ec);
              }
            }),
        func_(::std::forward<F>(f)) {}

  /// Constructs a write operation with a completion task.
  template <class F>
  CanChannelWrite(const can_msg& msg, F&& f)
      : CanChannelWrite(msg, nullptr, ::std::forward<F>(f)) {}

  CanChannelWrite(const CanChannelWrite&) = delete;

  CanChannelWrite& operator=(const CanChannelWrite&) = delete;

  operator ev_task&() & noexcept { return task; }

  /// Returns the executor to which the completion task is (to be) submitted.
  ev::Executor
  get_executor() const noexcept {
    return ev::Executor(task.exec);
  }

 private:
  ::std::function<Signature> func_;
};

/**
 * A reference to an abstract CAN controller. This class is a wrapper around
 * `#io_can_ctrl_t*`.
 */
class CanControllerBase {
 public:
  explicit CanControllerBase(io_can_ctrl_t* ctrl_) noexcept : ctrl(ctrl_) {}

  operator io_can_ctrl_t*() const noexcept { return ctrl; }

  /// @see io_can_ctrl_stop()
  void
  stop(::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    if (!io_can_ctrl_stop(*this))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_can_ctrl_stop()
  void
  stop() {
    ::std::error_code ec;
    stop(ec);
    if (ec) throw ::std::system_error(ec, "stop");
  }

  /// @see io_can_ctrl_stopped()
  bool
  stopped(::std::error_code& ec) const noexcept {
    int errsv = get_errc();
    set_errc(0);
    int stopped = io_can_ctrl_stopped(*this);
    if (stopped >= 0) {
      ec.clear();
    } else {
      ec = util::make_error_code();
      stopped = 0;
    }
    set_errc(errsv);
    return stopped != 0;
  }

  /// @see io_can_ctrl_stopped()
  bool
  stopped() const {
    ::std::error_code ec;
    auto result = stopped(ec);
    if (ec) throw ::std::system_error(ec, "stopped");
    return result;
  }

  /// @see io_can_ctrl_restart()
  void
  restart(::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    if (!io_can_ctrl_restart(*this))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_can_ctrl_restart()
  void
  restart() {
    ::std::error_code ec;
    restart(ec);
    if (ec) throw ::std::system_error(ec, "restart");
  }

  /// @see io_can_ctrl_get_bitrate()
  void
  get_bitrate(int* pnominal, int* pdata, ::std::error_code& ec) const noexcept {
    int errsv = get_errc();
    set_errc(0);
    if (!io_can_ctrl_get_bitrate(*this, pnominal, pdata))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_can_ctrl_get_bitrate()
  void
  get_bitrate(int* pnominal, int* pdata = nullptr) const {
    ::std::error_code ec;
    get_bitrate(pnominal, pdata, ec);
    if (ec) throw ::std::system_error(ec, "get_bitrate");
  }

  /// @see io_can_ctrl_get_bitrate()
  void
  set_bitrate(int nominal, int data, ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    if (!io_can_ctrl_set_bitrate(*this, nominal, data))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_can_ctrl_get_bitrate()
  void
  set_bitrate(int nominal, int data = 0) {
    ::std::error_code ec;
    set_bitrate(nominal, data, ec);
    if (ec) throw ::std::system_error(ec, "set_bitrate");
  }

  /// @see io_can_ctrl_get_state()
  CanState
  get_state(::std::error_code& ec) const noexcept {
    int errsv = get_errc();
    set_errc(0);
    int state = io_can_ctrl_get_state(*this);
    if (state >= 0) {
      ec.clear();
    } else {
      ec = util::make_error_code();
      state = 0;
    }
    set_errc(errsv);
    return static_cast<CanState>(state);
  }

  /// @see io_can_ctrl_get_state()
  CanState
  get_state() const {
    ::std::error_code ec;
    auto result = get_state(ec);
    if (ec) throw ::std::system_error(ec, "get_state");
    return result;
  }

 protected:
  io_can_ctrl_t* ctrl{nullptr};
};

/**
 * A reference to an abstract CAN channel. This class is a wrapper around
 * `#io_can_chan_t*`.
 */
class CanChannelBase : public Device {
 public:
  using Device::operator io_dev_t*;

  explicit CanChannelBase(io_can_chan_t* chan_) noexcept
      : Device(chan_ ? io_can_chan_get_dev(chan_) : nullptr), chan(chan_) {}

  operator io_can_chan_t*() const noexcept { return chan; }

  /// @see io_can_chan_get_flags()
  CanBusFlag
  get_flags() const {
    int flags = io_can_chan_get_flags(*this);
    if (flags == -1) util::throw_errc("get_flags");
    return static_cast<CanBusFlag>(flags);
  }

  /// @see io_can_chan_read()
  int
  read(can_msg* msg, can_err* err, ::std::chrono::nanoseconds* dp, int timeout,
       ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    timespec ts = {0, 0};
    int result = io_can_chan_read(*this, msg, err, dp ? &ts : nullptr, timeout);
    if (result >= 0)
      ec.clear();
    else
      ec = util::make_error_code();
    if (dp) *dp = util::from_timespec(ts);
    set_errc(errsv);
    return result;
  }

  /// @see io_can_chan_read()
  bool
  read(can_msg* msg, can_err* err = nullptr,
       ::std::chrono::nanoseconds* dp = nullptr, int timeout = -1) {
    ::std::error_code ec;
    int result = read(msg, err, dp, timeout, ec);
    if (result < 0) throw ::std::system_error(ec, "read");
    return result > 0;
  }

  /// @see io_can_chan_submit_read()
  void
  submit_read(struct io_can_chan_read& read) noexcept {
    io_can_chan_submit_read(*this, &read);
  }

  /// @see io_can_chan_submit_read()
  template <class F>
  void
  submit_read(can_msg* msg, can_err* err, ::std::chrono::nanoseconds* dp,
              ev_exec_t* exec, F&& f) {
    submit_read(*make_can_channel_read_wrapper(msg, err, dp, exec,
                                               ::std::forward<F>(f)));
  }

  /// @see io_can_chan_submit_read()
  template <class F>
  void
  submit_read(can_msg* msg, can_err* err, ::std::chrono::nanoseconds* dp,
              F&& f) {
    submit_read(msg, err, dp, nullptr, ::std::forward<F>(f));
  }

  /// @see io_can_chan_cancel_read()
  bool
  cancel_read(struct io_can_chan_read& read) noexcept {
    return io_can_chan_cancel_read(*this, &read) != 0;
  }

  /// @see io_can_chan_abort_read()
  bool
  abort_read(struct io_can_chan_read& read) noexcept {
    return io_can_chan_abort_read(*this, &read) != 0;
  }

  /// @see io_can_chan_async_read()
  ev::Future<int, int>
  async_read(ev_exec_t* exec, can_msg* msg, can_err* err = nullptr,
             timespec* tp = nullptr,
             struct io_can_chan_read** pread = nullptr) {
    return ev::Future<int, int>(
        io_can_chan_async_read(*this, exec, msg, err, tp, pread));
  }

  /// @see io_can_chan_async_read()
  ev::Future<int, int>
  async_read(can_msg* msg, can_err* err = nullptr, timespec* tp = nullptr,
             struct io_can_chan_read** pread = nullptr) {
    return async_read(nullptr, msg, err, tp, pread);
  }

  /// @see io_can_chan_write()
  void
  write(const can_msg& msg, int timeout, ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    if (!io_can_chan_write(*this, &msg, timeout))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_can_chan_write()
  void
  write(const can_msg& msg, int timeout = -1) {
    ::std::error_code ec;
    write(msg, timeout, ec);
    if (ec) throw ::std::system_error(ec, "write");
  }

  /// @see io_can_chan_submit_write()
  void
  submit_write(struct io_can_chan_write& write) noexcept {
    io_can_chan_submit_write(*this, &write);
  }

  /// @see io_can_chan_submit_write()
  template <class F>
  void
  submit_write(const can_msg& msg, ev_exec_t* exec, F&& f) {
    submit_write(
        *make_can_channel_write_wrapper(msg, exec, ::std::forward<F>(f)));
  }

  /// @see io_can_chan_submit_write()
  template <class F>
  void
  submit_write(const can_msg& msg, F&& f) {
    submit_write(msg, nullptr, ::std::forward<F>(f));
  }

  /// @see io_can_chan_cancel_write()
  bool
  cancel_write(struct io_can_chan_write& write) noexcept {
    return io_can_chan_cancel_write(*this, &write) != 0;
  }

  /// @see io_can_chan_abort_write()
  bool
  abort_write(struct io_can_chan_write& write) noexcept {
    return io_can_chan_abort_write(*this, &write) != 0;
  }

  /// @see io_can_chan_async_write()
  ev::Future<void, int>
  async_write(ev_exec_t* exec, const can_msg& msg,
              struct io_can_chan_write** pwrite = nullptr) {
    auto future = io_can_chan_async_write(*this, exec, &msg, pwrite);
    if (!future) util::throw_errc("async_write");
    return ev::Future<void, int>(future);
  }

  /// @see io_can_chan_async_write()
  ev::Future<void, int>
  async_write(const can_msg& msg, struct io_can_chan_write** pwrite = nullptr) {
    return async_write(nullptr, msg, pwrite);
  }

 protected:
  io_can_chan_t* chan{nullptr};
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_CAN_HPP_
