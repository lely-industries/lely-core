/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the CANopen device description.
 *
 * @see lely/coapp/device.hpp
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
#include <lely/co/csdo.h>
#if !LELY_NO_CO_DCF
#include <lely/co/dcf.h>
#endif
#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/pdo.h>
#include <lely/co/val.h>
#include <lely/coapp/device.hpp>
#if !LELY_NO_CO_RPDO && !LELY_NO_CO_MPDO
#include <lely/util/bits.h>
#endif
#include <lely/util/error.hpp>

#if !LELY_NO_CO_RPDO && !LELY_NO_CO_MPDO
#include <algorithm>
#endif
#if !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO
#include <map>
#endif
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace lely {

namespace canopen {

/// The internal implementation of the CANopen device description.
struct Device::Impl_ : util::BasicLockable {
  struct DeviceDeleter {
    void
    operator()(co_dev_t* dev) const noexcept {
      co_dev_destroy(dev);
    }
  };

#if !LELY_NO_CO_DCF
  Impl_(Device* self, const ::std::string& dcf_txt,
        const ::std::string& dcf_bin, uint8_t id, util::BasicLockable* mutex);
#endif
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
    return co_dev_get_netid(dev.get());
  }

  uint8_t
  id() const noexcept {
    return co_dev_get_id(dev.get());
  }

  void OnWrite(uint16_t idx, uint8_t subidx);

  ::std::tuple<uint16_t, uint8_t>
  RpdoMapping(uint8_t id, uint16_t idx, uint8_t subidx,
              ::std::error_code& ec) const noexcept {
    (void)id;
#if !LELY_NO_CO_RPDO
    auto it = rpdo_mapping.find((static_cast<uint32_t>(id) << 24) |
                                (static_cast<uint32_t>(idx) << 8) | subidx);
    if (it != rpdo_mapping.end()) {
      idx = (it->second >> 8) & 0xffff;
      subidx = it->second & 0xff;
      ec.clear();
    } else {
#endif
      idx = 0;
      subidx = 0;
      ec = SdoErrc::NO_PDO;
#if !LELY_NO_CO_RPDO
    }
#endif
    return ::std::make_tuple(idx, subidx);
  }

  ::std::tuple<uint8_t, uint16_t, uint8_t>
  RpdoMapping(uint16_t idx, uint8_t subidx,
              ::std::error_code& ec) const noexcept {
    uint8_t id = 0;
#if !LELY_NO_CO_RPDO
    auto it = rpdo_mapping.find((static_cast<uint32_t>(idx) << 8) | subidx);
    if (it != rpdo_mapping.end()) {
      id = (it->second >> 24) & 0xff;
      idx = (it->second >> 8) & 0xffff;
      subidx = it->second & 0xff;
      ec.clear();
    } else {
#endif
      idx = 0;
      subidx = 0;
      ec = SdoErrc::NO_PDO;
#if !LELY_NO_CO_RPDO
    }
#endif
    return ::std::make_tuple(id, idx, subidx);
  }

  ::std::tuple<uint16_t, uint8_t>
  TpdoMapping(uint8_t id, uint16_t idx, uint8_t subidx,
              ::std::error_code& ec) const noexcept {
    (void)id;
#if !LELY_NO_CO_TPDO
    auto it = tpdo_mapping.find((static_cast<uint32_t>(id) << 24) |
                                (static_cast<uint32_t>(idx) << 8) | subidx);
    if (it != tpdo_mapping.end()) {
      idx = (it->second >> 8) & 0xffff;
      subidx = it->second & 0xff;
      ec.clear();
    } else {
#endif
      idx = 0;
      subidx = 0;
      ec = SdoErrc::NO_PDO;
#if !LELY_NO_CO_TPDO
    }
#endif
    return ::std::make_tuple(idx, subidx);
  }

  Device* self;

  BasicLockable* mutex{nullptr};

  ::std::unique_ptr<co_dev_t, DeviceDeleter> dev;

#if !LELY_NO_CO_RPDO
  ::std::map<uint32_t, uint32_t> rpdo_mapping;
#endif
#if !LELY_NO_CO_TPDO
  ::std::map<uint32_t, uint32_t> tpdo_mapping;
#endif

  ::std::function<void(uint16_t, uint8_t)> on_write;
#if !LELY_NO_CO_LSS
  ::std::function<void(uint8_t, uint16_t, uint8_t)> on_rpdo_write;
#endif
};

#if !LELY_NO_CO_DCF
Device::Device(const ::std::string& dcf_txt, const ::std::string& dcf_bin,
               uint8_t id, util::BasicLockable* mutex)
    : impl_(new Impl_(this, dcf_txt, dcf_bin, id, mutex)) {}
#endif

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
OnDnCon(co_csdo_t*, uint16_t, uint8_t, uint32_t ac, void* data) noexcept {
  *static_cast<uint32_t*>(data) = ac;
}

template <class T>
void
OnUpCon(co_csdo_t*, uint16_t, uint8_t, uint32_t ac, const void* ptr, size_t n,
        void* data) noexcept {
  using traits = canopen_traits<T>;
  using c_type = typename traits::c_type;

  auto t = static_cast<::std::tuple<uint32_t&, T&>*>(data);

  auto val = c_type();
  if (!ac) {
    ::std::error_code ec;
    val = traits::construct(ptr, n, ec);
    if (ec) ac = static_cast<uint32_t>(sdo_errc(ec));
  }

  *t = ::std::forward_as_tuple(ac, traits::from_c_type(val));

  traits::destroy(val);
}

}  // namespace

