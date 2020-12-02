/**@file
 * This file is part of the utilities library; it contains the implementation of
 * the C++ exception functions.
 *
 * @see lely/util/exception.hpp
 *
 * @copyright 2016-2020 Lely Industries N.V.
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

// NOLINTNEXTLINE(build/include)
#include "util.h"

#if !LELY_NO_CXX

#include <lely/util/exception.hpp>

#include <cstdlib>

extern "C" {

_Noreturn void
__throw_or_abort(const char* what) noexcept {
  (void)what;

  ::std::abort();
}
}

#endif
