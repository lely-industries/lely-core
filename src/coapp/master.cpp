/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the CANopen master.
 *
 * @see lely/coapp/master.hpp
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

#if !LELY_NO_COAPP_MASTER

#include <lely/coapp/driver.hpp>

#include <algorithm>
#include <array>
#include <map>
#include <string>

#include <cassert>

#include <lely/co/dev.hpp>
#include <lely/co/nmt.hpp>

namespace lely {

namespace canopen {

/// The internal implementation of the CANopen master.
struct BasicMaster::Impl_ {
  Impl_(BasicMaster* self, CONMT* nmt);

  ev::Future<void> AsyncDeconfig(DriverBase* driver);

  void OnNgInd(CONMT* nmt, uint8_t id, int state, int reason) noexcept;
  void OnBootInd(CONMT* nmt, uint8_t id, uint8_t st, char es) noexcept;
  void OnCfgInd(CONMT* nmt, uint8_t id, COCSDO* sdo) noexcept;

  BasicMaster* self;
  ::std::function<void(uint8_t, bool)> on_node_guarding;
  ::std::function<void(uint8_t, NmtState, char, const ::std::string&)> on_boot;
  ::std::array<bool, CO_NUM_NODES> ready{{false}};
  ::std::map<uint8_t, Sdo> sdos;
};

void
BasicMaster::TpdoEventMutex::lock() {
  ::std::lock_guard<util::BasicLockable> lock(*node);
  Node::TpdoEventMutex::lock();
}

void
BasicMaster::TpdoEventMutex::unlock() {
  ::std::lock_guard<util::BasicLockable> lock(*node);
  Node::TpdoEventMutex::unlock();
}

BasicMaster::BasicMaster(ev_exec_t* exec, io::TimerBase& timer,
                         io::CanChannelBase& chan, const ::std::string& dcf_txt,
                         const ::std::string& dcf_bin, uint8_t id)
    : Node(exec, timer, chan, dcf_txt, dcf_bin, id),
      tpdo_event_mutex(*this),
      impl_(new Impl_(this, Node::nmt())) {}

BasicMaster::~BasicMaster() = default;

bool
BasicMaster::IsReady(uint8_t id) const {
  if (!id || id > CO_NUM_NODES) return false;

  ::std::lock_guard<util::BasicLockable> lock(const_cast<BasicMaster&>(*this));
  return impl_->ready[id - 1];
}

ev::Future<void>
BasicMaster::AsyncDeconfig(uint8_t id) {
  {
    ::std::lock_guard<util::BasicLockable> lock(*this);
    auto it = find(id);
    if (it != end()) return impl_->AsyncDeconfig(it->second);
  }
  return ev::make_empty_future();
}

ev::Future<::std::size_t, void>
BasicMaster::AsyncDeconfig() {
  ::std::array<ev::Future<void>, CO_NUM_NODES> futures;
  ::std::size_t n = 0;
  {
    ::std::lock_guard<util::BasicLockable> lock(*this);
    for (const auto& it : *this) futures[n++] = impl_->AsyncDeconfig(it.second);
  }
  // Create a temporary array of pointers, since ev::Future is not guaranteed to
  // be the same size as ev_future_t*.
  ::std::array<ev_future_t*, CO_NUM_NODES> tmp;
  ::std::copy_n(futures.begin(), n, tmp.begin());
  return ev::when_all(GetExecutor(), n, tmp.data());
}

void
BasicMaster::Error(uint8_t id) {
  ::std::lock_guard<util::BasicLockable> lock(*this);

  if (nmt()->nodeErrInd(id) == -1) util::throw_errc("Error");
}

void
BasicMaster::Error(uint16_t eec, uint8_t er, const uint8_t msef[5]) noexcept {
  ::std::lock_guard<util::BasicLockable> lock(*this);

  Node::Error(eec, er, msef);
}

void
BasicMaster::Command(NmtCommand cs, uint8_t id) {
  ::std::lock_guard<util::BasicLockable> lock(*this);

  if (nmt()->csReq(static_cast<uint8_t>(cs), id) == -1)
    util::throw_errc("Command");
}

void
BasicMaster::RpdoRtr(int num) noexcept {
  ::std::lock_guard<util::BasicLockable> lock(*this);

  Node::RpdoRtr(num);
}

void
BasicMaster::TpdoEvent(int num) noexcept {
  ::std::lock_guard<util::BasicLockable> lock(*this);

  Node::TpdoEvent(num);
}

::std::chrono::milliseconds
BasicMaster::GetTimeout() const {
  ::std::lock_guard<util::BasicLockable> lock(const_cast<BasicMaster&>(*this));

  return detail::from_sdo_timeout(nmt()->getTimeout());
}

void
BasicMaster::SetTimeout(const ::std::chrono::milliseconds& timeout) {
  ::std::lock_guard<util::BasicLockable> lock(*this);

  nmt()->setTimeout(detail::to_sdo_timeout(timeout));
}

void
BasicMaster::Insert(DriverBase& driver) {
  ::std::lock_guard<util::BasicLockable> lock(*this);

  if (!driver.id() || driver.id() > 0x7f)
    throw ::std::out_of_range("invalid node-ID: " +
                              ::std::to_string(driver.id()));
  if (driver.id() == nmt()->getDev()->getId())
    throw ::std::out_of_range("cannot register node-ID of master: " +
                              ::std::to_string(driver.id()));
  if (find(driver.id()) != end())
    throw ::std::out_of_range("node-ID " + ::std::to_string(driver.id()) +
                              " already registered");

  MapType::operator[](driver.id()) = &driver;
}

void
BasicMaster::Erase(DriverBase& driver) {
  ::std::lock_guard<util::BasicLockable> lock(*this);

  auto id = driver.id();
  auto it = find(id);
  if (it != end() && it->second == &driver) {
    CancelSdo(id);
    erase(it);
  }
}

void
BasicMaster::OnNodeGuarding(
    ::std::function<void(uint8_t, bool)> on_node_guarding) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_node_guarding = on_node_guarding;
}

void
BasicMaster::OnBoot(
    ::std::function<void(uint8_t, NmtState, char, const ::std::string&)>
        on_boot) {
  ::std::lock_guard<util::BasicLockable> lock(*this);
  impl_->on_boot = on_boot;
}

void
BasicMaster::IsReady(uint8_t id, bool ready) noexcept {
  if (id && id <= CO_NUM_NODES) impl_->ready[id - 1] = ready;
}

void
BasicMaster::OnCanError(io::CanError error) noexcept {
  for (const auto& it : *this) {
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    it.second->OnCanError(error);
  }
}

void
BasicMaster::OnCanState(io::CanState new_state,
                        io::CanState old_state) noexcept {
  for (const auto& it : *this) {
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    it.second->OnCanState(new_state, old_state);
  }
}

void
BasicMaster::OnRpdoWrite(uint8_t id, uint16_t idx, uint8_t subidx) noexcept {
  auto it = find(id);
  if (it != end()) {
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    it->second->OnRpdoWrite(idx, subidx);
  }
}

void
BasicMaster::OnCommand(NmtCommand cs) noexcept {
  // Abort all ongoing and pending SDO requests unless the master is in the
  // pre-operational or operational state.
  if (cs != NmtCommand::ENTER_PREOP && cs != NmtCommand::START) CancelSdo();
  for (const auto& it : *this) {
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    it.second->OnCommand(cs);
  }
}

void
BasicMaster::OnNodeGuarding(uint8_t id, bool occurred) noexcept {
  auto it = find(id);
  if (it != end()) {
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    it->second->OnNodeGuarding(occurred);
  }
}

void
BasicMaster::OnHeartbeat(uint8_t id, bool occurred) noexcept {
  auto it = find(id);
  if (it != end()) {
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    it->second->OnHeartbeat(occurred);
  }
}

void
BasicMaster::OnState(uint8_t id, NmtState st) noexcept {
  if (st == NmtState::BOOTUP) {
    IsReady(id, false);
    // Abort any ongoing or pending SDO requests for the slave, since the master
    // MAY need the Client-SDO service for the NMT 'boot slave' process.
    CancelSdo(id);
  }
  auto it = find(id);
  if (it != end()) {
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    it->second->OnState(st);
  }
}

void
BasicMaster::OnBoot(uint8_t id, NmtState st, char es,
                    const ::std::string& what) noexcept {
  auto it = find(id);
  if (it != end()) {
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    it->second->OnBoot(st, es, what);
  }
}

void
BasicMaster::OnConfig(uint8_t id) noexcept {
  auto it = find(id);
  // If no remote interface is registered for this node, the 'update
  // configuration' process is considered complete.
  if (it == end()) {
    ConfigResult(id, ::std::error_code());
    return;
  }
  // Let the driver perform the configuration update.
  util::UnlockGuard<util::BasicLockable> unlock(*this);
  it->second->OnConfig([this, id](::std::error_code ec) {
    ::std::lock_guard<util::BasicLockable> lock(*this);
    // Report the result of the 'update configuration' process.
    ConfigResult(id, ec);
  });
}

void
BasicMaster::ConfigResult(uint8_t id, ::std::error_code ec) noexcept {
  assert(nmt()->isBooting(id));
  // Destroy the Client-SDO, since it will be taken over by the master.
  impl_->sdos.erase(id);
  // Ignore any errors, since we cannot handle them here.
  nmt()->cfgRes(id, static_cast<uint32_t>(sdo_errc(ec)));
}

void
BasicMaster::OnSync(uint8_t cnt, const time_point& t) noexcept {
  for (const auto& it : *this) {
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    it.second->OnSync(cnt, t);
  }
}

void
BasicMaster::OnSyncError(uint16_t eec, uint8_t er) noexcept {
  for (const auto& it : *this) {
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    it.second->OnSyncError(eec, er);
  }
}

void
BasicMaster::OnTime(
    const ::std::chrono::system_clock::time_point& abs_time) noexcept {
  for (const auto& it : *this) {
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    it.second->OnTime(abs_time);
  }
}

void
BasicMaster::OnEmcy(uint8_t id, uint16_t eec, uint8_t er,
                    uint8_t msef[5]) noexcept {
  auto it = find(id);
  if (it != end()) {
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    it->second->OnEmcy(eec, er, msef);
  }
}

Sdo*
BasicMaster::GetSdo(uint8_t id) {
  if (!id || id > 0x7f)
    throw ::std::out_of_range("invalid node-ID: " + ::std::to_string(id));
  // The Client-SDO service only exists in the pre-operational and operational
  // state.
  auto st = nmt()->getSt();
  if (st != CO_NMT_ST_PREOP && st != CO_NMT_ST_START) return nullptr;
  // During the 'update configuration' step of the NMT 'boot slave' process, a
  // Client-SDO queue may be available.
  auto it = impl_->sdos.find(id);
  if (it != impl_->sdos.end()) return &it->second;
  // The master needs the Client-SDO service during the NMT 'boot slave'
  // process.
  if (nmt()->isBooting(id)) return nullptr;
  // Return a Client-SDO queue for the default SDO.
  return &(impl_->sdos[id] = Sdo(nmt()->getNet(), id));
}

void
BasicMaster::CancelSdo(uint8_t id) {
  if (id)
    impl_->sdos.erase(id);
  else
    impl_->sdos.clear();
}

void
AsyncMaster::OnCanError(io::CanError error) noexcept {
  for (const auto& it : *this) {
    DriverBase* driver = it.second;
    driver->GetExecutor().post([=]() { driver->OnCanError(error); });
  }
}

void
AsyncMaster::OnCanState(io::CanState new_state,
                        io::CanState old_state) noexcept {
  for (const auto& it : *this) {
    DriverBase* driver = it.second;
    driver->GetExecutor().post(
        [=]() { driver->OnCanState(new_state, old_state); });
  }
}

void
AsyncMaster::OnRpdoWrite(uint8_t id, uint16_t idx, uint8_t subidx) noexcept {
  auto it = find(id);
  if (it != end()) {
    DriverBase* driver = it->second;
    driver->GetExecutor().post([=]() { driver->OnRpdoWrite(idx, subidx); });
  }
}

void
AsyncMaster::OnCommand(NmtCommand cs) noexcept {
  // Abort all ongoing and pending SDO requests unless the master is in the
  // pre-operational or operational state.
  if (cs != NmtCommand::ENTER_PREOP && cs != NmtCommand::START) CancelSdo();
  for (const auto& it : *this) {
    DriverBase* driver = it.second;
    driver->GetExecutor().post([=]() { driver->OnCommand(cs); });
  }
}

void
AsyncMaster::OnNodeGuarding(uint8_t id, bool occurred) noexcept {
  auto it = find(id);
  if (it != end()) {
    DriverBase* driver = it->second;
    driver->GetExecutor().post([=]() { driver->OnNodeGuarding(occurred); });
  }
}

void
AsyncMaster::OnHeartbeat(uint8_t id, bool occurred) noexcept {
  auto it = find(id);
  if (it != end()) {
    DriverBase* driver = it->second;
    driver->GetExecutor().post([=]() { driver->OnHeartbeat(occurred); });
  }
}

void
AsyncMaster::OnState(uint8_t id, NmtState st) noexcept {
  if (st == NmtState::BOOTUP) {
    IsReady(id, false);
    // Abort any ongoing or pending SDO requests for the slave, since the master
    // MAY need the Client-SDO service for the NMT 'boot slave' process.
    CancelSdo(id);
  }
  auto it = find(id);
  if (it != end()) {
    DriverBase* driver = it->second;
    driver->GetExecutor().post([=]() { driver->OnState(st); });
  }
}

void
AsyncMaster::OnBoot(uint8_t id, NmtState st, char es,
                    const ::std::string& what) noexcept {
  auto it = find(id);
  if (it != end()) {
    DriverBase* driver = it->second;
    driver->GetExecutor().post([=]() { driver->OnBoot(st, es, what); });
  }
}

void
AsyncMaster::OnConfig(uint8_t id) noexcept {
  auto it = find(id);
  // If no remote interface is registered for this node, the 'update
  // configuration' process is considered complete.
  if (it == end()) {
    ConfigResult(id, ::std::error_code());
    return;
  }

  // Let the driver perform the configuration update.
  DriverBase* driver = it->second;
  driver->GetExecutor().post([this, id, driver]() {
    driver->OnConfig([this, id](::std::error_code ec) {
      ::std::lock_guard<util::BasicLockable> lock(*this);
      ConfigResult(id, ec);
    });
  });
}

void
AsyncMaster::OnSync(uint8_t cnt, const time_point& t) noexcept {
  for (const auto& it : *this) {
    DriverBase* driver = it.second;
    driver->GetExecutor().post([=]() { driver->OnSync(cnt, t); });
  }
}

void
AsyncMaster::OnSyncError(uint16_t eec, uint8_t er) noexcept {
  for (const auto& it : *this) {
    DriverBase* driver = it.second;
    driver->GetExecutor().post([=]() { driver->OnSyncError(eec, er); });
  }
}

void
AsyncMaster::OnTime(
    const ::std::chrono::system_clock::time_point& abs_time) noexcept {
  for (const auto& it : *this) {
    DriverBase* driver = it.second;
    driver->GetExecutor().post([=]() { driver->OnTime(abs_time); });
  }
}

void
AsyncMaster::OnEmcy(uint8_t id, uint16_t eec, uint8_t er,
                    uint8_t msef[5]) noexcept {
  ::std::array<uint8_t, 5> value = {0};
  ::std::copy_n(msef, value.size(), value.begin());
  auto it = find(id);
  if (it != end()) {
    DriverBase* driver = it->second;
    driver->GetExecutor().post(
        [=]() mutable { driver->OnEmcy(eec, er, value.data()); });
  }
}

BasicMaster::Impl_::Impl_(BasicMaster* self_, CONMT* nmt) : self(self_) {
  nmt->setNgInd<Impl_, &Impl_::OnNgInd>(this);
  nmt->setBootInd<Impl_, &Impl_::OnBootInd>(this);
  nmt->setCfgInd<Impl_, &Impl_::OnCfgInd>(this);
}

ev::Future<void>
BasicMaster::Impl_::AsyncDeconfig(DriverBase* driver) {
  self->IsReady(driver->id(), false);
  ev::Promise<void> p;
  driver->GetExecutor().post([=]() mutable {
    driver->OnDeconfig([p](::std::error_code ec) mutable { p.set(ec); });
  });
  return p.get_future();
}

void
BasicMaster::Impl_::OnNgInd(CONMT* nmt, uint8_t id, int state,
                            int reason) noexcept {
  // Invoke the default behavior before notifying the implementation.
  nmt->onNg(id, state, reason);
  // Only handle node guarding timeout events. State changes are handled by
  // OnSt().
  if (reason != CO_NMT_EC_TIMEOUT) return;
  // Notify the implementation.
  bool occurred = state == CO_NMT_EC_OCCURRED;
  self->OnNodeGuarding(id, occurred);
  if (on_node_guarding) {
    auto f = on_node_guarding;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(id, occurred);
  }
}

void
BasicMaster::Impl_::OnBootInd(CONMT*, uint8_t id, uint8_t st,
                              char es) noexcept {
  if (id && id <= CO_NUM_NODES && (!es || es == 'L')) ready[id - 1] = true;
  ::std::string what = es ? co_nmt_es2str(es) : "";
  self->OnBoot(id, static_cast<NmtState>(st), es, what);
  if (on_boot) {
    auto f = on_boot;
    util::UnlockGuard<util::BasicLockable> unlock(*self);
    f(id, static_cast<NmtState>(st), es, what);
  }
}

void
BasicMaster::Impl_::OnCfgInd(CONMT*, uint8_t id, COCSDO* sdo) noexcept {
  // Create a Client-SDO for the 'update configuration' process.
  try {
    sdos[id] = Sdo(sdo);
  } catch (...) {
    self->ConfigResult(id, SdoErrc::ERROR);
    return;
  }
  self->OnConfig(id);
}

}  // namespace canopen

}  // namespace lely

#endif  // !LELY_NO_COAPP_MASTER
