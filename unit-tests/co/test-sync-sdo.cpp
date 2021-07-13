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

#include <lely/co/csdo.h>
#include <lely/co/sync.h>
#include <lely/co/sdo.h>
#include <lely/co/val.h>

#include <libtest/allocators/default.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"

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

  void SetCobid(const co_unsigned32_t cobid) {
    co_sub_t* const sub_comm_cobid = co_dev_find_sub(dev, 0x1005u, 0x00u);
    CHECK(sub_comm_cobid != nullptr);
    co_sub_set_val_u32(sub_comm_cobid, cobid);
  }

  void RestartSYNC() {
    co_sync_stop(sync);
    CHECK_EQUAL(0, co_sync_start(sync));
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(allocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    dev_holder->CreateAndInsertObj(obj1005, 0x1005u);
    dev_holder->CreateAndInsertObj(obj1006, 0x1006u);
    dev_holder->CreateAndInsertObj(obj1019, 0x1019u);

    // 0x1005 - COB-ID
    obj1005->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32,
                             co_unsigned32_t{DEV_ID});

    // 0x1006 - communication cycle period
    obj1006->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t{0});

    // 0x1019 - counter overflow
    obj1019->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0});

    sync = co_sync_create(net, dev);
    CHECK(sync != nullptr);

    CoCsdoDnCon::Clear();

    CHECK_EQUAL(0, co_sync_start(sync));
  }

  TEST_TEARDOWN() {
    co_sync_destroy(sync);

    dev_holder.reset();
    can_net_destroy(net);
  }
};

/// @name SYNC service: object 0x1005 modification using SDO
///@{

/// \Given a pointer to a device (co_dev_t), the object dictionary
///        contains the COB-ID SYNC Message object (0x1005)
///
/// \When the download indication function for the object 0x1005 is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_SyncSdo, Co1005Dn_NonZeroAbortCode) {
  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret =
      LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1005u, 0x00u, ac);

  CHECK_EQUAL(ac, ret);
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1005 with
///        COB-ID SYNC value set
///
/// \When a value shorter than 4 bytes is downloaded to object 0x1005 using SDO
///
/// \Then CO_SDO_AC_TYPE_LEN_LO abort code is passed to CSDO download
///       confirmation function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
TEST(CO_SyncSdo, Co1005Dn_TypeLenTooLow) {
  const co_unsigned8_t num_of_mappings = 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1005, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_LO, CoCsdoDnCon::ac);
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1005 with
///        COB-ID SYNC set and with an additional sub-object at some non-zero
///        sub-index set to some value
///
/// \When the sub-object is changed using SDO
///
/// \Then CO_SDO_AC_NO_SUB abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_SyncSdo, Co1005Dn_InvalidSubobject) {
  obj1005->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                           co_unsigned32_t{DEV_ID});
  RestartSYNC();

  const co_unsigned32_t data = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1005, 0x01u, CO_DEFTYPE_UNSIGNED32, &data,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_NO_SUB, CoCsdoDnCon::ac);
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1005 with
///        COB-ID SYNC set
///
/// \When object 0x1005 is modified using SDO with same value as already set
///
/// \Then 0 is returned, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_SyncSdo, Co1005Dn_SameAsPrevious) {
  const co_unsigned32_t cobid = DEV_ID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1005, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1005 with
