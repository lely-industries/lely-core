/**@file
 * This header file is part of the C++ CANopen application library; it contains
 * the declarations for the remote node driver containing an event loop.
 *
 * @copyright 2018-2021 Lely Industries N.V.
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

#ifndef LELY_COAPP_LOOP_DRIVER_HPP_
#define LELY_COAPP_LOOP_DRIVER_HPP_

#include <lely/coapp/driver.hpp>
#include <lely/ev/loop.hpp>
#include <lely/ev/strand.hpp>

#include <memory>
#include <utility>

namespace lely {

namespace canopen {

namespace detail {

/**
 * A base class for #lely::canopen::LoopDriver, containing an event loop and the
 * associated executor.
 */
class LoopDriverBase {
 protected:
  ev::Loop loop{};
  ev::Strand strand{loop.get_executor()};
};

}  // namespace detail

/// A CANopen driver running its own dedicated event loop in a separate thread.
class LoopDriver : detail::LoopDriverBase, public BasicDriver {
 public:
  /**
   * Creates a new CANopen driver and starts a new event loop in a separate
   * thread to execute event handlers (and SDO confirmation functions).
   *
   * @param master a reference to a CANopen master.
   * @param id     the node-ID of the remote node (in the range [1..127]).
   *
   * @throws std::out_of_range if the node-ID is invalid or already registered.
   */
  explicit LoopDriver(AsyncMaster& master, uint8_t id);

  /**
   * Stops the event loop and terminates the thread in which it was running
   * before destroying the driver.
   *
   * @see AsyncStopped()
   */
  ~LoopDriver();

  /// Returns a reference to the dedicated event loop of the driver.
  ev::Loop&
  GetLoop() noexcept {
    return loop;
  }

  /// Returns the strand executor associated with the event loop of the driver.
  ev::Executor
  GetStrand() const noexcept {
    return strand;
  }

  /**
   * Stops the dedicated event loop of the driver and waits until the thread
   * running the event loop finishes its execution. If logical drivers have been
   * registered, this function SHOULD be invoked before those drivers are
   * destroyed. Otherwise pending tasks for those drivers may remain on the
   * event loop.
   *
   * This function can be called more than once and from multiple threads, but
   * only the first invocation waits for the thread to finish.
   */
  void Join();

  /**
   * Returns a future which becomes ready once the dedicated event loop of the
   * driver is stopped and the thread is (about to be) terminated.
   */
  ev::Future<void, void> AsyncStoppped() noexcept;

  /**
   * Schedules the specified Callable object for execution by the event loop for
   * this driver.
   *
   * @see GetStrand().
   */
  template <class F, class... Args>
  void
  Defer(F&& f, Args&&... args) {
    GetStrand().post(::std::forward<F>(f), ::std::forward<Args>(args)...);
  }

  /**
   * Waits for the specified future to become ready by running pending tasks on
   * the dedicated event loop of the driver.
   *
   * This function MUST only be called from tasks running on that event loop.
   *
   * @returns the value stored in the future on success.
   *
   * @throws the exception stored in the future on failure.
   *
   * @see GetLoop().
   */
  template <class T>
  T
  Wait(SdoFuture<T> f) {
    GetLoop().wait(f);
    try {
      return f.get().value();
    } catch (const ev::future_not_ready& e) {
      util::throw_error_code("Wait", ::std::errc::operation_canceled);
    }
  }

  /**
   * Waits for the specified future to become ready by running pending tasks on
   * the dedicated event loop of the driver. The error code (0 on success) is
   * stored in <b>ec</b>.
   *
   * This function MUST only be called from tasks running on that event loop.
   *
   * @returns the value stored in the future on success, or an empty value on
   * error.
   *
   * @see GetLoop().
   */
  template <class T>
  typename ::std::enable_if<!::std::is_void<T>::value, T>::type
  Wait(SdoFuture<T> f, ::std::error_code& ec) {
    GetLoop().wait(f, ec);
    try {
      return f.get().value();
    } catch (const ::std::system_error& e) {
      ec = e.code();
    } catch (const ev::future_not_ready& e) {
      ec = ::std::make_error_code(::std::errc::operation_canceled);
    }
    return T{};
  }

