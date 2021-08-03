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
#include <map>

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
#include <libtest/tools/can-send.hpp>
#include <libtest/tools/co-nmt-rdn-ind.hpp>
#include <libtest/tools/co-nmt-hb-ind.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"

#include "obj-init/nmt-hb-consumer.hpp"
#include "obj-init/nmt-hb-producer.hpp"
#include "obj-init/nmt-redundancy.hpp"
#include "obj-init/nmt-startup.hpp"
#include "obj-init/nmt-slave-assignment.hpp"
#include "obj-init/request-nmt.hpp"

namespace CO_NmtSdoConsts {
static const co_unsigned8_t MASTER_DEV_ID = 0x01u;
}

TEST_BASE(CO_NmtSdo) {
  TEST_BASE_SUPER(CO_NmtSdo);
  Allocators::Default allocator;

  static const co_unsigned8_t DEV_ID = 0x02u;
  static const co_unsigned8_t MASTER_DEV_ID = CO_NmtSdoConsts::MASTER_DEV_ID;

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
    CanSend::Clear();
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
    set_errnum(ERRNUM_SUCCESS);
  }
};

namespace CO_NmtSdo1016 {
static const co_unsigned8_t HB_NODE_ID = 0x05u;
static const co_unsigned16_t HB_TIMEOUT_MS = 500u;
}  // namespace CO_NmtSdo1016

TEST_GROUP_BASE(CO_NmtSdo1016, CO_NmtSdo) {
  static const co_unsigned8_t HB_NODE_ID = CO_NmtSdo1016::HB_NODE_ID;
  static const co_unsigned16_t HB_TIMEOUT_MS = CO_NmtSdo1016::HB_TIMEOUT_MS;
  static const co_unsigned8_t HB_IDX = 0x01u;

  std::unique_ptr<CoObjTHolder> obj1016;

  void CreateNmtAndReset() override {
    super::CreateNmtAndReset();

    co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, nullptr);
  }

  void CheckHbConsumer(
      const bool enabled, const co_unsigned8_t id = CO_NmtSdo1016::HB_NODE_ID,
      const co_unsigned16_t timeout = CO_NmtSdo1016::HB_TIMEOUT_MS) {
    can_msg msg = CAN_MSG_INIT;
    msg.id = CO_NMT_EC_CANID(id);
    msg.len = 1u;
    msg.data[0] = CO_NMT_ST_START;

    CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

    if (enabled) {
      CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
      CoNmtHbInd::Check(nmt, id, CO_NMT_EC_OCCURRED, CO_NMT_EC_STATE, nullptr);

      timespec ts = {0, 0};
      timespec_add_msec(&ts, timeout);
      can_net_set_time(net, &ts);

      CHECK_EQUAL(2u, CoNmtHbInd::GetNumCalled());
      CoNmtHbInd::Check(nmt, id, CO_NMT_EC_OCCURRED, CO_NMT_EC_TIMEOUT,
                        nullptr);
    } else {
      CHECK_EQUAL(0, CoNmtHbInd::GetNumCalled());
    }
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    dev_holder->CreateObj<Obj1016ConsumerHb>(obj1016);
    obj1016->EmplaceSub<Obj1016ConsumerHb::Sub00HighestSubidxSupported>(HB_IDX);
    obj1016->EmplaceSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(
        HB_IDX,
        Obj1016ConsumerHb::MakeHbConsumerEntry(HB_NODE_ID, HB_TIMEOUT_MS));
  }

  TEST_TEARDOWN() {
    CoNmtHbInd::Clear();
    TEST_BASE_TEARDOWN();
  }
};

