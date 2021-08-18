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

#include <memory>

#include <CppUTest/TestHarness.h>

#include <lely/co/csdo.h>
#include <lely/co/type.h>
#include <lely/can/net.h>

#include <libtest/allocators/default.hpp>
#include <libtest/tools/lely-unit-test.hpp>
#include <libtest/tools/can-send.hpp>

#include "holder/dev.hpp"
#include "obj-init/sdo-client-par.hpp"

TEST_GROUP(CO_CsdoDnInd) {
  using Sub00HighestSubidxSupported =
      Obj1280SdoClientPar::Sub00HighestSubidxSupported;
  using Sub01CobIdReq = Obj1280SdoClientPar::Sub01CobIdReq;
  using Sub02CobIdRes = Obj1280SdoClientPar::Sub02CobIdRes;
  using Sub03NodeId = Obj1280SdoClientPar::Sub03NodeId;

  static const co_unsigned8_t DEV_ID = 0x01u;
  static const co_unsigned8_t CSDO_NUM = 0x01u;
  static const co_unsigned32_t SERVER_NODEID = 0x42u;
  const co_unsigned16_t IDX = 0x1280u;
  const co_unsigned32_t DEFAULT_COBID_REQ = 0x600u + DEV_ID;
  const co_unsigned32_t DEFAULT_COBID_RES = 0x580u + DEV_ID;

  co_csdo_t* csdo = nullptr;
  co_dev_t* dev = nullptr;
  can_net_t* net = nullptr;
  Allocators::Default defaultAllocator;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1280;

  void StartCSDO() { co_csdo_start(csdo); }

  void CreateObj1280Defaults() const {
    obj1280->EmplaceSub<Sub00HighestSubidxSupported>(0x02u);
    obj1280->EmplaceSub<Sub01CobIdReq>(DEFAULT_COBID_REQ);
    obj1280->EmplaceSub<Sub02CobIdRes>(DEFAULT_COBID_RES);
    obj1280->EmplaceSub<Sub03NodeId>(SERVER_NODEID);
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(defaultAllocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    can_net_set_send_func(net, CanSend::Func, nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    dev_holder->CreateObj<Obj1280SdoClientPar>(obj1280);
    CreateObj1280Defaults();
    csdo = co_csdo_create(net, dev, CSDO_NUM);
    CHECK(csdo != nullptr);
  }

  TEST_TEARDOWN() {
    co_csdo_destroy(csdo);

    dev_holder.reset();
    can_net_destroy(net);

    CoCsdoDnCon::Clear();
  }
};

/// @name CSDO service: object 0x1280 modification using SDO
///@{

/// \Given a pointer to a device (co_dev_t) with a started CSDO service,
///        the object dictionary contains the SDO Client Parameter
///        object (0x1280)
///
/// \When the download indication function for the object 0x1280 is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_CsdoDnInd, NonZeroAbortCode) {
  StartCSDO();

  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret =
      LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1280u, 0x00u, ac);

  CHECK_EQUAL(ac, ret);
}

/// \Given a pointer to the device (co_dev_t) with the CSDO service started
///        and the object dictionary containing the 0x1280 object
///
/// \When a value is downloaded to the client parameter
///       "Highest sub-index supported" entry (idx: 0x1280, subidx: 0x00)
///
/// \Then 0 is returned, download confirmation function is called once,
///       CO_SDO_AC_NO_WRITE is set as the abort code, the requested entry
///       is not changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_CsdoDnInd, DownloadHighestSubidx) {
  StartCSDO();

  const co_unsigned8_t val = 0x04u;
  const auto ret = co_dev_dn_val_req(dev, IDX, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, 0x00u, CO_SDO_AC_NO_WRITE, nullptr);

  CHECK_EQUAL(0x02u, obj1280->GetSub<Sub00HighestSubidxSupported>());
}

