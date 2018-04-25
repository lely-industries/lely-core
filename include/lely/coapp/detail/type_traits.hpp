/*!\file
 * This header file is part of the C++ CANopen application library; it contains
 * the CANopen type traits.
 *
 * \copyright 2018 Lely Industries N.V.
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

#ifndef LELY_COAPP_DETAIL_TYPE_TRAITS_HPP_
#define LELY_COAPP_DETAIL_TYPE_TRAITS_HPP_

#include <lely/co/type.h>
#include <lely/coapp/coapp.hpp>

#include <string>
#include <type_traits>
#include <vector>

namespace lely {

namespace canopen {

namespace detail {

/*!
 * If \a T is one of the CANopen basic types, provides the member constant
 * `value` equal to `true`. For any other type, `value` is `false`.
 */
template <class T> struct IsCanopenBasic : ::std::false_type {};

/*!
 * If \a T is one of the CANopen array types, provides the member constant
 * `value` equal to `true`. For any other type, `value` is `false`.
 */
template <class T> struct IsCanopenArray : ::std::false_type {};

/*!
 * If \a T is one of the CANopen basic or array types, provides the member
 * constant `value` equal to `true`. For any other type, `value` is `false`.
 */
template <class T>
struct IsCanopenType
    : ::std::integral_constant<
          bool, IsCanopenBasic<T>::value || IsCanopenArray<T>::value
      > {};

/*!
 * Specialization of #lely::canopen::detail::IsCanopenBasic for the CANopen
 * basic type BOOLEAN.
 */
template <> struct IsCanopenBasic<bool> : ::std::true_type {};

/*!
 * Specialization of #lely::canopen::detail::IsCanopenBasic for the CANopen
 * basic type INTEGER8.
 */
template <> struct IsCanopenBasic<int8_t> : ::std::true_type {};

/*!
 * Specialization of #lely::canopen::detail::IsCanopenBasic for the CANopen
 * basic type INTEGER16.
 */
template <> struct IsCanopenBasic<int16_t> : ::std::true_type {};

/*!
 * Specialization of #lely::canopen::detail::IsCanopenBasic for the CANopen
 * basic type INTEGER32.
 */
template <> struct IsCanopenBasic<int32_t> : ::std::true_type {};

/*!
 * Specialization of #lely::canopen::detail::IsCanopenBasic for the CANopen
 * basic type UNSIGNED8.
 */
template <> struct IsCanopenBasic<uint8_t> : ::std::true_type {};

/*!
 * Specialization of #lely::canopen::detail::IsCanopenBasic for the CANopen
 * basic type UNSIGNED16.
 */
template <> struct IsCanopenBasic<uint16_t> : ::std::true_type {};

/*!
 * Specialization of #lely::canopen::detail::IsCanopenBasic for the CANopen
 * basic type UNSIGNED32.
 */
template <> struct IsCanopenBasic<uint32_t> : ::std::true_type {};

/*!
 * Specialization of #lely::canopen::detail::IsCanopenBasic for the CANopen
 * basic type REAL32.
 */
template <> struct IsCanopenBasic<float> : ::std::true_type {};

/*!
 * Specialization of #lely::canopen::detail::IsCanopenArray for the CANopen
 * array type VISIBLE_STRING.
 */
template <> struct IsCanopenArray<::std::string> : ::std::true_type {};

/*!
 * Specialization of #lely::canopen::detail::IsCanopenArray for the CANopen
 * array type OCTET_STRING.
 */
template <> struct IsCanopenArray<::std::vector<uint8_t>> : ::std::true_type {};

/*!
 * Specialization of #lely::canopen::detail::IsCanopenArray for the CANopen
 * array type UNICODE_STRING.
 */
template <>
struct IsCanopenArray<::std::basic_string<char16_t>> : ::std::true_type {};

// TIME_OF_DAY
// TIME_DIFFERENCE
// DOMAIN
// INTEGER24

/*!
 * Specialization of #lely::canopen::detail::IsCanopenBasic for the CANopen
 * basic type REAL64.
 */
template <> struct IsCanopenBasic<double> : ::std::true_type {};

// INTEGER40
// INTEGER48
// INTEGER56

/*!
 * Specialization of #lely::canopen::detail::IsCanopenBasic for the CANopen
 * basic type INTEGER64.
 */
template <> struct IsCanopenBasic<int64_t> : ::std::true_type {};

// UNSIGNED24
// UNSIGNED40
// UNSIGNED48
// UNSIGNED56

/*!
 * Specialization of #lely::canopen::detail::IsCanopenBasic for the CANopen
 * basic type UNSIGNED64.
 */
template <> struct IsCanopenBasic<uint64_t> : ::std::true_type {};

/*!
 * Returns `true` if the CANopen data types \a t1 and \a t2 map to the same C++
 * type, and `false` if not.
 */
inline bool
IsCanopenSame(uint16_t t1, uint16_t t2) {
  if (t1 == t2)
    return true;

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
