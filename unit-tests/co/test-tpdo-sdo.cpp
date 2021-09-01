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
#include <lely/co/tpdo.h>
#include <lely/co/sdo.h>
#include <lely/util/time.h>

#include <libtest/allocators/default.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>
#include <libtest/tools/can-send.hpp>
#include <libtest/tools/co-tpdo-ind.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "obj-init/tpdo-comm-par.hpp"
#include "obj-init/tpdo-map-par.hpp"

TEST_BASE(CO_SdoTpdoBase) {
  TEST_BASE_SUPER(CO_SdoTpdoBase);
  Allocators::Default allocator;

  const co_unsigned8_t DEV_ID = 0x01u;
  const co_unsigned16_t TPDO_NUM = 0x0001u;

  co_dev_t* dev = nullptr;
  can_net_t* net = nullptr;
  co_tpdo_t* tpdo = nullptr;

  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1800;
  std::unique_ptr<CoObjTHolder> obj1a00;

  int32_t ind_data = 0;

  void CreateTpdoAndStart() {
    tpdo = co_tpdo_create(net, dev, TPDO_NUM);
    CHECK(tpdo != nullptr);

    co_tpdo_set_ind(tpdo, CoTpdoInd::Func, &ind_data);
    co_tpdo_start(tpdo);
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(allocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    dev_holder->CreateObj<Obj1800TpdoCommPar>(obj1800);
    dev_holder->CreateObj<Obj1a00TpdoMapPar>(obj1a00);

    can_net_set_send_func(net, CanSend::Func, nullptr);
  }

  TEST_TEARDOWN() {
    CoCsdoDnCon::Clear();

    co_tpdo_destroy(tpdo);

    dev_holder.reset();
    can_net_destroy(net);
  }
};

TEST_GROUP_BASE(CO_SdoTpdo1800, CO_SdoTpdoBase) {
  using Sub00HighestSubidxSupported =
      Obj1800TpdoCommPar::Sub00HighestSubidxSupported;
  using Sub01CobId = Obj1800TpdoCommPar::Sub01CobId;
  using Sub02TransmissionType = Obj1800TpdoCommPar::Sub02TransmissionType;
  using Sub03InhibitTime = Obj1800TpdoCommPar::Sub03InhibitTime;
  using Sub04Reserved = Obj1800TpdoCommPar::Sub04Reserved;
  using Sub05EventTimer = Obj1800TpdoCommPar::Sub05EventTimer;
  using Sub06SyncStartValue = Obj1800TpdoCommPar::Sub06SyncStartValue;

  void AdvanceTimeMs(const uint32_t ms) {
    timespec ts = {0, 0};
    can_net_get_time(net, &ts);
    timespec_add_msec(&ts, ms);
    can_net_set_time(net, &ts);
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    obj1800->EmplaceSub<Sub00HighestSubidxSupported>(0x06u);
    obj1800->EmplaceSub<Sub01CobId>(DEV_ID);
    obj1800->EmplaceSub<Sub02TransmissionType>();
    obj1800->EmplaceSub<Sub03InhibitTime>(0);
    obj1800->EmplaceSub<Sub04Reserved>();
    obj1800->EmplaceSub<Sub05EventTimer>(0);
    obj1800->EmplaceSub<Sub06SyncStartValue>(0);
  }

  TEST_TEARDOWN() {
    CoTpdoInd::Clear();

    TEST_BASE_TEARDOWN();
  }
};

// NOLINTNEXTLINE(whitespace/line_length)
/// @name TPDO service: the TPDO Communication Parameter object 0x1800 modification using SDO
///@{

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800)
///
/// \When the download indication function for the object 0x1800 is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_SdoTpdo1800, Co1800DnInd_NonZeroAbortCode) {
  CreateTpdoAndStart();

  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret = LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1800u, 0x00u,
                                                        CO_SDO_AC_ERROR);

  CHECK_EQUAL(ac, ret);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with
