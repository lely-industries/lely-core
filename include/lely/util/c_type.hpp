/*!\file
 * This header file is part of the utilities library; it contains the C to C++
 * interface declarations.
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

#ifndef LELY_UTIL_C_TYPE_HPP
#define LELY_UTIL_C_TYPE_HPP

#include <lely/util/exception.hpp>

#include <new>
#include <utility>

namespace lely {

/*!
 * The type of objects thrown as exceptions to report a failure to initialize an
 * instantiation of a C type.
 */
class __dllexport bad_init: public ::std::exception {
public:
	virtual const char* what() const nothrow_or_noexcept;
};

/*!
 * The type of objects thrown as exceptions to report a failure to copy an
 * instantiation of a C type.
 */
class __dllexport bad_copy: public ::std::exception {
public:
	virtual const char* what() const nothrow_or_noexcept;
};

/*!
 * The type of objects thrown as exceptions to report a failure to move an
 * instantiation of a C type.
 */
class __dllexport bad_move: public ::std::exception {
public:
	virtual const char* what() const nothrow_or_noexcept;
};

/*!
 * A class template supplying a uniform interface to certain attributes of C
 * types.
 */
template <class T> struct c_type_traits;

//! The base class for a C++ interface to a trivial C type.
template <class T>
struct trivial_c_type {
	typedef typename c_type_traits<T>::value_type c_value_type;
	typedef typename c_type_traits<T>::reference c_reference;
	typedef typename c_type_traits<T>::const_reference c_const_reference;
	typedef typename c_type_traits<T>::pointer c_pointer;
	typedef typename c_type_traits<T>::const_pointer c_const_pointer;

	operator c_value_type() const noexcept { return c_ref(); }
	operator c_reference() noexcept { return c_ref(); }
	operator c_const_reference() const noexcept { return c_ref(); }

	c_reference c_ref() noexcept { return *c_ptr(); }
	c_const_reference c_ref() const noexcept { return *c_ptr(); }

	c_pointer c_ptr() noexcept { return reinterpret_cast<c_pointer>(this); }

	c_const_pointer
	c_ptr() const noexcept
	{
		return reinterpret_cast<c_const_pointer>(this);
	}

	static void dtor(trivial_c_type* p) noexcept { delete p; }
	void destroy() noexcept { dtor(this); }
};

template <class T>
inline void
destroy(trivial_c_type<T>* p) noexcept
{
	p->destroy();
}

//! The base class for a C++ interface to a standard layout C type.
template <class T>
class standard_c_type {
public:
	typedef typename c_type_traits<T>::value_type c_value_type;
	typedef typename c_type_traits<T>::reference c_reference;
	typedef typename c_type_traits<T>::const_reference c_const_reference;
	typedef typename c_type_traits<T>::pointer c_pointer;
	typedef typename c_type_traits<T>::const_pointer c_const_pointer;

	operator c_value_type() const noexcept { return c_ref(); }
	operator c_reference() noexcept { return c_ref(); }
	operator c_const_reference() const noexcept { return c_ref(); }

	c_reference c_ref() noexcept { return *c_ptr(); }
	c_const_reference c_ref() const noexcept { return *c_ptr(); }

	c_pointer c_ptr() noexcept { return reinterpret_cast<c_pointer>(this); }

	c_const_pointer
	c_ptr() const noexcept
	{
		return reinterpret_cast<c_const_pointer>(this);
	}

	static void dtor(standard_c_type* p) noexcept { delete p; }
	void destroy() noexcept { dtor(this); }

	standard_c_type&
	operator=(const standard_c_type& val)
	{
		if (!c_type_traits<T>::copy(c_ptr(), val.c_ptr()))
			throw_or_abort(bad_copy());
		return *this;
	}

#if __cplusplus >= 201103L
	standard_c_type&
	operator=(standard_c_type&& val)
	{
		if (!c_type_traits<T>::move(c_ptr(), val.c_ptr()))
			throw_or_abort(bad_move());
		return *this;
	}
#endif

protected:
#if __cplusplus >= 201103L
	template <class... Args>
	explicit
	standard_c_type(Args&&... args)
	{
		if (!c_type_traits<T>::init(c_ptr(),
				std::forward<Args>(args)...))
			throw_or_abort(bad_init());
	}
#else
	standard_c_type()
	{
		if (!c_type_traits<T>::init(c_ptr()))
			throw_or_abort(bad_init());
	}

