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
#include <lely/co/tpdo.h>
#include <lely/co/sdo.h>
#include <lely/util/errnum.h>

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/sub.hpp"

#include "allocators/heap.hpp"

#include "lely-cpputest-ext.hpp"
#include "lely-unit-test.hpp"

TEST_BASE(CO_TpdoBase) {
  TEST_BASE_SUPER(CO_TpdoBase);

  const co_unsigned8_t DEV_ID = 0x01u;
  const co_unsigned32_t CAN_ID = 0x000000ffu;
  const co_unsigned16_t TPDO_NUM = 0x0001u;

  can_net_t* net = nullptr;

  co_dev_t* dev = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1007;
  std::unique_ptr<CoObjTHolder> obj1800;
  std::unique_ptr<CoObjTHolder> obj1a00;
  std::unique_ptr<CoObjTHolder> obj2000;

  Allocators::HeapAllocator allocator;

  void CreateObjInDev(std::unique_ptr<CoObjTHolder> & obj_holder,
                      co_unsigned16_t idx) {
    obj_holder.reset(new CoObjTHolder(idx));
    CHECK(obj_holder->Get() != nullptr);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));
  }

  // obj 0x1800, sub 0x00 - highest sub-index supported
  void SetCommHighestSubidxSupported(co_unsigned8_t subidx) {
    obj1800->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, subidx);
  }

  // obj 0x1800, sub 0x01 contains COB-ID used by TPDO
  void SetCommCobid(co_unsigned32_t cobid) {
    obj1800->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32, cobid);
  }

  // obj 0x1800, sub 0x02 - transmission type
  void SetCommTransmissionType(co_unsigned8_t type) {
    obj1800->InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8, type);
  }

  // obj 0x1800, sub 0x03 - inhibit time
  void SetCommInhibitTime(co_unsigned16_t time) {
    obj1800->InsertAndSetSub(0x03u, CO_DEFTYPE_UNSIGNED16,
                             time);  // n*100 us
  }

  // obj 0x1800, sub 0x04 - reserved (compatibility entry)
  void SetCommCompatibilityEntry() {
    obj1800->InsertAndSetSub(0x04u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t(0x03u));
  }

  // 0bj 0x1800, sub 0x05 - event-timer
  void SetCommEventTimer(co_unsigned16_t time) {
    obj1800->InsertAndSetSub(0x05u, CO_DEFTYPE_UNSIGNED16,
                             time);  // ms
  }

  // obj 0x1800, sub 0x06 - SYNC start value
  void SetCommSyncStartValue(co_unsigned8_t val) {
    obj1800->InsertAndSetSub(0x06u, CO_DEFTYPE_UNSIGNED8, val);
  }

  // obj 0x1a00, sub 0x00 - number of mapped application objects in PDO
  void SetNumOfMappedAppObjs(co_unsigned8_t number) {
    obj1a00->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, number);
  }

  // obj 0x1a00 contains mappings; first mapping has a subidx 0x01
  void InsertInto1a00SubidxMapVal(co_unsigned8_t subidx,
                                  co_unsigned32_t mapping) {
    obj1a00->InsertAndSetSub(subidx, CO_DEFTYPE_UNSIGNED32, mapping);
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

TEST_GROUP_BASE(CO_TpdoInit, CO_TpdoBase) {
  const co_unsigned8_t CO_PDO_MAP_MAX_SUBIDX = 0x40u;

  co_tpdo_t* tpdo = nullptr;

  TEST_SETUP() {
    TEST_BASE_SETUP();
    tpdo = static_cast<co_tpdo_t*>(__co_tpdo_alloc(net));
    CHECK(tpdo != nullptr);
  }

  TEST_TEARDOWN() {
    __co_tpdo_free(tpdo);
    TEST_BASE_TEARDOWN();
  }
};

TEST(CO_TpdoInit, CoTpdoFree_Null) { __co_tpdo_free(nullptr); }

TEST(CO_TpdoInit, CoTpdoInit_ZeroNum) {
  const auto* ret = __co_tpdo_init(tpdo, net, dev, 0u);

  POINTERS_EQUAL(nullptr, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_TpdoInit, CoTpdoInit_InvalidNum) {
  const auto* ret = __co_tpdo_init(tpdo, net, dev, 513u);

  POINTERS_EQUAL(nullptr, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_TpdoInit, CoTpdoInit_NoTPDOParameters) {
  const auto* ret = __co_tpdo_init(tpdo, net, dev, TPDO_NUM);

  POINTERS_EQUAL(nullptr, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_TpdoInit, CoTpdoInit_NoTPDOMappingParamRecord) {
  CreateObjInDev(obj1800, 0x1800u);

  const auto* ret = __co_tpdo_init(tpdo, net, dev, TPDO_NUM);

  POINTERS_EQUAL(nullptr, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_TpdoInit, CoTpdoInit_NoTPDOCommParamRecord) {
  CreateObjInDev(obj1a00, 0x1a00u);

  const auto* ret = __co_tpdo_init(tpdo, net, dev, TPDO_NUM);

  POINTERS_EQUAL(nullptr, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_TpdoInit, CoTpdoInit_MinimalTPDO) {
  CreateObjInDev(obj1800, 0x1800u);
  CreateObjInDev(obj1a00, 0x1a00u);

  const auto* ret = __co_tpdo_init(tpdo, net, dev, TPDO_NUM);

  POINTERS_EQUAL(tpdo, ret);
  POINTERS_EQUAL(net, co_tpdo_get_net(tpdo));
  POINTERS_EQUAL(dev, co_tpdo_get_dev(tpdo));
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));

  const auto* comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0u, comm->n);
  CHECK_EQUAL(0u, comm->cobid);
  CHECK_EQUAL(0u, comm->trans);
  CHECK_EQUAL(0u, comm->inhibit);
  CHECK_EQUAL(0u, comm->reserved);
  CHECK_EQUAL(0u, comm->event);
  CHECK_EQUAL(0u, comm->sync);

  const auto* map = co_tpdo_get_map_par(tpdo);
  CHECK_EQUAL(0u, map->n);
  for (size_t i = 0; i < CO_PDO_MAP_MAX_SUBIDX; ++i)
    CHECK_EQUAL(0u, map->map[i]);

  co_tpdo_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_tpdo_get_ind(tpdo, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(nullptr, pind);
  FUNCTIONPOINTERS_EQUAL(nullptr, pdata);

  __co_tpdo_fini(tpdo);
}

TEST(CO_TpdoInit, CoTpdoInit_MinimalTPDOMaxNum) {
  const co_unsigned16_t MAX_TPDO_NUM = 0x0200u;

  CoObjTHolder obj19ff_holder(0x19ffu);
  CHECK(obj19ff_holder.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj19ff_holder.Take()));

  CoObjTHolder obj1bff_holder(0x1bffu);
  CHECK(obj1bff_holder.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1bff_holder.Take()));

  const auto* ret = __co_tpdo_init(tpdo, net, dev, MAX_TPDO_NUM);

  POINTERS_EQUAL(tpdo, ret);
  POINTERS_EQUAL(net, co_tpdo_get_net(tpdo));
  POINTERS_EQUAL(dev, co_tpdo_get_dev(tpdo));
  CHECK_EQUAL(MAX_TPDO_NUM, co_tpdo_get_num(tpdo));

  __co_tpdo_fini(tpdo);
}

TEST(CO_TpdoInit, CoTpdoInit_ExtendedFrame) {
  CreateObjInDev(obj1800, 0x1800u);
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID | CO_PDO_COBID_FRAME);
  SetCommTransmissionType(0x00u);

  CreateObjInDev(obj1a00, 0x1a00u);

  const auto* ret = __co_tpdo_init(tpdo, net, dev, TPDO_NUM);

  POINTERS_EQUAL(tpdo, ret);
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));

  const auto* comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0x02u, comm->n);
  CHECK_EQUAL(CAN_ID | CO_PDO_COBID_FRAME, comm->cobid);
  CHECK_EQUAL(0u, comm->trans);
  CHECK_EQUAL(0u, comm->inhibit);
  CHECK_EQUAL(0u, comm->reserved);
  CHECK_EQUAL(0u, comm->event);
  CHECK_EQUAL(0u, comm->sync);

  __co_tpdo_fini(tpdo);
}

