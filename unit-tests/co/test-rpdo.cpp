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
#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/rpdo.h>
#include <lely/co/sdo.h>
#include <lely/util/error.h>

#include <libtest/allocators/default.hpp>
#include <libtest/allocators/limited.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/sub.hpp"

TEST_BASE(CO_RpdoBase) {
  TEST_BASE_SUPER(CO_RpdoBase);

  const co_unsigned8_t DEV_ID = 0x01u;
  const co_unsigned16_t RPDO_NUM = 0x0001u;

  can_net_t* net = nullptr;

  co_dev_t* dev = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1007;
  std::unique_ptr<CoObjTHolder> obj1400;
  std::unique_ptr<CoObjTHolder> obj1600;
  std::unique_ptr<CoObjTHolder> obj2000;

  Allocators::Default allocator;

  void CreateObj(std::unique_ptr<CoObjTHolder> & obj_holder,
                 co_unsigned16_t idx) {
    obj_holder.reset(new CoObjTHolder(idx));
    CHECK(obj_holder->Get() != nullptr);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));
  }

  // obj 0x1400, sub 0x00 - highest sub-index supported
  void SetComm00HighestSubidxSupported(const co_unsigned8_t max_subidx) {
    obj1400->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, max_subidx);
  }

  // obj 0x1400, sub 0x01 - COB-ID used by RPDO
  void SetComm01CobId(const co_unsigned32_t cobid) {
    obj1400->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32, cobid);
  }

  // obj 0x1400, sub 0x02 - transmission type
  void SetComm02TransmissionType(const co_unsigned8_t type) {
    obj1400->InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8, type);
  }

  void SetComm02SynchronousTransmission() { SetComm02TransmissionType(0x00); }
  void SetComm02EventDrivenTransmission() { SetComm02TransmissionType(0xfeu); }

  // obj 0x1400, sub 0x03 - inhibit time, in multiples of 100 microseconds
  void SetComm03InhibitTime(const co_unsigned16_t inhibit_time) {
    obj1400->InsertAndSetSub(0x03u, CO_DEFTYPE_UNSIGNED16, inhibit_time);
  }

  // obj 0x1400, sub 0x04 - compatibility entry, reserved and unused
  void SetComm04CompatibilityEntry(const co_unsigned8_t compat_entry) {
    obj1400->InsertAndSetSub(0x04u, CO_DEFTYPE_UNSIGNED8, compat_entry);
  }

  // obj 0x1400, sub 0x05 - event-timer, in milliseconds
  void SetComm05EventTimer(const co_unsigned16_t timer) {
    obj1400->InsertAndSetSub(0x05u, CO_DEFTYPE_UNSIGNED16, timer);
  }

  // obj 0x1400, sub 0x06 - SYNC start value, not used
  void SetComm06SyncStartValue(const co_unsigned8_t sync_start) {
    obj1400->InsertAndSetSub(0x06u, CO_DEFTYPE_UNSIGNED8, sync_start);
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(allocator.ToAllocT());
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);
  }

  TEST_TEARDOWN() {
    dev_holder.reset();
    can_net_destroy(net);
  }
};

TEST_GROUP_BASE(CO_RpdoCreate, CO_RpdoBase) {
  co_rpdo_t* rpdo = nullptr;

  TEST_TEARDOWN() {
    co_rpdo_destroy(rpdo);
    TEST_BASE_TEARDOWN();
  }
};

TEST(CO_RpdoCreate, CoRpdoDestroy_Null) { co_rpdo_destroy(nullptr); }

TEST(CO_RpdoCreate, CoRpdoCreate_MissingObject) {
  rpdo = co_rpdo_create(net, dev, RPDO_NUM);
  POINTERS_EQUAL(nullptr, rpdo);
}

