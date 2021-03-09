/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2020 N7 Space Sp. z o.o.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <CppUTest/TestHarness.h>

#include <lely/can/msg.h>
#include <lely/co/type.h>
#include <lely/util/diag.h>

#include <cassert>

namespace LelyUnitTest {
/**
 * Set empty handlers for all diagnostic messages from lely-core library.
 *
 * @see diag_set_handler(), diag_at_set_handler()
 */
inline void
DisableDiagnosticMessages() {
#if LELY_NO_DIAG
  // enforce coverage in NO_DIAG mode
  diag(DIAG_DEBUG, 0, "Message suppressed");
  diag_if(DIAG_DEBUG, 0, nullptr, "Message suppressed");
#else
  diag_set_handler(nullptr, nullptr);
  diag_at_set_handler(nullptr, nullptr);
#endif
}
}  // namespace LelyUnitTest

struct CoCsdoDnCon {
  static co_csdo_t* sdo;
  static co_unsigned16_t idx;
  static co_unsigned8_t subidx;
  static co_unsigned32_t ac;
  static void* data;
  static unsigned int num_called;

  static inline void
  func(co_csdo_t* sdo_, co_unsigned16_t idx_, co_unsigned8_t subidx_,
       co_unsigned32_t ac_, void* data_) {
    sdo = sdo_;
    idx = idx_;
    subidx = subidx_;
    ac = ac_;
    data = data_;
    num_called++;
  }

  static inline void
  Clear() {
    sdo = nullptr;
    idx = 0;
    subidx = 0;
    ac = 0;
    data = nullptr;

    num_called = 0;
  }

  static inline void
  Check(const co_csdo_t* sdo_, const co_unsigned16_t idx_,
        const co_unsigned8_t subidx_, const co_unsigned32_t ac_,
        const void* data_) {
    POINTERS_EQUAL(sdo_, sdo);
    CHECK_EQUAL(idx_, idx);
    CHECK_EQUAL(subidx_, subidx);
    CHECK_EQUAL(ac_, ac);
    POINTERS_EQUAL(data_, data);
  }

  static inline bool
  called() {
    return num_called > 0;
  }
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

  static inline int
  func(const can_msg* msg_, void* data_) {
    assert(msg_);
    assert(num_called < buf_size);

    msg = *msg_;
    data = data_;

    if (num_called < buf_size) {
      msg_buf[num_called] = *msg_;
    }
    num_called++;

    return ret;
  }

  static inline bool
  called() {
    return num_called > 0;
  }

  static inline void
  Clear() {
    msg = CAN_MSG_INIT;
    data = nullptr;

    ret = 0;
    num_called = 0;

    buf_size = 1u;
    msg_buf = &msg;
  }

  /**
   * Set a message buffer.
   * 
   * @param buf a pointer to a message buffer.
   * @param size the number of frames available at <b>buf</b>.
   */
  static inline void
  SetMsgBuf(can_msg* const buf, const size_t size) {
    buf_size = size;
    msg_buf = buf;
  }
};

#endif  // LELY_UNIT_TEST_HPP_
