/**@file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the Layer Setting Services (LSS) and protocols declarations. See
 * lely/co/lss.h for the C interface.
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

#ifndef LELY_CO_LSS_HPP_
#define LELY_CO_LSS_HPP_

#if !defined(__cplusplus) || LELY_NO_CXX
#error "include <lely/co/lss.h> for the C interface"
#endif

#include <lely/can/net.hpp>
#include <lely/co/lss.h>

namespace lely {

/// The attributes of #co_lss_t required by #lely::COLSS.
template <>
struct c_type_traits<__co_lss> {
  typedef __co_lss value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __co_lss_alloc();
  }

  static void
  free(void* ptr) noexcept {
    __co_lss_free(ptr);
  }

  static pointer
  init(pointer p, CONMT* nmt) noexcept {
    return __co_lss_init(p, nmt);
  }

  static void
  fini(pointer p) noexcept {
    __co_lss_fini(p);
  }
};

/// An opaque CANopen LSS master/slave service type.
class COLSS : public incomplete_c_type<__co_lss> {
  typedef incomplete_c_type<__co_lss> c_base;

 public:
  explicit COLSS(CONMT* nmt) : c_base(nmt) {}

  int
  start() noexcept {
    return co_lss_start(this);
  }

  void
  stop() noexcept {
    co_lss_stop(this);
  }

  CONMT*
  getNMT() const noexcept {
    return co_lss_get_nmt(this);
  }

  void
  getRateInd(co_lss_rate_ind_t** pind, void** pdata) const noexcept {
    co_lss_get_rate_ind(this, pind, pdata);
  }

  void
  setRateInd(co_lss_rate_ind_t* ind, void* data) noexcept {
    co_lss_set_rate_ind(this, ind, data);
  }

  template <class F>
  void
  setRateInd(F* f) noexcept {
    setRateInd(&c_obj_call<co_lss_rate_ind_t*, F>::function,
               static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_lss_rate_ind_t*, C>::type M>
  void
  setRateInd(C* obj) noexcept {
    setRateInd(&c_mem_call<co_lss_rate_ind_t*, C, M>::function,
               static_cast<void*>(obj));
  }

  void
  getStoreInd(co_lss_store_ind_t** pind, void** pdata) const noexcept {
    co_lss_get_store_ind(this, pind, pdata);
  }

  void
  setStoreInd(co_lss_store_ind_t* ind, void* data) noexcept {
    co_lss_set_store_ind(this, ind, data);
  }

  template <class F>
  void
  setStoreInd(F* f) noexcept {
    setStoreInd(&c_obj_call<co_lss_store_ind_t*, F>::function,
                static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_lss_store_ind_t*, C>::type M>
  void
  setStoreInd(C* obj) noexcept {
    setStoreInd(&c_mem_call<co_lss_store_ind_t*, C, M>::function,
                static_cast<void*>(obj));
  }

  co_unsigned16_t
  getInhibit() const noexcept {
    return co_lss_get_inhibit(this);
  }

  void
  setInhibit(co_unsigned16_t inhibit) noexcept {
    co_lss_set_inhibit(this, inhibit);
  }

  int
  getTimeout() const noexcept {
    return co_lss_get_timeout(this);
  }

  void
  setTimeout(int timeout) noexcept {
    co_lss_set_timeout(this, timeout);
  }

  bool
  isMaster() const noexcept {
    return !!co_lss_is_master(this);
  }

  bool
  isIdle() const noexcept {
    return !!co_lss_is_idle(this);
  }

  void
  abortReq() noexcept {
    co_lss_abort_req(this);
  }

  int
  switchReq(co_unsigned8_t mode) noexcept {
    return co_lss_switch_req(this, mode);
  }

  int
  switchSelReq(const co_id& id, co_lss_cs_ind_t* ind, void* data) noexcept {
    return co_lss_switch_sel_req(this, &id, ind, data);
  }

  template <class F>
  int
  switchSelReq(const co_id& id, F* f) noexcept {
    return switchSelReq(id, &c_obj_call<co_lss_cs_ind_t*, F>::function,
                        static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_lss_cs_ind_t*, C>::type M>
  int
  switchSelReq(const co_id& id, C* obj) noexcept {
    return switchSelReq(id, &c_mem_call<co_lss_cs_ind_t*, C, M>::function,
                        static_cast<void*>(obj));
  }

  int
  setIdReq(co_unsigned8_t id, co_lss_err_ind_t* ind, void* data) noexcept {
    return co_lss_set_id_req(this, id, ind, data);
  }

  template <class F>
  int
  setIdReq(co_unsigned8_t id, F* f) noexcept {
    return setIdReq(id, &c_obj_call<co_lss_err_ind_t*, F>::function,
                    static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_lss_err_ind_t*, C>::type M>
  int
  setIdReq(co_unsigned8_t id, C* obj) noexcept {
    return setIdReq(id, &c_mem_call<co_lss_err_ind_t*, C, M>::function,
                    static_cast<void*>(obj));
  }

  int
  setRateReq(co_unsigned16_t rate, co_lss_err_ind_t* ind, void* data) noexcept {
    return co_lss_set_rate_req(this, rate, ind, data);
  }

  template <class F>
  int
  setRateReq(co_unsigned16_t rate, F* f) noexcept {
    return setRateReq(rate, &c_obj_call<co_lss_err_ind_t*, F>::function,
                      static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_lss_err_ind_t*, C>::type M>
  int
  setRateReq(co_unsigned16_t rate, C* obj) noexcept {
    return setRateReq(rate, &c_mem_call<co_lss_err_ind_t*, C, M>::function,
                      static_cast<void*>(obj));
  }

  int
  switchRateReq(int delay) noexcept {
    return co_lss_switch_rate_req(this, delay);
  }

  int
  storeReq(co_lss_err_ind_t* ind, void* data) noexcept {
    return co_lss_store_req(this, ind, data);
  }

  template <class F>
  int
  storeReq(F* f) noexcept {
    return storeReq(&c_obj_call<co_lss_err_ind_t*, F>::function,
                    static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_lss_err_ind_t*, C>::type M>
  int
  storeReq(C* obj) noexcept {
    return storeReq(&c_mem_call<co_lss_err_ind_t*, C, M>::function,
                    static_cast<void*>(obj));
  }

  int
  getVendorIdReq(co_lss_lssid_ind_t* ind, void* data) noexcept {
    return co_lss_get_vendor_id_req(this, ind, data);
  }

  template <class F>
  int
  getVendorIdReq(F* f) noexcept {
    return getVendorIdReq(&c_obj_call<co_lss_lssid_ind_t*, F>::function,
                          static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_lss_lssid_ind_t*, C>::type M>
  int
  getVendorIdReq(C* obj) noexcept {
    return getVendorIdReq(&c_mem_call<co_lss_lssid_ind_t*, C, M>::function,
                          static_cast<void*>(obj));
  }

  int
  getProductCodeReq(co_lss_lssid_ind_t* ind, void* data) noexcept {
    return co_lss_get_product_code_req(this, ind, data);
  }

  template <class F>
  int
  getProductCodeReq(F* f) noexcept {
    return getProductCodeReq(&c_obj_call<co_lss_lssid_ind_t*, F>::function,
                             static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_lss_lssid_ind_t*, C>::type M>
  int
  getProductCodeReq(C* obj) noexcept {
    return getProductCodeReq(&c_mem_call<co_lss_lssid_ind_t*, C, M>::function,
                             static_cast<void*>(obj));
  }

  int
  getRevisionReq(co_lss_lssid_ind_t* ind, void* data) noexcept {
    return co_lss_get_revision_req(this, ind, data);
  }

  template <class F>
  int
  getRevisionReq(F* f) noexcept {
    return getRevisionReq(&c_obj_call<co_lss_lssid_ind_t*, F>::function,
                          static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_lss_lssid_ind_t*, C>::type M>
  int
  getRevisionReq(C* obj) noexcept {
    return getRevisionReq(&c_mem_call<co_lss_lssid_ind_t*, C, M>::function,
                          static_cast<void*>(obj));
  }

  int
  getSerialNrReq(co_lss_lssid_ind_t* ind, void* data) noexcept {
    return co_lss_get_serial_nr_req(this, ind, data);
  }

  template <class F>
  int
  getSerialNrReq(F* f) noexcept {
    return getSerialNrReq(&c_obj_call<co_lss_lssid_ind_t*, F>::function,
                          static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_lss_lssid_ind_t*, C>::type M>
  int
  getSerialNrReq(C* obj) noexcept {
    return getSerialNrReq(&c_mem_call<co_lss_lssid_ind_t*, C, M>::function,
                          static_cast<void*>(obj));
  }

  int
  getIdReq(co_lss_nid_ind_t* ind, void* data) noexcept {
    return co_lss_get_id_req(this, ind, data);
  }

  template <class F>
  int
  getIdReq(F* f) noexcept {
    return getIdReq(&c_obj_call<co_lss_nid_ind_t*, F>::function,
                    static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_lss_nid_ind_t*, C>::type M>
  int
  getIdReq(C* obj) noexcept {
    return getIdReq(&c_mem_call<co_lss_nid_ind_t*, C, M>::function,
                    static_cast<void*>(obj));
  }

  int
  idSlaveReq(const co_id& lo, const co_id& hi, co_lss_cs_ind_t* ind,
             void* data) noexcept {
    return co_lss_id_slave_req(this, &lo, &hi, ind, data);
  }

  template <class F>
  int
  idSlaveReq(const co_id& lo, const co_id& hi, F* f) noexcept {
    return idSlaveReq(lo, hi, &c_obj_call<co_lss_cs_ind_t*, F>::function,
                      static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_lss_cs_ind_t*, C>::type M>
  int
  idSlaveReq(const co_id& lo, const co_id& hi, C* obj) noexcept {
    return idSlaveReq(lo, hi, &c_mem_call<co_lss_cs_ind_t*, C, M>::function,
                      static_cast<void*>(obj));
  }

  int
  idNonCfgSlaveReq(co_lss_cs_ind_t* ind, void* data) noexcept {
    return co_lss_id_non_cfg_slave_req(this, ind, data);
  }

  template <class F>
  int
  idNonCfgSlaveReq(F* f) noexcept {
    return idNonCfgSlaveReq(&c_obj_call<co_lss_cs_ind_t*, F>::function,
                            static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_lss_cs_ind_t*, C>::type M>
  int
  idNonCfgSlaveReq(C* obj) noexcept {
    return idNonCfgSlaveReq(&c_mem_call<co_lss_cs_ind_t*, C, M>::function,
                            static_cast<void*>(obj));
  }

  int
  slowscanReq(const co_id& lo, const co_id& hi, co_lss_scan_ind_t* ind,
              void* data) noexcept {
    return co_lss_slowscan_req(this, &lo, &hi, ind, data);
  }

  template <class F>
  int
  slowscanReq(const co_id& lo, const co_id& hi, F* f) noexcept {
    return slowscanReq(lo, hi, &c_obj_call<co_lss_scan_ind_t*, F>::function,
                       static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_lss_scan_ind_t*, C>::type M>
  int
  slowscanReq(const co_id& lo, const co_id& hi, C* obj) noexcept {
    return slowscanReq(lo, hi, &c_mem_call<co_lss_scan_ind_t*, C, M>::function,
                       static_cast<void*>(obj));
  }

  int
  fastscanReq(const co_id* id, const co_id* mask, co_lss_scan_ind_t* ind,
              void* data) noexcept {
    return co_lss_fastscan_req(this, id, mask, ind, data);
  }

  template <class F>
  int
  fastscanReq(const co_id* id, const co_id* mask, F* f) noexcept {
    return fastscanReq(id, mask, &c_obj_call<co_lss_scan_ind_t*, F>::function,
                       static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_lss_scan_ind_t*, C>::type M>
  int
  fastscanReq(const co_id* id, const co_id* mask, C* obj) noexcept {
    return fastscanReq(id, mask,
                       &c_mem_call<co_lss_scan_ind_t*, C, M>::function,
                       static_cast<void*>(obj));
  }

 protected:
  ~COLSS() = default;
};

}  // namespace lely

#endif  // !LELY_CO_LSS_HPP_
