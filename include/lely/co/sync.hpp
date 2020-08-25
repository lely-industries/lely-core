/**@file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the synchronization (SYNC) object. See lely/co/sync.h for the C
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

#ifndef LELY_CO_SYNC_HPP_
#define LELY_CO_SYNC_HPP_

#if !defined(__cplusplus) || LELY_NO_CXX
#error "include <lely/co/sync.h> for the C interface"
#endif

#include <lely/can/net.hpp>
#include <lely/co/sync.h>

namespace lely {

/// The attributes of #co_sync_t required by #lely::COSync.
template <>
struct c_type_traits<__co_sync> {
  typedef __co_sync value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __co_sync_alloc();
  }

  static void
  free(void* ptr) noexcept {
    __co_sync_free(ptr);
  }

  static pointer
  init(pointer p, CANNet* net, CODev* dev) noexcept {
    return __co_sync_init(p, net, dev);
  }

  static void
  fini(pointer p) noexcept {
    __co_sync_fini(p);
  }
};

/// An opaque CANopen SYNC producer/consumer service type.
class COSync : public incomplete_c_type<__co_sync> {
  typedef incomplete_c_type<__co_sync> c_base;

 public:
  COSync(CANNet* net, CODev* dev) : c_base(net, dev) {}

  int
  start() noexcept {
    return co_sync_start(this);
  }

  void
  stop() noexcept {
    co_sync_stop(this);
  }

  CANNet*
  getNet() const noexcept {
    return co_sync_get_net(this);
  }

  CODev*
  getDev() const noexcept {
    return co_sync_get_dev(this);
  }

  void
  getInd(co_sync_ind_t** pind, void** pdata) const noexcept {
    co_sync_get_ind(this, pind, pdata);
  }

  void
  setInd(co_sync_ind_t* ind, void* data) noexcept {
    co_sync_set_ind(this, ind, data);
  }

  template <class F>
  void
  setInd(F* f) noexcept {
    setInd(&c_obj_call<co_sync_ind_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_sync_ind_t*, C>::type M>
  void
  setInd(C* obj) noexcept {
    setInd(&c_mem_call<co_sync_ind_t*, C, M>::function,
           static_cast<void*>(obj));
  }

  void
  getErr(co_sync_err_t** perr, void** pdata) const noexcept {
    co_sync_get_err(this, perr, pdata);
  }

  void
  setErr(co_sync_err_t* err, void* data) noexcept {
    co_sync_set_err(this, err, data);
  }

  template <class F>
  void
  setErr(F* f) noexcept {
    setErr(&c_obj_call<co_sync_err_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_sync_err_t*, C>::type M>
  void
  setErr(C* obj) noexcept {
    setErr(&c_mem_call<co_sync_err_t*, C, M>::function,
           static_cast<void*>(obj));
  }

 protected:
  ~COSync() = default;
};

}  // namespace lely

#endif  // !LELY_CO_SYNC_HPP_
