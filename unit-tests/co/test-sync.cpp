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

#include "lely-cpputest-ext.hpp"
#include "lely-unit-test.hpp"

#include "holder/dev.hpp"
#include "holder/obj.hpp"

TEST_BASE(CO_SyncBase) {
  TEST_BASE_SUPER(CO_SyncBase);

  can_net_t* net = nullptr;
  const co_unsigned32_t CAN_ID = 0x000000ffu;

  static co_dev_t* dev;
  std::unique_ptr<CoDevTHolder> dev_holder;
  const co_unsigned8_t DEV_ID = 0x1fu;

  std::unique_ptr<CoObjTHolder> obj1005;

  static co_unsigned8_t DUMMY_VAR;

  Allocators::Default allocator;

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

  static void sync_ind_func_empty(co_sync_t*, co_unsigned8_t, void*) {}
  static void sync_err_func_empty(co_sync_t*, co_unsigned16_t, co_unsigned8_t,
                                  void*) {}

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
co_unsigned8_t CO_SyncBase::DUMMY_VAR = 0x00u;
co_dev_t* CO_SyncBase::dev = nullptr;

TEST_GROUP_BASE(CO_SyncInit, CO_SyncBase){};

TEST(CO_SyncInit, CoSyncInit_NoObj1005) {
  const auto ret = co_sync_create(net, dev);

  POINTERS_EQUAL(nullptr, ret);
}

TEST(CO_SyncInit, CoSyncInit) {
  CreateObjInDev(obj1005, 0x1005u);
  SetCobid(CAN_ID);

  const auto sync = co_sync_create(net, dev);

  CHECK(sync != nullptr);
  POINTERS_EQUAL(net, co_sync_get_net(sync));
  POINTERS_EQUAL(dev, co_sync_get_dev(sync));
  co_sync_ind_t* ind = &sync_ind_func_empty;
  void* data = &DUMMY_VAR;
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
  co_sync_ind_t* ind = &sync_ind_func_empty;
  void* data = &DUMMY_VAR;
  co_sync_get_ind(sync, &ind, &data);
  POINTERS_EQUAL(nullptr, ind);
  POINTERS_EQUAL(nullptr, data);

  co_sync_destroy(sync);
}

TEST(CO_SyncInit, CoSyncDestroy_DestroyNull) { co_sync_destroy(nullptr); }

TEST(CO_SyncInit, CoSyncFree_Null) { __co_sync_free(nullptr); }

TEST_GROUP_BASE(CO_Sync, CO_SyncBase) {
  static co_sync_t* sync;
  std::unique_ptr<CoObjTHolder> obj1006;
  std::unique_ptr<CoObjTHolder> obj1019;

  static bool sync_err_func_dummy_called;
  static bool sync_ind_func_dummy_called;
  static bool can_send_called;
  static can_msg sent_msg;

  // obj 0x1006, sub 0x00 contains communication cycle period in us
  void CreateObjAndSetPeriod(co_unsigned32_t period) {
    CreateObjInDev(obj1006, 0x1006u);
    obj1006->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32, period);
  }

  // obj 0x1019u, sub 0x00 contains synchronous counter overflow value
  void CreateObjAndSetCntOverflow(co_unsigned8_t overflow) {
    CreateObjInDev(obj1019, 0x1019u);
    obj1019->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, overflow);
  }

  void CheckSubDnIndDefault(co_unsigned16_t idx) {
    co_sub_t* const sub = co_dev_find_sub(dev, idx, 0x00u);
    CHECK(sub != nullptr);
    co_sub_dn_ind_t* ind = nullptr;
    void* data = &DUMMY_VAR;

    co_sub_get_dn_ind(sub, &ind, &data);

    FUNCTIONPOINTERS_EQUAL(&co_sub_default_dn_ind, ind);
    POINTERS_EQUAL(nullptr, data);
  }

  static void CheckSubDnIndNotNull(co_unsigned16_t idx) {
    co_sub_t* const sub = co_dev_find_sub(dev, idx, 0x00u);
    CHECK(sub != nullptr);
    co_sub_dn_ind_t* ind = nullptr;
    void* data = &DUMMY_VAR;

    co_sub_get_dn_ind(sub, &ind, &data);

    CHECK(ind != nullptr);
    POINTERS_EQUAL(sync, data);
  }

  void PrepareSyncToRecv(co_sync_err_t * err, co_sync_ind_t * ind) {
    SetCobid(CAN_ID);
    co_sync_set_err(sync, err, nullptr);
    co_sync_set_ind(sync, ind, nullptr);
    co_sync_start(sync);
  }

  void PrepareSyncTimer(can_send_func_t * send, co_sync_ind_t * ind,
                        co_unsigned32_t cobid) {
    SetCobid(cobid);
    can_net_set_send_func(net, send, nullptr);
    co_sync_set_ind(sync, ind, nullptr);
    co_sync_start(sync);
  }

  static void sync_err_func_dummy(co_sync_t * psync, co_unsigned16_t eec,
                                  co_unsigned8_t er, void* data) {
    POINTERS_EQUAL(sync, psync);
    CHECK_EQUAL(0x8240u, eec);
    CHECK_EQUAL(0x10u, er);
    POINTERS_EQUAL(nullptr, data);

    sync_err_func_dummy_called = true;
  }

  static void sync_ind_func_dummy(co_sync_t * psync, co_unsigned8_t cnt,
                                  void* data) {
    (void)cnt;

    POINTERS_EQUAL(sync, psync);
    POINTERS_EQUAL(nullptr, data);

    sync_ind_func_dummy_called = true;
  }

  static int can_send_func_dummy(const can_msg* msg, void*) {
    can_send_called = true;
    sent_msg = *msg;

    return 0;
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    CreateObjInDev(obj1005, 0x1005u);

    sync = co_sync_create(net, dev);
    CHECK(sync != nullptr);

    sync_err_func_dummy_called = false;
    sync_ind_func_dummy_called = false;
    can_send_called = false;
    sent_msg = CAN_MSG_INIT;
  }

  TEST_TEARDOWN() {
    co_sync_destroy(sync);

    TEST_BASE_TEARDOWN();
  }
};
co_sync_t* TEST_GROUP_CppUTestGroupCO_Sync::sync = nullptr;
bool TEST_GROUP_CppUTestGroupCO_Sync::sync_err_func_dummy_called = false;
bool TEST_GROUP_CppUTestGroupCO_Sync::sync_ind_func_dummy_called = false;
bool TEST_GROUP_CppUTestGroupCO_Sync::can_send_called = false;
can_msg TEST_GROUP_CppUTestGroupCO_Sync::sent_msg = CAN_MSG_INIT;

