/**@file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the Client-SDO declarations. See lely/co/csdo.h for the C
 * interface.
 *
 * @copyright 2020 Lely Industries N.V.
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

#ifndef LELY_CO_CSDO_HPP_
#define LELY_CO_CSDO_HPP_

#if !defined(__cplusplus) || LELY_NO_CXX
#error "include <lely/co/csdo.h> for the C interface"
#endif

#include <lely/can/net.hpp>
#include <lely/co/csdo.h>
#include <lely/co/val.hpp>

#include <string>
#include <vector>

namespace lely {

template <class>
struct COCSDOUpCon;

inline int
dnReq(CODev& dev, co_unsigned16_t idx, co_unsigned8_t subidx, const void* ptr,
      size_t n, co_csdo_dn_con_t* con, void* data) noexcept {
  return co_dev_dn_req(&dev, idx, subidx, ptr, n, con, data);
}

template <class F>
inline int
dnReq(CODev& dev, co_unsigned16_t idx, co_unsigned8_t subidx, const void* ptr,
      size_t n, F* f) noexcept {
  return dnReq(dev, idx, subidx, ptr, n,
               &c_obj_call<co_csdo_dn_con_t*, F>::function,
               static_cast<void*>(f));
}

template <class T, typename c_mem_fn<co_csdo_dn_con_t*, T>::type M>
inline int
dnReq(CODev& dev, co_unsigned16_t idx, co_unsigned8_t subidx, const void* ptr,
      size_t n, T* t) noexcept {
  return dnReq(dev, idx, subidx, ptr, n,
               &c_mem_call<co_csdo_dn_con_t*, T, M>::function,
               static_cast<void*>(t));
}

template <co_unsigned16_t N>
inline int
dnReq(CODev& dev, co_unsigned16_t idx, co_unsigned8_t subidx,
      const COVal<N>& val, co_csdo_dn_con_t* con, void* data) noexcept {
  return co_dev_dn_val_req(&dev, idx, subidx, N, &val, con, data);
}

template <co_unsigned16_t N, class F>
inline int
dnReq(CODev& dev, co_unsigned16_t idx, co_unsigned8_t subidx,
      const COVal<N>& val, F* f) noexcept {
  return dnReq<N>(dev, idx, subidx, val,
                  &c_obj_call<co_csdo_dn_con_t*, F>::function,
                  static_cast<void*>(f));
}

template <co_unsigned16_t N, class T,
          typename c_mem_fn<co_csdo_dn_con_t*, T>::type M>
inline int
dnReq(CODev& dev, co_unsigned16_t idx, co_unsigned8_t subidx,
      const COVal<N>& val, T* t) noexcept {
  return dnReq<N>(dev, idx, subidx, val,
                  &c_mem_call<co_csdo_dn_con_t*, T, M>::function,
                  static_cast<void*>(t));
}

inline int
upReq(const CODev& dev, co_unsigned16_t idx, co_unsigned8_t subidx,
      co_csdo_up_con_t* con, void* data) noexcept {
  return co_dev_up_req(&dev, idx, subidx, con, data);
}

template <class T, typename COCSDOUpCon<T>::type M>
inline int
upReq(const CODev& dev, co_unsigned16_t idx, co_unsigned8_t subidx,
      void* data) noexcept {
  return upReq(dev, idx, subidx, &COCSDOUpCon<T>::template function<M>, data);
}

template <class T, class F>
inline int
upReq(const CODev& dev, co_unsigned16_t idx, co_unsigned8_t subidx,
      F* f) noexcept {
  return upReq(dev, idx, subidx,
               &COCSDOUpCon<T>::template function<
                   &c_obj_call<typename COCSDOUpCon<T>::type, F>::function>,
               static_cast<void*>(f));
}

template <class T, class C,
          typename c_mem_fn<typename COCSDOUpCon<T>::type, C>::type M>
inline int
upReq(const CODev& dev, co_unsigned16_t idx, co_unsigned8_t subidx,
      C* obj) noexcept {
  return upReq(dev, idx, subidx,
               &COCSDOUpCon<T>::template function<
                   &c_mem_call<typename COCSDOUpCon<T>::type, C, M>::function>,
               static_cast<void*>(obj));
}

/// The attributes of #co_csdo_t required by #lely::COCSDO.
template <>
struct c_type_traits<__co_csdo> {
  typedef __co_csdo value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __co_csdo_alloc();
  }

  static void
  free(void* ptr) noexcept {
    __co_csdo_free(ptr);
  }

  static pointer
  init(pointer p, CANNet* net, CODev* dev, co_unsigned8_t num) noexcept {
    return __co_csdo_init(p, net, dev, num);
  }

  static void
  fini(pointer p) noexcept {
    __co_csdo_fini(p);
  }
};

/// An opaque CANopen Client-SDO service type.
class COCSDO : public incomplete_c_type<__co_csdo> {
  typedef incomplete_c_type<__co_csdo> c_base;

 public:
  COCSDO(CANNet* net, CODev* dev, co_unsigned8_t num) : c_base(net, dev, num) {}

  int
  start() noexcept {
    return co_csdo_start(this);
  }

  void
  stop() noexcept {
    co_csdo_stop(this);
  }

  CANNet*
  getNet() const noexcept {
    return co_csdo_get_net(this);
  }

  CODev*
  getDev() const noexcept {
    return co_csdo_get_dev(this);
  }

  co_unsigned8_t
  getNum() const noexcept {
    return co_csdo_get_num(this);
  }

  const co_sdo_par&
  getPar() const noexcept {
    return *co_csdo_get_par(this);
  }

  int
  getTimeout() const noexcept {
    return co_csdo_get_timeout(this);
  }

  void
  setTimeout(int timeout) noexcept {
    co_csdo_set_timeout(this, timeout);
  }

  void
  getDnInd(co_csdo_ind_t** pind, void** pdata) const noexcept {
    co_csdo_get_dn_ind(this, pind, pdata);
  }

  void
  setDnInd(co_csdo_ind_t* ind, void* data) noexcept {
    co_csdo_set_dn_ind(this, ind, data);
  }

  template <class F>
  void
  setDnInd(F* f) noexcept {
    setDnInd(&c_obj_call<co_csdo_ind_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_csdo_ind_t*, C>::type M>
  void
  setDnInd(C* obj) noexcept {
    setDnInd(&c_mem_call<co_csdo_ind_t*, C, M>::function,
             static_cast<void*>(obj));
  }

  void
  getUpInd(co_csdo_ind_t** pind, void** pdata) const noexcept {
    co_csdo_get_up_ind(this, pind, pdata);
  }

  void
  setUpInd(co_csdo_ind_t* ind, void* data) noexcept {
    co_csdo_set_up_ind(this, ind, data);
  }

  template <class F>
  void
  setUpInd(F* f) noexcept {
    setUpInd(&c_obj_call<co_csdo_ind_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_csdo_ind_t*, C>::type M>
  void
  setUpInd(C* obj) noexcept {
    setUpInd(&c_mem_call<co_csdo_ind_t*, C, M>::function,
             static_cast<void*>(obj));
  }

  bool
  isIdle() const noexcept {
    return !!co_csdo_is_idle(this);
  }

  void
  abortReq(co_unsigned32_t ac = CO_SDO_AC_ERROR) noexcept {
    co_csdo_abort_req(this, ac);
  }

  int
  dnReq(co_unsigned16_t idx, co_unsigned8_t subidx, const void* ptr, size_t n,
        co_csdo_dn_con_t* con, void* data) noexcept {
    return co_csdo_dn_req(this, idx, subidx, ptr, n, con, data);
  }

  template <class F>
  int
  dnReq(co_unsigned16_t idx, co_unsigned8_t subidx, const void* ptr, size_t n,
        F* f) noexcept {
    return dnReq(idx, subidx, ptr, n,
                 &c_obj_call<co_csdo_dn_con_t*, F>::function,
                 static_cast<void*>(f));
  }

  template <class T, typename c_mem_fn<co_csdo_dn_con_t*, T>::type M>
  int
  dnReq(co_unsigned16_t idx, co_unsigned8_t subidx, const void* ptr, size_t n,
        T* t) noexcept {
    return dnReq(idx, subidx, ptr, n,
                 &c_mem_call<co_csdo_dn_con_t*, T, M>::function,
                 static_cast<void*>(t));
  }

  template <co_unsigned16_t N>
  int
  dnReq(co_unsigned16_t idx, co_unsigned8_t subidx, const COVal<N>& val,
        co_csdo_dn_con_t* con, void* data) noexcept {
    return co_csdo_dn_val_req(this, idx, subidx, N, &val, con, data);
  }

  template <co_unsigned16_t N, class F>
  int
  dnReq(co_unsigned16_t idx, co_unsigned8_t subidx, const COVal<N>& val,
        F* f) noexcept {
    return dnReq<N>(idx, subidx, val,
                    &c_obj_call<co_csdo_dn_con_t*, F>::function,
                    static_cast<void*>(f));
  }

  template <co_unsigned16_t N, class T,
            typename c_mem_fn<co_csdo_dn_con_t*, T>::type M>
  int
  dnReq(co_unsigned16_t idx, co_unsigned8_t subidx, const COVal<N>& val,
        T* t) noexcept {
    return dnReq<N>(idx, subidx, val,
                    &c_mem_call<co_csdo_dn_con_t*, T, M>::function,
                    static_cast<void*>(t));
  }

  int
  upReq(co_unsigned16_t idx, co_unsigned8_t subidx, co_csdo_up_con_t* con,
        void* data) noexcept {
    return co_csdo_up_req(this, idx, subidx, con, data);
  }

  template <class T, typename COCSDOUpCon<T>::type M>
  int
  upReq(co_unsigned16_t idx, co_unsigned8_t subidx, void* data) noexcept {
    return upReq(idx, subidx, &COCSDOUpCon<T>::template function<M>, data);
  }

  template <class T, class F>
  int
  upReq(co_unsigned16_t idx, co_unsigned8_t subidx, F* f) noexcept {
    return upReq(idx, subidx,
                 &COCSDOUpCon<T>::template function<
                     &c_obj_call<typename COCSDOUpCon<T>::type, F>::function>,
                 static_cast<void*>(f));
  }

  template <class T, class C,
            typename c_mem_fn<typename COCSDOUpCon<T>::type, C>::type M>
  int
  upReq(co_unsigned16_t idx, co_unsigned8_t subidx, C* obj) noexcept {
    return upReq(
        idx, subidx,
        &COCSDOUpCon<T>::template function<
            &c_mem_call<typename COCSDOUpCon<T>::type, C, M>::function>,
        static_cast<void*>(obj));
  }

  int
  blkDnReq(co_unsigned16_t idx, co_unsigned8_t subidx, const void* ptr,
           size_t n, co_csdo_dn_con_t* con, void* data) noexcept {
    return co_csdo_blk_dn_req(this, idx, subidx, ptr, n, con, data);
  }

  template <class F>
  int
  blkDnReq(co_unsigned16_t idx, co_unsigned8_t subidx, const void* ptr,
           size_t n, F* f) noexcept {
    return blkDnReq(idx, subidx, ptr, n,
                    &c_obj_call<co_csdo_dn_con_t*, F>::function,
                    static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_csdo_dn_con_t*, C>::type M>
  int
  blkDnReq(co_unsigned16_t idx, co_unsigned8_t subidx, const void* ptr,
           size_t n, C* obj) noexcept {
    return blkDnReq(idx, subidx, ptr, n,
                    &c_mem_call<co_csdo_dn_con_t*, C, M>::function,
                    static_cast<void*>(obj));
  }

  int
  blkUpReq(co_unsigned16_t idx, co_unsigned8_t subidx, co_unsigned8_t pst,
           co_csdo_up_con_t* con, void* data) noexcept {
    return co_csdo_blk_up_req(this, idx, subidx, pst, con, data);
  }

  template <class T, typename COCSDOUpCon<T>::type M>
  int
  blkUpReq(co_unsigned16_t idx, co_unsigned8_t subidx, co_unsigned8_t pst,
           void* data) noexcept {
    return blkUpReq(idx, subidx, pst, &COCSDOUpCon<T>::template function<M>,
                    data);
  }

  template <class T, class F>
  int
  blkUpReq(co_unsigned16_t idx, co_unsigned8_t subidx, co_unsigned8_t pst,
           F* f) noexcept {
    return blkUpReq(
        idx, subidx, pst,
        &COCSDOUpCon<T>::template function<
            &c_obj_call<typename COCSDOUpCon<T>::type, F>::function>,
        static_cast<void*>(f));
  }

  template <class T, class C,
            typename c_mem_fn<typename COCSDOUpCon<T>::type, C>::type M>
  int
  blkUpReq(co_unsigned16_t idx, co_unsigned8_t subidx, co_unsigned8_t pst,
           C* obj) noexcept {
    return blkUpReq(
        idx, subidx, pst,
        &COCSDOUpCon<T>::template function<
            &c_mem_call<typename COCSDOUpCon<T>::type, C, M>::function>,
        static_cast<void*>(obj));
  }

 protected:
  ~COCSDO() = default;
};

/**
 * A CANopen Client-SDO upload confirmation callback wrapper that deserializes
 * the received value on success.
 *
 * @see co_csdo_up_con_t
 */
