/**@file
 * This header file is part of the utilities library; it contains the C++
 * interface of the random number generator.
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

#ifndef LELY_UTIL_RAND_HPP
#define LELY_UTIL_RAND_HPP

#include <lely/util/c_type.hpp>
#include <lely/util/rand.h>

#include <istream>
#include <limits>
#include <ostream>
#if __cplusplus >= 201103L
#include <type_traits>
#endif

namespace lely {

namespace impl {

template <class> struct rand_traits;

} // impl

/**
 * A uniformly distributed random number generator meeting the random number
 * engine requirements.
 */
template <class T>
class Rand
	: public standard_c_type<typename impl::rand_traits<T>::c_type>
	, private impl::rand_traits<T>::c_type {
	typedef standard_c_type<typename impl::rand_traits<T>::c_type> c_base;
public:
	typedef T result_type;

#if __cplusplus >= 201103L
	static constexpr result_type
#else
	static result_type
#endif
	min() { return ::std::numeric_limits<result_type>::min(); }

#if __cplusplus >= 201103L
	static constexpr result_type
#else
	static result_type
#endif
	max() { return ::std::numeric_limits<result_type>::max(); }

#if __cplusplus >= 201103L
	static constexpr result_type default_seed = 0;
#else
	static const result_type default_seed = 0;
#endif

	explicit Rand(result_type s = default_seed) { seed(s); }

#if __cplusplus >= 201103L
	template <class Sseq, class = typename ::std::enable_if<
		!::std::is_same<Sseq, Rand>::value
	>::type>
	explicit Rand(Sseq& q) { seed(q); }
#endif

	void
	seed(result_type s = default_seed)
	{
		impl::rand_traits<T>::seed(c_base::c_ptr(), s);
	}

#if __cplusplus >= 201103L
	template <class Sseq>
	typename ::std::enable_if<::std::is_class<Sseq>::value>::type
	seed(Sseq& q)
	{
		uint_least32_t a[2];
		q.generate(a, a + 2);

		uint64_t s = a[0];
		s = (s << 32) | a[1];

		impl::rand_traits<T>::seed(c_base::c_ptr(), s);
	}
#endif

	result_type
	operator()()
	{
		return impl::rand_traits<T>::get(c_base::c_ptr());
	}

	void
#if __cplusplus >= 201103L
	discard(unsigned long long z)
#else
	discard(uint64_t z)
#endif
	{
		impl::rand_traits<T>::discard(c_base::c_ptr(), z);
	}

	friend bool
	operator==(const Rand& lhs, const Rand& rhs)
	{
		return impl::rand_traits<T>::eq(lhs.c_ptr(), rhs.c_ptr());
	}

	template <class CharT, class Traits>
	friend ::std::basic_ostream<CharT,Traits>&
	operator<<(::std::basic_ostream<CharT,Traits>& ost, const Rand& e)
	{
		::std::ios_base::fmtflags flags = ost.flags();
		CharT fill = ost.fill();
		ost.fill(ost.widen(' '));

		impl::rand_traits<T>::write(e.c_ptr(), ost);

		ost.flags(flags);
		ost.fill(fill);
		return ost;
	}

	template <class CharT, class Traits>
	friend ::std::basic_istream<CharT,Traits>&
	operator>>(::std::basic_istream<CharT,Traits>& ist, Rand& e)
	{
		::std::ios_base::fmtflags flags = ist.flags();
		ist.flags(::std::ios_base::dec | ::std::ios_base::skipws);

		impl::rand_traits<T>::read(e.c_ptr(), ist);

		ist.flags(flags);
		return ist;
	}
};

template <class T>
inline bool
operator!=(const Rand<T>& lhs, const Rand<T>& rhs)
{
	return !(lhs == rhs);
}

namespace impl {

/// A class template supplying a wrapper around the #rand64 functions.
template <>
struct rand_traits<uint64_t> {
	typedef rand64 c_type;

	static void seed(c_type* p, uint64_t seed) { rand64_seed(p, seed); }
	static uint64_t get(c_type* p) { return rand64_get(p); }
	static void discard(c_type* p, uint64_t z) { rand64_discard(p, z); }

	static bool
	eq(const c_type* lhs, const c_type* rhs)
	{
		return lhs->u == rhs->u && lhs->v == rhs->v && lhs->w == rhs->w;
	}

