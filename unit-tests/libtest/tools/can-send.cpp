/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2021 N7 Space Sp. z o.o.
 *
 * Unit Test Suite was developed under a programme of,
 * and funded by, the European Space Agency.
 *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cassert>
#include <functional>
#include <vector>

#include <CppUTest/TestHarness.h>

#include "sdo-consts.hpp"

#include "can-send.hpp"

int CanSend::ret = 0;
void* CanSend::user_data = nullptr;
size_t CanSend::num_called = 0;
can_msg CanSend::msg = CAN_MSG_INIT;
int CanSend::bus_id = -1;
can_msg* CanSend::msg_buf = &CanSend::msg;
size_t CanSend::buf_size = 1u;
CanSend::CheckFunc CanSend::checkFunc = nullptr;

int
CanSend::Func(const can_msg* const msg_, const int bus_id_, void* const data_) {
  assert(msg_);

  if (checkFunc != nullptr) checkFunc(msg_, bus_id_, data_);

  msg = *msg_;
  user_data = data_;
  bus_id = bus_id_;

  if ((msg_buf != &msg) && (num_called < buf_size)) {
    msg_buf[num_called] = *msg_;
  }

  num_called++;

  return ret;
}

void
CanSend::CheckMsg(const uint_least32_t id, const uint_least8_t flags,
                  const uint_least8_t len, const uint_least8_t* const data) {
  CHECK_COMPARE(num_called, >, 0);
  CHECK_EQUAL(id, msg.id);
  CHECK_EQUAL(flags, msg.flags);
  CHECK_EQUAL(len, msg.len);
  if (data != nullptr) {
    MEMCMP_EQUAL(data, msg.data, len);
  }
}

void
CanSend::Clear() {
  msg = CAN_MSG_INIT;
  user_data = nullptr;
  bus_id = -1;

  ret = 0;
  num_called = 0;

  buf_size = 1u;
  msg_buf = &msg;

  checkFunc = nullptr;
}

void
CanSend::SetCheckSeq(const MsgSeq& msgSeq) {
  checkFunc = [msgSeq](const can_msg* const sent_msg, int, void*) -> int {
    CHECK_COMPARE(num_called, <, msgSeq.size());
    CHECK_EQUAL(msgSeq[num_called].id, sent_msg->id);
    CHECK_EQUAL(msgSeq[num_called].flags, sent_msg->flags);
    CHECK_EQUAL(msgSeq[num_called].len, sent_msg->len);
    MEMCMP_EQUAL(msgSeq[num_called].data, sent_msg->data,
                 msgSeq[num_called].len);
    return 0;
  };
}
