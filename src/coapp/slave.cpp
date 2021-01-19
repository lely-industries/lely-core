/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the CANopen slave.
 *
 * @see lely/coapp/slave.hpp
 *
 * @copyright 2018-2021 Lely Industries N.V.
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

#include "coapp.hpp"

#if !LELY_NO_COAPP_SLAVE

#include <lely/co/dev.h>
#include <lely/co/nmt.h>
#include <lely/co/obj.h>
#include <lely/co/sdo.h>
#include <lely/coapp/slave.hpp>

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <cassert>

namespace lely {

namespace canopen {

/// The internal implementation of the CANopen slave.
struct BasicSlave::Impl_ {
  Impl_(BasicSlave* self, co_nmt_t* nmt);

  uint8_t
  netid() const noexcept {
    return co_dev_get_netid(self->dev());
  }

  uint8_t
  id() const noexcept {
    return co_dev_get_id(self->dev());
  }

#if !LELY_NO_CO_NG
  void OnLgInd(co_nmt_t* nmt, int state) noexcept;
#endif

  static constexpr uint32_t Key(uint16_t idx, uint8_t subidx) noexcept;
  static uint32_t Key(const co_sub_t* sub) noexcept;

  BasicSlave* self;

  ::std::map<uint32_t, ::std::function<uint32_t(co_sub_t*, co_sdo_req*)>>
      dn_ind;
  ::std::map<uint32_t, ::std::function<uint32_t(const co_sub_t*, co_sdo_req*)>>
      up_ind;

