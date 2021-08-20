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
#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/tpdo.h>
#include <lely/co/sdo.h>
#include <lely/co/val.h>
#include <lely/util/error.h>
#include <lely/util/time.h>

#include <libtest/allocators/default.hpp>
#include <libtest/allocators/limited.hpp>
#include <libtest/tools/can-send.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>
#include <libtest/tools/co-tpdo-ind.hpp>
#include <libtest/tools/co-tpdo-sample-ind.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/sub.hpp"

#include "obj-init/sync-window-length.hpp"
#include "obj-init/tpdo-comm-par.hpp"
#include "obj-init/tpdo-map-par.hpp"

TEST_BASE(CO_TpdoBase) {
  TEST_BASE_SUPER(CO_TpdoBase);
  Allocators::Default allocator;

  const co_unsigned8_t DEV_ID = 0x01u;
  const co_unsigned16_t TPDO_NUM = 0x0001u;

  can_net_t* net = nullptr;

  co_dev_t* dev = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1007;
  std::unique_ptr<CoObjTHolder> obj1800;
  std::unique_ptr<CoObjTHolder> obj1a00;

  void CheckPdoMapParIsZeroed(const co_pdo_map_par* const map) const {
    CHECK_EQUAL(0, map->n);
    const std::array<co_unsigned32_t, CO_PDO_NUM_MAPS> expected_map = {0};
    MEMCMP_EQUAL(expected_map.data(), map->map,
                 expected_map.size() * sizeof(co_unsigned32_t));
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
  }
};

