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
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/sub.hpp"

struct co_nmt_hb;
typedef struct co_nmt_hb co_nmt_hb_t;
#include <lib/co/nmt_hb.h>

#if !LELY_NO_CO_NMT_BOOT
struct co_nmt_boot;
typedef struct co_nmt_boot co_nmt_boot_t;
#include <lib/co/nmt_boot.h>
#endif

#if !LELY_NO_CO_NMT_CFG
struct co_nmt_cfg;
typedef struct co_nmt_cfg co_nmt_cfg_t;
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
  std::unique_ptr<CoObjTHolder> obj1016;

  Allocators::Default allocator;

  void CreateObj(std::unique_ptr<CoObjTHolder> & obj_holder,
                 co_unsigned16_t idx) {
    obj_holder.reset(new CoObjTHolder(idx));
    CHECK(obj_holder->Get() != nullptr);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));
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

  TEST_TEARDOWN() {
    co_nmt_destroy(nmt);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_emcy_create()
///@{

/// \Given initialized device (co_dev_t) and network (can_net_t)
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a pointer to a created NMT service is returned, the service is
///       configured with default values
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

  POINTERS_EQUAL(net, co_nmt_get_net(nmt));
  POINTERS_EQUAL(dev, co_nmt_get_dev(nmt));

  void* pdata;
  co_nmt_cs_ind_t* cs_ind;
  co_nmt_get_cs_ind(nmt, &cs_ind, &pdata);
  FUNCTIONPOINTERS_EQUAL(nullptr, cs_ind);
  FUNCTIONPOINTERS_EQUAL(nullptr, pdata);

  co_nmt_hb_ind_t* hb_ind;
  co_nmt_get_hb_ind(nmt, &hb_ind, &pdata);
  CHECK(hb_ind != nullptr);
  FUNCTIONPOINTERS_EQUAL(nullptr, pdata);

  co_nmt_st_ind_t* st_ind;
  co_nmt_get_st_ind(nmt, &st_ind, &pdata);
  CHECK(st_ind != nullptr);
  FUNCTIONPOINTERS_EQUAL(nullptr, pdata);

#if !LELY_NO_CO_MASTER
  co_nmt_sdo_ind_t* dn_ind;
  co_nmt_get_dn_ind(nmt, &dn_ind, &pdata);
  FUNCTIONPOINTERS_EQUAL(nullptr, dn_ind);
  FUNCTIONPOINTERS_EQUAL(nullptr, pdata);

  co_nmt_sdo_ind_t* up_ind;
  co_nmt_get_up_ind(nmt, &up_ind, &pdata);
  FUNCTIONPOINTERS_EQUAL(nullptr, up_ind);
  FUNCTIONPOINTERS_EQUAL(nullptr, pdata);
#endif

  co_nmt_sync_ind_t* sync_ind;
  co_nmt_get_sync_ind(nmt, &sync_ind, &pdata);
  FUNCTIONPOINTERS_EQUAL(nullptr, sync_ind);
  FUNCTIONPOINTERS_EQUAL(nullptr, pdata);

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

///@}

TEST_GROUP_BASE(CO_NmtAllocation, CO_NmtBase) {
  Allocators::Limited limitedAllocator;
  co_nmt_t* nmt = nullptr;

  void CreateObj1016ConsumerHbTimeN(const size_t num) {
    assert(num <= CO_NMT_MAX_NHB);
    // object 0x1016 - Consumer heartbeat time
    CreateObj(obj1016, 0x1016u);

    // 0x00 - Highest sub-index supported
    obj1016->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(num));
    // 0x01-0x7f - Consumer heartbeat time
    for (co_unsigned8_t i = 0; i < num; ++i) {
      obj1016->InsertAndSetSub(
          i + 1, CO_DEFTYPE_UNSIGNED32,
          co_unsigned32_t(co_unsigned32_t(SLAVE_DEV_ID) << 16u) |
              co_unsigned32_t(0x0001));  //  1 ms
    }
  }

  size_t GetDcfParamsAllocSize() {
    size_t dcfs_size = 0;
    dcfs_size += co_dev_write_dcf(dev, 0x1000, 0x1fff, nullptr, nullptr);
#if !LELY_NO_CO_DCF_RESTORE
    dcfs_size += co_dev_write_dcf(dev, 0x2000, 0x9fff, nullptr, nullptr);
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

/// @name co_emcy_create()
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
///        the OD contains the Consumer Heartbeat Time object (0x1016) with at
///        least one entry
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
///       \Calls can_net_get_time()
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
///        required objects; the OD contains the Consumer Heartbeat Time object
///        (0x1016) with the maximum number of entries
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
///       \Calls can_net_get_time()
///       \IfCalls{LELY_NO_MALLOC, co_dev_get_val_u32()}
///       \IfCalls{!LELY_NO_CO_TPDO, co_dev_set_tpdo_event_ind()}
///       \Calls co_obj_set_dn_ind()
///       \IfCalls{LELY_NO_MALLOC && !LELY_NO_CO_NMT_BOOT, co_nmt_boot_create()}
///       \IfCalls{LELY_NO_MALLOC && !LELY_NO_CO_NMT_CFG, co_nmt_cfg_create()}
TEST(CO_NmtAllocation, CoNmtCreate_ExactMemory_WithObj1016) {
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