///        a sub-object
///
/// \When a value longer than the sub-object's data type length is downloaded
///       to the sub-object
///
/// \Then CO_SDO_AC_TYPE_LEN_HI abort code is passed to the download
///       confirmation function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
TEST(CO_SdoTpdo1800, Co1800DnInd_TypeLenTooHigh) {
  CreateTpdoAndStart();
  const auto val = obj1800->GetSub<Sub00HighestSubidxSupported>();

  const co_unsigned16_t value = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x00u, CO_DEFTYPE_UNSIGNED16, &value,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x00u, CO_SDO_AC_TYPE_LEN_HI, nullptr);
  CHECK_EQUAL(val, obj1800->GetSub<Sub00HighestSubidxSupported>());
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with no
///        sub-object at a given sub-index
///
/// \When any value is downloaded to the sub-index
///
/// \Then CO_SDO_AC_NO_SUB abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_SdoTpdo1800, Co1800DnInd_NoSub) {
  CreateTpdoAndStart();

  const co_unsigned8_t idx = 0x07u;
  const co_unsigned16_t val = 0u;

  const auto ret = co_dev_dn_val_req(dev, 0x1800u, idx, CO_DEFTYPE_UNSIGNED16,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, idx, CO_SDO_AC_NO_SUB, nullptr);
  CHECK_EQUAL(0, co_dev_get_val_u16(dev, 0x1800u, idx));
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "Highest sub-index supported" sub-object (0x00)
///
/// \When any value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_NO_WRITE abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub00HighestSubidxSupported_NoWrite) {
  CreateTpdoAndStart();
  const auto val = obj1800->GetSub<Sub00HighestSubidxSupported>();

  const co_unsigned8_t num = 0x7fu;
  const auto ret = co_dev_dn_val_req(dev, 0x1800u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &num, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x00u, CO_SDO_AC_NO_WRITE, nullptr);
  CHECK_EQUAL(val, obj1800->GetSub<Sub00HighestSubidxSupported>());
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "COB-ID used by TPDO" sub-object (0x01)
///
/// \When the same value as the current sub-object's value is downloaded to the
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object remains unchanged
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_sub_dn()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub01Cobid_SameValue) {
  CreateTpdoAndStart();
  const auto val = obj1800->GetSub<Sub01CobId>();

  const co_unsigned32_t cobid = DEV_ID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x01u, 0, nullptr);
  CHECK_EQUAL(val, obj1800->GetSub<Sub01CobId>());
  CHECK_EQUAL(val, co_tpdo_get_comm_par(tpdo)->cobid);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "COB-ID used by TPDO" sub-object (0x01) set to a valid COB-ID
///
/// \When a valid COB-ID with a different CAN-ID is downloaded to the
///       sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download
///       confirmation function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub01Cobid_ValidToValid_NewCanId) {
  CreateTpdoAndStart();
  const auto val = obj1800->GetSub<Sub01CobId>();

  const co_unsigned32_t cobid = DEV_ID + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x01u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1800->GetSub<Sub01CobId>());
  CHECK_EQUAL(val, co_tpdo_get_comm_par(tpdo)->cobid);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "COB-ID used by TPDO" sub-object (0x01) set to an invalid COB-ID
///
/// \When a valid COB-ID with a different CAN-ID is downloaded to the
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value and the value is updated
///       in the service
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls can_net_get_time()
///       \Calls can_recv_stop()
///       \Calls can_timer_stop()
///       \Calls co_sub_dn()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub01Cobid_InvalidToValid_NewCanId) {
  obj1800->SetSub<Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  CreateTpdoAndStart();

  const co_unsigned32_t cobid = DEV_ID + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x01u, 0, nullptr);
  CHECK_EQUAL(cobid, obj1800->GetSub<Sub01CobId>());
  CHECK_EQUAL(cobid, co_tpdo_get_comm_par(tpdo)->cobid);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "COB-ID used by TPDO" sub-object (0x01) set to a valid COB-ID
///
/// \When an invalid COB-ID with the same CAN-ID is downloaded to the
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value and the value is updated
///       in the service
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls can_recv_stop()
///       \Calls can_timer_stop()
///       \Calls co_sub_dn()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub01Cobid_ValidToInvalid_SameCanId) {
  CreateTpdoAndStart();

  const co_unsigned32_t cobid =
      obj1800->GetSub<Sub01CobId>() | CO_PDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x01u, 0, nullptr);
  CHECK_EQUAL(cobid, obj1800->GetSub<Sub01CobId>());
  CHECK_EQUAL(cobid, co_tpdo_get_comm_par(tpdo)->cobid);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "COB-ID used by TPDO" sub-object (0x01) set to a valid COB-ID
///
/// \When the same valid COB-ID, but with the `CO_PDO_COBID_FRAME` bit set is
///       downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value and the value is updated
///       in the service
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls can_recv_stop()
///       \Calls can_timer_stop()
///       \Calls co_sub_dn()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub01Cobid_ValidToValidWithFrameBit) {
  CreateTpdoAndStart();

  const co_unsigned32_t cobid =
      obj1800->GetSub<Sub01CobId>() | CO_PDO_COBID_FRAME;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x01u, 0, nullptr);
  CHECK_EQUAL(cobid, obj1800->GetSub<Sub01CobId>());
  CHECK_EQUAL(cobid, co_tpdo_get_comm_par(tpdo)->cobid);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "COB-ID used by TPDO" sub-object (0x01)
///
/// \When a invalid COB-ID with the 29-bit Extended Identifier, but with no
///       `CO_PDO_COBID_FRAME` bit set is downloaded to the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download
///       confirmation function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_SdoTpdo1800,
     Co1800DnInd_Sub01Cobid_ValidToInvalidWithExtendedIdButNoFrameBit) {
  CreateTpdoAndStart();
  const auto val = obj1800->GetSub<Sub01CobId>();

  const co_unsigned32_t cobid = CAN_MASK_EID | CO_PDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x01u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1800->GetSub<Sub01CobId>());
  CHECK_EQUAL(val, co_tpdo_get_comm_par(tpdo)->cobid);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "Transmission type" sub-object (0x02)