TEST(CO_RpdoCreate, CoRpdoCreate_ZeroNum) {
  rpdo = co_rpdo_create(net, dev, 0);

  POINTERS_EQUAL(nullptr, rpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_RpdoCreate, CoRpdoCreate_InvalidNum) {
  rpdo = co_rpdo_create(net, dev, CO_NUM_PDOS + 1u);

  POINTERS_EQUAL(nullptr, rpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_RpdoCreate, CoRpdoCreate_NoRPDOParameters) {
  rpdo = co_rpdo_create(net, dev, RPDO_NUM);

  POINTERS_EQUAL(nullptr, rpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_RpdoCreate, CoRpdoCreate_NoRPDOMappingParamRecord) {
  CreateObj(obj1400, 0x1400u);

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);

  POINTERS_EQUAL(nullptr, rpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_RpdoCreate, CoRpdoCreate_NoRPDOCommParamRecord) {
  CreateObj(obj1600, 0x1600u);

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);

  POINTERS_EQUAL(nullptr, rpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_RpdoCreate, CoRpdoCreate_MinimalRPDO) {
  CreateObj(obj1400, 0x1400u);
  CreateObj(obj1600, 0x1600u);

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);

  CHECK(rpdo != nullptr);
  POINTERS_EQUAL(net, co_rpdo_get_net(rpdo));
  POINTERS_EQUAL(dev, co_rpdo_get_dev(rpdo));
  CHECK_EQUAL(RPDO_NUM, co_rpdo_get_num(rpdo));

  const auto* comm = co_rpdo_get_comm_par(rpdo);
  CHECK_EQUAL(0, comm->n);
  CHECK_EQUAL(0, comm->cobid);
  CHECK_EQUAL(0, comm->trans);
  CHECK_EQUAL(0, comm->inhibit);
  CHECK_EQUAL(0, comm->reserved);
  CHECK_EQUAL(0, comm->event);
  CHECK_EQUAL(0, comm->sync);

  const auto* map = co_rpdo_get_map_par(rpdo);
  CHECK_EQUAL(0, map->n);
  for (size_t i = 0; i < CO_PDO_NUM_MAPS; ++i) CHECK_EQUAL(0, map->map[i]);

  co_rpdo_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_rpdo_get_ind(rpdo, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(nullptr, pind);
  FUNCTIONPOINTERS_EQUAL(nullptr, pdata);

  co_rpdo_err_t* perr = nullptr;
  void* perrdata = nullptr;
  co_rpdo_get_err(rpdo, &perr, &perrdata);
  FUNCTIONPOINTERS_EQUAL(nullptr, perr);
  FUNCTIONPOINTERS_EQUAL(nullptr, perrdata);
}

TEST(CO_RpdoCreate, CoRpdoCreate_MinimalRPDOMaxNum) {
  const co_unsigned16_t MAX_RPDO_NUM = 0x0200u;

  CoObjTHolder obj15ff_holder(0x15ffu);
  CHECK(obj15ff_holder.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj15ff_holder.Take()));

  CoObjTHolder obj17ff_holder(0x17ffu);
  CHECK(obj17ff_holder.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj17ff_holder.Take()));

  rpdo = co_rpdo_create(net, dev, MAX_RPDO_NUM);

  CHECK(rpdo != nullptr);
  POINTERS_EQUAL(net, co_rpdo_get_net(rpdo));
  POINTERS_EQUAL(dev, co_rpdo_get_dev(rpdo));
  CHECK_EQUAL(MAX_RPDO_NUM, co_rpdo_get_num(rpdo));

  co_rpdo_destroy(rpdo);  // explicit destroyed before object holders
  rpdo = nullptr;
}

TEST(CO_RpdoCreate, CoRpdoStart_ExtendedFrame) {
  CreateObj(obj1400, 0x1400u);
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(CO_PDO_COBID_FRAME | DEV_ID);
  SetComm02SynchronousTransmission();

  CreateObj(obj1600, 0x1600u);

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);
  CHECK(rpdo != nullptr);
  CHECK_EQUAL(RPDO_NUM, co_rpdo_get_num(rpdo));

  const auto ret = co_rpdo_start(rpdo);
  CHECK_EQUAL(0, ret);

  const auto* comm = co_rpdo_get_comm_par(rpdo);
  CHECK_EQUAL(0x02u, comm->n);
  CHECK_EQUAL(CO_PDO_COBID_FRAME | DEV_ID, comm->cobid);
  CHECK_EQUAL(0, comm->trans);
  CHECK_EQUAL(0, comm->inhibit);
  CHECK_EQUAL(0, comm->reserved);
  CHECK_EQUAL(0, comm->event);
  CHECK_EQUAL(0, comm->sync);
}

TEST(CO_RpdoCreate, CoRpdoStart_AlreadyStarted) {
  CreateObj(obj1400, 0x1400u);
  CreateObj(obj1600, 0x1600u);

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);
  co_rpdo_start(rpdo);

  const auto ret = co_rpdo_start(rpdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_rpdo_is_stopped(rpdo));
}

TEST(CO_RpdoCreate, CoRpdoStart_InvalidBit) {
  CreateObj(obj1400, 0x1400u);
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(CO_PDO_COBID_VALID | DEV_ID);
  SetComm02SynchronousTransmission();

  CreateObj(obj1600, 0x1600u);

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);
  CHECK(rpdo != nullptr);
  CHECK_EQUAL(RPDO_NUM, co_rpdo_get_num(rpdo));

  const auto ret = co_rpdo_start(rpdo);
  CHECK_EQUAL(0, ret);

  const auto* comm = co_rpdo_get_comm_par(rpdo);
  CHECK_EQUAL(0x02u, comm->n);
  CHECK_EQUAL(CO_PDO_COBID_VALID | DEV_ID, comm->cobid);
  CHECK_EQUAL(0, comm->trans);
  CHECK_EQUAL(0, comm->inhibit);
  CHECK_EQUAL(0, comm->reserved);
  CHECK_EQUAL(0, comm->event);
  CHECK_EQUAL(0, comm->sync);
}

