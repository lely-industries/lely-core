/*!\file
 * This header file is part of the C++ CANopen application library; it contains
 * the CANopen master declarations.
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

#ifndef LELY_COCPP_MASTER_HPP_
#define LELY_COCPP_MASTER_HPP_

#include <lely/coapp/node.hpp>

#include <map>

namespace lely {

namespace canopen {

class DriverBase;

/*!
 * The CANopen master. The master implements a CANopen node. Handling events for
 * remote CANopen slaves is delegated to drivers (see #lely::canopen::DriverBase
 * and #lely::canopen::BasicDriver), one of which can be registered for each
 * node-ID.
 *
 * For derived classes, the master behaves as an AssociativeContainer for
 * drivers.
 */
 class LELY_COAPP_EXTERN BasicMaster
    : public Node, protected ::std::map<uint8_t, DriverBase*> {
 public:
  /*!
   * Creates a new CANopen master. After creation, the master is in the NMT
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
  BasicMaster(aio::TimerBase& timer, aio::CanBusBase& bus,
      const ::std::string& dcf_txt, const ::std::string& dcf_bin = "",
      uint8_t id = 0xff);

  virtual ~BasicMaster();

  /*!
   * Indicates the occurrence of an error event on a remote node and triggers
   * the error handling process (see Fig. 12 in CiA 302-2 v4.1.0). Note that
   * depending on the value of objects 1F80 (NMT startup) and 1F81 (NMT slave
   * assignment), the occurrence of an error event MAY trigger an NMT state
   * transition of the entire network, including the master.
   *
   * \param id the node-ID (in the range[1..127]).
   */
  void Error(uint8_t id);

  /*!
   * Returns the SDO timeout used during the NMT 'boot slave' and 'check
   * configuration' processes.
   *
   * \see SetTimeout()
   */
  ::std::chrono::milliseconds GetTimeout() const;

  /*!
   * Sets the SDO timeout used during the NMT 'boot slave' and 'check
   * configuration' processes.
   *
   * \see GetTimeout()
   */
  void SetTimeout(const ::std::chrono::milliseconds& timeout);

  /*!
   * Registers a driver for a remote CANopen node. If an event occurs for that
   * node, or for the entire CANopen network, the corresponding method of the
   * driver will be invoked.
   *
   * \throws std::out_of_range if the node-ID is invalid or already registered.
   *
   * \see Erase()
   */
  void Insert(DriverBase& driver);

  /*!
   * Unregisters a driver for a remote CANopen node.
   *
   * \see Insert()
   */
  void Erase(DriverBase& driver);

 protected:
  using MapType = ::std::map<uint8_t, DriverBase*>;

  /*!
   * The default implementation notifies all registered drivers.
   *
   * \see IoContext::OnCanError(), DriverBase::OnCanError()
   */
  void OnCanError(CanError error) noexcept override;

  /*!
   * The default implementation invokes #lely::canopen::Node::OnCanState() and
   * notifies each registered driver.
   *
   * \see IoContext::OnCanState(), DriverBase::OnCanState()
   */
  void OnCanState(CanState new_state, CanState old_state) noexcept override;

  /*!
   * The default implementation notifies all registered drivers. Unless the
   * master enters the pre-operational or operational state, all ongoing and
   * pending SDO requests are aborted.
   *
   * \see Node::OnCommand(), DriverBase::OnCommand()
   */
  void OnCommand(NmtCommand cs) noexcept override;

  /*!
   * The function invoked when a node guarding timeout event occurs or is
   * resolved. Note that depending on the value of object 1029:01 (Error
   * behavior object), the occurrence of a node guarding event MAY trigger an
   * NMT state transition. If so, this function is called _after_ the state
   * change completes.
   *
   * The default implementation notifies the driver registered for node \a id.
   *
   * \param id       the node-ID (in the range [1..127]).
   * \param occurred `true` if the node guarding event occurred, `false` if it
   *                 was resolved.
   *
   * \see DriverBase::OnNodeGuarding()
   */
  virtual void OnNodeGuarding(uint8_t id, bool occurred) noexcept;

  /*!
   * The default implementation notifies the driver registered for node \a id.
   *
   * \see Node::OnHeartbeat(), DriverBase::OnHeartbeat()
   */
  void OnHeartbeat(uint8_t id, bool occurred) noexcept override;

  /*!
   * The default implementation notifies the driver registered for node \a id.
   * If a boot-up event (`st == NmtState::BOOTUP`) is detected, any ongoing or
   * pending SDO requests for the slave are aborted. This is necessary because
   * the master MAY need the Client-SDO service for the NMT 'boot slave'
   *  process.
   *
   * \see Node::OnState(), DriverBase::OnState()
   */
  void OnState(uint8_t id, NmtState st) noexcept override;

  /*!
   * The function invoked when the NMT 'boot slave' process completes.
   *
   * The default implementation notifies the driver registered for node \a id.
   *
   * \param id   the node-ID (in the range [1..127]).
   * \param st   the state of the remote node (including the toggle bit
   *             (#NmtState::TOGGLE) if node guarding is enabled).
   * \param es   the error status (in the range ['A'..'O'], or 0 on success):
   *             - 'A': The CANopen device is not listed in object 1F81.
   *             - 'B': No response received for upload request of object 1000.
   *             - 'C': Value of object 1000 from CANopen device is different to
   *               value in object 1F84 (%Device type).
   *             - 'D': Value of object 1018:01 from CANopen device is different
   *               to value in object 1F85 (Vendor-ID).
   *             - 'E': Heartbeat event. No heartbeat message received from
   *               CANopen device.
   *             - 'F': Node guarding event. No confirmation for guarding
   *               request received from CANopen device.
   *             - 'G': Objects for program download are not configured or
   *               inconsistent.
   *             - 'H': Software update is required, but not allowed because of
   *               configuration or current status.
   *             - 'I': Software update is required, but program download
   *               failed.
   *             - 'J': Configuration download failed.
   *             - 'K': Heartbeat event during start error control service. No
   *               heartbeat message received from CANopen device during start
   *               error control service.
   *             - 'L': NMT slave was initially operational. (CANopen manager
   *               may resume operation with other CANopen devices)
   *             - 'M': Value of object 1018:02 from CANopen device is different
   *               to value in object 1F86 (Product code).
   *             - 'N': Value of object 1018:03 from CANopen device is different
   *               to value in object 1F87 (Revision number).
   *             - 'O': Value of object 1018:04 from CANopen device is different
   *               to value in object 1F88 (Serial number).
   * \param what if \a es is non-zero, contains a string explaining the error.
   *
   * \see DriverBase::OnBoot()
   */
  virtual void OnBoot(uint8_t id, NmtState st, char es,
                      const ::std::string& what) noexcept;

  /*!
   * The function invoked when the 'update configuration' step is reached during
   * the NMT 'boot slave' process. The 'boot slave' process is halted until the
   * result of the 'update configuration' step is communicated to the NMT
   * service with #ConfigResult().
   *
   * The default implementation delegates the configuration update to the
   * driver, if one is registered for node \a id. If not, a successful result is
   * communicated to the NMT service.
   *
   * \see DriverBase::OnConfig()
   */
  virtual void OnConfig(uint8_t id) noexcept;

  /*!
   * Reports the result of the 'update configuration' step to the NMT service.
   *
   * \param id the node-ID (in the range [1..127]).
   * \param ec the SDO abort code (0 on success).
   */
  void ConfigResult(uint8_t id, ::std::error_code ec) noexcept;

  /*!
   * The default implementation notifies all registered drivers.
   *
   * \see Node::OnRpdo(), DriverBase::OnRpdo()
   */
  void OnRpdo(int num, ::std::error_code ec, const void* p, ::std::size_t n)
      noexcept override;

  /*!
   * The default implementation notifies all registered drivers.
   *
   * \see Node::OnRpdoError(), DriverBase::OnRpdoError()
   */
  void OnRpdoError(int num, uint16_t eec, uint8_t er) noexcept override;

  /*!
   * The default implementation notifies all registered drivers.
   *
   * \see Node::OnTpdo(), DriverBase::OnTpdo()
   */
  void OnTpdo(int num, ::std::error_code ec, const void* p, ::std::size_t n)
      noexcept override;

  /*!
   * The default implementation notifies all registered drivers.
   *
   * \see Node::OnSync(), DriverBase::OnSync()
   */
  void OnSync(uint8_t cnt) noexcept override;

  /*!
   * The default implementation notifies all registered drivers.
   *
   * \see Node::OnSyncError(), DriverBase::OnSyncError()
   */
  void OnSyncError(uint16_t eec, uint8_t er) noexcept override;

  /*!
   * The default implementation notifies all registered drivers.
   *
   * \see Node::OnTime(), DriverBase::OnTime()
   */
  void OnTime(const ::std::chrono::system_clock::time_point& abs_time) noexcept
      override;

  /*!
   * The default implementation notifies the driver registered for node \a id.
   *
   * \see Node::OnEmcy(), DriverBase::OnEmcy()
   */
  void OnEmcy(uint8_t id, uint16_t eec, uint8_t er, uint8_t msef[5]) noexcept
      override;

 private:
  struct Impl_;
  ::std::unique_ptr<Impl_> impl_;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COCPP_MASTER_HPP_
