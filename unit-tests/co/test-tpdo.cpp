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
#include <lely/co/val.h>
#include <lely/util/error.h>

#include <libtest/allocators/default.hpp>
#include <libtest/allocators/limited.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/sub.hpp"

// TPDO indication function
struct CoTpdoInd {
  static bool called;

  static co_tpdo_t* pdo;
  static co_unsigned32_t ac;
  static const void* ptr;
  static size_t n;
  static void* data;

  static void
  func(co_tpdo_t* pdo_, co_unsigned32_t ac_, const void* ptr_, size_t n_,
       void* data_) {
    called = true;

    pdo = pdo_;
    ac = ac_;
    ptr = ptr_;
    n = n_;
    data = data_;
  }

  static void
  Clear() {
    called = false;

    pdo = nullptr;
    ac = 0;
    ptr = nullptr;
    n = 0;
    data = nullptr;
  }
};

bool CoTpdoInd::called = false;
co_tpdo_t* CoTpdoInd::pdo = nullptr;
co_unsigned32_t CoTpdoInd::ac = 0;
const void* CoTpdoInd::ptr = nullptr;
size_t CoTpdoInd::n = 0;
void* CoTpdoInd::data = nullptr;

// TPDO sample indication function
struct CoTpdoSampleInd {
  static bool called;
  static int ret;

  static co_tpdo_t* pdo;
  static void* data;

  static int
  func(co_tpdo_t* pdo_, void* data_) {
    called = true;

    pdo = pdo_;
    data = data_;

    return ret;
  }

  static void
  Clear() {
    called = false;
    ret = 0;

    pdo = nullptr;
    data = nullptr;
  }
};

bool CoTpdoSampleInd::called = false;
int CoTpdoSampleInd::ret = 0;
co_tpdo_t* CoTpdoSampleInd::pdo = nullptr;
void* CoTpdoSampleInd::data = nullptr;