template <class T>
struct COCSDOUpCon {
  typedef void (*type)(COCSDO* sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
                       co_unsigned32_t ac, T val, void* data);

  template <type M>
  static void
  function(COCSDO* sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
           co_unsigned32_t ac, const void* ptr, size_t n, void* data) noexcept {
    if (!ac) {
      if (!ptr || n < sizeof(T)) {
        ac = CO_SDO_AC_TYPE_LEN_LO;
      } else if (n > sizeof(T)) {
        ac = CO_SDO_AC_TYPE_LEN_HI;
      }
    }
    COVal<co_type_traits_T<T>::index> val;
    if (!ac) ac = co_val_read_sdo(val.index, &val, ptr, n);
    return (*M)(sdo, idx, subidx, ac, val, data);
  }
};

/**
 * A CANopen Client-SDO upload confirmation callback wrapper that deserializes
 * the received array of visible characters on success.
 *
 * @see co_csdo_up_con_t
 */
template <>
struct COCSDOUpCon<char*> {
  typedef void (*type)(COCSDO* sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
                       co_unsigned32_t ac, const char* vs, void* data);

  template <type M>
  static void
  function(COCSDO* sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
           co_unsigned32_t ac, const void* ptr, size_t n, void* data) noexcept {
    COVal<CO_DEFTYPE_VISIBLE_STRING> val;
    if (!ac && ptr && n) ac = co_val_read_sdo(val.index, &val, ptr, n);
    return (*M)(sdo, idx, subidx, ac, val, data);
  }
};