TEST_GROUP_BASE(CO_TpdoCreate, CO_TpdoBase) {
  co_tpdo_t* tpdo = nullptr;

  TEST_TEARDOWN() {
    co_tpdo_destroy(tpdo);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_tpdo_create()
///@{

/// \Given initialized device (co_dev_t) and network (can_net_t)
///
/// \When co_tpdo_create() is called with pointers to the network and the
///       device, and a TPDO number equal to zero
///
/// \Then a null pointer is returned, the error number is set to ERRNUM_INVAL,
///       the TPDO service is not created
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_tpdo_alignof()
///       \Calls co_tpdo_sizeof()
///       \Calls errnum2c()
///       \Calls set_errc()
///       \Calls mem_free()
TEST(CO_TpdoCreate, CoTpdoCreate_ZeroNum) {
  tpdo = co_tpdo_create(net, dev, 0u);

  POINTERS_EQUAL(nullptr, tpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given initialized device (co_dev_t) and network (can_net_t)
///
/// \When co_tpdo_create() is called with pointers to the network and the
///       device, and a TPDO number larger than CO_NUM_PDOS
///
/// \Then a null pointer is returned, the error number is set to ERRNUM_INVAL,
///       the TPDO service is not created
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_tpdo_alignof()
///       \Calls co_tpdo_sizeof()
///       \Calls errnum2c()
///       \Calls set_errc()
///       \Calls mem_free()
TEST(CO_TpdoCreate, CoTpdoCreate_NumOverMax) {
  tpdo = co_tpdo_create(net, dev, CO_NUM_PDOS + 1u);

  POINTERS_EQUAL(nullptr, tpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given initialized device (co_dev_t) and network (can_net_t), the object
///        dictionary does not contain required objects
///
/// \When co_tpdo_create() is called with pointers to the network and the
///       device, and a TPDO number
///
/// \Then a null pointer is returned, the error number is set to ERRNUM_INVAL,
///       the TPDO service is not created
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_tpdo_alignof()
///       \Calls co_tpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls errnum2c()
///       \Calls set_errc()
///       \Calls mem_free()
TEST(CO_TpdoCreate, CoTpdoCreate_NoTpdoParameters) {
  tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  POINTERS_EQUAL(nullptr, tpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given initialized device (co_dev_t) and network (can_net_t), the object
///        dictionary contains only the TPDO communication parameter (0x1800)
///        object
///
/// \When co_tpdo_create() is called with pointers to the network and the
///       device, and a TPDO number
///
/// \Then a null pointer is returned, the error number is set to ERRNUM_INVAL,
///       the TPDO service is not created
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_tpdo_alignof()
///       \Calls co_tpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls errnum2c()
///       \Calls set_errc()
///       \Calls mem_free()
TEST(CO_TpdoCreate, CoTpdoCreate_NoTpdoMappingParam) {
  dev_holder->CreateObj<Obj1800TpdoCommPar>(obj1800);

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  POINTERS_EQUAL(nullptr, tpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given initialized device (co_dev_t) and network (can_net_t), the object
///        dictionary contains only the TPDO mapping parameter (0x1a00) object
///
/// \When co_tpdo_create() is called with pointers to the network and the
///       device, and a TPDO number
///
/// \Then a null pointer is returned, the error number is set to ERRNUM_INVAL,
///       the TPDO service is not created
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_tpdo_alignof()
///       \Calls co_tpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls errnum2c()
///       \Calls set_errc()
///       \Calls mem_free()
TEST(CO_TpdoCreate, CoTpdoCreate_NoTpdoCommParam) {
  dev_holder->CreateObj<Obj1a00TpdoMapPar>(obj1a00);

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  POINTERS_EQUAL(nullptr, tpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given initialized device (co_dev_t) and network (can_net_t), the object
///        dictionary contains the TPDO communication parameter (0x1800) and
///        the TPDO mapping parameter (0x1a00) objects
///
/// \When co_tpdo_create() is called with pointers to the network and the
///       device, and a TPDO number
///
/// \Then a pointer to the created TPDO service is returned, the service is
///       stopped and configured with the default values
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_tpdo_alignof()
///       \Calls co_tpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls memset()
///       \Calls can_recv_create()
///       \Calls co_tpdo_get_alloc()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls co_sdo_req_init()
TEST(CO_TpdoCreate, CoTpdoCreate_MinimalTPDO) {
  dev_holder->CreateObj<Obj1800TpdoCommPar>(obj1800);
  dev_holder->CreateObj<Obj1a00TpdoMapPar>(obj1a00);

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  POINTER_NOT_NULL(tpdo);
  POINTERS_EQUAL(net, co_tpdo_get_net(tpdo));
  POINTERS_EQUAL(dev, co_tpdo_get_dev(tpdo));
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));
  CHECK_TRUE(co_tpdo_is_stopped(tpdo));

  const auto* const comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0u, comm->n);
  CHECK_EQUAL(0u, comm->cobid);
  CHECK_EQUAL(0u, comm->trans);
  CHECK_EQUAL(0u, comm->inhibit);
  CHECK_EQUAL(0u, comm->reserved);
  CHECK_EQUAL(0u, comm->event);
  CHECK_EQUAL(0u, comm->sync);

  CheckPdoMapParIsZeroed(co_tpdo_get_map_par(tpdo));
}

/// \Given initialized device (co_dev_t) and network (can_net_t), the object
///        dictionary contains the TPDO communication parameter (0x15ff) and
///        the TPDO mapping parameter (0x17ff) objects
///
/// \When co_tpdo_create() is called with pointers to the network and the
///       device, and the maximum TPDO number
///
/// \Then a pointer to the created TPDO service is returned, the service is
///       stopped and configured with the default values
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_tpdo_alignof()
///       \Calls co_tpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls memset()
///       \Calls can_recv_create()
///       \Calls co_tpdo_get_alloc()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls co_sdo_req_init()
TEST(CO_TpdoCreate, CoTpdoCreate_MinimalTPDO_MaxNum) {
  const co_unsigned16_t MAX_TPDO_NUM = 0x0200u;

  dev_holder->CreateObj<Obj1800TpdoCommPar>(obj1800, 0x19ffu);
  dev_holder->CreateObj<Obj1a00TpdoMapPar>(obj1a00, 0x1bffu);

  tpdo = co_tpdo_create(net, dev, MAX_TPDO_NUM);

  POINTER_NOT_NULL(tpdo);
  POINTERS_EQUAL(net, co_tpdo_get_net(tpdo));
  POINTERS_EQUAL(dev, co_tpdo_get_dev(tpdo));
  CHECK_EQUAL(MAX_TPDO_NUM, co_tpdo_get_num(tpdo));
  CHECK_TRUE(co_tpdo_is_stopped(tpdo));

  const auto* const comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0u, comm->n);
  CHECK_EQUAL(0u, comm->cobid);
  CHECK_EQUAL(0u, comm->trans);
  CHECK_EQUAL(0u, comm->inhibit);
  CHECK_EQUAL(0u, comm->reserved);
  CHECK_EQUAL(0u, comm->event);
  CHECK_EQUAL(0u, comm->sync);

  CheckPdoMapParIsZeroed(co_tpdo_get_map_par(tpdo));
}

///@}

/// @name co_tpdo_destroy()
///@{

/// \Given N/A
///
/// \When co_tpdo_destroy() is called with a null TPDO service pointer
///
/// \Then nothing is changed
TEST(CO_TpdoCreate, CoTpdoDestroy_Null) { co_tpdo_destroy(nullptr); }

/// \Given a pointer to an initialized TPDO service (co_tpdo_t)
///
/// \When co_tpdo_destroy() is called with a pointer to the service
///
/// \Then the service is finalized and freed
///       \Calls co_tpdo_stop()
///       \Calls co_sdo_req_fini()
///       \Calls can_timer_destroy()
///       \Calls can_recv_destroy()
///       \Calls mem_free()
///       \Calls co_tpdo_get_alloc()
TEST(CO_TpdoCreate, CoTpdoDestroy_Nominal) {
  dev_holder->CreateObj<Obj1800TpdoCommPar>(obj1800);
  dev_holder->CreateObj<Obj1a00TpdoMapPar>(obj1a00);
  tpdo = co_tpdo_create(net, dev, TPDO_NUM);
  CHECK(tpdo != nullptr);

  co_tpdo_destroy(tpdo);
  tpdo = nullptr;
}

///@}

TEST_GROUP_BASE(CO_Tpdo, CO_TpdoBase) {
  static const co_unsigned16_t PDO_MAPPED_IDX = 0x2020u;
  static const co_unsigned8_t PDO_MAPPED_SUBIDX = 0x00u;

  co_tpdo_t* tpdo = nullptr;

  int32_t ind_data = 0;
  int32_t sind_data = 0;
  int32_t can_data = 0;

  void CreateTpdo() {
    tpdo = co_tpdo_create(net, dev, TPDO_NUM);
    POINTER_NOT_NULL(tpdo);
    CHECK(co_tpdo_is_stopped(tpdo));
  }

  void StartTpdo() {
    co_tpdo_set_ind(tpdo, CoTpdoInd::Func, &ind_data);

    CHECK(co_tpdo_is_stopped(tpdo));
    co_tpdo_start(tpdo);
    CHECK_FALSE(co_tpdo_is_stopped(tpdo));
  }

  void AdvanceTimeMs(const uint32_t ms) {
    timespec ts = {0, 0};
    can_net_get_time(net, &ts);
    timespec_add_msec(&ts, ms);
    can_net_set_time(net, &ts);
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    dev_holder->CreateObj<Obj1800TpdoCommPar>(obj1800);
    dev_holder->CreateObj<Obj1a00TpdoMapPar>(obj1a00);

    can_net_set_send_func(net, CanSend::Func, &can_data);
  }

  TEST_TEARDOWN() {
    CanSend::Clear();
    CoTpdoInd::Clear();
    CoTpdoSampleInd::Clear();

    co_tpdo_destroy(tpdo);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_tpdo_start()
///@{

/// \Given a pointer to an initialized TPDO service (co_tpdo_t)
///
/// \When co_tpdo_start() is called
///
/// \Then the service is started and configured with the default values
///       \Calls co_dev_find_obj()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_sizeof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_stop()
///       \Calls can_timer_stop()
TEST(CO_Tpdo, CoTpdoStart_Nominal) {
  CreateTpdo();

  co_tpdo_start(tpdo);

  CHECK_FALSE(co_tpdo_is_stopped(tpdo));

  const auto* const comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0, comm->n);
  CHECK_EQUAL(0, comm->cobid);
  CHECK_EQUAL(0, comm->trans);
  CHECK_EQUAL(0, comm->inhibit);
  CHECK_EQUAL(0, comm->reserved);
  CHECK_EQUAL(0, comm->event);
  CHECK_EQUAL(0, comm->sync);

  CheckPdoMapParIsZeroed(co_tpdo_get_map_par(tpdo));

  co_tpdo_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_tpdo_get_ind(tpdo, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(nullptr, pind);
  FUNCTIONPOINTERS_EQUAL(nullptr, pdata);

  co_tpdo_sample_ind_t* psind = nullptr;
  void* psdata = nullptr;
  co_tpdo_get_sample_ind(tpdo, &psind, &psdata);
  CHECK(psind != nullptr);  // default indication function
  FUNCTIONPOINTERS_EQUAL(nullptr, psdata);
}

/// \Given a pointer to an initialized TPDO service (co_tpdo_t), the object
///        dictionary contains the TPDO communication parameter (0x1800) object
///        with the "Transmission type" entry (0x02) set to non-RTR
///        transmission type
///
/// \When co_tpdo_start() is called
///
/// \Then the service is started and the RTR receiver is not started
///       \Calls co_dev_find_obj()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_sizeof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_stop()
///       \Calls can_timer_stop()
TEST(CO_Tpdo, CoTpdoStart_NoRTRRecv) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>();
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(
      Obj1800TpdoCommPar::EVENT_DRIVEN_TRANSMISSION);
  CreateTpdo();
  co_tpdo_set_ind(tpdo, CoTpdoInd::Func, &ind_data);
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &sind_data);

  co_tpdo_start(tpdo);

  CHECK_FALSE(co_tpdo_is_stopped(tpdo));

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags = CAN_FLAG_RTR;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CoTpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CoTpdoSampleInd::GetNumCalled());
}

/// \Given a pointer to an initialized TPDO service (co_tpdo_t), the object
///        dictionary contains the TPDO communication parameter (0x1800) object
///        with the "Transmission type" entry (0x02) set to RTR-only
///        transmission type
///
/// \When co_tpdo_start() is called
///
/// \Then the service is started and the RTR receiver is started
///       \Calls co_dev_find_obj()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_sizeof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_start()
///       \Calls can_timer_stop()
TEST(CO_Tpdo, CoTpdoStart_RTRRecv) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>();
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(
      Obj1800TpdoCommPar::EVENT_DRIVEN_RTR_TRANSMISSION);
  CreateTpdo();
  co_tpdo_set_ind(tpdo, CoTpdoInd::Func, &ind_data);
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &sind_data);

  co_tpdo_start(tpdo);

  CHECK_FALSE(co_tpdo_is_stopped(tpdo));

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags = CAN_FLAG_RTR;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CoTpdoInd::GetNumCalled());
  CHECK_EQUAL(1u, CoTpdoSampleInd::GetNumCalled());
  CoTpdoSampleInd::Check(tpdo, &sind_data);
}

/// \Given a pointer to an initialized TPDO service (co_tpdo_t), the object
///        dictionary contains the TPDO communication parameter (0x1800) object
///        with the "COB-ID" entry (0x01) that has the CO_PDO_COBID_FRAME bit
///        set and the "Transmission type" entry (0x02) set to RTR-only
///        transmission type
///
/// \When co_tpdo_start() is called
///
/// \Then the service is started and the RTR receiver for messages with the
///       29-bit CAN-ID (CAN extended frame) is started
///       \Calls co_dev_find_obj()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_sizeof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_start()
///       \Calls can_timer_stop()
TEST(CO_Tpdo, CoTpdoStart_RTRRecv_ExtendedFrame) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>();
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID |
                                                      CO_PDO_COBID_FRAME);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(
      Obj1800TpdoCommPar::EVENT_DRIVEN_RTR_TRANSMISSION);
  CreateTpdo();
  co_tpdo_set_ind(tpdo, CoTpdoInd::Func, &ind_data);
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &sind_data);

  co_tpdo_start(tpdo);

  CHECK_FALSE(co_tpdo_is_stopped(tpdo));

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags = CAN_FLAG_IDE | CAN_FLAG_RTR;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CoTpdoInd::GetNumCalled());
  CHECK_EQUAL(1u, CoTpdoSampleInd::GetNumCalled());
  CoTpdoSampleInd::Check(tpdo, &sind_data);
}

