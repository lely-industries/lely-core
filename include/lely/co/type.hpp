/**@file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the CANopen type definitions. See lely/co/type.h for the C
 * interface.
 *
 * @copyright 2016-2020 Lely Industries N.V.
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

#ifndef LELY_CO_TYPE_HPP_
#define LELY_CO_TYPE_HPP_

#if !defined(__cplusplus) || LELY_NO_CXX
#error "include <lely/co/type.h> for the C interface"
#endif

#include <lely/co/type.h>

#include <string>
#include <vector>

namespace lely {

/// A class template mapping CANopen types to C++ types.
template <co_unsigned16_t N, class T>
struct co_type_traits {
  /// The CANopen object index of the type definition.
  static const co_unsigned16_t index = N;
  /// The C++ type.
  typedef T type;
};

/// A class template mapping CANopen types to C++ types.
template <co_unsigned16_t N>
struct co_type_traits_N;

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  template <> \
  struct co_type_traits_N<CO_DEFTYPE_##a> \
      : co_type_traits<CO_DEFTYPE_##a, co_##b##_t> {};
#include <lely/co/def/type.def>
#undef LELY_CO_DEFINE_TYPE

/// A class template mapping CANopen types to C++ types.
template <class T>
struct co_type_traits_T;

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  template <> \
  struct co_type_traits_T<d> : co_type_traits<CO_DEFTYPE_##a, d> {};
#include <lely/co/def/cxx.def>
#undef LELY_CO_DEFINE_TYPE

template <::std::size_t N>
struct co_type_traits_T<char[N]>
    : co_type_traits<CO_DEFTYPE_VISIBLE_STRING, char[N]> {};

template <::std::size_t N>
struct co_type_traits_T<char16_t[N]>
    : co_type_traits<CO_DEFTYPE_UNICODE_STRING, char16_t[N]> {};

}  // namespace lely

#endif  // !LELY_CO_TYPE_HPP_
