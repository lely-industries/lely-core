/**@file
 * This header file is part of the C++ CANopen application library; it contains
 * the CANopen node declarations.
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

#ifndef LELY_COAPP_NODE_HPP_
#define LELY_COAPP_NODE_HPP_

#include <lely/coapp/device.hpp>
#include <lely/coapp/io_context.hpp>

#include <memory>
#include <string>

namespace lely {

// The CANopen NMT node service from <lely/co/nmt.hpp>.
class CONMT;

namespace canopen {

/// The NMT command specifiers.
enum class NmtCommand {
  /// Start.
  START = 0x01,
  /// Stop.
  STOP = 0x02,
  /// Enter pre-operational.
  ENTER_PREOP = 0x80,
  /// Reset node.
  RESET_NODE = 0x81,
  /// Reset communication.
  RESET_COMM = 0x82
};

/// The NMT states.
enum class NmtState {
  /// Boot-up.
  BOOTUP = 0x00,
  /// Stopped,
  STOP = 0x04,
  /// Operational.
  START = 0x05,
  /// Reset application (a local NMT sub-state).
  RESET_NODE = 0x06,
  /// Reset communication (a local NMT sub-state).
  RESET_COMM = 0x07,
  /// Pre-operational.
  PREOP = 0x7f,
  /// The mask to get/set the toggle bit from an NMT state.
  TOGGLE = 0x80
};

constexpr NmtState operator&(NmtState lhs, NmtState rhs) {
  return static_cast<NmtState>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

constexpr NmtState
operator|(NmtState lhs, NmtState rhs) {
  return static_cast<NmtState>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

constexpr NmtState
operator^(NmtState lhs, NmtState rhs) {
  return static_cast<NmtState>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
}

constexpr NmtState
operator~(NmtState lhs) {
  return static_cast<NmtState>(~static_cast<int>(lhs));
}

inline NmtState&
operator&=(NmtState& lhs, NmtState rhs) noexcept {
  return lhs = lhs & rhs;
}

inline NmtState&
operator|=(NmtState& lhs, NmtState rhs) noexcept {
  return lhs = lhs | rhs;
}

inline NmtState&
operator^=(NmtState& lhs, NmtState rhs) noexcept {
  return lhs = lhs ^ rhs;
}

/**
 * The base class for CANopen nodes.
 *
 * This class implements the #lely::util::BasicLockable mutex used by
 * #lely::canopen::IoContext and #lely::canopen::Device. The mutex MUST be
 * unlocked when any public member function is invoked (#Reset()); it will be
 * locked for the duration of any call to a virtual member function.
 */
class Node : protected util::BasicLockable, public IoContext, public Device {
 public:
  using time_point = ::std::chrono::steady_clock::time_point;

  /**
   * Creates a new CANopen node. After creation, the node is in the NMT
   * 'Initialisation' state and does not yet create any services or perform any
   * communication. Call #Reset() to start the boot-up process.
   *
   * @param timer   the timer used for CANopen events.
   * @param chan    a CAN channel.
   * @param dcf_txt the path of the text EDS or DCF containing the device
   *                description.
   * @param dcf_bin the path of the (binary) concise DCF containing the values
   *                of (some of) the objets in the object dictionary. If
   *                <b>dcf_bin</b> is empty, no concise DCF is loaded.
   * @param id      the node-ID (in the range [1..127, 255]). If <b>id</b> is
   *                255 (unconfigured), the node-ID is obtained from the DCF.
   */
  Node(io::TimerBase& timer, io::CanChannelBase& chan,
       const ::std::string& dcf_txt, const ::std::string& dcf_bin = "",
       uint8_t id = 0xff);

  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;

  virtual ~Node();

  /**
   * (Re)starts the node. This function behaves as if an NMT 'reset node'
   * command has been received. Note that this function will cause the
   * invocation of #OnCommand() and therefore MUST NOT be called from that
   * function.
   */
  void Reset();

  /**
   * Configures heartbeat consumption for the specified node by updating CANopen
   * object 1016 (Consumer heartbeat time).
   *
   * @param id the node-ID (in the range [1..127]).
   * @param ms the heartbeat timeout (in milliseconds).
   * @param ec if heartbeat consumption cannot be configured, the SDO abort code
   *           is stored in <b>ec</b>.
   */
  void ConfigHeartbeat(uint8_t id, const ::std::chrono::milliseconds& ms,
                       ::std::error_code& ec);

