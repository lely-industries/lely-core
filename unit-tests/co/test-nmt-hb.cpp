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

#include <functional>
#include <memory>

#include <CppUTest/TestHarness.h>

#include <lely/co/nmt.h>
#include <lely/util/time.h>
#include <lib/co/nmt_hb.h>

#include <libtest/allocators/default.hpp>
#include <libtest/allocators/limited.hpp>
#include <libtest/tools/can-send.hpp>
#include <libtest/tools/co-nmt-hb-ind.hpp>
#include <libtest/tools/co-nmt-st-ind.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"

#include "obj-init/nmt-hb-consumer.hpp"
#include "obj-init/nmt-startup.hpp"

TEST_BASE(CO_NmtHbBase) {
  TEST_BASE_SUPER(CO_NmtHbBase);

  static const co_unsigned8_t DEV_ID = 0x02u;
  static const co_unsigned8_t PRODUCER_DEV_ID = 0x01u;

  can_net_t* net = nullptr;
  co_dev_t* dev = nullptr;

  std::unique_ptr<CoDevTHolder> dev_holder;

  static const co_unsigned16_t HB_TIMEOUT_MS = 550u;
  Allocators::Default allocator;

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(allocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    can_net_set_send_func(net, CanSend::Func, nullptr);
  }

  TEST_TEARDOWN() {
    CoNmtHbInd::Clear();
    CoNmtStInd::Clear();
    CanSend::Clear();

    dev_holder.reset();
    can_net_destroy(net);
    set_errnum(0);
  }
};

