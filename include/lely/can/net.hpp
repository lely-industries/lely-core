/**@file
 * This header file is part of the CAN library; it contains the C++ interface of
 * the CAN network interface. @see lely/can/net.h for the C interface.
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

#ifndef LELY_CAN_NET_HPP_
#define LELY_CAN_NET_HPP_

#if !defined(__cplusplus) || LELY_NO_CXX
#error "include <lely/can/net.h> for the C interface"
#endif

#include <lely/util/c_call.hpp>
#include <lely/util/c_type.hpp>

namespace lely {
class CANNet;
}
/// An opaque CAN network interface type.
typedef lely::CANNet can_net_t;

namespace lely {
class CANTimer;
}
/// An opaque CAN timer type.
typedef lely::CANTimer can_timer_t;

namespace lely {
class CANRecv;
}
/// An opaque CAN frame receiver type.
typedef lely::CANRecv can_recv_t;

#include <lely/can/net.h>

namespace lely {

/// The attributes of #can_net_t required by #lely::CANNet.
template <>
struct c_type_traits<__can_net> {
  typedef __can_net value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __can_net_alloc();
  }

  static void
  free(void* ptr) noexcept {
    __can_net_free(ptr);
  }

  static pointer
  init(pointer p) noexcept {
    return __can_net_init(p);
  }

  static void
  fini(pointer p) noexcept {
    __can_net_fini(p);
  }
};

/// An opaque CAN network interface type.
class CANNet : public incomplete_c_type<__can_net> {
  typedef incomplete_c_type<__can_net> c_base;

 public:
  CANNet() : c_base() {}

  void
  getTime(timespec* tp) const noexcept {
    can_net_get_time(this, tp);
  }

  int
  setTime(const timespec& tp) noexcept {
    return can_net_set_time(this, &tp);
  }

  void
  getNextFunc(can_timer_func_t** pfunc, void** pdata) const noexcept {
    can_net_get_next_func(this, pfunc, pdata);
  }

  void
  setNextFunc(can_timer_func_t* func, void* data) noexcept {
    can_net_set_next_func(this, func, data);
  }

  template <class F>
  void
  setNextFunc(F* f) noexcept {
    setNextFunc(&c_obj_call<can_timer_func_t*, F>::function,
                static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<can_timer_func_t*, C>::type M>
  void
  setNextFunc(C* obj) noexcept {
    setNextFunc(&c_mem_call<can_timer_func_t*, C, M>::function,
                static_cast<void*>(obj));
  }

  int
  recv(const can_msg& msg) noexcept {
    return can_net_recv(this, &msg);
  }

  int
  send(const can_msg& msg) noexcept {
    return can_net_send(this, &msg);
  }

  void
  getSendFunc(can_send_func_t** pfunc, void** pdata) const noexcept {
    can_net_get_send_func(this, pfunc, pdata);
  }

  void
  setSendFunc(can_send_func_t* func, void* data) noexcept {
    can_net_set_send_func(this, func, data);
  }

  template <class F>
  void
  setSendFunc(F* f) noexcept {
    setSendFunc(&c_obj_call<can_send_func_t*, F>::function,
                static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<can_send_func_t*, C>::type M>
  void
  setSendFunc(C* obj) noexcept {
    setSendFunc(&c_mem_call<can_send_func_t*, C, M>::function,
                static_cast<void*>(obj));
  }

 protected:
  ~CANNet() = default;
};

/// The attributes of #can_timer_t required by #lely::CANTimer.
template <>
struct c_type_traits<__can_timer> {
  typedef __can_timer value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __can_timer_alloc();
  }
  static void
  free(void* ptr) noexcept {
    __can_timer_free(ptr);
  }

  static pointer
  init(pointer p) noexcept {
    return __can_timer_init(p);
  }
  static void
  fini(pointer p) noexcept {
    __can_timer_fini(p);
  }
};

/// An opaque CAN timer type.
class CANTimer : public incomplete_c_type<__can_timer> {
  typedef incomplete_c_type<__can_timer> c_base;

 public:
  CANTimer() : c_base() {}

  void
  getFunc(can_timer_func_t** pfunc, void** pdata) const noexcept {
    can_timer_get_func(this, pfunc, pdata);
  }

  void
  setFunc(can_timer_func_t* func, void* data) noexcept {
    can_timer_set_func(this, func, data);
  }

  template <class F>
  void
  setFunc(F* f) noexcept {
    setFunc(&c_obj_call<can_timer_func_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<can_timer_func_t*, C>::type M>
  void
  setFunc(C* obj) noexcept {
    setFunc(&c_mem_call<can_timer_func_t*, C, M>::function,
            static_cast<void*>(obj));
  }

  void
  start(CANNet& net, const timespec* start = 0,
        const timespec* interval = 0) noexcept {
    can_timer_start(this, &net, start, interval);
  }

  void
  stop() noexcept {
    can_timer_stop(this);
  }

  void
  timeout(CANNet& net, int timeout) noexcept {
    can_timer_timeout(this, &net, timeout);
  }

 protected:
  ~CANTimer() = default;
};

/// The attributes of #can_recv_t required by #lely::CANRecv.
template <>
struct c_type_traits<__can_recv> {
  typedef __can_recv value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __can_recv_alloc();
  }
  static void
  free(void* ptr) noexcept {
    __can_recv_free(ptr);
  }

  static pointer
  init(pointer p) noexcept {
    return __can_recv_init(p);
  }
  static void
  fini(pointer p) noexcept {
    __can_recv_fini(p);
  }
};

/// An opaque CAN frame receiver type.
class CANRecv : public incomplete_c_type<__can_recv> {
  typedef incomplete_c_type<__can_recv> c_base;

 public:
  CANRecv() : c_base() {}

  void
  getFunc(can_recv_func_t** pfunc, void** pdata) const noexcept {
    can_recv_get_func(this, pfunc, pdata);
  }

  void
  setFunc(can_recv_func_t* func, void* data) noexcept {
    can_recv_set_func(this, func, data);
  }

  template <class F>
  void
  setFunc(F* f) noexcept {
    setFunc(&c_obj_call<can_recv_func_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<can_recv_func_t*, C>::type M>
  void
  setFunc(C* obj) noexcept {
    setFunc(&c_mem_call<can_recv_func_t*, C, M>::function,
            static_cast<void*>(obj));
  }

  void
  start(CANNet& net, uint_least32_t id, uint_least8_t flags = 0) noexcept {
    can_recv_start(this, &net, id, flags);
  }

  void
  stop() noexcept {
    can_recv_stop(this);
  }

 protected:
  ~CANRecv() = default;
};

}  // namespace lely

#endif  // !LELY_CAN_NET_HPP_
