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
#include "obj-init/nmt-slave-assignment.hpp"
#include "obj-init/nmt-startup.hpp"
#include "obj-init/error-behavior-object.hpp"

#if LELY_NO_MALLOC

#ifndef CO_NMT_MAX_NHB
#define CO_NMT_MAX_NHB CO_NUM_NODES
#endif

#endif  // LELY_NO_MALLOC

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
    set_errnum(ERRNUM_SUCCESS);
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
/// \Then the platform-dependent size of the NMT heartbeat consumer service
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
/// \Then the platform-dependent alignment of the NMT heartbeat consumer
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
/// \Then a pointer to a created NMT heartbeat consumer service is returned
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

/// @name co_nmt_hb_destroy()
///@{

/// \Given N/A
///
/// \When co_nmt_hb_destroy() is called with a null NMT heartbeat consumer
///       service pointer
///
/// \Then nothing is changed
TEST(CO_NmtHbCreate, CoNmtHbDestroy_Null) { co_nmt_hb_destroy(nullptr); }

/// \Given an initialized NMT heartbeat consumer service (co_nmt_hb_t)
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
/// \When co_nmt_hb_create() is called with pointers to the network and the
///       service
///
/// \Then a pointer to a created NMT heartbeat consumer service is returned
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
  static constexpr co_unsigned8_t NMT_EC_MSG_SIZE = 1u;
  static constexpr co_unsigned8_t NMT_CS_MSG_SIZE = 2u;

  co_nmt_t* nmt = nullptr;
  co_nmt_hb_t* hb = nullptr;

  int32_t data = 0;
  int32_t hbIndData = 0;
  int32_t stIndData = 0;

  std::unique_ptr<CoObjTHolder> obj1016;
  std::unique_ptr<CoObjTHolder> obj1029;
  std::unique_ptr<CoObjTHolder> obj1f80;
  std::unique_ptr<CoObjTHolder> obj1f81;

  void CreateNmt() {
    nmt = co_nmt_create(net, dev);
    POINTER_NOT_NULL(nmt);

    co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, &hbIndData);
    co_nmt_set_st_ind(nmt, CoNmtStInd::Func, &stIndData);
  }

  void CreateNmtAndReset() {
    CreateNmt();
    CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));

    CanSend::Clear();
    CoNmtStInd::Clear();
    CoNmtHbInd::Clear();
  }

  void CreateHb() {
    CreateNmt();
    hb = co_nmt_hb_create(net, nmt);
    POINTER_NOT_NULL(hb);
  }

  void CreateObj1016ConsumerHbTimeN(const co_unsigned8_t num = 1u) {
    assert(num > 0);

    dev_holder->CreateObj<Obj1016ConsumerHb>(obj1016);

    obj1016->EmplaceSub<Obj1016ConsumerHb::Sub00HighestSubidxSupported>(num);
    for (co_unsigned8_t i = 1; i <= num; ++i) {
      obj1016->EmplaceSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(
          i, Obj1016ConsumerHb::MakeHbConsumerEntry(PRODUCER_DEV_ID,
                                                    HB_TIMEOUT_MS));
    }
  }

  void SetupMasterWithSlave() {
    dev_holder->CreateObjValue<Obj1f80NmtStartup>(
        obj1f80, Obj1f80NmtStartup::MASTER_BIT);

    dev_holder->CreateObj<Obj1f81NmtSlaveAssignment>(obj1f81);
    obj1f81->EmplaceSub<Obj1f81NmtSlaveAssignment::Sub00HighestSubidxSupported>(
        DEV_ID);
    obj1f81->EmplaceSub<Obj1f81NmtSlaveAssignment::SubNthSlaveEntry>(
        PRODUCER_DEV_ID, Obj1f81NmtSlaveAssignment::ASSIGNMENT_BIT);
    obj1f81->EmplaceSub<Obj1f81NmtSlaveAssignment::SubNthSlaveEntry>(
        DEV_ID, Obj1f81NmtSlaveAssignment::ASSIGNMENT_BIT);
  }

  void CreateObj1029ErrorBehaviour(const co_unsigned8_t eb) {
    dev_holder->CreateObj<Obj1029ErrorBehavior>(obj1029);
    obj1029->EmplaceSub<Obj1029ErrorBehavior::Sub00HighestSubidxSupported>();
    obj1029->EmplaceSub<Obj1029ErrorBehavior::Sub01CommError>(eb);
  }

  can_msg CreateHbMsg(const co_unsigned8_t id, const co_unsigned8_t st) const {
    can_msg msg = CAN_MSG_INIT;
    msg.id = CO_NMT_EC_CANID(id);
    msg.len = NMT_EC_MSG_SIZE;
    msg.data[0] = st;

    return msg;
  }

  can_msg CreateNmtMsg(const co_unsigned8_t id, const co_unsigned8_t cs) const {
    can_msg msg = CAN_MSG_INIT;
    msg.id = CO_NMT_CS_CANID;
    msg.len = NMT_CS_MSG_SIZE;
    msg.data[0] = cs;
    msg.data[1] = id;

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

  /// @name NMT heartbeat consumer service initialization
  ///@{

#if LELY_NO_MALLOC
/// \Given an initialized NMT service (co_nmt_t) configured with more than
///        `CO_NMT_MAX_NHB` heartbeat consumers
///
/// \When the node is reset with the NMT service RESET NODE
///
/// \Then none heartbeat consumers are initialized
TEST(CO_NmtHb, CoNmtHbInit_HbOverMax) {
  CreateObj1016ConsumerHbTimeN(CO_NMT_MAX_NHB + 1u);
  CreateNmt();

  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));
  CoNmtHbInd::Clear();

  const auto msg = CreateHbMsg(PRODUCER_DEV_ID, CO_NMT_ST_START);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0u));
  CHECK_EQUAL(0u, CoNmtHbInd::GetNumCalled());
}
#endif

