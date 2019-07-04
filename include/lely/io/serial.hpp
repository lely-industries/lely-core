/**@file
 * This header file is part of the I/O library; it contains the C++ interface of
 * the serial I/O device handle. @see lely/io/serial.h for the C interface.
 *
 * @copyright 2016-2019 Lely Industries N.V.
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

#ifndef LELY_IO_SERIAL_HPP_
#define LELY_IO_SERIAL_HPP_

#ifndef __cplusplus
#error "include <lely/io/serial.h> for the C interface"
#endif

#include <lely/io/io.hpp>
#include <lely/io/serial.h>

#include <utility>

namespace lely {

/// A serial I/O device handle.
class IOSerial : public IOHandle {
 public:
  IOSerial(const char* path, io_attr_t* attr = 0)
      : IOHandle(io_open_serial(path, attr)) {
    if (!operator bool()) throw_or_abort(bad_init());
  }

  IOSerial(const IOSerial& serial) noexcept : IOHandle(serial) {}

  IOSerial(IOSerial&& serial) noexcept
      : IOHandle(::std::forward<IOSerial>(serial)) {}

  IOSerial&
  operator=(const IOSerial& serial) noexcept {
    IOHandle::operator=(serial);
    return *this;
  }

  IOSerial&
  operator=(IOSerial&& serial) noexcept {
    IOHandle::operator=(::std::forward<IOSerial>(serial));
    return *this;
  }

  int
  purge(int flags) noexcept {
    return io_purge(*this, flags);
  }

  int
  getAttr(io_attr_t& attr) noexcept {
    return io_serial_get_attr(*this, &attr);
  }

  int
  setAttr(const io_attr_t& attr) noexcept {
    return io_serial_set_attr(*this, &attr);
  }
};

}  // namespace lely

#endif  // !LELY_IO_SERIAL_HPP_