TEST_BASE(CO_TpdoBase) {
  TEST_BASE_SUPER(CO_TpdoBase);
  Allocators::Default allocator;

  const co_unsigned8_t DEV_ID = 0x01u;
  const co_unsigned16_t TPDO_NUM = 0x0001u;

  can_net_t* net = nullptr;

  co_dev_t* dev = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1007;
  std::unique_ptr<CoObjTHolder> obj1800;
  std::unique_ptr<CoObjTHolder> obj1a00;
  std::unique_ptr<CoObjTHolder> obj2000;

  // PDO communication parameter record
  // obj 0x1800, sub 0x00 - highest sub-index supported
  void SetComm00HighestSubidxSupported(co_unsigned8_t subidx) {
    assert(subidx >= 0x02);
    obj1800->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, subidx);
  }

  // obj 0x1800, sub 0x01 contains COB-ID used by TPDO
  void SetComm01CobId(co_unsigned32_t cobid) {
    obj1800->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32, cobid);
  }

  // obj 0x1800, sub 0x02 - transmission type
  void SetComm02TransmissionType(co_unsigned8_t type) {
    obj1800->InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8, type);
  }

  // obj 0x1800, sub 0x03 - inhibit time
  void SetComm03InhibitTime(co_unsigned16_t inhibit_time) {
    obj1800->InsertAndSetSub(0x03u, CO_DEFTYPE_UNSIGNED16,
                             inhibit_time);  // n*100 us
  }

  // obj 0x1800, sub 0x04 - reserved (compatibility entry)
  void SetComm04CompatibilityEntry() {
    obj1800->InsertAndSetSub(0x04u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t(0x00u));
  }

  // 0bj 0x1800, sub 0x05 - event-timer
  void SetComm05EventTimer(co_unsigned16_t timer_time) {
    obj1800->InsertAndSetSub(0x05u, CO_DEFTYPE_UNSIGNED16,
                             timer_time);  // ms
  }

  // obj 0x1800, sub 0x06 - SYNC start value
  void SetComm06SyncStartValue(co_unsigned8_t start_val) {
    obj1800->InsertAndSetSub(0x06u, CO_DEFTYPE_UNSIGNED8, start_val);
  }

  // PDO mapping parameter record
  // obj 0x1a00, sub 0x00 - number of mapped application objects in TPDO
  void SetMapp00NumOfMappedAppObjs(co_unsigned8_t number) {
    obj1a00->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, number);
  }

  // obj 0x1a00, sub 0xNN - N-th application object; N must be > 0x00
  void SetMappNthAppObject(co_unsigned8_t subidx, co_unsigned32_t mapping) {
    assert(subidx > 0x00u);
    obj1a00->InsertAndSetSub(subidx, CO_DEFTYPE_UNSIGNED32, mapping);
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();

    net = can_net_create(allocator.ToAllocT(), 0);
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

TEST_GROUP_BASE(CO_TpdoCreate, CO_TpdoBase) {
  co_tpdo_t* tpdo = nullptr;

  TEST_TEARDOWN() {
    co_tpdo_destroy(tpdo);
    TEST_BASE_TEARDOWN();
  }
};

TEST(CO_TpdoCreate, CoTpdoDestroy_Null) { co_tpdo_destroy(nullptr); }

TEST(CO_TpdoCreate, CoTpdoCreate_Error) {
  tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  POINTERS_EQUAL(nullptr, tpdo);
}

TEST(CO_TpdoCreate, CoTpdoCreate_ZeroNum) {
  tpdo = co_tpdo_create(net, dev, 0u);

  POINTERS_EQUAL(nullptr, tpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_TpdoCreate, CoTpdoCreate_InvalidNum) {
  tpdo = co_tpdo_create(net, dev, CO_NUM_PDOS + 1u);

  POINTERS_EQUAL(nullptr, tpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_TpdoCreate, CoTpdoCreate_NoTPDOParameters) {
  tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  POINTERS_EQUAL(nullptr, tpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_TpdoCreate, CoTpdoCreate_NoTPDOMappingParamRecord) {
  dev_holder->CreateAndInsertObj(obj1800, 0x1800u);

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  POINTERS_EQUAL(nullptr, tpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_TpdoCreate, CoTpdoCreate_NoTPDOCommParamRecord) {
  dev_holder->CreateAndInsertObj(obj1a00, 0x1a00u);

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  POINTERS_EQUAL(nullptr, tpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_TpdoCreate, CoTpdoCreate_MinimalTPDO) {
  dev_holder->CreateAndInsertObj(obj1800, 0x1800u);
  dev_holder->CreateAndInsertObj(obj1a00, 0x1a00u);

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  CHECK(tpdo != nullptr);
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
  for (size_t i = 0; i < CO_PDO_NUM_MAPS; ++i) CHECK_EQUAL(0u, map->map[i]);

  co_tpdo_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_tpdo_get_ind(tpdo, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(nullptr, pind);
  FUNCTIONPOINTERS_EQUAL(nullptr, pdata);

  co_tpdo_sample_ind_t* psind = nullptr;
  void* psdata = nullptr;
  co_tpdo_get_sample_ind(tpdo, &psind, &pdata);
  CHECK(psind != nullptr);
  FUNCTIONPOINTERS_EQUAL(nullptr, psdata);
}

TEST(CO_TpdoCreate, CoTpdoCreate_MinimalTPDOMaxNum) {
  const co_unsigned16_t MAX_TPDO_NUM = 0x0200u;

  CoObjTHolder obj19ff_holder(0x19ffu);
  CHECK(obj19ff_holder.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj19ff_holder.Take()));

  CoObjTHolder obj1bff_holder(0x1bffu);
  CHECK(obj1bff_holder.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1bff_holder.Take()));

  tpdo = co_tpdo_create(net, dev, MAX_TPDO_NUM);

  CHECK(tpdo != nullptr);
  POINTERS_EQUAL(net, co_tpdo_get_net(tpdo));
  POINTERS_EQUAL(dev, co_tpdo_get_dev(tpdo));
  CHECK_EQUAL(MAX_TPDO_NUM, co_tpdo_get_num(tpdo));

  co_tpdo_destroy(tpdo);
  tpdo = nullptr;  // must be destroyed before objects
}

TEST(CO_TpdoCreate, CoTpdoStart_ExtendedFrame) {
  dev_holder->CreateAndInsertObj(obj1800, 0x1800u);
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID | CO_PDO_COBID_FRAME);
  SetComm02TransmissionType(0x00u);

  dev_holder->CreateAndInsertObj(obj1a00, 0x1a00u);

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  CHECK(tpdo != nullptr);
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));

  const auto ret = co_tpdo_start(tpdo);
  CHECK_EQUAL(0, ret);

  const auto* comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0x02u, comm->n);
  CHECK_EQUAL(DEV_ID | CO_PDO_COBID_FRAME, comm->cobid);
  CHECK_EQUAL(0u, comm->trans);
  CHECK_EQUAL(0u, comm->inhibit);
  CHECK_EQUAL(0u, comm->reserved);
  CHECK_EQUAL(0u, comm->event);
  CHECK_EQUAL(0u, comm->sync);
}

TEST(CO_TpdoCreate, CoTpdoStart_InvalidBit) {
  dev_holder->CreateAndInsertObj(obj1800, 0x1800u);
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID | CO_PDO_COBID_VALID);
  SetComm02TransmissionType(0x00u);

  dev_holder->CreateAndInsertObj(obj1a00, 0x1a00u);

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  CHECK(tpdo != nullptr);
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));

  const auto ret = co_tpdo_start(tpdo);
  CHECK_EQUAL(0, ret);

  const auto* comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0x02u, comm->n);
  CHECK_EQUAL(DEV_ID | CO_PDO_COBID_VALID, comm->cobid);
  CHECK_EQUAL(0u, comm->trans);
  CHECK_EQUAL(0u, comm->inhibit);
  CHECK_EQUAL(0u, comm->reserved);
  CHECK_EQUAL(0u, comm->event);
  CHECK_EQUAL(0u, comm->sync);
}

TEST(CO_TpdoCreate, CoTpdoStart_FullTPDOCommParamRecord) {
  dev_holder->CreateAndInsertObj(obj1800, 0x1800u);
  SetComm00HighestSubidxSupported(0x06u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x01u);
  SetComm03InhibitTime(0x0002u);
  SetComm04CompatibilityEntry();
  SetComm05EventTimer(0x0004u);
  SetComm06SyncStartValue(0x05u);

  dev_holder->CreateAndInsertObj(obj1a00, 0x1a00u);

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  CHECK(tpdo != nullptr);
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));

  CHECK_EQUAL(1, co_tpdo_is_stopped(tpdo));
  const auto ret = co_tpdo_start(tpdo);
  CHECK_EQUAL(0, ret);

  const auto* comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0x06u, comm->n);
  CHECK_EQUAL(DEV_ID, comm->cobid);
  CHECK_EQUAL(0x01u, comm->trans);
  CHECK_EQUAL(0x0002u, comm->inhibit);
  CHECK_EQUAL(0x00u, comm->reserved);
  CHECK_EQUAL(0x0004u, comm->event);
  CHECK_EQUAL(0x05u, comm->sync);
  CHECK_EQUAL(0, co_tpdo_is_stopped(tpdo));
}

