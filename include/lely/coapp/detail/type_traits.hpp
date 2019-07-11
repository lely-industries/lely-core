/**@file
 * This header file is part of the C++ CANopen application library; it contains
 * the CANopen type traits.
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

#ifndef LELY_COAPP_DETAIL_TYPE_TRAITS_HPP_
#define LELY_COAPP_DETAIL_TYPE_TRAITS_HPP_

#include <lely/co/type.h>

#include <string>
#include <type_traits>
#include <vector>

namespace lely {

namespace canopen {

namespace detail {

/**
 * If <b>T</b> is one of the CANopen basic types, provides the member constant
 * `value` equal to `true`. For any other type, `value` is `false`.
 */
template <class T>
struct is_canopen_basic : ::std::false_type {};

/**
 * If <b>T</b> is one of the CANopen array types, provides the member constant
 * `value` equal to `true`. For any other type, `value` is `false`.
 */
template <class T>
struct is_canopen_array : ::std::false_type {};

/**
 * If <b>T</b> is one of the CANopen basic or array types, provides the member
 * constant `value` equal to `true`. For any other type, `value` is `false`.
 */
template <class T>
struct is_canopen_type
    : ::std::integral_constant<bool, is_canopen_basic<T>::value ||
                                         is_canopen_array<T>::value> {};

/**
 * Specialization of #lely::canopen::detail::is_canopen_basic for the CANopen
 * basic type BOOLEAN.
 */
template <>
struct is_canopen_basic<bool> : ::std::true_type {};

/**
 * Specialization of #lely::canopen::detail::is_canopen_basic for the CANopen
 * basic type INTEGER8.
 */
template <>
struct is_canopen_basic<int8_t> : ::std::true_type {};

/**
 * Specialization of #lely::canopen::detail::is_canopen_basic for the CANopen
 * basic type INTEGER16.
 */
template <>
struct is_canopen_basic<int16_t> : ::std::true_type {};

/**
 * Specialization of #lely::canopen::detail::is_canopen_basic for the CANopen
 * basic type INTEGER32.
 */
template <>
struct is_canopen_basic<int32_t> : ::std::true_type {};

/**
 * Specialization of #lely::canopen::detail::is_canopen_basic for the CANopen
 * basic type UNSIGNED8.
 */
template <>
struct is_canopen_basic<uint8_t> : ::std::true_type {};

/**
 * Specialization of #lely::canopen::detail::is_canopen_basic for the CANopen
 * basic type UNSIGNED16.
 */
template <>
struct is_canopen_basic<uint16_t> : ::std::true_type {};

/**
 * Specialization of #lely::canopen::detail::is_canopen_basic for the CANopen
 * basic type UNSIGNED32.
 */
template <>
struct is_canopen_basic<uint32_t> : ::std::true_type {};

/**
 * Specialization of #lely::canopen::detail::is_canopen_basic for the CANopen
 * basic type REAL32.
 */
template <>
struct is_canopen_basic<float> : ::std::true_type {};

/**
 * Specialization of #lely::canopen::detail::is_canopen_array for the CANopen
 * array type VISIBLE_STRING.
 */
template <>
struct is_canopen_array<::std::string> : ::std::true_type {};

/**
 * Specialization of #lely::canopen::detail::is_canopen_array for the CANopen
 * array type OCTET_STRING.
 */
template <>
struct is_canopen_array<::std::vector<uint8_t>> : ::std::true_type {};

/**
 * Specialization of #lely::canopen::detail::is_canopen_array for the CANopen
 * array type UNICODE_STRING.
 */
template <>
struct is_canopen_array<::std::basic_string<char16_t>> : ::std::true_type {};

// TIME_OF_DAY
// TIME_DIFFERENCE
// DOMAIN
// INTEGER24

/**
 * Specialization of #lely::canopen::detail::is_canopen_basic for the CANopen
 * basic type REAL64.
 */
template <>
struct is_canopen_basic<double> : ::std::true_type {};

// INTEGER40
// INTEGER48
// INTEGER56

/**
 * Specialization of #lely::canopen::detail::is_canopen_basic for the CANopen
 * basic type INTEGER64.
 */
template <>
struct is_canopen_basic<int64_t> : ::std::true_type {};

// UNSIGNED24
// UNSIGNED40
// UNSIGNED48
// UNSIGNED56

/**
 * Specialization of #lely::canopen::detail::is_canopen_basic for the CANopen
 * basic type UNSIGNED64.
 */
template <>
struct is_canopen_basic<uint64_t> : ::std::true_type {};

/**
 * Returns `true` if the CANopen data types <b>t1</b> and <b>t2</b> map to the
 * same C++ type, and `false` if not.
 */
inline bool
is_canopen_same(uint16_t t1, uint16_t t2) {
  if (t1 == t2) return true;

  // OCTET_STRING and DOMAIN are both byte arrays.
  if ((t1 == CO_DEFTYPE_OCTET_STRING && t2 == CO_DEFTYPE_DOMAIN) ||
      (t1 == CO_DEFTYPE_DOMAIN && t2 == CO_DEFTYPE_OCTET_STRING))
    return true;

  return false;
}

}  // namespace detail

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_DETAIL_TYPE_TRAITS_HPP_
