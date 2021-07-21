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

#include <functional>
#include <memory>

#include <CppUTest/TestHarness.h>

#include <lely/co/csdo.h>
#include <lely/co/nmt.h>
#include <lely/co/obj.h>
#include <lely/co/sdo.h>
#if !LELY_NO_CO_ECSS_REDUNDANCY
#include <lib/co/nmt_rdn.h>
#endif
#include <lely/util/error.h>
#include <lely/util/time.h>

#include <libtest/allocators/default.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>
#include <libtest/tools/co-nmt-rdn-ind.hpp>
#include <libtest/tools/co-nmt-hb-ind.hpp>

#include "holder/dev.hpp"

#include "obj-init/nmt-hb-consumer.hpp"
#include "obj-init/nmt-redundancy.hpp"
#include "obj-init/nmt-startup.hpp"

TEST_BASE(CO_NmtSdo) {
  TEST_BASE_SUPER(CO_NmtSdo);
  Allocators::Default allocator;

  static const co_unsigned8_t DEV_ID = 0x02u;
  static const co_unsigned8_t MASTER_DEV_ID = 0x01u;

  co_dev_t* dev = nullptr;
  can_net_t* net = nullptr;
  co_nmt_t* nmt = nullptr;

  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1f80;

  void CreateNmt() {
    nmt = co_nmt_create(net, dev);
    CHECK(nmt != nullptr);
  }

  virtual void CreateNmtAndReset() {
    CreateNmt();
    CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(allocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    can_net_set_send_func(net, CanSend::Func, nullptr);
  }

  TEST_TEARDOWN() {
    CoCsdoDnCon::Clear();
    CanSend::Clear();

    co_nmt_destroy(nmt);

    dev_holder.reset();
    can_net_destroy(net);
    set_errnum(0);
  }
};

TEST_GROUP_BASE(CO_NmtSdoDummy, CO_NmtSdo){};

TEST(CO_NmtSdoDummy, Dummy) {
  // empty
}

#if !LELY_NO_CO_ECSS_REDUNDANCY

TEST_GROUP_BASE(CO_NmtRdnSdo, CO_NmtSdo) {
  std::unique_ptr<CoObjTHolder> obj_rdn;
  std::unique_ptr<CoObjTHolder> obj1016;

  static const co_unsigned8_t BUS_A_ID = 0x00;
  static const co_unsigned8_t BUS_B_ID = 0x01;

  static const co_unsigned16_t HB_TIMEOUT_MS = 550u;

  void ConfigRdn() {
    dev_holder->CreateObj<ObjNmtRedundancy>(obj_rdn);
    obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub00HighestSubidxSupported>();
    obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub01Bdefault>(BUS_A_ID);
    obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub02Ttoggle>(1u);
    obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub03Ntoggle>(1u);
    obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub04Ctoggle>(0);

    dev_holder->CreateObj<Obj1016ConsumerHb>(obj1016);
    obj1016->EmplaceSub<Obj1016ConsumerHb::Sub00HighestSubidxSupported>(0x01u);
    obj1016->EmplaceSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(
        Obj1016ConsumerHb::MakeHbConsumerEntry(MASTER_DEV_ID, HB_TIMEOUT_MS));
  }

  void CheckRdnService(const bool enabled, const co_unsigned8_t ttoggle = 1u) {
    if (enabled) co_nmt_set_alternate_bus_id(nmt, BUS_B_ID);

    can_msg msg = CAN_MSG_INIT;
    msg.id = CO_NMT_EC_CANID(MASTER_DEV_ID);
    msg.len = 1u;
    msg.data[0] = CO_NMT_ST_START;

    CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

    timespec ts = {0, 0};
    timespec_add_msec(&ts, HB_TIMEOUT_MS);
    can_net_set_time(net, &ts);
    CoNmtHbInd::Clear();

    if (enabled) {
      if (ttoggle > 1u) {
        timespec_add_msec(&ts, HB_TIMEOUT_MS * (ttoggle - 1));
        can_net_set_time(net, &ts);
      }

      CHECK_EQUAL(BUS_B_ID, co_nmt_get_active_bus_id(nmt));
      CHECK_EQUAL(0, CoNmtHbInd::GetNumCalled());
      CHECK_EQUAL(1u, CoNmtRdnInd::GetNumCalled());
      CoNmtRdnInd::Check(nmt, BUS_B_ID, CO_NMT_ECSS_RDN_BUS_SWITCH, nullptr);
    } else {
      CHECK_EQUAL(BUS_A_ID, co_nmt_get_active_bus_id(nmt));
      CHECK_EQUAL(0, CoNmtHbInd::GetNumCalled());
      CHECK_EQUAL(0, CoNmtRdnInd::GetNumCalled());
    }
  }

  void CreateNmtAndReset() override {
    super::CreateNmtAndReset();

    co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, nullptr);
    co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();
    ConfigRdn();
  }

  TEST_TEARDOWN() {
    CoNmtRdnInd::Clear();
    CoNmtHbInd::Clear();
    TEST_BASE_TEARDOWN();
  }
};

