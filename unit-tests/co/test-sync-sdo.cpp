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

#include <lely/co/csdo.h>
#include <lely/co/sync.h>
#include <lely/co/sdo.h>
#include <lely/co/val.h>

#include "holder/dev.hpp"
#include "holder/obj.hpp"

#include "allocators/default.hpp"

#include "lely-cpputest-ext.hpp"
#include "lely-unit-test.hpp"
#include "override/lelyco-val.hpp"

TEST_GROUP(CO_SyncSdo) {
  Allocators::Default allocator;

  const co_unsigned8_t DEV_ID = 0x01u;

  co_dev_t* dev = nullptr;
  can_net_t* net = nullptr;
  co_sync_t* sync = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1005;
  std::unique_ptr<CoObjTHolder> obj1006;
  std::unique_ptr<CoObjTHolder> obj1019;

  void CreateObjInDev(std::unique_ptr<CoObjTHolder> & obj_holder,
                      co_unsigned16_t idx) {
    obj_holder.reset(new CoObjTHolder(idx));
    CHECK(obj_holder->Get() != nullptr);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));
  }

  void SetCobid(const co_unsigned32_t cobid) {
    co_sub_t* const sub_comm_cobid = co_dev_find_sub(dev, 0x1005u, 0x00u);
    CHECK(sub_comm_cobid != nullptr);
    co_sub_set_val_u32(sub_comm_cobid, cobid);
  }

  void RestartSYNC() { CHECK_EQUAL(0, co_sync_start(sync)); }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(allocator.ToAllocT());
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    CreateObjInDev(obj1005, 0x1005u);
    CreateObjInDev(obj1006, 0x1006u);
    CreateObjInDev(obj1019, 0x1019u);

    // 0x1005 - COB-ID
    obj1005->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32,
                             co_unsigned32_t(DEV_ID));

    // 0x1006 - communication cycle period
    obj1006->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t(0));

    // 0x1019 - counter overflow
    obj1019->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0));

    sync = co_sync_create(net, dev);
    CHECK(sync != nullptr);

    CoCsdoDnCon::Clear();
  }

  TEST_TEARDOWN() {
    co_sync_destroy(sync);

    dev_holder.reset();
    can_net_destroy(net);
  }
};

