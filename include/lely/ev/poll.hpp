/**@file
 * This header file is part of the event library; it contains the C++ interface
 * for the abstract polling interface.
 *
 * @see lely/ev/poll.h
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

#ifndef LELY_EV_POLL_HPP_
#define LELY_EV_POLL_HPP_

#include <lely/ev/poll.h>
#include <lely/util/error.hpp>

namespace lely {
namespace ev {

/**
 * The abstract polling interface. This class is a wrapper around `#ev_poll_t*`.
 */
class Poll {
 public:
  explicit Poll(ev_poll_t* poll) noexcept : poll_(poll) {}

  operator ev_poll_t*() const noexcept { return poll_; }

  /// @see ev_poll_self()
  void*
  self() const noexcept {
    return ev_poll_self(*this);
  }

  /// @see ev_poll_wait()
  void
  wait(int timeout) {
    if (ev_poll_wait(*this, timeout) == -1) util::throw_errc("wait");
  }

  /// @see ev_poll_kill()
  void
  kill(void* thr) {
    if (ev_poll_kill(*this, thr) == -1) util::throw_errc("kill");
  }

 protected:
  ev_poll_t* poll_{nullptr};
};

}  // namespace ev
}  // namespace lely

#endif  // !LELY_EV_POLL_HPP_