/// @name NMT redundancy manager service: the redundancy object modification
///       using SDO
///@{

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Redundancy Object
///
/// \When the download indication function for the object is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
TEST(CO_NmtRdnSdo, CoRdnDnInd_NonZeroAC) {
  CreateNmtAndReset();

  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret = LelyUnitTest::CallDnIndWithAbortCode(
      dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, 0x00, ac);

  CHECK_EQUAL(ac, ret);
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Redundancy Object with a sub-object
///
/// \When a value longer than one byte is downloaded to the sub-object
///
/// \Then CO_SDO_AC_TYPE_LEN_HI abort code is passed to the download
///       confirmation function, nothing is changed
TEST(CO_NmtRdnSdo, CoRdnDnInd_TypeLenTooHigh) {
  CreateNmtAndReset();

  const co_unsigned16_t val = 0;
  const auto ret = co_dev_dn_val_req(dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, 0x00,
                                     CO_DEFTYPE_UNSIGNED16, &val, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, 0x00,
                     CO_SDO_AC_TYPE_LEN_HI, nullptr);
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Redundancy Object with the "Highest sub-index supported" sub-object
///        (0x00)
///
/// \When any value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_NO_WRITE abort code is passed to the download confirmation
///       function, nothing is changed
TEST(CO_NmtRdnSdo, CoRdnDnInd_Sub00_MaxSubidxSupported_NoWrite) {
  CreateNmtAndReset();

  const co_unsigned8_t val = 0;
  const auto ret = co_dev_dn_val_req(dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, 0x00,
                                     CO_DEFTYPE_UNSIGNED8, &val, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, 0x00,
                     CO_SDO_AC_NO_WRITE, nullptr);
  CHECK_EQUAL(ObjNmtRedundancy::Sub00HighestSubidxSupported::default_val,
              obj_rdn->GetSub<ObjNmtRedundancy::Sub00HighestSubidxSupported>());
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Redundancy Object with the "Bdefault" sub-object
///
/// \When a value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value
TEST(CO_NmtRdnSdo, CoRdnDnInd_Sub01_Bdefault) {
  CreateNmtAndReset();

  const co_unsigned8_t val = BUS_B_ID;
  const auto ret = co_dev_dn_val_req(
      dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, CO_NMT_RDN_BDEFAULT_SUBIDX,
      CO_DEFTYPE_UNSIGNED8, &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
                     CO_NMT_RDN_BDEFAULT_SUBIDX, 0, nullptr);
  CHECK_EQUAL(val, obj_rdn->GetSub<ObjNmtRedundancy::Sub01Bdefault>());
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Redundancy Object with the "Ttoggle" sub-object; the NMT redundancy
///        manager service is enabled
///
/// \When a non-zero value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value
TEST(CO_NmtRdnSdo, CoRdnDnInd_Sub02_Ttoggle) {
  CreateNmtAndReset();

  const co_unsigned8_t val = 2u;
  const auto ret = co_dev_dn_val_req(
      dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, CO_NMT_RDN_TTOGGLE_SUBIDX,
      CO_DEFTYPE_UNSIGNED8, &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
                     CO_NMT_RDN_TTOGGLE_SUBIDX, 0, nullptr);
  CHECK_EQUAL(val, obj_rdn->GetSub<ObjNmtRedundancy::Sub02Ttoggle>());
  CheckRdnService(true, val);
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Redundancy Object with the "Ttoggle" sub-object; the NMT redundancy
///        manager service is enabled
///
/// \When a zero value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value, the NMT redundancy
///       manager service is disabled
TEST(CO_NmtRdnSdo, CoRdnDnInd_Sub02_Ttoggle_DisableRdn) {
  CreateNmtAndReset();

  const co_unsigned8_t val = 0;
  const auto ret = co_dev_dn_val_req(
      dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, CO_NMT_RDN_TTOGGLE_SUBIDX,
      CO_DEFTYPE_UNSIGNED8, &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
                     CO_NMT_RDN_TTOGGLE_SUBIDX, 0, nullptr);
  CHECK_EQUAL(val, obj_rdn->GetSub<ObjNmtRedundancy::Sub02Ttoggle>());
  CheckRdnService(false);
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Redundancy Object with the "Ttoggle" sub-object; the NMT redundancy
///        manager service is disabled
///
/// \When a non-zero value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value, the NMT redundancy
///       manager service is enabled
TEST(CO_NmtRdnSdo, CoRdnDnInd_Sub02_Ttoggle_EnableRdn) {
  obj_rdn->SetSub<ObjNmtRedundancy::Sub02Ttoggle>(0);
  CreateNmtAndReset();

  const co_unsigned8_t val = 5u;

  const auto ret = co_dev_dn_val_req(
      dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, CO_NMT_RDN_TTOGGLE_SUBIDX,
      CO_DEFTYPE_UNSIGNED8, &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
                     CO_NMT_RDN_TTOGGLE_SUBIDX, 0, nullptr);
  CHECK_EQUAL(val, obj_rdn->GetSub<ObjNmtRedundancy::Sub02Ttoggle>());
  CheckRdnService(true, val);
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Redundancy Object with the "Ttoggle" sub-object; the NMT redundancy
///        manager service is disabled
///
/// \When a zero value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value, the NMT redundancy
///       manager service is disabled
TEST(CO_NmtRdnSdo, CoRdnDnInd_Sub02_Ttoggle_Zero_DisabledRdn) {
  obj_rdn->SetSub<ObjNmtRedundancy::Sub02Ttoggle>(0);
  CreateNmtAndReset();

  const co_unsigned8_t val = 0;

  const auto ret = co_dev_dn_val_req(
      dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, CO_NMT_RDN_TTOGGLE_SUBIDX,
      CO_DEFTYPE_UNSIGNED8, &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
                     CO_NMT_RDN_TTOGGLE_SUBIDX, 0, nullptr);
  CHECK_EQUAL(val, obj_rdn->GetSub<ObjNmtRedundancy::Sub02Ttoggle>());
  CheckRdnService(false);
}

/// \Given a started NMT service (co_nmt_t) configured as NMT master, the
///        object dictionary contains the Redundancy Object with the "Ttoggle"
///        sub-object
///
/// \When any value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value
TEST(CO_NmtRdnSdo, CoRdnDnInd_Sub02_Ttoggle_Master) {
  dev_holder->CreateObjValue<Obj1f80NmtStartup>(obj1f80,
                                                Obj1f80NmtStartup::MASTER_BIT);
  CreateNmtAndReset();

  const co_unsigned8_t val = 5u;

  const auto ret = co_dev_dn_val_req(
      dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, CO_NMT_RDN_TTOGGLE_SUBIDX,
      CO_DEFTYPE_UNSIGNED8, &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
                     CO_NMT_RDN_TTOGGLE_SUBIDX, 0, nullptr);
  CHECK_EQUAL(val, obj_rdn->GetSub<ObjNmtRedundancy::Sub02Ttoggle>());
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Redundancy Object with the "Ntoggle" sub-object
///
/// \When any value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value
TEST(CO_NmtRdnSdo, CoRdnDnInd_Sub03_Ntoggle) {
  CreateNmtAndReset();

  const co_unsigned8_t val = 5u;
  const auto ret = co_dev_dn_val_req(
      dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, CO_NMT_RDN_NTOGGLE_SUBIDX,
      CO_DEFTYPE_UNSIGNED8, &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
                     CO_NMT_RDN_NTOGGLE_SUBIDX, 0, nullptr);
  CHECK_EQUAL(val, obj_rdn->GetSub<ObjNmtRedundancy::Sub03Ntoggle>());
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Redundancy Object with the "Ctoggle" sub-object
///
/// \When any value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value
TEST(CO_NmtRdnSdo, CoRdnDnInd_Sub04_Ctoggle) {
  CreateNmtAndReset();

  const co_unsigned8_t val = 5u;
  const auto ret = co_dev_dn_val_req(
      dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, CO_NMT_RDN_CTOGGLE_SUBIDX,
      CO_DEFTYPE_UNSIGNED8, &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
                     CO_NMT_RDN_CTOGGLE_SUBIDX, 0, nullptr);
  CHECK_EQUAL(val, obj_rdn->GetSub<ObjNmtRedundancy::Sub04Ctoggle>());
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Redundancy Object with the sub-object after the "Ctoggle" sub-object
///
/// \When any value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_NO_SUB abort code is passed to the download confirmation
///       function, nothing is changed
TEST(CO_NmtRdnSdo, CoRdnDnInd_NoSub) {
  const co_unsigned8_t idx = CO_NMT_RDN_CTOGGLE_SUBIDX + 1u;

  obj_rdn->InsertAndSetSub(idx, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0});
  CreateNmtAndReset();

  const co_unsigned8_t val = 5u;
  const auto ret = co_dev_dn_val_req(dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, idx,
                                     CO_DEFTYPE_UNSIGNED8, &val, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, idx,
                     CO_SDO_AC_NO_SUB, nullptr);
  CHECK_EQUAL(0, co_dev_get_val_u8(dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, idx));
}

///@}

#endif  // !LELY_NO_CO_ECSS_REDUNDANCY