///
/// \When the same value as the current sub-object's value is downloaded to the
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_sub_dn()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub02TransmissionType_SameValue) {
  CreateTpdoAndStart();
  const auto val = obj1800->GetSub<Sub02TransmissionType>();

  const auto ret = co_dev_dn_val_req(dev, 0x1800u, 0x02u, CO_DEFTYPE_UNSIGNED8,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x02u, 0, nullptr);
  CHECK_EQUAL(val, obj1800->GetSub<Sub02TransmissionType>());
  CHECK_EQUAL(val, co_tpdo_get_comm_par(tpdo)->trans);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "Transmission type" sub-object (0x02)
///
/// \When a reserved transmission type value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub02TransmissionType_Reserved) {
  CreateTpdoAndStart();
  const auto val = obj1800->GetSub<Sub02TransmissionType>();

  for (co_unsigned8_t trans = 0xf1u; trans < 0xfcu; trans++) {
    const auto ret =
        co_dev_dn_val_req(dev, 0x1800u, 0x02u, CO_DEFTYPE_UNSIGNED8, &trans,
                          nullptr, CoCsdoDnCon::Func, nullptr);

    CHECK_EQUAL(0, ret);
    CoCsdoDnCon::Check(nullptr, 0x1800u, 0x02u, CO_SDO_AC_PARAM_VAL, nullptr);
    CHECK_EQUAL(val, obj1800->GetSub<Sub02TransmissionType>());
    CHECK_EQUAL(val, co_tpdo_get_comm_par(tpdo)->trans);

    CoCsdoDnCon::Clear();
  }
}

/// \Given a started TPDO service (co_tpdo_t) configured with the RTR not
///        allowed in COB-ID, the object dictionary contains the TPDO
///        Communication Parameter object (0x1800) with the "Transmission type"
///        sub-object (0x02)
///
/// \When the RTR-only (synchronous) transmission type value is downloaded to
///       the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_SdoTpdo1800,
     Co1800DnInd_Sub02TransmissionType_SynchronousRTR_ButRTRNotAllowed) {
  obj1800->SetSub<Sub01CobId>(DEV_ID | CO_PDO_COBID_RTR);
  CreateTpdoAndStart();
  const auto val = obj1800->GetSub<Sub02TransmissionType>();

  const co_unsigned8_t trans = Obj1800TpdoCommPar::SYNCHRONOUS_RTR_TRANSMISSION;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x02u, CO_DEFTYPE_UNSIGNED8, &trans,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x02u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1800->GetSub<Sub02TransmissionType>());
  CHECK_EQUAL(val, co_tpdo_get_comm_par(tpdo)->trans);
}

/// \Given a started TPDO service (co_tpdo_t) configured with the RTR not
///        allowed in COB-ID, the object dictionary contains the TPDO
///        Communication Parameter object (0x1800) with the "Transmission type"
///        sub-object (0x02)
///
/// \When the RTR-only (event-driven) transmission type value is downloaded to
///       the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_SdoTpdo1800,
     Co1800DnInd_Sub02TransmissionType_EventDrivenRTR_ButRTRNotAllowed) {
  obj1800->SetSub<Sub01CobId>(DEV_ID | CO_PDO_COBID_RTR);
  CreateTpdoAndStart();
  const auto val = obj1800->GetSub<Sub02TransmissionType>();

  const co_unsigned8_t trans =
      Obj1800TpdoCommPar::EVENT_DRIVEN_RTR_TRANSMISSION;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x02u, CO_DEFTYPE_UNSIGNED8, &trans,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x02u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1800->GetSub<Sub02TransmissionType>());
  CHECK_EQUAL(val, co_tpdo_get_comm_par(tpdo)->trans);
}

/// \Given a started TPDO service (co_tpdo_t) configured with the RTR allowed
///        in COB-ID, the object dictionary contains the TPDO Communication
///        Parameter object (0x1800) with the "Transmission type" sub-object
///        (0x02)
///
/// \When an RTR-only transmission type value is downloaded to
///       the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls can_recv_start()
///       \Calls co_sub_dn()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub02TransmissionType_RTROnly_RTRAllowed) {
  CreateTpdoAndStart();

  const co_unsigned8_t trans =
      Obj1800TpdoCommPar::EVENT_DRIVEN_RTR_TRANSMISSION;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x02u, CO_DEFTYPE_UNSIGNED8, &trans,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x02u, 0, nullptr);
  CHECK_EQUAL(trans, obj1800->GetSub<Sub02TransmissionType>());
  CHECK_EQUAL(trans, co_tpdo_get_comm_par(tpdo)->trans);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "Transmission type" sub-object (0x02)
