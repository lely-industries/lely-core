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

#include <lely/co/nmt.h>
#include <lely/util/time.h>
#include <lib/co/nmt_rdn.h>

#include <libtest/allocators/default.hpp>
#include <libtest/allocators/limited.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"

#include "obj-init/nmt-hb-consumer.hpp"
#include "obj-init/nmt-redundancy.hpp"
#include "obj-init/nmt-startup.hpp"

TEST_GROUP(CO_NmtRdn) {
  Allocators::Default allocator;

  static const co_unsigned8_t DEV_ID = 0x02u;
  static const co_unsigned8_t MASTER_DEV_ID = 0x01u;

  can_net_t* net = nullptr;
  co_dev_t* dev = nullptr;
  co_nmt_t* nmt = nullptr;

  std::unique_ptr<CoObjTHolder> objNmtRedundancy;
  std::unique_ptr<CoObjTHolder> obj1016;
  std::unique_ptr<CoObjTHolder> obj1f80;

  std::unique_ptr<CoDevTHolder> dev_holder;

  const uint_least16_t HB_TIMEOUT_MS = 550u;
  const uint_least8_t BDEFAULT = 0u;
  const uint_least8_t TTOGGLE = 3u;
  const uint_least8_t NTOGGLE = 5u;
  const uint_least8_t INITIAL_CTOGGLE = 0u;

  can_msg CreateMasterHbMsg(const co_unsigned8_t state) const {
    can_msg msg = CAN_MSG_INIT;
    msg.id = CO_NMT_EC_CANID(MASTER_DEV_ID);
    msg.len = 1u;
    msg.data[0] = state;

    return msg;
  }

  void CreateNMT() {
    nmt = co_nmt_create(net, dev);
    CHECK(nmt != nullptr);
  }

  void ConfigDefaultRdn() {
    dev_holder->CreateAndInsertObj(objNmtRedundancy,
                                   CO_NMT_RDN_REDUNDANCY_OBJ_IDX);
    ObjNmtRedundancy::InsertAndSetAllSubValues(
        *objNmtRedundancy, BDEFAULT, TTOGGLE, NTOGGLE, INITIAL_CTOGGLE);

    dev_holder->CreateAndInsertObj(obj1016, 0x1016u);
    Obj1016ConsumerHb::Set00HighestSubidxSupported(*obj1016, 0x01);
    Obj1016ConsumerHb::SetNthConsumerHbTime(
        *obj1016, 0x01,
        Obj1016ConsumerHb::MakeHbConsumerEntry(MASTER_DEV_ID, HB_TIMEOUT_MS));
  }

  void CreateDefaultNMT() {
    ConfigDefaultRdn();
    CreateNMT();
  }

  void ResetNMT() { co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE); }

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
    if (nmt != nullptr) co_nmt_destroy(nmt);

    dev_holder.reset();
    can_net_destroy(net);
    set_errnum(0);

    CoNmtRdnInd::Clear();
    CoNmtHbInd::Clear();
    CanSend::Clear();
  }
};

/// @name co_nmt_rdn_chk_dev()
///@{

/// \Given a pointer to the device (co_dev_t), the object dictionary contains
///        a correct NMT redundancy object with not entries
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 1 is returned
TEST(CO_NmtRdn, CoNmtRdnChkDev_Nominal) {
  ConfigDefaultRdn();

  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(1, ret);
}

/// \Given a pointer to the device (co_dev_t), the object dictionary does not
///        contain the NMT redundancy object
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 1 is returned
TEST(CO_NmtRdn, CoNmtRdnChkDev_NoObject) {
  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(1, ret);
}

/// \Given a pointer to the device (co_dev_t), the object dictionary contains
///        the NMT redundancy object which does not have the first entry with
///        the highest sub-index supported
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 0 is returned
TEST(CO_NmtRdn, CoNmtRdnChkDev_NoMaxSubidxSub) {
  dev_holder->CreateAndInsertObj(objNmtRedundancy,
                                 CO_NMT_RDN_REDUNDANCY_OBJ_IDX);

  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(0, ret);
}

/// \Given a pointer to the device (co_dev_t), the object dictionary contains
///        the NMT redundancy object which does not have the Bdefault entry
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 0 is returned
TEST(CO_NmtRdn, CoNmtRdnChkDev_NoBdefaultSub) {
  dev_holder->CreateAndInsertObj(objNmtRedundancy,
                                 CO_NMT_RDN_REDUNDANCY_OBJ_IDX);

  objNmtRedundancy->InsertAndSetSub(
      0x00u, CO_DEFTYPE_UNSIGNED8,
      co_unsigned8_t(CO_NMT_REDUNDANCY_OBJ_MAX_IDX));

  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(0, ret);
}