TEST(CO_TpdoInit, CoTpdoInit_InvalidBit) {
  CreateObjInDev(obj1800, 0x1800u);
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID | CO_PDO_COBID_VALID);
  SetCommTransmissionType(0x00u);

  CreateObjInDev(obj1a00, 0x1a00u);

  const auto* ret = __co_tpdo_init(tpdo, net, dev, TPDO_NUM);

  POINTERS_EQUAL(tpdo, ret);
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));

  const auto* comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0x02u, comm->n);
  CHECK_EQUAL(CAN_ID | CO_PDO_COBID_VALID, comm->cobid);
  CHECK_EQUAL(0u, comm->trans);
  CHECK_EQUAL(0u, comm->inhibit);
  CHECK_EQUAL(0u, comm->reserved);
  CHECK_EQUAL(0u, comm->event);
  CHECK_EQUAL(0u, comm->sync);

  __co_tpdo_fini(tpdo);
}

TEST(CO_TpdoInit, CoTpdoInit_FullTPDOCommParamRecord) {
  CreateObjInDev(obj1800, 0x1800u);
  SetCommHighestSubidxSupported(0x06u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0x01u);
  SetCommInhibitTime(0x0002u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0004u);
  SetCommSyncStartValue(0x05u);

  CreateObjInDev(obj1a00, 0x1a00u);

  const auto* ret = __co_tpdo_init(tpdo, net, dev, TPDO_NUM);

  POINTERS_EQUAL(tpdo, ret);
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));

  const auto* comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0x06u, comm->n);
  CHECK_EQUAL(CAN_ID, comm->cobid);
  CHECK_EQUAL(0x01u, comm->trans);
  CHECK_EQUAL(0x0002u, comm->inhibit);
  CHECK_EQUAL(0x03u, comm->reserved);
  CHECK_EQUAL(0x0004u, comm->event);
  CHECK_EQUAL(0x05u, comm->sync);

  __co_tpdo_fini(tpdo);
}

