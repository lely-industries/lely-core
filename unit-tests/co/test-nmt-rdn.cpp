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

#include <lely/co/nmt.h>
#include <lely/util/time.h>
#include <lib/co/nmt_rdn.h>

#include <libtest/allocators/default.hpp>
#include <libtest/allocators/limited.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>
#include <libtest/tools/co-nmt-rdn-ind.hpp>
#include <libtest/tools/co-nmt-hb-ind.hpp>

#include "holder/dev.hpp"

#include "obj-init/nmt-hb-consumer.hpp"
#include "obj-init/nmt-redundancy.hpp"
#include "obj-init/nmt-startup.hpp"

TEST_BASE(CO_NmtRdnBase) {
  TEST_BASE_SUPER(CO_NmtRdnBase);

  static const co_unsigned8_t DEV_ID = 0x02u;
  static const co_unsigned8_t MASTER_DEV_ID = 0x01u;

  can_net_t* net = nullptr;
  co_dev_t* dev = nullptr;

  std::unique_ptr<CoDevTHolder> dev_holder;

  std::unique_ptr<CoObjTHolder> obj_rdn;
  std::unique_ptr<CoObjTHolder> obj1016;
  std::unique_ptr<CoObjTHolder> obj1f80;

  static const co_unsigned8_t BUS_A_ID = 0x00;
  static const co_unsigned8_t BUS_B_ID = 0x01;

  static const co_unsigned16_t HB_TIMEOUT_MS = 550u;
  static const co_unsigned8_t BDEFAULT = BUS_A_ID;
  static const co_unsigned8_t TTOGGLE = 3u;
  static const co_unsigned8_t NTOGGLE = 5u;
  static const co_unsigned8_t INIT_CTOGGLE = 0u;
  Allocators::Default allocator;

  void ConfigRdnMasterHb() {
    dev_holder->CreateObj<Obj1016ConsumerHb>(obj1016);
    obj1016->EmplaceSub<Obj1016ConsumerHb::Sub00HighestSubidxSupported>(0x01u);
    obj1016->EmplaceSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(
        Obj1016ConsumerHb::MakeHbConsumerEntry(MASTER_DEV_ID, HB_TIMEOUT_MS));
  }

  void ConfigRdn() {
    dev_holder->CreateObj<ObjNmtRedundancy>(obj_rdn);
    obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub00HighestSubidxSupported>();
    obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub01Bdefault>(BDEFAULT);
    obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub02Ttoggle>(TTOGGLE);
    obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub03Ntoggle>(NTOGGLE);
    obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub04Ctoggle>(INIT_CTOGGLE);

    ConfigRdnMasterHb();
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
    CoNmtRdnInd::Clear();
    CoNmtHbInd::Clear();
    CanSend::Clear();

    dev_holder.reset();
    can_net_destroy(net);
    set_errnum(0);
  }
};

TEST_GROUP_BASE(CO_NmtRdnCheck, CO_NmtRdnBase){};

/// @name co_nmt_rdn_chk_dev()
///@{

/// \Given an initialized device (co_dev_t), the object dictionary contains
///        a correct NMT redundancy object with not entries
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 1 is returned
TEST(CO_NmtRdnCheck, CoNmtRdnChkDev_Nominal) {
  ConfigRdn();

  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(1, ret);
}

/// \Given an initialized device (co_dev_t), the object dictionary does not
///        contain the NMT redundancy object
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 1 is returned
TEST(CO_NmtRdnCheck, CoNmtRdnChkDev_NoObject) {
  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(1, ret);
}

/// \Given an initialized device (co_dev_t), the object dictionary contains
///        the NMT redundancy object which does not have the first entry with
///        the highest sub-index supported
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 0 is returned
TEST(CO_NmtRdnCheck, CoNmtRdnChkDev_NoSub00MaxSubidx) {
  dev_holder->CreateObj<ObjNmtRedundancy>(obj_rdn);

  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(0, ret);
}

/// \Given an initialized device (co_dev_t), the object dictionary contains
///        the NMT redundancy object, but the mandatory first entry with the
///        highest sub-index supported has an incorrect data type
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 0 is returned
TEST(CO_NmtRdnCheck, CoNmtRdnChkDev_Sub00MaxSubidx_BadType) {
  dev_holder->CreateObj<ObjNmtRedundancy>(obj_rdn);
  obj_rdn->InsertAndSetSub(0x00, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t{0});

  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(0, ret);
}

/// \Given an initialized device (co_dev_t), the object dictionary contains
///        the NMT redundancy object which does not have the mandatory Bdefault
///        entry
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 0 is returned
TEST(CO_NmtRdnCheck, CoNmtRdnChkDev_NoSub01Bdefault) {
  dev_holder->CreateObj<ObjNmtRedundancy>(obj_rdn);
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub00HighestSubidxSupported>();

  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(0, ret);
}

/// \Given an initialized device (co_dev_t), the object dictionary contains
///        the NMT redundancy object, but the Bdefault entry has an incorrect
///        data type
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 0 is returned
TEST(CO_NmtRdnCheck, CoNmtRdnChkDev_Sub01Bdefault_BadType) {
  dev_holder->CreateObj<ObjNmtRedundancy>(obj_rdn);
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub00HighestSubidxSupported>();
  obj_rdn->InsertAndSetSub(CO_NMT_RDN_BDEFAULT_SUBIDX, CO_DEFTYPE_UNSIGNED32,
                           co_unsigned32_t{0});

  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(0, ret);
}

/// \Given an initialized device (co_dev_t), the object dictionary contains
///        the NMT redundancy object which does not have the Ttoggle entry
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 1 is returned
TEST(CO_NmtRdnCheck, CoNmtRdnChkDev_NoSub02Ttoggle) {
  dev_holder->CreateObj<ObjNmtRedundancy>(obj_rdn);
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub00HighestSubidxSupported>();
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub01Bdefault>();

  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(1, ret);
}

