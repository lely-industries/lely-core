/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the Client-SDO queue.
 *
 * @see lely/coapp/sdo.hpp
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

#include "coapp.hpp"

#include <lely/coapp/detail/chrono.hpp>
#include <lely/coapp/sdo.hpp>

#include <cassert>

#include <lely/aio/queue.h>
#include <lely/co/csdo.hpp>

namespace lely {

namespace canopen {

namespace {

::std::error_code
ErrorCode(int errc, uint32_t ac) {
  if (errc)
    return ::std::error_code(errc, ::std::system_category());
  else if (ac)
    return static_cast<SdoErrc>(ac);
  else
    return {};
}

}  // namespace

using namespace aio;

/// The internal implementation of the Client-SDO queue.
struct Sdo::Impl_ {
  Impl_(CANNet* net, CODev* dev, uint8_t num);
  Impl_(COCSDO* sdo, int timeout);
  ~Impl_();

  void Submit(RequestBase_& req);

  ::std::size_t Cancel(RequestBase_* req, SdoErrc ac);

  template <class T, class U>
  void OnRequest(uint16_t idx, uint8_t subidx, U&& value);

  template <class T>
  void OnRequest(uint16_t idx, uint8_t subidx);

  void OnDnCon(COCSDO*, uint16_t idx, uint8_t subidx, uint32_t ac) noexcept;

  template <class T>
  void OnUpCon(COCSDO*, uint16_t idx, uint8_t subidx, uint32_t ac,
               T value) noexcept;

  void OnTaskFinished(RequestBase_& op) noexcept;

