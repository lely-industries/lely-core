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
#include <vector>

#include <CppUTest/TestHarness.h>

#include <lely/co/nmt.h>
#include <lely/co/csdo.h>
#include <lely/co/ssdo.h>
#include <lely/util/time.h>

#include <libtest/allocators/default.hpp>
#include <libtest/allocators/limited.hpp>
#include <libtest/override/lelyco-val.hpp>
#include <libtest/tools/can-send.hpp>
#include <libtest/tools/co-nmt-cs-ind.hpp>
#include <libtest/tools/co-nmt-st-ind.hpp>
#include <libtest/tools/co-csdo-dn-con.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/sub.hpp"
#include "common/nmt-alloc-sizes.hpp"

#include "obj-init/emcy-error-register.hpp"
#include "obj-init/emcy-predefined-error-field.hpp"
#include "obj-init/error-behavior-object.hpp"
#include "obj-init/nmt-hb-consumer.hpp"
#include "obj-init/nmt-hb-producer.hpp"
#include "obj-init/nmt-slave-assignment.hpp"
#include "obj-init/nmt-startup.hpp"

#include <lib/co/nmt_hb.h>
#if !LELY_NO_CO_NMT_BOOT
#include <lib/co/nmt_boot.h>
#endif
#if !LELY_NO_CO_NMT_CFG
#include <lib/co/nmt_cfg.h>
#endif
#if !LELY_NO_CO_ECSS_REDUNDANCY
#include <lib/co/nmt_rdn.h>
#endif

#if LELY_NO_MALLOC

#ifndef CO_NMT_CAN_BUF_SIZE
#define CO_NMT_CAN_BUF_SIZE 16u
#endif

#ifndef CO_NMT_MAX_NHB
#define CO_NMT_MAX_NHB CO_NUM_NODES
#endif

#endif  // LELY_NO_MALLOC

TEST_BASE(CO_NmtBase) {
  TEST_BASE_SUPER(CO_NmtBase);

  static const co_unsigned8_t DEV_ID = 0x01u;
  static const co_unsigned8_t MASTER_DEV_ID = DEV_ID;
  static const co_unsigned8_t SLAVE_DEV_ID = 0x02u;
  static const co_unsigned8_t OTHER_NODE_ID = 0x05u;
  static const co_unsigned16_t HB_TIMEOUT_MS = 550u;

  can_net_t* net = nullptr;
  co_dev_t* dev = nullptr;

  std::unique_ptr<CoDevTHolder> dev_holder;

  std::unique_ptr<CoObjTHolder> obj1000;
#if !LELY_NO_CO_DCF_RESTORE
  std::unique_ptr<CoObjTHolder> obj2001;
#endif

  std::unique_ptr<CoObjTHolder> obj1016;
  std::unique_ptr<CoObjTHolder> obj1017;
  std::unique_ptr<CoObjTHolder> obj1029;
  std::unique_ptr<CoObjTHolder> obj1f80;
  std::unique_ptr<CoObjTHolder> obj1f81;
  std::unique_ptr<CoObjTHolder> obj1f82;
  std::unique_ptr<CoObjTHolder> obj_rdn;

  Allocators::Default allocator;

  void CreateObj1016ConsumerHbTimeN(const co_unsigned8_t num) {
    assert(num > 0);
#if LELY_NO_MALLOC
    assert(num <= CO_NMT_MAX_NHB);
#endif

    dev_holder->CreateObj<Obj1016ConsumerHb>(obj1016);
    obj1016->EmplaceSub<Obj1016ConsumerHb::Sub00HighestSubidxSupported>(num);
    for (co_unsigned8_t i = 1u; i <= num; ++i) {
      obj1016->EmplaceSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(
          i,
          Obj1016ConsumerHb::MakeHbConsumerEntry(SLAVE_DEV_ID, 1u));  //  1 ms
    }
  }

  void CreateObj1017ProducerHeartbeatTime(const co_unsigned16_t hb_time) {
    dev_holder->CreateObjValue<Obj1017ProducerHb>(obj1017, hb_time);
  }

  void CreateObj1f80NmtStartup(const Obj1f80NmtStartup::sub_type startup) {
    dev_holder->CreateObjValue<Obj1f80NmtStartup>(obj1f80, startup);
  }

  void CreateObj1f81SlaveAssignmentN(const co_unsigned8_t num) {
    assert(num > 0 && num <= CO_NUM_NODES);
    // object 0x1f81 - Slave assignment object
    dev_holder->CreateAndInsertObj(obj1f81, 0x1f81u);

    // 0x00 - Highest sub-index supported
    obj1f81->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, num);
    // 0x01-0x7f - Slave with the given Node-ID
    for (co_unsigned8_t i = 0; i < num; ++i) {
      obj1f81->InsertAndSetSub(i + 1u, CO_DEFTYPE_UNSIGNED32,
                               co_unsigned32_t{0x01u});
    }
  }

  void CreateObj1f82RequestNmt(const co_unsigned8_t num) {
    assert(num > 0 && num <= CO_NUM_NODES);
    // object 0x1f82 - Request NMT object
    dev_holder->CreateAndInsertObj(obj1f82, 0x1f82u);

    // 0x00 - Highest sub-index supported
    obj1f82->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, num);
    // 0x01-0x7f - Request NMT-Service for slave with the given Node-ID
    for (co_unsigned8_t i = 0; i < num; ++i) {
      obj1f82->InsertAndSetSub(i + 1u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0});
    }
  }

#if !LELY_NO_CO_ECSS_REDUNDANCY
  void CreateEmptyRedundancyObject() {
    dev_holder->CreateAndInsertObj(obj_rdn, CO_NMT_RDN_REDUNDANCY_OBJ_IDX);
    obj_rdn->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0});
    obj_rdn->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0});
  }
#endif

  void CreateObj1029ErrorBehavior(const co_unsigned8_t eb) {
    dev_holder->CreateObj<Obj1029ErrorBehavior>(obj1029);
    obj1029->EmplaceSub<Obj1029ErrorBehavior::Sub00HighestSubidxSupported>();
    obj1029->EmplaceSub<Obj1029ErrorBehavior::Sub01CommError>(eb);
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(allocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);
  }

  TEST_TEARDOWN() {
    dev_holder.reset();
    can_net_destroy(net);
    set_errnum(ERRNUM_SUCCESS);
  }
};

TEST_GROUP_BASE(CO_NmtCreate, CO_NmtBase) {
  co_nmt_t* nmt = nullptr;

  void CheckNmtDefaults() const {
    POINTERS_EQUAL(net, co_nmt_get_net(nmt));
    POINTERS_EQUAL(dev, co_nmt_get_dev(nmt));

    void* pdata;
    co_nmt_cs_ind_t* cs_ind;
    co_nmt_get_cs_ind(nmt, &cs_ind, &pdata);
    FUNCTIONPOINTERS_EQUAL(nullptr, cs_ind);
    POINTERS_EQUAL(nullptr, pdata);

    co_nmt_hb_ind_t* hb_ind;
    co_nmt_get_hb_ind(nmt, &hb_ind, &pdata);
    CHECK(hb_ind != nullptr);
    POINTERS_EQUAL(nullptr, pdata);

    co_nmt_st_ind_t* st_ind;
    co_nmt_get_st_ind(nmt, &st_ind, &pdata);
    CHECK(st_ind != nullptr);
    POINTERS_EQUAL(nullptr, pdata);

#if !LELY_NO_CO_MASTER
    co_nmt_sdo_ind_t* dn_ind;
    co_nmt_get_dn_ind(nmt, &dn_ind, &pdata);
    FUNCTIONPOINTERS_EQUAL(nullptr, dn_ind);
    POINTERS_EQUAL(nullptr, pdata);

    co_nmt_sdo_ind_t* up_ind;
    co_nmt_get_up_ind(nmt, &up_ind, &pdata);
    FUNCTIONPOINTERS_EQUAL(nullptr, up_ind);
    POINTERS_EQUAL(nullptr, pdata);
#endif

    co_nmt_sync_ind_t* sync_ind;
    co_nmt_get_sync_ind(nmt, &sync_ind, &pdata);
    FUNCTIONPOINTERS_EQUAL(nullptr, sync_ind);
    POINTERS_EQUAL(nullptr, pdata);

    CHECK_EQUAL(DEV_ID, co_nmt_get_id(nmt));
    CHECK_EQUAL(CO_NMT_ST_BOOTUP, co_nmt_get_st(nmt));
    CHECK_FALSE(co_nmt_is_master(nmt));
#if !LELY_NO_CO_MASTER
#if !LELY_NO_CO_NMT_BOOT || !LELY_NO_CO_NMT_CFG
    CHECK_EQUAL(LELY_CO_NMT_TIMEOUT, co_nmt_get_timeout(nmt));
#else
    CHECK_EQUAL(0, co_nmt_get_timeout(nmt));
#endif
#endif
  }

  // co_dev_write_dcf(): every <> is a call to co_val_write() when writing DCFs
  static int32_t GetCoDevWriteDcf_NullBuf_CoValWriteCalls(
      const uint8_t num_subs) {
    // <total number of subs> + NUM_SUBS * <get sub's size>
    return 1 + static_cast<int32_t>(num_subs);
  }

  static int32_t GetCoDevWriteDcf_CoValWriteCalls(const uint8_t num_subs) {
    // <total number of subs> + NUM_SUBS * (<get sub's size> + <sub's index>
    //     + <sub's sub-index> + <sub's size> + <sub's value>)
    return 1 + (static_cast<int32_t>(num_subs) * 5);
  }

  TEST_TEARDOWN() {
    co_nmt_destroy(nmt);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_nmt_es2str()
///@{

/// \Given an NMT boot error status
///
/// \When co_nmt_es2str() is called with the status
///
/// \Then a pointer to an appropriate string describing the status is returned
TEST(CO_NmtCreate, CoNmtEs2Str_Nominal) {
  STRCMP_EQUAL("The CANopen device is not listed in object 1F81.",
               co_nmt_es2str('A'));
  STRCMP_EQUAL("No response received for upload request of object 1000.",
               co_nmt_es2str('B'));
  STRCMP_EQUAL(
      "Value of object 1000 from CANopen device is different to value in "
      "object 1F84 (Device type).",
      co_nmt_es2str('C'));
  STRCMP_EQUAL(
      "Value of object 1018 sub-index 01 from CANopen device is different to "
      "value in object 1F85 (Vendor-ID).",
      co_nmt_es2str('D'));
  STRCMP_EQUAL(
      "Heartbeat event. No heartbeat message received from CANopen device.",
      co_nmt_es2str('E'));
  STRCMP_EQUAL(
      "Node guarding event. No confirmation for guarding request received from "
      "CANopen device.",
      co_nmt_es2str('F'));
  STRCMP_EQUAL(
      "Objects for program download are not configured or inconsistent.",
      co_nmt_es2str('G'));
  STRCMP_EQUAL(
      "Software update is required, but not allowed because of configuration "
      "or current status.",
      co_nmt_es2str('H'));
  STRCMP_EQUAL("Software update is required, but program download failed.",
               co_nmt_es2str('I'));
  STRCMP_EQUAL("Configuration download failed.", co_nmt_es2str('J'));
  STRCMP_EQUAL(
      "Heartbeat event during start error control service. No heartbeat "
      "message received from CANopen device during start error control "
      "service.",
      co_nmt_es2str('K'));
  STRCMP_EQUAL("NMT slave was initially operational.", co_nmt_es2str('L'));
  STRCMP_EQUAL(
      "Value of object 1018 sub-index 02 from CANopen device is different to "
      "value in object 1F86 (Product code).",
      co_nmt_es2str('M'));
  STRCMP_EQUAL(
      "Value of object 1018 sub-index 03 from CANopen device is different to "
      "value in object 1F87 (Revision number).",
      co_nmt_es2str('N'));
  STRCMP_EQUAL(
      "Value of object 1018 sub-index 04 from CANopen device is different to "
      "value in object 1F88 (Serial number).",
      co_nmt_es2str('O'));
}

/// \Given an unknown NMT boot error status
///
/// \When co_nmt_es2str() is called with the status
///
/// \Then a pointer to "Unknown error status" string is returned
TEST(CO_NmtCreate, CoNmtEs2Str_Unknown) {
  STRCMP_EQUAL("Unknown error status", co_nmt_es2str('Z'));
}

///@}

/// @name co_nmt_sizeof()
///@{

/// \Given N/A
///
/// \When co_nmt_sizeof() is called
///
/// \Then the platform-dependent size of the NMT service object is returned
TEST(CO_NmtCreate, CoNmtSizeof_Nominal) {
  const auto ret = co_nmt_sizeof();

#if defined(__MINGW32__)

#if defined(__MINGW64__)
  CHECK_EQUAL(10728u, ret);
#else
  CHECK_EQUAL(6420u, ret);
#endif  // __MINGW64__

#elif LELY_NO_MALLOC  // !__MINGW32__

#if LELY_NO_CO_NG && LELY_NO_CO_NMT_BOOT && LELY_NO_CO_NMT_CFG  // ECSS

#if LELY_NO_CO_MASTER
  CHECK_EQUAL(1384u, ret);
#else
  CHECK_EQUAL(4792u, ret);
#endif  // LELY_NO_CO_MASTER

#else  // non-ECSS, LELY_NO_MALLOC
  CHECK_EQUAL(11872u, ret);
#endif

#elif LELY_NO_HOSTED  // !LELY_NO_MALLOC

  CHECK_EQUAL(11872u, ret);

#elif LELY_NO_CO_MASTER  // !LELY_NO_HOSTED

#if LELY_NO_MALLOC
  CHECK_EQUAL(400u, ret);
#else  // !LELY_NO_MALLOC

#if LELY_NO_CO_ECSS_REDUNDANCY
  CHECK_EQUAL(400u, ret);
#else
  CHECK_EQUAL(424u, ret);
#endif  // LELY_NO_CO_ECSS_REDUNDANCY

#endif  // LELY_NO_MALLOC

#else  // !LELY_NO_CO_MASTER

#if LELY_NO_CO_ECSS_REDUNDANCY
  CHECK_EQUAL(9712u, ret);
#else
  CHECK_EQUAL(9736u, ret);
#endif  // LELY_NO_CO_ECSS_REDUNDANCY

#endif
}

///@}

/// @name co_nmt_alignof()
///@{

/// \Given N/A
///
/// \When co_nmt_alignof() is called
///
/// \Then the platform-dependent alignment of the NMT service object is
///       returned
TEST(CO_NmtCreate, CoNmtAlignof_Nominal) {
  const auto ret = co_nmt_alignof();

#if defined(__MINGW32__) && !defined(__MINGW64__)
  CHECK_EQUAL(4u, ret);
#else
  CHECK_EQUAL(8u, ret);
#endif
}

///@}

/// @name co_nmt_create()
///@{

/// \Given initialized device (co_dev_t) and network (can_net_t)
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a pointer to a created NMT service is returned, the service is
///       configured with the default values
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_nmt_srv_init()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls co_dev_find_obj()
///       \IfCalls{LELY_NO_MALLOC, memset()}
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_create()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_buf_init()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_net_get_time()}
///       \IfCalls{LELY_NO_MALLOC, co_dev_get_val_u32()}
///       \IfCalls{!LELY_NO_CO_TPDO, co_dev_set_tpdo_event_ind()}
///       \Calls co_obj_set_dn_ind()
TEST(CO_NmtCreate, CoNmtCreate_Default) {
  nmt = co_nmt_create(net, dev);

  CHECK(nmt != nullptr);
  CheckNmtDefaults();
}

#if HAVE_LELY_OVERRIDE

#if !LELY_NO_CO_DCF_RESTORE
/// \Given initialized device (co_dev_t) and network (can_net_t), the object
///        dictionary contains a single entry in the application parameters
///        area (0x2000-0x9fff), a number of valid calls is limited to one call
///        to co_dev_write_dcf()
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, an NMT service is not created
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls mem_free()
///       \Calls get_errc()
///       \Calls set_errc()
TEST(CO_NmtCreate, CoNmtCreate_DcfAppParamsWriteFail) {
  const size_t NUM_SUBS = 1u;
  dev_holder->CreateAndInsertObj(obj2001, 0x2001u);
  obj2001->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0});

  LelyOverride::co_val_write(
      GetCoDevWriteDcf_NullBuf_CoValWriteCalls(NUM_SUBS));

  nmt = co_nmt_create(net, dev);

  POINTERS_EQUAL(nullptr, nmt);

  LelyOverride::co_val_write(Override::AllCallsValid);
}
#endif

/// \Given initialized device (co_dev_t) and network (can_net_t), the object
///        dictionary contains a single entry in the application parameters
///        area (0x2000-0x9fff) [if !LELY_NO_CO_DCF_RESTORE] and a single entry
///        in the communication parameters area (0x1000-0x1fff), a number of
///        valid calls is limited to three (or one [if
///        !LELY_NO_CO_DCF_RESTORE]) call to co_dev_write_dcf()
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, an NMT service is not created
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls mem_free()
///       \Calls get_errc()
///       \Calls set_errc()
TEST(CO_NmtCreate, CoNmtCreate_DcfCommParamsWriteFail) {
  const size_t NUM_SUBS = 1u;  // in each region
#if !LELY_NO_CO_DCF_RESTORE
  dev_holder->CreateAndInsertObj(obj2001, 0x2001u);
  obj2001->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0});
#endif
  dev_holder->CreateAndInsertObj(obj1000, 0x1000u);
  obj1000->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0});

#if !LELY_NO_CO_DCF_RESTORE
  LelyOverride::co_val_write(
      2 * GetCoDevWriteDcf_NullBuf_CoValWriteCalls(NUM_SUBS) +
      GetCoDevWriteDcf_CoValWriteCalls(NUM_SUBS));
#else
  LelyOverride::co_val_write(GetCoDevWrite_NullBuf_CoValWriteCalls(NUM_SUBS));
#endif

  nmt = co_nmt_create(net, dev);

  POINTERS_EQUAL(nullptr, nmt);

  LelyOverride::co_val_write(Override::AllCallsValid);
}

#endif  // HAVE_LELY_OVERRIDE