TEST(CO_TpdoCreate, CoTpdoStart_FullTPDOMappingParamRecord) {
  dev_holder->CreateAndInsertObj(obj1800, 0x1800u);

  dev_holder->CreateAndInsertObj(obj1a00, 0x1a00u);
  SetMapp00NumOfMappedAppObjs(CO_PDO_NUM_MAPS);
  // 0x01-0x40 - application objects
  for (co_unsigned8_t i = 0x01u; i <= CO_PDO_NUM_MAPS; ++i) {
    SetMappNthAppObject(i, i - 1u);
  }

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);
  CHECK(tpdo != nullptr);
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));

  const auto ret = co_tpdo_start(tpdo);
  CHECK_EQUAL(0, ret);

  const auto* map = co_tpdo_get_map_par(tpdo);
  CHECK_EQUAL(CO_PDO_NUM_MAPS, map->n);
  for (size_t i = 0; i < CO_PDO_NUM_MAPS; ++i) CHECK_EQUAL(i, map->map[i]);
}

TEST(CO_TpdoCreate, CoTpdoStart_OversizedTPDOCommParamRecord) {
  dev_holder->CreateAndInsertObj(obj1800, 0x1800u);
  SetComm00HighestSubidxSupported(0x07u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x01u);
  SetComm03InhibitTime(0x0002u);
  SetComm04CompatibilityEntry();
  SetComm05EventTimer(0x0004u);
  SetComm06SyncStartValue(0x05u);

  // sub 0x07 - illegal sub-object
  obj1800->InsertAndSetSub(0x07u, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t(0));

  dev_holder->CreateAndInsertObj(obj1a00, 0x1a00u);

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);
  CHECK(tpdo != nullptr);
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));

  const auto ret = co_tpdo_start(tpdo);
  CHECK_EQUAL(0, ret);

  const auto* comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0x07u, comm->n);
  CHECK_EQUAL(DEV_ID, comm->cobid);
  CHECK_EQUAL(0x01u, comm->trans);
  CHECK_EQUAL(0x0002u, comm->inhibit);
  CHECK_EQUAL(0x00u, comm->reserved);
  CHECK_EQUAL(0x0004u, comm->event);
  CHECK_EQUAL(0x05u, comm->sync);
}

TEST(CO_TpdoCreate, CoTpdoStart_EventDrivenTransmission) {
  dev_holder->CreateAndInsertObj(obj1800, 0x1800u);
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xfeu);

  dev_holder->CreateAndInsertObj(obj1a00, 0x1a00u);

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);
  CHECK(tpdo != nullptr);
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));

  const auto ret = co_tpdo_start(tpdo);
  CHECK_EQUAL(0, ret);

  const auto* comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0x02u, comm->n);
  CHECK_EQUAL(DEV_ID, comm->cobid);
  CHECK_EQUAL(0xfeu, comm->trans);
  CHECK_EQUAL(0u, comm->inhibit);
  CHECK_EQUAL(0u, comm->reserved);
  CHECK_EQUAL(0u, comm->event);
  CHECK_EQUAL(0u, comm->sync);
}

TEST(CO_TpdoCreate, CoTpdoStart_AlreadyStarted) {
  dev_holder->CreateAndInsertObj(obj1800, 0x1800u);
  SetComm00HighestSubidxSupported(0x06u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x01u);
  SetComm03InhibitTime(0x0002u);
  SetComm04CompatibilityEntry();
  SetComm05EventTimer(0x0004u);
  SetComm06SyncStartValue(0x05u);
  dev_holder->CreateAndInsertObj(obj1a00, 0x1a00u);

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);
  co_tpdo_start(tpdo);

  const auto ret = co_tpdo_start(tpdo);
  CHECK_EQUAL(0, ret);
}

TEST(CO_TpdoCreate, CoTpdoStart_TimerSet) {
  dev_holder->CreateAndInsertObj(obj1800, 0x1800u);
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x00u);

  dev_holder->CreateAndInsertObj(obj1a00, 0x1a00u);

  dev_holder->CreateAndInsertObj(obj1007, 0x1007u);
  // 0x00 - synchronous window length
  obj1007->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32,
                           co_unsigned32_t(0x00000001u));

  tpdo = co_tpdo_create(net, dev, 1u);
  CHECK(tpdo != nullptr);
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));

  const auto ret = co_tpdo_start(tpdo);
  CHECK_EQUAL(0, ret);

  const auto* comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0x02u, comm->n);
  CHECK_EQUAL(DEV_ID, comm->cobid);
  CHECK_EQUAL(0u, comm->trans);
  CHECK_EQUAL(0u, comm->inhibit);
  CHECK_EQUAL(0u, comm->reserved);
  CHECK_EQUAL(0u, comm->event);
  CHECK_EQUAL(0u, comm->sync);
}

TEST_GROUP_BASE(CO_Tpdo, CO_TpdoBase) {
  co_tpdo_t* tpdo = nullptr;

  int ind_data = 0;
  int sind_data = 0;
  int can_data = 0;

  void CreateTpdo() {
    tpdo = co_tpdo_create(net, dev, TPDO_NUM);
    CHECK(tpdo != nullptr);
    CHECK_EQUAL(0, co_tpdo_start(tpdo));
    co_tpdo_set_ind(tpdo, CoTpdoInd::func, &ind_data);
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    dev_holder->CreateAndInsertObj(obj1800, 0x1800u);
    dev_holder->CreateAndInsertObj(obj1a00, 0x1a00u);

    CanSend::Clear();
    CoTpdoInd::Clear();
    CoTpdoSampleInd::Clear();

    can_net_set_send_func(net, CanSend::func, &can_data);
  }

  TEST_TEARDOWN() {
    co_tpdo_destroy(tpdo);
    TEST_BASE_TEARDOWN();
  }
};

