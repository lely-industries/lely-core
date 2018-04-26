/*!\file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the CANopen master.
 *
 * \see lely/coapp/master.hpp
 *
 * \copyright 2018 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#include <lely/coapp/detail/chrono.hpp>
#include <lely/coapp/detail/mutex.hpp>
#include <lely/coapp/driver.hpp>

#include <cassert>

#include <lely/co/dev.hpp>
#include <lely/co/nmt.hpp>

namespace lely {

namespace canopen {

//! The internal implementation of the CANopen master.
struct BasicMaster::Impl_ {
  Impl_(BasicMaster* self, CONMT* nmt);

  void OnNgInd(CONMT* nmt, uint8_t id, int state, int reason) noexcept;
  void OnBootInd(CONMT* nmt, uint8_t id, uint8_t st, char es) noexcept;
  void OnCfgInd(CONMT* nmt, uint8_t id, COCSDO* sdo) noexcept;

  BasicMaster* self;
};

LELY_COAPP_EXPORT
BasicMaster::BasicMaster(aio::TimerBase& timer, aio::CanBusBase& bus,
    const ::std::string& dcf_txt, const ::std::string& dcf_bin, uint8_t id)
    : Node(timer, bus, dcf_txt, dcf_bin, id),
      impl_(new Impl_(this, Node::nmt())) {}

LELY_COAPP_EXPORT BasicMaster::~BasicMaster() = default;

LELY_COAPP_EXPORT ::std::chrono::milliseconds
BasicMaster::GetTimeout() const {
  ::std::lock_guard<BasicLockable> lock(const_cast<BasicMaster&>(*this));

  return detail::FromTimeout(nmt()->getTimeout());
}

LELY_COAPP_EXPORT void
BasicMaster::SetTimeout(const ::std::chrono::milliseconds& timeout) {
  ::std::lock_guard<BasicLockable> lock(*this);

  nmt()->setTimeout(detail::ToTimeout(timeout));
}

LELY_COAPP_EXPORT void
BasicMaster::Error(uint8_t id) {
  ::std::lock_guard<BasicLockable> lock(*this);

  if (nmt()->nodeErrInd(id) == -1)
    throw_errc("Error");
}

LELY_COAPP_EXPORT void
BasicMaster::Insert(DriverBase& driver) {
  ::std::lock_guard<BasicLockable> lock(*this);

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

LELY_COAPP_EXPORT void
BasicMaster::Erase(DriverBase& driver) {
  ::std::lock_guard<BasicLockable> lock(*this);
  assert(find(driver.id()) != end() && find(driver.id())->second == &driver);

  erase(driver.id());
}

LELY_COAPP_EXPORT void
BasicMaster::OnCanError(CanError error) noexcept {
  for (const auto& it : *this) {
    detail::UnlockGuard<BasicLockable> unlock(*this);
    it.second->OnCanError(error);
  }
}

LELY_COAPP_EXPORT void
BasicMaster::OnCanState(CanState new_state, CanState old_state) noexcept {
  for (const auto& it : *this) {
    detail::UnlockGuard<BasicLockable> unlock(*this);
    it.second->OnCanState(new_state, old_state);
  }
}

LELY_COAPP_EXPORT void
BasicMaster::OnCommand(NmtCommand cs) noexcept {
  // TODO: Abort all ongoing and pending SDO requests unless the master is in
  // the pre-operational or operational state.
  for (const auto& it : *this) {
    detail::UnlockGuard<BasicLockable> unlock(*this);
    it.second->OnCommand(cs);
  }
}

LELY_COAPP_EXPORT void
BasicMaster::OnNodeGuarding(uint8_t id, bool occurred) noexcept {
  auto it = find(id);
  if (it != end()) {
    detail::UnlockGuard<BasicLockable> unlock(*this);
    it->second->OnNodeGuarding(occurred);
  }
}

LELY_COAPP_EXPORT void
BasicMaster::OnHeartbeat(uint8_t id, bool occurred) noexcept {
  auto it = find(id);
  if (it != end()) {
    detail::UnlockGuard<BasicLockable> unlock(*this);
    it->second->OnHeartbeat(occurred);
  }
}

LELY_COAPP_EXPORT void
BasicMaster::OnState(uint8_t id, NmtState st) noexcept {
  // TODO: Abort any ongoing or pending SDO requests for the slave, since the
  // master MAY need the Client-SDO service for the NMT 'boot slave' process.
  auto it = find(id);
  if (it != end()) {
    detail::UnlockGuard<BasicLockable> unlock(*this);
    it->second->OnState(st);
  }
}

LELY_COAPP_EXPORT void
BasicMaster::OnBoot(uint8_t id, NmtState st, char es, const ::std::string& what)
    noexcept {
  auto it = find(id);
  if (it != end()) {
    detail::UnlockGuard<BasicLockable> unlock(*this);
    it->second->OnBoot(st, es, what);
  }
}

LELY_COAPP_EXPORT void
BasicMaster::OnConfig(uint8_t id) noexcept {
  auto it = find(id);
  // If no remote interface is registered for this node, the 'update
  // configuration' process is considered complete.
  if (it == end()) {
    ConfigResult(id, ::std::error_code());
    return;
  }
  // Let the driver perform the configuration update.
  detail::UnlockGuard<BasicLockable> unlock(*this);
  it->second->OnConfig(
      [=](::std::error_code ec) {
        ::std::lock_guard<BasicLockable> lock(*this);
        // Report the result of the 'update configuration' process.
        ConfigResult(id, ec);
      });
}

LELY_COAPP_EXPORT void
BasicMaster::ConfigResult(uint8_t id, ::std::error_code ec) noexcept {
  assert(nmt()->isBooting(id));
  // TODO: Disable Client-SDO.
  // Ignore any errors, since we cannot handle them here.
  nmt()->cfgRes(id, ec.value());
}

LELY_COAPP_EXPORT void
BasicMaster::OnRpdo(int num, ::std::error_code ec, const void* p,
                    ::std::size_t n) noexcept {
  for (const auto& it : *this) {
    detail::UnlockGuard<BasicLockable> unlock(*this);
    it.second->OnRpdo(num, ec, p, n);
  }
}

LELY_COAPP_EXPORT void
BasicMaster::OnRpdoError(int num, uint16_t eec, uint8_t er) noexcept {
  for (const auto& it : *this) {
    detail::UnlockGuard<BasicLockable> unlock(*this);
    it.second->OnRpdoError(num, eec, er);
  }
}

LELY_COAPP_EXPORT void
BasicMaster::OnTpdo(int num, ::std::error_code ec, const void* p,
                    ::std::size_t n) noexcept {
  for (const auto& it : *this) {
    detail::UnlockGuard<BasicLockable> unlock(*this);
    it.second->OnTpdo(num, ec, p, n);
  }
}

LELY_COAPP_EXPORT void
BasicMaster::OnSync(uint8_t cnt) noexcept {
  for (const auto& it : *this) {
    detail::UnlockGuard<BasicLockable> unlock(*this);
    it.second->OnSync(cnt);
  }
}

LELY_COAPP_EXPORT void
BasicMaster::OnSyncError(uint16_t eec, uint8_t er) noexcept {
  for (const auto& it : *this) {
    detail::UnlockGuard<BasicLockable> unlock(*this);
    it.second->OnSyncError(eec, er);
  }
}

LELY_COAPP_EXPORT void
BasicMaster::OnTime(const ::std::chrono::system_clock::time_point& abs_time)
    noexcept {
  for (const auto& it : *this) {
    detail::UnlockGuard<BasicLockable> unlock(*this);
    it.second->OnTime(abs_time);
  }
}

LELY_COAPP_EXPORT void
BasicMaster::OnEmcy(uint8_t id, uint16_t eec, uint8_t er, uint8_t msef[5])
    noexcept {
  auto it = find(id);
  if (it != end()) {
    detail::UnlockGuard<BasicLockable> unlock(*this);
    it->second->OnEmcy(eec, er, msef);
  }
}

BasicMaster::Impl_::Impl_(BasicMaster* self_, CONMT* nmt) : self(self_) {
  nmt->setNgInd<Impl_, &Impl_::OnNgInd>(this);
  nmt->setBootInd<Impl_, &Impl_::OnBootInd>(this);
  nmt->setCfgInd<Impl_, &Impl_::OnCfgInd>(this);
}

void
BasicMaster::Impl_::OnNgInd(CONMT* nmt, uint8_t id, int state, int reason)
    noexcept {
  // Invoke the default behavior before notifying the implementation.
  nmt->onNg(id, state, reason);
  // Only handle node guarding timeout events. State changes are handled by
  // OnSt().
  if (reason != CO_NMT_EC_TIMEOUT)
    return;
  // Notify the implementation.
  self->OnNodeGuarding(id, state == CO_NMT_EC_OCCURRED);
}

void
BasicMaster::Impl_::OnBootInd(CONMT*, uint8_t id, uint8_t st, char es)
    noexcept {
  self->OnBoot(id, static_cast<NmtState>(st), es, es ? co_nmt_es2str(es) : "");
}

void
BasicMaster::Impl_::OnCfgInd(CONMT*, uint8_t id, COCSDO*) noexcept {
  // TODO: Enable Client-SDO for the 'update configuration' process.
  self->OnConfig(id);
}

}  // namespace canopen

}  // namespace lely

#endif  // !LELY_NO_COAPP_MASTER
