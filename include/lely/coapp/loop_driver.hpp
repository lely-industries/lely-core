/**@file
 * This header file is part of the C++ CANopen application library; it contains
 * the declarations for the remote node driver containing an event loop.
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
class LoopDriver : private detail::LoopDriverBase, public BasicDriver {
 public:
  LoopDriver(BasicMaster& master, uint8_t id);
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
   * Returns a future which becomes ready once the dedicated event loop of the
   * driver is stopped and the thread is (about to be) terminated.
   */
  ev::Future<void, void> AsyncStoppped() noexcept;

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

 protected:
  template <class T>
  T
  Wait(ev::Future<T, ::std::exception_ptr> f) {
    GetLoop().wait(f);
    if (!f.is_ready())
      throw ::std::system_error(
          ::std::make_error_code(::std::errc::operation_canceled), "Wait");
    return f.get().value();
  }

  template <class T>
  typename ::std::enable_if<!::std::is_void<T>::value, T>::type
  Wait(ev::Future<T, ::std::exception_ptr> f, ::std::error_code& ec) {
    GetLoop().wait(f, ec);
    if (!f.is_ready()) {
      ec = ::std::make_error_code(::std::errc::operation_canceled);
      return T{};
    }
    auto& result = f.get();
    if (result.has_value()) {
      ec.clear();
      return result.value();
    } else {
      try {
        ::std::rethrow_exception(result.error());
      } catch (const ::std::system_error& e) {
        ec = e.code();
      }
      return T{};
    }
  }

  void Wait(ev::Future<void, ::std::exception_ptr> f, ::std::error_code& ec);

 private:
  struct Impl_;
  ::std::unique_ptr<Impl_> impl_;
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_LOOP_DRIVER_HPP_
