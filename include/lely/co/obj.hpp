/**@file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the object dictionary. See lely/co/obj.h for the C interface.
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

#ifndef LELY_CO_OBJ_HPP_
#define LELY_CO_OBJ_HPP_

#if !defined(__cplusplus) || LELY_NO_CXX
#error "include <lely/co/obj.h> for the C interface"
#endif

#include <lely/util/c_call.hpp>
#include <lely/util/c_type.hpp>
#include <lely/co/obj.h>
#include <lely/co/sdo.h>
#include <lely/co/val.hpp>

#include <vector>

namespace lely {

template <co_unsigned16_t>
struct COSubDnInd;
template <co_unsigned16_t>
struct COSubUpInd;

/// The attributes of #co_obj_t required by #lely::COObj.
template <>
struct c_type_traits<__co_obj> {
  typedef __co_obj value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __co_obj_alloc();
  }

  static void
  free(void* ptr) noexcept {
    __co_obj_free(ptr);
  }

  static pointer
  init(pointer p, co_unsigned16_t idx) noexcept {
    return __co_obj_init(p, idx, 0, 0);
  }

  static void
  fini(pointer p) noexcept {
    __co_obj_fini(p);
  }
};

/// An opaque CANopen object type.
class COObj : public incomplete_c_type<__co_obj> {
  typedef incomplete_c_type<__co_obj> c_base;

 public:
  explicit COObj(co_unsigned16_t idx) : c_base(idx) {}

  CODev*
  getDev() const noexcept {
    return co_obj_get_dev(this);
  }

  co_unsigned16_t
  getIdx() const noexcept {
    return co_obj_get_idx(this);
  }

  co_unsigned8_t
  getSubidx(co_unsigned8_t maxsubidx, co_unsigned8_t* subidx) const noexcept {
    return co_obj_get_subidx(this, maxsubidx, subidx);
  }

  ::std::vector<co_unsigned8_t>
  getSubidx() const {
    std::vector<co_unsigned8_t> subidx(getSubidx(0, 0));
    getSubidx(subidx.size(), subidx.data());
    return subidx;
  }

  int
  insert(COSub* sub) noexcept {
    return co_obj_insert_sub(this, sub);
  }
  int
  remove(COSub* sub) noexcept {
    return co_obj_remove_sub(this, sub);
  }

  COSub*
  find(co_unsigned8_t subidx) const noexcept {
    return co_obj_find_sub(this, subidx);
  }

  const char*
  getName() const noexcept {
    return co_obj_get_name(this);
  }

  int
  setName(const char* name) noexcept {
    return co_obj_set_name(this, name);
  }

  co_unsigned8_t
  getCode() const noexcept {
    return co_obj_get_code(this);
  }

  int
  setCode(co_unsigned8_t code) noexcept {
    return co_obj_set_code(this, code);
  }

  template <co_unsigned16_t N>
  const COVal<N>&
  getVal(co_unsigned8_t subidx) const noexcept {
    return *reinterpret_cast<const COVal<N>*>(co_obj_get_val(this, subidx));
  }

  ::std::size_t
  setVal(co_unsigned8_t subidx, const void* ptr, ::std::size_t n) noexcept {
    return co_obj_set_val(this, subidx, ptr, n);
  }

  template <co_unsigned16_t N>
  ::std::size_t
  setVal(co_unsigned8_t subidx, const COVal<N>& val) noexcept {
    return setVal(subidx, val.address(), val.size());
  }

  template <class T>
  ::std::size_t
  setVal(co_unsigned8_t subidx, const T& val) noexcept {
    return setVal<co_type_traits_T<T>::index>(subidx, val);
  }

  void
  setDnInd(co_sub_dn_ind_t* ind, void* data) noexcept {
    co_obj_set_dn_ind(this, ind, data);
  }

  template <class F>
  void
  setDnInd(F* f) noexcept {
    setDnInd(&c_obj_call<co_sub_dn_ind_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_sub_dn_ind_t*, C>::type M>
  void
  setDnInd(C* obj) noexcept {
    setDnInd(&c_mem_call<co_sub_dn_ind_t*, C, M>::function,
             static_cast<void*>(obj));
  }

  void
  setUpInd(co_sub_up_ind_t* ind, void* data) noexcept {
    co_obj_set_up_ind(this, ind, data);
  }

  template <class F>
  void
  setUpInd(F* f) noexcept {
    setUpInd(&c_obj_call<co_sub_up_ind_t*, F>::function, static_cast<void*>(f));
  }

  template <class C, typename c_mem_fn<co_sub_up_ind_t*, C>::type M>
  void
  setUpInd(C* obj) noexcept {
    setUpInd(&c_mem_call<co_sub_up_ind_t*, C, M>::function,
             static_cast<void*>(obj));
  }

 protected:
  ~COObj() = default;
};

/// The attributes of #co_sub_t required by #lely::COSub.
template <>
struct c_type_traits<__co_sub> {
  typedef __co_sub value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __co_sub_alloc();
  }

  static void
  free(void* ptr) noexcept {
    __co_sub_free(ptr);
  }

  static pointer
  init(pointer p, co_unsigned8_t subidx, co_unsigned16_t type) noexcept {
    return __co_sub_init(p, subidx, type, 0);
  }

  static void
  fini(pointer p) noexcept {
    __co_sub_fini(p);
  }
};

/// An opaque CANopen sub-object type.
class COSub : public incomplete_c_type<__co_sub> {
  typedef incomplete_c_type<__co_sub> c_base;

 public:
  COSub(co_unsigned8_t subidx, co_unsigned16_t type) : c_base(subidx, type) {}

  COObj*
  getObj() const noexcept {
    return co_sub_get_obj(this);
  }

  co_unsigned8_t
  getSubidx() const noexcept {
    return co_sub_get_subidx(this);
  }

  const char*
  getName() const noexcept {
    return co_sub_get_name(this);
  }

  int
  setName(const char* name) noexcept {
    return co_sub_set_name(this, name);
  }

  co_unsigned8_t
  getType() const noexcept {
    return co_sub_get_type(this);
  }

  const void*
  addressofMin() const noexcept {
    return co_sub_addressof_min(this);
  }

  ::std::size_t
  sizeofMin() const noexcept {
    return co_sub_sizeof_min(this);
  }

  template <co_unsigned16_t N>
  const COVal<N>&
  getMin() const noexcept {
    return *reinterpret_cast<const COVal<N>*>(co_sub_get_min(this));
  }

  ::std::size_t
  setMin(const void* ptr, ::std::size_t n) noexcept {
    return co_sub_set_min(this, ptr, n);
  }

  template <co_unsigned16_t N>
  ::std::size_t
  setMin(const COVal<N>& val) noexcept {
    return setMin(val.address(), val.size());
  }

  template <class T>
  ::std::size_t
  setMin(const T& val) noexcept {
    return setMin<co_type_traits_T<T>::index>(val);
  }

  const void*
  addressofMax() const noexcept {
    return co_sub_addressof_max(this);
  }

  ::std::size_t
  sizeofMax() const noexcept {
    return co_sub_sizeof_max(this);
  }

  template <co_unsigned16_t N>
  const COVal<N>&
  getMax() const noexcept {
    return *reinterpret_cast<const COVal<N>*>(co_sub_get_max(this));
  }

  ::std::size_t
  setMax(const void* ptr, ::std::size_t n) noexcept {
    return co_sub_set_max(this, ptr, n);
  }

  template <co_unsigned16_t N>
  ::std::size_t
  setMax(const COVal<N>& val) noexcept {
    return setMax(val.address(), val.size());
  }

  template <class T>
  ::std::size_t
  setMax(const T& val) noexcept {
    return setMax<co_type_traits_T<T>::index>(val);
  }

  const void*
  addressofDef() const noexcept {
    return co_sub_addressof_def(this);
  }

  ::std::size_t
  sizeofDef() const noexcept {
    return co_sub_sizeof_def(this);
  }

  template <co_unsigned16_t N>
  const COVal<N>&
  getDef() const noexcept {
    return *reinterpret_cast<const COVal<N>*>(co_sub_get_def(this));
  }

  ::std::size_t
  setDef(const void* ptr, ::std::size_t n) noexcept {
    return co_sub_set_def(this, ptr, n);
  }

  template <co_unsigned16_t N>
  ::std::size_t
  setDef(const COVal<N>& val) noexcept {
    return setDef(val.address(), val.size());
  }

  template <class T>
  ::std::size_t
  setDef(const T& val) noexcept {
    return setDef<co_type_traits_T<T>::index>(val);
  }

  const void*
  addressofVal() const noexcept {
    return co_sub_addressof_val(this);
  }

  ::std::size_t
  sizeofVal() const noexcept {
    return co_sub_sizeof_val(this);
  }

  template <co_unsigned16_t N>
  const COVal<N>&
  getVal() const noexcept {
    return *reinterpret_cast<const COVal<N>*>(co_sub_get_val(this));
  }

  ::std::size_t
  setVal(const void* ptr, ::std::size_t n) noexcept {
    return co_sub_set_val(this, ptr, n);
  }

  template <co_unsigned16_t N>
  ::std::size_t
  setVal(const COVal<N>& val) noexcept {
    return setVal(val.address(), val.size());
  }

  template <class T>
  ::std::size_t
  setVal(const T& val) noexcept {
    return setVal<co_type_traits_T<T>::index>(val);
  }

  template <co_unsigned16_t N>
  co_unsigned32_t
  chkVal(const COVal<N>& val) const noexcept {
    return co_sub_chk_val(this, N, &val);
  }

  template <class T>
  co_unsigned32_t
  chkVal(const T& val) const noexcept {
    return chkVal<co_type_traits_T<T>::index>(val);
  }

  unsigned int
  getAccess() const noexcept {
    return co_sub_get_access(this);
  }

  int
  setAccess(unsigned int access) noexcept {
    return co_sub_set_access(this, access);
  }

  int
  getPDOMapping() const noexcept {
    return co_sub_get_pdo_mapping(this);
  }

  void
  setPDOMapping(unsigned int pdo_mapping) noexcept {
    co_sub_set_pdo_mapping(this, pdo_mapping);
  }

  unsigned int
  getFlags() const noexcept {
    return co_sub_get_flags(this);
  }

  void
  setFlags(unsigned int flags) noexcept {
    co_sub_set_flags(this, flags);
  }

  const char*
  getUploadFile() const noexcept {
    return co_sub_get_upload_file(this);
  }

  int
  setUploadFile(const char* filename) noexcept {
    return co_sub_set_upload_file(this, filename);
  }

  const char*
  getDownloadFile() const noexcept {
    return co_sub_get_download_file(this);
  }

  int
  setDownloadFile(const char* filename) noexcept {
    return co_sub_set_download_file(this, filename);
  }

  void
  getDnInd(co_sub_dn_ind_t** pind, void** pdata) noexcept {
    co_sub_get_dn_ind(this, pind, pdata);
  }

  void
  setDnInd(co_sub_dn_ind_t* ind, void* data) noexcept {
    co_sub_set_dn_ind(this, ind, data);
  }

  template <co_unsigned16_t N, typename COSubDnInd<N>::type M>
  void
  setDnInd(void* data) noexcept {
    setDnInd(&COSubDnInd<N>::template function<M>, data);
  }

  template <co_unsigned16_t N, class F>
  void
  setDnInd(F* f) noexcept {
    setDnInd(&COSubDnInd<N>::template function<
                 &c_obj_call<typename COSubDnInd<N>::type, F>::function>,
             static_cast<void*>(f));
  }

  template <co_unsigned16_t N, class C,
            typename c_mem_fn<typename COSubDnInd<N>::type, C>::type M>
  void
  setDnInd(C* obj) noexcept {
    setDnInd(&COSubDnInd<N>::template function<
                 &c_mem_call<typename COSubDnInd<N>::type, C, M>::function>,
             static_cast<void*>(obj));
  }

  int
  onDn(co_sdo_req& req, co_unsigned32_t* pac) noexcept {
    return co_sub_on_dn(this, &req, pac);
  }

  co_unsigned32_t
  dnInd(co_sdo_req& req) noexcept {
    return co_sub_dn_ind(this, &req);
  }

  template <co_unsigned16_t N>
  co_unsigned32_t
  dnInd(const COVal<N>& val) noexcept {
    return co_sub_dn_ind_val(this, N, &val);
  }

  template <co_unsigned16_t N>
  int
  dn(COVal<N>& val) noexcept {
    return co_sub_dn(this, &val);
  }

  void
  getUpInd(co_sub_up_ind_t** pind, void** pdata) noexcept {
    co_sub_get_up_ind(this, pind, pdata);
  }

  void
  setUpInd(co_sub_up_ind_t* ind, void* data) noexcept {
    co_sub_set_up_ind(this, ind, data);
  }

  template <co_unsigned16_t N, typename COSubUpInd<N>::type M>
  void
  setUpInd(void* data) noexcept {
    setUpInd(&COSubUpInd<N>::template function<M>, data);
  }

  template <co_unsigned16_t N, class F>
  void
  setUpInd(F* f) noexcept {
    setUpInd(&COSubUpInd<N>::template function<
                 &c_obj_call<typename COSubUpInd<N>::type, F>::function>,
             static_cast<void*>(f));
  }

  template <co_unsigned16_t N, class C,
            typename c_mem_fn<typename COSubUpInd<N>::type, C>::type M>
  void
  setUpInd(C* obj) noexcept {
    setUpInd(&COSubUpInd<N>::template function<
                 &c_mem_call<typename COSubUpInd<N>::type, C, M>::function>,
             static_cast<void*>(obj));
  }

  int
  onUp(co_sdo_req& req, co_unsigned32_t* pac) const noexcept {
    return co_sub_on_up(this, &req, pac);
  }

  co_unsigned32_t
  upInd(co_sdo_req& req) const noexcept {
    return co_sub_up_ind(this, &req);
  }

 protected:
  ~COSub() = default;
};

/**
 * A CANopen CANopen sub-object download indication callback wrapper.
 *
 * @see co_sub_dn_ind_t
 */