TEST(CO_RpdoCreate, CoRpdoCreate_FullRPDOCommParamRecord) {
  CreateObj(obj1400, 0x1400u);
  SetComm00HighestSubidxSupported(0x06u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x01u);
  SetComm03InhibitTime(0x0002u);
  SetComm04CompatibilityEntry(0x03u);
  SetComm05EventTimer(0x0004u);
  SetComm06SyncStartValue(0x05u);

  CreateObj(obj1600, 0x1600u);

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);
  CHECK(rpdo != nullptr);
  CHECK_EQUAL(RPDO_NUM, co_rpdo_get_num(rpdo));

  const auto ret = co_rpdo_start(rpdo);
  CHECK_EQUAL(0, ret);

  const auto* comm = co_rpdo_get_comm_par(rpdo);
  CHECK_EQUAL(0x06u, comm->n);
  CHECK_EQUAL(DEV_ID, comm->cobid);
  CHECK_EQUAL(0x01u, comm->trans);
  CHECK_EQUAL(0x0002u, comm->inhibit);
  CHECK_EQUAL(0x03u, comm->reserved);
  CHECK_EQUAL(0x0004u, comm->event);
  CHECK_EQUAL(0x05u, comm->sync);
}

TEST(CO_RpdoCreate, CoRpdoCreate_FullRPDOMappingParamRecord) {
  CreateObj(obj1400, 0x1400u);

  CreateObj(obj1600, 0x1600u);
  // 0x00 - number of mapped application objects in PDO
  obj1600->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8,
                           co_unsigned8_t(CO_PDO_NUM_MAPS));
  // 0x01-0x40 - application objects
  for (co_unsigned8_t i = 0x01u; i <= CO_PDO_NUM_MAPS; ++i) {
    obj1600->InsertAndSetSub(i, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t(i - 1));
  }

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);
  CHECK(rpdo != nullptr);
  CHECK_EQUAL(RPDO_NUM, co_rpdo_get_num(rpdo));

  const auto ret = co_rpdo_start(rpdo);
  CHECK_EQUAL(0, ret);

  const auto* map = co_rpdo_get_map_par(rpdo);
  CHECK_EQUAL(CO_PDO_NUM_MAPS, map->n);
  for (size_t i = 0; i < CO_PDO_NUM_MAPS; ++i) CHECK_EQUAL(i, map->map[i]);
}

TEST(CO_RpdoCreate, CoRpdoCreate_OversizedRPDOCommParamRecord) {
  CreateObj(obj1400, 0x1400u);

  SetComm00HighestSubidxSupported(0x07u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x01u);
  SetComm03InhibitTime(0x0002u);
  SetComm04CompatibilityEntry(0x03u);
  SetComm05EventTimer(0x0004u);
  SetComm06SyncStartValue(0x05u);

  // 0x07 - illegal sub-object
  obj1400->InsertAndSetSub(0x07u, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t(0));

  CreateObj(obj1600, 0x1600u);

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);
  CHECK(rpdo != nullptr);
  CHECK_EQUAL(RPDO_NUM, co_rpdo_get_num(rpdo));

  const auto ret = co_rpdo_start(rpdo);
  CHECK_EQUAL(0, ret);

  const auto* comm = co_rpdo_get_comm_par(rpdo);
  CHECK_EQUAL(0x07u, comm->n);
  CHECK_EQUAL(DEV_ID, comm->cobid);
  CHECK_EQUAL(0x01u, comm->trans);
  CHECK_EQUAL(0x0002u, comm->inhibit);
  CHECK_EQUAL(0x03u, comm->reserved);
  CHECK_EQUAL(0x0004u, comm->event);
  CHECK_EQUAL(0x05u, comm->sync);
}

TEST(CO_RpdoCreate, CoRpdoCreate_EventDrivenTransmission) {
  CreateObj(obj1400, 0x1400u);
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02EventDrivenTransmission();

  CreateObj(obj1600, 0x1600u);

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);
  CHECK(rpdo != nullptr);
  CHECK_EQUAL(RPDO_NUM, co_rpdo_get_num(rpdo));

  const auto ret = co_rpdo_start(rpdo);
  CHECK_EQUAL(0, ret);

  const auto* comm = co_rpdo_get_comm_par(rpdo);
  CHECK_EQUAL(0x02u, comm->n);
  CHECK_EQUAL(DEV_ID, comm->cobid);
  CHECK_EQUAL(0xfeu, comm->trans);
  CHECK_EQUAL(0, comm->inhibit);
  CHECK_EQUAL(0, comm->reserved);
  CHECK_EQUAL(0, comm->event);
  CHECK_EQUAL(0, comm->sync);
}

