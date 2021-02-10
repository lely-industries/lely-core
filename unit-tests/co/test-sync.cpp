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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <memory>

#include <CppUTest/TestHarness.h>

#include <lely/can/net.h>
#include <lely/co/sync.h>
#include <lely/co/detail/obj.h>

#include "allocators/default.hpp"
#include "allocators/limited.hpp"

#include "lely-cpputest-ext.hpp"
#include "lely-unit-test.hpp"

#include "holder/dev.hpp"
#include "holder/obj.hpp"

struct SyncInd {
  static bool called;
  static co_sync_t* sync;
  static co_unsigned8_t cnt;
  static void* data;

  static void
  func(co_sync_t* sync_, co_unsigned8_t cnt_, void* data_) {
    sync = sync_;
    cnt = cnt_;
    data = data_;

    called = true;
  }

  static inline void
  Clear() {
    sync = nullptr;
    cnt = 0;
    data = nullptr;

    called = false;
  }
};

bool SyncInd::called = false;
co_sync_t* SyncInd::sync = nullptr;
co_unsigned8_t SyncInd::cnt = 0;
void* SyncInd::data = nullptr;

struct SyncErr {
  static co_sync_t* sync;
  static co_unsigned16_t eec;
  static co_unsigned8_t er;
  static void* data;
  static bool called;

  static inline void
  func(co_sync_t* sync_, co_unsigned16_t eec_, co_unsigned8_t er_,
       void* data_) {
    sync = sync_;
    eec = eec_;
    er = er_;
    data = data_;

    called = true;
  }

  static inline void
  Clear() {
    sync = nullptr;
    eec = 0;
    er = 0;
    data = nullptr;

    called = false;
  }
};

bool SyncErr::called = false;
co_sync_t* SyncErr::sync = nullptr;
co_unsigned16_t SyncErr::eec = 0;
co_unsigned8_t SyncErr::er = 0;
void* SyncErr::data = nullptr;

TEST_BASE(CO_SyncBase) {
  TEST_BASE_SUPER(CO_SyncBase);
  Allocators::Default allocator;

  const co_unsigned8_t DEV_ID = 0x01u;

  static co_dev_t* dev;
  can_net_t* net = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1005;

  void CreateObjInDev(std::unique_ptr<CoObjTHolder> & obj_holder,
                      co_unsigned16_t idx) {
    obj_holder.reset(new CoObjTHolder(idx));
    CHECK(obj_holder->Get() != nullptr);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));
  }

  // obj 0x1005, sub 0x00 contains COB-ID
  void SetCobid(co_unsigned32_t cobid) {
    obj1005->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32, cobid);
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    net = can_net_create(allocator.ToAllocT());
    CHECK(net != nullptr);
  }

  TEST_TEARDOWN() {
    can_net_destroy(net);
    dev_holder.reset();
  }
};
co_dev_t* CO_SyncBase::dev = nullptr;

TEST_GROUP_BASE(CO_SyncInit, CO_SyncBase){};

TEST(CO_SyncInit, CoSyncInit_NoObj1005) {
  const auto ret = co_sync_create(net, dev);

  POINTERS_EQUAL(nullptr, ret);
}

TEST(CO_SyncInit, CoSyncInit) {
  CreateObjInDev(obj1005, 0x1005u);
  SetCobid(DEV_ID);

  const auto sync = co_sync_create(net, dev);

  CHECK(sync != nullptr);
  POINTERS_EQUAL(net, co_sync_get_net(sync));
  POINTERS_EQUAL(dev, co_sync_get_dev(sync));
  co_sync_ind_t* ind = SyncInd::func;
  void* data = nullptr;
  co_sync_get_ind(sync, &ind, &data);
  POINTERS_EQUAL(nullptr, ind);
  POINTERS_EQUAL(nullptr, data);

  co_sync_destroy(sync);
}

TEST(CO_SyncInit, CoSyncCreate_InitError) {
  const auto ret = co_sync_create(net, dev);

  POINTERS_EQUAL(nullptr, ret);
}

TEST(CO_SyncInit, CoSyncCreate) {
  CreateObjInDev(obj1005, 0x1005u);

  const auto sync = co_sync_create(net, dev);

  CHECK(sync != nullptr);
  POINTERS_EQUAL(net, co_sync_get_net(sync));
  POINTERS_EQUAL(dev, co_sync_get_dev(sync));
  co_sync_ind_t* ind = SyncInd::func;
  void* data = nullptr;
  co_sync_get_ind(sync, &ind, &data);
  POINTERS_EQUAL(nullptr, ind);
  POINTERS_EQUAL(nullptr, data);

  co_sync_destroy(sync);
}

