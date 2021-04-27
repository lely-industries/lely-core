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
#include <lely/co/ssdo.h>

#include <libtest/allocators/default.hpp>
#include <libtest/allocators/limited.hpp>
#include <libtest/override/lelyco-val.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/sub.hpp"

#include <lib/co/nmt_hb.h>
#if !LELY_NO_CO_NMT_BOOT
#include <lib/co/nmt_boot.h>
#endif
#if !LELY_NO_CO_NMT_CFG
#include <lib/co/nmt_cfg.h>
#endif

#ifndef CO_NMT_MAX_NHB
#define CO_NMT_MAX_NHB CO_NUM_NODES
#endif

TEST_BASE(CO_NmtBase) {
  TEST_BASE_SUPER(CO_NmtBase);

  const co_unsigned8_t DEV_ID = 0x01u;
  const co_unsigned8_t MASTER_DEV_ID = DEV_ID;
  const co_unsigned8_t SLAVE_DEV_ID = 0x02u;

  can_net_t* net = nullptr;
  co_dev_t* dev = nullptr;

  std::unique_ptr<CoDevTHolder> dev_holder;

  std::unique_ptr<CoObjTHolder> obj1000;
#if !LELY_NO_CO_DCF_RESTORE
  std::unique_ptr<CoObjTHolder> obj2000;
#endif

  std::unique_ptr<CoObjTHolder> obj1016;
  std::unique_ptr<CoObjTHolder> obj1017;
  std::unique_ptr<CoObjTHolder> obj1f80;
  std::unique_ptr<CoObjTHolder> obj1f81;
  std::unique_ptr<CoObjTHolder> obj1f82;

  Allocators::Default allocator;

  void CreateObj(std::unique_ptr<CoObjTHolder> & obj_holder,
                 co_unsigned16_t idx) {
    obj_holder.reset(new CoObjTHolder(idx));
    CHECK(obj_holder->Get() != nullptr);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));
  }

  void CreateObj1016ConsumerHbTimeN(const size_t num) {
    assert(num > 0 && num <= CO_NMT_MAX_NHB);

    CreateObj(obj1016, 0x1016u);

    // 0x00 - Highest sub-index supported
    obj1016->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(num));
    // 0x01-0x7f - Consumer heartbeat time
    for (co_unsigned8_t i = 0; i < num; ++i) {
      obj1016->InsertAndSetSub(
          i + 1, CO_DEFTYPE_UNSIGNED32,
          co_unsigned32_t(co_unsigned32_t(SLAVE_DEV_ID) << 16u) |
              co_unsigned32_t(0x0001u));  //  1 ms
    }
  }

  void CreateObj1017ProducerHeartbeatTime(const co_unsigned16_t hb_time) {
    CreateObj(obj1017, 0x1017u);
    obj1017->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED16,
                             co_unsigned16_t(hb_time));
  }

  void CreateObj1f80NmtStartup(const co_unsigned32_t startup) {
    CreateObj(obj1f80, 0x1f80u);
    obj1f80->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32,
                             co_unsigned32_t(startup));
  }

  void CreateObj1f81SlaveAssignmentN(const size_t num) {
    assert(num > 0 && num <= CO_NUM_NODES);
    // object 0x1f81 - Slave assignment object
    CreateObj(obj1f81, 0x1f81u);

    // 0x00 - Highest sub-index supported
    obj1f81->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(num));
    // 0x01-0x7f - Slave with the given Node-ID
    for (co_unsigned8_t i = 0; i < num; ++i) {
      obj1f81->InsertAndSetSub(i + 1u, CO_DEFTYPE_UNSIGNED32,
                               co_unsigned32_t(0x01u));
    }
  }

  void CreateObj1f82RequestNmt(const size_t num) {
    assert(num > 0 && num <= CO_NUM_NODES);
    // object 0x1f82 - Request NMT object
    CreateObj(obj1f82, 0x1f82u);

    // 0x00 - Highest sub-index supported
    obj1f82->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(num));
    // 0x01-0x7f - Request NMT-Service for slave with the given Node-ID
    for (co_unsigned8_t i = 0; i < num; ++i) {
      obj1f82->InsertAndSetSub(i + 1u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0));
    }
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(allocator.ToAllocT());
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);
  }

  TEST_TEARDOWN() {
    dev_holder.reset();
    can_net_destroy(net);
    set_errnum(0);
  }
};

