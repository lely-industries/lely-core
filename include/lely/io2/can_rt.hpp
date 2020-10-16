/**@file
 * This header file is part of the I/O library; it contains the C++ interface
 * for the CAN frame router.
 *
 * @see lely/io2/can_rt.h
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

#ifndef LELY_IO2_CAN_RT_HPP_
#define LELY_IO2_CAN_RT_HPP_

#include <lely/io2/can.hpp>
#include <lely/io2/can_rt.h>

#include <utility>

namespace lely {
namespace io {

namespace detail {

template <class F>
class CanRouterReadFrameWrapper : public io_can_rt_read_msg {
 public:
  CanRouterReadFrameWrapper(uint_least32_t id, CanFlag flags, F&& f)
      : io_can_rt_read_msg IO_CAN_RT_READ_MSG_INIT(
            id, static_cast<uint_least8_t>(flags),
            [](ev_task* task) noexcept {
              auto read = io_can_rt_read_msg_from_task(task);
              auto msg = read->r.msg;
              ::std::error_code ec;
              if (!msg) ec = util::make_error_code(read->r.errc);
              auto self = static_cast<CanRouterReadFrameWrapper*>(read);
              compat::invoke(::std::move(self->func_), msg, ec);
              delete self;
            }),
        func_(::std::forward<F>(f)) {}

  CanRouterReadFrameWrapper(const CanRouterReadFrameWrapper&) = delete;

  CanRouterReadFrameWrapper& operator=(const CanRouterReadFrameWrapper&) =
      delete;

  operator ev_task&() & noexcept { return task; }

 private:
  typename ::std::decay<F>::type func_;
};

}  // namespace detail

/**
 * Creates a CAN frame read operation with a completion task. The operation
 * deletes itself after it is completed, so it MUST NOT be deleted once it is
 * submitted to a CAN frame router.
 */
template <class F>
inline typename ::std::enable_if<
    compat::is_invocable<F, const can_msg*, ::std::error_code>::value,
    detail::CanRouterReadFrameWrapper<F>*>::type
make_can_router_read_frame_wrapper(uint_least32_t id, CanFlag flags, F&& f) {
  return new detail::CanRouterReadFrameWrapper<F>(id, flags,
                                                  ::std::forward<F>(f));
}

/**
 * A CAN frame read operation suitable for use with a CAN frame router. This
 * class stores a callable object with signature
 * `void(const can_msg* msg, std::error_code ec)`, which is invoked upon
 * completion (or cancellation) of the read operation.
 */
class CanRouterReadFrame : public io_can_rt_read_msg {
 public:
  using Signature = void(const can_msg*, ::std::error_code);

  /// Constructs a CAN frame read operation with a completion task.
  template <class F>
  CanRouterReadFrame(uint_least32_t id, CanFlag flags, F&& f)
      : io_can_rt_read_msg IO_CAN_RT_READ_MSG_INIT(
            id, static_cast<uint_least8_t>(flags),
            [](ev_task* task) noexcept {
              auto read = io_can_rt_read_msg_from_task(task);
              auto self = static_cast<CanRouterReadFrame*>(read);
              if (self->func_) {
                auto msg = read->r.msg;
                ::std::error_code ec;
                if (!msg) ec = util::make_error_code(read->r.errc);
                self->func_(msg, ec);
              }
            }),
        func_(::std::forward<F>(f)) {}

  CanRouterReadFrame(const CanRouterReadFrame&) = delete;

  CanRouterReadFrame& operator=(const CanRouterReadFrame&) = delete;

  operator ev_task&() & noexcept { return task; }

  /// Returns the executor to which the completion task is (to be) submitted.
  ev::Executor
  get_executor() const noexcept {
    return ev::Executor(task.exec);
  }

 private:
  ::std::function<Signature> func_;
};

namespace detail {

template <class F>
class CanRouterReadErrorWrapper : public io_can_rt_read_err {
 public:
  CanRouterReadErrorWrapper(F&& f)
      : io_can_rt_read_err IO_CAN_RT_READ_ERR_INIT([](ev_task* task) noexcept {
          auto read = io_can_rt_read_err_from_task(task);
          auto err = read->r.err;
          ::std::error_code ec;
          if (!err) ec = util::make_error_code(read->r.errc);
          auto self = static_cast<CanRouterReadErrorWrapper*>(read);
          compat::invoke(::std::move(self->func_), err, ec);
          delete self;
        }),
        func_(::std::forward<F>(f)) {}

  CanRouterReadErrorWrapper(const CanRouterReadErrorWrapper&) = delete;

  CanRouterReadErrorWrapper& operator=(const CanRouterReadErrorWrapper&) =
      delete;

  operator ev_task&() & noexcept { return task; }

 private:
  typename ::std::decay<F>::type func_;
};

}  // namespace detail

/**
 * Creates a CAN error frame read operation with a completion task. The
 * operation deletes itself after it is completed, so it MUST NOT be deleted
 * once it is submitted to a CAN frame router.
 */
template <class F>
inline typename ::std::enable_if<
    compat::is_invocable<F, const can_err*, ::std::error_code>::value,
    detail::CanRouterReadErrorWrapper<F>*>::type
make_can_router_read_error_wrapper(F&& f) {
  return new detail::CanRouterReadErrorWrapper<F>(::std::forward<F>(f));
}

/**
 * A CAN error frame read operation suitable for use with a CAN frame router.
 * This class stores a callable object with signature
 * `void(const can_err* err, std::error_code ec)`, which is invoked upon
 * completion (or cancellation) of the read operation.
 */
