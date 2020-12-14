/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the SDO error and exception functions.
 *
 * @see lely/coapp/sdo_error.hpp
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

#include "coapp.hpp"
#include <lely/co/sdo.h>
#include <lely/coapp/sdo_error.hpp>
#include <lely/util/error.hpp>

#include <iomanip>
#include <sstream>
#include <string>

namespace lely {

namespace canopen {

namespace {

class SdoErrcCategory : public ::std::error_category {
 public:
  const char*
  name() const noexcept override {
    return "SDO";
  }

  ::std::error_condition default_error_condition(
      int ev) const noexcept override;

  ::std::string
  message(int ev) const override {
    return co_sdo_ac2str(ev);
  }
};

::std::error_condition
SdoErrcCategory::default_error_condition(int ev) const noexcept {
  switch (static_cast<SdoErrc>(ev)) {
    case SdoErrc::TOGGLE:
      return ::std::errc::protocol_error;
    case SdoErrc::TIMEOUT:
      return ::std::errc::timed_out;
    case SdoErrc::NO_CS:
    case SdoErrc::BLK_SIZE:
    case SdoErrc::BLK_SEQ:
      return ::std::errc::protocol_error;
    case SdoErrc::BLK_CRC:
      return ::std::errc::illegal_byte_sequence;
    case SdoErrc::NO_MEM:
      return ::std::errc::not_enough_memory;
    case SdoErrc::NO_ACCESS:
    case SdoErrc::NO_READ:
    case SdoErrc::NO_WRITE:
      return ::std::errc::permission_denied;
    case SdoErrc::NO_OBJ:
      return ::std::errc::no_such_file_or_directory;
    // case SdoErrc::NO_PDO:
    // case SdoErrc::PDO_LEN:
    case SdoErrc::PARAM:
      return ::std::errc::invalid_argument;
    case SdoErrc::COMPAT:
      return ::std::errc::no_such_device_or_address;
    case SdoErrc::HARDWARE:
      return ::std::errc::io_error;
    case SdoErrc::TYPE_LEN:
    case SdoErrc::TYPE_LEN_HI:
    case SdoErrc::TYPE_LEN_LO:
      return ::std::errc::invalid_argument;
    case SdoErrc::NO_SUB:
      return ::std::errc::no_such_file_or_directory;
    case SdoErrc::PARAM_VAL:
      return ::std::errc::invalid_argument;
    case SdoErrc::PARAM_HI:
    case SdoErrc::PARAM_LO:
    case SdoErrc::PARAM_RANGE:
      return ::std::errc::result_out_of_range;
    case SdoErrc::NO_SDO:
      return ::std::errc::protocol_not_supported;
    // case SdoErrc::ERROR:
    case SdoErrc::DATA:
      return ::std::errc::io_error;
    case SdoErrc::DATA_CTL:
    case SdoErrc::DATA_DEV:
      return ::std::errc::device_or_resource_busy;
      // case SdoErrc::NO_OD:
#if !defined(__MINGW32__) || defined(ENODATA)
    case SdoErrc::NO_VAL:
      return ::std::errc::no_message_available;
#endif
    default:
      return ::std::error_condition(ev, *this);
  }
}

::std::string
SdoWhat(uint8_t id, uint16_t idx, uint8_t subidx, ::std::error_code ec,
        const ::std::string& what_arg = "") {
  ::std::stringstream ss;
  ss << ::std::uppercase << ::std::setfill('0') << ::std::hex;
  if (!what_arg.empty()) ss << what_arg << ':';
  ss << ::std::setw(2) << int(id) << ':';
  ss << ::std::setw(4) << idx << ':' << ::std::setw(2) << int(subidx) << ": ";
  ss << ec.message();
  ss << " (" << ::std::setw(8) << uint32_t(ec.value()) << ')';
  return ss.str();
}

}  // namespace

SdoError::SdoError(uint8_t id, uint16_t idx, uint8_t subidx,
                   ::std::error_code ec)
    : ::std::system_error(ec, SdoWhat(id, idx, subidx, ec)),
      id_(id),
      idx_(idx),
      subidx_(subidx) {}

SdoError::SdoError(uint8_t id, uint16_t idx, uint8_t subidx,
                   ::std::error_code ec, const ::std::string& what_arg)
    : ::std::system_error(ec, SdoWhat(id, idx, subidx, ec, what_arg)),
      id_(id),
      idx_(idx),
      subidx_(subidx) {}

SdoError::SdoError(uint8_t id, uint16_t idx, uint8_t subidx,
                   ::std::error_code ec, const char* what_arg)
    : ::std::system_error(ec, SdoWhat(id, idx, subidx, ec, what_arg)),
      id_(id),
      idx_(idx),
      subidx_(subidx) {}

SdoError::SdoError(uint8_t id, uint16_t idx, uint8_t subidx, int ev)
    : SdoError(id, idx, subidx, ::std::error_code(ev, SdoCategory())) {}

SdoError::SdoError(uint8_t id, uint16_t idx, uint8_t subidx, int ev,
                   const ::std::string& what_arg)
    : SdoError(id, idx, subidx, ::std::error_code(ev, SdoCategory()),
               what_arg) {}

SdoError::SdoError(uint8_t id, uint16_t idx, uint8_t subidx, int ev,
                   const char* what_arg)
    : SdoError(id, idx, subidx, ::std::error_code(ev, SdoCategory()),
               what_arg) {}

const ::std::error_category&
SdoCategory() noexcept {
  static const SdoErrcCategory category;
  return category;
}

::std::error_code
make_error_code(SdoErrc e) noexcept {
  return {static_cast<int>(e), SdoCategory()};
}

::std::error_condition
make_error_condition(SdoErrc e) noexcept {
  return {static_cast<int>(e), SdoCategory()};
}

SdoErrc
sdo_errc(::std::error_code ec) {
  if (ec) {
    if (ec.category() == SdoCategory()) {
      return static_cast<SdoErrc>(ec.value());
    } else if (ec.category() == ::std::generic_category()) {
      switch (static_cast<::std::errc>(ec.value())) {
        case ::std::errc::timed_out:
          return SdoErrc::TIMEOUT;
        case ::std::errc::not_enough_memory:
          return SdoErrc::NO_MEM;
        default:
          return SdoErrc::ERROR;
      }
    } else if (ec.category() == ::std::system_category()) {
      switch (errc2num(ec.value())) {
        case ERRNUM_TIMEDOUT:
          return SdoErrc::TIMEOUT;
        case ERRNUM_NOMEM:
          return SdoErrc::NO_MEM;
        default:
          return SdoErrc::ERROR;
      }
    }
    return SdoErrc::ERROR;
  }
  return static_cast<SdoErrc>(0);
}

::std::exception_ptr
make_sdo_exception_ptr(uint8_t id, uint16_t idx, uint8_t subidx,
                       ::std::error_code ec) noexcept {
  try {
    if (ec.category() == SdoCategory())
      return ::std::make_exception_ptr(SdoError(id, idx, subidx, ec));
    else
      return ::std::make_exception_ptr(::std::system_error(ec));
  } catch (...) {
    return ::std::current_exception();
  }
}

::std::exception_ptr
make_sdo_exception_ptr(uint8_t id, uint16_t idx, uint8_t subidx,
                       ::std::error_code ec,
                       const ::std::string& what_arg) noexcept {
  try {
    if (ec.category() == SdoCategory())
      return ::std::make_exception_ptr(SdoError(id, idx, subidx, ec, what_arg));
    else
      return ::std::make_exception_ptr(::std::system_error(ec, what_arg));
  } catch (...) {
    return ::std::current_exception();
  }
}

::std::exception_ptr
make_sdo_exception_ptr(uint8_t id, uint16_t idx, uint8_t subidx,
                       ::std::error_code ec, const char* what_arg) noexcept {
  try {
    if (ec.category() == SdoCategory())
      return ::std::make_exception_ptr(SdoError(id, idx, subidx, ec, what_arg));
    else
      return ::std::make_exception_ptr(::std::system_error(ec, what_arg));
  } catch (...) {
    return ::std::current_exception();
  }
}

}  // namespace canopen

}  // namespace lely
