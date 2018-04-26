/*!\file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the remote node driver interface.
 *
 * \see lely/coapp/driver.hpp
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

#include <lely/coapp/driver.hpp>

namespace lely {

namespace canopen {

LELY_COAPP_EXPORT
BasicDriver::BasicDriver(aio::LoopBase& loop, aio::ExecutorBase& exec,
                         BasicMaster& master_, uint8_t id)
    : master(master_), loop_(loop), exec_(exec), id_(id) {
  master.Insert(*this);
}

LELY_COAPP_EXPORT
BasicDriver::~BasicDriver() { master.Erase(*this); }

LELY_COAPP_EXPORT uint8_t
BasicDriver::netid() const noexcept { return master.netid(); }

}  // namespace canopen

}  // namespace lely

#endif  // !LELY_NO_COAPP_MASTER
