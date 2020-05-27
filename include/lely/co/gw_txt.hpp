/**@file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the ASCII gateway declarations. See lely/co/gw_txt.h for the C
 * interface.
 *
 * @copyright 2017-2020 Lely Industries N.V.
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

#ifndef LELY_CO_GW_TXT_HPP_
#define LELY_CO_GW_TXT_HPP_

#if !defined(__cplusplus) || LELY_NO_CXX
#error "include <lely/co/gw_txt.h> for the C interface"
#endif

#include <lely/util/c_type.hpp>

namespace lely {
class COGWTxt;
}
/// An opaque CANopen ASCII gateway client type.
typedef lely::COGWTxt co_gw_txt_t;

#include <lely/co/gw_txt.h>

namespace lely {

/// The attributes of #co_gw_txt_t required by #lely::COGWTxt.
template <>
struct c_type_traits<__co_gw_txt> {
  typedef __co_gw_txt value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __co_gw_txt_alloc();
  }
  static void
  free(void* ptr) noexcept {
    __co_gw_txt_free(ptr);
  }

  static pointer
  init(pointer p) noexcept {
    return __co_gw_txt_init(p);
  }
  static void
  fini(pointer p) noexcept {
    __co_gw_txt_fini(p);
  }
};

/// An opaque CANopen ASCII gateway client type.
class COGWTxt : public incomplete_c_type<__co_gw_txt> {
  typedef incomplete_c_type<__co_gw_txt> c_base;

 public:
  COGWTxt() : c_base() {}

  int
  iec() noexcept {
    return co_gw_txt_iec(this);
  }

  ::std::size_t
  pending() const noexcept {
    return co_gw_txt_pending(this);
  }

  int
  recv(const co_gw_srv* srv) noexcept {
    return co_gw_txt_recv(this, srv);
  }

  void
  getRecvFunc(co_gw_txt_recv_func_t** pfunc, void** pdata) const noexcept {
    co_gw_txt_get_recv_func(this, pfunc, pdata);
  }

  void
  setRecvFunc(co_gw_txt_recv_func_t* func, void* data) noexcept {
    co_gw_txt_set_recv_func(this, func, data);
  }

  template <class F>
  void
  setRecvFunc(F* f) noexcept {
    setRecvFunc(&c_obj_call<co_gw_txt_recv_func_t*, F>::function,
                static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_gw_txt_recv_func_t*, C>::type M>
  void
  setRecvFunc(C* obj) noexcept {
    setRecvFunc(&c_mem_call<co_gw_txt_recv_func_t*, C, M>::function,
                static_cast<void*>(obj));
  }

  int
  send(const char* begin, const char* end = 0, floc* at = 0) noexcept {
    return co_gw_txt_send(this, begin, end, at);
  }

  void
  getSendFunc(co_gw_txt_send_func_t** pfunc, void** pdata) const noexcept {
    co_gw_txt_get_send_func(this, pfunc, pdata);
  }

  void
  setSendFunc(co_gw_txt_send_func_t* func, void* data) noexcept {
    co_gw_txt_set_send_func(this, func, data);
  }

  template <class F>
  void
  setSendFunc(F* f) noexcept {
    setSendFunc(&c_obj_call<co_gw_txt_send_func_t*, F>::function,
                static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_gw_txt_send_func_t*, C>::type M>
  void
  setSendFunc(C* obj) noexcept {
    setSendFunc(&c_mem_call<co_gw_txt_send_func_t*, C, M>::function,
                static_cast<void*>(obj));
  }

 protected:
  ~COGWTxt() = default;
};

}  // namespace lely

#endif  // !LELY_CO_GW_TXT_HPP_
