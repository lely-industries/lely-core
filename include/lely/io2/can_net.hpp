/**@file
 * This header file is part of the I/O library; it contains the C++ interface
 * for the timer queue.
 *
 * @see lely/io2/can_net.h
 *
 * @copyright 2018-2021 Lely Industries N.V.
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

#ifndef LELY_IO2_CAN_NET_HPP_
#define LELY_IO2_CAN_NET_HPP_

#include <lely/io2/can.hpp>
#include <lely/io2/can_net.h>
#include <lely/io2/timer.hpp>
#include <lely/util/mutex.hpp>

#include <utility>

namespace lely {
namespace io {

/// A CAN network interface. This class is a wrapper around `io_can_net_t*`.
class CanNet : protected util::BasicLockable {
 public:
  /// @see io_can_net_create()
  explicit CanNet(ev_exec_t* exec, io_timer_t* timer, io_can_chan_t* chan,
                  ::std::size_t txlen = 0, int txtimeo = 0)
      : net_(io_can_net_create(exec, timer, chan, txlen, txtimeo)) {
    io_can_net_get_on_read_error_func(*this, &on_read_error_func_,
                                      &on_read_error_arg_);
    io_can_net_set_on_read_error_func(*this, &CanNet::on_read_error_, this);
    io_can_net_get_on_queue_error_func(*this, &on_queue_error_func_,
                                       &on_queue_error_arg_);
    io_can_net_set_on_queue_error_func(*this, &CanNet::on_queue_error_, this);
    io_can_net_get_on_write_error_func(*this, &on_write_error_func_,
                                       &on_write_error_arg_);
    io_can_net_set_on_write_error_func(*this, &CanNet::on_write_error_, this);
    io_can_net_get_on_can_state_func(*this, &on_can_state_func_,
                                     &on_can_state_arg_);
    io_can_net_set_on_can_state_func(*this, &CanNet::on_can_state_, this);
    io_can_net_get_on_can_error_func(*this, &on_can_error_func_,
                                     &on_can_error_arg_);
    io_can_net_set_on_can_error_func(*this, &CanNet::on_can_error_, this);
  }

  /// @see io_can_net_create()
  explicit CanNet(io_timer_t* timer, io_can_chan_t* chan,
                  ::std::size_t txlen = 0, int txtimeo = 0)
      : CanNet(nullptr, timer, chan, txlen, txtimeo) {}

  CanNet(const CanNet&) = delete;
  CanNet& operator=(const CanNet&) = delete;

  /// @see io_can_net_destroy()
  virtual ~CanNet() { io_can_net_destroy(*this); }

  operator io_can_net_t*() const noexcept { return net_; }

  operator io_tqueue_t*() const noexcept {
    return io_can_net_get_tqueue(*this);
  }

  /// @see io_can_net_start()
  void
  start() noexcept {
    io_can_net_start(*this);
  }

  /// @see io_can_net_get_ctx()
  ContextBase
  get_ctx() const noexcept {
    return ContextBase(io_can_net_get_ctx(*this));
  }

  /// @see io_can_net_get_exec()
  ev::Executor
  get_executor() const noexcept {
    return ev::Executor(io_can_net_get_exec(*this));
  }

  /// @see io_can_net_get_clock()
  Clock
  get_clock() const noexcept {
    return Clock(io_can_net_get_clock(*this));
  }

 protected:
  void
  lock() final {
    if (io_can_net_lock(*this) == -1) util::throw_errc("lock");
  }

  void
  unlock() final {
    if (io_can_net_unlock(*this) == -1) util::throw_errc("unlock");
  }

  operator __can_net*() const noexcept { return io_can_net_get_net(*this); }

  /**
   * Updates the CAN network time.
   *
   * The mutex protecting the CAN network interface MUST be locked for the
   * duration of this call.
   *
   * @see io_can_net_set_time()
   */
  void
  set_time() {
    if (io_can_net_set_time(*this) == -1) util::throw_errc("set_time");
  }

  /**
   * The function invoked when a new CAN frame read error occurs, or when a read
   * operation completes successfully after one or more errors.
   *
   * The mutex protecting the CAN network interface will be locked when this
   * function is called.
   *
   * @param ec     the error code (0 on success).
   * @param errcnt the number of errors since the last successful read
   *               operation.
   */
  virtual void
  on_read_error(::std::error_code ec, ::std::size_t errcnt) noexcept {
    // Invoke the default callback.
    on_read_error_func_(ec.value(), errcnt, on_read_error_arg_);
  }

  /**
   * The function invoked when a CAN frame is dropped because the transmit queue
   * is full, or when a frame is successfully queued after one or more errors.
   *
   * The mutex protecting the CAN network interface will be locked when this
   * function is called.
   *
   * @param ec     the error code (0 on success).
   * @param errcnt the number of errors since the last frame was successfully
   *               queued.
   */
  virtual void
  on_queue_error(::std::error_code ec, ::std::size_t errcnt) noexcept {
    // Invoke the default callback.
    on_queue_error_func_(ec.value(), errcnt, on_queue_error_arg_);
  }

  /**
   * The function invoked when a new CAN frame write error occurs, or when a
   * write operation completes successfully after one or more errors.
   *
   * The mutex protecting the CAN network interface will be locked when this
   * function is called.
   *
   * @param ec     the error code (0 on success).
   * @param errcnt the number of errors since the last successful write
   *               operation.
   */
  virtual void
  on_write_error(::std::error_code ec, ::std::size_t errcnt) noexcept {
    // Invoke the default callback.
    on_write_error_func_(ec.value(), errcnt, on_write_error_arg_);
  }

  /**
   * The function invoked when a CAN bus state change is detected. The state is
   * represented by one the `CanState::ACTIVE`, `CanState::PASSIVE`,
   * `CanState::BUSOFF`, `CanState::SLEEPING` or `CanState::STOPPED` values.
   *
   * The mutex protecting the CAN network interface will be locked when this
   * function is called.
   *
   * @param new_state the current state of the CAN bus.
   * @param old_state the previous state of the CAN bus.
   */
  virtual void
  on_can_state(CanState new_state, CanState old_state) noexcept {
    // Invoke the default callback.
    on_can_state_func_(static_cast<int>(new_state), static_cast<int>(old_state),
                       on_can_state_arg_);
  }

  /**
   * The function invoked when an error is detected on the CAN bus.
   *
   * The mutex protecting the CAN network interface will be locked when this
   * function is called.
   *
   * @param error the detected errors (any combination of `CanError::BIT`,
   *              `CanError::STUFF`, `CanError::CRC`, `CanError::FORM`,
   *              `CanError::ACK` and `CanError::OTHER`).
   */
  virtual void
  on_can_error(CanError error) noexcept {
    // Invoke the default callback.
    on_can_error_func_(static_cast<int>(error), on_can_error_arg_);
  }

 private:
  static void
  on_read_error_(int errc, size_t errcnt, void* arg) noexcept {
    static_cast<CanNet*>(arg)->on_read_error(util::make_error_code(errc),
                                             errcnt);
  }

  static void
  on_queue_error_(int errc, size_t errcnt, void* arg) noexcept {
    static_cast<CanNet*>(arg)->on_queue_error(util::make_error_code(errc),
                                              errcnt);
  }

  static void
  on_write_error_(int errc, size_t errcnt, void* arg) noexcept {
    static_cast<CanNet*>(arg)->on_write_error(util::make_error_code(errc),
                                              errcnt);
  }

  static void
  on_can_state_(int new_state, int old_state, void* arg) noexcept {
    static_cast<CanNet*>(arg)->on_can_state(static_cast<CanState>(new_state),
                                            static_cast<CanState>(old_state));
  }

  static void
  on_can_error_(int error, void* arg) noexcept {
    static_cast<CanNet*>(arg)->on_can_error(static_cast<CanError>(error));
  }

  io_can_net* net_{nullptr};
  io_can_net_on_error_func_t* on_read_error_func_{nullptr};
  void* on_read_error_arg_{nullptr};
  io_can_net_on_error_func_t* on_queue_error_func_{nullptr};
  void* on_queue_error_arg_{nullptr};
  io_can_net_on_error_func_t* on_write_error_func_{nullptr};
  void* on_write_error_arg_{nullptr};
  io_can_net_on_can_state_func_t* on_can_state_func_{nullptr};
  void* on_can_state_arg_{nullptr};
  io_can_net_on_can_error_func_t* on_can_error_func_{nullptr};
  void* on_can_error_arg_{nullptr};
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_CAN_NET_HPP_