  /**
   * Configures heartbeat consumption for the specified node by updating CANopen
   * object 1016 (Consumer heartbeat time).
   *
   * @param id the node-ID (in the range [1..127]).
   * @param ms the heartbeat timeout (in milliseconds).
   *
   * @throws #lely::canopen::SdoError if heartbeat consumption cannot be
   * configured.
   */
  void ConfigHeartbeat(uint8_t id, const ::std::chrono::milliseconds& ms);

  /**
   * Registers the function to be invoked when an NMT command is received from
   * the master. Only a single function can be registered at any one time. If
   * <b>on_command</b> contains a callable function target, a copy of the
   * target is invoked _after_ OnCommand(NmtCommand) completes.
   */
  void OnCommand(::std::function<void(NmtCommand)> on_command);

  /**
   * Registers the function to be invoked when a heartbeat timeout event occurs
   * or is resolved. Only a single function can be registered at any one time.
   * If <b>on_heartbeat</b> contains a callable function target, a copy of the
   * target is invoked _after_ OnHeartbeat(uint8_t, bool) completes.
   */
  void OnHeartbeat(::std::function<void(uint8_t, bool)> on_heartbeat);

  /**
   * Registers the function to be invoked whenan NMT state change or boot-up
   * event is detected for a remote node by the heartbeat protocol. Only a
   * single function can be registered at any one time. If <b>on_state</b>
   * contains a callable function target, a copy of the target is invoked
   * _after_ OnState(uint8_t, NmtState) completes.
   */
  void OnState(::std::function<void(uint8_t, NmtState)> on_state);

  /**
   * Registers the function to be invoked when a Receive-PDO is processed. Only
   * a single function can be registered at any one time. If <b>on_rpdo</b>
   * contains a callable function target, a copy of the target is invoked
   * _after_ OnRpdo(int, ::std::error_code, const void*, ::std::size_t)
   * completes.
   */
  void OnRpdo(
      ::std::function<void(int, ::std::error_code, const void*, ::std::size_t)>
          on_rpdo);

  /**
   * Registers the function to be invoked when a Receive-PDO length mismatch or
   * timeout error occurs. Only a single function can be registered at any one
   * time. If <b>on_rpdo_error</b> contains a callable function target, a copy
   * of the target is invoked _after_ OnRpdoError(int, uint16_t, uint8_t)
   * completes.
   */
  void OnRpdoError(::std::function<void(int, uint16_t, uint8_t)> on_rpdo_error);

  /**
   * Registers the function to be invoked after a Transmit-PDO is sent or an
   * error occurs. Only a single function can be registered at any one time. If
   * <b>on_tpdo</b> contains a callable function target, a copy of the target is
   * invoked _after_ OnTpdo(int, ::std::error_code, const void*, ::std::size_t)
   * completes.
   */
  void OnTpdo(
      ::std::function<void(int, ::std::error_code, const void*, ::std::size_t)>
          on_tpdo);

  /**
   * Registers the function to be invoked when a SYNC message is sent/received.
   * Only a single function can be registered at any one time. If <b>on_sync</b>
   * contains a callable function target, a copy of the target is invoked
   * _after_ OnSync(uint8_t, const time_point&) completes.
   */
  void OnSync(::std::function<void(uint8_t, const time_point&)> on_sync);

  /**
   * Registers the function to be invoked when the data length of a received
   * SYNC message does not match. Only a single function can be registered at
   * any one time. If <b>on_sync_error</b> contains a callable function target,
   * a copy of the target is invoked _after_ OnSyncError(uint16_t, uint8_t)
   * completes.
   */
  void OnSyncError(::std::function<void(uint16_t, uint8_t)> on_sync_error);

  /**
   * Registers the function to be invoked when a TIME message is received. Only
   * a single function can be registered at any one time. If <b>on_time</b>
   * contains a callable function target, a copy of the target is invoked
   * _after_ OnTime(const ::std::chrono::system_clock::time_point&) completes.
   */
  void OnTime(
      ::std::function<void(const ::std::chrono::system_clock::time_point&)>
          on_time);

  /**
   * Registers the function to be invoked when an EMCY message is received. Only
   * a single function can be registered at any one time. If <b>on_emcy</b>
   * contains a callable function target, a copy of the target is invoked
   * _after_ OnEmcy(uint8_t, uint16_t, uint8_t, uint8_t[5]) completes.
   */
  void OnEmcy(
      ::std::function<void(uint8_t, uint16_t, uint8_t, uint8_t[5])> on_emcy);

