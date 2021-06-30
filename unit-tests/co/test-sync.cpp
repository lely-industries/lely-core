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

#include <memory>

#include <CppUTest/TestHarness.h>

#include <lely/can/net.h>
#include <lely/co/sync.h>
#include <lely/co/detail/obj.h>

#include <libtest/allocators/default.hpp>
#include <libtest/allocators/limited.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>

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

  // obj 0x1005, sub 0x00 contains COB-ID
  void SetCobid(co_unsigned32_t cobid) {
    obj1005->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32, cobid);
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    net = can_net_create(allocator.ToAllocT(), 0);
    CHECK(net != nullptr);
  }

  TEST_TEARDOWN() {
    can_net_destroy(net);
    dev_holder.reset();
  }
};
co_dev_t* CO_SyncBase::dev = nullptr;

TEST_GROUP_BASE(CO_SyncCreate, CO_SyncBase){};

/// @name co_sync_create()
///@{

/// \Given pointers to the initialized device (co_dev_t) and network (co_net_t)
///
/// \When co_sync_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned and a SYNC service is not created
///       \Calls co_dev_find_obj()
///       \Calls set_errc() with ERROR_CALL_NOT_IMPLEMENTED
TEST(CO_SyncCreate, CoSyncCreate_NoObj1005) {
  const auto ret = co_sync_create(net, dev);

  POINTERS_EQUAL(nullptr, ret);
}

/// \Given pointers to the initialized device (co_dev_t) and network (co_net_t),
///        the 0x1005 object with COB-ID SYNC set present in the object
///        dictionary
///
/// \When co_sync_create() is called with pointers to the network and the device
///
/// \Then a pointer to newly created SYNC service (co_sync_t) is returned, it
///       has pointers to network and device set properly, indication function
///       is not set
///       \Calls co_dev_find_obj()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
TEST(CO_SyncCreate, CoSyncCreate_Nominal) {
  dev_holder->CreateAndInsertObj(obj1005, 0x1005u);
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

///@}

/// @name co_sync_destroy()
///@{

/// \Given pointers to the initialized device (co_dev_t) and network (co_net_t)
///
/// \When co_sync_destroy() is called with a null pointer
///
/// \Then nothing is changed
TEST(CO_SyncCreate, CoSyncDestroy_DestroyNull) { co_sync_destroy(nullptr); }

/// \Given pointers to the initialized device (co_dev_t), network (co_net_t) and
///        SYNC service (co_sync_t)
///
/// \When co_sync_destroy() is called
///
/// \Then the SYNC service is destroyed
TEST(CO_SyncCreate, CoSyncDestroy_Nominal) {
  dev_holder->CreateAndInsertObj(obj1005, 0x1005u);
  const auto sync = co_sync_create(net, dev);

  co_sync_destroy(sync);
}

///@}

TEST_GROUP_BASE(CO_Sync, CO_SyncBase) {
  static co_sync_t* sync;
  std::unique_ptr<CoObjTHolder> obj1006;
  std::unique_ptr<CoObjTHolder> obj1019;

  // obj 0x1006, sub 0x00 contains communication cycle period in us
  void CreateObj1006AndSetPeriod(co_unsigned32_t period) {
    dev_holder->CreateAndInsertObj(obj1006, 0x1006u);
    obj1006->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32, period);
  }

  // obj 0x1019u, sub 0x00 contains synchronous counter overflow value
  void CreateObj1019AndSetCntOverflow(co_unsigned8_t overflow) {
    dev_holder->CreateAndInsertObj(obj1019, 0x1019u);
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

  void StartSYNC() { CHECK_EQUAL(0, co_sync_start(sync)); }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    dev_holder->CreateAndInsertObj(obj1005, 0x1005u);

    SyncErr::Clear();
    SyncInd::Clear();
    CanSend::Clear();

    sync = co_sync_create(net, dev);
    CHECK(sync != nullptr);
  }

  TEST_TEARDOWN() {
    co_sync_destroy(sync);
    sync = nullptr;

    TEST_BASE_TEARDOWN();
  }
};
co_sync_t* TEST_GROUP_CppUTestGroupCO_Sync::sync = nullptr;

/// @name co_sync_get_ind()
///@{

/// \Given a pointer to the SYNC service (co_sync_t)
///
/// \When co_sync_get_ind() is called with no memory area to store the results
///
/// \Then nothing is changed
TEST(CO_Sync, CoSyncGetInd_PointersNull) {
  co_sync_get_ind(sync, nullptr, nullptr);
}