/// \Given an initialized NMT service (co_nmt_t) configured with any correct
///        number of heartbeat consumers
///
/// \When the node is reset with the NMT service RESET NODE
///
/// \Then all configured heartbeat consumers are initialized
TEST(CO_NmtHb, CoNmtHbInit_Nominal) {
#if LELY_NO_MALLOC
  CreateObj1016ConsumerHbTimeN(CO_NMT_MAX_NHB);
#else
  CreateObj1016ConsumerHbTimeN(CO_NUM_NODES);
#endif
  CreateNmt();

  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));
  CoNmtHbInd::Clear();

  const auto msg = CreateHbMsg(PRODUCER_DEV_ID, CO_NMT_ST_START);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0u));
#if LELY_NO_MALLOC
  CHECK_EQUAL(CO_NMT_MAX_NHB, CoNmtHbInd::GetNumCalled());
#else
  CHECK_EQUAL(CO_NUM_NODES, CoNmtHbInd::GetNumCalled());
#endif
}

///@}

/// @name co_nmt_hb_ind()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t) with the NMT
///        heartbeat indication function set
///
/// \When co_nmt_hb_ind() is called with a pointer to the NMT service, a Node-ID
///       equal to 0, any event state, any reason and any state of the node
///
/// \Then the NMT heartbeat indication function is not called and the NMT state
///       change indication function is not called
TEST(CO_NmtHb, CoNmtHbInd_ZeroNodeId) {
  CreateNmt();
  const co_unsigned8_t nodeId = 0u;

  co_nmt_hb_ind(nmt, nodeId, CO_NMT_EC_OCCURRED, CO_NMT_EC_STATE, 0u);

  CHECK_EQUAL(0u, CoNmtHbInd::GetNumCalled());
  CHECK_EQUAL(0u, CoNmtStInd::GetNumCalled());
}

/// \Given a pointer to an initialized NMT service (co_nmt_t) with the NMT
///        heartbeat indication function set
///
/// \When co_nmt_hb_ind() is called with a pointer to the NMT service, a Node-ID
///       larger than CO_NUM_NODES, any event state, any reason and any
///       state of the node
///
/// \Then the NMT heartbeat indication function is not called and the NMT state
///       change indication function is not called
TEST(CO_NmtHb, CoNmtHbInd_NodeIdOverMax) {
  CreateNmt();
  const co_unsigned8_t nodeId = CO_NUM_NODES + 1u;

  co_nmt_hb_ind(nmt, nodeId, CO_NMT_EC_OCCURRED, CO_NMT_EC_STATE, 0u);

  CHECK_EQUAL(0u, CoNmtHbInd::GetNumCalled());
  CHECK_EQUAL(0u, CoNmtStInd::GetNumCalled());
}

