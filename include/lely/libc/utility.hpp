/**@file
 * This header file is part of the compatibility library; it includes
 * `<utility>` and defines any missing functionality.
 *
 * @copyright 2018-2019 Lely Industries N.V.
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

#ifndef LELY_LIBC_UTILITY_HPP_
#define LELY_LIBC_UTILITY_HPP_

#include <lely/features.h>

#include <utility>

namespace lely {
namespace compat {

#if __cplusplus >= 201402L

using ::std::index_sequence;
using ::std::index_sequence_for;
using ::std::integer_sequence;
using ::std::make_index_sequence;
using ::std::make_integer_sequence;

#else  // __cplusplus < 201402L

/// A compile-time sequence of integers.
template <typename T, T... Ints>
struct integer_sequence {
  typedef T value_type;

  static constexpr ::std::size_t
  size() noexcept {
    return sizeof...(Ints);
  }
};

namespace detail {

template <::std::size_t...>
struct index_tuple {};

template <class, class>
struct index_tuple_cat;

template <::std::size_t... I1, ::std::size_t... I2>
struct index_tuple_cat<index_tuple<I1...>, index_tuple<I2...>> {
  using type = index_tuple<I1..., (I2 + sizeof...(I1))...>;
};

template <::std::size_t N>
struct make_index_tuple
    : index_tuple_cat<typename make_index_tuple<N / 2>::type,
                      typename make_index_tuple<N - N / 2>::type> {};

template <>
struct make_index_tuple<1> {
  typedef index_tuple<0> type;
};

template <>
struct make_index_tuple<0> {
  typedef index_tuple<> type;
};

template <class T, T N, class = typename make_index_tuple<N>::type>
struct make_integer_sequence;

template <typename T, T N, ::std::size_t... Ints>
struct make_integer_sequence<T, N, index_tuple<Ints...>> {
  static_assert(N >= 0, "Cannot make integer sequence of negative length");

  typedef integer_sequence<T, static_cast<T>(Ints)...> type;
};

}  // namespace detail

/**
 * A helper alias template for #lely::compat::integer_sequence for the common
 * case where `T` is `std::size_t`.
 */
template <::std::size_t... Ints>
using index_sequence = integer_sequence<::std::size_t, Ints...>;

/**
 * A helper alias template to simplify the creation of
 * #lely::compat::integer_sequence types with 0, 1, 2, ..., N - 1 as `Ints`.
 */
template <typename T, T N>
using make_integer_sequence =
    typename detail::make_integer_sequence<T, N>::type;

/**
 * A helper alias template for `make_integer_sequence` for the common case where
 * `T` is `std::size_t`.
 */
template <::std::size_t N>
using make_index_sequence = make_integer_sequence<::std::size_t, N>;

/**
 * A helper alias template to convert any type parameter pack into an index
 * sequence of the same length.
 */
template <typename... T>
using index_sequence_for = make_index_sequence<sizeof...(T)>;

#endif  // __cplusplus < 201402L

}  // namespace compat
}  // namespace lely

#endif  // !LELY_LIBC_UTILITY_HPP_
