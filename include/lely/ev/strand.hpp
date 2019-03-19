/**@file
 * This header file is part of the event library; it contains the C++ interface
 * for the strand executor.
 *
 * @see lely/ev/strand.h
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

#ifndef LELY_EV_STRAND_HPP_
#define LELY_EV_STRAND_HPP_

#include <lely/ev/exec.hpp>
#include <lely/ev/strand.h>
#include <lely/util/error.hpp>

#include <utility>

namespace lely {
namespace ev {

/// A strand executor.
class Strand : public Executor {
 public:
  /// @see ev_strand_create()
  explicit Strand(Executor inner_exec)
      : Executor(ev_strand_create(inner_exec)) {
    if (!exec_) ::lely::util::throw_errc("Strand");
  }

  Strand(const Strand&) = delete;

  Strand(Strand&& other) noexcept : Executor(other) { other.exec_ = nullptr; }

  Strand& operator=(const Strand&) = delete;

  Strand&
  operator=(Strand&& other) noexcept {
    using ::std::swap;
    swap(exec_, other.exec_);
    return *this;
  }

  /// @see ev_strand_destroy()
  ~Strand() { ev_strand_destroy(exec_); }

  /// @see ev_strand_get_inner_exec()
  Executor
  get_inner_executor() const noexcept {
    return Executor(ev_strand_get_inner_exec(*this));
  }
};

}  // namespace ev
}  // namespace lely

#endif  // !LELY_EV_STRAND_HPP_
