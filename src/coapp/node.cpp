/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the CANopen node.
 *
 * @see lely/coapp/node.hpp
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

#include "coapp.hpp"
#include <lely/coapp/node.hpp>

#include <string>

#include <cassert>

#include <lely/co/dev.hpp>
#include <lely/co/emcy.hpp>
#include <lely/co/lss.hpp>
#include <lely/co/nmt.hpp>
#include <lely/co/rpdo.hpp>
#include <lely/co/sync.hpp>
#include <lely/co/time.hpp>
#include <lely/co/tpdo.hpp>

namespace lely {

namespace canopen {

/// The internal implementation of the CANopen node.
struct Node::Impl_ {
  Impl_(Node* self, CANNet* net, CODev* dev);

  void OnCsInd(CONMT* nmt, uint8_t cs) noexcept;
  void OnHbInd(CONMT* nmt, uint8_t id, int state, int reason) noexcept;
  void OnStInd(CONMT* nmt, uint8_t id, uint8_t st) noexcept;

  void OnRpdoInd(CORPDO* pdo, uint32_t ac, const void* ptr, size_t n) noexcept;
  void OnRpdoErr(CORPDO* pdo, uint16_t eec, uint8_t er) noexcept;

  void OnTpdoInd(COTPDO* pdo, uint32_t ac, const void* ptr, size_t n) noexcept;

  void OnSyncInd(CONMT* nmt, uint8_t cnt) noexcept;
  void OnSyncErr(COSync* sync, uint16_t eec, uint8_t er) noexcept;

  void OnTimeInd(COTime* time, const timespec* tp) noexcept;

  void OnEmcyInd(COEmcy* emcy, uint8_t id, uint16_t ec, uint8_t er,
                 uint8_t msef[5]) noexcept;

  void OnRateInd(COLSS*, uint16_t rate, int delay) noexcept;
  int OnStoreInd(COLSS*, uint8_t id, uint16_t rate) noexcept;

  void RpdoRtr(int num) noexcept;

  Node* self{nullptr};

  ::std::function<void(io::CanState, io::CanState)> on_can_state;
  ::std::function<void(io::CanError)> on_can_error;

  unique_c_ptr<CONMT> nmt;

  ::std::function<void(NmtCommand)> on_command;
  ::std::function<void(uint8_t, bool)> on_heartbeat;
  ::std::function<void(uint8_t, NmtState)> on_state;
  ::std::function<void(int, ::std::error_code, const void*, ::std::size_t)>
      on_rpdo;
  ::std::function<void(int, uint16_t, uint8_t)> on_rpdo_error;
  ::std::function<void(int, ::std::error_code, const void*, ::std::size_t)>
      on_tpdo;
  ::std::function<void(uint8_t, const time_point&)> on_sync;
  ::std::function<void(uint16_t, uint8_t)> on_sync_error;
  ::std::function<void(const ::std::chrono::system_clock::time_point&)> on_time;
  ::std::function<void(uint8_t, uint16_t, uint8_t, uint8_t[5])> on_emcy;
  ::std::function<void(int, ::std::chrono::milliseconds)> on_switch_bitrate;
};

Node::Node(ev_exec_t* exec, io::TimerBase& timer, io::CanChannelBase& chan,
           const ::std::string& dcf_txt, const ::std::string& dcf_bin,
           uint8_t id)
    : io::CanNet(exec, timer, chan, 0, 0),
      Device(dcf_txt, dcf_bin, id, this),
      tpdo_event_mutex(*this),
      impl_(new Impl_(this, net(), Device::dev())) {
  // Start processing CAN frames.
  start();
}

Node::~Node() = default;

ev::Executor
Node::GetExecutor() const noexcept {
  return get_executor();
}

io::ContextBase
Node::GetContext() const noexcept {
  return get_ctx();
}

io::Clock
Node::GetClock() const noexcept {
  return get_clock();
}

void
Node::SubmitWait(const time_point& t, io_tqueue_wait& wait) {
  wait.value = util::to_timespec(t);
  io_tqueue_submit_wait(*this, &wait);
}

void
Node::SubmitWait(const duration& d, io_tqueue_wait& wait) {
  SubmitWait(GetClock().gettime() + d, wait);
}

ev::Future<void, ::std::exception_ptr>
Node::AsyncWait(ev_exec_t* exec, const time_point& t, io_tqueue_wait** pwait) {
  if (!exec) exec = GetExecutor();
  auto value = util::to_timespec(t);
  ev::Future<void, int> f{io_tqueue_async_wait(*this, exec, &value, pwait)};
  if (!f) util::throw_errc("AsyncWait");
  return f.then(exec, [](ev::Future<void, int> f) {
    // Convert the error code into an exception pointer.
    int errc = f.get().error();
    if (errc) util::throw_errc("AsyncWait", errc);
  });
}

ev::Future<void, ::std::exception_ptr>
Node::AsyncWait(ev_exec_t* exec, const duration& d, io_tqueue_wait** pwait) {
  return AsyncWait(exec, GetClock().gettime() + d, pwait);
}

bool
Node::CancelWait(io_tqueue_wait& wait) noexcept {
  return io_tqueue_cancel_wait(*this, &wait) != 0;
}

bool
Node::AbortWait(io_tqueue_wait& wait) noexcept {
  return io_tqueue_abort_wait(*this, &wait) != 0;
}

ev::Future<void, ::std::exception_ptr>
Node::AsyncSwitchBitrate(io::CanControllerBase& ctrl, int bitrate,
                         ::std::chrono::milliseconds delay) {
  // Stop transmitting CAN frames.
  ctrl.stop();
  return AsyncWait(GetExecutor(), delay)
      .then(GetExecutor(),
            [this, delay](ev::Future<void, ::std::exception_ptr> f) {
              // Propagate the exception, if any.
              f.get().value();
              // Wait for the delay period before switching the bitrate.
              return AsyncWait(GetExecutor(), delay);
            })
      .then(GetExecutor(),
            [this, &ctrl, bitrate,
             delay](ev::Future<void, ::std::exception_ptr> f) {
              // Propagate the exception, if any.
              f.get().value();
              // Activate the new bitrate.
              ctrl.set_bitrate(bitrate);
              // Wait for the delay period before resuming CAN frame
              // transmission.
              return AsyncWait(GetExecutor(), delay);
            })
      .then(GetExecutor(),
            [this, &ctrl](ev::Future<void, ::std::exception_ptr> f) {
              // Propagate the exception, if any.
              f.get().value();
              // Resume CAN frame transmission.
              ctrl.restart();
            });
}

void
Node::OnCanState(
    ::std::function<void(io::CanState, io::CanState)> on_can_state) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_can_state = on_can_state;
}

void
Node::OnCanError(::std::function<void(io::CanError)> on_can_error) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_can_error = on_can_error;
}

