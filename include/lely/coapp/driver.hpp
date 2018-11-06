/*!\file
 * This header file is part of the C++ CANopen application library; it contains
 * the remote node driver interface declarations.
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

#ifndef LELY_COCPP_DRIVER_HPP_
#define LELY_COCPP_DRIVER_HPP_

#include <lely/aio/loop.hpp>
#include <lely/coapp/master.hpp>

namespace lely {

namespace canopen {

//! The abstract driver interface for a remote CANopen node.
class DriverBase {
 public:
  using CanState = BasicMaster::CanState;
  using CanError = BasicMaster::CanError;

  using time_point = BasicMaster::time_point;

  DriverBase() = default;

  DriverBase(const DriverBase&) = delete;

  DriverBase& operator=(const DriverBase&) = delete;

  virtual ~DriverBase() = default;

  /*!
   * Returns the executor used to execute event handlers for this driver,
   * including SDO confirmation functions.
   */
  virtual aio::ExecutorBase GetExecutor() const noexcept = 0;

  //! Returns the network-ID.
  virtual uint8_t netid() const noexcept = 0;

  //! Returns the node-ID.
  virtual uint8_t id() const noexcept = 0;

  /*!
   * The function invoked when a CAN bus state change is detected.
   *
   * \see BasicMaster::OnCanState()
   */
  virtual void OnCanState(CanState new_state, CanState old_state) noexcept = 0;

  /*!
   * The function invoked when an error is detected on the CAN bus.
   *
   * \see BasicMaster::OnCanError()
   */
  virtual void OnCanError(CanError error) noexcept = 0;

  /*!
   * The function invoked when an NMT state change occurs on the master.
   *
   * \param cs the NMT command specifier.
   *
   * \see BasicMaster::OnCommand()
   */
  virtual void OnCommand(NmtCommand cs) noexcept = 0;

  /*!
   * The function invoked when a node guarding timeout event occurs or is
   * resolved for the remote node. Note that depending on the value of object
   * 1029:01 (Error behavior object) in the object dictionary of the master, the
   * occurrence of a node guarding event MAY trigger an NMT state transition on
   * the master. If so, this function is called _after_ the state change
   * completes.
   *
   * \param occurred `true` if the node guarding event occurred, `false` if it
   *                 was resolved.
   *
   * \see BasicMaster::OnNodeGuarding()
   */
  virtual void OnNodeGuarding(bool occurred) noexcept = 0;

  /*!
   * The function invoked when a heartbeat timeout event occurs or is resolved
   * for the remote node. Note that depending on the value of object 1029:01
   * (Error behavior object) in the object dictionary of the master, the
   * occurrence of a heartbeat timeout event MAY trigger an NMT state transition
   * on the master. If so, this function is called _after_ the state change
   * completes.
   *
   * \param occurred `true` if the heartbeat timeout event occurred, `false` if
   *                 it was resolved.
   *
   * \see BasicMaster::OnHeartbeat()
   */
  virtual void OnHeartbeat(bool occurred) noexcept = 0;

  /*!
   * The function invoked when an NMT state change or boot-up event is detected
   * for the remote node by the heartbeat protocol.
   *
   * \param st the state of the remote node. Note that the NMT sub-states
   *           #NmtState::RESET_NODE and #NmtState::RESET_COMM are never
   *           reported for remote nodes.
   *
   * \see BasicMaster::OnState()
   */
  virtual void OnState(NmtState st) noexcept = 0;

  /*!
   * The function invoked when the NMT 'boot slave' process completes for the
   * remote node.
   *
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
   * \see BasicMaster::OnBoot()
   */
  virtual void OnBoot(NmtState st, char es, const ::std::string& what)
      noexcept = 0;

  /*!
   * The function invoked when the 'update configuration' step is reached during
   * the NMT 'boot slave' process of the remote node. The 'boot slave' process
   * is halted until the result of the 'update configuration' step is
   * communicated to the master.
   *
   * Note that `OnConfig()` MUST be a non-blocking function; the configuration
   * update MUST be executed asynchronously or run in a different thread.
   *
   * \param res the function to invoke on completion of the 'update
   *            configuration' step. The argument to \a res is the result: 0 on
   *            success, or an SDO abort code on error.
   *
   * \see BasicMaster::OnConfig()
   */
  virtual void OnConfig(::std::function<void(::std::error_code ec)> res)
      noexcept = 0;

  /*!
   * The function invoked when a Receive-PDO is processed by the master. In case
   * of a PDO length mismatch error, #OnRpdoError() is invoked after this
   * function.
   *
   * \param num the PDO number (in the range [1..512]).
   * \param ec  the SDO abort code:
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
   * \param p   a pointer to the bytes received.
   * \param n   the number of bytes at \a p.
   *
   * \see BasicMaster::OnRpdo()
   */
  virtual void OnRpdo(int num, ::std::error_code ec, const void* p,
                      ::std::size_t n) noexcept = 0;

  /*!
   * The function invoked when a Receive-PDO length mismatch or timeout error
   * occurs on the master.
   *
   * \param num the PDO number (in the range [1..512]).
   * \param eec the emergency error code:
   *            - 0x8210: PDO not processed due to length error;
   *            - 0x8220: PDO length exceeded;
   *            - 0x8250: RPDO timeout.
   * \param er  the error register (0x10).
   *
   * \see BasicMaster::OnRpdoError()
   */
  virtual void OnRpdoError(int num, uint16_t eec, uint8_t er) noexcept = 0;

  /*!
   * The function invoked after a Transmit-PDO is sent by the master or an error
   * occurs.
   *
   * \param num the PDO number (in the range [1..512]).
   * \param ec  the SDO abort code :
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
   * \param p   a pointer to the bytes sent.
   * \param n   the number of bytes at \a p.
   *
   * \see BasicMaster::OnTpdo()
   */
  virtual void OnTpdo(int num, ::std::error_code ec, const void* p,
                      ::std::size_t n) noexcept = 0;

  /*!
   * The function invoked when a SYNC message is sent/received by the master.
   * Note that this function is called _after_ all PDOs are processed/sent.
   *
   * \param cnt the counter (in the range [1..240]), or 0 if the SYNC message is
   *            empty.
   * \param t   the time at which the SYNC message was sent/received.
   *
   * \see BasicMaster::OnSync()
   */
  virtual void OnSync(uint8_t cnt, const Node::time_point& t) noexcept = 0;

  /*!
   * The function invoked when the data length of a received SYNC message does
   * not match.
   *
   * \param eec the emergency error code (0x8240).
   * \param er  the error register (0x10).
   *
   * \see BasicMaster::OnSyncError()
   */
  virtual void OnSyncError(uint16_t eec, uint8_t er) noexcept = 0;

  /*!
   * The function invoked when a TIME message is received by the master.
   *
   * \a abs_time a time point representing the received time stamp.
   *
   * \see BasicMaster::OnTime()
   */
  virtual void OnTime(const ::std::chrono::system_clock::time_point& abs_time)
      noexcept = 0;

  /*!
   * The function invoked when an EMCY message is received from the remote node.
   *
   * \param eec  the emergency error code.
   * \param er   the error register.
   * \param msef the manufacturer-specific error code.
   *
   * \see BasicMaster::OnEmcy()
   */
  virtual void OnEmcy(uint16_t eec, uint8_t er, uint8_t msef[5]) noexcept = 0;
};

