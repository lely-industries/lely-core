/**@file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the device description. See lely/co/dev.h for the C interface.
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

#ifndef LELY_CO_DEV_HPP_
#define LELY_CO_DEV_HPP_

#if !defined(__cplusplus) || LELY_NO_CXX
#error "include <lely/co/dev.h> for the C interface"
#endif

#include <lely/util/c_call.hpp>
#include <lely/util/c_type.hpp>
#include <lely/co/dev.h>
#include <lely/co/val.hpp>

#include <vector>

struct floc;
struct co_sdev;

namespace lely {

/// The attributes of #co_dev_t required by #lely::CODev.
template <>
struct c_type_traits<__co_dev> {
  typedef __co_dev value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __co_dev_alloc();
  }

  static void
  free(void* ptr) noexcept {
    __co_dev_free(ptr);
  }

  static pointer
  init(pointer p, co_unsigned8_t id) noexcept {
    return __co_dev_init(p, id);
  }

  static pointer init(pointer p, const char* filename) noexcept;
  static pointer init(pointer p, const char* begin, const char* end,
                      floc* at) noexcept;
  static pointer init(pointer p, const co_sdev* sdev) noexcept;

  static void
  fini(pointer p) noexcept {
    __co_dev_fini(p);
  }
};

/// An opaque CANopen device type.
class CODev : public incomplete_c_type<__co_dev> {
  typedef incomplete_c_type<__co_dev> c_base;

 public:
  explicit CODev(co_unsigned8_t id = 0xff) : c_base(id) {}

  explicit CODev(const char* filename) : c_base(filename) {}

  CODev(const char* begin, const char* end, floc* at = 0)
      : c_base(begin, end, at) {}

  explicit CODev(const co_sdev* sdev) : c_base(sdev) {}

  co_unsigned8_t
  getNetid() const noexcept {
    return co_dev_get_netid(this);
  }

  int
  setNetid(co_unsigned8_t id) noexcept {
    return co_dev_set_netid(this, id);
  }

  co_unsigned8_t
  getId() const noexcept {
    return co_dev_get_id(this);
  }

  int
  setId(co_unsigned8_t id) noexcept {
    return co_dev_set_id(this, id);
  }

  co_unsigned16_t
  getIdx(co_unsigned16_t maxidx, co_unsigned16_t* idx) const noexcept {
    return co_dev_get_idx(this, maxidx, idx);
  }

  ::std::vector<co_unsigned16_t>
  getIdx() const {
    std::vector<co_unsigned16_t> idx(getIdx(0, 0));
    getIdx(idx.size(), idx.data());
    return idx;
  }

  int
  insert(COObj* obj) noexcept {
    return co_dev_insert_obj(this, obj);
  }

  int
  remove(COObj* obj) noexcept {
    return co_dev_remove_obj(this, obj);
  }

  COObj*
  find(co_unsigned16_t idx) const noexcept {
    return co_dev_find_obj(this, idx);
  }

  COSub*
  find(co_unsigned16_t idx, co_unsigned8_t subidx) const noexcept {
    return co_dev_find_sub(this, idx, subidx);
  }

  const char*
  getName() const noexcept {
    return co_dev_get_name(this);
  }

  int
  setName(const char* name) noexcept {
    return co_dev_set_name(this, name);
  }

  const char*
  getVendorName() const noexcept {
    return co_dev_get_vendor_name(this);
  }

  int
  setVendorName(const char* vendor_name) noexcept {
    return co_dev_set_vendor_name(this, vendor_name);
  }

  co_unsigned32_t
  getVendorId() const noexcept {
    return co_dev_get_vendor_id(this);
  }

  void
  setVendorId(co_unsigned32_t vendor_id) noexcept {
    co_dev_set_vendor_id(this, vendor_id);
  }

  const char*
  getProductName() const noexcept {
    return co_dev_get_product_name(this);
  }

  int
  setProductName(const char* product_name) noexcept {
    return co_dev_set_product_name(this, product_name);
  }

  co_unsigned32_t
  getProductCode() const noexcept {
    return co_dev_get_product_code(this);
  }

  void
  setProductCode(co_unsigned32_t product_code) noexcept {
    co_dev_set_product_code(this, product_code);
  }

  co_unsigned32_t
  getRevision() const noexcept {
    return co_dev_get_revision(this);
  }

  void
  setRevision(co_unsigned32_t revision) noexcept {
    co_dev_set_revision(this, revision);
  }

  const char*
  getOrderCode() const noexcept {
    return co_dev_get_order_code(this);
  }

  int
  setOrderCode(const char* order_code) noexcept {
    return co_dev_set_order_code(this, order_code);
  }

  unsigned int
  getBaud() const noexcept {
    return co_dev_get_baud(this);
  }

  void
  setBaud(unsigned int baud) noexcept {
    co_dev_set_baud(this, baud);
  }

  co_unsigned16_t
  getRate() const noexcept {
    return co_dev_get_rate(this);
  }

  void
  setRate(co_unsigned16_t rate) noexcept {
    co_dev_set_rate(this, rate);
  }

  bool
  getLSS() const noexcept {
    return !!co_dev_get_lss(this);
  }

  void
  setLSS(bool lss) noexcept {
    co_dev_set_lss(this, lss);
  }

  co_unsigned32_t
  getDummy() const noexcept {
    return co_dev_get_dummy(this);
  }

  void
  setDummy(co_unsigned32_t dummy) noexcept {
    co_dev_set_dummy(this, dummy);
  }

  template <co_unsigned16_t N>
  const COVal<N>&
  getVal(co_unsigned16_t idx, co_unsigned8_t subidx) const noexcept {
    return *reinterpret_cast<const COVal<N>*>(
        co_dev_get_val(this, idx, subidx));
  }

  ::std::size_t
  setVal(co_unsigned16_t idx, co_unsigned8_t subidx, const void* ptr,
         ::std::size_t n) noexcept {
    return co_dev_set_val(this, idx, subidx, ptr, n);
  }

  template <co_unsigned16_t N>
  ::std::size_t
  setVal(co_unsigned16_t idx, co_unsigned8_t subidx,
         const COVal<N>& val) noexcept {
    return setVal(idx, subidx, val.address(), val.size());
  }

  template <class T>
  ::std::size_t
  setVal(co_unsigned16_t idx, co_unsigned8_t subidx, const T& val) noexcept {
    return setVal<co_type_traits_T<T>::index>(idx, subidx, val);
  }

  ::std::size_t
  readSub(co_unsigned16_t* pidx, co_unsigned8_t* psubidx,
          const uint_least8_t* begin, const uint_least8_t* end) noexcept {
    return co_dev_read_sub(this, pidx, psubidx, begin, end);
  }

  ::std::size_t
  writeSub(co_unsigned16_t idx, co_unsigned8_t subidx, uint_least8_t* begin,
           uint_least8_t* end = 0) const noexcept {
    return co_dev_write_sub(this, idx, subidx, begin, end);
  }

  int
  readDCF(co_unsigned16_t* pmin, co_unsigned16_t* pmax,
          const COVal<CO_DEFTYPE_DOMAIN>& val) noexcept {
    return co_dev_read_dcf(this, pmin, pmax,
                           reinterpret_cast<void* const*>(&val));
  }

  int
  readDCF(co_unsigned16_t* pmin, co_unsigned16_t* pmax,
          const char* filename) noexcept {
    return co_dev_read_dcf_file(this, pmin, pmax, filename);
  }

  int
  writeDCF(co_unsigned16_t min, co_unsigned16_t max,
           COVal<CO_DEFTYPE_DOMAIN>& val) const noexcept {
    return co_dev_write_dcf(this, min, max, reinterpret_cast<void**>(&val));
  }

  int
  writeDCF(co_unsigned16_t min, co_unsigned16_t max,
           const char* filename) const noexcept {
    return co_dev_write_dcf_file(this, min, max, filename);
  }

  void
  getTPDOEventInd(co_dev_tpdo_event_ind_t** pind, void** pdata) const noexcept {
    co_dev_get_tpdo_event_ind(this, pind, pdata);
  }

  void
  setTPDOEventInd(co_dev_tpdo_event_ind_t* ind, void* data) noexcept {
    co_dev_set_tpdo_event_ind(this, ind, data);
  }

  template <class F>
  void
  setTPDOEventInd(F* f) noexcept {
    setTPDOEventInd(&c_obj_call<co_dev_tpdo_event_ind_t*, F>::function,
                    static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_dev_tpdo_event_ind_t*, C>::type M>
  void
  setTPDOEventInd(C* obj) noexcept {
    setTPDOEventInd(&c_mem_call<co_dev_tpdo_event_ind_t*, C, M>::function,
                    static_cast<void*>(obj));
  }

  void
  TPDOEvent(co_unsigned16_t idx, co_unsigned8_t subidx) noexcept {
    co_dev_tpdo_event(this, idx, subidx);
  }

 protected:
  ~CODev() = default;
};

}  // namespace lely

#endif  // !LELY_CO_DEV_HPP_