/// \Given a pointer to the device (co_dev_t) with the CSDO service started
///        and the object dictionary containing the 0x1280 object
///
/// \When a value shorter than 4 bytes is downloaded to the client parameter
///       "COB-ID client -> server (tx)" entry (idx: 0x1280, subidx: 0x01)
///
/// \Then 0 is returned, download confirmation function is called once,
///       CO_SDO_AC_TYPE_LEN_LO is set as the abort code, the requested entry
///       is not changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
TEST(CO_CsdoDnInd, DownloadReqCobid_TooShort) {
  StartCSDO();

  const co_unsigned8_t val = 0u;
  const auto ret = co_dev_dn_val_req(dev, IDX, 0x01u, CO_DEFTYPE_UNSIGNED8,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, 0x01u, CO_SDO_AC_TYPE_LEN_LO, nullptr);

  CHECK_EQUAL(DEFAULT_COBID_REQ, obj1280->GetSub<Sub01CobIdReq>());
}

/// \Given a pointer to the device (co_dev_t) with the CSDO service started
///        and the object dictionary containing the 0x1280 object with
///        a client parameter "COB-ID client -> server (tx)" entry
///
/// \When the same COB-ID value is downloaded to the client parameter "COB-ID
///       client -> server (tx)" entry (idx: 0x1280, subidx: 0x01)
///
/// \Then 0 is returned, download confirmation function is called once,
///       0 is set as the abort code, the requested entry is not changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_CsdoDnInd, DownloadCobidReq_SameAsOld) {
  StartCSDO();

  const co_unsigned32_t new_cobid_req = DEFAULT_COBID_REQ;
  const auto ret =
      co_dev_dn_val_req(dev, IDX, 0x01u, CO_DEFTYPE_UNSIGNED32, &new_cobid_req,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, 0x01u, 0u, nullptr);

  CHECK_EQUAL(DEFAULT_COBID_REQ, obj1280->GetSub<Sub01CobIdReq>());
}

/// \Given a pointer to the device (co_dev_t) with the CSDO service started
///        and the object dictionary containing the 0x1280 object with
///        a valid client parameter "COB-ID client -> server (tx)" entry
///
/// \When a new valid COB-ID with a new CAN-ID is downloaded to the client
///       parameter "COB-ID client -> server (tx)" entry
///       (idx: 0x1280, subidx: 0x01)
///
/// \Then 0 is returned, download confirmation function is called once,
///       CO_SDO_AC_PARAM_VAL is set as the abort code, the requested entry
///       is not changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_CsdoDnInd, DownloadCobidReq_OldValid_NewValid_NewId) {
  StartCSDO();

  const co_unsigned32_t new_cobid_req = DEFAULT_COBID_REQ + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, IDX, 0x01u, CO_DEFTYPE_UNSIGNED32, &new_cobid_req,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, 0x01u, CO_SDO_AC_PARAM_VAL, nullptr);

  CHECK_EQUAL(DEFAULT_COBID_REQ, obj1280->GetSub<Sub01CobIdReq>());
}

/// \Given a pointer to the device (co_dev_t) with the CSDO service started
///        and the object dictionary containing the 0x1280 object with a valid
///        client parameter "COB-ID client -> server (tx)" entry
///
/// \When a new valid COB-ID with an old CAN-ID is downloaded to the client
///       parameter "COB-ID client -> server (tx)" entry (idx: 0x1280,
///       subidx: 0x01)
///
/// \Then 0 is returned, download confirmation function is called once,
///       0 is set as the abort code, the requested entry is changed to
///       the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_CsdoDnInd, DownloadCobidReq_OldValid_NewValid_OldId) {
  StartCSDO();

  const co_unsigned32_t new_cobid_req = DEFAULT_COBID_REQ | CO_SDO_COBID_FRAME;
  const auto ret =
      co_dev_dn_val_req(dev, IDX, 0x01u, CO_DEFTYPE_UNSIGNED32, &new_cobid_req,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, 0x01u, 0u, nullptr);

  CHECK_EQUAL(new_cobid_req, obj1280->GetSub<Sub01CobIdReq>());
}