  ::std::function<void(bool)> on_life_guarding;
};

BasicSlave::BasicSlave(ev_exec_t* exec, io::TimerBase& timer,
                       io::CanChannelBase& chan, const ::std::string& dcf_txt,
                       const ::std::string& dcf_bin, uint8_t id)
    : Node(exec, timer, chan, dcf_txt, dcf_bin, id),
      impl_(new Impl_(this, Node::nmt())) {}

BasicSlave::~BasicSlave() = default;

template <class T>
typename ::std::enable_if<is_canopen<T>::value>::type
BasicSlave::OnRead(uint16_t idx, uint8_t subidx,
                   ::std::function<OnReadSignature<T>> ind) {
  ::std::error_code ec;
  OnRead<T>(idx, subidx, ::std::move(ind), ec);
  if (ec) throw SdoError(impl_->id(), idx, subidx, ec);
}

namespace {

template <class T, class F>
uint32_t
OnUpInd(const co_sub_t* sub, co_sdo_req* req, const F& ind) noexcept {
  using traits = canopen_traits<T>;
  using c_type = typename traits::c_type;

  assert(co_sub_get_type(sub) == traits::index);

  auto pval = static_cast<const c_type*>(co_sub_get_val(sub));
  if (!pval) return CO_SDO_AC_NO_DATA;

  uint32_t ac = 0;

  auto value = traits::from_c_type(*pval);
  auto ec =
      ind(co_obj_get_idx(co_sub_get_obj(sub)), co_sub_get_subidx(sub), value);
  ac = static_cast<uint32_t>(ec.value());

  if (!ac) {
    auto val = traits::to_c_type(value, ec);
    ac = static_cast<uint32_t>(ec.value());
    if (!ac) co_sdo_req_up_val(req, traits::index, &val, &ac);
    traits::destroy(val);
  }

  return ac;
}

}  // namespace

template <class T>
typename ::std::enable_if<is_canopen<T>::value>::type
BasicSlave::OnRead(uint16_t idx, uint8_t subidx,
                   ::std::function<OnReadSignature<T>> ind,
                   ::std::error_code& ec) {
  using traits = canopen_traits<T>;

  auto obj = co_dev_find_obj(dev(), idx);
  if (!obj) {
    ec = SdoErrc::NO_OBJ;
    return;
  }

  auto sub = co_obj_find_sub(obj, subidx);
  if (!sub) {
    ec = SdoErrc::NO_SUB;
    return;
  }

  if (!is_canopen_same(traits::index, co_sub_get_type(sub))) {
    ec = SdoErrc::TYPE_LEN;
    return;
  }
  auto key = Impl_::Key(sub);
  if (ind) {
    impl_->up_ind[key] = [ind](const co_sub_t* sub, co_sdo_req* req) noexcept {
      return OnUpInd<T>(sub, req, ind);
    };
    co_sub_set_up_ind(
        sub,
        [](const co_sub_t* sub, co_sdo_req* req,
           void* data) noexcept -> uint32_t {
          auto self = static_cast<Impl_*>(data);
          auto it = self->up_ind.find(self->Key(sub));
          if (it == self->up_ind.end()) return 0;
          return it->second(sub, req);
        },
        impl_.get());
  } else {
    if (impl_->up_ind.erase(key)) co_sub_set_up_ind(sub, nullptr, nullptr);
  }
}

template <class T>
typename ::std::enable_if<is_canopen<T>::value>::type
BasicSlave::OnRead(uint16_t idx, ::std::function<OnReadSignature<T>> ind) {
  uint8_t n = (*this)[idx][0];
  for (uint8_t i = 1; i <= n; i++) OnRead<T>(idx, i, ind);
}

template <class T>
typename ::std::enable_if<is_canopen<T>::value>::type
BasicSlave::OnRead(uint16_t idx, ::std::function<OnReadSignature<T>> ind,
                   ::std::error_code& ec) {
  uint8_t n = (*this)[idx][0].Get<uint8_t>(ec);
  for (uint8_t i = 1; i <= n && !ec; i++) OnRead<T>(idx, i, ind, ec);
}

template <class T>
typename ::std::enable_if<is_canopen<T>::value>::type
BasicSlave::OnWrite(uint16_t idx, uint8_t subidx,
                    ::std::function<OnWriteSignature<T>> ind) {
  ::std::error_code ec;
  OnWrite<T>(idx, subidx, ::std::move(ind), ec);
  if (ec) throw SdoError(impl_->id(), idx, subidx, ec);
}

namespace {

template <class T, class F>
typename ::std::enable_if<is_canopen_basic<T>::value, uint32_t>::type
OnDnInd(co_sub_t* sub, co_sdo_req* req, const F& ind) noexcept {
  using traits = canopen_traits<T>;
  using c_type = typename traits::c_type;

  assert(co_sub_get_type(sub) == traits::index);

  uint32_t ac = 0;
  auto val = c_type();

  if (co_sdo_req_dn_val(req, traits::index, &val, &ac) == -1) return ac;

  if (!(ac = co_sub_chk_val(sub, traits::index, &val))) {
    auto pval = static_cast<const c_type*>(co_sub_get_val(sub));
    auto ec = ind(co_obj_get_idx(co_sub_get_obj(sub)), co_sub_get_subidx(sub),
                  val, *pval);
    ac = static_cast<uint32_t>(ec.value());
  }

  if (!ac) co_sub_dn(sub, &val);

  traits::destroy(val);
  return ac;
}

template <class T, class F>
typename ::std::enable_if<!is_canopen_basic<T>::value, uint32_t>::type
OnDnInd(co_sub_t* sub, co_sdo_req* req, const F& ind) noexcept {
  using traits = canopen_traits<T>;
  using c_type = typename traits::c_type;

  assert(co_sub_get_type(sub) == traits::index);

  uint32_t ac = 0;
  auto val = c_type();

  if (co_sdo_req_dn_val(req, traits::index, &val, &ac) == -1) return ac;

  if (!(ac = co_sub_chk_val(sub, traits::index, &val))) {
    auto value = traits::from_c_type(val);
    auto ec =
        ind(co_obj_get_idx(co_sub_get_obj(sub)), co_sub_get_subidx(sub), value);
    if (!ec) {
      traits::destroy(val);
      val = traits::to_c_type(value, ec);
    }
    ac = static_cast<uint32_t>(ec.value());
  }

  if (!ac) co_sub_dn(sub, &val);

  traits::destroy(val);
  return ac;
}

}  // namespace

template <class T>
typename ::std::enable_if<is_canopen<T>::value>::type
BasicSlave::OnWrite(uint16_t idx, uint8_t subidx,
                    ::std::function<OnWriteSignature<T>> ind,
                    ::std::error_code& ec) {
  using traits = canopen_traits<T>;

  auto obj = co_dev_find_obj(dev(), idx);
  if (!obj) {
    ec = SdoErrc::NO_OBJ;
    return;
  }

  auto sub = co_obj_find_sub(obj, subidx);
  if (!sub) {
    ec = SdoErrc::NO_SUB;
    return;
  }

  if (!is_canopen_same(traits::index, co_sub_get_type(sub))) {
    ec = SdoErrc::TYPE_LEN;
    return;
  }
  auto key = Impl_::Key(sub);
  if (ind) {
    impl_->dn_ind[key] = [ind](co_sub_t* sub, co_sdo_req* req) noexcept {
      return OnDnInd<T>(sub, req, ind);
    };
    co_sub_set_dn_ind(
        sub,
        [](co_sub_t* sub, co_sdo_req* req, void* data) noexcept -> uint32_t {
          auto self = static_cast<Impl_*>(data);
          auto it = self->dn_ind.find(self->Key(sub));
          if (it == self->dn_ind.end()) return 0;
          return it->second(sub, req);
        },
        impl_.get());
  } else {
    if (impl_->dn_ind.erase(key)) co_sub_set_dn_ind(sub, nullptr, nullptr);
  }
}

template <class T>
typename ::std::enable_if<is_canopen<T>::value>::type
BasicSlave::OnWrite(uint16_t idx, ::std::function<OnWriteSignature<T>> ind) {
  uint8_t n = (*this)[idx][0];
  for (uint8_t i = 1; i <= n; i++) OnWrite<T>(idx, i, ind);
}

template <class T>
typename ::std::enable_if<is_canopen<T>::value>::type
BasicSlave::OnWrite(uint16_t idx, ::std::function<OnWriteSignature<T>> ind,
                    ::std::error_code& ec) {
  uint8_t n = (*this)[idx][0].Get<uint8_t>(ec);
  for (uint8_t i = 1; i <= n && !ec; i++) OnWrite<T>(idx, i, ind, ec);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// BOOLEAN
template void BasicSlave::OnRead<bool>(uint16_t, uint8_t,
                                       ::std::function<OnReadSignature<bool>>);
template void BasicSlave::OnRead<bool>(uint16_t, uint8_t,
                                       ::std::function<OnReadSignature<bool>>,
                                       ::std::error_code&);
template void BasicSlave::OnRead<bool>(uint16_t,
                                       ::std::function<OnReadSignature<bool>>);
template void BasicSlave::OnRead<bool>(uint16_t,
                                       ::std::function<OnReadSignature<bool>>,
                                       ::std::error_code&);
template void BasicSlave::OnWrite<bool>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<bool>>);
template void BasicSlave::OnWrite<bool>(uint16_t, uint8_t,
                                        ::std::function<OnWriteSignature<bool>>,
                                        ::std::error_code&);
template void BasicSlave::OnWrite<bool>(
    uint16_t, ::std::function<OnWriteSignature<bool>>);
template void BasicSlave::OnWrite<bool>(uint16_t,
                                        ::std::function<OnWriteSignature<bool>>,
                                        ::std::error_code&);

// INTEGER8
template void BasicSlave::OnRead<int8_t>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<int8_t>>);
template void BasicSlave::OnRead<int8_t>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<int8_t>>,
    ::std::error_code&);