TEST_GROUP_BASE(CO_NmtCreate, CO_NmtBase) {
  co_nmt_t* nmt = nullptr;

  void CheckNmtDefaults() {
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
    CHECK_EQUAL(0, co_nmt_is_master(nmt));
#if !LELY_NO_CO_MASTER
#if !LELY_NO_CO_NMT_BOOT || !LELY_NO_CO_NMT_CFG
    CHECK_EQUAL(LELY_CO_NMT_TIMEOUT, co_nmt_get_timeout(nmt));
#else
    CHECK_EQUAL(0, co_nmt_get_timeout(nmt));
#endif
#endif
  }

  // co_dev_write_dcf(): every <> is a call to co_val_write() when writing DCFs
  static size_t GetCoDevWriteDcf_NullBuf_CoValWriteCalls(
      const size_t num_subs) {
    // <total number of subs> + NUM_SUBS * <get sub's size>
    return 1u + num_subs;
  }

  static size_t GetCoDevWriteDcf_CoValWriteCalls(const size_t num_subs) {
    // <total number of subs> + NUM_SUBS * (<get sub's size> + <sub's index>
    //     + <sub's sub-index> + <sub's size> + <sub's value>)
    return 1u + num_subs * 5u;
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
#endif

#elif LELY_NO_MALLOC
#if LELY_NO_CO_NG && LELY_NO_CO_NMT_BOOT && LELY_NO_CO_NMT_CFG  // ECSS
#if LELY_NO_CO_MASTER
  CHECK_EQUAL(1360u, ret);
#else
  CHECK_EQUAL(4768u, ret);
#endif
#else
  CHECK_EQUAL(11872u, ret);
#endif

#elif LELY_NO_HOSTED
  CHECK_EQUAL(11872u, ret);

#elif LELY_NO_CO_MASTER
  CHECK_EQUAL(400u, ret);

#else
  CHECK_EQUAL(9712u, ret);
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
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_nmt_srv_init()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls co_dev_find_obj()
///       \IfCalls{LELY_NO_MALLOC, memset()}
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
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls mem_free()
///       \Calls get_errc()
///       \Calls set_errc()
TEST(CO_NmtCreate, CoNmtCreate_DcfAppParamsWriteFail) {
  const size_t NUM_SUBS = 1u;
  CreateObj(obj2000, 0x2000u);
  obj2000->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0));

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
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls mem_free()
///       \Calls get_errc()
///       \Calls set_errc()
TEST(CO_NmtCreate, CoNmtCreate_DcfCommParamsWriteFail) {
  const size_t NUM_SUBS = 1u;  // in each region
#if !LELY_NO_CO_DCF_RESTORE
  CreateObj(obj2000, 0x2000u);
  obj2000->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0));
#endif
  CreateObj(obj1000, 0x1000u);
  obj1000->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0));