/// \Given a pointer to the device (co_dev_t) with the CSDO service started
///        and the object dictionary containing the 0x1280 object with
///        a valid client parameter "COB-ID client -> server (tx)" entry
///
/// \When a new invalid COB-ID with a new extended CAN-ID is downloaded to the
///       client parameter "COB-ID client -> server (tx)" entry
///       (idx: 0x1280, subidx: 0x01)
///
/// \Then 0 is returned, download confirmation function is called once,
///       CO_SDO_AC_PARAM_VAL is set as the abort code, the requested entry is
///       not changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_CsdoDnInd, DownloadCobidReq_FrameBitNotSet_NewIdExtended) {
  const co_unsigned32_t DEFAULT_COBID_REQ_EXT = DEFAULT_COBID_REQ | (1 << 28u);
  StartCSDO();

  const co_unsigned32_t new_cobid_req =
      DEFAULT_COBID_REQ_EXT | CO_SDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, IDX, 0x01u, CO_DEFTYPE_UNSIGNED32, &new_cobid_req,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, 0x01u, CO_SDO_AC_PARAM_VAL, nullptr);

  CHECK_EQUAL(DEFAULT_COBID_REQ, obj1280->GetSub<Sub01CobIdReq>());
}

/// \Given a pointer to the device (co_dev_t) with the CSDO service started
///        and the object dictionary containing the 0x1280 object with
///        an invalid client parameter "COB-ID client -> server (tx)" entry
///
/// \When a new valid COB-ID with the same CAN-ID is downloaded to the client
///       parameter "COB-ID client -> server (tx)" entry (idx: 0x1280,
///       subidx: 0x01)
///
/// \Then 0 is returned, download confirmation function is called once,
///       0 is set as the abort code, the requested entry is changed to
///       the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_CsdoDnInd, DownloadCobidReq_OldInvalid_NewValid) {
  obj1280->SetSub<Sub01CobIdReq>(obj1280->GetSub<Sub01CobIdReq>() |
                                 CO_SDO_COBID_VALID);
  StartCSDO();

  const co_unsigned32_t new_cobid_req = DEFAULT_COBID_REQ;
  const auto ret =
      co_dev_dn_val_req(dev, IDX, 0x01u, CO_DEFTYPE_UNSIGNED32, &new_cobid_req,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, 0x01u, 0u, nullptr);

  CHECK_EQUAL(new_cobid_req, obj1280->GetSub<Sub01CobIdReq>());
}

/// \Given a pointer to the device (co_dev_t) with the CSDO service started
///        and the object dictionary containing the 0x1280 object with
///        a valid client parameter "COB-ID client -> server (tx)" entry
///
/// \When a new invalid COB-ID with the same CAN-ID is downloaded to the
///       client parameter "COB-ID client -> server (tx)" entry
///       (idx: 0x1280, subidx: 0x01)
///
/// \Then 0 is returned, download confirmation function is called once,
///       0 is set as the abort code, the requested entry is changed to
///       the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_CsdoDnInd, DownloadCobidReq_OldValid_NewInvalid) {
  StartCSDO();

  const co_unsigned32_t new_cobid_req = DEFAULT_COBID_REQ | CO_SDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, IDX, 0x01u, CO_DEFTYPE_UNSIGNED32, &new_cobid_req,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, 0x01u, 0u, nullptr);

  CHECK_EQUAL(new_cobid_req, obj1280->GetSub<Sub01CobIdReq>());
}

/// \Given a pointer to the device (co_dev_t) with the CSDO service started
///        and the object dictionary containing the 0x1280 object with
///        a client parameter "COB-ID server -> client (rx)" entry
///
/// \When the same COB-ID value is downloaded to the client parameter "COB-ID
///       server -> client (tx)" entry (idx: 0x1280, subidx: 0x02)
///
/// \Then 0 is returned, download confirmation function is called once,
///       0 is set as the abort code, the requested entry is not changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_CsdoDnInd, DownloadCobidRes_SameAsOld) {
  StartCSDO();

  const co_unsigned32_t new_cobid_res = DEFAULT_COBID_RES;
  const auto ret =
      co_dev_dn_val_req(dev, IDX, 0x02u, CO_DEFTYPE_UNSIGNED32, &new_cobid_res,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, 0x02u, 0u, nullptr);

  CHECK_EQUAL(DEFAULT_COBID_RES, obj1280->GetSub<Sub02CobIdRes>());
}

