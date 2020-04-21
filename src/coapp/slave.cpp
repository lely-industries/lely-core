/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the CANopen slave.
 *
 * @see lely/coapp/slave.hpp
 *
 * @copyright 2018-2019 Lely Industries N.V.
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

#include <lely/coapp/slave.hpp>

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <cassert>

#include <lely/co/dev.hpp>
#include <lely/co/nmt.hpp>
#include <lely/co/obj.hpp>

namespace lely {

namespace canopen {

/// The internal implementation of the CANopen slave.
struct BasicSlave::Impl_ {
  Impl_(BasicSlave* self, CONMT* nmt);

  uint8_t
  netid() const noexcept {
    return self->dev()->getNetid();
  }

  uint8_t
  id() const noexcept {
    return self->dev()->getId();
  }

  void OnLgInd(CONMT* nmt, int state) noexcept;

  template <uint16_t N>
  uint32_t OnDnInd(COSub* sub, COVal<N>& val) noexcept;

  template <uint16_t N>
  uint32_t OnUpInd(const COSub* sub, COVal<N>& val) noexcept;

  static constexpr uint32_t Key(uint16_t idx, uint8_t subidx) noexcept;
  static uint32_t Key(const COSub* sub) noexcept;

  BasicSlave* self;

  ::std::map<uint32_t, ::std::function<uint32_t(COSub*, void*)>> dn_ind;
  ::std::map<uint32_t, ::std::function<uint32_t(const COSub*, void*)>> up_ind;