TEST(CO_TpdoInit, CoTpdoInit_FullTPDOMappingParamRecord) {
  CreateObjInDev(obj1800, 0x1800u);

  CreateObjInDev(obj1a00, 0x1a00u);
  SetNumOfMappedAppObjs(CO_PDO_MAP_MAX_SUBIDX);
  // 0x01-0x40 - application objects
  for (co_unsigned8_t i = 0x01u; i <= CO_PDO_MAP_MAX_SUBIDX; ++i) {
    InsertInto1a00SubidxMapVal(i, i - 1u);
  }

  const auto* ret = __co_tpdo_init(tpdo, net, dev, TPDO_NUM);

  POINTERS_EQUAL(tpdo, ret);
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));

  const auto* map = co_tpdo_get_map_par(tpdo);
  CHECK_EQUAL(CO_PDO_MAP_MAX_SUBIDX, map->n);
  for (size_t i = 0; i < CO_PDO_MAP_MAX_SUBIDX; ++i)
    CHECK_EQUAL(i, map->map[i]);

  __co_tpdo_fini(tpdo);
}

TEST(CO_TpdoInit, CoTpdoInit_OversizedTPDOCommParamRecord) {
  CreateObjInDev(obj1800, 0x1800u);
  SetCommHighestSubidxSupported(0x07u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0x01u);
  SetCommInhibitTime(0x0002u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0004u);
  SetCommSyncStartValue(0x05u);

  // sub 0x07 - illegal sub-object
  obj1800->InsertAndSetSub(0x07u, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t(0));

  CreateObjInDev(obj1a00, 0x1a00u);

  const auto* ret = __co_tpdo_init(tpdo, net, dev, TPDO_NUM);

  POINTERS_EQUAL(tpdo, ret);
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));

  const auto* comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0x07u, comm->n);
  CHECK_EQUAL(CAN_ID, comm->cobid);
  CHECK_EQUAL(0x01u, comm->trans);
  CHECK_EQUAL(0x0002u, comm->inhibit);
  CHECK_EQUAL(0x03u, comm->reserved);
  CHECK_EQUAL(0x0004u, comm->event);
  CHECK_EQUAL(0x05u, comm->sync);

  __co_tpdo_fini(tpdo);
}

TEST(CO_TpdoInit, CoTpdoInit_EventDrivenTransmission) {
  CreateObjInDev(obj1800, 0x1800u);
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfeu);

  CreateObjInDev(obj1a00, 0x1a00u);

  const auto* ret = __co_tpdo_init(tpdo, net, dev, 1u);

  POINTERS_EQUAL(tpdo, ret);
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));

  const auto* comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0x02u, comm->n);
  CHECK_EQUAL(CAN_ID, comm->cobid);
  CHECK_EQUAL(0xfeu, comm->trans);
  CHECK_EQUAL(0u, comm->inhibit);
  CHECK_EQUAL(0u, comm->reserved);
  CHECK_EQUAL(0u, comm->event);
  CHECK_EQUAL(0u, comm->sync);

  __co_tpdo_fini(tpdo);
}

TEST(CO_TpdoInit, CoTpdoInit_TimerSet) {
  CreateObjInDev(obj1800, 0x1800u);
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0x00u);

  CreateObjInDev(obj1a00, 0x1a00u);

  CreateObjInDev(obj1007, 0x1007u);
  // 0x00 - synchronous window length
  obj1007->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32,
                           co_unsigned32_t(0x00000001u));

  const auto* ret = __co_tpdo_init(tpdo, net, dev, 1u);

  POINTERS_EQUAL(tpdo, ret);
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));

  const auto* comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0x02u, comm->n);
  CHECK_EQUAL(CAN_ID, comm->cobid);
  CHECK_EQUAL(0u, comm->trans);
  CHECK_EQUAL(0u, comm->inhibit);
  CHECK_EQUAL(0u, comm->reserved);
  CHECK_EQUAL(0u, comm->event);
  CHECK_EQUAL(0u, comm->sync);

  __co_tpdo_fini(tpdo);
}

TEST_GROUP_BASE(CO_TpdoCreate, CO_TpdoBase){};

TEST(CO_TpdoCreate, CoTpdoDestroy_Null) { co_tpdo_destroy(nullptr); }

TEST(CO_TpdoCreate, CoTpdoCreate) {
  CreateObjInDev(obj1800, 0x1800u);

  CreateObjInDev(obj1a00, 0x1a00u);

  auto* const tpdo = co_tpdo_create(net, dev, TPDO_NUM);
  CHECK(tpdo != nullptr);

  co_tpdo_destroy(tpdo);
}

TEST(CO_TpdoCreate, CoTpdoCreate_Error) {
  const auto* const tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  POINTERS_EQUAL(nullptr, tpdo);
}

