/**@file
 * This header file is part of the I/O library; it contains the C++ interface of
 * the pipe device handle. @see lely/io/pipe.h for the C interface.
 *
 * @copyright 2017-2019 Lely Industries N.V.
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

#ifndef LELY_IO_PIPE_HPP_
#define LELY_IO_PIPE_HPP_

#ifndef __cplusplus
#error "include <lely/io/pipe.h> for the C interface"
#endif

#include <lely/io/io.hpp>
#include <lely/io/pipe.h>

#include <utility>

namespace lely {

/// A pipe I/O device handle.
class IOPipe : public IOHandle {
 public:
  IOPipe() = default;

  IOPipe(const IOPipe& pipe) noexcept : IOHandle(pipe) {}

  IOPipe(IOPipe&& pipe) noexcept : IOHandle(::std::forward<IOPipe>(pipe)) {}

  IOPipe&
  operator=(const IOPipe& pipe) noexcept {
    IOHandle::operator=(pipe);
    return *this;
  }

  IOPipe&
  operator=(IOPipe&& pipe) noexcept {
    IOHandle::operator=(::std::forward<IOPipe>(pipe));
    return *this;
  }

  static int
  open(IOPipe pipe[2]) noexcept {
    io_handle_t handle_vector[2];
    if (io_open_pipe(handle_vector) == -1) return -1;
    pipe[0] = IOPipe(handle_vector[0]);
    pipe[1] = IOPipe(handle_vector[1]);
    return 0;
  }

 protected:
  IOPipe(io_handle_t handle) noexcept : IOHandle(handle) {}
};

}  // namespace lely

#endif  // !LELY_IO_PIPE_HPP_
