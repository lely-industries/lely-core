/**@file
 * This header file is part of the  I/O library; it contains the C++ interface
 * for the I/O events.
 *
 * @see lely/io2/watch.h
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

#ifndef LELY_IO2_EVENT_HPP_
#define LELY_IO2_EVENT_HPP_

#include <lely/io2/event.h>

#if _WIN32
#ifdef IN
#undef IN
#endif
#ifdef OUT
#undef OUT
#endif
#endif

namespace lely {
namespace io {

/**
 * The type of I/O event monitored by #lely::io::Poll::watch() and reported to
 * #io_poll_watch_func_t callback functions.
 */
enum class Event {
  /**
   * Data (other than priority data) MAY be read without blocking. For
   * connection-oriented socket, this event is also reported when the peer
   * closed the connection. For listening sockets, this event indicates that
   * there are pending connections waiting to be accepted.
   */
  IN = IO_EVENT_IN,
  /**
   * Priority data MAY be read without blocking. For sockets, this event
   * typically indicates the presence of out-of-band data.
   */
  PRI = IO_EVENT_PRI,
  /**
   * Data (bot normal and priority data) MAY be written without blocking. For
   * connection-oriented sockets, this event is also reported when a connection
   * attempt completes (with success or failure).
   */
  OUT = IO_EVENT_OUT,
  /// An error has occurred. This event is always reported.
  ERR = IO_EVENT_ERR,
  /**
   * The device has been disconnected. For connection-oriented sockets, this
   * event is reported when a connection is shut down (by `closesocket()` on
   * Windows or `shutdown(socket, SHUT_RDWR)` on POSIX plafforms) or when a
   * connection attempt fails. This event is always reported.
   */
  HUP = IO_EVENT_HUP,
  NONE = IO_EVENT_NONE,
  MASK = IO_EVENT_MASK
};

constexpr Event
operator~(Event rhs) {
  return static_cast<Event>(~static_cast<int>(rhs));
}

constexpr Event
operator&(Event lhs, Event rhs) {
  return static_cast<Event>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

constexpr Event
operator^(Event lhs, Event rhs) {
  return static_cast<Event>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
}

constexpr Event
operator|(Event lhs, Event rhs) {
  return static_cast<Event>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline Event&
operator&=(Event& lhs, Event rhs) {
  return lhs = lhs & rhs;
}

inline Event&
operator^=(Event& lhs, Event rhs) {
  return lhs = lhs ^ rhs;
}

inline Event&
operator|=(Event& lhs, Event rhs) {
  return lhs = lhs | rhs;
}

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_EVENT_HPP_
