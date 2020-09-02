/**@file
 * This header file is part of the CAN library; it contains the C++ interface of
 * the CAN frame buffer. @see lely/can/buf.h for the C interface.
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

#ifndef LELY_CAN_BUF_HPP_
#define LELY_CAN_BUF_HPP_

#ifndef __cplusplus
#error "include <lely/can/buf.h> for the C interface"
#endif

#include <lely/util/c_type.hpp>
#include <lely/can/buf.h>

namespace lely {

/// The attributes of #can_buf required by #lely::CANBuf.
template <>
struct c_type_traits<can_buf> {
  typedef can_buf value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static pointer
  init(pointer p) noexcept {
    can_buf_init(p, nullptr, 0);
    return p;
  }

  static void
  fini(pointer p) noexcept {
    can_buf_fini(p);
  }
};

/// A CAN frame buffer.
class CANBuf : public standard_c_type<can_buf> {
  typedef standard_c_type<can_buf> c_base;

 public:
  CANBuf() : c_base() {}

  CANBuf& operator=(const CANBuf&) = delete;

  void
  clear() noexcept {
    can_buf_clear(c_ptr());
  }

  ::std::size_t
  size() const noexcept {
    return can_buf_size(c_ptr());
  }

  ::std::size_t
  capacity() const noexcept {
    return can_buf_capacity(c_ptr());
  }

  ::std::size_t
  reserve(::std::size_t n) noexcept {
    return can_buf_reserve(c_ptr(), n);
  }

  ::std::size_t
  peek(can_msg* ptr, ::std::size_t n) noexcept {
    return can_buf_peek(c_ptr(), ptr, n);
  }

  bool
  peek(can_msg& msg) noexcept {
    return !!peek(&msg, 1);
  }

  ::std::size_t
  read(can_msg* ptr, ::std::size_t n) noexcept {
    return can_buf_read(c_ptr(), ptr, n);
  }

  bool
  read(can_msg& msg) noexcept {
    return !!read(&msg, 1);
  }

  ::std::size_t
  write(const can_msg* ptr, ::std::size_t n) noexcept {
    return can_buf_write(c_ptr(), ptr, n);
  }

  bool
  write(const can_msg& msg) noexcept {
    return !!write(&msg, 1);
  }

 private:
  can_msg* m_ptr;
  ::std::size_t m_size;
  ::std::size_t m_begin;
  ::std::size_t m_end;
};

}  // namespace lely

#endif  // !LELY_CAN_BUF_HPP_
