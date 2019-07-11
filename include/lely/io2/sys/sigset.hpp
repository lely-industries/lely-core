/**@file
 * This header file is part of the I/O library; it contains the C++ interface
 * for the system signal handler.
 *
 * @see lely/io2/sys/sigset.h
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

#ifndef LELY_IO2_SYS_SIGSET_HPP_
#define LELY_IO2_SYS_SIGSET_HPP_

#include <lely/io2/sys/sigset.h>
#include <lely/io2/sigset.hpp>

#include <utility>

namespace lely {
namespace io {

/// A system signal handler.
class SignalSet : public SignalSetBase {
 public:
  /// @see io_sigset_create()
  SignalSet(io_poll_t* poll, ev_exec_t* exec)
      : SignalSetBase(io_sigset_create(poll, exec)) {
    if (!sigset) util::throw_errc("SignalSet");
  }

  SignalSet(const SignalSet&) = delete;

  SignalSet(SignalSet&& other) noexcept : SignalSetBase(other.sigset) {
    other.sigset = nullptr;
    other.dev = nullptr;
  }

  SignalSet& operator=(const SignalSet&) = delete;

  SignalSet&
  operator=(SignalSet&& other) noexcept {
    using ::std::swap;
    swap(sigset, other.sigset);
    swap(dev, other.dev);
    return *this;
  }

  /// @see io_sigset_destroy()
  ~SignalSet() { io_sigset_destroy(*this); }
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_SYS_SIGSET_HPP_