/// \Given a pointer to an initialized NMT service (co_nmt_t) with the NMT
///        heartbeat indication function set
///
/// \When co_nmt_hb_ind() is called with a pointer to the NMT service, a correct
///       Node-ID, any event state, CO_NMT_EC_TIMEOUT reason and any state of
///       the node
///
/// \Then the NMT heartbeat indication function is called with the passed
///       Node-ID, the state and the reason; the NMT state change indication
///       function is not called
TEST(CO_NmtHb, CoNmtHbInd_Nominal_ReasonTimeout) {
  CreateNmt();
  const auto state = CO_NMT_EC_OCCURRED;
  const auto reason = CO_NMT_EC_TIMEOUT;
  const co_unsigned8_t st = 0;
  CoNmtHbInd::SkipCallToDefaultInd();

  co_nmt_hb_ind(nmt, PRODUCER_DEV_ID, state, reason, st);

  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, PRODUCER_DEV_ID, state, reason, &hbIndData);
  CHECK_EQUAL(0u, CoNmtStInd::GetNumCalled());
}

/// \Given a pointer to an initialized NMT service (co_nmt_t) with the NMT
///        heartbeat indication function set
///
/// \When co_nmt_hb_ind() is called with a pointer to the NMT service, a correct
///       Node-ID, any event state, CO_NMT_EC_STATE reason and
///       any state of the node
///
/// \Then the NMT heartbeat indication function is called with the passed
///       Node-ID, the state and the reason; the NMT state change indication
///       function is called with the Node-ID and the state of the node
TEST(CO_NmtHb, CoNmtHbInd_Nominal_ReasonStateChange) {
  CreateNmt();
  const auto state = CO_NMT_EC_OCCURRED;
  const auto reason = CO_NMT_EC_STATE;
  const co_unsigned8_t st = CO_NMT_ST_STOP;
  CoNmtHbInd::SkipCallToDefaultInd();

  co_nmt_hb_ind(nmt, PRODUCER_DEV_ID, state, reason, st);

  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, PRODUCER_DEV_ID, state, reason, &hbIndData);
  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, PRODUCER_DEV_ID, st, &stIndData);
}

///@}

/// @name co_nmt_on_hb()
///@{

/// \Given a pointer to a started NMT service (co_nmt_t) configured as NMT
///        slave
///
/// \When co_nmt_on_hb() is called with a Node-ID equal to 0, any event state
///       and any event reason
///
/// \Then nothing is changed
TEST(CO_NmtHb, CoNmtOnHb_ZeroNodeId) {
  CreateObj1029ErrorBehaviour(2);
  CreateNmtAndReset();

  co_nmt_on_hb(nmt, 0, CO_NMT_EC_OCCURRED, CO_NMT_EC_STATE);

  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
}

/// \Given a pointer to a started NMT service (co_nmt_t) configured as NMT
///        slave
///
/// \When co_nmt_on_hb() is called with a Node-ID larger than CO_NUM_NODES, any
///       event state and any event reason
///
/// \Then nothing is changed
TEST(CO_NmtHb, CoNmtOnHb_NodeIdOverMax) {
  CreateObj1029ErrorBehaviour(2);
  CreateNmtAndReset();

  co_nmt_on_hb(nmt, CO_NUM_NODES + 1u, CO_NMT_EC_OCCURRED, CO_NMT_EC_STATE);

  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
}

/// \Given a pointer to a started NMT service (co_nmt_t) configured as NMT
///        slave
///
/// \When co_nmt_on_hb() is called with a Node-ID, CO_NMT_EC_OCCURRED state and
///       CO_NMT_EC_STATE reason
///
/// \Then nothing is changed
TEST(CO_NmtHb, CoNmtOnHb_StateOccured) {
  CreateObj1029ErrorBehaviour(2);
  CreateNmtAndReset();

  co_nmt_on_hb(nmt, PRODUCER_DEV_ID, CO_NMT_EC_OCCURRED, CO_NMT_EC_STATE);

  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
}

