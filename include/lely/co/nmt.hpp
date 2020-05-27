/**@file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the network management (NMT) declarations. See lely/co/nmt.h for
 * the C interface.
 *
 * @copyright 2017-2019 Lely Industries N.V.
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

#ifndef LELY_CO_NMT_HPP_
#define LELY_CO_NMT_HPP_

#if !defined(__cplusplus) || LELY_NO_CXX
#error "include <lely/co/nmt.h> for the C interface"
#endif

#include <lely/can/net.hpp>
#include <lely/co/nmt.h>

namespace lely {

inline co_unsigned32_t
cfgHb(CODev& dev, co_unsigned8_t id, co_unsigned16_t ms) noexcept {
  return co_dev_cfg_hb(&dev, id, ms);
}

/// The attributes of #co_nmt_t required by #lely::CONMT.
template <>
struct c_type_traits<__co_nmt> {
  typedef __co_nmt value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __co_nmt_alloc();
  }

  static void
  free(void* ptr) noexcept {
    __co_nmt_free(ptr);
  }

  static pointer
  init(pointer p, CANNet* net, CODev* dev) noexcept {
    return __co_nmt_init(p, net, dev);
  }

  static void
  fini(pointer p) noexcept {
    __co_nmt_fini(p);
  }
};

/// An opaque CANopen NMT master/slave service type.
class CONMT : public incomplete_c_type<__co_nmt> {
  typedef incomplete_c_type<__co_nmt> c_base;

 public:
  CONMT(CANNet* net, CODev* dev) : c_base(net, dev) {}

  CANNet*
  getNet() const noexcept {
    return co_nmt_get_net(this);
  }

  CODev*
  getDev() const noexcept {
    return co_nmt_get_dev(this);
  }

  void
  getCsInd(co_nmt_cs_ind_t** pind, void** pdata) const noexcept {
    co_nmt_get_cs_ind(this, pind, pdata);
  }

  void
  setCsInd(co_nmt_cs_ind_t* ind, void* data) noexcept {
    co_nmt_set_cs_ind(this, ind, data);
  }

  template <class F>
  void
  setCsInd(F* f) noexcept {
    setCsInd(&c_obj_call<co_nmt_cs_ind_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_nmt_cs_ind_t*, C>::type M>
  void
  setCsInd(C* obj) noexcept {
    setCsInd(&c_mem_call<co_nmt_cs_ind_t*, C, M>::function,
             static_cast<void*>(obj));
  }

  void
  getNgInd(co_nmt_ng_ind_t** pind, void** pdata) const noexcept {
    co_nmt_get_ng_ind(this, pind, pdata);
  }

  void
  setNgInd(co_nmt_ng_ind_t* ind, void* data) noexcept {
    co_nmt_set_ng_ind(this, ind, data);
  }

  template <class F>
  void
  setNgInd(F* f) noexcept {
    setNgInd(&c_obj_call<co_nmt_ng_ind_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_nmt_ng_ind_t*, C>::type M>
  void
  setNgInd(C* obj) noexcept {
    setNgInd(&c_mem_call<co_nmt_ng_ind_t*, C, M>::function,
             static_cast<void*>(obj));
  }

  void
  onNg(co_unsigned8_t id, int state, int reason) noexcept {
    co_nmt_on_ng(this, id, state, reason);
  }

  void
  getLgInd(co_nmt_lg_ind_t** pind, void** pdata) const noexcept {
    co_nmt_get_lg_ind(this, pind, pdata);
  }

  void
  setLgInd(co_nmt_lg_ind_t* ind, void* data) noexcept {
    co_nmt_set_lg_ind(this, ind, data);
  }

  template <class F>
  void
  setLgInd(F* f) noexcept {
    setLgInd(&c_obj_call<co_nmt_lg_ind_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_nmt_lg_ind_t*, C>::type M>
  void
  setLgInd(C* obj) noexcept {
    setLgInd(&c_mem_call<co_nmt_lg_ind_t*, C, M>::function,
             static_cast<void*>(obj));
  }

  void
  onLg(int state) noexcept {
    co_nmt_on_lg(this, state);
  }

  void
  getHbInd(co_nmt_hb_ind_t** pind, void** pdata) const noexcept {
    co_nmt_get_hb_ind(this, pind, pdata);
  }

  void
  setHbInd(co_nmt_hb_ind_t* ind, void* data) noexcept {
    co_nmt_set_hb_ind(this, ind, data);
  }

  template <class F>
  void
  setHbInd(F* f) noexcept {
    setHbInd(&c_obj_call<co_nmt_hb_ind_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_nmt_hb_ind_t*, C>::type M>
  void
  setHbInd(C* obj) noexcept {
    setHbInd(&c_mem_call<co_nmt_hb_ind_t*, C, M>::function,
             static_cast<void*>(obj));
  }

  void
  onHb(co_unsigned8_t id, int state, int reason) noexcept {
    co_nmt_on_hb(this, id, state, reason);
  }

  void
  getStInd(co_nmt_st_ind_t** pind, void** pdata) const noexcept {
    co_nmt_get_st_ind(this, pind, pdata);
  }

  void
  setStInd(co_nmt_st_ind_t* ind, void* data) noexcept {
    co_nmt_set_st_ind(this, ind, data);
  }

  template <class F>
  void
  setStInd(F* f) noexcept {
    setStInd(&c_obj_call<co_nmt_st_ind_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_nmt_st_ind_t*, C>::type M>
  void
  setStInd(C* obj) noexcept {
    setStInd(&c_mem_call<co_nmt_st_ind_t*, C, M>::function,
             static_cast<void*>(obj));
  }

  void
  onSt(co_unsigned8_t id, co_unsigned8_t st) noexcept {
    co_nmt_on_st(this, id, st);
  }

  void
  getLSSReq(co_nmt_lss_req_t** preq, void** pdata) const noexcept {
    co_nmt_get_lss_req(this, preq, pdata);
  }

  void
  setLSSReq(co_nmt_lss_req_t* req, void* data) noexcept {
    co_nmt_set_lss_req(this, req, data);
  }

  template <class F>
  void
  setLSSReq(F* f) noexcept {
    setLSSReq(&c_obj_call<co_nmt_lss_req_t*, F>::function,
              static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_nmt_lss_req_t*, C>::type M>
  void
  setLSSReq(C* obj) noexcept {
    setLSSReq(&c_mem_call<co_nmt_lss_req_t*, C, M>::function,
              static_cast<void*>(obj));
  }

  void
  getBootInd(co_nmt_boot_ind_t** pind, void** pdata) const noexcept {
    co_nmt_get_boot_ind(this, pind, pdata);
  }

  void
  setBootInd(co_nmt_boot_ind_t* ind, void* data) noexcept {
    co_nmt_set_boot_ind(this, ind, data);
  }

  template <class F>
  void
  setBootInd(F* f) noexcept {
    setBootInd(&c_obj_call<co_nmt_boot_ind_t*, F>::function,
               static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_nmt_boot_ind_t*, C>::type M>
  void
  setBootInd(C* obj) noexcept {
    setBootInd(&c_mem_call<co_nmt_boot_ind_t*, C, M>::function,
               static_cast<void*>(obj));
  }

  void
  getCfgInd(co_nmt_cfg_ind_t** pind, void** pdata) const noexcept {
    co_nmt_get_cfg_ind(this, pind, pdata);
  }

  void
  setCfgInd(co_nmt_cfg_ind_t* ind, void* data) noexcept {
    co_nmt_set_cfg_ind(this, ind, data);
  }

  template <class F>
  void
  setCfgInd(F* f) noexcept {
    setCfgInd(&c_obj_call<co_nmt_cfg_ind_t*, F>::function,
              static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_nmt_cfg_ind_t*, C>::type M>
  void
  setCfgInd(C* obj) noexcept {
    setCfgInd(&c_mem_call<co_nmt_cfg_ind_t*, C, M>::function,
              static_cast<void*>(obj));
  }

  void
  getDnInd(co_nmt_sdo_ind_t** pind, void** pdata) const noexcept {
    co_nmt_get_dn_ind(this, pind, pdata);
  }

  void
  setDnInd(co_nmt_sdo_ind_t* ind, void* data) noexcept {
    co_nmt_set_dn_ind(this, ind, data);
  }

  template <class F>
  void
  setDnInd(F* f) noexcept {
    setDnInd(&c_obj_call<co_nmt_sdo_ind_t*, F>::function,
             static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_nmt_sdo_ind_t*, C>::type M>
  void
  setDnInd(C* obj) noexcept {
    setDnInd(&c_mem_call<co_nmt_sdo_ind_t*, C, M>::function,
             static_cast<void*>(obj));
  }

  void
  getUpInd(co_nmt_sdo_ind_t** pind, void** pdata) const noexcept {
    co_nmt_get_up_ind(this, pind, pdata);
  }

  void
  setUpInd(co_nmt_sdo_ind_t* ind, void* data) noexcept {
    co_nmt_set_up_ind(this, ind, data);
  }

  template <class F>
  void
  setUpInd(F* f) noexcept {
    setUpInd(&c_obj_call<co_nmt_sdo_ind_t*, F>::function,
             static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_nmt_sdo_ind_t*, C>::type M>
  void
  setUpInd(C* obj) noexcept {
    setUpInd(&c_mem_call<co_nmt_sdo_ind_t*, C, M>::function,
             static_cast<void*>(obj));
  }

  void
  getSyncInd(co_nmt_sync_ind_t** pind, void** pdata) const noexcept {
    co_nmt_get_sync_ind(this, pind, pdata);
  }

  void
  setSyncInd(co_nmt_sync_ind_t* ind, void* data) noexcept {
    co_nmt_set_sync_ind(this, ind, data);
  }

  template <class F>
  void
  setSyncInd(F* f) noexcept {
    setSyncInd(&c_obj_call<co_nmt_sync_ind_t*, F>::function,
               static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_nmt_sync_ind_t*, C>::type M>
  void
  setSyncInd(C* obj) noexcept {
    setSyncInd(&c_mem_call<co_nmt_sync_ind_t*, C, M>::function,
               static_cast<void*>(obj));
  }

  void
  onSync(co_unsigned8_t cs = 0) noexcept {
    co_nmt_on_sync(this, cs);
  }

  void
  onErr(co_unsigned16_t eec, co_unsigned8_t er,
        const co_unsigned8_t msef[5] = 0) noexcept {
    co_nmt_on_err(this, eec, er, msef);
  }

  void
  onTPDOEvent(co_unsigned16_t n = 0) noexcept {
    co_nmt_on_tpdo_event(this, n);
  }

  void
  onTPDOEventLock() noexcept {
    co_nmt_on_tpdo_event_lock(this);
  }

  void
  onTPDOEventUnlock() noexcept {
    co_nmt_on_tpdo_event_unlock(this);
  }

  co_unsigned8_t
  getId() const noexcept {
    return co_nmt_get_id(this);
  }

  int
  setId(co_unsigned8_t id) noexcept {
    return co_nmt_set_id(this, id);
  }

  co_unsigned8_t
  getSt() const noexcept {
    return co_nmt_get_st(this);
  }

  bool
  isMaster() const noexcept {
    return !!co_nmt_is_master(this);
  }

  int
  getTimeout() const noexcept {
    return co_nmt_get_timeout(this);
  }

  void
  setTimeout(int timeout) noexcept {
    co_nmt_set_timeout(this, timeout);
  }

  int
  csReq(co_unsigned8_t cs, co_unsigned8_t id = 0) noexcept {
    return co_nmt_cs_req(this, cs, id);
  }

  int
  LSSCon() noexcept {
    return co_nmt_lss_con(this);
  }

  int
  bootReq(co_unsigned8_t id, int timeout) noexcept {
    return co_nmt_boot_req(this, id, timeout);
  }

  bool
  isBooting(co_unsigned8_t id) const noexcept {
    return !!co_nmt_is_booting(this, id);
  }

  int
  cfgReq(co_unsigned8_t id, int timeout, co_nmt_cfg_con_t* con,
         void* data) noexcept {
    return co_nmt_cfg_req(this, id, timeout, con, data);
  }

  template <class F>
  int
  cfgReq(co_unsigned8_t id, int timeout, F* f) noexcept {
    return cfgReq(id, timeout, &c_obj_call<co_nmt_cfg_con_t*, F>::function,
                  static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_nmt_cfg_con_t*, C>::type M>
  int
  cfgReq(co_unsigned8_t id, int timeout, C* obj) noexcept {
    return cfgReq(id, timeout, &c_mem_call<co_nmt_cfg_con_t*, C, M>::function,
                  static_cast<void*>(obj));
  }

  int
  cfgRes(co_unsigned8_t id, co_unsigned32_t ac) noexcept {
    return co_nmt_cfg_res(this, id, ac);
  }

  int
  ngReq(co_unsigned8_t id, co_unsigned16_t gt, co_unsigned8_t ltf) noexcept {
    return co_nmt_ng_req(this, id, gt, ltf);
  }

  int
  csInd(co_unsigned8_t cs) noexcept {
    return co_nmt_cs_ind(this, cs);
  }

  void
  commErrInd() noexcept {
    co_nmt_comm_err_ind(this);
  }

  int
  nodeErrInd(co_unsigned8_t id) noexcept {
    return co_nmt_node_err_ind(this, id);
  }

  CORPDO*
  getRPDO(co_unsigned16_t n) const noexcept {
    return co_nmt_get_rpdo(this, n);
  }

  COTPDO*
  getTPDO(co_unsigned16_t n) const noexcept {
    return co_nmt_get_tpdo(this, n);
  }

  COSSDO*
  getSSDO(co_unsigned8_t n) const noexcept {
    return co_nmt_get_ssdo(this, n);
  }

  COCSDO*
  getCSDO(co_unsigned8_t n) const noexcept {
    return co_nmt_get_csdo(this, n);
  }

  COSync*
  getSync() const noexcept {
    return co_nmt_get_sync(this);
  }

  COTime*
  getTime() const noexcept {
    return co_nmt_get_time(this);
  }

  COEmcy*
  getEmcy() const noexcept {
    return co_nmt_get_emcy(this);
  }

  COLSS*
  getLSS() const noexcept {
    return co_nmt_get_lss(this);
  }

 protected:
  ~CONMT() = default;
};

}  // namespace lely

#endif  // !LELY_CO_NMT_HPP_