TEST(CO_SyncInit, CoSyncDestroy_DestroyNull) { co_sync_destroy(nullptr); }

TEST_GROUP_BASE(CO_Sync, CO_SyncBase) {
  static co_sync_t* sync;
  std::unique_ptr<CoObjTHolder> obj1006;
  std::unique_ptr<CoObjTHolder> obj1019;

  // obj 0x1006, sub 0x00 contains communication cycle period in us
  void CreateObj1006AndSetPeriod(co_unsigned32_t period) {
    CreateObjInDev(obj1006, 0x1006u);
    obj1006->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32, period);
  }

  // obj 0x1019u, sub 0x00 contains synchronous counter overflow value
  void CreateObj1019AndSetCntOverflow(co_unsigned8_t overflow) {
    CreateObjInDev(obj1019, 0x1019u);
    obj1019->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, overflow);
  }

  void CheckSubDnIndDefault(co_unsigned16_t idx) {
    co_sub_t* const sub = co_dev_find_sub(dev, idx, 0x00u);
    CHECK(sub != nullptr);
    co_sub_dn_ind_t* ind = nullptr;
    void* data = nullptr;

    co_sub_get_dn_ind(sub, &ind, &data);

    FUNCTIONPOINTERS_EQUAL(&co_sub_default_dn_ind, ind);
    POINTERS_EQUAL(nullptr, data);
  }

  static void CheckSubDnIndIsSet(co_unsigned16_t idx) {
    co_sub_t* const sub = co_dev_find_sub(dev, idx, 0x00u);
    CHECK(sub != nullptr);
    co_sub_dn_ind_t* ind = nullptr;
    void* data = nullptr;

    co_sub_get_dn_ind(sub, &ind, &data);

    CHECK(ind != &co_sub_default_dn_ind);
    POINTERS_EQUAL(sync, data);
  }

  void SyncSetErrSetInd(co_sync_err_t * err, co_sync_ind_t * ind) {
    co_sync_set_err(sync, err, nullptr);
    co_sync_set_ind(sync, ind, nullptr);
  }

  void SyncSetSendSetInd(can_send_func_t * send, co_sync_ind_t * ind) {
    can_net_set_send_func(net, send, nullptr);
    co_sync_set_ind(sync, ind, nullptr);
  }

  void CreateSYNC() {
    sync = co_sync_create(net, dev);
    CHECK(sync != nullptr);
  }

  void StartSYNC() { CHECK_EQUAL(0, co_sync_start(sync)); }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    CreateObjInDev(obj1005, 0x1005u);

    SyncErr::Clear();
    SyncInd::Clear();
    CanSend::Clear();
  }

  TEST_TEARDOWN() {
    co_sync_destroy(sync);
    sync = nullptr;

    TEST_BASE_TEARDOWN();
  }
};
co_sync_t* TEST_GROUP_CppUTestGroupCO_Sync::sync = nullptr;

TEST(CO_Sync, CoSyncGetInd_PointersNull) {
  CreateSYNC();

  co_sync_get_ind(sync, nullptr, nullptr);
}

TEST(CO_Sync, CoSyncGetInd) {
  CreateSYNC();
  co_sync_ind_t* ind = SyncInd::func;
  void* data = nullptr;

  co_sync_get_ind(sync, &ind, &data);

  POINTERS_EQUAL(nullptr, ind);
  POINTERS_EQUAL(nullptr, data);
}

TEST(CO_Sync, CoSyncSetInd) {
  CreateSYNC();
  void* data = nullptr;

  co_sync_set_ind(sync, SyncInd::func, &data);

  co_sync_ind_t* ind = nullptr;
  void* ret_pdata = nullptr;
  co_sync_get_ind(sync, &ind, &ret_pdata);
  POINTERS_EQUAL(SyncInd::func, ind);
  POINTERS_EQUAL(&data, ret_pdata);
}

TEST(CO_Sync, CoSyncGetErr_PointersNull) {
  CreateSYNC();

  co_sync_get_err(sync, nullptr, nullptr);
}

TEST(CO_Sync, CoSyncGetErr) {
  CreateSYNC();
  co_sync_err_t* err = SyncErr::func;
  void* data = nullptr;

  co_sync_get_err(sync, &err, &data);

  POINTERS_EQUAL(nullptr, err);
  POINTERS_EQUAL(nullptr, data);
}

