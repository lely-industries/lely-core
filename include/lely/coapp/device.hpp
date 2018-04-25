/*!\file
 * This header file is part of the C++ CANopen application library; it contains
 * the CANopen device description declarations.
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

#ifndef LELY_COAPP_DEVICE_HPP_
#define LELY_COAPP_DEVICE_HPP_

#include <lely/coapp/detail/type_traits.hpp>
#include <lely/coapp/sdo_error.hpp>

#include <memory>

namespace lely {

// The CANopen device from <lely/co/dev.hpp>.
class CODev;

namespace canopen {

/*!
 * The CANopen device description. This class manages the object dictionary and
 * device setttings such as the network-ID and node-ID.
 */
class LELY_COAPP_EXTERN Device {
 public:
  /*!
   * Creates a new CANopen device description.
   *
   * \param dcf_txt the path of the text EDS or DCF containing the device
   *                description.
   * \param dcf_bin the path of the (binary) concise DCF containing the values
   *                of (some of) the objets in the object dictionary. If
   *                \a dcf_bin is empty, no concise DCF is loaded.
   * \param id      the node-ID (in the range [1..127, 255]). If \a id is 255
   *                (unconfigured), the node-ID is obtained from the DCF.
   * \param mutex   an (optional) pointer to the mutex to be locked while the
   *                internal device description is accessed. The mutex MUST be
   *                unlocked when any member function is invoked.
   */
  Device(const ::std::string& dcf_txt, const ::std::string& dcf_bin = "",
         uint8_t id = 0xff, BasicLockable* mutex = nullptr);

  Device(const Device&) = delete;
  Device(Device&&) = default;

  Device& operator=(const Device&) = delete;
  Device& operator=(Device&&);

  //! Returns the network-ID.
  uint8_t netid() const noexcept;

  //! Returns the node-ID.
  uint8_t id() const noexcept;

 protected:
  ~Device();

  //! Returns a pointer to the internal CANopen device from <lely/co/dev.hpp>.
  CODev* dev() const noexcept;

 private:
  struct Impl_;
  ::std::unique_ptr<Impl_> impl_;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_DEVICE_HPP_