/// \Given an initialized device (co_dev_t), the object dictionary contains
///        the NMT redundancy object, but the Ttoggle entry has an incorrect
///        data type
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 0 is returned
TEST(CO_NmtRdnCheck, CoNmtRdnChkDev_Sub02Ttoggle_BadType) {
  dev_holder->CreateObj<ObjNmtRedundancy>(obj_rdn);
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub00HighestSubidxSupported>();
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub01Bdefault>();
  obj_rdn->InsertAndSetSub(CO_NMT_RDN_TTOGGLE_SUBIDX, CO_DEFTYPE_UNSIGNED32,
                           co_unsigned32_t{0});

  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(0, ret);
}

/// \Given an initialized device (co_dev_t), the object dictionary contains
///        the NMT redundancy object which does not have the Ntoggle entry
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 1 is returned
TEST(CO_NmtRdnCheck, CoNmtRdnChkDev_NoSub03Ntoggle) {
  dev_holder->CreateObj<ObjNmtRedundancy>(obj_rdn);
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub00HighestSubidxSupported>();
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub01Bdefault>();
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub02Ttoggle>();

  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(1, ret);
}

/// \Given an initialized device (co_dev_t), the object dictionary contains
///        the NMT redundancy object, but the Ntoggle entry has an incorrect
///        data type
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 0 is returned
TEST(CO_NmtRdnCheck, CoNmtRdnChkDev_Sub03Ntoggle_BadType) {
  dev_holder->CreateObj<ObjNmtRedundancy>(obj_rdn);
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub00HighestSubidxSupported>();
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub01Bdefault>();
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub02Ttoggle>();
  obj_rdn->InsertAndSetSub(CO_NMT_RDN_NTOGGLE_SUBIDX, CO_DEFTYPE_UNSIGNED32,
                           co_unsigned32_t{0});

  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(0, ret);
}

/// \Given an initialized device (co_dev_t), the object dictionary contains
///        the NMT redundancy object which does not have the Ctoggle entry
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 1 is returned
TEST(CO_NmtRdnCheck, CoNmtRdnChkDev_NoSub04Ctoggle) {
  dev_holder->CreateObj<ObjNmtRedundancy>(obj_rdn);
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub00HighestSubidxSupported>();
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub01Bdefault>();
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub02Ttoggle>();
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub03Ntoggle>();

  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(1, ret);
}

/// \Given an initialized device (co_dev_t), the object dictionary contains
///        the NMT redundancy object, but the Ctoggle entry has an incorrect
///        data type
///
/// \When co_nmt_rdn_chk_dev() is called with the pointer to the device
///
/// \Then 0 is returned
TEST(CO_NmtRdnCheck, CoNmtRdnChkDev_Sub04Ctoggle_BadType) {
  dev_holder->CreateObj<ObjNmtRedundancy>(obj_rdn);
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub00HighestSubidxSupported>();
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub01Bdefault>();
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub02Ttoggle>();
  obj_rdn->EmplaceSub<ObjNmtRedundancy::Sub03Ntoggle>();
  obj_rdn->InsertAndSetSub(CO_NMT_RDN_CTOGGLE_SUBIDX, CO_DEFTYPE_UNSIGNED32,
                           co_unsigned32_t{0});

  const auto ret = co_nmt_rdn_chk_dev(dev);

  CHECK_EQUAL(0, ret);
}

///@}

