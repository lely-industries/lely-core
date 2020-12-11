/**@file
 * This header file is part of the I/O library; it contains the base class for
 * CAN channel read and write operations with a stackless coroutine as the
 * completion task.
 *
 * @see lely/util/coroutine.hpp, lely/io2/can.hpp
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

#ifndef LELY_IO2_CO_CAN_HPP_
#define LELY_IO2_CO_CAN_HPP_

#include <lely/io2/can.hpp>
#include <lely/util/coroutine.hpp>

namespace lely {
namespace io {

/**
 * A CAN channel read operation with a stackless coroutine as the completion
 * task.
 */
class CoCanChannelRead : public io_can_chan_read, public util::Coroutine {
 public:
  /// Constructs a read operation.
  explicit CoCanChannelRead(can_msg* msg, can_err* err = nullptr,
                            ::std::chrono::nanoseconds* dp = nullptr,
                            ev_exec_t* exec = nullptr) noexcept
      : io_can_chan_read IO_CAN_CHAN_READ_INIT(
            msg, err, dp ? &ts_ : nullptr, exec,
            [](ev_task* task) noexcept {
              auto read = io_can_chan_read_from_task(task);
              auto result = read->r.result;
              ::std::error_code ec;
              if (result == -1) ec = util::make_error_code(read->r.errc);
              auto self = static_cast<CoCanChannelRead*>(read);
              if (self->dp_) *self->dp_ = util::from_timespec(self->ts_);
              (*self)(result, ec);
            }),
        dp_(dp) {}

  virtual ~CoCanChannelRead() = default;

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
   * @param result 1 if a CAN frame is received, 0 if an error frame is
   *               received, or -1 on error (or if the operation is canceled).
   * @param ec     the error code, or 0 on success.
   */
  virtual void operator()(int result, ::std::error_code ec) noexcept = 0;

 private:
  timespec ts_{0, 0};
  ::std::chrono::nanoseconds* dp_{nullptr};
};

/**
 * A CAN channel write operation with a stackless coroutine as the completion
 * task.
 */
class CoCanChannelWrite : public io_can_chan_write, public util::Coroutine {
 public:
  /// Constructs a write operation.
  CoCanChannelWrite(const can_msg* msg, ev_exec_t* exec = nullptr) noexcept
      : io_can_chan_write
        IO_CAN_CHAN_WRITE_INIT(msg, exec, [](ev_task* task) noexcept {
          auto write = io_can_chan_write_from_task(task);
          ::std::error_code ec = util::make_error_code(write->errc);
          auto self = static_cast<CoCanChannelWrite*>(write);
          (*self)(ec);
        }) {}

  virtual ~CoCanChannelWrite() = default;

  operator ev_task&() & noexcept { return task; }

  /// Returns the executor to which the completion task is (to be) submitted.
  ev::Executor
  get_executor() const noexcept {
    return ev::Executor(task.exec);
  }

  /**
   * The coroutine to be executed once the write operation completes (or is
   * canceled).
   *
   * @param ec the error code, or 0 on success.
   */
  virtual void operator()(::std::error_code ec) noexcept = 0;
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_CO_CAN_HPP_