///
/// \When a transmission type value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value and the value is updated
///       in the service
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_sub_dn()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub02TransmissionType_NewValue) {
  CreateTpdoAndStart();

  const co_unsigned8_t trans = 0x35u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x02u, CO_DEFTYPE_UNSIGNED8, &trans,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x02u, 0, nullptr);
  CHECK_EQUAL(trans, obj1800->GetSub<Sub02TransmissionType>());
  CHECK_EQUAL(trans, co_tpdo_get_comm_par(tpdo)->trans);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "Transmission type" sub-object (0x02)
///
/// \When the maximum transmission type value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value and the value is updated
///       in the service
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_sub_dn()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub02Transmission_MaxValue) {
  CreateTpdoAndStart();

  const co_unsigned8_t trans = CO_UNSIGNED8_MAX;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x02u, CO_DEFTYPE_UNSIGNED8, &trans,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x02u, 0, nullptr);
  CHECK_EQUAL(trans, obj1800->GetSub<Sub02TransmissionType>());
  CHECK_EQUAL(trans, co_tpdo_get_comm_par(tpdo)->trans);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "Inhibit time" sub-object (0x03)
///
/// \When the same value as the current sub-object's value is downloaded to the
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u16()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub03InhibitTime_SameValue) {
  obj1800->SetSub<Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  CreateTpdoAndStart();
  const auto val = obj1800->GetSub<Sub03InhibitTime>();

  const auto ret = co_dev_dn_val_req(dev, 0x1800u, 0x03u, CO_DEFTYPE_UNSIGNED16,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x03u, 0, nullptr);
  CHECK_EQUAL(val, obj1800->GetSub<Sub03InhibitTime>());
  CHECK_EQUAL(val, co_tpdo_get_comm_par(tpdo)->inhibit);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "Inhibit time" sub-object (0x03); the TPDO is valid
///
/// \When a time value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u16()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub03InhibitTime_ValidTpdo) {
  CreateTpdoAndStart();
  const auto val = obj1800->GetSub<Sub03InhibitTime>();

  const co_unsigned16_t inhibit_time = 123u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x03u, CO_DEFTYPE_UNSIGNED16,
                        &inhibit_time, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x03u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1800->GetSub<Sub03InhibitTime>());
  CHECK_EQUAL(val, co_tpdo_get_comm_par(tpdo)->inhibit);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "Inhibit time" sub-object (0x03); the TPDO is invalid
///
/// \When a time value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value and the value is updated
///       in the service
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u16()
///       \Calls co_sub_dn()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub03InhibitTime_Nominal) {
  obj1800->SetSub<Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  CreateTpdoAndStart();

  const co_unsigned16_t inhibit_time = 123u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x03u, CO_DEFTYPE_UNSIGNED16,
                        &inhibit_time, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x03u, 0, nullptr);
  CHECK_EQUAL(inhibit_time, obj1800->GetSub<Sub03InhibitTime>());
  CHECK_EQUAL(inhibit_time, co_tpdo_get_comm_par(tpdo)->inhibit);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "Compatibility entry" sub-object (0x04)
///
/// \When any value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_NO_SUB abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub04CompatibilityEntry_NoSub) {
  CreateTpdoAndStart();
  const auto val = obj1800->GetSub<Sub04Reserved>();

  const co_unsigned8_t compat = CO_UNSIGNED8_MAX;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x04u, CO_DEFTYPE_UNSIGNED8, &compat,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x04u, CO_SDO_AC_NO_SUB, nullptr);
  CHECK_EQUAL(val, obj1800->GetSub<Sub04Reserved>());
  CHECK_EQUAL(val, co_tpdo_get_comm_par(tpdo)->reserved);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "Event timer" sub-object (0x05)
///
/// \When the same value as the current sub-object's value is downloaded to the
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u16()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub05EventTimer_SameValue) {
  CreateTpdoAndStart();
  const auto val = obj1800->GetSub<Sub05EventTimer>();

  const auto ret = co_dev_dn_val_req(dev, 0x1800u, 0x05u, CO_DEFTYPE_UNSIGNED16,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x05u, 0, nullptr);
  CHECK_EQUAL(val, obj1800->GetSub<Sub05EventTimer>());
  CHECK_EQUAL(val, co_tpdo_get_comm_par(tpdo)->event);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "Event timer" sub-object (0x05); the event timer is enabled
///
/// \When a new timer value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value and the value is updated
///       in the service; the event timer is updated and restarted
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u16()
///       \Calls can_timer_stop()
///       \Calls can_timer_timeout()
///       \Calls co_sub_dn()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub05EventTimer_NewValue) {
  const co_unsigned16_t old_event_timer_ms = 20u;
  obj1800->SetSub<Sub02TransmissionType>(
      Obj1800TpdoCommPar::EVENT_DRIVEN_TRANSMISSION);
  obj1800->SetSub<Sub05EventTimer>(old_event_timer_ms);
  CreateTpdoAndStart();

  AdvanceTimeMs(old_event_timer_ms - 1u);

  const co_unsigned16_t event_timer_ms = 10u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x05u, CO_DEFTYPE_UNSIGNED16,
                        &event_timer_ms, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x05u, 0, nullptr);
  CHECK_EQUAL(event_timer_ms, obj1800->GetSub<Sub05EventTimer>());
  CHECK_EQUAL(event_timer_ms, co_tpdo_get_comm_par(tpdo)->event);

  AdvanceTimeMs(event_timer_ms);

  CHECK_EQUAL(1u, CoTpdoInd::GetNumCalled());
  CoTpdoInd::CheckPtrNotNull(tpdo, 0, 0, &ind_data);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "Event timer" sub-object (0x05); the event timer is enabled
