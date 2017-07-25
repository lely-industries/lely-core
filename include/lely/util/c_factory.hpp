/*!\file
 * This header file is part of the utilities library; it contains the
 * implementation of the C++ factory pattern for C objects.
 *
 * \copyright 2017 Lely Industries N.V.
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

#ifndef LELY_UTIL_C_FACTORY_HPP
#define LELY_UTIL_C_FACTORY_HPP

#include <lely/util/c_type.hpp>
#include <lely/util/factory.h>
#include <lely/util/factory.hpp>

#include <string>

namespace lely {

//! The factory for C objects.
template <class T, class U = typename impl::remove_arguments<T>::type>
class c_factory;

//! The factory for heap-allocated C objects.
template <class T, class U>
class c_factory<T*, U*>: public virtual factory<U*> {
public:
	explicit c_factory(factory_ctor_t* ctor, factory_dtor_t* dtor)
	: m_ctor(ctor), m_dtor(dtor)
	{}

	explicit c_factory(const char* name)
#if __cplusplus >= 201103L
	: c_factory(factory_find_ctor(name), factory_find_dtor(name))
#else
	: m_ctor(factory_find_ctor(name)), m_dtor(factory_find_dtor(name))
#endif
	{}

#if __cplusplus >= 201103L
	explicit operator bool() const noexcept { return m_ctor; }
#else
	operator bool() const { return m_ctor; }
#endif

	virtual void
#if __cplusplus >= 201103L
	destroy(U* p) noexcept override
#else
	destroy(U* p)
#endif
	{
		factory_dtor_destroy(m_dtor, static_cast<void*>(p));
	}

protected:
	factory_ctor_t* m_ctor;
	factory_dtor_t* m_dtor;
};

#if __cplusplus >= 201103L

/*!
 * The factory for heap-allocated C objects that can be constructed with an
 * arbitrary number of arguments.
 */
template <class R, class... Args, class U>
class c_factory<R*(Args...), U*>
: public c_factory<R*, U*>, public factory<U*(Args...)> {
public:
	using c_factory<R*, U*>::c_factory;

	virtual U*
	create(Args... args) override
	{
		auto ptr = factory_ctor_create(c_factory::m_ctor, args...);
		if (__unlikely(!ptr))
			impl::throw_bad_init();
		return static_cast<R*>(ptr);
	}
};

#else

/*!
 * The factory for heap-allocated C objects that can be constructed without
 * arguments.
 */
template <class R, class U>
class c_factory<R*(), U*>
: public c_factory<R*, U*>, public factory<U*()> {
public:
	explicit c_factory(factory_ctor_t* ctor, factory_dtor_t* dtor)
	: c_factory<R, U>(ctor, dtor)
	{}

	explicit c_factory(const char* name): c_factory<R*, U*>(name) {}

	virtual U*
	create()
	{
		void *ptr = factory_ctor_create(c_factory::m_ctor);
		if (__unlikely(!ptr))
			impl::throw_bad_init();
		return static_cast<R*>(ptr);
	}
};

/*!
 * The factory for heap-allocated C objects that can be constructed with one
 * argument.
 */
template <class R, class T0, class U>
class c_factory<R*(T0), U*>
: public c_factory<R*, U*>, public factory<U*(T0)> {
public:
	explicit c_factory(factory_ctor_t* ctor, factory_dtor_t* dtor)
	: c_factory<R*, U*>(ctor, dtor)
	{}

	explicit c_factory(const char* name): c_factory<R*, U*>(name) {}

	virtual U*
	create(T0 t0)
	{
		void *ptr = factory_ctor_create(c_factory::m_ctor, t0);
		if (__unlikely(!ptr))
			impl::throw_bad_init();
		return static_cast<R*>(ptr);
	}
};

/*!
 * The factory for heap-allocated C objects that can be constructed with two
 * arguments.
 */
template <class R, class T0, class T1, class U>
class c_factory<R*(T0, T1), U*>
: public c_factory<R*, U*>, public factory<U*(T0, T1)> {
public:
	explicit c_factory(factory_ctor_t* ctor, factory_dtor_t* dtor)
	: c_factory<R*, U*>(ctor, dtor)
	{}

	explicit c_factory(const char* name): c_factory<R*, U*>(name) {}

	virtual U*
	create(T0 t0, T1 t1)
	{
		void *ptr = factory_ctor_create(c_factory::m_ctor, t0, t1);
		if (__unlikely(!ptr))
			impl::throw_bad_init();
		return static_cast<R*>(ptr);
	}
};

/*!
 * The factory for heap-allocated C objects that can be constructed with three
 * arguments.
 */
template <class R, class T0, class T1, class T2, class U>
class c_factory<R*(T0, T1, T2), U*>
: public c_factory<R*, U*>, public factory<U*(T0, T1, T2)> {
public:
	explicit c_factory(factory_ctor_t* ctor, factory_dtor_t* dtor)
	: c_factory<R*, U*>(ctor, dtor)
	{}

	explicit c_factory(const char* name): c_factory<R*, U*>(name) {}

	virtual U*
	create(T0 t0, T1 t1, T2 t2)
	{
		void *ptr = factory_ctor_create(c_factory::m_ctor, t0, t1, t2);
		if (__unlikely(!ptr))
			impl::throw_bad_init();
		return static_cast<R*>(ptr);
	}
};