TEST_GROUP_BASE(CO_Tpdo, CO_TpdoBase) {
  co_tpdo_t* tpdo = nullptr;

  static bool tpdo_ind_func_called;
  static bool can_send_func_called;
  static bool can_send_func_err_called;
  static void* can_send_func_data;
  static void* can_send_func_err_data;
  static can_msg sent_msg;

  static struct TpdoIndArgs {
    co_tpdo_t* tpdo;
    co_unsigned32_t ac;
    const void* ptr;
    size_t n;
    void* data;
  } tpdo_ind_args;

  int ind_data = 0;
  int can_data = 0;

  static void tpdo_ind_func(co_tpdo_t * pdo, co_unsigned32_t ac,
                            const void* ptr, size_t n, void* data) {
    tpdo_ind_func_called = true;
    tpdo_ind_args.tpdo = pdo;
    tpdo_ind_args.ac = ac;
    tpdo_ind_args.ptr = ptr;
    tpdo_ind_args.n = n;
    tpdo_ind_args.data = data;
  }

  static int can_send_func(const struct can_msg* msg, void* data) {
    can_send_func_called = true;
    sent_msg = *msg;
    can_send_func_data = data;

    return 0;
  }

  static int can_send_func_err(const struct can_msg* msg, void* data) {
    can_send_func_err_called = true;
    sent_msg = *msg;
    can_send_func_err_data = data;

    return -1;
  }

  void CreateTpdo() {
    tpdo = co_tpdo_create(net, dev, TPDO_NUM);
    CHECK(tpdo != nullptr);
    co_tpdo_set_ind(tpdo, tpdo_ind_func, &ind_data);
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    CreateObjInDev(obj1800, 0x1800u);
    CreateObjInDev(obj1a00, 0x1a00u);

    tpdo_ind_func_called = false;
    tpdo_ind_args.tpdo = nullptr;
    tpdo_ind_args.ac = 0;
    tpdo_ind_args.ptr = nullptr;
    tpdo_ind_args.n = 0;
    tpdo_ind_args.data = nullptr;

    can_send_func_called = false;
    can_send_func_data = nullptr;
    sent_msg = CAN_MSG_INIT;
    can_send_func_err_called = false;
    can_send_func_err_data = nullptr;

    can_net_set_send_func(net, can_send_func, &can_data);
  }

  TEST_TEARDOWN() {
    co_tpdo_destroy(tpdo);
    TEST_BASE_TEARDOWN();
  }
};
bool TEST_GROUP_CppUTestGroupCO_Tpdo::tpdo_ind_func_called = false;
bool TEST_GROUP_CppUTestGroupCO_Tpdo::can_send_func_called = false;
bool TEST_GROUP_CppUTestGroupCO_Tpdo::can_send_func_err_called = false;
void* TEST_GROUP_CppUTestGroupCO_Tpdo::can_send_func_data = nullptr;
void* TEST_GROUP_CppUTestGroupCO_Tpdo::can_send_func_err_data = nullptr;
can_msg TEST_GROUP_CppUTestGroupCO_Tpdo::sent_msg = CAN_MSG_INIT;
TEST_GROUP_CppUTestGroupCO_Tpdo::TpdoIndArgs
    TEST_GROUP_CppUTestGroupCO_Tpdo::tpdo_ind_args = {nullptr, 0u, nullptr, 0u,
                                                      nullptr};

TEST(CO_Tpdo, CoTpdoGetInd_Null) {
  CreateTpdo();

  co_tpdo_get_ind(tpdo, nullptr, nullptr);
}

TEST(CO_Tpdo, CoTpdoSetInd) {
  tpdo = co_tpdo_create(net, dev, TPDO_NUM);
  CHECK(tpdo != nullptr);

  int data = 0;
  co_tpdo_set_ind(tpdo, tpdo_ind_func, &data);

  co_tpdo_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_tpdo_get_ind(tpdo, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(tpdo_ind_func, pind);
  POINTERS_EQUAL(&data, pdata);
}

TEST(CO_Tpdo, CoTpdoEvent_CobidIsValid) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID | CO_PDO_COBID_VALID);

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(0, ret);
  CHECK(!tpdo_ind_func_called);
  CHECK(!can_send_func_called);
}

TEST(CO_Tpdo, CoTpdoEvent_TransmissionIsSynchronous) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0x01u);

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(0, ret);
  CHECK(!tpdo_ind_func_called);
  CHECK(!can_send_func_called);
}

TEST(CO_Tpdo, CoTpdoEvent_IsSynchronousButWindowExpired) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0x00u);
  SetCommEventTimer(0x0000u);

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(0, ret);
  CHECK(!tpdo_ind_func_called);
  CHECK(!can_send_func_called);
}

TEST(CO_Tpdo, CoTpdoEvent_NotSynchronousInitFrameSuccessNoRTR) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfdu);
  SetCommEventTimer(0x0000u);

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(0, ret);
  CHECK(!tpdo_ind_func_called);
  CHECK(!can_send_func_called);
}

TEST(CO_Tpdo, CoTpdoEvent_NotSynchronousInitFrameFail) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID | CO_PDO_COBID_FRAME | CO_PDO_COBID_RTR);
  SetCommTransmissionType(0xfdu);
  SetCommEventTimer(0x0000u);

  SetNumOfMappedAppObjs(0u);
  InsertInto1a00SubidxMapVal(0x01u, 0u);

  co_pdo_map_par map_par = CO_PDO_MAP_PAR_INIT;
  map_par.n = 0x01u;
  map_par.map[0] = 0x20200000u;
  CHECK_EQUAL(0u, co_dev_cfg_tpdo_map(dev, 1u, &map_par));

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(-1, ret);
  CHECK(!can_send_func_called);
  CHECK(tpdo_ind_func_called);
  POINTERS_EQUAL(&ind_data, tpdo_ind_args.data);
  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, tpdo_ind_args.ac);
  CHECK_EQUAL(0, tpdo_ind_args.n);
  POINTERS_EQUAL(nullptr, tpdo_ind_args.ptr);
  POINTERS_EQUAL(tpdo, tpdo_ind_args.tpdo);
}

TEST(CO_Tpdo, CoTpdoEvent_NotSynchronousInhibitTimeNotPassed) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfeu);
  SetCommInhibitTime(0x0010u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0000u);

  timespec ts = {0, 2000};
  CHECK_EQUAL(0, can_net_set_time(net, &ts));

  CreateTpdo();

  ts = {0, 1000};
  CHECK_EQUAL(0, can_net_set_time(net, &ts));

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_AGAIN, get_errnum());
  CHECK(!tpdo_ind_func_called);
  CHECK(!can_send_func_called);
}

