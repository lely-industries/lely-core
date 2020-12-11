/**@file
 * This header file is part of the I/O library; it contains the base class for
 * CAN frame router reead operations with a stackless coroutine as the
 * completion task.
 *
 * @see lely/util/coroutine.hpp, lely/io2/can_rt.hpp
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

#ifndef LELY_IO2_CO_CAN_RT_HPP_
#define LELY_IO2_CO_CAN_RT_HPP_

#include <lely/io2/can_rt.hpp>
#include <lely/util/coroutine.hpp>

namespace lely {
namespace io {

/**
 * A CAN frame read operation suitable for use with a CAN frame router, with a
 * stackless coroutine as the completion task.
 */
class CoCanRouterReadFrame : public io_can_rt_read_msg, public util::Coroutine {
 public:
  /// Constructs a CAN frame read operation.
  explicit CoCanRouterReadFrame(uint_least32_t id,
                                CanFlag flags = CanFlag::NONE) noexcept
      : io_can_rt_read_msg IO_CAN_RT_READ_MSG_INIT(
            id, static_cast<uint_least8_t>(flags), [](ev_task* task) noexcept {
              auto read_msg = io_can_rt_read_msg_from_task(task);
              auto msg = read_msg->r.msg;
              ::std::error_code ec;
              if (!msg) ec = util::make_error_code(read_msg->r.errc);
              auto self = static_cast<CoCanRouterReadFrame*>(read_msg);
              (*self)(msg, ec);
            }) {}

  virtual ~CoCanRouterReadFrame() = default;

  operator ev_task&() & noexcept { return task; }

  /// Returns the executor to which the completion task is (to be) submitted.
  ev::Executor
  get_executor() const noexcept {
    return ev::Executor(task.exec);
  }

  /**
   * The coroutine to be executed once the read operation completes (or is
   * canceled).
   *
   * @param msg a pointer to the received CAN frame, or 0 on error (or if the
   *            operation is canceled). In the latter case, the error number is
   *            stored in <b>ec</b>.
   * @param ec  the error code if <b>msg</b> is 0.
   */
  virtual void operator()(const can_msg* msg,
                          ::std::error_code ec) noexcept = 0;
};

/**
 * A CAN error frame read operation suitable for use with a CAN frame router,
 * with a stackless coroutine as the completion task.
 */
class CoCanRouterReadError : public io_can_rt_read_err, public util::Coroutine {
 public:
  /// Constructs a CAN error frame read operation.
  explicit CoCanRouterReadError() noexcept
      : io_can_rt_read_err IO_CAN_RT_READ_ERR_INIT([](ev_task* task) noexcept {
          auto read_err = io_can_rt_read_err_from_task(task);
          auto err = read_err->r.err;
          ::std::error_code ec;
          if (!err) ec = util::make_error_code(read_err->r.errc);
          auto self = static_cast<CoCanRouterReadError*>(read_err);
          (*self)(err, ec);
        }) {}

  virtual ~CoCanRouterReadError() = default;

  operator ev_task&() & noexcept { return task; }

  /// Returns the executor to which the completion task is (to be) submitted.
  ev::Executor
  get_executor() const noexcept {
    return ev::Executor(task.exec);
  }

  /**
   * The coroutine to be executed once the read operation completes (or is
   * canceled).
   *
   * @param err a pointer to the received CAN error frame, or 0 on error (or if
   *            the operation is canceled). In the latter case, the error number
   *            is stored in <b>ec</b>.
   * @param ec  the error code if <b>err</b> is 0.
   */
  virtual void operator()(const can_err* err,
                          ::std::error_code ec) noexcept = 0;
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_CO_CAN_RT_HPP_
