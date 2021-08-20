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
#include <lely/co/csdo.h>
#include <lely/co/rpdo.h>
#include <lely/co/sdo.h>
#include <lely/util/time.h>

#include <libtest/allocators/default.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>
#include <libtest/tools/co-rpdo-err.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "obj-init/rpdo-comm-par.hpp"
#include "obj-init/rpdo-map-par.hpp"

TEST_BASE(CO_SdoRpdoBase) {
  TEST_BASE_SUPER(CO_SdoRpdoBase);
  Allocators::Default allocator;

  const co_unsigned8_t DEV_ID = 0x01u;
  const co_unsigned16_t RPDO_NUM = 0x0001u;

  can_net_t* net = nullptr;
  co_dev_t* dev = nullptr;
  co_rpdo_t* rpdo = nullptr;

  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1400;
  std::unique_ptr<CoObjTHolder> obj1600;

  void CreateRpdo() {
    rpdo = co_rpdo_create(net, dev, RPDO_NUM);
    CHECK(rpdo != nullptr);

    co_rpdo_set_err(rpdo, &CoRpdoErr::Func, nullptr);
  }

  void CreateRpdoAndStart() {
    CreateRpdo();
    co_rpdo_start(rpdo);
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(allocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    dev_holder->CreateObj<Obj1400RpdoCommPar>(obj1400);
    dev_holder->CreateObj<Obj1600RpdoMapPar>(obj1600);
  }

  TEST_TEARDOWN() {
    CoCsdoDnCon::Clear();

    co_rpdo_destroy(rpdo);

    dev_holder.reset();
    can_net_destroy(net);
  }
};

TEST_GROUP_BASE(CO_SdoRpdo1400, CO_SdoRpdoBase) {
  const int32_t clang_format_fix = 0;  // dummy for workaround

  using Sub00HighestSubidxSupported =
      Obj1400RpdoCommPar::Sub00HighestSubidxSupported;
  using Sub01CobId = Obj1400RpdoCommPar::Sub01CobId;
  using Sub02TransmissionType = Obj1400RpdoCommPar::Sub02TransmissionType;
  using Sub03InhibitTime = Obj1400RpdoCommPar::Sub03InhibitTime;
  using Sub04Reserved = Obj1400RpdoCommPar::Sub04Reserved;
  using Sub05EventTimer = Obj1400RpdoCommPar::Sub05EventTimer;

  void ReceivePdoMessage() {
    can_msg msg = CAN_MSG_INIT;
    msg.id = DEV_ID;
    CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  }

  void AdvanceTimeMs(const uint32_t ms) {
    timespec ts = {0, 0};
    can_net_get_time(net, &ts);
    timespec_add_msec(&ts, ms);
    can_net_set_time(net, &ts);
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x05u);
    obj1400->EmplaceSub<Sub01CobId>(DEV_ID);
    obj1400->EmplaceSub<Sub02TransmissionType>();
    obj1400->EmplaceSub<Sub03InhibitTime>(0);
    obj1400->EmplaceSub<Sub04Reserved>();
    obj1400->EmplaceSub<Sub05EventTimer>(0);
  }

  TEST_TEARDOWN() {
    CoRpdoErr::Clear();

    TEST_BASE_TEARDOWN();
  }
};