/// \Given initialized device (co_dev_t) and network (can_net_t), the object
///        dictionary contains the Consumer Heartbeat Time object (0x1016) with
///        less than maximum number of entries
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a pointer to a created NMT service is returned, the service is
///       configured with the default values and the indication function is
///       set for the Consumer Heartbeat Time sub-objects (0x1016)
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_nmt_srv_init()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls co_dev_find_obj()
///       \IfCalls{LELY_NO_MALLOC, memset()}
///       \IfCalls{LELY_NO_MALLOC, co_obj_find_sub()}
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_create()}
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_create()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_buf_init()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_net_get_time()}
///       \IfCalls{LELY_NO_MALLOC, co_dev_get_val_u32()}
///       \IfCalls{!LELY_NO_CO_TPDO, co_dev_set_tpdo_event_ind()}
///       \Calls co_obj_set_dn_ind()
TEST(CO_NmtCreate, CoNmtCreate_WithObj1016_LessThanMaxEntries) {
  CreateObj1016ConsumerHbTimeN(1u);

  nmt = co_nmt_create(net, dev);

  CHECK(nmt != nullptr);
  CheckNmtDefaults();
  LelyUnitTest::CheckSubDnIndIsSet(dev, 0x1016u, nmt);
}

/// \Given initialized device (co_dev_t) and network (can_net_t), the object
///        dictionary contains the Slave Assignment object (0x1f81) with at
///        least one slave in the network list
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a pointer to a created NMT service is returned, the service is
///       configured with the default values, [if LELY_NO_CO_MASTER
///       && !LELY_NO_MALLOC] the indication function is set for the Slave
///       Assignment sub-objects (0x1f81)
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_nmt_srv_init()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls co_dev_find_obj()
///       \IfCalls{LELY_NO_MALLOC, memset()}
///       \IfCalls{LELY_NO_MALLOC, co_obj_find_sub()}
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_create()}
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_create()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_buf_init()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_net_get_time()}
///       \IfCalls{LELY_NO_MALLOC, co_dev_get_val_u32()}
///       \IfCalls{!LELY_NO_CO_TPDO, co_dev_set_tpdo_event_ind()}
///       \Calls co_obj_set_dn_ind()
///       \IfCalls{LELY_NO_MALLOC && !LELY_NO_CO_NMT_BOOT, co_nmt_boot_create()}
///       \IfCalls{LELY_NO_MALLOC && !LELY_NO_CO_NMT_CFG, co_nmt_cfg_create()}
TEST(CO_NmtCreate, CoNmtCreate_WithObj1f81) {
  CreateObj1f81SlaveAssignmentN(1u);

  nmt = co_nmt_create(net, dev);

  CHECK(nmt != nullptr);
  CheckNmtDefaults();
#if !LELY_NO_CO_MASTER && !LELY_NO_MALLOC
  LelyUnitTest::CheckSubDnIndIsSet(dev, 0x1f81u, nmt);
#endif
}

/// \Given initialized device (co_dev_t) and network (can_net_t), the object
///        dictionary contains the Consumer Heartbeat Time (0x1016), the
///        Producer Heartbeat Time (0x1017), the NMT Start-up (0x1f80), the
///        Slave Assignment (0x1f81) and the Request NMT (0x1f82) objects
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a pointer to a created NMT service is returned, the service is
///       configured with the default values and indication functions are set
///       for all sub-objects
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_nmt_srv_init()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls co_dev_find_obj()
///       \IfCalls{LELY_NO_MALLOC, memset()}
///       \IfCalls{LELY_NO_MALLOC, co_obj_find_sub()}
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_create()}
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_create()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_buf_init()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_net_get_time()}
///       \IfCalls{LELY_NO_MALLOC, co_dev_get_val_u32()}
///       \IfCalls{!LELY_NO_CO_TPDO, co_dev_set_tpdo_event_ind()}
///       \Calls co_obj_set_dn_ind()
///       \IfCalls{LELY_NO_MALLOC && !LELY_NO_CO_NMT_BOOT, co_nmt_boot_create()}
///       \IfCalls{LELY_NO_MALLOC && !LELY_NO_CO_NMT_CFG, co_nmt_cfg_create()}
TEST(CO_NmtCreate, CoNmtCreate_ConfigurationObjectsInds) {
  CreateObj1016ConsumerHbTimeN(1u);
  CreateObj1017ProducerHeartbeatTime(0);
  CreateObj1f80NmtStartup(0);
  CreateObj1f81SlaveAssignmentN(1u);
  CreateObj1f82RequestNmt(1u);

  nmt = co_nmt_create(net, dev);

  CHECK(nmt != nullptr);
  CheckNmtDefaults();

  LelyUnitTest::CheckSubDnIndIsSet(dev, 0x1016u, nmt);
  LelyUnitTest::CheckSubDnIndIsSet(dev, 0x1017u, nmt);
  LelyUnitTest::CheckSubDnIndIsSet(dev, 0x1f80u, nmt);
#if !LELY_NO_CO_MASTER && !LELY_NO_MALLOC
  LelyUnitTest::CheckSubDnIndIsSet(dev, 0x1f81, nmt);
#else
  LelyUnitTest::CheckSubDnIndIsDefault(dev, 0x1f81);
#endif
#if !LELY_NO_CO_MASTER
  LelyUnitTest::CheckSubDnIndIsSet(dev, 0x1f82u, nmt);
#else
  LelyUnitTest::CheckSubDnIndIsDefault(dev, 0x1f82);
#endif
}

///@}

/// @name co_nmt_destroy()
///@{

/// \Given N/A
///
/// \When co_nmt_destroy() is called with a null NMT service pointer
///
/// \Then nothing is changed
TEST(CO_NmtCreate, CoNmtDestroy_Null) { co_nmt_destroy(nullptr); }

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_destroy() is called with a pointer to the service
///
/// \Then the service is finalized and freed
///       \Calls co_nmt_get_alloc()
///       \Calls co_dev_find_obj()
///       \IfCalls{!LELY_NO_CO_TPDO, co_dev_set_tpdo_event_ind()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_timer_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_buf_fini()}
///       \IfCalls {LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \Calls can_timer_stop()
///       \Calls can_timer_destroy()
///       \Calls can_recv_destroy()
///       \Calls co_nmt_srv_fini()
///       \Calls mem_free()
TEST(CO_NmtCreate, CoNmtDestroy_Nominal) {
  nmt = co_nmt_create(net, dev);
  CHECK(nmt != nullptr);

  co_nmt_destroy(nmt);

  nmt = nullptr;
}

/// \Given a pointer to an initialized NMT service (co_nmt_t) configured with
///        the Consumer Heartbeat Time (0x1016), the Producer Heartbeat Time
///        (0x1017), the NMT Start-up (0x1f80), the Slave Assignment (0x1f81)
///        and the Request NMT (0x1f82) objects in the object dictionary
///
/// \When co_nmt_destroy() is called with a pointer to the service
///
/// \Then the service is finalized and freed, indication functions are set
///       back to default for all sub-objects
///       \Calls co_nmt_get_alloc()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_set_dn_ind()
///       \IfCalls{!LELY_NO_CO_TPDO, co_dev_set_tpdo_event_ind()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_timer_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_buf_fini()}
///       \IfCalls {LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \Calls can_timer_stop()
///       \Calls can_timer_destroy()
///       \Calls can_recv_destroy()
///       \Calls co_nmt_srv_fini()
///       \Calls mem_free()
TEST(CO_NmtCreate, CoNmtDestroy_ConfigurationObjectsInd) {
  CreateObj1016ConsumerHbTimeN(1u);
  CreateObj1017ProducerHeartbeatTime(0);
  CreateObj1f80NmtStartup(0);
  CreateObj1f81SlaveAssignmentN(1u);
  CreateObj1f82RequestNmt(1u);

  nmt = co_nmt_create(net, dev);
  CHECK(nmt != nullptr);

  co_nmt_destroy(nmt);

  LelyUnitTest::CheckSubDnIndIsDefault(dev, 0x1016u);
  LelyUnitTest::CheckSubDnIndIsDefault(dev, 0x1017u);
  LelyUnitTest::CheckSubDnIndIsDefault(dev, 0x1f80u);
  LelyUnitTest::CheckSubDnIndIsDefault(dev, 0x1f81u);
  LelyUnitTest::CheckSubDnIndIsDefault(dev, 0x1f82u);

  nmt = nullptr;
}

///@}

TEST_GROUP_BASE(CO_NmtAllocation, CO_NmtBase) {
  Allocators::Limited limitedAllocator;
  co_nmt_t* nmt = nullptr;

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(limitedAllocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);
  }

  TEST_TEARDOWN() {
    co_nmt_destroy(nmt);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_nmt_create()
///@{

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to 0 bytes
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, NMT service is not created and the error
///       number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \Calls get_errc()
///       \Calls set_errc()
TEST(CO_NmtAllocation, CoNmtCreate_NoMemory) {
  limitedAllocator.LimitAllocationTo(0);

  nmt = co_nmt_create(net, dev);

  POINTERS_EQUAL(nullptr, nmt);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the NMT service instance
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, NMT service is not created and the error
///       number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls set_errc()
TEST(CO_NmtAllocation, CoNmtCreate_NoMemoryForDcfParams) {
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof());

  nmt = co_nmt_create(net, dev);

  POINTERS_EQUAL(nullptr, nmt);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

#if !LELY_NO_CO_DCF_RESTORE
/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the NMT service instance and the
///        concise DCF backup for application parameters
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, NMT service is not created and the error
///       number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls set_errc()
TEST(CO_NmtAllocation, CoNmtCreate_NoMemoryForDcfCommParams) {
  const size_t app_param_size =
      co_dev_write_dcf(dev, 0x2000, 0x9fff, nullptr, nullptr);

  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() + app_param_size);

  nmt = co_nmt_create(net, dev);

  POINTERS_EQUAL(nullptr, nmt);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}
#endif

#if LELY_NO_MALLOC && !LELY_NO_CO_SDO

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the NMT service instance, the
///        concise DCF backups for application parameters and communication
///        parameters
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, NMT service is not created and the error
///       number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_nmt_srv_init()
///       \Calls co_nmt_srv_fini()
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls set_errc()
TEST(CO_NmtAllocation, CoNmtCreate_NoMemoryForDefaultServices) {
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() +
                                     NmtCommon::GetDcfParamsAllocSize(dev));

  nmt = co_nmt_create(net, dev);

  POINTERS_EQUAL(nullptr, nmt);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

#endif  // LELY_NO_MALLOC && #if !LELY_NO_CO_SDO

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the NMT service instance, DCFs for
///        application/communication parameters and the default services
///        instances
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, NMT service is not created and the error
///       number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_nmt_srv_init()
///       \Calls co_nmt_srv_fini()
///       \Calls can_recv_create()
///       \Calls can_recv_destroy()
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls set_errc()
TEST(CO_NmtAllocation, CoNmtCreate_NoMemoryForNmtRecv) {
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() +
                                     NmtCommon::GetDcfParamsAllocSize(dev) +
                                     NmtCommon::GetServicesAllocSize());

  nmt = co_nmt_create(net, dev);

  POINTERS_EQUAL(nullptr, nmt);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the NMT service instance, DCFs for
///        application/communication parameters, the default services instances
///        and a receiver for NMT messages
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, NMT service is not created and the error
///       number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_nmt_srv_init()
///       \Calls can_recv_create()
///       \Calls can_recv_destroy()
///       \Calls can_recv_set_func()
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls set_errc()
TEST(CO_NmtAllocation, CoNmtCreate_NoMemoryForEcRecv) {
  limitedAllocator.LimitAllocationTo(
      co_nmt_sizeof() + NmtCommon::GetDcfParamsAllocSize(dev) +
      NmtCommon::GetServicesAllocSize() + can_recv_sizeof());

  nmt = co_nmt_create(net, dev);

  POINTERS_EQUAL(nullptr, nmt);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the NMT service instance, DCFs for
///        application/communication parameters, the default services instances,
///        a receiver for NMT messages and a receiver for NMT error control
///        messages
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, NMT service is not created and the error
///       number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_nmt_srv_init()
///       \Calls can_recv_create()
///       \Calls can_recv_destroy()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls set_errc()
TEST(CO_NmtAllocation, CoNmtCreate_NoMemoryForEcTimer) {
  limitedAllocator.LimitAllocationTo(
      co_nmt_sizeof() + NmtCommon::GetDcfParamsAllocSize(dev) +
      NmtCommon::GetServicesAllocSize() + NmtCommon::GetNmtRecvsAllocSize());

  nmt = co_nmt_create(net, dev);

  POINTERS_EQUAL(nullptr, nmt);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

#if !LELY_NO_CO_ECSS_REDUNDANCY && LELY_NO_MALLOC
/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the NMT service instance, DCFs
///        for application/communication parameters, the default services
///        instances, all NMT receivers and a timer for life guarding/heartbeat
///        production
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, NMT service is not created and the error
///       number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_nmt_srv_init()
///       \Calls can_recv_create()
///       \Calls can_recv_destroy()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls can_timer_destroy()
///       \Calls co_dev_find_obj()
///       \Calls co_nmt_rdn_create()
///       \IfCalls{LELY_NO_MALLOC, memset()}
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_create()}
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls set_errc()
TEST(CO_NmtAllocation, CoNmtCreate_NoMemoryForRedundancy) {
  limitedAllocator.LimitAllocationTo(
      co_nmt_sizeof() + NmtCommon::GetDcfParamsAllocSize(dev) +
      NmtCommon::GetServicesAllocSize() + NmtCommon::GetNmtRecvsAllocSize() +
      can_timer_sizeof());

  nmt = co_nmt_create(net, dev);

  POINTERS_EQUAL(nullptr, nmt);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}
#endif

#if !LELY_NO_CO_MASTER
/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the NMT service instance, DCFs
///        for application/communication parameters, the default services
///        instances, all NMT receivers, a timer for life guarding/heartbeat
///        production and the NMT redundancy manager service instance
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, NMT service is not created and the error
///       number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_nmt_srv_init()
///       \Calls can_recv_create()
///       \Calls can_recv_destroy()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls can_timer_destroy()
///       \Calls co_dev_find_obj()
///       \Calls co_nmt_rdn_create()
///       \IfCalls{LELY_NO_MALLOC, memset()}
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_create()}
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls set_errc()
TEST(CO_NmtAllocation, CoNmtCreate_NoMemoryForCsTimer) {
  limitedAllocator.LimitAllocationTo(
      co_nmt_sizeof() + NmtCommon::GetDcfParamsAllocSize(dev) +
      NmtCommon::GetServicesAllocSize() + NmtCommon::GetNmtRecvsAllocSize() +
      NmtCommon::GetNmtRedundancyAllocSize() + can_timer_sizeof());

  nmt = co_nmt_create(net, dev);

  POINTERS_EQUAL(nullptr, nmt);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the NMT service instance, DCFs for
///        application/communication parameters, the default services instances,
///        all NMT receivers and a timer for life guarding/heartbeat production;
///        the object dictionary contains the Consumer Heartbeat Time object
///        (0x1016) with at least one entry
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, NMT service is not created and the error
///       number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_nmt_srv_init()
///       \Calls can_recv_create()
///       \Calls can_recv_destroy()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls can_timer_destroy()
///       \Calls co_dev_find_obj()
///       \IfCalls{LELY_NO_MALLOC, memset()}
///       \IfCalls{LELY_NO_MALLOC, co_dev_find_sub()}
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_create()}
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls set_errc()
TEST(CO_NmtAllocation, CoNmtCreate_NoMemoryForHbSrv_WithObj1016) {
  CreateObj1016ConsumerHbTimeN(1u);

  limitedAllocator.LimitAllocationTo(
      co_nmt_sizeof() + NmtCommon::GetDcfParamsAllocSize(dev) +
      NmtCommon::GetServicesAllocSize() + NmtCommon::GetNmtRecvsAllocSize() +
      can_timer_sizeof());

  nmt = co_nmt_create(net, dev);

  POINTERS_EQUAL(nullptr, nmt);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the NMT service instance, DCFs for
///        application/communication parameters, the default services instances,
///        all NMT receivers, a timer for life guarding/heartbeat production,
///        the NMT redundancy manager service instance and a timer for sending
///        buffered NMT messages
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, NMT service is not created and the error
///       number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_nmt_srv_init()
///       \Calls can_recv_create()
///       \Calls can_recv_destroy()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls can_timer_destroy()
///       \Calls co_dev_find_obj()
///       \IfCalls{LELY_NO_MALLOC, memset()}
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_create()}
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls set_errc()
TEST(CO_NmtAllocation, CoNmtCreate_NoMemoryForNmtSlaveRecvs) {
  limitedAllocator.LimitAllocationTo(
      co_nmt_sizeof() + NmtCommon::GetDcfParamsAllocSize(dev) +
      NmtCommon::GetServicesAllocSize() + NmtCommon::GetNmtRecvsAllocSize() +
      NmtCommon::GetNmtRedundancyAllocSize() +
      NmtCommon::GetNmtTimersAllocSize());

  nmt = co_nmt_create(net, dev);

  POINTERS_EQUAL(nullptr, nmt);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

#endif  // !LELY_NO_MASTER

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to exactly allocate the NMT service and all
///        required objects
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a pointer to a created NMT service is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_nmt_srv_init()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls co_dev_find_obj()
///       \IfCalls{LELY_NO_MALLOC, memset()}
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_create()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_buf_init()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_net_get_time()}
///       \IfCalls{LELY_NO_MALLOC, co_dev_get_val_u32()}
///       \IfCalls{!LELY_NO_CO_TPDO, co_dev_set_tpdo_event_ind()}
///       \Calls co_obj_set_dn_ind()
///       \IfCalls{LELY_NO_MALLOC && !LELY_NO_CO_NMT_BOOT, co_nmt_boot_create()}
///       \IfCalls{LELY_NO_MALLOC && !LELY_NO_CO_NMT_CFG, co_nmt_cfg_create()}
TEST(CO_NmtAllocation, CoNmtCreate_ExactMemory) {
  limitedAllocator.LimitAllocationTo(
      co_nmt_sizeof() + NmtCommon::GetDcfParamsAllocSize(dev) +
      NmtCommon::GetServicesAllocSize() + NmtCommon::GetNmtRecvsAllocSize() +
      NmtCommon::GetNmtRedundancyAllocSize() +
      NmtCommon::GetNmtTimersAllocSize() + NmtCommon::GetSlavesAllocSize());

  nmt = co_nmt_create(net, dev);

  CHECK(nmt != nullptr);
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

#if LELY_NO_MALLOC
/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to exactly allocate the NMT service and all
///        required objects; the object dictionary contains the Consumer
///        Heartbeat Time object (0x1016) with the maximum number of entries
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a pointer to a created NMT service is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_chk_dev()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_nmt_srv_init()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls co_dev_find_obj()
///       \IfCalls{LELY_NO_MALLOC, memset()}
///       \IfCalls{!LELY_NO_CO_ECSS_REDUNDANCY, co_nmt_rdn_create()}
///       \IfCalls{LELY_NO_MALLOC, co_obj_find_sub()}
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_create()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_buf_init()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_net_get_time()}
///       \IfCalls{LELY_NO_MALLOC, co_dev_get_val_u32()}
///       \IfCalls{!LELY_NO_CO_TPDO, co_dev_set_tpdo_event_ind()}
///       \Calls co_obj_set_dn_ind()
///       \IfCalls{LELY_NO_MALLOC && !LELY_NO_CO_NMT_BOOT, co_nmt_boot_create()}
///       \IfCalls{LELY_NO_MALLOC && !LELY_NO_CO_NMT_CFG, co_nmt_cfg_create()}
TEST(CO_NmtAllocation, CoNmtCreate_ExactMemory_WithObj1016_MaxEntries) {
  CreateObj1016ConsumerHbTimeN(CO_NMT_MAX_NHB);

  limitedAllocator.LimitAllocationTo(
      co_nmt_sizeof() + NmtCommon::GetDcfParamsAllocSize(dev) +
      NmtCommon::GetServicesAllocSize() + NmtCommon::GetNmtRecvsAllocSize() +
      NmtCommon::GetNmtRedundancyAllocSize() +
      NmtCommon::GetNmtTimersAllocSize() + NmtCommon::GetSlavesAllocSize() +
      NmtCommon::GetHbConsumersAllocSize(CO_NMT_MAX_NHB));

  nmt = co_nmt_create(net, dev);

  CHECK(nmt != nullptr);
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}
#endif

///@}

TEST_GROUP_BASE(CO_Nmt, CO_NmtBase) {
  static constexpr co_unsigned8_t NMT_EC_MSG_SIZE = 1u;
  static constexpr co_unsigned8_t NMT_CS_MSG_SIZE = 2u;

  co_nmt_t* nmt = nullptr;

  std::unique_ptr<CoObjTHolder> obj1001;
  std::unique_ptr<CoObjTHolder> obj1003;
  std::unique_ptr<CoObjTHolder> obj102a;
  std::unique_ptr<CoObjTHolder> obj1fff;
  std::unique_ptr<CoObjTHolder> obj2020;

  static void empty_cs_ind(co_nmt_t*, co_unsigned8_t, void*) {}
  static void empty_hb_ind(co_nmt_t*, co_unsigned8_t, co_nmt_ec_state_t,
                           co_nmt_ec_reason_t, void*) {}
  static void empty_st_ind(co_nmt_t*, co_unsigned8_t, co_unsigned8_t, void*) {}
  static void empty_sdo_ind(co_nmt_t*, co_unsigned8_t, co_unsigned16_t,
                            co_unsigned8_t, size_t, size_t, void*) {}
  static void empty_sync_ind(co_nmt_t*, co_unsigned8_t, void*) {}
  int32_t data = 0;

  const co_unsigned8_t invalidNmtCs = co_unsigned8_t{0xffu};

  void CreateNmt() {
    nmt = co_nmt_create(net, dev);
    CHECK(nmt != nullptr);
  }

  void CreateNmtAndReset() {
    CreateNmt();
    CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));

    CanSend::Clear();
    CoNmtCsInd::Clear();
    CoNmtStInd::Clear();

    co_nmt_set_cs_ind(nmt, &CoNmtCsInd::Func, nullptr);
    co_nmt_set_st_ind(nmt, &CoNmtStInd::Func, nullptr);
  }

  void CreateNmtAndStop() {
    CreateNmt();
    CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));
    CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_STOP));
  }

  void CreateUnconfNmtAndReset() {
    CreateNmt();
    CHECK_EQUAL(0, co_nmt_set_id(nmt, 0xffu));
    CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));
  }

  void AdvanceTimeMs(const uint32_t ms) {
    timespec ts = {0, 0};
    can_net_get_time(net, &ts);
    timespec_add_msec(&ts, ms);
    can_net_set_time(net, &ts);
  }

  void SetNmtCsStIndFunc(const CoNmtCsInd::Seq& csSeq = {},
                         const CoNmtStInd::Seq& stSeq = {}) {
    CoNmtCsInd::Clear();
    co_nmt_set_cs_ind(nmt, &CoNmtCsInd::Func, nullptr);
    if (csSeq.size() != 0) CoNmtCsInd::SetCheckSeq(nmt, csSeq);

    CoNmtStInd::Clear();
    co_nmt_set_st_ind(nmt, &CoNmtStInd::Func, nullptr);
    if (stSeq.size() != 0) CoNmtStInd::SetCheckSeq(nmt, DEV_ID, stSeq);
  }

  void CreateObj102aNmtInhibitTime(const co_unsigned16_t inhibit_time) {
    dev_holder->CreateAndInsertObj(obj102a, 0x102au);
    obj102a->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED16,
                             co_unsigned16_t{inhibit_time});
  }

  can_msg CreateNmtEcMsg(const co_unsigned8_t id, const co_unsigned8_t st)
      const {
    can_msg msg = CAN_MSG_INIT;
    msg.id = CO_NMT_EC_CANID(id);
    msg.len = NMT_EC_MSG_SIZE;
    msg.data[0] = st;

    return msg;
  }

  can_msg CreateNmtMsg(const co_unsigned8_t id, const co_unsigned8_t cs) const {
    can_msg msg = CAN_MSG_INIT;
    msg.id = CO_NMT_CS_CANID;
    msg.len = NMT_CS_MSG_SIZE;
    msg.data[0] = cs;
    msg.data[1] = id;

    return msg;
  }

  void SetupMaster(const co_unsigned32_t startup =
                       Obj1f80NmtStartup::MASTER_BIT) {
    dev_holder->CreateObjValue<Obj1f80NmtStartup>(obj1f80, startup);
  }

  void SetupMasterWithSlave(const co_unsigned32_t startup =
                                Obj1f80NmtStartup::MASTER_BIT) {
    SetupMaster(startup);

    dev_holder->CreateObj<Obj1f81NmtSlaveAssignment>(obj1f81);
    obj1f81->EmplaceSub<Obj1f81NmtSlaveAssignment::Sub00HighestSubidxSupported>(
        SLAVE_DEV_ID);
    obj1f81->EmplaceSub<Obj1f81NmtSlaveAssignment::SubNthSlaveEntry>(
        MASTER_DEV_ID, Obj1f81NmtSlaveAssignment::ASSIGNMENT_BIT |
                           Obj1f81NmtSlaveAssignment::MANDATORY_BIT);
    obj1f81->EmplaceSub<Obj1f81NmtSlaveAssignment::SubNthSlaveEntry>(
        SLAVE_DEV_ID, Obj1f81NmtSlaveAssignment::ASSIGNMENT_BIT |
                          Obj1f81NmtSlaveAssignment::MANDATORY_BIT);
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    can_net_set_send_func(net, CanSend::Func, nullptr);
  }

  TEST_TEARDOWN() {
    CoNmtCsInd::Clear();
    CoNmtStInd::Clear();
    CanSend::Clear();
    co_nmt_destroy(nmt);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_nmt_get_alloc()
///@{

/// \Given a pointer to an NMT service (co_nmt_t) created on a network with
///        an allocator
///
/// \When co_nmt_get_alloc() is called
///
/// \Then a pointer to the allocator (alloc_t) is returned
///       \Calls can_net_get_alloc()
TEST(CO_Nmt, CoNmtGetAlloc_Nominal) {
  CreateNmt();

  POINTERS_EQUAL(allocator.ToAllocT(), co_nmt_get_alloc(nmt));
}

///@}

/// @name co_nmt_get_net()
///@{

/// \Given a pointer to an NMT service (co_nmt_t) created on a network
///
/// \When co_nmt_get_net() is called
///
/// \Then a pointer to the network (can_net_t) is returned
TEST(CO_Nmt, CoNmtGetNet_Nominal) {
  CreateNmt();

  POINTERS_EQUAL(net, co_nmt_get_net(nmt));
}

///@}

/// @name co_nmt_get_dev()
///@{

/// \Given a pointer to an NMT service (co_nmt_t) created on a device
///
/// \When co_nmt_get_dev() is called
///
/// \Then a pointer to the device (co_dev_t) is returned
TEST(CO_Nmt, CoNmtGetDev_Nominal) {
  CreateNmt();

  POINTERS_EQUAL(dev, co_nmt_get_dev(nmt));
}

///@}

/// @name co_nmt_get_cs_ind()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_get_cs_ind() is called with no addresses to store the
///       indication function and user-specified data pointers at
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtGetCsInd_Null) {
  CreateNmt();

  co_nmt_get_cs_ind(nmt, nullptr, nullptr);
}

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_get_cs_ind() is called with an address to store the indication
///       function pointer and an address to store user-specified data pointer
///
/// \Then both pointers are set to a null pointer
TEST(CO_Nmt, CoNmtGetCsInd_Nominal) {
  CreateNmt();

  co_nmt_cs_ind_t* ind = &empty_cs_ind;
  void* cs_data = &data;

  co_nmt_get_cs_ind(nmt, &ind, &cs_data);

  FUNCTIONPOINTERS_EQUAL(nullptr, ind);
  POINTERS_EQUAL(nullptr, cs_data);
}

