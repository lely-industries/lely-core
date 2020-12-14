/**@file
 * This header file is part of the I/O library; it contains the C++ interface
 * for the abstract signal handler.
 *
 * @see lely/io2/sigset.h
 *
 * @copyright 2018-2020 Lely Industries N.V.
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

#ifndef LELY_IO2_SIGSET_HPP_
#define LELY_IO2_SIGSET_HPP_

#include <lely/ev/future.hpp>
#include <lely/io2/dev.hpp>
#include <lely/io2/sigset.h>

#include <utility>

namespace lely {
namespace io {

namespace detail {

template <class F>
class SignalSetWaitWrapper : public io_sigset_wait {
 public:
  SignalSetWaitWrapper(ev_exec_t* exec, F&& f)
      : io_sigset_wait IO_SIGSET_WAIT_INIT(
            exec,
            [](ev_task* task) noexcept {
              auto wait = io_sigset_wait_from_task(task);
              auto signo = wait->signo;
              auto self = static_cast<SignalSetWaitWrapper*>(wait);
              compat::invoke(::std::move(self->func_), signo);
              delete self;
            }),
        func_(::std::forward<F>(f)) {}

  SignalSetWaitWrapper(const SignalSetWaitWrapper&) = delete;

  SignalSetWaitWrapper& operator=(const SignalSetWaitWrapper&) = delete;

 private:
  typename ::std::decay<F>::type func_;
};

}  // namespace detail

/**
 * Creates a wait operation with a completion task. The operation deletes
 * itself after it is completed, so it MUST NOT be deleted once it is submitted
 * to a signal handler.
 */
template <class F>
inline typename ::std::enable_if<compat::is_invocable<F, int>::value,
                                 detail::SignalSetWaitWrapper<F>*>::type
make_signal_set_wait_wrapper(ev_exec_t* exec, F&& f) {
  return new detail::SignalSetWaitWrapper<F>(exec, ::std::forward<F>(f));
}

/**
 * A wait operation suitable for use with a signal handler. This class stores a
 * callable object with signature `void(int signo)`, which is invoked upon
 * completion (or cancellation) of the wait operation.
 */
class SignalSetWait : public io_sigset_wait {
 public:
  using Signature = void(int);

  /// Constructs a wait operation with a completion task.
  template <class F>
  SignalSetWait(ev_exec_t* exec, F&& f)
      : io_sigset_wait
        IO_SIGSET_WAIT_INIT(exec,
                            [](ev_task* task) noexcept {
                              auto wait = io_sigset_wait_from_task(task);
                              auto self = static_cast<SignalSetWait*>(wait);
                              if (self->func_) {
                                auto signo = wait->signo;
                                self->func_(signo);
                              }
                            }),
        func_(::std::forward<F>(f)) {}

  /// Constructs a wait operation with a completion task.
  template <class F>
  explicit SignalSetWait(F&& f)
      : SignalSetWait(nullptr, ::std::forward<F>(f)) {}

  SignalSetWait(const SignalSetWait&) = delete;

  SignalSetWait& operator=(const SignalSetWait&) = delete;

  operator ev_task&() & noexcept { return task; }

  /// Returns the executor to which the completion task is (to be) submitted.
  ev::Executor
  get_executor() const noexcept {
    return ev::Executor(task.exec);
  }

 private:
  ::std::function<Signature> func_;
};

/**
 * A reference to an abstract signal handler. This class is a wrapper around
 * `#io_sigset_t*`.
 */
class SignalSetBase : public Device {
 public:
  using Device::operator io_dev_t*;

  explicit SignalSetBase(io_sigset_t* sigset_) noexcept
      : Device(sigset_ ? io_sigset_get_dev(sigset_) : nullptr),
        sigset(sigset_) {}

  operator io_sigset_t*() const noexcept { return sigset; }

  /// @see io_sigset_clear()
  void
  clear(::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    if (!io_sigset_clear(*this))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_sigset_clear()
  void
  clear() {
    ::std::error_code ec;
    clear(ec);
    if (ec) throw ::std::system_error(ec, "clear");
  }

  /// @see io_sigset_insert()
  void
  insert(int signo, ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    if (!io_sigset_insert(*this, signo))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_sigset_insert()
  void
  insert(int signo) {
    ::std::error_code ec;
    insert(signo, ec);
    if (ec) throw ::std::system_error(ec, "insert");
  }

  /// @see io_sigset_remove()
  void
  remove(int signo, ::std::error_code& ec) noexcept {
    int errsv = get_errc();
    set_errc(0);
    if (!io_sigset_remove(*this, signo))
      ec.clear();
    else
      ec = util::make_error_code();
    set_errc(errsv);
  }

  /// @see io_sigset_remove()
  void
  remove(int signo) {
    ::std::error_code ec;
    remove(signo, ec);
    if (ec) throw ::std::system_error(ec, "remove");
  }

  /// @see io_sigset_submit_wait()
  void
  submit_wait(io_sigset_wait& wait) noexcept {
    io_sigset_submit_wait(*this, &wait);
  }

  /// @see io_sigset_submit_wait()
  template <class F>
  void
  submit_wait(ev_exec_t* exec, F&& f) {
    submit_wait(*make_signal_set_wait_wrapper(exec, ::std::forward<F>(f)));
  }

  /// @see io_sigset_submit_wait()
  template <class F>
  typename ::std::enable_if<!::std::is_base_of<
      io_sigset_wait, typename ::std::decay<F>::type>::value>::type
  submit_wait(F&& f) {
    submit_wait(nullptr, ::std::forward<F>(f));
  }

  /// @see io_sigset_async_wait()
  ev::Future<int, void>
  async_wait(ev_exec_t* exec, struct io_sigset_wait** pwait = nullptr) {
    auto future = io_sigset_async_wait(*this, exec, pwait);
    if (!future) util::throw_errc("async_wait");
    return ev::Future<int, void>(future);
  }

  /// @see io_sigset_async_wait()
  ev::Future<int, void>
  async_wait(struct io_sigset_wait** pwait = nullptr) {
    return async_wait(nullptr, pwait);
  }

 protected:
  io_sigset_t* sigset{nullptr};
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_SIGSET_HPP_