/// \Given a pointer to the SYNC service (co_sync_t)
///
/// \When co_sync_get_ind() is called with pointers to store indication function
///       and user-specified data
///
/// \Then passed pointers to indication function and data are set to null
TEST(CO_Sync, CoSyncGetInd) {
  co_sync_ind_t* ind = SyncInd::func;
  int data = 42;
  void* dataptr = &data;

  co_sync_get_ind(sync, &ind, &dataptr);

  POINTERS_EQUAL(nullptr, ind);
  POINTERS_EQUAL(nullptr, dataptr);
}

///@}

/// @name co_sync_set_ind()
///@{

/// \Given a pointer to the SYNC service (co_sync_t)
///
/// \When co_sync_set_ind() is called with custom indication function and a
///       non-null pointer to user-specified data
///
/// \Then indication function and pointer to user-specified data have requested
///       values and can be obtained using co_sync_get_ind()
TEST(CO_Sync, CoSyncSetInd) {
  int data = 42;

  co_sync_set_ind(sync, SyncInd::func, &data);

  co_sync_ind_t* ind = nullptr;
  void* ret_pdata = nullptr;
  co_sync_get_ind(sync, &ind, &ret_pdata);
  POINTERS_EQUAL(SyncInd::func, ind);
  POINTERS_EQUAL(&data, ret_pdata);
}

///@}

/// @name co_sync_get_err()
///@{

/// \Given a pointer to the SYNC service (co_sync_t)
///
/// \When co_sync_get_err() is called with no memory area to store the results
///
/// \Then nothing is changed
TEST(CO_Sync, CoSyncGetErr_PointersNull) {
  co_sync_get_err(sync, nullptr, nullptr);
}

/// \Given a pointer to the SYNC service (co_sync_t)
///
/// \When co_sync_get_err() is called with pointers to store error handling
///       function and user-specified data
///
/// \Then passed pointers to error handling function and data are set to null
TEST(CO_Sync, CoSyncGetErr) {
  co_sync_err_t* err = SyncErr::func;
  int data = 42;
  void* dataptr = &data;

  co_sync_get_err(sync, &err, &dataptr);

  POINTERS_EQUAL(nullptr, err);
  POINTERS_EQUAL(nullptr, dataptr);
}

///@}

/// @name co_sync_set_err()
///@{

/// \Given a pointer to the SYNC service (co_sync_t)
///
/// \When co_sync_set_err() is called with custom error handling function and
///       a non-null pointer to user-specified data
///
/// \Then error handling function and pointer to user-specified data have
///       requested values and can be obtained using co_sync_get_err()
TEST(CO_Sync, CoSyncSetErr) {
  int data = 42;

  co_sync_set_err(sync, SyncErr::func, &data);

  co_sync_err_t* err = nullptr;
  void* ret_pdata = nullptr;
  co_sync_get_err(sync, &err, &ret_pdata);
  POINTERS_EQUAL(SyncErr::func, err);
  POINTERS_EQUAL(&data, ret_pdata);
}

///@}

/// @name co_sync_start()
///@{

/// \Given a pointer to the SYNC service (co_sync_t), the 0x1005 object with
///        COB-ID SYNC set and present in the object dictionary, but with 0x1006
///        and 0x1019 objects missing
///
/// \When co_sync_start() is called
///
/// \Then 0 is returned, the SYNC service is started and download indication
///       function for the 0x1005 object is set
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_start()
///       \Calls can_timer_stop()
TEST(CO_Sync, CoSyncStart_NoObj1006NoObj1019) {
  SetCobid(DEV_ID);

  const auto ret = co_sync_start(sync);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_sync_is_stopped(sync));
  CheckSubDnIndIsSet(0x1005u);
}

/// \Given a pointer to the SYNC service (co_sync_t), with the 0x1005, 0x1006
///        and 0x1019 objects present in the object dictionary
///
/// \When co_sync_start() is called
///
/// \Then 0 is returned, the SYNC service is started and download indication
///       functions for the 0x1005, 0x1006 and 0x1019 objects are set
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls co_obj_get_val_u8()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_start()
///       \Calls can_timer_stop()
TEST(CO_Sync, CoSyncStart_Nominal) {
  SetCobid(DEV_ID);
  CreateObj1006AndSetPeriod(0x01u);
  CreateObj1019AndSetCntOverflow(0x01u);

  const auto ret = co_sync_start(sync);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_sync_is_stopped(sync));
  CheckSubDnIndIsSet(0x1005u);
  CheckSubDnIndIsSet(0x1006u);
  CheckSubDnIndIsSet(0x1019u);
}

