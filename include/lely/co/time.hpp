/**@file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the time stamp (TIME) object. See lely/co/time.h for the C
 * interface.
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

#ifndef LELY_CO_TIME_HPP_
#define LELY_CO_TIME_HPP_

#if !defined(__cplusplus) || LELY_NO_CXX
#error "include <lely/co/time.h> for the C interface"
#endif

#include <lely/can/net.hpp>
#include <lely/co/time.h>

namespace lely {

/// The attributes of #co_time_t required by #lely::COTime.
template <>
struct c_type_traits<__co_time> {
  typedef __co_time value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __co_time_alloc();
  }

  static void
  free(void* ptr) noexcept {
    __co_time_free(ptr);
  }

  static pointer
  init(pointer p, CANNet* net, CODev* dev) noexcept {
    return __co_time_init(p, net, dev);
  }

  static void
  fini(pointer p) noexcept {
    __co_time_fini(p);
  }
};

/// An opaque CANopen TIME producer/consumer service type.
class COTime : public incomplete_c_type<__co_time> {
  typedef incomplete_c_type<__co_time> c_base;

 public:
  COTime(CANNet* net, CODev* dev) : c_base(net, dev) {}

  int
  start() noexcept {
    return co_time_start(this);
  }

  void
  stop() noexcept {
    co_time_stop(this);
  }

  CANNet*
  getNet() const noexcept {
    return co_time_get_net(this);
  }

  CODev*
  getDev() const noexcept {
    return co_time_get_dev(this);
  }

  void
  getInd(co_time_ind_t** pind, void** pdata) const noexcept {
    co_time_get_ind(this, pind, pdata);
  }

  void
  setInd(co_time_ind_t* ind, void* data) noexcept {
    co_time_set_ind(this, ind, data);
  }

  template <class F>
  void
  setInd(F* f) noexcept {
    setInd(&c_obj_call<co_time_ind_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_time_ind_t*, C>::type M>
  void
  setInd(C* obj) noexcept {
    setInd(&c_mem_call<co_time_ind_t*, C, M>::function,
           static_cast<void*>(obj));
  }

  void
  startProd(const timespec* start = 0, const timespec* interval = 0) noexcept {
    co_time_start_prod(this, start, interval);
  }

  void
  stopProd() noexcept {
    co_time_stop_prod(this);
  }

 protected:
  ~COTime() = default;
};

}  // namespace lely

#endif  // !LELY_CO_TIME_HPP_
