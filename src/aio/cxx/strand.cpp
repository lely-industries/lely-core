/*!\file
 * This file is part of the C++ asynchronous I/O library; it contains ...
 *
 * \see lely/aio/strand.hpp
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

#include "aio.hpp"

#if !LELY_NO_CXX

#include <lely/aio/strand.hpp>

#include <lely/aio/strand.h>

namespace lely {

namespace aio {

LELY_AIO_EXPORT Strand::Strand(ExecutorBase& exec)
    : ExecutorBase(InvokeC("Strand", aio_strand_create, exec)) {}

LELY_AIO_EXPORT Strand::~Strand() { aio_strand_destroy(*this); }

LELY_AIO_EXPORT ExecutorBase
Strand::GetInnerExecutor() const noexcept {
  return aio_strand_get_inner_exec(*this);
}

}  // namespace aio

}  // namespace lely

#endif  // !LELY_NO_CXX