///@}

/// @name co_nmt_set_cs_ind()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_set_cs_ind() is called with a pointer to an indication
///       function and a pointer to user-specified data
///
/// \Then the indication function and the user-specified data pointers are set
///       in the NMT service
TEST(CO_Nmt, CoNmtSetCsInd_Nominal) {
  CreateNmt();

  co_nmt_set_cs_ind(nmt, &empty_cs_ind, &data);

  co_nmt_cs_ind_t* ind = nullptr;
  void* cs_data = nullptr;
  co_nmt_get_cs_ind(nmt, &ind, &cs_data);
  FUNCTIONPOINTERS_EQUAL(&empty_cs_ind, ind);
  POINTERS_EQUAL(&data, cs_data);
}

///@}

/// @name co_nmt_get_hb_ind()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_get_hb_ind() is called with no addresses to store the
///       indication function and user-specified data pointers at
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtGetHbInd_Null) {
  CreateNmt();

  co_nmt_get_hb_ind(nmt, nullptr, nullptr);
}

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_get_hb_ind() is called with an address to store the indication
///       function pointer and an address to store user-specified data pointer
///
/// \Then the indication function pointer is set to a non-null pointer and
///       the user-specified data pointer is set to a null pointer
TEST(CO_Nmt, CoNmtGetHbInd_Nominal) {
  CreateNmt();

  co_nmt_hb_ind_t* ind = nullptr;
  void* hb_data = &data;

  co_nmt_get_hb_ind(nmt, &ind, &hb_data);

  CHECK(ind != nullptr);
  POINTERS_EQUAL(nullptr, hb_data);
}

///@}

/// @name co_nmt_set_hb_ind()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_set_hb_ind() is called with a pointer to an indication
///       function and a pointer to user-specified data
///
/// \Then the indication function and the user-specified data pointers are set
///       in the NMT service
TEST(CO_Nmt, CoNmtSetHbInd_Nominal) {
  CreateNmt();

  co_nmt_set_hb_ind(nmt, &empty_hb_ind, &data);

  co_nmt_hb_ind_t* ind = nullptr;
  void* hb_data = nullptr;
  co_nmt_get_hb_ind(nmt, &ind, &hb_data);
  FUNCTIONPOINTERS_EQUAL(&empty_hb_ind, ind);
  POINTERS_EQUAL(&data, hb_data);
}

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_set_hb_ind() is called with a null indication function pointer
///       and a null user-specified data pointer
///
/// \Then the indication function pointer is set to a non-null pointer and
///       the user-specified data pointer is set to a null pointer
TEST(CO_Nmt, CoNmtSetHbInd_Null) {
  CreateNmt();
  co_nmt_set_hb_ind(nmt, &empty_hb_ind, &data);

  co_nmt_set_hb_ind(nmt, nullptr, nullptr);

  co_nmt_hb_ind_t* ind = nullptr;
  void* hb_data = nullptr;
  co_nmt_get_hb_ind(nmt, &ind, &hb_data);
  CHECK(ind != nullptr);
  CHECK(ind != &empty_hb_ind);
  POINTERS_EQUAL(nullptr, hb_data);
}

///@}

/// @name co_nmt_get_st_ind()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_get_st_ind() is called with no addresses to store the
///       indication function and user-specified data pointers at
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtGetStInd_Null) {
  CreateNmt();

  co_nmt_get_st_ind(nmt, nullptr, nullptr);
}

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_get_st_ind() is called with an address to store the indication
///       function pointer and an address to store user-specified data pointer
///
/// \Then the indication function pointer is set to a non-null pointer and
///       the user-specified data pointer is set to a null pointer
TEST(CO_Nmt, CoNmtGetStInd_Nominal) {
  CreateNmt();

  co_nmt_st_ind_t* ind = nullptr;
  void* st_data = &data;

  co_nmt_get_st_ind(nmt, &ind, &st_data);

  CHECK(ind != nullptr);
  POINTERS_EQUAL(nullptr, st_data);
}

///@}

/// @name co_nmt_set_st_ind()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_set_st_ind() is called with a pointer to an indication
///       function and a pointer to user-specified data
///
/// \Then the indication function and the user-specified data pointers are set
///       in the NMT service
TEST(CO_Nmt, CoNmtSetStInd_Nominal) {
  CreateNmt();

  co_nmt_set_st_ind(nmt, &empty_st_ind, &data);

  co_nmt_st_ind_t* ind = nullptr;
  void* st_data = nullptr;
  co_nmt_get_st_ind(nmt, &ind, &st_data);
  FUNCTIONPOINTERS_EQUAL(&empty_st_ind, ind);
  POINTERS_EQUAL(&data, st_data);
}

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_set_st_ind() is called with a null indication function pointer
///       and a null user-specified data pointer
///
/// \Then the indication function pointer is set to a non-null pointer and
///       the user-specified data pointer is set to a null pointer
TEST(CO_Nmt, CoNmtSetStInd_Null) {
  CreateNmt();
  co_nmt_set_st_ind(nmt, &empty_st_ind, &data);

  co_nmt_set_st_ind(nmt, nullptr, nullptr);

  co_nmt_st_ind_t* ind = nullptr;
  void* st_data = nullptr;
  co_nmt_get_st_ind(nmt, &ind, &st_data);
  CHECK(ind != nullptr);
  POINTERS_EQUAL(nullptr, st_data);
}

///@}

#if !LELY_NO_CO_MASTER

/// @name co_nmt_get_dn_ind()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_get_dn_ind() is called with no addresses to store the
///       indication function and user-specified data pointers at
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtGetDnInd_Null) {
  CreateNmt();

  co_nmt_get_dn_ind(nmt, nullptr, nullptr);
}

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_get_dn_ind() is called with an address to store the indication
///       function pointer and an address to store user-specified data pointer
///
/// \Then both pointers are set to a null pointer
TEST(CO_Nmt, CoNmtGetDnInd_Nominal) {
  CreateNmt();

  co_nmt_sdo_ind_t* ind = &empty_sdo_ind;
  void* dn_data = &data;

  co_nmt_get_dn_ind(nmt, &ind, &dn_data);

  FUNCTIONPOINTERS_EQUAL(nullptr, ind);
  POINTERS_EQUAL(nullptr, dn_data);
}

///@}

/// @name co_nmt_set_dn_ind()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_set_dn_ind() is called with a pointer to an indication
///       function and a pointer to user-specified data
///
/// \Then the indication function and the user-specified data pointers are set
///       in the NMT service
TEST(CO_Nmt, CoNmtSetDnInd_Nominal) {
  CreateNmt();

  co_nmt_set_dn_ind(nmt, &empty_sdo_ind, &data);

  co_nmt_sdo_ind_t* ind = nullptr;
  void* dn_data = nullptr;
  co_nmt_get_dn_ind(nmt, &ind, &dn_data);
  FUNCTIONPOINTERS_EQUAL(&empty_sdo_ind, ind);
  POINTERS_EQUAL(&data, dn_data);
}

///@}

/// @name co_nmt_get_up_ind()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_get_up_ind() is called with no addresses to store the
///       indication function and user-specified data pointers at
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtGetUpInd_Null) {
  CreateNmt();

  co_nmt_get_up_ind(nmt, nullptr, nullptr);
}

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_get_up_ind() is called with an address to store the indication
///       function pointer and an address to store user-specified data pointer
///
/// \Then both pointers are set to a null pointer
TEST(CO_Nmt, CoNmtGetUpInd_Nominal) {
  CreateNmt();

  co_nmt_sdo_ind_t* ind = &empty_sdo_ind;
  void* up_data = &data;

  co_nmt_get_up_ind(nmt, &ind, &up_data);

  FUNCTIONPOINTERS_EQUAL(nullptr, ind);
  POINTERS_EQUAL(nullptr, up_data);
}

///@}

/// @name co_nmt_set_up_ind()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_set_up_ind() is called with a pointer to an indication
///       function and a pointer to user-specified data
///
/// \Then the indication function and the user-specified data pointers are set
///       in the NMT service
TEST(CO_Nmt, CoNmtSetUpInd_Nominal) {
  CreateNmt();

  co_nmt_set_up_ind(nmt, &empty_sdo_ind, &data);

  co_nmt_sdo_ind_t* ind = nullptr;
  void* up_data = nullptr;
  co_nmt_get_up_ind(nmt, &ind, &up_data);
  FUNCTIONPOINTERS_EQUAL(&empty_sdo_ind, ind);
  POINTERS_EQUAL(&data, up_data);
}

///@}

#endif  // !LELY_NO_CO_MASTER

/// @name co_nmt_get_sync_ind()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_get_sync_ind() is called with no addresses to store the
///       indication function and user-specified data pointers at
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtGetSyncInd_Null) {
  CreateNmt();

  co_nmt_get_sync_ind(nmt, nullptr, nullptr);
}

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_get_sync_ind() is called with an address to store the
///       indication function pointer and an address to store user-specified
///       data pointer
///
/// \Then both pointers are set to a null pointer
TEST(CO_Nmt, CoNmtGetSyncInd_Nominal) {
  CreateNmt();

  co_nmt_sync_ind_t* ind = &empty_sync_ind;
  void* sync_data = &data;

  co_nmt_get_sync_ind(nmt, &ind, &sync_data);

  FUNCTIONPOINTERS_EQUAL(nullptr, ind);
  POINTERS_EQUAL(nullptr, sync_data);
}