TEST_GROUP_BASE(CO_NmtHbCreate, CO_NmtHbBase) {
  co_nmt_t* nmt = nullptr;
  co_nmt_hb_t* hb = nullptr;

  TEST_SETUP() {
    TEST_BASE_SETUP();
    nmt = co_nmt_create(net, dev);
    CHECK(nmt != nullptr);
  }

  TEST_TEARDOWN() {
    co_nmt_hb_destroy(hb);
    co_nmt_destroy(nmt);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_nmt_hb_sizeof()
///@{

/// \Given N/A
///
/// \When co_nmt_hb_sizeof() is called
///
/// \Then the platform-dependent size of the NMT redundancy manager service
///       object is returned
TEST(CO_NmtHbCreate, CoNmtHbSizeof_Nominal) {
  const auto ret = co_nmt_hb_sizeof();

#if defined(__MINGW32__) && !defined(__MINGW64__)
  CHECK_EQUAL(24u, ret);
#else
  CHECK_EQUAL(40u, ret);
#endif
}

///@}

/// @name co_nmt_hb_alignof()
///@{

/// \Given N/A
///
/// \When co_nmt_hb_alignof() is called
///
/// \Then the platform-dependent alignment of the NMT redundancy manager
///       service object is returned
TEST(CO_NmtHbCreate, CoNmtHbAlignof_Nominal) {
  const auto ret = co_nmt_hb_alignof();

#if defined(__MINGW32__) && !defined(__MINGW64__)
  CHECK_EQUAL(4u, ret);
#else
  CHECK_EQUAL(8u, ret);
#endif
}

///@}

/// @name co_nmt_hb_create()
///@{

/// \Given an initialized network (can_net_t) and NMT service (co_nmt_t)
///
/// \When co_nmt_hb_create() is called with pointers to the network and the
///       service
///
/// \Then a pointer to a created NMT redundancy manager service is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_hb_alignof()
///       \Calls co_nmt_hb_sizeof()
///       \Calls co_nmt_hb_get_alloc()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
TEST(CO_NmtHbCreate, CoNmtHbCreate_Default) {
  hb = co_nmt_hb_create(net, nmt);

  POINTER_NOT_NULL(hb);

  POINTERS_EQUAL(can_net_get_alloc(net), co_nmt_hb_get_alloc(hb));
}

///@}

/// @name co_nmt_rdn_destroy()
///@{

/// \Given N/A
///
/// \When co_nmt_hb_destroy() is called with a null NMT redundancy manager
///       service pointer
///
/// \Then nothing is changed
TEST(CO_NmtHbCreate, CoNmtHbDestroy_Null) { co_nmt_hb_destroy(nullptr); }

/// \Given an initialized NMT redundancy manager service (co_nmt_rdn_t)
///
/// \When co_nmt_hb_destroy() is called with a pointer to the service
///
/// \Then the service is finalized and freed
///       \Calls can_timer_destroy()
///       \Calls can_recv_destroy()
///       \Calls co_nmt_hb_get_alloc()
///       \Calls mem_free()
TEST(CO_NmtHbCreate, CoNmtHbDestroy_Nominal) {
  hb = co_nmt_hb_create(net, nmt);
  POINTER_NOT_NULL(hb);

  co_nmt_hb_destroy(hb);
  hb = nullptr;
}

///@}

TEST_GROUP_BASE(CO_NmtHbAllocation, CO_NmtHbBase) {
  Allocators::Limited limitedAllocator;
  co_nmt_t* nmt = nullptr;
  co_nmt_hb_t* hb = nullptr;

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(limitedAllocator.ToAllocT(), 0);
    POINTER_NOT_NULL(net);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    POINTER_NOT_NULL(dev);

    nmt = co_nmt_create(net, dev);
    POINTER_NOT_NULL(nmt);
  }

  TEST_TEARDOWN() {
    co_nmt_hb_destroy(hb);
    co_nmt_destroy(nmt);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_nmt_hb_create()
///@{

/// \Given an initialized network (can_net_t) and NMT service (co_nmt_t), the
///        network has a memory allocator without available memory
///
/// \When co_nmt_hb_create() is called with pointers to the network and the
///       service
///
/// \Then a null pointer is returned, the NMT heartbeat manager service is not
///       created and the error number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_hb_alignof()
///       \Calls co_nmt_hb_sizeof()
///       \Calls co_nmt_hb_get_alloc()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
TEST(CO_NmtHbAllocation, CoNmtHbCreate_InitAllocationFailedNoMemory) {
  limitedAllocator.LimitAllocationTo(0u);
  hb = co_nmt_hb_create(net, nmt);

  POINTERS_EQUAL(nullptr, hb);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given an initialized network (can_net_t) and NMT service (co_nmt_t), the
///        network has a memory allocator limited to only allocate an NMT
///        heartbeat manager service instance
///
/// \When co_nmt_hb_create() is called with pointers to the network and the
///       service
///
/// \Then a null pointer is returned, the NMT heartbeat manager service is not
///       created and the error number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_hb_alignof()
///       \Calls co_nmt_hb_sizeof()
///       \Calls co_nmt_hb_get_alloc()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
TEST(CO_NmtHbAllocation, CoNmtHbCreate_InitAllocationFailedOnlyManager) {
  limitedAllocator.LimitAllocationTo(co_nmt_hb_sizeof());
  hb = co_nmt_hb_create(net, nmt);

  POINTERS_EQUAL(nullptr, hb);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given an initialized network (can_net_t) and NMT service (co_nmt_t), the
///        network has a memory allocator limited to only allocate an NMT
///        heartbeat manager service instance and receiver
///
/// \When co_nmt_hb_create() is called with pointers to the network and the
///       service
///
/// \Then a null pointer is returned, the NMT heartbeat manager service is not
///       created and the error number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_hb_alignof()
///       \Calls co_nmt_hb_sizeof()
///       \Calls co_nmt_hb_get_alloc()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
TEST(CO_NmtHbAllocation,
     CoNmtHbCreate_InitAllocationFailedOnlyManagerAndReceiver) {
  limitedAllocator.LimitAllocationTo(co_nmt_hb_sizeof() + can_recv_sizeof());
  hb = co_nmt_hb_create(net, nmt);

  POINTERS_EQUAL(nullptr, hb);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given an initialized network (can_net_t) and NMT service (co_nmt_t), the
///        network has a memory allocator limited to exactly allocate an NMT
///        heartbeat service instance and all required objects
///
/// \When co_nmt_rdn_create() is called with pointers to the network and the
///       service
///
/// \Then a pointer to a created NMT redundancy manager service is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_get_val_u8()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
TEST(CO_NmtHbAllocation, CoNmtHbCreate_ExactMemory) {
  limitedAllocator.LimitAllocationTo(co_nmt_hb_sizeof() + can_recv_sizeof() +
                                     can_timer_sizeof());
  hb = co_nmt_hb_create(net, nmt);

  POINTER_NOT_NULL(hb);
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

///@}

TEST_GROUP_BASE(CO_NmtHb, CO_NmtHbBase) {
  co_nmt_t* nmt = nullptr;
  co_nmt_hb_t* hb = nullptr;

  int32_t data = 0;
  int32_t hbIndData = 0;
  int32_t stIndData = 0;

  void CreateNmt() {
    nmt = co_nmt_create(net, dev);
    POINTER_NOT_NULL(nmt);

    co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, &hbIndData);
    co_nmt_set_st_ind(nmt, CoNmtStInd::Func, &stIndData);
  }

  void CreateHb() {
    CreateNmt();
    hb = co_nmt_hb_create(net, nmt);
    POINTER_NOT_NULL(hb);
  }

  can_msg CreateHbMsg(const co_unsigned8_t id, const co_unsigned8_t st) const {
    can_msg msg = CAN_MSG_INIT;
    msg.id = CO_NMT_EC_CANID(id);
    msg.len = 1u;
    msg.data[0] = st;

    return msg;
  }

  void AdvanceTimeMs(const uint32_t ms) {
    timespec ts = {0, 0};
    can_net_get_time(net, &ts);
    timespec_add_msec(&ts, ms);
    can_net_set_time(net, &ts);
  }

  TEST_TEARDOWN() {
    co_nmt_hb_destroy(hb);
    co_nmt_destroy(nmt);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_nmt_hb_ind()
///@{

/// \Given a pointer to NMT service (co_nmt_t) with the NMT heartbeat indication
///        function set
///
/// \When co_nmt_hb_ind() is called with a pointer to the NMT service, a Node-ID
///       equal to 0, any event state, any reason and any state of the node
///
/// \Then the NMT heartbeat indication function is not called and the state
///       change indication function is not called
TEST(CO_NmtHb, CoNmtHbInd_ZeroId) {
  CreateNmt();
  const co_unsigned8_t nodeId = 0u;

  co_nmt_hb_ind(nmt, nodeId, 0, 0, 0u);

  CHECK_EQUAL(0u, CoNmtHbInd::GetNumCalled());
  CHECK_EQUAL(0u, CoNmtStInd::GetNumCalled());
}

/// \Given a pointer to NMT service (co_nmt_t) with the NMT heartbeat indication
///        function set
///
/// \When co_nmt_hb_ind() is called with a pointer to the NMT service, a Node-ID
///       larger than CO_NUM_NODES, any event state, any reason and any
///       state of the node
///
/// \Then the NMT heartbeat indication function is not called and the state
///       change indication function is not called
TEST(CO_NmtHb, CoNmtHbInd_IdOverMax) {
  CreateNmt();
  const co_unsigned8_t nodeId = CO_NUM_NODES + 1u;

  co_nmt_hb_ind(nmt, nodeId, 0, 0, 0u);

  CHECK_EQUAL(0u, CoNmtHbInd::GetNumCalled());
  CHECK_EQUAL(0u, CoNmtStInd::GetNumCalled());
}

/// \Given a pointer to NMT service (co_nmt_t) with the NMT heartbeat indication
///        function set
///
/// \When co_nmt_hb_ind() is called with a pointer to the NMT service, a correct
///       Node-ID, any event state, CO_NMT_EC_TIMEOUT reason and any state of
///       the node
///
/// \Then the NMT heartbeat indication function is called with the passed
///       Node-ID, the state and the reason; the state change indication
///       function is not called
TEST(CO_NmtHb, CoNmtHbInd_Nominal_ReasonTimeout) {
  CreateNmt();
  const int state = CO_NMT_EC_OCCURRED;
  const int reason = CO_NMT_EC_TIMEOUT;
  const co_unsigned8_t st = 0;
  CoNmtHbInd::SkipCallToDefaultInd();

  co_nmt_hb_ind(nmt, PRODUCER_DEV_ID, state, reason, st);

  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, PRODUCER_DEV_ID, state, reason, &hbIndData);
  CHECK_EQUAL(0u, CoNmtStInd::GetNumCalled());
}

/// \Given a pointer to NMT service (co_nmt_t) with the NMT heartbeat indication
///        function set
///
/// \When co_nmt_hb_ind() is called with a pointer to the NMT service, a correct
///       Node-ID, any event state, CO_NMT_EC_STATE reason and
///       any state of the node
///
/// \Then the NMT heartbeat indication function is called with the passed
///       Node-ID, the state and the reason; the state change indication
///       function is called with the state of the node
TEST(CO_NmtHb, CoNmtHbInd_Nominal_ReasonStateChange) {
  CreateNmt();
  const int state = CO_NMT_EC_OCCURRED;
  const int reason = CO_NMT_EC_STATE;
  const co_unsigned8_t st = CO_NMT_ST_STOP;
  CoNmtHbInd::SkipCallToDefaultInd();

  co_nmt_hb_ind(nmt, PRODUCER_DEV_ID, state, reason, st);

  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, PRODUCER_DEV_ID, state, reason, &hbIndData);
  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, PRODUCER_DEV_ID, st, &stIndData);
}

///@}

/// @name NMT service: the NMT heartbeat timeout
///@{

/// \Given a pointer to NMT service (co_nmt_t) with the NMT heartbeat consumer
///        service configured for a node
///
/// \When no heartbeat message from the node is received in the consumer
///       heartbeat time
///
/// \Then the NMT heartbeat indication function is called with the node's
///       Node-ID, CO_NMT_EC_OCCURRED state and CO_NMT_EC_TIMEOUT reason
TEST(CO_NmtHb, CoNmtHbInd_HbTimer_Timeout) {
  CreateHb();
  co_nmt_hb_set_1016(hb, PRODUCER_DEV_ID, HB_TIMEOUT_MS);
  co_nmt_hb_set_st(hb, CO_NMT_ST_START);

  AdvanceTimeMs(HB_TIMEOUT_MS);

  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, PRODUCER_DEV_ID, CO_NMT_EC_OCCURRED, CO_NMT_EC_TIMEOUT,
                    &hbIndData);
}

/// \Given a pointer to NMT service (co_nmt_t) with the NMT heartbeat consumer
///        service configured for an incorrect Node-ID (0)
///
/// \When no heartbeat message from the node is received in the consumer
///       heartbeat time
///
/// \Then the NMT heartbeat indication function is not called
TEST(CO_NmtHb, CoNmtHbInd_HbTimer_ZeroNodeId) {
  CreateHb();
  co_nmt_hb_set_1016(hb, 0u, HB_TIMEOUT_MS);
  co_nmt_hb_set_st(hb, CO_NMT_ST_START);

  AdvanceTimeMs(HB_TIMEOUT_MS);

  CHECK_EQUAL(0u, CoNmtHbInd::GetNumCalled());
}

/// \Given a pointer to NMT service (co_nmt_t) with the NMT heartbeat consumer
///        service configured for an incorrect Node-ID (larger than
///        CO_NUM_NODES)
///
/// \When no heartbeat message from the node is received in the consumer
///       heartbeat time
///
/// \Then the NMT heartbeat indication function is not called
TEST(CO_NmtHb, CoNmtHbInd_HbTimer_NodeIdTooLarge) {
  CreateHb();
  co_nmt_hb_set_1016(hb, CO_NUM_NODES + 1u, HB_TIMEOUT_MS);
  co_nmt_hb_set_st(hb, CO_NMT_ST_START);

  AdvanceTimeMs(HB_TIMEOUT_MS);

  CHECK_EQUAL(0u, CoNmtHbInd::GetNumCalled());
}

/// \Given a pointer to NMT service (co_nmt_t) with the NMT heartbeat consumer
///        service configured for a node with disabled timeout (0)
///
/// \When no heartbeat message from the node is received in the consumer
///       heartbeat time
///
/// \Then the NMT heartbeat indication function is not called
TEST(CO_NmtHb, CoNmtHbInd_HbTimer_ZeroTimeout) {
  CreateHb();
  co_nmt_hb_set_1016(hb, PRODUCER_DEV_ID, 0u);
  co_nmt_hb_set_st(hb, CO_NMT_ST_START);

  AdvanceTimeMs(HB_TIMEOUT_MS);

  CHECK_EQUAL(0u, CoNmtHbInd::GetNumCalled());
}

///@}

/// @name NMT service: the NMT heartbeat message reception
///@{

/// \Given a pointer to NMT service (co_nmt_t) with the NMT heartbeat consumer
///        service configured for a node
///
/// \When a heartbeat message with the node's changed state is received from the
///       node
///
/// \Then the NMT heartbeat indication function is called with the node's
///       Node-ID, CO_NMT_EC_OCCURRED event and CO_NMT_EC_STATE reason; the
///       state change indication function is called with the new state
TEST(CO_NmtHb, CoNmtHbInd_HbRecv_NodeStateChange) {
  CreateHb();
  co_nmt_hb_set_1016(hb, PRODUCER_DEV_ID, HB_TIMEOUT_MS);
  co_nmt_hb_set_st(hb, CO_NMT_ST_START);
  const uint_least8_t newSt = CO_NMT_ST_STOP;

  const auto msg = CreateHbMsg(PRODUCER_DEV_ID, newSt);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0u));

  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, PRODUCER_DEV_ID, CO_NMT_EC_OCCURRED, CO_NMT_EC_STATE,
                    &hbIndData);
  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, PRODUCER_DEV_ID, newSt, &stIndData);
}

