/*!\file
 * This header file is part of the utilities library; it contains the C++
 * interface of the factory pattern.
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

#ifndef LELY_UTIL_FACTORY_HPP
#define LELY_UTIL_FACTORY_HPP

#include <lely/util/exception.hpp>
#include <lely/util/factory.h>

#include <memory>
#if __cplusplus < 201103L && defined(__GNUC__)
#include <tr1/memory>
#endif

namespace lely {

namespace impl {

template <class T, class Base>
inline Base*
c_ctor() noexcept
{
#ifndef LELY_NO_EXCEPTIONS
	try {
#endif
		return new T();
#ifndef LELY_NO_EXCEPTIONS
	} catch (...) {
		return 0;
	}
#endif
}

template <class T> inline void c_dtor(T* p) noexcept { delete p; }

} // impl

//! An abstract factory.
template <class T>
class factory {
public:
	/*!
	 * The deleter used by `shared_ptr` and `unique_ptr` to delete objects
	 * created with this factory.
	 */
	class deleter_type {
		friend class factory;

	public:
		void operator()(T* p) const { m_f.destroy(p); }

	private:
		explicit deleter_type(const factory& f): m_f(f) {}

		const factory& m_f;
	};

#if __cplusplus >= 201103L
	typedef ::std::shared_ptr<T> shared_type;
	typedef ::std::unique_ptr<T, deleter_type> unique_type;
#elif defined(__GNUC__) || defined(_MSC_VER)
	typedef ::std::tr1::shared_ptr<T> shared_type;
#endif

#if __cplusplus >= 201103L
	virtual ~factory() = default;
#else
	virtual ~factory() {}
#endif

	virtual T* create() const = 0;
	virtual void destroy(T* p) const noexcept = 0;

#if __cplusplus >= 201103L || defined(__GNUC__) || defined(_MSC_VER)
	shared_type
	make_shared() const
	{
		return shared_type(create(), deleter_type(*this));
	}
#endif

#if __cplusplus >= 201103L
	unique_type
	make_shared() const
	{
		return unique_type(create(), deleter_type(*this));
	}
#endif
};

/*!
 * An implementation of #factory<Base> for object types registered with
 * LELY_C_FACTORY(name, T, Base).
 */
template <class T, class Base = T>
class c_factory: public factory<Base> {
public:
	explicit c_factory(const char* name)
	: m_ctor(factory_find_ctor(name)), m_dtor(factory_find_dtor(name))
	{}

	explicit operator bool() const { return m_ctor; }

	virtual Base*
	create() const
	{
		return static_cast<Base*>(m_ctor());
	}

	virtual void
	destroy(Base* p) const noexcept
	{
		if (m_dtor)
			m_dtor(static_cast<void*>(p));
	}

private:
	factory_ctor_t* m_ctor;
	factory_dtor_t* m_dtor;
};

} // lely

/*!
 * Registers a factory for C++ objects with factory_insert().
 *
 * \param name the name of the factory.
 * \param T    the type of the objects to be created by the factory.
 * \param Base the type of the objects returned by the factory (MUST be a base
 *             type of \a T).
 */
#ifdef __COUNTER__
#define LELY_C_FACTORY(name, T, Base) \
	LELY_C_FACTORY_(__COUNTER__, name, T, Base)
#else
#define LELY_C_FACTORY(name, T, Base) \
	LELY_C_FACTORY_(__LINE__, name, T, Base)
#endif
#define LELY_C_FACTORY_(n, name, T, Base) \
	LELY_C_FACTORY__(n, name, T, Base)
#define LELY_C_FACTORY__(n, name, T, Base) \
	namespace lely { namespace impl { namespace { \
	extern "C" { \
	static void* \
	lely_c_factory_ctor_##n() \
	{ \
		return static_cast<void*>(::lely::impl::c_ctor<T, Base>()); \
	} \
	static void \
	lely_c_factory_dtor_##n(void *ptr) \
	{ \
		::lely::impl::c_dtor(static_cast<Base*>(ptr)); \
	} \
	} \
	struct c_factory_##n { \
		c_factory_##n() \
		{ \
			factory_insert(name &lely_factory_ctor_##n, \
					&lely_factory_dtor_##n); \
		} \
		~c_factory_##n() { factory_remove(name); } \
	}; \
	static c_factory_##n lely_c_factory_##n; \
	} } }

#endif