template <co_unsigned16_t N>
struct COSubDnInd {
  typedef co_unsigned32_t (*type)(COSub* sub, COVal<N>& val, void* data);

  template <type M>
  static co_unsigned32_t
  function(COSub* sub, co_sdo_req* req, void* data) noexcept {
    co_unsigned32_t ac = 0;

    COVal<N> val;
    if (co_sdo_req_dn_val(req, N, &val, &ac) == -1) return ac;

    if ((ac = sub->chkVal(val))) return ac;

    if ((ac = (*M)(sub, val, data))) return ac;

    sub->dn(val);
    return ac;
  }
};

/**
 * A CANopen CANopen sub-object upload indication callback wrapper.
 *
 * @see co_sub_up_ind_t
 */
template <co_unsigned16_t N>
struct COSubUpInd {
  typedef co_unsigned32_t (*type)(const COSub* sub, COVal<N>& val, void* data);

  template <type M>
  static co_unsigned32_t
  function(const COSub* sub, co_sdo_req* req, void* data) noexcept {
    co_unsigned32_t ac = 0;

    COVal<N> val = sub->getVal<N>();

    if ((ac = (*M)(sub, val, data))) return ac;

    co_sdo_req_up_val(req, N, &val, &ac);
    return ac;
  }
};

}  // namespace lely

#endif  // !LELY_CO_OBJ_HPP_
