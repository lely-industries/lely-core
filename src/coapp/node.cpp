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
}

void
Node::Impl_::OnHbInd(CONMT* nmt, uint8_t id, int state, int reason) noexcept {
  // Invoke the default behavior before notifying the implementation.
  nmt->onHb(id, state, reason);
  // Only handle heartbeat timeout events. State changes are handled by OnSt().
  if (reason != CO_NMT_EC_TIMEOUT) return;
  // Notify the implementation.
  self->OnHeartbeat(id, state == CO_NMT_EC_OCCURRED);
}

void
Node::Impl_::OnStInd(CONMT* nmt, uint8_t id, uint8_t st) noexcept {
  // Invoke the default behavior before notifying the implementation.
  nmt->onSt(id, st);
  // Ignore local state changes.
  if (id == nmt->getDev()->getId()) return;
  // Notify the implementation.
  self->OnState(id, static_cast<NmtState>(st));
}

void
Node::Impl_::OnRpdoInd(CORPDO* pdo, uint32_t ac, const void* ptr,
                       size_t n) noexcept {
  int num = pdo->getNum();
  self->OnRpdo(num, static_cast<SdoErrc>(ac), ptr, n);
}

void
Node::Impl_::OnRpdoErr(CORPDO* pdo, uint16_t eec, uint8_t er) noexcept {
  self->OnRpdoError(pdo->getNum(), eec, er);
}

void
Node::Impl_::OnTpdoInd(COTPDO* pdo, uint32_t ac, const void* ptr,
                       size_t n) noexcept {
  self->OnTpdo(pdo->getNum(), static_cast<SdoErrc>(ac), ptr, n);
}

void
Node::Impl_::OnSyncInd(CONMT*, uint8_t cnt) noexcept {
  self->OnSync(cnt, Node::time_point::clock::now());
}

void
Node::Impl_::OnSyncErr(COSync*, uint16_t eec, uint8_t er) noexcept {
  self->OnSyncError(eec, er);
}

void
Node::Impl_::OnTimeInd(COTime*, const timespec* tp) noexcept {
  assert(tp);
  ::std::chrono::system_clock::time_point abs_time(util::from_timespec(*tp));
  self->OnTime(abs_time);
}

void
Node::Impl_::OnEmcyInd(COEmcy*, uint8_t id, uint16_t ec, uint8_t er,
                       uint8_t msef[5]) noexcept {
  self->OnEmcy(id, ec, er, msef);
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