TEST(CO_Tpdo, CoTpdoGetInd_Null) {
  CreateTpdo();

  co_tpdo_get_ind(tpdo, nullptr, nullptr);
}

TEST(CO_Tpdo, CoTpdoSetInd) {
  tpdo = co_tpdo_create(net, dev, TPDO_NUM);
  CHECK(tpdo != nullptr);

  int data = 0;
  co_tpdo_set_ind(tpdo, CoTpdoInd::func, &data);

  co_tpdo_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_tpdo_get_ind(tpdo, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(CoTpdoInd::func, pind);
  POINTERS_EQUAL(&data, pdata);
}

TEST(CO_Tpdo, CoTpdoGetSampleInd_Null) {
  CreateTpdo();

  co_tpdo_get_sample_ind(tpdo, nullptr, nullptr);
}

TEST(CO_Tpdo, CoTpdoSetSampleInd_Null) {
  CreateTpdo();

  co_tpdo_set_sample_ind(tpdo, nullptr, nullptr);

  co_tpdo_sample_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_tpdo_get_sample_ind(tpdo, &pind, &pdata);
  CHECK(pind != nullptr);
  POINTERS_EQUAL(nullptr, pdata);
}

TEST(CO_Tpdo, CoTpdoSetSampleInd) {
  CreateTpdo();

  int data = 0;
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::func, &data);

  co_tpdo_sample_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_tpdo_get_sample_ind(tpdo, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(CoTpdoSampleInd::func, pind);
  POINTERS_EQUAL(&data, pdata);
}

TEST(CO_Tpdo, CoTpdoEvent_InvalidCobId) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID | CO_PDO_COBID_VALID);
  SetComm02TransmissionType(0x00u);

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(0, ret);
  CHECK(!CoTpdoInd::called);
  CHECK(!CanSend::Called());
}

TEST(CO_Tpdo, CoTpdoEvent_AcyclicSynchronousTransmission) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x00u);

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(0, ret);
  CHECK(!CoTpdoInd::called);
  CHECK(!CanSend::Called());
}

TEST(CO_Tpdo, CoTpdoEvent_CyclicSynchronousTransmission) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x01u);

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(0, ret);
  CHECK(!CoTpdoInd::called);
  CHECK(!CanSend::Called());
}

TEST(CO_Tpdo, CoTpdoEvent_EventDrivenRTR_InitFrameSuccess) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID | CO_PDO_COBID_RTR);
  SetComm02TransmissionType(0xfdu);

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(0, ret);
  CHECK(!CoTpdoInd::called);
  CHECK(!CanSend::Called());
}

TEST(CO_Tpdo, CoTpdoEvent_EventDrivenRTR_InitFrameFail) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID | CO_PDO_COBID_FRAME | CO_PDO_COBID_RTR);
  SetComm02TransmissionType(0xfdu);

  SetMapp00NumOfMappedAppObjs(0x01u);
  SetMappNthAppObject(0x01u, 0xffff0000u);

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(-1, ret);
  CHECK(!CanSend::Called());

  CHECK(CoTpdoInd::called);
  POINTERS_EQUAL(tpdo, CoTpdoInd::pdo);
  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, CoTpdoInd::ac);
  POINTERS_EQUAL(nullptr, CoTpdoInd::ptr);
  CHECK_EQUAL(0, CoTpdoInd::n);
  POINTERS_EQUAL(&ind_data, CoTpdoInd::data);
}

TEST(CO_Tpdo, CoTpdoEvent_EventDriven_InhibitTimeNotPassed) {
  SetComm00HighestSubidxSupported(0x03u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xfeu);
  SetComm03InhibitTime(10u);  // 1 ms

  CreateTpdo();

  CHECK_EQUAL(0, co_tpdo_event(tpdo));
  CoTpdoInd::Clear();
  CanSend::Clear();

  const timespec ts = {0, 999999u};  // 0.999999 ms
  CHECK_EQUAL(0, can_net_set_time(net, &ts));

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_AGAIN, get_errnum());
  CHECK(!CoTpdoInd::called);
  CHECK(!CanSend::Called());
}

TEST(CO_Tpdo, CoTpdoEvent_EventDriven_InhibitTimePassedNoSendFunc) {
  can_net_set_send_func(net, nullptr, nullptr);

  SetComm00HighestSubidxSupported(0x03u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xfeu);
  SetComm03InhibitTime(10u);  // 1 ms

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_NOSYS, get_errnum());

  CHECK(CoTpdoInd::called);
  POINTERS_EQUAL(tpdo, CoTpdoInd::pdo);
  CHECK_EQUAL(CO_SDO_AC_ERROR, CoTpdoInd::ac);
  POINTERS_EQUAL(nullptr, CoTpdoInd::ptr);
  CHECK_EQUAL(0, CoTpdoInd::n);
  POINTERS_EQUAL(&ind_data, CoTpdoInd::data);
}

