/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the CANopen device description.
 *
 * @see lely/coapp/device.hpp
 *
 * @copyright 2018-2020 Lely Industries N.V.
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
#include <lely/coapp/device.hpp>
#include <lely/util/error.hpp>

#include <lely/co/csdo.hpp>
#include <lely/co/dcf.hpp>
#include <lely/co/obj.hpp>
#include <lely/co/pdo.h>

#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace lely {

namespace canopen {

/// The internal implementation of the CANopen device description.
struct Device::Impl_ : util::BasicLockable {
  Impl_(Device* self, const ::std::string& dcf_txt,
        const ::std::string& dcf_bin, uint8_t id, util::BasicLockable* mutex);
  virtual ~Impl_() = default;

  void
  lock() override {
    if (mutex) mutex->lock();
  }

  void
  unlock() override {
    if (mutex) mutex->unlock();
  }

  uint8_t
  netid() const noexcept {
    return dev->getNetid();
  }

  uint8_t
  id() const noexcept {
    return dev->getId();
  }

  void Set(uint16_t idx, uint8_t subidx, const ::std::string& value,
           ::std::error_code& ec) noexcept;

  void Set(uint16_t idx, uint8_t subidx, const ::std::vector<uint8_t>& value,
           ::std::error_code& ec) noexcept;

  void Set(uint16_t idx, uint8_t subidx,
           const ::std::basic_string<char16_t>& value,
           ::std::error_code& ec) noexcept;

  template <uint16_t N>
  void Set(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n,
           ::std::error_code& ec) noexcept;

  void OnWrite(uint16_t idx, uint8_t subidx);

  ::std::tuple<uint16_t, uint8_t>
  RpdoMapping(uint8_t id, uint16_t idx, uint8_t subidx,
              ::std::error_code& ec) const noexcept {
    auto it = rpdo_mapping.find((static_cast<uint32_t>(id) << 24) |
                                (static_cast<uint32_t>(idx) << 8) | subidx);
    if (it != rpdo_mapping.end()) {
      idx = (it->second >> 8) & 0xffff;
      subidx = it->second & 0xff;
    } else {
      ec = SdoErrc::NO_PDO;
      idx = 0;
      subidx = 0;
    }
    return ::std::make_tuple(idx, subidx);
  }