/// \Given a pointer to an initialized TPDO service (co_tpdo_t), the object
///        dictionary contains the TPDO communication parameter (0x1800) object
///        with the "COB-ID" entry (0x01) that has the CO_PDO_COBID_VALID bit
///        set and the "Transmission type" entry (0x02) set to RTR-only
///        transmission type
///
/// \When co_tpdo_start() is called
///
/// \Then the service is started, but the RTR receiver not started
///       \Calls co_dev_find_obj()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_sizeof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_stop()
///       \Calls can_timer_stop()
TEST(CO_Tpdo, CoTpdoStart_InvalidBit) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>();
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID |
                                                      CO_PDO_COBID_VALID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(
      Obj1800TpdoCommPar::EVENT_DRIVEN_RTR_TRANSMISSION);
  CreateTpdo();
  co_tpdo_set_ind(tpdo, CoTpdoInd::Func, &ind_data);
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &sind_data);

  co_tpdo_start(tpdo);

  CHECK_FALSE(co_tpdo_is_stopped(tpdo));

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags = CAN_FLAG_IDE | CAN_FLAG_RTR;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CoTpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CoTpdoSampleInd::GetNumCalled());
}

/// \Given a pointer to an initialized TPDO service (co_tpdo_t), the object
///        dictionary contains the TPDO communication parameter (0x1800) object
///        with all entires defined
///
/// \When co_tpdo_start() is called
///
/// \Then the service is started and configured with all values from the 0x1800
///       object
///       \Calls co_dev_find_obj()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_sizeof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_start()
///       \Calls can_timer_stop()
TEST(CO_Tpdo, CoTpdoStart_FullTPDOCommParamRecord) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x06u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0x01u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub03InhibitTime>(0x0002u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub04Reserved>();
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub05EventTimer>(0x0004u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub06SyncStartValue>(0x05u);
  CreateTpdo();

  co_tpdo_start(tpdo);

  CHECK_FALSE(co_tpdo_is_stopped(tpdo));

  const auto* const comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0x06u, comm->n);
  CHECK_EQUAL(DEV_ID, comm->cobid);
  CHECK_EQUAL(0x01u, comm->trans);
  CHECK_EQUAL(0x0002u, comm->inhibit);
  CHECK_EQUAL(0x00u, comm->reserved);
  CHECK_EQUAL(0x0004u, comm->event);
  CHECK_EQUAL(0x05u, comm->sync);
}

/// \Given a pointer to an initialized TPDO service (co_tpdo_t), the object
///        dictionary contains the TPDO mapping parameter (0x1a00) object
///        with all possible mapping entries defined
///
/// \When co_tpdo_start() is called
///
/// \Then the service is started and configured with all values from the 0x1a00
///       object
///       \Calls co_dev_find_obj()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_sizeof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_start()
///       \Calls can_timer_stop()
TEST(CO_Tpdo, CoTpdoStart_FullTPDOMappingParamRecord) {
  obj1a00->EmplaceSub<Obj1a00TpdoMapPar::Sub00NumOfMappedObjs>(CO_PDO_NUM_MAPS);
  for (co_unsigned8_t i = 0x01u; i <= CO_PDO_NUM_MAPS; ++i)
    obj1a00->EmplaceSub<Obj1a00TpdoMapPar::SubNthAppObject>(i, i - 1u);
  CreateTpdo();

  co_tpdo_start(tpdo);

  CHECK_FALSE(co_tpdo_is_stopped(tpdo));

  const auto* const map = co_tpdo_get_map_par(tpdo);
  CHECK_EQUAL(CO_PDO_NUM_MAPS, map->n);
  for (size_t i = 0; i < CO_PDO_NUM_MAPS; ++i) CHECK_EQUAL(i, map->map[i]);
}

