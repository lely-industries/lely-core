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

#if __cplusplus >= 201103L

/*!
 * Constructs a C++ object with an arbitrary number of arguments.
 *
 * \returns a pointer to the newly created object, or `nullptr` on error. In the
 * latter case, the error number can be obtained with `get_errnum()`.
 */
template <class R, class... Args>
inline void*
cpp_factory_ctor(Args... args) noexcept
{
#if !LELY_NO_EXCEPTIONS
	try {
#endif
		R* p = new R(args...);
		return static_cast<void*>(p);
#if !LELY_NO_EXCEPTIONS
	} catch (error& e) {
		set_errc(e.errc());
		return nullptr;
	} catch (::std::system_error& e) {
		if (e.code().category() == ::std::system_category())
			set_errc(e.code().value());
		return nullptr;
	} catch (::std::bad_alloc&) {
		set_errnum(ERRNUM_NOMEM);
		return nullptr;
	} catch (...) {
		return nullptr;
	}
#endif
}

#else

/*!
 * Constructs a C++ object without arguments.
 *
 * \returns a pointer to the newly created object, or 0 on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 */
template <class R>
inline void*
cpp_factory_ctor()
{
#if !LELY_NO_EXCEPTIONS
	try {
#endif
		R* p = new R();
		return static_cast<void*>(p);
#if !LELY_NO_EXCEPTIONS
	} catch (error& e) {
		set_errc(e.errc());
		return 0;
	} catch (::std::bad_alloc&) {
		set_errnum(ERRNUM_NOMEM);
		return 0;
	} catch (...) {
		return 0;
	}
#endif
}

/*!
 * Constructs a C++ object with one arguments.
 *
 * \returns a pointer to the newly created object, or 0 on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 */
template <class R, class T0>
inline void*
cpp_factory_ctor(T0 t0)
{
#if !LELY_NO_EXCEPTIONS
	try {
#endif
		R* p = new R(t0);
		return static_cast<void*>(p);
#if !LELY_NO_EXCEPTIONS
	} catch (error& e) {
		set_errc(e.errc());
		return 0;
	} catch (::std::bad_alloc&) {
		set_errnum(ERRNUM_NOMEM);
		return 0;
	} catch (...) {
		return 0;
	}
#endif
}

/*!
 * Constructs a C++ object with two arguments.
 *
 * \returns a pointer to the newly created object, or 0 on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 */
template <class R, class T0, class T1>
inline void*
cpp_factory_ctor(T0 t0, T1 t1)
{
#if !LELY_NO_EXCEPTIONS
	try {
#endif
		R* p = new R(t0, t1);
		return static_cast<void*>(p);
#if !LELY_NO_EXCEPTIONS
	} catch (error& e) {
		set_errc(e.errc());
		return 0;
	} catch (::std::bad_alloc&) {
		set_errnum(ERRNUM_NOMEM);
		return 0;
	} catch (...) {
		return 0;
	}
#endif
}

/*!
 * Constructs a C++ object with three arguments.
 *
 * \returns a pointer to the newly created object, or 0 on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 */
template <class R, class T0, class T1, class T2>
inline void*
cpp_factory_ctor(T0 t0, T1 t1, T2 t2)
{
#if !LELY_NO_EXCEPTIONS
	try {
#endif
		R* p = new R(t0, t1, t2);
		return static_cast<void*>(p);
#if !LELY_NO_EXCEPTIONS
	} catch (error& e) {
		set_errc(e.errc());
		return 0;
	} catch (::std::bad_alloc&) {
		set_errnum(ERRNUM_NOMEM);
		return 0;
	} catch (...) {
		return 0;
	}
#endif
}

/*!
 * Constructs a C++ object with four arguments.
 *
 * \returns a pointer to the newly created object, or 0 on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 */
template <class R, class T0, class T1, class T2, class T3>
inline void*
cpp_factory_ctor(T0 t0, T1 t1, T2 t2, T3 t3)
{
#if !LELY_NO_EXCEPTIONS
	try {
#endif
		R* p = new R(t0, t1, t2, t3);
		return static_cast<void*>(p);
#if !LELY_NO_EXCEPTIONS
	} catch (error& e) {
		set_errc(e.errc());
		return 0;
	} catch (::std::bad_alloc&) {
		set_errnum(ERRNUM_NOMEM);
		return 0;
	} catch (...) {
		return 0;
	}
#endif
}

