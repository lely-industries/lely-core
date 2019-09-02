/**@file
 * This header file is part of the compatibility library; it includes
 * `<functional>` and defines any missing functionality.
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

#ifndef LELY_LIBC_FUNCTIONAL_HPP_
#define LELY_LIBC_FUNCTIONAL_HPP_

#include <lely/features.h>
#if __cplusplus <= 201703L
#include <lely/libc/type_traits.hpp>
#endif

#include <functional>
#include <utility>

namespace lely {
namespace compat {

#if __cplusplus >= 201703L

using ::std::invoke;

#else  // __cplusplus < 201703L

/**
 * Invokes <b>f</b> with the arguments <b>args...</b> as if by
 * `INVOKE(forward<F>(f), forward<Args>(args)...)`.
 */
template <class F, class... Args>
inline invoke_result_t<F, Args...>
invoke(F&& f, Args&&... args) {
  return detail::invoke(::std::forward<F>(f), ::std::forward<Args>(args)...);
}

#endif  // __cplusplus < 201703L

}  // namespace compat
}  // namespace lely

#endif  // !LELY_LIBC_FUNCTIONAL_HPP_
