/**@file
 * This header file is part of the C++ CANopen application library; it contains
 * the I/O context declarations.
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

#ifndef LELY_COAPP_IO_CONTEXT_HPP_
#define LELY_COAPP_IO_CONTEXT_HPP_

#include <lely/io2/can.hpp>
#include <lely/io2/tqueue.hpp>
#include <lely/util/mutex.hpp>

#include <functional>
#include <memory>
#include <utility>

namespace lely {

// The CAN network interface from <lely/can/net.hpp>.
class CANNet;

namespace canopen {

/**
 * The I/O context. This context manages all timer and I/O events on the CAN
 * bus.
 */
class IoContext {
 public:
  using duration = io::TimerBase::duration;
  using time_point = io::TimerBase::time_point;

  /**
   * Creates a new I/O context.
   *
   * @param timer  the timer used for CANopen events.
   * @param chan   a CAN channel.
   * @param mutex  an (optional) pointer to the mutex to be locked while timer
   *               and I/O events are processed. The mutex MUST be unlocked when
   *               any public member function is invoked; it will be locked for
   *               the duration of any call to a virtual member function
   *               (#OnCanState() or #OnCanError()).
   */
  IoContext(io::TimerBase& timer, io::CanChannelBase& chan,
            util::BasicLockable* mutex = nullptr);

  IoContext(const IoContext&) = delete;
  IoContext& operator=(const IoContext&) = delete;

  /// Returns the executor used to process I/O events on the CAN bus.
  ev::Executor GetExecutor() const noexcept;

  /// Returns the underlying I/O context with which this context is registered.
  io::ContextBase GetContext() const noexcept;

  /// Returns the clock used by the timer.
  io::Clock GetClock() const noexcept;

  /**
   * Submits a wait operation. The completion task is submitted for execution
   * once the specified absolute timeout expires.
   */
  void SubmitWait(const time_point& t, io_tqueue_wait& wait);

  /**
   * Submits a wait operation. The completion task is submitted for execution
   * once the specified relative timeout expires.
   */
  void SubmitWait(const duration& d, io_tqueue_wait& wait);

  /**
   * Submits a wait operation. The completion task is submitted for execution
   * once the specified absolute timeout expires.
   *
   * @param t    the absolute expiration time of the wait operation.
   * @param exec the executor used to execute the completion task.
   * @param f    the function to be called on completion of the wait operation.
   */
  template <class F>
  void
  SubmitWait(const time_point& t, ev_exec_t* exec, F&& f) {
    SubmitWait(t,
               *io::make_timer_queue_wait_wrapper(exec, ::std::forward<F>(f)));
  }

  /**
   * Submits a wait operation. The completion task is submitted for execution
   * once the specified relative timeout expires.
   *
   * @param d    the relative expiration time of the wait operation.
   * @param exec the executor used to execute the completion task.
   * @param f    the function to be called on completion of the wait operation.
   */
  template <class F>
  void
  SubmitWait(const duration& d, ev_exec_t* exec, F&& f) {
    SubmitWait(d,
               *io::make_timer_queue_wait_wrapper(exec, ::std::forward<F>(f)));
  }

  /// Equivalent to `SubmitWait(t, nullptr, f)`.
  template <class F>
  typename ::std::enable_if<!::std::is_base_of<
      io_tqueue_wait, typename ::std::decay<F>::type>::value>::type
  SubmitWait(const time_point& t, F&& f) {
    SubmitWait(t, nullptr, ::std::forward<F>(f));
  }

  /// Equivalent to `SubmitWait(d, nullptr, f)`.
  template <class F>
  typename ::std::enable_if<!::std::is_base_of<
      io_tqueue_wait, typename ::std::decay<F>::type>::value>::type
  SubmitWait(const duration& d, F&& f) {
    SubmitWait(d, nullptr, ::std::forward<F>(f));
  }

  /**
   * Submits an asynchronous wait operation and creates a future which becomes
   * ready once the wait operation completes (or is canceled).
   *
   * @param exec  the executor used to execute the completion task.
   * @param t     the absolute expiration time of the wait operation.
   * @param pwait an optional address at which to store a pointer to the wait
   *              operation. This can be used to cancel the wait operation with
   *              CancelWait().
   *
   * @returns a future which holds an exception pointer on error.
   */
  ev::Future<void, ::std::exception_ptr> AsyncWait(
      ev_exec_t* exec, const time_point& t, io_tqueue_wait** pwait = nullptr);