TEST(CO_Tpdo, CoTpdoEvent_NotSynchronousInhibitTimePassedNoSendFunc) {
  can_net_set_send_func(net, nullptr, nullptr);
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfeu);
  SetCommInhibitTime(0x0010u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0000u);

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_NOSYS, get_errnum());
  CHECK(tpdo_ind_func_called);
  POINTERS_EQUAL(&ind_data, tpdo_ind_args.data);
  CHECK_EQUAL(CO_SDO_AC_ERROR, tpdo_ind_args.ac);
  CHECK_EQUAL(0, tpdo_ind_args.n);
  POINTERS_EQUAL(nullptr, tpdo_ind_args.ptr);
  POINTERS_EQUAL(tpdo, tpdo_ind_args.tpdo);
}

TEST(CO_Tpdo, CoTpdoEvent_EventDrivenTPDOInitFrameFailed) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfeu);
  SetCommInhibitTime(0x0010u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0000u);

  SetNumOfMappedAppObjs(0u);
  InsertInto1a00SubidxMapVal(0x01u, 0u);

  co_pdo_map_par map_par = CO_PDO_MAP_PAR_INIT;
  map_par.n = 0x01u;
  map_par.map[0] = 0x20200000u;
  CHECK_EQUAL(0u, co_dev_cfg_tpdo_map(dev, 1u, &map_par));

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(-1, ret);
  CHECK(!can_send_func_called);
  CHECK(tpdo_ind_func_called);
  POINTERS_EQUAL(&ind_data, tpdo_ind_args.data);
  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, tpdo_ind_args.ac);
  CHECK_EQUAL(0, tpdo_ind_args.n);
  POINTERS_EQUAL(nullptr, tpdo_ind_args.ptr);
  POINTERS_EQUAL(tpdo, tpdo_ind_args.tpdo);
}

TEST(CO_Tpdo, CoTpdoEvent_EventDrivenTPDOCANSendError) {
  int data = 0;
  can_net_set_send_func(net, can_send_func_err, &data);
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfeu);
  SetCommInhibitTime(0x0000u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0000u);

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(-1, ret);
  CHECK(can_send_func_err_called);
  POINTERS_EQUAL(&data, can_send_func_err_data);
  CHECK(tpdo_ind_func_called);
  POINTERS_EQUAL(&ind_data, tpdo_ind_args.data);
  CHECK_EQUAL(CO_SDO_AC_ERROR, tpdo_ind_args.ac);
  CHECK_EQUAL(0, tpdo_ind_args.n);
  POINTERS_EQUAL(nullptr, tpdo_ind_args.ptr);
  POINTERS_EQUAL(tpdo, tpdo_ind_args.tpdo);
}

TEST(CO_Tpdo, CoTpdoEvent_NotSynchronousInhibitTimeZero) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfeu);
  SetCommInhibitTime(0x0000u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0000u);

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(0, ret);
  timespec ts = {0u, 0u};
  co_tpdo_get_next(tpdo, &ts);
  CHECK(can_send_func_called);
  CHECK_EQUAL(&can_data, can_send_func_data);
  CHECK_EQUAL(CAN_ID, sent_msg.id);
  CHECK_EQUAL(0u, sent_msg.flags);
  CHECK_EQUAL(0u, sent_msg.len);
  CHECK_EQUAL(0u, ts.tv_nsec);
  CHECK_EQUAL(0u, ts.tv_sec);
  CHECK(tpdo_ind_func_called);
  POINTERS_EQUAL(&ind_data, tpdo_ind_args.data);
  CHECK_EQUAL(0, tpdo_ind_args.ac);
  CHECK_EQUAL(0, tpdo_ind_args.n);
  CHECK(tpdo_ind_args.ptr != nullptr);
  POINTERS_EQUAL(tpdo, tpdo_ind_args.tpdo);
}

TEST(CO_Tpdo, CoTpdoEvent_NotSynchronousInhibitTimePassed) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfeu);
  SetCommInhibitTime(0x0010u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0000u);

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(0, ret);
  timespec ts = {0u, 0u};
  co_tpdo_get_next(tpdo, &ts);
  CHECK(can_send_func_called);
  CHECK_EQUAL(&can_data, can_send_func_data);
  CHECK_EQUAL(CAN_ID, sent_msg.id);
  CHECK_EQUAL(0u, sent_msg.flags);
  CHECK_EQUAL(0u, sent_msg.len);
  CHECK_EQUAL(1600000, ts.tv_nsec);
  CHECK_EQUAL(0, ts.tv_sec);
  CHECK(tpdo_ind_func_called);
  POINTERS_EQUAL(&ind_data, tpdo_ind_args.data);
  CHECK_EQUAL(0, tpdo_ind_args.ac);
  CHECK_EQUAL(0, tpdo_ind_args.n);
  CHECK(tpdo_ind_args.ptr != nullptr);
  POINTERS_EQUAL(tpdo, tpdo_ind_args.tpdo);
}

TEST(CO_Tpdo, CoTpdoGetNext_TpIsNull) {
  CreateTpdo();

  co_tpdo_get_next(tpdo, nullptr);
}