TEST(CO_Sync, CoSyncGetInd_PointersNull) {
  co_sync_get_ind(sync, nullptr, nullptr);
}

TEST(CO_Sync, CoSyncGetInd) {
  co_sync_ind_t* ind = &sync_ind_func_empty;
  void* data = &DUMMY_VAR;

  co_sync_get_ind(sync, &ind, &data);

  POINTERS_EQUAL(nullptr, ind);
  POINTERS_EQUAL(nullptr, data);
}

TEST(CO_Sync, CoSyncSetInd) {
  co_sync_set_ind(sync, &sync_ind_func_empty, &DUMMY_VAR);

  co_sync_ind_t* ind = nullptr;
  void* data = nullptr;
  co_sync_get_ind(sync, &ind, &data);
  POINTERS_EQUAL(&sync_ind_func_empty, ind);
  POINTERS_EQUAL(&DUMMY_VAR, data);
}

TEST(CO_Sync, CoSyncGetErr_PointersNull) {
  co_sync_get_err(sync, nullptr, nullptr);
}

TEST(CO_Sync, CoSyncGetErr) {
  co_sync_err_t* err = &sync_err_func_empty;
  void* data = &DUMMY_VAR;

  co_sync_get_err(sync, &err, &data);

  POINTERS_EQUAL(nullptr, err);
  POINTERS_EQUAL(nullptr, data);
}

TEST(CO_Sync, CoSyncSetErr) {
  co_sync_set_err(sync, &sync_err_func_empty, &DUMMY_VAR);

  co_sync_err_t* err = nullptr;
  void* data = nullptr;
  co_sync_get_err(sync, &err, &data);
  POINTERS_EQUAL(&sync_err_func_empty, err);
  POINTERS_EQUAL(&DUMMY_VAR, data);
}