/*!
 * Constructs a C++ object with five arguments.
 *
 * \returns a pointer to the newly created object, or 0 on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 */
template <class R, class T0, class T1, class T2, class T3, class T4>
inline void*
cpp_factory_ctor(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4)
{
#if !LELY_NO_EXCEPTIONS
	try {
#endif
		R* p = new R(t0, t1, t2, t3, t4);
		return static_cast<void*>(p);
#if !LELY_NO_EXCEPTIONS
	} catch (error& e) {
		set_errc(e.errc());
		return 0;
	} catch (::std::bad_alloc&) {
		set_errnum(ERRNUM_NOMEM);
		return 0;
	} catch (...) {
		return 0;
	}
#endif
}

/*!
 * Constructs a C++ object with six arguments.
 *
 * \returns a pointer to the newly created object, or 0 on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5>
inline void*
cpp_factory_ctor(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5)
{
#if !LELY_NO_EXCEPTIONS
	try {
#endif
		R* p = new R(t0, t1, t2, t3, t4, t5);
		return static_cast<void*>(p);
#if !LELY_NO_EXCEPTIONS
	} catch (error& e) {
		set_errc(e.errc());
		return 0;
	} catch (::std::bad_alloc&) {
		set_errnum(ERRNUM_NOMEM);
		return 0;
	} catch (...) {
		return 0;
	}
#endif
}

/*!
 * Constructs a C++ object with seven arguments.
 *
 * \returns a pointer to the newly created object, or 0 on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
		class T6>
inline void*
cpp_factory_ctor(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6)
{
#if !LELY_NO_EXCEPTIONS
	try {
#endif
		R* p = new R(t0, t1, t2, t3, t4, t5, t6);
		return static_cast<void*>(p);
#if !LELY_NO_EXCEPTIONS
	} catch (error& e) {
		set_errc(e.errc());
		return 0;
	} catch (::std::bad_alloc&) {
		set_errnum(ERRNUM_NOMEM);
		return 0;
	} catch (...) {
		return 0;
	}
#endif
}

/*!
 * Constructs a C++ object with eight arguments.
 *
 * \returns a pointer to the newly created object, or 0 on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
		class T6, class T7>
inline void*
cpp_factory_ctor(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7)
{
#if !LELY_NO_EXCEPTIONS
	try {
#endif
		R* p = new R(t0, t1, t2, t3, t4, t5, t6, t7);
		return static_cast<void*>(p);
#if !LELY_NO_EXCEPTIONS
	} catch (error& e) {
		set_errc(e.errc());
		return 0;
	} catch (::std::bad_alloc&) {
		set_errnum(ERRNUM_NOMEM);
		return 0;
	} catch (...) {
		return 0;
	}
#endif
}

/*!
 * Constructs a C++ object with nine arguments.
 *
 * \returns a pointer to the newly created object, or 0 on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
		class T6, class T7, class T8>
inline void*
cpp_factory_ctor(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8)
{
#if !LELY_NO_EXCEPTIONS
	try {
#endif
		R* p = new R(t0, t1, t2, t3, t4, t5, t6, t7, t8);
		return static_cast<void*>(p);
#if !LELY_NO_EXCEPTIONS
	} catch (error& e) {
		set_errc(e.errc());
		return 0;
	} catch (::std::bad_alloc&) {
		set_errnum(ERRNUM_NOMEM);
		return 0;
	} catch (...) {
		return 0;
	}
#endif
}

/*!
 * Constructs a C++ object with ten arguments.
 *
 * \returns a pointer to the newly created object, or 0 on error. In the latter
 * case, the error number can be obtained with `get_errnum()`.
 */
template <class R, class T0, class T1, class T2, class T3, class T4, class T5,
		class T6, class T7, class T8, class T9>