TEST(CO_RpdoCreate, CoRpdoCreate_TimerSet) {
  CreateObj(obj1400, 0x1400u);
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02SynchronousTransmission();

  CreateObj(obj1600, 0x1600u);

  CreateObj(obj1007, 0x1007u);
  // 0x00 - synchronous window length
  obj1007->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32,
                           co_unsigned32_t(0x00000001u));

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);
  CHECK(rpdo != nullptr);
  CHECK_EQUAL(RPDO_NUM, co_rpdo_get_num(rpdo));

  const auto ret = co_rpdo_start(rpdo);
  CHECK_EQUAL(0, ret);

  const auto* comm = co_rpdo_get_comm_par(rpdo);
  CHECK_EQUAL(0x02u, comm->n);
  CHECK_EQUAL(DEV_ID, comm->cobid);
  CHECK_EQUAL(0, comm->trans);
  CHECK_EQUAL(0, comm->inhibit);
  CHECK_EQUAL(0, comm->reserved);
  CHECK_EQUAL(0, comm->event);
  CHECK_EQUAL(0, comm->sync);
}

namespace CO_RpdoStatic {
static bool rpdo_ind_func_called = false;
static struct {
  co_rpdo_t* rpdo = nullptr;
  co_unsigned32_t ac = 0;
  const void* ptr = nullptr;
  size_t n = 0;
  void* data = nullptr;
} rpdo_ind_args;

static bool rpdo_err_func_called = false;
static struct {
  co_rpdo_t* rpdo = nullptr;
  co_unsigned16_t eec = 0;
  co_unsigned8_t er = 0;
  void* data = nullptr;
} rpdo_err_args;

static bool can_send_func_called = false;

static can_msg sent_msg = CAN_MSG_INIT;

}  // namespace CO_RpdoStatic

TEST_GROUP_BASE(CO_Rpdo, CO_RpdoBase) {
  co_rpdo_t* rpdo = nullptr;

  static void rpdo_ind_func(co_rpdo_t * pdo, co_unsigned32_t ac,
                            const void* ptr, size_t n, void* data) {
    CO_RpdoStatic::rpdo_ind_func_called = true;
    CO_RpdoStatic::rpdo_ind_args.rpdo = pdo;
    CO_RpdoStatic::rpdo_ind_args.ac = ac;
    CO_RpdoStatic::rpdo_ind_args.ptr = ptr;
    CO_RpdoStatic::rpdo_ind_args.n = n;
    CO_RpdoStatic::rpdo_ind_args.data = data;
  }
  static void rpdo_err_func(co_rpdo_t * pdo, co_unsigned16_t eec,
                            co_unsigned8_t er, void* data) {
    CO_RpdoStatic::rpdo_err_func_called = true;
    CO_RpdoStatic::rpdo_err_args.rpdo = pdo;
    CO_RpdoStatic::rpdo_err_args.eec = eec;
    CO_RpdoStatic::rpdo_err_args.er = er;
    CO_RpdoStatic::rpdo_err_args.data = data;
  }

  static int can_send_func(const struct can_msg* msg, void*) {
    CO_RpdoStatic::can_send_func_called = true;
    CO_RpdoStatic::sent_msg = *msg;
    return 0;
  }

  void CreateRpdo() {
    rpdo = co_rpdo_create(net, dev, RPDO_NUM);
    CHECK(rpdo != nullptr);
  }

  void StartRpdo() {
    CHECK_EQUAL(1, co_rpdo_is_stopped(rpdo));
    CHECK_EQUAL(0, co_rpdo_start(rpdo));
    CHECK_EQUAL(0, co_rpdo_is_stopped(rpdo));
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    CreateObj(obj1400, 0x1400u);
    CreateObj(obj1600, 0x1600u);

    CO_RpdoStatic::rpdo_ind_func_called = false;
    CO_RpdoStatic::rpdo_ind_args.rpdo = nullptr;
    CO_RpdoStatic::rpdo_ind_args.ac = 0;
    CO_RpdoStatic::rpdo_ind_args.ptr = nullptr;
    CO_RpdoStatic::rpdo_ind_args.n = 0;
    CO_RpdoStatic::rpdo_ind_args.data = nullptr;

    CO_RpdoStatic::rpdo_err_func_called = false;
    CO_RpdoStatic::rpdo_err_args.rpdo = nullptr;
    CO_RpdoStatic::rpdo_err_args.eec = 0;
    CO_RpdoStatic::rpdo_err_args.er = 0;
    CO_RpdoStatic::rpdo_err_args.data = nullptr;

    CO_RpdoStatic::can_send_func_called = false;
    CO_RpdoStatic::sent_msg = CAN_MSG_INIT;
  }

  TEST_TEARDOWN() {
    co_rpdo_destroy(rpdo);
    TEST_BASE_TEARDOWN();
  }
};

TEST(CO_Rpdo, CoRpdoGetInd_Null) {
  CreateRpdo();

  co_rpdo_get_ind(rpdo, nullptr, nullptr);
}