/// \Given a pointer to the device (co_dev_t), the object dictionary contains
///        the NMT redundancy object which does not have the Ttoggle entry
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 1 is returned
TEST(CO_NmtRdn, CoNmtRdnChkDev_NoTtoggleSub) {
  dev_holder->CreateAndInsertObj(objNmtRedundancy,
                                 CO_NMT_RDN_REDUNDANCY_OBJ_IDX);

  objNmtRedundancy->InsertAndSetSub(
      0x00u, CO_DEFTYPE_UNSIGNED8,
      co_unsigned8_t(CO_NMT_REDUNDANCY_OBJ_MAX_IDX));
  objNmtRedundancy->InsertAndSetSub(CO_NMT_RDN_BDEFAULT_SUBIDX,
                                    CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0));

  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(1, ret);
}

/// \Given a pointer to the device (co_dev_t), the object dictionary contains
///        the NMT redundancy object which does not have the Ntoggle entry
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 1 is returned
TEST(CO_NmtRdn, CoNmtRdnChkDev_NoNtoggleSub) {
  dev_holder->CreateAndInsertObj(objNmtRedundancy,
                                 CO_NMT_RDN_REDUNDANCY_OBJ_IDX);

  objNmtRedundancy->InsertAndSetSub(
      0x00u, CO_DEFTYPE_UNSIGNED8,
      co_unsigned8_t(CO_NMT_REDUNDANCY_OBJ_MAX_IDX));
  objNmtRedundancy->InsertAndSetSub(CO_NMT_RDN_BDEFAULT_SUBIDX,
                                    CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0));
  objNmtRedundancy->InsertAndSetSub(CO_NMT_RDN_TTOGGLE_SUBIDX,
                                    CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0));

  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(1, ret);
}

/// \Given a pointer to the device (co_dev_t), the object dictionary contains
///        the NMT redundancy object which does not have the Ctoggle entry
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 1 is returned
TEST(CO_NmtRdn, CoNmtRdnChkDev_NoCtoggleSub) {
  dev_holder->CreateAndInsertObj(objNmtRedundancy,
                                 CO_NMT_RDN_REDUNDANCY_OBJ_IDX);

  objNmtRedundancy->InsertAndSetSub(
      0x00u, CO_DEFTYPE_UNSIGNED8,
      co_unsigned8_t(CO_NMT_REDUNDANCY_OBJ_MAX_IDX));
  objNmtRedundancy->InsertAndSetSub(CO_NMT_RDN_BDEFAULT_SUBIDX,
                                    CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0));
  objNmtRedundancy->InsertAndSetSub(CO_NMT_RDN_TTOGGLE_SUBIDX,
                                    CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0));
  objNmtRedundancy->InsertAndSetSub(CO_NMT_RDN_NTOGGLE_SUBIDX,
                                    CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0));

  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(1, ret);
}

///@}

/// @name co_nmt_get_ecss_rdn_ind()
///@{

/// \Given a pointer to the NMT service (co_nmt_t)
///
/// \When co_nmt_get_ecss_rdn_ind() is called with no memory area to store the
///       results
///
/// \Then nothing is changed
TEST(CO_NmtRdn, CoNmtGetEcssRdnInd_NoMemoryToStoreResults) {
  CreateDefaultNMT();

  co_nmt_get_ecss_rdn_ind(nmt, nullptr, nullptr);
}

/// \Given a pointer to the NMT service (co_nmt_t)
///
/// \When co_nmt_get_ecss_rdn_ind() is called with a memory area to store the
///       redundancy indication function and a memory area to store the
///       user-specified data pointer
///
/// \Then null pointers are returned
TEST(CO_NmtRdn, CoNmtGetEcssRdnInd_DefaultInd) {
  CreateDefaultNMT();

  int user_specified_data = 0;
  co_nmt_ecss_rdn_ind_t* ind = CoNmtRdnInd::Func;
  void* data = &user_specified_data;

  co_nmt_get_ecss_rdn_ind(nmt, &ind, &data);

  POINTERS_EQUAL(nullptr, ind);
  POINTERS_EQUAL(nullptr, data);
}