TEST_GROUP_BASE(CO_NmtRdnCreate, CO_NmtRdnBase) {
  co_nmt_t* nmt = nullptr;
  co_nmt_rdn_t* rdn = nullptr;

  TEST_SETUP() {
    TEST_BASE_SETUP();
    nmt = co_nmt_create(net, dev);
    CHECK(nmt != nullptr);
  }

  TEST_TEARDOWN() {
    co_nmt_rdn_destroy(rdn);
    co_nmt_destroy(nmt);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_nmt_rdn_sizeof()
///@{

/// \Given N/A
///
/// \When co_nmt_rdn_sizeof() is called
///
/// \Then the size of the NMT redundancy manager service object is returned
TEST(CO_NmtRdnCreate, CoNmtRdnSizeof_Nominal) {
  const auto ret = co_nmt_rdn_sizeof();

  CHECK_EQUAL(40u, ret);
}

///@}

/// @name co_nmt_rdn_alignof()
///@{

/// \Given N/A
///
/// \When co_nmt_rdn_alignof() is called
///
/// \Then the platform-dependent alignment of the NMT redundancy manager
///       service object is returned
TEST(CO_NmtRdnCreate, CoNmtRdnAlignof_Nominal) {
  const auto ret = co_nmt_rdn_alignof();

#if defined(__MINGW32__) && !defined(__MINGW64__)
  CHECK_EQUAL(4u, ret);
#else
  CHECK_EQUAL(8u, ret);
#endif
}

///@}

/// @name co_nmt_rdn_create()
///@{

/// \Given an initialized network (can_net_t) and NMT service (co_nmt_t)
///
/// \When co_nmt_rdn_create() is called with pointers to the network and the
///       service
///
/// \Then a pointer to a created NMT redundancy manager service is returned
TEST(CO_NmtRdnCreate, CoNmtRdnCreate_Default) {
  rdn = co_nmt_rdn_create(net, nmt);

  CHECK(rdn != nullptr);

  POINTERS_EQUAL(can_net_get_alloc(net), co_nmt_rdn_get_alloc(rdn));
  CHECK_EQUAL(0, co_nmt_rdn_get_master_id(rdn));
}

///@}

/// @name co_nmt_rdn_destroy()
///@{

/// \Given N/A
///
/// \When co_nmt_rdn_destroy() is called with a null NMT redundancy manager
///       service pointer
///
/// \Then nothing is changed
TEST(CO_NmtRdnCreate, CoNmtRdnDestroy_Null) { co_nmt_rdn_destroy(nullptr); }

/// \Given an initialized NMT redundancy manager service (co_nmt_rdn_t)
///
/// \When co_nmt_rdn_destroy() is called with a pointer to the service
///
/// \Then the service is finalized and freed
TEST(CO_NmtRdnCreate, CoNmtRdnDestroy_Nominal) {
  rdn = co_nmt_rdn_create(net, nmt);
  CHECK(rdn != nullptr);

  co_nmt_rdn_destroy(rdn);
  rdn = nullptr;
}

///@}

TEST_GROUP_BASE(CO_NmtRdnAllocation, CO_NmtRdnBase) {
  Allocators::Limited limitedAllocator;
  co_nmt_t* nmt = nullptr;
  co_nmt_rdn_t* rdn = nullptr;

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(limitedAllocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    nmt = co_nmt_create(net, dev);
    CHECK(nmt != nullptr);
  }

  TEST_TEARDOWN() {
    co_nmt_rdn_destroy(rdn);
    co_nmt_destroy(nmt);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_nmt_rdn_create()
///@{

/// \Given an initialized network (can_net_t) and NMT service (co_nmt_t), the
///        network has a memory allocator limited to only allocate an NMT
///        redundancy manager service instance
///
/// \When co_nmt_rdn_create() is called with pointers to the network and the
///       service
///
/// \Then a null pointer is returned, the NMT redundancy manager service is not
///       created and the error number is set to ERRNUM_NOMEM
TEST(CO_NmtRdnAllocation, CoNmtRdnCreate_InitAllocationFailed) {
  limitedAllocator.LimitAllocationTo(co_nmt_rdn_sizeof());
  rdn = co_nmt_rdn_create(net, nmt);

  POINTERS_EQUAL(nullptr, rdn);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given an initialized network (can_net_t) and NMT service (co_nmt_t), the
///        network has a memory allocator limited to exactly allocate an NMT
///        redundancy service instance and all required objects
///
/// \When co_nmt_rdn_create() is called with pointers to the network and the
///       service
///
/// \Then a pointer to a created NMT redundancy manager service is returned
TEST(CO_NmtRdnAllocation, CoNmtRdnCreate_ExactMemory) {
  limitedAllocator.LimitAllocationTo(co_nmt_rdn_sizeof() + can_timer_sizeof());
  rdn = co_nmt_rdn_create(net, nmt);

  CHECK(rdn != nullptr);
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

///@}

TEST_GROUP_BASE(CO_NmtRdn, CO_NmtRdnBase) {
  co_nmt_t* nmt = nullptr;

  int data = 0;

  can_msg CreateHbMsg(const co_unsigned8_t id, const co_unsigned8_t st) const {
    can_msg msg = CAN_MSG_INIT;
    msg.id = CO_NMT_EC_CANID(id);
    msg.len = 1u;
    msg.data[0] = st;

    return msg;
  }

  void CreateNmt() {
    nmt = co_nmt_create(net, dev);
    CHECK(nmt != nullptr);
  }

  void CreateNmtAndReset() {
    CreateNmt();
    CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));
  }

  TEST_TEARDOWN() {
    co_nmt_destroy(nmt);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_nmt_get_ecss_rdn_ind()
///@{

/// \Given a pointer to the NMT service (co_nmt_t)
///
/// \When co_nmt_get_ecss_rdn_ind() is called with no addresses to store the
///       indication function and user-specified data pointers at
///
/// \Then nothing is changed
TEST(CO_NmtRdn, CoNmtGetEcssRdnInd_NoMemoryToStoreResults) {
  CreateNmt();

  co_nmt_get_ecss_rdn_ind(nmt, nullptr, nullptr);
}

/// \Given a pointer to the NMT service (co_nmt_t)
///
/// \When co_nmt_get_ecss_rdn_ind() is called with an address to store the
///       indication function pointer and an address to store user-specified
///       data pointer
///
/// \Then both pointers are set to a null pointer
TEST(CO_NmtRdn, CoNmtGetEcssRdnInd_Defaults) {
  CreateNmt();

  co_nmt_ecss_rdn_ind_t* ind = CoNmtRdnInd::Func;
  void* rdn_data = &data;

  co_nmt_get_ecss_rdn_ind(nmt, &ind, &rdn_data);

  POINTERS_EQUAL(nullptr, ind);
  POINTERS_EQUAL(nullptr, rdn_data);
}

///@}

/// @name co_nmt_set_ecss_rdn_ind()
///@{

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_set_ecss_rdn_ind() is called with a pointer to an indication
///       function and a pointer to user-specified data
///
/// \Then the indication function and the user-specified data pointers are set
///       in the NMT service
TEST(CO_NmtRdn, CoNmtSetRdnInd_Nominal) {
  CreateNmt();

  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, &data);

  co_nmt_ecss_rdn_ind_t* ind = nullptr;
  void* rdn_data = nullptr;
  co_nmt_get_ecss_rdn_ind(nmt, &ind, &rdn_data);
  FUNCTIONPOINTERS_EQUAL(CoNmtRdnInd::Func, ind);
  POINTERS_EQUAL(&data, rdn_data);
}

/// \Given a pointer to an initialized NMT service (co_nmt_t)
///
/// \When co_nmt_set_ecss_rdn_ind() is called with a null indication function
///       pointer and any user-specified data pointer
///
/// \Then both pointers are set to a null pointer
TEST(CO_NmtRdn, CoNmtSetRdnInd_Null) {
  CreateNmt();
  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, &data);

  co_nmt_set_ecss_rdn_ind(nmt, nullptr, nullptr);

  co_nmt_ecss_rdn_ind_t* ind = nullptr;
  void* rdn_data = nullptr;
  co_nmt_get_ecss_rdn_ind(nmt, &ind, &rdn_data);
  POINTERS_EQUAL(nullptr, ind);
  POINTERS_EQUAL(nullptr, rdn_data);
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
  CreateNmt();

  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);

  co_nmt_ecss_rdn_ind(nmt, BUS_B_ID, CO_NMT_ECSS_RDN_BUS_SWITCH);

  CHECK_EQUAL(1u, CoNmtRdnInd::GetNumCalled());
  CoNmtRdnInd::Check(nmt, BUS_B_ID, CO_NMT_ECSS_RDN_BUS_SWITCH, nullptr);
}

/// \Given a pointer to the NMT service (co_nmt_t) with no NMT redundancy
///        indication function set
///
/// \When co_nmt_ecss_rdn_ind() is called with a pointer to the NMT service
///       any bus ID and any reason
///
/// \Given nothing is changed
TEST(CO_NmtRdn, CoNmtRdnInd_NoRdnInd) {
  CreateNmt();

  co_nmt_ecss_rdn_ind(nmt, 0, 0);

  CHECK_EQUAL(0, CoNmtRdnInd::GetNumCalled());
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
  dev_holder->CreateObjValue<Obj1f80NmtStartup>(obj1f80,
                                                Obj1f80NmtStartup::MASTER_BIT);
  CreateNmtAndReset();

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
  CreateNmtAndReset();

  const auto ret = co_nmt_set_active_bus(nmt, BUS_B_ID);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_PERM, get_errnum());
  CHECK_EQUAL(0, co_nmt_get_active_bus_id(nmt));
}

///@}

#endif  // !LELY_NO_CO_MASTER

/// @name NMT redundancy manager service initialization
///@{

/// \Given an initialized network (can_net_t) and device (co_dev_t), the object
///        dictionary contains a malformed Redundancy Object without any
///        sub-objects
///
/// \When co_nmt_create() is called with pointers to the network and the device
///
/// \Then a null pointer is returned, an NMT service is not created and the
///       error number is set to ERRNUM_INVAL
TEST(CO_NmtRdn, CoNmtRdnInit_InvalidRdnObject) {
  dev_holder->CreateObj<ObjNmtRedundancy>(obj_rdn);

  nmt = co_nmt_create(net, dev);

  POINTERS_EQUAL(nullptr, nmt);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

#if !LELY_NO_CO_MASTER
/// \Given an initialized NMT service (co_nmt_t), the object dictionary
///        contains a Redundancy Object, but the first sub-object (Highest
///        sub-index supported) value is set to 0; the node is configured as
///        NMT master
///
/// \When the node is reset with the NMT service RESET NODE
///
/// \Then the NMT service is started, but the active bus is not set to Bdefault
///       value
TEST(CO_NmtRdn, CoNmtRdnInit_Master_NoBdefault) {
  ConfigRdn();
  dev_holder->CreateObjValue<Obj1f80NmtStartup>(obj1f80,
                                                Obj1f80NmtStartup::MASTER_BIT);
  CreateNmt();
  obj_rdn->SetSub<ObjNmtRedundancy::Sub00HighestSubidxSupported>(0);
  obj_rdn->SetSub<ObjNmtRedundancy::Sub01Bdefault>(BUS_B_ID);

  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));

  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
  CHECK_EQUAL(0, co_nmt_get_active_bus_id(nmt));
}
#endif

/// \Given an initialized NMT service (co_nmt_t), the object dictionary
///        contains a Redundancy Object, but the first sub-object (Highest
///        sub-index supported) value is set to 0; the node is configured as
///        NMT slave
///
/// \When the node is reset with the NMT service RESET NODE
///
/// \Then the NMT service is started, but the NMT Redundancy manager service is
///       disabled and the active bus is not set to Bdefault
TEST(CO_NmtRdn, CoNmtRdnInit_Slave_IncompleteRdnObject) {
  ConfigRdn();
  obj_rdn->SetSub<ObjNmtRedundancy::Sub00HighestSubidxSupported>(0);
  obj_rdn->SetSub<ObjNmtRedundancy::Sub01Bdefault>(BUS_B_ID);
  CreateNmt();

  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));

  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
  CHECK_EQUAL(0, co_nmt_get_active_bus_id(nmt));

  timespec ts = {0, 0};
  timespec_add_msec(&ts, HB_TIMEOUT_MS * TTOGGLE);
  can_net_set_time(net, &ts);

  CHECK_EQUAL(0, CoNmtHbInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtRdnInd::GetNumCalled());
}

