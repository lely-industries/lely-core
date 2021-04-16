/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2020-2021 N7 Space Sp. z o.o.
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
#ifndef LELY_UNIT_TEST_HPP_
#define LELY_UNIT_TEST_HPP_

#include <lely/can/msg.h>
#include <lely/co/type.h>

namespace LelyUnitTest {
/**
 * Set empty handlers for all diagnostic messages from lely-core library.
 *
 * @see diag_set_handler(), diag_at_set_handler()
 */
void DisableDiagnosticMessages();

/**
 * Checks if a download indication function is set (not null) for
 * a sub-object with the given user-specifed data pointer.
 */
void CheckSubDnIndIsSet(const co_dev_t* dev, co_unsigned16_t idx,
                        const void* data);
/**
 * Checks if sub-object has a default download indication function and
 * user-specified data set.
 */
void CheckSubDnIndIsDefault(const co_dev_t* dev, co_unsigned16_t idx);

}  // namespace LelyUnitTest

struct CoCsdoDnCon {
  static co_csdo_t* sdo;
  static co_unsigned16_t idx;
  static co_unsigned8_t subidx;
  static co_unsigned32_t ac;
  static void* data;
  static unsigned int num_called;

  static void func(co_csdo_t* sdo_, co_unsigned16_t idx_,
                   co_unsigned8_t subidx_, co_unsigned32_t ac_, void* data_);

  // FIXME: should be named "Called()"
  static inline bool
  called() {
    return num_called > 0;
  }

  static void Clear();
  static void Check(const co_csdo_t* sdo_, const co_unsigned16_t idx_,
                    const co_unsigned8_t subidx_, const co_unsigned32_t ac_,
                    const void* data_);
};

struct CanSend {
 private:
  static size_t buf_size;

 public:
  static int ret;
  static void* data;
  static unsigned int num_called;
  static can_msg msg;
  static can_msg* msg_buf;

  static int func(const can_msg* msg_, void* data_);

  // FIXME: should be named "Called()"
  static inline bool
  called() {
    return num_called > 0;
  }

  /**
   * Set a message buffer.
   * 
   * @param buf a pointer to a message buffer.
   * @param size the number of frames available at <b>buf</b>.
   */
  static void
  SetMsgBuf(can_msg* const buf, const size_t size) {
    buf_size = size;
    msg_buf = buf;
  }

  static void Clear();
};

#endif  // LELY_UNIT_TEST_HPP_