// NOLINTNEXTLINE(whitespace/line_length)
/// @name RPDO service: the RPDO Communication Parameter object (0x1400-0x15ff) modification using SDO
///@{

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400)
///
/// \When the download indication function for the object 0x1400 is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_SdoRpdo1400, Co1400DnInd_NonZeroAbortCode) {
  CreateRpdoAndStart();

  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret =
      LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1400u, 0x00u, ac);

  CHECK_EQUAL(ac, ret);
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with
///        a sub-object
///
/// \When a value longer than the sub-object's data type length is downloaded
///       to the sub-object
///
/// \Then CO_SDO_AC_TYPE_LEN_HI abort code is passed to the download
///       confirmation function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
TEST(CO_SdoRpdo1400, Co1400DnInd_TypeLenTooHigh) {
  CreateRpdoAndStart();
  const auto val = obj1400->GetSub<Sub00HighestSubidxSupported>();

  const co_unsigned16_t value = CO_UNSIGNED16_MAX;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x00u, CO_DEFTYPE_UNSIGNED16, &value,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, 0x00u, CO_SDO_AC_TYPE_LEN_HI, nullptr);
  CHECK_EQUAL(val, obj1400->GetSub<Sub00HighestSubidxSupported>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with no
///        sub-object at a given sub-index
///
/// \When any value is downloaded to the sub-index
///
/// \Then CO_SDO_AC_NO_SUB abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_SdoRpdo1400, Co1400DnInd_NoSub) {
  CreateRpdoAndStart();

  const co_unsigned8_t idx = 0x07u;
  const co_unsigned16_t val = 0u;

  const auto ret = co_dev_dn_val_req(dev, 0x1400u, idx, CO_DEFTYPE_UNSIGNED16,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, idx, CO_SDO_AC_NO_SUB, nullptr);
  CHECK_EQUAL(0, co_dev_get_val_u16(dev, 0x1400u, idx));
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
///        "Highest sub-index supported" sub-object (0x00)
///
/// \When any value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_NO_WRITE abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub00MaxSubidxSupported_NoWrite) {
  CreateRpdoAndStart();
  const auto val = obj1400->GetSub<Sub00HighestSubidxSupported>();

  const co_unsigned8_t num_of_elems = 0x7fu;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_elems, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, 0x00u, CO_SDO_AC_NO_WRITE, nullptr);
  CHECK_EQUAL(val, obj1400->GetSub<Sub00HighestSubidxSupported>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
///        "COB-ID used by RPDO" sub-object (0x01)
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
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub01Cobid_SameAsPrevious) {
  CreateRpdoAndStart();
  const auto val = obj1400->GetSub<Sub01CobId>();

  const co_unsigned32_t cobid = DEV_ID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, 0x01u, 0, nullptr);
  CHECK_EQUAL(val, obj1400->GetSub<Sub01CobId>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
///        "COB-ID used by RPDO" sub-object (0x01) set to a valid COB-ID
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
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub01Cobid_ValidToValid_NewCanId) {
  obj1400->SetSub<Sub01CobId>(DEV_ID);
  CreateRpdoAndStart();
  const auto val = obj1400->GetSub<Sub01CobId>();

  const co_unsigned32_t cobid = DEV_ID + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, 0x01u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1400->GetSub<Sub01CobId>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
///        "COB-ID used by RPDO" sub-object (0x01) set to an invalid COB-ID
///
/// \When a valid COB-ID with a different CAN-ID is downloaded to the
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls can_recv_start()
///       \Calls can_timer_stop()
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub01Cobid_InvalidToValid_NewCanId) {
  obj1400->SetSub<Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  CreateRpdoAndStart();

  const co_unsigned32_t cobid = DEV_ID + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, 0x01u, 0, nullptr);
  CHECK_EQUAL(cobid, obj1400->GetSub<Sub01CobId>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
///        "COB-ID used by RPDO" sub-object (0x01) set to a valid COB-ID
///
/// \When an invalid COB-ID with the same CAN-ID is downloaded to the
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls can_recv_stop()
///       \Calls can_timer_stop()
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub01Cobid_ValidToInvalid_SameCanId) {
  CreateRpdoAndStart();

  const co_unsigned32_t cobid =
      obj1400->GetSub<Sub01CobId>() | CO_PDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, 0x01u, 0, nullptr);
  CHECK_EQUAL(cobid, obj1400->GetSub<Sub01CobId>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
///        "COB-ID used by RPDO" sub-object (0x01) set to a valid COB-ID
///
/// \When the same valid COB-ID, but with the `CO_PDO_COBID_FRAME` bit set is
///       downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls can_recv_start()
///       \Calls can_timer_stop()
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub01Cobid_ValidToValidWithFrameBit) {
  CreateRpdoAndStart();

  const co_unsigned32_t cobid =
      obj1400->GetSub<Sub01CobId>() | CO_PDO_COBID_FRAME;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, 0x01u, 0, nullptr);
  CHECK_EQUAL(cobid, obj1400->GetSub<Sub01CobId>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
///        "COB-ID used by RPDO" sub-object (0x01)
///
/// \When a valid COB-ID with the Extended Identifier, but with no
///       `CO_PDO_COBID_FRAME` bit set is downloaded to the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download
///       confirmation function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub01Cobid_ExtendedIdButNoFrameBit) {
  CreateRpdoAndStart();
  const auto val = obj1400->GetSub<Sub01CobId>();

  const co_unsigned32_t cobid = CAN_MASK_EID | CO_PDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, 0x01u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1400->GetSub<Sub01CobId>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
///        "COB-ID used by RPDO" sub-object (0x01) set to a valid COB-ID and
///        the "Event timer" sub-object (0x05) set to a non-zero value
///
/// \When an invalid COB-ID is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value; the event timer is
///       stopped
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls can_recv_stop()
///       \Calls can_timer_stop()
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub01Cobid_ValidToInvalid_StopEventTimer) {
  const co_unsigned16_t event_timer_ms = 1u;
  obj1400->SetSub<Sub05EventTimer>(event_timer_ms);
  CreateRpdoAndStart();

  ReceivePdoMessage();  // start the event timer

  const co_unsigned32_t cobid =
      obj1400->GetSub<Sub01CobId>() | CO_PDO_COBID_VALID;
  CHECK_EQUAL(
      0, co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                           nullptr, CoCsdoDnCon::Func, nullptr));

  AdvanceTimeMs(event_timer_ms);

  CHECK_EQUAL(0, CoRpdoErr::GetNumCalled());  // the event timer not triggered
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
///        "COB-ID used by RPDO" sub-object (0x01) set to an invalid COB-ID and
///        the "Event timer" sub-object (0x05) set to a non-zero value
///
/// \When a valid COB-ID is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value; the event timer is
///       started
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls can_recv_start()
///       \Calls can_timer_stop()
///       \Calls can_timer_timeout()
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub01Cobid_InvalidToValid_StartEventTimer) {
  const co_unsigned16_t event_timer_ms = 10u;
  obj1400->SetSub<Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1400->SetSub<Sub05EventTimer>(event_timer_ms);
  CreateRpdoAndStart();

  const co_unsigned32_t cobid =
      obj1400->GetSub<Sub01CobId>() & ~CO_PDO_COBID_VALID;
  CHECK_EQUAL(
      0, co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                           nullptr, CoCsdoDnCon::Func, nullptr));

  AdvanceTimeMs(event_timer_ms);

  CHECK_EQUAL(1u, CoRpdoErr::GetNumCalled());  // the event timer triggered
  CoRpdoErr::Check(rpdo, 0x8250u, 0x10u, nullptr);
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
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
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub02TransmissionType_SameValue) {
  CreateRpdoAndStart();
  const auto val = obj1400->GetSub<Sub02TransmissionType>();

  const co_unsigned8_t ttype = obj1400->GetSub<Sub02TransmissionType>();
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x02u, CO_DEFTYPE_UNSIGNED8, &ttype,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, 0x02u, 0, nullptr);
  CHECK_EQUAL(val, obj1400->GetSub<Sub02TransmissionType>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
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
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub02TransmissionType_Reserved) {
  CreateRpdoAndStart();
  const auto val = obj1400->GetSub<Sub02TransmissionType>();

  for (co_unsigned8_t ttype = 0xf1u; ttype < 0xfeu; ttype++) {
    const auto ret =
        co_dev_dn_val_req(dev, 0x1400u, 0x02u, CO_DEFTYPE_UNSIGNED8, &ttype,
                          nullptr, CoCsdoDnCon::Func, nullptr);

    CHECK_EQUAL(0, ret);
    CoCsdoDnCon::Check(nullptr, 0x1400u, 0x02u, CO_SDO_AC_PARAM_VAL, nullptr);
    CHECK_EQUAL(val, obj1400->GetSub<Sub02TransmissionType>());

    CoCsdoDnCon::Clear();
  }
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
///        "Transmission type" sub-object (0x02)
///
/// \When a transmission type value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub02TransmissionType_NewValue) {
  CreateRpdoAndStart();

  const co_unsigned8_t ttype = 0x35u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x02u, CO_DEFTYPE_UNSIGNED8, &ttype,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, 0x02u, 0, nullptr);
  CHECK_EQUAL(ttype, obj1400->GetSub<Sub02TransmissionType>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
///        "Transmission type" sub-object (0x02)
///
/// \When the maximum transmission type value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub02TransmissionType_MaxValue) {
  CreateRpdoAndStart();

  const co_unsigned8_t ttype = CO_UNSIGNED8_MAX;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x02u, CO_DEFTYPE_UNSIGNED8, &ttype,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, 0x02u, 0, nullptr);
  CHECK_EQUAL(ttype, obj1400->GetSub<Sub02TransmissionType>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
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
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub03InhibitTime_SameValue) {
  obj1400->SetSub<Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  CreateRpdoAndStart();
  const auto val = obj1400->GetSub<Sub03InhibitTime>();

  const co_unsigned16_t inhibit_time = 0x0000u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x03u, CO_DEFTYPE_UNSIGNED16,
                        &inhibit_time, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, 0x03u, 0, nullptr);
  CHECK_EQUAL(val, obj1400->GetSub<Sub03InhibitTime>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
///        "Inhibit time" sub-object (0x03); the RPDO is valid
///
/// \When a time value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u16()
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub03InhibitTime_ValidRpdo) {
  CreateRpdoAndStart();

  const co_unsigned16_t inhibit_time = 0x0012u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x03u, CO_DEFTYPE_UNSIGNED16,
                        &inhibit_time, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoCsdoDnCon::GetNumCalled());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