///
/// \When a zero timer value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value and the value is updated
///       in the service; the event timer is stopped and disabled
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u16()
///       \Calls can_timer_stop()
///       \Calls co_sub_dn()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub05EventTimer_DisableTimer) {
  const co_unsigned16_t old_event_timer_ms = 10u;
  obj1800->SetSub<Sub05EventTimer>(old_event_timer_ms);
  CreateTpdoAndStart();

  AdvanceTimeMs(old_event_timer_ms - 1u);

  const co_unsigned16_t event_timer_ms = 0u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x05u, CO_DEFTYPE_UNSIGNED16,
                        &event_timer_ms, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x05u, 0, nullptr);
  CHECK_EQUAL(event_timer_ms, obj1800->GetSub<Sub05EventTimer>());
  CHECK_EQUAL(event_timer_ms, co_tpdo_get_comm_par(tpdo)->event);

  AdvanceTimeMs(1u);

  CHECK_EQUAL(0, CoTpdoInd::GetNumCalled());
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "SYNC start value" sub-object (0x06)
///
/// \When the same value as the current sub-object's value is downloaded to the
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub06SyncStartValue_SameValue) {
  CreateTpdoAndStart();
  const auto val = obj1800->GetSub<Sub06SyncStartValue>();

  const auto ret = co_dev_dn_val_req(dev, 0x1800u, 0x06u, CO_DEFTYPE_UNSIGNED8,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x06u, 0, nullptr);
  CHECK_EQUAL(val, obj1800->GetSub<Sub06SyncStartValue>());
  CHECK_EQUAL(val, co_tpdo_get_comm_par(tpdo)->sync);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "SYNC start value" sub-object (0x06); the TPDO is valid
///
/// \When a start value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub06SyncStartValue_ValidTpdo) {
  CreateTpdoAndStart();
  const auto val = obj1800->GetSub<Sub06SyncStartValue>();

  const co_unsigned8_t sync = 0x01u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x06u, CO_DEFTYPE_UNSIGNED8, &sync,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x06u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1800->GetSub<Sub06SyncStartValue>());
  CHECK_EQUAL(val, co_tpdo_get_comm_par(tpdo)->sync);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800) with the
///        "SYNC start value" sub-object (0x06); the TPDO is invalid
///
/// \When a start value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value and the value is updated
///       in the service
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_sub_dn()
TEST(CO_SdoTpdo1800, Co1800DnInd_Sub06SyncStartValue_Nominal) {
  obj1800->SetSub<Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  CreateTpdoAndStart();

  const co_unsigned8_t sync = 0x01u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x06u, CO_DEFTYPE_UNSIGNED8, &sync,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1800u, 0x06u, 0, nullptr);
  CHECK_EQUAL(sync, obj1800->GetSub<Sub06SyncStartValue>());
  CHECK_EQUAL(sync, co_tpdo_get_comm_par(tpdo)->sync);
}

///@}

TEST_GROUP_BASE(CO_SdoTpdo1a00, CO_SdoTpdoBase) {
  using Sub00NumOfMappedObjs = Obj1a00TpdoMapPar::Sub00NumOfMappedObjs;
  using SubNthAppObject = Obj1a00TpdoMapPar::SubNthAppObject;
  static constexpr auto MakeMappingParam = Obj1a00TpdoMapPar::MakeMappingParam;

  static const co_unsigned16_t PDO_MAPPED_IDX = 0x2021u;
  static const co_unsigned8_t PDO_MAPPED_SUBIDX = 0x00u;
  static const co_unsigned8_t PDO_MAPPED_LEN = 0x20u;

  std::unique_ptr<CoObjTHolder> obj2021;

  void CreateMappableObject() {
    dev_holder->CreateAndInsertObj(obj2021, PDO_MAPPED_IDX);

    obj2021->InsertAndSetSub(PDO_MAPPED_SUBIDX, CO_DEFTYPE_UNSIGNED32,
                             co_unsigned32_t{0xdeadbeefu});
    co_sub_t* const sub2021 = obj2021->GetLastSub();
    co_sub_set_access(sub2021, CO_ACCESS_RW);
    co_sub_set_pdo_mapping(sub2021, true);
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>();
    obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
    obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(
        Obj1800TpdoCommPar::EVENT_DRIVEN_TRANSMISSION);

    obj1a00->EmplaceSub<Sub00NumOfMappedObjs>(CO_PDO_NUM_MAPS);
    for (co_unsigned8_t i = 1u; i <= CO_PDO_NUM_MAPS; ++i)
      obj1a00->EmplaceSub<SubNthAppObject>(i, 0u);
  }
};

