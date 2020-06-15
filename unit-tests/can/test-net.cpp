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

#include <CppUTest/TestHarness.h>

#include <lely/can/net.h>

TEST_GROUP(CAN_NetInit){};

TEST(CAN_NetInit, CanNetAllocFree) {
  void* const ptr = __can_net_alloc();

  CHECK(ptr != nullptr);

  __can_net_free(ptr);
}

TEST(CAN_NetInit, CanNetInit) {
  auto* const net = static_cast<can_net_t*>(__can_net_alloc());

  CHECK(net != nullptr);
  POINTERS_EQUAL(net, __can_net_init(net));

  timespec tp;
  can_net_get_time(net, &tp);
  CHECK_EQUAL(0, tp.tv_sec);
  CHECK_EQUAL(0L, tp.tv_nsec);

  can_timer_func_t* tfunc = nullptr;
  void* tdata = nullptr;
  can_net_get_next_func(net, &tfunc, &tdata);
  POINTERS_EQUAL(nullptr, tfunc);
  POINTERS_EQUAL(nullptr, tdata);

  can_send_func_t* sfunc = nullptr;
  void* sdata = nullptr;
  can_net_get_send_func(net, &sfunc, &sdata);
  POINTERS_EQUAL(nullptr, sfunc);
  POINTERS_EQUAL(nullptr, sdata);

  __can_net_fini(net);
  __can_net_free(net);
}

TEST(CAN_NetInit, CanNetFini) {
  auto* const net = static_cast<can_net_t*>(__can_net_alloc());

  CHECK(net != nullptr);
  POINTERS_EQUAL(net, __can_net_init(net));

  can_timer_t* const timer1 = can_timer_create();
  const timespec time1 = {0, 0L};
  can_timer_start(timer1, net, &time1, nullptr);
  can_recv_t* const recv1 = can_recv_create();
  can_recv_t* const recv2 = can_recv_create();
  can_recv_t* const recv3 = can_recv_create();
  can_recv_start(recv1, net, 0x0, CAN_FLAG_IDE);
  can_recv_start(recv2, net, 0x01, 0);
  can_recv_start(recv3, net, 0x01, 0);

  __can_net_fini(net);
  __can_net_free(net);

  can_timer_destroy(timer1);
  can_recv_destroy(recv1);
  can_recv_destroy(recv2);
  can_recv_destroy(recv3);
}

TEST(CAN_NetInit, CanNetDestroy_Null) { can_net_destroy(nullptr); }

TEST(CAN_NetInit, CanTimerAllocFree) {
  void* const ptr = __can_timer_alloc();

  CHECK(ptr != nullptr);

  __can_timer_free(ptr);
}

TEST(CAN_NetInit, CanTimerInitFinit) {
  auto* const timer = static_cast<can_timer_t*>(__can_timer_alloc());

  CHECK(timer != nullptr);
  POINTERS_EQUAL(timer, __can_timer_init(timer));

  can_timer_func_t* tfunc = nullptr;
  void* tdata = nullptr;
  can_timer_get_func(timer, &tfunc, &tdata);
  POINTERS_EQUAL(nullptr, tfunc);
  POINTERS_EQUAL(nullptr, tdata);

  __can_timer_fini(timer);
  __can_timer_free(timer);
}

TEST(CAN_NetInit, CanTimerDestroy_Null) { can_timer_destroy(nullptr); }

TEST(CAN_NetInit, CanRecvAllocFree) {
  void* const ptr = __can_recv_alloc();

  CHECK(ptr != nullptr);

  __can_recv_free(ptr);
}

TEST(CAN_NetInit, CanRecvInitFinit) {
  auto* const recv = static_cast<can_recv_t*>(__can_recv_alloc());

  CHECK(recv != nullptr);
  POINTERS_EQUAL(recv, __can_recv_init(recv));

  can_recv_func_t* tfunc = nullptr;
  void* tdata = nullptr;
  can_recv_get_func(recv, &tfunc, &tdata);
  POINTERS_EQUAL(nullptr, tfunc);
  POINTERS_EQUAL(nullptr, tdata);

  __can_recv_fini(recv);
  __can_recv_free(recv);
}

TEST(CAN_NetInit, CanRecvDestroy_Null) { can_recv_destroy(nullptr); }

namespace CAN_Net_Static {
static unsigned int tfunc_empty_counter = 0;
static unsigned int tfunc_err_counter = 0;
}  // namespace CAN_Net_Static

