/**@file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the emergency (EMCY) object. See lely/co/emcy.h for the C
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

#ifndef LELY_CO_EMCY_HPP_
#define LELY_CO_EMCY_HPP_

#if !defined(__cplusplus) || LELY_NO_CXX
#error "include <lely/co/emcy.h> for the C interface"
#endif

#include <lely/can/net.hpp>
#include <lely/co/emcy.h>

namespace lely {

/// The attributes of #co_emcy_t required by #lely::COEmcy.
template <>
struct c_type_traits<__co_emcy> {
  typedef __co_emcy value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __co_emcy_alloc();
  }

  static void
  free(void* ptr) noexcept {
    __co_emcy_free(ptr);
  }

  static pointer
  init(pointer p, CANNet* net, CODev* dev) noexcept {
    return __co_emcy_init(p, net, dev);
  }

  static void
  fini(pointer p) noexcept {
    __co_emcy_fini(p);
  }
};

/// An opaque CANopen EMCY producer/consumer service type.
class COEmcy : public incomplete_c_type<__co_emcy> {
  typedef incomplete_c_type<__co_emcy> c_base;

 public:
  COEmcy(CANNet* net, CODev* dev) : c_base(net, dev) {}

  int
  start() noexcept {
    return co_emcy_start(this);
  }

  void
  stop() noexcept {
    co_emcy_stop(this);
  }

  CANNet*
  getNet() const noexcept {
    return co_emcy_get_net(this);
  }

  CODev*
  getDev() const noexcept {
    return co_emcy_get_dev(this);
  }

  int
  push(co_unsigned16_t eec, co_unsigned8_t er,
       const co_unsigned8_t msef[5] = 0) noexcept {
    return co_emcy_push(this, eec, er, msef);
  }

  int
  pop(co_unsigned16_t* peec = 0, co_unsigned8_t* per = 0) noexcept {
    return co_emcy_pop(this, peec, per);
  }

  void
  peek(co_unsigned16_t* peec = 0, co_unsigned8_t* per = 0) const noexcept {
    co_emcy_peek(this, peec, per);
  }

  int
  clear() noexcept {
    return co_emcy_clear(this);
  }

  void
  getInd(co_emcy_ind_t** pind, void** pdata) const noexcept {
    co_emcy_get_ind(this, pind, pdata);
  }

  void
  setInd(co_emcy_ind_t* ind, void* data) noexcept {
    co_emcy_set_ind(this, ind, data);
  }

  template <class F>
  void
  setInd(F* f) noexcept {
    setInd(&c_obj_call<co_emcy_ind_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_emcy_ind_t*, C>::type M>
  void
  setInd(C* obj) noexcept {
    setInd(&c_mem_call<co_emcy_ind_t*, C, M>::function,
           static_cast<void*>(obj));
  }

 protected:
  ~COEmcy() = default;
};

}  // namespace lely

#endif  // !LELY_CO_EMCY_HPP_
