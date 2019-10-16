/**@file
 * This header file is part of the event library; it contains the C++ interface
 * for the fiber executor.
 *
 * @see lely/ev/fiber_exec.h
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

#ifndef LELY_EV_CO_FIBER_HPP_
#define LELY_EV_CO_FIBER_HPP_

#include <lely/ev/exec.hpp>
#include <lely/ev/fiber_exec.h>
#include <lely/ev/future.hpp>
#include <lely/util/fiber.hpp>

#include <utility>

namespace lely {
namespace ev {

using FiberFlag = util::FiberFlag;

/**
 * Convenience class providing a RAII-style mechanism to ensure the calling
 * thread can be used by fiber executors for the duration of a scoped block.
 */
class FiberThread {
 public:
  /**
   * Initializes the calling thread for use by fiber executors, if it was not
   * already initialized.
   *
   * @param flags      the flags used to initialize the internal fiber of the
   *                   calling thread (see #lely::util::FiberThread) and any
   *                   subsequent fibers created by a fiber executor.
   *                   <b>flags</b> can be any supported combination of
   *                   FiberFlag::SAVE_MASK, FiberFlag::SAVE_FENV,
   *                   FiberFlag::SAVE_ERROR and FiberFlag::GUARD_STACK.
   * @param stack_size the size (in bytes) of the stack frame to be allocated
   *                   for the fibers. If 0, the default size
   *                   (#LELY_FIBER_STKSZ) is used. The size of the allocated
   *                   stack is always at least #LELY_FIBER_MINSTKSZ bytes.
   * @param max_unused the maximum number of unused fibers kept alive for future
   *                   tasks. If 0, the default number
   *                   (#LELY_EV_FIBER_MAX_UNUSED) used.
   */
  FiberThread(FiberFlag flags = static_cast<FiberFlag>(0),
              ::std::size_t stack_size = 0, ::std::size_t max_unused = 0) {
    if (ev_fiber_thrd_init(static_cast<int>(flags), stack_size, max_unused) ==
        -1)
      util::throw_errc("FiberThread");
  }

  /**
   * Initializes the calling thread for use by fiber executors, if it was not
   * already initialized.
   *
   * @param flags      the flags used to initialize the internal fiber of the
   *                   calling thread (see #lely::util::FiberThread) and any
   *                   subsequent fibers created by a fiber executor.
   *                   <b>flags</b> can be any supported combination of
   *                   FiberFlag::SAVE_MASK, FiberFlag::SAVE_FENV,
   *                   FiberFlag::SAVE_ERROR and FiberFlag::GUARD_STACK.
   * @param stack_size the size (in bytes) of the stack frame to be allocated
   *                   for the fibers. If 0, the default size
   *                   (#LELY_FIBER_STKSZ) is used. The size of the allocated
   *                   stack is always at least #LELY_FIBER_MINSTKSZ bytes.
   * @param max_unused the maximum number of unused coroutines kept alive for
   *                   future tasks. If 0, the default number
   *                   (#LELY_EV_CO_EXEC_MAX_UNUSED) used.
   * @param already    set to true if the calling thread was already
   *                   initialized, and to false if not. In the former case, the
   *                   values of <b>flags</b>, <b>stack_size</b> and
   *                   <b>unused</b> are ignored.
   */
  FiberThread(FiberFlag flags, ::std::size_t stack_size,
              ::std::size_t max_unused, bool& already) {
    int result =
        ev_fiber_thrd_init(static_cast<int>(flags), stack_size, max_unused);
    if (result == -1) util::throw_errc("FiberThread");
    already = result != 0;
  }

  FiberThread(const FiberThread&) = delete;
  FiberThread(FiberThread&&) = delete;

  FiberThread& operator=(const FiberThread&) = delete;
  FiberThread& operator=(FiberThread&&) = delete;

  /**
   * Finalizes the calling thread and prevents further use by fiber executors,
   * unless another instance of this class is still in scope.
   */
  ~FiberThread() { ev_fiber_thrd_fini(); }
};

/// A fiber executor.
class FiberExecutor : public Executor {
 public:
  /// @see ev_fiber_exec_create()
  explicit FiberExecutor(Executor inner_exec)
      : Executor(ev_fiber_exec_create(inner_exec)) {
    if (!exec_) ::lely::util::throw_errc("FiberExecutor");
  }

  FiberExecutor(const FiberExecutor&) = delete;

  FiberExecutor(FiberExecutor&& other) noexcept : Executor(other) {
    other.exec_ = nullptr;
  }

  FiberExecutor& operator=(const FiberExecutor&) = delete;

  FiberExecutor&
  operator=(FiberExecutor&& other) noexcept {
    using ::std::swap;
    swap(exec_, other.exec_);
    return *this;
  }

  /// @see ev_fiber_exec_destroy()
  ~FiberExecutor() { ev_fiber_exec_destroy(exec_); }

  /// @see ev_fiber_exec_get_inner_exec()
  Executor
  get_inner_executor() const noexcept {
    return Executor(ev_fiber_exec_get_inner_exec(*this));
  }
};

/// @see ev_fiber_await()
template <class T, class E>
inline void
fiber_await(Future<T, E> f) noexcept {
  ev_fiber_await(f);
}

/**
 * Yields a currently running fiber. This function MUST only be invoked from
 * tasks submitted to a fiber executor.
 *
 * @see ev_fiber_await()
 */
inline void
fiber_yield() noexcept {
  ev_fiber_await(nullptr);
}

}  // namespace ev
}  // namespace lely

#endif  // !LELY_EV_CO_FIBER_HPP_