TEST_GROUP(CAN_Net) {
  can_net_t* net = nullptr;

  static int timer_func_empty(const timespec*, void*) {
    ++CAN_Net_Static::tfunc_empty_counter;
    return 0;
  }
  static int timer_func_err(const timespec*, void*) {
    ++CAN_Net_Static::tfunc_err_counter;
    return -1;
  }

  static int recv_func_empty(const can_msg*, void*) { return 0; }
  static int recv_func_err(const can_msg*, void*) { return -1; }

  static int send_func_empty(const can_msg*, void*) { return 0; }
  static int send_func_err(const can_msg*, void*) { return -1; }

  TEST_SETUP() {
    net = can_net_create();
    CHECK(net != nullptr);

    CAN_Net_Static::tfunc_empty_counter = 0;
    CAN_Net_Static::tfunc_err_counter = 0;
  }

  TEST_TEARDOWN() { can_net_destroy(net); }
};

TEST(CAN_Net, CanNetGetTime_Null) { can_net_get_time(net, nullptr); }

TEST(CAN_Net, CanNetSetTime_NoTimers) {
  const timespec tp = {256, 640000L};

  const auto ret = can_net_set_time(net, &tp);

  timespec out_tp = {0, 0L};
  can_net_get_time(net, &out_tp);
  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(tp.tv_sec, out_tp.tv_sec);
  CHECK_EQUAL(tp.tv_nsec, out_tp.tv_nsec);
}

TEST(CAN_Net, CanNetSetTime_NoCalls) {
  const timespec tp = {4, 0L};
  const timespec tstart = {5, 0L};
  can_timer_t* const timer = can_timer_create();
  can_timer_set_func(timer, timer_func_empty, nullptr);
  can_timer_start(timer, net, &tstart, nullptr);

  const auto ret = can_net_set_time(net, &tp);

  timespec out_tp = {0, 0L};
  can_net_get_time(net, &out_tp);
  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(tp.tv_sec, out_tp.tv_sec);
  CHECK_EQUAL(tp.tv_nsec, out_tp.tv_nsec);
  CHECK_EQUAL(0, CAN_Net_Static::tfunc_empty_counter);

  can_timer_destroy(timer);
}

TEST(CAN_Net, CanNetSetTime_OneCall) {
  const timespec tp = {5, 30L};
  const timespec tstart = {5, 0L};
  can_timer_t* const timer = can_timer_create();
  can_timer_set_func(timer, timer_func_empty, nullptr);
  can_timer_start(timer, net, &tstart, nullptr);

  const auto ret = can_net_set_time(net, &tp);

  timespec out_tp = {0, 0L};
  can_net_get_time(net, &out_tp);
  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(tp.tv_sec, out_tp.tv_sec);
  CHECK_EQUAL(tp.tv_nsec, out_tp.tv_nsec);
  CHECK_EQUAL(1, CAN_Net_Static::tfunc_empty_counter);

  can_timer_destroy(timer);
}

TEST(CAN_Net, CanNetSetTime_OneCallErr) {
  const timespec tp = {5, 30L};
  const timespec tstart = {5, 0L};
  can_timer_t* const timer = can_timer_create();
  can_timer_set_func(timer, timer_func_err, nullptr);
  can_timer_start(timer, net, &tstart, nullptr);

  const auto ret = can_net_set_time(net, &tp);

  timespec out_tp = {0, 0L};
  can_net_get_time(net, &out_tp);
  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(tp.tv_sec, out_tp.tv_sec);
  CHECK_EQUAL(tp.tv_nsec, out_tp.tv_nsec);
  CHECK_EQUAL(1, CAN_Net_Static::tfunc_err_counter);

  can_timer_destroy(timer);
}

TEST(CAN_Net, CanNetSetTime_OneCallNoFunc) {
  const timespec tp = {5, 30L};
  const timespec tstart = {5, 0L};
  can_timer_t* const timer = can_timer_create();
  can_timer_start(timer, net, &tstart, nullptr);

  const auto ret = can_net_set_time(net, &tp);

  timespec out_tp = {0, 0L};
  can_net_get_time(net, &out_tp);
  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(tp.tv_sec, out_tp.tv_sec);
  CHECK_EQUAL(tp.tv_nsec, out_tp.tv_nsec);

  can_timer_destroy(timer);
}

TEST(CAN_Net, CanNetSetTime_IntervalSec) {
  const timespec tp = {5, 30L};
  const timespec tstart = {5, 0L};
  const timespec interval = {1, 0L};
  can_timer_t* const timer = can_timer_create();
  can_timer_set_func(timer, timer_func_empty, nullptr);
  can_timer_start(timer, net, &tstart, &interval);

  const auto ret = can_net_set_time(net, &tp);

  timespec out_tp = {0, 0L};
  can_net_get_time(net, &out_tp);
  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(tp.tv_sec, out_tp.tv_sec);
  CHECK_EQUAL(tp.tv_nsec, out_tp.tv_nsec);
  CHECK_EQUAL(1, CAN_Net_Static::tfunc_empty_counter);

  can_timer_destroy(timer);
}

