/**@file
 * This header file is part of the C++ CANopen application library; it contains
 * the declarations for the remote node driver which runs its tasks and
 * callbacks in fibers.
 *
 * @copyright 2019 Lely Industries N.V.
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
  FiberDriver(ev_exec_t* exec, BasicMaster& master, uint8_t id);

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

 protected:
  template <class T>
  T
  Wait(SdoFuture<T> f) {
    fiber_await(f);
    if (!f.is_ready())
      throw ::std::system_error(
          ::std::make_error_code(::std::errc::operation_canceled), "Wait");
    return f.get().value();
  }

  template <class T>
  typename ::std::enable_if<!::std::is_void<T>::value, T>::type
  Wait(SdoFuture<T> f, ::std::error_code& ec) {
    fiber_await(f);
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

  void Wait(SdoFuture<void> f, ::std::error_code& ec);
};

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_FIBER_DRIVER_HPP_