void
Node::Reset() {
  ::std::lock_guard<util::BasicLockable> lock(*this);

  // Update the CAN network time before resetting the node. In the case of a
  // master, this ensures that SDO timeouts do not occur too soon.
  SetTime();

  if (impl_->nmt->csInd(CO_NMT_CS_RESET_NODE) == -1) util::throw_errc("Reset");
}

void
Node::ConfigHeartbeat(uint8_t id, const ::std::chrono::milliseconds& ms,
                      ::std::error_code& ec) {
  ::std::lock_guard<util::BasicLockable> lock(*this);

  auto ac = co_dev_cfg_hb(dev(), id, ms.count());

  if (ac)
    ec = static_cast<SdoErrc>(ac);
  else
    ec.clear();
}

void
Node::ConfigHeartbeat(uint8_t id, const ::std::chrono::milliseconds& ms) {
  ::std::error_code ec;
  ConfigHeartbeat(id, ms, ec);
  if (ec) throw SdoError(Device::id(), 0x1016, 0, ec, "ConfigHeartbeat");
}

void
Node::OnCommand(::std::function<void(NmtCommand)> on_command) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_command = on_command;
}

void
Node::OnHeartbeat(::std::function<void(uint8_t, bool)> on_heartbeat) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_heartbeat = on_heartbeat;
}

void
Node::OnState(::std::function<void(uint8_t, NmtState)> on_state) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_state = on_state;
}

void
Node::OnRpdo(
    ::std::function<void(int, ::std::error_code, const void*, ::std::size_t)>
        on_rpdo) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_rpdo = on_rpdo;
}

void
Node::OnRpdoError(::std::function<void(int, uint16_t, uint8_t)> on_rpdo_error) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_rpdo_error = on_rpdo_error;
}

void
Node::OnTpdo(
    ::std::function<void(int, ::std::error_code, const void*, ::std::size_t)>
        on_tpdo) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_tpdo = on_tpdo;
}

void
Node::OnSync(::std::function<void(uint8_t, const time_point&)> on_sync) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_sync = on_sync;
}

void
Node::OnSyncError(::std::function<void(uint16_t, uint8_t)> on_sync_error) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_sync_error = on_sync_error;
}