///        COB-ID SYNC with producer bit set
///
/// \When object 0x1005 is modified using SDO with new COB-ID SYNC with producer
///       bit set
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_SyncSdo, Co1005Dn_ProducerToProducer_NewCanId) {
  SetCobid(DEV_ID | CO_SYNC_COBID_PRODUCER);

  const co_unsigned32_t cobid = (DEV_ID + 1u) | CO_SYNC_COBID_PRODUCER;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1005, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1005 with
///        COB-ID SYNC with producer bit set
///
/// \When object 0x1005 is modified using SDO to new COB-ID with the same CAN-ID
///
/// \Then 0 is returned, new COB-ID is set in the 0x1005 object
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_sub_dn()
TEST(CO_SyncSdo, Co1005Dn_ProducerToProducer_SameCanId_NewCobid) {
  SetCobid(DEV_ID | CO_SYNC_COBID_PRODUCER);

  const co_unsigned32_t cobid =
      DEV_ID | CO_SYNC_COBID_PRODUCER | CO_SYNC_COBID_FRAME;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1005, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);

  CHECK_EQUAL(cobid, co_obj_get_val_u32(obj1005->Get(), 0x00u));
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1005 with
///        COB-ID SYNC set
///
/// \When object 0x1005 is modified using SDO to new COB-ID with the same CAN-ID
///       but with producer bit set
///
/// \Then 0 is returned, new COB-ID is set in the 0x1005 object
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_sub_dn()
TEST(CO_SyncSdo, Co1005Dn_ConsumerToProducer_SameCanId) {
  const co_unsigned32_t cobid = DEV_ID | CO_SYNC_COBID_PRODUCER;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1005, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);

  CHECK_EQUAL(cobid, co_obj_get_val_u32(obj1005->Get(), 0x00u));
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1005 with
///        COB-ID SYNC set
///
/// \When object 0x1005 is modified using SDO to new COB-ID using Extended
///       Identifier but without frame bit set
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_SyncSdo, Co1005Dn_ExtendedId_NoFrameBit) {
  const co_unsigned32_t cobid = DEV_ID | (1u << 28u);
  const auto ret =
      co_dev_dn_val_req(dev, 0x1005, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1005 with
///        COB-ID SYNC set
///
/// \When object 0x1005 is modified using SDO to new COB-ID using Extended
///       Identifier with frame bit set
///
/// \Then 0 is returned, new COB-ID is set in the 0x1005 object
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_sub_dn()
TEST(CO_SyncSdo, Co1005Dn_FrameBit) {
  const co_unsigned32_t cobid = DEV_ID | CO_SYNC_COBID_FRAME;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1005, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);

  CHECK_EQUAL(cobid, co_obj_get_val_u32(obj1005->Get(), 0x00u));
}

///@}

/// @name SYNC service: object 0x1006 modification using SDO
///@{

/// \Given a pointer to a device (co_dev_t), the object dictionary
///        contains the Communication Cycle Period object (0x1006)
///
/// \When the download indication function for the object 0x1006 is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_SyncSdo, Co1006Dn_NonZeroAbortCode) {
  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret =
      LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1006u, 0x00u, ac);

  CHECK_EQUAL(ac, ret);
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1006
///        with communication cycle period set to 0
///
/// \When a value shorter than 4 bytes is downloaded to object 0x1006 using SDO
///
/// \Then CO_SDO_AC_TYPE_LEN_LO abort code is passed to CSDO download
///       confirmation function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
TEST(CO_SyncSdo, Co1006Dn_TypeLenTooLow) {
  const co_unsigned16_t period = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1006, 0x00u, CO_DEFTYPE_UNSIGNED16, &period,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_LO, CoCsdoDnCon::ac);
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1006 with
///        communication cycle period set to 0 and with an additional sub-object
///        at some non-zero sub-index set to some value
///
/// \When the sub-object is changed using SDO
///
/// \Then CO_SDO_AC_NO_SUB abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_SyncSdo, Co1006Dn_InvalidSubobject) {
  obj1006->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED16,
                           co_unsigned16_t{0x00u});
  RestartSYNC();

  const co_unsigned16_t data = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1006, 0x01u, CO_DEFTYPE_UNSIGNED16, &data,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_NO_SUB, CoCsdoDnCon::ac);
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1006
///        with communication cycle period set to 0
///
/// \When object 0x1006 is modified using SDO with same value as already set
///
/// \Then 0 is returned, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_SyncSdo, Co1006Dn_SameAsPrevious) {
  const co_unsigned32_t period = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1006, 0x00u, CO_DEFTYPE_UNSIGNED32, &period,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1006 with
///        communication cycle period set to 0
///
/// \When object 0x1006 is modified using SDO with a new value
///
/// \Then 0 is returned, communication cycle period is set to the new value in
///       the 0x1006 object
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_sub_dn()
TEST(CO_SyncSdo, Co1006Dn_Nominal) {
  const co_unsigned32_t period = 231u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1006, 0x00u, CO_DEFTYPE_UNSIGNED32, &period,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);

  CHECK_EQUAL(period, co_obj_get_val_u32(obj1006->Get(), 0x00u));
}

