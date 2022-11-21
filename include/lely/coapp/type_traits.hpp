/**@file
 * This header file is part of the C++ CANopen application library; it contains
 * the CANopen type traits.
 *
 * @copyright 2018-2022 Lely Industries N.V.
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

#ifndef LELY_COAPP_TYPE_TRAITS_HPP_
#define LELY_COAPP_TYPE_TRAITS_HPP_

#include <lely/features.h>

#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

#include <cstdint>

namespace lely {

namespace canopen {

namespace detail {

template <class, uint16_t, bool = false>
struct canopen_traits;

template <class T, uint16_t N>
struct canopen_traits<T, N, false> {
  using type = T;

  static constexpr uint16_t index = N;

  static constexpr bool is_basic = false;
};

template <class T, uint16_t N>
struct canopen_traits<T, N, true> {
  using type = T;

  using c_type = T;

  static constexpr uint16_t index = N;

  static constexpr bool is_basic = true;

  static c_type construct(const void* p, ::std::size_t n,
                          ::std::error_code& ec) noexcept;

  static void
  destroy(c_type& /*val*/) noexcept {}

  static constexpr type
  from_c_type(c_type val) noexcept {
    return val;
  }

  static constexpr c_type
  to_c_type(type value, ::std::error_code& /*ec*/) noexcept {
    return value;
  }

  static constexpr void*
  address(c_type& val) noexcept {
    return &val;
  }

  static constexpr const void*
  address(const c_type& val) noexcept {
    return &val;
  }

  static constexpr ::std::size_t
  size(const c_type& /*val*/) noexcept {
    return sizeof(c_type);
  }
};

}  // namespace detail

/// A class template mapping CANopen types to C and C++ types.
template <class>
struct canopen_traits;

/**
 * Specialization of #lely::canopen::canopen_traits for the CANopen basic type
 * BOOLEAN.
 */
template <>
struct canopen_traits<bool> : detail::canopen_traits<bool, 0x0001, true> {};

/**
 * Specialization of #lely::canopen::canopen_traits for the CANopen basic type
 * INTEGER8.
 */
template <>
struct canopen_traits<int8_t> : detail::canopen_traits<int8_t, 0x0002, true> {};

/**
 * Specialization of #lely::canopen::canopen_traits for the CANopen basic type
 * INTEGER16.
 */
template <>
struct canopen_traits<int16_t> : detail::canopen_traits<int16_t, 0x0003, true> {
};

/**
 * Specialization of #lely::canopen::canopen_traits for the CANopen basic type
 * INTEGER32.
 */
template <>
struct canopen_traits<int32_t> : detail::canopen_traits<int32_t, 0x0004, true> {
};

/**
 * Specialization of #lely::canopen::canopen_traits for the CANopen basic type
 * UNSIGNED8.
 */
template <>
struct canopen_traits<uint8_t> : detail::canopen_traits<uint8_t, 0x0005, true> {
};

/**
 * Specialization of #lely::canopen::canopen_traits for the CANopen basic type
 * UNSIGNED16.
 */
template <>
struct canopen_traits<uint16_t>
    : detail::canopen_traits<uint16_t, 0x0006, true> {};

/**
 * Specialization of #lely::canopen::canopen_traits for the CANopen basic type
 * UNSIGNED32.
 */
template <>
struct canopen_traits<uint32_t>
    : detail::canopen_traits<uint32_t, 0x0007, true> {};

/**
 * Specialization of #lely::canopen::canopen_traits for the CANopen basic type
 * REAL32.
 */
template <>
struct canopen_traits<float> : detail::canopen_traits<float, 0x0008, true> {};

/**
 * Specialization of #lely::canopen::canopen_traits for the CANopen array type
 * VISIBLE_STRING.
 */
template <>
struct canopen_traits<::std::string>
    : detail::canopen_traits<::std::string, 0x0009> {
  using c_type = char*;

  static char* construct(const void* p, ::std::size_t n,
                         ::std::error_code& ec) noexcept;

  static void destroy(char*& val) noexcept;

  static ::std::string
  from_c_type(const char* val) {
    return val ? ::std::string{val} : ::std::string{};
  }

  static char*
  to_c_type(const type& vs, ::std::error_code& ec) noexcept {
    return to_c_type(vs.c_str(), ec);
  }

  static char* to_c_type(const char* vs, ::std::error_code& ec) noexcept;

  static constexpr void*
  address(char* val) noexcept {
    return val;
  }

  static constexpr const void*
  address(const char* val) noexcept {
    return val;
  }

  static ::std::size_t
  size(const char* val) noexcept {
    return val ? ::std::char_traits<char>::length(val) : 0;
  }
};