/// \Given an initialized NMT service (co_nmt_t), the object dictionary
///        contains a Redundancy Object, but no Redundancy Master Heartbeat
///        Consumer entry; the node is configured as NMT slave
///
/// \When the node is reset with the NMT service RESET NODE
///
/// \Then the NMT service is started, but the NMT Redundancy manager service is
///       disabled and the active bus is not set to Bdefault
TEST(CO_NmtRdn, CoNmtRdnInit_Slave_NoMasterHbEntry) {
  ConfigRdn();
  obj_rdn->SetSub<ObjNmtRedundancy::Sub01Bdefault>(BUS_B_ID);
  obj1016->SetSub<Obj1016ConsumerHb::Sub00HighestSubidxSupported>(0);
  CreateNmt();

  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));

  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
  CHECK_EQUAL(0, co_nmt_get_active_bus_id(nmt));

  timespec ts = {0, 0};
  timespec_add_msec(&ts, HB_TIMEOUT_MS * TTOGGLE);
  can_net_set_time(net, &ts);

  CHECK_EQUAL(0, CoNmtHbInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtRdnInd::GetNumCalled());
}

#if !LELY_NO_CO_MASTER
/// \Given an initialized NMT service (co_nmt_t), the object dictionary
///        contains a Redundancy Object; the node is configured as NMT master
///
/// \When the node is reset with the NMT service RESET NODE
///
/// \Then the NMT service is started, the active bus is set to Bdefault value
TEST(CO_NmtRdn, CoNmtRdnInit_Master_Nominal) {
  ConfigRdn();
  obj_rdn->SetSub<ObjNmtRedundancy::Sub01Bdefault>(BUS_B_ID);
  dev_holder->CreateObjValue<Obj1f80NmtStartup>(obj1f80,
                                                Obj1f80NmtStartup::MASTER_BIT);
  CreateNmt();

  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));

  CHECK_EQUAL(CO_NMT_ST_START, co_nmt_get_st(nmt));
  CHECK_EQUAL(BUS_B_ID, co_nmt_get_active_bus_id(nmt));
}
#endif

/// \Given an initialized NMT service (co_nmt_t), the object dictionary
///        contains a Redundancy Object; the node is configured as NMT slave
///
/// \When the node is reset with the NMT service RESET NODE
///
/// \Then the NMT service is in the NMT pre-operational state, the active bus
///       is set to Bdefault value
TEST(CO_NmtRdn, CoNmtRdnInit_Slave_Nominal) {
  ConfigRdn();
  obj_rdn->SetSub<ObjNmtRedundancy::Sub01Bdefault>(BUS_B_ID);
  CreateNmt();

  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));

  CHECK_EQUAL(CO_NMT_ST_PREOP, co_nmt_get_st(nmt));
  CHECK_EQUAL(BUS_B_ID, co_nmt_get_active_bus_id(nmt));
}

///@}

/// @name NMT slave master's heartbeat processing
///@{

