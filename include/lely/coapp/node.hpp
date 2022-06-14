/**@file
 * This header file is part of the C++ CANopen application library; it contains
 * the CANopen node declarations.
 *
 * @copyright 2018-2022 Lely Industries N.V.
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
#include <lely/io2/can_net.hpp>
#include <lely/io2/tqueue.hpp>

#include <functional>
#include <memory>
#include <string>
#include <utility>

// The CANopen NMT master/slave service from <lely/co/nmt.h>.
struct __co_nmt;

namespace lely {

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

constexpr NmtState
operator&(NmtState lhs, NmtState rhs) {
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
 * This class inherits the #lely::util::BasicLockable mutex used by
 * #lely::canopen::Device. The mutex MUST be unlocked when any public member
 * function is invoked (#Reset()); it will be locked for the duration of any
 * call to a virtual member function.
 */
class Node : public io::CanNet, public Device {
  friend class LssMaster;

 public:
  using duration = io::TimerBase::duration;
  using time_point = io::TimerBase::time_point;

  /**
   * Creates a new CANopen node. After creation, the node is in the NMT
   * 'Initialisation' state and does not yet create any services or perform any
   * communication. Call #Reset() to start the boot-up process.
   *
   * @param exec  the executor used to process I/O and CANopen events. If
   *              <b>exec</b> is a null pointer, the CAN channel executor is
   *              used.
   * @param timer the timer used for CANopen events. This timer MUST NOT be used
   *              for any other purpose.
   * @param chan  a CAN channel. This channel MUST NOT be used for any other
   *              purpose.
   * @param dev   a pointer to an internal device desciption. Ownership of
   *              <b>dev</b> is transfered to the new class instance and the
   *              internal device description will be destroyed when the class
   *              instance is destroyed.
   * @param id    the node-ID (in the range [1..127, 255]). If <b>id</b> is 255
   *              (unconfigured), the node-ID is obtained from the device
   *              description.
   */
  explicit Node(ev_exec_t* exec, io::TimerBase& timer, io::CanChannelBase& chan,
                __co_dev* dev, uint8_t id = 0xff);

  /// Creates a new CANopen node.
  explicit Node(io::TimerBase& timer, io::CanChannelBase& chan, __co_dev* dev,
                uint8_t id = 0xff)
      : Node(nullptr, timer, chan, dev, id) {}

  /**
   * Creates a new CANopen node. After creation, the node is in the NMT
   * 'Initialisation' state and does not yet create any services or perform any
   * communication. Call #Reset() to start the boot-up process.
   *
   * @param exec    the executor used to process I/O and CANopen events. If
   *                <b>exec</b> is a null pointer, the CAN channel executor is
   *                used.
   * @param timer   the timer used for CANopen events. This timer MUST NOT be
   *                used for any other purpose.
   * @param chan    a CAN channel. This channel MUST NOT be used for any other
   *                purpose.
   * @param dcf_txt the path of the text EDS or DCF containing the device
   *                description.
   * @param dcf_bin the path of the (binary) concise DCF containing the values
   *                of (some of) the objets in the object dictionary. If
   *                <b>dcf_bin</b> is empty, no concise DCF is loaded.
   * @param id      the node-ID (in the range [1..127, 255]). If <b>id</b> is
   *                255 (unconfigured), the node-ID is obtained from the DCF.
   */
  explicit Node(ev_exec_t* exec, io::TimerBase& timer, io::CanChannelBase& chan,
                const ::std::string& dcf_txt, const ::std::string& dcf_bin = "",
                uint8_t id = 0xff);

  /// Creates a new CANopen node.
  explicit Node(io::TimerBase& timer, io::CanChannelBase& chan,
                const ::std::string& dcf_txt, const ::std::string& dcf_bin = "",
                uint8_t id = 0xff)
      : Node(nullptr, timer, chan, dcf_txt, dcf_bin, id) {}