TEST(CO_Sync, CoSyncSetErr) {
  CreateSYNC();
  void* data = nullptr;

  co_sync_set_err(sync, SyncErr::func, &data);

  co_sync_err_t* err = nullptr;
  void* ret_pdata = nullptr;
  co_sync_get_err(sync, &err, &ret_pdata);
  POINTERS_EQUAL(SyncErr::func, err);
  POINTERS_EQUAL(&data, ret_pdata);
}

TEST(CO_Sync, CoSyncStart_NoObj1006NoObj1019) {
  SetCobid(DEV_ID);
  CreateSYNC();

  const auto ret = co_sync_start(sync);

  CHECK_EQUAL(0, ret);
  CheckSubDnIndIsSet(0x1005u);
}

TEST(CO_Sync, CoSyncStart) {
  SetCobid(DEV_ID);
  CreateObj1006AndSetPeriod(0x01u);
  CreateObj1019AndSetCntOverflow(0x01u);
  CreateSYNC();

  const auto ret = co_sync_start(sync);

  CHECK_EQUAL(0, ret);
  CheckSubDnIndIsSet(0x1005u);
  CheckSubDnIndIsSet(0x1006u);
  CheckSubDnIndIsSet(0x1019u);
}

TEST(CO_Sync, CoSyncStart_AlreadyStarted) {
  SetCobid(DEV_ID);
  CreateObj1006AndSetPeriod(0x01u);
  CreateObj1019AndSetCntOverflow(0x01u);
  CreateSYNC();

  co_sync_start(sync);
  const auto ret = co_sync_start(sync);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Sync, CoSyncIsStopped) {
  SetCobid(DEV_ID);
  CreateObj1006AndSetPeriod(0x01u);
  CreateObj1019AndSetCntOverflow(0x01u);
  CreateSYNC();

  CHECK_EQUAL(1, co_sync_is_stopped(sync));
  co_sync_start(sync);
  CHECK_EQUAL(0, co_sync_is_stopped(sync));
}

TEST(CO_Sync, CoSyncUpdate_IsProducer) {
  SetCobid(DEV_ID | CO_SYNC_COBID_PRODUCER);
  CreateObj1006AndSetPeriod(0x01u);
  CreateSYNC();

  const auto ret = co_sync_start(sync);

  CHECK_EQUAL(0, ret);
  CheckSubDnIndIsSet(0x1005u);
  CheckSubDnIndIsSet(0x1006u);
}

TEST(CO_Sync, CoSyncUpdate_FrameBitSet) {
  SetCobid(DEV_ID | CO_SYNC_COBID_FRAME);
  CreateObj1006AndSetPeriod(0x01u);
  CreateSYNC();

  const auto ret = co_sync_start(sync);

  CHECK_EQUAL(0, ret);
  CheckSubDnIndIsSet(0x1005u);
  CheckSubDnIndIsSet(0x1006u);
}

TEST(CO_Sync, CoSyncUpdate_PeriodValueZero) {
  SetCobid(DEV_ID | CO_SYNC_COBID_PRODUCER);
  CreateObj1006AndSetPeriod(0x00u);
  CreateSYNC();

  const auto ret = co_sync_start(sync);

  CHECK_EQUAL(0, ret);
  CheckSubDnIndIsSet(0x1005u);
  CheckSubDnIndIsSet(0x1006u);
}

TEST(CO_Sync, CoSyncStop_NoObj1019NoObj1006) {
  SetCobid(DEV_ID);
  CreateSYNC();

  co_sync_stop(sync);

  CheckSubDnIndDefault(0x1005u);
}

TEST(CO_Sync, CoSyncStop) {
  SetCobid(DEV_ID);
  CreateObj1019AndSetCntOverflow(0x01u);
  CreateObj1006AndSetPeriod(0x00000001u);
  CreateSYNC();

  co_sync_stop(sync);

  CheckSubDnIndDefault(0x1005u);
  CheckSubDnIndDefault(0x1006u);
  CheckSubDnIndDefault(0x1019u);
}

TEST(CO_Sync, CoSyncRecv_NoErrFuncNoIndFunc) {
  SetCobid(DEV_ID);
  CreateSYNC();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags = 0u;
  msg.len = 0u;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Sync, CoSyncRecv_Err) {
  SetCobid(DEV_ID);
  CreateSYNC();
  SyncSetErrSetInd(SyncErr::func, nullptr);
  StartSYNC();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags = 0u;
  msg.len = 1u;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret);
  CHECK(SyncErr::called);
  POINTERS_EQUAL(nullptr, SyncErr::data);
  CHECK_EQUAL(0x8240u, SyncErr::eec);
  CHECK_EQUAL(0x10u, SyncErr::er);
  CHECK_EQUAL(sync, SyncErr::sync);
}