class CanRouterReadError : public io_can_rt_read_err {
 public:
  using Signature = void(const can_err*, ::std::error_code);

  /// Constructs a CAN error frame read operation with a completion task.
  template <class F>
  CanRouterReadError(F&& f)
      : io_can_rt_read_err IO_CAN_RT_READ_ERR_INIT([](ev_task* task) noexcept {
          auto read = io_can_rt_read_err_from_task(task);
          auto self = static_cast<CanRouterReadError*>(read);
          if (self->func_) {
            auto err = read->r.err;
            ::std::error_code ec;
            if (!err) ec = util::make_error_code(read->r.errc);
            self->func_(err, ec);
          }
        }),
        func_(::std::forward<F>(f)) {}

  CanRouterReadError(const CanRouterReadError&) = delete;

  CanRouterReadError& operator=(const CanRouterReadError&) = delete;

  operator ev_task&() & noexcept { return task; }

  /// Returns the executor to which the completion task is (to be) submitted.
  ev::Executor
  get_executor() const noexcept {
    return ev::Executor(task.exec);
  }

 private:
  ::std::function<Signature> func_;
};

/// A CAN frame rounter. This class is a wrapper around `#io_can_rt_t*`.
class CanRouter : public Device {
 public:
  using Device::operator io_dev_t*;

  /// @see io_can_rt_create()
  CanRouter(io_can_chan_t* chan, ev_exec_t* exec)
      : Device(nullptr), rt_(io_can_rt_create(chan, exec)) {
    if (rt_)
      dev = io_can_rt_get_dev(*this);
    else
      util::throw_errc("CanRouter");
  }

  CanRouter(const CanRouter&) = delete;

  CanRouter(CanRouter&& other) noexcept : Device(other.dev), rt_(other.rt_) {
    other.rt_ = nullptr;
    other.dev = nullptr;
  }

  CanRouter& operator=(const CanRouter&) = delete;

  CanRouter&
  operator=(CanRouter&& other) noexcept {
    using ::std::swap;
    swap(rt_, other.rt_);
    swap(dev, other.dev);
    return *this;
  }

  /// @see io_can_rt_destroy()
  ~CanRouter() { io_can_rt_destroy(*this); }

  operator io_can_rt_t*() const noexcept { return rt_; }

  /// @see io_can_rt_get_chan()
  CanChannelBase
  get_channel() const noexcept {
    return CanChannelBase(io_can_rt_get_chan(*this));
  }

  /// @see io_can_rt_submit_read_msg()
  void
  submit_read_frame(struct io_can_rt_read_msg& read_msg) noexcept {
    io_can_rt_submit_read_msg(*this, &read_msg);
  }

  /// @see io_can_rt_submit_read_msg()
  template <class F>
  void
  submit_read_frame(uint_least32_t id, CanFlag flags, F&& f) {
    submit_read_frame(
        *make_can_router_read_frame_wrapper(id, flags, ::std::forward<F>(f)));
  }

  /// @see io_can_rt_cancel_read_msg()
  bool
  cancel_read_frame(struct io_can_rt_read_msg& read_msg) noexcept {
    return io_can_rt_cancel_read_msg(*this, &read_msg) != 0;
  }

  /// @see io_can_rt_abort_read_msg()
  bool
  abort_read_frame(struct io_can_rt_read_msg& read_msg) noexcept {
    return io_can_rt_abort_read_msg(*this, &read_msg) != 0;
  }

  /// @see io_can_rt_async_read_msg()
  ev::Future<const can_msg*, int>
  async_read_frame(uint_least32_t id, CanFlag flags,
                   struct io_can_rt_read_msg** pread_msg = nullptr) {
    auto future = io_can_rt_async_read_msg(
        *this, id, static_cast<uint_least8_t>(flags), pread_msg);
    if (!future) util::throw_errc("async_read_frame");
    return ev::Future<const can_msg*, int>(future);
  }

  /// @see io_can_rt_submit_read_err()
  void
  submit_read_error(struct io_can_rt_read_err& read_err) noexcept {
    io_can_rt_submit_read_err(*this, &read_err);
  }

  /// @see io_can_rt_submit_read_err()
  template <class F>
  typename ::std::enable_if<!::std::is_base_of<
      io_can_rt_read_err, typename ::std::decay<F>::type>::value>::type
  submit_read_error(F&& f) {
    submit_read_error(
        *make_can_router_read_error_wrapper(::std::forward<F>(f)));
  }

  /// @see io_can_rt_cancel_read_err()
  bool
  cancel_read_error(struct io_can_rt_read_err& read_err) noexcept {
    return io_can_rt_cancel_read_err(*this, &read_err) != 0;
  }

  /// @see io_can_rt_abort_read_err()
  bool
  abort_read_error(struct io_can_rt_read_err& read_err) noexcept {
    return io_can_rt_abort_read_err(*this, &read_err) != 0;
  }

  /// @see io_can_rt_async_read_err()
  ev::Future<const can_err*, int>
  async_read_error(struct io_can_rt_read_err** pread_err = nullptr) {
    auto future = io_can_rt_async_read_err(*this, pread_err);
    if (!future) util::throw_errc("async_read_error");
    return ev::Future<const can_err*, int>(future);
  }

  /// @see io_can_rt_async_shutdown()
  ev::Future<void, void>
  async_shutdown() {
    auto future = io_can_rt_async_shutdown(*this);
    if (!future) util::throw_errc("async_shutdown");
    return ev::Future<void, void>(future);
  }

 private:
  io_can_rt_t* rt_{nullptr};
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_CAN_RT_HPP_
