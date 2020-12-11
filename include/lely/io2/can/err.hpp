/**@file
 * This header file is part of the I/O library; it contains the C++ CAN bus
 * error definitions.
 *
 * @see lely/io2/can/err.h
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

#ifndef LELY_IO2_CAN_ERR_HPP_
#define LELY_IO2_CAN_ERR_HPP_

#include <lely/io2/can/err.h>

namespace lely {
namespace io {

/// The states of a CAN node, depending on the TX/RX error count.
enum class CanState : int {
  /// The error active state (TX/RX error count < 128).
  ACTIVE = CAN_STATE_ACTIVE,
  /// The error passive state (TX/RX error count < 256).
  PASSIVE = CAN_STATE_PASSIVE,
  /// The bus off state (TX/RX error count >= 256).
  BUSOFF = CAN_STATE_BUSOFF,
  /// The device is in sleep mode.
  SLEEPING = CAN_STATE_SLEEPING,
  /// The device is stopped.
  STOPPED = CAN_STATE_STOPPED
};

/// The error flags of a CAN bus, which are not mutually exclusive.
enum class CanError : int {
  /// A single bit error.
  BIT = CAN_ERROR_BIT,
  /// A bit stuffing error.
  STUFF = CAN_ERROR_STUFF,
  /// A CRC sequence error.
  CRC = CAN_ERROR_CRC,
  /// A form error.
  FORM = CAN_ERROR_FORM,
  /// An acknowledgment error.
  ACK = CAN_ERROR_ACK,
  /// One or more other errors.
  OTHER = CAN_ERROR_OTHER,
  NONE = 0
};

constexpr CanError
operator~(CanError rhs) {
  return static_cast<CanError>(~static_cast<int>(rhs));
}

constexpr CanError
operator&(CanError lhs, CanError rhs) {
  return static_cast<CanError>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

constexpr CanError
operator^(CanError lhs, CanError rhs) {
  return static_cast<CanError>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
}

constexpr CanError
operator|(CanError lhs, CanError rhs) {
  return static_cast<CanError>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline CanError&
operator&=(CanError& lhs, CanError rhs) {
  return lhs = lhs & rhs;
}

inline CanError&
operator^=(CanError& lhs, CanError rhs) {
  return lhs = lhs ^ rhs;
}

inline CanError&
operator|=(CanError& lhs, CanError rhs) {
  return lhs = lhs | rhs;
}

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_CAN_ERR_HPP_