/**
 * A CANopen Client-SDO upload confirmation callback wrapper that deserializes
 * the received array of visible characters on success.
 *
 * @see co_csdo_up_con_t
 */
template <>
struct COCSDOUpCon<::std::string> {
  typedef void (*type)(COCSDO* sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
                       co_unsigned32_t ac, ::std::string vs, void* data);

  template <type M>
  static void
  function(COCSDO* sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
           co_unsigned32_t ac, const void* ptr, size_t n, void* data) noexcept {
    const char* vs = static_cast<const char*>(ptr);
    if (!ac && vs && n) {
#if !__cpp_exceptions
      try {
#endif
        return (*M)(sdo, idx, subidx, ac, ::std::string(vs, vs + n), data);
#if !__cpp_exceptions
      } catch (...) {
        ac = CO_SDO_AC_NO_MEM;
      }
#endif
    }
    return (*M)(sdo, idx, subidx, ac, ::std::string(), data);
  }
};

/**
 * A CANopen Client-SDO upload confirmation callback wrapper for an array of
 * octets.
 *
 * @see co_csdo_up_con_t
 */
template <>
struct COCSDOUpCon<uint_least8_t*> {
  typedef void (*type)(COCSDO* sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
                       co_unsigned32_t ac, const uint_least8_t* os, size_t n,
                       void* data);

