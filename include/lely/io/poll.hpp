/**@file
 * This header file is part of the I/O library; it contains the C++ interface of
 * the I/O polling interface. @see lely/io/poll.h for the C interface.
 *
 * @copyright 2016-2018 Lely Industries N.V.
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

#ifndef LELY_IO_POLL_HPP_
#define LELY_IO_POLL_HPP_

#ifndef __cplusplus
#error "include <lely/io/poll.h> for the C interface"
#endif

#include <lely/util/c_type.hpp>

namespace lely {
class IOPoll;
}
/// An opaque I/O polling interface type.
typedef lely::IOPoll io_poll_t;

#include <lely/io/poll.h>

namespace lely {

/// The attributes of #io_poll_t required by #lely::IOPoll.
template <>
struct c_type_traits<__io_poll> {
  typedef __io_poll value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __io_poll_alloc();
  }
  static void
  free(void* ptr) noexcept {
    __io_poll_free(ptr);
  }

  static pointer
  init(pointer p) noexcept {
    return __io_poll_init(p);
  }
  static void
  fini(pointer p) noexcept {
    __io_poll_fini(p);
  }
};

/// An opaque I/O polling interface type.
class IOPoll : public incomplete_c_type<__io_poll> {
  typedef incomplete_c_type<__io_poll> c_base;

 public:
  IOPoll() : c_base() {}

  int
  watch(io_handle_t handle, struct io_event* event,
        bool keep = false) noexcept {
    return io_poll_watch(this, handle, event, keep);
  }

  int
  wait(int maxevents, struct io_event* events, int timeout = 0) noexcept {
    return io_poll_wait(this, maxevents, events, timeout);
  }

  int
  signal(unsigned char sig) noexcept {
    return io_poll_signal(this, sig);
  }

 protected:
  ~IOPoll() {}
};

}  // namespace lely

#endif  // !LELY_IO_POLL_HPP_