	template <class CharT, class Traits>
	static void
	read(c_type* p, ::std::basic_istream<CharT,Traits>& ist)
	{
		ist >> p->u;
		ist >> p->v;
		ist >> p->w;
	}

	template <class CharT, class Traits>
	static void
	write(const c_type* p, ::std::basic_ostream<CharT,Traits>& ost)
	{
		ost << p->u;
		ost << ost.widen(' ');
		ost << p->v;
		ost << ost.widen(' ');
		ost << p->w;
	}
};

/// A class template supplying a wrapper around the #rand32 functions.
template <>
struct rand_traits<uint32_t> {
	typedef rand32 c_type;

	static void seed(c_type* p, uint64_t seed) { rand32_seed(p, seed); }
	static uint32_t get(c_type* p) { return rand32_get(p); }
	static void discard(c_type* p, uint64_t z) { rand32_discard(p, z); }

	static bool
	eq(const c_type* lhs, const c_type* rhs)
	{
		return rand_traits<uint64_t>::eq(&lhs->r, &rhs->r)
				&& lhs->x == rhs->x && lhs->n == rhs->n;
	}

	template <class CharT, class Traits>
	static void
	read(c_type* p, ::std::basic_istream<CharT,Traits>& ist)
	{
		rand_traits<uint64_t>::read(&p->r, ist);

		ist >> p->x;
		ist >> p->n;
	}

	template <class CharT, class Traits>
	static void
	write(const c_type* p, ::std::basic_ostream<CharT,Traits>& ost)
	{
		rand_traits<uint64_t>::write(&p->r, ost);

		ost << ost.widen(' ');
		ost << p->x;
		ost << ost.widen(' ');
		ost << p->n;
	}
};

/// A class template supplying a wrapper around the #rand16 functions.
template <>
struct rand_traits<uint16_t> {
	typedef rand16 c_type;

	static void seed(c_type* p, uint64_t seed) { rand16_seed(p, seed); }
	static uint16_t get(c_type* p) { return rand16_get(p); }
	static void discard(c_type* p, uint64_t z) { rand16_discard(p, z); }

	static bool
	eq(const c_type* lhs, const c_type* rhs)
	{
		return rand_traits<uint64_t>::eq(&lhs->r, &rhs->r)
				&& lhs->x == rhs->x && lhs->n == rhs->n;
	}

	template <class CharT, class Traits>
	static void
	read(c_type* p, ::std::basic_istream<CharT,Traits>& ist)
	{
		rand_traits<uint64_t>::read(&p->r, ist);

		ist >> p->x;
		ist >> p->n;
	}

	template <class CharT, class Traits>
	static void
	write(const c_type* p, ::std::basic_ostream<CharT,Traits>& ost)
	{
		rand_traits<uint64_t>::write(&p->r, ost);

		ost << ost.widen(' ');
		ost << p->x;
		ost << ost.widen(' ');
		ost << p->n;
	}
};

/// A class template supplying a wrapper around the #rand8 functions.
template <>
struct rand_traits<uint8_t> {
	typedef rand8 c_type;

	static void seed(c_type* p, uint64_t seed) { rand8_seed(p, seed); }
	static uint8_t get(c_type* p) { return rand8_get(p); }
	static void discard(c_type* p, uint64_t z) { rand8_discard(p, z); }

	static bool
	eq(const c_type* lhs, const c_type* rhs)
	{
		return rand_traits<uint64_t>::eq(&lhs->r, &rhs->r)
				&& lhs->x == rhs->x && lhs->n == rhs->n;
	}

	template <class CharT, class Traits>
	static void
	read(c_type* p, ::std::basic_istream<CharT,Traits>& ist)
	{
		rand_traits<uint64_t>::read(&p->r, ist);

		ist >> p->x;
		ist >> p->n;
	}

	template <class CharT, class Traits>
	static void
	write(const c_type* p, ::std::basic_ostream<CharT,Traits>& ost)
	{
		rand_traits<uint64_t>::write(&p->r, ost);

		ost << ost.widen(' ');
		ost << p->x;
		ost << ost.widen(' ');
		ost << p->n;
	}
};

} // lely::impl

typedef Rand<uint8_t> Rand8;
typedef Rand<uint16_t> Rand16;
typedef Rand<uint32_t> Rand32;
typedef Rand<uint64_t> Rand64;

} // lely

#endif