template void BasicSlave::OnRead<int8_t>(
    uint16_t, ::std::function<OnReadSignature<int8_t>>);
template void BasicSlave::OnRead<int8_t>(
    uint16_t, ::std::function<OnReadSignature<int8_t>>, ::std::error_code&);
template void BasicSlave::OnWrite<int8_t>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<int8_t>>);
template void BasicSlave::OnWrite<int8_t>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<int8_t>>,
    ::std::error_code&);
template void BasicSlave::OnWrite<int8_t>(
    uint16_t, ::std::function<OnWriteSignature<int8_t>>);
template void BasicSlave::OnWrite<int8_t>(
    uint16_t, ::std::function<OnWriteSignature<int8_t>>, ::std::error_code&);

// INTEGER16
template void BasicSlave::OnRead<int16_t>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<int16_t>>);
template void BasicSlave::OnRead<int16_t>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<int16_t>>,
    ::std::error_code&);
template void BasicSlave::OnRead<int16_t>(
    uint16_t, ::std::function<OnReadSignature<int16_t>>);
template void BasicSlave::OnRead<int16_t>(
    uint16_t, ::std::function<OnReadSignature<int16_t>>, ::std::error_code&);
template void BasicSlave::OnWrite<int16_t>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<int16_t>>);
template void BasicSlave::OnWrite<int16_t>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<int16_t>>,
    ::std::error_code&);
template void BasicSlave::OnWrite<int16_t>(
    uint16_t, ::std::function<OnWriteSignature<int16_t>>);
template void BasicSlave::OnWrite<int16_t>(
    uint16_t, ::std::function<OnWriteSignature<int16_t>>, ::std::error_code&);

// INTEGER32
template void BasicSlave::OnRead<int32_t>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<int32_t>>);
template void BasicSlave::OnRead<int32_t>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<int32_t>>,
    ::std::error_code&);