/// \Given a pointer to a started NMT service (co_nmt_t) configured as NMT
///        slave
///
/// \When co_nmt_on_hb() is called with a Node-ID, CO_NMT_EC_RESOLVED state and
///       CO_NMT_EC_TIMEOUT reason
///
/// \Then nothing is changed
TEST(CO_NmtHb, CoNmtOnHb_TimeoutResolved) {
  CreateObj1029ErrorBehaviour(2);
  CreateNmtAndReset();

  co_nmt_on_hb(nmt, PRODUCER_DEV_ID, CO_NMT_EC_RESOLVED, CO_NMT_EC_TIMEOUT);

  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
}

/// \Given a pointer to a started NMT service (co_nmt_t) configured as NMT
///        slave
///
/// \When co_nmt_on_hb() is called with a Node-ID, CO_NMT_EC_OCCURED state and
///       CO_NMT_EC_TIMEOUT reason
///
/// \Then the configured error behaviour is invoked, the node transitions to
///       the NMT 'stop' state
TEST(CO_NmtHb, CoNmtOnHb_TimeoutOccured) {
  CreateObj1029ErrorBehaviour(2);
  CreateNmtAndReset();

  co_nmt_on_hb(nmt, PRODUCER_DEV_ID, CO_NMT_EC_OCCURRED, CO_NMT_EC_TIMEOUT);

  CHECK_EQUAL(CO_NMT_ST_STOP, co_nmt_get_st(nmt));
  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, DEV_ID, CO_NMT_ST_STOP, &stIndData);
}

#if !LELY_NO_CO_MASTER
/// \Given a pointer to a started NMT service (co_nmt_t) configured as NMT
///        master
///
/// \When co_nmt_on_hb() is called with a Node-ID, CO_NMT_EC_OCCURED state and
///       CO_NMT_EC_TIMEOUT reason
///
/// \Then the NMT error indication is invoked, the master submits an NMT 'reset
///       node' request for the node with the passed Node-ID
TEST(CO_NmtHb, CoNmtOnHb_TimeoutOccured_Master) {
  CreateObj1029ErrorBehaviour(2);
  SetupMasterWithSlave();
  CreateNmtAndReset();

  co_nmt_on_hb(nmt, PRODUCER_DEV_ID, CO_NMT_EC_OCCURRED, CO_NMT_EC_TIMEOUT);

  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const can_msg msg = CreateNmtMsg(PRODUCER_DEV_ID, CO_NMT_CS_RESET_NODE);
  CanSend::CheckMsg(msg);
}
#endif

///@}

/// @name NMT service: the default heartbeat event handler
///@{

/// \Given an started NMT service (co_nmt_t) with the NMT heartbeat consumer
///        service configured for a node
///
/// \When a heartbeat message with the node's changed state is received from the
///       node
///
/// \Then the NMT heartbeat indication function is called with the node's
///       Node-ID, CO_NMT_EC_OCCURRED event and CO_NMT_EC_STATE reason; the
///       state change indication function is called with the Node-ID and the
///       new state of the node
TEST(CO_NmtHb, CoNmtHbInd_Default) {
  CreateObj1016ConsumerHbTimeN();
  CreateNmtAndReset();
  co_nmt_set_hb_ind(nmt, nullptr, nullptr);

  const auto msg = CreateHbMsg(PRODUCER_DEV_ID, CO_NMT_ST_START);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0u));

  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, PRODUCER_DEV_ID, CO_NMT_ST_START, &stIndData);
}

///@}

/// @name NMT service: the NMT heartbeat timeout
///@{