//! The base class for drivers for remote CANopen nodes.
class LELY_COAPP_EXTERN BasicDriver : private DriverBase {
  friend class BasicMaster;

 public:
  using DriverBase::time_point;

  /*!
   * Creates a new driver for a remote CANopen node and registers it with the
   * master.
   *
   * \param loop   the event loop used to create promises and futures.
   * \param exec   the executor used to execute event handlers for this driver,
   *               including SDO confirmation functions. The executor SHOULD be
   *               based on \a loop.
   * \param master a reference to a CANopen master.
   * \param id     the node-ID of the remote node (in the range [1..127]).
   *
   * \throws std::out_of_range if the node-ID is invalid or already registered.
   */
  BasicDriver(aio::LoopBase& loop, aio::ExecutorBase& exec, BasicMaster& master,
              uint8_t id);

  virtual ~BasicDriver();

  //! Returns the event loop used to create promises and futures.
  aio::LoopBase GetLoop() const noexcept { return loop_; }

  aio::ExecutorBase
  GetExecutor() const noexcept final override { return exec_; }

  uint8_t netid() const noexcept final override;

  uint8_t id() const noexcept final override { return id_; }

  /*!
   * Indicates the occurrence of an error event on the remote node and triggers
   * the error handling process.
   *
   * \see BasicMaster::Error()
   */
  void Error() { master.Error(id()); }