/// \Given a pointer to NMT service (co_nmt_t) with the NMT heartbeat consumer
///        service configured for a node
///
/// \When a heartbeat message with the node's current state is received from the
///       node
///
/// \Then the NMT heartbeat indication function is not called; the state change
///       indication function is not called
TEST(CO_NmtHb, CoNmtHbInd_HbRecv_NodeStateNotChanged) {
  CreateHb();
  const uint_least8_t st = CO_NMT_ST_START;
  co_nmt_hb_set_1016(hb, PRODUCER_DEV_ID, HB_TIMEOUT_MS);
  co_nmt_hb_set_st(hb, st);

  const auto msg = CreateHbMsg(PRODUCER_DEV_ID, st);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0u));

  CHECK_EQUAL(0u, CoNmtHbInd::GetNumCalled());
  CHECK_EQUAL(0u, CoNmtStInd::GetNumCalled());
}

/// \Given a pointer to NMT service (co_nmt_t) with the NMT heartbeat consumer
///        service configured for a node
///
/// \When a malformed heartbeat message with an incorrect message length is
///       received from the node
///
/// \Then the NMT heartbeat indication function is not called; the state change
///       indication function is not called
TEST(CO_NmtHb, CoNmtHbInd_HbRecv_MsgTooShort) {
  CreateHb();
  const uint_least8_t st = CO_NMT_ST_START;
  co_nmt_hb_set_1016(hb, PRODUCER_DEV_ID, HB_TIMEOUT_MS);
  co_nmt_hb_set_st(hb, st);

  auto msg = CreateHbMsg(PRODUCER_DEV_ID, st);
  msg.len = 0;
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0u));

  CHECK_EQUAL(0u, CoNmtHbInd::GetNumCalled());
  CHECK_EQUAL(0u, CoNmtStInd::GetNumCalled());
}

