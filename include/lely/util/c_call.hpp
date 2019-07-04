// NOLINT(legal/copyright)
/**@file
 * This header file is part of the utilities library; it contains the C callback
 * wrapper declarations.
 *
 * C callback functions are typically free functions of the form
 * `R function(ArgTypes... args, void* data)`. The last argument is an arbitrary
 * pointer which can be specified by the user and is passed unmodified to the
 * callback function. The pointer can be used to allow C++ function objects and
 * member functions to be used as callbacks.
 *
 * Take
 *
 *     extern "C" {
 *         typedef R func_t(ArgTypes... args, void* data);
 *         void set_func(func_t *func, void *data);
 *     }
 *
 * to be the definition of a C callback function, with `set_func()` the function
 * used to register a callback. If `F` is type type of a function object (_not_
 * an actual function, but a class with `R operatoR (*)(ArgTypes... args)` as a
 * member), then an instance `f` can be registered as callback by invoking
 *
 *     set_func(&c_obj_call<func_t*, F>::function, static_cast<void*>(f))
 *
 * However, a member function does not have to be named `operatoR (*)()` to be
 * used as a callback. If a class `C` has a member function
 * `R func(ArgTypes... args)`, then this function (belonging to an instance
 * `obj`) can be registered with
 *
 *     set_func(&c_mem_call<func_t*, C, &C::func>::function,
 * static_cast<void*>(obj))
 *
 * The definition of #lely::c_mem_call makes use of the fact that member
 * function pointers can be template parameters.
 *
 * @copyright 2017-2019 Lely Industries N.V.
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

#ifndef LELY_UTIL_C_CALL_HPP_
#define LELY_UTIL_C_CALL_HPP_

#include <lely/util/util.h>

namespace lely {

template <class, class>
struct c_obj_call;

template <class, class>
struct c_mem_fn;
template <class F, class C, typename c_mem_fn<F, C>::type>
struct c_mem_call;

namespace impl {

template <class...>
struct c_pack;

template <class, class>
struct c_pack_push_front;
/// Pushes a type to the front of a parameter pack.
template <class T, class... S>
struct c_pack_push_front<T, c_pack<S...>> {
  using type = c_pack<T, S...>;
};

/// Pops a type from the back of a parameter pack.
template <class T, class... S>
struct c_pack_pop_back
    : c_pack_push_front<T, typename c_pack_pop_back<S...>::type> {};
/// Pops a type from the back of a parameter pack.
template <class T, class S>
struct c_pack_pop_back<T, S> {
  using type = c_pack<T>;
};

}  // namespace impl

/**
 * Provides a C wrapper for a function object with an arbitrary number of
 * arguments.
 */
template <class R, class... ArgTypes, class F>
struct c_obj_call<impl::c_pack<R, ArgTypes...>, F> {
  static R
  function(ArgTypes... args, void* data) noexcept {
    return (*static_cast<F*>(data))(args...);
  }
};

/**
 * Provides a C wrapper for a function object with an arbitrary number of
 * arguments.
 */
template <class R, class... ArgTypes, class F>
struct c_obj_call<R (*)(ArgTypes...), F>
    : c_obj_call<typename impl::c_pack_pop_back<R, ArgTypes...>::type, F> {};

/**
 * A class template supplying the type of a member function with an arbitrary
 * number of arguments.
 */
template <class R, class... ArgTypes, class C>
struct c_mem_fn<impl::c_pack<R, ArgTypes...>, C> {
  using type = R (C::*)(ArgTypes...);
};

/**
 * A class template supplying the type of a member function with an arbitrary
 * number of arguments.
 */
template <class R, class... ArgTypes, class C>
struct c_mem_fn<R (*)(ArgTypes...), C>
    : c_mem_fn<typename impl::c_pack_pop_back<R, ArgTypes...>::type, C> {};

/**
 * Provides a C wrapper for a member function with an arbitrary number of
 * arguments.
 */
template <class R, class... ArgTypes, class C,
          typename c_mem_fn<impl::c_pack<R, ArgTypes...>, C>::type M>
struct c_mem_call<impl::c_pack<R, ArgTypes...>, C, M> {
  static R
  function(ArgTypes... args, void* data) noexcept {
    return ((*static_cast<C*>(data)).*M)(args...);
  }
};

/**
 * Provides a C wrapper for a member function with an arbitrary number of
 * arguments.
 */
template <class R, class... ArgTypes, class C,
          typename c_mem_fn<R (*)(ArgTypes...), C>::type M>
struct c_mem_call<R (*)(ArgTypes...), C, M>
    : c_mem_call<typename impl::c_pack_pop_back<R, ArgTypes...>::type, C, M> {};

}  // namespace lely

#endif  // !LELY_UTIL_C_CALL_HPP_