TEST(CO_Tpdo, CoTpdoSync_CounterOverLimit) {
  CreateTpdo();

  const auto ret = co_tpdo_sync(tpdo, 0xffu);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK(!can_send_func_called);
  CHECK(!tpdo_ind_func_called);
}

TEST(CO_Tpdo, CoTpdoSync_CobidFlagInvalid) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID | CO_PDO_COBID_VALID);
  SetCommTransmissionType(0xf1u);

  CreateTpdo();

  const auto ret = co_tpdo_sync(tpdo, 0x00u);

  CHECK_EQUAL(0, ret);
  CHECK(!can_send_func_called);
  CHECK(!tpdo_ind_func_called);
}

TEST(CO_Tpdo, CoTpdoSync_TransmissionNotSynchronous) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xf1u);

  CreateTpdo();

  const auto ret = co_tpdo_sync(tpdo, 0x00u);

  CHECK_EQUAL(0, ret);
  CHECK(!can_send_func_called);
  CHECK(!tpdo_ind_func_called);
}

TEST(CO_Tpdo, CoTpdoSync_CounterIsZero) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfcu);
  SetCommInhibitTime(0x0000u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0001u);
  SetCommSyncStartValue(0x02u);

  CreateTpdo();

  const auto ret = co_tpdo_sync(tpdo, 0x00u);

  CHECK_EQUAL(0, ret);
  CHECK(!can_send_func_called);
  CHECK(!tpdo_ind_func_called);
}

TEST(CO_Tpdo, CoTpdoSync_AcyclicTPDOEventOccuredCANSendErr) {
  int data = 0;
  can_net_set_send_func(net, can_send_func_err, &data);
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0x00u);
  SetCommInhibitTime(0x0000u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0003u);
  SetCommSyncStartValue(0x02u);

  CreateTpdo();
  co_tpdo_event(tpdo);

  const auto ret = co_tpdo_sync(tpdo, 0x00u);

  CHECK_EQUAL(-1, ret);
  CHECK(can_send_func_err_called);
  POINTERS_EQUAL(&data, can_send_func_err_data);
  CHECK_EQUAL(CAN_ID, sent_msg.id);
  CHECK_EQUAL(0u, sent_msg.flags);
  CHECK_EQUAL(0u, sent_msg.len);
  CHECK(tpdo_ind_func_called);
  POINTERS_EQUAL(&ind_data, tpdo_ind_args.data);
  CHECK_EQUAL(CO_SDO_AC_ERROR, tpdo_ind_args.ac);
  CHECK_EQUAL(0, tpdo_ind_args.n);
  POINTERS_EQUAL(nullptr, tpdo_ind_args.ptr);
  POINTERS_EQUAL(tpdo, tpdo_ind_args.tpdo);
}

TEST(CO_Tpdo, CoTpdoSync_AcyclicTPDOEventNotOccured) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0x00u);
  SetCommInhibitTime(0x0000u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0001u);
  SetCommSyncStartValue(0x02u);

  CreateTpdo();

  const auto ret = co_tpdo_sync(tpdo, 0x00u);

  CHECK_EQUAL(0, ret);
  CHECK(!can_send_func_called);
  CHECK(!tpdo_ind_func_called);
}

TEST(CO_Tpdo, CoTpdoSync_CyclicTPDO) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0x02u);
  SetCommInhibitTime(0x0000u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0001u);
  SetCommSyncStartValue(0x02u);

  CreateTpdo();

  const auto ret = co_tpdo_sync(tpdo, 0x00u);

  CHECK_EQUAL(0, ret);
  CHECK(!can_send_func_called);
  CHECK(!tpdo_ind_func_called);

  const auto ret2 = co_tpdo_sync(tpdo, 0x00u);

  CHECK_EQUAL(0, ret2);
  CHECK(can_send_func_called);
  POINTERS_EQUAL(&can_data, can_send_func_data);
  CHECK_EQUAL(CAN_ID, sent_msg.id);
  CHECK_EQUAL(0u, sent_msg.flags);
  CHECK_EQUAL(0u, sent_msg.len);
  CHECK(tpdo_ind_func_called);
  POINTERS_EQUAL(&ind_data, tpdo_ind_args.data);
  CHECK_EQUAL(0, tpdo_ind_args.ac);
  CHECK_EQUAL(0, tpdo_ind_args.n);
  CHECK(tpdo_ind_args.ptr != nullptr);
  POINTERS_EQUAL(tpdo, tpdo_ind_args.tpdo);
}

TEST(CO_Tpdo, CoTpdoSync_PDOSyncNotEqualToCounter) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfcu);
  SetCommInhibitTime(0x0000u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0001u);
  SetCommSyncStartValue(0x02u);

  CreateTpdo();

  const auto ret = co_tpdo_sync(tpdo, 0x03u);

  CHECK_EQUAL(0, ret);
  CHECK(!can_send_func_called);
  CHECK(!tpdo_ind_func_called);
}

TEST(CO_Tpdo, CoTpdoSync_NoPDOSync) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfcu);
  SetCommInhibitTime(0x0000u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0001u);
  SetCommSyncStartValue(0x00u);

  CreateTpdo();

  const auto ret = co_tpdo_sync(tpdo, 0x03u);

  CHECK_EQUAL(0, ret);
  CHECK(!can_send_func_called);
  CHECK(!tpdo_ind_func_called);
}

