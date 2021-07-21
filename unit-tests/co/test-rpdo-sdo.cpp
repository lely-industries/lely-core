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

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "obj-init/rpdo-comm-par.hpp"

TEST_BASE(CO_SdoRpdoBase) {
  TEST_BASE_SUPER(CO_SdoRpdoBase);
  Allocators::Default allocator;

  const co_unsigned8_t DEV_ID = 0x01u;
  const co_unsigned16_t RPDO_NUM = 0x0001u;

  co_dev_t* dev = nullptr;
  can_net_t* net = nullptr;
  co_rpdo_t* rpdo = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1400;
  std::unique_ptr<CoObjTHolder> obj1600;

  void SetPdoCommCobid(const co_unsigned32_t cobid) const {
    co_sub_t* const sub_comm_cobid = co_dev_find_sub(dev, 0x1400u, 0x01u);
    CHECK(sub_comm_cobid != nullptr);
    co_sub_set_val_u32(sub_comm_cobid, cobid);
  }

  void RestartRPDO() {
    co_rpdo_stop(rpdo);
    co_rpdo_start(rpdo);
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(allocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    dev_holder->CreateAndInsertObj(obj1400, 0x1400u);
    dev_holder->CreateAndInsertObj(obj1600, 0x1600u);

    // 0x00 - highest sub-index supported
    obj1400->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t{0x02u});
    // 0x01 - COB-ID used by RPDO
    obj1400->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                             co_unsigned32_t{DEV_ID});
    // 0x02 - transmission type
    obj1400->InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t{0xfeu});  // event-driven

    rpdo = co_rpdo_create(net, dev, RPDO_NUM);
    CHECK(rpdo != nullptr);

    CoCsdoDnCon::Clear();
  }

  TEST_TEARDOWN() {
    co_rpdo_destroy(rpdo);

    dev_holder.reset();
    can_net_destroy(net);
  }
};

namespace CO_SdoRpdo1400Static {
static bool rpdo_err_func_called = false;
static co_unsigned16_t rpdo_err_func_eec = 0u;
static co_unsigned8_t rpdo_err_func_er = 0u;
}  // namespace CO_SdoRpdo1400Static

TEST_GROUP_BASE(CO_SdoRpdo1400, CO_SdoRpdoBase) {
  int32_t clang_format_fix = 0;  // dummy for workaround

  using Sub00HighestSubidxSupported =
      Obj1400RpdoCommPar::Sub00HighestSubidxSupported;
  using Sub01CobId = Obj1400RpdoCommPar::Sub01CobId;
  using Sub02TransmissionType = Obj1400RpdoCommPar::Sub02TransmissionType;
  using Sub03InhibitTime = Obj1400RpdoCommPar::Sub03InhibitTime;
  using Sub04Reserved = Obj1400RpdoCommPar::Sub04Reserved;

  static void rpdo_err_func(co_rpdo_t*, const co_unsigned16_t eec,
                            const co_unsigned8_t er, void*) {
    CO_SdoRpdo1400Static::rpdo_err_func_called = true;
    CO_SdoRpdo1400Static::rpdo_err_func_eec = eec;
    CO_SdoRpdo1400Static::rpdo_err_func_er = er;
  }

  void SetPdoCommEventTimer(const co_unsigned16_t milliseconds) const {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1400u, 0x05u);
    CHECK(sub != nullptr);
    co_sub_set_val_u16(sub, milliseconds);
  }

  void ReceiveMessage() {
    can_msg msg = CAN_MSG_INIT;
    msg.id = DEV_ID;
    CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  }

  void SetCurrentTimeMs(const uint_least64_t ms) {
    timespec tp = {0, 0u};
    timespec_add_msec(&tp, ms);
    CHECK_EQUAL(0, can_net_set_time(net, &tp));
  }

  void Insert1400Values() const {
    // adjust highest subindex supported
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1400u, 0x00u);
    CHECK(sub != nullptr);
    co_sub_set_val_u8(sub, 0x05u);

    // 0x03 - inhibit time
    obj1400->InsertAndSetSub(0x03u, CO_DEFTYPE_UNSIGNED16,
                             co_unsigned16_t{0x0000u});  // n*100 us
    // 0x04 - reserved (compatibility entry)
    obj1400->InsertAndSetSub(0x04u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t{0x00u});
    // 0x05 - event-timer
    obj1400->InsertAndSetSub(0x05u, CO_DEFTYPE_UNSIGNED16,
                             co_unsigned16_t{0x0000u});  // ms
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    CO_SdoRpdo1400Static::rpdo_err_func_called = false;
    CO_SdoRpdo1400Static::rpdo_err_func_eec = 0u;
    CO_SdoRpdo1400Static::rpdo_err_func_er = 0u;

    Insert1400Values();
    co_rpdo_set_err(rpdo, &rpdo_err_func, nullptr);
    co_rpdo_start(rpdo);
  }

  TEST_TEARDOWN() {
    co_rpdo_stop(rpdo);

    TEST_BASE_TEARDOWN();
  }
};

