/**@file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the Transmit-PDO declarations. See lely/co/tpdo.h for the C
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

#ifndef LELY_CO_TPDO_HPP_
#define LELY_CO_TPDO_HPP_

#if !defined(__cplusplus) || LELY_NO_CXX
#error "include <lely/co/tpdo.h> for the C interface"
#endif

#include <lely/can/net.hpp>
#include <lely/co/tpdo.h>

namespace lely {

/// The attributes of #co_tpdo_t required by #lely::COTPDO.
template <>
struct c_type_traits<__co_tpdo> {
  typedef __co_tpdo value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __co_tpdo_alloc();
  }

  static void
  free(void* ptr) noexcept {
    __co_tpdo_free(ptr);
  }

  static pointer
  init(pointer p, CANNet* net, CODev* dev, co_unsigned16_t num) noexcept {
    return __co_tpdo_init(p, net, dev, num);
  }

  static void
  fini(pointer p) noexcept {
    __co_tpdo_fini(p);
  }
};

/// An opaque CANopen Transmit-PDO service type.
class COTPDO : public incomplete_c_type<__co_tpdo> {
  typedef incomplete_c_type<__co_tpdo> c_base;

 public:
  COTPDO(CANNet* net, CODev* dev, co_unsigned16_t num)
      : c_base(net, dev, num) {}

  int
  start() noexcept {
    return co_tpdo_start(this);
  }

  void
  stop() noexcept {
    co_tpdo_stop(this);
  }

  CANNet*
  getNet() const noexcept {
    return co_tpdo_get_net(this);
  }

  CODev*
  getDev() const noexcept {
    return co_tpdo_get_dev(this);
  }

  co_unsigned16_t
  getNum() const noexcept {
    return co_tpdo_get_num(this);
  }

  const co_pdo_comm_par&
  getCommPar() const noexcept {
    return *co_tpdo_get_comm_par(this);
  }

  const co_pdo_map_par&
  getMapPar() const noexcept {
    return *co_tpdo_get_map_par(this);
  }

  void
  getInd(co_tpdo_ind_t** pind, void** pdata) const noexcept {
    co_tpdo_get_ind(this, pind, pdata);
  }

  void
  setInd(co_tpdo_ind_t* ind, void* data) noexcept {
    co_tpdo_set_ind(this, ind, data);
  }

  template <class F>
  void
  setInd(F* f) noexcept {
    setInd(&c_obj_call<co_tpdo_ind_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_tpdo_ind_t*, C>::type M>
  void
  setInd(C* obj) noexcept {
    setInd(&c_mem_call<co_tpdo_ind_t*, C, M>::function,
           static_cast<void*>(obj));
  }

  int
  event() noexcept {
    return co_tpdo_event(this);
  }

  int
  sync(co_unsigned8_t cnt) noexcept {
    return co_tpdo_sync(this, cnt);
  }

  void
  getNext(timespec& tp) noexcept {
    co_tpdo_get_next(this, &tp);
  }

 protected:
  ~COTPDO() = default;
};

}  // namespace lely

#endif  // !LELY_CO_TPDO_HPP_
