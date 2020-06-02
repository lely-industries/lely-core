/**@file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the gateway declarations. See lely/co/gw.h for the C interface.
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

#ifndef LELY_CO_GW_HPP_
#define LELY_CO_GW_HPP_

#if !defined(__cplusplus) || LELY_NO_CXX
#error "include <lely/co/gw.h> for the C interface"
#endif

#include <lely/util/c_type.hpp>

namespace lely {
class COGW;
}
/// An opaque CANopen gateway type.
typedef lely::COGW co_gw_t;

#include <lely/co/gw.h>

namespace lely {

/// The attributes of #co_gw_t required by #lely::COGW.
template <>
struct c_type_traits<__co_gw> {
  typedef __co_gw value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __co_gw_alloc();
  }
  static void
  free(void* ptr) noexcept {
    __co_gw_free(ptr);
  }

  static pointer
  init(pointer p) noexcept {
    return __co_gw_init(p);
  }
  static void
  fini(pointer p) noexcept {
    __co_gw_fini(p);
  }
};

/// An opaque CANopen gateway type.
class COGW : public incomplete_c_type<__co_gw> {
  typedef incomplete_c_type<__co_gw> c_base;

 public:
  COGW() : c_base() {}

  int
  initNet(co_unsigned16_t id, CONMT* nmt) noexcept {
    return co_gw_init_net(this, id, nmt);
  }

  int
  finiNet(co_unsigned16_t id) noexcept {
    return co_gw_fini_net(this, id);
  }

  int
  recv(const co_gw_req* req) noexcept {
    return co_gw_recv(this, req);
  }

  void
  getSendFunc(co_gw_send_func_t** pfunc, void** pdata) const noexcept {
    co_gw_get_send_func(this, pfunc, pdata);
  }

  void
  setSendFunc(co_gw_send_func_t* func, void* data) noexcept {
    co_gw_set_send_func(this, func, data);
  }

  template <class F>
  void
  setSendFunc(F* f) noexcept {
    setSendFunc(&c_obj_call<co_gw_send_func_t*, F>::function,
                static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_gw_send_func_t*, C>::type M>
  void
  setSendFunc(C* obj) noexcept {
    setSendFunc(&c_mem_call<co_gw_send_func_t*, C, M>::function,
                static_cast<void*>(obj));
  }

  void
  getRateFunc(co_gw_rate_func_t** pfunc, void** pdata) const noexcept {
    co_gw_get_rate_func(this, pfunc, pdata);
  }

  void
  setRateFunc(co_gw_rate_func_t* func, void* data) noexcept {
    co_gw_set_rate_func(this, func, data);
  }

  template <class F>
  void
  setRateFunc(F* f) noexcept {
    setRateFunc(&c_obj_call<co_gw_rate_func_t*, F>::function,
                static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_gw_rate_func_t*, C>::type M>
  void
  setRateFunc(C* obj) noexcept {
    setRateFunc(&c_mem_call<co_gw_rate_func_t*, C, M>::function,
                static_cast<void*>(obj));
  }

 protected:
  ~COGW() = default;
};

}  // namespace lely

#endif  // !LELY_CO_GW_HPP_