TEST(CO_Sync, CoSyncStart_NoObj1006NoObj1019) {
  SetCobid(CAN_ID);

  const auto ret = co_sync_start(sync);

  CHECK_EQUAL(0, ret);
  CheckSubDnIndNotNull(0x1005u);
}

TEST(CO_Sync, CoSyncStart) {
  SetCobid(CAN_ID);
  CreateObjAndSetPeriod(0x01u);
  CreateObjAndSetCntOverflow(0x01u);

  const auto ret = co_sync_start(sync);

  CHECK_EQUAL(0, ret);
  CheckSubDnIndNotNull(0x1005u);
  CheckSubDnIndNotNull(0x1006u);
  CheckSubDnIndNotNull(0x1019u);
}

TEST(CO_Sync, CoSyncUpdate_IsProducer) {
  SetCobid(CAN_ID | CO_SYNC_COBID_PRODUCER);
  CreateObjAndSetPeriod(0x01u);

  const auto ret = co_sync_start(sync);

  CHECK_EQUAL(0, ret);
  CheckSubDnIndNotNull(0x1005u);
  CheckSubDnIndNotNull(0x1006u);
}

TEST(CO_Sync, CoSyncUpdate_CobidFrame) {
  SetCobid(CAN_ID | CO_SYNC_COBID_FRAME);
  CreateObjAndSetPeriod(0x01u);

  const auto ret = co_sync_start(sync);

  CHECK_EQUAL(0, ret);
  CheckSubDnIndNotNull(0x1005u);
  CheckSubDnIndNotNull(0x1006u);
}

TEST(CO_Sync, CoSyncUpdate_PeriodValueZero) {
  SetCobid(CAN_ID | CO_SYNC_COBID_PRODUCER);
  CreateObjAndSetPeriod(0x00u);

  const auto ret = co_sync_start(sync);

  CHECK_EQUAL(0, ret);
  CheckSubDnIndNotNull(0x1005u);
  CheckSubDnIndNotNull(0x1006u);
}

TEST(CO_Sync, CoSyncStop_NoObj1019NoObj1006) {
  SetCobid(CAN_ID);

  co_sync_stop(sync);

  CheckSubDnIndDefault(0x1005u);
}

TEST(CO_Sync, CoSyncStop) {
  SetCobid(CAN_ID);
  CreateObjAndSetCntOverflow(0x01u);
  CreateObjAndSetPeriod(0x00000001u);

  co_sync_stop(sync);

  CheckSubDnIndDefault(0x1005u);
  CheckSubDnIndDefault(0x1006u);
  CheckSubDnIndDefault(0x1019u);
}

TEST(CO_Sync, CoSyncRecv_NoErrFuncNoIndFunc) {
  PrepareSyncToRecv(nullptr, nullptr);

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  msg.flags = 0u;
  msg.len = 0u;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Sync, CoSyncRecv_Err) {
  PrepareSyncToRecv(sync_err_func_dummy, nullptr);

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  msg.flags = 0u;
  msg.len = 1u;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(true, sync_err_func_dummy_called);
}

TEST(CO_Sync, CoSyncRecv_NoErrHandlerWhenNeeded) {
  PrepareSyncToRecv(nullptr, sync_ind_func_dummy);

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  msg.flags = 0u;
  msg.len = 1u;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(true, sync_ind_func_dummy_called);
}

TEST(CO_Sync, CoSyncRecv_OverflowSetToOne) {
  CreateObjAndSetCntOverflow(0x01u);
  PrepareSyncToRecv(sync_err_func_dummy, sync_ind_func_dummy);

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  msg.flags = 0u;
  msg.len = 0u;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(true, sync_err_func_dummy_called);
  CHECK_EQUAL(true, sync_ind_func_dummy_called);
}

TEST(CO_Sync, CoSyncRecv_OverflowSetToOneEqualToMsgLen) {
  CreateObjAndSetCntOverflow(0x01u);
  PrepareSyncToRecv(sync_err_func_dummy, sync_ind_func_dummy);

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  msg.flags = 0u;
  msg.len = 1u;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(false, sync_err_func_dummy_called);
  CHECK_EQUAL(true, sync_ind_func_dummy_called);
}

