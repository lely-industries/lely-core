/*!\file
 * This header file is part of the C++ CANopen application library; it contains
 * the I/O context declarations.
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

#ifndef LELY_COAPP_IO_CONTEXT_HPP_
#define LELY_COAPP_IO_CONTEXT_HPP_

#include <lely/aio/can_bus.hpp>
#include <lely/aio/timer.hpp>
#include <lely/coapp/coapp.hpp>

#include <memory>

namespace lely {

// The CAN network interface from <lely/can/net.hpp>.
class CANNet;

namespace canopen {

/*!
 * The I/O context. This context manages all timer and I/O events on the CAN
 * bus.
 */
class LELY_COAPP_EXTERN IoContext {
 public:
  using CanState = aio::CanBus::State;
  using CanError = aio::CanBus::Error;

  /*!
   * Creates a new I/O context.
   *
   * \param timer  the timer used for CANopen events.
   * \param bus    a handle to the CAN bus.
   * \param mutex  an (optional) pointer to the mutex to be locked while timer
   *               and I/O events are processed. The mutex MUST be unlocked when
   *               any public member function is invoked; it will be locked for
   *               the duration of any call to a virtual member function
   *               (#OnCanState() or #OnCanError()).
   */
  IoContext(aio::TimerBase& timer, aio::CanBusBase& bus,
            BasicLockable* mutex = nullptr);

  IoContext(const IoContext&) = delete;
  IoContext& operator=(const IoContext&) = delete;

 protected:
  ~IoContext();

  /*!
   * Returns a pointer to the internal CAN network interface from
   * <lely/can/net.hpp>.
   */
  CANNet* net() const noexcept;

  /*!
   * Update the CAN network time. If a mutex was passed to the constructtor, it
   * MUST be locked for the duration of this call.
   */
  void SetTime();

#ifndef DOXYGEN_SHOULD_SKIP_THIS
 private:
#endif
  /*!
   * The function invoked when a CAN bus state change is detected. The state is
   * represented by one the `CanState::ACTIVE`, `CanState::PASSIVE` or
   * `CanState::BUSOFF` values.
   *
   * \param new_state the current state of the CAN bus.
   * \param old_state the previous state of the CAN bus.
   */
  virtual void
  OnCanState(CanState new_state, CanState old_state) noexcept {
    (void)new_state;
    (void)old_state;
  }

  /*!
   * The function invoked when an error is detected on the CAN bus.
   *
   * \param error a bitwise combination of `CanError::BIT`, `CanError::STUFF`,
                  `CanError::CRC`, `CanError::FORM`, `CanError::ACK` and
   *              `CanError::OTHER`.
   */
  virtual void OnCanError(CanError error) noexcept { (void)error; }

 private:
  struct Impl_;
  ::std::unique_ptr<Impl_> impl_;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_IO_CONTEXT_HPP_
