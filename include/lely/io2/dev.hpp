/**@file
 * This header file is part of the I/O library; it contains the C++ interface
 * for the abstract I/O device.
 *
 * @see lely/io2/dev.h
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

#ifndef LELY_IO2_DEV_HPP_
#define LELY_IO2_DEV_HPP_

#include <lely/ev/exec.hpp>
#include <lely/io2/ctx.hpp>
#include <lely/io2/dev.h>

namespace lely {
namespace io {

/// An abstract I/O device. This class is a wrapper around `#io_dev_t*`.
class Device {
 public:
  explicit Device(io_dev_t* dev_) noexcept : dev(dev_) {}

  operator io_dev_t*() const noexcept { return dev; }

  /// @see io_dev_get_ctx()
  ContextBase
  get_ctx() const noexcept {
    return ContextBase(io_dev_get_ctx(*this));
  }

  /// @see io_dev_get_exec()
  ev::Executor
  get_executor() const noexcept {
    return ev::Executor(io_dev_get_exec(*this));
  }

  /// @see io_dev_cancel()
  bool
  cancel(ev_task& task) noexcept {
    return io_dev_cancel(*this, &task) != 0;
  }

  /// @see io_dev_cancel()
  ::std::size_t
  cancel_all() noexcept {
    return io_dev_cancel(*this, nullptr);
  }

  /// @see io_dev_abort()
  bool
  abort(ev_task& task) noexcept {
    return io_dev_abort(*this, &task) != 0;
  }

  /// @see io_dev_abort()
  ::std::size_t
  abort_all() noexcept {
    return io_dev_abort(*this, nullptr);
  }

 protected:
  io_dev_t* dev{nullptr};
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_DEV_HPP_
