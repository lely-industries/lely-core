/*!\file
 * This header file is part of the C++ CANopen application library; it contains
 * the CANopen slave declarations.
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

#ifndef LELY_COCPP_SLAVE_HPP_
#define LELY_COCPP_SLAVE_HPP_

#include <lely/coapp/node.hpp>

namespace lely {

namespace canopen {

//! The base class for CANopen slaves.
class LELY_COAPP_EXTERN BasicSlave : public Node {
 public:
  /*!
   * Creates a new CANopen slave. After creation, the slave is in the NMT
   * 'Initialisation' state and does not yet create any services or perform any
   * communication. Call #Reset() to start the boot-up process.
   *
   * \param timer   the timer used for CANopen events.
   * \param bus     a handle to the CAN bus.
   * \param dcf_txt the path of the text EDS or DCF containing the device
   *                description.
   * \param dcf_bin the path of the (binary) concise DCF containing the values
   *                of (some of) the objets in the object dictionary. If
   *                \a dcf_bin is empty, no concise DCF is loaded.
   * \param id      the node-ID (in the range [1..127, 255]). If \a id is 255
   *                (unconfigured), the node-ID is obtained from the DCF.
   */
  BasicSlave(aio::TimerBase& timer, aio::CanBusBase& bus,
      const ::std::string& dcf_txt, const ::std::string& dcf_bin = "",
      uint8_t id = 0xff);

  virtual ~BasicSlave();

#ifndef DOXYGEN_SHOULD_SKIP_THIS
 private:
#endif
  /*!
   * The function invoked when a life guarding event occurs or is resolved. Note
   * that depending on the value of object 1029:01 (Error behavior object), the
   * occurrence of a life guarding event MAY trigger an NMT state transition. If
   * so, this function is called _after_ the state change completes.
   *
   * \param occurred `true` if the life guarding event occurred, `false` if it
   *                 was resolved.
   */
  virtual void OnLifeGuarding(bool occurred) noexcept { (void)occurred; };

 private:
  struct Impl_;
  ::std::unique_ptr<Impl_> impl_;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COCPP_SLAVE_HPP_
