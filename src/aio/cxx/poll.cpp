/**@file
 * This file is part of the C++ asynchronous I/O library; it contains ...
 *
 * @see lely/aio/poll.hpp
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

#include "aio.hpp"

#if !LELY_NO_CXX

#include <lely/aio/poll.hpp>

namespace lely {

namespace aio {

::std::size_t
PollBase::Wait(int timeout) {
  return InvokeC("Wait", aio_poll_wait, *this, timeout);
}

::std::size_t
PollBase::Wait(int timeout, ::std::error_code& ec) {
  return InvokeC(ec, aio_poll_wait, *this, timeout);
}

void
PollBase::Stop() {
  aio_poll_stop(*this);
}

}  // namespace aio

}  // namespace lely

#endif  // !LELY_NO_CXX