/// \Given an initialized NMT service (co_nmt_t) with the NMT heartbeat
///        consumer service configured for a node
///
/// \When no heartbeat message from the node is received in the consumer
///       heartbeat time
///
/// \Then the NMT heartbeat indication function is called with the node's
///       Node-ID, CO_NMT_EC_OCCURRED state and CO_NMT_EC_TIMEOUT reason
TEST(CO_NmtHb, CoNmtHbTimer_Timeout) {
  CreateHb();
  co_nmt_hb_set_1016(hb, PRODUCER_DEV_ID, HB_TIMEOUT_MS);
  co_nmt_hb_set_st(hb, CO_NMT_ST_START);

  AdvanceTimeMs(HB_TIMEOUT_MS);

  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, PRODUCER_DEV_ID, CO_NMT_EC_OCCURRED, CO_NMT_EC_TIMEOUT,
                    &hbIndData);
}

///@}

/// @name co_nmt_hb_set_1016()
///@{

/// \Given a pointer to the NMT heartbeat consumer service (co_nmt_hb_t)
///
/// \When co_nmt_hb_set_1016() is called with a Node-ID equal to zero and any
///       heartbeat consumer time
///
/// \Then the NMT heartbeat receiver is not started, the NMT heartbeat
///       indication function is not called
TEST(CO_NmtHb, CoNmtHbSet1016_ZeroNodeId) {
  CreateHb();

  co_nmt_hb_set_1016(hb, 0u, HB_TIMEOUT_MS);

  const auto msg = CreateHbMsg(PRODUCER_DEV_ID, CO_NMT_ST_START);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0u));
  CHECK_EQUAL(0u, CoNmtHbInd::GetNumCalled());
}

/// \Given a pointer to the NMT heartbeat consumer service (co_nmt_hb_t)
///
/// \When co_nmt_hb_set_1016() is called with a Node-ID larger than
///       CO_NUM_NODES and any heartbeat consumer time
///
/// \Then the NMT heartbeat receiver is not started, the NMT heartbeat
///       indication function is not called
TEST(CO_NmtHb, CoNmtHbSet1016_NodeIdTooLarge) {
  CreateHb();

  co_nmt_hb_set_1016(hb, CO_NUM_NODES + 1u, HB_TIMEOUT_MS);

  const auto msg = CreateHbMsg(PRODUCER_DEV_ID, CO_NMT_ST_START);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0u));
  CHECK_EQUAL(0u, CoNmtHbInd::GetNumCalled());
}

/// \Given a pointer to the NMT heartbeat consumer service (co_nmt_hb_t)
///
/// \When co_nmt_hb_set_1016() is called with a Node-ID and the heartbeat
///       consumer time equal to 0
///
/// \Then the NMT heartbeat receiver is not started, the NMT heartbeat
///       indication function is not called
TEST(CO_NmtHb, CoNmtHbSet1016_ZeroTimeout) {
  CreateHb();

  co_nmt_hb_set_1016(hb, PRODUCER_DEV_ID, 0u);

  const auto msg = CreateHbMsg(PRODUCER_DEV_ID, CO_NMT_ST_START);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0u));
  CHECK_EQUAL(0u, CoNmtHbInd::GetNumCalled());
}

/// \Given a pointer to the NMT heartbeat consumer service (co_nmt_hb_t)
///
/// \When co_nmt_hb_set_1016() is called with a Node-ID and a heartbeat
///       consumer time
///
/// \Then the NMT heartbeat receiver is started, after receiving a heartbeat
///       message with a new state from the node the NMT heartbeat indication
///       function is called with the node's Node-ID, CO_NMT_EC_OCCURRED state,
///       CO_NMT_EC_STATE reason and a pointer to user-specified data
TEST(CO_NmtHb, CoNmtHbSet1016_Nominal) {
  CreateHb();

  co_nmt_hb_set_1016(hb, PRODUCER_DEV_ID, HB_TIMEOUT_MS);

  const auto msg = CreateHbMsg(PRODUCER_DEV_ID, CO_NMT_ST_START);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0u));

  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, PRODUCER_DEV_ID, CO_NMT_EC_OCCURRED, CO_NMT_EC_STATE,
                    &hbIndData);
}

///@}

/// @name co_nmt_hb_set_st()
///@{

