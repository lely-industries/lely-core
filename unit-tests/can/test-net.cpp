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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <CppUTest/TestHarness.h>

#include <lely/can/net.h>

#include <libtest/allocators/default.hpp>
#include <libtest/allocators/limited.hpp>

namespace CAN_Net_Static {
static size_t tfunc_empty_counter = 0;
static size_t tfunc_err_counter = 0;
static size_t rfunc_empty_counter = 0;
static size_t rfunc_err_counter = 0;
static size_t sfunc_empty_counter = 0;
}  // namespace CAN_Net_Static

TEST_GROUP(CAN_NetAllocation) {
  Allocators::Limited allocator;
  can_net_t* net = nullptr;

  TEST_TEARDOWN() { can_net_destroy(net); }
};

/// @name can_net_create()
///@{

/// \Given N/A
///
/// \When can_net_create() is called with a limited allocator with no memory
///
/// \Then a null pointer is returned
TEST(CAN_NetAllocation, CanNetCreate_NoMemoryAvailable) {
  allocator.LimitAllocationTo(0u);
  net = can_net_create(allocator.ToAllocT(), 0);

  POINTERS_EQUAL(nullptr, net);
}

/// \Given N/A
///
/// \When can_net_create() is called with an allocator
///
/// \Then a non-null pointer to the network (can_net_t) is returned, network
///       default parameters are set
TEST(CAN_NetAllocation, CanNetCreate_Nominal) {
  allocator.LimitAllocationTo(can_net_sizeof());
  net = can_net_create(allocator.ToAllocT(), 0);

  CHECK(net != nullptr);
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

  CHECK_EQUAL(0, can_net_get_active_bus(net));
}