void
Node::OnTime(
    ::std::function<void(const ::std::chrono::system_clock::time_point&)>
        on_time) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_time = on_time;
}

void
Node::OnEmcy(
    ::std::function<void(uint8_t, uint16_t, uint8_t, uint8_t[5])> on_emcy) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_emcy = on_emcy;
}

void
Node::OnSwitchBitrate(
    ::std::function<void(int, ::std::chrono::milliseconds)> on_switch_bitrate) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_switch_bitrate = on_switch_bitrate;
}

void
Node::TpdoEventMutex::lock() {
  node->impl_->nmt->onTPDOEventLock();
}

void
Node::TpdoEventMutex::unlock() {
  node->impl_->nmt->onTPDOEventUnlock();
}

CANNet*
Node::net() const noexcept {
  return *this;
}

void
Node::SetTime() {
  set_time();
}

void
Node::OnCanState(io::CanState new_state, io::CanState old_state) noexcept {
  assert(new_state != old_state);

  // TODO(jseldenthuis@lely.com): Clear EMCY in error active mode.
  if (new_state == io::CanState::PASSIVE) {
    // CAN in error passive mode.
    Error(0x8120, 0x10);
  } else if (old_state == io::CanState::BUSOFF) {
    // Recovered from bus off.
    Error(0x8140, 0x10);
  }
}

CONMT*
Node::nmt() const noexcept {
  return impl_->nmt.get();
}

void
Node::Error(uint16_t eec, uint8_t er, const uint8_t msef[5]) noexcept {
  impl_->nmt->onErr(eec, er, msef);
}

void
Node::RpdoRtr(int num) noexcept {
  if (num) {
    impl_->RpdoRtr(num);
  } else {
    for (num = 1; num <= 512; num++) impl_->RpdoRtr(num);
  }
}

void
Node::TpdoEvent(int num) noexcept {
  impl_->nmt->onTPDOEvent(num);
}

void
Node::on_can_state(io::CanState new_state, io::CanState old_state) noexcept {
  OnCanState(new_state, old_state);
  if (impl_->on_can_state) {
    auto f = impl_->on_can_state;
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    f(new_state, old_state);
  }
}

void
Node::on_can_error(io::CanError error) noexcept {
  OnCanError(error);
  if (impl_->on_can_error) {
    auto f = impl_->on_can_error;
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    f(error);
  }
}

void
Node::OnStore(uint8_t, int) {
  util::throw_error_code("OnStore", ::std::errc::operation_not_supported);
}

Node::Impl_::Impl_(Node* self_, CANNet* net, CODev* dev)
    : self(self_), nmt(make_unique_c<CONMT>(net, dev)) {
  nmt->setCsInd<Impl_, &Impl_::OnCsInd>(this);
  nmt->setHbInd<Impl_, &Impl_::OnHbInd>(this);
  nmt->setStInd<Impl_, &Impl_::OnStInd>(this);

  nmt->setSyncInd<Impl_, &Impl_::OnSyncInd>(this);
}

void
Node::Impl_::OnCsInd(CONMT* nmt, uint8_t cs) noexcept {
  if (cs == CO_NMT_CS_RESET_COMM) {
    auto lss = nmt->getLSS();
    if (lss) {
      lss->setRateInd<Impl_, &Impl_::OnRateInd>(this);
      lss->setStoreInd<Impl_, &Impl_::OnStoreInd>(this);
    }
  }

  if (cs == CO_NMT_CS_START || cs == CO_NMT_CS_ENTER_PREOP) {
    auto sync = nmt->getSync();
    if (sync) sync->setErr<Impl_, &Impl_::OnSyncErr>(this);
    auto time = nmt->getTime();
    if (time) time->setInd<Impl_, &Impl_::OnTimeInd>(this);
    auto emcy = nmt->getEmcy();
    if (emcy) emcy->setInd<Impl_, &Impl_::OnEmcyInd>(this);
  }

  if (cs == CO_NMT_CS_START) {
    for (int i = 1; i <= 512; i++) {
      auto pdo = nmt->getRPDO(i);
      if (pdo) {
        pdo->setInd<Impl_, &Impl_::OnRpdoInd>(this);
        pdo->setErr<Impl_, &Impl_::OnRpdoErr>(this);
      }
    }
    for (int i = 1; i <= 512; i++) {
      auto pdo = nmt->getTPDO(i);
      if (pdo) pdo->setInd<Impl_, &Impl_::OnTpdoInd>(this);
    }
  }

  if (cs != CO_NMT_CS_RESET_NODE && cs != CO_NMT_CS_RESET_COMM) {
    self->UpdateRpdoMapping();
    self->UpdateTpdoMapping();
  }

  self->OnCommand(static_cast<NmtCommand>(cs));

  if (on_command) {
    auto f = on_command;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(static_cast<NmtCommand>(cs));
  }
}