/**
 * Specialization of #lely::canopen::canopen_traits for the CANopen array type
 * OCTET_STRING.
 */
template <>
struct canopen_traits<::std::vector<uint8_t>>
    : detail::canopen_traits<::std::vector<uint8_t>, 0x000a> {
  using c_type = uint8_t*;

  static uint8_t* construct(const void* p, ::std::size_t n,
                            ::std::error_code& ec) noexcept;

  static void destroy(uint8_t*& val) noexcept;

  static ::std::vector<uint8_t>
  from_c_type(const uint8_t* val) {
    return {val, val + size(val)};
  }

  static uint8_t*
  to_c_type(const ::std::vector<uint8_t>& os, ::std::error_code& ec) noexcept {
    return to_c_type(os.data(), os.size(), ec);
  }

  static uint8_t* to_c_type(const uint8_t* os, ::std::size_t n,
                            ::std::error_code& ec) noexcept;

  static constexpr void*
  address(uint8_t* val) noexcept {
    return val;
  }

  static constexpr const void*
  address(const uint8_t* val) noexcept {
    return val;
  }

  static ::std::size_t size(const uint8_t* val) noexcept;
};

/**
 * Specialization of #lely::canopen::canopen_traits for the CANopen array type
 * UNICODE_STRING.
 */
template <>
struct canopen_traits<::std::basic_string<char16_t>>
    : detail::canopen_traits<::std::basic_string<char16_t>, 0x000b> {
  using c_type = char16_t*;

  static char16_t* construct(const void* p, ::std::size_t n,
                             ::std::error_code& ec) noexcept;

  static void destroy(char16_t*& val) noexcept;

  static ::std::basic_string<char16_t>
  from_c_type(const char16_t* val) {
    return val ? ::std::basic_string<char16_t>{val}
               : ::std::basic_string<char16_t>{};
  }

  static char16_t*
  to_c_type(const ::std::basic_string<char16_t>& us,
            ::std::error_code& ec) noexcept {
    return to_c_type(us.c_str(), ec);
  }

  static char16_t* to_c_type(const char16_t* us,
                             ::std::error_code& ec) noexcept;

  static constexpr void*
  address(char16_t* val) noexcept {
    return val;
  }

  static constexpr const void*
  address(const char16_t* val) noexcept {
    return val;
  }

  static ::std::size_t
  size(const char16_t* val) noexcept {
    return val ? ::std::char_traits<char16_t>::length(val) : 0;
  }
};

// TIME_OF_DAY
// TIME_DIFFERENCE.
// DOMAIN
// INTEGER24

/**
 * Specialization of #lely::canopen::canopen_traits for the CANopen basic type
 * REAL64.
 */
template <>
struct canopen_traits<double> : detail::canopen_traits<double, 0x0011, true> {};

// INTEGER40
// INTEGER48
// INTEGER56

/**
 * Specialization of #lely::canopen::canopen_traits for the CANopen basic type
 * INTEGER64.
 */
template <>
struct canopen_traits<int64_t> : detail::canopen_traits<int64_t, 0x0015, true> {
};

// UNSIGNED24
// UNSIGNED40
// UNSIGNED48
// UNSIGNED56

/**
 * Specialization of #lely::canopen::canopen_traits for the CANopen basic type
 * UNSIGNED64.
 */
template <>
struct canopen_traits<uint64_t>
    : detail::canopen_traits<uint64_t, 0x001b, true> {};

/**
 * If <b>T</b> is one of the CANopen basic or array types, provides the member
 * constant `value` equal to `true`. For any other type, `value` is `false`.
 */
template <class T, class = T>
struct is_canopen : ::std::false_type {};

template <class T>
struct is_canopen<T, typename canopen_traits<T>::type> : ::std::true_type {};

/**
 * If <b>T</b> is one of the CANopen basic types, provides the member constant
 * `value` equal to `true`. For any other type, `value` is `false`.
 */
template <class T, class = T>
struct is_canopen_basic : ::std::false_type {};

template <class T>
struct is_canopen_basic<T, typename canopen_traits<T>::type>
    : ::std::integral_constant<bool, canopen_traits<T>::is_basic> {};

/**
 * Returns `true` if the CANopen data types <b>t1</b> and <b>t2</b> map to the
 * same C++ type, and `false` if not.
 */
inline constexpr bool
is_canopen_same(uint16_t t1, uint16_t t2) noexcept {
  // OCTET_STRING and DOMAIN are both byte arrays.
  return t1 == t2 || (t1 == 0x000a && t2 == 0x000f) ||
         (t1 == 0x000f && t2 == 0x000a);
}

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_TYPE_TRAITS_HPP_
