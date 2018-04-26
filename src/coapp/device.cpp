/*!\file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the CANopen device description.
 *
 * \see lely/coapp/device.hpp
 *
 * \copyright 2018 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#include <mutex>

#include <lely/co/dcf.hpp>
#include <lely/co/obj.hpp>

namespace lely {

namespace canopen {

//! The internal implementation of the CANopen device description.
struct Device::Impl_ : public BasicLockable {
  Impl_(const ::std::string& dcf_txt, const ::std::string& dcf_bin, uint8_t id,
        BasicLockable* mutex);

  virtual void lock() override { if (mutex) mutex->lock(); }
  virtual void unlock() override { if (mutex) mutex->unlock(); }

  uint8_t netid() const noexcept { return dev->getNetid(); }
  uint8_t id() const noexcept { return dev->getId(); }

  void Set(uint16_t idx, uint8_t subidx, const ::std::string& value,
           ::std::error_code& ec);

  void Set(uint16_t idx, uint8_t subidx, const ::std::vector<uint8_t>& value,
           ::std::error_code& ec);

  void Set(uint16_t idx, uint8_t subidx,
           const ::std::basic_string<char16_t>& value, ::std::error_code& ec);

  template <uint16_t N>
  void Set(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n,
           ::std::error_code& ec);

  BasicLockable* mutex { nullptr };

  unique_c_ptr<CODev> dev;
};

LELY_COAPP_EXPORT Device::Device(const ::std::string& dcf_txt,
    const ::std::string& dcf_bin, uint8_t id, BasicLockable* mutex)
    : impl_(new Impl_(dcf_txt, dcf_bin, id, mutex)) {}

LELY_COAPP_EXPORT Device& Device::operator=(Device&&) = default;

LELY_COAPP_EXPORT Device::~Device() = default;

LELY_COAPP_EXPORT uint8_t
Device::netid() const noexcept {
  ::std::lock_guard<Impl_> lock(*impl_);

  return impl_->netid();
}

LELY_COAPP_EXPORT uint8_t
Device::id() const noexcept {
  ::std::lock_guard<Impl_> lock(*impl_);

  return impl_->id();
}

LELY_COAPP_EXPORT CODev*
Device::dev() const noexcept { return impl_->dev.get(); }

LELY_COAPP_EXPORT const ::std::type_info&
Device::Type(uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  auto& ti = Type(idx, subidx, ec);
  if (ec)
    throw SdoError(impl_->netid(), impl_->id(), idx, subidx, ec, "Type");
  return ti;
}

LELY_COAPP_EXPORT const ::std::type_info&
Device::Type(uint16_t idx, uint8_t subidx, ::std::error_code& ec) const {
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
  case CO_DEFTYPE_BOOLEAN: return typeid(bool);
  case CO_DEFTYPE_INTEGER8: return typeid(int8_t);
  case CO_DEFTYPE_INTEGER16: return typeid(int16_t);
  case CO_DEFTYPE_INTEGER32: return typeid(int32_t);
  case CO_DEFTYPE_UNSIGNED8: return typeid(uint8_t);
  case CO_DEFTYPE_UNSIGNED16: return typeid(uint16_t);
  case CO_DEFTYPE_UNSIGNED32: return typeid(uint32_t);
  case CO_DEFTYPE_REAL32: return typeid(float);
  case CO_DEFTYPE_VISIBLE_STRING: return typeid(::std::string);
  case CO_DEFTYPE_OCTET_STRING: return typeid(::std::vector<uint8_t>);
  case CO_DEFTYPE_UNICODE_STRING: return typeid(::std::basic_string<char16_t>);
  // case CO_DEFTYPE_TIME_OF_DAY: ...
  // case CO_DEFTYPE_TIME_DIFFERENCE: ...
  case CO_DEFTYPE_DOMAIN: return typeid(::std::vector<uint8_t>);
  // case CO_DEFTYPE_INTEGER24: ...
  case CO_DEFTYPE_REAL64: return typeid(double);
  // case CO_DEFTYPE_INTEGER40: ...
  // case CO_DEFTYPE_INTEGER48: ...
  // case CO_DEFTYPE_INTEGER56: ...
  case CO_DEFTYPE_INTEGER64: return typeid(int64_t);
  // case CO_DEFTYPE_UNSIGNED24: ...
  // case CO_DEFTYPE_UNSIGNED40: ...
  // case CO_DEFTYPE_UNSIGNED48: ...
  // case CO_DEFTYPE_UNSIGNED56: ...
  case CO_DEFTYPE_UNSIGNED64: return typeid(uint64_t);
  default: return typeid(void);
  }
}

template <class T>
LELY_COAPP_EXPORT typename ::std::enable_if<
    detail::IsCanopenType<T>::value, T
>::type
Device::Get(uint16_t idx, uint8_t subidx) const {
  ::std::error_code ec;
  auto value = Get<T>(idx, subidx, ec);
  if (ec)
    throw SdoError(impl_->netid(), impl_->id(), idx, subidx, ec, "Get");
  return value;
}

template <class T>
LELY_COAPP_EXPORT typename ::std::enable_if<
    detail::IsCanopenType<T>::value, T
>::type
Device::Get(uint16_t idx, uint8_t subidx, ::std::error_code& ec) const {
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

  if (!detail::IsCanopenSame(N, sub->getType())) {
    ec = SdoErrc::TYPE_LEN;
    return T();
  }

  ec.clear();
  // This is efficient, even for CANopen array values, since getVal<N>() returns
  // a reference.
  return sub->getVal<N>();
}

template <class T>
LELY_COAPP_EXPORT typename ::std::enable_if<
    detail::IsCanopenBasic<T>::value
>::type
Device::Set(uint16_t idx, uint8_t subidx, T value) {
  ::std::error_code ec;
  Set(idx, subidx, value, ec);
  if (ec)
    throw SdoError(impl_->netid(), impl_->id(), idx, subidx, ec, "Set");
}

template <class T>
LELY_COAPP_EXPORT typename ::std::enable_if<
    detail::IsCanopenBasic<T>::value
>::type
Device::Set(uint16_t idx, uint8_t subidx, T value, ::std::error_code& ec) {
  constexpr auto N = co_type_traits_T<T>::index;

  impl_->Set<N>(idx, subidx, &value, sizeof(value), ec);
}

template <class T>
LELY_COAPP_EXPORT typename ::std::enable_if<
    detail::IsCanopenArray<T>::value
>::type
Device::Set(uint16_t idx, uint8_t subidx, const T& value) {
  ::std::error_code ec;
  Set(idx, subidx, value, ec);
  if (ec)
    throw SdoError(impl_->netid(), impl_->id(), idx, subidx, ec, "Set");
}

template <class T>
LELY_COAPP_EXPORT typename ::std::enable_if<
    detail::IsCanopenArray<T>::value
>::type
Device::Set(uint16_t idx, uint8_t subidx, const T& value,
            ::std::error_code& ec) {
  impl_->Set(idx, subidx, value, ec);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// BOOLEAN
template LELY_COAPP_EXPORT bool
Device::Get<bool>(uint16_t, uint8_t) const;
template LELY_COAPP_EXPORT bool
Device::Get<bool>(uint16_t, uint8_t, ::std::error_code&) const;
template LELY_COAPP_EXPORT void
Device::Set<bool>(uint16_t, uint8_t, bool);
template LELY_COAPP_EXPORT void
Device::Set<bool>(uint16_t, uint8_t, bool, ::std::error_code&);

// INTEGER8
template LELY_COAPP_EXPORT int8_t
Device::Get<int8_t>(uint16_t, uint8_t) const;
template LELY_COAPP_EXPORT int8_t
Device::Get<int8_t>(uint16_t, uint8_t, ::std::error_code&) const;
template LELY_COAPP_EXPORT void
Device::Set<int8_t>(uint16_t, uint8_t, int8_t);
template LELY_COAPP_EXPORT void
Device::Set<int8_t>(uint16_t, uint8_t, int8_t, ::std::error_code&);

// INTEGER16
template LELY_COAPP_EXPORT int16_t
Device::Get<int16_t>(uint16_t, uint8_t) const;
template LELY_COAPP_EXPORT int16_t
Device::Get<int16_t>(uint16_t, uint8_t, ::std::error_code&) const;
template LELY_COAPP_EXPORT void
Device::Set<int16_t>(uint16_t, uint8_t, int16_t);
template LELY_COAPP_EXPORT void
Device::Set<int16_t>(uint16_t, uint8_t, int16_t, ::std::error_code&);

// INTEGER32
template LELY_COAPP_EXPORT int32_t
Device::Get<int32_t>(uint16_t, uint8_t) const;
template LELY_COAPP_EXPORT int32_t
Device::Get<int32_t>(uint16_t, uint8_t, ::std::error_code&) const;
template LELY_COAPP_EXPORT void
Device::Set<int32_t>(uint16_t, uint8_t, int32_t);
template LELY_COAPP_EXPORT void
Device::Set<int32_t>(uint16_t, uint8_t, int32_t, ::std::error_code&);

// UNSIGNED8
template LELY_COAPP_EXPORT uint8_t
Device::Get<uint8_t>(uint16_t, uint8_t) const;
template LELY_COAPP_EXPORT uint8_t
Device::Get<uint8_t>(uint16_t, uint8_t, ::std::error_code&) const;
template LELY_COAPP_EXPORT void
Device::Set<uint8_t>(uint16_t, uint8_t, uint8_t);
template LELY_COAPP_EXPORT void
Device::Set<uint8_t>(uint16_t, uint8_t, uint8_t, ::std::error_code&);

// UNSIGNED16
template LELY_COAPP_EXPORT uint16_t
Device::Get<uint16_t>(uint16_t, uint8_t) const;
template LELY_COAPP_EXPORT uint16_t
Device::Get<uint16_t>(uint16_t, uint8_t, ::std::error_code&) const;
template LELY_COAPP_EXPORT void
Device::Set<uint16_t>(uint16_t, uint8_t, uint16_t);
template LELY_COAPP_EXPORT void
Device::Set<uint16_t>(uint16_t, uint8_t, uint16_t, ::std::error_code&);

// UNSIGNED32
template LELY_COAPP_EXPORT uint32_t
Device::Get<uint32_t>(uint16_t, uint8_t) const;
template LELY_COAPP_EXPORT uint32_t
Device::Get<uint32_t>(uint16_t, uint8_t, ::std::error_code&) const;
template LELY_COAPP_EXPORT void
Device::Set<uint32_t>(uint16_t, uint8_t, uint32_t);
template LELY_COAPP_EXPORT void
Device::Set<uint32_t>(uint16_t, uint8_t, uint32_t, ::std::error_code&);

// REAL32
template LELY_COAPP_EXPORT float
Device::Get<float>(uint16_t, uint8_t) const;
template LELY_COAPP_EXPORT float
Device::Get<float>(uint16_t, uint8_t, ::std::error_code&) const;
template LELY_COAPP_EXPORT void
Device::Set<float>(uint16_t, uint8_t, float);
template LELY_COAPP_EXPORT void
Device::Set<float>(uint16_t, uint8_t, float, ::std::error_code&);

// VISIBLE_STRING
template LELY_COAPP_EXPORT ::std::string
Device::Get<::std::string>(uint16_t, uint8_t) const;
template LELY_COAPP_EXPORT ::std::string
Device::Get<::std::string>(uint16_t, uint8_t, ::std::error_code&) const;
template LELY_COAPP_EXPORT void
Device::Set<::std::string>(uint16_t, uint8_t, const ::std::string&);
template LELY_COAPP_EXPORT void
Device::Set<::std::string>(uint16_t, uint8_t, const ::std::string&,
                           ::std::error_code&);

// OCTET_STRING
template LELY_COAPP_EXPORT ::std::vector<uint8_t>
Device::Get<::std::vector<uint8_t>>(uint16_t, uint8_t) const;
template LELY_COAPP_EXPORT ::std::vector<uint8_t>
Device::Get<::std::vector<uint8_t>>(uint16_t, uint8_t, ::std::error_code&)
    const;
template LELY_COAPP_EXPORT void
Device::Set<::std::vector<uint8_t>>(uint16_t, uint8_t,
                                    const ::std::vector<uint8_t>&);
template LELY_COAPP_EXPORT void
Device::Set<::std::vector<uint8_t>>(uint16_t, uint8_t,
    const ::std::vector<uint8_t>&, ::std::error_code&);

// UNICODE_STRING
template LELY_COAPP_EXPORT ::std::basic_string<char16_t>
Device::Get<::std::basic_string<char16_t>>(uint16_t, uint8_t) const;
template LELY_COAPP_EXPORT ::std::basic_string<char16_t>
Device::Get<::std::basic_string<char16_t>>(uint16_t, uint8_t,
                                           ::std::error_code&) const;
template LELY_COAPP_EXPORT void
Device::Set<::std::basic_string<char16_t>>(uint16_t, uint8_t,
    const ::std::basic_string<char16_t>&);
template LELY_COAPP_EXPORT void
Device::Set<::std::basic_string<char16_t>>(uint16_t, uint8_t,
      const ::std::basic_string<char16_t>&, ::std::error_code&);

// TIME_OF_DAY
// TIME_DIFFERENCE
// DOMAIN
// INTEGER24

// REAL64
template LELY_COAPP_EXPORT double
Device::Get<double>(uint16_t, uint8_t) const;
template LELY_COAPP_EXPORT double
Device::Get<double>(uint16_t, uint8_t, ::std::error_code&) const;
template LELY_COAPP_EXPORT void
Device::Set<double>(uint16_t, uint8_t, double);
template LELY_COAPP_EXPORT void
Device::Set<double>(uint16_t, uint8_t, double, ::std::error_code&);

// INTEGER40
// INTEGER48
// INTEGER56

// INTEGER64
template LELY_COAPP_EXPORT int64_t
Device::Get<int64_t>(uint16_t, uint8_t) const;
template LELY_COAPP_EXPORT int64_t
Device::Get<int64_t>(uint16_t, uint8_t, ::std::error_code&) const;
template LELY_COAPP_EXPORT void
Device::Set<int64_t>(uint16_t, uint8_t, int64_t);
template LELY_COAPP_EXPORT void
Device::Set<int64_t>(uint16_t, uint8_t, int64_t, ::std::error_code&);

// UNSIGNED24
// UNSIGNED40
// UNSIGNED48
// UNSIGNED56

// UNSIGNED64
template LELY_COAPP_EXPORT uint64_t
Device::Get<uint64_t>(uint16_t, uint8_t) const;
template LELY_COAPP_EXPORT uint64_t
Device::Get<uint64_t>(uint16_t, uint8_t, ::std::error_code&) const;
template LELY_COAPP_EXPORT void
Device::Set<uint64_t>(uint16_t, uint8_t, uint64_t);
template LELY_COAPP_EXPORT void
Device::Set<uint64_t>(uint16_t, uint8_t, uint64_t, ::std::error_code&);

#endif  // DOXYGEN_SHOULD_SKIP_THIS

LELY_COAPP_EXPORT void
Device::Set(uint16_t idx, uint8_t subidx, const char* value) {
  ::std::error_code ec;
  Set(idx, subidx, value, ec);
  if (ec)
    throw SdoError(impl_->netid(), impl_->id(), idx, subidx, ec, "Set");
}

LELY_COAPP_EXPORT void
Device::Set(uint16_t idx, uint8_t subidx, const char* value,
            ::std::error_code& ec) {
  impl_->Set<CO_DEFTYPE_VISIBLE_STRING>(idx, subidx, value, 0, ec);
}

LELY_COAPP_EXPORT void
Device::Set(uint16_t idx, uint8_t subidx, const char16_t* value) {
  ::std::error_code ec;
  Set(idx, subidx, value, ec);
  if (ec)
    throw SdoError(impl_->netid(), impl_->id(), idx, subidx, ec, "Set");
}

LELY_COAPP_EXPORT void
Device::Set(uint16_t idx, uint8_t subidx, const char16_t* value,
            ::std::error_code& ec) {
  impl_->Set<CO_DEFTYPE_UNICODE_STRING>(idx, subidx, value, 0, ec);
}

LELY_COAPP_EXPORT void
Device::Set(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n) {
  ::std::error_code ec;
  Set(idx, subidx, p, n, ec);
  if (ec)
    throw SdoError(impl_->netid(), impl_->id(), idx, subidx, ec, "Set");
}

LELY_COAPP_EXPORT void
Device::Set(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n,
            ::std::error_code& ec) {
  impl_->Set<CO_DEFTYPE_OCTET_STRING>(idx, subidx, p, n, ec);
}

Device::Impl_::Impl_(const ::std::string& dcf_txt, const ::std::string& dcf_bin,
                     uint8_t id, BasicLockable* mutex_)
    : mutex(mutex_), dev(make_unique_c<CODev>(dcf_txt.c_str())) {
  if (!dcf_bin.empty() && dev->readDCF(nullptr, nullptr, dcf_bin.c_str()) == -1)
    throw_errc("Device");

  if (id != 0xff && dev->setId(id) == -1)
    throw_errc("Device");
}

void
Device::Impl_::Set(uint16_t idx, uint8_t subidx, const ::std::string& value,
                   ::std::error_code& ec) {
  Set<CO_DEFTYPE_VISIBLE_STRING>(idx, subidx, value.c_str(), 0, ec);
}

void
Device::Impl_::Set(uint16_t idx, uint8_t subidx,
                   const ::std::vector<uint8_t>& value, ::std::error_code& ec) {
  Set<CO_DEFTYPE_OCTET_STRING>(idx, subidx, value.data(), value.size(), ec);
}

void
Device::Impl_::Set(uint16_t idx, uint8_t subidx,
const ::std::basic_string<char16_t>& value, ::std::error_code& ec) {
  Set<CO_DEFTYPE_UNICODE_STRING>(idx, subidx, value.c_str(), 0, ec);
}

template <uint16_t N>
void
Device::Impl_::Set(uint16_t idx, uint8_t subidx, const void* p, ::std::size_t n,
                   ::std::error_code& ec) {
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

  if (!detail::IsCanopenSame(N, sub->getType())) {
    ec = SdoErrc::TYPE_LEN;
    return;
  }

  ec.clear();
  sub->setVal(p, n);
}

}  // namespace canopen

}  // namespace lely