///@}

/// @name co_nmt_set_sync_ind()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_set_sync_ind() is called with a pointer to an indication
///       function and a pointer to user-specified data
///
/// \Then the indication function and the user-specified data pointers are set
///       in the NMT service
TEST(CO_Nmt, CoNmtSetSyncInd_Nominal) {
  CreateNmt();

  co_nmt_set_sync_ind(nmt, &empty_sync_ind, &data);

  co_nmt_sync_ind_t* ind = nullptr;
  void* sync_data = nullptr;
  co_nmt_get_sync_ind(nmt, &ind, &sync_data);
  FUNCTIONPOINTERS_EQUAL(&empty_sync_ind, ind);
  POINTERS_EQUAL(&data, sync_data);
}

///@}

/// @name co_nmt_get_id()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_get_id() is called
///
/// \Then the pending Node-ID is returned
TEST(CO_Nmt, CoNmtGetId_Nominal) {
  CreateNmt();

  const auto ret = co_nmt_get_id(nmt);

  CHECK_EQUAL(DEV_ID, ret);
}

///@}

/// @name co_nmt_set_id()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_set_id() is called with a Node-ID equal to `0`
///
/// \Then -1 is returned, the error number it set to ERRNUM_INVAL
TEST(CO_Nmt, CoNmtSetId_ZeroId) {
  CreateNmt();

  const auto ret = co_nmt_set_id(nmt, 0);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(DEV_ID, co_nmt_get_id(nmt));
}

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_set_id() is called with a Node-ID over the maximum value
///
/// \Then -1 is returned, the error number it set to ERRNUM_INVAL
TEST(CO_Nmt, CoNmtSetId_OverMax) {
  CreateNmt();

  const auto ret = co_nmt_set_id(nmt, CO_NUM_NODES + 1u);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(DEV_ID, co_nmt_get_id(nmt));
}

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_set_id() is called with the unconfigured Node-ID (`255`)
///
/// \Then 0 is returned, the pending Node-ID is set to the unconfigured Node-ID
TEST(CO_Nmt, CoNmtSetId_Unconfigured) {
  const co_unsigned8_t unconfNodeId = 255u;
  CreateNmt();

  const auto ret = co_nmt_set_id(nmt, unconfNodeId);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(unconfNodeId, co_nmt_get_id(nmt));
}

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_set_id() is called with a Node-ID
///
/// \Then 0 is returned, the pending Node-ID is set
TEST(CO_Nmt, CoNmtSetId_Nominal) {
  const co_unsigned8_t nodeId = 0x05;
  CreateNmt();

  const auto ret = co_nmt_set_id(nmt, nodeId);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(nodeId, co_nmt_get_id(nmt));
}

///@}

/// @name co_nmt_get_st()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_get_st() is called
///
/// \Then the current state of the NMT service is returned
TEST(CO_Nmt, CoNmtGetSt_Nominal) {
  CreateNmt();

  const auto ret = co_nmt_get_st(nmt);

  CHECK_EQUAL(CO_NMT_ST_BOOTUP, ret);
}

///@}

/// @name co_nmt_is_master()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t) before the
///        initial reset
///
/// \When co_nmt_is_master() is called
///
/// \Then 0 is returned
TEST(CO_Nmt, CoNmtIsMaster_BeforeInitialReset) {
  CreateNmt();

  const auto ret = co_nmt_is_master(nmt);

  CHECK_FALSE(ret);
}

/// \Given a pointer to an initialized NMT service (co_nmt_t) configured as
///        NMT slave
///
/// \When co_nmt_is_master() is called
///
/// \Then 0 is returned
TEST(CO_Nmt, CoNmtIsMaster_Slave) {
  CreateObj1f80NmtStartup(0x00);
  CreateNmt();
  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));

  const auto ret = co_nmt_is_master(nmt);

  CHECK_FALSE(ret);
}

#if !LELY_NO_CO_MASTER
/// \Given a pointer to an initialized NMT service (co_nmt_t) configured as
///        NMT master
///
/// \When co_nmt_is_master() is called
///
/// \Then 1 is returned
TEST(CO_Nmt, CoNmtIsMaster_Master) {
  CreateObj1f80NmtStartup(0x01u);
  CreateNmt();
  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));

  const auto ret = co_nmt_is_master(nmt);

  CHECK_EQUAL(1, ret);
}
#endif

///@}

#if !LELY_NO_CO_MASTER

/// @name co_nmt_get_timeout()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_get_timeout() is called
///
/// \Then the default SDO timeout is returned
TEST(CO_Nmt, CoNmtGetTimeout_Nominal) {
  CreateNmt();

  const auto ret = co_nmt_get_timeout(nmt);

#if !LELY_NO_CO_NMT_BOOT || !LELY_NO_CO_NMT_CFG
  CHECK_EQUAL(LELY_CO_NMT_TIMEOUT, ret);
#else
  CHECK_EQUAL(0, ret);
#endif
}

///@}

/// @name co_nmt_set_timeout()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_set_timeout() is called with a timeout value
///
/// \Then the default SDO timeout is set
TEST(CO_Nmt, CoNmtSetTimeout_Nominal) {
  const int_least32_t timeout = 500;
  CreateNmt();

  co_nmt_set_timeout(nmt, timeout);

  CHECK_EQUAL(timeout, co_nmt_get_timeout(nmt));
}

///@}

#endif  // !LELY_NO_CO_MASTER

/// @name co_nmt_on_st()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_on_st() is called with a Node-ID equal to `0` and any NMT
///       state
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtOnSt_ZeroId) {
  CreateNmt();

  co_nmt_on_st(nmt, 0, CO_NMT_ST_BOOTUP);
}

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_on_st() is called with a Node-ID over the maximum value and
///       any NMT state
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtOnSt_OverMax) {
  CreateNmt();

  co_nmt_on_st(nmt, CO_NUM_NODES + 1u, CO_NMT_ST_BOOTUP);
}

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_on_st() is called with a Node-ID and any NMT state
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtOnSt_Nominal) {
  CreateNmt();

  co_nmt_on_st(nmt, DEV_ID, CO_NMT_ST_BOOTUP);
}

///@}

#if !LELY_NO_CO_MASTER

/// @name co_nmt_cs_req()
///@{

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as
///        NMT slave
///
/// \When co_nmt_cs_req() is called with any NMT command specifier and
///       any Node-ID
///
/// \Then -1 is returned, the error number it set to ERRNUM_PERM, the request
///       is not sent
///       \Calls set_errnum()
TEST(CO_Nmt, CoNmtCsReq_Slave) {
  CreateNmtAndReset();

  const auto ret = co_nmt_cs_req(nmt, CO_NMT_CS_START, 0);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_PERM, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as
///        NMT master
///
/// \When co_nmt_cs_req() is called with an invalid NMT command specifier and
///       a Node-ID
///
/// \Then -1 is returned, the error number it set to ERRNUM_INVAL, the request
///       is not sent
///       \Calls set_errnum()
TEST(CO_Nmt, CoNmtCsReq_InvalidCs) {
  SetupMaster();
  CreateNmtAndReset();

  const auto ret = co_nmt_cs_req(nmt, invalidNmtCs, SLAVE_DEV_ID);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as
///        NMT master
///
/// \When co_nmt_cs_req() is called with an NMT command specifier and a Node-ID
///       over the maximum value
///
/// \Then -1 is returned, the error number it set to ERRNUM_INVAL, the request
///       is not sent
///       \Calls set_errnum()
TEST(CO_Nmt, CoNmtCsReq_NodeIdOverMax) {
  SetupMaster();
  CreateNmtAndReset();

  const auto ret = co_nmt_cs_req(nmt, CO_NMT_CS_START, CO_NUM_NODES + 1u);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as
///        NMT master
///
/// \When co_nmt_cs_req() is called with an NMT command specifier and master's
///       Node-ID
///
/// \Then 0 is returned, the local request is issued and master transitions to
///       the state defined by the command
///       \Calls co_dev_get_id()
///       \Calls co_nmt_cs_ind()
TEST(CO_Nmt, CoNmtCsReq_MasterId) {
  SetupMaster();
  CreateNmtAndReset();

  const auto ret = co_nmt_cs_req(nmt, CO_NMT_CS_START, MASTER_DEV_ID);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

#if LELY_NO_MALLOC
/// \Given a pointer to a booted NMT service (co_nmt_t) configured as
///        NMT master, the NMT service's CAN frame buffer is full
///
/// \When co_nmt_cs_req() is called with an NMT command specifier and
///       a Node-ID
///
/// \Then -1 is returned, the error number is set to ERRNUM_NOMEM, the request
///       is not sent
///       \Calls co_dev_get_id()
///       \Calls can_buf_write()
TEST(CO_Nmt, CoNmtCsReq_FrameBufferOverflow) {
  CreateObj102aNmtInhibitTime(1u);  // 100 usec
  SetupMaster();
  CreateNmtAndReset();

  size_t msg_counter = 1u;  // initial boot-up message
  while (msg_counter < CO_NMT_CAN_BUF_SIZE) {
    CHECK_EQUAL(0, co_nmt_cs_req(nmt, CO_NMT_CS_ENTER_PREOP, SLAVE_DEV_ID));
    ++msg_counter;
  }
  CanSend::Clear();

  const auto ret = co_nmt_cs_req(nmt, CO_NMT_CS_START, SLAVE_DEV_ID);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}
#endif

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as
///        NMT master
///
/// \When co_nmt_cs_req() is called with an NMT command specifier and
///       a Node-ID, but sending the CAN message failed
///
/// \Then -1 is returned, the request is not sent; the request is resent on the
///       next call to co_nmt_cs_req() along with the next request
TEST(CO_Nmt, CoNmtCsReq_SendFail) {
  SetupMaster();
  CreateNmtAndReset();
  CanSend::ret = -1;

  CHECK_EQUAL(-1, co_nmt_cs_req(nmt, CO_NMT_CS_START, SLAVE_DEV_ID));

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CanSend::CheckMsg(CreateNmtMsg(SLAVE_DEV_ID, CO_NMT_CS_START));
  CanSend::Clear();

  const CanSend::MsgSeq msgs = {
      CreateNmtMsg(SLAVE_DEV_ID, CO_NMT_CS_START),
      CreateNmtMsg(SLAVE_DEV_ID, CO_NMT_CS_STOP),
  };
  CanSend::SetCheckSeq(msgs);

  CHECK_EQUAL(0, co_nmt_cs_req(nmt, CO_NMT_CS_STOP, SLAVE_DEV_ID));
  CHECK_EQUAL(2u, CanSend::GetNumCalled());
}

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as
///        NMT master, the object dictionary contains the NMT Inhibit Time
///        object (0x102a) with non-zero value
///
/// \When co_nmt_cs_req() is called (at least once) with an NMT command
///       specifier and a Node-ID
///
/// \Then 0 is returned, the request(s) are not sent immediately; each request
///       is sent after the inhibit time has passed
TEST(CO_Nmt, CoNmtCsReq_DelayedMessages) {
  const co_unsigned16_t inhibit_time_ms = 10u;
  CreateObj102aNmtInhibitTime(inhibit_time_ms * 10u);  // 10 ms
  SetupMaster();
  CreateNmtAndReset();

  CHECK_EQUAL(0, co_nmt_cs_req(nmt, CO_NMT_CS_START, SLAVE_DEV_ID));
  CHECK_EQUAL(0, co_nmt_cs_req(nmt, CO_NMT_CS_STOP, SLAVE_DEV_ID));
  CHECK_EQUAL(0, CanSend::GetNumCalled());

  AdvanceTimeMs(inhibit_time_ms);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CanSend::CheckMsg(CreateNmtMsg(SLAVE_DEV_ID, CO_NMT_CS_START));

  AdvanceTimeMs(inhibit_time_ms);

  CHECK_EQUAL(2u, CanSend::GetNumCalled());
  CanSend::CheckMsg(CreateNmtMsg(SLAVE_DEV_ID, CO_NMT_CS_STOP));
}

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as
///        NMT master
///
/// \When co_nmt_cs_req() is called with an NMT command specifier and
///       a Node-ID
///
/// \Then 0 is returned, the request is sent
///       \Calls co_dev_get_id()
///       \Calls can_buf_write()
///       \Calls co_dev_get_val_u16()
///       \Calls can_timer_stop()
///       \Calls can_buf_peek()
///       \Calls can_net_send()
///       \Calls can_buf_read()
///       \Calls can_net_get_time()
///       \Calls timespec_add_usec()
TEST(CO_Nmt, CoNmtCsReq_SingleNode) {
  SetupMaster();
  CreateNmtAndReset();

  const auto ret = co_nmt_cs_req(nmt, CO_NMT_CS_START, SLAVE_DEV_ID);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CanSend::CheckMsg(CreateNmtMsg(SLAVE_DEV_ID, CO_NMT_CS_START));
}

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as
///        NMT master with a slave node that has booted
///
/// \When co_nmt_cs_req() is called with an NMT command specifier and
///       a Node-ID
///
/// \Then 0 is returned, the request is sent
///       \Calls co_dev_get_id()
///       \Calls can_buf_write()
///       \Calls co_dev_get_val_u16()
///       \Calls can_timer_stop()
///       \Calls can_buf_peek()
///       \Calls can_net_send()
///       \Calls can_buf_read()
///       \Calls can_net_get_time()
///       \Calls timespec_add_usec()
TEST(CO_Nmt, CoNmtCsReq_SingleNode_Booted) {
  SetupMasterWithSlave();
  CreateNmtAndReset();

  const can_msg bootup = CreateNmtEcMsg(SLAVE_DEV_ID, CO_NMT_ST_BOOTUP);
  CHECK_EQUAL(1, can_net_recv(net, &bootup, 0));

  const auto ret = co_nmt_cs_req(nmt, CO_NMT_CS_START, SLAVE_DEV_ID);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CanSend::CheckMsg(CreateNmtMsg(SLAVE_DEV_ID, CO_NMT_CS_START));
}

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as
///        NMT master
///
/// \When co_nmt_cs_req() is called with an NMT command specifier and
///       a Node-ID equal to `0` (all nodes)
///
/// \Then 0 is returned, the request is sent
///       \Calls co_dev_get_id()
///       \Calls can_buf_write()
///       \Calls co_dev_get_val_u16()
///       \Calls can_timer_stop()
///       \Calls can_buf_peek()
///       \Calls can_net_send()
///       \Calls can_buf_read()
///       \Calls can_net_get_time()
///       \Calls timespec_add_usec()
TEST(CO_Nmt, CoNmtCsReq_AllNodes) {
  SetupMaster();
  CreateNmtAndReset();

  const auto ret = co_nmt_cs_req(nmt, CO_NMT_CS_START, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CanSend::CheckMsg(CreateNmtMsg(0, CO_NMT_CS_START));
}

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as
///        NMT master with at least one node that has booted
///
/// \When co_nmt_cs_req() is called with an NMT command specifier and
///       a Node-ID equal to `0` (all nodes)
///
/// \Then 0 is returned, the request is sent
///       \Calls co_dev_get_id()
///       \Calls can_buf_write()
///       \Calls co_dev_get_val_u16()
///       \Calls can_timer_stop()
///       \Calls can_buf_peek()
///       \Calls can_net_send()
///       \Calls can_buf_read()
///       \Calls can_net_get_time()
///       \Calls timespec_add_usec()
TEST(CO_Nmt, CoNmtCsReq_AllNodes_Booted) {
  SetupMasterWithSlave();
  CreateNmtAndReset();

  const can_msg bootup = CreateNmtEcMsg(SLAVE_DEV_ID, CO_NMT_ST_BOOTUP);
  CHECK_EQUAL(1, can_net_recv(net, &bootup, 0));

  const auto ret = co_nmt_cs_req(nmt, CO_NMT_CS_START, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CanSend::CheckMsg(CreateNmtMsg(0, CO_NMT_CS_START));
}

///@}

#endif  // !LELY_NO_CO_MASTER

#if !LELY_NO_CO_MASTER

/// @name co_nmt_chk_bootup()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t) configured as
///        NMT slave
///
/// \When co_nmt_chk_bootup() is called with any Node-ID
///
/// \Then -1 is returned, the error number it set to ERRNUM_PERM
///       \Calls set_errnum()
TEST(CO_Nmt, CoNmtChkBootup_Slave) {
  CreateNmt();
  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));

  const auto ret = co_nmt_chk_bootup(nmt, 0);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_PERM, get_errnum());
}

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as NMT
///        master
///
/// \When co_nmt_chk_bootup() is called with a Node-ID over the maximum value
///
/// \Then -1 is returned, the error number it set to ERRNUM_INVAL
///       \Calls set_errnum()
TEST(CO_Nmt, CoNmtChkBootup_NodeIdOverMax) {
  CreateObj1f80NmtStartup(0x01u);
  CreateNmt();
  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));

  const auto ret = co_nmt_chk_bootup(nmt, CO_NUM_NODES + 1u);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as NMT
///        master
///
/// \When co_nmt_chk_bootup() is called with master's Node-ID
///
/// \Then 1 is returned
///       \Calls co_dev_get_id()
TEST(CO_Nmt, CoNmtChkBootup_MasterId_Booted) {
  CreateObj1f80NmtStartup(0x01u);
  CreateNmt();
  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));

  const auto ret = co_nmt_chk_bootup(nmt, MASTER_DEV_ID);

  CHECK_EQUAL(1u, ret);
}