  template <type M>
  static void
  function(COCSDO* sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
           co_unsigned32_t ac, const void* ptr, size_t n, void* data) noexcept {
    const uint_least8_t* os = static_cast<const uint_least8_t*>(ptr);
    return (*M)(sdo, idx, subidx, ac, n ? os : 0, n, data);
  }
};

/**
 * A CANopen Client-SDO upload confirmation callback wrapper that deserializes
 * the received array of octets on success.
 *
 * @see co_csdo_up_con_t
 */
template <>
struct COCSDOUpCon<::std::vector<uint_least8_t>> {
  typedef void (*type)(COCSDO* sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
                       co_unsigned32_t ac, ::std::vector<uint_least8_t> os,
                       void* data);

  template <type M>
  static void
  function(COCSDO* sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
           co_unsigned32_t ac, const void* ptr, size_t n, void* data) noexcept {
    const uint_least8_t* os = static_cast<const uint_least8_t*>(ptr);
    if (!ac && os && n) {
#if !__cpp_exceptions
      try {
#endif
        return (*M)(sdo, idx, subidx, ac,
                    ::std::vector<uint_least8_t>(os, os + n), data);
#if !__cpp_exceptions
      } catch (...) {
        ac = CO_SDO_AC_NO_MEM;
      }
#endif
    }
    return (*M)(sdo, idx, subidx, ac, ::std::vector<uint_least8_t>(), data);
  }
};