  ::std::function<void(bool)> on_life_guarding;
};

BasicSlave::BasicSlave(ev_exec_t* exec, io::TimerBase& timer,
                       io::CanChannelBase& chan, const ::std::string& dcf_txt,
                       const ::std::string& dcf_bin, uint8_t id)
    : Node(exec, timer, chan, dcf_txt, dcf_bin, id),
      impl_(new Impl_(this, Node::nmt())) {}

BasicSlave::~BasicSlave() = default;

void
BasicSlave::OnLifeGuarding(::std::function<void(bool)> on_life_guarding) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_life_guarding = on_life_guarding;
}

template <class T>
typename ::std::enable_if<detail::is_canopen_type<T>::value>::type
BasicSlave::OnRead(uint16_t idx, uint8_t subidx,
                   ::std::function<OnReadSignature<T>> ind) {
  ::std::error_code ec;
  OnRead<T>(idx, subidx, ::std::move(ind), ec);
  if (ec) throw SdoError(impl_->id(), idx, subidx, ec);
}

namespace {

template <class T, class F, uint16_t N = co_type_traits_T<T>::index>
typename ::std::enable_if<detail::is_canopen_basic<T>::value,
                          ::std::error_code>::type
OnUpInd(const COSub* sub, COVal<N>& val, const F& ind) {
  assert(sub);

  try {
    return ind(sub->getObj()->getIdx(), sub->getSubidx(), val);
  } catch (...) {
    return SdoErrc::ERROR;
  }
}

template <class T, class F, uint16_t N = co_type_traits_T<T>::index>
typename ::std::enable_if<detail::is_canopen_array<T>::value,
                          ::std::error_code>::type
OnUpInd(const COSub* sub, COVal<N>& val, const F& ind) {
  assert(sub);

  try {
    T value = val;
    auto ec = ind(sub->getObj()->getIdx(), sub->getSubidx(), value);
    if (!ec) val = ::std::move(value);
    return ec;
  } catch (...) {
    return SdoErrc::ERROR;
  }
}

}  // namespace

template <class T>
typename ::std::enable_if<detail::is_canopen_type<T>::value>::type
BasicSlave::OnRead(uint16_t idx, uint8_t subidx,
                   ::std::function<OnReadSignature<T>> ind,
                   ::std::error_code& ec) {
  constexpr auto N = co_type_traits_T<T>::index;

  auto obj = dev()->find(idx);
  if (!obj) {
    ec = SdoErrc::NO_OBJ;
    return;
  }

  auto sub = obj->find(subidx);
  if (!sub) {
    ec = SdoErrc::NO_SUB;
    return;
  }

  if (!detail::is_canopen_same(N, sub->getType())) {
    ec = SdoErrc::TYPE_LEN;
    return;
  }

  auto key = Impl_::Key(sub);
  if (ind) {
    impl_->up_ind[key] = [this, ind](const COSub* sub, void* p) {
      auto ec = OnUpInd<T>(sub, *static_cast<COVal<N>*>(p), ind);
      return static_cast<uint32_t>(ec.value());
    };
    sub->setUpInd<N, Impl_, &Impl_::OnUpInd>(impl_.get());
  } else {
    if (impl_->up_ind.erase(key)) sub->setUpInd(nullptr, nullptr);
  }
}

template <class T>
typename ::std::enable_if<detail::is_canopen_type<T>::value>::type
BasicSlave::OnRead(uint16_t idx, ::std::function<OnReadSignature<T>> ind) {
  uint8_t n = (*this)[idx][0];
  for (uint8_t i = 1; i <= n; i++) OnRead<T>(idx, i, ind);
}

template <class T>
typename ::std::enable_if<detail::is_canopen_type<T>::value>::type
BasicSlave::OnRead(uint16_t idx, ::std::function<OnReadSignature<T>> ind,
                   ::std::error_code& ec) {
  uint8_t n = (*this)[idx][0].Get<uint8_t>(ec);
  for (uint8_t i = 1; i <= n && !ec; i++) OnRead<T>(idx, i, ind, ec);
}

template <class T>
typename ::std::enable_if<detail::is_canopen_type<T>::value>::type
BasicSlave::OnWrite(uint16_t idx, uint8_t subidx,
                    ::std::function<OnWriteSignature<T>> ind) {
  ::std::error_code ec;
  OnWrite<T>(idx, subidx, ::std::move(ind), ec);
  if (ec) throw SdoError(impl_->id(), idx, subidx, ec);
}

namespace {

template <class T, class F, uint16_t N = co_type_traits_T<T>::index>
typename ::std::enable_if<detail::is_canopen_basic<T>::value,
                          ::std::error_code>::type
OnDnInd(COSub* sub, COVal<N>& val, const F& ind) {
  assert(sub);

  try {
    return ind(sub->getObj()->getIdx(), sub->getSubidx(), val,
               sub->getVal<N>());
  } catch (...) {
    return SdoErrc::ERROR;
  }
}

template <class T, class F, uint16_t N = co_type_traits_T<T>::index>
typename ::std::enable_if<detail::is_canopen_array<T>::value,
                          ::std::error_code>::type
OnDnInd(COSub* sub, COVal<N>& val, const F& ind) {
  assert(sub);

  try {
    T value = val;
    auto ec = ind(sub->getObj()->getIdx(), sub->getSubidx(), value);
    if (!ec) val = ::std::move(value);
    return ec;
  } catch (...) {
    return SdoErrc::ERROR;
  }
}

}  // namespace

template <class T>
typename ::std::enable_if<detail::is_canopen_type<T>::value>::type
BasicSlave::OnWrite(uint16_t idx, uint8_t subidx,
                    ::std::function<OnWriteSignature<T>> ind,
                    ::std::error_code& ec) {
  constexpr auto N = co_type_traits_T<T>::index;

  auto obj = dev()->find(idx);
  if (!obj) {
    ec = SdoErrc::NO_OBJ;
    return;
  }

  auto sub = obj->find(subidx);
  if (!sub) {
    ec = SdoErrc::NO_SUB;
    return;
  }

  if (!detail::is_canopen_same(N, sub->getType())) {
    ec = SdoErrc::TYPE_LEN;
    return;
  }

  auto key = Impl_::Key(sub);
  if (ind) {
    impl_->dn_ind[key] = [this, ind](COSub* sub, void* p) {
      auto ec = OnDnInd<T>(sub, *static_cast<COVal<N>*>(p), ind);
      return static_cast<uint32_t>(ec.value());
    };
    sub->setDnInd<N, Impl_, &Impl_::OnDnInd>(impl_.get());
  } else {
    if (impl_->dn_ind.erase(key)) sub->setDnInd(nullptr, nullptr);
  }
}

template <class T>
typename ::std::enable_if<detail::is_canopen_type<T>::value>::type
BasicSlave::OnWrite(uint16_t idx, ::std::function<OnWriteSignature<T>> ind) {
  uint8_t n = (*this)[idx][0];
  for (uint8_t i = 1; i <= n; i++) OnWrite<T>(idx, i, ind);
}

template <class T>
typename ::std::enable_if<detail::is_canopen_type<T>::value>::type
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

BasicSlave::Impl_::Impl_(BasicSlave* self_, CONMT* nmt) : self(self_) {
  nmt->setLgInd<Impl_, &Impl_::OnLgInd>(this);
}

void
BasicSlave::Impl_::OnLgInd(CONMT* nmt, int state) noexcept {
  // Invoke the default behavior before notifying the implementation.
  nmt->onLg(state);
  // Notify the implementation.
  bool occurred = state == CO_NMT_EC_OCCURRED;
  self->OnLifeGuarding(occurred);
  if (on_life_guarding) {
    auto f = on_life_guarding;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(occurred);
  }
}

template <uint16_t N>
uint32_t
BasicSlave::Impl_::OnDnInd(COSub* sub, COVal<N>& val) noexcept {
  auto it = dn_ind.find(Key(sub));
  if (it == dn_ind.end()) return 0;
  return it->second(sub, static_cast<void*>(&val));
}

template <uint16_t N>
uint32_t
BasicSlave::Impl_::OnUpInd(const COSub* sub, COVal<N>& val) noexcept {
  auto it = up_ind.find(Key(sub));
  if (it == up_ind.end()) return 0;
  return it->second(sub, static_cast<void*>(&val));
}

constexpr uint32_t
BasicSlave::Impl_::Key(uint16_t idx, uint8_t subidx) noexcept {
  return (uint32_t(idx) << 8) | subidx;
}

uint32_t
BasicSlave::Impl_::Key(const COSub* sub) noexcept {
  assert(sub);

  return Key(sub->getObj()->getIdx(), sub->getSubidx());
}

}  // namespace canopen

}  // namespace lely

#endif  // !LELY_NO_COAPP_SLAVE