/// \Given a pointer to already started SYNC service (co_sync_t), with the
///        0x1005, 0x1006 and 0x1019 objects present in the object dictionary
///
/// \When co_sync_start() is called
///
/// \Then 0 is returned, nothing is chnaged
TEST(CO_Sync, CoSyncStart_AlreadyStarted) {
  SetCobid(DEV_ID);
  CreateObj1006AndSetPeriod(0x01u);
  CreateObj1019AndSetCntOverflow(0x01u);

  co_sync_start(sync);
  const auto ret = co_sync_start(sync);

  CHECK_EQUAL(0, ret);
}

///@}

/// @name co_sync_is_stopped()
///@{

/// \Given a pointer to the SYNC service (co_sync_t), with the 0x1005, 0x1006
///        and 0x1019 objects present in the object dictionary
///
/// \When co_sync_is_stopped() is called before and after a call to
///       co_sync_start()
///
/// \Then 1 is returned in case of the first call (before co_sync_start()),
///       0 is returned in case of the second call (after co_sync_start())
TEST(CO_Sync, CoSyncIsStopped_BeforeAfterStart) {
  SetCobid(DEV_ID);
  CreateObj1006AndSetPeriod(0x01u);
  CreateObj1019AndSetCntOverflow(0x01u);

  CHECK_EQUAL(1, co_sync_is_stopped(sync));
  co_sync_start(sync);
  CHECK_EQUAL(0, co_sync_is_stopped(sync));
}

///@}

/// @name co_sync_start()
///@{

/// \Given a pointer to the SYNC service (co_sync_t), the 0x1005 object with
///        COB-ID SYNC with CO_SYNC_COBID_PRODUCER bit set and the 0x1006 object
///        with the communication cycle period set to 1 us
///
/// \When co_sync_start() is called
///
/// \Then 0 is returned, the SYNC service is started, download indication
///       functions for the 0x1005 and 0x1006 objects are set, SYNC service has
///       started cycle period timer and disabled network receiver
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_stop()
///       \Calls can_timer_start()
TEST(CO_Sync, CoSyncStart_IsProducer) {
  SetCobid(DEV_ID | CO_SYNC_COBID_PRODUCER);
  CreateObj1006AndSetPeriod(0x01u);

  const auto ret = co_sync_start(sync);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_sync_is_stopped(sync));
  CheckSubDnIndIsSet(0x1005u);
  CheckSubDnIndIsSet(0x1006u);
}

/// \Given a pointer to the SYNC service (co_sync_t), the 0x1005 object with
///        COB-ID SYNC with CO_SYNC_COBID_FRAME bit set and the 0x1006 object
///        with the communication cycle period set to 1 us
///
/// \When co_sync_start() is called
///
/// \Then 0 is returned, the SYNC service is started, download indication
///       functions for the 0x1005 and 0x1006 objects are set, SYNC service has
///       started receiving SYNC messages using the CAN Extended Format 29-bit
///       identifier
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_start()
///       \Calls can_timer_stop()
TEST(CO_Sync, CoSyncStart_FrameBitSet) {
  SetCobid(DEV_ID | CO_SYNC_COBID_FRAME);
  CreateObj1006AndSetPeriod(0x01u);

  const auto ret = co_sync_start(sync);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_sync_is_stopped(sync));
  CheckSubDnIndIsSet(0x1005u);
  CheckSubDnIndIsSet(0x1006u);
}

/// \Given a pointer to the SYNC service (co_sync_t), the 0x1005 object with
///        COB-ID SYNC with CO_SYNC_COBID_PRODUCER bit set and the 0x1006 object
///        with the communication cycle period set to zero
///
/// \When co_sync_start() is called
///
/// \Then 0 is returned, the SYNC service is started, download indication
///       functions for the 0x1005 and 0x1006 objects are set, SYNC service has
///       disabled cycle period timer and disabled network receiver i.e. cannot
///       produce SYNC messages
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_stop()
///       \Calls can_timer_stop()
TEST(CO_Sync, CoSyncStart_PeriodValueZero) {
  SetCobid(DEV_ID | CO_SYNC_COBID_PRODUCER);
  CreateObj1006AndSetPeriod(0x00u);
  SyncSetSendSetInd(CanSend::Func, nullptr);

  const auto ret = co_sync_start(sync);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_sync_is_stopped(sync));
  CheckSubDnIndIsSet(0x1005u);
  CheckSubDnIndIsSet(0x1006u);

  const timespec tp = {10L, 0L};
  CHECK_EQUAL(0, can_net_set_time(net, &tp));
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

