/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the CANopen node.
 *
 * @see lely/coapp/node.hpp
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
#include <lely/co/dev.h>
#if !LELY_NO_CO_EMCY
#include <lely/co/emcy.h>
#endif
#if !LELY_NO_CO_LSS
#include <lely/co/lss.h>
#endif
#include <lely/co/nmt.h>
#if !LELY_NO_CO_RPDO
#include <lely/co/rpdo.h>
#endif
#if !LELY_NO_CO_SYNC
#include <lely/co/sync.h>
#endif
#if !LELY_NO_CO_TIME
#include <lely/co/time.h>
#endif
#if !LELY_NO_CO_TPDO
#include <lely/co/tpdo.h>
#if !LELY_NO_CO_MPDO
#include <lely/co/val.h>
#endif
#endif  // !LELY_NO_CO_TPDO
#include <lely/coapp/node.hpp>
#if !LELY_NO_CO_RPDO && !LELY_NO_CO_MPDO
#include <lely/util/endian.h>
#endif

#include <memory>
#include <string>

#include <cassert>

namespace lely {

namespace canopen {

/// The internal implementation of the CANopen node.
struct Node::Impl_ {
  struct NmtDeleter {
    void
    operator()(co_nmt_t* nmt) const noexcept {
      co_nmt_destroy(nmt);
    }
  };

  Impl_(Node* self, can_net_t* net, co_dev_t* dev);

  void OnCsInd(co_nmt_t* nmt, uint8_t cs) noexcept;
  void OnHbInd(co_nmt_t* nmt, uint8_t id, int state, int reason) noexcept;
  void OnStInd(co_nmt_t* nmt, uint8_t id, uint8_t st) noexcept;

#if !LELY_NO_CO_RPDO
  void OnRpdoInd(co_rpdo_t* pdo, uint32_t ac, const void* ptr,
                 size_t n) noexcept;
  void OnRpdoErr(co_rpdo_t* pdo, uint16_t eec, uint8_t er) noexcept;
#endif

#if !LELY_NO_CO_TPDO
  void OnTpdoInd(co_tpdo_t* pdo, uint32_t ac, const void* ptr,
                 size_t n) noexcept;
#endif

#if !LELY_NO_CO_SYNC
  void OnSyncInd(co_nmt_t* nmt, uint8_t cnt) noexcept;
  void OnSyncErr(co_sync_t* sync, uint16_t eec, uint8_t er) noexcept;
#endif

#if !LELY_NO_CO_TIME
  void OnTimeInd(co_time_t* time, const timespec* tp) noexcept;
#endif

#if !LELY_NO_CO_EMCY
  void OnEmcyInd(co_emcy_t* emcy, uint8_t id, uint16_t ec, uint8_t er,
                 uint8_t msef[5]) noexcept;
#endif

#if !LELY_NO_CO_LSS
  void OnRateInd(co_lss_t*, uint16_t rate, int delay) noexcept;
  int OnStoreInd(co_lss_t*, uint8_t id, uint16_t rate) noexcept;
#endif

#if !LELY_NO_CO_RPDO
  void RpdoRtr(int num) noexcept;
#endif

  Node* self{nullptr};

  ::std::function<void(io::CanState, io::CanState)> on_can_state;
  ::std::function<void(io::CanError)> on_can_error;

  ::std::unique_ptr<co_nmt_t, NmtDeleter> nmt;

  ::std::function<void(NmtCommand)> on_command;
  ::std::function<void(uint8_t, bool)> on_heartbeat;
  ::std::function<void(uint8_t, NmtState)> on_state;
#if !LELY_NO_CO_RPDO
  ::std::function<void(int, ::std::error_code, const void*, ::std::size_t)>
      on_rpdo;
  ::std::function<void(int, uint16_t, uint8_t)> on_rpdo_error;
#endif
#if !LELY_NO_CO_TPDO
  ::std::function<void(int, ::std::error_code, const void*, ::std::size_t)>
      on_tpdo;
#endif
#if !LELY_NO_CO_SYNC
  ::std::function<void(uint8_t, const time_point&)> on_sync;
  ::std::function<void(uint16_t, uint8_t)> on_sync_error;
#endif
#if !LELY_NO_CO_TIME
  ::std::function<void(const ::std::chrono::system_clock::time_point&)> on_time;
#endif
#if !LELY_NO_CO_EMCY
  ::std::function<void(uint8_t, uint16_t, uint8_t, uint8_t[5])> on_emcy;
#endif
#if !LELY_NO_CO_LSS
  ::std::function<void(int, ::std::chrono::milliseconds)> on_switch_bitrate;
#endif
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