///        "Inhibit time" sub-object (0x03); the RPDO is invalid
///
/// \When a time value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u16()
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub03InhibitTime) {
  obj1400->SetSub<Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  CreateRpdoAndStart();

  const co_unsigned16_t inhibit_time = 0x0034u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x03u, CO_DEFTYPE_UNSIGNED16,
                        &inhibit_time, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, 0x03u, 0, nullptr);
  CHECK_EQUAL(inhibit_time, obj1400->GetSub<Sub03InhibitTime>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
///        "Compatibility entry" sub-object (0x04)
///
/// \When any value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_NO_SUB abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub04CompatibilityEntry_NoSub) {
  CreateRpdoAndStart();

  const co_unsigned8_t compat = CO_UNSIGNED8_MAX;

  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x04u, CO_DEFTYPE_UNSIGNED8, &compat,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, 0x04u, CO_SDO_AC_NO_SUB, nullptr);
  CHECK_EQUAL(0, co_dev_get_val_u16(dev, 0x1400u, 0x04u));
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
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
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub05EventTimer_SameValue) {
  CreateRpdoAndStart();
  const auto val = obj1400->GetSub<Sub05EventTimer>();

  const co_unsigned16_t event_timer = 0x0000u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x05u, CO_DEFTYPE_UNSIGNED16,
                        &event_timer, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, 0x05u, 0, nullptr);
  CHECK_EQUAL(val, obj1400->GetSub<Sub05EventTimer>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
///        "Event timer" sub-object (0x05); the event timer is enabled
///
/// \When a new timer value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value; the event timer is
///       updated and restarted
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u16()
///       \Calls can_timer_stop()
///       \Calls can_timer_timeout()
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub05EventTimer_NewTimerValue) {
  const co_unsigned16_t old_event_timer_ms = 20u;
  obj1400->SetSub<Sub05EventTimer>(old_event_timer_ms);
  CreateRpdoAndStart();

  ReceivePdoMessage();  // start the event timer
  AdvanceTimeMs(old_event_timer_ms - 1u);

  const co_unsigned16_t event_timer_ms = 20u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x05u, CO_DEFTYPE_UNSIGNED16,
                        &event_timer_ms, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, 0x05u, 0, nullptr);
  CHECK_EQUAL(event_timer_ms, obj1400->GetSub<Sub05EventTimer>());

  AdvanceTimeMs(event_timer_ms);

  CHECK_EQUAL(1u, CoRpdoErr::GetNumCalled());
  CoRpdoErr::Check(rpdo, 0x8250u, 0x10u, nullptr);
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400) with the
///        "Event timer" sub-object (0x05); the event timer is enabled
///
/// \When a zero timer value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value; the event timer is
///       disabled and stopped
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u16()
///       \Calls can_timer_stop()
TEST(CO_SdoRpdo1400, Co1400DnInd_Sub05EventTimer_DisableTimer) {
  const co_unsigned16_t old_event_timer_ms = 10u;
  obj1400->SetSub<Sub05EventTimer>(old_event_timer_ms);
  CreateRpdoAndStart();

  ReceivePdoMessage();  // start the event timer
  AdvanceTimeMs(old_event_timer_ms - 1u);

  const co_unsigned16_t event_timer_ms = 0u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x05u, CO_DEFTYPE_UNSIGNED16,
                        &event_timer_ms, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1400u, 0x05u, 0, nullptr);
  CHECK_EQUAL(event_timer_ms, obj1400->GetSub<Sub05EventTimer>());

  AdvanceTimeMs(old_event_timer_ms);
  CHECK_EQUAL(0, CoRpdoErr::GetNumCalled());
}

