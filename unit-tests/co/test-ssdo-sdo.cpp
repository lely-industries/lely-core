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
#include <lely/co/ssdo.h>
#include <lely/co/sdo.h>

#include "holder/dev.hpp"
#include "holder/obj.hpp"

#include "allocators/default.hpp"

#include "lely-cpputest-ext.hpp"
#include "lely-unit-test.hpp"
#include "override/lelyco-val.hpp"

TEST_GROUP(CO_SsdoDnInd) {
  Allocators::Default allocator;

  static const co_unsigned8_t DEV_ID = 0x01u;
  static const co_unsigned8_t SSDO_NUM = 0x01u;
  static const co_unsigned32_t CAN_ID = DEV_ID;
  static const co_unsigned32_t CAN_ID_EXT =
      co_unsigned32_t(DEV_ID) | 0x10000000u;

  co_dev_t* dev = nullptr;
  can_net_t* net = nullptr;
  co_ssdo_t* ssdo = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1200;

  void CreateObjInDev(std::unique_ptr<CoObjTHolder> & obj_holder,
                      const co_unsigned16_t idx) {
    assert(dev);
    obj_holder.reset(new CoObjTHolder(idx));
    CHECK(obj_holder->Get() != nullptr);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));
  }

  co_unsigned32_t GetSrv01CobidReq() {
    return co_dev_get_val_u32(dev, 0x1200u, 0x01u);
  }

  co_unsigned32_t GetSrv02CobidRes() {
    return co_dev_get_val_u32(dev, 0x1200u, 0x02u);
  }

  co_unsigned8_t GetSrv03NodeId() {
    return co_dev_get_val_u8(dev, 0x1200u, 0x03u);
  }

  // obj 0x1200, sub 0x00 - highest sub-index supported
  void SetSrv00HighestSubidxSupported(co_unsigned8_t subidx) {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1200u, 0x00u);
    if (sub != nullptr)
      co_sub_set_val_u8(sub, subidx);
    else
      obj1200->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, subidx);
  }

  // obj 0x1200, sub 0x01 - COB-ID client -> server (rx)
  void SetSrv01CobidReq(const co_unsigned32_t cobid) {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1200u, 0x01u);
    if (sub != nullptr)
      co_sub_set_val_u32(sub, cobid);
    else
      obj1200->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32, cobid);
  }

  // obj 0x1200, sub 0x02 - COB-ID server -> client (tx)
  void SetSrv02CobidRes(const co_unsigned32_t cobid) {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1200u, 0x02u);
    if (sub != nullptr)
      co_sub_set_val_u32(sub, cobid);
    else
      obj1200->InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED32, cobid);
  }

  // obj 0x1200, sub 0x03 - Node-ID of the SDO client
  void SetSrv03NodeId(co_unsigned8_t id) {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1200u, 0x03u);
    if (sub != nullptr)
      co_sub_set_val_u8(sub, id);
    else
      obj1200->InsertAndSetSub(0x03u, CO_DEFTYPE_UNSIGNED8, id);
  }

  void RestartSSDO() {
    co_ssdo_stop(ssdo);
    co_ssdo_start(ssdo);
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(allocator.ToAllocT());
    assert(net);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    assert(dev);

    CreateObjInDev(obj1200, 0x1200u);
    SetSrv00HighestSubidxSupported(0x03u);
    SetSrv01CobidReq(CAN_ID);
    SetSrv02CobidRes(CAN_ID);
    SetSrv03NodeId(0);

    ssdo = co_ssdo_create(net, dev, SSDO_NUM);
    CHECK(ssdo != nullptr);
    co_ssdo_start(ssdo);

    CoCsdoDnCon::Clear();
  }

  TEST_TEARDOWN() {
    co_ssdo_stop(ssdo);
    co_ssdo_destroy(ssdo);

    dev_holder.reset();
    can_net_destroy(net);
  }
};

#if HAVE_LELY_OVERRIDE
TEST(CO_SsdoDnInd, CoValReadZero) {
  LelyOverride::co_val_read(0);
  const int data = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x01u, CO_DEFTYPE_UNSIGNED8, &data,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_LO, CoCsdoDnCon::ac);
}
#endif  // HAVE_LELY_OVERRIDE

TEST(CO_SsdoDnInd, DownloadHighestSubidx) {
  const int data = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x00u, CO_DEFTYPE_UNSIGNED8, &data,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(CO_SDO_AC_NO_WRITE, CoCsdoDnCon::ac);
}