  /**
   * Creates a new CANopen node. After creation, the node is in the NMT
   * 'Initialisation' state and does not yet create any services or perform any
   * communication. Call #Reset() to start the boot-up process.
   *
   * @param exec  the executor used to process I/O and CANopen events. If
   *              <b>exec</b> is a null pointer, the CAN channel executor is
   *              used.
   * @param timer the timer used for CANopen events. This timer MUST NOT be used
   *              for any other purpose.
   * @param chan  a CAN channel. This channel MUST NOT be used for any other
   *              purpose.
   * @param sdev  a pointer to a static device desciption.
   * @param id    the node-ID (in the range [1..127, 255]). If <b>id</b> is 255
   *              (unconfigured), the node-ID is obtained from the device
   *              description.
   */
  explicit Node(ev_exec_t* exec, io::TimerBase& timer, io::CanChannelBase& chan,
                const co_sdev* sdev, uint8_t id = 0xff);

  /// Creates a new CANopen node.
  explicit Node(io::TimerBase& timer, io::CanChannelBase& chan,
                const co_sdev* sdev, uint8_t id = 0xff)
      : Node(nullptr, timer, chan, sdev, id) {}

  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;

  virtual ~Node();

  /// Returns the executor used to process I/O and CANopen events.
  ev::Executor GetExecutor() const noexcept;

  /// Returns the underlying I/O context with which this context is registered.
  io::ContextBase GetContext() const noexcept;

  /// Returns the clock used by the timer.
  io::Clock GetClock() const noexcept;

  /**
   * Submits a wait operation. The completion task is submitted for execution
   * once the specified absolute timeout expires.
   */
  void SubmitWait(const time_point& t, io_tqueue_wait& wait);

  /**
   * Submits a wait operation. The completion task is submitted for execution
   * once the specified relative timeout expires.
   */
  void SubmitWait(const duration& d, io_tqueue_wait& wait);

  /**
   * Submits a wait operation. The completion task is submitted for execution
   * once the specified absolute timeout expires.
   *
   * @param t    the absolute expiration time of the wait operation.
   * @param exec the executor used to execute the completion task.
   * @param f    the function to be called on completion of the wait operation.
   */
  template <class F>
  void
  SubmitWait(const time_point& t, ev_exec_t* exec, F&& f) {
    SubmitWait(t,
               *io::make_timer_queue_wait_wrapper(exec, ::std::forward<F>(f)));
  }

  /**
   * Submits a wait operation. The completion task is submitted for execution
   * once the specified relative timeout expires.
   *
   * @param d    the relative expiration time of the wait operation.
   * @param exec the executor used to execute the completion task.
   * @param f    the function to be called on completion of the wait operation.
   */
  template <class F>
  void
  SubmitWait(const duration& d, ev_exec_t* exec, F&& f) {
    SubmitWait(d,
               *io::make_timer_queue_wait_wrapper(exec, ::std::forward<F>(f)));
  }

  /// Equivalent to `SubmitWait(t, nullptr, f)`.
  template <class F>
  typename ::std::enable_if<!::std::is_base_of<
      io_tqueue_wait, typename ::std::decay<F>::type>::value>::type
  SubmitWait(const time_point& t, F&& f) {
    SubmitWait(t, nullptr, ::std::forward<F>(f));
  }

  /// Equivalent to `SubmitWait(d, nullptr, f)`.
  template <class F>
  typename ::std::enable_if<!::std::is_base_of<
      io_tqueue_wait, typename ::std::decay<F>::type>::value>::type
  SubmitWait(const duration& d, F&& f) {
    SubmitWait(d, nullptr, ::std::forward<F>(f));
  }

  /**
   * Submits an asynchronous wait operation and creates a future which becomes
   * ready once the wait operation completes (or is canceled).
   *
   * @param exec  the executor used to execute the completion task.
   * @param t     the absolute expiration time of the wait operation.
   * @param pwait an optional address at which to store a pointer to the wait
   *              operation. This can be used to cancel the wait operation with
   *              CancelWait().
   *
   * @returns a future which holds an exception pointer on error.
   */
  ev::Future<void, ::std::exception_ptr> AsyncWait(
      ev_exec_t* exec, const time_point& t, io_tqueue_wait** pwait = nullptr);

  /**
   * Submits an asynchronous wait operation and creates a future which becomes
   * ready once the wait operation completes (or is canceled).
   *
   * @param exec  the executor used to execute the completion task.
   * @param d     the relative expiration time of the wait operation.
   * @param pwait an optional address at which to store a pointer to the wait
   *              operation. This can be used to cancel the wait operation with
   *              CancelWait().
   *
   * @returns a future which holds an exception pointer on error.
   */
  ev::Future<void, ::std::exception_ptr> AsyncWait(
      ev_exec_t* exec, const duration& d, io_tqueue_wait** pwait = nullptr);