  /**
   * Waits for the specified future to become ready by running pending tasks on
   * the dedicated event loop of the driver. The error code (0 on success) is
   * stored in <b>ec</b>.
   *
   * This function MUST only be called from tasks running on that event loop.
   *
   * @throws the exception stored in the future on failure.
   *
   * @see GetLoop().
   */
  void Wait(SdoFuture<void> f, ::std::error_code& ec);

  /**
   * Runs the event loop for <b>usec</b> microseconds. This function is
   * equivalent to `Wait(AsyncWait(::std::chrono::microseconds(usec)))`.
   */
  void USleep(uint_least64_t usec);

  /**
   * Runs the event loop for <b>usec</b> microseconds. This function is
   * equivalent to `Wait(AsyncWait(::std::chrono::microseconds(usec)), ec)`.
   */
  void USleep(uint_least64_t usec, ::std::error_code& ec);

  /**
   * Equivalent to
   * #RunRead(uint16_t idx, uint8_t subidx, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T>
  T
  RunRead(uint16_t idx, uint8_t subidx) {
    return Wait(AsyncRead<T>(idx, subidx));
  }

  /**
   * Equivalent to
   * #RunRead(uint16_t idx, uint8_t subidx, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T>
  T
  RunRead(uint16_t idx, uint8_t subidx, ::std::error_code& ec) {
    return Wait(AsyncRead<T>(idx, subidx), ec);
  }

  /**
   * Equivalent to
   * #RunRead(uint16_t idx, uint8_t subidx, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T>
  T
  RunRead(uint16_t idx, uint8_t subidx,
          const ::std::chrono::milliseconds& timeout) {
    return Wait(AsyncRead<T>(idx, subidx, timeout));
  }

  /**
   * Queues an asynchronous read (SDO upload) operation and runs the event loop
   * until the operation is complete.
   *
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   * @param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   *
   * @returns the received value.
   */
  template <class T>
  T
  RunRead(uint16_t idx, uint8_t subidx,
          const ::std::chrono::milliseconds& timeout, ::std::error_code& ec) {
    return Wait(AsyncRead<T>(idx, subidx, timeout), ec);
  }

  /**
   * Equivalent to
   * #RunWrite(uint16_t idx, uint8_t subidx, T&& value, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T>
  void
  RunWrite(uint16_t idx, uint8_t subidx, T&& value) {
    Wait(AsyncWrite(idx, subidx, ::std::forward<T>(value)));
  }

  /**
   * Equivalent to
   * #RunWrite(uint16_t idx, uint8_t subidx, T&& value, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it uses the SDO timeout given by
   * #lely::canopen::BasicMaster::GetTimeout().
   */
  template <class T>
  void
  RunWrite(uint16_t idx, uint8_t subidx, T&& value, ::std::error_code& ec) {
    Wait(AsyncWrite(idx, subidx, ::std::forward<T>(value)), ec);
  }

  /**
   * Equivalent to
   * #RunWrite(uint16_t idx, uint8_t subidx, T&& value, const ::std::chrono::milliseconds& timeout, ::std::error_code& ec),
   * except that it throws #lely::canopen::SdoError on error.
   */
  template <class T>
  void
  RunWrite(uint16_t idx, uint8_t subidx, T&& value,
           const ::std::chrono::milliseconds& timeout) {
    Wait(AsyncWrite(idx, subidx, ::std::forward<T>(value), timeout));
  }

  /**
   * Queues an asynchronous write (SDO download) operation and runs the event
   * loop until the operation is complete.
   *
   * @param idx     the object index.
   * @param subidx  the object sub-index.
   * @param value   the value to be written.
   * @param timeout the SDO timeout. If, after the request is initiated, the
   *                timeout expires before receiving a response from the server,
   *                the client aborts the transfer with abort code
   *                #SdoErrc::TIMEOUT.
   * @param ec      the error code (0 on success). `ec == SdoErrc::NO_SDO` if no
   *                client-SDO is available.
   */
  template <class T>
  void
  RunWrite(uint16_t idx, uint8_t subidx, T&& value,
           const ::std::chrono::milliseconds& timeout, ::std::error_code& ec) {
    Wait(AsyncWrite(idx, subidx, ::std::forward<T>(value), timeout), ec);
  }

 private:
  struct Impl_;
  ::std::unique_ptr<Impl_> impl_;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_LOOP_DRIVER_HPP_