///@}

TEST_GROUP_BASE(CO_SdoRpdo1600, CO_SdoRpdoBase) {
  using Sub00NumOfMappedObjs = Obj1600RpdoMapPar::Sub00NumOfMappedObjs;
  using SubNthAppObject = Obj1600RpdoMapPar::SubNthAppObject;
  static constexpr auto MakeMappingParam = Obj1600RpdoMapPar::MakeMappingParam;

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

    obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub00HighestSubidxSupported>();
    obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID);
    obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub02TransmissionType>(
        Obj1400RpdoCommPar::EVENT_DRIVEN_TRANSMISSION);

    obj1600->EmplaceSub<Sub00NumOfMappedObjs>(CO_PDO_NUM_MAPS);
    for (co_unsigned8_t i = 1u; i <= CO_PDO_NUM_MAPS; ++i)
      obj1600->EmplaceSub<SubNthAppObject>(i, 0u);
  }
};

// NOLINTNEXTLINE(whitespace/line_length)
/// @name RPDO service: the RPDO Mapping Parameter object (0x1600-0x17fff) modification using SDO
///@{

/// \Given a started RPDO service (co_rpdo_t), the object dictionary contains
///        the RPDO Mapping Parameter object (0x1600)
///
/// \When the download indication function for the object 0x1600 is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_SdoRpdo1600, Co1600DnInd_NonZeroAbortCode) {
  CreateRpdoAndStart();

  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret =
      LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1600u, 0x00u, ac);

  CHECK_EQUAL(ac, ret);
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary contains
///        the RPDO Mapping Parameter object (0x1600) with a sub-object
///
/// \When a value longer than the sub-object's data type length is downloaded
///       to the sub-object
///
/// \Then CO_SDO_AC_TYPE_LEN_HI abort code is passed to the download
///       confirmation function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
TEST(CO_SdoRpdo1600, Co1600DnInd_TypeLenTooHigh) {
  CreateRpdoAndStart();
  const auto val = obj1600->GetSub<Sub00NumOfMappedObjs>();

  const co_unsigned32_t value = CO_UNSIGNED32_MAX;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED32, &value,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1600u, 0x00u, CO_SDO_AC_TYPE_LEN_HI, nullptr);
  CHECK_EQUAL(val, obj1600->GetSub<Sub00NumOfMappedObjs>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary contains
///        the RPDO Mapping Parameter object (0x1600) with the "Number of
///        mapped objects" sub-object (0x00) and an "Application object" entry
///        with a mapping that exceeds the maximum PDO length; the RPDO is
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
TEST(CO_SdoRpdo1600, Co1600DnInd_Sub00NumOfMappedObjs_MappingLenOverMax) {
  obj1400->SetSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1600->SetSub<SubNthAppObject>(
      0x01u, MakeMappingParam(PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX, 0xffu));
  CreateMappableObject();
  CreateRpdoAndStart();
  const auto val = obj1600->GetSub<Sub00NumOfMappedObjs>();

  const co_unsigned8_t num = 1u;
  const auto ret = co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &num, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1600u, 0x00u, CO_SDO_AC_PDO_LEN, nullptr);
  CHECK_EQUAL(val, obj1600->GetSub<Sub00NumOfMappedObjs>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary contains
///        the RPDO Mapping Parameter object (0x1600) with the "Number of
///        mapped objects" sub-object (0x00) and an "Application object" entry
///        with an empty mapping; the RPDO is invalid
///
/// \When a new number of mapped objects value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_dev_chk_rpdo()
///       \Calls co_sub_dn()
TEST(CO_SdoRpdo1600, Co1600DnInd_Sub00NumOfMappedObjs_EmptyMapping) {
  obj1400->SetSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1600->SetSub<SubNthAppObject>(0x01u, 0u);
  CreateMappableObject();
  CreateRpdoAndStart();

  const co_unsigned8_t num = 1u;
  const auto ret = co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &num, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1600u, 0x00u, 0, nullptr);
  CHECK_EQUAL(num, obj1600->GetSub<Sub00NumOfMappedObjs>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary contains
///        the RPDO Mapping Parameter object (0x1600) with the "Number of
///        mapped objects" sub-object (0x00) and an "Application object" entry
///        that maps into a non-existing object; the RPDO is invalid
///
/// \When a new number of mapped objects value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_NO_OBJ abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_dev_chk_rpdo()
TEST(CO_SdoRpdo1600, Co1600DnInd_Sub00NumOfMappedObjs_MappingNonExistingObj) {
  obj1400->SetSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1600->SetSub<SubNthAppObject>(0x01u,
                                   MakeMappingParam(0xffffu, 0x00u, 0x00u));
  CreateRpdoAndStart();
  const auto val = obj1600->GetSub<Sub00NumOfMappedObjs>();

  const co_unsigned8_t num = 1u;
  const auto ret = co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &num, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1600u, 0x00u, CO_SDO_AC_NO_OBJ, nullptr);
  CHECK_EQUAL(val, obj1600->GetSub<Sub00NumOfMappedObjs>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary contains
///        the RPDO Mapping Parameter object (0x1600) with the "Number of
///        mapped objects" sub-object (0x00); the RPDO is invalid
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
TEST(CO_SdoRpdo1600, Co1600DnInd_Sub00NumOfMappedObjs_SameValue) {
  obj1400->SetSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  CreateRpdoAndStart();

  const co_unsigned8_t num = obj1600->GetSub<Sub00NumOfMappedObjs>();
  const auto ret = co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &num, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1600u, 0x00u, 0, nullptr);
  CHECK_EQUAL(num, obj1600->GetSub<Sub00NumOfMappedObjs>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary contains
///        the RPDO Mapping Parameter object (0x1600) with the "Number of
///        mapped objects" sub-object (0x00); the RPDO is valid
///
/// \When any value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_SdoRpdo1600, Co1600DnInd_Sub00NumOfMappedObjs_ValidRpdo) {
  CreateRpdoAndStart();
  const auto val = obj1600->GetSub<Sub00NumOfMappedObjs>();

  const co_unsigned8_t num = 2u;
  const auto ret = co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &num, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1600u, 0x00u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1600->GetSub<Sub00NumOfMappedObjs>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary contains
///        the RPDO Mapping Parameter object (0x1600) with the "Number of
///        mapped objects" sub-object (0x00); the RPDO is invalid
///
/// \When a value larger than CO_PDO_NUM_MAPS is downloaded to the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_SdoRpdo1600, Co1600DnInd_Sub00NumOfMappedObjs_NumOverMax) {
  obj1400->SetSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  CreateRpdoAndStart();
  const auto val = obj1600->GetSub<Sub00NumOfMappedObjs>();

  const co_unsigned8_t num = CO_PDO_NUM_MAPS + 1u;
  const auto ret = co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &num, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1600u, 0x00u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1600->GetSub<Sub00NumOfMappedObjs>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary contains
///        the RPDO Mapping Parameter object (0x1600) with the "Number of
///        mapped objects" sub-object (0x00); the RPDO is invalid
///
/// \When a zero value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
TEST(CO_SdoRpdo1600, Co1600DnInd_Sub00NumOfMappedObjs_NoMappings) {
  CreateRpdoAndStart();
  const auto val = obj1600->GetSub<Sub00NumOfMappedObjs>();

  const co_unsigned8_t num = 0;
  const auto ret = co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &num, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1600u, 0x00u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1600->GetSub<Sub00NumOfMappedObjs>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary contains
///        the RPDO Mapping Parameter object (0x1600) with the "Number of
///        mapped objects" sub-object (0x00) and some "Application object"
///        entires; the RPDO is invalid
///
/// \When a non-zero value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_dev_chk_rpdo()
///       \Calls co_sub_dn()
TEST(CO_SdoRpdo1600, Co1600DnInd_Sub00NumOfMappedObjs_Nominal) {
  obj1400->SetSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1600->SetSub<SubNthAppObject>(
      0x01u,
      MakeMappingParam(PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX, PDO_MAPPED_LEN));
  CreateMappableObject();
  CreateRpdoAndStart();

  const co_unsigned8_t num = 1u;
  const auto ret = co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &num, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1600u, 0x00u, 0, nullptr);
  CHECK_EQUAL(num, obj1600->GetSub<Sub00NumOfMappedObjs>());
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary contains
///        the RPDO Mapping Parameter object (0x1600) with the "Number of
///        mapped objects" sub-object (0x00) equal to zero and with an
///        "Application object" sub-object (0x01-0x40); the RPDO is invalid
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
///       \Calls co_dev_chk_rpdo()
TEST(CO_SdoRpdo1600, Co1600DnInd_SubNthAppObj_NonExistingObj) {
  obj1400->SetSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1600->SetSub<Sub00NumOfMappedObjs>(0x00u);
  CreateRpdoAndStart();
  const auto val = obj1600->GetSub<SubNthAppObject>(0x01u);

  const co_unsigned32_t mapping = MakeMappingParam(0xffffu, 0x00u, 0x00u);
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1600u, 0x01u, CO_SDO_AC_NO_OBJ, nullptr);
  CHECK_EQUAL(val, obj1600->GetSub<SubNthAppObject>(0x01u));
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary contains
///        the RPDO Mapping Parameter object (0x1600) with the "Number of
///        mapped objects" sub-object (0x00) equal to zero and with an
///        "Application object" sub-object (0x01-0x40); the RPDO is invalid
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
TEST(CO_SdoRpdo1600, Co1600DnInd_SubNthAppObj_SameValue) {
  obj1400->SetSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1600->SetSub<SubNthAppObject>(
      0x01u,
      MakeMappingParam(PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX, PDO_MAPPED_LEN));
  CreateMappableObject();
  CreateRpdoAndStart();

  const co_unsigned32_t mapping = obj1600->GetSub<SubNthAppObject>(0x01u);
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1600u, 0x01u, 0, nullptr);
  CHECK_EQUAL(mapping, obj1600->GetSub<SubNthAppObject>(0x01u));
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary contains
///        the RPDO Mapping Parameter object (0x1600) with the "Number of
///        mapped objects" sub-object (0x00) does not equal zero and with an
///        "Application object" sub-object (0x01-0x40); the RPDO is invalid
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
TEST(CO_SdoRpdo1600, Co1600DnInd_SubNthAppObj_NumOfMappingsNonzero) {
  obj1400->SetSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1600->SetSub<Sub00NumOfMappedObjs>(0x01u);
  CreateMappableObject();
  CreateRpdoAndStart();
  const auto val = obj1600->GetSub<SubNthAppObject>(0x01u);

  const co_unsigned32_t mapping =
      MakeMappingParam(PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX, PDO_MAPPED_LEN);
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1600u, 0x01u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1600->GetSub<SubNthAppObject>(0x01u));
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary contains
///        the RPDO Mapping Parameter object (0x1600) with the "Number of
///        mapped objects" sub-object (0x00) equal to zero and with an
///        "Application object" sub-object (0x01-0x40); the RPDO is valid
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
TEST(CO_SdoRpdo1600, Co1600DnInd_SubNthAppObj_ValidRpdo) {
  obj1400->SetSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID);
  CreateMappableObject();
  CreateRpdoAndStart();
  const auto val = obj1600->GetSub<SubNthAppObject>(0x01u);

  const co_unsigned32_t mapping =
      MakeMappingParam(PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX, PDO_MAPPED_LEN);
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1600u, 0x01u, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(val, obj1600->GetSub<SubNthAppObject>(0x01u));
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary contains
///        the RPDO Mapping Parameter object (0x1600) with the "Number of
///        mapped objects" sub-object (0x00) equal to zero and with an
///        "Application object" sub-object (0x01-0x40); the RPDO is invalid
///
/// \When a new mapping value is downloaded to the "Application object"
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_dev_chk_rpdo()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_dn()
TEST(CO_SdoRpdo1600, Co1600DnInd_SubNthAppObj_Nominal) {
  obj1400->SetSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1600->SetSub<Sub00NumOfMappedObjs>(0x00);
  CreateMappableObject();
  CreateRpdoAndStart();

  const co_unsigned32_t mapping =
      MakeMappingParam(PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX, PDO_MAPPED_LEN);
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600u, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1600u, 0x01u, 0, nullptr);
  CHECK_EQUAL(mapping, obj1600->GetSub<SubNthAppObject>(0x01u));
}

/// \Given a started RPDO service (co_rpdo_t), the object dictionary contains
///        the RPDO Mapping Parameter object (0x1600) with the "Number of
///        mapped objects" sub-object (0x00) equal to zero and with an
///        "Application object" sub-object (0x01-0x40); the RPDO is invalid
///
/// \When an empty mapping value is downloaded to the "Application object"
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_dev_chk_rpdo()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_dn()
TEST(CO_SdoRpdo1600, Co1600DnInd_SubNthAppObj_EmptyMapping) {
  obj1400->SetSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID | CO_PDO_COBID_VALID);
  obj1600->SetSub<Sub00NumOfMappedObjs>(0x00);
  obj1600->SetSub<SubNthAppObject>(
      0x01u,
      MakeMappingParam(PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX, PDO_MAPPED_LEN));
  CreateMappableObject();
  CreateRpdoAndStart();

  const co_unsigned32_t mapping = 0u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x01, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1600u, 0x01u, 0, nullptr);
  CHECK_EQUAL(mapping, obj1600->GetSub<SubNthAppObject>(0x01u));
}

///@}
