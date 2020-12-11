/**@file
 * This header file is part of the I/O library; it contains the C++ interface
 * for the timer queue.
 *
 * @see lely/io2/tqueue.h
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

#ifndef LELY_IO2_TQUEUE_HPP_
#define LELY_IO2_TQUEUE_HPP_

#include <lely/io2/timer.hpp>
#include <lely/io2/tqueue.h>

#include <utility>

namespace lely {
namespace io {

namespace detail {

template <class F>
class TimerQueueWaitWrapper : public io_tqueue_wait {
 public:
  TimerQueueWaitWrapper(ev_exec_t* exec, F&& f)
      : io_tqueue_wait IO_TQUEUE_WAIT_INIT(
            0, 0, exec,
            [](ev_task* task) noexcept {
              auto wait = io_tqueue_wait_from_task(task);
              ::std::error_code ec;
              if (wait->errc) ec = util::make_error_code(wait->errc);
              auto self = static_cast<TimerQueueWaitWrapper*>(wait);
              compat::invoke(::std::move(self->func_), ec);
              delete self;
            }),
        func_(::std::forward<F>(f)) {}

  TimerQueueWaitWrapper(const TimerQueueWaitWrapper&) = delete;

  TimerQueueWaitWrapper& operator=(const TimerQueueWaitWrapper&) = delete;

 private:
  typename ::std::decay<F>::type func_;
};

}  // namespace detail

/**
 * Creates a wait operation with a completion task. The operation deletes
 * itself after it is completed, so it MUST NOT be deleted once it is submitted
 * to an timer.
 */
template <class F>
// clang-format off
inline typename ::std::enable_if<
    compat::is_invocable<F, ::std::error_code>::value,
    detail::TimerQueueWaitWrapper<F>*>::type
make_timer_queue_wait_wrapper(ev_exec_t* exec, F&& f) {
  // clang-format on
  return new detail::TimerQueueWaitWrapper<F>(exec, ::std::forward<F>(f));
}

/**
 * A wait operation suitable for use with a timerqueue. This class stores a
 * callable object with signature `void(std::error_code ec)`, which is invoked
 * upon completion (or cancellation) of the wait operation.
 */
class TimerQueueWait : public io_tqueue_wait {
 public:
  using Signature = void(::std::error_code);

  /// Constructs a wait operation with a completion task.
  template <class F>
  TimerQueueWait(ev_exec_t* exec, F&& f)
      : io_tqueue_wait
        IO_TQUEUE_WAIT_INIT(0, 0, exec,
                            [](ev_task* task) noexcept {
                              auto wait = io_tqueue_wait_from_task(task);
                              auto self = static_cast<TimerQueueWait*>(wait);
                              if (self->func_) {
                                ::std::error_code ec;
                                if (wait->errc)
                                  ec = util::make_error_code(wait->errc);
                                self->func_(ec);
                              }
                            }),
        func_(::std::forward<F>(f)) {}

  /// Constructs a wait operation with a completion task.
  template <class F>
  explicit TimerQueueWait(F&& f)
      : TimerQueueWait(nullptr, ::std::forward<F>(f)) {}

  TimerQueueWait(const TimerQueueWait&) = delete;

  TimerQueueWait& operator=(const TimerQueueWait&) = delete;

  operator ev_task&() & noexcept { return task; }

  /// Returns the executor to which the completion task is (to be) submitted.
  ev::Executor
  get_executor() const noexcept {
    return ev::Executor(task.exec);
  }

 private:
  ::std::function<Signature> func_;
};

/// A timer queue. This class is a wrapper around `#io_tqueue_t*`.
class TimerQueue : public Device {
 public:
  using Device::operator io_dev_t*;

  /// @see io_tqueue_create()
  explicit TimerQueue(io_timer_t* timer, ev_exec_t* exec = nullptr)
      : Device(nullptr), tq_(io_tqueue_create(timer, exec)) {
    if (tq_)
      dev = io_tqueue_get_dev(*this);
    else
      util::throw_errc("TimerQueue");
  }

  TimerQueue(const TimerQueue&) = delete;

  TimerQueue(TimerQueue&& other) noexcept : Device(other.dev), tq_(other.tq_) {
    other.tq_ = nullptr;
    other.dev = nullptr;
  }