/// \Given a started NMT service (co_nmt_t) configured as NMT slave with the
///        NMT Redundancy manager service enabled; the bus selection process is
///        not active
///
/// \When the node receives NMT heartbeat message with a state change
///       information from the Redundancy Master
///
/// \Then the active bus is not switched, the NMT redundancy indication
///       function is not invoked, the NMT heartbeat indication function is
///       invoked with the Redundancy Master's Node-ID, CO_NMT_EC_OCCURRED
///       state, CO_NMT_EC_STATE reason and a null user-specified data pointer
TEST(CO_NmtRdn, CoNmtRdnSlaveOnMasterHb_MasterStateChange) {
  ConfigRdn();
  CreateNmtAndReset();
  co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, nullptr);
  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);
  co_nmt_set_alternate_bus_id(nmt, BUS_B_ID);

  can_msg msg = CreateHbMsg(MASTER_DEV_ID, CO_NMT_ST_START);
  CHECK_EQUAL(1, can_net_recv(net, &msg, BUS_A_ID));
  CoNmtHbInd::Clear();

  msg = CreateHbMsg(MASTER_DEV_ID, CO_NMT_ST_STOP);
  CHECK_EQUAL(1, can_net_recv(net, &msg, BUS_A_ID));

  CHECK_EQUAL(BUS_A_ID, co_nmt_get_active_bus_id(nmt));
  CHECK_EQUAL(0u, CoNmtRdnInd::GetNumCalled());
  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, MASTER_DEV_ID, CO_NMT_EC_OCCURRED, CO_NMT_EC_STATE,
                    nullptr);
}

///@}

/// @name NMT slave bus selection process
///@{

/// \Given a started NMT service (co_nmt_t) configured as NMT slave with the
///        NMT redundancy manager configured; the initial bus selection process
///        is active
///
/// \When an NMT heartbeat message from the Redundancy Master is received
///
/// \Then the active bus is not switched, the NMT redundancy indication
///       function is not invoked, the NMT heartbeat indication function is
///       invoked with the Redundancy Master's Node-ID, CO_NMT_EC_OCCURRED
///       state, CO_NMT_EC_STATE reason and a null user-specified data pointer
TEST(CO_NmtRdn, CoNmtRdnSlaveBusSelection_InitToNormalOperation) {
  ConfigRdn();
  CreateNmtAndReset();
  co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, nullptr);
  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);
  co_nmt_set_alternate_bus_id(nmt, BUS_B_ID);

  const can_msg msg = CreateHbMsg(MASTER_DEV_ID, CO_NMT_ST_START);
  CHECK_EQUAL(1, can_net_recv(net, &msg, BUS_A_ID));

  CHECK_EQUAL(BUS_A_ID, co_nmt_get_active_bus_id(nmt));
  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, MASTER_DEV_ID, CO_NMT_EC_OCCURRED, CO_NMT_EC_STATE,
                    nullptr);
  CHECK_EQUAL(0u, CoNmtRdnInd::GetNumCalled());
}

/// \Given a started NMT service (co_nmt_t) configured as NMT slave with the
///        NMT redundancy manager configured; the bus selection process is not
///        active
///
/// \When the NMT consumer heartbeat timer for the Redundancy Master expires
///
/// \Then the active bus is not switched, the NMT redundancy indication
///       function is not invoked, the NMT heartbeat indication function is
///       invoked with the Redundancy Master's Node-ID, CO_NMT_EC_OCCURRED
///       state, CO_NMT_EC_TIMEOUT reason and a null user-specified data
///       pointer
TEST(CO_NmtRdn, CoNmtRdnSlaveBusSelection_MissedHb) {
  ConfigRdn();
  CreateNmtAndReset();
  co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, nullptr);
  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);
  co_nmt_set_alternate_bus_id(nmt, BUS_B_ID);

  const can_msg msg = CreateHbMsg(MASTER_DEV_ID, CO_NMT_ST_START);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  CoNmtHbInd::Clear();

  timespec ts = {0, 0};
  timespec_add_msec(&ts, HB_TIMEOUT_MS);
  can_net_set_time(net, &ts);

  CHECK_EQUAL(BUS_A_ID, co_nmt_get_active_bus_id(nmt));
  CHECK_EQUAL(0, CoNmtRdnInd::GetNumCalled());
  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, MASTER_DEV_ID, CO_NMT_EC_OCCURRED, CO_NMT_EC_TIMEOUT,
                    nullptr);
}

/// \Given a started NMT service (co_nmt_t) configured as NMT slave with the
///        NMT redundancy manager configured; the bus selection process is
///        active
///
/// \When an NMT heartbeat message from the Redundancy Master is received
///
/// \Then the active bus is not switched, the NMT redundancy indication
///       function is not invoked, the NMT heartbeat indication function is
///       invoked with the Redundancy Master's Node-ID, CO_NMT_EC_RESOLVED
///       state, CO_NMT_EC_TIMEOUT reason and a null user-specified data
///       pointer
TEST(CO_NmtRdn, CoNmtRdnSlaveBusSelection_Resolved) {
  ConfigRdn();
  CreateNmtAndReset();
  co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, nullptr);
  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);
  co_nmt_set_alternate_bus_id(nmt, BUS_B_ID);

  const can_msg msg = CreateHbMsg(MASTER_DEV_ID, CO_NMT_ST_START);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  timespec ts = {0, 0};
  timespec_add_msec(&ts, HB_TIMEOUT_MS);
  can_net_set_time(net, &ts);
  CoNmtHbInd::Clear();

  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(BUS_A_ID, co_nmt_get_active_bus_id(nmt));
  CHECK_EQUAL(0, CoNmtRdnInd::GetNumCalled());
  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, MASTER_DEV_ID, CO_NMT_EC_RESOLVED, CO_NMT_EC_TIMEOUT,
                    nullptr);
}