/// \Given a pointer to an initialized TPDO service (co_tpdo_t), the object
///        dictionary contains the TPDO communication parameter (0x1800) object
///        with all entires defined and one additional entry (illegal)
///
/// \When co_tpdo_start() is called
///
/// \Then the service is started and configured with all values from the 0x1800
///       object, the illegal entry is omitted
///       \Calls co_dev_find_obj()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_sizeof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_start()
///       \Calls can_timer_stop()
TEST(CO_Tpdo, CoTpdoStart_OversizedTPDOCommParamRecord) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x07u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0x01u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub03InhibitTime>(0x0002u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub04Reserved>();
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub05EventTimer>(0x0004u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub06SyncStartValue>(0x05u);
  // illegal sub-object
  obj1800->InsertAndSetSub(0x07u, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t{0});
  CreateTpdo();

  co_tpdo_start(tpdo);

  CHECK_FALSE(co_tpdo_is_stopped(tpdo));

  const auto* const comm = co_tpdo_get_comm_par(tpdo);
  CHECK_EQUAL(0x07u, comm->n);
  CHECK_EQUAL(DEV_ID, comm->cobid);
  CHECK_EQUAL(0x01u, comm->trans);
  CHECK_EQUAL(0x0002u, comm->inhibit);
  CHECK_EQUAL(0x00u, comm->reserved);
  CHECK_EQUAL(0x0004u, comm->event);
  CHECK_EQUAL(0x05u, comm->sync);
}

/// \Given a pointer to an initialized TPDO service (co_tpdo_t), the object
///        dictionary contains the TPDO communication parameter (0x1800) object
///        with the "Transmission type" entry (0x02) set to an event-driven
///        transmission type
///
/// \When co_tpdo_start() is called
///
/// \Then the service is started, the event timer is started
///       \Calls co_dev_find_obj()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_sizeof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_stop()
///       \Calls can_timer_stop()
///       \Calls can_timer_timeout()
TEST(CO_Tpdo, CoTpdoStart_EventDrivenTransmission_EventTimer) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x05u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(
      Obj1800TpdoCommPar::EVENT_DRIVEN_TRANSMISSION);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub03InhibitTime>(0u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub04Reserved>();
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub05EventTimer>(1u);  // 1 ms
  CreateTpdo();
  co_tpdo_set_ind(tpdo, CoTpdoInd::Func, &ind_data);
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &sind_data);

  co_tpdo_start(tpdo);

  CHECK_FALSE(co_tpdo_is_stopped(tpdo));

  AdvanceTimeMs(1u);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CanSend::CheckMsg(DEV_ID, 0, 0, nullptr);
  CHECK_EQUAL(1u, CoTpdoInd::GetNumCalled());
  CoTpdoInd::Check(tpdo, 0, nullptr, 0, &ind_data);
  CHECK_EQUAL(0, CoTpdoSampleInd::GetNumCalled());
}

/// \Given a pointer to an initialized TPDO service (co_tpdo_t), the object
///        dictionary contains the TPDO communication parameter (0x1800) object
///        with the "Transmission type" entry (0x02) set to a non-event-driven
///        transmission type
///
/// \When co_tpdo_start() is called
///
/// \Then the service is started, the event timer is not started
///       \Calls co_dev_find_obj()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_sizeof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_stop()
///       \Calls can_timer_stop()
///       \Calls can_timer_timeout()
TEST(CO_Tpdo, CoTpdoStart_NonEventDrivenTransmission_NoEventTimer) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x05u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(
      Obj1800TpdoCommPar::SYNCHRONOUS_ACYCLIC_TRANSMISSION);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub03InhibitTime>(0u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub04Reserved>();
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub05EventTimer>(1u);  // 1 ms
  CreateTpdo();
  co_tpdo_set_ind(tpdo, CoTpdoInd::Func, &ind_data);
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &sind_data);

  co_tpdo_start(tpdo);

  CHECK_FALSE(co_tpdo_is_stopped(tpdo));

  AdvanceTimeMs(1u);

  CHECK_EQUAL(0, CoTpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CoTpdoSampleInd::GetNumCalled());
}

/// \Given a pointer to a started TPDO service (co_tpdo_t)
///
/// \When co_tpdo_start() is called
///
/// \Then nothing is changed
TEST(CO_Tpdo, CoTpdoStart_AlreadyStarted) {
  CreateTpdo();
  StartTpdo();

  co_tpdo_start(tpdo);

  CHECK_FALSE(co_tpdo_is_stopped(tpdo));
}

///@}

/// @name co_tpdo_get_ind()
///@{

/// \Given a pointer to an initialized TPDO service (co_tpdo_t)
///
/// \When co_tpdo_get_ind() is called with no addresses to store the indication
///       function and user-specified data pointers at
///
/// \Then nothing is changed
TEST(CO_Tpdo, CoTpdoGetInd_Null) {
  CreateTpdo();

  co_tpdo_get_ind(tpdo, nullptr, nullptr);
}

