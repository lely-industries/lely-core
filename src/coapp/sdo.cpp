/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the Client-SDO queue.
 *
 * @see lely/coapp/sdo.hpp
 *
 * @copyright 2018-2021 Lely Industries N.V.
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

#if !LELY_NO_CO_CSDO
#include <lely/co/csdo.h>
#endif
#include <lely/co/val.h>
#include <lely/coapp/sdo.hpp>

#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <cassert>

namespace lely {

namespace canopen {

/// The internal implementation of the Client-SDO queue.
struct Sdo::Impl_ {
  Impl_(__can_net* net, __co_dev* dev, uint8_t num);
  Impl_(__co_csdo* sdo, int timeout);
  Impl_(const Impl_&) = delete;
  Impl_& operator=(const Impl_&) = delete;
  ~Impl_();

  void Submit(detail::SdoRequestBase& req);
  ::std::size_t Cancel(detail::SdoRequestBase* req, SdoErrc ac);
  ::std::size_t Abort(detail::SdoRequestBase* req);

#if !LELY_NO_CO_CSDO
  bool Pop(detail::SdoRequestBase* req, sllist& queue);

  template <class T>
  void OnDownload(detail::SdoDownloadRequestBase<T>& req) noexcept;
  void OnDownloadDcf(detail::SdoDownloadDcfRequestBase& req) noexcept;
  template <class T>
  void OnUpload(detail::SdoUploadRequestBase<T>& req) noexcept;

  void OnDnCon(__co_csdo*, uint16_t idx, uint8_t subidx, uint32_t ac) noexcept;
  template <class T>
  void OnUpCon(__co_csdo*, uint16_t idx, uint8_t subidx, uint32_t ac,
               const void* ptr, size_t n) noexcept;

  void OnCompletion(detail::SdoRequestBase& req) noexcept;

  ::std::shared_ptr<__co_csdo> sdo;