  if (co_nmt_cs_ind(nmt(), CO_NMT_CS_RESET_NODE) == -1)
    util::throw_errc("Reset");
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
#if LELY_NO_CO_RPDO
  (void)on_rpdo;
#else
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_rpdo = on_rpdo;
#endif
}

void
Node::OnRpdoError(::std::function<void(int, uint16_t, uint8_t)> on_rpdo_error) {
#if LELY_NO_CO_RPDO
  (void)on_rpdo_error;
#else
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_rpdo_error = on_rpdo_error;
#endif
}

void
Node::OnTpdo(
    ::std::function<void(int, ::std::error_code, const void*, ::std::size_t)>
        on_tpdo) {
#if LELY_NO_CO_TPDO
  (void)on_tpdo;
#else
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_tpdo = on_tpdo;
#endif
}

void
Node::OnSync(::std::function<void(uint8_t, const time_point&)> on_sync) {
#if LELY_NO_CO_SYNC
  (void)on_sync;
#else
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_sync = on_sync;
#endif
}

void
Node::OnSyncError(::std::function<void(uint16_t, uint8_t)> on_sync_error) {
#if LELY_NO_CO_SYNC
  (void)on_sync_error;
#else
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_sync_error = on_sync_error;
#endif
}

void
Node::OnTime(
    ::std::function<void(const ::std::chrono::system_clock::time_point&)>
        on_time) {
#if LELY_NO_CO_TIME
  (void)on_time;
#else
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_time = on_time;
#endif
}

void
Node::OnEmcy(
    ::std::function<void(uint8_t, uint16_t, uint8_t, uint8_t[5])> on_emcy) {
#if LELY_NO_CO_EMCY
  (void)on_emcy;
#else
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_emcy = on_emcy;
#endif
}

void
Node::OnSwitchBitrate(
    ::std::function<void(int, ::std::chrono::milliseconds)> on_switch_bitrate) {
#if LELY_NO_CO_LSS
  (void)on_switch_bitrate;
#else
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_switch_bitrate = on_switch_bitrate;
#endif
}

void
Node::TpdoEventMutex::lock() {
#if !LELY_NO_CO_TPDO
  co_nmt_on_tpdo_event_lock(node->nmt());
#endif
}

void
Node::TpdoEventMutex::unlock() {
#if !LELY_NO_CO_TPDO
  co_nmt_on_tpdo_event_unlock(node->nmt());
#endif
}

can_net_t*
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

co_nmt_t*
Node::nmt() const noexcept {
  return impl_->nmt.get();
}

void
Node::Error(uint16_t eec, uint8_t er, const uint8_t msef[5]) noexcept {
  co_nmt_on_err(nmt(), eec, er, msef);
}

void
Node::RpdoRtr(int num) noexcept {
#if LELY_NO_CO_RPDO
  (void)num;
#else
  if (num) {
    impl_->RpdoRtr(num);
  } else {
    for (num = 1; num <= 512; num++) impl_->RpdoRtr(num);
  }
#endif
}

void
Node::TpdoEvent(int num) noexcept {
#if LELY_NO_CO_TPDO
  (void)num;
#else
  co_nmt_on_tpdo_event(nmt(), num);
#endif
}

template <class T>
typename ::std::enable_if<is_canopen_basic<T>::value && sizeof(T) <= 4,
                          void>::type
Node::DamMpdoEvent(int num, uint8_t id, uint16_t idx, uint8_t subidx, T value) {
#if LELY_NO_CO_MPDO
  (void)num;
  (void)id;
  (void)idx;
  (void)subidx;
  (void)value;
#else
  auto pdo = co_nmt_get_tpdo(nmt(), num);
  if (pdo) {
    uint8_t buf[4] = {0};
    co_val_write(canopen_traits<T>::index, &value, buf, buf + 4);
    co_dam_mpdo_event(pdo, id, idx, subidx, buf);
  }
#endif
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// BOOLEAN
template void Node::DamMpdoEvent<bool>(int, uint8_t, uint16_t, uint8_t, bool);

// INTEGER8
template void Node::DamMpdoEvent<int8_t>(int, uint8_t, uint16_t, uint8_t,
                                         int8_t);

// INTEGER16
template void Node::DamMpdoEvent<int16_t>(int, uint8_t, uint16_t, uint8_t,
                                          int16_t);

// INTEGER32
template void Node::DamMpdoEvent<int32_t>(int, uint8_t, uint16_t, uint8_t,
                                          int32_t);

// UNSIGNED8
template void Node::DamMpdoEvent<uint8_t>(int, uint8_t, uint16_t, uint8_t,
                                          uint8_t);

// UNSIGNED16
template void Node::DamMpdoEvent<uint16_t>(int, uint8_t, uint16_t, uint8_t,
                                           uint16_t);

// UNSIGNED32
template void Node::DamMpdoEvent<uint32_t>(int, uint8_t, uint16_t, uint8_t,
                                           uint32_t);

// REAL32
template void Node::DamMpdoEvent<float>(int, uint8_t, uint16_t, uint8_t, float);

// VISIBLE_STRING
// OCTET_STRING
// UNICODE_STRING
// TIME_OF_DAY
// TIME_DIFFERENCE
// DOMAIN
// INTEGER24
// REAL64
// INTEGER40
// INTEGER48
// INTEGER56
// INTEGER64
// UNSIGNED24
// UNSIGNED40
// UNSIGNED48
// UNSIGNED56
// UNSIGNED64

#endif  // !DOXYGEN_SHOULD_SKIP_THIS

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

Node::Impl_::Impl_(Node* self_, can_net_t* net, co_dev_t* dev)
    : self(self_), nmt(co_nmt_create(net, dev)) {
  co_nmt_set_cs_ind(
      nmt.get(),
      [](co_nmt_t* nmt, uint8_t cs, void* data) noexcept {
        static_cast<Impl_*>(data)->OnCsInd(nmt, cs);
      },
      this);

  co_nmt_set_hb_ind(
      nmt.get(),
      [](co_nmt_t* nmt, uint8_t id, int state, int reason,
         void* data) noexcept {
        static_cast<Impl_*>(data)->OnHbInd(nmt, id, state, reason);
      },
      this);

  co_nmt_set_st_ind(
      nmt.get(),
      [](co_nmt_t* nmt, uint8_t id, uint8_t st, void* data) noexcept {
        static_cast<Impl_*>(data)->OnStInd(nmt, id, st);
      },
      this);

#if !LELY_NO_CO_SYNC
  co_nmt_set_sync_ind(
      nmt.get(),
      [](co_nmt_t* nmt, uint8_t cnt, void* data) noexcept {
        static_cast<Impl_*>(data)->OnSyncInd(nmt, cnt);
      },
      this);
#endif
}

void
Node::Impl_::OnCsInd(co_nmt_t* nmt, uint8_t cs) noexcept {
  (void)nmt;

  if (cs == CO_NMT_CS_RESET_COMM) {
#if !LELY_NO_CO_LSS
    auto lss = co_nmt_get_lss(nmt);
    if (lss) {
      co_lss_set_rate_ind(
          lss,
          [](co_lss_t* lss, uint16_t rate, int delay, void* data) noexcept {
            static_cast<Impl_*>(data)->OnRateInd(lss, rate, delay);
          },
          this);

      co_lss_set_store_ind(
          lss,
          [](co_lss_t* lss, uint8_t id, uint16_t rate, void* data) noexcept {
            return static_cast<Impl_*>(data)->OnStoreInd(lss, id, rate);
          },
          this);
    }
#endif
  }

#if !LELY_NO_CO_SYNC
  if (cs == CO_NMT_CS_START || cs == CO_NMT_CS_ENTER_PREOP) {
    auto sync = co_nmt_get_sync(nmt);
    if (sync) {
      co_sync_set_err(
          sync,
          [](co_sync_t* sync, uint16_t eec, uint8_t er, void* data) noexcept {
            static_cast<Impl_*>(data)->OnSyncErr(sync, eec, er);
          },
          this);
    }
#endif

#if !LELY_NO_CO_TIME
    auto time = co_nmt_get_time(nmt);
    if (time) {
      co_time_set_ind(
          time,
          [](co_time_t* time, const timespec* tp, void* data) noexcept {
            static_cast<Impl_*>(data)->OnTimeInd(time, tp);
          },
          this);
    }
#endif

#if !LELY_NO_CO_EMCY
    auto emcy = co_nmt_get_emcy(nmt);
    if (emcy) {
      co_emcy_set_ind(
          emcy,
          [](co_emcy_t* emcy, uint8_t id, uint16_t eec, uint8_t er,
             uint8_t msef[5], void* data) noexcept {
            static_cast<Impl_*>(data)->OnEmcyInd(emcy, id, eec, er, msef);
          },
          this);
    }
  }
#endif

  if (cs == CO_NMT_CS_START) {
#if !LELY_NO_CO_RPDO
    for (int i = 1; i <= 512; i++) {
      auto pdo = co_nmt_get_rpdo(nmt, i);
      if (pdo) {
        co_rpdo_set_ind(
            pdo,
            [](co_rpdo_t* pdo, uint32_t ac, const void* ptr, size_t n,
               void* data) noexcept {
              static_cast<Impl_*>(data)->OnRpdoInd(pdo, ac, ptr, n);
            },
            this);

        co_rpdo_set_err(
            pdo,
            [](co_rpdo_t* pdo, uint16_t eec, uint8_t er, void* data) noexcept {
              static_cast<Impl_*>(data)->OnRpdoErr(pdo, eec, er);
            },
            this);
      }
    }
#endif
#if !LELY_NO_CO_TPDO
    for (int i = 1; i <= 512; i++) {
      auto pdo = co_nmt_get_tpdo(nmt, i);
      if (pdo) {
        co_tpdo_set_ind(
            pdo,
            [](co_tpdo_t* pdo, uint32_t ac, const void* ptr, size_t n,
               void* data) noexcept {
              static_cast<Impl_*>(data)->OnTpdoInd(pdo, ac, ptr, n);
            },
            this);
      }
    }
#endif
  }

  if (cs != CO_NMT_CS_RESET_NODE && cs != CO_NMT_CS_RESET_COMM) {
#if !LELY_NO_CO_RPDO
    self->UpdateRpdoMapping();
#endif
#if !LELY_NO_CO_TPDO
    self->UpdateTpdoMapping();
#endif
  }

  self->OnCommand(static_cast<NmtCommand>(cs));

  if (on_command) {
    auto f = on_command;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(static_cast<NmtCommand>(cs));
  }
}

void
Node::Impl_::OnHbInd(co_nmt_t* nmt, uint8_t id, int state,
                     int reason) noexcept {
  // Invoke the default behavior before notifying the implementation.
  co_nmt_on_hb(nmt, id, state, reason);
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
Node::Impl_::OnStInd(co_nmt_t* nmt, uint8_t id, uint8_t st) noexcept {
  // Invoke the default behavior before notifying the implementation.
  co_nmt_on_st(nmt, id, st);
  // Ignore local state changes.
  if (id == co_dev_get_id(co_nmt_get_dev(nmt))) return;
  // Notify the implementation.
  self->OnState(id, static_cast<NmtState>(st));

  if (on_state) {
    auto f = on_state;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(id, static_cast<NmtState>(st));
  }
}

#if !LELY_NO_CO_RPDO

void
Node::Impl_::OnRpdoInd(co_rpdo_t* pdo, uint32_t ac, const void* ptr,
                       size_t n) noexcept {
#if !LELY_NO_CO_MPDO
  assert(ptr || !n);

  if (!ac) {
    // Check if this is a SAM-MPDO.
    auto par = co_rpdo_get_map_par(pdo);
    assert(par);
    auto buf = static_cast<const uint8_t*>(ptr);
    if (par->n == CO_PDO_MAP_SAM_MPDO && n == CAN_MAX_LEN && !(buf[0] & 0x80))
      self->RpdoWrite(buf[0], ldle_u16(buf + 1), buf[3]);
  }
#endif

  int num = co_rpdo_get_num(pdo);
  self->OnRpdo(num, static_cast<SdoErrc>(ac), ptr, n);

  if (on_rpdo) {
    auto f = on_rpdo;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(num, static_cast<SdoErrc>(ac), ptr, n);
  }
}

void
Node::Impl_::OnRpdoErr(co_rpdo_t* pdo, uint16_t eec, uint8_t er) noexcept {
  int num = co_rpdo_get_num(pdo);
  self->OnRpdoError(num, eec, er);

  if (on_rpdo_error) {
    auto f = on_rpdo_error;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(num, eec, er);
  }
}

#endif  // !LELY_NO_CO_RPDO

#if !LELY_NO_CO_TPDO
void
Node::Impl_::OnTpdoInd(co_tpdo_t* pdo, uint32_t ac, const void* ptr,
                       size_t n) noexcept {
  int num = co_tpdo_get_num(pdo);
  self->OnTpdo(num, static_cast<SdoErrc>(ac), ptr, n);

  if (on_tpdo) {
    auto f = on_tpdo;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(num, static_cast<SdoErrc>(ac), ptr, n);
  }
}
#endif

#if !LELY_NO_CO_SYNC

void
Node::Impl_::OnSyncInd(co_nmt_t*, uint8_t cnt) noexcept {
  auto t = self->GetClock().gettime();
  self->OnSync(cnt, t);

  if (on_sync) {
    auto f = on_sync;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(cnt, t);
  }
}

void
Node::Impl_::OnSyncErr(co_sync_t*, uint16_t eec, uint8_t er) noexcept {
  self->OnSyncError(eec, er);

  if (on_sync_error) {
    auto f = on_sync_error;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(eec, er);
  }
}

#endif  // !LELY_NO_CO_SYNC

#if !LELY_NO_CO_TIME
void
Node::Impl_::OnTimeInd(co_time_t*, const timespec* tp) noexcept {
  assert(tp);
  ::std::chrono::system_clock::time_point abs_time(util::from_timespec(*tp));
  self->OnTime(abs_time);

  if (on_time) {
    auto f = on_time;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(abs_time);
  }
}
#endif

#if !LELY_NO_CO_EMCY
void
Node::Impl_::OnEmcyInd(co_emcy_t*, uint8_t id, uint16_t ec, uint8_t er,
                       uint8_t msef[5]) noexcept {
  self->OnEmcy(id, ec, er, msef);

  if (on_emcy) {
    auto f = on_emcy;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(id, ec, er, msef);
  }
}
#endif

#if !LELY_NO_CO_LSS

void
Node::Impl_::OnRateInd(co_lss_t*, uint16_t rate, int delay) noexcept {
  self->OnSwitchBitrate(rate * 1000, ::std::chrono::milliseconds(delay));

  if (on_switch_bitrate) {
    auto f = on_switch_bitrate;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(rate * 1000, ::std::chrono::milliseconds(delay));
  }
}

int
Node::Impl_::OnStoreInd(co_lss_t*, uint8_t id, uint16_t rate) noexcept {
  try {
    self->OnStore(id, rate * 1000);
    return 0;
  } catch (...) {
    return -1;
  }
}

#endif  // !LELY_NO_CO_LSS

#if !LELY_NO_CO_RPDO
void
Node::Impl_::RpdoRtr(int num) noexcept {
  auto pdo = co_nmt_get_rpdo(nmt.get(), num);
  if (pdo) {
    int errsv = get_errc();
    co_rpdo_rtr(pdo);
    set_errc(errsv);
  }
}
#endif

}  // namespace canopen

}  // namespace lely