/// Test group for #can_net_t tests.
TEST_GROUP(CAN_Net) {
  Allocators::Default allocator;
  can_net_t* net = nullptr;

  static int timer_func_empty(const timespec*, void*) {
    ++CAN_Net_Static::tfunc_empty_counter;
    return 0;
  }
  static int timer_func_err(const timespec*, void*) {
    ++CAN_Net_Static::tfunc_err_counter;
    return -1;
  }

  static int recv_func_empty(const can_msg*, void*) {
    ++CAN_Net_Static::rfunc_empty_counter;
    return 0;
  }
  static int recv_func_err(const can_msg*, void*) {
    ++CAN_Net_Static::rfunc_err_counter;
    return -1;
  }

  static int send_func_empty(const can_msg*, uint_least8_t, void*) {
    ++CAN_Net_Static::sfunc_empty_counter;
    return 0;
  }
  static int send_func_err(const can_msg*, uint_least8_t, void*) { return -1; }

  TEST_SETUP() {
    net = can_net_create(allocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    CAN_Net_Static::tfunc_empty_counter = 0;
    CAN_Net_Static::tfunc_err_counter = 0;
    CAN_Net_Static::rfunc_empty_counter = 0;
    CAN_Net_Static::rfunc_err_counter = 0;
    CAN_Net_Static::sfunc_empty_counter = 0;
  }

  TEST_TEARDOWN() { can_net_destroy(net); }
};

///@}

/// @name can_net_destroy()
///@{

/// \Given a pointer to the network (can_net_t) with multiple receivers and
///        a timer
///
/// \When can_net_destroy() is called
///
/// \Then the network is destroyed
TEST(CAN_Net, CanNetDestroy_Nominal) {
  can_timer_t* const timer1 = can_timer_create(allocator.ToAllocT());
  const timespec time1 = {0, 0L};
  can_timer_start(timer1, net, &time1, nullptr);
  can_recv_t* const recv1 = can_recv_create(allocator.ToAllocT());
  can_recv_t* const recv2 = can_recv_create(allocator.ToAllocT());
  can_recv_t* const recv3 = can_recv_create(allocator.ToAllocT());
  can_recv_start(recv1, net, 0x00, CAN_FLAG_IDE);
  can_recv_start(recv2, net, 0x01, 0);
  can_recv_start(recv3, net, 0x01, 0);

  can_net_destroy(net);
  net = nullptr;

  can_timer_destroy(timer1);
  can_recv_destroy(recv1);
  can_recv_destroy(recv2);
  can_recv_destroy(recv3);
}

/// \Given a null pointer to the network (can_net_t)
///
/// \When can_net_destroy() is called
///
/// \Then nothing is changed
TEST(CAN_Net, CanNetDestroy_Null) {
  can_net_t* const net = nullptr;

  can_net_destroy(net);
}

///@}

/// @name can_net_get_time()
///@{

/// \Given a pointer to the network (co_net_t)
///
/// \When can_net_get_time() is called with no memory area to store the return
///       value
///
/// \Then nothing is changed
TEST(CAN_Net, CanNetGetTime_Null) { can_net_get_time(net, nullptr); }

///@}

/// @name can_net_set_time()
///@{

/// \Given a pointer to the network (co_net_t) with no timers
///
/// \When can_net_set_time() is called with a time interval
///
/// \Then 0 is returned, the requested time is set
TEST(CAN_Net, CanNetSetTime_NoTimers) {
  const timespec tp = {256, 640000L};
  const auto ret = can_net_set_time(net, &tp);

  timespec out_tp = {0, 0L};
  can_net_get_time(net, &out_tp);
  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(tp.tv_sec, out_tp.tv_sec);
  CHECK_EQUAL(tp.tv_nsec, out_tp.tv_nsec);
}

/// \Given a pointer to the network (co_net_t) with a timer with a callback set
///
/// \When can_net_set_time() is called with a time interval less than the
///       timer's trigger value
///
/// \Then 0 is returned, the requested time is set, the timer callback is not
///       called
TEST(CAN_Net, CanNetSetTime_NoCalls) {
  const timespec tp = {4, 0L};
  const timespec tstart = {5, 0L};
  can_timer_t* const timer = can_timer_create(allocator.ToAllocT());
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

/// \Given a pointer to the network (co_net_t) with a timer with a callback set
///
/// \When can_net_set_time() is called with a time interval greater than the
///       timer's trigger value
///
/// \Then 0 is returned, the requested time is set, the timer callback is called
///       once
TEST(CAN_Net, CanNetSetTime_OneCall) {
  const timespec tp = {5, 30L};
  const timespec tstart = {5, 0L};
  can_timer_t* const timer = can_timer_create(allocator.ToAllocT());
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

/// \Given a pointer to the network (co_net_t) with a timer with a callback
///        returning an error set
///
/// \When can_net_set_time() is called with a time interval greater than the
///       timer's trigger value
///
/// \Then -1 is returned, the requested time is set, the timer callback is
///       called once
TEST(CAN_Net, CanNetSetTime_OneCallErr) {
  const timespec tp = {5, 30L};
  const timespec tstart = {5, 0L};
  can_timer_t* const timer = can_timer_create(allocator.ToAllocT());
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

/// \Given a pointer to the network (co_net_t) with a timer without a callback
///
/// \When can_net_set_time() is called with a time interval greater than the
///       timer's trigger value
///
/// \Then 0 is returned, the requested time is set
TEST(CAN_Net, CanNetSetTime_OneCallNoFunc) {
  const timespec tp = {5, 30L};
  const timespec tstart = {5, 0L};
  can_timer_t* const timer = can_timer_create(allocator.ToAllocT());
  can_timer_start(timer, net, &tstart, nullptr);

  const auto ret = can_net_set_time(net, &tp);

  timespec out_tp = {0, 0L};
  can_net_get_time(net, &out_tp);
  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(tp.tv_sec, out_tp.tv_sec);
  CHECK_EQUAL(tp.tv_nsec, out_tp.tv_nsec);

  can_timer_destroy(timer);
}

/// \Given a pointer to the network (co_net_t) with a timer with at least one
///        second interval and a callback set
///
/// \When can_net_set_time() is called with a time interval greater than the
///       timer's trigger value
///
/// \Then 0 is returned, the requested time is set, the timer callback is called
///       once
TEST(CAN_Net, CanNetSetTime_IntervalSec) {
  const timespec tp = {5, 30L};
  const timespec tstart = {5, 0L};
  const timespec interval = {1, 0L};
  can_timer_t* const timer = can_timer_create(allocator.ToAllocT());
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

/// \Given a pointer to the network (co_net_t) with a timer with interval less
///        than one second and a callback set
///
/// \When can_net_set_time() is called with a time interval greater than the
///       timer's trigger value
///
/// \Then 0 is returned, the requested time is set, the timer callback is called
///       once
TEST(CAN_Net, CanNetSetTime_IntervalNSec) {
  const timespec tp = {5, 30L};
  const timespec tstart = {5, 0L};
  const timespec interval = {0, 40L};
  can_timer_t* const timer = can_timer_create(allocator.ToAllocT());
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

/// \Given a pointer to the network (co_net_t) with a timer with no start time
///        value, a non-zero interval and a callback set
///
/// \When can_net_set_time() is called with a time interval few times greater
///       than the timer's interval value
///
/// \Then 0 is returned, the requested time is set, the timer callback is called
///       multiple times
TEST(CAN_Net, CanNetSetTime_OnlyInterval) {
  const timespec tp = {5, 30L};
  const timespec interval = {1, 0L};
  can_timer_t* const timer = can_timer_create(allocator.ToAllocT());
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

/// \Given a pointer to the network (co_net_t) with two timers with a callback
///        returning error set
///
/// \When can_net_set_time() is called with a time interval greater than the
///       timer's interval value
///
/// \Then -1 is returned, the requested time is set, the timer callback is
///        called twice
TEST(CAN_Net, CanNetSetTime_MultipleCallsErr) {
  const timespec tp = {5, 30L};
  const timespec tstart = {5, 0L};
  can_timer_t* const timer1 = can_timer_create(allocator.ToAllocT());
  can_timer_t* const timer2 = can_timer_create(allocator.ToAllocT());
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

/// \Given a pointer to the network (co_net_t) with two timers with different
///        start values and the next timer callback set
///
/// \When can_net_set_time() is called with a time interval between the timers
///       start values
///
/// \Then 0 is returned, the next timer callback is called once
TEST(CAN_Net, CanNetSetTime_NextFunc) {
  int32_t data = 256;
  const timespec tp = {5, 30L};
  const timespec tstart1 = {5, 0L};
  const timespec tstart2 = {6, 0L};
  can_timer_t* const timer1 = can_timer_create(allocator.ToAllocT());
  can_timer_t* const timer2 = can_timer_create(allocator.ToAllocT());
  can_timer_start(timer1, net, &tstart1, nullptr);
  can_timer_start(timer2, net, &tstart2, nullptr);

  can_net_set_next_func(net, timer_func_empty, &data);

  const auto ret = can_net_set_time(net, &tp);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1, CAN_Net_Static::tfunc_empty_counter);

  can_timer_destroy(timer1);
  can_timer_destroy(timer2);
}

///@}

/// @name can_net_get_next_func()
///@{

/// \Given a pointer to the network (co_net_t)
///
/// \When can_net_get_next_func() is called with no memory area to store the
///       result
///
/// \Then nothing is changed
TEST(CAN_Net, CanNetGetNextFunc_Null) {
  can_net_get_next_func(net, nullptr, nullptr);
}

///@}

/// @name can_net_set_next_func()
///@{

/// \Given a pointer to the network (co_net_t)
///
/// \When can_net_set_next_func() is called with pointers to the next timer
///       callback and a user-specified data
///
/// \Then requested pointers are set
TEST(CAN_Net, CanNetSetNextFunc_Nominal) {
  int32_t data = 256;

  can_net_set_next_func(net, timer_func_empty, &data);

  can_timer_func_t* out_ptr = nullptr;
  void* out_data = nullptr;
  can_net_get_next_func(net, &out_ptr, &out_data);

  FUNCTIONPOINTERS_EQUAL(timer_func_empty, out_ptr);
  POINTERS_EQUAL(&data, out_data);
}

///@}

/// @name can_net_get_send_func()
///@{

/// \Given a pointer to the network (can_net_t)
///
/// \When can_net_get_send_func() is called with no memory area to store
///       the results
///
/// \Then nothing is changed
TEST(CAN_Net, CanNetGetSendFunc_Null) {
  can_net_get_send_func(net, nullptr, nullptr);
}

///@}

/// @name can_net_set_send_func()
///@{

/// \Given a pointer to the network (can_net_t)
///
/// \When can_net_set_send_func() is called with pointers to the send function
///       and a user-specified data
///
/// \Then the requested pointers are set
TEST(CAN_Net, CanNetSetSendFunc_Nominal) {
  int32_t data = 512;

  can_net_set_send_func(net, send_func_empty, &data);

  can_send_func_t* out_ptr = nullptr;
  void* out_data = nullptr;
  can_net_get_send_func(net, &out_ptr, &out_data);

  FUNCTIONPOINTERS_EQUAL(send_func_empty, out_ptr);
  POINTERS_EQUAL(&data, out_data);
}

///@}

/// @name can_net_get_active_bus()
///@{

/// \Given a pointer to the network (can_net_t)
///
/// \When can_net_get_active_bus() is called
///
/// \Then the active bus ID is returned
TEST(CAN_Net, CanNetGetActiveBus_Nominal) {
  const auto ret = can_net_get_active_bus(net);

  CHECK_EQUAL(0, ret);
}

///@}

/// @name can_net_set_active_bus()
///@{

/// \Given a pointer to the network (can_net_t)
///
/// \When can_net_set_active_bus() is called with a bus ID
///
/// \Then the active bus ID is set
TEST(CAN_Net, CanNetSetActiveBus_Nominal) {
  can_net_set_active_bus(net, 7);

  CHECK_EQUAL(7u, can_net_get_active_bus(net));
}

///@}

/// @name can_net_recv()
///@{

/// \Given a pointer to the network (can_net_t) with one receiver
///
/// \When can_net_recv() is called with a pointer to a CAN message and a bus ID
///       of the active bus
///
/// \Then 1 is returned, the receiver callback is called once
TEST(CAN_Net, CanNetRecv_Nominal) {
  can_msg msg = CAN_MSG_INIT;
  msg.id = 0x01u;

  can_recv_t* const recv = can_recv_create(allocator.ToAllocT());
  can_recv_set_func(recv, recv_func_empty, nullptr);
  can_recv_start(recv, net, msg.id, 0);

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK_EQUAL(1u, CAN_Net_Static::rfunc_empty_counter);

  can_recv_destroy(recv);
}

/// \Given a pointer to the network (can_net_t) with no receivers
///
/// \When can_net_recv() is called with a pointer to a CAN message and a bus ID
///       of the active bus
///
/// \Then 1 is returned
TEST(CAN_Net, CanNetRecv_RecvListEmpty) {
  can_msg msg = CAN_MSG_INIT;
  msg.id = 0x01u;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
}

/// \Given a pointer to the network (can_net_t) with three receivers - two with
///        a callback function returning an error and one with no callback
///
/// \When can_net_recv() is called with a pointer to a CAN message and a bus ID
///       of the active bus
///
/// \Then -1 is returned, the timer callback is called twice
TEST(CAN_Net, CanNetRecv_RecvFuncError) {
  can_msg msg = CAN_MSG_INIT;
  msg.id = 0x01u;

  can_recv_t* const recv1 = can_recv_create(allocator.ToAllocT());
  can_recv_t* const recv2 = can_recv_create(allocator.ToAllocT());
  can_recv_t* const recv3 = can_recv_create(allocator.ToAllocT());
  can_recv_set_func(recv1, recv_func_err, nullptr);
  can_recv_set_func(recv2, recv_func_err, nullptr);
  can_recv_start(recv1, net, msg.id, 0);
  can_recv_start(recv2, net, msg.id, 0);
  can_recv_start(recv3, net, msg.id, 0);

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(2u, CAN_Net_Static::rfunc_err_counter);

  can_recv_destroy(recv1);
  can_recv_destroy(recv2);
  can_recv_destroy(recv3);
}

/// \Given a pointer to the network (can_net_t) with one receiver
///
/// \When can_net_recv() is called with a pointer to a CAN message and a bus ID
///       of an inactive bus
///
/// \Then 0 is returned, the receiver callback is not called
TEST(CAN_Net, CanNetRecv_InactiveBus) {
  can_msg msg = CAN_MSG_INIT;
  msg.id = 0x01u;

  can_recv_t* const recv = can_recv_create(allocator.ToAllocT());
  can_recv_set_func(recv, recv_func_empty, nullptr);
  can_recv_start(recv, net, msg.id, 0);

  can_net_set_active_bus(net, 0);

  const auto ret = can_net_recv(net, &msg, 5);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CAN_Net_Static::rfunc_empty_counter);

  can_recv_destroy(recv);
}

///@}

/// @name can_net_send()
///@{

/// \Given a pointer to the network (can_net_t) with a callback send
///        function set
///
/// \When can_net_send() is called with a CAN message
///
/// \Then 0 is returned, the send function is called once
TEST(CAN_Net, CanNetSend) {
  const can_msg msg = CAN_MSG_INIT;
  can_net_set_send_func(net, send_func_empty, nullptr);

  const auto ret = can_net_send(net, &msg);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CAN_Net_Static::sfunc_empty_counter);
}

/// \Given a pointer to the network (can_net_t) with a callback send function
///        not set
///
/// \When can_net_send() is called with a CAN message
///
/// \Then -1 is returned
TEST(CAN_Net, CanNetSend_Err) {
  const can_msg msg = CAN_MSG_INIT;

  const auto ret = can_net_send(net, &msg);

  CHECK_EQUAL(-1, ret);
}

///@}

namespace CAN_NetTimer_Static {
static size_t timer_func_counter = 0;
}

TEST_GROUP(CAN_NetTimer) {
  can_timer_t* timer = nullptr;
  Allocators::Default allocator;

  static int timer_func(const timespec*, void*) {
    ++CAN_NetTimer_Static::timer_func_counter;
    return 0;
  }

  TEST_SETUP() {
    timer = can_timer_create(allocator.ToAllocT());
    CHECK(timer != nullptr);
    ++CAN_NetTimer_Static::timer_func_counter = 0;
  }

  TEST_TEARDOWN() { can_timer_destroy(timer); }
};

/// @name can_net_timer_get_alloc()
///@{

/// \Given a pointer to the timer (can_timer_t)
///
/// \When can_timer_get_alloc() is called
///
/// \Then allocator is returned
TEST(CAN_NetTimer, CanTimerGetAlloc_Nominal) {
  can_timer_func_t* tfunc = nullptr;
  void* tdata = nullptr;
  can_timer_get_func(timer, &tfunc, &tdata);
  POINTERS_EQUAL(nullptr, tfunc);
  POINTERS_EQUAL(nullptr, tdata);

  POINTERS_EQUAL(allocator.ToAllocT(), can_timer_get_alloc(timer));
}

///@}

/// @name can_timer_destroy()
///@{

/// \Given a null pointer to the timer (can_timer_t)
///
/// \When can_timer_destroy() is called
///
/// \Then nothing is changed
TEST(CAN_NetTimer, CanTimerDestroy_Null) {
  can_timer_t* const timer = nullptr;

  can_timer_destroy(timer);
}

///@}

/// @name can_timer_get_func()
///@{

/// \Given a pointer to the timer (can_timer_t)
///
/// \When can_timer_get_func() is called with no memory area to store
///       the result
///
/// \Then nothing is changed
TEST(CAN_NetTimer, CanTimerGetFunc_Null) {
  can_timer_get_func(timer, nullptr, nullptr);
}

///@}

/// @name can_timer_set_func()
///@{

/// \Given a pointer to the timer (can_timer_t)
///
/// \When can_timer_set_func() is called with pointers to a timer callback
///       function and a user-specified data
///
/// \Then requested pointers are set
TEST(CAN_NetTimer, CanTimerSetFunc_Nominal) {
  int32_t data = 768;

  can_timer_set_func(timer, timer_func, &data);

  can_timer_func_t* out_ptr = nullptr;
  void* out_data = nullptr;
  can_timer_get_func(timer, &out_ptr, &out_data);
  FUNCTIONPOINTERS_EQUAL(timer_func, out_ptr);
  POINTERS_EQUAL(&data, out_data);
}

///@}

/// @name can_timer_start()
///@{

/// \Given a pointer to the timer (can_timer_t)
///
/// \When can_timer_start() is called with a pointer to the network and no start
///       nor interval for the timer
///
/// \Then nothing is changed
///       \Calls can_timer_stop()
TEST(CAN_NetTimer, CanTimerStart_Null) {
  can_net_t* const net = can_net_create(allocator.ToAllocT(), 0);

  can_timer_start(timer, net, nullptr, nullptr);

  can_net_destroy(net);
}

///@}

/// @name can_timer_timeout()
///@{

/// \Given a pointer to the timer (can_timer_t)
///
/// \When can_timer_timeout() is called with a pointer to the network and
///       a timeout value
///
/// \Then 0 is returned, the timer is started with a given timeout
///       \Calls can_net_get_time()
///       \Calls can_timer_start()
TEST(CAN_NetTimer, CanTimerTimeout_Nominal) {
  can_net_t* const net = can_net_create(allocator.ToAllocT(), 0);
  can_timer_set_func(timer, timer_func, nullptr);

  can_timer_timeout(timer, net, 500);

  const timespec tp = {1, 0L};
  const auto ret = can_net_set_time(net, &tp);
  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1, CAN_NetTimer_Static::timer_func_counter);

  can_net_destroy(net);
}

/// \Given a pointer to the timer (can_timer_t)
///
/// \When can_timer_timeout() is called with a pointer to the network and
///       a negative timeout value
///
/// \Then 0 is returned, the timer is not started
///       \Calls can_timer_stop()
TEST(CAN_NetTimer, CanTimerTimeout_Negative) {
  can_net_t* const net = can_net_create(allocator.ToAllocT(), 0);
  can_timer_set_func(timer, timer_func, nullptr);

  can_timer_timeout(timer, net, -1);

  const timespec tp = {1, 0L};
  const auto ret = can_net_set_time(net, &tp);
  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CAN_NetTimer_Static::timer_func_counter);

  can_net_destroy(net);
}

///@}

TEST_GROUP(CAN_NetRecv) {
  can_recv_t* recv = nullptr;
  Allocators::Default allocator;

  static int recv_func(const can_msg*, void*) { return 0; }

  TEST_SETUP() {
    recv = can_recv_create(allocator.ToAllocT());
    CHECK(recv != nullptr);
  }

  TEST_TEARDOWN() { can_recv_destroy(recv); }
};

/// @name can_net_recv()
///@{

/// \Given a pointer to the receiver (can_recv_t)
///
/// \When can_recv_get_alloc() is called
///
/// \Then allocator is returned
TEST(CAN_NetRecv, CanRecvGetAlloc) {
  can_recv_func_t* out_ptr = nullptr;
  void* out_data = nullptr;
  can_recv_get_func(recv, &out_ptr, &out_data);

  FUNCTIONPOINTERS_EQUAL(nullptr, out_ptr);
  POINTERS_EQUAL(nullptr, out_data);

  POINTERS_EQUAL(allocator.ToAllocT(), can_recv_get_alloc(recv));
}

///@}

/// @name can_recv_destroy()
///@{

/// \Given a null pointer to the receiver (can_recv_t)
///
/// \When can_recv_destroy() is called
///
/// \Then nothing is changed
TEST(CAN_NetRecv, CanRecvDestroy_Null) {
  can_recv_t* const recv = nullptr;

  can_recv_destroy(recv);
}

///@}

/// @name can_recv_get_func()
///@{

/// \Given a null pointer to the receiver (can_recv_t)
///
/// \When can_recv_get_func() is called with no memory area to store the results
///
/// \Then nothing is changed
TEST(CAN_NetRecv, CanRecvGetFunc_Null) {
  can_recv_get_func(recv, nullptr, nullptr);
}

///@}

/// @name can_recv_set_func()
///@{

/// \Given a pointer to the receiver (can_recv_t)
///
/// \When can_recv_set_func() is called with pointers to receive function and
///       the user-specified data
///
/// \Then requested pointers are set
TEST(CAN_NetRecv, CanRecvSetFunc) {
  int32_t data = 1024;

  can_recv_set_func(recv, recv_func, &data);

  can_recv_func_t* out_ptr = nullptr;
  void* out_data = nullptr;
  can_recv_get_func(recv, &out_ptr, &out_data);

  FUNCTIONPOINTERS_EQUAL(recv_func, out_ptr);
  POINTERS_EQUAL(&data, out_data);
}

///@}
