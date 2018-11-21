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
 * @copyright 2017-2018 Lely Industries N.V.
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

#ifndef LELY_UTIL_C_CALL_HPP
#define LELY_UTIL_C_CALL_HPP

#include <lely/util/util.h>

namespace lely {

template <class, class>
struct c_obj_call;

template <class, class>
struct c_mem_fn;
template <class F, class C, typename c_mem_fn<F, C>::type>
struct c_mem_call;

#if __cplusplus >= 201103L

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

#else

/// Provides a C wrapper for a function object taking no arguments.
template <class R, class F>
struct c_obj_call<R (*)(void*), F> {
  static R
  function(void* data) {
    return (*static_cast<F*>(data))();
  }
};

/// Provides a C wrapper for a function object taking one argument.
template <class R, class T0, class F>
struct c_obj_call<R (*)(T0, void*), F> {
  static R
  function(T0 t0, void* data) {
    return (*static_cast<F*>(data))(t0);
  }
};

/// Provides a C wrapper for a function object taking two arguments.
template <class R, class T0, class T1, class F>
struct c_obj_call<R (*)(T0, T1, void*), F> {
  static R
  function(T0 t0, T1 t1, void* data) {
    return (*static_cast<F*>(data))(t0, t1);
  }
};

/// Provides a C wrapper for a function object taking three arguments.
template <class R, class T0, class T1, class T2, class F>
struct c_obj_call<R (*)(T0, T1, T2, void*), F> {
  static R
  function(T0 t0, T1 t1, T2 t2, void* data) {
    return (*static_cast<F*>(data))(t0, t1, t2);
  }
};

/// Provides a C wrapper for a function object taking four arguments.
template <class R, class T0, class T1, class T2, class T3, class F>
struct c_obj_call<R (*)(T0, T1, T2, T3, void*), F> {
  static R
  function(T0 t0, T1 t1, T2 t2, T3 t3, void* data) {
    return (*static_cast<F*>(data))(t0, t1, t2, t3);
  }
};

/// Provides a C wrapper for a function object taking five arguments.
template <class R, class T0, class T1, class T2, class T3, class T4, class F>
struct c_obj_call<R (*)(T0, T1, T2, T3, T4, void*), F> {
  static R
  function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, void* data) {
    return (*static_cast<F*>(data))(t0, t1, t2, t3, t4);
  }
};

/// Provides a C wrapper for a function object taking six arguments.
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class F>
struct c_obj_call<R (*)(T0, T1, T2, T3, T4, T5, void*), F> {
  static R
  function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, void* data) {
    return (*static_cast<F*>(data))(t0, t1, t2, t3, t4, t5);
  }
};

/// Provides a C wrapper for a function object taking seven arguments.
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class F>
struct c_obj_call<R (*)(T0, T1, T2, T3, T4, T5, T6, void*), F> {
  static R
  function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, void* data) {
    return (*static_cast<F*>(data))(t0, t1, t2, t3, t4, t5, t6);
  }
};

/// Provides a C wrapper for a function object taking eight arguments.
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class T7, class F>
struct c_obj_call<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, void*), F> {
  static R
  function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, void* data) {
    return (*static_cast<F*>(data))(t0, t1, t2, t3, t4, t5, t6, t7);
  }
};

/// Provides a C wrapper for a function object taking nine arguments.
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class T7, class T8, class F>
struct c_obj_call<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, void*), F> {
  static R
  function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8,
           void* data) {
    return (*static_cast<F*>(data))(t0, t1, t2, t3, t4, t5, t6, t7, t8);
  }
};

/// Provides a C wrapper for a function object taking ten arguments.
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class T7, class T8, class T9, class F>
struct c_obj_call<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, void*), F> {
  static R
  function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9,
           void* data) {
    return (*static_cast<F*>(data))(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9);
  }
};

/// A class template supplying the type of a member function with no arguments.
template <class R, class C>
struct c_mem_fn<R (*)(void*), C> {
  typedef R (C::*type)();
};

/// A class template supplying the type of a member function with one arguments.
template <class R, class T0, class C>
struct c_mem_fn<R (*)(T0, void*), C> {
  typedef R (C::*type)(T0);
};

/// A class template supplying the type of a member function with two arguments.
template <class R, class T0, class T1, class C>
struct c_mem_fn<R (*)(T0, T1, void*), C> {
  typedef R (C::*type)(T0, T1);
};

/**
 * A class template supplying the type of a member function with three
 * arguments.
 */
template <class R, class T0, class T1, class T2, class C>
struct c_mem_fn<R (*)(T0, T1, T2, void*), C> {
  typedef R (C::*type)(T0, T1, T2);
};