TEST(CO_Sync, CoSyncRecv_NoErrHandlerWhenNeeded) {
  SetCobid(DEV_ID);
  CreateSYNC();
  SyncSetErrSetInd(nullptr, SyncInd::func);
  StartSYNC();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags = 0u;
  msg.len = 1u;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret);
  CHECK(SyncInd::called);
  POINTERS_EQUAL(nullptr, SyncInd::data);
  CHECK_EQUAL(0, SyncInd::cnt);
  POINTERS_EQUAL(sync, SyncInd::sync);
}

TEST(CO_Sync, CoSyncRecv_OverflowSetToOne) {
  SetCobid(DEV_ID);
  CreateObj1019AndSetCntOverflow(0x01u);
  CreateSYNC();
  SyncSetErrSetInd(SyncErr::func, SyncInd::func);
  StartSYNC();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags = 0u;
  msg.len = 0u;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret);
  CHECK(SyncErr::called);
  POINTERS_EQUAL(nullptr, SyncErr::data);
  CHECK_EQUAL(0x8240u, SyncErr::eec);
  CHECK_EQUAL(0x10u, SyncErr::er);
  CHECK_EQUAL(sync, SyncErr::sync);
  CHECK(SyncInd::called);
  POINTERS_EQUAL(nullptr, SyncInd::data);
  CHECK_EQUAL(0, SyncInd::cnt);
  POINTERS_EQUAL(sync, SyncInd::sync);
}

TEST(CO_Sync, CoSyncRecv_OverflowSetToOneEqualToMsgLen) {
  SetCobid(DEV_ID);
  CreateObj1019AndSetCntOverflow(0x01u);
  CreateSYNC();
  SyncSetErrSetInd(SyncErr::func, SyncInd::func);
  StartSYNC();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags = 0u;
  msg.len = 1u;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret);
  CHECK(!SyncErr::called);
  CHECK(SyncInd::called);
  POINTERS_EQUAL(nullptr, SyncInd::data);
  CHECK_EQUAL(0, SyncInd::cnt);
  POINTERS_EQUAL(sync, SyncInd::sync);
}

TEST(CO_Sync, CoSyncRecv) {
  SetCobid(DEV_ID);
  CreateSYNC();
  SyncSetErrSetInd(SyncErr::func, SyncInd::func);
  StartSYNC();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags = 0u;
  msg.len = 0u;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret);
  CHECK(!SyncErr::called);
  CHECK(SyncInd::called);
  POINTERS_EQUAL(nullptr, SyncInd::data);
  CHECK_EQUAL(0, SyncInd::cnt);
  POINTERS_EQUAL(sync, SyncInd::sync);
}

