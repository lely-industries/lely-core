/**@file
 * This header file is part of the utilities library; it contains a function
 * object that can be used to store a Callable together with its arguments.
 *
 * @copyright 2019 Lely Industries N.V.
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

#ifndef LELY_UTIL_INVOKER_HPP_
#define LELY_UTIL_INVOKER_HPP_

#include <lely/libc/functional.hpp>
#include <lely/libc/utility.hpp>

#include <tuple>
#include <utility>

namespace lely {
namespace util {

namespace detail {

template <class Tuple>
class invoker {
  Tuple tuple_;

  template <::std::size_t... I>
  auto
  invoke_(compat::index_sequence<I...>) noexcept(
      noexcept(compat::invoke(::std::get<I>(::std::move(tuple_))...)))
      -> decltype(compat::invoke(::std::get<I>(::std::move(tuple_))...)) {
    return compat::invoke(::std::get<I>(::std::move(tuple_))...);
  }

  using sequence_ =
      compat::make_index_sequence<::std::tuple_size<Tuple>::value>;

 public:
  template <class F, class... Args,
            class = typename ::std::enable_if<!::std::is_same<
                typename ::std::decay<F>::type, invoker>::value>::type>
  explicit invoker(F&& f, Args&&... args)
      : tuple_{::std::forward<F>(f), ::std::forward<Args>(args)...} {}

  auto
  operator()() noexcept(
      noexcept(::std::declval<invoker&>().invoke_(sequence_{})))
      -> decltype(::std::declval<invoker&>().invoke_(sequence_{})) {
    return invoke_(sequence_{});
  }
};

}  // namespace detail

/**
 * A helper alias template for the result of
 * #lely::util::make_invoker<F, Args...>().
 */
template <class F, class... Args>
using invoker_t =
    detail::invoker<::std::tuple<typename ::std::decay<F>::type,
                                 typename ::std::decay<Args>::type...>>;

/**
 * Creates a function object containing a Callable and its arguments. The
 * resulting function object is callable without any arguments.
 */
template <class F, class... Args>
inline invoker_t<F, Args...>
make_invoker(F&& f, Args&&... args) {
  return invoker_t<F, Args...>{::std::forward<F>(f),
                               ::std::forward<Args>(args)...};
}

}  // namespace util
}  // namespace lely

#endif  // !LELY_UTIL_INVOKER_HPP_