#if !LELY_NO_CO_DCF_RESTORE
  LelyOverride::co_val_write(
      2u * GetCoDevWriteDcf_NullBuf_CoValWriteCalls(NUM_SUBS) +
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

  size_t GetDcfParamsAllocSize() {
    size_t dcfs_size = 0;
    dcfs_size += co_dev_write_dcf(dev, 0x1000u, 0x1fffu, nullptr, nullptr);
#if !LELY_NO_CO_DCF_RESTORE
    dcfs_size += co_dev_write_dcf(dev, 0x2000u, 0x9fffu, nullptr, nullptr);
#endif

    return dcfs_size;
  }

  static size_t GetSlavesAllocSize() {
    size_t size = 0;
#if !LELY_NO_CO_MASTER
    size += can_recv_sizeof();
#if !LELY_NO_CO_NG
    size += can_timer_sizeof();
#endif
#endif  // !LELY_NO_CO_MASTER
    return CO_NUM_NODES * size;
  }

  static size_t GetHbConsumersAllocSize(const size_t hb_num) {
    return hb_num *
           (co_nmt_hb_sizeof() + can_recv_sizeof() + can_timer_sizeof());
  }

  static size_t GetSsdoAllocSize(size_t ssdo_num = 1u) {
    return ssdo_num * (sizeof(co_ssdo_t*) + co_ssdo_sizeof() +
                       can_recv_sizeof() + can_timer_sizeof());
  }

  static size_t GetServicesAllocSize() {
    size_t size = 0;
#if LELY_NO_MALLOC && !LELY_NO_CO_SDO
    size += GetSsdoAllocSize();
#endif
    return size;
  }

  static size_t GetNmtTimersAllocSize() {
    size_t size = can_timer_sizeof();
#if !LELY_NO_CO_MASTER
    size += can_timer_sizeof();
#endif
    return size;
  }

  static size_t GetNmtRecvsAllocSize() { return 2u * can_recv_sizeof(); }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(limitedAllocator.ToAllocT());
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
///        allocator limited to only allocate the NMT service instance and DCF
///        for application parameters
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, NMT service is not created and the error
///       number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
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
///        allocator limited to only allocate the NMT service instance, DCF for
///        application parameters and DCF for communication parameters
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, NMT service is not created and the error
///       number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_nmt_srv_init()
///       \Calls co_nmt_srv_fini()
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls set_errc()
TEST(CO_NmtAllocation, CoNmtCreate_NoMemoryForDefaultServices) {
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() + GetDcfParamsAllocSize());

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
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() + GetDcfParamsAllocSize() +
                                     GetServicesAllocSize());

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
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() + GetDcfParamsAllocSize() +
                                     GetServicesAllocSize() +
                                     can_recv_sizeof());

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
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() + GetDcfParamsAllocSize() +
                                     GetServicesAllocSize() +
                                     GetNmtRecvsAllocSize());

  nmt = co_nmt_create(net, dev);

  POINTERS_EQUAL(nullptr, nmt);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

#if !LELY_NO_CO_MASTER
/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the NMT service instance, DCFs for
///        application/communication parameters, the default services instances,
///        all NMT receivers and a timer for life guarding/heartbeat production
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, NMT service is not created and the error
///       number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
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
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls set_errc()
TEST(CO_NmtAllocation, CoNmtCreate_NoMemoryForCsTimer) {
  limitedAllocator.LimitAllocationTo(
      co_nmt_sizeof() + GetDcfParamsAllocSize() + GetServicesAllocSize() +
      GetNmtRecvsAllocSize() + can_timer_sizeof());

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
      co_nmt_sizeof() + GetDcfParamsAllocSize() + GetServicesAllocSize() +
      GetNmtRecvsAllocSize() + can_timer_sizeof());

  nmt = co_nmt_create(net, dev);

  POINTERS_EQUAL(nullptr, nmt);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the NMT service instance, DCFs for