// given: valid SYNC
// when: co_1005_dn_ind()
// then: CO_SDO_AC_TYPE_LEN_LO abort code is returned
TEST(CO_SyncSdo, Co1005Dn_TypeLenTooLow) {
  const co_unsigned8_t num_of_mappings = 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1005, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_LO, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: co_1005_dn_ind()
// then: CO_SDO_AC_NO_SUB abort code is returned
TEST(CO_SyncSdo, Co1005Dn_InvalidSubobject) {
  obj1005->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                           co_unsigned32_t(DEV_ID));
  RestartSYNC();

  const co_unsigned32_t data = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1005, 0x01u, CO_DEFTYPE_UNSIGNED32, &data,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_NO_SUB, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: co_1005_dn_ind()
// then: 0 abort code is returned
TEST(CO_SyncSdo, Co1005Dn_SameAsPrevious) {
  const co_unsigned32_t cobid = DEV_ID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1005, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: co_1005_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SyncSdo, Co1005Dn_ProducerToProducer_NewCanId) {
  SetCobid(DEV_ID | CO_SYNC_COBID_PRODUCER);
  RestartSYNC();

  const co_unsigned32_t cobid = (DEV_ID + 1u) | CO_SYNC_COBID_PRODUCER;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1005, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: new COB-ID with the same CAN-ID is downloaded
// then: 0 abort code is returned
TEST(CO_SyncSdo, Co1005Dn_ProducerToProducer_SameCanId_NewCobid) {
  SetCobid(DEV_ID | CO_SYNC_COBID_PRODUCER);
  RestartSYNC();

  const co_unsigned32_t cobid =
      DEV_ID | CO_SYNC_COBID_PRODUCER | CO_SYNC_COBID_FRAME;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1005, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: co_1005_dn_ind()
// then: 0 abort code is returned
TEST(CO_SyncSdo, Co1005Dn_ConsumerToProducer_SameCanId) {
  const co_unsigned32_t cobid = DEV_ID | CO_SYNC_COBID_PRODUCER;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1005, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: co_1005_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SyncSdo, Co1005Dn_ExtendedId_NoFrameBit) {
  const co_unsigned32_t cobid = DEV_ID | (1u << 28u);
  const auto ret =
      co_dev_dn_val_req(dev, 0x1005, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: co_1005_dn_ind()
// then: 0 abort code is returned
TEST(CO_SyncSdo, Co1005Dn_FrameBit) {
  const co_unsigned32_t cobid = DEV_ID | CO_SYNC_COBID_FRAME;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1005, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: co_1006_dn_ind()
// then: CO_SDO_AC_TYPE_LEN_LO abort code is returned
TEST(CO_SyncSdo, Co1006Dn_TypeLenTooLow) {
  const co_unsigned16_t period = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1006, 0x00u, CO_DEFTYPE_UNSIGNED16, &period,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_LO, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: co_1006_dn_ind()
// then: CO_SDO_AC_NO_SUB abort code is returned
TEST(CO_SyncSdo, Co1006Dn_InvalidSubobject) {
  obj1006->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED16,
                           co_unsigned16_t(0x00u));
  RestartSYNC();

  const co_unsigned16_t data = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1006, 0x01u, CO_DEFTYPE_UNSIGNED16, &data,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_NO_SUB, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: co_1006_dn_ind()
// then: 0 abort code is returned
TEST(CO_SyncSdo, Co1006Dn_SameAsPrevious) {
  const co_unsigned32_t period = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1006, 0x00u, CO_DEFTYPE_UNSIGNED32, &period,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: co_1006_dn_ind()
// then: 0 abort code is returned
TEST(CO_SyncSdo, Co1006Dn) {
  const co_unsigned32_t period = 231u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1006, 0x00u, CO_DEFTYPE_UNSIGNED32, &period,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: co_1019_dn_ind()
// then: CO_SDO_AC_TYPE_LEN_HI abort code is returned
TEST(CO_SyncSdo, Co1019Dn_TypeLenTooHigh) {
  const co_unsigned16_t data = 0u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1019u, 0x00u, CO_DEFTYPE_UNSIGNED16, &data,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_HI, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: co_1019_dn_ind()
// then: CO_SDO_AC_NO_SUB abort code is returned
TEST(CO_SyncSdo, Co1019Dn_InvalidSubobject) {
  obj1019->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  RestartSYNC();

  const co_unsigned8_t data = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1019u, 0x01u, CO_DEFTYPE_UNSIGNED8, &data,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_NO_SUB, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: co_1019_dn_ind()
// then: 0 abort code is returned
TEST(CO_SyncSdo, Co1019Dn_SameAsPrevious) {
  const co_unsigned8_t cnt = 0u;
  const auto ret = co_dev_dn_val_req(dev, 0x1019u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &cnt, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: co_1019_dn_ind()
// then: CO_SDO_AC_DATA_DEV abort code is returned
TEST(CO_SyncSdo, Co1019Dn_CommCyclePeriodNotZero) {
  co_sub_t* const sub = co_dev_find_sub(dev, 0x1006u, 0x00u);
  co_sub_set_val_u32(sub, CO_UNSIGNED32_MAX);
  RestartSYNC();

  const co_unsigned8_t cnt = 32u;
  const auto ret = co_dev_dn_val_req(dev, 0x1019u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &cnt, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_DATA_DEV, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: co_1019_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SyncSdo, Co1019Dn_OverflowEveryTime) {
  const co_unsigned8_t cnt = 1u;
  const auto ret = co_dev_dn_val_req(dev, 0x1019u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &cnt, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: co_1019_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SyncSdo, Co1019Dn_OverflowMoreThanMax) {
  const co_unsigned8_t cnt = 241u;
  const auto ret = co_dev_dn_val_req(dev, 0x1019u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &cnt, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: valid SYNC
// when: co_1019_dn_ind()
// then: 0 abort code is returned
TEST(CO_SyncSdo, Co1019Dn) {
  const co_unsigned8_t cnt = 32u;
  const auto ret = co_dev_dn_val_req(dev, 0x1019u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &cnt, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}