TEST(CO_Tpdo, CoTpdoSync_SynchronousTPDODontSend) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfcu);
  SetCommInhibitTime(0x0000u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0001u);
  SetCommSyncStartValue(0x02u);

  CreateTpdo();

  const auto ret = co_tpdo_sync(tpdo, 0x02u);

  CHECK_EQUAL(0, ret);
  CHECK(!can_send_func_called);
  CHECK(!tpdo_ind_func_called);
}

TEST(CO_Tpdo, CoTpdoSync_SynchronousTPDOInitFrameFail) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfcu);
  SetCommInhibitTime(0x0000u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0001u);
  SetCommSyncStartValue(0x02u);

  SetNumOfMappedAppObjs(0u);
  InsertInto1a00SubidxMapVal(0x01u, 0u);

  co_pdo_map_par map_par = CO_PDO_MAP_PAR_INIT;
  map_par.n = 0x01u;
  map_par.map[0] = 0x20200000u;
  CHECK_EQUAL(0u, co_dev_cfg_tpdo_map(dev, 1u, &map_par));

  CreateTpdo();

  const auto ret = co_tpdo_sync(tpdo, 0x02u);

  CHECK_EQUAL(-1, ret);
  CHECK(!can_send_func_called);
  CHECK(tpdo_ind_func_called);
  POINTERS_EQUAL(&ind_data, tpdo_ind_args.data);
  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, tpdo_ind_args.ac);
  CHECK_EQUAL(0, tpdo_ind_args.n);
  POINTERS_EQUAL(nullptr, tpdo_ind_args.ptr);
  POINTERS_EQUAL(tpdo, tpdo_ind_args.tpdo);
}

TEST(CO_Tpdo, CoTpdoSync_PDOSyncEqualToCounterCANSendFail) {
  int data = 0;
  can_net_set_send_func(net, can_send_func_err, &data);
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0x01u);
  SetCommInhibitTime(0x0000u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0001u);
  SetCommSyncStartValue(0x02u);

  CreateTpdo();

  const auto ret = co_tpdo_sync(tpdo, 0x02u);

  CHECK_EQUAL(-1, ret);
  CHECK(can_send_func_err_called);
  POINTERS_EQUAL(&data, can_send_func_err_data);
  CHECK_EQUAL(CAN_ID, sent_msg.id);
  CHECK_EQUAL(0u, sent_msg.flags);
  CHECK_EQUAL(0u, sent_msg.len);
  CHECK(tpdo_ind_func_called);
  POINTERS_EQUAL(&ind_data, tpdo_ind_args.data);
  CHECK_EQUAL(CO_SDO_AC_ERROR, tpdo_ind_args.ac);
  CHECK_EQUAL(0, tpdo_ind_args.n);
  POINTERS_EQUAL(nullptr, tpdo_ind_args.ptr);
  POINTERS_EQUAL(tpdo, tpdo_ind_args.tpdo);
}

TEST(CO_Tpdo, CoTpdoRecv_SynchronousTPDOExtendedFrame) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID | CO_PDO_COBID_FRAME);
  SetCommTransmissionType(0xfcu);

  CreateTpdo();

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  msg.flags |= CAN_FLAG_RTR;
  msg.flags |= CAN_FLAG_IDE;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret);
  CHECK(tpdo_ind_func_called);
  POINTERS_EQUAL(&ind_data, tpdo_ind_args.data);
  CHECK_EQUAL(0, tpdo_ind_args.ac);
  CHECK_EQUAL(0, tpdo_ind_args.n);
  CHECK(tpdo_ind_args.ptr != nullptr);
  POINTERS_EQUAL(tpdo, tpdo_ind_args.tpdo);
  CHECK(can_send_func_called);
  CHECK_EQUAL(CAN_ID, sent_msg.id);
  CHECK_EQUAL(0u | CAN_FLAG_IDE, sent_msg.flags);
  CHECK_EQUAL(0u, sent_msg.len);
}

TEST(CO_Tpdo, CoTpdoRecv_SynchronousTPDOFrameUnavailable) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfcu);

  CreateTpdo();

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  msg.flags |= CAN_FLAG_RTR;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret);
  CHECK(tpdo_ind_func_called);
  POINTERS_EQUAL(&ind_data, tpdo_ind_args.data);
  CHECK_EQUAL(0, tpdo_ind_args.ac);
  CHECK_EQUAL(0, tpdo_ind_args.n);
  CHECK(tpdo_ind_args.ptr != nullptr);
  POINTERS_EQUAL(tpdo, tpdo_ind_args.tpdo);
  CHECK(can_send_func_called);
  POINTERS_EQUAL(&can_data, can_send_func_data);
  CHECK_EQUAL(CAN_ID, sent_msg.id);
  CHECK_EQUAL(0u, sent_msg.flags);
  CHECK_EQUAL(0u, sent_msg.len);
}

TEST(CO_Tpdo, CoTpdoRecv_SynchronousTPDOFrameAvailableTpdoIndIsNull) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfcu);

  CreateTpdo();
  co_tpdo_set_ind(tpdo, nullptr, nullptr);

  const int sync_ret = co_tpdo_sync(tpdo, 0x00u);

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  msg.flags |= CAN_FLAG_RTR;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, sync_ret);
  CHECK_EQUAL(0, ret);
  CHECK(can_send_func_called);
  POINTERS_EQUAL(&can_data, can_send_func_data);
  CHECK_EQUAL(CAN_ID, sent_msg.id);
  CHECK_EQUAL(0u, sent_msg.flags);
  CHECK_EQUAL(0u, sent_msg.len);
}