TEST(CO_Tpdo, CoTpdoEvent_EventDriven_InitFrameFailed) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xfeu);

  SetMapp00NumOfMappedAppObjs(0x01u);
  SetMappNthAppObject(0x01u, 0xffff0000u);

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(-1, ret);
  CHECK(!CanSend::Called());

  CHECK(CoTpdoInd::called);
  POINTERS_EQUAL(tpdo, CoTpdoInd::pdo);
  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, CoTpdoInd::ac);
  POINTERS_EQUAL(nullptr, CoTpdoInd::ptr);
  CHECK_EQUAL(0, CoTpdoInd::n);
  POINTERS_EQUAL(&ind_data, CoTpdoInd::data);
}

TEST(CO_Tpdo, CoTpdoEvent_EventDriven_SendFrameError) {
  CanSend::ret = -1;

  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xfeu);

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(-1, ret);
  CHECK(CanSend::Called());

  CHECK(CoTpdoInd::called);
  POINTERS_EQUAL(tpdo, CoTpdoInd::pdo);
  CHECK_EQUAL(CO_SDO_AC_ERROR, CoTpdoInd::ac);
  POINTERS_EQUAL(nullptr, CoTpdoInd::ptr);
  CHECK_EQUAL(0, CoTpdoInd::n);
  POINTERS_EQUAL(&ind_data, CoTpdoInd::data);
}

TEST(CO_Tpdo, CoTpdoEvent_EventDriven) {
  SetComm00HighestSubidxSupported(0x05u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xfeu);
  SetComm03InhibitTime(0);
  SetComm04CompatibilityEntry();
  SetComm05EventTimer(1);

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(0, ret);

  timespec ts = {0u, 0u};
  co_tpdo_get_next(tpdo, &ts);
  CHECK_EQUAL(0u, ts.tv_nsec);
  CHECK_EQUAL(0u, ts.tv_sec);

  CHECK(CanSend::Called());
  POINTERS_EQUAL(&can_data, CanSend::data);
  CHECK_EQUAL(DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0u, CanSend::msg.flags);
  CHECK_EQUAL(0u, CanSend::msg.len);

  CHECK(CoTpdoInd::called);
  POINTERS_EQUAL(tpdo, CoTpdoInd::pdo);
  CHECK_EQUAL(0, CoTpdoInd::ac);
  CHECK(CoTpdoInd::ptr != nullptr);
  CHECK_EQUAL(0, CoTpdoInd::n);
  POINTERS_EQUAL(&ind_data, CoTpdoInd::data);
}

TEST(CO_Tpdo, CoTpdoEvent_EventDriven_EventTimer) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xffu);
  SetComm03InhibitTime(0);
  SetComm04CompatibilityEntry();
  SetComm05EventTimer(1);  // 1 ms

  CreateTpdo();

  CHECK_EQUAL(0, co_tpdo_event(tpdo));
  CHECK(CanSend::Called());
  CHECK(CoTpdoInd::called);
  CHECK_EQUAL(0, CoTpdoInd::ac);

  CanSend::Clear();
  CoTpdoInd::Clear();

  const timespec ts = {0, 1000000u};  // 1 ms
  CHECK_EQUAL(0, can_net_set_time(net, &ts));

  CHECK(CanSend::Called());
  POINTERS_EQUAL(&can_data, CanSend::data);

  CHECK(CoTpdoInd::called);
  POINTERS_EQUAL(tpdo, CoTpdoInd::pdo);
  CHECK_EQUAL(0, CoTpdoInd::ac);
  CHECK(CoTpdoInd::ptr != nullptr);
  CHECK_EQUAL(0, CoTpdoInd::n);
  POINTERS_EQUAL(&ind_data, CoTpdoInd::data);
}

TEST(CO_Tpdo, CoTpdoEvent_EventDriven_EventTimer_InhibitTimeNotElapsed) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xffu);
  SetComm03InhibitTime(11u);
  SetComm04CompatibilityEntry();
  SetComm05EventTimer(1);  // 1 ms

  CreateTpdo();

  CHECK_EQUAL(0, co_tpdo_event(tpdo));
  CHECK(CanSend::Called());
  CHECK(CoTpdoInd::called);
  CHECK_EQUAL(0, CoTpdoInd::ac);

  CanSend::Clear();
  CoTpdoInd::Clear();

  const timespec ts = {0, 1000000u};  // 1 ms
  CHECK_EQUAL(0, can_net_set_time(net, &ts));

  CHECK(!CanSend::Called());
  CHECK(!CoTpdoInd::called);
}

TEST(CO_Tpdo, CoTpdoGetNext_Null) {
  CreateTpdo();

  co_tpdo_get_next(tpdo, nullptr);
}

TEST(CO_Tpdo, CoTpdoSync_CounterOverLimit) {
  CreateTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::func, &sind_data);

  const auto ret = co_tpdo_sync(tpdo, 241);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK(!CoTpdoSampleInd::called);
}

TEST(CO_Tpdo, CoTpdoSync_InvalidCobId) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID | CO_PDO_COBID_VALID);
  SetComm02TransmissionType(0xf1u);

  CreateTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::func, &sind_data);

  const auto ret = co_tpdo_sync(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK(!CoTpdoSampleInd::called);
}

TEST(CO_Tpdo, CoTpdoSync_EventDrivenTransmission) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xffu);

  CreateTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::func, &sind_data);

  const auto ret = co_tpdo_sync(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK(!CoTpdoSampleInd::called);
}