/// \Given a pointer to NMT service (co_nmt_t) with the NMT heartbeat consumer
///        service configured for a node
///
/// \When an NMT error control message with the toggle bit set (not a heartbeat
///       message) is received from the node
///
/// \Then the NMT heartbeat indication function is not called; the state change
///       indication function is not called
TEST(CO_NmtHb, CoNmtHbInd_HbRecv_MsgWithToggleBit) {
  CreateHb();
  const uint_least8_t st = CO_NMT_ST_START;
  co_nmt_hb_set_1016(hb, PRODUCER_DEV_ID, HB_TIMEOUT_MS);
  co_nmt_hb_set_st(hb, st);

  const auto msg = CreateHbMsg(PRODUCER_DEV_ID, st | CO_NMT_ST_TOGGLE);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0u));

  CHECK_EQUAL(0u, CoNmtHbInd::GetNumCalled());
  CHECK_EQUAL(0u, CoNmtStInd::GetNumCalled());
}

/// \Given a pointer to NMT service (co_nmt_t) with the NMT heartbeat consumer
///        service configured for a node and the node's heartbeat time has
///        already passed
///
/// \When a heartbeat message with the node's current state is received from the
///       node
///
/// \Then the NMT heartbeat indication function is called with the node's
///       Node-ID, CO_NMT_EC_RESOLVED event and CO_NMT_EC_TIMEOUT reason; the
///       state change indication function is not called
TEST(CO_NmtHb, CoNmtHbInd_HbRecv_MessageAfterTimeout) {
  CreateHb();
  const uint_least8_t st = CO_NMT_ST_START;
  co_nmt_hb_set_1016(hb, PRODUCER_DEV_ID, HB_TIMEOUT_MS);
  co_nmt_hb_set_st(hb, st);
  AdvanceTimeMs(HB_TIMEOUT_MS);

  CoNmtHbInd::Clear();
  CoNmtStInd::Clear();

  const auto msg = CreateHbMsg(PRODUCER_DEV_ID, st);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0u));

  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, PRODUCER_DEV_ID, CO_NMT_EC_RESOLVED, CO_NMT_EC_TIMEOUT,
                    &hbIndData);
  CHECK_EQUAL(0u, CoNmtStInd::GetNumCalled());
}