///@}

/// @name co_sync_stop()
///@{

/// \Given a pointer to not started SYNC service (co_sync_t)
///
/// \When co_sync_stop() is called
///
/// \Then nothing is changed
TEST(CO_Sync, CoSyncStop_NotStarted) { co_sync_stop(sync); }

/// \Given a pointer to started SYNC service (co_sync_t), the 0x1005 object with
///        COB-ID SYNC set and present in the object dictionary, but with 0x1006
///        and 0x1019 objects missing
///
/// \When co_sync_stop() is called
///
/// \Then the SYNC service is stopped and download indication function for the
///       0x1005 object is set to default
///       \Calls co_dev_find_obj()
///       \Calls co_obj_set_dn_ind()
TEST(CO_Sync, CoSyncStop_NoObj1019NoObj1006) {
  SetCobid(DEV_ID);
  StartSYNC();

  co_sync_stop(sync);

  CheckSubDnIndDefault(0x1005u);
}

/// \Given a pointer to started SYNC service (co_sync_t), with the 0x1005,
///        0x1006 and 0x1019 objects present in the object dictionary
///
/// \When co_sync_stop() is called
///
/// \Then the SYNC service is stopped and download indication functions for the
///       0x1005, 0x1006 and 0x1019 objects are set to default
///       \Calls co_dev_find_obj()
///       \Calls co_obj_set_dn_ind()
TEST(CO_Sync, CoSyncStop_Nominal) {
  SetCobid(DEV_ID);
  CreateObj1019AndSetCntOverflow(0x01u);
  CreateObj1006AndSetPeriod(0x00000001u);
  StartSYNC();

  co_sync_stop(sync);

  CheckSubDnIndDefault(0x1005u);
  CheckSubDnIndDefault(0x1006u);
  CheckSubDnIndDefault(0x1019u);
}

///@}

/// @name co_sync_is_stopped()
///@{

/// \Given a pointer to started SYNC service (co_sync_t), with the 0x1005,
///        0x1006 and 0x1019 objects present in the object dictionary
///
/// \When co_sync_is_stopped() is called before and after a call to
///       co_sync_stop()
///
/// \Then 0 is returned in case of the first call (before co_sync_stop()),
///       1 is returned in case of the second call (after co_sync_stop())
TEST(CO_Sync, CoSyncIsStopped_BeforeAfterStop) {
  SetCobid(DEV_ID);
  CreateObj1006AndSetPeriod(0x01u);
  CreateObj1019AndSetCntOverflow(0x01u);
  StartSYNC();

  CHECK_EQUAL(0, co_sync_is_stopped(sync));
  co_sync_stop(sync);
  CHECK_EQUAL(1, co_sync_is_stopped(sync));
}

///@}

/// @name SYNC message receiver
///@{

/// \Given a pointer to started SYNC service (co_sync_t), configured without
///        indication nor error handling functions
///
/// \When SYNC message is received
///
/// \Then nothing is changed
TEST(CO_Sync, CoSyncRecv_NoErrFuncNoIndFunc) {
  SetCobid(DEV_ID);
  StartSYNC();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags = 0u;
  msg.len = 0u;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
}

/// \Given a pointer to started SYNC service (co_sync_t), configured without
///        indication function but with error handling function, with the 0x1019
///        object not present in the object dictionary
///
/// \When SYNC message with unexpected data length of 1 is received
///
/// \Then error handling function is called with 0x8240 emergency error code,
///       0x10 error register and a pointer to the SYNC service
TEST(CO_Sync, CoSyncRecv_ErrHandlerOnly_NoIndFunc) {
  SetCobid(DEV_ID);
  SyncSetErrSetInd(SyncErr::func, nullptr);
  StartSYNC();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags = 0u;
  msg.len = 1u;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK(SyncErr::called);
  POINTERS_EQUAL(nullptr, SyncErr::data);
  CHECK_EQUAL(0x8240u, SyncErr::eec);
  CHECK_EQUAL(0x10u, SyncErr::er);
  CHECK_EQUAL(sync, SyncErr::sync);
}