TEST(CO_Rpdo, CoRpdoSetInd) {
  int data = 0;
  CreateRpdo();

  co_rpdo_set_ind(rpdo, rpdo_ind_func, &data);

  co_rpdo_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_rpdo_get_ind(rpdo, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(rpdo_ind_func, pind);
  POINTERS_EQUAL(&data, pdata);
}

TEST(CO_Rpdo, CoRpdoGetErr_Null) {
  CreateRpdo();

  co_rpdo_get_err(rpdo, nullptr, nullptr);
}

TEST(CO_Rpdo, CoRpdoSetErr) {
  int data = 0;
  CreateRpdo();

  co_rpdo_set_err(rpdo, rpdo_err_func, &data);

  co_rpdo_err_t* perr = nullptr;
  void* pdata = nullptr;
  co_rpdo_get_err(rpdo, &perr, &pdata);
  FUNCTIONPOINTERS_EQUAL(rpdo_err_func, perr);
  POINTERS_EQUAL(&data, pdata);
}

TEST(CO_Rpdo, CoRpdoRtr_RPDONotValid) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(CO_PDO_COBID_VALID | DEV_ID);
  SetComm02SynchronousTransmission();

  CreateRpdo();
  StartRpdo();

  const auto ret = co_rpdo_rtr(rpdo);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Rpdo, CoRpdoRtr) {
  can_net_set_send_func(net, can_send_func, nullptr);

  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(CAN_MASK_EID);  // all bits set
  SetComm02SynchronousTransmission();

  CreateRpdo();
  StartRpdo();

  const auto ret = co_rpdo_rtr(rpdo);

  CHECK_EQUAL(0, ret);
  CHECK(CO_RpdoStatic::can_send_func_called);
  BITS_EQUAL(CAN_MASK_BID, CO_RpdoStatic::sent_msg.id, CAN_MASK_BID);
  BITS_EQUAL(CAN_FLAG_RTR, CO_RpdoStatic::sent_msg.flags, CAN_FLAG_RTR);
}

TEST(CO_Rpdo, CoRpdoRtr_ExtendedFrame) {
  can_net_set_send_func(net, can_send_func, nullptr);

  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(CAN_MASK_EID | CO_PDO_COBID_FRAME);
  SetComm02SynchronousTransmission();

  CreateRpdo();
  StartRpdo();

  const auto ret = co_rpdo_rtr(rpdo);

  CHECK_EQUAL(0, ret);
  CHECK(CO_RpdoStatic::can_send_func_called);
  BITS_EQUAL(CAN_MASK_EID, CO_RpdoStatic::sent_msg.id, CAN_MASK_EID);
  BITS_EQUAL(CAN_FLAG_RTR, CO_RpdoStatic::sent_msg.flags, CAN_FLAG_RTR);
  BITS_EQUAL(CAN_FLAG_IDE, CO_RpdoStatic::sent_msg.flags, CAN_FLAG_IDE);
}

TEST(CO_Rpdo, CoRpdoSync_CounterOverLimit) {
  CreateRpdo();

  const auto ret = co_rpdo_sync(rpdo, 0xffu);

  CHECK_EQUAL(-1, ret);
}

TEST(CO_Rpdo, CoRpdoSync_RPDONotValid) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(CO_PDO_COBID_VALID | DEV_ID);
  SetComm02SynchronousTransmission();

  CreateRpdo();
  StartRpdo();

  const auto ret = co_rpdo_sync(rpdo, 0x00u);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Rpdo, CoRpdoSync_TransmissionNotSynchronous) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xf1u);  // not synchronous

  CreateRpdo();
  StartRpdo();

  const auto ret = co_rpdo_sync(rpdo, 0x00u);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Rpdo, CoRpdoSync_NoFrame) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02SynchronousTransmission();

  CreateRpdo();

  const auto ret = co_rpdo_sync(rpdo, 0x00u);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Rpdo, CoRpdoSync_NoCallbacks) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02SynchronousTransmission();

  CreateRpdo();
  StartRpdo();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  const auto recv = can_net_recv(net, &msg);
  CHECK_EQUAL(0, recv);

  const auto ret = co_rpdo_sync(rpdo, 0x00u);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Rpdo, CoRpdoSync_WithCallbacks) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02SynchronousTransmission();

  CreateRpdo();
  int data = 0;
  co_rpdo_set_ind(rpdo, rpdo_ind_func, &data);
  co_rpdo_set_err(rpdo, rpdo_err_func, nullptr);
  StartRpdo();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  const auto recv = can_net_recv(net, &msg);
  CHECK_EQUAL(0, recv);

  const auto ret = co_rpdo_sync(rpdo, 0x00u);

  CHECK_EQUAL(0, ret);

  CHECK(CO_RpdoStatic::rpdo_ind_func_called);
  POINTERS_EQUAL(rpdo, CO_RpdoStatic::rpdo_ind_args.rpdo);
  CHECK_EQUAL(0, CO_RpdoStatic::rpdo_ind_args.ac);
  CHECK(CO_RpdoStatic::rpdo_ind_args.ptr != nullptr);
  CHECK_EQUAL(msg.len, CO_RpdoStatic::rpdo_ind_args.n);
  POINTERS_EQUAL(&data, CO_RpdoStatic::rpdo_ind_args.data);

  CHECK(!CO_RpdoStatic::rpdo_err_func_called);
}