 protected:
  void lock() final;
  void unlock() final;

  /**
   * Implements the default behavior for a CAN bus state change. If the CAN bus
   * is in error passive mode or has recovered from bus off, an EMCY message is
   * sent (see Table 26 in CiA 301 v4.2.0).
   */
  void OnCanState(io::CanState new_state,
                  io::CanState old_state) noexcept override;

  /**
   * Returns a pointer to the internal CANopen NMT master/slave service from
   * <lely/co/nmt.hpp>.
   */
  CONMT* nmt() const noexcept;

  /*
   * Generates an EMCY error and triggers the error handling behavior according
   * to object 1029:01 (Error behavior object) in case of a communication error
   * (emergency error code 0x81xx).
   *
   * @param eec  the emergency error code.
   * @param er   the error register.
   * @param msef the manufacturer-specific error code.
   */
  void Error(uint16_t eec, uint8_t er,
             const uint8_t msef[5] = nullptr) noexcept;

  /**
   * Requests the transmission of a PDO by sending a CAN frame with the RTR
   * (Remote Transmission Request) bit set.
   *
   * @param num the PDO number (in the range [1..512]). If <b>num</b> is 0, the
   *            transmission of all PDOs is requested.
   * @param ec  the error code (0 on success).
   */
  void RpdoRtr(int num, ::std::error_code& ec) noexcept;

  /**
   * Requests the transmission of a PDO by sending a CAN frame with the RTR
   * (Remote Transmission Request) bit set.
   *
   * @param num the PDO number (in the range [1..512]). If <b>num</b> is 0, the
   *            transmission of all PDOs is requested.
   *
   * @throws std::system_error if an error occurred while sending the CAN frame.
   */
  void RpdoRtr(int num = 0);

  /**
   * Triggers the transmission of an event-driven (asynchronous) PDO.
   *
   * @param num the PDO number (in the range [1..512]). If <b>num</b> is 0, the
   *            transmission of all PDOs is triggered.
   * @param ec  the error code (0 on success).
   *            `ec == std::errc::resource_unavailable_try_again` if the inhibit
   *            time has not yet elapsed.
   */
  void TpdoEvent(int num, ::std::error_code& ec) noexcept;

  /**
   * Triggers the transmission of an event-driven (asynchronous) PDO.
   *
   * @param num the PDO number (in the range [1..512]). If <b>num</b> is 0, the
   *            transmission of all PDOs is triggered.
   *
   * @throws std::system_error if an error occurred while sending the CAN frame,
   * or std::system_error(std::errc::resource_unavailable_try_again) if the
   * inhibit time has not yet elapsed.
   */
  void TpdoEvent(int num = 0);
#ifndef DOXYGEN_SHOULD_SKIP_THIS

 private:
#endif
  /**
   * The function invoked when an NMT command is received from the master. Note
   * that #Reset() MUST NOT be called from `OnCommand()`, since the node is
   * undergoing an NMT state transition, possibly triggered by #Reset() itself.
   *
   * @param cs the NMT command specifier.
   */
  virtual void
  OnCommand(NmtCommand cs) noexcept {
    (void)cs;
  }

  /**
   * The function invoked when a heartbeat timeout event occurs or is resolved.
   * Note that depending on the value of object 1029:01 (Error behavior object),
   * the occurrence of a heartbeat timeout event MAY trigger an NMT state
   * transition. If so, this function is called _after_ the state change
   * completes.
   *
   * @param id       the node-ID (in the range [1..127]).
   * @param occurred `true` if the heartbeat timeout event occurred, `false` if
   *                 it was resolved.
   */
  virtual void
  OnHeartbeat(uint8_t id, bool occurred) noexcept {
    (void)id;
    (void)occurred;
  }

  /**
   * The function invoked when an NMT state change or boot-up event is detected
   * for a remote node by the heartbeat protocol.
   *
   * @param id the node-ID (in the range [1..127]).
   * @param st the state of the remote node. Note that the NMT sub-states
   *           #NmtState::RESET_NODE and #NmtState::RESET_COMM are never
   *           reported for remote nodes.
   */
  virtual void
  OnState(uint8_t id, NmtState st) noexcept {
    (void)id;
    (void)st;
  }