TEST(CAN_Net, CanNetSetTime_IntervalNSec) {
  const timespec tp = {5, 30L};
  const timespec tstart = {5, 0L};
  const timespec interval = {0, 40L};
  can_timer_t* const timer = can_timer_create();
  can_timer_set_func(timer, timer_func_empty, nullptr);
  can_timer_start(timer, net, &tstart, &interval);

  const auto ret = can_net_set_time(net, &tp);

  timespec out_tp = {0, 0L};
  can_net_get_time(net, &out_tp);
  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(tp.tv_sec, out_tp.tv_sec);
  CHECK_EQUAL(tp.tv_nsec, out_tp.tv_nsec);
  CHECK_EQUAL(1, CAN_Net_Static::tfunc_empty_counter);

  can_timer_destroy(timer);
}

TEST(CAN_Net, CanNetSetTime_OnlyInterval) {
  const timespec tp = {5, 30L};
  const timespec interval = {1, 0L};
  can_timer_t* const timer = can_timer_create();
  can_timer_set_func(timer, timer_func_empty, nullptr);
  can_timer_start(timer, net, nullptr, &interval);

  const auto ret = can_net_set_time(net, &tp);

  timespec out_tp = {0, 0L};
  can_net_get_time(net, &out_tp);
  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(tp.tv_sec, out_tp.tv_sec);
  CHECK_EQUAL(tp.tv_nsec, out_tp.tv_nsec);
  CHECK_EQUAL(5, CAN_Net_Static::tfunc_empty_counter);

  can_timer_destroy(timer);
}

TEST(CAN_Net, CanNetSetTime_IntervalMultipleCalls) {
  const timespec tp = {30, 0L};
  const timespec tstart = {5, 0L};
  const timespec interval = {1, 300000L};
  can_timer_t* const timer = can_timer_create();
  can_timer_set_func(timer, timer_func_empty, nullptr);
  can_timer_start(timer, net, &tstart, &interval);

  const auto ret = can_net_set_time(net, &tp);

  timespec out_tp = {0, 0L};
  can_net_get_time(net, &out_tp);
  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(tp.tv_sec, out_tp.tv_sec);
  CHECK_EQUAL(tp.tv_nsec, out_tp.tv_nsec);
  CHECK_EQUAL(25, CAN_Net_Static::tfunc_empty_counter);

  can_timer_destroy(timer);
}

TEST(CAN_Net, CanNetSetTime_MultipleCallsErr) {
  const timespec tp = {5, 30L};
  const timespec tstart = {5, 0L};
  can_timer_t* const timer1 = can_timer_create();
  can_timer_t* const timer2 = can_timer_create();
  can_timer_set_func(timer1, timer_func_err, nullptr);
  can_timer_set_func(timer2, timer_func_err, nullptr);
  can_timer_start(timer1, net, &tstart, nullptr);
  can_timer_start(timer2, net, &tstart, nullptr);

  const auto ret = can_net_set_time(net, &tp);

  timespec out_tp = {0, 0L};
  can_net_get_time(net, &out_tp);
  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(tp.tv_sec, out_tp.tv_sec);
  CHECK_EQUAL(tp.tv_nsec, out_tp.tv_nsec);
  CHECK_EQUAL(2, CAN_Net_Static::tfunc_err_counter);

  can_timer_destroy(timer1);
  can_timer_destroy(timer2);
}

TEST(CAN_Net, CanNetGetNextFunc_Null) {
  can_net_get_next_func(net, nullptr, nullptr);
}

TEST(CAN_Net, CanNetSetNextFunc) {
  int data = 256;

  can_net_set_next_func(net, timer_func_empty, &data);

  can_timer_func_t* out_ptr = nullptr;
  void* out_data = nullptr;
  can_net_get_next_func(net, &out_ptr, &out_data);

  FUNCTIONPOINTERS_EQUAL(timer_func_empty, out_ptr);
  POINTERS_EQUAL(&data, out_data);
}

TEST(CAN_Net, CanNetSetNextFunc_TimerCall) {
  int data = 256;
  const timespec tp = {5, 30L};
  const timespec tstart1 = {5, 0L};
  const timespec tstart2 = {6, 0L};
  can_timer_t* const timer1 = can_timer_create();
  can_timer_t* const timer2 = can_timer_create();
  can_timer_start(timer1, net, &tstart1, nullptr);
  can_timer_start(timer2, net, &tstart2, nullptr);

  can_net_set_next_func(net, timer_func_empty, &data);

  const auto ret = can_net_set_time(net, &tp);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1, CAN_Net_Static::tfunc_empty_counter);

  can_timer_destroy(timer1);
  can_timer_destroy(timer2);
}

TEST(CAN_Net, CanNetGetSendFunc_Null) {
  can_net_get_send_func(net, nullptr, nullptr);
}

