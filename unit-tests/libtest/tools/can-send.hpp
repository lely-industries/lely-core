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

#ifndef LELY_UNIT_TEST_CAN_SEND_HPP_
#define LELY_UNIT_TEST_CAN_SEND_HPP_

#include <vector>
#include <functional>

#include <lely/can/msg.h>
#include <lely/can/net.h>
#include <lely/co/type.h>
#include <lely/util/endian.h>

/// #can_send_func_t mock.
class CanSend {
 public:
  using CheckFunc = std::function<can_send_func_t>;
  using MsgSeq = std::vector<can_msg>;

  static int ret;
  static void* user_data;
  static uint_least8_t bus_id;
  static can_msg msg;
  static can_msg* msg_buf;

  static int Func(const can_msg* msg_, uint_least8_t bus_id_, void* data_);
  static void CheckMsg(uint_least32_t id, uint_least8_t flags,
                       uint_least8_t len, const uint_least8_t* data);
  static void CheckMsg(const can_msg& expected_msg);

  static void Clear();
  static void SetCheckSeq(const MsgSeq& msgSeq);

  static inline size_t
  GetNumCalled() {
    return num_called;
  }

  /**
   * Sets a message buffer.
   *
   * @param buf a pointer to a CAN message buffer.
   * @param size the number of frames available at <b>buf</b>.
   */
  static inline void
  SetMsgBuf(can_msg* const buf, const size_t size) {
    buf_size = size;
    msg_buf = buf;
  }

 private:
  static size_t buf_size;
  static size_t num_called;
  static CheckFunc checkFunc;
};
#endif  // LELY_UNIT_TEST_CAN_SEND_HPP_
