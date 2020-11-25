/**@file
 * This header file is part of the compatibility library; it includes `<chrono>`
 * and defines any missing functionality.
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

#ifndef LELY_LIBC_CHRONO_HPP_
#define LELY_LIBC_CHRONO_HPP_

#include <lely/features.h>

#include <chrono>
#include <type_traits>

namespace lely {
namespace compat {

#if __cpp_lib_chrono >= 201907L

using ::std::chrono::clock_cast;

#else  // __cpp_lib_chrono < 201907L

template <class Dest, class Source, class Duration>
// clang-format off
inline typename ::std::enable_if<
    ::std::is_same<Dest, Source>::value,
    ::std::chrono::time_point<Dest, Duration>>::type
clock_cast(const ::std::chrono::time_point<Source, Duration>& t) {
  // clang-format on
  return t;
}

/**
 * Converts a time point with respects to <b>Source</b>'s epoch to one with
 * respect to <b>Dest</b>'s epoch.
 */
template <class Dest, class Source, class Duration>
// clang-format off
inline typename ::std::enable_if<
    !::std::is_same<Dest, Source>::value,
    ::std::chrono::time_point<Dest, Duration>>::type
clock_cast(const ::std::chrono::time_point<Source, Duration>& t) {
  // clang-format on
  using time_point = ::std::chrono::time_point<Dest, Duration>;
  auto d1 = Dest::now().time_since_epoch();
  auto s = Source::now().time_since_epoch();
  auto d2 = Dest::now().time_since_epoch();
  return time_point{t.time_since_epoch() + ((d1 - s) + (d2 - s)) / 2};
}

#endif  // __cpp_lib_chrono < 201907L

}  // namespace compat
}  // namespace lely

#endif  // !LELY_LIBC_CHRONO_HPP_
