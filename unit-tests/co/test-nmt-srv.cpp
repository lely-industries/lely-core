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

#include <include/lely/co/nmt.h>
#include <include/lely/util/error.h>

#include <libtest/tools/lely-unit-test.hpp>
#include <libtest/allocators/default.hpp>
#include <libtest/allocators/limited.hpp>

#include <lely/co/rpdo.h>
#include <lely/co/tpdo.h>

#include "holder/dev.hpp"

#include "obj-init/tpdo-map-par.hpp"
#include "obj-init/tpdo-comm-par.hpp"
#include "obj-init/rpdo-map-par.hpp"
#include "obj-init/rpdo-comm-par.hpp"

#include "common/nmt-alloc-sizes.hpp"

TEST_GROUP(CO_NmtSrv) {
  static const co_unsigned8_t DEV_ID = 0x01u;
  static const co_unsigned16_t RPDO_NUM = 1u;
  static const co_unsigned16_t TPDO_NUM = 1u;

  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1400;
  std::unique_ptr<CoObjTHolder> obj1600;
  std::unique_ptr<CoObjTHolder> obj1800;
  std::unique_ptr<CoObjTHolder> obj1a00;

  co_dev_t* dev = nullptr;
  can_net_t* net = nullptr;
  can_net_t* fail_net = nullptr;
  co_nmt_t* nmt = nullptr;

  Allocators::Default allocator;
  Allocators::Limited limitedAllocator;

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(allocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    fail_net = can_net_create(limitedAllocator.ToAllocT(), 0);
    CHECK(fail_net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);
  }

  void CreateObj1400Defaults() {
    dev_holder->CreateObj<Obj1400RpdoCommPar>(obj1400);
    obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
    obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub01CobId>(0);
    obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub02TransmissionType>(0xfeu);
  }

  void CreateObj1600Defaults() {
    dev_holder->CreateObj<Obj1600RpdoMapPar>(obj1600);
    obj1600->EmplaceSub<Obj1600RpdoMapPar::Sub00NumOfMappedObjs>(0x01u);
    obj1600->EmplaceSub<Obj1600RpdoMapPar::SubNthAppObject>(0x01u, 0);
  }

  void CreateObj1800Defaults() {
    dev_holder->CreateObj<Obj1800TpdoCommPar>(obj1800);
    obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
    obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(0);
    obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0);
  }

  void CreateObj1a00Defaults() {
    dev_holder->CreateObj<Obj1a00TpdoMapPar>(obj1a00);
    obj1a00->EmplaceSub<Obj1a00TpdoMapPar::Sub00NumOfMappedObjs>(0x01u);
    obj1a00->EmplaceSub<Obj1a00TpdoMapPar::SubNthAppObject>(0x01u, 0);
  }

  TEST_TEARDOWN() {
    co_nmt_destroy(nmt);

    dev_holder.reset();
    can_net_destroy(fail_net);
    can_net_destroy(net);
    set_errnum(0);
  }
};

/// @name co_nmt_srv_init()
///@{

// TODO(N7S) add some tests to run in !LELY_NO_MALLOC
TEST(CO_NmtSrv, Dummy) {
  // this is a dummy test case to solve the container errors about no tests
}

#if LELY_NO_MALLOC
/// \Given a pointer to the network (can_net_t), a pointer to a device
///        (co_dev_t) with all services configured
///
/// \When NMT service is created
///
/// \Then the configured services are initialized
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls mem_alloc()
///       \Calls co_rpdo_create()
///       \Calls co_rpdo_set_err()
///       \Calls co_tpdo_create()
TEST(CO_NmtSrv, CoNmtSrvInit_Nominal) {
#if !LELY_NO_CO_RPDO
  CreateObj1400Defaults();
  CreateObj1600Defaults();
#endif
#if !LELY_NO_CO_TPDO
  CreateObj1800Defaults();
  CreateObj1a00Defaults();
#endif

  // TODO(N7S) add objects for other services and verify if they were created
  //           by the service manager (also add \Calls)

  nmt = co_nmt_create(net, dev);

#if LELY_NO_CO_RPDO
  POINTERS_EQUAL(nullptr, co_nmt_get_rpdo(nmt, RPDO_NUM));
#else
  const co_rpdo_t* const rpdo = co_nmt_get_rpdo(nmt, RPDO_NUM);
  CHECK(rpdo != nullptr);
  POINTERS_EQUAL(net, co_rpdo_get_net(rpdo));
  POINTERS_EQUAL(dev, co_rpdo_get_dev(rpdo));
  CHECK_EQUAL(1, co_rpdo_is_stopped(rpdo));
  CHECK_EQUAL(RPDO_NUM, co_rpdo_get_num(rpdo));
#endif  // LELY_NO_CO_RPDO

#if LELY_NO_CO_TPDO
  POINTERS_EQUAL(nullptr, co_nmt_get_tpdo(nmt, TPDO_NUM));
#else
  const co_tpdo_t* const tpdo = co_nmt_get_tpdo(nmt, TPDO_NUM);
  CHECK(tpdo != nullptr);
  POINTERS_EQUAL(net, co_tpdo_get_net(tpdo));
  POINTERS_EQUAL(dev, co_tpdo_get_dev(tpdo));
  CHECK_EQUAL(1, co_tpdo_is_stopped(tpdo));
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));
#endif  // LELY_NO_CO_TPDO
  // TODO(N7S) add checks for other services here
}

#if !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO
/// \Given a pointer to the NMT service manager (co_nmt_srv), a device with PDO
///        service(s) configured; the allocator has not enough memory for PDO
///        services
///
/// \When co_nmt_create() is called with a pointer to the device
///
/// \Then a null pointer is returned
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls mem_alloc()
///       \Calls diag()
///       \Calls mem_free()
TEST(CO_NmtSrv, CoNmtSrvInit_FailPdoAllocation) {
#if !LELY_NO_CO_RPDO
  CreateObj1400Defaults();
  CreateObj1600Defaults();
#endif
#if !LELY_NO_CO_TPDO
  CreateObj1800Defaults();
  obj1800->SetSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xfe);
  CreateObj1a00Defaults();
#endif

  const size_t dcf_app_par_size = NmtCommon::GetDcfParamsAllocSize(dev);
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() + dcf_app_par_size);

  nmt = co_nmt_create(fail_net, dev);

  POINTERS_EQUAL(nullptr, nmt);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}
#endif  // !LELY_NO_CO_RPDO || !LELY_NO_CO_TPDO

#endif  // LELY_NO_MALLOC

///@}