	template <class U0>
	explicit
	standard_c_type(U0 u0)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1>
	standard_c_type(U0 u0, U1 u1)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1, class U2>
	standard_c_type(U0 u0, U1 u1, U2 u2)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1, u2))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1, class U2, class U3>
	standard_c_type(U0 u0, U1 u1, U2 u2, U3 u3)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1, u2, u3))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1, class U2, class U3, class U4>
	standard_c_type(U0 u0, U1 u1, U2 u2, U3 u3, U4 u4)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1, u2, u3, u4))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1, class U2, class U3, class U4, class U5>
	standard_c_type(U0 u0, U1 u1, U2 u2, U3 u3, U4 u4, U5 u5)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1, u2, u3, u4, u5))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1, class U2, class U3, class U4, class U5,
			class U6>
	standard_c_type(U0 u0, U1 u1, U2 u2, U3 u3, U4 u4, U5 u5, U6 u6)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1, u2, u3, u4, u5,
				u6))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1, class U2, class U3, class U4, class U5,
			class U6, class U7>
	standard_c_type(U0 u0, U1 u1, U2 u2, U3 u3, U4 u4, U5 u5, U6 u6, U7 u7)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1, u2, u3, u4, u5, u6,
				u7))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1, class U2, class U3, class U4, class U5,
			class U6, class U7, class U8>
	standard_c_type(U0 u0, U1 u1, U2 u2, U3 u3, U4 u4, U5 u5, U6 u6, U7 u7,
			U8 u8)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1, u2, u3, u4, u5, u6,
				u7, u8))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1, class U2, class U3, class U4, class U5,
			class U6, class U7, class U8, class U9>
	standard_c_type(U0 u0, U1 u1, U2 u2, U3 u3, U4 u4, U5 u5, U6 u6, U7 u7,
			U8 u8, U9 u9)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1, u2, u3, u4, u5, u6,
				u7, u8, u9))
			throw_or_abort(bad_init());
	}
#endif // __cplusplus >= 201103L

	~standard_c_type() { c_type_traits<T>::fini(c_ptr()); }
};

template <class T>
inline void
destroy(standard_c_type<T>* p) noexcept
{
	p->destroy();
}

/*!
 * The base class for a C++ interface to an incomplete C type. This class
 * requires a specialization of `c_type_traits<T>`.
 */
template <class T>
class incomplete_c_type {
public:
	typedef typename c_type_traits<T>::value_type c_value_type;
	typedef typename c_type_traits<T>::reference c_reference;
	typedef typename c_type_traits<T>::const_reference c_const_reference;
	typedef typename c_type_traits<T>::pointer c_pointer;
	typedef typename c_type_traits<T>::const_pointer c_const_pointer;

	operator c_reference() noexcept { return c_ref(); }
	operator c_const_reference() const noexcept { return c_ref(); }

	c_reference c_ref() noexcept { return *c_ptr(); }
	c_const_reference c_ref() const noexcept { return *c_ptr(); }

	c_pointer c_ptr() noexcept { return reinterpret_cast<c_pointer>(this); }

	c_const_pointer
	c_ptr() const noexcept
	{
		return reinterpret_cast<c_const_pointer>(this);
	}

	static void dtor(incomplete_c_type* p) noexcept { delete p; }
	void destroy() noexcept { dtor(this); }

	static void*
	operator new(std::size_t size)
	{
		void* ptr = operator new(size, ::std::nothrow);
		if (__unlikely(!ptr))
			throw_or_abort(std::bad_alloc());
		return ptr;
	}

	static void*
	operator new(std::size_t, const ::std::nothrow_t&) noexcept
	{
		return c_type_traits<T>::alloc();
	}

	static void
	operator delete(void* ptr) noexcept
	{
		operator delete(ptr, ::std::nothrow);
	}

	static void
	operator delete(void* ptr, const ::std::nothrow_t&) noexcept
	{
		c_type_traits<T>::free(ptr);
	}

	incomplete_c_type&
	operator=(const incomplete_c_type& val)
	{
		if (!c_type_traits<T>::copy(c_ptr(), val.c_ptr()))
			throw_or_abort(bad_copy());
		return *this;
	}

#if __cplusplus >= 201103L
	incomplete_c_type&
	operator=(incomplete_c_type&& val)
	{
		if (!c_type_traits<T>::move(c_ptr(), val.c_ptr()))
			throw_or_abort(bad_move());
		return *this;
	}
#endif

protected:
#if __cplusplus >= 201103L
	template <class... Args>
	explicit
	incomplete_c_type(Args&&... args)
	{
		if (!c_type_traits<T>::init(c_ptr(),
				::std::forward<Args>(args)...))
			throw_or_abort(bad_init());
	}
#else
	incomplete_c_type()
	{
		if (!c_type_traits<T>::init(c_ptr()))
			throw_or_abort(bad_init());
	}