/**
 * A CANopen Client-SDO upload confirmation callback wrapper that deserializes
 * the received array of (16-bit) Unicode characters on success.
 *
 * @see co_csdo_up_con_t
 */
template <>
struct COCSDOUpCon<char16_t*> {
  typedef void (*type)(COCSDO* sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
                       co_unsigned32_t ac, const char16_t* us, void* data);

  template <type M>
  static void
  function(COCSDO* sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
           co_unsigned32_t ac, const void* ptr, size_t n, void* data) noexcept {
    COVal<CO_DEFTYPE_UNICODE_STRING> val;
    if (!ac && ptr && n) ac = co_val_read_sdo(val.index, &val, ptr, n);
    return (*M)(sdo, idx, subidx, ac, val, data);
  }
};

/**
 * A CANopen Client-SDO upload confirmation callback wrapper that deserializes
 * the received array of (16-bit) Unicode characters on success.
 *
 * @see co_csdo_up_con_t
 */
template <>
struct COCSDOUpCon<::std::basic_string<char16_t>> {
  typedef void (*type)(COCSDO* sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
                       co_unsigned32_t ac, ::std::basic_string<char16_t> us,
                       void* data);

  template <type M>
  static void
  function(COCSDO* sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
           co_unsigned32_t ac, const void* ptr, size_t n, void* data) noexcept {
    COVal<CO_DEFTYPE_UNICODE_STRING> val;
    if (!ac && ptr && n) ac = co_val_read_sdo(val.index, &val, ptr, n);
    const char16_t* us = val;
    if (!ac && us && n) {
#if !__cpp_exceptions
      try {
#endif
        return (*M)(sdo, idx, subidx, ac,
                    ::std::basic_string<char16_t>(us, us + n), data);
#if !__cpp_exceptions
      } catch (...) {
        ac = CO_SDO_AC_NO_MEM;
      }
#endif
    }
    return (*M)(sdo, idx, subidx, ac, ::std::basic_string<char16_t>(), data);
  }
};

/**
 * A CANopen Client-SDO upload confirmation callback wrapper for an arbitrary
 * large block of data.
 *
 * @see co_csdo_up_con_t
 */
template <>
struct COCSDOUpCon<void*> {
  typedef void (*type)(COCSDO* sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
                       co_unsigned32_t ac, const void* dom, size_t n,
                       void* data);

  template <type M>
  static void
  function(COCSDO* sdo, co_unsigned16_t idx, co_unsigned8_t subidx,
           co_unsigned32_t ac, const void* ptr, size_t n, void* data) noexcept {
    return (*M)(sdo, idx, subidx, ac, n ? ptr : 0, n, data);
  }
};

}  // namespace lely

#endif  // !LELY_CO_CSDO_HPP_
