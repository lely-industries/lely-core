/**@file
 * This header file is part of the C++ CANopen application library; it contains
 * the declarations for the remote node driver which runs its tasks and
 * callbacks in fibers.
 *
 * @copyright 2019-2020 Lely Industries N.V.
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

#ifndef LELY_COAPP_FIBER_DRIVER_HPP_
#define LELY_COAPP_FIBER_DRIVER_HPP_

#include <lely/coapp/driver.hpp>
#include <lely/ev/fiber_exec.hpp>
#include <lely/ev/strand.hpp>

#include <utility>

namespace lely {

namespace canopen {

namespace detail {

/// A base class for #lely::canopen::FiberDriver, containing a fiber executor.
class FiberDriverBase {
 protected:
  FiberDriverBase(ev_exec_t* exec);

  ev::FiberThread thrd;
  ev::FiberExecutor exec;
  ev::Strand strand;
};

}  // namespace detail

/**
 * A CANopen driver running its tasks and callbacks in fibers. The driver MUST
 * be instantiated on the thread on which its task are run.
 */
class FiberDriver : detail::FiberDriverBase, public BasicDriver {
 public:
  /**
   * Creates a new CANopen driver and its associated fiber executor.
   *
   * @param exec   the inner executor used to create the fiber executor. If
   *               <b>exec</b> is a null pointer, the CANopen master executor is
   *               used.
   * @param master a reference to a CANopen master.
   * @param id     the node-ID of the remote node (in the range [1..127]).
   *
   * @throws std::out_of_range if the node-ID is invalid or already registered.
   */
  explicit FiberDriver(ev_exec_t* exec, AsyncMaster& master, uint8_t id);

  /// Creates a new CANopen driver and its associated fiber executor.
  explicit FiberDriver(AsyncMaster& master, uint8_t id)
      : FiberDriver(nullptr, master, id) {}

  /// Returns the strand executor associated with the driver.
  ev::Executor
  GetStrand() const noexcept {
    return strand;
  }

  /**
   * Schedules the specified Callable object for execution by strand for this
   * driver.
   *
   * @see GetStrand().
   */
  template <class F, class... Args>
  void
  Defer(F&& f, Args&&... args) {
    GetStrand().post(::std::forward<F>(f), ::std::forward<Args>(args)...);
  }

  /**
   * Waits for the specified future to become ready by suspending the calling
   * fiber.
   *
   * This function MUST only be called from tasks submitted to the executor
   * associated with this driver.
   *
   * @returns the value stored in the future on success.
   *
   * @throws the exception stored in the future on failure.
   *
   * @see DriverBase::GetExecutor()
   */
  template <class T>
  T
  Wait(SdoFuture<T> f) {
    fiber_await(f);
    try {
      return f.get().value();
    } catch (const ev::future_not_ready& e) {
      util::throw_error_code("Wait", ::std::errc::operation_canceled);
    }
  }

  /**
   * Waits for the specified future to become ready by suspending the calling
   * fiber. The error code (0 on success) is stored in <b>ec</b>.
   *
   * This function MUST only be called from tasks submitted to the executor
   * associated with this driver.
   *
   * @returns the value stored in the future on success, or an empty value on
   * error.
   *
   * @see DriverBase::GetExecutor()
   */
  template <class T>
  typename ::std::enable_if<!::std::is_void<T>::value, T>::type
  Wait(SdoFuture<T> f, ::std::error_code& ec) {
    fiber_await(f);
    try {
      return f.get().value();
    } catch (const ::std::system_error& e) {
      ec = e.code();
    } catch (const ev::future_not_ready& e) {
      ec = ::std::make_error_code(::std::errc::operation_canceled);
    }
  }

  /**
   * Waits for the specified future to become ready by suspending the calling
   * fiber. The error code (0 on success) is stored in <b>ec</b>.
   *
   * This function MUST only be called from tasks submitted to the executor
   * associated with this driver.
   *
   * @see DriverBase::GetExecutor()
   */
  void Wait(SdoFuture<void> f, ::std::error_code& ec);

  /**
   * Suspends the calling fiber for <b>usec</b> microseconds. This function is
   * equivalent to `Wait(AsyncWait(::std::chrono::microseconds(usec)))`.
   */
  void USleep(uint_least64_t usec);

  /**
   * Suspends the calling fiber for <b>usec</b> microseconds. This function is
   * equivalent to `Wait(AsyncWait(::std::chrono::microseconds(usec)), ec)`.
   */
  void USleep(uint_least64_t usec, ::std::error_code& ec);
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_FIBER_DRIVER_HPP_