inline void*
cpp_factory_ctor(T0 t0, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8,
		T9 t9)
{
#if !LELY_NO_EXCEPTIONS
	try {
#endif
		R* p = new R(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9);
		return static_cast<void*>(p);
#if !LELY_NO_EXCEPTIONS
	} catch (error& e) {
		set_errc(e.errc());
		return 0;
	} catch (::std::bad_alloc&) {
		set_errnum(ERRNUM_NOMEM);
		return 0;
	} catch (...) {
		return 0;
	}
#endif
}

#endif // __cplusplus >= 201103L

//! Destroys a C++ object.
template <class T>
inline void
cpp_factory_dtor(void* ptr) noexcept { delete static_cast<T*>(ptr); }

/*!
 * A class template which statically registers a C constructor and destructor
 * function under the specified name.
 */
template <factory_ctor_t* ctor, factory_dtor_t* dtor>
struct c_static_factory {
	static c_static_factory instance;

	explicit
	c_static_factory(const ::std::string& name): m_name(name)
	{
		if (__unlikely(factory_insert(m_name.c_str(), ctor, dtor)
				== -1))
			impl::throw_bad_init();
	}

	~c_static_factory() { factory_remove(m_name.c_str()); }

private:
	::std::string m_name;
};

} // lely

/*!
 * Statically registers a C constructor and destructor function under the
 * specified name.
 */
#define LELY_C_STATIC_FACTORY(name, ctor, dtor) \
	namespace lely { \
		template <> \
		c_static_factory<ctor, dtor> \
		c_static_factory<ctor, dtor>::instance(name); \
	}

#define LELY_CPP_FACTORY_CTOR_0(R) \
	extern "C" void* \
	lely_cpp_factory_ctor_0_##R(va_list) \
	{ \
		return ::lely::cpp_factory_ctor<R>(); \
	}

#define LELY_CPP_FACTORY_CTOR_1(R, T0) \
	extern "C" void* \
	lely_cpp_factory_ctor_1_##R(va_list ap) \
	{ \
		T0 t0 = va_arg(ap, T0); \
		return ::lely::cpp_factory_ctor<R>(t0); \
	}

#define LELY_CPP_FACTORY_CTOR_2(R, T0, T1) \
	extern "C" void* \
	lely_cpp_factory_ctor_2_##R(va_list ap) \
	{ \
		T0 t0 = va_arg(ap, T0); \
		T1 t1 = va_arg(ap, T1); \
		return ::lely::cpp_factory_ctor<R>(t0, t1); \
	}

#define LELY_CPP_FACTORY_CTOR_3(R, T0, T1, T2) \
	extern "C" void* \
	lely_cpp_factory_ctor_3_##R(va_list ap) \
	{ \
		T0 t0 = va_arg(ap, T0); \
		T1 t1 = va_arg(ap, T1); \
		T2 t2 = va_arg(ap, T2); \
		return ::lely::cpp_factory_ctor<R>(t0, t1, t2); \
	}

#define LELY_CPP_FACTORY_CTOR_4(R, T0, T1, T2, T3) \
	extern "C" void* \
	lely_cpp_factory_ctor_4_##R(va_list ap) \
	{ \
		T0 t0 = va_arg(ap, T0); \
		T1 t1 = va_arg(ap, T1); \
		T2 t2 = va_arg(ap, T2); \
		T3 t3 = va_arg(ap, T3); \
		return ::lely::cpp_factory_ctor<R>(t0, t1, t2, t3); \
	}

#define LELY_CPP_FACTORY_CTOR_5(R, T0, T1, T2, T3, T4) \
	extern "C" void* \
	lely_cpp_factory_ctor_5_##R(va_list ap) \
	{ \
		T0 t0 = va_arg(ap, T0); \
		T1 t1 = va_arg(ap, T1); \
		T2 t2 = va_arg(ap, T2); \
		T3 t3 = va_arg(ap, T3); \
		T4 t4 = va_arg(ap, T4); \
		return ::lely::cpp_factory_ctor<R>(t0, t1, t2, t3, t4); \
	}

