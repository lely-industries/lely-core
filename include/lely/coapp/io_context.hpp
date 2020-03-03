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