///        application/communication parameters, the default services instances,
///        all NMT receivers, a timer for life guarding/heartbeat production and
///        a timer for sending buffered NMT messages
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, NMT service is not created and the error
///       number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_nmt_alignof()
///       \Calls co_nmt_sizeof()
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
///       \Calls get_errc()
///       \Calls mem_free()
///       \Calls set_errc()
TEST(CO_NmtAllocation, CoNmtCreate_NoMemoryForNmtSlaveRecvs) {
  limitedAllocator.LimitAllocationTo(
      co_nmt_sizeof() + GetDcfParamsAllocSize() + GetServicesAllocSize() +
      GetNmtRecvsAllocSize() + GetNmtTimersAllocSize());

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
///       \Calls co_dev_get_id()
///       \Calls co_dev_write_dcf()
///       \Calls co_nmt_srv_init()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls co_dev_find_obj()
///       \IfCalls{LELY_NO_MALLOC, memset()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_buf_init()}
///       \IfCalls{!LELY_NO_CO_MASTER, can_net_get_time()}
///       \IfCalls{LELY_NO_MALLOC, co_dev_get_val_u32()}
///       \IfCalls{!LELY_NO_CO_TPDO, co_dev_set_tpdo_event_ind()}
///       \Calls co_obj_set_dn_ind()
///       \IfCalls{LELY_NO_MALLOC && !LELY_NO_CO_NMT_BOOT, co_nmt_boot_create()}
///       \IfCalls{LELY_NO_MALLOC && !LELY_NO_CO_NMT_CFG, co_nmt_cfg_create()}
TEST(CO_NmtAllocation, CoNmtCreate_ExactMemory) {
  limitedAllocator.LimitAllocationTo(
      co_nmt_sizeof() + GetDcfParamsAllocSize() + GetServicesAllocSize() +
      GetNmtRecvsAllocSize() + GetNmtTimersAllocSize() + GetSlavesAllocSize());

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
      co_nmt_sizeof() + GetDcfParamsAllocSize() + GetServicesAllocSize() +
      GetNmtRecvsAllocSize() + GetNmtTimersAllocSize() + GetSlavesAllocSize() +
      GetHbConsumersAllocSize(CO_NMT_MAX_NHB));

  nmt = co_nmt_create(net, dev);

  CHECK(nmt != nullptr);
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}
#endif

///@}

TEST_GROUP_BASE(CO_Nmt, CO_NmtBase) {
  co_nmt_t* nmt = nullptr;

  static void empty_cs_ind(co_nmt_t*, co_unsigned8_t, void*) {}
  static void empty_hb_ind(co_nmt_t*, co_unsigned8_t, int, int, void*) {}
  static void empty_st_ind(co_nmt_t*, co_unsigned8_t, co_unsigned8_t, void*) {}
  static void empty_sdo_ind(co_nmt_t*, co_unsigned8_t, co_unsigned16_t,
                            co_unsigned8_t, size_t, size_t, void*) {}
  static void empty_sync_ind(co_nmt_t*, co_unsigned8_t, void*) {}
  int data = 0;

  void CreateNmt() {
    nmt = co_nmt_create(net, dev);
    CHECK(nmt != nullptr);
  }

  TEST_SETUP() { TEST_BASE_SETUP(); }

  TEST_TEARDOWN() {
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

  CHECK_EQUAL(0, ret);
}

/// \Given a pointer to an initialized NMT service (co_nmt_t) configured as
///        a slave
///
/// \When co_nmt_is_master() is called
///
/// \Then 0 is returned
TEST(CO_Nmt, CoNmtIsMaster_Slave) {
  CreateObj1f80NmtStartup(0x00);
  CreateNmt();
  co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE);

  const auto ret = co_nmt_is_master(nmt);

  CHECK_EQUAL(0, ret);
}

#if !LELY_NO_CO_MASTER
/// \Given a pointer to an initialized NMT service (co_nmt_t) configured as
///        a master
///
/// \When co_nmt_is_master() is called
///
/// \Then 1 is returned
TEST(CO_Nmt, CoNmtIsMaster_Master) {
  CreateObj1f80NmtStartup(0x01);
  CreateNmt();
  co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE);

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
  const int timeout = 500;
  CreateNmt();

  co_nmt_set_timeout(nmt, timeout);

  CHECK_EQUAL(timeout, co_nmt_get_timeout(nmt));
}

///@}

#endif  // !LELY_NO_CO_MASTER