// NOLINTNEXTLINE(whitespace/line_length)
/// @name NMT service: the Consumer Heartbeat Time object (0x1016) modification using SDO
///@{

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Consumer Heartbeat Time object (0x1016)
///
/// \When the download indication function for the object is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_NmtSdo1016, Co1016DnInd_NonZeroAC) {
  CreateNmtAndReset();

  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret =
      LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1016u, 0x00u, ac);

  CHECK_EQUAL(ac, ret);
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Consumer Heartbeat Time object (0x1016) with a sub-object
///
/// \When a value longer than the sub-object's data type length is downloaded
///       to the sub-object
///
/// \Then CO_SDO_AC_TYPE_LEN_HI abort code is passed to the download
///       confirmation function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
TEST(CO_NmtSdo1016, Co1016DnInd_TypeLenTooHigh) {
  CreateNmtAndReset();

  const co_unsigned16_t val = 0;
  const auto ret = co_dev_dn_val_req(dev, 0x1016u, 0x00u, CO_DEFTYPE_UNSIGNED16,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1016u, 0x00u, CO_SDO_AC_TYPE_LEN_HI, nullptr);
  CHECK_EQUAL(
      HB_IDX,
      obj1016->GetSub<Obj1016ConsumerHb::Sub00HighestSubidxSupported>());
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Consumer Heartbeat Time object (0x1016) with the "Highest sub-index
///        supported" sub-object (0x00)
///
/// \When any value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_NO_WRITE abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_NmtSdo1016, Co1016DnInd_Sub00_MaxSubidxSupported_NoWrite) {
  CreateNmtAndReset();

  const co_unsigned8_t val = 0;
  const auto ret = co_dev_dn_val_req(dev, 0x1016u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1016u, 0x00u, CO_SDO_AC_NO_WRITE, nullptr);
  CHECK_EQUAL(
      HB_IDX,
      obj1016->GetSub<Obj1016ConsumerHb::Sub00HighestSubidxSupported>());
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Consumer Heartbeat Time object (0x1016) with the sub-object after
///        the sub-index declared in the "Highest sub-index supported"
///        sub-object (0x00)
///
/// \When any value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_NO_SUB abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_NmtSdo1016, Co1016DnInd_NoSub) {
  const co_unsigned8_t idx = 0x02u;
  obj1016->EmplaceSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(idx, 0);
  CreateNmtAndReset();

  const co_unsigned32_t val = 5u;
  const auto ret = co_dev_dn_val_req(dev, 0x1016u, idx, CO_DEFTYPE_UNSIGNED32,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1016u, idx, CO_SDO_AC_NO_SUB, nullptr);
  CHECK_EQUAL(0, obj1016->GetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(idx));
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Consumer Heartbeat Time object (0x1016) with a heartbeat consumer
///        entry
///
/// \When the same value as the current sub-object's value is downloaded to the
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object's value remains unchanged
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_NmtSdo1016, Co1016DnInd_SubN_ConsumerHeartbeatTime_SameValue) {
  CreateNmtAndReset();

  const co_unsigned32_t val =
      obj1016->GetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(HB_IDX);

  const auto ret =
      co_dev_dn_val_req(dev, 0x1016u, HB_IDX, CO_DEFTYPE_UNSIGNED32, &val,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1016u, HB_IDX, 0, nullptr);
  CHECK_EQUAL(val,
              obj1016->GetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(HB_IDX));
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Consumer Heartbeat Time object (0x1016) with a heartbeat consumer
///        entry set up for a node
///
/// \When a value with the Node-ID equal to zero is downloaded to the
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value, the heartbeat consumer
///       for the node is disabled
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_dev_find_obj()
///       \Calls co_sub_dn()
///       \Calls co_nmt_hb_set_1016()
TEST(CO_NmtSdo1016, Co1016DnInd_SubN_ConsumerHeartbeatTime_NodeId_Zero) {
  CreateNmtAndReset();

  const co_unsigned32_t val =
      Obj1016ConsumerHb::MakeHbConsumerEntry(0, HB_TIMEOUT_MS);

  const auto ret =
      co_dev_dn_val_req(dev, 0x1016u, HB_IDX, CO_DEFTYPE_UNSIGNED32, &val,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1016u, HB_IDX, 0, nullptr);
  CHECK_EQUAL(val,
              obj1016->GetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(HB_IDX));
  CheckHbConsumer(false);
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Consumer Heartbeat Time object (0x1016) with a heartbeat consumer
///        entry set up for a node
///
/// \When a value with the Node-ID over `CO_NUM_NODES` is downloaded to the
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value, the heartbeat consumer
///       for the node is disabled
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_dev_find_obj()
///       \Calls co_sub_dn()
///       \Calls co_nmt_hb_set_1016()
TEST(CO_NmtSdo1016, Co1016DnInd_SubN_ConsumerHeartbeatTime_NodeId_OverMax) {
  CreateNmtAndReset();

  const co_unsigned32_t val =
      Obj1016ConsumerHb::MakeHbConsumerEntry(CO_NUM_NODES + 1u, HB_TIMEOUT_MS);

  const auto ret =
      co_dev_dn_val_req(dev, 0x1016u, HB_IDX, CO_DEFTYPE_UNSIGNED32, &val,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1016u, HB_IDX, 0, nullptr);
  CHECK_EQUAL(val,
              obj1016->GetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(HB_IDX));
  CheckHbConsumer(false);
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Consumer Heartbeat Time object (0x1016) with a heartbeat consumer
///        entry set up for a node
///
/// \When a value with the heartbeat time equal to zero is downloaded to the
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value, the heartbeat consumer
///       for the node is disabled
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_dev_find_obj()
///       \Calls co_sub_dn()
///       \Calls co_nmt_hb_set_1016()
TEST(CO_NmtSdo1016, Co1016DnInd_SubN_ConsumerHeartbeatTime_HbTime_Zero) {
  CreateNmtAndReset();

  const co_unsigned32_t val =
      Obj1016ConsumerHb::MakeHbConsumerEntry(HB_NODE_ID, 0);

  const auto ret =
      co_dev_dn_val_req(dev, 0x1016u, HB_IDX, CO_DEFTYPE_UNSIGNED32, &val,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1016u, HB_IDX, 0, nullptr);
  CHECK_EQUAL(val,
              obj1016->GetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(HB_IDX));
  CheckHbConsumer(false);
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Consumer Heartbeat Time object (0x1016) with a heartbeat consumer
///        entry set up for a node
///
/// \When a value with the same Node-ID as the existing one is downloaded to
///       another sub-object
///
/// \Then CO_SDO_AC_PARAM abort code is passed to the download confirmation
///       function, the sub-object's value remains unchanged, the heartbeat
///       consumer for the node remains enabled
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_dev_find_obj()
TEST(CO_NmtSdo1016, Co1016DnInd_SubN_ConsumerHeartbeatTime_DuplicatedNodeId) {
  const co_unsigned8_t idx = 0x02u;
  obj1016->SetSub<Obj1016ConsumerHb::Sub00HighestSubidxSupported>(idx);
  obj1016->EmplaceSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(idx, 0);
  CreateNmtAndReset();

  const co_unsigned32_t val =
      Obj1016ConsumerHb::MakeHbConsumerEntry(HB_NODE_ID, HB_TIMEOUT_MS);

  const auto ret = co_dev_dn_val_req(dev, 0x1016u, idx, CO_DEFTYPE_UNSIGNED32,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1016u, idx, CO_SDO_AC_PARAM, nullptr);
  CHECK_EQUAL(0, obj1016->GetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(idx));
  CheckHbConsumer(true);
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Consumer Heartbeat Time object (0x1016) with a heartbeat consumer
///        entry set up for a node
///
/// \When a new value with the same Node-ID as the existing one is downloaded
///       to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value, the heartbeat consumer
///       for the node is updated
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_dev_find_obj()
///       \Calls co_sub_dn()
///       \Calls co_nmt_hb_set_1016()
TEST(CO_NmtSdo1016, Co1016DnInd_SubN_ConsumerHeartbeatTime_UpdateConsumer) {
  CreateNmtAndReset();

  const co_unsigned32_t val =
      Obj1016ConsumerHb::MakeHbConsumerEntry(HB_NODE_ID, HB_TIMEOUT_MS + 1u);

  const auto ret =
      co_dev_dn_val_req(dev, 0x1016u, HB_IDX, CO_DEFTYPE_UNSIGNED32, &val,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1016u, HB_IDX, 0, nullptr);
  CHECK_EQUAL(val,
              obj1016->GetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(HB_IDX));
  CheckHbConsumer(true, HB_NODE_ID, HB_TIMEOUT_MS + 1u);
}

///@}

TEST_GROUP_BASE(CO_NmtSdo1017, CO_NmtSdo) {
  static const co_unsigned16_t HB_TIMEOUT_MS = 500u;

  std::unique_ptr<CoObjTHolder> obj1017;

  void CreateNmtAndReset() override {
    super::CreateNmtAndReset();

    co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, nullptr);
  }

  void CheckHbProducer(const bool enabled, const co_unsigned16_t timeout) {
    timespec ts = {0, 0};
    timespec_add_msec(&ts, timeout);
    can_net_set_time(net, &ts);

    if (enabled) {
      CHECK_EQUAL(1u, CanSend::GetNumCalled());
      const uint_least8_t data[1u] = {CO_NMT_ST_START};
      CanSend::CheckMsg(CO_NMT_EC_CANID(DEV_ID), 0, 1u, data);
    } else {
      CHECK_EQUAL(0, CanSend::GetNumCalled());
    }
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    dev_holder->CreateObjValue<Obj1017ProducerHb>(obj1017, HB_TIMEOUT_MS);
  }

  TEST_TEARDOWN() {
    CoNmtHbInd::Clear();
    TEST_BASE_TEARDOWN();
  }
};

// NOLINTNEXTLINE(whitespace/line_length)
/// @name NMT service: the Producer Heartbeat Time object (0x1017) modification using SDO
///@{

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Producer Heartbeat Time object (0x1017)
///
/// \When the download indication function for the object is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_NmtSdo1017, Co1017DnInd_NonZeroAC) {
  CreateNmtAndReset();

  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret =
      LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1017u, 0x00u, ac);

  CHECK_EQUAL(ac, ret);
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Producer Heartbeat Time object (0x1017) with a sub-object
///
/// \When a value longer than the sub-object's data type length is downloaded
///       to the sub-object
///
/// \Then CO_SDO_AC_TYPE_LEN_HI abort code is passed to the download
///       confirmation function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
TEST(CO_NmtSdo1017, Co1017DnInd_TypeLenTooHigh) {
  CreateNmtAndReset();

  const co_unsigned32_t val = 0;
  const auto ret = co_dev_dn_val_req(dev, 0x1017u, 0x00u, CO_DEFTYPE_UNSIGNED32,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1017u, 0x00u, CO_SDO_AC_TYPE_LEN_HI, nullptr);
  CHECK_EQUAL(HB_TIMEOUT_MS, obj1017->GetSub<Obj1017ProducerHb>());
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Producer Heartbeat Time object (0x1017) with the sub-object at the
///        sub-index 0x02
///
/// \When any value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_NO_SUB abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_NmtSdo1017, Co1017DnInd_NoSub) {
  const co_unsigned8_t idx = 0x02u;
  obj1017->InsertAndSetSub(idx, CO_DEFTYPE_UNSIGNED16, co_unsigned16_t{0});
  CreateNmtAndReset();

  const co_unsigned32_t val = 5u;
  const auto ret = co_dev_dn_val_req(dev, 0x1017u, idx, CO_DEFTYPE_UNSIGNED16,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1017u, idx, CO_SDO_AC_NO_SUB, nullptr);
  CHECK_EQUAL(0, co_dev_get_val_u16(dev, 0x1017u, idx));
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains
///        Producer Heartbeat Time object (0x1017) with a sub-object
///
/// \When the same value as the current sub-object's value is downloaded to the
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object's value remains unchanged
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u16()
TEST(CO_NmtSdo1017, Co1017DnInd_SameValue) {
  CreateNmtAndReset();

  const co_unsigned16_t val = obj1017->GetSub<Obj1017ProducerHb>();

  const auto ret = co_dev_dn_val_req(dev, 0x1017u, 0x00u, CO_DEFTYPE_UNSIGNED16,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1017u, 0x00u, 0, nullptr);
  CHECK_EQUAL(val, obj1017->GetSub<Obj1017ProducerHb>());
  CheckHbProducer(true, val);
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains
///        Producer Heartbeat Time object (0x1017) with a sub-object
///
/// \When a new value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value, the heartbeat producer
///       is updated
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u16()
///       \Calls co_sub_dn()
///       \Calls can_timer_start()
TEST(CO_NmtSdo1017, Co1017DnInd_Update) {
  CreateNmtAndReset();

  const co_unsigned16_t val = HB_TIMEOUT_MS - 1u;

  const auto ret = co_dev_dn_val_req(dev, 0x1017u, 0x00u, CO_DEFTYPE_UNSIGNED16,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1017u, 0x00u, 0, nullptr);
  CHECK_EQUAL(val, obj1017->GetSub<Obj1017ProducerHb>());
  CheckHbProducer(true, HB_TIMEOUT_MS - 1u);
}

///@}

#if !LELY_NO_CO_ECSS_REDUNDANCY

TEST_GROUP_BASE(CO_NmtRdnSdo, CO_NmtSdo) {
  std::unique_ptr<CoObjTHolder> obj_rdn;
  std::unique_ptr<CoObjTHolder> obj1016;

  static const co_unsigned8_t BUS_A_ID = 0x00u;
  static const co_unsigned8_t BUS_B_ID = 0x01u;

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

  void CheckRdnService(
      const bool enabled, const co_unsigned8_t ttoggle = 1u,
      const co_unsigned8_t master_id = CO_NmtSdoConsts::MASTER_DEV_ID) {
    if (enabled) co_nmt_set_alternate_bus_id(nmt, BUS_B_ID);

    can_msg msg = CAN_MSG_INIT;
    msg.id = CO_NMT_EC_CANID(master_id);
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

// NOLINTNEXTLINE(whitespace/line_length)
/// @name NMT redundancy manager service: the redundancy object modification using SDO
///@{

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Redundancy Object
///
/// \When the download indication function for the object is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_NmtRdnSdo, CoRdnDnInd_NonZeroAC) {
  CreateNmtAndReset();

  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret = LelyUnitTest::CallDnIndWithAbortCode(
      dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, 0x00u, ac);

  CHECK_EQUAL(ac, ret);
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Redundancy Object with a sub-object
///
/// \When a value longer than the sub-object's data type length is downloaded
///       to the sub-object
///
/// \Then CO_SDO_AC_TYPE_LEN_HI abort code is passed to the download
///       confirmation function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
TEST(CO_NmtRdnSdo, CoRdnDnInd_TypeLenTooHigh) {
  CreateNmtAndReset();

  const co_unsigned16_t val = 0;
  const auto ret = co_dev_dn_val_req(dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, 0x00u,
                                     CO_DEFTYPE_UNSIGNED16, &val, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, 0x00u,
                     CO_SDO_AC_TYPE_LEN_HI, nullptr);
  CHECK_EQUAL(ObjNmtRedundancy::Sub00HighestSubidxSupported::default_val,
              obj_rdn->GetSub<ObjNmtRedundancy::Sub00HighestSubidxSupported>());
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Redundancy Object with the "Highest sub-index supported" sub-object
///        (0x00)
///
/// \When any value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_NO_WRITE abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_NmtRdnSdo, CoRdnDnInd_Sub00_MaxSubidxSupported_NoWrite) {
  CreateNmtAndReset();

  const co_unsigned8_t val = 0;
  const auto ret = co_dev_dn_val_req(dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, 0x00u,
                                     CO_DEFTYPE_UNSIGNED8, &val, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, 0x00u,
                     CO_SDO_AC_NO_WRITE, nullptr);
  CHECK_EQUAL(ObjNmtRedundancy::Sub00HighestSubidxSupported::default_val,
              obj_rdn->GetSub<ObjNmtRedundancy::Sub00HighestSubidxSupported>());
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Redundancy Object with any legal sub-object
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
TEST(CO_NmtRdnSdo, CoRdnDnInd_Sub01_SameValue) {
  CreateNmtAndReset();

  const co_unsigned8_t val = obj_rdn->GetSub<ObjNmtRedundancy::Sub01Bdefault>();
  const auto ret = co_dev_dn_val_req(
      dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, CO_NMT_RDN_BDEFAULT_SUBIDX,
      CO_DEFTYPE_UNSIGNED8, &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, CO_NMT_RDN_REDUNDANCY_OBJ_IDX,
                     CO_NMT_RDN_BDEFAULT_SUBIDX, 0, nullptr);
  CHECK_EQUAL(val, obj_rdn->GetSub<ObjNmtRedundancy::Sub01Bdefault>());
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Redundancy Object with the "Bdefault" sub-object
///
/// \When a value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_sub_dn()
///       \Calls co_nmt_is_master()
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
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_sub_dn()
///       \Calls co_nmt_is_master()
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
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_sub_dn()
///       \Calls co_nmt_is_master()
///       \Calls co_nmt_rdn_set_active_bus_default()
///       \Calls co_nmt_rdn_destroy()
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
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_sub_dn()
///       \Calls co_nmt_is_master()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u32()
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_rdn_create()}
///       \Calls co_nmt_rdn_set_master_id()
///       \Calls co_nmt_rdn_select_default_bus()
///       \Calls co_dev_set_val_u8()
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
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_sub_dn()
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
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_sub_dn()
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
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_sub_dn()
///       \Calls co_nmt_is_master()
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
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u8()
///       \Calls co_sub_dn()
///       \Calls co_nmt_is_master()
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
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
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

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Consumer Heartbeat Time object (0x1016) with the heartbeat consumer
///        entry set up for the Redundnacy Master; the NMT redundancy manager
///        service is enabled
///
/// \When any value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value, the Redundancy Master's
///       Node-ID is updated
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_dev_find_obj()
///       \Calls co_sub_dn()
///       \Calls co_nmt_hb_set_1016()
///       \Calls co_nmt_rdn_set_master_id()
TEST(CO_NmtRdnSdo, Co1016DnInd_SubN_ConsumerHeartbeatTime_UpdateRdnMaster) {
  const co_unsigned8_t NEW_MASTER_ID = 0x05u;
  CreateNmtAndReset();

  const co_unsigned32_t val =
      Obj1016ConsumerHb::MakeHbConsumerEntry(NEW_MASTER_ID, HB_TIMEOUT_MS);
  const auto ret = co_dev_dn_val_req(dev, 0x1016u, CO_NMT_RDN_MASTER_HB_IDX,
                                     CO_DEFTYPE_UNSIGNED32, &val, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1016u, CO_NMT_RDN_MASTER_HB_IDX, 0, nullptr);
  CHECK_EQUAL(val, obj1016->GetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(
                       CO_NMT_RDN_MASTER_HB_IDX));
  CheckRdnService(true, obj_rdn->GetSub<ObjNmtRedundancy::Sub02Ttoggle>(),
                  NEW_MASTER_ID);
}

///@}

#endif  // !LELY_NO_CO_ECSS_REDUNDANCY

TEST_GROUP_BASE(CO_NmtSdo1f80, CO_NmtSdo) {
  int32_t ignore = 0;  // clang-format fix

  TEST_SETUP() {
    TEST_BASE_SETUP();

    dev_holder->CreateObjValue<Obj1f80NmtStartup>(obj1f80, 0);
  }

  TEST_TEARDOWN() { TEST_BASE_TEARDOWN(); }
};

/// @name NMT service: the NMT Start-up object (0x1f80) modification using SDO
///@{

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        NMT Start-up object (0x1f80) with a sub-object
///
/// \When the download indication function for the object is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_NmtSdo1f80, Co1f80DnInd_NonZeroAC) {
  CreateNmtAndReset();

  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret = LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1f80u, 0x00, ac);

  CHECK_EQUAL(ac, ret);
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        NMT Start-up object (0x1f80) with a sub-object (0x00)
///
/// \When a value of incompatible size is downloaded to the sub-object
///
/// \Then CO_SDO_AC_TYPE_LEN_HI abort code is passed to the download
///       confirmation function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
TEST(CO_NmtSdo1f80, Co1f80DnInd_TypeLenTooHigh) {
  CreateNmtAndReset();

  const co_unsigned64_t val = 0;
  const auto ret = co_dev_dn_val_req(dev, 0x1f80u, 0x00, CO_DEFTYPE_UNSIGNED64,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1f80, 0x00, CO_SDO_AC_TYPE_LEN_HI, nullptr);
  CHECK_EQUAL(0, obj1f80->GetSub<Obj1f80NmtStartup>());
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        NMT Start-up object (0x1f80) with a sub-object (0x01)
///
/// \When any value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_NO_SUB abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_NmtSdo1f80, Co1f80DnInd_NoSub) {
  obj1f80->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED16, co_unsigned16_t{0});
  CreateNmtAndReset();

  const co_unsigned16_t val = 0xffffu;
  const auto ret = co_dev_dn_val_req(dev, 0x1f80u, 0x01, CO_DEFTYPE_UNSIGNED16,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1f80, 0x01, CO_SDO_AC_NO_SUB, nullptr);
  CHECK_EQUAL(0, co_dev_get_val_u16(dev, 0x1f80u, 0x01u));
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        NMT Start-up object (0x1f80) with a sub-object (0x00)
///
/// \When the value with an unsupported bit is downloaded to the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_NmtSdo1f80, Co1f80DnInd_UnsupportedBit) {
  CreateNmtAndReset();

  const co_unsigned32_t val = 0x20u;  // unsupported bit
  const auto ret = co_dev_dn_val_req(dev, 0x1f80u, 0x00, CO_DEFTYPE_UNSIGNED32,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1f80, 0x00, CO_SDO_AC_PARAM_VAL, nullptr);
  CHECK_EQUAL(0, obj1f80->GetSub<Obj1f80NmtStartup>());
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        NMT Start-up object (0x1f80) with a sub-object (0x00)
///
/// \When the same value as the current sub-object's value is downloaded to the
///       sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is not changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_NmtSdo1f80, Co1f80DnInd_SameValue) {
  CreateNmtAndReset();

  const co_unsigned32_t val = obj1f80->GetSub<Obj1f80NmtStartup>();
  const auto ret = co_dev_dn_val_req(dev, 0x1f80u, 0x00, CO_DEFTYPE_UNSIGNED32,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1f80u, 0x00, 0, nullptr);
  CHECK_EQUAL(0, obj1f80->GetSub<Obj1f80NmtStartup>());
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        NMT Start-up object (0x1f80) with a sub-object (0x00)
///
/// \When a correct value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       the sub-object is set to the requested value
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_sub_dn()
TEST(CO_NmtSdo1f80, Co1f80DnInd_Nominal) {
  CreateNmtAndReset();

  const co_unsigned32_t val = 0x01u;
  const auto ret = co_dev_dn_val_req(dev, 0x1f80u, 0x00, CO_DEFTYPE_UNSIGNED32,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1f80u, 0x00, 0, nullptr);
  CHECK_EQUAL(val, obj1f80->GetSub<Obj1f80NmtStartup>());
}

///@}

#if !LELY_NO_CO_MASTER

TEST_GROUP_BASE(CO_NmtSdo1f82, CO_NmtSdo) {
  static const co_unsigned8_t SLAVE_ID = 0x01u;
  static const size_t NMT_CS_MSG_SIZE = 2u;

  std::unique_ptr<CoObjTHolder> obj1f81;
  std::unique_ptr<CoObjTHolder> obj1f82;

  TEST_SETUP() {
    TEST_BASE_SETUP();

    dev_holder->CreateObjValue<Obj1f80NmtStartup>(
        obj1f80, Obj1f80NmtStartup::MASTER_BIT);

    dev_holder->CreateObj<Obj1f81NmtSlaveAssignment>(obj1f81);
    obj1f81->EmplaceSub<Obj1f81NmtSlaveAssignment::Sub00HighestSubidxSupported>(
        1u);
    obj1f81->EmplaceSub<Obj1f81NmtSlaveAssignment::SubNthSlaveEntry>(
        0x01u, Obj1f81NmtSlaveAssignment::ASSIGNMENT_BIT);

    dev_holder->CreateObj<Obj1f82RequestNmt>(obj1f82);
    obj1f82->EmplaceSub<Obj1f82RequestNmt::Sub00SupportedNumberOfSlaves>(
        CO_NUM_NODES);
    obj1f82->EmplaceSub<Obj1f82RequestNmt::SubNthRequestNmtService>(SLAVE_ID,
                                                                    0);
    obj1f82->EmplaceSub<Obj1f82RequestNmt::SubNthRequestNmtService>(DEV_ID, 0);
    obj1f82->EmplaceSub<Obj1f82RequestNmt::SubNthRequestNmtService>(0x03u, 0);
  }
};

/// @name NMT service: the Request NMT object (0x1f82) modification using SDO
///@{

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Request NMT object (0x1f82) with a sub-object
///
/// \When the download indication function for the object is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_NmtSdo1f82, Co1f82DnInd_NonZeroAC) {
  CreateNmtAndReset();

  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret = LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1f82u, 0x00, ac);

  CHECK_EQUAL(ac, ret);
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Request NMT object (0x1f82) with a sub-object
///
/// \When a value of incompatible size is downloaded to the sub-object
///
/// \Then CO_SDO_AC_TYPE_LEN_HI abort code is passed to the download
///       confirmation function, the sub-object is not modifed, an NMT request
///       is not sent
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
TEST(CO_NmtSdo1f82, Co1f82DnInd_TypeLenTooHigh) {
  CreateNmtAndReset();

  const co_unsigned16_t val = 0;
  const auto ret = co_dev_dn_val_req(dev, 0x1f82u, 0x00, CO_DEFTYPE_UNSIGNED16,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, 0x1f82, 0x00, CO_SDO_AC_TYPE_LEN_HI, nullptr);

  CHECK_EQUAL(
      CO_NUM_NODES,
      obj1f82->GetSub<Obj1f82RequestNmt::Sub00SupportedNumberOfSlaves>());
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains the
///        Request NMT object (0x1f82) with the "Supported number of slaves"
///        sub-object (0x00)
///
/// \When any value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_NO_WRITE abort code is passed to the download confirmation
///       function, the sub-object is not modifed, an NMT request is not sent
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_NmtSdo1f82, Co1f82DnInd_Sub00_SupportedNumberOfSlaves_NoWrite) {
  CreateNmtAndReset();

  const co_unsigned8_t val = 0;
  const auto ret = co_dev_dn_val_req(dev, 0x1f82, 0x00, CO_DEFTYPE_UNSIGNED8,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, 0x1f82, 0x00, CO_SDO_AC_NO_WRITE, nullptr);

  CHECK_EQUAL(
      CO_NUM_NODES,
      obj1f82->GetSub<Obj1f82RequestNmt::Sub00SupportedNumberOfSlaves>());
}

/// \Given a started NMT service (co_nmt_t) configured as NMT master, the
///        object dictionary contains the Request NMT object (0x1f82) with
///        a Request NMT Service entry (0x02), but a node with `Node-ID = 0x02`
///        is not known
///
/// \When any value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, the sub-object is not modifed, an NMT request is not sent
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_NmtSdo1f82, Co1f82DnInd_SubN_RequestNmt_UnknownNode) {
  CreateNmtAndReset();

  const co_unsigned8_t val = CO_NMT_ST_PREOP;
  const auto ret = co_dev_dn_val_req(dev, 0x1f82, 0x03u, CO_DEFTYPE_UNSIGNED8,
                                     &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, 0x1f82, 0x03u, CO_SDO_AC_PARAM_VAL, nullptr);

  CHECK_EQUAL(
      0, obj1f82->GetSub<Obj1f82RequestNmt::SubNthRequestNmtService>(0x03u));
}

/// \Given a started NMT service (co_nmt_t) configured as NMT master, the
///        object dictionary contains the Request NMT object (0x1f82) with
///        a Request NMT Service entry at a sub-index over the all-nodes value
///
/// \When any value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, the sub-object is not modifed, an NMT request is not sent
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_NmtSdo1f82, Co1f82DnInd_SubN_RequestNmt_NodeIdOverAllNodes) {
  obj1f82->EmplaceSub<Obj1f82RequestNmt::SubNthRequestNmtService>(
      Obj1f82RequestNmt::ALL_NODES + 1u, 0);
  CreateNmtAndReset();

  const co_unsigned8_t val = CO_NMT_ST_PREOP;
  const auto ret = co_dev_dn_val_req(
      dev, 0x1f82, Obj1f82RequestNmt::ALL_NODES + 1u, CO_DEFTYPE_UNSIGNED8,
      &val, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, 0x1f82, Obj1f82RequestNmt::ALL_NODES + 1u,
                     CO_SDO_AC_PARAM_VAL, nullptr);

  CHECK_EQUAL(0, obj1f82->GetSub<Obj1f82RequestNmt::SubNthRequestNmtService>(
                     Obj1f82RequestNmt::ALL_NODES + 1u));
}

/// \Given a started NMT service (co_nmt_t) configured as NMT master, the
///        object dictionary contains the Request NMT object (0x1f82) with
///        a Request NMT Service entry at a sub-index of a known slave node
///
/// \When a state value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       and the NMT request with the command specifier for a requested state
///       is sent to the slave, the sub-object is not modified
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_nmt_cs_req()
TEST(CO_NmtSdo1f82, Co1f82DnInd_SubN_RequestNmt_State) {
  CreateNmtAndReset();

  const std::map<co_unsigned8_t, co_unsigned8_t> request_nmt = {
      {CO_NMT_ST_STOP, CO_NMT_CS_STOP},
      {CO_NMT_ST_START, CO_NMT_CS_START},
      {CO_NMT_ST_RESET_NODE, CO_NMT_CS_RESET_NODE},
      {CO_NMT_ST_RESET_COMM, CO_NMT_CS_RESET_COMM},
      {CO_NMT_ST_PREOP, CO_NMT_CS_ENTER_PREOP},
  };

  for (auto const& req : request_nmt) {
    const co_unsigned8_t val = req.first;
    const auto ret =
        co_dev_dn_val_req(dev, 0x1f82, SLAVE_ID, CO_DEFTYPE_UNSIGNED8, &val,
                          nullptr, CoCsdoDnCon::Func, nullptr);

    CHECK_EQUAL(0, ret);
    CoCsdoDnCon::Check(nullptr, 0x1f82, SLAVE_ID, 0, nullptr);

    CHECK_EQUAL(1u, CanSend::GetNumCalled());
    const uint_least8_t data[NMT_CS_MSG_SIZE] = {req.second, SLAVE_ID};
    CanSend::CheckMsg(CO_NMT_CS_CANID, 0, NMT_CS_MSG_SIZE, data);

    CHECK_EQUAL(0, obj1f82->GetSub<Obj1f82RequestNmt::SubNthRequestNmtService>(
                       SLAVE_ID));
    CanSend::Clear();
  }
}

/// \Given a started NMT service (co_nmt_t) configured as NMT master, the
///        object dictionary contains the Request NMT object (0x1f82) with
///        the "all-nodes" sub-object (0x80)
///
/// \When a state value is downloaded to the sub-object
///
/// \Then a zero abort code is passed to the download confirmation function,
///       and the NMT request with the command specifier for a requested state
///       is sent to all nodes (`Node-ID = 0`), the sub-object is not modifed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_nmt_cs_req()
TEST(CO_NmtSdo1f82, Co1f82DnInd_SubN_RequestNmt_AllNodes) {
  obj1f82->EmplaceSub<Obj1f82RequestNmt::SubNthRequestNmtService>(
      Obj1f82RequestNmt::ALL_NODES, 0);
  CreateNmtAndReset();

  const co_unsigned8_t val = CO_NMT_ST_STOP;
  const auto ret = co_dev_dn_val_req(dev, 0x1f82, Obj1f82RequestNmt::ALL_NODES,
                                     CO_DEFTYPE_UNSIGNED8, &val, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CoCsdoDnCon::Check(nullptr, 0x1f82, Obj1f82RequestNmt::ALL_NODES, 0, nullptr);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  const uint_least8_t data[NMT_CS_MSG_SIZE] = {CO_NMT_CS_STOP, 0};
  CanSend::CheckMsg(CO_NMT_CS_CANID, 0, NMT_CS_MSG_SIZE, data);

  CHECK_EQUAL(0, obj1f82->GetSub<Obj1f82RequestNmt::SubNthRequestNmtService>(
                     Obj1f82RequestNmt::ALL_NODES));
}

/// \Given a started NMT service (co_nmt_t) configured as NMT master, the
///        object dictionary contains the Request NMT object (0x1f82) with
///        a Request NMT Service entry at a sub-index of a known slave node
///
/// \When a invalid state value is downloaded to the sub-object
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to the download confirmation
///       function, the sub-object is not modifed, an NMT request is not sent
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_NmtSdo1f82, Co1f82DnInd_SubN_RequestNmt_InvalidState) {
  CreateNmtAndReset();

  const co_unsigned8_t val = 0xffu;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1f82, SLAVE_ID, CO_DEFTYPE_UNSIGNED8, &val,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CoCsdoDnCon::Check(nullptr, 0x1f82, SLAVE_ID, CO_SDO_AC_PARAM_VAL, nullptr);

  CHECK_EQUAL(
      0, obj1f82->GetSub<Obj1f82RequestNmt::SubNthRequestNmtService>(SLAVE_ID));
}

///@}

#endif  // !LELY_NO_CO_MASTER