/// @name RPDO service: object 0x1400 modification using SDO
///@{

/// \Given a pointer to a device (co_dev_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400)
///
/// \When the download indication function for the object 0x1400 is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_SdoRpdo1400, Co1400DnInd_NonZeroAbortCode) {
  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret =
      LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1400u, 0x00u, ac);

  CHECK_EQUAL(ac, ret);
}

/// \Given a pointer to a device (co_dev_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400)
///
/// \When a too long value is downloaded to the RPDO Communication Parameter
///       "Highest sub-index supported" entry (idx: 0x1400, subidx: 0x00)
///
/// \Then download confirmation function is called once, CO_SDO_AC_TYPE_LEN_HI
///       is set as the abort code, the requested entry is not changed
TEST(CO_SdoRpdo1400, Co1400DnInd_TooLongData) {
  const auto val = obj1400->GetSub<Sub00HighestSubidxSupported>();

  const co_unsigned16_t data = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x00u, CO_DEFTYPE_UNSIGNED16, &data,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_HI, CoCsdoDnCon::ac);
  CHECK_EQUAL(val, obj1400->GetSub<Sub00HighestSubidxSupported>());
}

/// \Given a pointer to a device (co_dev_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400)
///
/// \When a value is downloaded to the RPDO Communication Parameter
///       "Highest sub-index supported" entry (idx: 0x1400, subidx: 0x00)
///
/// \Then download confirmation function is called once, CO_SDO_AC_NO_WRITE
///       is set as the abort code, the requested entry is not changed
TEST(CO_SdoRpdo1400, Co1400DnInd_DownloadNumOfElements) {
  const auto val = obj1400->GetSub<Sub00HighestSubidxSupported>();

  const co_unsigned8_t num_of_elems = 0x7fu;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_elems, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_NO_WRITE, CoCsdoDnCon::ac);
  CHECK_EQUAL(val, obj1400->GetSub<Sub00HighestSubidxSupported>());
}

/// \Given a pointer to a device (co_dev_t), the object dictionary
///        contains the RPDO Communication Parameter object (0x1400)
///
/// \When a value is downloaded to the RPDO Communication Parameter
///       "COB-ID used by RPDO" entry (idx: 0x1400, subidx: 0x01)
///
/// \Then download confirmation function is called once, 0 is set as the abort
///       code, the requested entry is not changed
TEST(CO_SdoRpdo1400, Co1400DnInd_CobidSameAsPrevious) {
  const auto val = obj1400->GetSub<Sub01CobId>();

  const co_unsigned32_t cobid = DEV_ID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
  CHECK_EQUAL(val, obj1400->GetSub<Sub01CobId>());
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: CO_SDO_AC_PARAM_VAL  abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_CobidValidToValid_NewCanId) {
  SetPdoCommCobid(DEV_ID);
  RestartRPDO();

  const co_unsigned32_t cobid = DEV_ID + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_CobidInvalidToValid_NewCanId) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartRPDO();

  const co_unsigned32_t cobid = DEV_ID + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_CobidValidToValid_FrameBit) {
  const co_unsigned32_t cobid = DEV_ID | CO_PDO_COBID_FRAME;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: COB-ID with frame bit and CO_PDO_COBID_VALID set is downloaded
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_CobidValidToInvalid_ExtendedId_NoFrameBit) {
  const co_unsigned32_t cobid = DEV_ID | (1u << 28u) | CO_PDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: COB-ID with CO_PDO_COBID_VALID set is downloaded
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_CobidValidToInvalid) {
  const co_unsigned32_t cobid = DEV_ID | CO_PDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

/// \Given a pointer to a device with started RPDO with valid COB-ID and event
///        timer set
///
/// \When a new COB-ID with valid bit set is downloaded to the RPDO COB-ID
///       object entry (idx: 0x1400, subidx: 0x01)
///
/// \Then 0 is returned, event timer is stopped and not triggered later
///       \Calls co_dev_find_sub()
///       \Calls co_sub_set_val()
///       \Calls co_rpdo_stop()
///       \Calls co_rpdo_start()
///       \Calls can_net_recv()
///       \Calls can_net_set_time()
TEST(CO_SdoRpdo1400, Co1400DnInd_CobidValidToInvalid_DisableEventTimer) {
  SetPdoCommEventTimer(1u);
  RestartRPDO();

  ReceiveMessage();  // timer started

  const co_unsigned32_t cobid = DEV_ID | CO_PDO_COBID_VALID;
  CHECK_EQUAL(
      0, co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                           nullptr, CoCsdoDnCon::Func, nullptr));

  SetCurrentTimeMs(2u);

  // event timer handler was not called
  CHECK(!CO_SdoRpdo1400Static::rpdo_err_func_called);
}

