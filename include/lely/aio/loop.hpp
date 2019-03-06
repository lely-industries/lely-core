/**@file
 * This header file is part of the C++ asynchronous I/O library; it contains ...
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

#ifndef LELY_AIO_LOOP_HPP_
#define LELY_AIO_LOOP_HPP_

#include <lely/aio/future.hpp>
#include <lely/aio/poll.hpp>

#include <lely/aio/loop.h>

namespace lely {

namespace aio {

class LoopBase : public detail::CBase<aio_loop_t> {
 public:
  using CBase::CBase;

  PollBase GetPoll() const noexcept;

  PromiseBase GetPromise(ExecutorBase& exec, aio_dtor_t* dtor = nullptr,
                         void* arg = nullptr);

  template <class T, class... Args>
  Promise<T>
  GetPromise(ExecutorBase& exec, Args&&... args) {
    return Promise<T>(*this, exec, ::std::forward<Args>(args)...);
  }

  void Post(aio_task& task);

  void OnTaskStarted();

  void OnTaskFinished();

  ::std::size_t Run();

  ::std::size_t Run(::std::error_code& ec);

  ::std::size_t
  RunFor() {
    return RunUntil(nullptr);
  }

  ::std::size_t
  RunFor(::std::error_code& ec) {
    return RunUntil(nullptr, ec);
  }

  template <class Rep, class Period>
  ::std::size_t
  RunFor(const ::std::chrono::duration<Rep, Period>& rel_time) {
    auto ts = detail::AbsTime(rel_time);
    return RunUntil(&ts);
  }

  template <class Rep, class Period>
  ::std::size_t
  RunFor(const ::std::chrono::duration<Rep, Period>& rel_time,
         ::std::error_code& ec) {
    auto ts = detail::AbsTime(rel_time);
    return RunUntil(&ts, ec);
  }

  template <class Clock, class Duration>
  ::std::size_t
  RunUntil(const ::std::chrono::time_point<Clock, Duration>& abs_time) {
    auto ts = detail::AbsTime(abs_time);
    return RunUntil(&ts);
  }

  template <class Clock, class Duration>
  ::std::size_t
  RunUntil(const ::std::chrono::time_point<Clock, Duration>& abs_time,
           ::std::error_code& ec) {
    auto ts = detail::AbsTime(abs_time);
    return RunUntil(&ts, ec);
  }

  void Stop();

  bool Stopped() const noexcept;

  void Restart();

 protected:
  ::std::size_t RunUntil(const timespec* tp);

  ::std::size_t RunUntil(const timespec* tp, ::std::error_code& ec);
};

class Loop : public LoopBase {
 public:
  Loop();

  explicit Loop(const PollBase& poll);

  Loop(const Loop&) = delete;

  Loop(Loop&& other) : LoopBase(other.c_ptr) { other.c_ptr = nullptr; }

  Loop& operator=(const Loop&) = delete;

  Loop&
  operator=(Loop&& other) {
    ::std::swap(c_ptr, other.c_ptr);
    return *this;
  }

  ~Loop();
};

}  // namespace aio

}  // namespace lely

#endif  // LELY_AIO_LOOP_HPP_