/// \Given a pointer to the NMT heartbeat consumer service (co_nmt_hb_t) with
///        a heartbeat consumer set up for a node with Node-ID equal to 0 and
///        any heartbeat consumer time
///
/// \When co_nmt_hb_set_st() is called with any state of the node
///
/// \Then the NMT heartbeat consumer timer is not started, the NMT heartbeat
///       indication function is not called after the heartbeat consumer time
///       has passed
TEST(CO_NmtHb, CoNmtHbSetSt_ZeroNodeId) {
  CreateHb();
  co_nmt_hb_set_1016(hb, 0u, HB_TIMEOUT_MS);

  co_nmt_hb_set_st(hb, CO_NMT_ST_START);

  AdvanceTimeMs(HB_TIMEOUT_MS);
  CHECK_EQUAL(0u, CoNmtHbInd::GetNumCalled());
}

/// \Given a pointer to the NMT heartbeat consumer service (co_nmt_hb_t) with
///        a heartbeat consumer set up for a node with Node-ID larger than
///        CO_NUM_NODES and any heartbeat consumer time
///
/// \When co_nmt_hb_set_st() is called with any state of the node
///
/// \Then the NMT heartbeat consumer timer is not started, the NMT heartbeat
///       indication function is not called after the heartbeat consumer time
///       has passed
TEST(CO_NmtHb, CoNmtHbSetSt_NodeIdTooLarge) {
  CreateHb();
  co_nmt_hb_set_1016(hb, CO_NUM_NODES + 1u, HB_TIMEOUT_MS);

  co_nmt_hb_set_st(hb, CO_NMT_ST_START);

  AdvanceTimeMs(HB_TIMEOUT_MS);
  CHECK_EQUAL(0u, CoNmtHbInd::GetNumCalled());
}

/// \Given a pointer to the NMT heartbeat consumer service (co_nmt_hb_t) with
///        a heartbeat consumer set up for a node with the heartbeat consumer
///        timeout equal to 0
///
/// \When co_nmt_hb_set_st() is called with any state of the node
///
/// \Then the NMT heartbeat consumer timer is not started, the NMT heartbeat
///       indication function is not called
TEST(CO_NmtHb, CoNmtHbSetSt_ZeroTimeout) {
  CreateHb();
  co_nmt_hb_set_1016(hb, PRODUCER_DEV_ID, 0u);

  co_nmt_hb_set_st(hb, CO_NMT_ST_START);

  AdvanceTimeMs(HB_TIMEOUT_MS);
  CHECK_EQUAL(0u, CoNmtHbInd::GetNumCalled());
}

/// \Given a pointer to the NMT heartbeat consumer service (co_nmt_hb_t) with
///        a heartbeat consumer set up for a node
///
/// \When co_nmt_hb_set_st() is called with any state of the node
///
/// \Then the NMT heartbeat consumer timer is started, after the node's
///       heartbeat consumer time have passed the NMT heartbeat indication
///       function is called with the node's Node-ID, CO_NMT_EC_OCCURRED state,
///       CO_NMT_EC_TIMEOUT reason and a pointer to user-specified data
TEST(CO_NmtHb, CoNmtHbSetSt_Nominal) {
  CreateHb();
  co_nmt_hb_set_1016(hb, PRODUCER_DEV_ID, HB_TIMEOUT_MS);

  co_nmt_hb_set_st(hb, CO_NMT_ST_START);

  AdvanceTimeMs(HB_TIMEOUT_MS);
  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, PRODUCER_DEV_ID, CO_NMT_EC_OCCURRED, CO_NMT_EC_TIMEOUT,
                    &hbIndData);
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
///       state change indication function is called with the Node-ID and the
///       new state of the node
TEST(CO_NmtHb, CoNmtHbRecv_NodeStateChange) {
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
TEST(CO_NmtHb, CoNmtHbRecv_NodeStateNotChanged) {
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
TEST(CO_NmtHb, CoNmtHbRecv_MsgTooShort) {
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
TEST(CO_NmtHb, CoNmtHbRecv_MsgWithToggleBit) {
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
TEST(CO_NmtHb, CoNmtHbRecv_MessageAfterTimeout) {
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
///       change indication function is called with the Node-ID and the new
///       state of the node
TEST(CO_NmtHb, CoNmtHbRecv_MessageAfterTimeoutWithNewNodeState) {
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
