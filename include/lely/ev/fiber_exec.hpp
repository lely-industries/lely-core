/**@file
 * This header file is part of the event library; it contains the C++ interface
 * for the fiber executor, mutex and condition variable.
 *
 * @see lely/ev/fiber_exec.h
 *
 * @copyright 2019-2022 Lely Industries N.V.
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

#ifndef LELY_EV_FIBER_EXEC_HPP_
#define LELY_EV_FIBER_EXEC_HPP_

#include <lely/ev/exec.hpp>
#include <lely/ev/fiber_exec.h>
#include <lely/ev/future.hpp>
#include <lely/util/fiber.hpp>

#include <mutex>
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
   *                   (#LELY_EV_FIBER_MAX_UNUSED) is used.
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
   *                   (#LELY_EV_FIBER_MAX_UNUSED) is used.
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

namespace detail {

inline void
throw_fiber_error(const char* what_arg, int ev) {
  switch (ev) {
    case ev_fiber_success:
      break;
    case ev_fiber_error:
      util::throw_errc(what_arg);
    case ev_fiber_timedout:
      throw ::std::system_error(::std::make_error_code(::std::errc::timed_out),
                                what_arg);
    case ev_fiber_busy:
      throw ::std::system_error(
          ::std::make_error_code(::std::errc::resource_unavailable_try_again),
          what_arg);
    case ev_fiber_nomem:
      throw ::std::system_error(
          ::std::make_error_code(::std::errc::not_enough_memory), what_arg);
  }
}

/// The base class for mutexes suitable for use in fibers.
class FiberMutexBase {
 public:
  FiberMutexBase() = default;

  FiberMutexBase(const FiberMutexBase&) = delete;
  FiberMutexBase(FiberMutexBase&& other) = delete;

  FiberMutexBase& operator=(const FiberMutexBase&) = delete;
  FiberMutexBase& operator=(FiberMutexBase&& other) = delete;

  ~FiberMutexBase() { ev_fiber_mtx_destroy(*this); }

  operator ev_fiber_mtx_t*() noexcept { return &mtx_; }

  /// @see ev_fiber_mtx_lock()
  void
  lock() {
    int ev = ev_fiber_mtx_lock(*this);
    if (ev != ev_fiber_success) detail::throw_fiber_error("lock", ev);
  }

  /// @see ev_fiber_mtx_trylock()
  bool
  try_lock() {
    int ev = ev_fiber_mtx_trylock(*this);
    switch (ev) {
      case ev_fiber_success:
        return true;
      case ev_fiber_busy:
        return false;
      default:
        detail::throw_fiber_error("try_lock", ev);
        return false;
    }
  }

  /// @see ev_fiber_mtx_unlock()
  void
  unlock() {
    int ev = ev_fiber_mtx_unlock(*this);
    if (ev != ev_fiber_success) detail::throw_fiber_error("unlock", ev);
  }

 private:
  ev_fiber_mtx_t mtx_{nullptr};
};

}  // namespace detail

/// A plain mutex suitable for use in fibers.
class FiberMutex : public detail::FiberMutexBase {
 public:
  FiberMutex() {
    int ev = ev_fiber_mtx_init(*this, ev_fiber_mtx_plain);
    if (ev != ev_fiber_success) detail::throw_fiber_error("FiberMutex", ev);
  }
};

/// A recursive mutex suitable for use in fibers.
class FiberRecursiveMutex : public detail::FiberMutexBase {
 public:
  FiberRecursiveMutex() {
    int ev = ev_fiber_mtx_init(*this, ev_fiber_mtx_recursive);
    if (ev != ev_fiber_success)
      detail::throw_fiber_error("FiberRecursiveMutex", ev);
  }
};

/// A condition variable suitable for use in fibers.
class FiberConditionVariable {
 public:
  FiberConditionVariable() {
    if (ev_fiber_cnd_init(*this) != ev_fiber_success)
      ::lely::util::throw_errc("FiberConditionVariable");
  }

  FiberConditionVariable(const FiberConditionVariable&) = delete;
  FiberConditionVariable(FiberConditionVariable&& other) = delete;

  FiberConditionVariable& operator=(const FiberConditionVariable&) = delete;
  FiberConditionVariable& operator=(FiberConditionVariable&& other) = delete;

  ~FiberConditionVariable() { ev_fiber_cnd_destroy(*this); }

  operator ev_fiber_cnd_t*() noexcept { return &cond_; }

  /// @see ev_fiber_cnd_signal()
  void
  notify_one() noexcept {
    ev_fiber_cnd_signal(*this);
  }

  /// @see ev_fiber_cnd_broadcast()
  void
  notify_all() noexcept {
    ev_fiber_cnd_broadcast(*this);
  }

  /// @see ev_fiber_cnd_wait()
  void
  wait(std::unique_lock<FiberMutex>& lock) {
    int ev = ev_fiber_cnd_wait(*this, *lock.mutex());
    if (ev != ev_fiber_success) detail::throw_fiber_error("wait", ev);
  }

  /// @see ev_fiber_cnd_wait()
  template <class Predicate>
  void
  wait(std::unique_lock<FiberMutex>& lock, Predicate pred) {
    while (!pred()) wait(lock);
  }

 private:
  ev_fiber_cnd_t cond_{nullptr};
};

}  // namespace ev
}  // namespace lely

#endif  // !LELY_EV_FIBER_EXEC_HPP_
