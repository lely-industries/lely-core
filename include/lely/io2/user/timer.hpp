/**@file
 * This header file is part of the I/O library; it contains the C++ interface
 * for the user-defined timer.
 *
 * @see lely/io2/user/timer.h
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

#ifndef LELY_IO2_USER_TIMER_HPP_
#define LELY_IO2_USER_TIMER_HPP_

#include <lely/libc/type_traits.hpp>
#include <lely/io2/user/timer.h>
#include <lely/io2/timer.hpp>

#include <utility>

namespace lely {
namespace io {

/// A user-defined timer.
class UserTimer : public TimerBase {
 public:
  /// @see io_user_timer_create()
  UserTimer(io_ctx_t* ctx, ev_exec_t* exec,
            io_user_timer_setnext_t* func = nullptr, void* arg = nullptr)
      : TimerBase(io_user_timer_create(ctx, exec, func, arg)) {
    if (!timer) util::throw_errc("UserTimer");
  }

  UserTimer(const UserTimer&) = delete;

  UserTimer(UserTimer&& other) noexcept : TimerBase(other.timer) {
    other.timer = nullptr;
    other.dev = nullptr;
  }

  UserTimer& operator=(const UserTimer&) = delete;

  UserTimer&
  operator=(UserTimer&& other) noexcept {
    using ::std::swap;
    swap(timer, other.timer);
    swap(dev, other.dev);
    return *this;
  }

  /// @see io_user_timer_destroy()
  ~UserTimer() { io_user_timer_destroy(*this); }
};

template <class T, class U = typename ::std::decay<T>::type>
inline
    typename ::std::enable_if<compat::is_invocable<U, const timespec*>::value,
                              UserTimer>::type
    make_user_timer(io_ctx_t* ctx, ev_exec_t* exec,
                    ::std::reference_wrapper<T> obj) {
  return UserTimer(
      ctx, exec,
      [](const timespec* tp, void* arg) noexcept {
        auto* obj = static_cast<T*>(arg);
        (*obj)(tp);
      },
      static_cast<void*>(const_cast<U*>(&obj.get())));
}

template <class T, class U = typename ::std::decay<T>::type>
inline typename ::std::enable_if<
    compat::is_invocable<U, const UserTimer::time_point&>::value,
    UserTimer>::type
make_user_timer(io_ctx_t* ctx, ev_exec_t* exec,
                ::std::reference_wrapper<T> obj) {
  return UserTimer(
      ctx, exec,
      [](const timespec* tp, void* arg) noexcept {
        auto* obj = static_cast<T*>(arg);
        (*obj)(UserTimer::time_point{util::from_timespec(*tp)});
      },
      static_cast<void*>(const_cast<U*>(&obj.get())));
}

template <class C, void (C::*M)(const timespec*)>
inline UserTimer
make_user_timer(io_ctx_t* ctx, ev_exec_t* exec, C* obj) {
  return UserTimer(
      ctx, exec,
      [](const timespec* tp, void* arg) noexcept {
        auto obj = static_cast<C*>(arg);
        (obj->*M)(tp);
      },
      static_cast<void*>(obj));
}

template <class C, void (C::*M)(const UserTimer::time_point&)>
inline UserTimer
make_user_timer(io_ctx_t* ctx, ev_exec_t* exec, C* obj) {
  return UserTimer(
      ctx, exec,
      [](const timespec* tp, void* arg) noexcept {
        auto obj = static_cast<C*>(arg);
        (obj->*M)(UserTimer::time_point{util::from_timespec(*tp)});
      },
      static_cast<void*>(obj));
}

}  // namespace io
}  // namespace lely

#endif  // !LELY_IO2_USER_TIMER_HPP_