#define LELY_CPP_FACTORY_CTOR_6(R, T0, T1, T2, T3, T4, T5) \
	extern "C" void* \
	lely_cpp_factory_ctor_6_##R(va_list ap) \
	{ \
		T0 t0 = va_arg(ap, T0); \
		T1 t1 = va_arg(ap, T1); \
		T2 t2 = va_arg(ap, T2); \
		T3 t3 = va_arg(ap, T3); \
		T4 t4 = va_arg(ap, T4); \
		T5 t5 = va_arg(ap, T5); \
		return ::lely::cpp_factory_ctor<R>(t0, t1, t2, t3, t4, t5); \
	}

#define LELY_CPP_FACTORY_CTOR_7(R, T0, T1, T2, T3, T4, T5, T6) \
	extern "C" void* \
	lely_cpp_factory_ctor_7_##R(va_list ap) \
	{ \
		T0 t0 = va_arg(ap, T0); \
		T1 t1 = va_arg(ap, T1); \
		T2 t2 = va_arg(ap, T2); \
		T3 t3 = va_arg(ap, T3); \
		T4 t4 = va_arg(ap, T4); \
		T5 t5 = va_arg(ap, T5); \
		T6 t6 = va_arg(ap, T6); \
		return ::lely::cpp_factory_ctor<R>(t0, t1, t2, t3, t4, t5, t6); \
	}

#define LELY_CPP_FACTORY_CTOR_8(R, T0, T1, T2, T3, T4, T5, T6, T7) \
	extern "C" void* \
	lely_cpp_factory_ctor_8_##R(va_list ap) \
	{ \
		T0 t0 = va_arg(ap, T0); \
		T1 t1 = va_arg(ap, T1); \
		T2 t2 = va_arg(ap, T2); \
		T3 t3 = va_arg(ap, T3); \
		T4 t4 = va_arg(ap, T4); \
		T5 t5 = va_arg(ap, T5); \
		T6 t6 = va_arg(ap, T6); \
		T7 t7 = va_arg(ap, T7); \
		return ::lely::cpp_factory_ctor<R>(t0, t1, t2, t3, t4, t5, t6, \
				t7); \
	}

#define LELY_CPP_FACTORY_CTOR_9(R, T0, T1, T2, T3, T4, T5, T6, T7, T8) \
	extern "C" void* \
	lely_cpp_factory_ctor_9_##R(va_list ap) \
	{ \
		T0 t0 = va_arg(ap, T0); \
		T1 t1 = va_arg(ap, T1); \
		T2 t2 = va_arg(ap, T2); \
		T3 t3 = va_arg(ap, T3); \
		T4 t4 = va_arg(ap, T4); \
		T5 t5 = va_arg(ap, T5); \
		T6 t6 = va_arg(ap, T6); \
		T7 t7 = va_arg(ap, T7); \
		T8 t8 = va_arg(ap, T8); \
		return ::lely::cpp_factory_ctor<R>(t0, t1, t2, t3, t4, t5, t6, \
				t7, t8); \
	}

#define LELY_CPP_FACTORY_CTOR_10(R, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9) \
	extern "C" void* \
	lely_cpp_factory_ctor_10_##R(va_list ap) \
	{ \
		T0 t0 = va_arg(ap, T0); \
		T1 t1 = va_arg(ap, T1); \
		T2 t2 = va_arg(ap, T2); \
		T3 t3 = va_arg(ap, T3); \
		T4 t4 = va_arg(ap, T4); \
		T5 t5 = va_arg(ap, T5); \
		T6 t6 = va_arg(ap, T6); \
		T7 t7 = va_arg(ap, T7); \
		T8 t8 = va_arg(ap, T8); \
		T9 t9 = va_arg(ap, T9); \
		return ::lely::cpp_factory_ctor<R>(t0, t1, t2, t3, t4, t5, t6, \
				t7, t8, t9); \
	}

#define LELY_CPP_FACTORY_DTOR(T) \
	extern "C" void \
	lely_cpp_factory_dtor_##T(void* ptr) \
	{ \
		::lely::cpp_factory_dtor<T>(ptr); \
	}

/*!
 * Statically registers the constructor for a C++ type taking no arguments and
 * the default destructor under the specified name.
 */
#define LELY_C_STATIC_FACTORY_0(name, R) \
	LELY_CPP_FACTORY_CTOR_0(R) \
	LELY_C_STATIC_FACTORY_(0, name, R)