TEST(CO_Rpdo, CoRpdoSync_BadMapping) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02SynchronousTransmission();

  // object 0x1600
  // 0x00 - number of mapped application objects in PDO
  obj1600->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x01u));

  // 0x01 - 1st application object (idx:0x2000 subidx:0x00 len:0x00)
  obj1600->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                           co_unsigned32_t(0x20000000u));

  CreateRpdo();
  int data = 0;
  co_rpdo_set_ind(rpdo, rpdo_ind_func, &data);
  co_rpdo_set_err(rpdo, rpdo_err_func, nullptr);
  StartRpdo();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  const auto recv = can_net_recv(net, &msg);
  CHECK_EQUAL(0, recv);

  const auto ret = co_rpdo_sync(rpdo, 0x00u);

  CHECK_EQUAL(-1, ret);

  CHECK(CO_RpdoStatic::rpdo_ind_func_called);
  POINTERS_EQUAL(rpdo, CO_RpdoStatic::rpdo_ind_args.rpdo);
  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, CO_RpdoStatic::rpdo_ind_args.ac);
  CHECK(CO_RpdoStatic::rpdo_ind_args.ptr != nullptr);
  CHECK_EQUAL(msg.len, CO_RpdoStatic::rpdo_ind_args.n);
  POINTERS_EQUAL(&data, CO_RpdoStatic::rpdo_ind_args.data);

  CHECK(!CO_RpdoStatic::rpdo_err_func_called);
}

TEST(CO_Rpdo, CoRpdoSync_BadMappingLength) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02SynchronousTransmission();

  // object 0x1600
  // 0x00 - number of mapped application objects in PDO
  obj1600->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x01u));

  // 0x01 - 1st application object (idx:0x2000 subidx:0x00 len:0x01)
  obj1600->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                           co_unsigned32_t(0x20000001u));

  // object 0x2000
  CreateObj(obj2000, 0x2000u);
  // 0x00
  obj2000->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));

  CreateRpdo();
  int ind_data = 0;
  co_rpdo_set_ind(rpdo, rpdo_ind_func, &ind_data);
  int err_data = 0;
  co_rpdo_set_err(rpdo, rpdo_err_func, &err_data);
  StartRpdo();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  const auto recv = can_net_recv(net, &msg);
  CHECK_EQUAL(0, recv);

  const auto ret = co_rpdo_sync(rpdo, 0x00u);

  CHECK_EQUAL(-1, ret);

  CHECK(CO_RpdoStatic::rpdo_ind_func_called);
  POINTERS_EQUAL(rpdo, CO_RpdoStatic::rpdo_ind_args.rpdo);
  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, CO_RpdoStatic::rpdo_ind_args.ac);
  CHECK(CO_RpdoStatic::rpdo_ind_args.ptr != nullptr);
  CHECK_EQUAL(msg.len, CO_RpdoStatic::rpdo_ind_args.n);
  POINTERS_EQUAL(&ind_data, CO_RpdoStatic::rpdo_ind_args.data);

  CHECK(CO_RpdoStatic::rpdo_err_func_called);
  POINTERS_EQUAL(rpdo, CO_RpdoStatic::rpdo_err_args.rpdo);
  CHECK_EQUAL(0x8210u, CO_RpdoStatic::rpdo_err_args.eec);
  CHECK_EQUAL(0x10, CO_RpdoStatic::rpdo_err_args.er);
  POINTERS_EQUAL(&err_data, CO_RpdoStatic::rpdo_err_args.data);
}

TEST(CO_Rpdo, CoRpdoSync_RPDOLengthExceedsMapping) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02SynchronousTransmission();

  // object 0x1600
  // 0x00 - number of mapped application objects in PDO
  obj1600->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x01u));

  // 0x01 - 1st application object (idx:0x2000 subidx:0x00 len:0x01)
  obj1600->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                           co_unsigned32_t(0x20000001u));

  CreateObj(obj2000, 0x2000u);
  // 0x00
  obj2000->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  co_sub_set_pdo_mapping(obj2000->GetLastSub(), 1);

  CreateRpdo();
  int ind_data = 0;
  co_rpdo_set_ind(rpdo, rpdo_ind_func, &ind_data);
  int err_data = 0;
  co_rpdo_set_err(rpdo, rpdo_err_func, &err_data);
  StartRpdo();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.len = CAN_MAX_LEN;
  const auto recv = can_net_recv(net, &msg);
  CHECK_EQUAL(0, recv);

  const auto ret = co_rpdo_sync(rpdo, 0x00u);

  CHECK_EQUAL(0, ret);

  CHECK(CO_RpdoStatic::rpdo_ind_func_called);
  POINTERS_EQUAL(rpdo, CO_RpdoStatic::rpdo_ind_args.rpdo);
  CHECK_EQUAL(0, CO_RpdoStatic::rpdo_ind_args.ac);
  CHECK(CO_RpdoStatic::rpdo_ind_args.ptr != nullptr);
  CHECK_EQUAL(msg.len, CO_RpdoStatic::rpdo_ind_args.n);
  POINTERS_EQUAL(&ind_data, CO_RpdoStatic::rpdo_ind_args.data);

  CHECK(CO_RpdoStatic::rpdo_err_func_called);
  POINTERS_EQUAL(rpdo, CO_RpdoStatic::rpdo_err_args.rpdo);
  CHECK_EQUAL(0x8220u, CO_RpdoStatic::rpdo_err_args.eec);
  CHECK_EQUAL(0x10, CO_RpdoStatic::rpdo_err_args.er);
  POINTERS_EQUAL(&err_data, CO_RpdoStatic::rpdo_err_args.data);
}