/// \Given a pointer to an initialized NMT service (co_nmt_t) configured as NMT
///        master, the NMT service hasn't finished the boot-up procedure
///
/// \When co_nmt_chk_bootup() is called with master's Node-ID
///
/// \Then 0 is returned
///       \Calls co_dev_get_id()
TEST(CO_Nmt, CoNmtChkBootup_MasterId_BeforeBoot) {
  co_nmt_st_ind_t* const st_ind = [](co_nmt_t* nmt, co_unsigned8_t id,
                                     co_unsigned8_t st, void*) {
    static size_t bootup_cnt = 0;

    CHECK_EQUAL(MASTER_DEV_ID, id);

    if (st == CO_NMT_ST_BOOTUP) {
      // on first state change node is not yet configured as master
      if (bootup_cnt == 1u) {
        const auto ret = co_nmt_chk_bootup(nmt, id);
        CHECK_EQUAL(0, ret);
      }
      ++bootup_cnt;
    }
  };

  CreateObj1f80NmtStartup(0x01u);
  CreateNmt();
  co_nmt_set_st_ind(nmt, st_ind, nullptr);
  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));
}

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as NMT
///        master without any slaves
///
/// \When co_nmt_chk_bootup() is called with Node-ID equal to `0`
///
/// \Then 1 is returned
///       \Calls co_dev_get_id()
TEST(CO_Nmt, CoNmtChkBootup_ZeroId_NoSlaves) {
  CreateObj1f80NmtStartup(0x01u);
  CreateNmt();
  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));

  const auto ret = co_nmt_chk_bootup(nmt, 0);

  CHECK_EQUAL(1, ret);
}

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as NMT
///        master with a non-mandatory slave
///
/// \When co_nmt_chk_bootup() is called with Node-ID equal to `0`
///
/// \Then 1 is returned
///       \Calls co_dev_get_id()
TEST(CO_Nmt, CoNmtChkBootup_ZeroId_NonMandatorySlave) {
  CreateObj1f80NmtStartup(0x01u);
  CreateObj1f81SlaveAssignmentN(2u);
  CreateNmt();
  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));

  const auto ret = co_nmt_chk_bootup(nmt, 0);

  CHECK_EQUAL(1, ret);
}

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as NMT
///        master with at least one mandatory slave that hasn't booted
///
/// \When co_nmt_chk_bootup() is called with Node-ID equal to `0`
///
/// \Then 0 is returned
///       \Calls co_dev_get_id()
TEST(CO_Nmt, CoNmtChkBootup_ZeroId_NotBootedMandatorySlave) {
  CreateObj1f80NmtStartup(0x01u);
  CreateObj1f81SlaveAssignmentN(2u);
  co_dev_set_val_u32(dev, 0x1f81, 0x02, 0x09);
  CreateNmt();
  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));

  const auto ret = co_nmt_chk_bootup(nmt, 0);

  CHECK_EQUAL(0, ret);
}

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as NMT
///        master with a mandatory slave that has booted
///
/// \When co_nmt_chk_bootup() is called with Node-ID equal to `0`
///
/// \Then 1 is returned
///       \Calls co_dev_get_id()
TEST(CO_Nmt, CoNmtChkBootup_ZeroId_MandatorySlaveBooted) {
  CreateObj1f80NmtStartup(0x01u);
  CreateObj1f81SlaveAssignmentN(2u);
  co_dev_set_val_u32(dev, 0x1f81, 0x02, 0x09);
  CreateNmt();

  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));
  const can_msg msg = CreateNmtEcMsg(SLAVE_DEV_ID, CO_NMT_ST_BOOTUP);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  const auto ret = co_nmt_chk_bootup(nmt, 0);

  CHECK_EQUAL(1, ret);
}

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as NMT
///        master with a slave that hasn't booted
///
/// \When co_nmt_chk_bootup() is called with the slave's Node-ID
///
/// \Then 0 is returned
///       \Calls co_dev_get_id()
TEST(CO_Nmt, CoNmtChkBootup_SlaveId_NotBooted) {
  CreateObj1f80NmtStartup(0x01u);
  CreateNmt();
  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));

  const auto ret = co_nmt_chk_bootup(nmt, SLAVE_DEV_ID);

  CHECK_EQUAL(0, ret);
}

/// \Given a pointer to a booted NMT service (co_nmt_t) configured as NMT
///        master with a slave that has booted
///
/// \When co_nmt_chk_bootup() is called with the slave's Node-ID
///
/// \Then 1 is returned
///       \Calls co_dev_get_id()
TEST(CO_Nmt, CoNmtChkBootup_SlaveId_Booted) {
  CreateObj1f80NmtStartup(0x01u);
  CreateObj1f81SlaveAssignmentN(2u);
  co_dev_set_val_u32(dev, 0x1f81, 0x02, 0x09);
  CreateNmt();

  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));
  const can_msg msg = CreateNmtEcMsg(SLAVE_DEV_ID, CO_NMT_ST_BOOTUP);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  const auto ret = co_nmt_chk_bootup(nmt, SLAVE_DEV_ID);

  CHECK_EQUAL(1, ret);
}

///@}

#endif  // !LELY_NO_CO_MASTER

/// @name co_nmt_cs_ind()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with an invalid NMT command specifier
///
/// \Then -1 is returned, the error number is set to ERRNUM_INVAL, nothing
///       is changed
///       \Calls set_errnum()
TEST(CO_Nmt, CoNmtCsInd_InvalidCs) {
  CreateNmt();
  SetNmtCsStIndFunc();

  const auto ret = co_nmt_cs_ind(nmt, invalidNmtCs);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(0, CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(CO_NMT_ST_BOOTUP, co_nmt_get_st(nmt));
}

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with any NMT command specifier other than
///       'reset node'
///
/// \Then 0 is returned, nothing is changed
TEST(CO_Nmt, CoNmtCsInd_Init_BeforeReset) {
  CreateNmt();
  SetNmtCsStIndFunc();

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_START);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(CO_NMT_ST_BOOTUP, co_nmt_get_st(nmt));
}

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset node' command specifier
///
/// \Then 0 is returned, the service is started
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_set_1016()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \IfCalls{!LELY_NO_MALLOC, free()}
///       \IfCalls{!LELY_NO_CO_NG, can_recv_stop()}
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, co_dev_read_dcf()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, get_errc()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, diag()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_read_dcf()
///       \Calls get_errc()
///       \Calls co_dev_get_val_u32()
///       \Calls co_nmt_is_master()
///       \Calls can_recv_start()
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_LSS, co_nmt_get_lss()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u16()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u8()}
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \IfCalls{LELY_NO_MALLOC, set_errnum()}
///       \IfCalls{!LELY_NO_MALLOC, calloc()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_create()}
///       \Calls co_obj_get_val_u32()
///       \Calls co_nmt_hb_set_1016()
///       \Calls can_net_send()
TEST(CO_Nmt, CoNmtCsInd_Init_ResetNode) {
  const CoNmtCsInd::Seq csSeq = {CO_NMT_CS_RESET_NODE, CO_NMT_CS_RESET_COMM,
                                 CO_NMT_CS_ENTER_PREOP, CO_NMT_CS_START};
  const CoNmtStInd::Seq stSeq = {CO_NMT_ST_BOOTUP, CO_NMT_ST_BOOTUP,
                                 CO_NMT_ST_PREOP, CO_NMT_ST_START};
  CreateNmt();
  SetNmtCsStIndFunc(csSeq, stSeq);

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(csSeq.size(), CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(stSeq.size(), CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
}

/// \Given a pointer to a started NMT service (co_nmt_t), the object dictionary
///        contains objects in the application parameters area (0x2000-0x9fff)
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset node' command specifier
///
/// \Then 0 is returned, the service is restarted, the application parameters
///       are restored from the concise DCF backup
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_set_1016()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \IfCalls{!LELY_NO_MALLOC, free()}
///       \IfCalls{!LELY_NO_CO_NG, can_recv_stop()}
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, co_dev_read_dcf()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, get_errc()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, diag()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_read_dcf()
///       \Calls get_errc()
///       \Calls co_dev_get_val_u32()
///       \Calls co_nmt_is_master()
///       \Calls can_recv_start()
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_LSS, co_nmt_get_lss()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u16()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u8()}
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \IfCalls{LELY_NO_MALLOC, set_errnum()}
///       \IfCalls{!LELY_NO_MALLOC, calloc()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_create()}
///       \Calls co_obj_get_val_u32()
///       \Calls co_nmt_hb_set_1016()
///       \Calls can_net_send()
TEST(CO_Nmt, CoNmtCsInd_ResetNode_RestoreAppDcf) {
  const CoNmtCsInd::Seq csSeq = {CO_NMT_CS_RESET_NODE, CO_NMT_CS_RESET_COMM,
                                 CO_NMT_CS_ENTER_PREOP, CO_NMT_CS_START};
  const CoNmtStInd::Seq stSeq = {CO_NMT_ST_BOOTUP, CO_NMT_ST_BOOTUP,
                                 CO_NMT_ST_PREOP, CO_NMT_ST_START};

  dev_holder->CreateAndInsertObj(obj2020, 0x2020u);
  obj2020->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED16,
                           co_unsigned16_t{0x1234u});
  CreateNmtAndReset();
  SetNmtCsStIndFunc(csSeq, stSeq);

  CHECK_EQUAL(sizeof(co_unsigned16_t),
              co_dev_set_val_u16(dev, 0x2020u, 0x00u, 0));

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(csSeq.size(), CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(stSeq.size(), CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
  CHECK_EQUAL(0x1234u, co_dev_get_val_u16(dev, 0x2020u, 0x00u));
}

#if HAVE_LELY_OVERRIDE
/// \Given a pointer to a started NMT service (co_nmt_t), the object dictionary
///        contains objects in the application parameters area (0x2000-0x9fff)
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset node' command specifier,
///       but the service fails to restore the application parameters from the
///       concise DCF backup
///
/// \Then 0 is returned, the service is restarted, the application parameters
///       are not restored
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_set_1016()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \IfCalls{!LELY_NO_MALLOC, free()}
///       \IfCalls{!LELY_NO_CO_NG, can_recv_stop()}
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, co_dev_read_dcf()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, get_errc()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, diag()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_read_dcf()
///       \Calls get_errc()
///       \Calls co_dev_get_val_u32()
///       \Calls co_nmt_is_master()
///       \Calls can_recv_start()
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_LSS, co_nmt_get_lss()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u16()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u8()}
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \IfCalls{LELY_NO_MALLOC, set_errnum()}
///       \IfCalls{!LELY_NO_MALLOC, calloc()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_create()}
///       \Calls co_obj_get_val_u32()
///       \Calls co_nmt_hb_set_1016()
///       \Calls can_net_send()
TEST(CO_Nmt, CoNmtCsInd_ResetNode_FailedToRestoreAppDcf) {
  const CoNmtCsInd::Seq csSeq = {CO_NMT_CS_RESET_NODE, CO_NMT_CS_RESET_COMM,
                                 CO_NMT_CS_ENTER_PREOP, CO_NMT_CS_START};
  const CoNmtStInd::Seq stSeq = {CO_NMT_ST_BOOTUP, CO_NMT_ST_BOOTUP,
                                 CO_NMT_ST_PREOP, CO_NMT_ST_START};

  dev_holder->CreateAndInsertObj(obj2020, 0x2020u);
  obj2020->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED16,
                           co_unsigned16_t{0x1234u});
  CreateNmtAndReset();
  SetNmtCsStIndFunc(csSeq, stSeq);

  CHECK_EQUAL(sizeof(co_unsigned16_t),
              co_dev_set_val_u16(dev, 0x2020u, 0x00u, 0));

  LelyOverride::co_val_read(Override::NoneCallsValid);
  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE);
  LelyOverride::co_val_read(Override::AllCallsValid);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(csSeq.size(), CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(stSeq.size(), CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
  CHECK_EQUAL(0, co_dev_get_val_u16(dev, 0x2020u, 0x00u));
}
#endif

/// \Given a pointer to an initialized NMT service (co_nmt_t) with
///        an unconfigured Node-ID
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset node' command specifier
///
/// \Then 0 is returned, the service resets the node and transitions to the
///       NMT 'reset communication' state
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_set_1016()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \IfCalls{!LELY_NO_MALLOC, free()}
///       \IfCalls{!LELY_NO_CO_NG, can_recv_stop()}
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, co_dev_read_dcf()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, get_errc()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, diag()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_read_dcf()
///       \Calls get_errc()
///       \Calls co_dev_set_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_dev_get_val_u32()
///       \Calls co_nmt_is_master()
///       \Calls can_recv_start()
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_LSS, co_nmt_get_lss()}
TEST(CO_Nmt, CoNmtCsInd_Init_ResetNode_UnconfiguredId) {
  const CoNmtCsInd::Seq csSeq = {CO_NMT_CS_RESET_NODE, CO_NMT_CS_RESET_COMM};

  CreateNmt();
  SetNmtCsStIndFunc(csSeq);
  CHECK_EQUAL(0, co_nmt_set_id(nmt, 0xffu));

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(csSeq.size(), CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, DEV_ID, CO_NMT_ST_BOOTUP, nullptr);
  CHECK_EQUAL(CO_NMT_ST_RESET_COMM, co_nmt_get_st(nmt));
}

/// \Given a pointer to a partially booted NMT service (co_nmt_t) in the NMT
///        'reset communication' sub-state with an unconfigured Node-ID
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset node' command specifier
///       after setting a proper Node-ID
///
/// \Then 0 is returned, the service resets the node and transitions to the
///       NMT 'start' state
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_set_1016()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \IfCalls{!LELY_NO_MALLOC, free()}
///       \IfCalls{!LELY_NO_CO_NG, can_recv_stop()}
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, co_dev_read_dcf()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, get_errc()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, diag()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_read_dcf()
///       \Calls get_errc()
///       \Calls co_dev_set_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_dev_get_val_u32()
///       \Calls co_nmt_is_master()
///       \Calls can_recv_start()
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_LSS, co_nmt_get_lss()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u16()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u8()}
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \IfCalls{LELY_NO_MALLOC, set_errnum()}
///       \IfCalls{!LELY_NO_MALLOC, calloc()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_create()}
///       \Calls co_obj_get_val_u32()
///       \Calls co_nmt_hb_set_1016()
///       \Calls can_net_send()
TEST(CO_Nmt, CoNmtCsInd_BootupResetComm_ResetNode) {
  const CoNmtCsInd::Seq csSeq = {CO_NMT_CS_RESET_NODE, CO_NMT_CS_RESET_COMM,
                                 CO_NMT_CS_ENTER_PREOP, CO_NMT_CS_START};
  const CoNmtStInd::Seq stSeq = {CO_NMT_ST_BOOTUP, CO_NMT_ST_PREOP,
                                 CO_NMT_ST_START};

  CreateUnconfNmtAndReset();
  SetNmtCsStIndFunc(csSeq, stSeq);

  CHECK_EQUAL(0, co_nmt_set_id(nmt, DEV_ID));

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(csSeq.size(), CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(stSeq.size(), CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
}

/// \Given a pointer to a partially booted NMT service (co_nmt_t) in the NMT
///        'reset communication' sub-state
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset communication' command
///       specifier
///
/// \Then 0 is returned, the service resets the communication and transitions
///       to the NMT 'start' state
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_set_1016()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \IfCalls{!LELY_NO_MALLOC, free()}
///       \IfCalls{!LELY_NO_CO_NG, can_recv_stop()}
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \Calls co_dev_get_id()
///       \Calls co_dev_read_dcf()
///       \Calls get_errc()
///       \Calls co_dev_set_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_dev_get_val_u32()
///       \Calls co_nmt_is_master()
///       \Calls can_recv_start()
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_LSS, co_nmt_get_lss()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u16()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u8()}
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \IfCalls{LELY_NO_MALLOC, set_errnum()}
///       \IfCalls{!LELY_NO_MALLOC, calloc()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_create()}
///       \Calls co_obj_get_val_u32()
///       \Calls co_nmt_hb_set_1016()
///       \Calls can_net_send()
TEST(CO_Nmt, CoNmtCsInd_BootupResetComm_ResetComm) {
  const CoNmtCsInd::Seq csSeq = {CO_NMT_CS_RESET_COMM, CO_NMT_CS_ENTER_PREOP,
                                 CO_NMT_CS_START};
  const CoNmtStInd::Seq stSeq = {CO_NMT_ST_BOOTUP, CO_NMT_ST_PREOP,
                                 CO_NMT_ST_START};

  CreateUnconfNmtAndReset();
  SetNmtCsStIndFunc(csSeq, stSeq);

  CHECK_EQUAL(0, co_nmt_set_id(nmt, DEV_ID));

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_COMM);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(csSeq.size(), CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(stSeq.size(), CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
}

/// \Given a pointer to a partially booted NMT service (co_nmt_t) in the NMT
///        'reset communication' sub-state
///
/// \When co_nmt_cs_ind() is called with any NMT command specifier other than
///       'reset node' or 'reset communication'
///
/// \Then 0 is returned, nothing is changed
TEST(CO_Nmt, CoNmtCsInd_BootupResetComm_NoReset) {
  CreateUnconfNmtAndReset();
  SetNmtCsStIndFunc();

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_START);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(CO_NMT_ST_RESET_COMM, co_nmt_get_st(nmt));
}

/// \Given a pointer to a booted NMT service (co_nmt_t) in the NMT
///        'pre-operational' state
///
/// \When co_nmt_cs_ind() is called with the NMT 'start' command specifier
///
/// \Then 0 is returned, the service transitions to the NMT 'start' state
///       \Calls diag()
///       \Calls co_nmt_srv_set()
///       \Calls co_dev_get_id()
TEST(CO_Nmt, CoNmtCsInd_PreOperational_Start) {
  CreateObj1f80NmtStartup(0x04);  // do not start automatically
  CreateNmtAndReset();
  SetNmtCsStIndFunc();

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_START);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoNmtCsInd::GetNumCalled());
  CoNmtCsInd::Check(nmt, CO_NMT_CS_START, nullptr);
  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, DEV_ID, CO_NMT_ST_START, nullptr);
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
}

/// \Given a pointer to a booted NMT service (co_nmt_t) in the NMT
///        'pre-operational' state
///
/// \When co_nmt_cs_ind() is called with the NMT 'stop' command specifier
///
/// \Then 0 is returned, the service transitions to the NMT 'stop' state
///       \Calls co_nmt_srv_set()
///       \Calls co_dev_get_id()
TEST(CO_Nmt, CoNmtCsInd_PreOperational_Stop) {
  CreateObj1f80NmtStartup(0x04);  // do not start automatically
  CreateNmtAndReset();
  SetNmtCsStIndFunc();

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_STOP);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoNmtCsInd::GetNumCalled());
  CoNmtCsInd::Check(nmt, CO_NMT_CS_STOP, nullptr);
  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, DEV_ID, CO_NMT_ST_STOP, nullptr);
  CHECK_EQUAL(CO_NMT_ST_STOP, co_nmt_get_st(nmt));
}