/// \Given a pointer to a device with started RPDO with invalid COB-ID and with
///        event timer set
///
/// \When a valid COB-ID is downloaded to the RPDO COB-ID object entry (idx:
///       0x1400, subidx: 0x01)
///
/// \Then 0 is returned, event timer is started and triggered after required
///       time passes after COB-ID change
///       \Calls co_dev_find_sub()
///       \Calls co_sub_set_val()
///       \Calls co_rpdo_stop()
///       \Calls co_rpdo_start()
///       \Calls can_net_recv()
///       \Calls can_net_set_time()
TEST(CO_SdoRpdo1400, Co1400DnInd_CobidInvalidToValid_ReenableEventTimer) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  SetPdoCommEventTimer(10u);
  RestartRPDO();

  SetCurrentTimeMs(9u);

  const co_unsigned32_t cobid = DEV_ID;
  CHECK_EQUAL(
      0, co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                           nullptr, CoCsdoDnCon::Func, nullptr));

  SetCurrentTimeMs(18u);
  CHECK(!CO_SdoRpdo1400Static::rpdo_err_func_called);

  SetCurrentTimeMs(20u);

  CHECK(CO_SdoRpdo1400Static::rpdo_err_func_called);
  CHECK_EQUAL(0x8250u, CO_SdoRpdo1400Static::rpdo_err_func_eec);
  CHECK_EQUAL(0x10u, CO_SdoRpdo1400Static::rpdo_err_func_er);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_TransmissionTypeSameAsPrevious) {
  const co_unsigned8_t transmission_type = 0xfeu;
  const auto ret = co_dev_dn_val_req(dev, 0x1400u, 0x02u, CO_DEFTYPE_UNSIGNED8,
                                     &transmission_type, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: CO_SDO_AC_PARAM_VAL  abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_TransmissionTypeReserved) {
  for (co_unsigned8_t i = 0xf1u; i < 0xfeu; i++) {
    CoCsdoDnCon::Clear();

    const co_unsigned8_t transmission_type = i;
    const auto ret = co_dev_dn_val_req(dev, 0x1400u, 0x02u,
                                       CO_DEFTYPE_UNSIGNED8, &transmission_type,
                                       nullptr, CoCsdoDnCon::Func, nullptr);

    CHECK_EQUAL(0, ret);
    CHECK(CoCsdoDnCon::Called());
    CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
  }
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_TransmissionTypeMax) {
  const co_unsigned8_t transmission_type = 0xffu;
  const auto ret = co_dev_dn_val_req(dev, 0x1400u, 0x02u, CO_DEFTYPE_UNSIGNED8,
                                     &transmission_type, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_TransmissionType) {
  const co_unsigned8_t transmission_type = 0x35u;
  const auto ret = co_dev_dn_val_req(dev, 0x1400u, 0x02u, CO_DEFTYPE_UNSIGNED8,
                                     &transmission_type, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_InhibitTimeSameAsPrevious) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartRPDO();

  const co_unsigned16_t inhibit_time = 0x0000u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x03u, CO_DEFTYPE_UNSIGNED16,
                        &inhibit_time, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_InhibitTimeValidRPDO) {
  const co_unsigned16_t inhibit_time = 0x0001u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x03u, CO_DEFTYPE_UNSIGNED16,
                        &inhibit_time, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_InhibitTime) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartRPDO();

  const co_unsigned16_t inhibit_time = 0x0003u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x03u, CO_DEFTYPE_UNSIGNED16,
                        &inhibit_time, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: CO_SDO_AC_NO_SUB abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_CompatibilityEntry) {
  const co_unsigned8_t compat = 0x44u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x04u, CO_DEFTYPE_UNSIGNED8, &compat,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_NO_SUB, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_EventTimerSameAsPrevious) {
  const co_unsigned16_t event_timer = 0x0000u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x05u, CO_DEFTYPE_UNSIGNED16,
                        &event_timer, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_EventTimer) {
  const co_unsigned16_t event_timer = 0x3456u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x05u, CO_DEFTYPE_UNSIGNED16,
                        &event_timer, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

/// \Given a pointer to a device with started RPDO with event timer set
///
/// \When a new non-zero event timer value is downloaded to the RPDO event timer
///       object entry (idx: 0x1400, subidx: 0x05)
///
/// \Then 0 is returned, event timer is restarted and triggered after required
///       time passes after event timer change
///       \Calls co_dev_find_sub()
///       \Calls co_sub_set_val()
///       \Calls co_rpdo_stop()
///       \Calls co_rpdo_start()
///       \Calls can_net_recv()
///       \Calls can_net_set_time()
TEST(CO_SdoRpdo1400, Co1400DnInd_EventTimerSetToNonZero_ReenableEventTimer) {
  SetPdoCommEventTimer(10u);
  RestartRPDO();

  ReceiveMessage();  // timer started
  SetCurrentTimeMs(5u);

  const co_unsigned16_t event_timer = 20u;
  CHECK_EQUAL(
      0, co_dev_dn_val_req(dev, 0x1400u, 0x05u, CO_DEFTYPE_UNSIGNED16,
                           &event_timer, nullptr, CoCsdoDnCon::Func, nullptr));

  SetCurrentTimeMs(24u);
  CHECK(!CO_SdoRpdo1400Static::rpdo_err_func_called);

  SetCurrentTimeMs(25u);
  CHECK(CO_SdoRpdo1400Static::rpdo_err_func_called);
  CHECK_EQUAL(0x8250u, CO_SdoRpdo1400Static::rpdo_err_func_eec);
  CHECK_EQUAL(0x10u, CO_SdoRpdo1400Static::rpdo_err_func_er);
}

/// \Given a pointer to a device with started RPDO with event timer set
///
/// \When a new event timer value of zero is downloaded to the RPDO event timer
///       object entry (idx: 0x1400, subidx: 0x05)
///
/// \Then 0 is returned, event timer is stopped and not triggered later
///       \Calls co_dev_find_sub()
///       \Calls co_sub_set_val()
///       \Calls co_rpdo_stop()
///       \Calls co_rpdo_start()
///       \Calls can_net_recv()
///       \Calls can_net_set_time()
TEST(CO_SdoRpdo1400, Co1400DnInd_EventTimerSetToZero_DisableEventTimer) {
  SetPdoCommEventTimer(10u);
  RestartRPDO();

  ReceiveMessage();  // timer started
  SetCurrentTimeMs(5u);

  const co_unsigned16_t event_timer = 0u;
  CHECK_EQUAL(
      0, co_dev_dn_val_req(dev, 0x1400u, 0x05u, CO_DEFTYPE_UNSIGNED16,
                           &event_timer, nullptr, CoCsdoDnCon::Func, nullptr));

  SetCurrentTimeMs(100u);
  CHECK(!CO_SdoRpdo1400Static::rpdo_err_func_called);
}

///@}

TEST_GROUP_BASE(CO_SdoRpdo1600, CO_SdoRpdoBase) {
  std::unique_ptr<CoObjTHolder> obj2021;

  void Insert1600Values() {
    // 0x00 - number of mapped application objects in PDO
    obj1600->InsertAndSetSub(0x00, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t{CO_PDO_NUM_MAPS});
    // 0x01-0x40 - application objects
    for (co_unsigned8_t i = 0x01u; i <= CO_PDO_NUM_MAPS; ++i) {
      obj1600->InsertAndSetSub(i, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t{0});
    }
  }

  void Set1600Sub1Mapping(const co_unsigned32_t mapping) {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1600u, 0x01u);
    co_sub_set_val_u32(sub, mapping);
  }

  void Insert2021Values() {
    assert(obj2021->Get());

    obj2021->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32,
                             co_unsigned32_t{0xdeadbeefu});
    co_sub_t* const sub2021 = obj2021->GetLastSub();
    co_sub_set_access(sub2021, CO_ACCESS_RW);
    co_sub_set_pdo_mapping(sub2021, 1);
  }

  void SetNumOfMappings(const co_unsigned8_t mappings_num) {
    co_sub_t* const sub_map_n = co_dev_find_sub(dev, 0x1600u, 0x00u);
    co_sub_set_val_u8(sub_map_n, mappings_num);
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    Insert1600Values();
    co_rpdo_start(rpdo);

    CoCsdoDnCon::Clear();
  }

  TEST_TEARDOWN() {
    co_rpdo_stop(rpdo);

    TEST_BASE_TEARDOWN();
  }
};