// NOLINTNEXTLINE(whitespace/line_length)
/// @name TPDO service: the TPDO Mapping Parameter object (0x1a00-0x1bfff) modification using SDO
/// @{

/// \Given a started TPDO service (co_tpdo_t), the object dictionary contains
///        the TPDO Mapping Parameter object (0x1a00)
///
/// \When the download indication function for the object 0x1a00 is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_SdoTpdo1a00, Co1a00DnInd_NonZeroAbortCode) {
  CreateTpdoAndStart();

  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret =
      LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1a00u, 0x00u, ac);

  CHECK_EQUAL(ac, ret);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary contains
///        the TPDO Mapping Parameter object (0x1a00) with a sub-object
///
/// \When a value longer than the sub-object's data type length is downloaded
///       to the sub-object
///
/// \Then CO_SDO_AC_TYPE_LEN_HI abort code is passed to the download
///       confirmation function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
TEST(CO_SdoTpdo1a00, Co1a00DnInd_TypeLenTooHigh) {
  CreateTpdoAndStart();
  const auto val = obj1a00->GetSub<Sub00NumOfMappedObjs>();

  const co_unsigned32_t value = CO_UNSIGNED32_MAX;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED32, &value,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1a00u, 0x00u, CO_SDO_AC_TYPE_LEN_HI, nullptr);
  CHECK_EQUAL(val, obj1a00->GetSub<Sub00NumOfMappedObjs>());
  CHECK_EQUAL(val, co_tpdo_get_map_par(tpdo)->n);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary contains
///        the TPDO Mapping Parameter object (0x1a00) with the "Number of
///        mapped objects" sub-object (0x00)
///
/// \When the same value as the current sub-object's value is downloaded to the
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object remains unchanged
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_SdoTpdo1a00, Co1a00DnInd_Sub00NumOfMappedObjs_SameValue) {
  CreateTpdoAndStart();

  const co_unsigned8_t num = obj1a00->GetSub<Sub00NumOfMappedObjs>();
  const auto ret = co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &num, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1a00u, 0x00u, 0, nullptr);
  CHECK_EQUAL(num, obj1a00->GetSub<Sub00NumOfMappedObjs>());
  CHECK_EQUAL(num, co_tpdo_get_map_par(tpdo)->n);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary contains
///        the TPDO Mapping Parameter object (0x1a00) with the "Number of
///        mapped objects" sub-object (0x00); the TPDO is valid
///
/// \When any value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_SdoTpdo1a00, Co1a00DnInd_Sub00NumOfMappedObjs_ValidTpdo) {
  CreateTpdoAndStart();
  const auto val = obj1a00->GetSub<Sub00NumOfMappedObjs>();

  const co_unsigned8_t num = 2u;
  const auto ret = co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &num, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1a00u, 0x00u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1a00->GetSub<Sub00NumOfMappedObjs>());
  CHECK_EQUAL(val, co_tpdo_get_map_par(tpdo)->n);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary contains
///        the TPDO Mapping Parameter object (0x1a00) with the "Number of
///        mapped objects" sub-object (0x00); the TPDO is invalid
///
/// \When a value larger than CO_PDO_NUM_MAPS is downloaded to the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_SdoTpdo1a00, Co1a00DnInd_Sub00NumOfMappedObjs_NumOverMax) {
  obj1800->SetSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  CreateTpdoAndStart();
  const auto val = obj1a00->GetSub<Sub00NumOfMappedObjs>();

  const co_unsigned8_t num = CO_PDO_NUM_MAPS + 1u;
  const auto ret = co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &num, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1a00u, 0x00u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1a00->GetSub<Sub00NumOfMappedObjs>());
  CHECK_EQUAL(val, co_tpdo_get_map_par(tpdo)->n);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary contains
///        the TPDO Mapping Parameter object (0x1a00) with the "Number of
///        mapped objects" sub-object (0x00) and an "Application object" entry
///        with an empty mapping; the TPDO is invalid
///
/// \When a new number of mapped objects value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value and the value is updated
///       in the service
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_dev_chk_tpdo()
///       \Calls co_sub_dn()
TEST(CO_SdoTpdo1a00, Co1a00DnInd_Sub00NumOfMappedObjs_EmptyMapping) {
  obj1800->SetSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1a00->SetSub<SubNthAppObject>(0x01u, 0u);
  CreateTpdoAndStart();

  const co_unsigned8_t num = 1u;
  const auto ret = co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &num, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1a00u, 0x00u, 0, nullptr);
  CHECK_EQUAL(num, obj1a00->GetSub<Sub00NumOfMappedObjs>());
  CHECK_EQUAL(num, co_tpdo_get_map_par(tpdo)->n);
  CHECK_EQUAL(0, co_tpdo_get_map_par(tpdo)->map[0]);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary contains
