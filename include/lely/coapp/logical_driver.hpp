/**@file
 * This header file is part of the C++ CANopen application library; it contains
 * the logical device driver interface declarations.
 *
 * @copyright 2019-2021 Lely Industries N.V.
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

#ifndef LELY_COAPP_LOGICAL_DRIVER_HPP_
#define LELY_COAPP_LOGICAL_DRIVER_HPP_

#include <lely/coapp/driver.hpp>

#include <string>
#include <utility>

namespace lely {

namespace canopen {

template <class = BasicDriver>
class BasicLogicalDriver;

/// The base class for drivers for logical devices on remote CANopen nodes.
template <>
class BasicLogicalDriver<BasicDriver> : LogicalDriverBase {
 public:
  using DriverBase::time_point;
  using duration = BasicMaster::duration;
  using TpdoEventMutex = BasicDriver::TpdoEventMutex;

  BasicLogicalDriver(BasicDriver& driver, int num = 1, uint32_t dev = 0);

  virtual ~BasicLogicalDriver();

  ev::Executor
  GetExecutor() const noexcept final {
    return driver.GetExecutor();
  }

  uint8_t
  netid() const noexcept final {
    return driver.netid();
  }

  uint8_t
  id() const noexcept final {
    return driver.id();
  }

  int
  Number() const noexcept final {
    return num_;
  }

  /// Returns the device type of the logical device on the remote node.
  uint32_t
  DeviceType() const noexcept {
    return dev_;
  }

  /**
   * Returns the device profile number of the logical device on the remote node,
   * or 0 if the device does not follow a standardized profile.
   */
  int
  Profile() const noexcept {
    return DeviceType() & 0xffff;
  }

  /**
   * Returns true if the remote node is ready (i.e., the NMT `boot slave`
   * process has successfully completed and no subsequent boot-up event has been
   * received) and false if not.
   *
   * @see BasicDriver::IsReady()
   */
  bool
  IsReady() const {
    return driver.IsReady();
  }

  /**
   * Indicates the occurrence of an error event on the remote node and triggers
   * the error handling process.
   *
   * @see BasicDriver::Error()
   */
  void
  Error() {
    driver.Error();
  }

  /**
   * Submits a wait operation. The completion task is submitted for execution
   * once the specified absolute timeout expires.
   *
   * @param t the absolute expiration time of the wait operation.
   * @param f the function to be called on completion of the wait operation.
   */
  template <class F>
  void
  SubmitWait(const time_point& t, F&& f) {
    return driver.SubmitWait(t, ::std::forward<F>(f));
  }

  /**
   * Submits a wait operation. The completion task is submitted for execution
   * once the specified relative timeout expires.
   *
   * @param d the relative expiration time of the wait operation.
   * @param f the function to be called on completion of the wait operation.
   */
  template <class F>
  void
  SubmitWait(const duration& d, F&& f) {
    return driver.SubmitWait(d, ::std::forward<F>(f));
  }

  /**
   * Submits an asynchronous wait operation and creates a future which becomes
   * ready once the wait operation completes (or is canceled).
   *
   * @param t the absolute expiration time of the wait operation.
   *
   * @returns a future which holds an exception pointer on error.
   */
  SdoFuture<void>
  AsyncWait(const time_point& t) {
    return driver.AsyncWait(t);
  }

  /**
   * Submits an asynchronous wait operation and creates a future which becomes
   * ready once the wait operation completes (or is canceled).
   *
   * @param d the relative expiration time of the wait operation.
   *
   * @returns a future which holds an exception pointer on error.
   */
  SdoFuture<void>
  AsyncWait(const duration& d) {
    return driver.AsyncWait(d);
  }

  /**
   * Equivalent to
   * #SubmitRead(uint16_t idx, uint8_t subidx, F&& con, const ::std::chrono::milliseconds& timeout),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T, class F>
  void
  SubmitRead(uint16_t idx, uint8_t subidx, F&& con) {
    driver.SubmitRead<T>(ObjectIndex(idx), subidx, ::std::forward<F>(con));
  }

  /**
   * Equivalent to
   * #SubmitRead(uint16_t idx, uint8_t subidx, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T, class F>
  void
  SubmitRead(uint16_t idx, uint8_t subidx, F&& con, ::std::error_code& ec) {
    driver.SubmitRead<T>(ObjectIndex(idx), subidx, ::std::forward<F>(con), ec);
  }

  /**
   * Equivalent to
   * #SubmitRead(uint16_t idx, uint8_t subidx, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T, class F>
  void
  SubmitRead(uint16_t idx, uint8_t subidx, F&& con,
             const ::std::chrono::milliseconds& timeout) {
    driver.SubmitRead<T>(ObjectIndex(idx), subidx, ::std::forward<F>(con),
                         timeout);
  }

  /**
   * Queues an asynchronous read (SDO upload) operation. This function reads the
   * value of a sub-object in a remote object dictionary.
   *
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param con     the confirmation function to be called on completion of the
   *                SDO request.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   * @param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   */
  template <class T, class F>
  void
  SubmitRead(uint16_t idx, uint8_t subidx, F&& con,
             const ::std::chrono::milliseconds& timeout,
             ::std::error_code& ec) {
    driver.SubmitRead<T>(ObjectIndex(idx), subidx, ::std::forward<F>(con),
                         timeout, ec);
  }

  /**
   * Equivalent to
   * #SubmitBlockRead(uint16_t idx, uint8_t subidx, F&& con, const ::std::chrono::milliseconds& timeout),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T, class F>
  void
  SubmitBlockRead(uint16_t idx, uint8_t subidx, F&& con) {
    driver.SubmitBlockRead<T>(ObjectIndex(idx), subidx, ::std::forward<F>(con));
  }

  /**
   * Equivalent to
   * #SubmitBlockRead(uint16_t idx, uint8_t subidx, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T, class F>
  void
  SubmitBlockRead(uint16_t idx, uint8_t subidx, F&& con,
                  ::std::error_code& ec) {
    driver.SubmitBlockRead<T>(ObjectIndex(idx), subidx, ::std::forward<F>(con),
                              ec);
  }

  /**
   * Equivalent to
   * #SubmitBlockRead(uint16_t idx, uint8_t subidx, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T, class F>
  void
  SubmitBlockRead(uint16_t idx, uint8_t subidx, F&& con,
                  const ::std::chrono::milliseconds& timeout) {
    driver.SubmitBlockRead<T>(ObjectIndex(idx), subidx, ::std::forward<F>(con),
                              timeout);
  }

  /**
   * Queues an asynchronous read (SDO block upload) operation. This function
   * reads the value of a sub-object in a remote object dictionary using SDO
   * block transfer. SDO block transfer is more effecient than segmented
   * transfer for large values, but may not be supported by the remote server.
   * If not, the operation will most likely fail with the #SdoErrc::NO_CS abort
   * code.
   *
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param con     the confirmation function to be called on completion of the
   *                SDO request.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   * @param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   */
  template <class T, class F>
  void
  SubmitBlockRead(uint16_t idx, uint8_t subidx, F&& con,
                  const ::std::chrono::milliseconds& timeout,
                  ::std::error_code& ec) {
    driver.SubmitBlockRead<T>(ObjectIndex(idx), subidx, ::std::forward<F>(con),
                              timeout, ec);
  }

  /**
   * Equivalent to
   * #SubmitWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con, const ::std::chrono::milliseconds& timeout),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T, class F>
  void
  SubmitWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con) {
    driver.SubmitWrite(ObjectIndex(idx), subidx, ::std::forward<T>(value),
                       ::std::forward<F>(con));
  }

  /**
   * Equivalent to
   * #SubmitWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T, class F>
  void
  SubmitWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con,
              ::std::error_code& ec) {
    driver.SubmitWrite(ObjectIndex(idx), subidx, ::std::forward<T>(value),
                       ::std::forward<F>(con), ec);
  }

  /**
   * Equivalent to
   * #SubmitWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T, class F>
  void
  SubmitWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con,
              const ::std::chrono::milliseconds& timeout) {
    driver.SubmitWrite(ObjectIndex(idx), subidx, ::std::forward<T>(value),
                       ::std::forward<F>(con), timeout);
  }

  /**
   * Queues an asynchronous write (SDO download) operation. This function writes
   * a value to a sub-object in a remote object dictionary.
   *
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param value   the value to be written.
   * @param con     the confirmation function to be called on completion of the
   *                SDO request.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   * @param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   */
  template <class T, class F>
  void
  SubmitWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con,
              const ::std::chrono::milliseconds& timeout,
              ::std::error_code& ec) {
    driver.SubmitWrite(ObjectIndex(idx), subidx, ::std::forward<T>(value),
                       ::std::forward<F>(con), timeout, ec);
  }

  /**
   * Equivalent to
   * #SubmitBlockWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con, const ::std::chrono::milliseconds& timeout),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T, class F>
  void
  SubmitBlockWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con) {
    driver.SubmitBlockWrite(ObjectIndex(idx), subidx, ::std::forward<T>(value),
                            ::std::forward<F>(con));
  }

  /**
   * Equivalent to
   * #SubmitBlockWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T, class F>
  void
  SubmitBlockWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con,
                   ::std::error_code& ec) {
    driver.SubmitBlockWrite(ObjectIndex(idx), subidx, ::std::forward<T>(value),
                            ::std::forward<F>(con), ec);
  }

  /**
   * Equivalent to
   * #SubmitBlockWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T, class F>
  void
  SubmitBlockWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con,
                   const ::std::chrono::milliseconds& timeout) {
    driver.SubmitBlockWrite(ObjectIndex(idx), subidx, ::std::forward<T>(value),
                            ::std::forward<F>(con), timeout);
  }

  /**
   * Queues an asynchronous write (SDO block download) operation. This function
   * writes a value to a sub-object in a remote object dictionary using SDO
   * block transfer. SDO block transfer is more effecient than segmented
   * transfer for large values, but may not be supported by the remote server.
   * If not, the operation will most likely fail with the #SdoErrc::NO_CS abort
   * code.
   *
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param value   the value to be written.
   * @param con     the confirmation function to be called on completion of the
   *                SDO request.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   * @param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   */
  template <class T, class F>
  void
  SubmitBlockWrite(uint16_t idx, uint8_t subidx, T&& value, F&& con,
                   const ::std::chrono::milliseconds& timeout,
                   ::std::error_code& ec) {
    driver.SubmitBlockWrite(ObjectIndex(idx), subidx, ::std::forward<T>(value),
                            ::std::forward<F>(con), timeout, ec);
  }

  /**
   * Equivalent to
   * #AsyncRead(uint16_t idx, uint8_t subidx, const ::std::chrono::milliseconds& timeout),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T>
  SdoFuture<T>
  AsyncRead(uint16_t idx, uint8_t subidx) {
    return driver.AsyncRead<T>(ObjectIndex(idx), subidx);
  }

  /**
   * Queues an asynchronous read (SDO upload) operation and creates a future
   * which becomes ready once the request completes (or is canceled).
   *
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * @returns a future which holds the received value on success and the SDO
   * error on failure.
   */
  template <class T>
  SdoFuture<T>
  AsyncRead(uint16_t idx, uint8_t subidx,
            const ::std::chrono::milliseconds& timeout) {
    return driver.AsyncRead<T>(ObjectIndex(idx), subidx, timeout);
  }

  /**
   * Equivalent to
   * #AsyncBlockRead(uint16_t idx, uint8_t subidx, const ::std::chrono::milliseconds& timeout),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T>
  SdoFuture<T>
  AsyncBlockRead(uint16_t idx, uint8_t subidx) {
    return driver.AsyncBlockRead<T>(ObjectIndex(idx), subidx);
  }

  /**
   * Queues an asynchronous read (SDO block upload) operation and creates a
   * future which becomes ready once the request completes (or is canceled).
   * This function uses SDO block transfer, which is more effecient than
   * segmented transfer for large values, but may not be supported by the remote
   * server. If not, the operation will most likely fail with the
   * #SdoErrc::NO_CS abort code.
   *
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * @returns a future which holds the received value on success and the SDO
   * error on failure.
   */
  template <class T>
  SdoFuture<T>
  AsyncBlockRead(uint16_t idx, uint8_t subidx,
                 const ::std::chrono::milliseconds& timeout) {
    return driver.AsyncBlockRead<T>(ObjectIndex(idx), subidx, timeout);
  }

  /**
   * Equivalent to
   * #AsyncWrite(uint16_t idx, uint8_t subidx, T&& value, const ::std::chrono::milliseconds& timeout),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T>
  SdoFuture<void>
  AsyncWrite(uint16_t idx, uint8_t subidx, T&& value) {
    return driver.AsyncWrite(ObjectIndex(idx), subidx,
                             ::std::forward<T>(value));
  }

  /**
   * Queues an asynchronous write (SDO download) operation and creates a future
   * which becomes ready once the request completes (or is canceled).
   *
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param value   the value to be written.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * @returns a future which holds the SDO error on failure.
   */
  template <class T>
  SdoFuture<void>
  AsyncWrite(uint16_t idx, uint8_t subidx, T&& value,
             const ::std::chrono::milliseconds& timeout) {
    return driver.AsyncWrite(ObjectIndex(idx), subidx, ::std::forward<T>(value),
                             timeout);
  }

  /**
   * Equivalent to
   * #AsyncBlockWrite(uint16_t idx, uint8_t subidx, T&& value, const ::std::chrono::milliseconds& timeout),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T>
  SdoFuture<void>
  AsyncBlockWrite(uint16_t idx, uint8_t subidx, T&& value) {
    return driver.AsyncBlockWrite(ObjectIndex(idx), subidx,
                                  ::std::forward<T>(value));
  }

  /**
   * Queues an asynchronous write (SDO block download) operation and creates a
   * future which becomes ready once the request completes (or is canceled).
   * This function uses SDO block transfer, which is more effecient than
   * segmented transfer for large values, but may not be supported by the remote
   * server. If not, the operation will most likely fail with the
   * #SdoErrc::NO_CS abort code.
   *
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param value   the value to be written.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   *
   * @returns a future which holds the SDO error on failure.
   */
  template <class T>
  SdoFuture<void>
  AsyncBlockWrite(uint16_t idx, uint8_t subidx, T&& value,
                  const ::std::chrono::milliseconds& timeout) {
    return driver.AsyncBlockWrite(ObjectIndex(idx), subidx,
                                  ::std::forward<T>(value), timeout);
  }

  /**
   * Schedules the specified Callable object for execution by the executor for
   * this driver.
   *
   * @see GetExecutor()
   */
  template <class F, class... Args>
  void
  Post(F&& f, Args&&... args) {
    GetExecutor().post(::std::forward<F>(f), ::std::forward<Args>(args)...);
  }

  /// A reference to the master with which #driver is registered.
  BasicMaster& master;

  /**
   * A reference to the driver with which this logical device driver is
   * registered.
   */
  BasicDriver& driver;

  class RpdoMapped {
    friend class BasicLogicalDriver;

   public:
    BasicMaster::ConstObject
    operator[](uint16_t idx) const {
      return self_.driver.rpdo_mapped[self_.ObjectIndex(idx)];
    }

   private:
    explicit RpdoMapped(BasicLogicalDriver& self) noexcept : self_(self) {}

    BasicLogicalDriver& self_;
  } rpdo_mapped;

  class TpdoMapped {
    friend class BasicLogicalDriver;

   public:
    BasicMaster::Object
    operator[](uint16_t idx) {
      return self_.driver.tpdo_mapped[self_.ObjectIndex(idx)];
    }

    BasicMaster::ConstObject
    operator[](uint16_t idx) const {
      const auto& tpdo_mapped = self_.driver.tpdo_mapped;
      return tpdo_mapped[self_.ObjectIndex(idx)];
    }

   private:
    explicit TpdoMapped(BasicLogicalDriver& self) noexcept : self_(self) {}

    BasicLogicalDriver& self_;
  } tpdo_mapped;

  /// @see BasicMaster::tpdo_event_mutex
  TpdoEventMutex& tpdo_event_mutex;

 protected:
  void
  OnCanState(io::CanState /*new_state*/,
             io::CanState /*old_state*/) noexcept override {}

  void
  OnCanError(io::CanError /*error*/) noexcept override {}

  void
  OnRpdoWrite(uint16_t /*idx*/, uint8_t /*subidx*/) noexcept override {}

  void
  OnCommand(NmtCommand /*cs*/) noexcept override {}

  void
  // NOLINTNEXTLINE(readability/casting)
  OnHeartbeat(bool /*occurred*/) noexcept override {}

  void
  OnState(NmtState /*st*/) noexcept override {}

  void
  OnSync(uint8_t /*cnt*/, const time_point& /*t*/) noexcept override {}

  void
  OnSyncError(uint16_t /*eec*/, uint8_t /*er*/) noexcept override {}

  void
  OnTime(const ::std::chrono::system_clock::time_point& /*abs_time*/) noexcept
      override {}

  void
  OnEmcy(uint16_t /*eec*/, uint8_t /*er*/,
         uint8_t /*msef*/[5]) noexcept override {}

  void
  // NOLINTNEXTLINE(readability/casting)
  OnNodeGuarding(bool /*occurred*/) noexcept override {}

  void
  OnBoot(NmtState /*st*/, char /*es*/,
         const ::std::string& /*what*/) noexcept override {}

  void
  OnConfig(::std::function<void(::std::error_code ec)> res) noexcept override {
    res(::std::error_code{});
  }

  void
  OnDeconfig(
      ::std::function<void(::std::error_code ec)> res) noexcept override {
    res(::std::error_code{});
  }

  /**
   * Converts an object index, if it is part of the standardized profile area,
   * from the first logical device to the actual logical device. This allows the
   * driver to treat index 6000..67FF as the profile area, even if
   * `Number() != 1`.
   */
  uint16_t
  ObjectIndex(uint16_t idx) const noexcept {
    return (idx >= 0x6000 && idx <= 0x67ff) ? idx + (num_ - 1) * 0x800 : idx;
  }

 private:
  SdoFuture<void> AsyncConfig() final;
  SdoFuture<void> AsyncDeconfig() final;

  int num_{1};
  uint32_t dev_{0};
};

/// The base class for drivers for logical devices on remote CANopen nodes.
template <class Driver>
class BasicLogicalDriver : public BasicLogicalDriver<BasicDriver> {
 public:
  BasicLogicalDriver(Driver& driver_, int num = 1, uint32_t dev = 0)
      : BasicLogicalDriver<BasicDriver>(driver_, num, dev), driver(driver_) {}

  /**
   * A reference to the driver with which this logical device driver is
   * registered.
   */
  Driver& driver;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_LOGICAL_DRIVER_HPP_