  /// Equivalent to `AsyncWait(nullptr, t, pwait)`.
  ev::Future<void, ::std::exception_ptr>
  AsyncWait(const time_point& t, io_tqueue_wait** pwait = nullptr) {
    return AsyncWait(nullptr, t, pwait);
  }

  /// Equivalent to `AsyncWait(nullptr, d, pwait)`.
  ev::Future<void, ::std::exception_ptr>
  AsyncWait(const duration& d, io_tqueue_wait** pwait = nullptr) {
    return AsyncWait(nullptr, d, pwait);
  }

  /**
   * Cancels the specified wait operation if it is pending. If canceled, the
   * completion task is submitted for exection with <b>ec</b> =
   * `::std::errc::operation_canceled`.
   *
   * @returns true if the operation was canceled, and false if it was not
   * pending.
   */
  bool CancelWait(io_tqueue_wait& wait) noexcept;

  /**
   * Aborts the specified wait operation if it is pending. If aborted, the
   * completion task is _not_ submitted for execution.
   *
   * @returns true if the operation was aborted, and false if it was not
   * pending.
   */
  bool AbortWait(io_tqueue_wait& wait) noexcept;

  /**
   * Stops the specified CAN controller and submits asynchronous operations to
   * wait for the delay period, set the new bit rate, wait for the delay period
   * again, and restart the CAN controller.
   *
   * This function can be used to implement the
   * OnSwitchBitrate(int, ::std::chrono::milliseconds) callback in accordance
   * with CiA 305.
   *
   * @param ctrl    a CAN controller.
   * @param bitrate the new bit rate (in bit/s).
   * @param delay   the delay before and after the switch, during which the CAN
   *                controller is stopped.
   *
   * @returns a future which becomes ready once the CAN controller is restarted
   * or an error occurs.
   */
  ev::Future<void, ::std::exception_ptr> AsyncSwitchBitrate(
      io::CanControllerBase& ctrl, int bitrate,
      ::std::chrono::milliseconds delay);

  /**
   * Registers the function to be invoked when a CAN bus state change is
   * detected. Only a single function can be registered at any one time. If
   * <b>on_can_state</b> contains a callable function target, a copy of the
   * target is invoked _after_ OnCanState(io::CanState, io::CanState) completes.
   */
  void OnCanState(
      ::std::function<void(io::CanState, io::CanState)> on_can_state);

  /**
   * Registers the function to be invoked when an error is detected on the CAN
   * bus. Only a single function can be registered at any one time. If
   * <b>on_can_error</b> contains a callable function target, a copy of the
   * target is invoked _after_ OnCanError(io::CanError) completes.
   */
  void OnCanError(::std::function<void(io::CanError)> on_can_error);

  /**
   * (Re)starts the node. This function behaves as if an NMT 'reset node'
   * command has been received. Note that this function will cause the
   * invocation of #OnCommand() and therefore MUST NOT be called from that
   * function.
   */
  void Reset();

  /**
   * Starts the TIME producer, if it exists. The first time stamp will be sent
   * immediately. The rest will be sent after the specified interval, if
   * non-zero.
   */
  void StartTime(const duration& interval = duration::zero());

  /**
   * Starts the TIME producer, if it exists. The first time stamp will be sent
   * at the specified time. The rest will be sent after the specified interval,
   * if non-zero.
   */
  void StartTime(const time_point& start,
                 const duration& interval = duration::zero());

  /// Stops the TIME producer, if it exists.
  void StopTime();

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
   * Registers the function to be invoked when an NMT state change or boot-up
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

  /**
   * Registers the function to be invoked when the LSS master activates the bit
   * rate of all CANopen devices in the network. Only a single function can be
   * registered at any one time. If <b>on_switch_bitrate</b> contains a callable
   * function target, a copy of the target is invoked _after_
   * OnSwitchBitrate(int, ::std::chrono::milliseconds) completes.
   */
  void OnSwitchBitrate(::std::function<void(int, ::std::chrono::milliseconds)>
                           on_switch_bitrate);

