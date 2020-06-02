/**@file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the Wireless Transmission Media (WTM) declarations. See
 * lely/co/wtm.h for the C interface.
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

#ifndef LELY_CO_WTM_HPP_
#define LELY_CO_WTM_HPP_

#if !defined(__cplusplus) || LELY_NO_CXX
#error "include <lely/co/wtm.h> for the C interface"
#endif

#include <lely/util/c_call.hpp>
#include <lely/util/c_type.hpp>

namespace lely {
class COWTM;
}
/// An opaque CANopen Wireless Transmission Media (WTM) interface type.
typedef lely::COWTM co_wtm_t;

#include <lely/co/wtm.h>

namespace lely {

/// The attributes of #co_wtm_t required by #lely::COWTM.
template <>
struct c_type_traits<__co_wtm> {
  typedef __co_wtm value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __co_wtm_alloc();
  }
  static void
  free(void* ptr) noexcept {
    __co_wtm_free(ptr);
  }

  static pointer
  init(pointer p) noexcept {
    return __co_wtm_init(p);
  }
  static void
  fini(pointer p) noexcept {
    __co_wtm_fini(p);
  }
};

/// An opaque CANopen Wireless Transmission Media (WTM) interface type.
class COWTM : public incomplete_c_type<__co_wtm> {
  typedef incomplete_c_type<__co_wtm> c_base;

 public:
  COWTM() : c_base() {}

  uint_least8_t
  getNIF() const noexcept {
    return co_wtm_get_nif(this);
  }

  int
  setNIF(uint_least8_t nif = 1) noexcept {
    return co_wtm_set_nif(this, nif);
  }

  int
  setDiagCAN(uint_least8_t nif, uint_least8_t st = 0xf, uint_least8_t err = 0xf,
             uint_least8_t load = 0xff, uint_least16_t ec = 0xffff,
             uint_least16_t foc = 0xffff,
             uint_least16_t coc = 0xffff) noexcept {
    return co_wtm_set_diag_can(this, nif, st, err, load, ec, foc, coc);
  }

  int
  setDiagWTM(uint_least8_t quality = 0xff) noexcept {
    return co_wtm_set_diag_wtm(this, quality);
  }

  void
  getDiagCANCon(co_wtm_diag_can_con_t** pfunc, void** pdata) const noexcept {
    co_wtm_get_diag_can_con(this, pfunc, pdata);
  }

  void
  setDiagCANCon(co_wtm_diag_can_con_t* func, void* data) noexcept {
    co_wtm_set_diag_can_con(this, func, data);
  }

  template <class F>
  void
  setDiagCANCon(F* f) noexcept {
    setDiagCANCon(&c_obj_call<co_wtm_diag_can_con_t*, F>::function,
                  static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_wtm_diag_can_con_t*, C>::type M>
  void
  setDiagCANCon(C* obj) noexcept {
    setDiagCANCon(&c_mem_call<co_wtm_diag_can_con_t*, C, M>::function,
                  static_cast<void*>(obj));
  }

  void
  getDiagWTMCon(co_wtm_diag_wtm_con_t** pfunc, void** pdata) const noexcept {
    co_wtm_get_diag_wtm_con(this, pfunc, pdata);
  }

  void
  setDiagWTMCon(co_wtm_diag_wtm_con_t* func, void* data) noexcept {
    co_wtm_set_diag_wtm_con(this, func, data);
  }

  template <class F>
  void
  setDiagWTMCon(F* f) noexcept {
    setDiagWTMCon(&c_obj_call<co_wtm_diag_wtm_con_t*, F>::function,
                  static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_wtm_diag_wtm_con_t*, C>::type M>
  void
  setDiagWTMCon(C* obj) noexcept {
    setDiagWTMCon(&c_mem_call<co_wtm_diag_wtm_con_t*, C, M>::function,
                  static_cast<void*>(obj));
  }

  void
  getDiagCANInd(co_wtm_diag_can_ind_t** pfunc, void** pdata) const noexcept {
    co_wtm_get_diag_can_ind(this, pfunc, pdata);
  }

  void
  setDiagCANInd(co_wtm_diag_can_ind_t* func, void* data) noexcept {
    co_wtm_set_diag_can_ind(this, func, data);
  }

  template <class F>
  void
  setDiagCANInd(F* f) noexcept {
    setDiagCANInd(&c_obj_call<co_wtm_diag_can_ind_t*, F>::function,
                  static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_wtm_diag_can_ind_t*, C>::type M>
  void
  setDiagCANInd(C* obj) noexcept {
    setDiagCANInd(&c_mem_call<co_wtm_diag_can_ind_t*, C, M>::function,
                  static_cast<void*>(obj));
  }

  void
  getDiagWTMInd(co_wtm_diag_wtm_ind_t** pfunc, void** pdata) const noexcept {
    co_wtm_get_diag_wtm_ind(this, pfunc, pdata);
  }

  void
  setDiagWTMInd(co_wtm_diag_wtm_ind_t* func, void* data) noexcept {
    co_wtm_set_diag_wtm_ind(this, func, data);
  }

  template <class F>
  void
  setDiagWTMInd(F* f) noexcept {
    setDiagWTMInd(&c_obj_call<co_wtm_diag_wtm_ind_t*, F>::function,
                  static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_wtm_diag_wtm_ind_t*, C>::type M>
  void
  setDiagWTMInd(C* obj) noexcept {
    setDiagWTMInd(&c_mem_call<co_wtm_diag_wtm_ind_t*, C, M>::function,
                  static_cast<void*>(obj));
  }

  void
  getDiagACInd(co_wtm_diag_ac_ind_t** pfunc, void** pdata) const noexcept {
    co_wtm_get_diag_ac_ind(this, pfunc, pdata);
  }

  void
  setDiagACInd(co_wtm_diag_ac_ind_t* func, void* data) noexcept {
    co_wtm_set_diag_ac_ind(this, func, data);
  }

  template <class F>
  void
  setDiagACInd(F* f) noexcept {
    setDiagACInd(&c_obj_call<co_wtm_diag_ac_ind_t*, F>::function,
                 static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_wtm_diag_ac_ind_t*, C>::type M>
  void
  setDiagACInd(C* obj) noexcept {
    setDiagACInd(&c_mem_call<co_wtm_diag_ac_ind_t*, C, M>::function,
                 static_cast<void*>(obj));
  }

  void
  recv(const void* buf, size_t nbytes) noexcept {
    co_wtm_recv(this, buf, nbytes);
  }

  void
  getRecvFunc(co_wtm_recv_func_t** pfunc, void** pdata) const noexcept {
    co_wtm_get_recv_func(this, pfunc, pdata);
  }

  void
  setRecvFunc(co_wtm_recv_func_t* func, void* data) noexcept {
    co_wtm_set_recv_func(this, func, data);
  }

  template <class F>
  void
  setRecvFunc(F* f) noexcept {
    setRecvFunc(&c_obj_call<co_wtm_recv_func_t*, F>::function,
                static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_wtm_recv_func_t*, C>::type M>
  void
  setRecvFunc(C* obj) noexcept {
    setRecvFunc(&c_mem_call<co_wtm_recv_func_t*, C, M>::function,
                static_cast<void*>(obj));
  }

  int
  getTime(uint_least8_t nif, timespec* tp) const noexcept {
    return co_wtm_get_time(this, nif, tp);
  }

  int
  setTime(uint_least8_t nif, const timespec& tp) noexcept {
    return co_wtm_set_time(this, nif, &tp);
  }

  int
  send(uint_least8_t nif, const can_msg& msg) noexcept {
    return co_wtm_send(this, nif, &msg);
  }

  int
  sendAlive() noexcept {
    return co_wtm_send_alive(this);
  }

  int
  sendDiagCANReq(uint_least8_t nif) noexcept {
    return co_wtm_send_diag_can_req(this, nif);
  }

  int
  sendDiagWTMReq(uint_least8_t nif) noexcept {
    return co_wtm_send_diag_wtm_req(this, nif);
  }

  int
  sendDiagCANRst(uint_least8_t nif) noexcept {
    return co_wtm_send_diag_can_rst(this, nif);
  }

  int
  sendDiagWTMRst(uint_least8_t nif) noexcept {
    return co_wtm_send_diag_wtm_rst(this, nif);
  }

  int
  sendDiagAC(uint_least32_t ac) noexcept {
    return co_wtm_send_diag_ac(this, ac);
  }

  int
  flush() noexcept {
    return co_wtm_flush(this);
  }

  void
  getSendFunc(co_wtm_send_func_t** pfunc, void** pdata) const noexcept {
    co_wtm_get_send_func(this, pfunc, pdata);
  }

  void
  setSendFunc(co_wtm_send_func_t* func, void* data) noexcept {
    co_wtm_set_send_func(this, func, data);
  }

  template <class F>
  void
  setSendFunc(F* f) noexcept {
    setSendFunc(&c_obj_call<co_wtm_send_func_t*, F>::function,
                static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_wtm_send_func_t*, C>::type M>
  void
  setSendFunc(C* obj) noexcept {
    setSendFunc(&c_mem_call<co_wtm_send_func_t*, C, M>::function,
                static_cast<void*>(obj));
  }

 protected:
  ~COWTM() = default;
};

}  // namespace lely

#endif  // !LELY_CO_WTM_HPP_