  ::std::tuple<uint8_t, uint16_t, uint8_t>
  RpdoMapping(uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
      noexcept {
    uint8_t id = 0;
    auto it = rpdo_mapping.find((static_cast<uint32_t>(idx) << 8) | subidx);
    if (it != rpdo_mapping.end()) {
      id = (it->second >> 24) & 0xff;
      idx = (it->second >> 8) & 0xffff;
      subidx = it->second & 0xff;
    } else {
      ec = SdoErrc::NO_PDO;
      idx = 0;
      subidx = 0;
    }
    return ::std::make_tuple(id, idx, subidx);
  }

  ::std::tuple<uint16_t, uint8_t>
  TpdoMapping(uint8_t id, uint16_t idx, uint8_t subidx,
              ::std::error_code& ec) const noexcept {
    auto it = tpdo_mapping.find((static_cast<uint32_t>(id) << 24) |
                                (static_cast<uint32_t>(idx) << 8) | subidx);
    if (it != tpdo_mapping.end()) {
      idx = (it->second >> 8) & 0xffff;
      subidx = it->second & 0xff;
    } else {
      ec = SdoErrc::NO_PDO;
      idx = 0;
      subidx = 0;
    }
    return ::std::make_tuple(idx, subidx);
  }

  Device* self;

  BasicLockable* mutex{nullptr};

  unique_c_ptr<CODev> dev;

  ::std::map<uint32_t, uint32_t> rpdo_mapping;
  ::std::map<uint32_t, uint32_t> tpdo_mapping;

  ::std::function<void(uint16_t, uint8_t)> on_write;
  ::std::function<void(uint8_t, uint16_t, uint8_t)> on_rpdo_write;
};

Device::Device(const ::std::string& dcf_txt, const ::std::string& dcf_bin,
               uint8_t id, util::BasicLockable* mutex)
    : impl_(new Impl_(this, dcf_txt, dcf_bin, id, mutex)) {}

Device::~Device() = default;

uint8_t
Device::netid() const noexcept {
  ::std::lock_guard<Impl_> lock(*impl_);

  return impl_->netid();
}

uint8_t
Device::id() const noexcept {
  ::std::lock_guard<Impl_> lock(*impl_);

  return impl_->id();
}

namespace {

void
OnDnCon(COCSDO*, uint16_t, uint8_t, uint32_t ac, void* data) noexcept {
  *static_cast<uint32_t*>(data) = ac;
}

template <class T>
void
OnUpCon(COCSDO*, uint16_t, uint8_t, uint32_t ac, T value, void* data) noexcept {
  auto* t = static_cast<decltype(::std::tie(ac, value))*>(data);
  *t = ::std::forward_as_tuple(ac, ::std::move(value));
}

}  // namespace

template <class T>
typename ::std::enable_if<detail::is_canopen_type<T>::value, T>::type
Device::Read(uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  T value(Read<T>(idx, subidx, ec));
  if (ec) throw_sdo_error(id(), idx, subidx, ec, "Read");
  return value;
}

template <class T>
typename ::std::enable_if<detail::is_canopen_type<T>::value, T>::type
Device::Read(uint16_t idx, uint8_t subidx, ::std::error_code& ec) const {
  uint32_t ac = 0;
  T value = T();
  auto t = ::std::tie(ac, value);

  ::std::lock_guard<Impl_> lock(*impl_);
  int errsv = get_errc();
  set_errc(0);
  if (!upReq<T, &OnUpCon<T>>(*impl_->dev, idx, subidx, &t)) {
    if (ac)
      ec = static_cast<SdoErrc>(ac);
    else
      ec.clear();
  } else {
    ec = util::make_error_code();
  }
  set_errc(errsv);
  return value;
}

template <class T>
typename ::std::enable_if<detail::is_canopen_basic<T>::value>::type
Device::Write(uint16_t idx, uint8_t subidx, T value) {
  ::std::error_code ec;
  Write(idx, subidx, value, ec);
  if (ec) throw_sdo_error(id(), idx, subidx, ec, "Write");
}

template <class T>
typename ::std::enable_if<detail::is_canopen_basic<T>::value>::type
Device::Write(uint16_t idx, uint8_t subidx, T value, ::std::error_code& ec) {
  constexpr auto N = co_type_traits_T<T>::index;
  uint32_t ac = 0;

  ::std::lock_guard<Impl_> lock(*impl_);
  int errsv = get_errc();
  set_errc(0);
  if (!dnReq<N>(*impl_->dev, idx, subidx, value, &OnDnCon, &ac)) {
    if (ac)
      ec = static_cast<SdoErrc>(ac);
    else
      ec.clear();
  } else {
    ec = util::make_error_code();
  }
  set_errc(errsv);
}

template <class T>
typename ::std::enable_if<detail::is_canopen_array<T>::value>::type
Device::Write(uint16_t idx, uint8_t subidx, const T& value) {
  ::std::error_code ec;
  Write(idx, subidx, value, ec);
  if (ec) throw_sdo_error(id(), idx, subidx, ec, "Write");
}

template <class T>
typename ::std::enable_if<detail::is_canopen_array<T>::value>::type
Device::Write(uint16_t idx, uint8_t subidx, const T& value,
              ::std::error_code& ec) {
  constexpr auto N = co_type_traits_T<T>::index;
  uint32_t ac = 0;

  ::std::lock_guard<Impl_> lock(*impl_);
  int errsv = get_errc();
  set_errc(0);
  if (!dnReq<N>(*impl_->dev, idx, subidx, value, &OnDnCon, &ac)) {
    if (ac)
      ec = static_cast<SdoErrc>(ac);
    else
      ec.clear();
  } else {
    ec = util::make_error_code();
  }
  set_errc(errsv);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// BOOLEAN
template bool Device::Read<bool>(uint16_t, uint8_t) const;
template bool Device::Read<bool>(uint16_t, uint8_t, ::std::error_code&) const;
template void Device::Write<bool>(uint16_t, uint8_t, bool);
template void Device::Write<bool>(uint16_t, uint8_t, bool, ::std::error_code&);

// INTEGER8
template int8_t Device::Read<int8_t>(uint16_t, uint8_t) const;
template int8_t Device::Read<int8_t>(uint16_t, uint8_t,
                                     ::std::error_code&) const;
template void Device::Write<int8_t>(uint16_t, uint8_t, int8_t);
template void Device::Write<int8_t>(uint16_t, uint8_t, int8_t,
                                    ::std::error_code&);

// INTEGER16
template int16_t Device::Read<int16_t>(uint16_t, uint8_t) const;
template int16_t Device::Read<int16_t>(uint16_t, uint8_t,
                                       ::std::error_code&) const;
template void Device::Write<int16_t>(uint16_t, uint8_t, int16_t);
template void Device::Write<int16_t>(uint16_t, uint8_t, int16_t,
                                     ::std::error_code&);

// INTEGER32
template int32_t Device::Read<int32_t>(uint16_t, uint8_t) const;
template int32_t Device::Read<int32_t>(uint16_t, uint8_t,
                                       ::std::error_code&) const;
template void Device::Write<int32_t>(uint16_t, uint8_t, int32_t);
template void Device::Write<int32_t>(uint16_t, uint8_t, int32_t,
                                     ::std::error_code&);

// UNSIGNED8
template uint8_t Device::Read<uint8_t>(uint16_t, uint8_t) const;
template uint8_t Device::Read<uint8_t>(uint16_t, uint8_t,
                                       ::std::error_code&) const;
template void Device::Write<uint8_t>(uint16_t, uint8_t, uint8_t);
template void Device::Write<uint8_t>(uint16_t, uint8_t, uint8_t,
                                     ::std::error_code&);

// UNSIGNED16
template uint16_t Device::Read<uint16_t>(uint16_t, uint8_t) const;
template uint16_t Device::Read<uint16_t>(uint16_t, uint8_t,
                                         ::std::error_code&) const;
template void Device::Write<uint16_t>(uint16_t, uint8_t, uint16_t);
template void Device::Write<uint16_t>(uint16_t, uint8_t, uint16_t,
                                      ::std::error_code&);

// UNSIGNED32
template uint32_t Device::Read<uint32_t>(uint16_t, uint8_t) const;
template uint32_t Device::Read<uint32_t>(uint16_t, uint8_t,
                                         ::std::error_code&) const;
template void Device::Write<uint32_t>(uint16_t, uint8_t, uint32_t);
template void Device::Write<uint32_t>(uint16_t, uint8_t, uint32_t,
                                      ::std::error_code&);

// REAL32
template float Device::Read<float>(uint16_t, uint8_t) const;
template float Device::Read<float>(uint16_t, uint8_t, ::std::error_code&) const;
template void Device::Write<float>(uint16_t, uint8_t, float);
template void Device::Write<float>(uint16_t, uint8_t, float,
                                   ::std::error_code&);

// VISIBLE_STRING
template ::std::string Device::Read<::std::string>(uint16_t, uint8_t) const;
template ::std::string Device::Read<::std::string>(uint16_t, uint8_t,
                                                   ::std::error_code&) const;
template void Device::Write<::std::string>(uint16_t, uint8_t,
                                           const ::std::string&);
template void Device::Write<::std::string>(uint16_t, uint8_t,
                                           const ::std::string&,
                                           ::std::error_code&);

// OCTET_STRING
template ::std::vector<uint8_t> Device::Read<::std::vector<uint8_t>>(
    uint16_t, uint8_t) const;
template ::std::vector<uint8_t> Device::Read<::std::vector<uint8_t>>(
    uint16_t, uint8_t, ::std::error_code&) const;
template void Device::Write<::std::vector<uint8_t>>(
    uint16_t, uint8_t, const ::std::vector<uint8_t>&);
template void Device::Write<::std::vector<uint8_t>>(
    uint16_t, uint8_t, const ::std::vector<uint8_t>&, ::std::error_code&);

// UNICODE_STRING
template ::std::basic_string<char16_t>
    Device::Read<::std::basic_string<char16_t>>(uint16_t, uint8_t) const;
template ::std::basic_string<char16_t>
Device::Read<::std::basic_string<char16_t>>(uint16_t, uint8_t,
                                            ::std::error_code&) const;
template void Device::Write<::std::basic_string<char16_t>>(
    uint16_t, uint8_t, const ::std::basic_string<char16_t>&);
template void Device::Write<::std::basic_string<char16_t>>(
    uint16_t, uint8_t, const ::std::basic_string<char16_t>&,
    ::std::error_code&);

// TIME_OF_DAY
// TIME_DIFFERENCE
// DOMAIN
// INTEGER24

// REAL64
template double Device::Read<double>(uint16_t, uint8_t) const;
template double Device::Read<double>(uint16_t, uint8_t,
                                     ::std::error_code&) const;
template void Device::Write<double>(uint16_t, uint8_t, double);
template void Device::Write<double>(uint16_t, uint8_t, double,
                                    ::std::error_code&);

// INTEGER40
// INTEGER48
// INTEGER56

// INTEGER64
template int64_t Device::Read<int64_t>(uint16_t, uint8_t) const;
template int64_t Device::Read<int64_t>(uint16_t, uint8_t,
                                       ::std::error_code&) const;
template void Device::Write<int64_t>(uint16_t, uint8_t, int64_t);
template void Device::Write<int64_t>(uint16_t, uint8_t, int64_t,
                                     ::std::error_code&);

// UNSIGNED24
// UNSIGNED40
// UNSIGNED48
// UNSIGNED56

// UNSIGNED64
template uint64_t Device::Read<uint64_t>(uint16_t, uint8_t) const;
template uint64_t Device::Read<uint64_t>(uint16_t, uint8_t,
                                         ::std::error_code&) const;
template void Device::Write<uint64_t>(uint16_t, uint8_t, uint64_t);
template void Device::Write<uint64_t>(uint16_t, uint8_t, uint64_t,
                                      ::std::error_code&);

#endif  // DOXYGEN_SHOULD_SKIP_THIS

void
Device::Write(uint16_t idx, uint8_t subidx, const char* value) {
  ::std::error_code ec;
  Write(idx, subidx, value, ec);
  if (ec) throw_sdo_error(id(), idx, subidx, ec, "Write");
}

void
Device::Write(uint16_t idx, uint8_t subidx, const char* value,
              ::std::error_code& ec) {
  Write(idx, subidx, value, ::std::char_traits<char>::length(value), ec);
}

void
Device::Write(uint16_t idx, uint8_t subidx, const char16_t* value) {
  ::std::error_code ec;
  Write(idx, subidx, value, ec);
  if (ec) throw_sdo_error(id(), idx, subidx, ec, "Write");
}

void
Device::Write(uint16_t idx, uint8_t subidx, const char16_t* value,
              ::std::error_code& ec) {
  constexpr auto N = CO_DEFTYPE_UNICODE_STRING;
  uint32_t ac = 0;

  ::std::lock_guard<Impl_> lock(*impl_);
  int errsv = get_errc();
  set_errc(0);
  // TODO(jseldenthuis@lely.com): Prevent unnecessary copy.
  if (!dnReq<N>(*impl_->dev, idx, subidx, value, &OnDnCon, &ac)) {
    if (ac)
      ec = static_cast<SdoErrc>(ac);
    else
      ec.clear();
  } else {
    ec = util::make_error_code();
  }
  set_errc(errsv);
}

void
Device::Write(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n) {
  ::std::error_code ec;
  Write(idx, subidx, p, n, ec);
  if (ec) throw_sdo_error(id(), idx, subidx, ec, "Write");
}

void
Device::Write(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n,
              ::std::error_code& ec) {
  uint32_t ac = 0;

  ::std::lock_guard<Impl_> lock(*impl_);
  int errsv = get_errc();
  set_errc(0);
  if (!dnReq(*impl_->dev, idx, subidx, p, n, &OnDnCon, &ac)) {
    if (ac)
      ec = static_cast<SdoErrc>(ac);
    else
      ec.clear();
  } else {
    ec = util::make_error_code();
  }
  set_errc(errsv);
}

void
Device::WriteEvent(uint16_t idx, uint8_t subidx) {
  ::std::error_code ec;
  WriteEvent(idx, subidx, ec);
  if (ec) throw_sdo_error(id(), idx, subidx, ec, "WriteEvent");
}

void
Device::WriteEvent(uint16_t idx, uint8_t subidx,
                   ::std::error_code& ec) noexcept {
  ::std::lock_guard<Impl_> lock(*impl_);

  SetEvent(idx, subidx, ec);
}

template <class T>
typename ::std::enable_if<detail::is_canopen_basic<T>::value, T>::type
Device::RpdoRead(uint8_t id, uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  T value(RpdoRead<T>(id, idx, subidx, ec));
  if (ec) throw_sdo_error(id, idx, subidx, ec, "RpdoRead");
  return value;
}

template <class T>
typename ::std::enable_if<detail::is_canopen_basic<T>::value, T>::type
Device::RpdoRead(uint8_t id, uint16_t idx, uint8_t subidx,
                 ::std::error_code& ec) const {
  ec.clear();
  {
    ::std::lock_guard<Impl_> lock(*impl_);
    ::std::tie(idx, subidx) = impl_->RpdoMapping(id, idx, subidx, ec);
  }
  if (ec) return T();
  return Read<T>(idx, subidx, ec);
}

template <class T>
typename ::std::enable_if<detail::is_canopen_basic<T>::value, T>::type
Device::TpdoRead(uint8_t id, uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  T value(TpdoRead<T>(id, idx, subidx, ec));
  if (ec) throw_sdo_error(id, idx, subidx, ec, "TpdoRead");
  return value;
}

template <class T>
typename ::std::enable_if<detail::is_canopen_basic<T>::value, T>::type
Device::TpdoRead(uint8_t id, uint16_t idx, uint8_t subidx,
                 ::std::error_code& ec) const {
  ec.clear();
  {
    ::std::lock_guard<Impl_> lock(*impl_);
    ::std::tie(idx, subidx) = impl_->TpdoMapping(id, idx, subidx, ec);
  }
  if (ec) return T();
  return Read<T>(idx, subidx, ec);
}

template <class T>
typename ::std::enable_if<detail::is_canopen_basic<T>::value>::type
Device::TpdoWrite(uint8_t id, uint16_t idx, uint8_t subidx, T value) {
  ::std::error_code ec;
  TpdoWrite(id, idx, subidx, value, ec);
  if (ec) throw_sdo_error(id, idx, subidx, ec, "TpdoWrite");
}

template <class T>
typename ::std::enable_if<detail::is_canopen_basic<T>::value>::type
Device::TpdoWrite(uint8_t id, uint16_t idx, uint8_t subidx, T value,
                  ::std::error_code& ec) {
  ec.clear();
  {
    ::std::lock_guard<Impl_> lock(*impl_);
    ::std::tie(idx, subidx) = impl_->TpdoMapping(id, idx, subidx, ec);
  }
  if (!ec) Write<T>(idx, subidx, value, ec);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// BOOLEAN
template bool Device::RpdoRead<bool>(uint8_t, uint16_t, uint8_t) const;
template bool Device::RpdoRead<bool>(uint8_t, uint16_t, uint8_t,
                                     ::std::error_code&) const;
template bool Device::TpdoRead<bool>(uint8_t, uint16_t, uint8_t) const;
template bool Device::TpdoRead<bool>(uint8_t, uint16_t, uint8_t,
                                     ::std::error_code&) const;
template void Device::TpdoWrite<bool>(uint8_t, uint16_t, uint8_t, bool);
template void Device::TpdoWrite<bool>(uint8_t, uint16_t, uint8_t, bool,
                                      ::std::error_code&);

// INTEGER8
template int8_t Device::RpdoRead<int8_t>(uint8_t, uint16_t, uint8_t) const;
template int8_t Device::RpdoRead<int8_t>(uint8_t, uint16_t, uint8_t,
                                         ::std::error_code&) const;
template int8_t Device::TpdoRead<int8_t>(uint8_t, uint16_t, uint8_t) const;
template int8_t Device::TpdoRead<int8_t>(uint8_t, uint16_t, uint8_t,
                                         ::std::error_code&) const;
template void Device::TpdoWrite<int8_t>(uint8_t, uint16_t, uint8_t, int8_t);
template void Device::TpdoWrite<int8_t>(uint8_t, uint16_t, uint8_t, int8_t,
                                        ::std::error_code&);

// INTEGER16
template int16_t Device::RpdoRead<int16_t>(uint8_t, uint16_t, uint8_t) const;
template int16_t Device::RpdoRead<int16_t>(uint8_t, uint16_t, uint8_t,
                                           ::std::error_code&) const;
template int16_t Device::TpdoRead<int16_t>(uint8_t, uint16_t, uint8_t) const;
template int16_t Device::TpdoRead<int16_t>(uint8_t, uint16_t, uint8_t,
                                           ::std::error_code&) const;
template void Device::TpdoWrite<int16_t>(uint8_t, uint16_t, uint8_t, int16_t);
template void Device::TpdoWrite<int16_t>(uint8_t, uint16_t, uint8_t, int16_t,
                                         ::std::error_code&);

// INTEGER32
template int32_t Device::RpdoRead<int32_t>(uint8_t, uint16_t, uint8_t) const;
template int32_t Device::RpdoRead<int32_t>(uint8_t, uint16_t, uint8_t,
                                           ::std::error_code&) const;
template int32_t Device::TpdoRead<int32_t>(uint8_t, uint16_t, uint8_t) const;
template int32_t Device::TpdoRead<int32_t>(uint8_t, uint16_t, uint8_t,
                                           ::std::error_code&) const;
template void Device::TpdoWrite<int32_t>(uint8_t, uint16_t, uint8_t, int32_t);
template void Device::TpdoWrite<int32_t>(uint8_t, uint16_t, uint8_t, int32_t,
                                         ::std::error_code&);

// UNSIGNED8
template uint8_t Device::RpdoRead<uint8_t>(uint8_t, uint16_t, uint8_t) const;
template uint8_t Device::RpdoRead<uint8_t>(uint8_t, uint16_t, uint8_t,
                                           ::std::error_code&) const;
template uint8_t Device::TpdoRead<uint8_t>(uint8_t, uint16_t, uint8_t) const;
template uint8_t Device::TpdoRead<uint8_t>(uint8_t, uint16_t, uint8_t,
                                           ::std::error_code&) const;
template void Device::TpdoWrite<uint8_t>(uint8_t, uint16_t, uint8_t, uint8_t);
template void Device::TpdoWrite<uint8_t>(uint8_t, uint16_t, uint8_t, uint8_t,
                                         ::std::error_code&);

// UNSIGNED16
template uint16_t Device::RpdoRead<uint16_t>(uint8_t, uint16_t, uint8_t) const;
template uint16_t Device::RpdoRead<uint16_t>(uint8_t, uint16_t, uint8_t,
                                             ::std::error_code&) const;
template uint16_t Device::TpdoRead<uint16_t>(uint8_t, uint16_t, uint8_t) const;
template uint16_t Device::TpdoRead<uint16_t>(uint8_t, uint16_t, uint8_t,
                                             ::std::error_code&) const;
template void Device::TpdoWrite<uint16_t>(uint8_t, uint16_t, uint8_t, uint16_t);
template void Device::TpdoWrite<uint16_t>(uint8_t, uint16_t, uint8_t, uint16_t,
                                          ::std::error_code&);

// UNSIGNED32
template uint32_t Device::RpdoRead<uint32_t>(uint8_t, uint16_t, uint8_t) const;
template uint32_t Device::RpdoRead<uint32_t>(uint8_t, uint16_t, uint8_t,
                                             ::std::error_code&) const;
template uint32_t Device::TpdoRead<uint32_t>(uint8_t, uint16_t, uint8_t) const;
template uint32_t Device::TpdoRead<uint32_t>(uint8_t, uint16_t, uint8_t,
                                             ::std::error_code&) const;
template void Device::TpdoWrite<uint32_t>(uint8_t, uint16_t, uint8_t, uint32_t);
template void Device::TpdoWrite<uint32_t>(uint8_t, uint16_t, uint8_t, uint32_t,
                                          ::std::error_code&);

// REAL32
template float Device::RpdoRead<float>(uint8_t, uint16_t, uint8_t) const;
template float Device::RpdoRead<float>(uint8_t, uint16_t, uint8_t,
                                       ::std::error_code&) const;
template float Device::TpdoRead<float>(uint8_t, uint16_t, uint8_t) const;
template float Device::TpdoRead<float>(uint8_t, uint16_t, uint8_t,
                                       ::std::error_code&) const;
template void Device::TpdoWrite<float>(uint8_t, uint16_t, uint8_t, float);
template void Device::TpdoWrite<float>(uint8_t, uint16_t, uint8_t, float,
                                       ::std::error_code&);

// VISIBLE_STRING
// OCTET_STRING
// UNICODE_STRING
// TIME_OF_DAY
// TIME_DIFFERENCE
// DOMAIN
// INTEGER24

// REAL64
template double Device::RpdoRead<double>(uint8_t, uint16_t, uint8_t) const;
template double Device::RpdoRead<double>(uint8_t, uint16_t, uint8_t,
                                         ::std::error_code&) const;
template double Device::TpdoRead<double>(uint8_t, uint16_t, uint8_t) const;
template double Device::TpdoRead<double>(uint8_t, uint16_t, uint8_t,
                                         ::std::error_code&) const;
template void Device::TpdoWrite<double>(uint8_t, uint16_t, uint8_t, double);
template void Device::TpdoWrite<double>(uint8_t, uint16_t, uint8_t, double,
                                        ::std::error_code&);

// INTEGER40
// INTEGER48
// INTEGER56

// INTEGER64
template int64_t Device::RpdoRead<int64_t>(uint8_t, uint16_t, uint8_t) const;
template int64_t Device::RpdoRead<int64_t>(uint8_t, uint16_t, uint8_t,
                                           ::std::error_code&) const;
template int64_t Device::TpdoRead<int64_t>(uint8_t, uint16_t, uint8_t) const;
template int64_t Device::TpdoRead<int64_t>(uint8_t, uint16_t, uint8_t,
                                           ::std::error_code&) const;
template void Device::TpdoWrite<int64_t>(uint8_t, uint16_t, uint8_t, int64_t);
template void Device::TpdoWrite<int64_t>(uint8_t, uint16_t, uint8_t, int64_t,
                                         ::std::error_code&);

// UNSIGNED24
// UNSIGNED40
// UNSIGNED48
// UNSIGNED56

// UNSIGNED64
template uint64_t Device::RpdoRead<uint64_t>(uint8_t, uint16_t, uint8_t) const;
template uint64_t Device::RpdoRead<uint64_t>(uint8_t, uint16_t, uint8_t,
                                             ::std::error_code&) const;
template uint64_t Device::TpdoRead<uint64_t>(uint8_t, uint16_t, uint8_t) const;
template uint64_t Device::TpdoRead<uint64_t>(uint8_t, uint16_t, uint8_t,
                                             ::std::error_code&) const;
template void Device::TpdoWrite<uint64_t>(uint8_t, uint16_t, uint8_t, uint64_t);
template void Device::TpdoWrite<uint64_t>(uint8_t, uint16_t, uint8_t, uint64_t,
                                          ::std::error_code&);

#endif  // DOXYGEN_SHOULD_SKIP_THIS

void
Device::TpdoWriteEvent(uint8_t id, uint16_t idx, uint8_t subidx) {
  ::std::error_code ec;
  TpdoWriteEvent(id, idx, subidx, ec);
  if (ec) throw_sdo_error(id, idx, subidx, ec, "TpdoWriteEvent");
}

void
Device::TpdoWriteEvent(uint8_t id, uint16_t idx, uint8_t subidx,
                       ::std::error_code& ec) noexcept {
  ::std::lock_guard<Impl_> lock(*impl_);

  TpdoSetEvent(id, idx, subidx, ec);
}

void
Device::OnWrite(::std::function<void(uint16_t, uint8_t)> on_write) {
  ::std::lock_guard<Impl_> lock(*impl_);
  impl_->on_write = on_write;
}

void
Device::OnRpdoWrite(
    ::std::function<void(uint8_t, uint16_t, uint8_t)> on_rpdo_write) {
  ::std::lock_guard<Impl_> lock(*impl_);
  impl_->on_rpdo_write = on_rpdo_write;
}

CODev*
Device::dev() const noexcept {
  return impl_->dev.get();
}

const ::std::type_info&
Device::Type(uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  auto& ti = Type(idx, subidx, ec);
  if (ec) throw_sdo_error(impl_->id(), idx, subidx, ec, "Type");
  return ti;
}

const ::std::type_info&
Device::Type(uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
    noexcept {
  auto obj = impl_->dev->find(idx);
  if (!obj) {
    ec = SdoErrc::NO_OBJ;
    return typeid(void);
  }

  auto sub = obj->find(subidx);
  if (!sub) {
    ec = SdoErrc::NO_SUB;
    return typeid(void);
  }

  ec.clear();
  switch (sub->getType()) {
    case CO_DEFTYPE_BOOLEAN:
      return typeid(bool);
    case CO_DEFTYPE_INTEGER8:
      return typeid(int8_t);
    case CO_DEFTYPE_INTEGER16:
      return typeid(int16_t);
    case CO_DEFTYPE_INTEGER32:
      return typeid(int32_t);
    case CO_DEFTYPE_UNSIGNED8:
      return typeid(uint8_t);
    case CO_DEFTYPE_UNSIGNED16:
      return typeid(uint16_t);
    case CO_DEFTYPE_UNSIGNED32:
      return typeid(uint32_t);
    case CO_DEFTYPE_REAL32:
      return typeid(float);
    case CO_DEFTYPE_VISIBLE_STRING:
      return typeid(::std::string);
    case CO_DEFTYPE_OCTET_STRING:
      return typeid(::std::vector<uint8_t>);
    case CO_DEFTYPE_UNICODE_STRING:
      return typeid(::std::basic_string<char16_t>);
    // case CO_DEFTYPE_TIME_OF_DAY: ...
    // case CO_DEFTYPE_TIME_DIFFERENCE: ...
    case CO_DEFTYPE_DOMAIN:
      return typeid(::std::vector<uint8_t>);
    // case CO_DEFTYPE_INTEGER24: ...
    case CO_DEFTYPE_REAL64:
      return typeid(double);
    // case CO_DEFTYPE_INTEGER40: ...
    // case CO_DEFTYPE_INTEGER48: ...
    // case CO_DEFTYPE_INTEGER56: ...
    case CO_DEFTYPE_INTEGER64:
      return typeid(int64_t);
    // case CO_DEFTYPE_UNSIGNED24: ...
    // case CO_DEFTYPE_UNSIGNED40: ...
    // case CO_DEFTYPE_UNSIGNED48: ...
    // case CO_DEFTYPE_UNSIGNED56: ...
    case CO_DEFTYPE_UNSIGNED64:
      return typeid(uint64_t);
    default:
      return typeid(void);
  }
}

template <class T>
typename ::std::enable_if<detail::is_canopen_type<T>::value, T>::type
Device::Get(uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  auto value = Get<T>(idx, subidx, ec);
  if (ec) throw_sdo_error(impl_->id(), idx, subidx, ec, "Get");
  return value;
}

template <class T>
typename ::std::enable_if<detail::is_canopen_type<T>::value, T>::type
Device::Get(uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
    noexcept {
  constexpr auto N = co_type_traits_T<T>::index;

  auto obj = impl_->dev->find(idx);
  if (!obj) {
    ec = SdoErrc::NO_OBJ;
    return T();
  }

  auto sub = obj->find(subidx);
  if (!sub) {
    ec = SdoErrc::NO_SUB;
    return T();
  }

  if (!detail::is_canopen_same(N, sub->getType())) {
    ec = SdoErrc::TYPE_LEN;
    return T();
  }

  ec.clear();
  // This is efficient, even for CANopen array values, since getVal<N>() returns
  // a reference.
  return sub->getVal<N>();
}

template <class T>
typename ::std::enable_if<detail::is_canopen_basic<T>::value>::type
Device::Set(uint16_t idx, uint8_t subidx, T value) {
  ::std::error_code ec;
  Set(idx, subidx, value, ec);
  if (ec) throw_sdo_error(impl_->id(), idx, subidx, ec, "Set");
}

template <class T>
typename ::std::enable_if<detail::is_canopen_basic<T>::value>::type
Device::Set(uint16_t idx, uint8_t subidx, T value,
            ::std::error_code& ec) noexcept {
  constexpr auto N = co_type_traits_T<T>::index;

  impl_->Set<N>(idx, subidx, &value, sizeof(value), ec);
}

template <class T>
typename ::std::enable_if<detail::is_canopen_array<T>::value>::type
Device::Set(uint16_t idx, uint8_t subidx, const T& value) {
  ::std::error_code ec;
  Set(idx, subidx, value, ec);
  if (ec) throw_sdo_error(impl_->id(), idx, subidx, ec, "Set");
}

template <class T>
typename ::std::enable_if<detail::is_canopen_array<T>::value>::type
Device::Set(uint16_t idx, uint8_t subidx, const T& value,
            ::std::error_code& ec) noexcept {
  impl_->Set(idx, subidx, value, ec);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// BOOLEAN
template bool Device::Get<bool>(uint16_t, uint8_t) const;
template bool Device::Get<bool>(uint16_t, uint8_t, ::std::error_code&) const
    noexcept;
template void Device::Set<bool>(uint16_t, uint8_t, bool);
template void Device::Set<bool>(uint16_t, uint8_t, bool,
                                ::std::error_code&) noexcept;

// INTEGER8
template int8_t Device::Get<int8_t>(uint16_t, uint8_t) const;
template int8_t Device::Get<int8_t>(uint16_t, uint8_t, ::std::error_code&) const
    noexcept;
template void Device::Set<int8_t>(uint16_t, uint8_t, int8_t);
template void Device::Set<int8_t>(uint16_t, uint8_t, int8_t,
                                  ::std::error_code&) noexcept;

// INTEGER16
template int16_t Device::Get<int16_t>(uint16_t, uint8_t) const;
template int16_t Device::Get<int16_t>(uint16_t, uint8_t,
                                      ::std::error_code&) const noexcept;
template void Device::Set<int16_t>(uint16_t, uint8_t, int16_t);
template void Device::Set<int16_t>(uint16_t, uint8_t, int16_t,
                                   ::std::error_code&) noexcept;

// INTEGER32
template int32_t Device::Get<int32_t>(uint16_t, uint8_t) const;
template int32_t Device::Get<int32_t>(uint16_t, uint8_t,
                                      ::std::error_code&) const noexcept;
template void Device::Set<int32_t>(uint16_t, uint8_t, int32_t);
template void Device::Set<int32_t>(uint16_t, uint8_t, int32_t,
                                   ::std::error_code&) noexcept;

// UNSIGNED8
template uint8_t Device::Get<uint8_t>(uint16_t, uint8_t) const;
template uint8_t Device::Get<uint8_t>(uint16_t, uint8_t,
                                      ::std::error_code&) const noexcept;
template void Device::Set<uint8_t>(uint16_t, uint8_t, uint8_t);
template void Device::Set<uint8_t>(uint16_t, uint8_t, uint8_t,
                                   ::std::error_code&) noexcept;

// UNSIGNED16
template uint16_t Device::Get<uint16_t>(uint16_t, uint8_t) const;
template uint16_t Device::Get<uint16_t>(uint16_t, uint8_t,
                                        ::std::error_code&) const noexcept;
template void Device::Set<uint16_t>(uint16_t, uint8_t, uint16_t);
template void Device::Set<uint16_t>(uint16_t, uint8_t, uint16_t,
                                    ::std::error_code&) noexcept;

// UNSIGNED32
template uint32_t Device::Get<uint32_t>(uint16_t, uint8_t) const;
template uint32_t Device::Get<uint32_t>(uint16_t, uint8_t,
                                        ::std::error_code&) const noexcept;
template void Device::Set<uint32_t>(uint16_t, uint8_t, uint32_t);
template void Device::Set<uint32_t>(uint16_t, uint8_t, uint32_t,
                                    ::std::error_code&) noexcept;

// REAL32
template float Device::Get<float>(uint16_t, uint8_t) const;
template float Device::Get<float>(uint16_t, uint8_t, ::std::error_code&) const
    noexcept;
template void Device::Set<float>(uint16_t, uint8_t, float);
template void Device::Set<float>(uint16_t, uint8_t, float,
                                 ::std::error_code&) noexcept;

// VISIBLE_STRING
template ::std::string Device::Get<::std::string>(uint16_t, uint8_t) const;
template ::std::string Device::Get<::std::string>(uint16_t, uint8_t,
                                                  ::std::error_code&) const
    noexcept;
template void Device::Set<::std::string>(uint16_t, uint8_t,
                                         const ::std::string&);
template void Device::Set<::std::string>(uint16_t, uint8_t,
                                         const ::std::string&,
                                         ::std::error_code&) noexcept;

// OCTET_STRING
template ::std::vector<uint8_t> Device::Get<::std::vector<uint8_t>>(
    uint16_t, uint8_t) const;
template ::std::vector<uint8_t> Device::Get<::std::vector<uint8_t>>(
    uint16_t, uint8_t, ::std::error_code&) const noexcept;
template void Device::Set<::std::vector<uint8_t>>(
    uint16_t, uint8_t, const ::std::vector<uint8_t>&);
template void Device::Set<::std::vector<uint8_t>>(uint16_t, uint8_t,
                                                  const ::std::vector<uint8_t>&,
                                                  ::std::error_code&) noexcept;

// UNICODE_STRING
template ::std::basic_string<char16_t>
    Device::Get<::std::basic_string<char16_t>>(uint16_t, uint8_t) const;
template ::std::basic_string<char16_t>
Device::Get<::std::basic_string<char16_t>>(uint16_t, uint8_t,
                                           ::std::error_code&) const noexcept;
template void Device::Set<::std::basic_string<char16_t>>(
    uint16_t, uint8_t, const ::std::basic_string<char16_t>&);
template void Device::Set<::std::basic_string<char16_t>>(
    uint16_t, uint8_t, const ::std::basic_string<char16_t>&,
    ::std::error_code&) noexcept;

// TIME_OF_DAY
// TIME_DIFFERENCE
// DOMAIN
// INTEGER24

// REAL64
template double Device::Get<double>(uint16_t, uint8_t) const;
template double Device::Get<double>(uint16_t, uint8_t, ::std::error_code&) const
    noexcept;
template void Device::Set<double>(uint16_t, uint8_t, double);
template void Device::Set<double>(uint16_t, uint8_t, double,
                                  ::std::error_code&) noexcept;

// INTEGER40
// INTEGER48
// INTEGER56

// INTEGER64
template int64_t Device::Get<int64_t>(uint16_t, uint8_t) const;
template int64_t Device::Get<int64_t>(uint16_t, uint8_t,
                                      ::std::error_code&) const noexcept;
template void Device::Set<int64_t>(uint16_t, uint8_t, int64_t);
template void Device::Set<int64_t>(uint16_t, uint8_t, int64_t,
                                   ::std::error_code&) noexcept;

// UNSIGNED24
// UNSIGNED40
// UNSIGNED48
// UNSIGNED56

// UNSIGNED64
template uint64_t Device::Get<uint64_t>(uint16_t, uint8_t) const;
template uint64_t Device::Get<uint64_t>(uint16_t, uint8_t,
                                        ::std::error_code&) const noexcept;
template void Device::Set<uint64_t>(uint16_t, uint8_t, uint64_t);
template void Device::Set<uint64_t>(uint16_t, uint8_t, uint64_t,
                                    ::std::error_code&) noexcept;

#endif  // DOXYGEN_SHOULD_SKIP_THIS

void
Device::Set(uint16_t idx, uint8_t subidx, const char* value) {
  ::std::error_code ec;
  Set(idx, subidx, value, ec);
  if (ec) throw_sdo_error(impl_->id(), idx, subidx, ec, "Set");
}

void
Device::Set(uint16_t idx, uint8_t subidx, const char* value,
            ::std::error_code& ec) noexcept {
  impl_->Set<CO_DEFTYPE_VISIBLE_STRING>(idx, subidx, value, 0, ec);
}

void
Device::Set(uint16_t idx, uint8_t subidx, const char16_t* value) {
  ::std::error_code ec;
  Set(idx, subidx, value, ec);
  if (ec) throw_sdo_error(impl_->id(), idx, subidx, ec, "Set");
}

void
Device::Set(uint16_t idx, uint8_t subidx, const char16_t* value,
            ::std::error_code& ec) noexcept {
  impl_->Set<CO_DEFTYPE_UNICODE_STRING>(idx, subidx, value, 0, ec);
}

void
Device::Set(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n) {
  ::std::error_code ec;
  Set(idx, subidx, p, n, ec);
  if (ec) throw_sdo_error(impl_->id(), idx, subidx, ec, "Set");
}

void
Device::Set(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n,
            ::std::error_code& ec) noexcept {
  impl_->Set<CO_DEFTYPE_OCTET_STRING>(idx, subidx, p, n, ec);
}

const char*
Device::GetUploadFile(uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  auto filename = GetUploadFile(idx, subidx, ec);
  if (ec) throw_sdo_error(impl_->id(), idx, subidx, ec, "GetUploadFile");
  return filename;
}

const char*
Device::GetUploadFile(uint16_t idx, uint8_t subidx, ::std::error_code& ec) const
    noexcept {
  auto obj = impl_->dev->find(idx);
  if (!obj) {
    ec = SdoErrc::NO_OBJ;
    return nullptr;
  }

  auto sub = obj->find(subidx);
  if (!sub) {
    ec = SdoErrc::NO_SUB;
    return nullptr;
  }

  ec.clear();
  return sub->getUploadFile();
}

void
Device::SetUploadFile(uint16_t idx, uint8_t subidx, const char* filename) {
  ::std::error_code ec;
  SetUploadFile(idx, subidx, filename, ec);
  if (ec) throw_sdo_error(impl_->id(), idx, subidx, ec, "SetUploadFile");
}

void
Device::SetUploadFile(uint16_t idx, uint8_t subidx, const char* filename,
                      ::std::error_code& ec) noexcept {
  auto obj = impl_->dev->find(idx);
  if (!obj) {
    ec = SdoErrc::NO_OBJ;
    return;
  }

  auto sub = obj->find(subidx);
  if (!sub) {
    ec = SdoErrc::NO_SUB;
    return;
  }

  int errsv = get_errc();
  set_errc(0);
  if (!sub->setUploadFile(filename))
    ec.clear();
  else
    ec = util::make_error_code();
  set_errc(errsv);
}

const char*
Device::GetDownloadFile(uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  auto filename = GetDownloadFile(idx, subidx, ec);
  if (ec) throw_sdo_error(impl_->id(), idx, subidx, ec, "GetDownloadFile");
  return filename;
}

const char*
Device::GetDownloadFile(uint16_t idx, uint8_t subidx,
                        ::std::error_code& ec) const noexcept {
  auto obj = impl_->dev->find(idx);
  if (!obj) {
    ec = SdoErrc::NO_OBJ;
    return nullptr;
  }

  auto sub = obj->find(subidx);
  if (!sub) {
    ec = SdoErrc::NO_SUB;
    return nullptr;
  }

  ec.clear();
  return sub->getDownloadFile();
}

void
Device::SetDownloadFile(uint16_t idx, uint8_t subidx, const char* filename) {
  ::std::error_code ec;
  SetDownloadFile(idx, subidx, filename, ec);
  if (ec) throw_sdo_error(impl_->id(), idx, subidx, ec, "SetDownloadFile");
}

void
Device::SetDownloadFile(uint16_t idx, uint8_t subidx, const char* filename,
                        ::std::error_code& ec) noexcept {
  auto obj = impl_->dev->find(idx);
  if (!obj) {
    ec = SdoErrc::NO_OBJ;
    return;
  }

  auto sub = obj->find(subidx);
  if (!sub) {
    ec = SdoErrc::NO_SUB;
    return;
  }

  int errsv = get_errc();
  set_errc(0);
  if (!sub->setDownloadFile(filename))
    ec.clear();
  else
    ec = util::make_error_code();
  set_errc(errsv);
}

void
Device::SetEvent(uint16_t idx, uint8_t subidx) {
  ::std::error_code ec;
  SetEvent(idx, subidx, ec);
  if (ec) throw_sdo_error(impl_->id(), idx, subidx, ec, "SetEvent");
}

void
Device::SetEvent(uint16_t idx, uint8_t subidx, ::std::error_code& ec) noexcept {
  auto obj = impl_->dev->find(idx);
  if (!obj) {
    ec = SdoErrc::NO_OBJ;
    return;
  }

  auto sub = obj->find(subidx);
  if (!sub) {
    ec = SdoErrc::NO_SUB;
    return;
  }

  impl_->dev->TPDOEvent(idx, subidx);
}

template <class T>
typename ::std::enable_if<detail::is_canopen_basic<T>::value, T>::type
Device::RpdoGet(uint8_t id, uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  auto value = RpdoGet<T>(id, idx, subidx, ec);
  if (ec) throw_sdo_error(id, idx, subidx, ec, "RpdoGet");
  return value;
}

template <class T>
typename ::std::enable_if<detail::is_canopen_basic<T>::value, T>::type
Device::RpdoGet(uint8_t id, uint16_t idx, uint8_t subidx,
                ::std::error_code& ec) const noexcept {
  ec.clear();
  ::std::tie(idx, subidx) = impl_->RpdoMapping(id, idx, subidx, ec);
  if (ec) return T();
  return Get<T>(idx, subidx, ec);
}

template <class T>
typename ::std::enable_if<detail::is_canopen_basic<T>::value, T>::type
Device::TpdoGet(uint8_t id, uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  auto value = TpdoGet<T>(id, idx, subidx, ec);
  if (ec) throw_sdo_error(id, idx, subidx, ec, "TpdoGet");
  return value;
}

template <class T>
typename ::std::enable_if<detail::is_canopen_basic<T>::value, T>::type
Device::TpdoGet(uint8_t id, uint16_t idx, uint8_t subidx,
                ::std::error_code& ec) const noexcept {
  ec.clear();
  ::std::tie(idx, subidx) = impl_->TpdoMapping(id, idx, subidx, ec);
  if (ec) return T();
  return Get<T>(idx, subidx, ec);
}

template <class T>
typename ::std::enable_if<detail::is_canopen_basic<T>::value>::type
Device::TpdoSet(uint8_t id, uint16_t idx, uint8_t subidx, T value) {
  ::std::error_code ec;
  TpdoSet(id, idx, subidx, value, ec);
  if (ec) throw_sdo_error(id, idx, subidx, ec, "TpdoSet");
}

template <class T>
typename ::std::enable_if<detail::is_canopen_basic<T>::value>::type
Device::TpdoSet(uint8_t id, uint16_t idx, uint8_t subidx, T value,
                ::std::error_code& ec) noexcept {
  ec.clear();
  ::std::tie(idx, subidx) = impl_->TpdoMapping(id, idx, subidx, ec);
  if (!ec) Set<T>(idx, subidx, value, ec);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// BOOLEAN
template bool Device::RpdoGet<bool>(uint8_t, uint16_t, uint8_t) const;
template bool Device::RpdoGet<bool>(uint8_t, uint16_t, uint8_t,
                                    ::std::error_code&) const noexcept;
template bool Device::TpdoGet<bool>(uint8_t, uint16_t, uint8_t) const;
template bool Device::TpdoGet<bool>(uint8_t, uint16_t, uint8_t,
                                    ::std::error_code&) const noexcept;
template void Device::TpdoSet<bool>(uint8_t, uint16_t, uint8_t, bool);
template void Device::TpdoSet<bool>(uint8_t, uint16_t, uint8_t, bool,
                                    ::std::error_code&) noexcept;

// INTEGER8
template int8_t Device::RpdoGet<int8_t>(uint8_t, uint16_t, uint8_t) const;
template int8_t Device::RpdoGet<int8_t>(uint8_t, uint16_t, uint8_t,
                                        ::std::error_code&) const noexcept;
template int8_t Device::TpdoGet<int8_t>(uint8_t, uint16_t, uint8_t) const;
template int8_t Device::TpdoGet<int8_t>(uint8_t, uint16_t, uint8_t,
                                        ::std::error_code&) const noexcept;
template void Device::TpdoSet<int8_t>(uint8_t, uint16_t, uint8_t, int8_t);
template void Device::TpdoSet<int8_t>(uint8_t, uint16_t, uint8_t, int8_t,
                                      ::std::error_code&) noexcept;

// INTEGER16
template int16_t Device::RpdoGet<int16_t>(uint8_t, uint16_t, uint8_t) const;
template int16_t Device::RpdoGet<int16_t>(uint8_t, uint16_t, uint8_t,
                                          ::std::error_code&) const noexcept;
template int16_t Device::TpdoGet<int16_t>(uint8_t, uint16_t, uint8_t) const;
template int16_t Device::TpdoGet<int16_t>(uint8_t, uint16_t, uint8_t,
                                          ::std::error_code&) const noexcept;
template void Device::TpdoSet<int16_t>(uint8_t, uint16_t, uint8_t, int16_t);
template void Device::TpdoSet<int16_t>(uint8_t, uint16_t, uint8_t, int16_t,
                                       ::std::error_code&) noexcept;

// INTEGER32
template int32_t Device::RpdoGet<int32_t>(uint8_t, uint16_t, uint8_t) const;
template int32_t Device::RpdoGet<int32_t>(uint8_t, uint16_t, uint8_t,
                                          ::std::error_code&) const noexcept;
template int32_t Device::TpdoGet<int32_t>(uint8_t, uint16_t, uint8_t) const;
template int32_t Device::TpdoGet<int32_t>(uint8_t, uint16_t, uint8_t,
                                          ::std::error_code&) const noexcept;
template void Device::TpdoSet<int32_t>(uint8_t, uint16_t, uint8_t, int32_t);
template void Device::TpdoSet<int32_t>(uint8_t, uint16_t, uint8_t, int32_t,
                                       ::std::error_code&) noexcept;

// UNSIGNED8
template uint8_t Device::RpdoGet<uint8_t>(uint8_t, uint16_t, uint8_t) const;
template uint8_t Device::RpdoGet<uint8_t>(uint8_t, uint16_t, uint8_t,
                                          ::std::error_code&) const noexcept;
template uint8_t Device::TpdoGet<uint8_t>(uint8_t, uint16_t, uint8_t) const;
template uint8_t Device::TpdoGet<uint8_t>(uint8_t, uint16_t, uint8_t,
                                          ::std::error_code&) const noexcept;
template void Device::TpdoSet<uint8_t>(uint8_t, uint16_t, uint8_t, uint8_t);
template void Device::TpdoSet<uint8_t>(uint8_t, uint16_t, uint8_t, uint8_t,
                                       ::std::error_code&) noexcept;

// UNSIGNED16
template uint16_t Device::RpdoGet<uint16_t>(uint8_t, uint16_t, uint8_t) const;
template uint16_t Device::RpdoGet<uint16_t>(uint8_t, uint16_t, uint8_t,
                                            ::std::error_code&) const noexcept;
template uint16_t Device::TpdoGet<uint16_t>(uint8_t, uint16_t, uint8_t) const;
template uint16_t Device::TpdoGet<uint16_t>(uint8_t, uint16_t, uint8_t,
                                            ::std::error_code&) const noexcept;
template void Device::TpdoSet<uint16_t>(uint8_t, uint16_t, uint8_t, uint16_t);
template void Device::TpdoSet<uint16_t>(uint8_t, uint16_t, uint8_t, uint16_t,
                                        ::std::error_code&) noexcept;

// UNSIGNED32
template uint32_t Device::RpdoGet<uint32_t>(uint8_t, uint16_t, uint8_t) const;
template uint32_t Device::RpdoGet<uint32_t>(uint8_t, uint16_t, uint8_t,
                                            ::std::error_code&) const noexcept;
template uint32_t Device::TpdoGet<uint32_t>(uint8_t, uint16_t, uint8_t) const;
template uint32_t Device::TpdoGet<uint32_t>(uint8_t, uint16_t, uint8_t,
                                            ::std::error_code&) const noexcept;
template void Device::TpdoSet<uint32_t>(uint8_t, uint16_t, uint8_t, uint32_t);
template void Device::TpdoSet<uint32_t>(uint8_t, uint16_t, uint8_t, uint32_t,
                                        ::std::error_code&) noexcept;

// REAL32
template float Device::RpdoGet<float>(uint8_t, uint16_t, uint8_t) const;
template float Device::RpdoGet<float>(uint8_t, uint16_t, uint8_t,
                                      ::std::error_code&) const noexcept;
template float Device::TpdoGet<float>(uint8_t, uint16_t, uint8_t) const;
template float Device::TpdoGet<float>(uint8_t, uint16_t, uint8_t,
                                      ::std::error_code&) const noexcept;
template void Device::TpdoSet<float>(uint8_t, uint16_t, uint8_t, float);
template void Device::TpdoSet<float>(uint8_t, uint16_t, uint8_t, float,
                                     ::std::error_code&) noexcept;

// VISIBLE_STRING
// OCTET_STRING
// UNICODE_STRING
// TIME_OF_DAY
// TIME_DIFFERENCE
// DOMAIN
// INTEGER24

// REAL64
template double Device::RpdoGet<double>(uint8_t, uint16_t, uint8_t) const;
template double Device::RpdoGet<double>(uint8_t, uint16_t, uint8_t,
                                        ::std::error_code&) const noexcept;
template double Device::TpdoGet<double>(uint8_t, uint16_t, uint8_t) const;
template double Device::TpdoGet<double>(uint8_t, uint16_t, uint8_t,
                                        ::std::error_code&) const noexcept;
template void Device::TpdoSet<double>(uint8_t, uint16_t, uint8_t, double);
template void Device::TpdoSet<double>(uint8_t, uint16_t, uint8_t, double,
                                      ::std::error_code&) noexcept;

// INTEGER40
// INTEGER48
// INTEGER56

// INTEGER64
template int64_t Device::RpdoGet<int64_t>(uint8_t, uint16_t, uint8_t) const;
template int64_t Device::RpdoGet<int64_t>(uint8_t, uint16_t, uint8_t,
                                          ::std::error_code&) const noexcept;
template int64_t Device::TpdoGet<int64_t>(uint8_t, uint16_t, uint8_t) const;
template int64_t Device::TpdoGet<int64_t>(uint8_t, uint16_t, uint8_t,
                                          ::std::error_code&) const noexcept;
template void Device::TpdoSet<int64_t>(uint8_t, uint16_t, uint8_t, int64_t);
template void Device::TpdoSet<int64_t>(uint8_t, uint16_t, uint8_t, int64_t,
                                       ::std::error_code&) noexcept;

// UNSIGNED24
// UNSIGNED40
// UNSIGNED48
// UNSIGNED56

// UNSIGNED64
template uint64_t Device::RpdoGet<uint64_t>(uint8_t, uint16_t, uint8_t) const;
template uint64_t Device::RpdoGet<uint64_t>(uint8_t, uint16_t, uint8_t,
                                            ::std::error_code&) const noexcept;
template uint64_t Device::TpdoGet<uint64_t>(uint8_t, uint16_t, uint8_t) const;
template uint64_t Device::TpdoGet<uint64_t>(uint8_t, uint16_t, uint8_t,
                                            ::std::error_code&) const noexcept;
template void Device::TpdoSet<uint64_t>(uint8_t, uint16_t, uint8_t, uint64_t);
template void Device::TpdoSet<uint64_t>(uint8_t, uint16_t, uint8_t, uint64_t,
                                        ::std::error_code&) noexcept;

#endif  // DOXYGEN_SHOULD_SKIP_THIS

void
Device::TpdoSetEvent(uint8_t id, uint16_t idx, uint8_t subidx) {
  ::std::error_code ec;
  TpdoSetEvent(id, idx, subidx, ec);
  if (ec) throw_sdo_error(id, idx, subidx, ec, "TpdoSetEvent");
}

void
Device::TpdoSetEvent(uint8_t id, uint16_t idx, uint8_t subidx,
                     ::std::error_code& ec) noexcept {
  ec.clear();
  ::std::tie(idx, subidx) = impl_->TpdoMapping(id, idx, subidx, ec);
  if (!ec) SetEvent(idx, subidx, ec);
}

void
Device::UpdateRpdoMapping() {
  impl_->rpdo_mapping.clear();

  for (int i = 0; i < 512; i++) {
    auto obj_1400 = impl_->dev->find(0x1400 + i);
    if (!obj_1400) continue;
    // Skip invalid PDOs.
    auto cobid = obj_1400->getVal<CO_DEFTYPE_UNSIGNED32>(1);
    if (cobid & CO_PDO_COBID_VALID) continue;
    // Obtain the remote node-ID.
    uint8_t id = 0;
    auto obj_5800 = impl_->dev->find(0x5800 + i);
    if (obj_5800) {
      id = obj_5800->getVal<CO_DEFTYPE_UNSIGNED32>(0) & 0xff;
    } else {
      // Obtain the node-ID from the predefined connection, if possible.
      if (cobid & CO_PDO_COBID_FRAME) continue;
      switch (cobid & 0x780) {
        case 0x180:
        case 0x280:
        case 0x380:
        case 0x480:
          id = cobid & 0x7f;
          break;
        default:
          continue;
      }
    }
    // Skip invalid node-IDs.
    if (!id || id > CO_NUM_NODES) continue;
    // Obtain the local RPDO mapping.
    auto obj_1600 = impl_->dev->find(0x1600 + i);
    if (!obj_1600) continue;
    // Obtain the remote TPDO mapping.
    auto obj_5a00 = impl_->dev->find(0x5a00 + i);
    if (!obj_5a00) continue;
    // Check if the number of mapped objects is the same.
    auto n = obj_1600->getVal<CO_DEFTYPE_UNSIGNED8>(0);
    if (n != obj_5a00->getVal<CO_DEFTYPE_UNSIGNED8>(0)) continue;
    for (int i = 1; i <= n; i++) {
      auto rmap = obj_1600->getVal<CO_DEFTYPE_UNSIGNED32>(i);
      auto tmap = obj_5a00->getVal<CO_DEFTYPE_UNSIGNED32>(i);
      // Ignore empty mapping entries.
      if (!rmap && !tmap) continue;
      // Check if the mapped objects have the same length.
      if ((rmap & 0xff) != (tmap & 0xff)) break;
      rmap >>= 8;
      tmap >>= 8;
      // Skip dummy-mapped objects.
      if (co_type_is_basic((rmap >> 8) & 0xffff) && !(rmap & 0xff)) continue;
      tmap |= static_cast<uint32_t>(id) << 24;
      impl_->rpdo_mapping[tmap] = rmap;
      // Store the reverse mapping for OnRpdoWrite().
      impl_->rpdo_mapping[rmap] = tmap;
    }
  }
}

void
Device::UpdateTpdoMapping() {
  impl_->tpdo_mapping.clear();

  for (int i = 0; i < 512; i++) {
    auto obj_1800 = impl_->dev->find(0x1800 + i);
    if (!obj_1800) continue;
    // Skip invalid PDOs.
    auto cobid = obj_1800->getVal<CO_DEFTYPE_UNSIGNED32>(1);
    if (cobid & CO_PDO_COBID_VALID) continue;
    // Obtain the remote node-ID.
    uint8_t id = 0;
    auto obj_5c00 = impl_->dev->find(0x5c00 + i);
    if (obj_5c00) {
      id = obj_5c00->getVal<CO_DEFTYPE_UNSIGNED32>(0) & 0xff;
    } else {
      // Obtain the node-ID from the predefined connection, if possible.
      if (cobid & CO_PDO_COBID_FRAME) continue;
      switch (cobid & 0x780) {
        case 0x200:
        case 0x300:
        case 0x400:
        case 0x500:
          id = cobid & 0x7f;
          break;
        default:
          continue;
      }
    }
    // Skip invalid node-IDs.
    if (!id || id > CO_NUM_NODES) continue;
    // Obtain the local TPDO mapping.
    auto obj_1a00 = impl_->dev->find(0x1a00 + i);
    if (!obj_1a00) continue;
    // Obtain the remote RPDO mapping.
    auto obj_5e00 = impl_->dev->find(0x5e00 + i);
    if (!obj_5e00) continue;
    // Check if the number of mapped objects is the same.
    auto n = obj_1a00->getVal<CO_DEFTYPE_UNSIGNED8>(0);
    if (n != obj_5e00->getVal<CO_DEFTYPE_UNSIGNED8>(0)) continue;
    for (int i = 1; i <= n; i++) {
      auto tmap = obj_1a00->getVal<CO_DEFTYPE_UNSIGNED32>(i);
      auto rmap = obj_5e00->getVal<CO_DEFTYPE_UNSIGNED32>(i);
      // Ignore empty mapping entries.
      if (!rmap && !tmap) continue;
      // Check if the mapped objects have the same length.
      if ((tmap & 0xff) != (rmap & 0xff)) break;
      tmap >>= 8;
      rmap >>= 8;
      rmap |= static_cast<uint32_t>(id) << 24;
      impl_->tpdo_mapping[rmap] = tmap;
    }
  }
}

Device::Impl_::Impl_(Device* self_, const ::std::string& dcf_txt,
                     const ::std::string& dcf_bin, uint8_t id,
                     util::BasicLockable* mutex_)
    : self(self_), mutex(mutex_), dev(make_unique_c<CODev>(dcf_txt.c_str())) {
  if (!dcf_bin.empty() && dev->readDCF(nullptr, nullptr, dcf_bin.c_str()) == -1)
    util::throw_errc("Device");

  if (id != 0xff && dev->setId(id) == -1) util::throw_errc("Device");

  // Register a notification function for all objects in the object dictionary
  // in case of write (SDO upload) access.
  for (auto obj = co_dev_first_obj(dev.get()); obj; obj = co_obj_next(obj)) {
    // Skip data types and the communication profile area.
    if (obj->getIdx() < 0x2000) continue;
    // Skip reserved objects.
    if (obj->getIdx() >= 0xC000) break;
    obj->setDnInd(
        [](COSub* sub, co_sdo_req* req, void* data) -> uint32_t {
          // Implement the default behavior, but do not issue a notification for
          // incomplete or failed writes.
          uint32_t ac = 0;
          if (sub->onDn(*req, &ac) == -1 || ac) return ac;
          auto impl_ = static_cast<Impl_*>(data);
          impl_->OnWrite(sub->getObj()->getIdx(), sub->getSubidx());
          return 0;
        },
        static_cast<void*>(this));
  }
}

void
Device::Impl_::Set(uint16_t idx, uint8_t subidx, const ::std::string& value,
                   ::std::error_code& ec) noexcept {
  Set<CO_DEFTYPE_VISIBLE_STRING>(idx, subidx, value.c_str(), 0, ec);
}

void
Device::Impl_::Set(uint16_t idx, uint8_t subidx,
                   const ::std::vector<uint8_t>& value,
                   ::std::error_code& ec) noexcept {
  Set<CO_DEFTYPE_OCTET_STRING>(idx, subidx, value.data(), value.size(), ec);
}

void
Device::Impl_::Set(uint16_t idx, uint8_t subidx,
                   const ::std::basic_string<char16_t>& value,
                   ::std::error_code& ec) noexcept {
  Set<CO_DEFTYPE_UNICODE_STRING>(idx, subidx, value.c_str(), 0, ec);
}

template <uint16_t N>
void
Device::Impl_::Set(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n,
                   ::std::error_code& ec) noexcept {
  auto obj = dev->find(idx);
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

  int errsv = get_errc();
  set_errc(0);
  if (sub->setVal(p, n) == n)
    ec.clear();
  else
    ec = util::make_error_code();
  set_errc(errsv);
}

void
Device::Impl_::OnWrite(uint16_t idx, uint8_t subidx) {
  self->OnWrite(idx, subidx);

  if (on_write) {
    auto f = on_write;
    util::UnlockGuard<Impl_> unlock(*this);
    f(idx, subidx);
  }

  uint8_t id = 0;
  ::std::error_code ec;
  ::std::tie(id, idx, subidx) = RpdoMapping(idx, subidx, ec);
  if (!ec) {
    self->OnRpdoWrite(id, idx, subidx);

    if (on_rpdo_write) {
      auto f = on_rpdo_write;
      util::UnlockGuard<Impl_> unlock(*this);
      f(id, idx, subidx);
    }
  }
}

}  // namespace canopen

}  // namespace lely