TEST(CO_Rpdo, CoRpdoRecv_ReservedTransmissionRPDO) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xf1u);  // reserved

  CreateRpdo();
  co_rpdo_set_ind(rpdo, rpdo_ind_func, nullptr);
  co_rpdo_set_err(rpdo, rpdo_err_func, nullptr);
  StartRpdo();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;

  const auto recv = can_net_recv(net, &msg);

  CHECK_EQUAL(0, recv);
  CHECK(!CO_RpdoStatic::rpdo_ind_func_called);
  CHECK(!CO_RpdoStatic::rpdo_err_func_called);
}

TEST(CO_Rpdo, CoRpdoRecv_EventDrivenRPDO) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02EventDrivenTransmission();

  CreateRpdo();
  int data = 0;
  co_rpdo_set_ind(rpdo, rpdo_ind_func, &data);
  co_rpdo_set_err(rpdo, rpdo_err_func, nullptr);
  StartRpdo();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;

  const auto recv = can_net_recv(net, &msg);

  CHECK_EQUAL(0, recv);

  CHECK(CO_RpdoStatic::rpdo_ind_func_called);
  POINTERS_EQUAL(rpdo, CO_RpdoStatic::rpdo_ind_args.rpdo);
  CHECK_EQUAL(0, CO_RpdoStatic::rpdo_ind_args.ac);
  CHECK(CO_RpdoStatic::rpdo_ind_args.ptr != nullptr);
  CHECK_EQUAL(msg.len, CO_RpdoStatic::rpdo_ind_args.n);
  POINTERS_EQUAL(&data, CO_RpdoStatic::rpdo_ind_args.data);

  CHECK(!CO_RpdoStatic::rpdo_err_func_called);
}

TEST(CO_Rpdo, CoRpdoRecv_ExpiredSyncWindow) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02SynchronousTransmission();

  CreateObj(obj1007, 0x1007u);
  // 0x00 - synchronous window length
  obj1007->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32,
                           co_unsigned32_t(0x00000001u));  // us

  CreateRpdo();
  co_rpdo_set_ind(rpdo, rpdo_ind_func, nullptr);
  co_rpdo_set_err(rpdo, rpdo_err_func, nullptr);
  StartRpdo();

  // start sync timer
  CHECK_EQUAL(0, co_rpdo_sync(rpdo, 0x00u));

  // expire sync window
  const timespec tp = {0, 1000u};
  const auto ret = can_net_set_time(net, &tp);
  CHECK_EQUAL(0, ret);

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  const auto recv = can_net_recv(net, &msg);
  CHECK_EQUAL(0, recv);

  CHECK_EQUAL(0, co_rpdo_sync(rpdo, 0x00u));

  // message was ignored as sync window had already expired when it was received
  CHECK(!CO_RpdoStatic::rpdo_ind_func_called);
  CHECK(!CO_RpdoStatic::rpdo_err_func_called);
}

TEST(CO_Rpdo, CoRpdoRecv_NoPDOInSyncWindow_NoErrFunc) {
  SetComm00HighestSubidxSupported(0x05u);
  SetComm01CobId(DEV_ID);
  SetComm02SynchronousTransmission();
  SetComm03InhibitTime(0x0000u);
  SetComm04CompatibilityEntry(0x00u);
  SetComm05EventTimer(0x0001u);

  CreateObj(obj1007, 0x1007u);
  // 0x00 - synchronous window length
  obj1007->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32,
                           co_unsigned32_t(0x00000001u));  // us

  CreateRpdo();
  co_rpdo_set_ind(rpdo, rpdo_ind_func, nullptr);
  StartRpdo();

  const timespec tp = {0, 1000000u};  // 1 ms
  const auto ret = can_net_set_time(net, &tp);
  CHECK_EQUAL(0, ret);

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;

  const auto recv = can_net_recv(net, &msg);

  CHECK_EQUAL(0, recv);
  CHECK(!CO_RpdoStatic::rpdo_ind_func_called);

  const timespec tp2 = {0, 2000000u};  // 2 ms
  const auto ret2 = can_net_set_time(net, &tp2);
  CHECK_EQUAL(0, ret2);
}

