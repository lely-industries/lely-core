/*!\file
 * This file is part of the C++ asynchronous I/O library; it contains ...
 *
 * \see lely/aio/loop.hpp
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

#include "aio.hpp"

#if !LELY_NO_CXX

#include <lely/aio/future.hpp>
#include <lely/aio/loop.hpp>

namespace lely {

namespace aio {

LELY_AIO_EXPORT
PromiseBase::PromiseBase(LoopBase& loop, ExecutorBase& exec, aio_dtor_t* dtor,
                         void* arg)
    : CBase(InvokeC("PromiseBase", aio_promise_create, loop, exec, dtor, arg)) {
}

LELY_AIO_EXPORT
PromiseBase::~PromiseBase() { aio_promise_destroy(*this); }

LELY_AIO_EXPORT FutureBase
PromiseBase::GetFuture() noexcept { return FutureBase(*this); }

LELY_AIO_EXPORT void PromiseBase::Cancel() { aio_promise_cancel(*this); }

LELY_AIO_EXPORT void
PromiseBase::SetValue(void* value) { aio_promise_set_value(*this, value); }

LELY_AIO_EXPORT void
PromiseBase::SetErrorCode(const ::std::error_code& ec) {
  aio_promise_set_errc(*this, ec.value());
}

LELY_AIO_EXPORT
FutureBase::FutureBase(PromiseBase& promise)
    : FutureBase(InvokeC("FutureBase", aio_future_create, promise)) {}

LELY_AIO_EXPORT FutureBase::~FutureBase() { aio_future_destroy(*this); }

LELY_AIO_EXPORT aio_loop_t*
FutureBase::GetLoop() const noexcept { return aio_future_get_loop(*this); }

LELY_AIO_EXPORT ExecutorBase
FutureBase::GetExecutor() const noexcept { return aio_future_get_exec(*this); }

LELY_AIO_EXPORT FutureBase::State
FutureBase::GetState() const noexcept {
  return static_cast<State>(aio_future_get_state(*this));
}

LELY_AIO_EXPORT bool
FutureBase::IsReady() const noexcept { return aio_future_is_ready(*this); }

LELY_AIO_EXPORT bool
FutureBase::IsCanceled() const noexcept {
  return aio_future_is_canceled(*this);
}

LELY_AIO_EXPORT bool
FutureBase::HasValue() const noexcept { return aio_future_has_value(*this); }

LELY_AIO_EXPORT bool
FutureBase::HasErrorCode() const noexcept { return aio_future_has_errc(*this); }

LELY_AIO_EXPORT void*
FutureBase::GetValue() const noexcept { return aio_future_get_value(*this); }

LELY_AIO_EXPORT ::std::error_code
FutureBase::GetErrorCode() const noexcept {
  return ::std::error_code(aio_future_get_errc(*this),
                           ::std::system_category());
}

LELY_AIO_EXPORT void
FutureBase::SubmitWait(aio_task& task) { aio_future_submit_wait(*this, &task); }

LELY_AIO_EXPORT ::std::size_t
FutureBase::Cancel(aio_task& task) { return aio_future_cancel(*this, &task); }

LELY_AIO_EXPORT ::std::size_t
FutureBase::Cancel() { return aio_future_cancel(*this, nullptr); }

LELY_AIO_EXPORT void
FutureBase::RunWait() { InvokeC("RunWait", aio_future_run_wait, *this); }

LELY_AIO_EXPORT void
FutureBase::RunWait(::std::error_code& ec) {
  InvokeC(ec, aio_future_run_wait, *this);
}

LELY_AIO_EXPORT void
FutureBase::RunWaitUntil(const timespec* tp) {
  InvokeC("RunWaitUntil", aio_future_run_wait_until, *this, tp);
}

LELY_AIO_EXPORT void
FutureBase::RunWaitUntil(const timespec* tp, ::std::error_code& ec) {
  InvokeC(ec, aio_future_run_wait_until, *this, tp);
}

}  // namespace aio

}  // namespace lely

#endif  // !LELY_NO_CXX
