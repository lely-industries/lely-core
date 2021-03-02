/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the CANopen Layer Setting Services (LSS) master.
 *
 * @see lely/coapp/lss_master.hpp
 *
 * @copyright 2020-2021 Lely Industries N.V.
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

#if !LELY_NO_CO_LSS

#include <lely/coapp/lss_master.hpp>

#include <lely/co/lss.h>
#include <lely/co/nmt.h>

#include <algorithm>
#include <limits>

#include <cassert>

namespace lely {

namespace canopen {

struct LssMaster::Impl_ : public util::BasicLockable {
  Impl_(LssMaster* self, ev_exec_t* exec, Node& node,
        io::CanControllerBase* ctrl, co_nmt_t* nmt);
  ~Impl_();

  void
  lock() final {
    self->lock();
  }

  void
  unlock() final {
    self->unlock();
  }

  void OnLssReq(co_nmt_t*, co_lss_t* lss) noexcept;

  void Submit(detail::LssRequestBase& req);
  ::std::size_t Cancel(detail::LssRequestBase* req, ::std::error_code ec);
  ::std::size_t Abort(detail::LssRequestBase* req);

  bool Pop(detail::LssRequestBase* req, sllist& queue);

  // The functions called by the OnRequest() methods to initiate an LSS request.
  void OnSwitch(detail::LssSwitchRequestBase& req) noexcept;
  void OnSwitchSelective(detail::LssSwitchSelectiveRequestBase& req) noexcept;
  void OnSetId(detail::LssSetIdRequestBase& req) noexcept;
  void OnSetBitrate(detail::LssSetBitrateRequestBase& req) noexcept;
  void OnSwitchBitrate(detail::LssSwitchBitrateRequestBase& req) noexcept;
  void OnStore(detail::LssStoreRequestBase& req) noexcept;
  void OnGetVendorId(detail::LssGetVendorIdRequestBase& req) noexcept;
  void OnGetProductCode(detail::LssGetProductCodeRequestBase& req) noexcept;
  void OnGetRevision(detail::LssGetRevisionRequestBase& req) noexcept;
  void OnGetSerialNr(detail::LssGetSerialNrRequestBase& req) noexcept;
  void OnGetId(detail::LssGetIdRequestBase& req) noexcept;
  void OnIdNonConfig(detail::LssIdNonConfigRequestBase& req) noexcept;
  void OnSlowscan(detail::LssSlowscanRequestBase& req) noexcept;
  void OnFastscan(detail::LssFastscanRequestBase& req) noexcept;

  // The indication functions called by the LSS master service when an LSS
  // request completes. See the function type definitions in <lely/co/lss.h> for
  // more information.
  void OnCsInd(co_lss_t*, uint8_t cs) noexcept;
  void OnErrInd(co_lss_t*, uint8_t cs, uint8_t err, uint8_t spec) noexcept;
  void OnLssIdInd(co_lss_t*, uint8_t cs, co_unsigned32_t id) noexcept;
  void OnNidInd(co_lss_t*, uint8_t cs, uint8_t id) noexcept;
  void OnScanInd(co_lss_t*, uint8_t cs, const co_id* id) noexcept;

  void OnWait(::std::error_code) noexcept;

  void OnRequest(detail::LssRequestBase& req) noexcept;
  void OnCompletion(detail::LssRequestBase& req) noexcept;

  LssMaster* self{nullptr};

  ev_exec_t* exec{nullptr};
  Node& node;
  io::CanControllerBase* ctrl{nullptr};
  co_nmt_t* nmt{nullptr};
  co_lss_t* lss{nullptr};
  uint16_t inhibit{LELY_CO_LSS_INHIBIT};
  int timeout{LELY_CO_LSS_TIMEOUT};

  co_nmt_lss_req_t* lss_func{nullptr};
  void* lss_data{nullptr};

  sllist queue;

  int bitrate{0};