/*!
 * Statically registers the constructor for a C++ type taking one argument and
 * the default destructor under the specified name.
 */
#define LELY_C_STATIC_FACTORY_1(name, R, T0) \
	LELY_CPP_FACTORY_CTOR_1(R, T0) \
	LELY_C_STATIC_FACTORY_(1, name, R)

/*!
 * Statically registers the constructor for a C++ type taking two arguments and
 * the default destructor under the specified name.
 */
#define LELY_C_STATIC_FACTORY_2(name, R, T0, T1) \
	LELY_CPP_FACTORY_CTOR_2(R, T0, T1) \
	LELY_C_STATIC_FACTORY_(2, name, R)

/*!
 * Statically registers the constructor for a C++ type taking three arguments
 * and the default destructor under the specified name.
 */
#define LELY_C_STATIC_FACTORY_3(name, R, T0, T1, T2) \
	LELY_CPP_FACTORY_CTOR_3(R, T0, T1, T2) \
	LELY_C_STATIC_FACTORY_(3, name, R)

/*!
 * Statically registers the constructor for a C++ type taking four arguments and
 * the default destructor under the specified name.
 */
#define LELY_C_STATIC_FACTORY_4(name, R, T0, T1, T2, T3) \
	LELY_CPP_FACTORY_CTOR_4(R, T0, T1, T2, T3) \
	LELY_C_STATIC_FACTORY_(4, name, R)

/*!
 * Statically registers the constructor for a C++ type taking five arguments and
 * the default destructor under the specified name.
 */
#define LELY_C_STATIC_FACTORY_5(name, R, T0, T1, T2, T3, T4) \
	LELY_CPP_FACTORY_CTOR_5(R, T0, T1, T2, T3, T4) \
	LELY_C_STATIC_FACTORY_(5, name, R)

/*!
 * Statically registers the constructor for a C++ type taking six arguments and
 * the default destructor under the specified name.
 */
#define LELY_C_STATIC_FACTORY_6(name, R, T0, T1, T2, T3, T4, T5) \
	LELY_CPP_FACTORY_CTOR_6(R, T0, T1, T2, T3, T4, T5) \
	LELY_C_STATIC_FACTORY_(6, name, R)

/*!
 * Statically registers the constructor for a C++ type taking seven arguments
 * and the default destructor under the specified name.
 */
#define LELY_C_STATIC_FACTORY_7(name, R, T0, T1, T2, T3, T4, T5, T6) \
	LELY_CPP_FACTORY_CTOR_7(R, T0, T1, T2, T3, T4, T5, T6) \
	LELY_C_STATIC_FACTORY_(7, name, R)

/*!
 * Statically registers the constructor for a C++ type taking eight arguments
 * and the default destructor under the specified name.
 */
#define LELY_C_STATIC_FACTORY_8(name, R, T0, T1, T2, T3, T4, T5, T6, T7) \
	LELY_CPP_FACTORY_CTOR_8(R, T0, T1, T2, T3, T4, T5, T6, T7) \
	LELY_C_STATIC_FACTORY_(8, name, R)

/*!
 * Statically registers the constructor for a C++ type taking nine arguments and
 * the default destructor under the specified name.
 */
#define LELY_C_STATIC_FACTORY_9(name, R, T0, T1, T2, T3, T4, T5, T6, T7, T8) \
	LELY_CPP_FACTORY_CTOR_9(R, T0, T1, T2, T3, T4, T5, T6, T7, T8) \
	LELY_C_STATIC_FACTORY_(9, name, R)

/*!
 * Statically registers the constructor for a C++ type taking ten arguments and
 * the default destructor under the specified name.
 */
#define LELY_C_STATIC_FACTORY_10(name, R, T0, T1, T2, T3, T4, T5, T6, T7, T8, \
		T9) \
	LELY_CPP_FACTORY_CTOR_10(R, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9) \
	LELY_C_STATIC_FACTORY_(10, name, R)

#define LELY_C_STATIC_FACTORY_(N, name, T) \
	LELY_CPP_FACTORY_DTOR(T) \
	LELY_C_STATIC_FACTORY(name, &lely_cpp_factory_ctor_##N##_##T, \
			&lely_cpp_factory_dtor_##T)

#endif