  /*!
   * Equivalent to
   * #SubmitRead(uint16_t idx, uint8_t subidx, F&& con, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T, class F>
  void
  SubmitRead(uint16_t idx, uint8_t subidx, F&& con) {
    auto exec = GetExecutor();
    master.SubmitRead<T>(id(), idx, subidx, exec, ::std::forward<F>(con));
  }

  /*!
   * Equivalent to
   * #SubmitRead(uint16_t idx, uint8_t subidx, F&& con, const Sdo::duration& timeout, ::std::error_code& ec),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T, class F>
  void
  SubmitRead(uint16_t idx, uint8_t subidx, F&& con, ::std::error_code& ec) {
    auto exec = GetExecutor();
    master.SubmitRead<T>(id(), idx, subidx, exec, ::std::forward<F>(con), ec);
  }

  /*!
   * Equivalent to
   * #SubmitRead(uint16_t idx, uint8_t subidx, F&& con, const Sdo::duration& timeout, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T, class F>
  void
  SubmitRead(uint16_t idx, uint8_t subidx, F&& con,
             const Sdo::duration& timeout) {
    auto exec = GetExecutor();
    master.SubmitRead<T>(id(), idx, subidx, exec, ::std::forward<F>(con),
                         timeout);
  }

  /*!
   * Queues an asynchronous read (SDO upload) operation. This function reads the
   * value of a sub-object in the remote object dictionary.
   *
   * \param idx     the object index.
   * \param subidx  the object sub-index.
   * \param con     the confirmation function to be called on completion of the
   *                SDO request.
   * \param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   * \param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   */
  template <class T, class F>
  void
  SubmitRead(uint16_t idx, uint8_t subidx, F&& con,
             const Sdo::duration& timeout, ::std::error_code& ec) {
    auto exec = GetExecutor();
    master.SubmitRead<T>(id(), idx, subidx, exec, ::std::forward<F>(con),
                         timeout, ec);
  }