TEST(CO_Tpdo, CoTpdoSync_SyncStartValue_CntZero) {
  SetComm00HighestSubidxSupported(0x06u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x01u);
  SetComm03InhibitTime(0);
  SetComm04CompatibilityEntry();
  SetComm05EventTimer(0);
  SetComm06SyncStartValue(2u);

  CreateTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::func, &sind_data);

  const auto ret = co_tpdo_sync(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK(CoTpdoSampleInd::called);
  POINTERS_EQUAL(tpdo, CoTpdoSampleInd::pdo);
  POINTERS_EQUAL(&sind_data, CoTpdoSampleInd::data);
}

TEST(CO_Tpdo, CoTpdoSync_SyncStartValue_CntNotEquals) {
  SetComm00HighestSubidxSupported(0x06u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x01u);
  SetComm03InhibitTime(0);
  SetComm04CompatibilityEntry();
  SetComm05EventTimer(0);
  SetComm06SyncStartValue(2u);

  CreateTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::func, &sind_data);

  const auto ret = co_tpdo_sync(tpdo, 1u);

  CHECK_EQUAL(0, ret);
  CHECK(!CoTpdoSampleInd::called);
}

TEST(CO_Tpdo, CoTpdoSync_SyncStartValue_CntEquals) {
  SetComm00HighestSubidxSupported(0x06u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x01u);
  SetComm03InhibitTime(0);
  SetComm04CompatibilityEntry();
  SetComm05EventTimer(0);
  SetComm06SyncStartValue(2u);

  CreateTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::func, &sind_data);

  const auto ret = co_tpdo_sync(tpdo, 2u);

  CHECK_EQUAL(0, ret);
  CHECK(CoTpdoSampleInd::called);
  POINTERS_EQUAL(tpdo, CoTpdoSampleInd::pdo);
  POINTERS_EQUAL(&sind_data, CoTpdoSampleInd::data);
}

TEST(CO_Tpdo, CoTpdoSync_SyncRTR_StartSyncWindowTimer) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xfcu);

  // object 0x1007
  CoObjTHolder obj1007(0x1007u);
  CHECK(obj1007.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1007.Take()));

  // 0x00 - synchronous window length
  obj1007.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32,
                          co_unsigned32_t(1000));  // us

  CreateTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::func, &sind_data);
  co_tpdo_event(tpdo);

  const auto ret = co_tpdo_sync(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK(CoTpdoSampleInd::called);
  POINTERS_EQUAL(tpdo, CoTpdoSampleInd::pdo);
  POINTERS_EQUAL(&sind_data, CoTpdoSampleInd::data);
}

TEST(CO_Tpdo, CoTpdoSync_SyncAcyclic_Event) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x00u);

  CreateTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::func, &sind_data);
  co_tpdo_event(tpdo);

  const auto ret = co_tpdo_sync(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK(CoTpdoSampleInd::called);
  POINTERS_EQUAL(tpdo, CoTpdoSampleInd::pdo);
  POINTERS_EQUAL(&sind_data, CoTpdoSampleInd::data);
}

TEST(CO_Tpdo, CoTpdoSync_SyncAcyclic_NoEvent) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x00u);

  CreateTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::func, &sind_data);

  const auto ret = co_tpdo_sync(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK(!CoTpdoSampleInd::called);
}

TEST(CO_Tpdo, CoTpdoSync_SyncCyclicNoSample) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x02u);

  CreateTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::func, &sind_data);

  const auto ret = co_tpdo_sync(tpdo, 1);

  CHECK_EQUAL(0, ret);
  CHECK(!CoTpdoSampleInd::called);
}

TEST(CO_Tpdo, CoTpdoSampleRes_InvalidPDO) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID | CO_PDO_COBID_VALID);
  SetComm02TransmissionType(0x00u);

  CreateTpdo();

  const auto ret = co_tpdo_sample_res(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK(!CoTpdoInd::called);
  CHECK(!CanSend::Called());
}

TEST(CO_Tpdo, CoTpdoSampleRes_EventDriven) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xffu);

  CreateTpdo();

  const auto ret = co_tpdo_sample_res(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK(!CoTpdoInd::called);
  CHECK(!CanSend::Called());
}

TEST(CO_Tpdo, CoTpdoSampleRes_ACErrorArg) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xfcu);

  CreateTpdo();

  const auto ret = co_tpdo_sample_res(tpdo, CO_UNSIGNED32_MAX);

  CHECK_EQUAL(0, ret);
  CHECK(!CanSend::Called());

  CHECK(CoTpdoInd::called);
  CHECK_EQUAL(tpdo, CoTpdoInd::pdo);
  CHECK_EQUAL(CO_UNSIGNED32_MAX, CoTpdoInd::ac);
  POINTERS_EQUAL(nullptr, CoTpdoInd::ptr);
  CHECK_EQUAL(0, CoTpdoInd::n);
  POINTERS_EQUAL(&ind_data, CoTpdoInd::data);
}

TEST(CO_Tpdo, CoTpdoSampleRes_ACErrorArg_NoIndFunc) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x00u);

  CreateTpdo();
  co_tpdo_set_ind(tpdo, nullptr, nullptr);

  const auto ret = co_tpdo_sample_res(tpdo, CO_UNSIGNED32_MAX);

  CHECK_EQUAL(0, ret);
  CHECK(!CanSend::Called());
}