/// \Given a started NMT service (co_nmt_t) configured as NMT slave with the
///        NMT redundancy manager configured, but `Ttoggle` is set to 0; the
///        bus selection process is not active
///
/// \When the NMT consumer heartbeat timer for the Redundancy Master expires
///
/// \Then the bus selection process is not activated - the active bus is not
///       switched, the NMT redundancy indication function and the NMT
///       heartbeat indication function are not invoked
TEST(CO_NmtRdn, CoNmtRdnSlaveBusSelection_MissedHb_ZeroTtoggle) {
  ConfigRdn();
  obj_rdn->SetSub<ObjNmtRedundancy::Sub02Ttoggle>(0);
  CreateNmtAndReset();
  co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, nullptr);
  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);
  co_nmt_set_alternate_bus_id(nmt, BUS_B_ID);

  const can_msg msg = CreateHbMsg(MASTER_DEV_ID, CO_NMT_ST_START);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  timespec ts = {0, 0};
  timespec_add_msec(&ts, HB_TIMEOUT_MS);
  can_net_set_time(net, &ts);
  CoNmtHbInd::Clear();

  timespec_add_msec(&ts, HB_TIMEOUT_MS);
  can_net_set_time(net, &ts);

  CHECK_EQUAL(BUS_A_ID, co_nmt_get_active_bus_id(nmt));
  CHECK_EQUAL(0, CoNmtRdnInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtHbInd::GetNumCalled());
}

///@}

/// @name NMT slave heartbeat timeout
///@{

/// \Given a started NMT service (co_nmt_t) configured as NMT slave with the
///        NMT redundancy manager configured; the initial bus selection process
///        is active
///
/// \When the NMT redundancy bus toggle timer expires
///
/// \Then the active bus is switched to the alternate, the NMT redundancy
///       indication function is invoked with the alternate bus ID,
///       CO_NMT_ECSS_RDN_BUS_SWITCH reason and a null user-specified data
///       pointer, the NMT heartbeat indication function is not invoked
TEST(CO_NmtRdn, CoNmtRdnHbTimeout_SwitchBus_OnInit) {
  ConfigRdn();
  CreateNmtAndReset();
  co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, nullptr);
  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);
  co_nmt_set_alternate_bus_id(nmt, BUS_B_ID);

  timespec ts = {0, 0};
  timespec_add_msec(&ts, HB_TIMEOUT_MS * TTOGGLE);
  can_net_set_time(net, &ts);

  CHECK_EQUAL(BUS_B_ID, co_nmt_get_active_bus_id(nmt));
  CHECK_EQUAL(0, CoNmtHbInd::GetNumCalled());
  CHECK_EQUAL(1u, CoNmtRdnInd::GetNumCalled());
  CoNmtRdnInd::Check(nmt, BUS_B_ID, CO_NMT_ECSS_RDN_BUS_SWITCH, nullptr);
}

/// \Given a started NMT service (co_nmt_t) configured as NMT slave with the
///        NMT redundancy manager configured, but not alternate bus is set; the
///        initial bus selection process is active
///
/// \When the NMT redundancy bus toggle timer expires
///
/// \Then the active bus is not switched, the NMT redundancy indication
///       function is invoked with the primary bus ID,
///       CO_NMT_ECSS_RDN_BUS_SWITCH reason and a null user-specified data
///       pointer, the NMT heartbeat indication function is not invoked
TEST(CO_NmtRdn, CoNmtRdnHbTimeout_SwitchBus_OnInitSameBus) {
  ConfigRdn();
  CreateNmtAndReset();
  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);

  timespec ts = {0, 0};
  timespec_add_msec(&ts, HB_TIMEOUT_MS * TTOGGLE);
  can_net_set_time(net, &ts);

  CHECK_EQUAL(BUS_A_ID, co_nmt_get_active_bus_id(nmt));
  CHECK_EQUAL(0, CoNmtHbInd::GetNumCalled());
  CHECK_EQUAL(1u, CoNmtRdnInd::GetNumCalled());
  CoNmtRdnInd::Check(nmt, BUS_A_ID, CO_NMT_ECSS_RDN_BUS_SWITCH, nullptr);
}

/// \Given a started NMT service (co_nmt_t) configured as NMT slave with the
///        NMT redundancy manager configured; the bus selection process is
///        active
///
/// \When the NMT redundancy bus toggle timer expires
///
/// \Then the active bus is switched to the alternate, the NMT redundancy
///       indication function is invoked with the alternate bus ID,
///       CO_NMT_ECSS_RDN_BUS_SWITCH reason and a null user-specified data
///       pointer, the NMT heartbeat indication function is not invoked
TEST(CO_NmtRdn, CoNmtRdnHbTimeout_SwitchBus) {
  ConfigRdn();
  CreateNmtAndReset();
  co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, nullptr);
  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);
  co_nmt_set_alternate_bus_id(nmt, BUS_B_ID);

  const can_msg msg = CreateHbMsg(MASTER_DEV_ID, CO_NMT_ST_START);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  timespec ts = {0, 0};
  timespec_add_msec(&ts, HB_TIMEOUT_MS);
  can_net_set_time(net, &ts);
  CoNmtHbInd::Clear();

  timespec_add_msec(&ts, HB_TIMEOUT_MS * (TTOGGLE - 1));
  can_net_set_time(net, &ts);

  CHECK_EQUAL(BUS_B_ID, co_nmt_get_active_bus_id(nmt));
  CHECK_EQUAL(0, CoNmtHbInd::GetNumCalled());
  CHECK_EQUAL(1u, CoNmtRdnInd::GetNumCalled());
  CoNmtRdnInd::Check(nmt, BUS_B_ID, CO_NMT_ECSS_RDN_BUS_SWITCH, nullptr);
}

/// \Given a started NMT service (co_nmt_t) configured as NMT slave with the
///        NMT redundancy manager configured; the bus selection process is
///        active, and bus have already been switched to the alternate
///
/// \When an NMT heartbeat message from the Redundancy Master is received
///
/// \Then the active bus is not switched, the NMT redundancy indication
///       function is not invoked, the NMT heartbeat indication function is
///       invoked with the Redundancy Master's Node-ID, CO_NMT_EC_OCCURRED
///       state, CO_NMT_EC_STATE reason and a null user-specified data pointer
TEST(CO_NmtRdn, CoNmtRdnHbTimeout_ResolvedAfterSwitchBus) {
  ConfigRdn();
  CreateNmtAndReset();
  co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, nullptr);
  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);
  co_nmt_set_alternate_bus_id(nmt, BUS_B_ID);

  timespec ts = {0, 0};
  timespec_add_msec(&ts, HB_TIMEOUT_MS * TTOGGLE);
  can_net_set_time(net, &ts);
  CoNmtRdnInd::Clear();

  const can_msg msg = CreateHbMsg(MASTER_DEV_ID, CO_NMT_ST_START);
  CHECK_EQUAL(1u, can_net_recv(net, &msg, BUS_B_ID));

  CHECK_EQUAL(BUS_B_ID, co_nmt_get_active_bus_id(nmt));
  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, MASTER_DEV_ID, CO_NMT_EC_OCCURRED, CO_NMT_EC_STATE,
                    nullptr);
  CHECK_EQUAL(0, CoNmtRdnInd::GetNumCalled());
}

