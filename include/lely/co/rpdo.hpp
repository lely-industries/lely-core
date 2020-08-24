/**@file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the Receive-PDO declarations. See lely/co/rpdo.h for the C
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

#ifndef LELY_CO_RPDO_HPP_
#define LELY_CO_RPDO_HPP_

#if !defined(__cplusplus) || LELY_NO_CXX
#error "include <lely/co/rpdo.h> for the C interface"
#endif

#include <lely/can/net.hpp>
#include <lely/co/rpdo.h>

namespace lely {

/// The attributes of #co_rpdo_t required by #lely::CORPDO.
template <>
struct c_type_traits<__co_rpdo> {
  typedef __co_rpdo value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __co_rpdo_alloc();
  }

  static void
  free(void* ptr) noexcept {
    __co_rpdo_free(ptr);
  }

  static pointer
  init(pointer p, CANNet* net, CODev* dev, co_unsigned16_t num) noexcept {
    return __co_rpdo_init(p, net, dev, num);
  }

  static void
  fini(pointer p) noexcept {
    __co_rpdo_fini(p);
  }
};

/// An opaque CANopen Receive-PDO service type.
class CORPDO : public incomplete_c_type<__co_rpdo> {
  typedef incomplete_c_type<__co_rpdo> c_base;

 public:
  CORPDO(CANNet* net, CODev* dev, co_unsigned16_t num)
      : c_base(net, dev, num) {}

  int
  start() noexcept {
    return co_rpdo_start(this);
  }

  void
  stop() noexcept {
    co_rpdo_stop(this);
  }

  CANNet*
  getNet() const noexcept {
    return co_rpdo_get_net(this);
  }

  CODev*
  getDev() const noexcept {
    return co_rpdo_get_dev(this);
  }

  co_unsigned16_t
  getNum() const noexcept {
    return co_rpdo_get_num(this);
  }

  const co_pdo_comm_par&
  getCommPar() const noexcept {
    return *co_rpdo_get_comm_par(this);
  }

  const co_pdo_map_par&
  getMapPar() const noexcept {
    return *co_rpdo_get_map_par(this);
  }

  void
  getInd(co_rpdo_ind_t** pind, void** pdata) const noexcept {
    co_rpdo_get_ind(this, pind, pdata);
  }

  void
  setInd(co_rpdo_ind_t* ind, void* data) noexcept {
    co_rpdo_set_ind(this, ind, data);
  }

  template <class F>
  void
  setInd(F* f) noexcept {
    setInd(&c_obj_call<co_rpdo_ind_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_rpdo_ind_t*, C>::type M>
  void
  setInd(C* obj) noexcept {
    setInd(&c_mem_call<co_rpdo_ind_t*, C, M>::function,
           static_cast<void*>(obj));
  }

  void
  getErr(co_rpdo_err_t** perr, void** pdata) const noexcept {
    co_rpdo_get_err(this, perr, pdata);
  }

  void
  setErr(co_rpdo_err_t* err, void* data) noexcept {
    co_rpdo_set_err(this, err, data);
  }

  template <class F>
  void
  setErr(F* f) noexcept {
    setErr(&c_obj_call<co_rpdo_err_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_rpdo_err_t*, C>::type M>
  void
  setErr(C* obj) noexcept {
    setErr(&c_mem_call<co_rpdo_err_t*, C, M>::function,
           static_cast<void*>(obj));
  }

  int
  sync(co_unsigned8_t cnt) noexcept {
    return co_rpdo_sync(this, cnt);
  }

  int
  rtr() noexcept {
    return co_rpdo_rtr(this);
  }

 protected:
  ~CORPDO() = default;
};

}  // namespace lely

#endif  // !LELY_CO_RPDO_HPP_