TEST(CO_SsdoDnInd, DownloadReqCobid_SameAsOld) {
  const co_unsigned32_t cobid = CAN_ID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
  CHECK_EQUAL(cobid, GetSrv01CobidReq());
}

TEST(CO_SsdoDnInd, DownloadReqCobid_OldValidNewValidNewId) {
  const co_unsigned32_t cobid = CAN_ID + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
  CHECK_EQUAL(CAN_ID, GetSrv01CobidReq());
}

TEST(CO_SsdoDnInd, DownloadReqCobid_OldValidNewInvalidNewId) {
  const co_unsigned32_t cobid = (CAN_ID + 1u) | CO_SDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
  CHECK_EQUAL(cobid, GetSrv01CobidReq());
}

TEST(CO_SsdoDnInd, DownloadReqCobid_OldInvalidNewValidNewId) {
  SetSrv01CobidReq(CAN_ID | CO_SDO_COBID_VALID);
  RestartSSDO();

  const co_unsigned32_t cobid = CAN_ID + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
  CHECK_EQUAL(cobid, GetSrv01CobidReq());
}

TEST(CO_SsdoDnInd, DownloadReqCobid_OldValidNewValidOldId) {
  const co_unsigned32_t cobid = CAN_ID | CO_SDO_COBID_FRAME;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
  CHECK_EQUAL(cobid, GetSrv01CobidReq());
}

TEST(CO_SsdoDnInd, DownloadReqCobid_OldValidNewInvalidNewIdExtended) {
  co_unsigned32_t cobid = CAN_ID_EXT | CO_SDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
  CHECK_EQUAL(CAN_ID, GetSrv01CobidReq());
}

TEST(CO_SsdoDnInd, DownloadResCobid_SameAsOld) {
  const co_unsigned32_t cobid = CAN_ID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x02u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
  CHECK_EQUAL(cobid, GetSrv02CobidRes());
}

TEST(CO_SsdoDnInd, DownloadResCobid_OldValidNewValidNewId) {
  const co_unsigned32_t cobid = CAN_ID + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x02u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
  CHECK_EQUAL(CAN_ID, GetSrv02CobidRes());
}

TEST(CO_SsdoDnInd, DownloadResCobid_OldValidNewInvalidNewId) {
  const co_unsigned32_t cobid = CAN_ID | CO_SDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x02u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
  CHECK_EQUAL(cobid, GetSrv02CobidRes());
}

TEST(CO_SsdoDnInd, DownloadResCobid_OldInvalidNewValidNewId) {
  SetSrv02CobidRes(CAN_ID | CO_SDO_COBID_VALID);
  RestartSSDO();

  const co_unsigned32_t cobid = CAN_ID + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x02u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
  CHECK_EQUAL(cobid, GetSrv02CobidRes());
}

TEST(CO_SsdoDnInd, DownloadResCobid_OldValidNewValidOldId) {
  const co_unsigned32_t cobid = CAN_ID | CO_SDO_COBID_FRAME;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x02u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
  CHECK_EQUAL(cobid, GetSrv02CobidRes());
}

TEST(CO_SsdoDnInd, DownloadResCobid_OldValidNewInvalidOldId) {
  co_unsigned32_t cobid = CAN_ID_EXT | CO_SDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x02u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
  CHECK_EQUAL(CAN_ID, GetSrv02CobidRes());
}

TEST(CO_SsdoDnInd, DownloadResCobid_CobidExtendedId_Invalid) {
  co_unsigned32_t cobid = CAN_ID | CO_SDO_COBID_FRAME | CO_SDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x02u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
  CHECK_EQUAL(cobid, GetSrv02CobidRes());
}

TEST(CO_SsdoDnInd, DownloadNodeId_SameAsOld) {
  const co_unsigned8_t new_id = 0x00u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x03u, CO_DEFTYPE_UNSIGNED8, &new_id,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
  CHECK_EQUAL(0, GetSrv03NodeId());
}

TEST(CO_SsdoDnInd, DownloadNodeId_Nominal) {
  const co_unsigned8_t new_id = 0x01u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x03u, CO_DEFTYPE_UNSIGNED8, &new_id,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
  CHECK_EQUAL(new_id, GetSrv03NodeId());
}

TEST(CO_SsdoDnInd, DownloadNodeId_InvalidSubidx) {
  obj1200->InsertAndSetSub(0x04u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  RestartSSDO();

  const co_unsigned8_t data = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1200u, 0x04u, CO_DEFTYPE_UNSIGNED8, &data,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::num_called);
  CHECK_EQUAL(CO_SDO_AC_NO_SUB, CoCsdoDnCon::ac);
}
