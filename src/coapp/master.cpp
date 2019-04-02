/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the CANopen master.
 *
 * @see lely/coapp/master.hpp
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

  void OnNgInd(CONMT* nmt, uint8_t id, int state, int reason) noexcept;
  void OnBootInd(CONMT* nmt, uint8_t id, uint8_t st, char es) noexcept;
  void OnCfgInd(CONMT* nmt, uint8_t id, COCSDO* sdo) noexcept;

  BasicMaster* self;
  ::std::map<uint8_t, Sdo> sdos;
};

BasicMaster::BasicMaster(io::TimerBase& timer, io::CanChannelBase& chan,
                         const ::std::string& dcf_txt,
                         const ::std::string& dcf_bin, uint8_t id)
    : Node(timer, chan, dcf_txt, dcf_bin, id),
      impl_(new Impl_(this, Node::nmt())) {}

BasicMaster::~BasicMaster() = default;

void
BasicMaster::Error(uint8_t id) {
  ::std::lock_guard<util::BasicLockable> lock(*this);

  if (nmt()->nodeErrInd(id) == -1) util::throw_errc("Error");
}

void
BasicMaster::Error(uint16_t eec, uint8_t er, const uint8_t msef[5]) {
  ::std::lock_guard<util::BasicLockable> lock(*this);

  Node::Error(eec, er, msef);
}

void
BasicMaster::RpdoRtr(int num) {
  ::std::lock_guard<util::BasicLockable> lock(*this);

  Node::RpdoRtr(num);
}

void
BasicMaster::TpdoEvent(int num) {
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
    erase(id);
  }
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
  // Abort any ongoing or pending SDO requests for the slave, since the master
  // MAY need the Client-SDO service for the NMT 'boot slave' process.
  if (st == NmtState::BOOTUP) CancelSdo(id);
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
  it->second->OnConfig([=](::std::error_code ec) {
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
  nmt()->cfgRes(id, ec.value());
}

void
BasicMaster::OnRpdo(int num, ::std::error_code ec, const void* p,
                    ::std::size_t n) noexcept {
  for (const auto& it : *this) {
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    it.second->OnRpdo(num, ec, p, n);
  }
}

void
BasicMaster::OnRpdoError(int num, uint16_t eec, uint8_t er) noexcept {
  for (const auto& it : *this) {
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    it.second->OnRpdoError(num, eec, er);
  }
}

void
BasicMaster::OnTpdo(int num, ::std::error_code ec, const void* p,
                    ::std::size_t n) noexcept {
  for (const auto& it : *this) {
    util::UnlockGuard<util::BasicLockable> unlock(*this);
    it.second->OnTpdo(num, ec, p, n);
  }
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
  // Abort any ongoing or pending SDO requests for the slave, since the master
  // MAY need the Client-SDO service for the NMT 'boot slave' process.
  if (st == NmtState::BOOTUP) CancelSdo(id);
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
  driver->GetExecutor().post([=]() {
    driver->OnConfig([=](::std::error_code ec) {
      ::std::lock_guard<util::BasicLockable> lock(*this);
      ConfigResult(id, ec);
    });
  });
}

void
AsyncMaster::OnRpdo(int num, ::std::error_code ec, const void* p,
                    ::std::size_t n) noexcept {
  ::std::array<uint8_t, CAN_MAX_LEN> value;
  ::std::copy_n(static_cast<const uint8_t*>(p), ::std::min(n, value.size()),
                value.begin());
  for (const auto& it : *this) {
    DriverBase* driver = it.second;
    driver->GetExecutor().post(
        [=]() { driver->OnRpdo(num, ec, value.data(), value.size()); });
  }
}

void
AsyncMaster::OnRpdoError(int num, uint16_t eec, uint8_t er) noexcept {
  for (const auto& it : *this) {
    DriverBase* driver = it.second;
    driver->GetExecutor().post([=]() { driver->OnRpdoError(num, eec, er); });
  }
}

void
AsyncMaster::OnTpdo(int num, ::std::error_code ec, const void* p,
                    ::std::size_t n) noexcept {
  ::std::array<uint8_t, CAN_MAX_LEN> value;
  ::std::copy_n(static_cast<const uint8_t*>(p), ::std::min(n, value.size()),
                value.begin());
  for (const auto& it : *this) {
    DriverBase* driver = it.second;
    driver->GetExecutor().post(
        [=]() { driver->OnTpdo(num, ec, value.data(), value.size()); });
  }
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

void
BasicMaster::Impl_::OnNgInd(CONMT* nmt, uint8_t id, int state,
                            int reason) noexcept {
  // Invoke the default behavior before notifying the implementation.
  nmt->onNg(id, state, reason);
  // Only handle node guarding timeout events. State changes are handled by
  // OnSt().
  if (reason != CO_NMT_EC_TIMEOUT) return;
  // Notify the implementation.
  self->OnNodeGuarding(id, state == CO_NMT_EC_OCCURRED);
}

void
BasicMaster::Impl_::OnBootInd(CONMT*, uint8_t id, uint8_t st,
                              char es) noexcept {
  self->OnBoot(id, static_cast<NmtState>(st), es, es ? co_nmt_es2str(es) : "");
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