/// \Given a pointer to an initialized TPDO service (co_tpdo_t)
///
/// \When co_tpdo_get_ind() is called with an address to store the indication
///       function pointer and an address to store user-specified data pointer
///
/// \Then both pointers are set to a null pointer (default values)
TEST(CO_Tpdo, CoTpdoGetInd_Nominal) {
  CreateTpdo();

  co_tpdo_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_tpdo_get_ind(tpdo, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(nullptr, pind);
  POINTERS_EQUAL(nullptr, pdata);
}

///@}

/// @name co_tpdo_set_ind()
///@{

/// \Given a pointer to an initialized TPDO service (co_tpdo_t)
///
/// \When co_tpdo_set_ind() is called with a pointer to an indication
///       function and a pointer to user-specified data
///
/// \Then the indication function and the user-specified data pointers are set
///       in the TPDO service
TEST(CO_Tpdo, CoTpdoSetInd_Nominal) {
  int32_t data = 0;
  CreateTpdo();

  co_tpdo_set_ind(tpdo, CoTpdoInd::Func, &data);

  co_tpdo_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_tpdo_get_ind(tpdo, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(CoTpdoInd::Func, pind);
  POINTERS_EQUAL(&data, pdata);
}

///@}

/// @name co_tpdo_get_sample_ind()
///@{

/// \Given a pointer to an initialized TPDO service (co_tpdo_t)
///
/// \When co_tpdo_get_sample_ind() is called with no addresses to store the
///       indication function and user-specified data pointers at
///
/// \Then nothing is changed
TEST(CO_Tpdo, CoTpdoGetSampleInd_Null) {
  CreateTpdo();

  co_tpdo_get_sample_ind(tpdo, nullptr, nullptr);
}

/// \Given a pointer to an initialized TPDO service (co_tpdo_t)
///
/// \When co_tpdo_get_sample_ind() is called with an address to store the
///       indication function pointer and an address to store user-specified
///       data pointer
///
/// \Then both pointers are set to a null pointer (default values)
TEST(CO_Tpdo, CoTpdoGetSampleInd_Nominal) {
  CreateTpdo();

  co_tpdo_sample_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_tpdo_get_sample_ind(tpdo, &pind, &pdata);
  CHECK(pind != nullptr);
  POINTERS_EQUAL(nullptr, pdata);
}

///@}

/// @name co_tpdo_set_sample_ind()
///@{

/// \Given a pointer to an initialized TPDO service (co_tpdo_t)
///
/// \When co_tpdo_set_sample_ind() is called with a pointer to an indication
///       function and a pointer to user-specified data
///
/// \Then the indication function and the user-specified data pointers are set
///       in the TPDO service
TEST(CO_Tpdo, CoTpdoSetSampleInd_Nominal) {
  int32_t data = 0;
  CreateTpdo();

  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &data);

  co_tpdo_sample_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_tpdo_get_sample_ind(tpdo, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(CoTpdoSampleInd::Func, pind);
  POINTERS_EQUAL(&data, pdata);
}

/// \Given a pointer to an initialized TPDO service (co_tpdo_t)
///
/// \When co_tpdo_set_sample_ind() is called with a null indication function
///       pointer and any pointer to user-specified data
///
/// \Then the indication function and the user-specified data pointers are
///       reset to the default values in the TPDO service
TEST(CO_Tpdo, CoTpdoSetSampleInd_Null) {
  int32_t data = 0;
  CreateTpdo();

  co_tpdo_sample_ind_t* default_pind = nullptr;
  co_tpdo_get_sample_ind(tpdo, &default_pind, nullptr);
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &data);

  co_tpdo_set_sample_ind(tpdo, nullptr, &data);

  co_tpdo_sample_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_tpdo_get_sample_ind(tpdo, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(default_pind, pind);
  POINTERS_EQUAL(nullptr, pdata);
}

///@}

/// @name co_tpdo_event()
///@{

TEST(CO_Tpdo, CoTpdoEvent_Stopped) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>();
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>();

  CreateTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoTpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

TEST(CO_Tpdo, CoTpdoEvent_InvalidRpdo) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>();
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID |
                                                      CO_PDO_COBID_VALID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>();

  CreateTpdo();
  StartTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoTpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

TEST(CO_Tpdo, CoTpdoEvent_AcyclicSynchronousTransmission) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(
      Obj1800TpdoCommPar::SYNCHRONOUS_ACYCLIC_TRANSMISSION);

  CreateTpdo();
  StartTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoTpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

TEST(CO_Tpdo, CoTpdoEvent_CyclicSynchronousTransmission) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(
      Obj1800TpdoCommPar::SYNCHRONOUS_TRANSMISSION(1u));

  CreateTpdo();
  StartTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoTpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

TEST(CO_Tpdo, CoTpdoEvent_EventDrivenRTR_InitFrameSuccess) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID |
                                                      CO_PDO_COBID_RTR);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(
      Obj1800TpdoCommPar::EVENT_DRIVEN_RTR_TRANSMISSION);

  CreateTpdo();
  StartTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoTpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

TEST(CO_Tpdo, CoTpdoEvent_EventDrivenRTR_InitFrameFail) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(
      DEV_ID | CO_PDO_COBID_FRAME | CO_PDO_COBID_RTR);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xfdu);

  obj1a00->EmplaceSub<Obj1a00TpdoMapPar::Sub00NumOfMappedObjs>(0x01u);
  obj1a00->EmplaceSub<Obj1a00TpdoMapPar::SubNthAppObject>(0x01u, 0xffff0000u);

  CreateTpdo();
  StartTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());

  CHECK_EQUAL(1u, CoTpdoInd::GetNumCalled());
  CoTpdoInd::Check(tpdo, CO_SDO_AC_NO_OBJ, nullptr, 0, &ind_data);
}

TEST(CO_Tpdo, CoTpdoEvent_EventDriven_InhibitTimeNotPassed) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x03u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xfeu);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub03InhibitTime>(10u);  // 1 ms

  CreateTpdo();
  StartTpdo();

  CHECK_EQUAL(0, co_tpdo_event(tpdo));
  CoTpdoInd::Clear();
  CanSend::Clear();

  const timespec ts = {0, 999999u};  // 0.999999 ms
  CHECK_EQUAL(0, can_net_set_time(net, &ts));

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_AGAIN, get_errnum());
  CHECK_EQUAL(0, CoTpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

TEST(CO_Tpdo, CoTpdoEvent_EventDriven_InhibitTimePassedNoSendFunc) {
  can_net_set_send_func(net, nullptr, nullptr);

  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x03u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xfeu);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub03InhibitTime>(10u);  // 1 ms

  CreateTpdo();
  StartTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_NOSYS, get_errnum());

  CHECK_EQUAL(1u, CoTpdoInd::GetNumCalled());
  CoTpdoInd::Check(tpdo, CO_SDO_AC_ERROR, nullptr, 0, &ind_data);
}

TEST(CO_Tpdo, CoTpdoEvent_EventDriven_InitFrameFailed) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xfeu);

  obj1a00->EmplaceSub<Obj1a00TpdoMapPar::Sub00NumOfMappedObjs>(0x01u);
  obj1a00->EmplaceSub<Obj1a00TpdoMapPar::SubNthAppObject>(0x01u, 0xffff0000u);

  CreateTpdo();
  StartTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());

  CHECK_EQUAL(1u, CoTpdoInd::GetNumCalled());
  CoTpdoInd::Check(tpdo, CO_SDO_AC_NO_OBJ, nullptr, 0, &ind_data);
}

TEST(CO_Tpdo, CoTpdoEvent_EventDriven_SendFrameError) {
  CanSend::ret = -1;

  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xfeu);

  CreateTpdo();
  StartTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(-1, ret);
  CHECK_COMPARE(CanSend::GetNumCalled(), >, 0);

  CHECK_EQUAL(1u, CoTpdoInd::GetNumCalled());
  CoTpdoInd::Check(tpdo, CO_SDO_AC_ERROR, nullptr, 0, &ind_data);
}

