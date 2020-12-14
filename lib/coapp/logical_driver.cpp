/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the logical device driver interface.
 *
 * @see lely/coapp/logical_driver.hpp
 *
 * @copyright 2019-2019 Lely Industries N.V.
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

#if !LELY_NO_COAPP_MASTER

#include <lely/coapp/logical_driver.hpp>

namespace lely {

namespace canopen {

BasicLogicalDriver<BasicDriver>::BasicLogicalDriver(BasicDriver& driver_,
                                                    int num, uint32_t dev)
    : master(driver_.master),
      driver(driver_),
      rpdo_mapped(*this),
      tpdo_mapped(*this),
      tpdo_event_mutex(driver_.tpdo_event_mutex),
      num_(num),
      dev_(dev) {
  driver.Insert(*this);
}

BasicLogicalDriver<BasicDriver>::~BasicLogicalDriver() { driver.Erase(*this); }

SdoFuture<void>
BasicLogicalDriver<BasicDriver>::AsyncConfig() {
  SdoFuture<void> f;
  // A convenience function which reads and copies the device type from object
  // 67FF:00 (adjusted for logical device number) in the remote object
  // dictionary.
  auto read_67ff = [this]() {
    return AsyncRead<uint32_t>(0x67ff, 0).then(
        GetExecutor(),
        [this](SdoFuture<uint32_t> f) { dev_ = f.get().value(); });
  };
  if (num_ == 1) {
    // A convenience function which checks and copies the value of the
    // (expected) device type or, if it indicates multiple logical devices,
    // reads the value from object 67FF:00 in the remote object dictionary.
    auto check_1000 = [this, read_67ff](uint32_t value) -> SdoFuture<void> {
      if ((value) >> 16 == 0xffff) return read_67ff();
      dev_ = value;
      return make_empty_sdo_future();
    };
    ::std::error_code ec;
    // Try to read the expected device type from the local object dictionary
    // (object 1F84:$NODEID).
    auto value = driver.master[0x1f84][driver.id()].Read<uint32_t>(ec);
    if (!ec) {
      f = check_1000(value);
    } else {
      // If the expected device type is not available, read the device type from
      // the remote object dictionary (object 1000:00).
      f = AsyncRead<uint32_t>(0x1000, 0).then(
          GetExecutor(), [this, check_1000](SdoFuture<uint32_t> f) {
            return check_1000(f.get().value());
          });
    }
  } else {
    // This is not the first logical device. Read and copy the device type from
    // object 67FF:00 (adjusted for logical device number) in the remote object
    // dictionary.
    f = read_67ff();
  }
  // Run OnConfig() after the device type has been obtained.
  return f.then(GetExecutor(), [this](SdoFuture<void> f) {
    // Throw an exception if an SDO error occurred.
    f.get().value();
    // Only invoke OnConfig() if the previous operations succeeded.
    SdoPromise<void> p;
    OnConfig([p](::std::error_code ec) mutable {
      p.set(::std::make_exception_ptr(::std::system_error(ec, "OnConfig")));
    });
    return p.get_future();
  });
}

SdoFuture<void>
BasicLogicalDriver<BasicDriver>::AsyncDeconfig() {
  SdoPromise<void> p;
  // Post a task for OnDeconfig() to ensure this function does not block.
  GetExecutor().post([this, p]() mutable {
    OnDeconfig([p](::std::error_code ec) mutable {
      p.set(::std::make_exception_ptr(::std::system_error(ec, "OnDeconfig")));
    });
  });
  return p.get_future();
}

}  // namespace canopen

}  // namespace lely

#endif  // !LELY_NO_COAPP_MASTER
