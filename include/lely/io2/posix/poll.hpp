/**@file
 * This header file is part of the  I/O library; it contains the C++ interface
 * for the I/O polling interface for POSIX platforms.
 *
 * @see lely/io2/posix/poll.h
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

#ifndef LELY_IO2_POSIX_POLL_HPP_
#define LELY_IO2_POSIX_POLL_HPP_

#include <lely/ev/poll.hpp>
#include <lely/io2/ctx.hpp>
#include <lely/io2/event.hpp>
#include <lely/io2/posix/poll.h>
#include <lely/util/error.hpp>

#include <utility>

namespace lely {
namespace io {

/**
 * The system-dependent I/O polling interface. This class is a wrapper around
 * `#io_poll_t*`.
 */
class Poll {
 public:
  /// @see io_poll_create()
  Poll(ContextBase& ctx, int signo = 0) : poll_(io_poll_create(ctx, signo)) {
    if (!poll_) util::throw_errc("Poll");
  }

  Poll(const Poll&) = delete;

  Poll(Poll&& other) noexcept : poll_(other.poll_) { other.poll_ = nullptr; }

  Poll& operator=(const Poll&) = delete;

  Poll&
  operator=(Poll&& other) noexcept {
    using ::std::swap;
    swap(poll_, other.poll_);
    return *this;
  }

  /// @see io_poll_destroy()
  ~Poll() { io_poll_destroy(*this); }

  operator io_poll_t*() const noexcept { return poll_; }

  /// @see io_poll_get_ctx()
  ContextBase
  get_ctx() const noexcept {
    return ContextBase(io_poll_get_ctx(*this));
  }

  /// @see io_poll_get_poll()
  ev::Poll
  get_poll() const noexcept {
    return ev::Poll(io_poll_get_poll(*this));
  }

  /// @see io_poll_watch()
  void
  watch(int fd, Event events, struct io_poll_watch& watch,
        ::std::error_code& ec) noexcept {
    int errsv = errno;
    errno = 0;
    if (!io_poll_watch(*this, fd, static_cast<int>(events), &watch))
      ec.clear();
    else
      ec = util::make_error_code();
    errno = errsv;
  }

  /// @see io_poll_watch()
  void
  watch(int fd, Event events, struct io_poll_watch& watch) {
    ::std::error_code ec;
    this->watch(fd, events, watch, ec);
    if (ec) throw ::std::system_error(ec, "watch");
  }

 protected:
  io_poll_t* poll_{nullptr};
};

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_POSIX_POLL_HPP_
