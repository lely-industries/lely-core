/**@file
 * This header file is part of the I/O library; it contains the C++ CAN frame
 * declarations.
 *
 * @see lely/io2/can/msg.h
 *
 * @copyright 2019-2020 Lely Industries N.V.
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

#ifndef LELY_IO2_CAN_MSG_HPP_
#define LELY_IO2_CAN_MSG_HPP_

#include <lely/io2/can/msg.h>

namespace lely {
namespace io {

/// The error flags of a CAN bus, which are not mutually exclusive.
enum class CanFlag : int {
  /**
   * The Identifier Extension (IDE) flag. If this flag is set, the CAN Extended
   * Format (with a 29-bit identifier) is used, otherwise the CAN Base Format
   * (with an 11-bit identifier) is used.
   */
  IDE = CAN_FLAG_IDE,
  /**
   * The Remote Transmission Request (RTR) flag (unavailable in CAN FD format
   * frames). If this flag is set, the frame has no payload.
   */
  RTR = CAN_FLAG_RTR,
#if !LELY_NO_CANFD
  /**
   * The FD Format (FDF) flag, formerly known as Extended Data Length (EDL).
   * This flag is set for CAN FD format frames.
   */
  FDF = CAN_FLAG_FDF,
  /**
   * The Bit Rate Switch (BRS) flag (only available in CAN FD format frames). If
   * this flag is set, the bit rate is switched from the standard bit rate of
   * the arbitration phase to the preconfigured alternate bit rate of the data
   * phase.
   */
  BRS = CAN_FLAG_BRS,
  /**
   * The Error State Indicator (ESI) flag (only available in CAN FD format
   * frames).
   */
  ESI = CAN_FLAG_ESI,
#endif  // !LELY_NO_CANFD
  NONE = 0
};

constexpr CanFlag
operator~(CanFlag rhs) {
  return static_cast<CanFlag>(~static_cast<int>(rhs));
}

constexpr CanFlag
operator&(CanFlag lhs, CanFlag rhs) {
  return static_cast<CanFlag>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

constexpr CanFlag
operator^(CanFlag lhs, CanFlag rhs) {
  return static_cast<CanFlag>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
}

constexpr CanFlag
operator|(CanFlag lhs, CanFlag rhs) {
  return static_cast<CanFlag>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline CanFlag&
operator&=(CanFlag& lhs, CanFlag rhs) {
  return lhs = lhs & rhs;
}

inline CanFlag&
operator^=(CanFlag& lhs, CanFlag rhs) {
  return lhs = lhs ^ rhs;
}

inline CanFlag&
operator|=(CanFlag& lhs, CanFlag rhs) {
  return lhs = lhs | rhs;
}

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_CAN_MSG_HPP_