TEST(CO_Tpdo, CoTpdoEvent_EventDriven) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x05u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xfeu);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub03InhibitTime>(0);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub04Reserved>();
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub05EventTimer>(1);

  CreateTpdo();
  StartTpdo();

  const auto ret = co_tpdo_event(tpdo);

  CHECK_EQUAL(0, ret);

  timespec ts = {0u, 0u};
  co_tpdo_get_next(tpdo, &ts);
  CHECK_EQUAL(0u, ts.tv_nsec);
  CHECK_EQUAL(0u, ts.tv_sec);

  CHECK_COMPARE(CanSend::GetNumCalled(), >, 0);
  POINTERS_EQUAL(&can_data, CanSend::user_data);
  CHECK_EQUAL(DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0u, CanSend::msg.flags);
  CHECK_EQUAL(0u, CanSend::msg.len);

  CHECK_EQUAL(1u, CoTpdoInd::GetNumCalled());
  CoTpdoInd::CheckPtrNotNull(tpdo, 0, 0, &ind_data);
}

TEST(CO_Tpdo, CoTpdoEvent_EventDriven_EventTimer) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xffu);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub03InhibitTime>(0);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub04Reserved>();
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub05EventTimer>(1);  // 1 ms

  CreateTpdo();
  StartTpdo();

  CHECK_EQUAL(0, co_tpdo_event(tpdo));
  CHECK_COMPARE(CanSend::GetNumCalled(), >, 0);
  CHECK_EQUAL(1u, CoTpdoInd::GetNumCalled());
  CoTpdoInd::CheckPtrNotNull(tpdo, 0, 0, &ind_data);

  CanSend::Clear();
  CoTpdoInd::Clear();

  const timespec ts = {0, 1000000u};  // 1 ms
  CHECK_EQUAL(0, can_net_set_time(net, &ts));

  CHECK_COMPARE(CanSend::GetNumCalled(), >, 0);
  POINTERS_EQUAL(&can_data, CanSend::user_data);

  CHECK_EQUAL(1u, CoTpdoInd::GetNumCalled());
  CoTpdoInd::CheckPtrNotNull(tpdo, 0, 0, &ind_data);
}

TEST(CO_Tpdo, CoTpdoEvent_EventDriven_EventTimer_InhibitTimeNotElapsed) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xffu);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub03InhibitTime>(11u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub04Reserved>();
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub05EventTimer>(1);  // 1 ms

  CreateTpdo();
  StartTpdo();

  CHECK_EQUAL(0, co_tpdo_event(tpdo));
  CHECK_COMPARE(CanSend::GetNumCalled(), >, 0);
  CHECK_EQUAL(1u, CoTpdoInd::GetNumCalled());
  CoTpdoInd::CheckPtrNotNull(tpdo, 0, 0, &ind_data);

  CanSend::Clear();
  CoTpdoInd::Clear();

  const timespec ts = {0, 1000000u};  // 1 ms
  CHECK_EQUAL(0, can_net_set_time(net, &ts));

  CHECK_EQUAL(0, CanSend::GetNumCalled());
  CHECK_EQUAL(0, CoTpdoInd::GetNumCalled());
}

///@}

/// @name co_tpdo_get_next()
///@{

TEST(CO_Tpdo, CoTpdoGetNext_Null) {
  CreateTpdo();

  co_tpdo_get_next(tpdo, nullptr);
}

///@}

/// @name co_tpdo_sync()
///@{

TEST(CO_Tpdo, CoTpdoSync_CounterOverLimit) {
  CreateTpdo();
  StartTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &sind_data);

  const auto ret = co_tpdo_sync(tpdo, 241);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(0, CoTpdoSampleInd::GetNumCalled());
}

TEST(CO_Tpdo, CoTpdoSync_InvalidCobId) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID |
                                                      CO_PDO_COBID_VALID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xf1u);

  CreateTpdo();
  StartTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &sind_data);

  const auto ret = co_tpdo_sync(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoTpdoSampleInd::GetNumCalled());
}

TEST(CO_Tpdo, CoTpdoSync_EventDrivenTransmission) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xffu);

  CreateTpdo();
  StartTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &sind_data);

  const auto ret = co_tpdo_sync(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoTpdoSampleInd::GetNumCalled());
}

TEST(CO_Tpdo, CoTpdoSync_SyncStartValue_CntZero) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x06u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0x01u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub03InhibitTime>(0);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub04Reserved>();
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub05EventTimer>(0);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub06SyncStartValue>(2u);

  CreateTpdo();
  StartTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &sind_data);

  const auto ret = co_tpdo_sync(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoTpdoSampleInd::GetNumCalled());
  CoTpdoSampleInd::Check(tpdo, &sind_data);
}

TEST(CO_Tpdo, CoTpdoSync_SyncStartValue_CntNotEquals) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x06u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0x01u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub03InhibitTime>(0);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub04Reserved>();
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub05EventTimer>(0);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub06SyncStartValue>(2u);

  CreateTpdo();
  StartTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &sind_data);

  const auto ret = co_tpdo_sync(tpdo, 1u);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoTpdoSampleInd::GetNumCalled());
}

TEST(CO_Tpdo, CoTpdoSync_SyncStartValue_CntEquals) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x06u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0x01u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub03InhibitTime>(0);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub04Reserved>();
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub05EventTimer>(0);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub06SyncStartValue>(2u);

  CreateTpdo();
  StartTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &sind_data);

  const auto ret = co_tpdo_sync(tpdo, 2u);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoTpdoSampleInd::GetNumCalled());
  CoTpdoSampleInd::Check(tpdo, &sind_data);
}

TEST(CO_Tpdo, CoTpdoSync_SyncRTR_StartSyncWindowTimer) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xfcu);

  dev_holder->CreateObjValue<Obj1007SyncWindowLength>(obj1007, 1000u);  // 1 ms

  CreateTpdo();
  StartTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &sind_data);
  co_tpdo_event(tpdo);

  const auto ret = co_tpdo_sync(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoTpdoSampleInd::GetNumCalled());
  CoTpdoSampleInd::Check(tpdo, &sind_data);
}

TEST(CO_Tpdo, CoTpdoSync_SyncAcyclic_Event) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0x00u);

  CreateTpdo();
  StartTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &sind_data);
  co_tpdo_event(tpdo);

  const auto ret = co_tpdo_sync(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoTpdoSampleInd::GetNumCalled());
  CoTpdoSampleInd::Check(tpdo, &sind_data);
}

TEST(CO_Tpdo, CoTpdoSync_SyncAcyclic_NoEvent) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0x00u);

  CreateTpdo();
  StartTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &sind_data);

  const auto ret = co_tpdo_sync(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoTpdoSampleInd::GetNumCalled());
}

