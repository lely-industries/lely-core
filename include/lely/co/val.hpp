/*!\file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the CANopen value declarations. See lely/co/val.h for the C
 * interface.
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

#ifndef LELY_CO_VAL_HPP
#define LELY_CO_VAL_HPP

#ifndef __cplusplus
#error "include <lely/co/val.h> for the C interface"
#endif

#include <lely/util/c_type.hpp>
#include <lely/co/type.hpp>
#include <lely/co/val.h>

namespace lely {

//! A CANopen value.
template <co_unsigned16_t N>
class COVal {
	typedef co_type_traits_N<N> traits;

public:
	static const co_unsigned16_t index = traits::index;

	typedef typename traits::type type;

	operator type() const noexcept { return m_val; }
	operator type&() noexcept { return m_val; }
	operator const type&() const noexcept { return m_val; }

	COVal(): m_val() {}

#if __cplusplus >= 201103L
	COVal(const COVal&) = default;
	COVal(COVal&&) = default;
	COVal(const type& val): m_val(val) {}
	COVal(type&& val): m_val(::std::move(val)) {}
#else
	COVal(const COVal& val): m_val(val.m_val) {}
	COVal(const type& val): m_val(val) {}
#endif

	COVal(const void* ptr, ::std::size_t n)
	{
		if (__unlikely(!co_val_make(index, this, ptr, n)))
			throw_or_abort(bad_init());
	}

#if __cplusplus >= 201103L
	COVal& operator=(const COVal&) = default;
	COVal& operator=(COVal&&) = default;
#else
	COVal& operator=(const COVal& val) { m_val = val.m_val; return *this; }
#endif
	COVal& operator=(const type& val) { m_val = val; return *this; }
#if __cplusplus >= 201103L
	COVal& operator=(type&& val) { m_val = ::std::move(val); return *this; }
#endif

	const void*
	address() const noexcept
	{
		return static_cast<const void*>(&m_val);
	}

	::std::size_t size() const noexcept { return sizeof(type); }

private:
	type m_val;
};

//! A CANopen value containing an array of visible characters.
template <>
class COVal<CO_DEFTYPE_VISIBLE_STRING> {
	typedef co_type_traits_N<CO_DEFTYPE_VISIBLE_STRING> traits;

public:
	static const co_unsigned16_t index = traits::index;

	typedef typename traits::type type;

	operator type() const noexcept { return m_val; }

	COVal(): m_val() {}
	COVal(const COVal& val): m_val() { *this = val; }
#if __cplusplus >= 201103L
	COVal(COVal&& val): m_val() { *this = ::std::move(val); }
#endif
	COVal(const void* ptr, ::std::size_t n)
	{
		if (__unlikely(!co_val_make(index, this, ptr, n)))
			throw_or_abort(bad_init());
	}

	COVal(const char* vs) { init(vs); }
	COVal(const ::std::string& vs) { init(vs); }

	~COVal() { co_val_fini(index, &m_val); }

	COVal&
	operator=(const COVal& val)
	{
		this->~COVal();
		if (__unlikely(!co_val_copy(index, &m_val, &val.m_val)))
			throw_or_abort(bad_copy());
		return *this;
	}

#if __cplusplus >= 201103L
	COVal&
	operator=(COVal&& val)
	{
		this->~COVal();
		if (__unlikely(!co_val_move(index, &m_val, &val.m_val)))
			throw_or_abort(bad_move());
		return *this;
	}
#endif

	COVal&
	operator=(const char* vs)
	{
		this->~COVal();
		init(vs);
		return *this;
	}

	COVal&
	operator=(const ::std::string& vs)
	{
		this->~COVal();
		init(vs);
		return *this;
	}

	const void*
	address() const noexcept
	{
		return co_val_addressof(index, this);
	}

	::std::size_t
	size() const noexcept
	{
		return co_val_sizeof(index, this);
	}

private:
	void
	init(const char* vs)
	{
		if (__unlikely(co_val_init_vs(&m_val, vs) == -1))
			throw_or_abort(bad_init());
	}

	void init(const ::std::string& vs) { init(vs.c_str()); }

	type m_val;
};

//! A CANopen value containing an array of octets.
template <>
class COVal<CO_DEFTYPE_OCTET_STRING> {
	typedef co_type_traits_N<CO_DEFTYPE_OCTET_STRING> traits;

public:
	static const co_unsigned16_t index = traits::index;

	typedef typename traits::type type;

	operator type() const noexcept { return m_val; }

	COVal(): m_val() {}
	COVal(const COVal& val): m_val() { *this = val; }
#if __cplusplus >= 201103L
	COVal(COVal&& val): m_val() { *this = ::std::move(val); }
#endif

	COVal(const void* ptr, ::std::size_t n)
	{
		if (__unlikely(!co_val_make(index, this, ptr, n)))
			throw_or_abort(bad_init());
	}

	COVal(const uint8_t* os, ::std::size_t n) { init(os, n); }
	COVal(const ::std::vector<uint8_t>& os) { init(os); }

	~COVal() { co_val_fini(index, &m_val); }

	COVal&
	operator=(const COVal& val)
	{
		this->~COVal();
		if (__unlikely(!co_val_copy(index, &m_val, &val.m_val)))
			throw_or_abort(bad_copy());
		return *this;
	}

#if __cplusplus >= 201103L
	COVal&
	operator=(COVal&& val)
	{
		this->~COVal();
		if (__unlikely(!co_val_move(index, &m_val, &val.m_val)))
			throw_or_abort(bad_move());
		return *this;
	}
#endif

	COVal&
	operator=(const ::std::vector<uint8_t>& os)
	{
		this->~COVal();
		init(os);
		return *this;
	}

	const void*
	address() const noexcept
	{
		return co_val_addressof(index, this);
	}

	::std::size_t
	size() const noexcept
	{
		return co_val_sizeof(index, this);
	}

private:
	void
	init(const uint8_t* os, ::std::size_t n)
	{
		if (__unlikely(co_val_init_os(&m_val, os, n)))
			throw_or_abort(bad_init());
	}

	void
	init(const ::std::vector<uint8_t>& os)
	{
		init(os.data(), os.size());
	}

	type m_val;
};

//! A CANopen value containing an array of (16-bit) Unicode characters.
template <>
class COVal<CO_DEFTYPE_UNICODE_STRING> {
	typedef co_type_traits_N<CO_DEFTYPE_UNICODE_STRING> traits;

public:
	static const co_unsigned16_t index = traits::index;

	typedef typename traits::type type;

	operator type() const noexcept { return m_val; }

	COVal(): m_val() {}
	COVal(const COVal& val): m_val() { *this = val; }
#if __cplusplus >= 201103L
	COVal(COVal&& val): m_val() { *this = ::std::move(val); }
#endif

	COVal(const void* ptr, ::std::size_t n)
	{
		if (__unlikely(!co_val_make(index, this, ptr, n)))
			throw_or_abort(bad_init());
	}

	COVal(const char16_t* us) { init(us); }
	COVal(const ::std::basic_string<char16_t>& us) { init(us); }

	~COVal() { co_val_fini(index, &m_val); }

	COVal&
	operator=(const COVal& val)
	{
		this->~COVal();
		if (__unlikely(!co_val_copy(index, &m_val, &val.m_val)))
			throw_or_abort(bad_copy());
		return *this;
	}

#if __cplusplus >= 201103L
	COVal&
	operator=(COVal&& val)
	{
		this->~COVal();
		if (__unlikely(!co_val_move(index, &m_val, &val.m_val)))
			throw_or_abort(bad_move());
		return *this;
	}
#endif

	COVal&
	operator=(const char16_t* us)
	{
		this->~COVal();
		init(us);
		return *this;
	}

	COVal&
	operator=(const ::std::basic_string<char16_t>& us)
	{
		this->~COVal();
		init(us);
		return *this;
	}

	const void*
	address() const noexcept
	{
		return co_val_addressof(index, this);
	}

	::std::size_t
	size() const noexcept
	{
		return co_val_sizeof(index, this);
	}

private:
	void
	init(const char16_t* us)
	{
		if (__unlikely(co_val_init_us(&m_val, us) == -1))
			throw_or_abort(bad_init());
	}

	void init(const ::std::basic_string<char16_t>& us) { init(us.c_str()); }

	type m_val;
};

//! A CANopen value containing an arbitrary large block of data.
template <>
class COVal<CO_DEFTYPE_DOMAIN> {
	typedef co_type_traits_N<CO_DEFTYPE_DOMAIN> traits;

public:
	static const co_unsigned16_t index = traits::index;

	typedef typename traits::type type;

	operator type() const noexcept { return m_val; }

	COVal(): m_val() {}
	COVal(const COVal& val): m_val() { *this = val; }
#if __cplusplus >= 201103L
	COVal(COVal&& val): m_val() { *this = ::std::move(val); }
#endif

	COVal(const void* dom, ::std::size_t n)
	{
		if (__unlikely(co_val_init_dom(&m_val, dom, n)))
			throw_or_abort(bad_init());
	}

	~COVal() { co_val_fini(index, &m_val); }

	COVal&
	operator=(const COVal& val)
	{
		this->~COVal();
		if (__unlikely(!co_val_copy(index, &m_val, &val.m_val)))
			throw_or_abort(bad_copy());
		return *this;
	}

#if __cplusplus >= 201103L
	COVal&
	operator=(COVal&& val)
	{
		this->~COVal();
		if (__unlikely(!co_val_move(index, &m_val, &val.m_val)))
			throw_or_abort(bad_move());
		return *this;
	}
#endif

	const void*
	address() const noexcept
	{
		return co_val_addressof(index, this);
	}

	::std::size_t
	size() const noexcept
	{
		return co_val_sizeof(index, this);
	}

private:
	type m_val;
};

} // lely

#endif