///@}

/// @name SYNC service: object 0x1019 modification using SDO
///@{

/// \Given a pointer to a device (co_dev_t), the object dictionary
///        contains the Synchronous Counter Overflow object (0x1019)
///
/// \When the download indication function for the object 0x1019 is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_SyncSdo, Co1019Dn_NonZeroAbortCode) {
  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret =
      LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1019u, 0x00u, ac);

  CHECK_EQUAL(ac, ret);
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1019 with
///        counter overflow value set to 0
///
/// \When a value larger than one byte is downloaded to object 0x1019 using SDO
///
/// \Then CO_SDO_AC_TYPE_LEN_HI abort code is passed to CSDO download
///       confirmation function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
TEST(CO_SyncSdo, Co1019Dn_TypeLenTooHigh) {
  const co_unsigned16_t data = 0u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1019u, 0x00u, CO_DEFTYPE_UNSIGNED16, &data,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_HI, CoCsdoDnCon::ac);
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1019 with
///        counter overflow value set to 0 and with an additional sub-object at
///        some non-zero sub-index set to some value
///
/// \When the sub-object is changed using SDO
///
/// \Then CO_SDO_AC_NO_SUB abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_SyncSdo, Co1019Dn_InvalidSubobject) {
  obj1019->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0x00u});
  RestartSYNC();

  const co_unsigned8_t data = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1019u, 0x01u, CO_DEFTYPE_UNSIGNED8, &data,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_NO_SUB, CoCsdoDnCon::ac);
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1019 with
///        counter overflow value set to 0
///
/// \When object 0x1019 is modified using SDO with same value as already set
///
/// \Then 0 is returned, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_SyncSdo, Co1019Dn_SameAsPrevious) {
  const co_unsigned8_t cnt = 0u;
  const auto ret = co_dev_dn_val_req(dev, 0x1019u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &cnt, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1019 with
///        counter overflow value set to 0 and object 0x1006 with communication
///        cycle period set to a non-zero value
///
/// \When object 0x1019 is modified using SDO with a new value
///
/// \Then CO_SDO_AC_DATA_DEV abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_SyncSdo, Co1019Dn_CommCyclePeriodNotZero) {
  co_sub_t* const sub = co_dev_find_sub(dev, 0x1006u, 0x00u);
  co_sub_set_val_u32(sub, CO_UNSIGNED32_MAX);
  RestartSYNC();

  const co_unsigned8_t cnt = 32u;
  const auto ret = co_dev_dn_val_req(dev, 0x1019u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &cnt, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_DATA_DEV, CoCsdoDnCon::ac);
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1019 with
///        counter overflow value set to 0
///
/// \When object 0x1019 is modified using SDO with new value equal to the lower
///       limit
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_SyncSdo, Co1019Dn_OverflowEveryTime) {
  const co_unsigned8_t cnt = 1u;
  const auto ret = co_dev_dn_val_req(dev, 0x1019u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &cnt, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1019 with
///        counter overflow value set to 0
///
/// \When object 0x1019 is modified using SDO with new value greater than the
///       maximum allowed counter overflow value
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_SyncSdo, Co1019Dn_OverflowMoreThanMax) {
  const co_unsigned8_t cnt = 241u;
  const auto ret = co_dev_dn_val_req(dev, 0x1019u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &cnt, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

/// \Given a pointer to started SYNC service (co_sync_t), object 0x1019 with
///        counter overflow value set to 0
///
/// \When object 0x1019 is modified using SDO with a new value within allowed
///       limits
///
/// \Then 0 is returned, counter overflow value is set to new value in the
///       0x1019 object
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_sub_dn()
TEST(CO_SyncSdo, Co1019Dn_Nominal) {
  const co_unsigned8_t cnt = 32u;
  const auto ret = co_dev_dn_val_req(dev, 0x1019u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &cnt, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);

  CHECK_EQUAL(cnt, co_obj_get_val_u8(obj1019->Get(), 0x00u));
}

///@}
