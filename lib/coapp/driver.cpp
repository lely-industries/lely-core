/**@file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the remote node driver interface.
 *
 * @see lely/coapp/driver.hpp
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
#include <string>

#include <cassert>

namespace lely {

namespace canopen {

BasicDriver::BasicDriver(ev_exec_t* exec, BasicMaster& master_, uint8_t id)
    : master(master_),
      rpdo_mapped(master.RpdoMapped(id)),
      tpdo_mapped(master.TpdoMapped(id)),
      tpdo_event_mutex(master.tpdo_event_mutex),
      exec_(exec ? exec : static_cast<ev_exec_t*>(master.GetExecutor())),
      id_(id) {
  master.Insert(*this);
}

BasicDriver::~BasicDriver() { master.Erase(*this); }

void
BasicDriver::Insert(LogicalDriverBase& driver) {
  if (driver.Number() < 1 || driver.Number() > 8)
    throw ::std::out_of_range("invalid logical device number: " +
                              ::std::to_string(driver.Number()));
  if (find(driver.id()) != end())
    throw ::std::out_of_range("logical device number " +
                              ::std::to_string(driver.Number()) +
                              " already registered");

  MapType::operator[](driver.Number()) = &driver;
}

void
BasicDriver::Erase(LogicalDriverBase& driver) {
  auto it = find(driver.Number());
  if (it != end() && it->second == &driver) erase(it);
}

SdoFuture<void>
BasicDriver::AsyncConfig(int num) {
  if (num) {
    auto it = find(num);
    if (it != end()) return it->second->AsyncConfig();
  } else if (size() == 1) {
    // Shortcut in case of a single logical device.
    return begin()->second->AsyncConfig();
  } else if (!empty()) {
    ::std::array<SdoFuture<void>, 8> futures;
    ::std::size_t n = 0;
    // Post an OnConfig() task for each logical device driver.
    for (const auto& it : *this) futures[n++] = it.second->AsyncConfig();
    // Create a temporary array of pointers, since SdoFuture is not guaranteed
    // to be the same size as ev_future_t*.
    ::std::array<ev_future_t*, 8> tmp;
    ::std::copy(futures.begin(), futures.end(), tmp.begin());
    // Create a future which becomes ready when all OnConfig() tasks have
    // finished.
    return ev::when_all(GetExecutor(), n, tmp.data())
        // Check the results of the tasks.
        .then(GetExecutor(), [futures](ev::Future<::std::size_t, void>) {
          for (const auto& it : futures) {
            // Throw an exception in an error occurred.
            if (it) it.get().value();
          }
        });
  }
  return make_empty_sdo_future();
}

SdoFuture<void>
BasicDriver::AsyncDeconfig(int num) {
  if (num) {
    auto it = find(num);
    if (it != end()) return it->second->AsyncDeconfig();
  } else if (size() == 1) {
    // Shortcut in case of a single logical device.
    return begin()->second->AsyncDeconfig();
  } else if (!empty()) {
    ::std::array<SdoFuture<void>, 8> futures;
    ::std::size_t n = 0;
    // Post an OnConfig() task for each logical device driver.
    for (const auto& it : *this) futures[n++] = it.second->AsyncDeconfig();
    assert(n <= 8);
    // Create a temporary array of pointers, since SdoFuture is not guaranteed
    // to be the same size as ev_future_t*.
    ::std::array<ev_future_t*, 8> tmp;
    ::std::copy(futures.begin(), futures.end(), tmp.begin());
    // Create a future which becomes ready when all OnDeconfig() tasks have
    // finished.
    return ev::when_all(GetExecutor(), n, tmp.data())
        // Check the results of the tasks.
        .then(GetExecutor(), [futures](ev::Future<::std::size_t, void>) {
          for (const auto& it : futures) {
            // Throw an exception in an error occurred.
            if (it) it.get().value();
          }
        });
  }
  return make_empty_sdo_future();
}

void
BasicDriver::OnCanState(io::CanState new_state,
                        io::CanState old_state) noexcept {
  for (const auto& it : *this) it.second->OnCanState(new_state, old_state);
}

void
BasicDriver::OnCanError(io::CanError error) noexcept {
  for (const auto& it : *this) it.second->OnCanError(error);
}

void
BasicDriver::OnRpdoWrite(uint16_t idx, uint8_t subidx) noexcept {
  if (idx >= 0x6000 && idx <= 0x9fff) {
    int num = (idx - 0x6000) / 0x800 + 1;
    auto it = find(num);
    if (it != end()) it->second->OnRpdoWrite(idx - (num - 1) * 0x800, subidx);
  } else {
    for (const auto& it : *this) it.second->OnRpdoWrite(idx, subidx);
  }
}

void
BasicDriver::OnCommand(NmtCommand cs) noexcept {
  for (const auto& it : *this) it.second->OnCommand(cs);
}

void
BasicDriver::OnHeartbeat(bool occurred) noexcept {
  for (const auto& it : *this) it.second->OnHeartbeat(occurred);
}

void
BasicDriver::OnState(NmtState st) noexcept {
  for (const auto& it : *this) it.second->OnState(st);
}

void
BasicDriver::OnSync(uint8_t cnt, const time_point& t) noexcept {
  for (const auto& it : *this) it.second->OnSync(cnt, t);
}

void
BasicDriver::OnSyncError(uint16_t eec, uint8_t er) noexcept {
  for (const auto& it : *this) it.second->OnSyncError(eec, er);
}

void
BasicDriver::OnTime(
    const ::std::chrono::system_clock::time_point& abs_time) noexcept {
  for (const auto& it : *this) it.second->OnTime(abs_time);
}

void
BasicDriver::OnEmcy(uint16_t eec, uint8_t er, uint8_t msef[5]) noexcept {
  for (const auto& it : *this) it.second->OnEmcy(eec, er, msef);
}

void
BasicDriver::OnNodeGuarding(bool occurred) noexcept {
  for (const auto& it : *this) it.second->OnNodeGuarding(occurred);
}

void
BasicDriver::OnBoot(NmtState st, char es, const ::std::string& what) noexcept {
  for (const auto& it : *this) it.second->OnBoot(st, es, what);
}

void
BasicDriver::OnConfig(
    ::std::function<void(::std::error_code ec)> res) noexcept {
  if (empty()) {
    // Shortcut if no logical device drivers have been registered.
    res(::std::error_code{});
  } else {
    try {
      auto f = AsyncConfig();
      // Invoke res() when AsyncConfig() completes.
      f.submit(GetExecutor(), [res, f] {
        // Extract the error code from the exception pointer, if any.
        ::std::error_code ec;
        auto& result = f.get();
        if (result.has_error()) {
          try {
            ::std::rethrow_exception(result.error());
          } catch (const ::std::system_error& e) {
            ec = e.code();
          } catch (...) {
            // Ignore exceptions we cannot handle.
          }
        }
        res(ec);
      });
    } catch (::std::system_error& e) {
      res(e.code());
    }
  }
}

void
BasicDriver::OnDeconfig(
    ::std::function<void(::std::error_code ec)> res) noexcept {
  if (empty()) {
    // Shortcut if no logical device drivers have been registered.
    res(::std::error_code{});
  } else {
    try {
      auto f = AsyncDeconfig();
      // Invoke res() when AsyncConfig() completes.
      f.submit(GetExecutor(), [res, f] {
        // Extract the error code from the exception pointer, if any.
        ::std::error_code ec;
        auto& result = f.get();
        if (result.has_error()) {
          try {
            ::std::rethrow_exception(result.error());
          } catch (const ::std::system_error& e) {
            ec = e.code();
          } catch (...) {
            // Ignore exceptions we cannot handle.
          }
        }
        res(ec);
      });
    } catch (::std::system_error& e) {
      res(e.code());
    }
  }
}

}  // namespace canopen

}  // namespace lely

#endif  // !LELY_NO_COAPP_MASTER