  /**
   * The function invoked when a Receive-PDO is processed. In case of a PDO
   * length mismatch error, #OnRpdoError() is invoked after this function.
   *
   * @param num the PDO number (in the range [1..512]).
   * @param ec  the SDO abort code:
   *            - 0 on success;
   *            - #SdoErrc::NO_OBJ or #SdoErrc::NO_SUB if one of the objects or
   *              sub-objects does not exist, respectively;
   *            - #SdoErrc::NO_WRITE if one of the sub-objects is read only;
   *            - #SdoErrc::NO_PDO if one of the sub-objects cannot be mapped to
   *              a PDO;
   *            - #SdoErrc::PDO_LEN if the number and length of the mapped
   *              objects exceeds the PDO length (#OnRpdoError() will be invoked
   *              after this function returns);
   *            - the abort code generated by the SDO download request to the
   *              local object dictionary.
   * @param p   a pointer to the bytes received.
   * @param n   the number of bytes at <b>p</b>.
   */
  virtual void
  OnRpdo(int num, ::std::error_code ec, const void* p,
         ::std::size_t n) noexcept {
    (void)num;
    (void)ec;
    (void)p;
    (void)n;
  }

  /**
   * The function invoked when a Receive-PDO length mismatch or timeout error
   * occurs.
   *
   * The default implementation sends an EMCY message.
   *
   * @param num the PDO number (in the range [1..512]).
   * @param eec the emergency error code:
   *            - 0x8210: PDO not processed due to length error;
   *            - 0x8220: PDO length exceeded;
   *            - 0x8250: RPDO timeout.
   * @param er  the error register (0x10).
   */
  virtual void
  OnRpdoError(int num, uint16_t eec, uint8_t er) noexcept {
    (void)num;
    Error(eec, er);
  }

  /**
   * The function invoked after a Transmit-PDO is sent or an error occurs.
   *
   * @param num the PDO number (in the range [1..512]).
   * @param ec  the SDO abort code:
   *            - 0 on success;
   *            - #SdoErrc::NO_OBJ or #SdoErrc::NO_SUB if one of the objects or
   *              sub-objects does not exist, respectively;
   *            - #SdoErrc::NO_READ if one of the sub-objects is write only;
   *            - #SdoErrc::NO_PDO if one of the sub-objects cannot be mapped to
   *              a PDO;
   *            - #SdoErrc::PDO_LEN if the number and length of the mapped
   *              objects exceeds the PDO length;
   *            - #SdoErrc::TIMEOUT if the synchronous time window expires;
   *            - the abort code generated by the SDO upload request to the
   *              local object dictionary.
   * @param p   a pointer to the bytes sent.
   * @param n   the number of bytes at <b>p</b>.
   */
  virtual void
  OnTpdo(int num, ::std::error_code ec, const void* p,
         ::std::size_t n) noexcept {
    (void)num;
    (void)ec;
    (void)p;
    (void)n;
  }

  /**
   * The function invoked when a SYNC message is sent/received. Note that this
   * function is called _after_ all PDOs are processed/sent.
   *
   * @param cnt the counter (in the range [1..240]), or 0 if the SYNC message is
   *            empty.
   * @param t   the time at which the SYNC message was sent/received.
   */
  virtual void
  OnSync(uint8_t cnt, const time_point& t) noexcept {
    (void)cnt;
    (void)t;
  }

  /**
   * The function invoked when the data length of a received SYNC message does
   * not match.
   *
   * The default implementation transmits an EMCY message.
   *
   * @param eec the emergency error code (0x8240).
   * @param er  the error register (0x10).
   */
  virtual void
  OnSyncError(uint16_t eec, uint8_t er) noexcept {
    Error(eec, er);
  }

  /**
   * The function invoked when a TIME message is received.
   *
   * @param abs_time a time point representing the received time stamp.
   */
  virtual void
  OnTime(const ::std::chrono::system_clock::time_point& abs_time) noexcept {
    (void)abs_time;
  }

  /**
   * The function invoked when an EMCY message is received.
   *
   * @param id   the node-ID (in the range [1..127]).
   * @param eec  the emergency error code.
   * @param er   the error register.
   * @param msef the manufacturer-specific error code.
   */
  virtual void
  OnEmcy(uint8_t id, uint16_t eec, uint8_t er, uint8_t msef[5]) noexcept {
    (void)id;
    (void)eec;
    (void)er;
    (void)msef;
  }
#ifdef DOXYGEN_SHOULD_SKIP_THIS

 private:
#endif
  struct Impl_;
  ::std::unique_ptr<Impl_> impl_;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_NODE_HPP_
