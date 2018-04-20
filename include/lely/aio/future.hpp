/*!\file
 * This header file is part of the C++ asynchronous I/O library; it contains ...
 *
 * \copyright 2018 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#ifndef LELY_AIO_FUTURE_HPP_
#define LELY_AIO_FUTURE_HPP_

#include <lely/aio/detail/timespec.hpp>
#include <lely/aio/exec.hpp>

#include <lely/aio/future.h>

#include <chrono>

namespace lely {

namespace aio {

class PromiseBase : public detail::CBase<aio_promise_t> {
  friend class LoopBase;

 public:
  PromiseBase(const PromiseBase&) = delete;

  PromiseBase(PromiseBase&& other) : CBase(other.c_ptr) {
    other.c_ptr = nullptr;
  }

  PromiseBase& operator=(const PromiseBase&) = delete;

  PromiseBase&
  operator=(PromiseBase&& other) {
    ::std::swap(c_ptr, other.c_ptr);
    return *this;
  }

  ~PromiseBase();

  FutureBase GetFuture() noexcept;

  void Cancel();

  void SetValue(void* value = nullptr);

  void SetErrorCode(const ::std::error_code& ec);

 protected:
  PromiseBase(LoopBase& loop, ExecutorBase& exec, aio_dtor_t* dtor, void* arg);
};

class FutureBase : public detail::CBase<aio_future_t> {
  friend class PromiseBase;

 public:
  enum class State {
    WAITING,
    CANCELED,
    VALUE,
    ERROR
  };

  using Clock = ::std::chrono::system_clock;

  using WaitOperation = Task;

  explicit FutureBase(aio_future_t*&& c_ptr) : CBase(c_ptr) {}

  FutureBase(const FutureBase&) = delete;

  FutureBase(FutureBase&& other) : CBase(other.c_ptr) { other.c_ptr = nullptr; }

  FutureBase& operator=(const FutureBase&) = delete;

  FutureBase&
  operator=(FutureBase&& other) {
    ::std::swap(c_ptr, other.c_ptr);
    return *this;
  }

  ~FutureBase();

  aio_loop_t* GetLoop() const noexcept;

  ExecutorBase GetExecutor() const noexcept;

  State GetState() const noexcept;

  bool IsReady() const noexcept;

  bool IsCanceled() const noexcept;

  bool HasValue() const noexcept;

  bool HasErrorCode() const noexcept;

  void* GetValue() const noexcept;

  ::std::error_code GetErrorCode() const noexcept;

  void SubmitWait(aio_task& task);

  void
  SubmitWait(WaitOperation& op) { SubmitWait(static_cast<aio_task&>(op)); }

  template <class F>
  void
  SubmitWait(F&& f) {
    aio_task* task = new WaitOperationWrapper(::std::forward<F>(f));
    SubmitWait(*task);
  }

  ::std::size_t Cancel(aio_task& task);

  ::std::size_t Cancel();

  void RunWait();

  void RunWait(::std::error_code& ec);

  void RunWaitFor() { RunWaitUntil(nullptr); }

  void RunWaitFor(::std::error_code& ec) { RunWaitUntil(nullptr, ec); }

  template <class Rep, class Period>
  void
  RunWaitFor(const ::std::chrono::duration<Rep, Period>& rel_time) {
    auto ts = detail::AbsTime(rel_time);
    RunWaitUntil(&ts);
  }

  template <class Rep, class Period>
  void
  RunWaitFor(const ::std::chrono::duration<Rep, Period>& rel_time,
             ::std::error_code& ec) {
    auto ts = detail::AbsTime(rel_time);
    RunWaitUntil(&ts, ec);
  }

  template <class Clock, class Duration>
  void
  RunWaitUntil(const ::std::chrono::time_point<Clock, Duration>& abs_time) {
    auto ts = detail::AbsTime(abs_time);
    RunWaitUntil(&ts);
  }

  template <class Clock, class Duration>
  void
  RunWaitUntil(const ::std::chrono::time_point<Clock, Duration>& abs_time,
               ::std::error_code& ec) {
    auto ts = detail::AbsTime(abs_time);
    RunWaitUntil(&ts, ec);
  }

 protected:
  using WaitOperationWrapper = TaskWrapper;

  explicit FutureBase(PromiseBase& promise);

  void RunWaitUntil(const timespec* tp);

  void RunWaitUntil(const timespec* tp, ::std::error_code& ec);
};

template <class T>
class Promise : public PromiseBase {
  friend class LoopBase;

 public:
  template <class... Args>
  Promise(LoopBase& loop, ExecutorBase& base, Args&&... args)
      : Promise(loop, base, new T(::std::forward<Args>(args)...)) {}

  Promise(Promise&& other)
      : PromiseBase(::std::move(other)), value_(other.value_) {
    other.value_ = nullptr;
  }

  Promise&
  operator=(Promise&& other) {
    PromiseBase::operator=(::std::move(other));
    ::std::swap(value_, other.value_);
    return *this;
  }

  Future<T> GetFuture() noexcept { return Future<T>(*this); }

  template <class U>
  void
  SetValue(U&& value) {
    *value_ = ::std::forward<U>(value);
    PromiseBase::SetValue(value_);
  }

 private:
  Promise(LoopBase& loop, ExecutorBase& base, T* value)
      : PromiseBase(loop, base, &Dtor_, value), value_(value) {}

  static void Dtor_(void* arg) noexcept { delete static_cast<T*>(arg); }

  T* value_ { nullptr };
};

template <class T>
class Future : public FutureBase {
  friend class Promise<T>;

 public:
  using FutureBase::FutureBase;

  T& GetValue() const { return *static_cast<T*>(FutureBase::GetValue()); }

  T&
  Get() {
    RunWait();
    switch (GetState()) {
    case State::WAITING: {
      auto ec = ::std::make_error_code(::std::errc::operation_would_block);
      throw ::std::system_error(ec, "Get");
    }
    case State::CANCELED: {
      auto ec = ::std::make_error_code(::std::errc::operation_canceled);
      throw ::std::system_error(ec, "Get");
    }
    case State::VALUE:
      return GetValue();
    case State::ERROR:
    default:
      throw ::std::system_error(GetErrorCode(), "Get");
    }
  }
};

}  // namespace aio

}  // namespace lely

#endif  // LELY_AIO_FUTURE_HPP_