TEST(CO_Tpdo, CoTpdoSync_SyncCyclicNoSample) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0x02u);

  CreateTpdo();
  StartTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &sind_data);

  const auto ret = co_tpdo_sync(tpdo, 1);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoTpdoSampleInd::GetNumCalled());
}

///@}

/// @name co_tpdo_sample_res()
///@{

TEST(CO_Tpdo, CoTpdoSampleRes_InvalidPDO) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID |
                                                      CO_PDO_COBID_VALID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0x00u);

  CreateTpdo();
  StartTpdo();

  const auto ret = co_tpdo_sample_res(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoTpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

TEST(CO_Tpdo, CoTpdoSampleRes_EventDriven) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xffu);

  CreateTpdo();
  StartTpdo();

  const auto ret = co_tpdo_sample_res(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoTpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

TEST(CO_Tpdo, CoTpdoSampleRes_ACErrorArg) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xfcu);

  CreateTpdo();
  StartTpdo();

  const auto ret = co_tpdo_sample_res(tpdo, CO_UNSIGNED32_MAX);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());

  CHECK_EQUAL(1u, CoTpdoInd::GetNumCalled());
  CoTpdoInd::Check(tpdo, CO_UNSIGNED32_MAX, nullptr, 0, &ind_data);
}

TEST(CO_Tpdo, CoTpdoSampleRes_ACErrorArg_NoIndFunc) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0x00u);

  CreateTpdo();
  StartTpdo();
  co_tpdo_set_ind(tpdo, nullptr, nullptr);

  const auto ret = co_tpdo_sample_res(tpdo, CO_UNSIGNED32_MAX);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

TEST(CO_Tpdo, CoTpdoSampleRes_SyncWindowTimeout) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0x01u);

  dev_holder->CreateObjValue<Obj1007SyncWindowLength>(obj1007, 1000u);  // 1 ms

  CreateTpdo();
  StartTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, &sind_data);
  co_tpdo_event(tpdo);

  CHECK_EQUAL(0, co_tpdo_sync(tpdo, 0));
  CHECK_EQUAL(1u, CoTpdoSampleInd::GetNumCalled());
  CoTpdoSampleInd::Check(tpdo, &sind_data);

  const timespec ts = {0, 1000000u};  // 1 ms
  CHECK_EQUAL(0, can_net_set_time(net, &ts));

  const auto ret = co_tpdo_sample_res(tpdo, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());

  CHECK_EQUAL(1u, CoTpdoInd::GetNumCalled());
  CoTpdoInd::Check(tpdo, CO_SDO_AC_TIMEOUT, nullptr, 0, &ind_data);
}

TEST(CO_Tpdo, CoTpdoSampleRes_InitFrameFail) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0x00u);

  obj1a00->EmplaceSub<Obj1a00TpdoMapPar::Sub00NumOfMappedObjs>(0x01u);
  obj1a00->EmplaceSub<Obj1a00TpdoMapPar::SubNthAppObject>(0x01u, 0xffff0000u);

  CreateTpdo();
  StartTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, nullptr);
  CHECK_EQUAL(0, co_tpdo_sync(tpdo, 0));

  const auto ret = co_tpdo_sample_res(tpdo, 0);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());

  CHECK_EQUAL(1u, CoTpdoInd::GetNumCalled());
  CoTpdoInd::Check(tpdo, CO_SDO_AC_NO_OBJ, nullptr, 0, &ind_data);
}

TEST(CO_Tpdo, CoTpdoSampleRes_CanSendError) {
  CanSend::ret = -1;

  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0x00u);

  CreateTpdo();
  StartTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, nullptr);
  CHECK_EQUAL(0, co_tpdo_sync(tpdo, 0));

  const auto ret = co_tpdo_sample_res(tpdo, 0);

  CHECK_EQUAL(-1, ret);

  CHECK_COMPARE(CanSend::GetNumCalled(), >, 0);
  POINTERS_EQUAL(&can_data, CanSend::user_data);

  CHECK_EQUAL(1u, CoTpdoInd::GetNumCalled());
  CoTpdoInd::Check(tpdo, CO_SDO_AC_ERROR, nullptr, 0, &ind_data);
}

TEST(CO_Tpdo, CoTpdoSampleRes) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0x00u);

  CreateTpdo();
  StartTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, nullptr);
  CHECK_EQUAL(0, co_tpdo_sync(tpdo, 0));

  const auto ret = co_tpdo_sample_res(tpdo, 0);

  CHECK_EQUAL(0, ret);

  CHECK_COMPARE(CanSend::GetNumCalled(), >, 0);
  POINTERS_EQUAL(&can_data, CanSend::user_data);
  CHECK_EQUAL(DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0u, CanSend::msg.flags);
  CHECK_EQUAL(0u, CanSend::msg.len);

  CHECK_EQUAL(1u, CoTpdoInd::GetNumCalled());
  CoTpdoInd::CheckPtrNotNull(tpdo, 0, 0, &ind_data);
}

///@}

/// @name TPDO received message processing
///@{

TEST(CO_Tpdo, CoTpdoRecv_SyncRTR_NoBufferedFrame_ExtendedFrame) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID |
                                                      CO_PDO_COBID_FRAME);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xfcu);

  CreateTpdo();
  StartTpdo();
  co_tpdo_set_sample_ind(tpdo, CoTpdoSampleInd::Func, nullptr);

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags |= CAN_FLAG_RTR;
  msg.flags |= CAN_FLAG_IDE;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK_EQUAL(0, CoTpdoSampleInd::GetNumCalled());
}

TEST(CO_Tpdo, CoTpdoRecv_SynchRTR_NoInd) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xfcu);

  CreateTpdo();
  StartTpdo();
  co_tpdo_set_ind(tpdo, nullptr, nullptr);

  CHECK_EQUAL(0, co_tpdo_sync(tpdo, 0x00u));

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags |= CAN_FLAG_RTR;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);

  CHECK_COMPARE(CanSend::GetNumCalled(), >, 0);
  POINTERS_EQUAL(&can_data, CanSend::user_data);
  CHECK_EQUAL(DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0u, CanSend::msg.flags);
  CHECK_EQUAL(0u, CanSend::msg.len);
}