/// \Given a pointer to a booted NMT service (co_nmt_t) in the NMT
///        'pre-operational' state
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset node' command specifier
///
/// \Then 0 is returned, the service resets the node and transitions back to
///       the NMT 'pre-operational' state
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_set_1016()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \IfCalls{!LELY_NO_MALLOC, free()}
///       \IfCalls{!LELY_NO_CO_NG, can_recv_stop()}
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, co_dev_read_dcf()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, get_errc()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, diag()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_read_dcf()
///       \Calls get_errc()
///       \Calls co_dev_set_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_dev_get_val_u32()
///       \Calls co_nmt_is_master()
///       \Calls can_recv_start()
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_LSS, co_nmt_get_lss()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u16()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u8()}
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \IfCalls{LELY_NO_MALLOC, set_errnum()}
///       \IfCalls{!LELY_NO_MALLOC, calloc()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_create()}
///       \Calls co_obj_get_val_u32()
///       \Calls co_nmt_hb_set_1016()
///       \Calls can_net_send()
TEST(CO_Nmt, CoNmtCsInd_PreOperational_ResetNode) {
  const CoNmtCsInd::Seq csSeq = {CO_NMT_CS_RESET_NODE, CO_NMT_CS_RESET_COMM,
                                 CO_NMT_CS_ENTER_PREOP};
  const CoNmtStInd::Seq stSeq = {CO_NMT_ST_BOOTUP, CO_NMT_ST_BOOTUP,
                                 CO_NMT_ST_PREOP};

  CreateObj1f80NmtStartup(0x04);  // do not start automatically
  CreateNmtAndReset();
  SetNmtCsStIndFunc(csSeq, stSeq);

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(csSeq.size(), CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(stSeq.size(), CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(CO_NMT_ST_PREOP, co_nmt_get_st(nmt));
}

/// \Given a pointer to a booted NMT service (co_nmt_t) in the NMT
///        'pre-operational' state
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset communication' command
///       specifier
///
/// \Then 0 is returned, the service resets the communication and transitions
///       back to the NMT 'pre-operational' state
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_set_1016()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \IfCalls{!LELY_NO_MALLOC, free()}
///       \IfCalls{!LELY_NO_CO_NG, can_recv_stop()}
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \Calls co_dev_get_id()
///       \Calls co_dev_read_dcf()
///       \Calls get_errc()
///       \Calls co_dev_set_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_dev_get_val_u32()
///       \Calls co_nmt_is_master()
///       \Calls can_recv_start()
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_LSS, co_nmt_get_lss()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u16()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u8()}
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \IfCalls{LELY_NO_MALLOC, set_errnum()}
///       \IfCalls{!LELY_NO_MALLOC, calloc()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_create()}
///       \Calls co_obj_get_val_u32()
///       \Calls co_nmt_hb_set_1016()
///       \Calls can_net_send()
TEST(CO_Nmt, CoNmtCsInd_PreOperational_ResetComm) {
  const CoNmtCsInd::Seq csSeq = {CO_NMT_CS_RESET_COMM, CO_NMT_CS_ENTER_PREOP};
  const CoNmtStInd::Seq stSeq = {CO_NMT_ST_BOOTUP, CO_NMT_ST_PREOP};

  CreateObj1f80NmtStartup(0x04);  // do not start automatically
  CreateNmtAndReset();
  SetNmtCsStIndFunc(csSeq, stSeq);

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_COMM);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(csSeq.size(), CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(stSeq.size(), CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(CO_NMT_ST_PREOP, co_nmt_get_st(nmt));
}

/// \Given a pointer to a booted NMT service (co_nmt_t) in the NMT
///        'pre-operational' state
///
/// \When co_nmt_cs_ind() is called with the NMT 'enter pre-operational'
///       command specifier
///
/// \Then 0 is returned, nothing is changed
TEST(CO_Nmt, CoNmtCsInd_PreOperational_EnterPreOperational) {
  CreateObj1f80NmtStartup(0x04);  // do not start automatically
  CreateNmtAndReset();
  SetNmtCsStIndFunc();

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_ENTER_PREOP);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(CO_NMT_ST_PREOP, co_nmt_get_st(nmt));
}

/// \Given a pointer to a started NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with the NMT 'enter pre-operational'
///       command specifier
///
/// \Then 0 is returned, the service transitions to the NMT 'pre-operational'
///       state
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \Calls co_dev_get_id()
TEST(CO_Nmt, CoNmtCsInd_Start_EnterPreOperational) {
  CreateObj1f80NmtStartup(0x04);  // do not start automatically
  CreateNmtAndReset();
  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_START));
  CoNmtCsInd::Clear();
  CoNmtStInd::Clear();
  SetNmtCsStIndFunc();

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_ENTER_PREOP);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoNmtCsInd::GetNumCalled());
  CoNmtCsInd::Check(nmt, CO_NMT_CS_ENTER_PREOP, nullptr);
  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, DEV_ID, CO_NMT_ST_PREOP, nullptr);
  CHECK_EQUAL(CO_NMT_ST_PREOP, co_nmt_get_st(nmt));
}

/// \Given a pointer to a started NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with the NMT 'stop' command specifier
///
/// \Then 0 is returned, the service transitions to the NMT 'stop' state
///       \Calls co_nmt_srv_set()
///       \Calls co_dev_get_id()
TEST(CO_Nmt, CoNmtCsInd_Start_Stop) {
  CreateNmtAndReset();
  SetNmtCsStIndFunc();

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_STOP);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoNmtCsInd::GetNumCalled());
  CoNmtCsInd::Check(nmt, CO_NMT_CS_STOP, nullptr);
  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, DEV_ID, CO_NMT_ST_STOP, nullptr);
  CHECK_EQUAL(CO_NMT_ST_STOP, co_nmt_get_st(nmt));
}

/// \Given a pointer to a started NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset node' command specifier
///
/// \Then 0 is returned, the service resets the node and transitions back to
///       the NMT 'start' state
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_set_1016()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \IfCalls{!LELY_NO_MALLOC, free()}
///       \IfCalls{!LELY_NO_CO_NG, can_recv_stop()}
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, co_dev_read_dcf()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, get_errc()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, diag()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_read_dcf()
///       \Calls get_errc()
///       \Calls co_dev_get_val_u32()
///       \Calls co_nmt_is_master()
///       \Calls can_recv_start()
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_LSS, co_nmt_get_lss()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u16()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u8()}
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \IfCalls{LELY_NO_MALLOC, set_errnum()}
///       \IfCalls{!LELY_NO_MALLOC, calloc()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_create()}
///       \Calls co_obj_get_val_u32()
///       \Calls co_nmt_hb_set_1016()
///       \Calls can_net_send()
TEST(CO_Nmt, CoNmtCsInd_Start_ResetNode) {
  const CoNmtCsInd::Seq csSeq = {CO_NMT_CS_RESET_NODE, CO_NMT_CS_RESET_COMM,
                                 CO_NMT_CS_ENTER_PREOP, CO_NMT_CS_START};
  const CoNmtStInd::Seq stSeq = {CO_NMT_ST_BOOTUP, CO_NMT_ST_BOOTUP,
                                 CO_NMT_ST_PREOP, CO_NMT_ST_START};
  CreateNmtAndReset();
  SetNmtCsStIndFunc(csSeq, stSeq);

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(csSeq.size(), CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(stSeq.size(), CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
}

/// \Given a pointer to a started NMT service (co_nmt_t), the object dictionary
///        contains objects in the communication parameters area
///        (0x1000-0x1fff)
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset communication' command
///       specifier
///
/// \Then 0 is returned, the service resets the communication and transitions
///       back to the NMT 'start' state; the communication parameters are
///       restored from the concise DCF backup
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_set_1016()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \IfCalls{!LELY_NO_MALLOC, free()}
///       \IfCalls{!LELY_NO_CO_NG, can_recv_stop()}
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \Calls co_dev_get_id()
///       \Calls co_dev_read_dcf()
///       \Calls get_errc()
///       \Calls co_dev_set_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_dev_get_val_u32()
///       \Calls co_nmt_is_master()
///       \Calls can_recv_start()
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_LSS, co_nmt_get_lss()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u16()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u8()}
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \IfCalls{LELY_NO_MALLOC, set_errnum()}
///       \IfCalls{!LELY_NO_MALLOC, calloc()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_create()}
///       \Calls co_obj_get_val_u32()
///       \Calls co_nmt_hb_set_1016()
///       \Calls can_net_send()
TEST(CO_Nmt, CoNmtCsInd_Start_ResetComm) {
  const CoNmtCsInd::Seq csSeq = {CO_NMT_CS_RESET_COMM, CO_NMT_CS_ENTER_PREOP,
                                 CO_NMT_CS_START};
  const CoNmtStInd::Seq stSeq = {CO_NMT_ST_BOOTUP, CO_NMT_ST_PREOP,
                                 CO_NMT_ST_START};

  dev_holder->CreateAndInsertObj(obj1fff, 0x1fffu);
  obj1fff->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED16,
                           co_unsigned16_t{0x1234u});
  CreateNmtAndReset();
  SetNmtCsStIndFunc(csSeq, stSeq);

  CHECK_EQUAL(sizeof(co_unsigned16_t),
              co_dev_set_val_u16(dev, 0x1fffu, 0x00u, 0));

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_COMM);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
  CHECK_EQUAL(csSeq.size(), CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(stSeq.size(), CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(0x1234u, co_dev_get_val_u16(dev, 0x1fffu, 0x00u));
}

#if HAVE_LELY_OVERRIDE
/// \Given a pointer to a started NMT service (co_nmt_t), the object dictionary
///        contains objects in the communication parameters area
///        (0x1000-0x1fff)
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset communication' command
///       specifier, but the service fails to restore the communication
///       parameters from the concise DCF backup
///
/// \Then 0 is returned, the service resets the communication and transitions
///       back to the NMT 'start' state; the communication parameters are
///       not restored
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_set_1016()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \IfCalls{!LELY_NO_MALLOC, free()}
///       \IfCalls{!LELY_NO_CO_NG, can_recv_stop()}
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \Calls co_dev_get_id()
///       \Calls co_dev_read_dcf()
///       \Calls get_errc()
///       \Calls co_dev_set_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_dev_get_val_u32()
///       \Calls co_nmt_is_master()
///       \Calls can_recv_start()
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_LSS, co_nmt_get_lss()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u16()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u8()}
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \IfCalls{LELY_NO_MALLOC, set_errnum()}
///       \IfCalls{!LELY_NO_MALLOC, calloc()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_create()}
///       \Calls co_obj_get_val_u32()
///       \Calls co_nmt_hb_set_1016()
///       \Calls can_net_send()
TEST(CO_Nmt, CoNmtCsInd_ResetComm_FailedToRestoreCommDcf) {
  dev_holder->CreateAndInsertObj(obj1fff, 0x1fffu);
  obj1fff->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED16,
                           co_unsigned16_t{0x1234u});
  CreateNmtAndReset();

  CHECK_EQUAL(sizeof(co_unsigned16_t),
              co_dev_set_val_u16(dev, 0x1fffu, 0x00u, 0));

  LelyOverride::co_val_read(Override::NoneCallsValid);
  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_COMM);
  LelyOverride::co_val_read(Override::AllCallsValid);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
  CHECK_EQUAL(0, co_dev_get_val_u16(dev, 0x1fffu, 0x00u));
}
#endif

/// \Given a pointer to a started NMT service (co_nmt_t), the object dictionary
///        contains objects with the `CO_OBJ_FLAGS_VAL_NODEID` flag set in the
///        communication parameters area (0x1000-0x1fff) and there is a new
///        pending Node-ID set
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset communication' command
///       specifier
///
/// \Then 0 is returned, the service resets the communication and transitions
///       back to the NMT 'start' state; objects with the
///       `CO_OBJ_FLAGS_VAL_NODEID` flag set are updated and new values are
///       stored in the concise DCF backup
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_set_1016()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \IfCalls{!LELY_NO_MALLOC, free()}
///       \IfCalls{!LELY_NO_CO_NG, can_recv_stop()}
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \Calls co_dev_get_id()
///       \Calls co_dev_read_dcf()
///       \Calls get_errc()
///       \Calls co_dev_set_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_dev_get_val_u32()
///       \Calls co_nmt_is_master()
///       \Calls can_recv_start()
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_LSS, co_nmt_get_lss()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u16()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u8()}
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \IfCalls{LELY_NO_MALLOC, set_errnum()}
///       \IfCalls{!LELY_NO_MALLOC, calloc()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_create()}
///       \Calls co_obj_get_val_u32()
///       \Calls co_nmt_hb_set_1016()
///       \Calls can_net_send()
TEST(CO_Nmt, CoNmtCsInd_ResetComm_NewNodeId) {
  const co_unsigned16_t val = 0x1200u;
  dev_holder->CreateAndInsertObj(obj1fff, 0x1fffu);
  obj1fff->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED16,
                           co_unsigned16_t{val + DEV_ID});
  co_sub_set_flags(obj1fff->GetLastSub(), CO_OBJ_FLAGS_VAL_NODEID);
  CreateNmtAndReset();

  // new pending Node-ID
  const co_unsigned8_t new_id = 0x42u;
  co_nmt_set_id(nmt, new_id);

  // modify the value with the CO_OBJ_FLAGS_VAL_NODEID flag
  CHECK_EQUAL(sizeof(co_unsigned16_t),
              co_dev_set_val_u16(dev, 0x1fffu, 0x00u, 0));

  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_COMM));
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));

  // the value is restored and the new Node-ID is applied to it
  CHECK_EQUAL(val + new_id, co_dev_get_val_u16(dev, 0x1fffu, 0x00u));

  // modify the value once more
  CHECK_EQUAL(sizeof(co_unsigned16_t),
              co_dev_set_val_u16(dev, 0x1fffu, 0x00u, 0));

  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_COMM));
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));

  // the value with the new Node-ID is restored
  CHECK_EQUAL(val + new_id, co_dev_get_val_u16(dev, 0x1fffu, 0x00u));
}

#if HAVE_LELY_OVERRIDE
/// \Given a pointer to a started NMT service (co_nmt_t), the object dictionary
///        contains objects with the `CO_OBJ_FLAGS_VAL_NODEID` flag set in the
///        communication parameters area (0x1000-0x1fff) and there is a new
///        pending Node-ID set
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset communication' command
///       specifier, but the service fails to store new values in the concise
///       DCF backup
///
/// \Then 0 is returned, the service resets the communication and transitions
///       back to the NMT 'start' state; objects with the
///       `CO_OBJ_FLAGS_VAL_NODEID` flag set are updated, but new values are
///       not stored in the concise DCF backup
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_set_1016()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \IfCalls{!LELY_NO_MALLOC, free()}
///       \IfCalls{!LELY_NO_CO_NG, can_recv_stop()}
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \Calls co_dev_get_id()
///       \Calls co_dev_read_dcf()
///       \Calls get_errc()
///       \Calls co_dev_set_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_dev_get_val_u32()
///       \Calls co_nmt_is_master()
///       \Calls can_recv_start()
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_LSS, co_nmt_get_lss()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u16()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u8()}
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \IfCalls{LELY_NO_MALLOC, set_errnum()}
///       \IfCalls{!LELY_NO_MALLOC, calloc()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_create()}
///       \Calls co_obj_get_val_u32()
///       \Calls co_nmt_hb_set_1016()
///       \Calls can_net_send()
TEST(CO_Nmt, CoNmtCsInd_ResetComm_NewNodeId_FailedToStoreAppDcf) {
  const co_unsigned16_t val = 0x1200u;
  dev_holder->CreateAndInsertObj(obj1fff, 0x1fffu);
  obj1fff->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED16,
                           co_unsigned16_t{val + DEV_ID});
  co_sub_set_flags(obj1fff->GetLastSub(), CO_OBJ_FLAGS_VAL_NODEID);
  CreateNmtAndReset();

  // new pending Node-ID
  const co_unsigned8_t new_id = 0x42u;
  co_nmt_set_id(nmt, new_id);

  // modify the value with the CO_OBJ_FLAGS_VAL_NODEID flag
  CHECK_EQUAL(sizeof(co_unsigned16_t),
              co_dev_set_val_u16(dev, 0x1fffu, 0x00u, 0));

  LelyOverride::co_val_write(Override::NoneCallsValid);
  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_COMM));
  LelyOverride::co_val_write(Override::AllCallsValid);

  // the value is restored and the new Node-ID is applied
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
  CHECK_EQUAL(val + new_id, co_dev_get_val_u16(dev, 0x1fffu, 0x00u));

  // modify the value once more
  CHECK_EQUAL(sizeof(co_unsigned16_t),
              co_dev_set_val_u16(dev, 0x1fffu, 0x00u, 0));

  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_COMM));
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));

  // the value is restored, but with an old Node-ID
  CHECK_EQUAL(val + DEV_ID, co_dev_get_val_u16(dev, 0x1fffu, 0x00u));
}
#endif

/// \Given a pointer to a started NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with the NMT 'start' command specifier
///
/// \Then 0 is returned, nothing is changed
TEST(CO_Nmt, CoNmtCsInd_Start_Start) {
  CreateNmtAndReset();
  SetNmtCsStIndFunc();

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_START);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
}

/// \Given a pointer to a stopped NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with the NMT 'enter pre-operational'
///       command specifier
///
/// \Then 0 is returned, the service transitions to the NMT 'pre-operational'
///       state
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \Calls co_dev_get_id()
TEST(CO_Nmt, CoNmtCsInd_Stop_EnterPreOperational) {
  CreateObj1f80NmtStartup(0x04);  // do not start automatically
  CreateNmtAndStop();
  SetNmtCsStIndFunc();

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_ENTER_PREOP);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoNmtCsInd::GetNumCalled());
  CoNmtCsInd::Check(nmt, CO_NMT_CS_ENTER_PREOP, nullptr);
  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, DEV_ID, CO_NMT_ST_PREOP, nullptr);
  CHECK_EQUAL(CO_NMT_ST_PREOP, co_nmt_get_st(nmt));
}

/// \Given a pointer to a stopped NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with the NMT 'start' command specifier
///
/// \Then 0 is returned, the service transitions to the NMT 'start' state
///       \Calls diag()
///       \Calls co_nmt_srv_set()
///       \Calls co_dev_get_id()
TEST(CO_Nmt, CoNmtCsInd_Stop_Start) {
  CreateNmtAndStop();
  SetNmtCsStIndFunc();

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_START);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoNmtCsInd::GetNumCalled());
  CoNmtCsInd::Check(nmt, CO_NMT_CS_START, nullptr);
  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, DEV_ID, CO_NMT_ST_START, nullptr);
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
}