/// \Given a pointer to the NMT service (co_nmt_t) with the NMT redundancy
///        indication function set
///
/// \When co_nmt_get_ecss_rdn_ind() is called with a memory area to store the
///       redundancy indication function and a memory area to store the
///       user-specified data pointer
///
/// \Then the correct pointers are returned
TEST(CO_NmtRdn, CoNmtGetEcssRdnInd_CustomInd) {
  CreateDefaultNMT();
  int user_specified_data = 0;
  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, &user_specified_data);

  co_nmt_ecss_rdn_ind_t* ind = nullptr;
  void* data = nullptr;

  co_nmt_get_ecss_rdn_ind(nmt, &ind, &data);

  POINTERS_EQUAL(CoNmtRdnInd::Func, ind);
  POINTERS_EQUAL(&user_specified_data, data);
}

///@}

/// @name co_nmt_ecss_rdn_ind()
///@{

/// \Given a pointer to NMT service (co_nmt_t) with the NMT redundancy
///        indication function set
///
/// \When co_nmt_ecss_rdn_ind() is called with a pointer to the NMT service,
///       any bus ID and any reason
///
/// \Then the NMT redundancy indication function is called with the pointer to
///       the NMT service, the given bus ID and state, and a null pointer as
///       user-specified data pointer
TEST(CO_NmtRdn, CoNmtEcssRdnInd_Nominal) {
  CreateDefaultNMT();

  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);

  co_nmt_ecss_rdn_ind(nmt, 0, 0);

  CHECK_EQUAL(1u, CoNmtRdnInd::GetNumCalled());
  CoNmtRdnInd::Check(nmt, 0, 0, nullptr);
}

/// \Given a pointer to the NMT service (co_nmt_t) with no NMT redundancy
///        indication function set
///
/// \When co_nmt_ecss_rdn_ind() is called with a pointer to the NMT service
///       any bus ID and any reason
///
/// \Given nothing is changed
TEST(CO_NmtRdn, CoNmtRdnInd_NoRdnInd) {
  CreateDefaultNMT();

  co_nmt_ecss_rdn_ind(nmt, 0, 0);
}

///@}

#if !LELY_NO_CO_MASTER

/// @name co_nmt_set_active_bus()
///@{

/// \Given a pointer to the NMT service (co_nmt_t) configured as NMT master
///
/// \When co_nmt_set_active_bus() is called with a bus identifier
///
/// \Then 0 is returned, the active bus is set
TEST(CO_NmtRdn, CoNmtSetActiveBus_Master) {
  dev_holder->CreateAndInsertObj(obj1f80, 0x1f80u);
  Obj1f80NmtStartup::SetVal(*obj1f80, Obj1f80NmtStartup::MASTER_BIT);
  CreateNMT();
  ResetNMT();
  CanSend::Clear();

  const co_unsigned8_t bus_id = 1u;
  const auto ret = co_nmt_set_active_bus(nmt, bus_id);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(bus_id, co_nmt_get_active_bus_id(nmt));
}

/// \Given a pointer to the NMT service (co_nmt_t) configured as NMT slave
///
/// \When co_nmt_set_active_bus() is called with a bus identifier
///
/// \Then -1 is returned, the error number is set to ERRNUM_PERM and the
///       active bus is not changed
TEST(CO_NmtRdn, CoNmtSetActiveBus_Slave) {
  CreateNMT();
  ResetNMT();
  CanSend::Clear();

  const auto ret = co_nmt_set_active_bus(nmt, 1u);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_PERM, get_errnum());
  CHECK_EQUAL(0, co_nmt_get_active_bus_id(nmt));
}

///@}

#endif  // !LELY_NO_CO_MASTER

/// @name NMT slave bus selection process
///@{

/// \Given a pointer to the NMT service (co_nmt_t) configured as NMT slave with
///        with the NMT redundancy manager configured; the bus selection
///        process is active
///
/// \When an NMT heartbeat message from the Redundancy Master is received
///
/// \Then the NMT heartbeat indication function is called with
///       CO_NMT_EC_OCCURRED state and CO_NMT_EC_STATE reason, the NMT
///       redundancy indication function is not called
TEST(CO_NmtRdn, CoNmtRdnSlaveBusSelection_NormalOperation) {
  CreateDefaultNMT();
  ResetNMT();
  co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, nullptr);
  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);

  can_msg msg = CreateMasterHbMsg(CO_NMT_ST_START);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, 1, CO_NMT_EC_OCCURRED, CO_NMT_EC_STATE, nullptr);
  CHECK_EQUAL(0u, CoNmtRdnInd::GetNumCalled());
}

///@}

/// @name NMT slave heartbeat timeout
///@{

