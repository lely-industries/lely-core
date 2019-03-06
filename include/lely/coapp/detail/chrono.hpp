/**@file
 * This header file is part of the C++ CANopen application library; it contains
 * the timeout conversion functions.
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

#ifndef LELY_COAPP_DETAIL_CHRONO_HPP_
#define LELY_COAPP_DETAIL_CHRONO_HPP_

#include <lely/coapp/coapp.hpp>

#include <chrono>
#include <limits>

namespace lely {

namespace canopen {

namespace detail {

/// Converts an SDO timeout to a duration.
inline ::std::chrono::milliseconds
FromTimeout(int timeout) {
  using namespace ::std::chrono;
  return timeout <= 0 ? milliseconds::max() : milliseconds(timeout);
}

/// Converts a duration to an SDO timeout.
template <class Rep, class Period>
inline int
ToTimeout(const ::std::chrono::duration<Rep, Period>& d) {
  using namespace ::std::chrono;
  // The maximum duration is interpreted as an infinite timeout.
  if (d == duration<Rep, Period>::max()) return 0;
  auto timeout = duration_cast<milliseconds>(d).count();
  // A timeout less than 1 ms is rounded up to keep it finite.
  if (timeout < 1) return 1;
  if (timeout > ::std::numeric_limits<int>::max())
    return ::std::numeric_limits<int>::max();
  return timeout;
}

}  // namespace detail

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_DETAIL_CHRONO_HPP_