/// \Given a pointer to started SYNC service (co_sync_t), configured with
///        indication function but without error handling function, with the
///        0x1019 object not present in the object dictionary
///
/// \When SYNC message with unexpected data length of 1 is received
///
/// \Then indication function is called with a pointer to the SYNC service and
//        counter set to 0
TEST(CO_Sync, CoSyncRecv_IndFuncOnly_NoErrHandler) {
  SetCobid(DEV_ID);
  SyncSetErrSetInd(nullptr, SyncInd::func);
  StartSYNC();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags = 0u;
  msg.len = 1u;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK(SyncInd::called);
  POINTERS_EQUAL(nullptr, SyncInd::data);
  CHECK_EQUAL(0, SyncInd::cnt);
  POINTERS_EQUAL(sync, SyncInd::sync);
}

/// \Given a pointer to started SYNC service (co_sync_t), configured with both
///        indication function and error handling function, the object 0x1019
///        with counter overflow value set to 1
///
/// \When SYNC message with unexpected data length of 0 is received
///
/// \Then indication function is called with a pointer to the SYNC service and
///       counter set to 0;
///       error handling function is called with 0x8240 emergency error code,
///       0x10 error register and a pointer to the SYNC service
TEST(CO_Sync, CoSyncRecv_OverflowSetToOne) {
  SetCobid(DEV_ID);
  CreateObj1019AndSetCntOverflow(0x01u);
  SyncSetErrSetInd(SyncErr::func, SyncInd::func);
  StartSYNC();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags = 0u;
  msg.len = 0u;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
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

/// \Given a pointer to started SYNC service (co_sync_t), configured with both
///        indication function and error handling function, the object 0x1019
///        with counter overflow value set to 1
///
/// \When SYNC message with data length of 1 is received
///
/// \Then indication function is called with a pointer to the SYNC service and
///       counter set to what was received in the SYNC message;
///       error handling function is not called
TEST(CO_Sync, CoSyncRecv_OverflowSetToOneEqualToMsgLen) {
  SetCobid(DEV_ID);
  CreateObj1019AndSetCntOverflow(0x01u);
  SyncSetErrSetInd(SyncErr::func, SyncInd::func);
  StartSYNC();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags = 0u;
  msg.len = 1u;
  msg.data[0] = 0x42u;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK(!SyncErr::called);
  CHECK(SyncInd::called);
  POINTERS_EQUAL(nullptr, SyncInd::data);
  CHECK_EQUAL(0x42u, SyncInd::cnt);
  POINTERS_EQUAL(sync, SyncInd::sync);
}

/// \Given a pointer to started SYNC service (co_sync_t), configured with both
///        indication function and error handling function, with the 0x1019
///        object not present in the object dictionary
///
/// \When SYNC message with data length of 0 is received
///
/// \Then indication function is called with a pointer to the SYNC service and
///       counter set to 0;
///       error handling function is not called
TEST(CO_Sync, CoSyncRecv_Nominal) {
  SetCobid(DEV_ID);
  SyncSetErrSetInd(SyncErr::func, SyncInd::func);
  StartSYNC();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags = 0u;
  msg.len = 0u;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK(!SyncErr::called);
  CHECK(SyncInd::called);
  POINTERS_EQUAL(nullptr, SyncInd::data);
  CHECK_EQUAL(0, SyncInd::cnt);
  POINTERS_EQUAL(sync, SyncInd::sync);
}

///@}

/// @name SYNC message producer
///@{

/// \Given a pointer to started producer SYNC service (co_sync_t), configured
///        with indication function, communication cycle period set to some
///        non-zero value and COB-ID with CAN Extended Format set, with the
///        0x1019 object not present in the object dictionary
///
/// \When communication cycle period has passed
///
/// \Then indication function is called with a pointer to the SYNC service and
///       counter set to 0;
///       SYNC message with data length of 0 and Identifier Extension flag is
///       sent
///       \Calls can_net_send()
TEST(CO_Sync, CoSyncTimer_ExtendedCANID) {
  CreateObj1006AndSetPeriod(500u);
  SetCobid(DEV_ID | CO_SYNC_COBID_PRODUCER | CO_SYNC_COBID_FRAME);
  SyncSetSendSetInd(CanSend::Func, SyncInd::func);
  StartSYNC();
  const timespec tp = {0L, 600000L};

  const auto ret = can_net_set_time(net, &tp);

  CHECK_EQUAL(0, ret);
  CHECK(SyncInd::called);
  POINTERS_EQUAL(nullptr, SyncInd::data);
  CHECK_EQUAL(0, SyncInd::cnt);
  POINTERS_EQUAL(sync, SyncInd::sync);
  CHECK_COMPARE(CanSend::GetNumCalled(), >, 0);
  CHECK_EQUAL(DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(CAN_FLAG_IDE, CanSend::msg.flags);
  CHECK_EQUAL(0u, CanSend::msg.len);
  CHECK_EQUAL(0u, CanSend::msg.data[0]);
}

/// \Given a pointer to started producer SYNC service (co_sync_t), configured
///        without indication function, communication cycle period set to some
///        non-zero value, without counter overflow value in the 0x1019 object
///
/// \When communication cycle period has passed
///
/// \Then indication function is not called;
///       SYNC message with data length of 0 and no additional flags is sent
///       \Calls can_net_send()
TEST(CO_Sync, CoSyncTimer_NoIndMaxCntNotSet) {
  CreateObj1006AndSetPeriod(500u);
  SetCobid(DEV_ID | CO_SYNC_COBID_PRODUCER);
  SyncSetSendSetInd(CanSend::Func, nullptr);
  StartSYNC();
  const timespec tp = {0L, 600000L};

  const auto ret = can_net_set_time(net, &tp);

  CHECK_EQUAL(0, ret);
  CHECK_COMPARE(CanSend::GetNumCalled(), >, 0);
  CHECK_EQUAL(DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0u, CanSend::msg.flags);
  CHECK_EQUAL(0u, CanSend::msg.len);
  CHECK_EQUAL(0u, CanSend::msg.data[0]);
}

/// \Given a pointer to started producer SYNC service (co_sync_t), configured
///        with indication function, communication cycle period set to some
///        non-zero value, with counter overflow value set to 2
///
/// \When communication cycle period has passed twice
///
/// \Then indication function is called twice with a pointer to the SYNC service
///       and counter set to 1 and 2, two SYNC messages with data length of 1
///       and with counter values equal to 1 and 2, respectively, are sent after
///       first and second time communication cycle period has passed
///       \Calls can_net_send()
TEST(CO_Sync, CoSyncTimer_MaxCntSet) {
  CreateObj1006AndSetPeriod(500u);
  CreateObj1019AndSetCntOverflow(0x02u);
  SetCobid(DEV_ID | CO_SYNC_COBID_PRODUCER);
  SyncSetSendSetInd(CanSend::Func, SyncInd::func);
  StartSYNC();
  const timespec tp[2] = {{0L, 600000L}, {0L, 1200000L}};

  SyncInd::cnt = 2u;

  const auto ret = can_net_set_time(net, &tp[0]);

  CHECK_EQUAL(0, ret);
  CHECK(SyncInd::called);
  POINTERS_EQUAL(nullptr, SyncInd::data);
  CHECK_EQUAL(1, SyncInd::cnt);
  POINTERS_EQUAL(sync, SyncInd::sync);
  CHECK_COMPARE(CanSend::GetNumCalled(), >, 0);
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
  CHECK_COMPARE(CanSend::GetNumCalled(), >, 0);
  CHECK_EQUAL(DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0u, CanSend::msg.flags);
  CHECK_EQUAL(1u, CanSend::msg.len);
  CHECK_EQUAL(2u, CanSend::msg.data[0]);
}

/// \Given a pointer to started producer SYNC service (co_sync_t), configured
///        with indication function, the communication cycle period set to some
///        non-zero value, without counter overflow value in the 0x1019 object
///
/// \When communication cycle period has passed
///
/// \Then indication function is called with a pointer to the SYNC service and
///       counter set to 0;
///       SYNC message with data length of 0 and no additional flags is sent
///       \Calls can_net_send()
TEST(CO_Sync, CoSyncTimer) {
  CreateObj1006AndSetPeriod(500u);
  SetCobid(DEV_ID | CO_SYNC_COBID_PRODUCER);
  SyncSetSendSetInd(CanSend::Func, SyncInd::func);
  StartSYNC();
  const timespec tp = {0L, 600000L};

  const auto ret = can_net_set_time(net, &tp);

  CHECK_EQUAL(0, ret);
  CHECK(SyncInd::called);
  POINTERS_EQUAL(nullptr, SyncInd::data);
  CHECK_EQUAL(0, SyncInd::cnt);
  POINTERS_EQUAL(sync, SyncInd::sync);
  CHECK_COMPARE(CanSend::GetNumCalled(), >, 0);
  CHECK_EQUAL(DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0u, CanSend::msg.flags);
  CHECK_EQUAL(0u, CanSend::msg.len);
  CHECK_EQUAL(0u, CanSend::msg.data[0]);
}

///@}

TEST_GROUP_BASE(Co_SyncAllocation, CO_SyncBase) {
  co_sync_t* sync;
  Allocators::Limited limitedAllocator;

  TEST_SETUP() {
    TEST_BASE_SETUP();

    can_net_destroy(net);
    net = can_net_create(limitedAllocator.ToAllocT(), 0);

    dev_holder->CreateAndInsertObj(obj1005, 0x1005u);
  }

  TEST_TEARDOWN() {
    co_sync_destroy(sync);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_sync_create()
///@{

/// \Given pointers to the initialized device (co_dev_t) and network (co_net_t)
///        with memory allocator limited to 0 bytes
///
/// \When co_sync_create() is called with pointers to the network and the device
///
/// \Then null pointer is returned and SYNC service is not created
TEST(Co_SyncAllocation, CoSyncCreate_NoMoreMemory) {
  limitedAllocator.LimitAllocationTo(0u);

  sync = co_sync_create(net, dev);

  POINTERS_EQUAL(nullptr, sync);
}

/// \Given pointers to the initialized device (co_dev_t) and network (co_net_t)
///        with memory allocator limited to only create the SYNC service
///        instance
///
/// \When co_sync_create() is called with pointers to the network and the device
///
/// \Then null pointer is returned and SYNC service is not created
///       \Calls co_dev_find_obj()
///       \Calls can_recv_create()
TEST(Co_SyncAllocation, CoSyncCreate_MemoryOnlyForSyncT) {
  limitedAllocator.LimitAllocationTo(co_sync_sizeof());

  sync = co_sync_create(net, dev);

  POINTERS_EQUAL(nullptr, sync);
}

/// \Given pointers to the initialized device (co_dev_t) and network (co_net_t)
///        with memory allocator limited to create the SYNC service and
///        frame receiver (can_recv_t) instances
///
/// \When co_sync_create() is called with pointers to the network and the device
///
/// \Then null pointer is returned and SYNC service is not created
///       \Calls co_dev_find_obj()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
TEST(Co_SyncAllocation, CoSyncCreate_MemoryOnlyForSyncTAndRecv) {
  limitedAllocator.LimitAllocationTo(co_sync_sizeof() + can_recv_sizeof());

  sync = co_sync_create(net, dev);

  POINTERS_EQUAL(nullptr, sync);
}

/// \Given pointers to the initialized device (co_dev_t) and network (co_net_t)
///        with memory allocator limited to create the SYNC service and
///        timer (can_timer_t) instances
///
/// \When co_sync_create() is called with pointers to the network and the device
///
/// \Then null pointer is returned and SYNC service is not created
///       \Calls co_dev_find_obj()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
TEST(Co_SyncAllocation, CoSyncCreate_MemoryOnlyForSyncTAndTimer) {
  limitedAllocator.LimitAllocationTo(co_sync_sizeof() + can_timer_sizeof());

  sync = co_sync_create(net, dev);

  POINTERS_EQUAL(nullptr, sync);
}

/// \Given pointers to the initialized device (co_dev_t) and network (co_net_t)
///        with memory allocator limited to create the SYNC service, frame
///        receiver (can_recv_t) and timer (can_timer_t) instances
///
/// \When co_sync_create() is called with pointers to the network and the device
///
/// \Then a pointer to newly created SYNC service (co_sync_t) is returned
///       \Calls co_dev_find_obj()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
TEST(Co_SyncAllocation, CoSyncCreate_AllNecessaryMemoryAvailable) {
  limitedAllocator.LimitAllocationTo(co_sync_sizeof() + can_recv_sizeof() +
                                     can_timer_sizeof());

  sync = co_sync_create(net, dev);

  CHECK(sync != nullptr);
}

///@}