/// \Given a pointer to the device (co_dev_t) with the CSDO service started
///        and the object dictionary containing the 0x1280 object with
///        a valid client parameter "COB-ID server -> client (rx)" entry
///
/// \When a new valid COB-ID with a new CAN-ID is downloaded to the client
///       parameter "COB-ID server -> client (tx)" entry (idx: 0x1280,
///       subidx: 0x02)
///
/// \Then 0 is returned, download confirmation function is called once,
///       CO_SDO_AC_PARAM_VAL is set as the abort code, the requested entry
///       is not changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_CsdoDnInd, DownloadCobidRes_OldValid_NewValid_NewId) {
  StartCSDO();

  const co_unsigned32_t new_cobid_res = DEFAULT_COBID_RES + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, IDX, 0x02u, CO_DEFTYPE_UNSIGNED32, &new_cobid_res,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, 0x02u, CO_SDO_AC_PARAM_VAL, nullptr);

  CHECK_EQUAL(DEFAULT_COBID_RES, obj1280->GetSub<Sub02CobIdRes>());
}

/// \Given a pointer to the device (co_dev_t) with the CSDO service started
///        and the object dictionary containing the 0x1280 object with a valid
///        client parameter "COB-ID server -> client (rx)" entry
///
/// \When a new valid COB-ID with a CAN-ID with an old value but extended
///       flag set is downloaded to the client parameter
///       "COB-ID server -> client (tx)" entry (idx: 0x1280, subidx: 0x02)
///
/// \Then 0 is returned, download confirmation function is called once,
///       0 is set as the abort code, the requested entry is changed to
///       the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_CsdoDnInd, DownloadCobidRes_OldValid_NewValid_OldIdExtended) {
  StartCSDO();

  const co_unsigned32_t new_cobid_res = DEFAULT_COBID_RES | CO_SDO_COBID_FRAME;
  const auto ret =
      co_dev_dn_val_req(dev, IDX, 0x02u, CO_DEFTYPE_UNSIGNED32, &new_cobid_res,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, 0x02u, 0u, nullptr);

  CHECK_EQUAL(new_cobid_res, obj1280->GetSub<Sub02CobIdRes>());
}

/// \Given a pointer to the device (co_dev_t) with the CSDO service started
///        and the object dictionary containing the 0x1280 object with
///        a valid client parameter "COB-ID server -> client (rx)" entry
///
/// \When a new invalid COB-ID with a new extended CAN-ID but extended
///       flag not set is downloaded to the client parameter
///       "COB-ID server -> client (tx)" entry (idx: 0x1280, subidx: 0x02)
///
/// \Then 0 is returned, download confirmation function is called once,
///       CO_SDO_AC_PARAM_VAL is set as the abort code, the requested entry is
///       not changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_CsdoDnInd, DownloadCobidRes_NewIdExtended_FrameBitNotSet) {
  StartCSDO();

  const co_unsigned32_t DEFAULT_COBID_RES_EXT = DEFAULT_COBID_RES | (1 << 28u);
  const co_unsigned32_t new_cobid_res =
      DEFAULT_COBID_RES_EXT | CO_SDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, IDX, 0x02u, CO_DEFTYPE_UNSIGNED32, &new_cobid_res,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, 0x02u, CO_SDO_AC_PARAM_VAL, nullptr);

  CHECK_EQUAL(DEFAULT_COBID_RES, obj1280->GetSub<Sub02CobIdRes>());
}

