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

Device::Impl_::Impl_(const ::std::string& dcf_txt, const ::std::string& dcf_bin,
                     uint8_t id, BasicLockable* mutex_)
    : mutex(mutex_), dev(make_unique_c<CODev>(dcf_txt.c_str())) {
  if (!dcf_bin.empty() && dev->readDCF(nullptr, nullptr, dcf_bin.c_str()) == -1)
    throw_errc("Device");

  if (id != 0xff && dev->setId(id) == -1)
    throw_errc("Device");
}

}  // namespace canopen

}  // namespace lely
