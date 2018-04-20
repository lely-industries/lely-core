/*!\file
 * This header file is part of the C++ asynchronous I/O library; it contains ...
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

#ifndef LELY_AIO_STRAND_HPP_
#define LELY_AIO_STRAND_HPP_

#include <lely/aio/exec.hpp>

namespace lely {

namespace aio {

class LELY_AIO_EXTERN Strand : public ExecutorBase {
 public:
  explicit Strand(ExecutorBase& exec);

  Strand(const Strand&) = delete;

  Strand(Strand&& other) : ExecutorBase(other.c_ptr) {
    other.c_ptr = nullptr;
  }

  Strand& operator=(const Strand&) = delete;

  Strand&
  operator=(Strand&& other) {
    ::std::swap(c_ptr, other.c_ptr);
    return *this;
  }

  ~Strand();

  ExecutorBase GetInnerExecutor() const noexcept;
};

}  // namespace aio

}  // namespace lely

#endif  // LELY_AIO_STRAND_HPP_