TEST(CO_Tpdo, CoTpdoRecv_EventDrivenRTR) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xfdu);

  obj1a00->EmplaceSub<Obj1a00TpdoMapPar::Sub00NumOfMappedObjs>(0u);
  obj1a00->EmplaceSub<Obj1a00TpdoMapPar::SubNthAppObject>(0x01u, 0u);

  CreateTpdo();
  StartTpdo();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags |= CAN_FLAG_RTR;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);

  CHECK_COMPARE(CanSend::GetNumCalled(), >, 0);
  POINTERS_EQUAL(&can_data, CanSend::user_data);
  CHECK_EQUAL(DEV_ID, CanSend::msg.id);
  CHECK_EQUAL(0u, CanSend::msg.flags);
  CHECK_EQUAL(0u, CanSend::msg.len);
  CHECK_EQUAL(0u, CanSend::msg.data[0]);

  CHECK_EQUAL(1u, CoTpdoInd::GetNumCalled());
  CoTpdoInd::CheckPtrNotNull(tpdo, 0, 0, &ind_data);
}

TEST(CO_Tpdo, CoTpdoRecv_EventDrivenRTR_InitFrameFail_NoInd) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xfdu);

  obj1a00->EmplaceSub<Obj1a00TpdoMapPar::Sub00NumOfMappedObjs>(0x01u);
  obj1a00->EmplaceSub<Obj1a00TpdoMapPar::SubNthAppObject>(0x01u, 0xffff0000u);

  CreateTpdo();
  StartTpdo();
  co_tpdo_set_ind(tpdo, nullptr, nullptr);

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags |= CAN_FLAG_RTR;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK_EQUAL(0, CoTpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

TEST(CO_Tpdo, CoTpdoRecv_EventDrivenRTR_InitFrameFail) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0xfdu);

  obj1a00->EmplaceSub<Obj1a00TpdoMapPar::Sub00NumOfMappedObjs>(0x01u);
  obj1a00->EmplaceSub<Obj1a00TpdoMapPar::SubNthAppObject>(0x01u, 0x200000ffu);

  CreateTpdo();
  StartTpdo();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags |= CAN_FLAG_RTR;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());

  CHECK_EQUAL(1u, CoTpdoInd::GetNumCalled());
  CoTpdoInd::Check(tpdo, CO_SDO_AC_PDO_LEN, nullptr, 0, &ind_data);
}

TEST(CO_Tpdo, CoTpdoRecv_NoRTRTransmission) {
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(DEV_ID);
  obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(0x00u);

  obj1a00->EmplaceSub<Obj1a00TpdoMapPar::Sub00NumOfMappedObjs>(0u);
  obj1a00->EmplaceSub<Obj1a00TpdoMapPar::SubNthAppObject>(0x01u, 0u);

  CreateTpdo();
  StartTpdo();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags |= CAN_FLAG_RTR;

  const auto ret = can_net_recv(net, &msg, 0);

  CHECK_EQUAL(1, ret);
  CHECK_EQUAL(0, CoTpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

///@}

TEST_GROUP_BASE(CO_TpdoAllocation, CO_TpdoBase) {
  Allocators::Limited limitedAllocator;
  co_tpdo_t* tpdo = nullptr;

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(limitedAllocator.ToAllocT(), 0);
    POINTER_NOT_NULL(net);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    POINTER_NOT_NULL(dev);

    dev_holder->CreateObj<Obj1800TpdoCommPar>(obj1800);
    dev_holder->CreateObj<Obj1a00TpdoMapPar>(obj1a00);
  }

  TEST_TEARDOWN() {
    co_tpdo_destroy(tpdo);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_tpdo_create()
///@{

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to 0 bytes
///
/// \When co_tpdo_create() is called with pointers to the network and the
///       device, and a TPDO number
///
/// \Then a null pointer is returned, the TPDO service is not created and the
///       error number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_tpdo_alignof()
///       \Calls co_tpdo_sizeof()
///       \Calls get_errc()
///       \Calls set_errc()
TEST(CO_TpdoAllocation, CoTpdoCreate_NoMemory) {
  limitedAllocator.LimitAllocationTo(0u);

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  POINTERS_EQUAL(nullptr, tpdo);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the TPDO service instance
///
/// \When co_tpdo_create() is called with pointers to the network and the
///       device, and a TPDO number
///
/// \Then a null pointer is returned, the TPDO service is not created and the
///       error number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_tpdo_alignof()
///       \Calls co_tpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls memset()
///       \Calls can_recv_create()
///       \Calls mem_free()
///       \Calls co_tpdo_get_alloc()
///       \Calls get_errc()
///       \Calls set_errc()
TEST(CO_TpdoAllocation, CoTpdoCreate_NoMemoryForRecv) {
  limitedAllocator.LimitAllocationTo(co_tpdo_sizeof());

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  POINTERS_EQUAL(nullptr, tpdo);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the TPDO service instance and the
///        CAN receiver
///
/// \When co_tpdo_create() is called with pointers to the network and the
///       device, and a TPDO number
///
/// \Then a null pointer is returned, the TPDO service is not created and the
///       error number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_tpdo_alignof()
///       \Calls co_tpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls memset()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_recv_destroy()
///       \Calls mem_free()
///       \Calls co_tpdo_get_alloc()
///       \Calls get_errc()
///       \Calls set_errc()
TEST(CO_TpdoAllocation, CoTpdoCreate_NoMemoryForTimer) {
  limitedAllocator.LimitAllocationTo(co_tpdo_sizeof() + can_recv_sizeof());

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  POINTERS_EQUAL(nullptr, tpdo);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the TPDO service instance, the
///        CAN receiver and one CAN timer
///
/// \When co_tpdo_create() is called with pointers to the network and the
///       device, and a TPDO number
///
/// \Then a null pointer is returned, the TPDO service is not created and the
///       error number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_tpdo_alignof()
///       \Calls co_tpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls memset()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_destroy()
///       \Calls can_recv_destroy()
///       \Calls mem_free()
///       \Calls co_tpdo_get_alloc()
///       \Calls get_errc()
///       \Calls set_errc()
TEST(CO_TpdoAllocation, CoTpdoCreate_NoMemoryForSecondTimer) {
  limitedAllocator.LimitAllocationTo(co_tpdo_sizeof() + can_recv_sizeof() +
                                     can_timer_sizeof());

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  POINTERS_EQUAL(nullptr, tpdo);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the TPDO service and all required
///        objects
///
/// \When co_tpdo_create() is called with pointers to the network and the
///       device, and a TPDO number
///
/// \Then a pointer to the created TPDO service is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_tpdo_alignof()
///       \Calls co_tpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls memset()
///       \Calls can_recv_create()
///       \Calls co_tpdo_get_alloc()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls co_sdo_req_init()
TEST(CO_TpdoAllocation, CoTpdoCreate_ExactMemory) {
  limitedAllocator.LimitAllocationTo(co_tpdo_sizeof() + can_recv_sizeof() +
                                     2u * can_timer_sizeof());

  tpdo = co_tpdo_create(net, dev, TPDO_NUM);

  POINTER_NOT_NULL(tpdo);
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

///@}