TEST(CO_Sync, CoSyncRecv) {
  PrepareSyncToRecv(sync_err_func_dummy, sync_ind_func_dummy);

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  msg.flags = 0u;
  msg.len = 0u;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(false, sync_err_func_dummy_called);
  CHECK_EQUAL(true, sync_ind_func_dummy_called);
}

TEST(CO_Sync, CoSyncTimer_ExtendedCANID) {
  CreateObjAndSetPeriod(500u);
  PrepareSyncTimer(can_send_func_dummy, sync_ind_func_dummy,
                   CAN_ID | CO_SYNC_COBID_PRODUCER | CO_SYNC_COBID_FRAME);
  const timespec tp = {0L, 600000L};

  const auto ret = can_net_set_time(net, &tp);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(true, sync_ind_func_dummy_called);
  CHECK_EQUAL(true, can_send_called);
  CHECK_EQUAL(CAN_ID, sent_msg.id);
  CHECK_EQUAL(CAN_FLAG_IDE, sent_msg.flags);
  CHECK_EQUAL(0u, sent_msg.len);
  CHECK_EQUAL(0u, sent_msg.data[0]);
}

TEST(CO_Sync, CoSyncTimer_NoIndMaxCntNotSet) {
  CreateObjAndSetPeriod(500u);
  PrepareSyncTimer(&can_send_func_dummy, nullptr,
                   CAN_ID | CO_SYNC_COBID_PRODUCER);
  const timespec tp = {0L, 600000L};

  const auto ret = can_net_set_time(net, &tp);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(true, can_send_called);
  CHECK_EQUAL(CAN_ID, sent_msg.id);
  CHECK_EQUAL(0u, sent_msg.flags);
  CHECK_EQUAL(0u, sent_msg.len);
  CHECK_EQUAL(0u, sent_msg.data[0]);
}

TEST(CO_Sync, CoSyncTimer_MaxCntSet) {
  CreateObjAndSetPeriod(500u);
  CreateObjAndSetCntOverflow(0x02u);
  PrepareSyncTimer(&can_send_func_dummy, sync_ind_func_dummy,
                   CAN_ID | CO_SYNC_COBID_PRODUCER);
  const timespec tp[2] = {{0L, 600000L}, {0L, 1200000L}};

  const auto ret = can_net_set_time(net, &tp[0]);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(true, sync_ind_func_dummy_called);
  CHECK_EQUAL(true, can_send_called);
  CHECK_EQUAL(CAN_ID, sent_msg.id);
  CHECK_EQUAL(0u, sent_msg.flags);
  CHECK_EQUAL(1u, sent_msg.len);
  CHECK_EQUAL(1u, sent_msg.data[0]);

  sync_ind_func_dummy_called = false;
  can_send_called = false;
  sent_msg = CAN_MSG_INIT;

  const auto ret2 = can_net_set_time(net, &tp[1]);

  CHECK_EQUAL(0, ret2);
  CHECK_EQUAL(true, sync_ind_func_dummy_called);
  CHECK_EQUAL(true, can_send_called);
  CHECK_EQUAL(CAN_ID, sent_msg.id);
  CHECK_EQUAL(0u, sent_msg.flags);
  CHECK_EQUAL(1u, sent_msg.len);
  CHECK_EQUAL(2u, sent_msg.data[0]);
}

TEST(CO_Sync, CoSyncTimer) {
  CreateObjAndSetPeriod(500u);
  PrepareSyncTimer(&can_send_func_dummy, sync_ind_func_dummy,
                   CAN_ID | CO_SYNC_COBID_PRODUCER);
  const timespec tp = {0L, 600000L};

  const auto ret = can_net_set_time(net, &tp);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(true, sync_ind_func_dummy_called);
  CHECK_EQUAL(true, can_send_called);
  CHECK_EQUAL(CAN_ID, sent_msg.id);
  CHECK_EQUAL(0u, sent_msg.flags);
  CHECK_EQUAL(0u, sent_msg.len);
  CHECK_EQUAL(0u, sent_msg.data[0]);
}
