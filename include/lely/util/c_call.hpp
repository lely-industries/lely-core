/*!\file
 * This header file is part of the utilities library; it contains the C callback
 * wrapper declarations.
 *
 * \copyright 2016 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
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

template <class, class> struct c_call;

#if __cplusplus >= 201103L

namespace impl {

template <class...> struct c_pack;

template <class, class> struct c_pack_push;
//! Pushes a type to the front of a parameter pack.
template <class T, class... S>
struct c_pack_push<T, c_pack<S...>> { using type = c_pack<T, S...>; };

template <class T, class... S>
//! Pops a type from the back of a parameter pack.
struct c_pack_pop: c_pack_push<T, typename c_pack_pop<S...>::type> {};
//! Pops a type from the back of a parameter pack.
template <class T, class S> struct c_pack_pop<T, S> { using type = c_pack<T>; };

} // impl

//! Provides a C wrapper for a callable with an arbitrary number of arguments.
template <class F, class R, class... ArgTypes>
struct c_call<F, impl::c_pack<R, ArgTypes...>> {
	typedef R result_type;

	static R
	function(ArgTypes... args, void* data) noexcept
	{
		return (*static_cast<F*>(data))(args...);
	}
};

//! Provides a C wrapper for a callable with an arbitrary number of arguments.
template <class F, class R, class... ArgTypes>
struct c_call<F, R(ArgTypes...)>
: c_call<F, typename impl::c_pack_pop<R, ArgTypes...>::type>
{};

#else

//! Provides a C wrapper for a callable taking no arguments.
template <class F, class R>
struct c_call<F, R(void*)> {
	typedef R result_type;

	static R
	function(void* data)
	{
		return (*static_cast<F*>(data))();
	}
};

//! Provides a C wrapper for a callable taking one argument.
template <class F, class R, class T0>
struct c_call<F, R(T0, void*)> {
	typedef R result_type;

	static R
	function(T0 t0, void* data)
	{
		return (*static_cast<F*>(data))(t0);
	}
};

//! Provides a C wrapper for a callable taking two arguments.
template <class F, class R, class T0, class T1>
struct c_call<F, R(T0, T1, void*)> {
	typedef R result_type;

	static R
	function(T0 t0, T1 t1, void* data)
	{
		return (*static_cast<F*>(data))(t0, t1);
	}
};

//! Provides a C wrapper for a callable taking three arguments.
template <class F, class R, class T0, class T1, class T2>
struct c_call<F, R(T0, T1, T2, void*)> {
	typedef R result_type;

	static R
	function(T0 t0, T1 t1, T2 t2, void* data)
	{
		return (*static_cast<F*>(data))(t0, t1, t2);
	}
};

//! Provides a C wrapper for a callable taking four arguments.
template <class F, class R, class T0, class T1, class T2, class T3>
struct c_call<F, R(T0, T1, T2, T3, void*)> {
	typedef R result_type;

	static R
	function(T0 t0, T1 t1, T2 t2, T3 t3, void* data)
	{
		return (*static_cast<F*>(data))(t0, t1, t2, t3);
	}
};

//! Provides a C wrapper for a callable taking five arguments.
template <class F, class R, class T0, class T1, class T2, class T3, class T4>
struct c_call<F, R(T0, T1, T2, T3, T4, void*)> {
	typedef R result_type;

	static R
	function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, void* data)
	{
		return (*static_cast<F*>(data))(t0, t1, t2, t3, t4);
	}
};

//! Provides a C wrapper for a callable taking six arguments.
template <class F, class R, class T0, class T1, class T2, class T3, class T4,
		class T5>
struct c_call<F, R(T0, T1, T2, T3, T4, T5, void*)> {
	typedef R result_type;

	static R
	function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, void* data)
	{
		return (*static_cast<F*>(data))(t0, t1, t2, t3, t4, t5);
	}
};

//! Provides a C wrapper for a callable taking seven arguments.
template <class F, class R, class T0, class T1, class T2, class T3, class T4,
		class T5, class T6>
struct c_call<F, R(T0, T1, T2, T3, T4, T5, T6, void*)> {
	typedef R result_type;

	static R
	function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, void* data)
	{
		return (*static_cast<F*>(data))(t0, t1, t2, t3, t4, t5, t6);
	}
};

//! Provides a C wrapper for a callable taking eight arguments.
template <class F, class R, class T0, class T1, class T2, class T3, class T4,
		class T5, class T6, class T7>
struct c_call<F, R(T0, T1, T2, T3, T4, T5, T6, T7, void*)> {
	typedef R result_type;

	static R
	function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7,
			void* data)
	{
		return (*static_cast<F*>(data))(t0, t1, t2, t3, t4, t5, t6, t7);
	}
};

//! Provides a C wrapper for a callable taking nine arguments.
template <class F, class R, class T0, class T1, class T2, class T3, class T4,
		class T5, class T6, class T7, class T8>
struct c_call<F, R(T0, T1, T2, T3, T4, T5, T6, T7, T8, void*)> {
	typedef R result_type;

	static R
	function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8,
			void* data)
	{
		return (*static_cast<F*>(data))(t0, t1, t2, t3, t4, t5, t6, t7,
				t8);
	}
};

//! Provides a C wrapper for a callable taking ten arguments.
template <class F, class R, class T0, class T1, class T2, class T3, class T4,
		class T5, class T6, class T7, class T8, class T9>
struct c_call<F, R(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, void*)> {
	typedef R result_type;

	static R
	function(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8,
			T9 t9, void* data)
	{
		return (*static_cast<F*>(data))(t0, t1, t2, t3, t4, t5, t6, t7,
				t8, t9);
	}
};

#endif // __cplusplus >= 201103L

} // lely

#endif

