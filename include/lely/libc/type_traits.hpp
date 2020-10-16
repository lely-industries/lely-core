/**@file
 * This header file is part of the compatibility library; it includes
 * `<type_traits>` and defines any missing functionality.
 *
 * @copyright 2018-2020 Lely Industries N.V.
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

#ifndef LELY_LIBC_TYPE_TRAITS_HPP_
#define LELY_LIBC_TYPE_TRAITS_HPP_

#include <lely/features.h>

#include <type_traits>
#include <utility>

namespace lely {
namespace compat {

#if __cplusplus <= 201703L

template <class T>
struct remove_cvref {
  using type = typename ::std::remove_cv<
      typename ::std::remove_reference<T>::type>::type;
};

template <class T>
using remove_cvref_t = typename remove_cvref<T>::type;

#endif  // __cplusplus <= 201703L

#if __cplusplus >= 201703L

using ::std::bool_constant;

using ::std::invoke_result;
using ::std::invoke_result_t;

using ::std::is_invocable;
using ::std::is_invocable_r;

using ::std::void_t;

using ::std::conjunction;
using ::std::disjunction;
using ::std::negation;

#else  // __cplusplus < 201703L

template <bool B>
using bool_constant = ::std::integral_constant<bool, B>;

namespace detail {

template <class T>
struct is_reference_wrapper : ::std::false_type {};

template <class U>
struct is_reference_wrapper<::std::reference_wrapper<U>> : ::std::true_type {};

template <class T>
struct invoke_impl {
  template <class F, class... Args>
  static auto
  call(F&& f, Args&&... args)
      -> decltype(::std::forward<F>(f)(::std::forward<Args>(args)...)) {
    return ::std::forward<F>(f)(::std::forward<Args>(args)...);
  }

  template <class F, class... Args>
  static constexpr bool
  is_nothrow() {
    return noexcept(::std::declval<F>()(::std::declval<Args>()...));
  }
};

template <class MT, class B>
struct invoke_impl<MT B::*> {
  template <
      class T, class Td = typename ::std::decay<T>::type,
      class = typename ::std::enable_if<::std::is_base_of<B, Td>::value>::type>
  static T&&
  get(T&& t) {
    return static_cast<T&&>(t);
  }

  template <
      class T, class Td = typename ::std::decay<T>::type,
      class = typename ::std::enable_if<is_reference_wrapper<Td>::value>::type>
  static auto
  get(T&& t) -> decltype(t.get()) {
    return t.get();
  }

  template <
      class T, class Td = typename ::std::decay<T>::type,
      class = typename ::std::enable_if<!::std::is_base_of<B, Td>::value>::type,
      class = typename ::std::enable_if<!is_reference_wrapper<Td>::value>::type>
  static auto
  get(T&& t) -> decltype(*::std::forward<T>(t)) {
    return *::std::forward<T>(t);
  }

  template <
      class T, class... Args, class MT1,
      class = typename ::std::enable_if<::std::is_function<MT1>::value>::type>
  static auto
  call(MT1 B::*pmf, T&& t, Args&&... args)
      -> decltype((invoke_impl::get(::std::forward<T>(t)).*
                   pmf)(::std::forward<Args>(args)...)) {
    return (invoke_impl::get(::std::forward<T>(t)).*
            pmf)(::std::forward<Args>(args)...);
  }

  template <class T>
  static auto
  call(MT B::*pmd, T&& t)
      -> decltype(invoke_impl::get(::std::forward<T>(t)).*pmd) {
    return invoke_impl::get(::std::forward<T>(t)).*pmd;
  }
};

template <class F, class... Args, class Fd = typename ::std::decay<F>::type>
inline auto
invoke(F&& f, Args&&... args)
    -> decltype(invoke_impl<Fd>::call(::std::forward<F>(f),
                                      ::std::forward<Args>(args)...)) {
  return invoke_impl<Fd>::call(::std::forward<F>(f),
                               ::std::forward<Args>(args)...);
}

template <class, class, class...>
struct invoke_result {};

template <class F, class... Args>
struct invoke_result<decltype(void(invoke(::std::declval<F>(),
                                          ::std::declval<Args>()...))),
                     F, Args...> {
  using type = decltype(invoke(::std::declval<F>(), ::std::declval<Args>()...));
};

}  // namespace detail

/**
 * Deduces the return type of an INVOKE expression at compile time. If the
 * expression `INVOKE(declval<F>(), declval<Args>()...)` is well-formed when
 * treated as an unevaluated operand, the member typedef <b>type</b> names the
 * type `decltype(INVOKE(declval<F>(), declval<Args>()...))`; otherwise, there
 * shall be no member <b>type</b>.
 */
template <class F, class... Args>
struct invoke_result : detail::invoke_result<void, F, Args...> {};

template <class F, class... Args>
using invoke_result_t = typename invoke_result<F, Args...>::type;

namespace detail {

template <class>
struct make_void {
  using type = void;
};

}  // namespace detail

/**
 * Utility metafunction that maps a sequence of any types to the type `void`.
 * The standard definition, `template <class...> using void_t = void;`, does not
 * work on GCC 4.9.
 */
template <class... T>
using void_t = typename detail::make_void<T...>::type;

namespace detail {

template <class, class, class = void>
struct is_invocable : ::std::false_type {};

template <class Result, class R>
struct is_invocable<Result, R, typename make_void<typename Result::type>::type>
    : bool_constant<::std::is_void<R>::value ||
                    ::std::is_convertible<typename Result::type, R>::value> {};

}  // namespace detail

/**
 * Determines whether <b>F</b> can be invoked with the arguments <b>Args...</b>.
 * Formally, determines whether `INVOKE(declval<F>(), declval<Args>()...)` is
 * well-formed when treated as an unevaluated operand.
 */
template <class F, class... Args>
struct is_invocable : detail::is_invocable<invoke_result<F, Args...>, void> {};

/**
 * Determines whether <b>F</b> can be invoked with the arguments <b>Args...</b>
 * to yield a result that is convertable to <b>R</b>. Formally, determines
 * whether `INVOKE<R>(declval<F>(), declval<Args>()...)` is well-formed when
 * treated as an unevaluated operand.
 */
template <class R, class F, class... Args>
struct is_invocable_r : detail::is_invocable<invoke_result<F, Args...>, R> {};

/**
 * Forms the logical conjunction of the type traits <b>B...</b>, effectively
 * performing a logical AND on the sequence of traits.
 */
template <class... B>
struct conjunction : ::std::true_type {};

template <class B1>
struct conjunction<B1> : B1 {};

template <class B1, class... Bn>
struct conjunction<B1, Bn...>
    : ::std::conditional<static_cast<bool>(B1::value), conjunction<Bn...>,
                         B1>::type {};

/**
 * Forms the logical disjunction of the type traits <b>B...</b>, effectively
 * performing a logical OR on the sequence of traits.
 */
template <class... B>
struct disjunction : ::std::false_type {};

template <class B1>
struct disjunction<B1> : B1 {};

template <class B1, class... Bn>
struct disjunction<B1, Bn...>
    : ::std::conditional<static_cast<bool>(B1::value), B1,
                         disjunction<Bn...>>::type {};

/// Forms the logical negation of the type trait <b>B</b>.
template <class B>
struct negation : bool_constant<!static_cast<bool>(B::value)> {};

#endif  // __cplusplus < 201703L

}  // namespace compat
}  // namespace lely

#endif  // !LELY_LIBC_TYPE_TRAITS_HPP_