  /*!
   * Equivalent to
   * #SubmitWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T, class F>
  void
  SubmitWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con) {
    auto exec = GetExecutor();
    master.SubmitWrite<T>(id(), idx, subidx, ::std::forward<T>(value), exec,
                          ::std::forward<F>(con));
  }

  /*!
   * Equivalent to
   * #SubmitWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con, const Sdo::duration& timeout, ::std::error_code& ec),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T, class F>
  void
  SubmitWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con,
              ::std::error_code& ec) {
    auto exec = GetExecutor();
    master.SubmitWrite<T>(id(), idx, subidx, ::std::forward<T>(value), exec,
                          ::std::forward<F>(con), ec);
  }

  /*!
   * Equivalent to
   * #SubmitWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con, const Sdo::duration& timeout, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T, class F>
  void
  SubmitWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con,
              const Sdo::duration& timeout) {
    auto exec = GetExecutor();
    master.SubmitWrite<T>(id(), idx, subidx, ::std::forward<T>(value), exec,
                          ::std::forward<F>(con), timeout);
  }

  /*!
   * Queues an asynchronous write (SDO download) operation. This function writes
   * a value to a sub-object in the remote object dictionary.
   *
   * \param idx     the object index.
   * \param subidx  the object sub-index.
   * \param value   the value to be written.
   * \param con     the confirmation function to be called on completion of the
   *                SDO request.
   * \param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   * \param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   */
  template <class T, class F>
  void
  SubmitWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con,
              const Sdo::duration& timeout, ::std::error_code& ec) {
    auto exec = GetExecutor();
    master.SubmitWrite<T>(id(), idx, subidx, ::std::forward<T>(value), exec,
                          ::std::forward<F>(con), timeout, ec);
  }

  /*!
   * Equivalent to
   * #AsyncRead(uint16_t idx, uint8_t subidx, const Sdo::duration& timeout),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T>
  aio::Future<::std::tuple<::std::error_code, T>>
  AsyncRead(uint16_t idx, uint8_t subidx) {
    auto loop = GetLoop();
    auto exec = GetExecutor();
    return master.AsyncRead<T>(loop, exec, id(), idx, subidx);
  }

  /*!
   * Queues an asynchronous read (SDO upload) operation and returns a future.
   *
   * \param idx     the object index.
   * \param subidx  the object sub-index.
   * \param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * \returns a future which, on completion, holds the SDO abort code and the
   * received value. Note that if the client-SDO service is not available, the
   * returned future is invalid.
   */
  template <class T>
  aio::Future<::std::tuple<::std::error_code, T>>
  AsyncRead(uint16_t idx, uint8_t subidx, const Sdo::duration& timeout) {
    auto loop = GetLoop();
    auto exec = GetExecutor();
    return master.AsyncRead<T>(loop, exec, id(), idx, subidx, timeout);
  }

  /*!
   * Equivalent to
   * #AsyncWrite(uint16_t idx, uint8_t subidx, T&& value, const Sdo::duration& timeout),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T>
  aio::Future<::std::error_code>
  AsyncWrite(uint16_t idx, uint8_t subidx, T&& value) {
    auto loop = GetLoop();
    auto exec = GetExecutor();
    return master.AsyncWrite(loop, exec, id(), idx, subidx,
                             ::std::forward<T>(value));
  }

  /*!
   * Queues an asynchronous write (SDO download) operation and returns a future.
   *
   * \param idx     the object index.
   * \param subidx  the object sub-index.
   * \param value   the value to be written.
   * \param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * \returns a future which, on completion, holds the SDO abort code. Note that
   * if the client-SDO service is not available, the returned future is invalid.
   */
  template <class T>
  aio::Future<::std::error_code>
  AsyncWrite(uint16_t idx, uint8_t subidx, T&& value,
             const Sdo::duration& timeout) {
    auto loop = GetLoop();
    auto exec = GetExecutor();
    return master.AsyncWrite(loop, exec, id(), idx, subidx,
                             ::std::forward<T>(value), timeout);
  }

  /*!
   * Equivalent to
   * #RunRead(uint16_t idx, uint8_t subidx, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T>
  T
  RunRead(uint16_t idx, uint8_t subidx) {
    ::std::error_code ec;
    auto r = RunRead<T>(idx, subidx, ec);
    if (ec)
      throw SdoError(netid(), id(), idx, subidx, ec, "RunRead");
    return r;
  }

  /*!
   * Equivalent to
   * #RunRead(uint16_t idx, uint8_t subidx, const Sdo::duration& timeout, ::std::error_code& ec),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T>
  T
  RunRead(uint16_t idx, uint8_t subidx, ::std::error_code& ec) {
    T value {};
    auto future = AsyncRead<T>(idx, subidx);
    if (future) {
      auto t = future.Get();
      ec = ::std::get<0>(t);
      value = ::std::move(::std::get<1>(t));
    } else {
      ec = SdoErrc::NO_SDO;
    }
    return value;
  }

  /*!
   * Equivalent to
   * #RunRead(uint16_t idx, uint8_t subidx, const Sdo::duration& timeout, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T>
  T
  RunRead(uint16_t idx, uint8_t subidx, const Sdo::duration& timeout) {
    ::std::error_code ec;
    auto r = RunRead<T>(idx, subidx, timeout, ec);
    if (ec)
      throw SdoError(netid(), id(), idx, subidx, ec, "RunRead");
    return r;
  }

  /*!
   * Queues an asynchronous read (SDO upload) operation and runs the event loop
   * until the operation is complete.
   *
   * \param idx     the object index.
   * \param subidx  the object sub-index.
   * \param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   * \param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   *
   * \returns the received value.
   */
  template <class T>
  T
  RunRead(uint16_t idx, uint8_t subidx, const Sdo::duration& timeout,
          ::std::error_code& ec) {
    T value {};
    auto future = AsyncRead<T>(idx, subidx, timeout);
    if (future) {
      auto t = future.Get();
      ec = ::std::get<0>(t);
      value = ::std::move(::std::get<1>(t));
    } else {
      ec = SdoErrc::NO_SDO;
    }
    return value;
  }

  /*!
   * Equivalent to
   * #RunWrite(uint16_t idx, uint8_t subidx, T&& value, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T>
  void
  RunWrite(uint16_t idx, uint8_t subidx, T&& value) {
    ::std::error_code ec;
    RunWrite(idx, subidx, ::std::forward<T>(value), ec);
    if (ec)
      throw SdoError(netid(), id(), idx, subidx, ec, "RunWrite");
  }

  /*!
   * Equivalent to
   * #RunWrite(uint16_t idx, uint8_t subidx, T&& value, const Sdo::duration& timeout, ::std::error_code& ec),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T>
  void
  RunWrite(uint16_t idx, uint8_t subidx, T&& value, ::std::error_code& ec) {
    auto future = AsyncWrite(idx, subidx, ::std::forward<T>(value));
    ec = future ? future.Get() : SdoErrc::NO_SDO;
  }

  /*!
   * Equivalent to
   * #RunWrite(uint16_t idx, uint8_t subidx, T&& value, const Sdo::duration& timeout, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T>
  void
  RunWrite(uint16_t idx, uint8_t subidx, T&& value,
           const Sdo::duration& timeout) {
    ::std::error_code ec;
    RunWrite(idx, subidx, ::std::forward<T>(value), timeout, ec);
    if (ec)
      throw SdoError(netid(), id(), idx, subidx, ec, "RunWrite");
  }

  /*!
   * Queues an asynchronous write (SDO download) operation and runs the event
   * loop until the operation is complete.
   *
   * \param idx     the object index.
   * \param subidx  the object sub-index.
   * \param value   the value to be written.
   * \param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   * \param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   */
  template <class T>
  void
  RunWrite(uint16_t idx, uint8_t subidx, T&& value,
           const Sdo::duration& timeout, ::std::error_code& ec) {
    auto future = AsyncWrite(idx, subidx, ::std::forward<T>(value), timeout);
    ec = future ? future.Get() : SdoErrc::NO_SDO;
  }

  //! A reference to the master with which this driver is registered.
  BasicMaster& master;

  /*!
   * Schedules the specified Callable object for execution by the executor for
   * this driver.
   *
   * \see GetExecutor()
   */
  template <class F>
  void
  Post(F&& f) {
    auto* op = new aio::TaskWrapper(
        [=](::std::error_code ec) { if (!ec) f(); }
    );
    GetExecutor().Post(*op);
  }

 private:
  void
  OnCanState(CanState /*new_state*/, CanState /*old_state*/) noexcept override {
  }

  void OnCanError(CanError /*error*/) noexcept override {}

  void OnCommand(NmtCommand /*cs*/) noexcept override {}

  void OnNodeGuarding(bool /*occurred*/) noexcept override {}

  void OnHeartbeat(bool /*occurred*/) noexcept override {}

  void OnState(NmtState /*st*/) noexcept override {}

  void OnBoot(NmtState /*st*/, char /*es*/, const ::std::string& /*what*/)
      noexcept override {}

  void
  OnConfig(::std::function<void(::std::error_code ec)> res) noexcept override {
    res(::std::error_code());
  }

  void
  OnRpdo(int /*num*/, ::std::error_code /*ec*/, const void* /*p*/,
         ::std::size_t /*n*/) noexcept override {}

  void
  OnRpdoError(int /*num*/, uint16_t /*eec*/, uint8_t /*er*/) noexcept override {
  }

  void
  OnTpdo(int /*num*/, ::std::error_code /*ec*/, const void* /*p*/,
         ::std::size_t /*n*/) noexcept override {}

  void OnSync(uint8_t /*cnt*/, const time_point& /*t*/) noexcept override {}

  void OnSyncError(uint16_t /*eec*/, uint8_t /*er*/) noexcept override {}

  void
  OnTime(const ::std::chrono::system_clock::time_point& /*abs_time*/) noexcept
      override {}

  void
  OnEmcy(uint16_t /*eec*/, uint8_t /*er*/, uint8_t /*msef*/[5]) noexcept
      override {}

  aio_loop_t* loop_ { nullptr };
  aio_exec_t* exec_ { nullptr };
  uint8_t id_ { 0xff };
};

namespace detail {

/*!
 * A base class for #lely::canopen::LoopDriver, containing an event loop and the
 * associated executor.
 */
class LoopDriverBase {
 protected:
  aio::Loop loop {};
  aio::Executor exec { loop };
};

}  // namespace detail

//! A CANopen driver running its own dedicated event loop in a separate thread.
class LELY_COAPP_EXTERN LoopDriver
    : private detail::LoopDriverBase, public BasicDriver {
 public:
  LoopDriver(BasicMaster& master, uint8_t id);
  ~LoopDriver();

 private:
  struct Impl_;
  ::std::unique_ptr<Impl_> impl_;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COCPP_DRIVER_HPP_