/**
 * A class template supplying the type of a member function with four arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class C>
struct c_mem_fn<R (*)(T0, T1, T2, T3, void*), C> {
  typedef R (C::*type)(T0, T1, T2, T3);
};

/**
 * A class template supplying the type of a member function with five arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class C>
struct c_mem_fn<R (*)(T0, T1, T2, T3, T4, void*), C> {
  typedef R (C::*type)(T0, T1, T2, T3, T4);
};

/// A class template supplying the type of a member function with six arguments.
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class C>
struct c_mem_fn<R (*)(T0, T1, T2, T3, T4, T5, void*), C> {
  typedef R (C::*type)(T0, T1, T2, T3, T4, T5);
};

/**
 * A class template supplying the type of a member function with seven
 * arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class C>
struct c_mem_fn<R (*)(T0, T1, T2, T3, T4, T5, T6, void*), C> {
  typedef R (C::*type)(T0, T1, T2, T3, T4, T5, T6);
};

/**
 * A class template supplying the type of a member function with eight
 * arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class T7, class C>
struct c_mem_fn<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, void*), C> {
  typedef R (C::*type)(T0, T1, T2, T3, T4, T5, T6, T7);
};

/**
 * A class template supplying the type of a member function with nine arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class T7, class T8, class C>
struct c_mem_fn<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, void*), C> {
  typedef R (C::*type)(T0, T1, T2, T3, T4, T5, T6, T7, T8);
};

/// A class template supplying the type of a member function with ten arguments.
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class T7, class T8, class T9, class C>
struct c_mem_fn<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, void*), C> {
  typedef R (C::*type)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9);
};

/// Provides a C wrapper for a member function taking no arguments.
template <class C, class R, typename c_mem_fn<R (*)(void*), C>::type M>
struct c_mem_call<R (*)(void*), C, M> {
  static R
  function(void* data) {
    return ((*static_cast<C*>(data)).*M)();
  }
};

/// Provides a C wrapper for a member function taking one argument.
template <class C, class R, class T0,
          typename c_mem_fn<R (*)(T0, void*), C>::type M>
struct c_mem_call<R (*)(T0, void*), C, M> {
  static R
  function(T0 t0, void* data) {
    return ((*static_cast<C*>(data)).*M)(t0);
  }
};

/// Provides a C wrapper for a member function taking two arguments.
template <class C, class R, class T0, class T1,
          typename c_mem_fn<R (*)(T0, T1, void*), C>::type M>
struct c_mem_call<R (*)(T0, T1, void*), C, M> {
  static R
  function(T0 t0, T1 t1, void* data) {
    return ((*static_cast<C*>(data)).*M)(t0, t1);
  }
};

/// Provides a C wrapper for a member function taking three arguments.
template <class C, class R, class T0, class T1, class T2,
          typename c_mem_fn<R (*)(T0, T1, T2, void*), C>::type M>
struct c_mem_call<R (*)(T0, T1, T2, void*), C, M> {
  static R
  function(T0 t0, T1 t1, T2 t2, void* data) {
    return ((*static_cast<C*>(data)).*M)(t0, t1, t2);
  }
};

/// Provides a C wrapper for a member function taking four arguments.
template <class C, class R, class T0, class T1, class T2, class T3,
          typename c_mem_fn<R (*)(T0, T1, T2, T3, void*), C>::type M>
struct c_mem_call<R (*)(T0, T1, T2, T3, void*), C, M> {
  static R
  function(T0 t0, T1 t1, T2 t2, T3 t3, void* data) {
    return ((*static_cast<C*>(data)).*M)(t0, t1, t2, t3);
  }
};

/// Provides a C wrapper for a member function taking five arguments.
template <class C, class R, class T0, class T1, class T2, class T3, class T4,
          typename c_mem_fn<R (*)(T0, T1, T2, T3, T4, void*), C>::type M>
struct c_mem_call<R (*)(T0, T1, T2, T3, T4, void*), C, M> {
  static R
  function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, void* data) {
    return ((*static_cast<C*>(data)).*M)(t0, t1, t2, t3, t4);
  }
};

/// Provides a C wrapper for a member function taking six arguments.
template <class C, class R, class T0, class T1, class T2, class T3, class T4,
          class T5,
          typename c_mem_fn<R (*)(T0, T1, T2, T3, T4, T5, void*), C>::type M>
struct c_mem_call<R (*)(T0, T1, T2, T3, T4, T5, void*), C, M> {
  static R
  function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, void* data) {
    return ((*static_cast<C*>(data)).*M)(t0, t1, t2, t3, t4, t5);
  }
};

/// Provides a C wrapper for a member function taking seven arguments.
template <
    class C, class R, class T0, class T1, class T2, class T3, class T4,
    class T5, class T6,
    typename c_mem_fn<R (*)(T0, T1, T2, T3, T4, T5, T6, void*), C>::type M>
struct c_mem_call<R (*)(T0, T1, T2, T3, T4, T5, T6, void*), C, M> {
  static R
  function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, void* data) {
    return ((*static_cast<C*>(data)).*M)(t0, t1, t2, t3, t4, t5, t6);
  }
};

/// Provides a C wrapper for a member function taking eight arguments.
template <
    class C, class R, class T0, class T1, class T2, class T3, class T4,
    class T5, class T6, class T7,
    typename c_mem_fn<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, void*), C>::type M>
struct c_mem_call<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, void*), C, M> {
  static R
  function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, void* data) {
    return ((*static_cast<C*>(data)).*M)(t0, t1, t2, t3, t4, t5, t6, t7);
  }
};

/// Provides a C wrapper for a member function taking nine arguments.
template <class C, class R, class T0, class T1, class T2, class T3, class T4,
          class T5, class T6, class T7, class T8,
          typename c_mem_fn<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, void*),
                            C>::type M>
struct c_mem_call<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, void*), C, M> {
  static R
  function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8,
           void* data) {
    return ((*static_cast<C*>(data)).*M)(t0, t1, t2, t3, t4, t5, t6, t7, t8);
  }
};

/// Provides a C wrapper for a member function taking ten arguments.
template <class C, class R, class T0, class T1, class T2, class T3, class T4,
          class T5, class T6, class T7, class T8, class T9,
          typename c_mem_fn<
              R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, void*), C>::type M>
struct c_mem_call<R (*)(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, void*), C, M> {
  static R
  function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9,
           void* data) {
    return ((*static_cast<C*>(data)).*M)(t0, t1, t2, t3, t4, t5, t6, t7, t8,
                                         t9);
  }
};

#endif  // __cplusplus >= 201103L

}  // namespace lely

#endif