TEST(CO_Rpdo, CoRpdoRecv_NoPDOInSyncWindow) {
  SetComm00HighestSubidxSupported(0x05u);
  SetComm01CobId(DEV_ID);
  SetComm02SynchronousTransmission();
  SetComm03InhibitTime(0x0000u);
  SetComm04CompatibilityEntry(0x00u);
  SetComm05EventTimer(0x0001u);

  CreateObj(obj1007, 0x1007u);
  // 0x00 - synchronous window length
  obj1007->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32,
                           co_unsigned32_t(0x00000001u));  // us

  CreateRpdo();
  co_rpdo_set_ind(rpdo, rpdo_ind_func, nullptr);
  int data = 0;
  co_rpdo_set_err(rpdo, rpdo_err_func, &data);
  StartRpdo();

  const timespec tp = {0, 1000000u};  // 1 ms
  const auto ret = can_net_set_time(net, &tp);
  CHECK_EQUAL(0, ret);

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;

  const auto recv = can_net_recv(net, &msg);

  CHECK_EQUAL(0, recv);
  CHECK(!CO_RpdoStatic::rpdo_ind_func_called);
  CHECK(!CO_RpdoStatic::rpdo_err_func_called);

  const timespec tp2 = {0, 2000000u};  // 2 ms
  const auto ret2 = can_net_set_time(net, &tp2);
  CHECK_EQUAL(0, ret2);

  CHECK(CO_RpdoStatic::rpdo_err_func_called);
  POINTERS_EQUAL(rpdo, CO_RpdoStatic::rpdo_err_args.rpdo);
  CHECK_EQUAL(0x8250u, CO_RpdoStatic::rpdo_err_args.eec);
  CHECK_EQUAL(0x10, CO_RpdoStatic::rpdo_err_args.er);
  POINTERS_EQUAL(&data, CO_RpdoStatic::rpdo_err_args.data);
}

TEST_GROUP_BASE(CO_RpdoAllocation, CO_RpdoBase) {
  Allocators::Limited limitedAllocator;
  co_rpdo_t* rpdo = nullptr;

  void BasicConfiguration() {
    CreateObj(obj1400, 0x1400u);
    CreateObj(obj1600, 0x1600u);
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    can_net_destroy(net);
    net = can_net_create(limitedAllocator.ToAllocT());

    BasicConfiguration();
  }

  TEST_TEARDOWN() {
    co_rpdo_destroy(rpdo);
    TEST_BASE_TEARDOWN();
  }
};

TEST(CO_RpdoAllocation, CoRpdoCreate_NoMemoryAvailable) {
  limitedAllocator.LimitAllocationTo(0u);

  rpdo = co_rpdo_create(net, dev, DEV_ID);

  POINTERS_EQUAL(nullptr, rpdo);
}

TEST(CO_RpdoAllocation, CoRpdoCreate_MemoryOnlyForRpdo) {
  limitedAllocator.LimitAllocationTo(co_rpdo_sizeof());

  rpdo = co_rpdo_create(net, dev, DEV_ID);

  POINTERS_EQUAL(nullptr, rpdo);
}

TEST(CO_RpdoAllocation, CoRpdoCreate_MemoryOnlyForRpdoAndRecv) {
  limitedAllocator.LimitAllocationTo(co_rpdo_sizeof() + can_recv_sizeof());

  rpdo = co_rpdo_create(net, dev, DEV_ID);

  POINTERS_EQUAL(nullptr, rpdo);
}

TEST(CO_RpdoAllocation, CoRpdoCreate_MemoryOnlyForRpdoAndTimer) {
  limitedAllocator.LimitAllocationTo(co_rpdo_sizeof() + can_timer_sizeof());

  rpdo = co_rpdo_create(net, dev, DEV_ID);

  POINTERS_EQUAL(nullptr, rpdo);
}

TEST(CO_RpdoAllocation, CoRpdoCreate_MemoryOnlyForRpdoAndRecvAndSingleTimer) {
  limitedAllocator.LimitAllocationTo(co_rpdo_sizeof() + can_recv_sizeof() +
                                     can_timer_sizeof());

  rpdo = co_rpdo_create(net, dev, DEV_ID);

  POINTERS_EQUAL(nullptr, rpdo);
}

TEST(CO_RpdoAllocation, CoRpdoCreate_MemoryOnlyForRpdoAndTwoTimers) {
  limitedAllocator.LimitAllocationTo(co_rpdo_sizeof() + 2 * can_timer_sizeof());

  rpdo = co_rpdo_create(net, dev, DEV_ID);

  POINTERS_EQUAL(nullptr, rpdo);
}

TEST(CO_RpdoAllocation, CoRpdoCreate_AllNecessaryMemoryIsAvailable) {
  limitedAllocator.LimitAllocationTo(co_rpdo_sizeof() + can_recv_sizeof() +
                                     2 * can_timer_sizeof());

  rpdo = co_rpdo_create(net, dev, DEV_ID);

  CHECK(rpdo != nullptr);
}