/// \Given a started NMT service (co_nmt_t) configured as NMT slave with the
///        NMT redundancy manager configured; the bus selection process is
///        active
///
/// \When the NMT redundancy bus toggle timer expires `Ntoggle` times
///
/// \Then the active bus is switched `Ntoggle` times, the NMT redundancy
///       indication function is invoked `Ntoggle + 1` timer with the bus ID
///       alternating between the primary and the alternate,
///       CO_NMT_ECSS_RDN_BUS_SWITCH reason and a null user-specified data
///       pointer, the NMT heartbeat indication function is not invoked
TEST(CO_NmtRdn, CoNmtRdnHbTimeout_NoMaster) {
  ConfigRdn();
  CreateNmtAndReset();
  co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, nullptr);
  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);
  co_nmt_set_alternate_bus_id(nmt, BUS_B_ID);

  co_unsigned8_t bus_id = BUS_A_ID;
  CoNmtRdnInd::SetCheckFunc(
      [&bus_id, this](const co_nmt_t* const, const co_unsigned8_t bus_id_,
                      const int reason_, const void* const) {
        if (reason_ == CO_NMT_ECSS_RDN_BUS_SWITCH)
          bus_id = bus_id == BUS_A_ID ? BUS_B_ID : BUS_A_ID;

        if (CoNmtRdnInd::GetNumCalled() < NTOGGLE) {
          CHECK_EQUAL(bus_id, bus_id_);
          CHECK_EQUAL(CO_NMT_ECSS_RDN_BUS_SWITCH, reason_);
        } else {
          CHECK_EQUAL(bus_id, bus_id_);
          CHECK_EQUAL(CO_NMT_ECSS_RDN_NO_MASTER, reason_);

          const co_unsigned8_t ctoggle = co_sub_get_val_u8(co_dev_find_sub(
              dev, CO_NMT_RDN_REDUNDANCY_OBJ_IDX, CO_NMT_RDN_CTOGGLE_SUBIDX));
          CHECK_EQUAL(NTOGGLE, ctoggle);
        }
      });

  timespec ts = {0, 0};

  for (co_unsigned8_t i = 0; i < NTOGGLE; ++i) {
    timespec_add_msec(&ts, HB_TIMEOUT_MS * TTOGGLE);
    can_net_set_time(net, &ts);
  }

  CHECK_EQUAL(bus_id, co_nmt_get_active_bus_id(nmt));
  CHECK_EQUAL(NTOGGLE + 1u, CoNmtRdnInd::GetNumCalled());
  CHECK_EQUAL(0, CoNmtHbInd::GetNumCalled());
}

/// \Given a started NMT service (co_nmt_t) configured as NMT slave the NMT
///        heartbeat consumer for the Redundancy Master configured, but the NMT
///        redundancy manager is not enabled
///
/// \When the NMT consumer heartbeat timer for the Redundancy Master expires
///
/// \Then the NMT heartbeat indication function is invoked with the Redundancy
///       Master's Node-ID, CO_NMT_EC_OCCURRED state, CO_NMT_EC_TIMEOUT reason
///       and a null user-specified data pointer; the NMT redundancy indication
///       function is not invoked and the bus is not switched after the NMT
///       redundancy bus toggle time have passed
TEST(CO_NmtRdn, CoNmtRdnHbTimeout_RdnNotEnabled) {
  ConfigRdnMasterHb();
  CreateNmtAndReset();
  co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, nullptr);
  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);
  co_nmt_set_alternate_bus_id(nmt, BUS_B_ID);

  const can_msg msg = CreateHbMsg(MASTER_DEV_ID, CO_NMT_ST_START);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  CoNmtHbInd::Clear();

  timespec ts = {0, 0};
  timespec_add_msec(&ts, HB_TIMEOUT_MS);
  can_net_set_time(net, &ts);

  CHECK_EQUAL(0, CoNmtRdnInd::GetNumCalled());
  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, MASTER_DEV_ID, CO_NMT_EC_OCCURRED, CO_NMT_EC_TIMEOUT,
                    nullptr);

  timespec_add_msec(&ts, HB_TIMEOUT_MS * (TTOGGLE - 1));
  can_net_set_time(net, &ts);

  CHECK_EQUAL(BUS_A_ID, co_nmt_get_active_bus_id(nmt));
  CHECK_EQUAL(0, CoNmtRdnInd::GetNumCalled());
  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
}

/// \Given a started NMT service (co_nmt_t) configured as NMT slave with the
///        NMT redundancy manager configured; the bus selection process is not
///        active
///
/// \When the NMT consumer heartbeat timer for the other node expires
///
/// \Then the NMT heartbeat indication function is invoked with the other
///       node's Node-ID, CO_NMT_EC_OCCURRED state, CO_NMT_EC_TIMEOUT reason
///       and a null user-specified data pointer; the NMT redundancy indication
///       function is not invoked and the bus is not switched after the NMT
///       redundancy bus toggle time have passed
TEST(CO_NmtRdn, CoNmtRdnHbTimeout_OtherNodeHb) {
  ConfigRdn();
  const co_unsigned8_t other_id = 0x05u;
  const co_unsigned16_t other_hb_timout = HB_TIMEOUT_MS / (TTOGGLE + 1u);
  obj1016->SetSub<Obj1016ConsumerHb::Sub00HighestSubidxSupported>(0x02u);
  obj1016->EmplaceSub<Obj1016ConsumerHb::SubNthConsumerHbTime>(
      0x02u, Obj1016ConsumerHb::MakeHbConsumerEntry(other_id, other_hb_timout));

  CreateNmtAndReset();
  co_nmt_set_hb_ind(nmt, CoNmtHbInd::Func, nullptr);
  co_nmt_set_ecss_rdn_ind(nmt, CoNmtRdnInd::Func, nullptr);
  co_nmt_set_alternate_bus_id(nmt, BUS_B_ID);

  can_msg msg = CreateHbMsg(MASTER_DEV_ID, CO_NMT_ST_START);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  msg = CreateHbMsg(other_id, CO_NMT_ST_START);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  CoNmtHbInd::Clear();

  timespec ts = {0, 0};
  timespec_add_msec(&ts, other_hb_timout);
  can_net_set_time(net, &ts);

  CHECK_EQUAL(0, CoNmtRdnInd::GetNumCalled());
  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
  CoNmtHbInd::Check(nmt, other_id, CO_NMT_EC_OCCURRED, CO_NMT_EC_TIMEOUT,
                    nullptr);

  timespec_add_msec(&ts, other_hb_timout * (TTOGGLE - 1));
  can_net_set_time(net, &ts);

  CHECK_EQUAL(BUS_A_ID, co_nmt_get_active_bus_id(nmt));
  CHECK_EQUAL(0, CoNmtRdnInd::GetNumCalled());
  CHECK_EQUAL(1u, CoNmtHbInd::GetNumCalled());
}