  /**
   * Submits an asynchronous wait operation and creates a future which becomes
   * ready once the wait operation completes (or is canceled).
   *
   * @param exec  the executor used to execute the completion task.
   * @param d     the relative expiration time of the wait operation.
   * @param pwait an optional address at which to store a pointer to the wait
   *              operation. This can be used to cancel the wait operation with
   *              CancelWait().
   *
   * @returns a future which holds an exception pointer on error.
   */
  ev::Future<void, ::std::exception_ptr> AsyncWait(
      ev_exec_t* exec, const duration& d, io_tqueue_wait** pwait = nullptr);

  /// Equivalent to `AsyncWait(nullptr, t, pwait)`.
  ev::Future<void, ::std::exception_ptr>
  AsyncWait(const time_point& t, io_tqueue_wait** pwait = nullptr) {
    return AsyncWait(nullptr, t, pwait);
  }

  /// Equivalent to `AsyncWait(nullptr, d, pwait)`.
  ev::Future<void, ::std::exception_ptr>
  AsyncWait(const duration& d, io_tqueue_wait** pwait = nullptr) {
    return AsyncWait(nullptr, d, pwait);
  }

  /**
   * Cancels the specified wait operation if it is pending. If canceled, the
   * completion task is submitted for exection with <b>ec</b> =
   * `::std::errc::operation_canceled`.
   *
   * @returns true if the operation was canceled, and false if it was not
   * pending.
   */
  bool CancelWait(io_tqueue_wait& wait) noexcept;

  /**
   * Aborts the specified wait operation if it is pending. If aborted, the
   * completion task is _not_ submitted for execution.
   *
   * @returns true if the operation was aborted, and false if it was not
   * pending.
   */
  bool AbortWait(io_tqueue_wait& wait) noexcept;

  /**
   * Registers the function to be invoked when a CAN bus state change is
   * detected. Only a single function can be registered at any one time. If
   * <b>on_can_state</b> contains a callable function target, a copy of the
   * target is invoked _after_ OnCanState(io::CanState, io::CanState) completes.
   */
  void OnCanState(
      ::std::function<void(io::CanState, io::CanState)> on_can_state);

  /**
   * Registers the function to be invoked when an error is detected on the CAN
   * bus. Only a single function can be registered at any one time. If
   * <b>on_can_error</b> contains a callable function target, a copy of the
   * target is invoked _after_ OnCanError(io::CanError) completes.
   */
  void OnCanError(::std::function<void(io::CanError)> on_can_error);

 protected:
  ~IoContext();

  /**
   * Returns a pointer to the internal CAN network interface from
   * <lely/can/net.hpp>.
   */
  CANNet* net() const noexcept;

  /**
   * Update the CAN network time. If a mutex was passed to the constructor, it
   * MUST be locked for the duration of this call.
   */
  void SetTime();
#ifndef DOXYGEN_SHOULD_SKIP_THIS

 private:
#endif
  /**
   * The function invoked when a CAN bus state change is detected. The state is
   * represented by one the `CanState::ACTIVE`, `CanState::PASSIVE` or
   * `CanState::BUSOFF` values.
   *
   * @param new_state the current state of the CAN bus.
   * @param old_state the previous state of the CAN bus.
   */
  virtual void
  OnCanState(io::CanState new_state, io::CanState old_state) noexcept {
    (void)new_state;
    (void)old_state;
  }

  /**
   * The function invoked when an error is detected on the CAN bus.
   *
   * @param error a bitwise combination of `CanError::BIT`, `CanError::STUFF`,
                  `CanError::CRC`, `CanError::FORM`, `CanError::ACK` and
   *              `CanError::OTHER`.
   */
  virtual void
  OnCanError(io::CanError error) noexcept {
    (void)error;
  }
#ifdef DOXYGEN_SHOULD_SKIP_THIS

 private:
#endif
  struct Impl_;
  ::std::unique_ptr<Impl_> impl_;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_IO_CONTEXT_HPP_