template void BasicSlave::OnRead<int32_t>(
    uint16_t, ::std::function<OnReadSignature<int32_t>>);
template void BasicSlave::OnRead<int32_t>(
    uint16_t, ::std::function<OnReadSignature<int32_t>>, ::std::error_code&);
template void BasicSlave::OnWrite<int32_t>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<int32_t>>);
template void BasicSlave::OnWrite<int32_t>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<int32_t>>,
    ::std::error_code&);
template void BasicSlave::OnWrite<int32_t>(
    uint16_t, ::std::function<OnWriteSignature<int32_t>>);
template void BasicSlave::OnWrite<int32_t>(
    uint16_t, ::std::function<OnWriteSignature<int32_t>>, ::std::error_code&);

// UNSIGNED8
template void BasicSlave::OnRead<uint8_t>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<uint8_t>>);
template void BasicSlave::OnRead<uint8_t>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<uint8_t>>,
    ::std::error_code&);
template void BasicSlave::OnRead<uint8_t>(
    uint16_t, ::std::function<OnReadSignature<uint8_t>>);
template void BasicSlave::OnRead<uint8_t>(
    uint16_t, ::std::function<OnReadSignature<uint8_t>>, ::std::error_code&);
template void BasicSlave::OnWrite<uint8_t>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<uint8_t>>);
template void BasicSlave::OnWrite<uint8_t>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<uint8_t>>,
    ::std::error_code&);
template void BasicSlave::OnWrite<uint8_t>(
    uint16_t, ::std::function<OnWriteSignature<uint8_t>>);
template void BasicSlave::OnWrite<uint8_t>(
    uint16_t, ::std::function<OnWriteSignature<uint8_t>>, ::std::error_code&);

// UNSIGNED16
template void BasicSlave::OnRead<uint16_t>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<uint16_t>>);
template void BasicSlave::OnRead<uint16_t>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<uint16_t>>,
    ::std::error_code&);
template void BasicSlave::OnRead<uint16_t>(
    uint16_t, ::std::function<OnReadSignature<uint16_t>>);
template void BasicSlave::OnRead<uint16_t>(
    uint16_t, ::std::function<OnReadSignature<uint16_t>>, ::std::error_code&);
template void BasicSlave::OnWrite<uint16_t>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<uint16_t>>);
template void BasicSlave::OnWrite<uint16_t>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<uint16_t>>,
    ::std::error_code&);
template void BasicSlave::OnWrite<uint16_t>(
    uint16_t, ::std::function<OnWriteSignature<uint16_t>>);
template void BasicSlave::OnWrite<uint16_t>(
    uint16_t, ::std::function<OnWriteSignature<uint16_t>>, ::std::error_code&);

// UNSIGNED32
template void BasicSlave::OnRead<uint32_t>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<uint32_t>>);
template void BasicSlave::OnRead<uint32_t>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<uint32_t>>,
    ::std::error_code&);
template void BasicSlave::OnRead<uint32_t>(
    uint16_t, ::std::function<OnReadSignature<uint32_t>>);
template void BasicSlave::OnRead<uint32_t>(
    uint16_t, ::std::function<OnReadSignature<uint32_t>>, ::std::error_code&);
template void BasicSlave::OnWrite<uint32_t>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<uint32_t>>);
template void BasicSlave::OnWrite<uint32_t>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<uint32_t>>,
    ::std::error_code&);
template void BasicSlave::OnWrite<uint32_t>(
    uint16_t, ::std::function<OnWriteSignature<uint32_t>>);
template void BasicSlave::OnWrite<uint32_t>(
    uint16_t, ::std::function<OnWriteSignature<uint32_t>>, ::std::error_code&);

// REAL32
template void BasicSlave::OnRead<float>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<float>>);
template void BasicSlave::OnRead<float>(uint16_t, uint8_t,
                                        ::std::function<OnReadSignature<float>>,
                                        ::std::error_code&);
template void BasicSlave::OnRead<float>(
    uint16_t, ::std::function<OnReadSignature<float>>);
template void BasicSlave::OnRead<float>(uint16_t,
                                        ::std::function<OnReadSignature<float>>,
                                        ::std::error_code&);
template void BasicSlave::OnWrite<float>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<float>>);
template void BasicSlave::OnWrite<float>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<float>>,
    ::std::error_code&);
template void BasicSlave::OnWrite<float>(
    uint16_t, ::std::function<OnWriteSignature<float>>);