TEST(CO_Tpdo, CoTpdoSampleRes_SyncWindowTimeout) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x01u);

  // object 0x1007
  CoObjTHolder obj1007(0x1007u);
  CHECK(obj1007.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1007.Take()));

  // 0x00 - synchronous window length
  obj1007.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32,
                          co_unsigned32_t(1000u));  // us

  CreateTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::func, &sind_data);
  co_tpdo_event(tpdo);

  CHECK_EQUAL(0, co_tpdo_sync(tpdo, 0));
  CHECK(CoTpdoSampleInd::called);
  POINTERS_EQUAL(tpdo, CoTpdoSampleInd::pdo);
  POINTERS_EQUAL(&sind_data, CoTpdoSampleInd::data);

  const timespec ts = {0, 1000000u};  // 1 ms
  CHECK_EQUAL(0, can_net_set_time(net, &ts));

  const auto ret = co_tpdo_sample_res(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK(!CanSend::Called());

  CHECK(CoTpdoInd::called);
  POINTERS_EQUAL(tpdo, CoTpdoInd::pdo);
  CHECK_EQUAL(CO_SDO_AC_TIMEOUT, CoTpdoInd::ac);
  POINTERS_EQUAL(nullptr, CoTpdoInd::ptr);
  CHECK_EQUAL(0, CoTpdoInd::n);
  POINTERS_EQUAL(&ind_data, CoTpdoInd::data);
}

TEST(CO_Tpdo, CoTpdoSampleRes_InitFrameFail) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x00u);

  SetMapp00NumOfMappedAppObjs(0x01u);
  SetMappNthAppObject(0x01u, 0xffff0000u);

  CreateTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::func, nullptr);
  CHECK_EQUAL(0, co_tpdo_sync(tpdo, 0));

  const auto ret = co_tpdo_sample_res(tpdo, 0);

  CHECK_EQUAL(-1, ret);
  CHECK(!CanSend::Called());

  CHECK(CoTpdoInd::called);
  POINTERS_EQUAL(tpdo, CoTpdoInd::pdo);
  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, CoTpdoInd::ac);
  POINTERS_EQUAL(nullptr, CoTpdoInd::ptr);
  CHECK_EQUAL(0, CoTpdoInd::n);
  POINTERS_EQUAL(&ind_data, CoTpdoInd::data);
}

TEST(CO_Tpdo, CoTpdoSampleRes_CanSendError) {
  CanSend::ret = -1;

  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x00u);

  CreateTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::func, nullptr);
  CHECK_EQUAL(0, co_tpdo_sync(tpdo, 0));

  const auto ret = co_tpdo_sample_res(tpdo, 0);

  CHECK_EQUAL(-1, ret);

  CHECK(CanSend::Called());
  POINTERS_EQUAL(&can_data, CanSend::data);

  CHECK(CoTpdoInd::called);
  POINTERS_EQUAL(tpdo, CoTpdoInd::pdo);
  CHECK_EQUAL(CO_SDO_AC_ERROR, CoTpdoInd::ac);
  POINTERS_EQUAL(nullptr, CoTpdoInd::ptr);
  CHECK_EQUAL(0, CoTpdoInd::n);
  POINTERS_EQUAL(&ind_data, CoTpdoInd::data);
}

TEST(CO_Tpdo, CoTpdoSampleRes) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x00u);

  CreateTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::func, nullptr);
  CHECK_EQUAL(0, co_tpdo_sync(tpdo, 0));

  const auto ret = co_tpdo_sample_res(tpdo, 0);

  CHECK_EQUAL(0, ret);

  CHECK(CanSend::Called());
  POINTERS_EQUAL(&can_data, CanSend::data);
  CHECK_EQUAL(DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0u, CanSend::msg.flags);
  CHECK_EQUAL(0u, CanSend::msg.len);

  CHECK(CoTpdoInd::called);
  POINTERS_EQUAL(tpdo, CoTpdoInd::pdo);
  CHECK_EQUAL(0, CoTpdoInd::ac);
  CHECK(CoTpdoInd::ptr != nullptr);
  CHECK_EQUAL(0, CoTpdoInd::n);
  POINTERS_EQUAL(&ind_data, CoTpdoInd::data);
}

TEST(CO_Tpdo, CoTpdoRecv_SyncRTR_NoBufferedFrame_ExtendedFrame) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID | CO_PDO_COBID_FRAME);
  SetComm02TransmissionType(0xfcu);

  CreateTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::func, nullptr);

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags |= CAN_FLAG_RTR;
  msg.flags |= CAN_FLAG_IDE;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK(!CoTpdoSampleInd::called);
}

TEST(CO_Tpdo, CoTpdoRecv_SynchRTR_NoInd) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xfcu);

  CreateTpdo();
  co_tpdo_set_ind(tpdo, nullptr, nullptr);

  CHECK_EQUAL(0, co_tpdo_sync(tpdo, 0x00u));

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags |= CAN_FLAG_RTR;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);

  CHECK(CanSend::Called());
  POINTERS_EQUAL(&can_data, CanSend::data);
  CHECK_EQUAL(DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0u, CanSend::msg.flags);
  CHECK_EQUAL(0u, CanSend::msg.len);
}

TEST(CO_Tpdo, CoTpdoRecv_EventDrivenRTR) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xfdu);

  SetMapp00NumOfMappedAppObjs(0u);
  SetMappNthAppObject(0x01u, 0u);

  CreateTpdo();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags |= CAN_FLAG_RTR;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);

  CHECK(CanSend::Called());
  POINTERS_EQUAL(&can_data, CanSend::data);
  CHECK_EQUAL(DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0u, CanSend::msg.flags);
  CHECK_EQUAL(0u, CanSend::msg.len);
  CHECK_EQUAL(0u, CanSend::msg.data[0]);

  CHECK(CoTpdoInd::called);
  POINTERS_EQUAL(&ind_data, CoTpdoInd::data);
  POINTERS_EQUAL(tpdo, CoTpdoInd::pdo);
  CHECK_EQUAL(0u, CoTpdoInd::ac);
  CHECK(CoTpdoInd::ptr != nullptr);
  CHECK_EQUAL(msg.len, CoTpdoInd::n);
  POINTERS_EQUAL(&ind_data, CoTpdoInd::data);
}