  aio_queue queue;
  ::std::shared_ptr<COCSDO> sdo;
};

template <class T>
void
Sdo::DownloadRequest<T>::OnRequest(Impl_* impl) noexcept {
  impl->OnRequest<T>(this->idx, this->subidx, this->value);
}

template <class T>
void
Sdo::DownloadRequest<T>::Func_(aio_task* task) noexcept {
  auto self = static_cast<Sdo::DownloadRequest<T>*>(task);

  if (self->con_)
    self->con_(self->idx, self->subidx, ErrorCode(self->errc, self->ac));
}

template <class T>
void
Sdo::UploadRequest<T>::OnRequest(Impl_* impl) noexcept {
  impl->OnRequest<T>(this->idx, this->subidx);
}

template <class T>
void
Sdo::UploadRequest<T>::Func_(aio_task* task) noexcept {
  auto self = static_cast<Sdo::UploadRequest<T>*>(task);

  if (self->con_)
    self->con_(self->idx, self->subidx, ErrorCode(self->errc, self->ac),
               ::std::move(self->value));
}

template <class T>
void
Sdo::DownloadRequestWrapper<T>::OnRequest(Impl_* impl) noexcept {
  impl->OnRequest<T>(this->idx, this->subidx, this->value);
}

template <class T>
void
Sdo::DownloadRequestWrapper<T>::Func_(aio_task* task) noexcept {
  auto self = static_cast<Sdo::DownloadRequestWrapper<T>*>(task);

  ::std::function<DownloadSignature> con = ::std::move(self->con_);
  auto idx = self->idx;
  auto subidx = self->subidx;
  auto ec = ErrorCode(self->errc, self->ac);
  delete self;

  if (con) con(idx, subidx, ec);
}

template <class T>
void
Sdo::UploadRequestWrapper<T>::OnRequest(Impl_* impl) noexcept {
  impl->OnRequest<T>(this->idx, this->subidx);
}

template <class T>
void
Sdo::UploadRequestWrapper<T>::Func_(aio_task* task) noexcept {
  auto self = static_cast<Sdo::UploadRequestWrapper<T>*>(task);

  ::std::function<UploadSignature<T>> con = ::std::move(self->con_);
  auto idx = self->idx;
  auto subidx = self->subidx;
  auto ec = ErrorCode(self->errc, self->ac);
  T value = ::std::move(self->value);
  delete self;

  if (con) con(idx, subidx, ec, ::std::move(value));
}

template <class T>
void
Sdo::AsyncDownloadRequest<T>::OnRequest(Impl_* impl) noexcept {
  impl->OnRequest<T>(this->idx, this->subidx, this->value);
}

template <class T>
void
Sdo::AsyncDownloadRequest<T>::Func_(aio_task* task) noexcept {
  auto self = static_cast<Sdo::AsyncDownloadRequest<T>*>(task);

  aio::Promise<::std::error_code> promise = ::std::move(self->promise_);
  auto ec = ErrorCode(self->errc, self->ac);
  delete self;

  promise.SetValue(ec);
}

template <class T>
void
Sdo::AsyncUploadRequest<T>::OnRequest(Impl_* impl) noexcept {
  impl->OnRequest<T>(this->idx, this->subidx);
}

template <class T>
void
Sdo::AsyncUploadRequest<T>::Func_(aio_task* task) noexcept {
  auto self = static_cast<Sdo::AsyncUploadRequest<T>*>(task);

  aio::Promise<::std::tuple<::std::error_code, T>> promise =
      ::std::move(self->promise_);
  auto value = ::std::make_tuple(ErrorCode(self->errc, self->ac),
                                 ::std::move(self->value));
  delete self;

  promise.SetValue(value);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// BOOLEAN
template class Sdo::DownloadRequest<bool>;
template class Sdo::UploadRequest<bool>;
template class Sdo::DownloadRequestWrapper<bool>;
template class Sdo::UploadRequestWrapper<bool>;
template class Sdo::AsyncDownloadRequest<bool>;
template class Sdo::AsyncUploadRequest<bool>;

// INTEGER8
template class Sdo::DownloadRequest<int8_t>;
template class Sdo::UploadRequest<int8_t>;
template class Sdo::DownloadRequestWrapper<int8_t>;
template class Sdo::UploadRequestWrapper<int8_t>;
template class Sdo::AsyncDownloadRequest<int8_t>;
template class Sdo::AsyncUploadRequest<int8_t>;

// INTEGER16
template class Sdo::DownloadRequest<int16_t>;
template class Sdo::UploadRequest<int16_t>;
template class Sdo::DownloadRequestWrapper<int16_t>;
template class Sdo::UploadRequestWrapper<int16_t>;
template class Sdo::AsyncDownloadRequest<int16_t>;
template class Sdo::AsyncUploadRequest<int16_t>;

// INTEGER32
template class Sdo::DownloadRequest<int32_t>;
template class Sdo::UploadRequest<int32_t>;
template class Sdo::DownloadRequestWrapper<int32_t>;
template class Sdo::UploadRequestWrapper<int32_t>;
template class Sdo::AsyncDownloadRequest<int32_t>;
template class Sdo::AsyncUploadRequest<int32_t>;

// UNSIGNED8
template class Sdo::DownloadRequest<uint8_t>;
template class Sdo::UploadRequest<uint8_t>;
template class Sdo::DownloadRequestWrapper<uint8_t>;
template class Sdo::UploadRequestWrapper<uint8_t>;
template class Sdo::AsyncDownloadRequest<uint8_t>;
template class Sdo::AsyncUploadRequest<uint8_t>;

// UNSIGNED16
template class Sdo::DownloadRequest<uint16_t>;
template class Sdo::UploadRequest<uint16_t>;
template class Sdo::DownloadRequestWrapper<uint16_t>;
template class Sdo::UploadRequestWrapper<uint16_t>;
template class Sdo::AsyncDownloadRequest<uint16_t>;
template class Sdo::AsyncUploadRequest<uint16_t>;

// UNSIGNED32
template class Sdo::DownloadRequest<uint32_t>;
template class Sdo::UploadRequest<uint32_t>;
template class Sdo::DownloadRequestWrapper<uint32_t>;
template class Sdo::UploadRequestWrapper<uint32_t>;
template class Sdo::AsyncDownloadRequest<uint32_t>;
template class Sdo::AsyncUploadRequest<uint32_t>;

// REAL32
template class Sdo::DownloadRequest<float>;
template class Sdo::UploadRequest<float>;
template class Sdo::DownloadRequestWrapper<float>;
template class Sdo::UploadRequestWrapper<float>;
template class Sdo::AsyncDownloadRequest<float>;
template class Sdo::AsyncUploadRequest<float>;

// VISIBLE_STRING
template class Sdo::DownloadRequest<::std::string>;
template class Sdo::UploadRequest<::std::string>;
template class Sdo::DownloadRequestWrapper<::std::string>;
template class Sdo::UploadRequestWrapper<::std::string>;
template class Sdo::AsyncDownloadRequest<::std::string>;
template class Sdo::AsyncUploadRequest<::std::string>;

// OCTET_STRING
template class Sdo::DownloadRequest<::std::vector<uint8_t>>;
template class Sdo::UploadRequest<::std::vector<uint8_t>>;
template class Sdo::DownloadRequestWrapper<::std::vector<uint8_t>>;
template class Sdo::UploadRequestWrapper<::std::vector<uint8_t>>;
template class Sdo::AsyncDownloadRequest<::std::vector<uint8_t>>;
template class Sdo::AsyncUploadRequest<::std::vector<uint8_t>>;

// UNICODE_STRING
template class Sdo::DownloadRequest<::std::basic_string<char16_t>>;
template class Sdo::UploadRequest<::std::basic_string<char16_t>>;
template class Sdo::DownloadRequestWrapper<::std::basic_string<char16_t>>;
template class Sdo::UploadRequestWrapper<::std::basic_string<char16_t>>;
template class Sdo::AsyncDownloadRequest<::std::basic_string<char16_t>>;
template class Sdo::AsyncUploadRequest<::std::basic_string<char16_t>>;

// TIME_OF_DAY
// TIME_DIFFERENCE
// DOMAIN
// INTEGER24

// REAL64
template class Sdo::DownloadRequest<double>;
template class Sdo::UploadRequest<double>;
template class Sdo::DownloadRequestWrapper<double>;
template class Sdo::UploadRequestWrapper<double>;
template class Sdo::AsyncDownloadRequest<double>;
template class Sdo::AsyncUploadRequest<double>;

// INTEGER40
// INTEGER48
// INTEGER56

// INTEGER64
template class Sdo::DownloadRequest<int64_t>;
template class Sdo::UploadRequest<int64_t>;
template class Sdo::DownloadRequestWrapper<int64_t>;
template class Sdo::UploadRequestWrapper<int64_t>;
template class Sdo::AsyncDownloadRequest<int64_t>;
template class Sdo::AsyncUploadRequest<int64_t>;

// UNSIGNED24
// UNSIGNED40
// UNSIGNED48
// UNSIGNED56

// UNSIGNED64
template class Sdo::DownloadRequest<uint64_t>;
template class Sdo::UploadRequest<uint64_t>;
template class Sdo::DownloadRequestWrapper<uint64_t>;
template class Sdo::UploadRequestWrapper<uint64_t>;
template class Sdo::AsyncDownloadRequest<uint64_t>;
template class Sdo::AsyncUploadRequest<uint64_t>;

#endif  // !DOXYGEN_SHOULD_SKIP_THIS

Sdo::Sdo() = default;

Sdo::Sdo(CANNet* net, uint8_t id) : Sdo(net, nullptr, id) {}

Sdo::Sdo(CANNet* net, CODev* dev, uint8_t num)
    : impl_(new Impl_(net, dev, num)) {}

Sdo::Sdo(COCSDO* sdo) : impl_(new Impl_(sdo, sdo->getTimeout())) {}

Sdo& Sdo::operator=(Sdo&&) = default;

Sdo::~Sdo() = default;

void
Sdo::Submit(RequestBase_& op) {
  impl_->Submit(op);
}

::std::size_t
Sdo::Cancel(RequestBase_& op, SdoErrc ac) {
  return impl_->Cancel(&op, ac);
}

::std::size_t
Sdo::Cancel(SdoErrc ac) {
  return impl_->Cancel(nullptr, ac);
}

Sdo::Impl_::Impl_(CANNet* net, CODev* dev, uint8_t num)
    : sdo(make_shared_c<COCSDO>(net, dev, num)) {
  aio_queue_init(&queue);
}

Sdo::Impl_::Impl_(COCSDO* sdo_, int timeout)
    : sdo(sdo_, [=](COCSDO* sdo) { sdo->setTimeout(timeout); }) {
  aio_queue_init(&queue);
}

Sdo::Impl_::~Impl_() { Cancel(nullptr, SdoErrc::NO_SDO); }

void
Sdo::Impl_::Submit(RequestBase_& req) {
  assert(req.exec);
  req.GetExecutor().OnTaskStarted();
  req.errc = errnum2c(ERRNUM_INPROGRESS);

  if (!sdo) {
    req.errc = 0;
    req.ac = static_cast<uint32_t>(SdoErrc::NO_SDO);
    OnTaskFinished(req);
  } else {
    bool first = aio_queue_empty(&queue);
    aio_queue_push(&queue, &req);
    if (first) req.OnRequest(this);
  }
}

::std::size_t
Sdo::Impl_::Cancel(RequestBase_* req, SdoErrc ac) {
  aio_queue queue_;
  aio_queue_init(&queue_);

  if (!req || req != aio_queue_front(&queue)) {
    auto task = aio_queue_pop(&queue);
    aio_queue_move(&queue_, &queue, req);
    if (task) aio_queue_push(&queue, task);
    req = task && !req ? static_cast<RequestBase_*>(task) : nullptr;
  }

  if (req) {
    assert(req == aio_queue_front(&queue));
    sdo->abortReq(static_cast<uint32_t>(ac));
  }

  auto n = aio_queue_cancel(&queue_, errnum2c(ERRNUM_CANCELED));
  if (req) n += n < ::std::numeric_limits<decltype(n)>::max();
  return n;
}

template <class T, class U>
void
Sdo::Impl_::OnRequest(uint16_t idx, uint8_t subidx, U&& value) {
  constexpr auto N = co_type_traits_T<T>::index;

  auto task = aio_queue_front(&queue);
  assert(task);
  auto& req = *static_cast<Sdo::RequestBase_*>(task);

  sdo->setTimeout(detail::ToTimeout(req.timeout));
  if (sdo->dnReq<N, Impl_, &Impl_::OnDnCon>(
          idx, subidx, ::std::forward<U>(value), this) == -1) {
    req.errc = get_errc();
    OnTaskFinished(req);
  }
}

template <class T>
void
Sdo::Impl_::OnRequest(uint16_t idx, uint8_t subidx) {
  auto task = aio_queue_front(&queue);
  assert(task);
  auto& req = *static_cast<Sdo::RequestBase_*>(task);

  sdo->setTimeout(detail::ToTimeout(req.timeout));
  if (sdo->upReq<T, Impl_, &Impl_::OnUpCon<T>>(idx, subidx, this) == -1) {
    req.errc = get_errc();
    OnTaskFinished(req);
  }
}

void
Sdo::Impl_::OnDnCon(COCSDO*, uint16_t idx, uint8_t subidx,
                    uint32_t ac) noexcept {
  auto task = aio_queue_pop(&queue);
  assert(task);
  auto& req = *static_cast<Sdo::RequestBase_*>(task);

  req.errc = 0;
  req.idx = idx;
  req.subidx = subidx;
  req.ac = ac;

  OnTaskFinished(req);
}

template <class T>
void
Sdo::Impl_::OnUpCon(COCSDO*, uint16_t idx, uint8_t subidx, uint32_t ac,
                    T value) noexcept {
  auto task = aio_queue_pop(&queue);
  assert(task);
  auto& req = *static_cast<Sdo::UploadRequestBase_<T>*>(task);

  req.errc = 0;
  req.idx = idx;
  req.subidx = subidx;
  req.ac = ac;
  req.value = ::std::move(value);

  OnTaskFinished(req);
}

void
Sdo::Impl_::OnTaskFinished(RequestBase_& req) noexcept {
  auto exec = req.GetExecutor();
  exec.Post(req);
  exec.OnTaskFinished();

  auto task = aio_queue_front(&queue);
  if (task) static_cast<Sdo::RequestBase_*>(task)->OnRequest(this);
}

}  // namespace canopen

}  // namespace lely