  TimerQueue& operator=(const TimerQueue&) = delete;

  TimerQueue&
  operator=(TimerQueue&& other) noexcept {
    using ::std::swap;
    swap(tq_, other.tq_);
    swap(dev, other.dev);
    return *this;
  }

  /// @see io_tqueue_destroy()
  ~TimerQueue() { io_tqueue_destroy(*this); }

  operator io_tqueue_t*() const noexcept { return tq_; }

  /// @see io_tqueue_get_timer()
  TimerBase
  get_timer() const noexcept {
    return TimerBase(io_tqueue_get_timer(*this));
  }

  /// @see io_tqueue_wait()
  void
  submit_wait(io_tqueue_wait& wait) noexcept {
    io_tqueue_submit_wait(*this, &wait);
  }

  /// @see io_tqueue_wait()
  void
  submit_wait(const TimerBase::time_point& t, io_tqueue_wait& wait) noexcept {
    wait.value = util::to_timespec(t);
    submit_wait(wait);
  }

  /// @see io_tqueue_wait()
  void
  submit_wait(const TimerBase::duration& d, io_tqueue_wait& wait) {
    auto now = get_timer().get_clock().gettime();
    wait.value = util::to_timespec(now + d);
    submit_wait(wait);
  }

  /// @see io_tqueue_wait()
  template <class F>
  void
  submit_wait(const TimerBase::time_point& t, ev_exec_t* exec, F&& f) {
    submit_wait(t, *make_timer_queue_wait_wrapper(exec, ::std::forward<F>(f)));
  }

  /// @see io_tqueue_wait()
  template <class F>
  void
  submit_wait(const TimerBase::duration& d, ev_exec_t* exec, F&& f) {
    submit_wait(d, *make_timer_queue_wait_wrapper(exec, ::std::forward<F>(f)));
  }

  /// @see io_tqueue_wait()
  template <class F>
  typename ::std::enable_if<!::std::is_base_of<
      io_tqueue_wait, typename ::std::decay<F>::type>::value>::type
  submit_wait(const TimerBase::time_point& t, F&& f) {
    submit_wait(t, nullptr, ::std::forward<F>(f));
  }

  /// @see io_tqueue_wait()
  template <class F>
  typename ::std::enable_if<!::std::is_base_of<
      io_tqueue_wait, typename ::std::decay<F>::type>::value>::type
  submit_wait(const TimerBase::duration& d, F&& f) {
    submit_wait(d, nullptr, ::std::forward<F>(f));
  }

  /// @see io_tqueue_cancel_wait()
  bool
  cancel_wait(struct io_tqueue_wait& wait) noexcept {
    return io_tqueue_cancel_wait(*this, &wait) != 0;
  }

  /// @see io_tqueue_abort_wait()
  bool
  abort_wait(struct io_tqueue_wait& wait) noexcept {
    return io_tqueue_abort_wait(*this, &wait) != 0;
  }

  /// @see io_tqueue_async_wait()
  ev::Future<void, int>
  async_wait(ev_exec_t* exec, const TimerBase::time_point& t,
             struct io_tqueue_wait** pwait = nullptr) {
    auto value = util::to_timespec(t);
    auto future = io_tqueue_async_wait(*this, exec, &value, pwait);
    if (!future) util::throw_errc("async_wait");
    return ev::Future<void, int>(future);
  }

  /// @see io_tqueue_async_wait()
  ev::Future<void, int>
  async_wait(const TimerBase::time_point& t,
             struct io_tqueue_wait** pwait = nullptr) {
    return async_wait(nullptr, t, pwait);
  }

  /// @see io_tqueue_async_wait()
  ev::Future<void, int>
  async_wait(ev_exec_t* exec, const TimerBase::duration& d,
             struct io_tqueue_wait** pwait = nullptr) {
    auto now = get_timer().get_clock().gettime();
    return async_wait(exec, now + d, pwait);
  }

  /// @see io_tqueue_async_wait()
  ev::Future<void, int>
  async_wait(const TimerBase::duration& d,
             struct io_tqueue_wait** pwait = nullptr) {
    return async_wait(nullptr, d, pwait);
  }

 private:
  io_tqueue_t* tq_{nullptr};
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_TQUEUE_HPP_