TEST(CO_Tpdo, CoTpdoRecv_EventDrivenTPDO) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfdu);

  SetNumOfMappedAppObjs(0u);
  InsertInto1a00SubidxMapVal(0x01u, 0u);

  CreateTpdo();

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  msg.flags |= CAN_FLAG_RTR;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret);
  CHECK(tpdo_ind_func_called);
  POINTERS_EQUAL(&ind_data, tpdo_ind_args.data);
  POINTERS_EQUAL(tpdo, tpdo_ind_args.tpdo);
  CHECK_EQUAL(0u, tpdo_ind_args.ac);
  CHECK(tpdo_ind_args.ptr != nullptr);
  CHECK_EQUAL(msg.len, tpdo_ind_args.n);
  POINTERS_EQUAL(&ind_data, tpdo_ind_args.data);
  CHECK(can_send_func_called);
  POINTERS_EQUAL(&can_data, can_send_func_data);
  CHECK_EQUAL(CAN_ID, sent_msg.id);
  CHECK_EQUAL(0u, sent_msg.flags);
  CHECK_EQUAL(0u, sent_msg.len);
  CHECK_EQUAL(0u, sent_msg.data[0]);
}

TEST(CO_Tpdo, CoTpdoRecv_EventDrivenTPDOFrameInitFailNoInd) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfdu);

  SetNumOfMappedAppObjs(0x01u);
  InsertInto1a00SubidxMapVal(0x01u, 0x202000f0u);

  CreateTpdo();
  co_tpdo_set_ind(tpdo, nullptr, nullptr);

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  msg.flags |= CAN_FLAG_RTR;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret);
  CHECK(!tpdo_ind_func_called);
  CHECK(!can_send_func_called);
}

TEST(CO_Tpdo, CoTpdoRecv_EventDrivenTPDOFrameInitFailInd) {
  SetCommHighestSubidxSupported(0x02u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0xfdu);

  SetNumOfMappedAppObjs(0x01u);
  InsertInto1a00SubidxMapVal(0x01u, 0x202000f0u);

  CreateTpdo();

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  msg.flags |= CAN_FLAG_RTR;

  const auto ret = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret);
  CHECK(tpdo_ind_func_called);
  POINTERS_EQUAL(&ind_data, tpdo_ind_args.data);
  POINTERS_EQUAL(tpdo, tpdo_ind_args.tpdo);
  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, tpdo_ind_args.ac);
  POINTERS_EQUAL(nullptr, tpdo_ind_args.ptr);
  CHECK(!can_send_func_called);
}

TEST(CO_Tpdo, CoTpdoRecv_NoPDOInSyncWindow_NoIndFunc) {
  SetCommHighestSubidxSupported(0x05u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0x00u);
  SetCommInhibitTime(0x0000u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0001u);

  // object 0x1007
  CoObjTHolder obj1007(0x1007u);
  CHECK(obj1007.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1007.Take()));

  // 0x00 - synchronous window length
  obj1007.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32,
                          co_unsigned32_t(0x00000001u));  // us

  CreateTpdo();
  co_tpdo_set_ind(tpdo, nullptr, nullptr);
  const timespec tp = {0, 1000000u};  // 1 ms

  const auto ret = can_net_set_time(net, &tp);
  CHECK_EQUAL(0, ret);

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  msg.flags = CAN_FLAG_RTR;

  const auto ret2 = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret2);
  CHECK(!tpdo_ind_func_called);
  CHECK(!can_send_func_called);
}

TEST(CO_Tpdo, CoTpdoRecv_NoPDOInSyncWindow) {
  SetCommHighestSubidxSupported(0x05u);
  SetCommCobid(CAN_ID);
  SetCommTransmissionType(0x00u);
  SetCommInhibitTime(0x0000u);
  SetCommCompatibilityEntry();
  SetCommEventTimer(0x0001u);

  CreateObjInDev(obj1007, 0x1007u);

  // 0x00 - synchronous window length
  obj1007->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32,
                           co_unsigned32_t(0x00000001u));  // us

  CreateTpdo();
  int data = 0;
  co_tpdo_set_ind(tpdo, tpdo_ind_func, &data);
  const timespec tp = {0, 1000000u};  // 1 ms

  const auto ret = can_net_set_time(net, &tp);
  CHECK_EQUAL(0, ret);

  can_msg msg = CAN_MSG_INIT;
  msg.id = CAN_ID;
  msg.flags |= CAN_FLAG_RTR;

  const auto ret2 = can_net_recv(net, &msg);

  CHECK_EQUAL(0, ret2);
  CHECK(tpdo_ind_func_called);
  POINTERS_EQUAL(&data, tpdo_ind_args.data);
  POINTERS_EQUAL(tpdo, tpdo_ind_args.tpdo);
  CHECK_EQUAL(CO_SDO_AC_TIMEOUT, tpdo_ind_args.ac);
  POINTERS_EQUAL(nullptr, tpdo_ind_args.ptr);
  CHECK(!can_send_func_called);

  const timespec tp2 = {0, 2000000u};  // 2 ms

  const auto ret3 = can_net_set_time(net, &tp2);
  CHECK_EQUAL(0, ret3);
  CHECK(!can_send_func_called);
}
