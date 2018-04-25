/*!\file
 * This header file is part of the C++ CANopen application library; it contains
 * the SDO error declarations.
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

#ifndef LELY_COAPP_SDO_ERROR_HPP_
#define LELY_COAPP_SDO_ERROR_HPP_

#include <lely/coapp/coapp.hpp>

#include <stdexcept>
#include <system_error>

#include <cstdint>

namespace lely {

namespace canopen {

//! The SDO abort codes.
enum class SdoErrc : uint32_t {
  //! Toggle bit not altered.
  TOGGLE = UINT32_C(0x05030000),
  //! SDO protocol timed out.
  TIMEOUT = UINT32_C(0x05040000),
  //! Client/server command specifier not valid or unknown.
  NO_CS = UINT32_C(0x05040001),
  //! Invalid block size (block mode only).
  BLK_SIZE = UINT32_C(0x05040002),
  //! Invalid sequence number (block mode only).
  BLK_SEQ = UINT32_C(0x05040003),
  //! CRC error (block mode only).
  BLK_CRC = UINT32_C(0x05040004),
  //! Out of memory.
  NO_MEM = UINT32_C(0x05040005),
  //! Unsupported access to an object.
  NO_ACCESS = UINT32_C(0x06010000),
  //! Attempt to read a write only object.
  NO_READ = UINT32_C(0x06010001),
  //! Attempt to write a read only object.
  NO_WRITE = UINT32_C(0x06010002),
  //! Object does not exist in the object dictionary.
  NO_OBJ = UINT32_C(0x06020000),
  //! Object cannot be mapped to the PDO.
  NO_PDO = UINT32_C(0x06040041),
  /*!
   * The number and length of the objects to be mapped would exceed the PDO
   * length.
   */
  PDO_LEN = UINT32_C(0x06040042),
  //! General parameter incompatibility reason.
  PARAM = UINT32_C(0x06040043),
  //! General internal incompatibility in the device.
  COMPAT = UINT32_C(0x06040047),
  //! Access failed due to a hardware error.
  HARDWARE = UINT32_C(0x06060000),
  //! Data type does not match, length of service parameter does not match.
  TYPE_LEN = UINT32_C(0x06070010),
  //! Data type does not match, length of service parameter too high.
  TYPE_LEN_HI = UINT32_C(0x06070012),
  //! Data type does not match, length of service parameter too low.
  TYPE_LEN_LO = UINT32_C(0x06070013),
  //! Sub-index does not exist.
  NO_SUB = UINT32_C(0x06090011),
  //! Invalid value for parameter (download only).
  PARAM_VAL = UINT32_C(0x06090030),
  //! Value of parameter written too high (download only).
  PARAM_HI = UINT32_C(0x06090031),
  //! Value of parameter written too low (download only).
  PARAM_LO = UINT32_C(0x06090032),
  //! Maximum value is less than minimum value (download only).
  PARAM_RANGE = UINT32_C(0x06090036),
  //! Resource not available: SDO connection.
  NO_SDO = UINT32_C(0x060a0023),
  //! General error.
  ERROR = UINT32_C(0x08000000),
  //! Data cannot be transferred or stored to the application.
  DATA = UINT32_C(0x08000020),
  /*!
   * Data cannot be transferred or stored to the application because of local
   * control.
   */
  DATA_CTL = UINT32_C(0x08000021),
  /*!
   * Data cannot be transferred or stored to the application because of the
   * present device state.
   */
  DATA_DEV = UINT32_C(0x08000022),
  /*!
   * Object dictionary dynamic generation fails or no object dictionary is
   * present (e.g. object dictionary is generated from file and generation fails
   * because of a file error).
   */
  NO_OD = UINT32_C(0x08000023),
  //! No data available. (`NO_DATA` is a macro defined in <netdb.h>)
  NO_VAL = UINT32_C(0x08000024)
};

//! The type of exception thrown when an SDO abort code is received.
class LELY_COAPP_EXTERN SdoError : public ::std::runtime_error {
 public:
  SdoError(uint8_t netid, uint8_t id, uint16_t idx, uint8_t subidx,
           ::std::error_code ec);
  SdoError(uint8_t netid, uint8_t id, uint16_t idx, uint8_t subidx,
           ::std::error_code ec, const ::std::string& what_arg);
  SdoError(uint8_t netid, uint8_t id, uint16_t idx, uint8_t subidx,
           ::std::error_code ec, const char* what_arg);
  SdoError(uint8_t netid, uint8_t id, uint16_t idx, uint8_t subidx, int ev,
           const char* what_arg);
  SdoError(uint8_t netid, uint8_t id, uint16_t idx, uint8_t subidx, int ev);

  //! Returns the network-ID.
  uint8_t netid() const noexcept { return netid_; }

  //! Returns the node-ID.
  uint8_t id() const noexcept { return id_; }

  //! Returns the object index.
  uint16_t idx() const noexcept { return idx_; }

  //! Returns the object sub-index.
  uint8_t subidx() const noexcept { return subidx_; }

  //! Returns the stored error code.
  const ::std::error_code& code() const noexcept { return ec_; }

 private:
  uint8_t netid_ { 0 };
  uint8_t id_ { 0 };
  uint16_t idx_ { 0 };
  uint8_t subidx_ { 0 };
  ::std::error_code ec_;
};

//! Returns a reference to the error category object for SDO abort codes.
LELY_COAPP_EXTERN const ::std::error_category& SdoCategory() noexcept;

//! Creates an error code corresponding to an SDO abort code.
LELY_COAPP_EXTERN ::std::error_code make_error_code(SdoErrc e) noexcept;

//! Creates an error condition corresponding to an SDO abort code.
LELY_COAPP_EXTERN ::std::error_condition make_error_condition(SdoErrc e)
    noexcept;

}  // namespace canopen

}  // namespace lely

namespace std {

template <>
struct is_error_code_enum<::lely::canopen::SdoErrc> : true_type {};

}  // namespace std

#endif  // LELY_COAPP_SDO_ERROR_HPP_
