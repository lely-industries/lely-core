/**@file
 * This header file is part of the I/O library; it contains the C++ interface of
 * the I/O device handle. @see lely/io/io.h for the C interface.
 *
 * @copyright 2016-2018 Lely Industries N.V.
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

#ifndef LELY_IO_IO_HPP_
#define LELY_IO_IO_HPP_

#ifndef __cplusplus
#error "include <lely/io/io.h> for the C interface"
#endif

#include <lely/util/c_type.hpp>
#include <lely/io/io.h>

namespace lely {

/// An I/O device handle.
class IOHandle {
 public:
  operator io_handle_t() const noexcept { return m_handle; }

  IOHandle() noexcept : m_handle(IO_HANDLE_ERROR) {}

  IOHandle(const IOHandle& handle) noexcept
      : m_handle(io_handle_acquire(handle.m_handle)) {}

  IOHandle(IOHandle&& handle) noexcept : m_handle(handle.m_handle) {
    handle.m_handle = IO_HANDLE_ERROR;
  }

  virtual ~IOHandle() { io_handle_release(m_handle); }

  IOHandle&
  operator=(const IOHandle& handle) noexcept {
    if (this != &handle) {
      io_handle_t tmp = io_handle_acquire(handle.m_handle);
      io_handle_release(m_handle);
      m_handle = tmp;
    }
    return *this;
  }

  IOHandle&
  operator=(IOHandle&& handle) noexcept {
    io_handle_release(m_handle);
    m_handle = handle.m_handle;
    handle.m_handle = IO_HANDLE_ERROR;
    return *this;
  }

  operator bool() const noexcept { return m_handle != IO_HANDLE_ERROR; }

  bool
  unique() const noexcept {
    return !!io_handle_unique(m_handle);
  }

  int
  close() noexcept {
    io_handle_t handle = m_handle;
    m_handle = IO_HANDLE_ERROR;
    return io_close(handle);
  }

  int
  getType() const noexcept {
    return io_get_type(*this);
  }

#ifdef _WIN32
  HANDLE
  getFd() const noexcept { return io_get_fd(*this); }
#else
  int
  getFd() const noexcept {
    return io_get_fd(*this);
  }
#endif

  int
  getFlags() const noexcept {
    return io_get_flags(*this);
  }

  int
  setFlags(int flags) noexcept {
    return io_set_flags(*this, flags);
  }

  ssize_t
  read(void* buf, size_t nbytes) noexcept {
    return io_read(*this, buf, nbytes);
  }

  ssize_t
  write(const void* buf, size_t nbytes) noexcept {
    return io_write(*this, buf, nbytes);
  }

  int
  flush() noexcept {
    return io_flush(*this);
  }

 protected:
  IOHandle(io_handle_t handle) noexcept : m_handle(handle) {}

 private:
  io_handle_t m_handle;
};

}  // namespace lely

#endif  // !LELY_IO_IO_HPP_
