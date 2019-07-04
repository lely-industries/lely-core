/**@file
 * This header file is part of the I/O library; it contains the C++ interface of
 * the regular file handle. @see lely/io/file.h for the C interface.
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

#ifndef LELY_IO_FILE_HPP_
#define LELY_IO_FILE_HPP_

#ifndef __cplusplus
#error "include <lely/io/file.h> for the C interface"
#endif

#include <lely/io/file.h>
#include <lely/io/io.hpp>

#include <utility>

namespace lely {

/// A regular file device handle.
class IOFile : public IOHandle {
 public:
  IOFile(const char* path, int flags) : IOHandle(io_open_file(path, flags)) {
    if (!operator bool()) throw_or_abort(bad_init());
  }

  IOFile(const IOFile& file) noexcept : IOHandle(file) {}

  IOFile(IOFile&& file) noexcept : IOHandle(::std::forward<IOFile>(file)) {}

  IOFile&
  operator=(const IOFile& file) noexcept {
    IOHandle::operator=(file);
    return *this;
  }

  IOFile&
  operator=(IOFile&& file) noexcept {
    IOHandle::operator=(::std::forward<IOFile>(file));
    return *this;
  }

  io_off_t
  seek(io_off_t offset, int whence) noexcept {
    return io_seek(*this, offset, whence);
  }

  ssize_t
  pread(void* buf, size_t nbytes, io_off_t offset) noexcept {
    return io_pread(*this, buf, nbytes, offset);
  }

  ssize_t
  pwrite(const void* buf, size_t nbytes, io_off_t offset) noexcept {
    return io_pwrite(*this, buf, nbytes, offset);
  }
};

}  // namespace lely

#endif  // !LELY_IO_FILE_HPP_