/*!
 * The factory for heap-allocated C objects that can be constructed with four
 * arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class U>
class c_factory<R*(T0, T1, T2, T3), U*>
: public c_factory<R*, U*>, public factory<U*(T0, T1, T2, T3)> {
public:
	explicit c_factory(factory_ctor_t* ctor, factory_dtor_t* dtor)
	: c_factory<R*, U*>(ctor, dtor)
	{}

	explicit c_factory(const char* name): c_factory<R*, U*>(name) {}

	virtual U*
	create(T0 t0, T1 t1, T2 t2, T3 t3)
	{
		void *ptr = factory_ctor_create(c_factory::m_ctor, t0, t1, t2,
				t3);
		if (__unlikely(!ptr))
			impl::throw_bad_init();
		return static_cast<R*>(ptr);
	}
};

/*!
 * The factory for heap-allocated C objects that can be constructed with five
 * arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class U>
class c_factory<R*(T0, T1, T2, T3, T4), U*>
: public c_factory<R*, U*>, public factory<U*(T0, T1, T2, T3, T4)> {
public:
	explicit c_factory(factory_ctor_t* ctor, factory_dtor_t* dtor)
	: c_factory<R*, U*>(ctor, dtor)
	{}

	explicit c_factory(const char* name): c_factory<R*, U*>(name) {}

	virtual U*
	create(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4)
	{
		void *ptr = factory_ctor_create(c_factory::m_ctor, t0, t1, t2,
				t3, t4);
		if (__unlikely(!ptr))
			impl::throw_bad_init();
		return static_cast<R*>(ptr);
	}
};

/*!
 * The factory for heap-allocated C objects that can be constructed with six
 * arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
		class U>
class c_factory<R*(T0, T1, T2, T3, T4, T5), U*>
: public c_factory<R*, U*>, public factory<U*(T0, T1, T2, T3, T4, T5)> {
public:
	explicit c_factory(factory_ctor_t* ctor, factory_dtor_t* dtor)
	: c_factory<R*, U*>(ctor, dtor)
	{}

	explicit c_factory(const char* name): c_factory<R*, U*>(name) {}

	virtual U*
	create(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5)
	{
		void *ptr = factory_ctor_create(c_factory::m_ctor, t0, t1, t2,
				t3, t4, t5);
		if (__unlikely(!ptr))
			impl::throw_bad_init();
		return static_cast<R*>(ptr);
	}
};

/*!
 * The factory for heap-allocated C objects that can be constructed with seven
 * arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
		class T6, class U>
class c_factory<R*(T0, T1, T2, T3, T4, T5, T6), U*>
: public c_factory<R*, U*>, public factory<U*(T0, T1, T2, T3, T4, T5, T6)> {
public:
	explicit c_factory(factory_ctor_t* ctor, factory_dtor_t* dtor)
	: c_factory<R*, U*>(ctor, dtor)
	{}

	explicit c_factory(const char* name): c_factory<R*, U*>(name) {}

	virtual U*
	create(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6)
	{
		void *ptr = factory_ctor_create(c_factory::m_ctor, t0, t1, t2,
				t3, t4, t5, t6);
		if (__unlikely(!ptr))
			impl::throw_bad_init();
		return static_cast<R*>(ptr);
	}
};

/*!
 * The factory for heap-allocated C objects that can be constructed with eight
 * arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
		class T6, class T7, class U>
class c_factory<R*(T0, T1, T2, T3, T4, T5, T6, T7), U*>
: public c_factory<R*, U*>, public factory<U*(T0, T1, T2, T3, T4, T5, T6, T7)> {
public:
	explicit c_factory(factory_ctor_t* ctor, factory_dtor_t* dtor)
	: c_factory<R*, U*>(ctor, dtor)
	{}

	explicit c_factory(const char* name): c_factory<R*, U*>(name) {}

	virtual U*
	create(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7)
	{
		void *ptr = factory_ctor_create(c_factory::m_ctor, t0, t1, t2,
				t3, t4, t5, t6, t7);
		if (__unlikely(!ptr))
			impl::throw_bad_init();
		return static_cast<R*>(ptr);
	}
};

/*!
 * The factory for heap-allocated C objects that can be constructed with nine
 * arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
		class T6, class T7, class T8, class U>
class c_factory<R*(T0, T1, T2, T3, T4, T5, T6, T7, T8), U*>
: public c_factory<R*, U*>
, public factory<U*(T0, T1, T2, T3, T4, T5, T6, T7, T8)> {
public:
	explicit c_factory(factory_ctor_t* ctor, factory_dtor_t* dtor)
	: c_factory<R*, U*>(ctor, dtor)
	{}

	explicit c_factory(const char* name): c_factory<R*, U*>(name) {}

	virtual U*
	create(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8)
	{
		void *ptr = factory_ctor_create(c_factory::m_ctor, t0, t1, t2,
				t3, t4, t5, t6, t7, t8);
		if (__unlikely(!ptr))
			impl::throw_bad_init();
		return static_cast<R*>(ptr);
	}
};

/*!
 * The factory for heap-allocated C objects that can be constructed with nine
 * arguments.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
		class T6, class T7, class T8, class T9, class U>
class c_factory<R*(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9), U*>
: public c_factory<R*, U*>
, public factory<U*(T0, T1, T2, T3, T4, T5, T6, T7, T8, T9)> {
public:
	explicit c_factory(factory_ctor_t* ctor, factory_dtor_t* dtor)
	: c_factory<R*, U*>(ctor, dtor)
	{}

	explicit c_factory(const char* name): c_factory<R*, U*>(name) {}

	virtual U*
	create(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8,
			T9 t9)
	{
		void *ptr = factory_ctor_create(c_factory::m_ctor, t0, t1, t2,
				t3, t4, t5, t6, t7, t8, t9);
		if (__unlikely(!ptr))
			impl::throw_bad_init();
		return static_cast<R*>(ptr);
	}
};

#endif // __cplusplus >= 201103L

} // lely

#endif