void
Node::Impl_::OnHbInd(CONMT* nmt, uint8_t id, int state, int reason) noexcept {
  // Invoke the default behavior before notifying the implementation.
  nmt->onHb(id, state, reason);
  // Only handle heartbeat timeout events. State changes are handled by OnSt().
  if (reason != CO_NMT_EC_TIMEOUT) return;
  // Notify the implementation.
  bool occurred = state == CO_NMT_EC_OCCURRED;
  self->OnHeartbeat(id, occurred);

  if (on_heartbeat) {
    auto f = on_heartbeat;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(id, occurred);
  }
}

void
Node::Impl_::OnStInd(CONMT* nmt, uint8_t id, uint8_t st) noexcept {
  // Invoke the default behavior before notifying the implementation.
  nmt->onSt(id, st);
  // Ignore local state changes.
  if (id == nmt->getDev()->getId()) return;
  // Notify the implementation.
  self->OnState(id, static_cast<NmtState>(st));

  if (on_state) {
    auto f = on_state;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(id, static_cast<NmtState>(st));
  }
}

void
Node::Impl_::OnRpdoInd(CORPDO* pdo, uint32_t ac, const void* ptr,
                       size_t n) noexcept {
  int num = pdo->getNum();
  self->OnRpdo(num, static_cast<SdoErrc>(ac), ptr, n);

  if (on_rpdo) {
    auto f = on_rpdo;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(num, static_cast<SdoErrc>(ac), ptr, n);
  }
}

void
Node::Impl_::OnRpdoErr(CORPDO* pdo, uint16_t eec, uint8_t er) noexcept {
  int num = pdo->getNum();
  self->OnRpdoError(num, eec, er);

  if (on_rpdo_error) {
    auto f = on_rpdo_error;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(num, eec, er);
  }
}

void
Node::Impl_::OnTpdoInd(COTPDO* pdo, uint32_t ac, const void* ptr,
                       size_t n) noexcept {
  int num = pdo->getNum();
  self->OnTpdo(num, static_cast<SdoErrc>(ac), ptr, n);

  if (on_tpdo) {
    auto f = on_tpdo;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(num, static_cast<SdoErrc>(ac), ptr, n);
  }
}

void
Node::Impl_::OnSyncInd(CONMT*, uint8_t cnt) noexcept {
  auto t = self->GetClock().gettime();
  self->OnSync(cnt, t);

  if (on_sync) {
    auto f = on_sync;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(cnt, t);
  }
}

void
Node::Impl_::OnSyncErr(COSync*, uint16_t eec, uint8_t er) noexcept {
  self->OnSyncError(eec, er);

  if (on_sync_error) {
    auto f = on_sync_error;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(eec, er);
  }
}

void
Node::Impl_::OnTimeInd(COTime*, const timespec* tp) noexcept {
  assert(tp);
  ::std::chrono::system_clock::time_point abs_time(util::from_timespec(*tp));
  self->OnTime(abs_time);

  if (on_time) {
    auto f = on_time;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(abs_time);
  }
}

void
Node::Impl_::OnEmcyInd(COEmcy*, uint8_t id, uint16_t ec, uint8_t er,
                       uint8_t msef[5]) noexcept {
  self->OnEmcy(id, ec, er, msef);

  if (on_emcy) {
    auto f = on_emcy;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(id, ec, er, msef);
  }
}

void
Node::Impl_::OnRateInd(COLSS*, uint16_t rate, int delay) noexcept {
  self->OnSwitchBitrate(rate * 1000, ::std::chrono::milliseconds(delay));

  if (on_switch_bitrate) {
    auto f = on_switch_bitrate;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(rate * 1000, ::std::chrono::milliseconds(delay));
  }
}

int
Node::Impl_::OnStoreInd(COLSS*, uint8_t id, uint16_t rate) noexcept {
  try {
    self->OnStore(id, rate * 1000);
    return 0;
  } catch (...) {
    return -1;
  }
}

void
Node::Impl_::RpdoRtr(int num) noexcept {
  auto pdo = nmt->getRPDO(num);
  if (pdo) {
    int errsv = get_errc();
    pdo->rtr();
    set_errc(errsv);
  }
}

}  // namespace canopen

}  // namespace lely