TEST(CAN_Net, CanNetSetSendFunc) {
  int data = 512;

  can_net_set_send_func(net, send_func_empty, &data);

  can_send_func_t* out_ptr = nullptr;
  void* out_data = nullptr;
  can_net_get_send_func(net, &out_ptr, &out_data);

  FUNCTIONPOINTERS_EQUAL(send_func_empty, out_ptr);
  POINTERS_EQUAL(&data, out_data);
}

TEST(CAN_Net, CanNetRecv) {
  can_msg msg = CAN_MSG_INIT;
  msg.id = 0x01;

  can_recv_t* const recv1 = can_recv_create();
  can_recv_t* const recv2 = can_recv_create();
  can_recv_t* const recv3 = can_recv_create();
  can_recv_set_func(recv1, recv_func_err, nullptr);
  can_recv_set_func(recv2, recv_func_err, nullptr);
  can_recv_start(recv1, net, 0x01, 0);
  can_recv_start(recv2, net, 0x01, 0);
  can_recv_start(recv3, net, 0x01, 0);

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(-1, ret);

  can_recv_destroy(recv1);
  can_recv_destroy(recv2);
  can_recv_destroy(recv3);
}

TEST(CAN_Net, CanNetSend) {
  const can_msg msg = CAN_MSG_INIT;
  can_net_set_send_func(net, send_func_empty, nullptr);

  const auto ret = can_net_send(net, &msg);

  CHECK_EQUAL(0, ret);
}

TEST(CAN_Net, CanNetSend_Err) {
  const can_msg msg = CAN_MSG_INIT;

  const auto ret = can_net_send(net, &msg);

  CHECK_EQUAL(-1, ret);
}

namespace CAN_NetTimer_Static {
static unsigned int timer_func_counter = 0;
}

TEST_GROUP(CAN_NetTimer) {
  can_timer_t* timer = nullptr;

  static int timer_func(const timespec*, void*) {
    ++CAN_NetTimer_Static::timer_func_counter;
    return 0;
  }

  TEST_SETUP() {
    timer = can_timer_create();
    CHECK(timer != nullptr);
    ++CAN_NetTimer_Static::timer_func_counter = 0;
  }

  TEST_TEARDOWN() { can_timer_destroy(timer); }
};

TEST(CAN_NetTimer, CanTimerGetFunc_Null) {
  can_timer_get_func(timer, nullptr, nullptr);
}

TEST(CAN_NetTimer, CanTimerSetFunc) {
  int data = 768;

  can_timer_set_func(timer, timer_func, &data);

  can_timer_func_t* out_ptr = nullptr;
  void* out_data = nullptr;
  can_timer_get_func(timer, &out_ptr, &out_data);

  FUNCTIONPOINTERS_EQUAL(timer_func, out_ptr);
  POINTERS_EQUAL(&data, out_data);
}

TEST(CAN_NetTimer, CanTimerStart_Null) {
  can_net_t* const net = can_net_create();

  can_timer_start(timer, net, nullptr, nullptr);

  can_net_destroy(net);
}

TEST(CAN_NetTimer, CanTimerTimeout) {
  can_net_t* const net = can_net_create();
  can_timer_set_func(timer, timer_func, nullptr);

  can_timer_timeout(timer, net, 500);

  const timespec tp = {1, 0L};
  const auto ret = can_net_set_time(net, &tp);
  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1, CAN_NetTimer_Static::timer_func_counter);

  can_net_destroy(net);
}

TEST(CAN_NetTimer, CanTimerTimeout_Negative) {
  can_net_t* const net = can_net_create();
  can_timer_set_func(timer, timer_func, nullptr);

  can_timer_timeout(timer, net, -1);

  const timespec tp = {1, 0L};
  const auto ret = can_net_set_time(net, &tp);
  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CAN_NetTimer_Static::timer_func_counter);

  can_net_destroy(net);
}

TEST_GROUP(CAN_NetRecv) {
  can_recv_t* recv = nullptr;

  static int recv_func(const can_msg*, void*) { return 0; }

  TEST_SETUP() {
    recv = can_recv_create();
    CHECK(recv != nullptr);
  }

  TEST_TEARDOWN() { can_recv_destroy(recv); }
};

TEST(CAN_NetRecv, CanRecvGetFunc_Null) {
  can_recv_get_func(recv, nullptr, nullptr);
}

TEST(CAN_NetRecv, CanRecvSetFunc) {
  int data = 1024;

  can_recv_set_func(recv, recv_func, &data);

  can_recv_func_t* out_ptr = nullptr;
  void* out_data = nullptr;
  can_recv_get_func(recv, &out_ptr, &out_data);

  FUNCTIONPOINTERS_EQUAL(recv_func, out_ptr);
  POINTERS_EQUAL(&data, out_data);
}