 protected:
  /**
   * A recursive mutex-like object that can be used to postpone the transmission
   * of acyclic and event-driven Transmit-PDOs while the lock is held.
   */
  class TpdoEventMutex : public util::BasicLockable {
    friend class Node;

   public:
    void lock() override;
    void unlock() override;

   protected:
    TpdoEventMutex(Node& node_) noexcept : node(&node_) {}

    Node* node;
  };

  /**
   * Returns a pointer to the internal CAN network interface
   * from <lely/can/net.h>.
   */
  __can_net* net() const noexcept;

  /// Updates the CAN network time.
  void SetTime();

  /**
   * The function invoked when a CAN bus state change is detected. The state is
   * represented by one the `CanState::ACTIVE`, `CanState::PASSIVE`,
   * `CanState::BUSOFF`, `CanState::SLEEPING` or `CanState::STOPPED` values.
   *
   * The default implementation sends an EMCY message if the CAN bus is in error
   * passive mode or has recovered from bus off (see Table 26 in CiA 301
   * v4.2.0).
   *
   * @param new_state the current state of the CAN bus.
   * @param old_state the previous state of the CAN bus.
   */
  virtual void OnCanState(io::CanState new_state,
                          io::CanState old_state) noexcept;

  /**
   * The function invoked when an error is detected on the CAN bus.
   *
   * @param error the detected errors (any combination of `CanError::BIT`,
   *              `CanError::STUFF`, `CanError::CRC`, `CanError::FORM`,
   *              `CanError::ACK` and `CanError::OTHER`).
   */
  virtual void
  OnCanError(io::CanError error) noexcept {
    (void)error;
  }

  /**
   * Returns a pointer to the internal CANopen NMT master/slave service from
   * <lely/co/nmt.hpp>.
   */
  __co_nmt* nmt() const noexcept;

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
   */
  void RpdoRtr(int num = 0) noexcept;

  /**
   * Triggers the transmission of an acyclic or event-driven PDO.
   *
   * The transmission of PDOs can be postponed by locking #tpdo_event_mutex.
   *
   * @param num the PDO number (in the range [1..512]). If <b>num</b> is 0, the
   *            transmission of all PDOs is triggered.
   */
  void TpdoEvent(int num = 0) noexcept;

  /**
   * Triggers the transmission of a destination address mode multiplex PDO
   * (DAM-MPDO).
   *
   * @param num    the Transmit-PDO number (in the range [1..512]).
   * @param id     the node-ID.
   * @param idx    the remote object index.
   * @param subidx the remote object sub-index.
   * @param value  the value to be transmitted.
   */
  template <class T>
  typename ::std::enable_if<is_canopen_basic<T>::value && sizeof(T) <= 4,
                            void>::type
  DamMpdoEvent(int num, uint8_t id, uint16_t idx, uint8_t subidx, T value);

  /**
   * The recursive mutex used to postpone the transmission of acyclic and
   * event-driven PDOs triggered by TpdoWriteEvent() or WriteEvent().
   */
  TpdoEventMutex tpdo_event_mutex;

 private:
  void on_can_state(io::CanState new_state,
                    io::CanState old_state) noexcept final;
  void on_can_error(io::CanError error) noexcept final;

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

  /**
   * The function invoked when the LSS master activates the bit rate of all
   * CANopen devices in the network.
   *
   * @param bitrate the new bit rate (in bit/s).
   * @param delay   the delay before and after the switch, during which CAN
   *                frames MUST NOT be sent.
   */
  virtual void
  OnSwitchBitrate(int bitrate, ::std::chrono::milliseconds delay) noexcept {
    (void)bitrate;
    (void)delay;
  }

  /**
   * The function invoked then a request is received from the LSS master to
   * store the pending node-ID and bit rate to non-volatile memory. If this
   * function throws an exception, an error is reported to the master.
   *
   * The default implementation throws std::system_error (containing the
   * std::errc::operation_not_supported error code).
   *
   * @param id      the pending node-ID to be stored.
   * @param bitrate the pending bit rate (in bit/s) to be stored.
   */
  virtual void OnStore(uint8_t id, int bitrate);

  struct Impl_;
  ::std::unique_ptr<Impl_> impl_;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_NODE_HPP_
