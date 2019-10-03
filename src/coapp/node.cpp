/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the CANopen node.
 *
 * @see lely/coapp/node.hpp
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
#include <lely/coapp/node.hpp>

#include <string>

#include <cassert>

#if !LELY_NO_THREADS && defined(__MINGW32__)
#include <lely/libc/threads.h>
#endif
#include <lely/co/dev.hpp>
#include <lely/co/emcy.hpp>
#include <lely/co/nmt.hpp>
#include <lely/co/rpdo.hpp>
#include <lely/co/sync.hpp>
#include <lely/co/time.hpp>
#include <lely/co/tpdo.hpp>

namespace lely {

namespace canopen {

/// The internal implementation of the CANopen node.
struct Node::Impl_ : public util::BasicLockable {
  Impl_(Node* self, CANNet* net, CODev* dev);
#if !LELY_NO_THREADS && defined(__MINGW32__)
  virtual ~Impl_();
#else
  virtual ~Impl_() = default;
#endif

  void lock() override;
  void unlock() override;

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

  void RpdoRtr(int num);
  void TpdoEvent(int num);

  Node* self{nullptr};

#if !LELY_NO_THREADS
#ifdef __MINGW32__
  mtx_t mutex;
#else
  ::std::mutex mutex;
#endif
#endif

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
};

Node::Node(io::TimerBase& timer, io::CanChannelBase& chan,
           const ::std::string& dcf_txt, const ::std::string& dcf_bin,
           uint8_t id)
    : IoContext(timer, chan, this),
      Device(dcf_txt, dcf_bin, id, this),
      impl_(new Impl_(this, IoContext::net(), Device::dev())) {}

Node::~Node() = default;

void
Node::Reset() {
  ::std::lock_guard<Impl_> lock(*impl_);

  // Update the CAN network time before resetting the node. In the case of a
  // master, this ensures that SDO timeouts do not occur too soon.
  SetTime();

  if (impl_->nmt->csInd(CO_NMT_CS_RESET_NODE) == -1) util::throw_errc("Reset");
}

void
Node::ConfigHeartbeat(uint8_t id, const ::std::chrono::milliseconds& ms,
                      ::std::error_code& ec) {
  ::std::lock_guard<Impl_> lock(*impl_);

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
  ::std::lock_guard<Impl_> lock(*impl_);
  impl_->on_command = on_command;
}

void
Node::OnHeartbeat(::std::function<void(uint8_t, bool)> on_heartbeat) {
  ::std::lock_guard<Impl_> lock(*impl_);
  impl_->on_heartbeat = on_heartbeat;
}

void
Node::OnState(::std::function<void(uint8_t, NmtState)> on_state) {
  ::std::lock_guard<Impl_> lock(*impl_);
  impl_->on_state = on_state;
}

void
Node::OnRpdo(
    ::std::function<void(int, ::std::error_code, const void*, ::std::size_t)>
        on_rpdo) {
  ::std::lock_guard<Impl_> lock(*impl_);
  impl_->on_rpdo = on_rpdo;
}

void
Node::OnRpdoError(::std::function<void(int, uint16_t, uint8_t)> on_rpdo_error) {
  ::std::lock_guard<Impl_> lock(*impl_);
  impl_->on_rpdo_error = on_rpdo_error;
}

void
Node::OnTpdo(
    ::std::function<void(int, ::std::error_code, const void*, ::std::size_t)>
        on_tpdo) {
  ::std::lock_guard<Impl_> lock(*impl_);
  impl_->on_tpdo = on_tpdo;
}

void
Node::OnSync(::std::function<void(uint8_t, const time_point&)> on_sync) {
  ::std::lock_guard<Impl_> lock(*impl_);
  impl_->on_sync = on_sync;
}

void
Node::OnSyncError(::std::function<void(uint16_t, uint8_t)> on_sync_error) {
  ::std::lock_guard<Impl_> lock(*impl_);
  impl_->on_sync_error = on_sync_error;
}

void
Node::OnTime(
    ::std::function<void(const ::std::chrono::system_clock::time_point&)>
        on_time) {
  ::std::lock_guard<Impl_> lock(*impl_);
  impl_->on_time = on_time;
}

void
Node::OnEmcy(
    ::std::function<void(uint8_t, uint16_t, uint8_t, uint8_t[5])> on_emcy) {
  ::std::lock_guard<Impl_> lock(*impl_);
  impl_->on_emcy = on_emcy;
}

void
Node::lock() {
  impl_->lock();
}
void
Node::unlock() {
  impl_->unlock();
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
Node::Error(uint16_t eec, uint8_t er, const uint8_t msef[5]) {
  impl_->nmt->onErr(eec, er, msef);
}

void
Node::RpdoRtr(int num) {
  if (num) {
    impl_->RpdoRtr(num);
  } else {
    for (num = 1; num <= 512; num++) impl_->RpdoRtr(num);
  }
}

void
Node::TpdoEvent(int num) {
  if (num) {
    impl_->TpdoEvent(num);
  } else {
    for (num = 1; num <= 512; num++) impl_->TpdoEvent(num);
  }
}

Node::Impl_::Impl_(Node* self_, CANNet* net, CODev* dev)
    : self(self_), nmt(make_unique_c<CONMT>(net, dev)) {
#if !LELY_NO_THREADS
#ifdef __MINGW32__
  if (mtx_init(&mutex, mtx_plain) != thrd_success) util::throw_errc("mtx_init");
#endif
#endif

  nmt->setCsInd<Impl_, &Impl_::OnCsInd>(this);
  nmt->setHbInd<Impl_, &Impl_::OnHbInd>(this);
  nmt->setStInd<Impl_, &Impl_::OnStInd>(this);

  nmt->setSyncInd<Impl_, &Impl_::OnSyncInd>(this);
}

#if !LELY_NO_THREADS && defined(__MINGW32__)
Node::Impl_::~Impl_() {
  // Make sure no callback functions will be invoked after the mutex is
  // destroyed.
  nmt.reset();

  mtx_destroy(&mutex);
}
#endif

void
Node::Impl_::lock() {
#if !LELY_NO_THREADS
#ifdef __MINGW32__
  if (mtx_lock(&mutex) != thrd_success) util::throw_errc("mtx_lock");
#else
  mutex.lock();
#endif
#endif
}

void
Node::Impl_::unlock() {
#if !LELY_NO_THREADS
#ifdef __MINGW32__
  if (mtx_unlock(&mutex) != thrd_success) util::throw_errc("mtx_lock");
#else
  mutex.unlock();
#endif
#endif
}

void
Node::Impl_::OnCsInd(CONMT* nmt, uint8_t cs) noexcept {
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

  self->OnCommand(static_cast<NmtCommand>(cs));

  if (on_command) {
    auto f = on_command;
    util::UnlockGuard<Impl_> unlock(*this);
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
    util::UnlockGuard<Impl_> unlock(*this);
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
    util::UnlockGuard<Impl_> unlock(*this);
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
    util::UnlockGuard<Impl_> unlock(*this);
    f(num, static_cast<SdoErrc>(ac), ptr, n);
  }
}

void
Node::Impl_::OnRpdoErr(CORPDO* pdo, uint16_t eec, uint8_t er) noexcept {
  int num = pdo->getNum();
  self->OnRpdoError(num, eec, er);

  if (on_rpdo_error) {
    auto f = on_rpdo_error;
    util::UnlockGuard<Impl_> unlock(*this);
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
    util::UnlockGuard<Impl_> unlock(*this);
    f(num, static_cast<SdoErrc>(ac), ptr, n);
  }
}

void
Node::Impl_::OnSyncInd(CONMT*, uint8_t cnt) noexcept {
  auto t = Node::time_point::clock::now();
  self->OnSync(cnt, t);

  if (on_sync) {
    auto f = on_sync;
    util::UnlockGuard<Impl_> unlock(*this);
    f(cnt, t);
  }
}

void
Node::Impl_::OnSyncErr(COSync*, uint16_t eec, uint8_t er) noexcept {
  self->OnSyncError(eec, er);

  if (on_sync_error) {
    auto f = on_sync_error;
    util::UnlockGuard<Impl_> unlock(*this);
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
    util::UnlockGuard<Impl_> unlock(*this);
    f(abs_time);
  }
}

void
Node::Impl_::OnEmcyInd(COEmcy*, uint8_t id, uint16_t ec, uint8_t er,
                       uint8_t msef[5]) noexcept {
  self->OnEmcy(id, ec, er, msef);

  if (on_emcy) {
    auto f = on_emcy;
    util::UnlockGuard<Impl_> unlock(*this);
    f(id, ec, er, msef);
  }
}

void
Node::Impl_::RpdoRtr(int num) {
  auto pdo = nmt->getRPDO(num);
  if (pdo && pdo->rtr() == -1) util::throw_errc("RpdoRtr");
}

void
Node::Impl_::TpdoEvent(int num) {
  auto pdo = nmt->getTPDO(num);
  if (pdo && pdo->event() == -1) util::throw_errc("TpdoEvent");
}

}  // namespace canopen

}  // namespace lely