template <class T>
typename ::std::enable_if<is_canopen<T>::value, T>::type
Device::Read(uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  T value(Read<T>(idx, subidx, ec));
  if (ec) throw_sdo_error(id(), idx, subidx, ec, "Read");
  return value;
}

template <class T>
typename ::std::enable_if<is_canopen<T>::value, T>::type
Device::Read(uint16_t idx, uint8_t subidx, ::std::error_code& ec) const {
  uint32_t ac = 0;
  T value = T();
  auto t = ::std::tie(ac, value);

  ::std::lock_guard<Impl_> lock(*impl_);
  int errsv = get_errc();
  set_errc(0);
  if (!co_dev_up_req(dev(), idx, subidx, &OnUpCon<T>, &t)) {
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
typename ::std::enable_if<is_canopen<T>::value>::type
Device::Write(uint16_t idx, uint8_t subidx, const T& value) {
  ::std::error_code ec;
  Write(idx, subidx, value, ec);
  if (ec) throw_sdo_error(id(), idx, subidx, ec, "Write");
}

template <class T>
typename ::std::enable_if<is_canopen<T>::value>::type
Device::Write(uint16_t idx, uint8_t subidx, const T& value,
              ::std::error_code& ec) {
  using traits = canopen_traits<T>;

  auto val = traits::to_c_type(value, ec);
  if (ec) return;
  uint32_t ac = 0;

  {
    ::std::lock_guard<Impl_> lock(*impl_);
    int errsv = get_errc();
    set_errc(0);
    if (!co_dev_dn_val_req(dev(), idx, subidx, traits::index, &val, &OnDnCon,
                           &ac)) {
      if (ac)
        ec = static_cast<SdoErrc>(ac);
      else
        ec.clear();
    } else {
      ec = util::make_error_code();
    }
    set_errc(errsv);
  }

  traits::destroy(val);
}

template <>
void
Device::Write(uint16_t idx, uint8_t subidx, const ::std::string& value,
              ::std::error_code& ec) {
  Write(idx, subidx, value.c_str(), ec);
}

template <>
void
Device::Write(uint16_t idx, uint8_t subidx, const ::std::vector<uint8_t>& value,
              ::std::error_code& ec) {
  Write(idx, subidx, value.data(), value.size(), ec);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// BOOLEAN
template bool Device::Read<bool>(uint16_t, uint8_t) const;
template bool Device::Read<bool>(uint16_t, uint8_t, ::std::error_code&) const;
template void Device::Write<bool>(uint16_t, uint8_t, const bool&);
template void Device::Write<bool>(uint16_t, uint8_t, const bool&,
                                  ::std::error_code&);

// INTEGER8
template int8_t Device::Read<int8_t>(uint16_t, uint8_t) const;
template int8_t Device::Read<int8_t>(uint16_t, uint8_t,
                                     ::std::error_code&) const;
template void Device::Write<int8_t>(uint16_t, uint8_t, const int8_t&);
template void Device::Write<int8_t>(uint16_t, uint8_t, const int8_t&,
                                    ::std::error_code&);

// INTEGER16
template int16_t Device::Read<int16_t>(uint16_t, uint8_t) const;
template int16_t Device::Read<int16_t>(uint16_t, uint8_t,
                                       ::std::error_code&) const;
template void Device::Write<int16_t>(uint16_t, uint8_t, const int16_t&);
template void Device::Write<int16_t>(uint16_t, uint8_t, const int16_t&,
                                     ::std::error_code&);

// INTEGER32
template int32_t Device::Read<int32_t>(uint16_t, uint8_t) const;
template int32_t Device::Read<int32_t>(uint16_t, uint8_t,
                                       ::std::error_code&) const;
template void Device::Write<int32_t>(uint16_t, uint8_t, const int32_t&);
template void Device::Write<int32_t>(uint16_t, uint8_t, const int32_t&,
                                     ::std::error_code&);

// UNSIGNED8
template uint8_t Device::Read<uint8_t>(uint16_t, uint8_t) const;
template uint8_t Device::Read<uint8_t>(uint16_t, uint8_t,
                                       ::std::error_code&) const;
template void Device::Write<uint8_t>(uint16_t, uint8_t, const uint8_t&);
template void Device::Write<uint8_t>(uint16_t, uint8_t, const uint8_t&,
                                     ::std::error_code&);

// UNSIGNED16
template uint16_t Device::Read<uint16_t>(uint16_t, uint8_t) const;
template uint16_t Device::Read<uint16_t>(uint16_t, uint8_t,
                                         ::std::error_code&) const;
template void Device::Write<uint16_t>(uint16_t, uint8_t, const uint16_t&);
template void Device::Write<uint16_t>(uint16_t, uint8_t, const uint16_t&,
                                      ::std::error_code&);

// UNSIGNED32
template uint32_t Device::Read<uint32_t>(uint16_t, uint8_t) const;
template uint32_t Device::Read<uint32_t>(uint16_t, uint8_t,
                                         ::std::error_code&) const;
template void Device::Write<uint32_t>(uint16_t, uint8_t, const uint32_t&);
template void Device::Write<uint32_t>(uint16_t, uint8_t, const uint32_t&,
                                      ::std::error_code&);

// REAL32
template float Device::Read<float>(uint16_t, uint8_t) const;
template float Device::Read<float>(uint16_t, uint8_t, ::std::error_code&) const;
template void Device::Write<float>(uint16_t, uint8_t, const float&);
template void Device::Write<float>(uint16_t, uint8_t, const float&,
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
template void Device::Write<double>(uint16_t, uint8_t, const double&);
template void Device::Write<double>(uint16_t, uint8_t, const double&,
                                    ::std::error_code&);

// INTEGER40
// INTEGER48
// INTEGER56

// INTEGER64
template int64_t Device::Read<int64_t>(uint16_t, uint8_t) const;
template int64_t Device::Read<int64_t>(uint16_t, uint8_t,
                                       ::std::error_code&) const;
template void Device::Write<int64_t>(uint16_t, uint8_t, const int64_t&);
template void Device::Write<int64_t>(uint16_t, uint8_t, const int64_t&,
                                     ::std::error_code&);

// UNSIGNED24
// UNSIGNED40
// UNSIGNED48
// UNSIGNED56

// UNSIGNED64
template uint64_t Device::Read<uint64_t>(uint16_t, uint8_t) const;
template uint64_t Device::Read<uint64_t>(uint16_t, uint8_t,
                                         ::std::error_code&) const;
template void Device::Write<uint64_t>(uint16_t, uint8_t, const uint64_t&);
template void Device::Write<uint64_t>(uint16_t, uint8_t, const uint64_t&,
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
  using traits = canopen_traits<::std::basic_string<char16_t>>;

  auto val = traits::to_c_type(value, ec);
  if (ec) return;
  uint32_t ac = 0;

  {
    ::std::lock_guard<Impl_> lock(*impl_);
    int errsv = get_errc();
    set_errc(0);
    if (!co_dev_dn_val_req(dev(), idx, subidx, traits::index, &val, &OnDnCon,
                           &ac)) {
      if (ac)
        ec = static_cast<SdoErrc>(ac);
      else
        ec.clear();
    } else {
      ec = util::make_error_code();
    }
    set_errc(errsv);
  }

  traits::destroy(val);
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
  if (!co_dev_dn_req(dev(), idx, subidx, p, n, &OnDnCon, &ac)) {
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
Device::WriteDcf(const uint8_t* begin, const uint8_t* end) {
  ::std::error_code ec;
  WriteDcf(begin, end, ec);
  if (ec) throw_sdo_error(id(), 0, 0, ec, "WriteDcf");
}

void
Device::WriteDcf(const uint8_t* begin, const uint8_t* end,
                 ::std::error_code& ec) {
  uint32_t ac = 0;

  ::std::lock_guard<Impl_> lock(*impl_);
  int errsv = get_errc();
  set_errc(0);
  if (!co_dev_dn_dcf_req(dev(), begin, end, &OnDnCon, &ac)) {
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
Device::WriteDcf(const char* path) {
  ::std::error_code ec;
  WriteDcf(path, ec);
  if (ec) throw_sdo_error(id(), 0, 0, ec, "WriteDcf");
}

#if !LELY_NO_STDIO
void
Device::WriteDcf(const char* path, ::std::error_code& ec) {
  int errsv = get_errc();
  set_errc(0);

  void* dom = nullptr;
  if (co_val_read_file(CO_DEFTYPE_DOMAIN, &dom, path)) {
    auto begin =
        static_cast<const uint8_t*>(co_val_addressof(CO_DEFTYPE_DOMAIN, &dom));
    auto end = begin + co_val_sizeof(CO_DEFTYPE_DOMAIN, &dom);
    WriteDcf(begin, end, ec);
  } else {
    ec = util::make_error_code();
  }
  if (dom) co_val_fini(CO_DEFTYPE_DOMAIN, &dom);
  set_errc(errsv);
}
#endif

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
typename ::std::enable_if<is_canopen_basic<T>::value, T>::type
Device::RpdoRead(uint8_t id, uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  T value(RpdoRead<T>(id, idx, subidx, ec));
  if (ec) throw_sdo_error(id, idx, subidx, ec, "RpdoRead");
  return value;
}

template <class T>
typename ::std::enable_if<is_canopen_basic<T>::value, T>::type
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
typename ::std::enable_if<is_canopen_basic<T>::value, T>::type
Device::TpdoRead(uint8_t id, uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  T value(TpdoRead<T>(id, idx, subidx, ec));
  if (ec) throw_sdo_error(id, idx, subidx, ec, "TpdoRead");
  return value;
}

template <class T>
typename ::std::enable_if<is_canopen_basic<T>::value, T>::type
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
typename ::std::enable_if<is_canopen_basic<T>::value>::type
Device::TpdoWrite(uint8_t id, uint16_t idx, uint8_t subidx, T value) {
  ::std::error_code ec;
  TpdoWrite(id, idx, subidx, value, ec);
  if (ec) throw_sdo_error(id, idx, subidx, ec, "TpdoWrite");
}

template <class T>
typename ::std::enable_if<is_canopen_basic<T>::value>::type
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
#if LELY_NO_CO_RPDO
  (void)on_rpdo_write;
#else
  ::std::lock_guard<Impl_> lock(*impl_);
  impl_->on_rpdo_write = on_rpdo_write;
#endif
}

co_dev_t*
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
Device::Type(uint16_t idx, uint8_t subidx,
             ::std::error_code& ec) const noexcept {
  auto obj = co_dev_find_obj(dev(), idx);
  if (!obj) {
    ec = SdoErrc::NO_OBJ;
    return typeid(void);
  }

  auto sub = co_obj_find_sub(obj, subidx);
  if (!sub) {
    ec = SdoErrc::NO_SUB;
    return typeid(void);
  }

  ec.clear();
  switch (co_sub_get_type(sub)) {
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
    // case CO_DEFTYPE_TIME_DIFF: ...
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
typename ::std::enable_if<is_canopen<T>::value, T>::type
Device::Get(uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  auto value = Get<T>(idx, subidx, ec);
  if (ec) throw_sdo_error(impl_->id(), idx, subidx, ec, "Get");
  return value;
}

template <class T>
typename ::std::enable_if<is_canopen<T>::value, T>::type
Device::Get(uint16_t idx, uint8_t subidx,
            ::std::error_code& ec) const noexcept {
  using traits = canopen_traits<T>;
  using c_type = typename traits::c_type;

  auto obj = co_dev_find_obj(dev(), idx);
  if (!obj) {
    ec = SdoErrc::NO_OBJ;
    return T();
  }

  auto sub = co_obj_find_sub(obj, subidx);
  if (!sub) {
    ec = SdoErrc::NO_SUB;
    return T();
  }

  if (!is_canopen_same(traits::index, co_sub_get_type(sub))) {
    ec = SdoErrc::TYPE_LEN;
    return T();
  }

  ec.clear();

  auto pval = static_cast<const c_type*>(co_sub_get_val(sub));
  return traits::from_c_type(*pval);
}

template <class T>
typename ::std::enable_if<is_canopen<T>::value>::type
Device::Set(uint16_t idx, uint8_t subidx, const T& value) {
  ::std::error_code ec;
  Set(idx, subidx, value, ec);
  if (ec) throw_sdo_error(impl_->id(), idx, subidx, ec, "Set");
}

template <class T>
typename ::std::enable_if<is_canopen<T>::value>::type
Device::Set(uint16_t idx, uint8_t subidx, const T& value,
            ::std::error_code& ec) noexcept {
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

  auto val = traits::to_c_type(value, ec);
  if (ec) return;
  auto p = traits::address(val);
  auto n = traits::size(val);

  int errsv = get_errc();
  set_errc(0);
  if (co_sub_set_val(sub, p, n) == n)
    ec.clear();
  else
    ec = util::make_error_code();
  set_errc(errsv);

  traits::destroy(val);
}

template <>
void
Device::Set(uint16_t idx, uint8_t subidx, const ::std::string& value,
            ::std::error_code& ec) noexcept {
  Set(idx, subidx, value.c_str(), ec);
}

template <>
void
Device::Set(uint16_t idx, uint8_t subidx, const ::std::vector<uint8_t>& value,
            ::std::error_code& ec) noexcept {
  Set(idx, subidx, value.data(), value.size(), ec);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// BOOLEAN
template bool Device::Get<bool>(uint16_t, uint8_t) const;
template bool Device::Get<bool>(uint16_t, uint8_t,
                                ::std::error_code&) const noexcept;
template void Device::Set<bool>(uint16_t, uint8_t, const bool&);
template void Device::Set<bool>(uint16_t, uint8_t, const bool&,
                                ::std::error_code&) noexcept;

// INTEGER8
template int8_t Device::Get<int8_t>(uint16_t, uint8_t) const;
template int8_t Device::Get<int8_t>(uint16_t, uint8_t,
                                    ::std::error_code&) const noexcept;
template void Device::Set<int8_t>(uint16_t, uint8_t, const int8_t&);
template void Device::Set<int8_t>(uint16_t, uint8_t, const int8_t&,
                                  ::std::error_code&) noexcept;

// INTEGER16
template int16_t Device::Get<int16_t>(uint16_t, uint8_t) const;
template int16_t Device::Get<int16_t>(uint16_t, uint8_t,
                                      ::std::error_code&) const noexcept;
template void Device::Set<int16_t>(uint16_t, uint8_t, const int16_t&);
template void Device::Set<int16_t>(uint16_t, uint8_t, const int16_t&,
                                   ::std::error_code&) noexcept;

// INTEGER32
template int32_t Device::Get<int32_t>(uint16_t, uint8_t) const;
template int32_t Device::Get<int32_t>(uint16_t, uint8_t,
                                      ::std::error_code&) const noexcept;
template void Device::Set<int32_t>(uint16_t, uint8_t, const int32_t&);
template void Device::Set<int32_t>(uint16_t, uint8_t, const int32_t&,
                                   ::std::error_code&) noexcept;

// UNSIGNED8
template uint8_t Device::Get<uint8_t>(uint16_t, uint8_t) const;
template uint8_t Device::Get<uint8_t>(uint16_t, uint8_t,
                                      ::std::error_code&) const noexcept;
template void Device::Set<uint8_t>(uint16_t, uint8_t, const uint8_t&);
template void Device::Set<uint8_t>(uint16_t, uint8_t, const uint8_t&,
                                   ::std::error_code&) noexcept;

// UNSIGNED16
template uint16_t Device::Get<uint16_t>(uint16_t, uint8_t) const;
template uint16_t Device::Get<uint16_t>(uint16_t, uint8_t,
                                        ::std::error_code&) const noexcept;
template void Device::Set<uint16_t>(uint16_t, uint8_t, const uint16_t&);
template void Device::Set<uint16_t>(uint16_t, uint8_t, const uint16_t&,
                                    ::std::error_code&) noexcept;

// UNSIGNED32
template uint32_t Device::Get<uint32_t>(uint16_t, uint8_t) const;
template uint32_t Device::Get<uint32_t>(uint16_t, uint8_t,
                                        ::std::error_code&) const noexcept;
template void Device::Set<uint32_t>(uint16_t, uint8_t, const uint32_t&);
template void Device::Set<uint32_t>(uint16_t, uint8_t, const uint32_t&,
                                    ::std::error_code&) noexcept;

// REAL32
template float Device::Get<float>(uint16_t, uint8_t) const;
template float Device::Get<float>(uint16_t, uint8_t,
                                  ::std::error_code&) const noexcept;
template void Device::Set<float>(uint16_t, uint8_t, const float&);
template void Device::Set<float>(uint16_t, uint8_t, const float&,
                                 ::std::error_code&) noexcept;

// VISIBLE_STRING
template ::std::string Device::Get<::std::string>(uint16_t, uint8_t) const;
template ::std::string Device::Get<::std::string>(
    uint16_t, uint8_t, ::std::error_code&) const noexcept;
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
template double Device::Get<double>(uint16_t, uint8_t,
                                    ::std::error_code&) const noexcept;
template void Device::Set<double>(uint16_t, uint8_t, const double&);
template void Device::Set<double>(uint16_t, uint8_t, const double&,
                                  ::std::error_code&) noexcept;

// INTEGER40
// INTEGER48
// INTEGER56

// INTEGER64
template int64_t Device::Get<int64_t>(uint16_t, uint8_t) const;
template int64_t Device::Get<int64_t>(uint16_t, uint8_t,
                                      ::std::error_code&) const noexcept;
template void Device::Set<int64_t>(uint16_t, uint8_t, const int64_t&);
template void Device::Set<int64_t>(uint16_t, uint8_t, const int64_t&,
                                   ::std::error_code&) noexcept;

// UNSIGNED24
// UNSIGNED40
// UNSIGNED48
// UNSIGNED56

// UNSIGNED64
template uint64_t Device::Get<uint64_t>(uint16_t, uint8_t) const;
template uint64_t Device::Get<uint64_t>(uint16_t, uint8_t,
                                        ::std::error_code&) const noexcept;
template void Device::Set<uint64_t>(uint16_t, uint8_t, const uint64_t&);
template void Device::Set<uint64_t>(uint16_t, uint8_t, const uint64_t&,
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
  Set(idx, subidx, value, ::std::char_traits<char>::length(value), ec);
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
  using traits = canopen_traits<::std::basic_string<char16_t>>;

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

  auto val = traits::to_c_type(value, ec);
  if (ec) return;
  auto p = traits::address(val);
  auto n = traits::size(val);

  int errsv = get_errc();
  set_errc(0);
  if (co_sub_set_val(sub, p, n) == n)
    ec.clear();
  else
    ec = util::make_error_code();
  set_errc(errsv);

  traits::destroy(val);
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

  int errsv = get_errc();
  set_errc(0);
  if (co_sub_set_val(sub, p, n) == n)
    ec.clear();
  else
    ec = util::make_error_code();
  set_errc(errsv);
}

const char*
Device::GetUploadFile(uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  auto filename = GetUploadFile(idx, subidx, ec);
  if (ec) throw_sdo_error(impl_->id(), idx, subidx, ec, "GetUploadFile");
  return filename;
}

const char*
Device::GetUploadFile(uint16_t idx, uint8_t subidx,
                      ::std::error_code& ec) const noexcept {
  auto obj = co_dev_find_obj(dev(), idx);
  if (!obj) {
    ec = SdoErrc::NO_OBJ;
    return nullptr;
  }

  auto sub = co_obj_find_sub(obj, subidx);
  if (!sub) {
    ec = SdoErrc::NO_SUB;
    return nullptr;
  }

  ec.clear();
  return co_sub_get_upload_file(sub);
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

  int errsv = get_errc();
  set_errc(0);
  if (!co_sub_set_upload_file(sub, filename))
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
  auto obj = co_dev_find_obj(dev(), idx);
  if (!obj) {
    ec = SdoErrc::NO_OBJ;
    return nullptr;
  }

  auto sub = co_obj_find_sub(obj, subidx);
  if (!sub) {
    ec = SdoErrc::NO_SUB;
    return nullptr;
  }

  ec.clear();
  return co_sub_get_download_file(sub);
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

  int errsv = get_errc();
  set_errc(0);
  if (!co_sub_set_download_file(sub, filename))
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

#if !LELY_NO_CO_TPDO
  co_dev_tpdo_event(dev(), idx, subidx);
#if !LELYY_NO_CO_MPDO
  co_dev_sam_mpdo_event(dev(), idx, subidx);
#endif
#endif  // !LELY_NO_CO_TPDO
}

template <class T>
typename ::std::enable_if<is_canopen_basic<T>::value, T>::type
Device::RpdoGet(uint8_t id, uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  auto value = RpdoGet<T>(id, idx, subidx, ec);
  if (ec) throw_sdo_error(id, idx, subidx, ec, "RpdoGet");
  return value;
}

template <class T>
typename ::std::enable_if<is_canopen_basic<T>::value, T>::type
Device::RpdoGet(uint8_t id, uint16_t idx, uint8_t subidx,
                ::std::error_code& ec) const noexcept {
  ec.clear();
  ::std::tie(idx, subidx) = impl_->RpdoMapping(id, idx, subidx, ec);
  if (ec) return T();
  return Get<T>(idx, subidx, ec);
}

template <class T>
typename ::std::enable_if<is_canopen_basic<T>::value, T>::type
Device::TpdoGet(uint8_t id, uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  auto value = TpdoGet<T>(id, idx, subidx, ec);
  if (ec) throw_sdo_error(id, idx, subidx, ec, "TpdoGet");
  return value;
}

template <class T>
typename ::std::enable_if<is_canopen_basic<T>::value, T>::type
Device::TpdoGet(uint8_t id, uint16_t idx, uint8_t subidx,
                ::std::error_code& ec) const noexcept {
  ec.clear();
  ::std::tie(idx, subidx) = impl_->TpdoMapping(id, idx, subidx, ec);
  if (ec) return T();
  return Get<T>(idx, subidx, ec);
}

template <class T>
typename ::std::enable_if<is_canopen_basic<T>::value>::type
Device::TpdoSet(uint8_t id, uint16_t idx, uint8_t subidx, T value) {
  ::std::error_code ec;
  TpdoSet(id, idx, subidx, value, ec);
  if (ec) throw_sdo_error(id, idx, subidx, ec, "TpdoSet");
}

template <class T>
typename ::std::enable_if<is_canopen_basic<T>::value>::type
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
#if !LELY_NO_CO_RPDO
  impl_->rpdo_mapping.clear();

  // Loop over all RPDOs.
  co_obj_t* obj_1400 = nullptr;
  for (int i = 0; !obj_1400 && i < 512; i++)
    obj_1400 = co_dev_find_obj(dev(), 0x1400 + i);
  for (; obj_1400; obj_1400 = co_obj_next(obj_1400)) {
    int i = co_obj_get_idx(obj_1400) - 0x1400;
    if (i >= 512) break;
    // Skip invalid PDOs.
    auto cobid = co_obj_get_val_u32(obj_1400, 1);
    if (cobid & CO_PDO_COBID_VALID) continue;
    // Obtain the remote node-ID.
    uint8_t id = 0;
    auto obj_5800 = co_dev_find_obj(dev(), 0x5800 + i);
    if (obj_5800) {
      id = co_obj_get_val_u32(obj_5800, 0) & 0xff;
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
    auto obj_1600 = co_dev_find_obj(dev(), 0x1600 + i);
    if (!obj_1600) continue;
    // Obtain the remote TPDO mapping.
    auto obj_5a00 = co_dev_find_obj(dev(), 0x5a00 + i);
    if (!obj_5a00) continue;
    // Check if the number of mapped objects is the same.
    auto n = co_obj_get_val_u8(obj_1600, 0);
    if (!n || n > CO_PDO_NUM_MAPS) continue;
    if (n != co_obj_get_val_u8(obj_5a00, 0)) continue;
    for (int j = 1; j <= n; j++) {
      auto rmap = co_obj_get_val_u32(obj_1600, j);
      auto tmap = co_obj_get_val_u32(obj_5a00, j);
      // Ignore empty mapping entries.
      if (!rmap && !tmap) continue;
      // Check if the mapped objects have the same length.
      if ((rmap & 0xff) != (tmap & 0xff)) break;
      rmap >>= 8;
      tmap >>= 8;
      // Skip dummy-mapped objects.
      if (co_type_is_basic((rmap >> 8) & 0xffff)) continue;
      tmap |= static_cast<uint32_t>(id) << 24;
      impl_->rpdo_mapping[tmap] = rmap;
      // Store the reverse mapping for OnRpdoWrite().
      impl_->rpdo_mapping[rmap] = tmap;
    }
  }

#if !LELY_NO_CO_MPDO
  // Check if at least one RPDO is a SAM-MPDO consumer.
  bool has_sam_mpdo = false;
  // Loop over all RPDOs.
  obj_1400 = nullptr;
  for (int i = 0; !obj_1400 && i < CO_NUM_PDOS; i++)
    obj_1400 = co_dev_find_obj(dev(), 0x1400 + i);
  for (; !has_sam_mpdo && obj_1400; obj_1400 = co_obj_next(obj_1400)) {
    int i = co_obj_get_idx(obj_1400) - 0x1400;
    if (i >= CO_NUM_PDOS) break;
    // Skip invalid PDOs.
    if (co_obj_get_val_u32(obj_1400, 1) & CO_PDO_COBID_VALID) continue;
    // SAM-MPDOs MUST be event-driven.
    if (co_obj_get_val_u8(obj_1400, 2) < 0xfe) continue;
    // Check if the PDO is a SAM-MPDO.
    if (co_dev_get_val_u8(dev(), 0x1600 + i, 0) != CO_PDO_MAP_SAM_MPDO)
      continue;
    has_sam_mpdo = true;
  }

  if (has_sam_mpdo) {
    // Loop over the object dispatching list (object 1FD0..1FFF).
    co_obj_t* obj_1fd0 = nullptr;
    for (int i = 0; !obj_1fd0 && i < 48; i++)
      obj_1fd0 = co_dev_find_obj(dev(), 0x1fd0 + i);
    for (; obj_1fd0; obj_1fd0 = co_obj_next(obj_1fd0)) {
      if (co_obj_get_idx(obj_1fd0) > 0x1fff) break;
      auto n = co_obj_get_val_u8(obj_1fd0, 0);
      if (!n) continue;
      auto sub = co_sub_next(co_obj_first_sub(obj_1fd0));
      for (; sub && co_sub_get_subidx(sub) <= n; sub = co_sub_next(sub)) {
        auto val = co_sub_get_val_u64(sub);
        if (!val) continue;
        // Extract the mapping.
        uint32_t rmap = (val << 8) >> 40;
        uint32_t tmap = ror32(val, 8);
        // Skip dummy-mapped objects.
        if (co_type_is_basic((rmap >> 8) & 0xffff)) continue;
        // Extract the block of sub-indices.
        uint8_t min = (val >> 8) & 0xff;
        uint8_t max = min;
        uint8_t blk = (val >> 56) & 0xff;
        if (blk) max += ::std::min(blk - 1, 0xff - min);
        // Store the (reverse) mapping for each of the sub-indices.
        for (int j = 0; j <= max - min; j++) {
          // Ignore out-of-range sub-indices.
          if (static_cast<uint8_t>(j) > 0xff - (rmap & 0xff)) break;
          impl_->rpdo_mapping[tmap + j] = rmap + j;
        }
      }
    }
  }
#endif  // !LELY_NO_CO_MPDO
#endif  // !LELY_NO_CO_RPDO
}

void
Device::UpdateTpdoMapping() {
#if !LELY_NO_CO_TPDO
  impl_->tpdo_mapping.clear();

  // Loop over all TPDOs.
  co_obj_t* obj_1800 = nullptr;
  for (int i = 0; !obj_1800 && i < 512; i++)
    obj_1800 = co_dev_find_obj(dev(), 0x1800 + i);
  for (; obj_1800; obj_1800 = co_obj_next(obj_1800)) {
    int i = co_obj_get_idx(obj_1800) - 0x1800;
    if (i >= 512) break;
    // Skip invalid PDOs.
    auto cobid = co_obj_get_val_u32(obj_1800, 1);
    if (cobid & CO_PDO_COBID_VALID) continue;
    // Obtain the remote node-ID.
    uint8_t id = 0;
    auto obj_5c00 = co_dev_find_obj(dev(), 0x5c00 + i);
    if (obj_5c00) {
      id = co_obj_get_val_u32(obj_5c00, 0) & 0xff;
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
    auto obj_1a00 = co_dev_find_obj(dev(), 0x1a00 + i);
    if (!obj_1a00) continue;
    // Obtain the remote RPDO mapping.
    auto obj_5e00 = co_dev_find_obj(dev(), 0x5e00 + i);
    if (!obj_5e00) continue;
    // Check if the number of mapped objects is the same.
    auto n = co_obj_get_val_u8(obj_1a00, 0);
    if (!n || n > CO_PDO_NUM_MAPS) continue;
    if (n != co_obj_get_val_u8(obj_5e00, 0)) continue;
    for (int j = 1; j <= n; j++) {
      auto tmap = co_obj_get_val_u32(obj_1a00, j);
      auto rmap = co_obj_get_val_u32(obj_5e00, j);
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
#endif  // !LELY_NO_CO_TPDO
}

void
Device::RpdoWrite(uint8_t id, uint16_t idx, uint8_t subidx) {
#if LELY_NO_CO_RPDO
  (void)id;
  (void)idx;
  (void)subidx;
#else
  OnRpdoWrite(id, idx, subidx);

  if (impl_->on_rpdo_write) {
    auto f = impl_->on_rpdo_write;
    util::UnlockGuard<Impl_> unlock(*impl_);
    f(id, idx, subidx);
  }
#endif
}

#if !LELY_NO_CO_DCF
Device::Impl_::Impl_(Device* self_, const ::std::string& dcf_txt,
                     const ::std::string& dcf_bin, uint8_t id,
                     util::BasicLockable* mutex_)
    : self(self_),
      mutex(mutex_),
      dev(co_dev_create_from_dcf_file(dcf_txt.c_str())) {
  if (!dcf_bin.empty() &&
      co_dev_read_dcf_file(dev.get(), nullptr, nullptr, dcf_bin.c_str()) == -1)
    util::throw_errc("Device");

  if (id != 0xff && co_dev_set_id(dev.get(), id) == -1)
    util::throw_errc("Device");

  // Register a notification function for all objects in the object dictionary
  // in case of write (SDO upload) access.
  for (auto obj = co_dev_first_obj(dev.get()); obj; obj = co_obj_next(obj)) {
    // Skip data types and the communication profile area.
    if (co_obj_get_idx(obj) < 0x2000) continue;
    // Skip reserved objects.
    if (co_obj_get_idx(obj) >= 0xC000) break;
    co_obj_set_dn_ind(
        obj,
        [](co_sub_t* sub, co_sdo_req* req, void* data) -> uint32_t {
          // Implement the default behavior, but do not issue a notification for
          // incomplete or failed writes.
          uint32_t ac = 0;
          if (co_sub_on_dn(sub, req, &ac) == -1 || ac) return ac;
          auto impl_ = static_cast<Impl_*>(data);
          impl_->OnWrite(co_obj_get_idx(co_sub_get_obj(sub)),
                         co_sub_get_subidx(sub));
          return 0;
        },
        static_cast<void*>(this));
  }
}
#endif  // !LELY_NO_CO_DCF

void
Device::Impl_::OnWrite(uint16_t idx, uint8_t subidx) {
  self->OnWrite(idx, subidx);

  if (on_write) {
    auto f = on_write;
    util::UnlockGuard<Impl_> unlock(*this);
    f(idx, subidx);
  }

#if !LELY_NO_CO_RPDO
  uint8_t id = 0;
  ::std::error_code ec;
  ::std::tie(id, idx, subidx) = RpdoMapping(idx, subidx, ec);
  if (!ec) self->RpdoWrite(id, idx, subidx);
#endif
}

}  // namespace canopen

}  // namespace lely