  co_id id1{4, 0, 0, 0, 0};
  co_id id2{4, 0, 0, 0, 0};
};

namespace detail {

void
LssSwitchRequestBase::OnRequest(void* data) noexcept {
  static_cast<LssMaster::Impl_*>(data)->OnSwitch(*this);
}

void
LssSwitchSelectiveRequestBase::OnRequest(void* data) noexcept {
  static_cast<LssMaster::Impl_*>(data)->OnSwitchSelective(*this);
}

void
LssSetIdRequestBase::OnRequest(void* data) noexcept {
  static_cast<LssMaster::Impl_*>(data)->OnSetId(*this);
}

void
LssSetBitrateRequestBase::OnRequest(void* data) noexcept {
  static_cast<LssMaster::Impl_*>(data)->OnSetBitrate(*this);
}

void
LssSwitchBitrateRequestBase::OnRequest(void* data) noexcept {
  static_cast<LssMaster::Impl_*>(data)->OnSwitchBitrate(*this);
}

void
LssStoreRequestBase::OnRequest(void* data) noexcept {
  static_cast<LssMaster::Impl_*>(data)->OnStore(*this);
}

void
LssGetVendorIdRequestBase::OnRequest(void* data) noexcept {
  static_cast<LssMaster::Impl_*>(data)->OnGetVendorId(*this);
}

void
LssGetProductCodeRequestBase::OnRequest(void* data) noexcept {
  static_cast<LssMaster::Impl_*>(data)->OnGetProductCode(*this);
}

void
LssGetRevisionRequestBase::OnRequest(void* data) noexcept {
  static_cast<LssMaster::Impl_*>(data)->OnGetRevision(*this);
}

void
LssGetSerialNrRequestBase::OnRequest(void* data) noexcept {
  static_cast<LssMaster::Impl_*>(data)->OnGetSerialNr(*this);
}

void
LssGetIdRequestBase::OnRequest(void* data) noexcept {
  static_cast<LssMaster::Impl_*>(data)->OnGetId(*this);
}

void
LssIdNonConfigRequestBase::OnRequest(void* data) noexcept {
  static_cast<LssMaster::Impl_*>(data)->OnIdNonConfig(*this);
}

void
LssSlowscanRequestBase::OnRequest(void* data) noexcept {
  static_cast<LssMaster::Impl_*>(data)->OnSlowscan(*this);
}

void
LssFastscanRequestBase::OnRequest(void* data) noexcept {
  static_cast<LssMaster::Impl_*>(data)->OnFastscan(*this);
}

}  // namespace detail

LssMaster::LssMaster(ev_exec_t* exec, Node& node, io::CanControllerBase* ctrl)
    : impl_(new Impl_(this, exec, node, ctrl, node.nmt())) {}

LssMaster::~LssMaster() = default;

ev::Executor
LssMaster::GetExecutor() const noexcept {
  return impl_->exec;
}

Node&
LssMaster::GetNode() const noexcept {
  return impl_->node;
}

io::CanControllerBase*
LssMaster::GetController() const noexcept {
  return impl_->ctrl;
}

::std::chrono::microseconds
LssMaster::GetInhibit() const {
  ::std::lock_guard<Impl_> lock(*impl_);
  ::std::chrono::microseconds::rep value = impl_->inhibit;
  return ::std::chrono::microseconds(value * 100);
}

void
LssMaster::SetInhibit(const ::std::chrono::microseconds& inhibit) {
  auto value = inhibit.count();
  if (value >= 0) {
    if (value > ::std::numeric_limits<uint16_t>::max() * 100)
      value = ::std::numeric_limits<uint16_t>::max() * 100;

    ::std::lock_guard<Impl_> lock(*impl_);
    // Round the value up to the nearest multiple of 100 us.
    impl_->inhibit = (value + 99) / 100;
    if (impl_->lss) co_lss_set_inhibit(impl_->lss, impl_->inhibit);
  }
}

::std::chrono::milliseconds
LssMaster::GetTimeout() const {
  ::std::lock_guard<Impl_> lock(*impl_);
  return ::std::chrono::milliseconds(impl_->timeout);
}

void
LssMaster::SetTimeout(const ::std::chrono::milliseconds& timeout) {
  auto value = timeout.count();
  if (value >= 0) {
    if (value > ::std::numeric_limits<int>::max())
      value = ::std::numeric_limits<int>::max();

    ::std::lock_guard<Impl_> lock(*impl_);
    impl_->timeout = value;
    if (impl_->lss) co_lss_set_timeout(impl_->lss, impl_->timeout);
  }
}

LssFuture<void>
LssMaster::AsyncSwitch(ev_exec_t* exec, LssState state,
                       detail::LssSwitchRequestBase** preq) {
  LssPromise<void> p;
  auto req =
      make_lss_switch_request(exec, state, [p](::std::error_code ec) mutable {
        p.set(ec ? ::std::make_exception_ptr(
                       ::std::system_error(ec, "AsyncSwitch"))
                 : nullptr);
      });
  if (preq) *preq = req;
  Submit(*req);
  return p.get_future();
}

LssFuture<void>
LssMaster::AsyncSwitchSelective(ev_exec_t* exec, const LssAddress& address,
                                detail::LssSwitchSelectiveRequestBase** preq) {
  LssPromise<void> p;
  auto req = make_lss_switch_selective_request(
      exec, address, [p](::std::error_code ec) mutable {
        p.set(ec ? ::std::make_exception_ptr(
                       ::std::system_error(ec, "AsyncSwitchSelective"))
                 : nullptr);
      });
  if (preq) *preq = req;
  Submit(*req);
  return p.get_future();
}

LssFuture<void>
LssMaster::AsyncSetId(ev_exec_t* exec, uint8_t id,
                      detail::LssSetIdRequestBase** preq) {
  LssPromise<void> p;
  auto req =
      make_lss_set_id_request(exec, id, [p](::std::error_code ec) mutable {
        p.set(ec ? ::std::make_exception_ptr(
                       ::std::system_error(ec, "AsyncSetId"))
                 : nullptr);
      });
  if (preq) *preq = req;
  Submit(*req);
  return p.get_future();
}

LssFuture<void>
LssMaster::AsyncSetBitrate(ev_exec_t* exec, int bitrate,
                           detail::LssSetBitrateRequestBase** preq) {
  LssPromise<void> p;
  auto req = make_lss_set_bitrate_request(
      exec, bitrate, [p](::std::error_code ec) mutable {
        p.set(ec ? ::std::make_exception_ptr(
                       ::std::system_error(ec, "AsyncSetBitrate"))
                 : nullptr);
      });
  if (preq) *preq = req;
  Submit(*req);
  return p.get_future();
}

LssFuture<void>
LssMaster::AsyncStore(ev_exec_t* exec, detail::LssStoreRequestBase** preq) {
  LssPromise<void> p;
  auto req = make_lss_store_request(exec, [p](::std::error_code ec) mutable {
    p.set(ec ? ::std::make_exception_ptr(::std::system_error(ec, "AsyncStore"))
             : nullptr);
  });
  if (preq) *preq = req;
  Submit(*req);
  return p.get_future();
}

LssFuture<uint32_t>
LssMaster::AsyncGetVendorId(ev_exec_t* exec,
                            detail::LssGetVendorIdRequestBase** preq) {
  LssPromise<uint32_t> p;
  auto req = make_lss_get_vendor_id_request(
      exec, [p](::std::error_code ec, uint32_t number) mutable {
        if (ec)
          p.set(util::failure(::std::make_exception_ptr(
              ::std::system_error(ec, "AsyncGetVendorId"))));
        else
          p.set(util::success(number));
      });
  if (preq) *preq = req;
  Submit(*req);
  return p.get_future();
}

LssFuture<uint32_t>
LssMaster::AsyncGetProductCode(ev_exec_t* exec,
                               detail::LssGetProductCodeRequestBase** preq) {
  LssPromise<uint32_t> p;
  auto req = make_lss_get_product_code_request(
      exec, [p](::std::error_code ec, uint32_t number) mutable {
        if (ec)
          p.set(util::failure(::std::make_exception_ptr(
              ::std::system_error(ec, "AsyncGetProductCode"))));
        else
          p.set(util::success(number));
      });
  if (preq) *preq = req;
  Submit(*req);
  return p.get_future();
}

LssFuture<uint32_t>
LssMaster::AsyncGetRevision(ev_exec_t* exec,
                            detail::LssGetRevisionRequestBase** preq) {
  LssPromise<uint32_t> p;
  auto req = make_lss_get_revision_request(
      exec, [p](::std::error_code ec, uint32_t number) mutable {
        if (ec)
          p.set(util::failure(::std::make_exception_ptr(
              ::std::system_error(ec, "AsyncGetRevision"))));
        else
          p.set(util::success(number));
      });
  if (preq) *preq = req;
  Submit(*req);
  return p.get_future();
}

LssFuture<uint32_t>
LssMaster::AsyncGetSerialNr(ev_exec_t* exec,
                            detail::LssGetSerialNrRequestBase** preq) {
  LssPromise<uint32_t> p;
  auto req = make_lss_get_serial_nr_request(
      exec, [p](::std::error_code ec, uint32_t number) mutable {
        if (ec)
          p.set(util::failure(::std::make_exception_ptr(
              ::std::system_error(ec, "AsyncGetSerialNr"))));
        else
          p.set(util::success(number));
      });
  if (preq) *preq = req;
  Submit(*req);
  return p.get_future();
}

LssFuture<uint8_t>
LssMaster::AsyncGetId(ev_exec_t* exec, detail::LssGetIdRequestBase** preq) {
  LssPromise<uint8_t> p;
  auto req = make_lss_get_id_request(
      exec, [p](::std::error_code ec, uint8_t id) mutable {
        if (ec)
          p.set(util::failure(::std::make_exception_ptr(
              ::std::system_error(ec, "AsyncGetId"))));
        else
          p.set(util::success(id));
      });
  if (preq) *preq = req;
  Submit(*req);
  return p.get_future();
}

LssFuture<bool>
LssMaster::AsyncIdNonConfig(ev_exec_t* exec,
                            detail::LssIdNonConfigRequestBase** preq) {
  LssPromise<bool> p;
  auto req = make_lss_id_non_config_request(
      exec, [p](::std::error_code ec, bool found) mutable {
        if (ec)
          p.set(util::failure(::std::make_exception_ptr(
              ::std::system_error(ec, "AsyncIdNonConfig"))));
        else
          p.set(util::success(found));
      });
  if (preq) *preq = req;
  Submit(*req);
  return p.get_future();
}

LssFuture<LssAddress>
LssMaster::AsyncSlowscan(ev_exec_t* exec, const LssAddress& lo,
                         const LssAddress& hi,
                         detail::LssSlowscanRequestBase** preq) {
  LssPromise<LssAddress> p;
  auto req = make_lss_slowscan_request(
      exec, lo, hi,
      [p](::std::error_code ec, const LssAddress& address) mutable {
        if (ec)
          p.set(util::failure(::std::make_exception_ptr(
              ::std::system_error(ec, "AsyncSlowscan"))));
        else
          p.set(util::success(address));
      });
  if (preq) *preq = req;
  Submit(*req);
  return p.get_future();
}

LssFuture<LssAddress>
LssMaster::AsyncFastscan(ev_exec_t* exec, const LssAddress& address,
                         const LssAddress& mask,
                         detail::LssFastscanRequestBase** preq) {
  LssPromise<LssAddress> p;
  auto req = make_lss_fastscan_request(
      exec, address, mask,
      [p](::std::error_code ec, const LssAddress& address) mutable {
        if (ec)
          p.set(util::failure(::std::make_exception_ptr(
              ::std::system_error(ec, "AsyncFastscan"))));
        else
          p.set(util::success(address));
      });
  if (preq) *preq = req;
  Submit(*req);
  return p.get_future();
}

void
LssMaster::Submit(detail::LssRequestBase& req) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->Submit(req);
}

bool
LssMaster::Cancel(detail::LssRequestBase& req) {
  auto ec = ::std::make_error_code(::std::errc::operation_canceled);
  ::std::lock_guard<Impl_> lock(*impl_);
  return impl_->Cancel(&req, ec) != 0;
}

::std::size_t
LssMaster::CancelAll() {
  auto ec = ::std::make_error_code(::std::errc::operation_canceled);
  ::std::lock_guard<Impl_> lock(*impl_);
  return impl_->Cancel(nullptr, ec);
}

bool
LssMaster::Abort(detail::LssRequestBase& req) {
  ::std::lock_guard<Impl_> lock(*impl_);
  return impl_->Abort(&req) != 0;
}

::std::size_t
LssMaster::AbortAll() {
  ::std::lock_guard<Impl_> lock(*impl_);
  return impl_->Abort(nullptr);
}

void
LssMaster::OnSwitchBitrate(
    int bitrate, ::std::chrono::milliseconds delay,
    ::std::function<void(::std::error_code ec)> res) noexcept {
  if (GetController()) {
    // Wait for half a delay period to give the CAN channel time to sent the
    // LSS request.
    GetNode()
        .AsyncWait(GetExecutor(), delay / 2)
        .then(GetExecutor(),
              [this, delay](LssFuture<void> f) {
                // Propagate the exception, if any.
                f.get().value();
                // Stop transmitting CAN frames.
                GetController()->stop();
                // Wait for the second half of the delay period before
                // switching the bitrate.
                return GetNode().AsyncWait(GetExecutor(), delay / 2);
              })
        .then(GetExecutor(),
              [this, bitrate, delay](LssFuture<void> f) {
                // Propagate the exception, if any.
                f.get().value();
                // Activate the new bitrate.
                GetController()->set_bitrate(bitrate);
                // Wait for the delay period before resuming CAN frame
                // transmission.
                return GetNode().AsyncWait(GetExecutor(), delay);
              })
        .then(GetExecutor(), [this, res](LssFuture<void> f) noexcept {
          ::std::error_code ec;
          try {
            // Propagate the exception, if any.
            f.get().value();
            // Resume CAN frame transmission.
            GetController()->restart();
          } catch (::std::system_error& e) {
            ec = e.code();
          }
          // Report the result.
          res(ec);
        });
  } else {
    // Without a CAN controller, the bit rate switch is a no-op.
    res({});
  }
}

void
LssMaster::lock() {
  GetNode().lock();
}

void
LssMaster::unlock() {
  GetNode().unlock();
}

void
LssMaster::SetTime() {
  GetNode().SetTime();
}

LssMaster::Impl_::Impl_(LssMaster* self_, ev_exec_t* exec_, Node& node_,
                        io::CanControllerBase* ctrl_, co_nmt_t* nmt_)
    : self(self_),
      exec(exec_ ? exec_ : static_cast<ev_exec_t*>(node_.GetExecutor())),
      node(node_),
      ctrl(ctrl_),
      nmt(nmt_),
      lss(co_nmt_get_lss(nmt)) {
  co_nmt_get_lss_req(nmt, &lss_func, &lss_data);
  co_nmt_set_lss_req(
      nmt,
      [](co_nmt_t* nmt, co_lss_t* lss, void* data) noexcept {
        static_cast<Impl_*>(data)->OnLssReq(nmt, lss);
      },
      this);

  if (lss) {
    inhibit = co_lss_get_inhibit(lss);
    timeout = co_lss_get_timeout(lss);
  }

  sllist_init(&queue);

  if (ctrl) {
    // Try to obtain the current bitrate. It is not an error if this fails.
    ::std::error_code ec;
    ctrl->get_bitrate(&bitrate, nullptr, ec);
  }
}

LssMaster::Impl_::~Impl_() { co_nmt_set_lss_req(nmt, lss_func, lss_data); }

void
LssMaster::Impl_::OnLssReq(co_nmt_t*, co_lss_t* lss) noexcept {
  assert(lss);

  co_lss_set_inhibit(lss, inhibit);
  co_lss_set_timeout(lss, timeout);
  this->lss = lss;

  // Post a task to execute the LSS requests.
  self->GetExecutor().post([this]() noexcept {
    self->OnStart([this](::std::error_code) noexcept {
      ::std::lock_guard<Impl_> lock(*this);
      // Ignore any errors, since we cannot handle them here.
      co_nmt_lss_con(nmt);
    });
  });
}

void
LssMaster::Impl_::Submit(detail::LssRequestBase& req) {
  if (!req.exec) req.exec = self->GetExecutor();
  auto exec = req.GetExecutor();
  exec.on_task_init();
  // Add the request to the queue and start it if it's the first.
  bool first = sllist_empty(&queue);
  sllist_push_back(&queue, &req._node);
  if (first) OnRequest(req);
}

::std::size_t
LssMaster::Impl_::Cancel(detail::LssRequestBase* req, ::std::error_code ec) {
  sllist queue;
  sllist_init(&queue);

  // Cancel all matching requests, except for the first (ongoing) request.
  if (Pop(req, queue))
    // Stop the ongoing request, if any.
    co_lss_abort_req(lss);

  ::std::size_t n = 0;
  slnode* node;
  while ((node = sllist_pop_front(&queue))) {
    req = static_cast<detail::LssRequestBase*>(ev_task_from_node(node));
    req->ec = ec;

    auto exec = req->GetExecutor();
    exec.post(*req);
    exec.on_task_fini();

    n += n < ::std::numeric_limits<::std::size_t>::max();
  }
  return n;
}

::std::size_t
LssMaster::Impl_::Abort(detail::LssRequestBase* req) {
  sllist queue;
  sllist_init(&queue);

  // Abort all matching requests, except for the first (ongoing) request.
  Pop(req, queue);

  return ev_task_queue_abort(&queue);
}

bool
LssMaster::Impl_::Pop(detail::LssRequestBase* req, sllist& queue) {
  if (!req) {
    // Cancel all pending requests, except for the first (ongoing) request.
    slnode* node = (!lss || co_lss_is_idle(lss))
                       ? nullptr
                       : sllist_pop_front(&this->queue);
    sllist_append(&queue, &this->queue);
    if (node) {
      sllist_push_front(&this->queue, node);
      req = static_cast<detail::LssRequestBase*>(ev_task_from_node(node));
    }
  } else if (&req->_node != sllist_first(&this->queue)) {
    if (sllist_remove(&queue, &req->_node))
      sllist_push_back(&this->queue, &req->_node);
    req = nullptr;
  }
  // Return true if the first request matched (but was not removed).
  return req != nullptr;
}

void
LssMaster::Impl_::OnSwitch(detail::LssSwitchRequestBase& req) noexcept {
  assert(lss);
  assert(&req._node == sllist_first(&queue));

  int errsv = get_errc();
  set_errc(0);
  ::std::error_code ec;
  if (!co_lss_switch_req(lss, static_cast<int>(req.state)))
    req.ec.clear();
  else
    req.ec = util::make_error_code();
  set_errc(errsv);

  OnCompletion(req);
}

void
LssMaster::Impl_::OnSwitchSelective(
    detail::LssSwitchSelectiveRequestBase& req) noexcept {
  assert(lss);
  assert(&req._node == sllist_first(&queue));

  id1 = {4, req.address.vendor_id, req.address.product_code,
         req.address.revision, req.address.serial_nr};

  int errsv = get_errc();
  set_errc(0);

  self->SetTime();

  if (co_lss_switch_sel_req(
          lss, &id1,
          [](co_lss_t* lss, uint8_t cs, void* data) noexcept {
            static_cast<Impl_*>(data)->OnCsInd(lss, cs);
          },
          this) == -1) {
    req.ec = util::make_error_code();
    OnCompletion(req);
  }

  set_errc(errsv);
}

void
LssMaster::Impl_::OnSetId(detail::LssSetIdRequestBase& req) noexcept {
  assert(lss);
  assert(&req._node == sllist_first(&queue));

  int errsv = get_errc();
  set_errc(0);

  self->SetTime();

  if (co_lss_set_id_req(
          lss, req.id,
          [](co_lss_t* lss, uint8_t cs, uint8_t err, uint8_t spec,
             void* data) noexcept {
            static_cast<Impl_*>(data)->OnErrInd(lss, cs, err, spec);
          },
          this) == -1) {
    req.ec = util::make_error_code();
    OnCompletion(req);
  }

  set_errc(errsv);
}

void
LssMaster::Impl_::OnSetBitrate(detail::LssSetBitrateRequestBase& req) noexcept {
  assert(lss);
  assert(&req._node == sllist_first(&queue));

  int errsv = get_errc();
  set_errc(0);

  self->SetTime();

  switch (req.bitrate) {
    case 1000000:
    case 800000:
    case 500000:
    case 250000:
    case 125000:
    case 50000:
    case 20000:
    case 10000:
      if (co_lss_set_rate_req(
              lss, req.bitrate / 1000,
              [](co_lss_t* lss, uint8_t cs, uint8_t err, uint8_t spec,
                 void* data) noexcept {
                static_cast<Impl_*>(data)->OnErrInd(lss, cs, err, spec);
              },
              this) == -1) {
        req.ec = util::make_error_code();
        OnCompletion(req);
      }
      bitrate = req.bitrate;
      break;
    default:
      req.ec = ::std::make_error_code(::std::errc::invalid_argument);
      OnCompletion(req);
      break;
  }

  set_errc(errsv);
}

void
LssMaster::Impl_::OnSwitchBitrate(
    detail::LssSwitchBitrateRequestBase& req) noexcept {
  assert(lss);
  assert(&req._node == sllist_first(&queue));

  int errsv = get_errc();
  set_errc(0);

  self->SetTime();

  if (!bitrate || req.delay < 0 ||
      req.delay > ::std::numeric_limits<uint16_t>::max()) {
    req.ec = ::std::make_error_code(::std::errc::invalid_argument);
    OnCompletion(req);
  } else if (co_lss_switch_rate_req(lss, req.delay) == -1) {
    req.ec = util::make_error_code();
    OnCompletion(req);
  } else if (!ctrl) {
    req.ec.clear();
    OnCompletion(req);
  } else {
    ::std::chrono::milliseconds delay(req.delay);
    // Post a task to perform the actual bit rate switch.
    self->GetExecutor().post([this, delay]() noexcept {
      self->OnSwitchBitrate(bitrate, delay, [this](::std::error_code ec) {
        // Finalize the request.
        ::std::lock_guard<Impl_> lock(*this);
        auto task = ev_task_from_node(sllist_first(&queue));
        assert(task);
        auto req = static_cast<detail::LssSwitchBitrateRequestBase*>(task);
        req->ec = ec;
        OnCompletion(*req);
      });
    });
  }

  set_errc(errsv);
}

void
LssMaster::Impl_::OnStore(detail::LssStoreRequestBase& req) noexcept {
  assert(lss);
  assert(&req._node == sllist_first(&queue));

  int errsv = get_errc();
  set_errc(0);

  self->SetTime();

  if (co_lss_store_req(
          lss,
          [](co_lss_t* lss, uint8_t cs, uint8_t err, uint8_t spec,
             void* data) noexcept {
            static_cast<Impl_*>(data)->OnErrInd(lss, cs, err, spec);
          },
          this) == -1) {
    req.ec = util::make_error_code();
    OnCompletion(req);
  }

  set_errc(errsv);
}

void
LssMaster::Impl_::OnGetVendorId(
    detail::LssGetVendorIdRequestBase& req) noexcept {
  assert(lss);
  assert(&req._node == sllist_first(&queue));

  int errsv = get_errc();
  set_errc(0);

  self->SetTime();

  req.number = 0;
  if (co_lss_get_vendor_id_req(
          lss,
          [](co_lss_t* lss, uint8_t cs, uint32_t id, void* data) noexcept {
            static_cast<Impl_*>(data)->OnLssIdInd(lss, cs, id);
          },
          this) == -1) {
    req.ec = util::make_error_code();
    OnCompletion(req);
  }

  set_errc(errsv);
}

void
LssMaster::Impl_::OnGetProductCode(
    detail::LssGetProductCodeRequestBase& req) noexcept {
  assert(lss);
  assert(&req._node == sllist_first(&queue));

  int errsv = get_errc();
  set_errc(0);

  self->SetTime();

  req.number = 0;
  if (co_lss_get_product_code_req(
          lss,
          [](co_lss_t* lss, uint8_t cs, uint32_t id, void* data) noexcept {
            static_cast<Impl_*>(data)->OnLssIdInd(lss, cs, id);
          },
          this) == -1) {
    req.ec = util::make_error_code();
    OnCompletion(req);
  }

  set_errc(errsv);
}

void
LssMaster::Impl_::OnGetRevision(
    detail::LssGetRevisionRequestBase& req) noexcept {
  assert(lss);
  assert(&req._node == sllist_first(&queue));

  int errsv = get_errc();
  set_errc(0);

  self->SetTime();

  req.number = 0;
  if (co_lss_get_revision_req(
          lss,
          [](co_lss_t* lss, uint8_t cs, uint32_t id, void* data) noexcept {
            static_cast<Impl_*>(data)->OnLssIdInd(lss, cs, id);
          },
          this) == -1) {
    req.ec = util::make_error_code();
    OnCompletion(req);
  }

  set_errc(errsv);
}

void
LssMaster::Impl_::OnGetSerialNr(
    detail::LssGetSerialNrRequestBase& req) noexcept {
  assert(lss);
  assert(&req._node == sllist_first(&queue));

  int errsv = get_errc();
  set_errc(0);

  self->SetTime();

  req.number = 0;
  if (co_lss_get_serial_nr_req(
          lss,
          [](co_lss_t* lss, uint8_t cs, uint32_t id, void* data) noexcept {
            static_cast<Impl_*>(data)->OnLssIdInd(lss, cs, id);
          },
          this) == -1) {
    req.ec = util::make_error_code();
    OnCompletion(req);
  }

  set_errc(errsv);
}

void
LssMaster::Impl_::OnGetId(detail::LssGetIdRequestBase& req) noexcept {
  assert(lss);
  assert(&req._node == sllist_first(&queue));

  int errsv = get_errc();
  set_errc(0);

  self->SetTime();

  req.id = 0;
  if (co_lss_get_id_req(
          lss,
          [](co_lss_t* lss, uint8_t cs, uint8_t id, void* data) noexcept {
            static_cast<Impl_*>(data)->OnNidInd(lss, cs, id);
          },
          this) == -1) {
    req.ec = util::make_error_code();
    OnCompletion(req);
  }

  set_errc(errsv);
}

void
LssMaster::Impl_::OnIdNonConfig(
    detail::LssIdNonConfigRequestBase& req) noexcept {
  assert(lss);
  assert(&req._node == sllist_first(&queue));

  int errsv = get_errc();
  set_errc(0);

  self->SetTime();

  if (co_lss_id_non_cfg_slave_req(
          lss,
          [](co_lss_t* lss, uint8_t cs, void* data) noexcept {
            static_cast<Impl_*>(data)->OnCsInd(lss, cs);
          },
          this) == -1) {
    req.ec = util::make_error_code();
    OnCompletion(req);
  }

  set_errc(errsv);
}

void
LssMaster::Impl_::OnSlowscan(detail::LssSlowscanRequestBase& req) noexcept {
  assert(lss);
  assert(&req._node == sllist_first(&queue));

  id1 = {4, req.lo.vendor_id, req.lo.product_code, req.lo.revision,
         req.lo.serial_nr};
  id2 = {4, req.hi.vendor_id, req.hi.product_code, req.hi.revision,
         req.hi.serial_nr};

  int errsv = get_errc();
  set_errc(0);

  self->SetTime();

  req.address = {0, 0, 0, 0};
  if (co_lss_slowscan_req(
          lss, &id1, &id2,
          [](co_lss_t* lss, uint8_t cs, const co_id* id, void* data) noexcept {
            static_cast<Impl_*>(data)->OnScanInd(lss, cs, id);
          },
          this) == -1) {
    req.ec = util::make_error_code();
    OnCompletion(req);
  }

  set_errc(errsv);
}

void
LssMaster::Impl_::OnFastscan(detail::LssFastscanRequestBase& req) noexcept {
  assert(lss);
  assert(&req._node == sllist_first(&queue));

  id1 = {4, req.address.vendor_id, req.address.product_code,
         req.address.revision, req.address.serial_nr};
  id2 = {4, req.mask.vendor_id, req.mask.product_code, req.mask.revision,
         req.mask.serial_nr};

  int errsv = get_errc();
  set_errc(0);

  self->SetTime();

  if (co_lss_fastscan_req(
          lss, &id1, &id2,
          [](co_lss_t* lss, uint8_t cs, const co_id* id, void* data) noexcept {
            static_cast<Impl_*>(data)->OnScanInd(lss, cs, id);
          },
          this) == -1) {
    req.ec = util::make_error_code();
    OnCompletion(req);
  }

  set_errc(errsv);
}

void
LssMaster::Impl_::OnCsInd(co_lss_t*, uint8_t cs) noexcept {
  auto task = ev_task_from_node(sllist_first(&queue));
  assert(task);
  auto req = static_cast<detail::LssSwitchRequestBase*>(task);

  if (cs)
    req->ec.clear();
  else
    req->ec = ::std::make_error_code(::std::errc::timed_out);

  OnCompletion(*req);
}

void
LssMaster::Impl_::OnErrInd(co_lss_t*, uint8_t cs, uint8_t err,
                           uint8_t) noexcept {
  auto task = ev_task_from_node(sllist_first(&queue));
  assert(task);
  auto req = static_cast<detail::LssRequestBase*>(task);

  req->ec = ::std::make_error_code(::std::errc::protocol_error);
  switch (cs) {
    case 0:
      req->ec = ::std::make_error_code(::std::errc::timed_out);
      break;
    case 0x11:  // 'configure node-ID'
      switch (err) {
        case 0:  // Protocol successfully completed
          req->ec.clear();
          break;
        case 1:  // NID out of range
          req->ec = ::std::make_error_code(::std::errc::result_out_of_range);
          break;
      }
      break;
    case 0x13:  // 'configure bit timing parameters'
      switch (err) {
        case 0:  // Protocol successfully completed
          req->ec.clear();
          break;
        case 1:  // Bit timing / Bit rate not supported
          req->ec = ::std::make_error_code(::std::errc::invalid_argument);
          break;
      }
      break;
    case 0x17:  // 'store configuration'
      switch (err) {
        case 0:  // Protocol successfully completed
          req->ec.clear();
          break;
        case 1:  // Store configuration not supported
          req->ec =
              ::std::make_error_code(::std::errc::operation_not_supported);
          break;
        case 2:  // Storage media access error
          req->ec = ::std::make_error_code(::std::errc::io_error);
          break;
      }
      break;
  }

  OnCompletion(*req);
}

void
LssMaster::Impl_::OnLssIdInd(co_lss_t*, uint8_t cs,
                             co_unsigned32_t id) noexcept {
  auto task = ev_task_from_node(sllist_first(&queue));
  assert(task);
  auto req = static_cast<detail::LssGetNumberRequestBase*>(task);

  if (cs) {
    req->ec.clear();
    req->number = id;
  } else {
    req->ec = ::std::make_error_code(::std::errc::timed_out);
  }

  OnCompletion(*req);
}

void
LssMaster::Impl_::OnNidInd(co_lss_t*, uint8_t cs, uint8_t id) noexcept {
  auto task = ev_task_from_node(sllist_first(&queue));
  assert(task);
  auto req = static_cast<detail::LssGetIdRequestBase*>(task);

  if (cs) {
    req->ec.clear();
    req->id = id;
  } else {
    req->ec = ::std::make_error_code(::std::errc::timed_out);
  }

  OnCompletion(*req);
}

void
LssMaster::Impl_::OnScanInd(co_lss_t*, uint8_t cs, const co_id* id) noexcept {
  assert(!cs || id);
  auto task = ev_task_from_node(sllist_first(&queue));
  assert(task);
  auto req = static_cast<detail::LssScanRequestBase*>(task);

  if (cs) {
    req->ec.clear();
    req->address.vendor_id = id->vendor_id;
    req->address.product_code = id->product_code;
    req->address.revision = id->revision;
    req->address.serial_nr = id->serial_nr;
  } else {
    req->ec = ::std::make_error_code(::std::errc::timed_out);
  }

  OnCompletion(*req);
}

void
LssMaster::Impl_::OnRequest(detail::LssRequestBase& req) noexcept {
  if (lss) {
    req.OnRequest(this);
  } else {
    req.ec = ::std::make_error_code(::std::errc::operation_not_permitted);
    OnCompletion(req);
  }
}

void
LssMaster::Impl_::OnCompletion(detail::LssRequestBase& req) noexcept {
  assert(&req._node == sllist_first(&queue));
  sllist_pop_front(&queue);

  auto exec = req.GetExecutor();
  exec.post(req);
  exec.on_task_fini();

  auto task = ev_task_from_node(sllist_first(&queue));
  if (task) OnRequest(*static_cast<detail::LssRequestBase*>(task));
}

}  // namespace canopen

}  // namespace lely

#endif  // !LELY_NO_CO_LSS