TEST(CO_Sync, CoSyncTimer_ExtendedCANID) {
  CreateObj1006AndSetPeriod(500u);
  SetCobid(DEV_ID | CO_SYNC_COBID_PRODUCER | CO_SYNC_COBID_FRAME);
  CreateSYNC();
  SyncSetSendSetInd(CanSend::func, SyncInd::func);
  StartSYNC();
  const timespec tp = {0L, 600000L};

  const auto ret = can_net_set_time(net, &tp);

  CHECK_EQUAL(0, ret);
  CHECK(SyncInd::called);
  POINTERS_EQUAL(nullptr, SyncInd::data);
  CHECK_EQUAL(0, SyncInd::cnt);
  POINTERS_EQUAL(sync, SyncInd::sync);
  CHECK(CanSend::called());
  CHECK_EQUAL(DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(CAN_FLAG_IDE, CanSend::msg.flags);
  CHECK_EQUAL(0u, CanSend::msg.len);
  CHECK_EQUAL(0u, CanSend::msg.data[0]);
}

TEST(CO_Sync, CoSyncTimer_NoIndMaxCntNotSet) {
  CreateObj1006AndSetPeriod(500u);
  SetCobid(DEV_ID | CO_SYNC_COBID_PRODUCER);
  CreateSYNC();
  SyncSetSendSetInd(CanSend::func, nullptr);
  StartSYNC();
  const timespec tp = {0L, 600000L};

  const auto ret = can_net_set_time(net, &tp);

  CHECK_EQUAL(0, ret);
  CHECK(CanSend::called());
  CHECK_EQUAL(DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0u, CanSend::msg.flags);
  CHECK_EQUAL(0u, CanSend::msg.len);
  CHECK_EQUAL(0u, CanSend::msg.data[0]);
}

TEST(CO_Sync, CoSyncTimer_MaxCntSet) {
  CreateObj1006AndSetPeriod(500u);
  CreateObj1019AndSetCntOverflow(0x02u);
  SetCobid(DEV_ID | CO_SYNC_COBID_PRODUCER);
  CreateSYNC();
  SyncSetSendSetInd(CanSend::func, SyncInd::func);
  StartSYNC();
  const timespec tp[2] = {{0L, 600000L}, {0L, 1200000L}};

  SyncInd::cnt = 2u;

  const auto ret = can_net_set_time(net, &tp[0]);

  CHECK_EQUAL(0, ret);
  CHECK(SyncInd::called);
  POINTERS_EQUAL(nullptr, SyncInd::data);
  CHECK_EQUAL(1, SyncInd::cnt);
  POINTERS_EQUAL(sync, SyncInd::sync);
  CHECK(CanSend::called());
  CHECK_EQUAL(DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0u, CanSend::msg.flags);
  CHECK_EQUAL(1u, CanSend::msg.len);
  CHECK_EQUAL(1u, CanSend::msg.data[0]);

  SyncInd::Clear();
  CanSend::Clear();

  const auto ret2 = can_net_set_time(net, &tp[1]);

  CHECK_EQUAL(0, ret2);
  CHECK(SyncInd::called);
  POINTERS_EQUAL(nullptr, SyncInd::data);
  CHECK_EQUAL(2, SyncInd::cnt);
  POINTERS_EQUAL(sync, SyncInd::sync);
  CHECK(CanSend::called());
  CHECK_EQUAL(DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0u, CanSend::msg.flags);
  CHECK_EQUAL(1u, CanSend::msg.len);
  CHECK_EQUAL(2u, CanSend::msg.data[0]);
}

TEST(CO_Sync, CoSyncTimer) {
  CreateObj1006AndSetPeriod(500u);
  SetCobid(DEV_ID | CO_SYNC_COBID_PRODUCER);
  CreateSYNC();
  SyncSetSendSetInd(CanSend::func, SyncInd::func);
  StartSYNC();
  const timespec tp = {0L, 600000L};

  const auto ret = can_net_set_time(net, &tp);

  CHECK_EQUAL(0, ret);
  CHECK(SyncInd::called);
  POINTERS_EQUAL(nullptr, SyncInd::data);
  CHECK_EQUAL(0, SyncInd::cnt);
  POINTERS_EQUAL(sync, SyncInd::sync);
  CHECK(CanSend::called());
  CHECK_EQUAL(DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0u, CanSend::msg.flags);
  CHECK_EQUAL(0u, CanSend::msg.len);
  CHECK_EQUAL(0u, CanSend::msg.data[0]);
}

TEST_GROUP_BASE(Co_SyncAllocation, CO_SyncBase) {
  co_sync_t* sync;
  Allocators::Limited limitedAllocator;

  TEST_SETUP() {
    TEST_BASE_SETUP();

    can_net_destroy(net);
    net = can_net_create(limitedAllocator.ToAllocT());

    CreateObjInDev(obj1005, 0x1005u);
  }

  TEST_TEARDOWN() {
    co_sync_destroy(sync);
    TEST_BASE_TEARDOWN();
  }
};

TEST(Co_SyncAllocation, CoSyncCreate_NoMoreMemory) {
  limitedAllocator.LimitAllocationTo(0u);

  sync = co_sync_create(net, dev);

  POINTERS_EQUAL(nullptr, sync);
}

TEST(Co_SyncAllocation, CoSyncCreate_MemoryOnlyForSyncT) {
  limitedAllocator.LimitAllocationTo(co_sync_sizeof());

  sync = co_sync_create(net, dev);

  POINTERS_EQUAL(nullptr, sync);
}

TEST(Co_SyncAllocation, CoSyncCreate_MemoryOnlyForSyncTAndRecv) {
  limitedAllocator.LimitAllocationTo(co_sync_sizeof() + can_recv_sizeof());

  sync = co_sync_create(net, dev);

  POINTERS_EQUAL(nullptr, sync);
}

TEST(Co_SyncAllocation, CoSyncCreate_MemoryOnlyForSyncTAndTimer) {
  limitedAllocator.LimitAllocationTo(co_sync_sizeof() + can_timer_sizeof());

  sync = co_sync_create(net, dev);

  POINTERS_EQUAL(nullptr, sync);
}

TEST(Co_SyncAllocation, CoSyncCreate_AllNecessaryMemoryAvailable) {
  limitedAllocator.LimitAllocationTo(co_sync_sizeof() + can_recv_sizeof() +
                                     can_timer_sizeof());

  sync = co_sync_create(net, dev);

  CHECK(sync != nullptr);
}