/// \Given a pointer to a stopped NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset node' command specifier
///
/// \Then 0 is returned, the service resets the node and transitions to
///       the NMT 'start' state
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_set_1016()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \IfCalls{!LELY_NO_MALLOC, free()}
///       \IfCalls{!LELY_NO_CO_NG, can_recv_stop()}
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, co_dev_read_dcf()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, get_errc()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, diag()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_read_dcf()
///       \Calls get_errc()
///       \Calls co_dev_set_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_dev_get_val_u32()
///       \Calls co_nmt_is_master()
///       \Calls can_recv_start()
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_LSS, co_nmt_get_lss()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u16()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u8()}
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \IfCalls{LELY_NO_MALLOC, set_errnum()}
///       \IfCalls{!LELY_NO_MALLOC, calloc()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_create()}
///       \Calls co_obj_get_val_u32()
///       \Calls co_nmt_hb_set_1016()
///       \Calls can_net_send()
TEST(CO_Nmt, CoNmtCsInd_Stop_ResetNode) {
  const CoNmtCsInd::Seq csSeq = {CO_NMT_CS_RESET_NODE, CO_NMT_CS_RESET_COMM,
                                 CO_NMT_CS_ENTER_PREOP, CO_NMT_CS_START};
  const CoNmtStInd::Seq stSeq = {CO_NMT_ST_BOOTUP, CO_NMT_ST_BOOTUP,
                                 CO_NMT_ST_PREOP, CO_NMT_ST_START};
  CreateNmtAndStop();
  SetNmtCsStIndFunc(csSeq, stSeq);

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(csSeq.size(), CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(stSeq.size(), CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
}

/// \Given a pointer to a stopped NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset communication' command
///       specifier
///
/// \Then 0 is returned, the service resets the communication and transitions
///       to the NMT 'start' state
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_set_1016()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \IfCalls{!LELY_NO_MALLOC, free()}
///       \IfCalls{!LELY_NO_CO_NG, can_recv_stop()}
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \Calls co_dev_get_id()
///       \Calls co_dev_read_dcf()
///       \Calls get_errc()
///       \Calls co_dev_set_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_dev_get_val_u32()
///       \Calls co_nmt_is_master()
///       \Calls can_recv_start()
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_LSS, co_nmt_get_lss()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u16()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u8()}
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \IfCalls{LELY_NO_MALLOC, set_errnum()}
///       \IfCalls{!LELY_NO_MALLOC, calloc()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_create()}
///       \Calls co_obj_get_val_u32()
///       \Calls co_nmt_hb_set_1016()
///       \Calls can_net_send()
TEST(CO_Nmt, CoNmtCsInd_Stop_ResetComm) {
  const CoNmtCsInd::Seq csSeq = {CO_NMT_CS_RESET_COMM, CO_NMT_CS_ENTER_PREOP,
                                 CO_NMT_CS_START};
  const CoNmtStInd::Seq stSeq = {CO_NMT_ST_BOOTUP, CO_NMT_ST_PREOP,
                                 CO_NMT_ST_START};

  CreateNmtAndStop();
  SetNmtCsStIndFunc(csSeq, stSeq);

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_COMM);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(csSeq.size(), CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(stSeq.size(), CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
}

/// \Given a pointer to a stopped NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with the NMT 'stop' command specifier
///
/// \Then 0 is returned, nothing is changed
TEST(CO_Nmt, CoNmtCsInd_Stop_Stop) {
  CreateNmtAndStop();
  SetNmtCsStIndFunc();

  const auto ret = co_nmt_cs_ind(nmt, CO_NMT_CS_STOP);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(CO_NMT_ST_STOP, co_nmt_get_st(nmt));
}

/// \Given a pointer to a initialized NMT service (co_nmt_t) with no NMT
///        command indication function
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset node' command specifier
///       and then with the NMT 'stop' command specifier
///
/// \Then 0 is returned for both calls and the service transitions through all
///       NMT states
///       \Calls diag()
///       \IfCalls{!LELY_NO_CO_MASTER, can_recv_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NG, can_timer_stop()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_BOOT &&
///           !LELY_NO_MALLOC, co_nmt_boot_destroy()}
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_NMT_CFG &&
///           !LELY_NO_MALLOC, co_nmt_cfg_destroy()}
///       \Calls co_nmt_srv_set()
///       \IfCalls{LELY_NO_MALLOC, co_nmt_hb_set_1016()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_destroy()}
///       \IfCalls{!LELY_NO_MALLOC, free()}
///       \IfCalls{!LELY_NO_CO_NG, can_recv_stop()}
///       \Calls can_timer_stop()
///       \Calls can_recv_stop()
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, co_dev_read_dcf()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, get_errc()}
///       \IfCalls{!LELY_NO_CO_DCF_RESTORE, diag()}
///       \Calls co_dev_get_id()
///       \Calls co_dev_read_dcf()
///       \Calls get_errc()
///       \Calls co_dev_get_val_u32()
///       \Calls co_nmt_is_master()
///       \Calls can_recv_start()
///       \IfCalls{!LELY_NO_CO_MASTER && !LELY_NO_CO_LSS, co_nmt_get_lss()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u16()}
///       \IfCalls{!LELY_NO_CO_NG, co_dev_get_val_u8()}
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \IfCalls{LELY_NO_MALLOC, set_errnum()}
///       \IfCalls{!LELY_NO_MALLOC, calloc()}
///       \IfCalls{!LELY_NO_MALLOC, co_nmt_hb_create()}
///       \Calls co_obj_get_val_u32()
///       \Calls co_nmt_hb_set_1016()
///       \Calls can_net_send()
TEST(CO_Nmt, CoNmtCsInd_WithoutCsInd) {
  CreateNmt();

  const CoNmtStInd::Seq stSeq = {CO_NMT_ST_BOOTUP, CO_NMT_ST_BOOTUP,
                                 CO_NMT_ST_PREOP, CO_NMT_ST_START,
                                 CO_NMT_ST_STOP};
  co_nmt_set_st_ind(nmt, &CoNmtStInd::Func, nullptr);
  CoNmtStInd::SetCheckSeq(nmt, DEV_ID, stSeq);

  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));
  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_STOP));

  CHECK_EQUAL(stSeq.size(), CoNmtStInd::GetNumCalled());
}

///@}

/// @name co_nmt_comm_err_ind()
///@{

