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
#include <lely/co/dev.h>
#include <lely/co/emcy.h>
#include <lely/co/lss.h>
#include <lely/co/nmt.h>
#include <lely/co/rpdo.h>
#include <lely/co/sync.h>
#include <lely/co/time.h>
#include <lely/co/tpdo.h>
#include <lely/coapp/node.hpp>

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

  void OnRpdoInd(co_rpdo_t* pdo, uint32_t ac, const void* ptr,
                 size_t n) noexcept;
  void OnRpdoErr(co_rpdo_t* pdo, uint16_t eec, uint8_t er) noexcept;

  void OnTpdoInd(co_tpdo_t* pdo, uint32_t ac, const void* ptr,
                 size_t n) noexcept;

  void OnSyncInd(co_nmt_t* nmt, uint8_t cnt) noexcept;
  void OnSyncErr(co_sync_t* sync, uint16_t eec, uint8_t er) noexcept;

  void OnTimeInd(co_time_t* time, const timespec* tp) noexcept;

  void OnEmcyInd(co_emcy_t* emcy, uint8_t id, uint16_t ec, uint8_t er,
                 uint8_t msef[5]) noexcept;

  void OnRateInd(co_lss_t*, uint16_t rate, int delay) noexcept;
  int OnStoreInd(co_lss_t*, uint8_t id, uint16_t rate) noexcept;

  void RpdoRtr(int num) noexcept;

  Node* self{nullptr};

  ::std::function<void(io::CanState, io::CanState)> on_can_state;
  ::std::function<void(io::CanError)> on_can_error;

  ::std::unique_ptr<co_nmt_t, NmtDeleter> nmt;

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
  co_nmt_on_tpdo_event_lock(node->nmt());
}

void
Node::TpdoEventMutex::unlock() {
  co_nmt_on_tpdo_event_unlock(node->nmt());
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
  if (num) {
    impl_->RpdoRtr(num);
  } else {
    for (num = 1; num <= 512; num++) impl_->RpdoRtr(num);
  }
}

void
Node::TpdoEvent(int num) noexcept {
  co_nmt_on_tpdo_event(nmt(), num);
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

  co_nmt_set_sync_ind(
      nmt.get(),
      [](co_nmt_t* nmt, uint8_t cnt, void* data) noexcept {
        static_cast<Impl_*>(data)->OnSyncInd(nmt, cnt);
      },
      this);
}

void
Node::Impl_::OnCsInd(co_nmt_t* nmt, uint8_t cs) noexcept {
  if (cs == CO_NMT_CS_RESET_COMM) {
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
  }

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

    auto time = co_nmt_get_time(nmt);
    if (time) {
      co_time_set_ind(
          time,
          [](co_time_t* time, const timespec* tp, void* data) noexcept {
            static_cast<Impl_*>(data)->OnTimeInd(time, tp);
          },
          this);
    }

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

  if (cs == CO_NMT_CS_START) {
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

void
Node::Impl_::OnRpdoInd(co_rpdo_t* pdo, uint32_t ac, const void* ptr,
                       size_t n) noexcept {
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

void
Node::Impl_::RpdoRtr(int num) noexcept {
  auto pdo = co_nmt_get_rpdo(nmt.get(), num);
  if (pdo) {
    int errsv = get_errc();
    co_rpdo_rtr(pdo);
    set_errc(errsv);
  }
}

}  // namespace canopen

}  // namespace lely