///        the TPDO Mapping Parameter object (0x1a00) with the "Number of
///        mapped objects" sub-object (0x00) and an "Application object" entry
///        with a mapping that exceeds the maximum PDO length; the TPDO is
///        invalid
///
/// \When a new number of mapped objects value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_PDO_LEN abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_SdoTpdo1a00, Co1a00DnInd_Sub00NumOfMappedObjs_MappingLenOverMax) {
  obj1800->SetSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1a00->SetSub<SubNthAppObject>(
      0x01u, MakeMappingParam(PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX, 0xffu));
  CreateMappableObject();
  CreateTpdoAndStart();
  const auto val = obj1a00->GetSub<Sub00NumOfMappedObjs>();

  const co_unsigned8_t num = 1u;
  const auto ret = co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &num, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1a00u, 0x00u, CO_SDO_AC_PDO_LEN, nullptr);
  CHECK_EQUAL(val, obj1a00->GetSub<Sub00NumOfMappedObjs>());
  CHECK_EQUAL(val, co_tpdo_get_map_par(tpdo)->n);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary contains
///        the TPDO Mapping Parameter object (0x1a00) with the "Number of
///        mapped objects" sub-object (0x00) and an "Application object" entry
///        that maps into a non-existing object; the TPDO is invalid
///
/// \When a new number of mapped objects value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_NO_OBJ abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_dev_chk_tpdo()
TEST(CO_SdoTpdo1a00, Co1a00DnInd_Sub00NumOfMappedObjs_MappingNonExistingObj) {
  obj1800->SetSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1a00->SetSub<SubNthAppObject>(0x01u,
                                   MakeMappingParam(0xffffu, 0x00u, 0x00u));
  CreateTpdoAndStart();
  const auto val = obj1a00->GetSub<Sub00NumOfMappedObjs>();

  const co_unsigned8_t num = 1u;
  const auto ret = co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &num, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1a00u, 0x00u, CO_SDO_AC_NO_OBJ, nullptr);
  CHECK_EQUAL(val, obj1a00->GetSub<Sub00NumOfMappedObjs>());
  CHECK_EQUAL(val, co_tpdo_get_map_par(tpdo)->n);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary contains
///        the TPDO Mapping Parameter object (0x1a00) with the "Number of
///        mapped objects" sub-object (0x00); the TPDO is invalid
///
/// \When a zero value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_SdoTpdo1a00, Co1a00DnInd_Sub00NumOfMappedObjs_NoMappings) {
  CreateTpdoAndStart();
  const auto val = obj1a00->GetSub<Sub00NumOfMappedObjs>();

  const co_unsigned8_t num = 0;
  const auto ret = co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &num, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1a00u, 0x00u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1a00->GetSub<Sub00NumOfMappedObjs>());
  CHECK_EQUAL(val, co_tpdo_get_map_par(tpdo)->n);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary contains
///        the TPDO Mapping Parameter object (0x1a00) with the "Number of
///        mapped objects" sub-object (0x00) and some "Application object"
///        entires; the TPDO is invalid
///
/// \When a non-zero value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value and the value is updated
///       in the service
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_dev_chk_tpdo()
///       \Calls co_sub_dn()
TEST(CO_SdoTpdo1a00, Co1a00DnInd_Sub00NumOfMappedObjs_Nominal) {
  obj1800->SetSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1a00->SetSub<SubNthAppObject>(
      0x01u,
      MakeMappingParam(PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX, PDO_MAPPED_LEN));
  CreateMappableObject();
  CreateTpdoAndStart();

  const co_unsigned8_t num = 1u;
  const auto ret = co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &num, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1a00u, 0x00u, 0, nullptr);
  CHECK_EQUAL(num, obj1a00->GetSub<Sub00NumOfMappedObjs>());
  CHECK_EQUAL(num, co_tpdo_get_map_par(tpdo)->n);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary contains
///        the TPDO Mapping Parameter object (0x1a00) with the "Number of
///        mapped objects" sub-object (0x00) equal to zero and with an
///        "Application object" sub-object (0x01-0x40); the TPDO is invalid
///
/// \When the same value as the current sub-object's value is downloaded to the
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object remains unchanged
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_SdoTpdo1a00, Co1a00DnInd_SubNthAppObj_SameValue) {
  obj1a00->SetSub<SubNthAppObject>(
      0x01u,
      MakeMappingParam(PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX, PDO_MAPPED_LEN));
  CreateMappableObject();
  CreateTpdoAndStart();

  const co_unsigned32_t mapping = obj1a00->GetSub<SubNthAppObject>(0x01u);
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1a00u, 0x01u, 0, nullptr);
  CHECK_EQUAL(mapping, obj1a00->GetSub<SubNthAppObject>(0x01u));
  CHECK_EQUAL(mapping, co_tpdo_get_map_par(tpdo)->map[0]);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary contains