/// @name RPDO service: object 0x1600 modification using SDO
///@{

/// \Given a pointer to a device (co_dev_t), the object dictionary
///        contains the RPDO Mapping Parameter object (0x1600)
///
/// \When the download indication function for the object 0x1600 is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_SdoRpdo1600, Co1600DnInd_NonZeroAbortCode) {
  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret =
      LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1600u, 0x00u, ac);

  CHECK_EQUAL(ac, ret);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: CO_SDO_AC_PDO_LEN  abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_NumOfMappingsLenGreaterThanMax) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  Set1600Sub1Mapping(0x202100ffu);
  RestartRPDO();
  // object which could be mapped
  dev_holder->CreateAndInsertObj(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned8_t num_of_mappings = 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_EmptyMapping) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  Set1600Sub1Mapping(0x00000000u);
  RestartRPDO();
  // object which could be mapped
  dev_holder->CreateAndInsertObj(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned8_t num_of_mappings = 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: too long value is downloaded
// then: CO_SDO_AC_TYPE_LEN_HI abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_NumOfMappingsRequestFailed) {
  const co_unsigned32_t data = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED32, &data,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_HI, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: CO_SDO_AC_NO_OBJ abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_NumOfMappingsNonExistingObjMapping) {
  Set1600Sub1Mapping(0xffff0000u);
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartRPDO();

  const co_unsigned8_t num_of_mappings = 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_NumOfMappingsSameAsPrevious) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartRPDO();

  const co_unsigned8_t num_of_mappings = CO_PDO_NUM_MAPS;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1600_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_NumOfMappingsValidRPDO) {
  const co_unsigned8_t num_of_mappings = 2u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_NumOfMappingsTooManyObjsToMap) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartRPDO();

  const co_unsigned8_t num_of_mappings = CO_PDO_NUM_MAPS + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1600_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_NumOfMappingsNoMappings) {
  const co_unsigned8_t num_of_mappings = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_NumOfMappings) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  Set1600Sub1Mapping(0x20210020u);
  RestartRPDO();
  // object which could be mapped
  dev_holder->CreateAndInsertObj(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned8_t num_of_mappings = 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: CO_SDO_AC_NO_OBJ abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_MappingNonexisting) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  SetNumOfMappings(0x00);
  RestartRPDO();

  const co_unsigned32_t mapping = 0xffff0000u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_MappingSameAsPrevious) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  Set1600Sub1Mapping(0x20210020u);
  RestartRPDO();
  // object which could be mapped
  dev_holder->CreateAndInsertObj(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned32_t mapping = 0x20210020u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_MappingNumOfMappingsNonzero) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  SetNumOfMappings(0x01u);
  RestartRPDO();
  // object which could be mapped
  dev_holder->CreateAndInsertObj(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned32_t mapping = 0x20210110u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1600_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_MappingValidRPDO) {
  SetPdoCommCobid(DEV_ID);
  SetNumOfMappings(0x01u);
  RestartRPDO();
  // object which could be mapped
  dev_holder->CreateAndInsertObj(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned32_t mapping = 0x20210020u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_Mapping) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  SetNumOfMappings(0x00);
  RestartRPDO();
  // object which could be mapped
  dev_holder->CreateAndInsertObj(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned32_t mapping = 0x20210020u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600u, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_MappingZeros) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  SetNumOfMappings(0x00);
  Set1600Sub1Mapping(0x20210020u);
  RestartRPDO();
  // object which could be mapped
  dev_holder->CreateAndInsertObj(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned32_t mapping = 0x00000000u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x01, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

///@}
