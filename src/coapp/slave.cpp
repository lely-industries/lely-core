/*!\file
 * This file is part of the C++ CANopen application library; it contains the
 * implementation of the CANopen slave.
 *
 * \see lely/coapp/slave.hpp
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

#if !LELY_NO_COAPP_SLAVE

#include <lely/coapp/slave.hpp>

#include <lely/co/dev.hpp>
#include <lely/co/nmt.hpp>

namespace lely {

namespace canopen {

//! The internal implementation of the CANopen slave.
struct BasicSlave::Impl_ {
  Impl_(BasicSlave* self, CONMT* nmt);

  void OnLgInd(CONMT* nmt, int state) noexcept;

  BasicSlave* self;
};

LELY_COAPP_EXPORT
BasicSlave::BasicSlave(aio::TimerBase& timer, aio::CanBusBase& bus,
    const ::std::string& dcf_txt, const ::std::string& dcf_bin, uint8_t id)
    : Node(timer, bus, dcf_txt, dcf_bin, id),
      impl_(new Impl_(this, Node::nmt())) {}

LELY_COAPP_EXPORT BasicSlave::~BasicSlave() = default;

BasicSlave::Impl_::Impl_(BasicSlave* self_, CONMT* nmt) : self(self_) {
  nmt->setLgInd<Impl_, &Impl_::OnLgInd>(this);
}

void
BasicSlave::Impl_::OnLgInd(CONMT* nmt, int state) noexcept {
  // Invoke the default behavior before notifying the implementation.
  nmt->onLg(state);
  // Notify the implementation.
  self->OnLifeGuarding(state == CO_NMT_EC_OCCURRED);
}

}  // namespace canopen

}  // namespace lely

#endif  // !LELY_NO_COAPP_SLAVE