///        the TPDO Mapping Parameter object (0x1a00) with the "Number of
///        mapped objects" sub-object (0x00) equal to zero and with an
///        "Application object" sub-object (0x01-0x40); the TPDO is valid
///
/// \When a new mapping value is downloaded to the "Application object"
///       sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_SdoTpdo1a00, Co1a00DnInd_SubNthAppObj_ValidTpdo) {
  obj1800->SetSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  CreateMappableObject();
  CreateTpdoAndStart();
  const auto val = obj1a00->GetSub<SubNthAppObject>(0x01u);

  const co_unsigned32_t mapping =
      MakeMappingParam(PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX, PDO_MAPPED_LEN);
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1a00u, 0x01u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1a00->GetSub<SubNthAppObject>(0x01u));
  CHECK_EQUAL(val, co_tpdo_get_map_par(tpdo)->map[0]);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary contains
///        the TPDO Mapping Parameter object (0x1a00) with the "Number of
///        mapped objects" sub-object (0x00) does not equal zero and with an
///        "Application object" sub-object (0x01-0x40); the TPDO is invalid
///
/// \When a new mapping value is downloaded to the "Application object"
///       sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_SdoTpdo1a00, Co1a00DnInd_SubNthAppObj_NumOfMappedObjsNonZero) {
  obj1800->SetSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1a00->SetSub<Sub00NumOfMappedObjs>(0x01u);
  CreateMappableObject();
  CreateTpdoAndStart();
  const auto val = obj1a00->GetSub<SubNthAppObject>(0x01u);

  const co_unsigned32_t mapping =
      MakeMappingParam(PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX, PDO_MAPPED_LEN);
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1a00u, 0x01u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1a00->GetSub<SubNthAppObject>(0x01u));
  CHECK_EQUAL(val, co_tpdo_get_map_par(tpdo)->map[0]);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary contains
///        the TPDO Mapping Parameter object (0x1a00) with the "Number of
///        mapped objects" sub-object (0x00) equal to zero and with an
///        "Application object" sub-object (0x01-0x40); the TPDO is invalid
///
/// \When an empty mapping value is downloaded to the "Application object"
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value and the value is updated
///       in the service
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_dev_chk_tpdo()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_dn()
TEST(CO_SdoTpdo1a00, Co1a00DnInd_SubNthAppObj_EmptyMapping) {
  obj1800->SetSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1a00->SetSub<Sub00NumOfMappedObjs>(0x00);
  obj1a00->SetSub<SubNthAppObject>(
      0x01u,
      MakeMappingParam(PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX, PDO_MAPPED_LEN));
  CreateMappableObject();
  CreateTpdoAndStart();

  const co_unsigned32_t mapping = 0u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x01, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1a00u, 0x01u, 0, nullptr);
  CHECK_EQUAL(mapping, obj1a00->GetSub<SubNthAppObject>(0x01u));
  CHECK_EQUAL(mapping, co_tpdo_get_map_par(tpdo)->map[0]);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary contains
///        the TPDO Mapping Parameter object (0x1a00) with the "Number of
///        mapped objects" sub-object (0x00) equal to zero and with an
///        "Application object" sub-object (0x01-0x40); the TPDO is invalid
///
/// \When a mapping that maps into a non-existing object is downloaded to the
///       "Application object" sub-object
///
/// \Then CO_SDO_AC_NO_OBJ abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_dev_chk_tpdo()
TEST(CO_SdoTpdo1a00, Co1a00DnInd_SubNthAppObj_NonExistingObj) {
  obj1800->SetSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1a00->SetSub<Sub00NumOfMappedObjs>(0x00);
  CreateTpdoAndStart();
  const auto val = obj1a00->GetSub<SubNthAppObject>(0x01u);

  const co_unsigned32_t mapping = MakeMappingParam(0xffffu, 0x00u, 0x00u);
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1a00u, 0x01u, CO_SDO_AC_NO_OBJ, nullptr);
  CHECK_EQUAL(val, obj1a00->GetSub<SubNthAppObject>(0x01u));
  CHECK_EQUAL(val, co_tpdo_get_map_par(tpdo)->map[0]);
}

/// \Given a started TPDO service (co_tpdo_t), the object dictionary contains
///        the TPDO Mapping Parameter object (0x1a00) with the "Number of
///        mapped objects" sub-object (0x00) equal to zero and with an
///        "Application object" sub-object (0x01-0x40); the TPDO is invalid
///
/// \When a new mapping value is downloaded to the "Application object"
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value and the value is updated
///       in the service
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_dev_chk_tpdo()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_dn()
TEST(CO_SdoTpdo1a00, Co1a00DnInd_SubNthAppObj_Nominal) {
  obj1800->SetSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1a00->SetSub<Sub00NumOfMappedObjs>(0x00);
  CreateMappableObject();
  CreateTpdoAndStart();

  const co_unsigned32_t mapping =
      MakeMappingParam(PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX, PDO_MAPPED_LEN);
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1a00u, 0x01u, 0, nullptr);
  CHECK_EQUAL(mapping, obj1a00->GetSub<SubNthAppObject>(0x01u));
  CHECK_EQUAL(mapping, co_tpdo_get_map_par(tpdo)->map[0]);
}

///@}