///@}

TEST_GROUP_BASE(CO_NmtRdnPriv, CO_NmtRdnBase) {
  co_nmt_t* nmt = nullptr;
  co_nmt_rdn_t* rdn = nullptr;

  void CreateNmtRdnAndReset() {
    nmt = co_nmt_create(net, dev);
    CHECK(nmt != nullptr);
    rdn = co_nmt_rdn_create(net, nmt);
    CHECK(rdn != nullptr);

    CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));
  }

  TEST_TEARDOWN() {
    co_nmt_rdn_destroy(rdn);
    co_nmt_destroy(nmt);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_nmt_rdn_get_master_id()
///@{

/// \Given a pointer to the NMT redundancy manager service (co_nmt_rdn_t)
///        from a node configured as NMT slave
///
/// \When co_nmt_rdn_get_master_id() is called
///
/// \Then 0 is returned
TEST(CO_NmtRdnPriv, CoNmtRdnGetMasterId_Default) {
  CreateNmtRdnAndReset();

  const auto ret = co_nmt_rdn_get_master_id(rdn);

  CHECK_EQUAL(0, ret);
}

#if !LELY_NO_CO_MASTER
/// \Given a pointer to the NMT redundancy manager service (co_nmt_rdn_t)
///        from a node configured as NMT master
///
/// \When co_nmt_rdn_get_master_id() is called
///
/// \Then the master's Node-ID is returned
TEST(CO_NmtRdnPriv, CoNmtRdnGetMasterId_Master) {
  dev_holder->CreateObjValue<Obj1f80NmtStartup>(obj1f80,
                                                Obj1f80NmtStartup::MASTER_BIT);
  CreateNmtRdnAndReset();

  const auto ret = co_nmt_rdn_get_master_id(rdn);

  CHECK_EQUAL(co_nmt_get_id(nmt), ret);
}
#endif

///@}

/// @name co_nmt_rdn_set_master_id()
///@{

#if !LELY_NO_CO_MASTER
/// \Given a pointer to the NMT redundancy manager service (co_nmt_rdn_t)
///        from a node configured as NMT master
///
/// \When co_nmt_rdn_set_master_id() is called with any Node-ID and any
///       heartbeat time
///
/// \Then -1 is returned, the error number is set to ERRNUM_PERM, the
///       Redundancy Master's Node-ID is not modified
TEST(CO_NmtRdnPriv, CoNmtRdnSetMasterId_Master) {
  dev_holder->CreateObjValue<Obj1f80NmtStartup>(obj1f80,
                                                Obj1f80NmtStartup::MASTER_BIT);
  CreateNmtRdnAndReset();

  const auto ret = co_nmt_rdn_set_master_id(rdn, 0, 0);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_PERM, get_errnum());
  CHECK_EQUAL(co_nmt_get_id(nmt), co_nmt_rdn_get_master_id(rdn));
}
#endif

/// \Given a pointer to the NMT redundancy manager service (co_nmt_rdn_t)
///        from a node configured as NMT slave
///
/// \When co_nmt_rdn_set_master_id() is called with a zero Node-ID and any
///       heartbeat time
///
/// \Then -1 is returned, the error number is set to ERRNUM_INVAL, the
///       Redundancy Master's Node-ID is not modified
TEST(CO_NmtRdnPriv, CoNmtRdnSetMasterId_ZeroId) {
  CreateNmtRdnAndReset();
  CHECK_EQUAL(0, co_nmt_rdn_set_master_id(rdn, MASTER_DEV_ID, 0));

  const auto ret = co_nmt_rdn_set_master_id(rdn, 0, 0);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(MASTER_DEV_ID, co_nmt_rdn_get_master_id(rdn));
}

/// \Given a pointer to the NMT redundancy manager service (co_nmt_rdn_t)
///        from a node configured as NMT slave
///
/// \When co_nmt_rdn_set_master_id() is called with a Node-ID over the maximum
///       value and any heartbeat time
///
/// \Then -1 is returned, the error number is set to ERRNUM_INVAL, the
///       Redundancy Master's Node-ID is not modified
TEST(CO_NmtRdnPriv, CoNmtRdnSetMasterId_OverMaxId) {
  CreateNmtRdnAndReset();

  const auto ret = co_nmt_rdn_set_master_id(rdn, CO_NUM_NODES + 1u, 0);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(0, co_nmt_rdn_get_master_id(rdn));
}

/// \Given a pointer to the NMT redundancy manager service (co_nmt_rdn_t)
///        from a node configured as NMT slave
///
/// \When co_nmt_rdn_set_master_id() is called with a Node-ID and a heartbeat
///       time
///
/// \Then 0 is returned, the Redundancy Master's Node-ID is set
TEST(CO_NmtRdnPriv, CoNmtRdnSetMasterId_Nominal) {
  CreateNmtRdnAndReset();

  const auto ret = co_nmt_rdn_set_master_id(rdn, MASTER_DEV_ID, HB_TIMEOUT_MS);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(MASTER_DEV_ID, co_nmt_rdn_get_master_id(rdn));
}

///@}