template void BasicSlave::OnWrite<float>(
    uint16_t, ::std::function<OnWriteSignature<float>>, ::std::error_code&);

// VISIBLE_STRING
template void BasicSlave::OnRead<::std::string>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<::std::string>>);
template void BasicSlave::OnRead<::std::string>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<::std::string>>,
    ::std::error_code&);
template void BasicSlave::OnRead<::std::string>(
    uint16_t, ::std::function<OnReadSignature<::std::string>>);
template void BasicSlave::OnRead<::std::string>(
    uint16_t, ::std::function<OnReadSignature<::std::string>>,
    ::std::error_code&);
template void BasicSlave::OnWrite<::std::string>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<::std::string>>);
template void BasicSlave::OnWrite<::std::string>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<::std::string>>,
    ::std::error_code&);
template void BasicSlave::OnWrite<::std::string>(
    uint16_t, ::std::function<OnWriteSignature<::std::string>>);
template void BasicSlave::OnWrite<::std::string>(
    uint16_t, ::std::function<OnWriteSignature<::std::string>>,
    ::std::error_code&);

// OCTET_STRING
template void BasicSlave::OnRead<::std::vector<uint8_t>>(
    uint16_t, uint8_t,
    ::std::function<OnReadSignature<::std::vector<uint8_t>>>);
template void BasicSlave::OnRead<::std::vector<uint8_t>>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<::std::vector<uint8_t>>>,
    ::std::error_code&);
template void BasicSlave::OnRead<::std::vector<uint8_t>>(
    uint16_t, ::std::function<OnReadSignature<::std::vector<uint8_t>>>);
template void BasicSlave::OnRead<::std::vector<uint8_t>>(
    uint16_t, ::std::function<OnReadSignature<::std::vector<uint8_t>>>,
    ::std::error_code&);
template void BasicSlave::OnWrite<::std::vector<uint8_t>>(
    uint16_t, uint8_t,
    ::std::function<OnWriteSignature<::std::vector<uint8_t>>>);
template void BasicSlave::OnWrite<::std::vector<uint8_t>>(
    uint16_t, uint8_t,
    ::std::function<OnWriteSignature<::std::vector<uint8_t>>>,
    ::std::error_code&);
template void BasicSlave::OnWrite<::std::vector<uint8_t>>(
    uint16_t, ::std::function<OnWriteSignature<::std::vector<uint8_t>>>);
template void BasicSlave::OnWrite<::std::vector<uint8_t>>(
    uint16_t, ::std::function<OnWriteSignature<::std::vector<uint8_t>>>,
    ::std::error_code&);

// UNICODE_STRING
template void BasicSlave::OnRead<::std::basic_string<char16_t>>(
    uint16_t, uint8_t,
    ::std::function<OnReadSignature<::std::basic_string<char16_t>>>);
template void BasicSlave::OnRead<::std::basic_string<char16_t>>(
    uint16_t, uint8_t,
    ::std::function<OnReadSignature<::std::basic_string<char16_t>>>,
    ::std::error_code&);
template void BasicSlave::OnRead<::std::basic_string<char16_t>>(
    uint16_t, ::std::function<OnReadSignature<::std::basic_string<char16_t>>>);
template void BasicSlave::OnRead<::std::basic_string<char16_t>>(
    uint16_t, ::std::function<OnReadSignature<::std::basic_string<char16_t>>>,
    ::std::error_code&);
template void BasicSlave::OnWrite<::std::basic_string<char16_t>>(
    uint16_t, uint8_t,
    ::std::function<OnWriteSignature<::std::basic_string<char16_t>>>);
template void BasicSlave::OnWrite<::std::basic_string<char16_t>>(
    uint16_t, uint8_t,
    ::std::function<OnWriteSignature<::std::basic_string<char16_t>>>,
    ::std::error_code&);
template void BasicSlave::OnWrite<::std::basic_string<char16_t>>(
    uint16_t, ::std::function<OnWriteSignature<::std::basic_string<char16_t>>>);
template void BasicSlave::OnWrite<::std::basic_string<char16_t>>(
    uint16_t, ::std::function<OnWriteSignature<::std::basic_string<char16_t>>>,
    ::std::error_code&);

// TIME_OF_DAY
// TIME_DIFFERENCE
// DOMAIN
// INTEGER24

// REAL64
template void BasicSlave::OnRead<double>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<double>>);
template void BasicSlave::OnRead<double>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<double>>,
    ::std::error_code&);