  sllist queue;
#endif
};

namespace detail {

#if !LELY_NO_STDIO
void
SdoDownloadDcfRequestBase::Read(const char* path) {
  begin = nullptr;
  end = nullptr;
  if (dom_) {
    co_val_fini(CO_DEFTYPE_DOMAIN, &dom_);
    dom_ = nullptr;
  }

  if (!co_val_read_file(CO_DEFTYPE_DOMAIN, &dom_, path))
    util::throw_errc("ReadFile");

  begin =
      static_cast<const uint8_t*>(co_val_addressof(CO_DEFTYPE_DOMAIN, &dom_));
  end = begin + co_val_sizeof(CO_DEFTYPE_DOMAIN, &dom_);
}
#endif

SdoDownloadDcfRequestBase::~SdoDownloadDcfRequestBase() {
  if (dom_) co_val_fini(CO_DEFTYPE_DOMAIN, &dom_);
}

template <class T>
void
SdoDownloadRequestWrapper<T>::operator()() noexcept {
  auto id = this->id;
  auto idx = this->idx;
  auto subidx = this->subidx;
  auto ec = this->ec;
  ::std::function<Signature> con;
  con.swap(this->con_);
  delete this;
  if (con) con(id, idx, subidx, ec);
}

template <class T>
void
SdoDownloadRequestWrapper<T>::OnRequest(void* data) noexcept {
#if LELY_NO_CO_CSDO
  (void)data;
#else
  static_cast<Sdo::Impl_*>(data)->OnDownload(*this);
#endif
}

void
SdoDownloadDcfRequestWrapper::operator()() noexcept {
  auto id = this->id;
  auto idx = this->idx;
  auto subidx = this->subidx;
  auto ec = this->ec;
  ::std::function<Signature> con;
  con.swap(this->con_);
  delete this;
  if (con) con(id, idx, subidx, ec);
}

void
SdoDownloadDcfRequestWrapper::OnRequest(void* data) noexcept {
#if LELY_NO_CO_CSDO
  (void)data;
#else
  static_cast<Sdo::Impl_*>(data)->OnDownloadDcf(*this);
#endif
}

template <class T>
void
SdoUploadRequestWrapper<T>::operator()() noexcept {
  auto id = this->id;
  auto idx = this->idx;
  auto subidx = this->subidx;
  auto ec = this->ec;
  T value = ::std::move(this->value);
  ::std::function<Signature> con;
  con.swap(this->con_);
  delete this;
  if (con) con(id, idx, subidx, ec, ::std::move(value));
}

template <class T>
void
SdoUploadRequestWrapper<T>::OnRequest(void* data) noexcept {
#if LELY_NO_CO_CSDO
  (void)data;
#else
  static_cast<Sdo::Impl_*>(data)->OnUpload(*this);
#endif
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// BOOLEAN
template class SdoDownloadRequestWrapper<bool>;
template class SdoUploadRequestWrapper<bool>;

// INTEGER8
template class SdoDownloadRequestWrapper<int8_t>;
template class SdoUploadRequestWrapper<int8_t>;

// INTEGER16
template class SdoDownloadRequestWrapper<int16_t>;
template class SdoUploadRequestWrapper<int16_t>;

// INTEGER32
template class SdoDownloadRequestWrapper<int32_t>;
template class SdoUploadRequestWrapper<int32_t>;

// UNSIGNED8
template class SdoDownloadRequestWrapper<uint8_t>;
template class SdoUploadRequestWrapper<uint8_t>;

// UNSIGNED16
template class SdoDownloadRequestWrapper<uint16_t>;
template class SdoUploadRequestWrapper<uint16_t>;

// UNSIGNED32
template class SdoDownloadRequestWrapper<uint32_t>;
template class SdoUploadRequestWrapper<uint32_t>;

// REAL32
template class SdoDownloadRequestWrapper<float>;
template class SdoUploadRequestWrapper<float>;

// VISIBLE_STRING
template class SdoDownloadRequestWrapper<::std::string>;
template class SdoUploadRequestWrapper<::std::string>;

// OCTET_STRING
template class SdoDownloadRequestWrapper<::std::vector<uint8_t>>;
template class SdoUploadRequestWrapper<::std::vector<uint8_t>>;

// UNICODE_STRING
template class SdoDownloadRequestWrapper<::std::basic_string<char16_t>>;
template class SdoUploadRequestWrapper<::std::basic_string<char16_t>>;

// TIME_OF_DAY
// TIME_DIFFERENCE
// DOMAIN
// INTEGER24

// REAL64
template class SdoDownloadRequestWrapper<double>;
template class SdoUploadRequestWrapper<double>;

// INTEGER40
// INTEGER48
// INTEGER56

// INTEGER64
template class SdoDownloadRequestWrapper<int64_t>;
template class SdoUploadRequestWrapper<int64_t>;

// UNSIGNED24
// UNSIGNED40
// UNSIGNED48
// UNSIGNED56

// UNSIGNED64
template class SdoDownloadRequestWrapper<uint64_t>;
template class SdoUploadRequestWrapper<uint64_t>;

#endif  // !DOXYGEN_SHOULD_SKIP_THIS

}  // namespace detail

template <class T>
void
SdoDownloadRequest<T>::operator()() noexcept {
  if (con_) con_(this->id, this->idx, this->subidx, this->ec);
}

template <class T>
void
SdoDownloadRequest<T>::OnRequest(void* data) noexcept {
#if LELY_NO_CO_CSDO
  (void)data;
#else
  static_cast<Sdo::Impl_*>(data)->OnDownload(*this);
#endif
}

void
SdoDownloadDcfRequest::operator()() noexcept {
  if (con_) con_(this->id, this->idx, this->subidx, this->ec);
}

void
SdoDownloadDcfRequest::OnRequest(void* data) noexcept {
#if LELY_NO_CO_CSDO
  (void)data;
#else
  static_cast<Sdo::Impl_*>(data)->OnDownloadDcf(*this);
#endif
}

template <class T>
void
SdoUploadRequest<T>::operator()() noexcept {
  if (con_)
    con_(this->id, this->idx, this->subidx, this->ec, ::std::move(this->value));
}

template <class T>
void
SdoUploadRequest<T>::OnRequest(void* data) noexcept {
#if LELY_NO_CO_CSDO
  (void)data;
#else
  static_cast<Sdo::Impl_*>(data)->OnUpload(*this);
#endif
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// BOOLEAN
template class SdoDownloadRequest<bool>;
template class SdoUploadRequest<bool>;

// INTEGER8
template class SdoDownloadRequest<int8_t>;
template class SdoUploadRequest<int8_t>;

// INTEGER16
template class SdoDownloadRequest<int16_t>;
template class SdoUploadRequest<int16_t>;

// INTEGER32
template class SdoDownloadRequest<int32_t>;
template class SdoUploadRequest<int32_t>;

// UNSIGNED8
template class SdoDownloadRequest<uint8_t>;
template class SdoUploadRequest<uint8_t>;

// UNSIGNED16
template class SdoDownloadRequest<uint16_t>;
template class SdoUploadRequest<uint16_t>;

// UNSIGNED32
template class SdoDownloadRequest<uint32_t>;
template class SdoUploadRequest<uint32_t>;

// REAL32
template class SdoDownloadRequest<float>;
template class SdoUploadRequest<float>;

// VISIBLE_STRING
template class SdoDownloadRequest<::std::string>;
template class SdoUploadRequest<::std::string>;

// OCTET_STRING
template class SdoDownloadRequest<::std::vector<uint8_t>>;
template class SdoUploadRequest<::std::vector<uint8_t>>;

// UNICODE_STRING
template class SdoDownloadRequest<::std::basic_string<char16_t>>;
template class SdoUploadRequest<::std::basic_string<char16_t>>;

// TIME_OF_DAY
// TIME_DIFFERENCE
// DOMAIN
// INTEGER24

// REAL64
template class SdoDownloadRequest<double>;
template class SdoUploadRequest<double>;

// INTEGER40
// INTEGER48
// INTEGER56

// INTEGER64
template class SdoDownloadRequest<int64_t>;
template class SdoUploadRequest<int64_t>;

// UNSIGNED24
// UNSIGNED40
// UNSIGNED48
// UNSIGNED56

// UNSIGNED64
template class SdoDownloadRequest<uint64_t>;
template class SdoUploadRequest<uint64_t>;

#endif  // !DOXYGEN_SHOULD_SKIP_THIS

Sdo::Sdo() = default;

Sdo::Sdo(__can_net* net, uint8_t id) : Sdo(net, nullptr, id) {}

Sdo::Sdo(__can_net* net, __co_dev* dev, uint8_t num)
    : impl_(new Impl_(net, dev, num)) {}

Sdo::Sdo(__co_csdo* sdo)
#if LELY_NO_CO_CSDO
    : impl_(new Impl_(sdo, 0))
#else
    : impl_(new Impl_(sdo, co_csdo_get_timeout(sdo)))
#endif
{
}

Sdo& Sdo::operator=(Sdo&&) = default;

Sdo::~Sdo() = default;

SdoFuture<void>
Sdo::AsyncDownloadDcf(ev_exec_t* exec, const uint8_t* begin, const uint8_t* end,
                      const ::std::chrono::milliseconds& timeout) {
  SdoPromise<void> p;
  SubmitDownloadDcf(
      exec, begin, end,
      [p](uint8_t id, uint16_t idx, uint8_t subidx,
          ::std::error_code ec) mutable {
        if (ec)
          p.set(util::failure(
              make_sdo_exception_ptr(id, idx, subidx, ec, "AsyncDownloadDcf")));
        else
          p.set(util::success());
      },
      timeout);
  return p.get_future();
}

SdoFuture<void>
Sdo::AsyncDownloadDcf(ev_exec_t* exec, const char* path,
                      const ::std::chrono::milliseconds& timeout) {
  SdoPromise<void> p;
  SubmitDownloadDcf(
      exec, path,
      [p](uint8_t id, uint16_t idx, uint8_t subidx,
          ::std::error_code ec) mutable {
        if (ec)
          p.set(util::failure(
              make_sdo_exception_ptr(id, idx, subidx, ec, "AsyncDownloadDcf")));
        else
          p.set(util::success());
      },
      timeout);
  return p.get_future();
}

void
Sdo::Submit(detail::SdoRequestBase& req) {
  impl_->Submit(req);
}

bool
Sdo::Cancel(detail::SdoRequestBase& req, SdoErrc ac) {
  return impl_->Cancel(&req, ac) != 0;
}

::std::size_t
Sdo::CancelAll(SdoErrc ac) {
  return impl_->Cancel(nullptr, ac);
}

bool
Sdo::Abort(detail::SdoRequestBase& req) {
  return impl_->Abort(&req) != 0;
}

::std::size_t
Sdo::AbortAll() {
  return impl_->Abort(nullptr);
}

Sdo::Impl_::Impl_(__can_net* net, __co_dev* dev, uint8_t num)
#if LELY_NO_CO_CSDO
{
  (void)net;
  (void)dev;
  (void)num;
#else
    : sdo(co_csdo_create(net, dev, num),
          [=](__co_csdo* sdo) noexcept { co_csdo_destroy(sdo); }) {
  sllist_init(&queue);
#endif
}

Sdo::Impl_::Impl_(__co_csdo* sdo_, int timeout)
#if LELY_NO_CO_CSDO
{
  (void)sdo_;
  (void)timeout;
#else
    : sdo(sdo_,
          [=](__co_csdo* sdo) noexcept { co_csdo_set_timeout(sdo, timeout); }) {
  sllist_init(&queue);
#endif
}

Sdo::Impl_::~Impl_() { Cancel(nullptr, SdoErrc::NO_SDO); }

void
Sdo::Impl_::Submit(detail::SdoRequestBase& req) {
  assert(req.exec);
  ev::Executor exec(req.exec);

#if !LELY_NO_CO_CSDO
  exec.on_task_init();
  if (!sdo) {
#endif
    req.id = 0;
    req.ec = SdoErrc::NO_SDO;
    exec.post(req);
#if !LELY_NO_CO_CSDO
    exec.on_task_fini();
  } else {
    req.id = co_csdo_get_par(sdo.get())->id;
    bool first = sllist_empty(&queue);
    sllist_push_back(&queue, &req._node);
    if (first) req.OnRequest(this);
  }
#endif
}

::std::size_t
Sdo::Impl_::Cancel(detail::SdoRequestBase* req, SdoErrc ac) {
#if LELY_NO_CO_CSDO
  (void)req;
  (void)ac;

  return 0;
#else
  sllist queue;
  sllist_init(&queue);

  // Cancel all matching requests, except for the first (ongoing) request.
  if (Pop(req, queue))
    // Stop the ongoing request, if any.
    co_csdo_abort_req(sdo.get(), static_cast<uint32_t>(ac));

  ::std::size_t n = 0;
  slnode* node;
  while ((node = sllist_pop_front(&queue))) {
    req = static_cast<detail::SdoRequestBase*>(ev_task_from_node(node));
    req->ec = ac;

    auto exec = req->GetExecutor();
    exec.post(*req);
    exec.on_task_fini();

    n += n < ::std::numeric_limits<::std::size_t>::max();
  }
  return n;
#endif
}

::std::size_t
Sdo::Impl_::Abort(detail::SdoRequestBase* req) {
#if LELY_NO_CO_CSDO
  (void)req;

  return 0;
#else
  sllist queue;
  sllist_init(&queue);

  // Abort all matching requests, except for the first (ongoing) request.
  Pop(req, queue);

  return ev_task_queue_abort(&queue);
#endif
}

#if !LELY_NO_CO_CSDO

bool
Sdo::Impl_::Pop(detail::SdoRequestBase* req, sllist& queue) {
  if (!req) {
    // Cancel all pending requests except for the first (ongoing) request.
    auto node = sllist_pop_front(&this->queue);
    sllist_append(&queue, &this->queue);
    if (node) {
      sllist_push_front(&this->queue, node);
      req = static_cast<detail::SdoRequestBase*>(ev_task_from_node(node));
    }
  } else if (&req->_node != sllist_first(&this->queue)) {
    if (sllist_remove(&queue, &req->_node))
      sllist_push_back(&this->queue, &req->_node);
    req = nullptr;
  }
  // Return true if the first request matched (but was not removed).
  return req != nullptr;
}

template <class T>
void
Sdo::Impl_::OnDownload(detail::SdoDownloadRequestBase<T>& req) noexcept {
  assert(&req._node == sllist_first(&queue));

  using traits = canopen_traits<T>;

  auto val = traits::to_c_type(req.value, req.ec);
  if (req.ec) {
    OnCompletion(req);
  } else {
    int errsv = get_errc();
    set_errc(0);

    co_csdo_set_timeout(sdo.get(), detail::to_sdo_timeout(req.timeout));

    auto con = [](__co_csdo* sdo, uint16_t idx, uint8_t subidx, uint32_t ac,
                  void* data) noexcept {
      static_cast<Impl_*>(data)->OnDnCon(sdo, idx, subidx, ac);
    };
    int result = req.block
                     ? co_csdo_blk_dn_val_req(sdo.get(), req.idx, req.subidx,
                                              traits::index, &val, con, this)
                     : co_csdo_dn_val_req(sdo.get(), req.idx, req.subidx,
                                          traits::index, &val, con, this);
    if (result == -1) {
      req.ec = util::make_error_code();
      OnCompletion(req);
    }

    set_errc(errsv);
    traits::destroy(val);
  }
}

void
Sdo::Impl_::OnDownloadDcf(detail::SdoDownloadDcfRequestBase& req) noexcept {
  assert(&req._node == sllist_first(&queue));

  int errsv = get_errc();
  set_errc(0);

  co_csdo_set_timeout(sdo.get(), detail::to_sdo_timeout(req.timeout));
  if (co_csdo_dn_dcf_req(
          sdo.get(), req.begin, req.end,
          [](__co_csdo* sdo, uint16_t idx, uint8_t subidx, uint32_t ac,
             void* data) noexcept {
            static_cast<Impl_*>(data)->OnDnCon(sdo, idx, subidx, ac);
          },
          this) == -1) {
    req.ec = util::make_error_code();
    OnCompletion(req);
  }

  set_errc(errsv);
}

template <class T>
void
Sdo::Impl_::OnUpload(detail::SdoUploadRequestBase<T>& req) noexcept {
  assert(&req._node == sllist_first(&queue));

  int errsv = get_errc();
  set_errc(0);

  co_csdo_set_timeout(sdo.get(), detail::to_sdo_timeout(req.timeout));

  auto con = [](__co_csdo* sdo, uint16_t idx, uint8_t subidx, uint32_t ac,
                const void* ptr, size_t n, void* data) noexcept {
    static_cast<Impl_*>(data)->OnUpCon<T>(sdo, idx, subidx, ac, ptr, n);
  };
  int result =
      req.block
          ? co_csdo_blk_up_req(sdo.get(), req.idx, req.subidx, 0, con, this)
          : co_csdo_up_req(sdo.get(), req.idx, req.subidx, con, this);
  if (result == -1) {
    req.ec = util::make_error_code();
    OnCompletion(req);
  }

  set_errc(errsv);
}

void
Sdo::Impl_::OnDnCon(__co_csdo*, uint16_t idx, uint8_t subidx,
                    uint32_t ac) noexcept {
  auto task = ev_task_from_node(sllist_first(&queue));
  assert(task);
  auto req = static_cast<detail::SdoRequestBase*>(task);

  req->idx = idx;
  req->subidx = subidx;
  req->ec = SdoErrc(ac);

  OnCompletion(*req);
}

template <class T>
void
Sdo::Impl_::OnUpCon(__co_csdo*, uint16_t idx, uint8_t subidx, uint32_t ac,
                    const void* ptr, size_t n) noexcept {
  using traits = canopen_traits<T>;
  using c_type = typename traits::c_type;

  auto task = ev_task_from_node(sllist_first(&queue));
  assert(task);
  auto req = static_cast<detail::SdoUploadRequestBase<T>*>(task);

  auto val = c_type();
  if (!ac) {
    ::std::error_code ec;
    val = traits::construct(ptr, n, ec);
    if (ec) ac = static_cast<uint32_t>(sdo_errc(ec));
  }

  req->idx = idx;
  req->subidx = subidx;
  req->ec = SdoErrc(ac);
  req->value = ac ? T() : traits::from_c_type(val);

  traits::destroy(val);

  OnCompletion(*req);
}

void
Sdo::Impl_::OnCompletion(detail::SdoRequestBase& req) noexcept {
  assert(&req._node == sllist_first(&queue));
  sllist_pop_front(&queue);

  ev::Executor exec(req.exec);
  exec.post(req);
  exec.on_task_fini();

  auto task = ev_task_from_node(sllist_first(&queue));
  if (task) static_cast<detail::SdoRequestBase*>(task)->OnRequest(this);
}

#endif  // !LELY_NO_CO_CSDO

}  // namespace canopen

}  // namespace lely