/// \Given a started NMT service (co_nmt_t), the object dictionary contains
///        the Error Behavior object (0x1029) containing the Communication
///        Error entry (0x01) with the value `0x00`; the node is in the
///        operational state
///
/// \When co_nmt_comm_err_ind() is called
///
/// \Then the node transitions to the NMT 'pre-operational' state
TEST(CO_Nmt, CoNmtCommErrInd_Obj1029_Value00_Operational) {
  CreateObj1f80NmtStartup(Obj1f80NmtStartup::AUTOSTART_BIT);
  CreateObj1029ErrorBehavior(Obj1029ErrorBehavior::CHANGE_TO_PREOP);
  CreateNmtAndReset();
  co_nmt_cs_ind(nmt, CO_NMT_CS_START);
  CoNmtCsInd::Clear();
  CoNmtStInd::Clear();

  co_nmt_comm_err_ind(nmt);

  CHECK_EQUAL(CO_NMT_ST_PREOP, co_nmt_get_st(nmt));
  CHECK_EQUAL(1u, CoNmtCsInd::GetNumCalled());
  CoNmtCsInd::Check(nmt, CO_NMT_CS_ENTER_PREOP, nullptr);
  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, DEV_ID, CO_NMT_ST_PREOP, nullptr);
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains
///        the Error Behavior object (0x1029) containing the Communication
///        Error entry (0x01) with the value `0x00`; the node is not in the
///        operational state
///
/// \When co_nmt_comm_err_ind() is called
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtCommErrInd_Obj1029_Value00_NotOperational) {
  CreateObj1029ErrorBehavior(Obj1029ErrorBehavior::CHANGE_TO_PREOP);
  CreateNmtAndReset();
  co_nmt_cs_ind(nmt, CO_NMT_CS_STOP);
  CoNmtCsInd::Clear();
  CoNmtStInd::Clear();

  co_nmt_comm_err_ind(nmt);

  CHECK_EQUAL(CO_NMT_ST_STOP, co_nmt_get_st(nmt));
  CHECK_EQUAL(0, CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains
///        the Error Behavior object (0x1029) containing the Communication
///        Error entry (0x01) with the value `0x01`
///
/// \When co_nmt_comm_err_ind() is called
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtCommErrInd_Obj1029_Value01) {
  CreateObj1029ErrorBehavior(Obj1029ErrorBehavior::NO_CHANGE);
  CreateNmtAndReset();

  co_nmt_comm_err_ind(nmt);

  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
  CHECK_EQUAL(0, CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
}

/// \Given a started NMT service (co_nmt_t), the object dictionary contains
///        the Error Behavior object (0x1029) containing the Communication
///        Error entry (0x01) with the value `0x02`
///
/// \When co_nmt_comm_err_ind() is called
///
/// \Then the node transitions to the NMT 'stop' state
TEST(CO_Nmt, CoNmtCommErrInd_Obj1029_Value02) {
  CreateObj1029ErrorBehavior(Obj1029ErrorBehavior::CHANGE_TO_STOP);
  CreateNmtAndReset();

  co_nmt_comm_err_ind(nmt);

  CHECK_EQUAL(CO_NMT_ST_STOP, co_nmt_get_st(nmt));
  CHECK_EQUAL(1u, CoNmtCsInd::GetNumCalled());
  CoNmtCsInd::Check(nmt, CO_NMT_CS_STOP, nullptr);
  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, DEV_ID, CO_NMT_ST_STOP, nullptr);
}

///@}

#if !LELY_NO_CO_MASTER

/// @name co_nmt_node_err_ind()
///@{

/// \Given a started NMT service (co_nmt_t) configured as NMT slave
///
/// \When co_nmt_node_err_ind() is called with any Node-ID
///
/// \Then -1 is returned and the error number is set to ERRNUM_PERM
TEST(CO_Nmt, CoNmtNodeErrInd_NotMaster) {
  CreateNmtAndReset();

  const auto ret = co_nmt_node_err_ind(nmt, 0);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_PERM, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(0, CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
}

/// \Given a started NMT service (co_nmt_t) configured as NMT master
///
/// \When co_nmt_node_err_ind() is called with a Node-ID equal to 0
///
/// \Then -1 is returned and the error number is set to ERRNUM_INVAL
TEST(CO_Nmt, CoNmtNodeErrInd_ZeroNodeId) {
  SetupMaster();
  CreateNmtAndReset();

  const auto ret = co_nmt_node_err_ind(nmt, 0);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(0, CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
}

/// \Given a started NMT service (co_nmt_t) configured as NMT master
///
/// \When co_nmt_node_err_ind() is called with a Node-ID larger than
///       CO_NUM_NODES
///
/// \Then -1 is returned and the error number is set to ERRNUM_INVAL
TEST(CO_Nmt, CoNmtNodeErrInd_NodeIdOverMax) {
  SetupMaster();
  CreateNmtAndReset();

  const auto ret = co_nmt_node_err_ind(nmt, CO_NUM_NODES + 1u);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(0, CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
}

/// \Given a started NMT service (co_nmt_t) configured as NMT master
///
/// \When co_nmt_node_err_ind() is called with a Node-ID of an unknown slave
///
/// \Then 0 is returned, nothing is changed
TEST(CO_Nmt, CoNmtNodeErrInd_UnknownSlave) {
  SetupMaster();
  CreateNmtAndReset();

  const auto ret = co_nmt_node_err_ind(nmt, SLAVE_DEV_ID);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(0, CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
}

// FIXME: there is a bug in the NMT 'boot slave' in MALLOC mode (non-ECSS)
#if LELY_NO_MALLOC

/// \Given a started NMT service (co_nmt_t) configured as NMT master with
///        `0x40` bit set in the NMT Startup object (0x1f80)
///
/// \When co_nmt_node_err_ind() is called with a Node-ID of a mandatory slave
///
/// \Then 0 is returned, the NMT 'stop' command is sent to all nodes
///       (`Node-ID = 0`) and the master transitions to the NMT 'stop' state
TEST(CO_Nmt, CoNmtNodeErrInd_MandatorySlave_StopAllNodes) {
  SetupMasterWithSlave(Obj1f80NmtStartup::MASTER_BIT |
                       Obj1f80NmtStartup::STOP_NODES_ON_ERR);
  CreateNmtAndReset();
#if LELY_NO_CO_NMT_BOOT
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
#else
  CHECK_EQUAL(CO_NMT_ST_PREOP, co_nmt_get_st(nmt));
#endif

  const auto ret = co_nmt_node_err_ind(nmt, SLAVE_DEV_ID);

  CHECK_EQUAL(0, ret);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CanSend::CheckMsg(CreateNmtMsg(0, CO_NMT_CS_STOP));

  CHECK_EQUAL(CO_NMT_ST_STOP, co_nmt_get_st(nmt));
  CHECK_EQUAL(1u, CoNmtCsInd::GetNumCalled());
  CoNmtCsInd::Check(nmt, CO_NMT_CS_STOP, nullptr);
  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, DEV_ID, CO_NMT_ST_STOP, nullptr);
}

/// \Given a started NMT service (co_nmt_t) configured as NMT master with
///        `0x10` bit set in the NMT Startup object (0x1f80)
///
/// \When co_nmt_node_err_ind() is called with a Node-ID of a mandatory slave
///
/// \Then 0 is returned, the NMT 'reset node' command is sent to all nodes
///       (`Node-ID = 0`) and the master resets itself
TEST(CO_Nmt, CoNmtNodeErrInd_MandatorySlave_ResetAllNodes) {
  SetupMasterWithSlave(Obj1f80NmtStartup::MASTER_BIT |
                       Obj1f80NmtStartup::RESET_NODES_ON_ERR);
  CreateNmtAndReset();
#if LELY_NO_CO_NMT_BOOT
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
#else
  CHECK_EQUAL(CO_NMT_ST_PREOP, co_nmt_get_st(nmt));
#endif

  const CoNmtCsInd::Seq csSeq = {CO_NMT_CS_RESET_NODE, CO_NMT_CS_RESET_COMM,
                                 CO_NMT_CS_ENTER_PREOP, CO_NMT_CS_START};
  const CoNmtStInd::Seq stSeq = {CO_NMT_ST_BOOTUP, CO_NMT_ST_BOOTUP,
                                 CO_NMT_ST_PREOP, CO_NMT_ST_START};
  SetNmtCsStIndFunc(csSeq, stSeq);

  const CanSend::MsgSeq msgSeq = {
      CreateNmtMsg(0, CO_NMT_CS_RESET_NODE),
      CreateNmtEcMsg(MASTER_DEV_ID, CO_NMT_ST_BOOTUP),
      CreateNmtMsg(0, CO_NMT_CS_RESET_COMM),
  };
  CanSend::SetCheckSeq(msgSeq);

  const auto ret = co_nmt_node_err_ind(nmt, SLAVE_DEV_ID);

  CHECK_EQUAL(0, ret);

  CHECK_EQUAL(msgSeq.size(), CanSend::GetNumCalled());

#if LELY_NO_CO_NMT_BOOT
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
  CHECK_EQUAL(csSeq.size(), CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(stSeq.size(), CoNmtStInd::GetNumCalled());
#else
  CHECK_EQUAL(CO_NMT_ST_PREOP, co_nmt_get_st(nmt));
  CHECK_EQUAL(csSeq.size() - 1u, CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(stSeq.size() - 1u, CoNmtStInd::GetNumCalled());
#endif
}

#endif  // LELY_NO_MALLOC

/// \Given a started NMT service (co_nmt_t) configured as NMT master, no
///        additional bits in the NMT Startup object (0x1f80) are set
///
/// \When co_nmt_node_err_ind() is called with a Node-ID of a mandatory slave
///
/// \Then 0 is returned, the NMT 'reset node' command is sent to the slave
TEST(CO_Nmt, CoNmtNodeErrInd_MandatorySlave_NoBits) {
  SetupMasterWithSlave();
  CreateNmtAndReset();

  const auto ret = co_nmt_node_err_ind(nmt, SLAVE_DEV_ID);

  CHECK_EQUAL(0, ret);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CanSend::CheckMsg(CreateNmtMsg(SLAVE_DEV_ID, CO_NMT_CS_RESET_NODE));

  CHECK_EQUAL(0, CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
}

/// \Given a started NMT service (co_nmt_t) configured as NMT master
///
/// \When co_nmt_node_err_ind() is called with a Node-ID of a non-mandatory
///       slave
///
/// \Then 0 is returned, the NMT 'reset node' command is sent to the slave
TEST(CO_Nmt, CoNmtNodeErrInd_NonMandatorySlave) {
  SetupMasterWithSlave();
  obj1f81->SetSub<Obj1f81NmtSlaveAssignment::SubNthSlaveEntry>(
      SLAVE_DEV_ID, Obj1f81NmtSlaveAssignment::ASSIGNMENT_BIT);

  CreateNmtAndReset();

  const auto ret = co_nmt_node_err_ind(nmt, SLAVE_DEV_ID);

  CHECK_EQUAL(0, ret);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CanSend::CheckMsg(CreateNmtMsg(SLAVE_DEV_ID, CO_NMT_CS_RESET_NODE));

  CHECK_EQUAL(0, CoNmtCsInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
}

///@}

#endif  // !LELY_NO_CO_MASTER

/// @name co_dev_cfg_hb()
///@{

/// \Given a pointer to an initialized device (co_dev_t), the object dictionary
///        does not contain the Consumer Heartbeat Time object (0x1016)
///
/// \When co_dev_cfg_hb() is called with a pointer to the device and with other
///       arguments having any value
///
/// \Then CO_SDO_AC_NO_OBJ is returned, the device is not modified
///       \Calls co_dev_find_obj()
TEST(CO_Nmt, CoDevCfgHb_NoObject1016) {
  CreateNmt();

  const auto ret = co_dev_cfg_hb(dev, 0u, 0u);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
  POINTERS_EQUAL(nullptr, co_dev_find_obj(dev, 0x1016u));
}

/// \Given a pointer to an initialized device (co_dev_t), the object dictionary
///        contains the Consumer Heartbeat Time object (0x1016), but without
///        any sub-objects
///
/// \When co_dev_cfg_hb() is called with a pointer to the device and with other
///       arguments having any correct value
///
/// \Then CO_SDO_AC_NO_SUB is returned, the device is not modified
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
TEST(CO_Nmt, CoDevCfgHb_NoSubsInObject1016) {
  CreateNmt();
  dev_holder->CreateObj<Obj1016ConsumerHb>(obj1016);
  const co_unsigned8_t nodeId = 1u;

  const auto ret = co_dev_cfg_hb(dev, nodeId, 1u);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
}

/// \Given a pointer to an initialized device (co_dev_t), the object dictionary
///        contains the Consumer Heartbeat Time object (0x1016) with the
///        "Highest sub-index supported" sub-object (0x00) set to a value
///        greater than zero, but without any other sub-objects (a malformed
///        object).
///
/// \When co_dev_cfg_hb() is called with a pointer to the device and with other
///       arguments having any correct value
///
/// \Then CO_SDO_AC_NO_SUB is returned, the device is not modified
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \Calls co_obj_find_sub()
TEST(CO_Nmt, CoDevCfgHb_MissingSubInObject1016) {
  CreateNmt();
  dev_holder->CreateObj<Obj1016ConsumerHb>(obj1016);
  const co_unsigned8_t highestIdx = 1u;
  obj1016->EmplaceSub<Obj1016ConsumerHb::Sub00HighestSubidxSupported>(
      highestIdx);
  const co_unsigned8_t nodeId = 1u;

  const auto ret = co_dev_cfg_hb(dev, nodeId, 1u);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
}

/// \Given a pointer to an initialized device (co_dev_t), the object dictionary
///        contains the Consumer Heartbeat Time object (0x1016)
///
/// \When co_dev_cfg_hb() is called with a pointer to the device, an incorrect
///       Node-ID (zero) and any correct heartbeat time
///
/// \Then CO_SDO_AC_PARAM_LO is returned, the device is not modified
///       \Calls co_dev_find_obj()
TEST(CO_Nmt, CoDevCfgHb_NodeIdZero) {
  CreateNmt();
  dev_holder->CreateObj<Obj1016ConsumerHb>(obj1016);

  const auto ret = co_dev_cfg_hb(dev, 0u, 1u);

  CHECK_EQUAL(CO_SDO_AC_PARAM_LO, ret);
}

/// \Given a pointer to an initialized device (co_dev_t), the object dictionary
///        contains the Consumer Heartbeat Time object (0x1016)
///
/// \When co_dev_cfg_hb() is called with a pointer to the device, an incorrect
///       Node-ID (larger than CO_NUM_NODES) and any correct heartbeat time
///
/// \Then CO_SDO_AC_PARAM_HI is returned, the device is not modified
///       \Calls co_dev_find_obj()
TEST(CO_Nmt, CoDevCfgHb_NodeIdOverMax) {
  CreateNmt();
  dev_holder->CreateObj<Obj1016ConsumerHb>(obj1016);

  const auto ret = co_dev_cfg_hb(dev, CO_NUM_NODES + 1u, 1u);

  CHECK_EQUAL(CO_SDO_AC_PARAM_HI, ret);
}

/// \Given a pointer to an initialized device (co_dev_t), the object dictionary
///        contains the Consumer Heartbeat Time object (0x1016) with multiple
///        Consumer Heartbeat Time sub-objects, one containing an incorrect
///        Node-ID (zero)
///
/// \Wen co_dev_cfg_hb() is called with a pointer to the device, a selected
///       Node-ID and a heartbeat time
///
/// \Then 0 is returned and the requested value is assigned to the sub-object
///       with an incorrect Node-ID
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \Calls co_obj_find_sub()
///       \Calls co_sub_dn_ind_val()
TEST(CO_Nmt, CoDevCfgHb_OverwriteSubWithZeroId) {
  CreateNmt();
  const co_unsigned8_t highestIdx = 10u;
  const co_unsigned8_t selectedIdx = 7u;
  CreateObj1016ConsumerHbTimeN(highestIdx);
  obj1016->SetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(selectedIdx, 0u);

  const co_unsigned8_t nodeId = 42u;
  const co_unsigned16_t ms = 1410u;
  const auto ret = co_dev_cfg_hb(dev, nodeId, ms);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(
      Obj1016ConsumerHb::MakeHbConsumerEntry(nodeId, ms),
      obj1016->GetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(selectedIdx));
}

/// \Given a pointer to an initialized device (co_dev_t), the object dictionary
///        contains the Consumer Heartbeat Time object (0x1016) with multiple
///        Consumer Heartbeat Time sub-objects, one containing an incorrect
///        Node-ID (larger than CO_NUM_NODES)
///
/// \When co_dev_cfg_hb() is called with a pointer to the device, a selected
///       Node-ID and a heartbeat time
///
/// \Then 0 is returned and the requested value is assigned to the sub-object
///       with an incorrect Node-ID
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \Calls co_obj_find_sub()
///       \Calls co_sub_dn_ind_val()
TEST(CO_Nmt, CoDevCfgHb_OverwriteSubWithIdOverMax) {
  CreateNmt();
  const co_unsigned8_t highestIdx = 10u;
  const co_unsigned8_t selectedIdx = 7u;
  CreateObj1016ConsumerHbTimeN(highestIdx);
  obj1016->SetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(
      selectedIdx,
      Obj1016ConsumerHb::MakeHbConsumerEntry(CO_NUM_NODES + 1u, 0u));

  const co_unsigned8_t nodeId = 42u;
  const co_unsigned16_t ms = 1410u;
  const auto ret = co_dev_cfg_hb(dev, nodeId, ms);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(
      Obj1016ConsumerHb::MakeHbConsumerEntry(nodeId, ms),
      obj1016->GetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(selectedIdx));
}

/// \Given a pointer to an initialized device (co_dev_t), the object dictionary
///        contains the Consumer Heartbeat Time object (0x1016) with multiple
///        Consumer Heartbeat Time sub-objects, one containing a selected
///        Node-ID
///
/// \When co_dev_cfg_hb() is called with a pointer to the device, the selected
///       Node-ID and a heartbeat time
///
/// \Then 0 is returned and the Consumer Heartbeat Time sub-object containing
///       the selected Node-ID is updated
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \Calls co_obj_find_sub()
///       \Calls co_sub_dn_ind_val()
TEST(CO_Nmt, CoDevCfgHb_UpdateSubWithSelectedId) {
  CreateNmt();
  const co_unsigned8_t highestIdx = 10u;
  const co_unsigned8_t selectedIdx = 8u;
  CreateObj1016ConsumerHbTimeN(highestIdx);
  obj1016->SetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(
      selectedIdx, Obj1016ConsumerHb::MakeHbConsumerEntry(DEV_ID, 0u));

  const co_unsigned16_t ms = 1410u;
  const auto ret = co_dev_cfg_hb(dev, DEV_ID, ms);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(
      Obj1016ConsumerHb::MakeHbConsumerEntry(DEV_ID, ms),
      obj1016->GetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(selectedIdx));
}

/// \Given a pointer to an initialized device (co_dev_t), the object dictionary
///        contains the Consumer Heartbeat Time object (0x1016) with multiple
///        Consumer Heartbeat Time sub-objects, one containing a selected
///        Node-ID
///
/// \When co_dev_cfg_hb() is called with a pointer to the device, the selected
///       Node-ID and a heartbeat time equal 0
///
/// \Then 0 is returned and the Consumer Heartbeat Time sub-object containing
///       the selected Node-ID is marked as "unused" (zero)
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \Calls co_obj_find_sub()
///       \Calls co_sub_dn_ind_val()
TEST(CO_Nmt, CoDevCfgHb_ClearSubWithSelectedId) {
  CreateNmt();
  const co_unsigned8_t highestIdx = 10u;
  const co_unsigned8_t selectedIdx = 4u;
  CreateObj1016ConsumerHbTimeN(highestIdx);
  obj1016->SetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(
      selectedIdx, Obj1016ConsumerHb::MakeHbConsumerEntry(DEV_ID, 0u));

  const auto ret = co_dev_cfg_hb(dev, DEV_ID, 0u);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(0u, obj1016->GetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(
                      selectedIdx));
}

/// \Given a pointer to an initialized device (co_dev_t), the object dictionary
///        contains the Consumer Heartbeat Time object (0x1016) with multiple
///        Consumer Heartbeat Time sub-objects, none containing a selected
///        Node-ID
///
/// \When co_dev_cfg_hb() is called with a pointer to the device, the selected
///       Node-ID and a heartbeat time equal 0
///
/// \Then 0 is returned and sub-objects are not modified
///       \Calls co_dev_find_obj()
///       \Calls co_obj_get_val_u8()
///       \Calls co_obj_find_sub()
TEST(CO_Nmt, CoDevCfgHb_ClearNonExistingId) {
  CreateNmt();
  const co_unsigned8_t highestIdx = 10u;
  CreateObj1016ConsumerHbTimeN(highestIdx);

  const auto ret = co_dev_cfg_hb(dev, DEV_ID, 0u);

  CHECK_EQUAL(0u, ret);
  for (co_unsigned8_t i = 1u; i <= highestIdx; ++i) {
    CHECK_COMPARE(obj1016->GetSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(i),
                  !=, 0u);
  }
}

///@}

/// @name co_nmt_on_err()
///@{

/// \Given a pointer to a started NMT service (co_nmt_t)
///
/// \When co_nmt_on_err() is called with an emergency error code equal to zero,
///       any error register value and any manufacturer-specific error code
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtOnErr_EecZero) {
  dev_holder->CreateObjValue<Obj1001ErrorRegister>(obj1001, 0);
  CreateNmtAndReset();

  const co_unsigned8_t er = 0x01u;

  co_nmt_on_err(nmt, 0, er, nullptr);

  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
  CHECK_EQUAL(0, obj1001->GetSub<Obj1001ErrorRegister>());
}

/// \Given a pointer to a started NMT service (co_nmt_t), the EMCY service is
///        not initialized
///
/// \When co_nmt_on_err() is called with a non-zero emergency error code, any
///       error register value and any manufacturer-specific error code
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtOnErr_NoEmcy) {
  CreateNmtAndReset();
  POINTERS_EQUAL(nullptr, co_nmt_get_emcy(nmt));

  const co_unsigned16_t eec = 0x1000u;
  const co_unsigned8_t er = 0x01u;

  co_nmt_on_err(nmt, eec, er, nullptr);

  CHECK_EQUAL(0, co_dev_get_val_u8(dev, 0x1001u, 0x00u));
  CHECK_EQUAL(0, co_dev_get_val_u8(dev, 0x1003u, 0x00u));
}

/// \Given a pointer to a started NMT service (co_nmt_t), the EMCY service is
///        initialized and running, the object dictionary contains the Error
///        Register object (0x1001) and the Pre-defined Error Field object
///        (0x1003) with no errors recorded
///
/// \When co_nmt_on_err() is called with a non-zero emergency error code, any
///       error register value and any manufacturer-specific error code
///
/// \Then the error register value is set in the Error Register object and the
///       emergency error code is added to the Pre-defined Error Field object
///       \Calls co_emcy_push()
TEST(CO_Nmt, CoNmtOnErr_EmcyPush) {
  dev_holder->CreateObjValue<Obj1001ErrorRegister>(obj1001);
  dev_holder->CreateObj<Obj1003PredefinedErrorField>(obj1003);
  obj1003->EmplaceSub<Obj1003PredefinedErrorField::Sub00NumberOfErrors>(0);
  obj1003->EmplaceSub<Obj1003PredefinedErrorField::SubNStandardErrorField>(0);
  CreateNmtAndReset();

  const co_unsigned16_t eec = 0x1000u;
  const co_unsigned8_t er = 0x01u;

  co_nmt_on_err(nmt, eec, er, nullptr);

  CHECK_EQUAL(er, obj1001->GetSub<Obj1001ErrorRegister>());
  CHECK_EQUAL(
      0x01u,
      obj1003->GetSub<Obj1003PredefinedErrorField::Sub00NumberOfErrors>());
  CHECK_EQUAL(
      eec,
      obj1003->GetSub<Obj1003PredefinedErrorField::SubNStandardErrorField>());
}

/// \Given a pointer to a started NMT service (co_nmt_t), the object dictionary
///        contains the Error Behavior object (0x1029)
///
/// \When co_nmt_on_err() is called with a communication error (`0x81xx`)
///       emergency error code, any error register value and any
///       manufacturer-specific error code
///
/// \Then the behaviour specified by the Error Behavior object is invoked
///       \Calls co_nmt_comm_err_ind()
TEST(CO_Nmt, CoNmtOnErr_ErrInd) {
  CreateObj1029ErrorBehavior(Obj1029ErrorBehavior::CHANGE_TO_STOP);
  CreateNmtAndReset();

  const co_unsigned16_t eec = 0x8100u;

  co_nmt_on_err(nmt, eec, 0, nullptr);

  CHECK_EQUAL(CO_NMT_ST_STOP, co_nmt_get_st(nmt));
  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, DEV_ID, CO_NMT_ST_STOP, nullptr);
}

///@}

/// @name NMT service: received NMT message processing
///@{

#if !LELY_NO_CO_MASTER
/// \Given a started NMT service (co_nmt_t) configured as NMT master
///
/// \When an NMT message is received
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtRecv000_Master) {
  SetupMaster();
  CreateNmtAndReset();

  const can_msg msg = CreateNmtMsg(DEV_ID, CO_NMT_CS_STOP);
  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
}
#endif

/// \Given a started NMT service (co_nmt_t) configured as NMT slave
///
/// \When an NMT message with an incorrect message length is received
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtRecv000_IncorrectMsgLength) {
  CreateNmtAndReset();

  can_msg msg = CreateNmtMsg(DEV_ID, CO_NMT_CS_STOP);
  msg.len = 1u;
  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
}

/// \Given a started NMT service (co_nmt_t) configured as NMT slave
///
/// \When an NMT message with a Node-ID equal to zero (all nodes) is received
///
/// \Then the command is processed, the node transitions to the requested state
TEST(CO_Nmt, CoNmtRecv000_ZeroId) {
  CreateNmtAndReset();

  const can_msg msg = CreateNmtMsg(0, CO_NMT_CS_STOP);
  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK_EQUAL(CO_NMT_ST_STOP, co_nmt_get_st(nmt));
}

/// \Given a started NMT service (co_nmt_t) configured as NMT slave
///
/// \When an NMT message with other node's Node-ID is received
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtRecv000_OtherNode) {
  CreateNmtAndReset();

  const can_msg msg = CreateNmtMsg(DEV_ID + 1u, CO_NMT_CS_STOP);
  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
}

/// \Given a started NMT service (co_nmt_t) configured as NMT slave
///
/// \When an NMT message with the node's Node-ID is received
///
/// \Then the command is processed, the node transitions to the requested state
TEST(CO_Nmt, CoNmtRecv000_Nominal) {
  CreateNmtAndReset();

  const can_msg msg = CreateNmtMsg(DEV_ID, CO_NMT_CS_STOP);
  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK_EQUAL(CO_NMT_ST_STOP, co_nmt_get_st(nmt));
}

///@}

/// @name NMT service: received NMT error control message processing
///@{

#if !LELY_NO_CO_MASTER

/// \Given a started NMT service (co_nmt_t) configured as NMT master
///
/// \When an NMT error control message with a Node-ID equal to zero is received
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtRecv700_ZeroId) {
  SetupMasterWithSlave();
  CreateNmtAndReset();

  const can_msg msg = CreateNmtEcMsg(0, CO_NMT_ST_START);
  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
}

/// \Given a started NMT service (co_nmt_t) configured as NMT master
///
/// \When an NMT error control message with an incorrect message length is
///       received
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtRecv700_IncorrectMsgLength) {
  SetupMasterWithSlave();
  CreateNmtAndReset();

  can_msg msg = CreateNmtEcMsg(SLAVE_DEV_ID, CO_NMT_ST_BOOTUP);
  msg.len = 0;
  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
}

/// \Given a started NMT service (co_nmt_t) configured as NMT master with
///        a configured slave node
///
/// \When an NMT boot-up message from the slave node is received
///
/// \Then the NMT state change indication function is invoked with the slave's
///       Node-ID, CO_NMT_ST_BOOTUP state and a null user-specified data
///       pointer, the receipt of a boot-up message from the slave is denoted
TEST(CO_Nmt, CoNmtRecv700_BootUpMsg) {
  SetupMasterWithSlave();
  CreateNmtAndReset();

  const can_msg msg = CreateNmtEcMsg(SLAVE_DEV_ID, CO_NMT_ST_BOOTUP);
  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK_EQUAL(1u, CoNmtStInd::GetNumCalled());
  CoNmtStInd::Check(nmt, SLAVE_DEV_ID, CO_NMT_ST_BOOTUP, nullptr);
  CHECK_EQUAL(1, co_nmt_chk_bootup(nmt, SLAVE_DEV_ID));
}

/// \Given a started NMT service (co_nmt_t) configured as NMT master with
///        a configured slave node
///
/// \When an NMT hearbeat message from the slave node is received
///
/// \Then nothing is changed
TEST(CO_Nmt, CoNmtRecv700_HbMsg) {
  SetupMasterWithSlave();
  CreateNmtAndReset();

  const can_msg msg = CreateNmtEcMsg(SLAVE_DEV_ID, CO_NMT_ST_START);
  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK_EQUAL(0, CoNmtStInd::GetNumCalled());
}

#endif  // !LELY_NO_CO_MASTER

///@}

/// @name NMT service: NMT error control timer
///@{

/// \Given a started NMT service (co_nmt_t) with the heartbeat producer service
///        set up
///
/// \When the producer heartbeat time passes
///
/// \Then the heartbeat message with the current state of the NMT service is
///       sent
TEST(CO_Nmt, CoNmtEcTimer_ProduceHb) {
  CreateObj1017ProducerHeartbeatTime(HB_TIMEOUT_MS);
  CreateNmtAndReset();

  AdvanceTimeMs(HB_TIMEOUT_MS);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CanSend::CheckMsg(CreateNmtEcMsg(DEV_ID, CO_NMT_ST_START));
}

/// \Given a started NMT service (co_nmt_t) with the heartbeat producer service
///        set up, but the producer heartbeat time is zeroed after start
///
/// \When the producer heartbeat time passes
///
/// \Then no heartbeat message is sent
TEST(CO_Nmt, CoNmtEcTimer_ZeroedMs) {
  CreateObj1017ProducerHeartbeatTime(HB_TIMEOUT_MS);
  CreateNmtAndReset();

  const co_unsigned16_t ms = 0;
  CHECK_EQUAL(0, co_dev_dn_val_req(dev, 0x1017u, 0x00u, CO_DEFTYPE_UNSIGNED16,
                                   &ms, nullptr, CoCsdoDnCon::Func, nullptr));
  CoCsdoDnCon::Check(nullptr, 0x1017u, 0x00, 0, nullptr);

  AdvanceTimeMs(HB_TIMEOUT_MS);

  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

///@}