template void BasicSlave::OnRead<double>(
    uint16_t, ::std::function<OnReadSignature<double>>);
template void BasicSlave::OnRead<double>(
    uint16_t, ::std::function<OnReadSignature<double>>, ::std::error_code&);
template void BasicSlave::OnWrite<double>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<double>>);
template void BasicSlave::OnWrite<double>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<double>>,
    ::std::error_code&);
template void BasicSlave::OnWrite<double>(
    uint16_t, ::std::function<OnWriteSignature<double>>);
template void BasicSlave::OnWrite<double>(
    uint16_t, ::std::function<OnWriteSignature<double>>, ::std::error_code&);

// INTEGER40
// INTEGER48
// INTEGER56

// INTEGER64
template void BasicSlave::OnRead<int64_t>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<int64_t>>);
template void BasicSlave::OnRead<int64_t>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<int64_t>>,
    ::std::error_code&);
template void BasicSlave::OnRead<int64_t>(
    uint16_t, ::std::function<OnReadSignature<int64_t>>);
template void BasicSlave::OnRead<int64_t>(
    uint16_t, ::std::function<OnReadSignature<int64_t>>, ::std::error_code&);
template void BasicSlave::OnWrite<int64_t>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<int64_t>>);
template void BasicSlave::OnWrite<int64_t>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<int64_t>>,
    ::std::error_code&);
template void BasicSlave::OnWrite<int64_t>(
    uint16_t, ::std::function<OnWriteSignature<int64_t>>);
template void BasicSlave::OnWrite<int64_t>(
    uint16_t, ::std::function<OnWriteSignature<int64_t>>, ::std::error_code&);

// UNSIGNED24
// UNSIGNED40
// UNSIGNED48
// UNSIGNED56

// UNSIGNED64
template void BasicSlave::OnRead<uint64_t>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<uint64_t>>);
template void BasicSlave::OnRead<uint64_t>(
    uint16_t, uint8_t, ::std::function<OnReadSignature<uint64_t>>,
    ::std::error_code&);
template void BasicSlave::OnRead<uint64_t>(
    uint16_t, ::std::function<OnReadSignature<uint64_t>>);
template void BasicSlave::OnRead<uint64_t>(
    uint16_t, ::std::function<OnReadSignature<uint64_t>>, ::std::error_code&);
template void BasicSlave::OnWrite<uint64_t>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<uint64_t>>);
template void BasicSlave::OnWrite<uint64_t>(
    uint16_t, uint8_t, ::std::function<OnWriteSignature<uint64_t>>,
    ::std::error_code&);
template void BasicSlave::OnWrite<uint64_t>(
    uint16_t, ::std::function<OnWriteSignature<uint64_t>>);
template void BasicSlave::OnWrite<uint64_t>(
    uint16_t, ::std::function<OnWriteSignature<uint64_t>>, ::std::error_code&);

#endif  // DOXYGEN_SHOULD_SKIP_THIS

void
BasicSlave::OnLifeGuarding(::std::function<void(bool)> on_life_guarding) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_life_guarding = on_life_guarding;
}

BasicSlave::Impl_::Impl_(BasicSlave* self_, co_nmt_t* nmt) : self(self_) {
#if LELY_NO_CO_NG
  (void)nmt;
#else
  co_nmt_set_lg_ind(
      nmt,
      [](co_nmt_t* nmt, int state, void* data) noexcept {
        static_cast<Impl_*>(data)->OnLgInd(nmt, state);
      },
      this);
#endif
}

#if !LELY_NO_CO_NG
void
BasicSlave::Impl_::OnLgInd(co_nmt_t* nmt, int state) noexcept {
  // Invoke the default behavior before notifying the implementation.
  co_nmt_on_lg(nmt, state);
  // Notify the implementation.
  bool occurred = state == CO_NMT_EC_OCCURRED;
  self->OnLifeGuarding(occurred);
  if (on_life_guarding) {
    auto f = on_life_guarding;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(occurred);
  }
}
#endif

constexpr uint32_t
BasicSlave::Impl_::Key(uint16_t idx, uint8_t subidx) noexcept {
  return (uint32_t(idx) << 8) | subidx;
}

uint32_t
BasicSlave::Impl_::Key(const co_sub_t* sub) noexcept {
  assert(sub);

  return Key(co_obj_get_idx(co_sub_get_obj(sub)), co_sub_get_subidx(sub));
}

}  // namespace canopen

}  // namespace lely

#endif  // !LELY_NO_COAPP_SLAVE
