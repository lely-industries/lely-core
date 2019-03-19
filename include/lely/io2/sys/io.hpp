/**@file
 * This header file is part of the I/O library; it contains the C++ interface
 * for the initialization/finalization functions.
 *
 * @see lely/io2/sys/io.h
 *
 * @copyright 2019 Lely Industries N.V.
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

#ifndef LELY_IO2_SYS_IO_HPP_
#define LELY_IO2_SYS_IO_HPP_

#include <lely/io2/sys/io.h>
#include <lely/util/error.hpp>

namespace lely {
namespace io {

/// A RAII-style wrapper around io_init() and io_fini().
class IoGuard {
 public:
  IoGuard() {
    if (io_init() == -1) util::throw_errc("IoGuard");
  }

  IoGuard(const IoGuard&) = delete;
  IoGuard& operator=(const IoGuard&) = delete;

  ~IoGuard() noexcept { io_fini(); }
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_SYS_IO_HPP_