/// \Given a pointer to NMT service (co_nmt_t) with the NMT heartbeat consumer
///        service configured for a node and the node's heartbeat time has
///        already passed
///
/// \When a heartbeat message with the node's changed state is received from the
///       node
///
/// \Then the NMT heartbeat indication function is called twice and the state
///       change indication function is called with new state
TEST(CO_NmtHb, CoNmtHbInd_HbRecv_MessageAfterTimeoutWithNewNodeState) {
  CreateHb();
  const uint_least8_t newSt = CO_NMT_ST_STOP;
  co_nmt_hb_set_1016(hb, PRODUCER_DEV_ID, HB_TIMEOUT_MS);
  co_nmt_hb_set_st(hb, CO_NMT_ST_START);
  AdvanceTimeMs(HB_TIMEOUT_MS);

  CoNmtStInd::Clear();
  CoNmtHbInd::Clear();
  CoNmtHbIndMock mock;
  co_nmt_set_hb_ind(nmt, mock.GetFunc(), mock.GetData());

  mock.Expect(nmt, PRODUCER_DEV_ID, CO_NMT_EC_RESOLVED, CO_NMT_EC_TIMEOUT);
  mock.Expect(nmt, PRODUCER_DEV_ID, CO_NMT_EC_OCCURRED, CO_NMT_EC_STATE);

  const auto msg = CreateHbMsg(PRODUCER_DEV_ID, newSt);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0u));

  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, PRODUCER_DEV_ID, newSt, &stIndData);
}

///@}