TEST(CO_Tpdo, CoTpdoRecv_EventDrivenRTR_InitFrameFail_NoInd) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xfdu);

  SetMapp00NumOfMappedAppObjs(0x01u);
  SetMappNthAppObject(0x01u, 0xffff0000u);

  CreateTpdo();
  co_tpdo_set_ind(tpdo, nullptr, nullptr);

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags |= CAN_FLAG_RTR;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK(!CoTpdoInd::called);
  CHECK(!CanSend::Called());
}

TEST(CO_Tpdo, CoTpdoRecv_EventDrivenRTR_InitFrameFail) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0xfdu);

  SetMapp00NumOfMappedAppObjs(0x01u);
  SetMappNthAppObject(0x01u, 0x200000ffu);

  CreateTpdo();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags |= CAN_FLAG_RTR;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK(!CanSend::Called());

  CHECK(CoTpdoInd::called);
  POINTERS_EQUAL(tpdo, CoTpdoInd::pdo);
  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, CoTpdoInd::ac);
  POINTERS_EQUAL(nullptr, CoTpdoInd::ptr);
  CHECK_EQUAL(0, CoTpdoInd::n);
  POINTERS_EQUAL(&ind_data, CoTpdoInd::data);
}

TEST(CO_Tpdo, CoTpdoRecv_NoRTRTransmission) {
  SetComm00HighestSubidxSupported(0x02u);
  SetComm01CobId(DEV_ID);
  SetComm02TransmissionType(0x00u);

  SetMapp00NumOfMappedAppObjs(0u);
  SetMappNthAppObject(0x01u, 0u);

  CreateTpdo();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags |= CAN_FLAG_RTR;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK(!CoTpdoInd::called);
  CHECK(!CanSend::Called());
}

TEST_GROUP_BASE(CO_TpdoAllocation, CO_TpdoBase) {
  Allocators::Limited limitedAllocator;
  co_tpdo_t* tpdo = nullptr;

  void CompleteConfiguration() {
    dev_holder->CreateAndInsertObj(obj1800, 0x1800u);
    SetComm00HighestSubidxSupported(0x06u);
    SetComm01CobId(DEV_ID);
    SetComm02TransmissionType(0x01u);
    SetComm03InhibitTime(0x0002u);
    SetComm04CompatibilityEntry();
    SetComm05EventTimer(0x0004u);
    SetComm06SyncStartValue(0x05u);

    dev_holder->CreateAndInsertObj(obj1a00, 0x1a00u);
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    can_net_destroy(net);
    net = can_net_create(limitedAllocator.ToAllocT(), 0);

    CompleteConfiguration();
  }

  TEST_TEARDOWN() {
    co_tpdo_destroy(tpdo);
    TEST_BASE_TEARDOWN();
  }
};

TEST(CO_TpdoAllocation, CoTpdoCreate_NoMemoryAvailable) {
  limitedAllocator.LimitAllocationTo(0u);

  tpdo = co_tpdo_create(net, dev, DEV_ID);

  POINTERS_EQUAL(nullptr, tpdo);
}

TEST(CO_TpdoAllocation, CoTpdoCreate_MemoryOnlyForTpdo) {
  limitedAllocator.LimitAllocationTo(co_tpdo_sizeof());

  tpdo = co_tpdo_create(net, dev, DEV_ID);

  POINTERS_EQUAL(nullptr, tpdo);
}

TEST(CO_TpdoAllocation, CoTpdoCreate_MemoryOnlyForTpdoAndRecv) {
  limitedAllocator.LimitAllocationTo(co_tpdo_sizeof() + can_recv_sizeof());

  tpdo = co_tpdo_create(net, dev, DEV_ID);

  POINTERS_EQUAL(nullptr, tpdo);
}

TEST(CO_TpdoAllocation, CoTpdoCreate_MemoryOnlyForTpdoAndTimer) {
  limitedAllocator.LimitAllocationTo(co_tpdo_sizeof() + can_timer_sizeof());

  tpdo = co_tpdo_create(net, dev, DEV_ID);

  POINTERS_EQUAL(nullptr, tpdo);
}

TEST(CO_TpdoAllocation, CoTpdoCreate_MemoryOnlyForTpdoAndRecvAndSingleTimer) {
  limitedAllocator.LimitAllocationTo(co_tpdo_sizeof() + can_recv_sizeof() +
                                     can_timer_sizeof());

  tpdo = co_tpdo_create(net, dev, DEV_ID);

  POINTERS_EQUAL(nullptr, tpdo);
}

TEST(CO_TpdoAllocation, CoTpdoCreate_MemoryOnlyForTpdoAndTwoTimers) {
  limitedAllocator.LimitAllocationTo(co_tpdo_sizeof() + 2 * can_timer_sizeof());

  tpdo = co_tpdo_create(net, dev, DEV_ID);

  POINTERS_EQUAL(nullptr, tpdo);
}

TEST(CO_TpdoAllocation, CoTpdoCreate_AllNecessaryMemoryIsAvailable) {
  limitedAllocator.LimitAllocationTo(co_tpdo_sizeof() + can_recv_sizeof() +
                                     2 * can_timer_sizeof());

  tpdo = co_tpdo_create(net, dev, DEV_ID);

  CHECK(tpdo != nullptr);
}
