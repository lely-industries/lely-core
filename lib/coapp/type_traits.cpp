/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the CANopen type traits methods.
 *
 * @see lely/coapp/type_traits.hpp
 *
 * @copyright 2020 Lely Industries N.V.
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

#include "coapp.hpp"
#include <lely/co/val.h>
#include <lely/coapp/sdo_error.hpp>
#include <lely/coapp/type_traits.hpp>
#include <lely/util/error.hpp>

#include <string>
#include <vector>

namespace lely {

namespace canopen {

namespace detail {

template <class T, uint16_t N>
typename canopen_traits<T, N, true>::c_type
canopen_traits<T, N, true>::construct(const void* p, ::std::size_t n,
                                      ::std::error_code& ec) noexcept {
  auto val = typename canopen_traits<T, N, true>::c_type();
  uint32_t ac = co_val_read_sdo(N, &val, p, n);
  if (ac)
    ec = make_error_code(static_cast<lely::canopen::SdoErrc>(ac));
  else
    ec.clear();
  return val;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// BOOLEAN
template bool canopen_traits<bool, 0x0001, true>::construct(
    const void*, ::std::size_t, ::std::error_code&) noexcept;

// INTEGER8
template int8_t canopen_traits<int8_t, 0x0002, true>::construct(
    const void*, ::std::size_t, ::std::error_code&) noexcept;

// INTEGER16
template int16_t canopen_traits<int16_t, 0x0003, true>::construct(
    const void*, ::std::size_t, ::std::error_code&) noexcept;

// INTEGER32
template int32_t canopen_traits<int32_t, 0x0004, true>::construct(
    const void*, ::std::size_t, ::std::error_code&) noexcept;

// UNSIGNED8
template uint8_t canopen_traits<uint8_t, 0x0005, true>::construct(
    const void*, ::std::size_t, ::std::error_code&) noexcept;

// UNSIGNED16
template uint16_t canopen_traits<uint16_t, 0x0006, true>::construct(
    const void*, ::std::size_t, ::std::error_code&) noexcept;

// UNSIGNED32
template uint32_t canopen_traits<uint32_t, 0x0007, true>::construct(
    const void*, ::std::size_t, ::std::error_code&) noexcept;

// REAL32
template float canopen_traits<float, 0x0008, true>::construct(
    const void*, ::std::size_t, ::std::error_code&) noexcept;

// VISIBLE_STRING
// OCTET_STRING
// UNICODE_STRING
// TIME_OF_DAY
// TIME_DIFFERENCE
// DOMAIN
// INTEGER24

// REAL64
template double canopen_traits<double, 0x0011, true>::construct(
    const void*, ::std::size_t, ::std::error_code&) noexcept;

// INTEGER40
// INTEGER48
// INTEGER56

// INTEGER64
template int64_t canopen_traits<int64_t, 0x0015, true>::construct(
    const void*, ::std::size_t, ::std::error_code&) noexcept;

// UNSIGNED24
// UNSIGNED40
// UNSIGNED48
// UNSIGNED56

// UNSIGNED64
template uint64_t canopen_traits<uint64_t, 0x001b, true>::construct(
    const void*, ::std::size_t, ::std::error_code&) noexcept;

#endif  // DOXYGEN_SHOULD_SKIP_THIS

}  // namespace detail

char*
canopen_traits<::std::string>::construct(const void* p, ::std::size_t n,
                                         ::std::error_code& ec) noexcept {
  char* val = nullptr;
  uint32_t ac = co_val_read_sdo(index, &val, p, n);
  if (ac)
    ec = make_error_code(static_cast<lely::canopen::SdoErrc>(ac));
  else
    ec.clear();
  return val;
}

void
canopen_traits<::std::string>::destroy(char*& val) noexcept {
  co_val_fini(index, &val);
}

char*
canopen_traits<::std::string>::to_c_type(const char* vs,
                                         ::std::error_code& ec) noexcept {
  char* val = nullptr;
  int errsv = get_errc();
  set_errc(0);
  if (!vs || co_val_init_vs(&val, vs))
    ec.clear();
  else
    ec = util::make_error_code();
  set_errc(errsv);
  return val;
}

uint8_t*
canopen_traits<::std::vector<uint8_t>>::construct(
    const void* p, ::std::size_t n, ::std::error_code& ec) noexcept {
  uint8_t* val = nullptr;
  uint32_t ac = co_val_read_sdo(index, &val, p, n);
  if (ac)
    ec = make_error_code(static_cast<lely::canopen::SdoErrc>(ac));
  else
    ec.clear();
  return val;
}

void
canopen_traits<::std::vector<uint8_t>>::destroy(uint8_t*& val) noexcept {
  co_val_fini(index, &val);
}

uint8_t*
canopen_traits<::std::vector<uint8_t>>::to_c_type(
    const uint8_t* os, ::std::size_t n, ::std::error_code& ec) noexcept {
  uint8_t* val = nullptr;
  int errsv = get_errc();
  set_errc(0);
  if (!os || co_val_init_os(&val, os, n))
    ec.clear();
  else
    ec = util::make_error_code();
  set_errc(errsv);
  return val;
}

::std::size_t
canopen_traits<::std::vector<uint8_t>>::size(const uint8_t* val) noexcept {
  return val ? co_val_sizeof(index, &val) : 0;
}

char16_t*
canopen_traits<::std::basic_string<char16_t>>::construct(
    const void* p, ::std::size_t n, ::std::error_code& ec) noexcept {
  char16_t* val = nullptr;
  uint32_t ac = co_val_read_sdo(index, &val, p, n);
  if (ac)
    ec = make_error_code(static_cast<lely::canopen::SdoErrc>(ac));
  else
    ec.clear();
  return val;
}

void
canopen_traits<::std::basic_string<char16_t>>::destroy(
    char16_t*& val) noexcept {
  co_val_fini(index, &val);
}

char16_t*
canopen_traits<::std::basic_string<char16_t>>::to_c_type(
    const char16_t* us, ::std::error_code& ec) noexcept {
  char16_t* val = nullptr;
  int errsv = get_errc();
  set_errc(0);
  if (!us || co_val_init_us(&val, us))
    ec.clear();
  else
    ec = util::make_error_code();
  set_errc(errsv);
  return val;
}

}  // namespace canopen

}  // namespace lely