/// \Given a pointer to the NMT service (co_nmt_t) configured as NMT slave with
///        with the NMT redundancy manager configured and the alternate bus
///        set; the node is in the bus selection process after missing an NMT
///        heartbeat message from the Redundancy Master
///
/// \When the node switches to the alternate bus and an NMT heartbeat message
///       from the Redundancy Master is received on the alternate bus
///
/// \Then the NMT heartbeat indication function is called with
///       CO_NMT_EC_RESOLVED state and CO_NMT_EC_TIMEOUT reason
TEST(CO_NmtRdn, CoNmtRdnHbTimeout_SwitchBus_Resolved) {
  CreateDefaultNMT();
  ResetNMT();
  co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, nullptr);
  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);
  co_nmt_set_alternate_bus_id(nmt, 1);

  timespec ts = {0, 0};
  can_net_set_time(net, &ts);

  can_msg msg = CreateMasterHbMsg(CO_NMT_ST_START);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CoNmtRdnInd::GetNumCalled());
  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, 1, CO_NMT_EC_OCCURRED, CO_NMT_EC_STATE, nullptr);
  CoNmtHbInd::Clear();

  timespec_add_msec(&ts, HB_TIMEOUT_MS);
  can_net_set_time(net, &ts);

  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, 1, CO_NMT_EC_OCCURRED, CO_NMT_EC_TIMEOUT, nullptr);
  CHECK_EQUAL(0, CoNmtRdnInd::GetNumCalled());
  CoNmtHbInd::Clear();

  for (int i = 0; i < TTOGGLE; i++) {
    timespec_add_msec(&ts, HB_TIMEOUT_MS);
    can_net_set_time(net, &ts);
  }

  CHECK_EQUAL(1, CoNmtRdnInd::GetNumCalled());
  CoNmtRdnInd::Check(nmt, 1, CO_NMT_ECSS_RDN_BUS_SWITCH, nullptr);
  CoNmtRdnInd::Clear();

  CHECK_EQUAL(1u, can_net_recv(net, &msg, 1));

  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, 1, CO_NMT_EC_RESOLVED, CO_NMT_EC_TIMEOUT, nullptr);
  CHECK_EQUAL(0, CoNmtRdnInd::GetNumCalled());
}

/// \Given a pointer to the NMT service (co_nmt_t) configured as NMT slave with
///        with the NMT redundancy manager configured and the alternate bus
///        set; the node is in the bus selection process after missing an NMT
///        heartbeat message from the Redundancy Master
///
/// \When the node switches NTOGGLE times between the primary and the alternate
///       buses and no NMT heartbeat message is received from the Redundancy
///       Master during
///       `NTOGGLE * TTOGGLE * <the Redundancy Master heartbeat time>` time
///       (ms)
///
/// \Then the NMT redundancy indication function is called NTOGGLE times with
///       CO_NMT_ECSS_RDN_BUS_SWITCH reason and at the end once more with
///       CO_NMT_ECSS_RDN_NO_MASTER reason
TEST(CO_NmtRdn, CoNmtRdnHbTimeout_SwitchBus_NoMaster) {
  CreateDefaultNMT();
  ResetNMT();
  co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, nullptr);
  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);
  co_nmt_set_alternate_bus_id(nmt, 1);

  timespec ts = {0, 0};
  can_net_set_time(net, &ts);

  can_msg msg = CreateMasterHbMsg(CO_NMT_ST_START);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CoNmtRdnInd::GetNumCalled());
  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, 1, CO_NMT_EC_OCCURRED, CO_NMT_EC_STATE, nullptr);
  CoNmtHbInd::Clear();

  timespec_add_msec(&ts, HB_TIMEOUT_MS);
  can_net_set_time(net, &ts);

  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, 1, CO_NMT_EC_OCCURRED, CO_NMT_EC_TIMEOUT, nullptr);
  CHECK_EQUAL(0, CoNmtRdnInd::GetNumCalled());
  CoNmtHbInd::Clear();

  for (int i = 0; i < TTOGGLE; i++) {
    timespec_add_msec(&ts, HB_TIMEOUT_MS);
    can_net_set_time(net, &ts);
  }
  CHECK_EQUAL(1, CoNmtRdnInd::GetNumCalled());
  CoNmtRdnInd::Check(nmt, 1, CO_NMT_ECSS_RDN_BUS_SWITCH, nullptr);

  for (unsigned int i = 1; i < NTOGGLE; i++) {
    timespec_add_msec(&ts, TTOGGLE * HB_TIMEOUT_MS);
    can_net_set_time(net, &ts);
  }
  const co_unsigned8_t ctoggle = co_sub_get_val_u8(co_dev_find_sub(
      dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, CO_NMT_RDN_CTOGGLE_SUBIDX));
  CHECK_EQUAL(NTOGGLE, ctoggle);
  CoNmtRdnInd::Check(nmt, 1, CO_NMT_ECSS_RDN_NO_MASTER, nullptr);
}

///@}
