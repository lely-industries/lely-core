/**@file
 * This header file is part of the I/O library; it contains the standard C++
 * system clock definitions.
 *
 * @see lely/io2/sys/clock.h, lely/io2/clock.hpp
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

#ifndef LELY_IO2_SYS_CLOCK_HPP_
#define LELY_IO2_SYS_CLOCK_HPP_

#include <lely/io2/sys/clock.h>
#include <lely/io2/clock.hpp>

namespace lely {
namespace io {

/// @see IO_CLOCK_REALTIME
constexpr Clock clock_realtime{IO_CLOCK_REALTIME};

/// @see IO_CLOCK_MONOTONIC
constexpr Clock clock_monotonic{IO_CLOCK_MONOTONIC};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_SYS_CLOCK_HPP_