/// \Given a pointer to the device (co_dev_t) with the CSDO service started
///        and the object dictionary containing the 0x1280 object with
///        an invalid client parameter "COB-ID server -> client (rx)" entry
///
/// \When a new valid COB-ID with the same CAN-ID is downloaded to the client
///       parameter "COB-ID server -> client (tx)" entry (idx: 0x1280,
///       subidx: 0x02)
///
/// \Then 0 is returned, download confirmation function is called once,
///       0 is set as the abort code, the requested entry is changed to
///       the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_CsdoDnInd, DownloadCobidRes_OldInvalid_NewValid_OldId) {
  obj1280->SetSub<Sub02CobIdRes>(obj1280->GetSub<Sub02CobIdRes>() |
                                 CO_SDO_COBID_VALID);
  StartCSDO();

  const co_unsigned32_t new_cobid_res = DEFAULT_COBID_RES;
  const auto ret =
      co_dev_dn_val_req(dev, IDX, 0x02u, CO_DEFTYPE_UNSIGNED32, &new_cobid_res,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, 0x02u, 0u, nullptr);

  CHECK_EQUAL(new_cobid_res, obj1280->GetSub<Sub02CobIdRes>());
}

/// \Given a pointer to the device (co_dev_t) with the CSDO service started
///        and the object dictionary containing the 0x1280 object with
///        a valid client parameter "COB-ID server -> client (rx)" entry
///
/// \When a new invalid COB-ID with an old CAN-ID is downloaded to the client
///       parameter "COB-ID server -> client (tx)" entry (idx: 0x1280,
///       subidx: 0x02)
///
/// \Then 0 is returned, download confirmation function is called once,
///       0 is set as the abort code, the requested entry is changed to
///       the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_CsdoDnInd, DownloadCobidRes_OldValid_NewInvalid_OldId) {
  StartCSDO();

  const co_unsigned32_t new_cobid_res = DEFAULT_COBID_RES | CO_SDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, IDX, 0x02u, CO_DEFTYPE_UNSIGNED32, &new_cobid_res,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, 0x02u, 0u, nullptr);

  CHECK_EQUAL(new_cobid_res, obj1280->GetSub<Sub02CobIdRes>());
}

/// \Given a pointer to the device (co_dev_t) with the CSDO service started and
///        the object 0x1280 inserted and a valid Node-ID
///
/// \When Node-ID with the same value as the current Node-ID is downloaded to
///       the client parameter "Node-ID of the SDO server" entry (idx: 0x1280,
///       subidx: 0x03)
///
/// \Then 0 is returned, confirmation function is called once with 0 as abort
///       code and the COB-ID is not changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_CsdoDnInd, DownloadNodeId_SameAsOld) {
  StartCSDO();

  const co_unsigned8_t new_id = SERVER_NODEID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1280u, 0x03u, CO_DEFTYPE_UNSIGNED8, &new_id,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, 0x03u, 0u, nullptr);
  CHECK_EQUAL(SERVER_NODEID, obj1280->GetSub<Sub03NodeId>());
}

/// \Given a pointer to the device (co_dev_t) with the CSDO service started and
///        the object 0x1280 inserted and a valid Node-ID
///
/// \When Node-ID new value is downloaded to the client parameter "Node-ID of
///       the SDO server" entry (idx: 0x1280, subidx: 0x03)
///
/// \Then 0 is returned, confirmation function is called once with 0 as abort
///       code and the requested Node-ID value is set
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_sub_dn()
///       \Calls can_recv_start()
TEST(CO_CsdoDnInd, DownloadNodeId_Nominal) {
  StartCSDO();

  const co_unsigned8_t new_id = 0x01u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1280u, 0x03u, CO_DEFTYPE_UNSIGNED8, &new_id,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, 0x03u, 0u, nullptr);
  CHECK_EQUAL(new_id, obj1280->GetSub<Sub03NodeId>());
}

/// \Given a pointer to the device (co_dev_t) with the CSDO service started and
///        the object 0x1280 inserted
///
/// \When a value is downloaded to an invalid sub-object entry (idx: 0x1280,
///       subidx: 0x04)
///
/// \Then 0 is returned, confirmation function is called once with
///       CO_SDO_AC_NO_SUB as abort code
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_CsdoDnInd, DownloadNodeId_InvalidSubidx) {
  obj1280->InsertAndSetSub(0x04u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0x00u});
  StartCSDO();

  const co_unsigned8_t data = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1280u, 0x04u, CO_DEFTYPE_UNSIGNED8, &data,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, IDX, 0x04u, CO_SDO_AC_NO_SUB, nullptr);
}

///@}