	template <class U0>
	explicit
	incomplete_c_type(U0 u0)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1>
	incomplete_c_type(U0 u0, U1 u1)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1, class U2>
	incomplete_c_type(U0 u0, U1 u1, U2 u2)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1, u2))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1, class U2, class U3>
	incomplete_c_type(U0 u0, U1 u1, U2 u2, U3 u3)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1, u2, u3))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1, class U2, class U3, class U4>
	incomplete_c_type(U0 u0, U1 u1, U2 u2, U3 u3, U4 u4)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1, u2, u3, u4))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1, class U2, class U3, class U4, class U5>
	incomplete_c_type(U0 u0, U1 u1, U2 u2, U3 u3, U4 u4, U5 u5)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1, u2, u3, u4, u5))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1, class U2, class U3, class U4, class U5,
			class U6>
	incomplete_c_type(U0 u0, U1 u1, U2 u2, U3 u3, U4 u4, U5 u5, U6 u6)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1, u2, u3, u4, u5,
				u6))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1, class U2, class U3, class U4, class U5,
			class U6, class U7>
	incomplete_c_type(U0 u0, U1 u1, U2 u2, U3 u3, U4 u4, U5 u5, U6 u6,
			U7 u7)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1, u2, u3, u4, u5, u6,
				u7))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1, class U2, class U3, class U4, class U5,
			class U6, class U7, class U8>
	incomplete_c_type(U0 u0, U1 u1, U2 u2, U3 u3, U4 u4, U5 u5, U6 u6,
			U7 u7, U8 u8)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1, u2, u3, u4, u5, u6,
				u7, u8))
			throw_or_abort(bad_init());
	}

	template <class U0, class U1, class U2, class U3, class U4, class U5,
			class U6, class U7, class U8, class U9>
	incomplete_c_type(U0 u0, U1 u1, U2 u2, U3 u3, U4 u4, U5 u5, U6 u6,
			U7 u7, U8 u8, U9 u9)
	{
		if (!c_type_traits<T>::init(c_ptr(), u0, u1, u2, u3, u4, u5, u6,
				u7, u8, u9))
			throw_or_abort(bad_init());
	}
#endif // __cplusplus >= 201103L

	~incomplete_c_type() { c_type_traits<T>::fini(c_ptr()); }

#if __cplusplus >= 201103L
public:
	static void* operator new[](std::size_t) = delete;
	static void* operator new[](std::size_t, const ::std::nothrow_t&)
			= delete;
	static void operator delete[](void*) = delete;
	static void operator delete[](void*, const ::std::nothrow_t&) = delete;
#else
private:
	static void* operator new[](std::size_t);
	static void* operator new[](std::size_t, const ::std::nothrow_t&);
	static void operator delete[](void*);
	static void operator delete[](void*, const ::std::nothrow_t&);
#endif
};

template <class T>
inline void
destroy(incomplete_c_type<T>* p) noexcept
{
	p->destroy();
}

/*!
 * A class template supplying a uniform interface to certain attributes of
 * trivial and standard layout C types.
 */
template <class T>
struct c_type_traits {
	typedef T value_type;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;

	static void*
	alloc() noexcept
	{
		return operator new(sizeof(T), ::std::nothrow);
	}

	static void
	free(void* ptr) noexcept
	{
		operator delete(ptr, ::std::nothrow);
	}

	static pointer init(pointer p) noexcept { return p; }

	static pointer
	init(pointer p, const T& val) noexcept
	{
		return new (static_cast<void*>(p)) T(val);
	}

	static void fini(pointer p) noexcept { p->~T(); }

	static pointer
	copy(pointer p1, const_pointer p2) noexcept
	{
		*p1 = *p2;
		return p1;
	}

	static pointer
	move(pointer p1, pointer p2) noexcept
	{
#if __cplusplus >= 201103L
		*p1 = ::std::move(*p2);
#else
		*p1 = *p2;
#endif
		return p1;
	}
};

/*!
 * A class template supplying a uniform interface to certain attributes of the C
 * type `void`.
 */
template <>
struct c_type_traits<void> {
	typedef void value_type;
	typedef struct {} __type;
	typedef __type& reference;
	typedef const __type& const_reference;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;

	static void*
	alloc() noexcept
	{
		return operator new(0, ::std::nothrow);
	}

	static void
	free(void* ptr) noexcept
	{
		operator delete(ptr, ::std::nothrow);
	}

	static pointer init(pointer p) noexcept { return p; }

	static void fini(pointer p) noexcept { __unused_var(p); }

	static pointer
	copy(pointer p1, const_pointer p2) noexcept
	{
		__unused_var(p2);

		return p1;
	}

	static pointer
	move(pointer p1, pointer p2) noexcept
	{
		__unused_var(p2);

		return p1;
	}
};

} // lely

#endif

