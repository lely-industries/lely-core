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
#include <lely/co/rpdo.h>
#include <lely/co/sdo.h>
#include <lely/util/error.h>
#include <lely/util/time.h>

#include <libtest/allocators/default.hpp>
#include <libtest/allocators/limited.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>
#include <libtest/tools/can-send.hpp>
#include <libtest/tools/co-rpdo-err.hpp>
#include <libtest/tools/co-rpdo-ind.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/sub.hpp"

#include "obj-init/rpdo-comm-par.hpp"
#include "obj-init/rpdo-map-par.hpp"
#include "obj-init/sync-window-length.hpp"

TEST_BASE(CO_RpdoBase) {
  TEST_BASE_SUPER(CO_RpdoBase);

  const co_unsigned8_t DEV_ID = 0x01u;
  const co_unsigned16_t RPDO_NUM = 0x0001u;

  can_net_t* net = nullptr;
  co_dev_t* dev = nullptr;

  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1007;
  std::unique_ptr<CoObjTHolder> obj1400;
  std::unique_ptr<CoObjTHolder> obj1600;
  std::unique_ptr<CoObjTHolder> obj2020;

  Allocators::Default allocator;

  void AdvanceTimeMs(const uint32_t ms) {
    timespec ts = {0, 0};
    can_net_get_time(net, &ts);
    timespec_add_msec(&ts, ms);
    can_net_set_time(net, &ts);
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

TEST_GROUP_BASE(CO_RpdoCreate, CO_RpdoBase) {
  co_rpdo_t* rpdo = nullptr;

  TEST_TEARDOWN() {
    co_rpdo_destroy(rpdo);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_rpdo_create()
///@{

/// \Given initialized device (co_dev_t) and network (can_net_t)
///
/// \When co_rpdo_create() is called with pointers to the network and the
///       device, and an RPDO number equal to zero
///
/// \Then a null pointer is returned, the error number is set to ERRNUM_INVAL,
///       the RPDO service is not created
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_rpdo_alignof()
///       \Calls co_rpdo_sizeof()
///       \Calls errnum2c()
///       \Calls set_errc()
///       \Calls co_rpdo_free()
TEST(CO_RpdoCreate, CoRpdoCreate_ZeroNum) {
  rpdo = co_rpdo_create(net, dev, 0);

  POINTERS_EQUAL(nullptr, rpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given initialized device (co_dev_t) and network (can_net_t)
///
/// \When co_rpdo_create() is called with pointers to the network and the
///       device, and an RPDO number larger than CO_NUM_PDOS
///
/// \Then a null pointer is returned, the error number is set to ERRNUM_INVAL,
///       the RPDO service is not created
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_rpdo_alignof()
///       \Calls co_rpdo_sizeof()
///       \Calls errnum2c()
///       \Calls set_errc()
///       \Calls co_rpdo_free()
TEST(CO_RpdoCreate, CoRpdoCreate_NumOverMax) {
  rpdo = co_rpdo_create(net, dev, CO_NUM_PDOS + 1u);

  POINTERS_EQUAL(nullptr, rpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given initialized device (co_dev_t) and network (can_net_t), the object
///        dictionary does not contain required objects
///
/// \When co_rpdo_create() is called with pointers to the network and the
///       device, and an RPDO number
///
/// \Then a null pointer is returned, the error number is set to ERRNUM_INVAL,
///       the RPDO service is not created
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_rpdo_alignof()
///       \Calls co_rpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls errnum2c()
///       \Calls set_errc()
///       \Calls co_rpdo_free()
TEST(CO_RpdoCreate, CoRpdoCreate_NoRpdoParameters) {
  rpdo = co_rpdo_create(net, dev, RPDO_NUM);

  POINTERS_EQUAL(nullptr, rpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given initialized device (co_dev_t) and network (can_net_t), the object
///        dictionary contains only the RPDO communication parameter (0x1400)
///        object
///
/// \When co_rpdo_create() is called with pointers to the network and the
///       device, and an RPDO number
///
/// \Then a null pointer is returned, the error number is set to ERRNUM_INVAL,
///       the RPDO service is not created
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_rpdo_alignof()
///       \Calls co_rpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls errnum2c()
///       \Calls set_errc()
///       \Calls co_rpdo_free()
TEST(CO_RpdoCreate, CoRpdoCreate_NoRpdoMappingParam) {
  dev_holder->CreateObj<Obj1400RpdoCommPar>(obj1400);

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);

  POINTERS_EQUAL(nullptr, rpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given initialized device (co_dev_t) and network (can_net_t), the object
///        dictionary contains only the RPDO mapping parameter (0x1600) object
///
/// \When co_rpdo_create() is called with pointers to the network and the
///       device, and an RPDO number
///
/// \Then a null pointer is returned, the error number is set to ERRNUM_INVAL,
///       the RPDO service is not created
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_rpdo_alignof()
///       \Calls co_rpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls errnum2c()
///       \Calls set_errc()
///       \Calls co_rpdo_free()
TEST(CO_RpdoCreate, CoRpdoCreate_NoRpdoCommParam) {
  dev_holder->CreateObj<Obj1600RpdoMapPar>(obj1600);

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);

  POINTERS_EQUAL(nullptr, rpdo);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given initialized device (co_dev_t) and network (can_net_t), the object
///        dictionary contains the RPDO communication parameter (0x1400) and
///        the RPDO mapping parameter (0x1600) objects
///
/// \When co_rpdo_create() is called with pointers to the network and the
///       device, and an RPDO number
///
/// \Then a pointer to the created RPDO service is returned, the service is
///       stopped and configured with the default values
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_rpdo_alignof()
///       \Calls co_rpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls memset()
///       \Calls can_recv_create()
///       \Calls co_rpdo_get_alloc()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls co_sdo_req_init()
TEST(CO_RpdoCreate, CoRpdoCreate_MinimalRPDO) {
  dev_holder->CreateObj<Obj1400RpdoCommPar>(obj1400);
  dev_holder->CreateObj<Obj1600RpdoMapPar>(obj1600);

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);

  CHECK(rpdo != nullptr);
  POINTERS_EQUAL(net, co_rpdo_get_net(rpdo));
  POINTERS_EQUAL(dev, co_rpdo_get_dev(rpdo));
  CHECK_EQUAL(RPDO_NUM, co_rpdo_get_num(rpdo));
  CHECK_TRUE(co_rpdo_is_stopped(rpdo));

  const auto* const comm = co_rpdo_get_comm_par(rpdo);
  CHECK_EQUAL(0, comm->n);
  CHECK_EQUAL(0, comm->cobid);
  CHECK_EQUAL(0, comm->trans);
  CHECK_EQUAL(0, comm->inhibit);
  CHECK_EQUAL(0, comm->reserved);
  CHECK_EQUAL(0, comm->event);
  CHECK_EQUAL(0, comm->sync);

  const auto* const map_par = co_rpdo_get_map_par(rpdo);
  CHECK_EQUAL(0, map_par->n);
  for (size_t i = 0; i < CO_PDO_NUM_MAPS; ++i) CHECK_EQUAL(0, map_par->map[i]);
}

/// \Given initialized device (co_dev_t) and network (can_net_t), the object
///        dictionary contains the RPDO communication parameter (0x15ff) and
///        the RPDO mapping parameter (0x17ff) objects
///
/// \When co_rpdo_create() is called with pointers to the network and the
///       device, and the maximum RPDO number
///
/// \Then a pointer to the created RPDO service is returned, the service is
///       stopped and configured with the default values
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_rpdo_alignof()
///       \Calls co_rpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls memset()
///       \Calls can_recv_create()
///       \Calls co_rpdo_get_alloc()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls co_sdo_req_init()
TEST(CO_RpdoCreate, CoRpdoCreate_MinimalRPDO_MaxNum) {
  const co_unsigned16_t MAX_RPDO_NUM = 0x0200u;

  dev_holder->CreateObj<Obj1400RpdoCommPar>(obj1400, 0x15ffu);
  dev_holder->CreateObj<Obj1600RpdoMapPar>(obj1600, 0x17ffu);

  rpdo = co_rpdo_create(net, dev, MAX_RPDO_NUM);

  CHECK(rpdo != nullptr);
  POINTERS_EQUAL(net, co_rpdo_get_net(rpdo));
  POINTERS_EQUAL(dev, co_rpdo_get_dev(rpdo));
  CHECK_EQUAL(MAX_RPDO_NUM, co_rpdo_get_num(rpdo));
  CHECK_TRUE(co_rpdo_is_stopped(rpdo));

  const auto* const comm = co_rpdo_get_comm_par(rpdo);
  CHECK_EQUAL(0, comm->n);
  CHECK_EQUAL(0, comm->cobid);
  CHECK_EQUAL(0, comm->trans);
  CHECK_EQUAL(0, comm->inhibit);
  CHECK_EQUAL(0, comm->reserved);
  CHECK_EQUAL(0, comm->event);
  CHECK_EQUAL(0, comm->sync);

  const auto* const map_par = co_rpdo_get_map_par(rpdo);
  CHECK_EQUAL(0, map_par->n);
  for (size_t i = 0; i < CO_PDO_NUM_MAPS; ++i) CHECK_EQUAL(0, map_par->map[i]);
}

///@}

/// @name co_rpdo_destroy()
///@{

/// \Given N/A
///
/// \When co_rpdo_destroy() is called with a null RPDO service pointer
///
/// \Then nothing is changed
TEST(CO_RpdoCreate, CoRpdoDestroy_Null) { co_rpdo_destroy(nullptr); }

/// \Given a pointer to an initialized RPDO service (co_rpdo_t)
///
/// \When co_rpdo_destroy() is called with a pointer to the service
///
/// \Then the service is finalized and freed
///       \Calls co_rpdo_stop()
///       \Calls co_sdo_req_fini()
///       \Calls can_timer_destroy()
///       \Calls can_recv_destroy()
///       \Calls mem_free()
///       \Calls co_rpdo_get_alloc()
TEST(CO_RpdoCreate, CoRpdoDestroy_Nominal) {
  dev_holder->CreateObj<Obj1400RpdoCommPar>(obj1400);
  dev_holder->CreateObj<Obj1600RpdoMapPar>(obj1600);
  rpdo = co_rpdo_create(net, dev, RPDO_NUM);
  CHECK(rpdo != nullptr);

  co_rpdo_destroy(rpdo);
  rpdo = nullptr;
}

///@}

TEST_GROUP_BASE(CO_Rpdo, CO_RpdoBase) {
  static const co_unsigned16_t PDO_MAPPED_IDX = 0x2020u;
  static const co_unsigned8_t PDO_MAPPED_SUBIDX = 0x00u;

  co_rpdo_t* rpdo = nullptr;

  int32_t ind_data = 0;
  int32_t err_data = 0;

  void CreateRpdo() {
    rpdo = co_rpdo_create(net, dev, RPDO_NUM);
    CHECK(rpdo != nullptr);
    CHECK(co_rpdo_is_stopped(rpdo));
  }

  void StartRpdo() {
    co_rpdo_set_ind(rpdo, CoRpdoInd::Func, &ind_data);
    co_rpdo_set_err(rpdo, CoRpdoErr::Func, &err_data);

    CHECK(co_rpdo_is_stopped(rpdo));
    CHECK_EQUAL(0, co_rpdo_start(rpdo));
    CHECK_FALSE(co_rpdo_is_stopped(rpdo));
  }

  void SetupRpdo(const co_unsigned32_t cobid,
                 const co_unsigned8_t transmission =
                     Obj1400RpdoCommPar::SYNCHRONOUS_TRANSMISSION) {
    obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub00HighestSubidxSupported>();
    obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub01CobId>(cobid);
    obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub02TransmissionType>(
        transmission);

    obj1600->EmplaceSub<Obj1600RpdoMapPar::Sub00NumOfMappedObjs>(0x01u);
    obj1600->EmplaceSub<Obj1600RpdoMapPar::SubNthAppObject>(
        0x01u, Obj1600RpdoMapPar::MakeMappingParam(PDO_MAPPED_IDX,
                                                   PDO_MAPPED_SUBIDX, 0x40u));

    dev_holder->CreateAndInsertObj(obj2020, PDO_MAPPED_IDX);
    obj2020->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED64, co_unsigned64_t{0});
    co_sub_set_pdo_mapping(obj2020->GetLastSub(), true);
  }

  can_msg CreatePdoMsg_U64(const co_unsigned64_t val) const {
    assert(CAN_MSG_MAX_LEN >= sizeof(val));

    can_msg msg = CAN_MSG_INIT;
    msg.id = DEV_ID;
    msg.len = sizeof(val);
    memcpy(msg.data, &val, msg.len);

    return msg;
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    dev_holder->CreateObj<Obj1400RpdoCommPar>(obj1400);
    dev_holder->CreateObj<Obj1600RpdoMapPar>(obj1600);

    can_net_set_send_func(net, CanSend::Func, nullptr);
  }

  TEST_TEARDOWN() {
    CanSend::Clear();
    CoRpdoInd::Clear();
    CoRpdoErr::Clear();

    co_rpdo_destroy(rpdo);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_rpdo_start()
///@{

/// \Given a pointer to an initialized RPDO service (co_rpdo_t)
///
/// \When co_rpdo_start() is called
///
/// \Then the service is started and configured with the default values
///       \Calls co_dev_find_obj()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_sizeof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_start()
TEST(CO_Rpdo, CoRpdoStart_Nominal) {
  CreateRpdo();

  const auto ret = co_rpdo_start(rpdo);

  CHECK_EQUAL(0, ret);
  CHECK_FALSE(co_rpdo_is_stopped(rpdo));

  const auto* const comm = co_rpdo_get_comm_par(rpdo);
  CHECK_EQUAL(0, comm->n);
  CHECK_EQUAL(0, comm->cobid);
  CHECK_EQUAL(0, comm->trans);
  CHECK_EQUAL(0, comm->inhibit);
  CHECK_EQUAL(0, comm->reserved);
  CHECK_EQUAL(0, comm->event);
  CHECK_EQUAL(0, comm->sync);

  const auto* const map_par = co_rpdo_get_map_par(rpdo);
  CHECK_EQUAL(0, map_par->n);
  for (size_t i = 0; i < CO_PDO_NUM_MAPS; ++i) CHECK_EQUAL(0, map_par->map[i]);

  co_rpdo_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_rpdo_get_ind(rpdo, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(nullptr, pind);
  FUNCTIONPOINTERS_EQUAL(nullptr, pdata);

  co_rpdo_err_t* perr = nullptr;
  void* perrdata = nullptr;
  co_rpdo_get_err(rpdo, &perr, &perrdata);
  FUNCTIONPOINTERS_EQUAL(nullptr, perr);
  FUNCTIONPOINTERS_EQUAL(nullptr, perrdata);
}

/// \Given a pointer to an initialized RPDO service (co_rpdo_t)
///
/// \When co_rpdo_start() is called
///
/// \Then the service is started and the default receiver is started
///       \Calls co_dev_find_obj()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_sizeof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_start()
TEST(CO_Rpdo, CoRpdoStart_NominalRecv) {
  CreateRpdo();
  co_rpdo_set_ind(rpdo, CoRpdoInd::Func, &ind_data);
  co_rpdo_set_err(rpdo, CoRpdoErr::Func, &err_data);

  const auto ret = co_rpdo_start(rpdo);

  CHECK_EQUAL(0, ret);
  CHECK_FALSE(co_rpdo_is_stopped(rpdo));

  const can_msg msg = CAN_MSG_INIT;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  CHECK_EQUAL(0, co_rpdo_sync(rpdo, 0));

  CHECK_EQUAL(1u, CoRpdoInd::GetNumCalled());
  CoRpdoInd::CheckPtrNotNull(rpdo, 0, msg.len, &ind_data);
  CHECK_EQUAL(0, CoRpdoErr::GetNumCalled());
}

/// \Given a pointer to an initialized RPDO service (co_rpdo_t), the object
///        dictionary contains the RPDO communication parameter (0x1400) object
///        with the "COB-ID" entry the CO_PDO_COBID_FRAME bit set
///
/// \When co_rpdo_start() is called
///
/// \Then the service is started and the receiver for messages with the
///       29-bit CAN-ID (CAN extended frame) is started
///       \Calls co_dev_find_obj()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_sizeof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_start()
TEST(CO_Rpdo, CoRpdoStart_ExtendedFrame) {
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub00HighestSubidxSupported>();
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub01CobId>(CO_PDO_COBID_FRAME |
                                                      DEV_ID);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub02TransmissionType>(
      Obj1400RpdoCommPar::EVENT_DRIVEN_TRANSMISSION);
  CreateRpdo();
  co_rpdo_set_ind(rpdo, CoRpdoInd::Func, &ind_data);
  co_rpdo_set_err(rpdo, CoRpdoErr::Func, &err_data);

  const auto ret = co_rpdo_start(rpdo);

  CHECK_EQUAL(0, ret);
  CHECK_FALSE(co_rpdo_is_stopped(rpdo));

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.flags = CAN_FLAG_IDE;

  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CoRpdoInd::GetNumCalled());
  CoRpdoInd::CheckPtrNotNull(rpdo, 0, msg.len, &ind_data);
  CHECK_EQUAL(0, CoRpdoErr::GetNumCalled());
}

/// \Given a pointer to an initialized RPDO service (co_rpdo_t), the object
///        dictionary contains the RPDO communication parameter (0x1400) object
///        with the "COB-ID" entry the CO_PDO_COBID_VALID bit set
///
/// \When co_rpdo_start() is called
///
/// \Then the service is started, but the receiver not started
///       \Calls co_dev_find_obj()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_sizeof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_stop()
TEST(CO_Rpdo, CoRpdoStart_InvalidBit) {
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub00HighestSubidxSupported>();
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub01CobId>(CO_PDO_COBID_VALID |
                                                      DEV_ID);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub02TransmissionType>();
  CreateRpdo();
  co_rpdo_set_ind(rpdo, CoRpdoInd::Func, &ind_data);
  co_rpdo_set_err(rpdo, CoRpdoErr::Func, &err_data);

  const auto ret = co_rpdo_start(rpdo);

  CHECK_EQUAL(0, ret);
  CHECK_FALSE(co_rpdo_is_stopped(rpdo));
  CHECK_EQUAL(0, CoRpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CoRpdoErr::GetNumCalled());
}

/// \Given a pointer to an initialized RPDO service (co_rpdo_t), the object
///        dictionary contains the RPDO communication parameter (0x1400) object
///        with all entires defined
///
/// \When co_rpdo_start() is called
///
/// \Then the service is started and configured with all values from the 0x1400
///       object
///       \Calls co_dev_find_obj()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_sizeof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_start()
TEST(CO_Rpdo, CoRpdoStart_FullRPDOCommParamRecord) {
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub00HighestSubidxSupported>(0x06u);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub02TransmissionType>(0x01u);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub03InhibitTime>(0x0002u);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub04Reserved>(0x03u);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub05EventTimer>(0x0004u);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub06SyncStartValue>(0x05u);
  CreateRpdo();

  const auto ret = co_rpdo_start(rpdo);

  CHECK_EQUAL(0, ret);
  CHECK_FALSE(co_rpdo_is_stopped(rpdo));

  const auto* const comm = co_rpdo_get_comm_par(rpdo);
  CHECK_EQUAL(0x06u, comm->n);
  CHECK_EQUAL(DEV_ID, comm->cobid);
  CHECK_EQUAL(0x01u, comm->trans);
  CHECK_EQUAL(0x0002u, comm->inhibit);
  CHECK_EQUAL(0x03u, comm->reserved);
  CHECK_EQUAL(0x0004u, comm->event);
  CHECK_EQUAL(0x05u, comm->sync);
}

/// \Given a pointer to an initialized RPDO service (co_rpdo_t), the object
///        dictionary contains the RPDO mapping parameter (0x1600) object
///        with all possible mapping entries defined
///
/// \When co_rpdo_start() is called
///
/// \Then the service is started and configured with all values from the 0x1600
///       object
///       \Calls co_dev_find_obj()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_sizeof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_start()
TEST(CO_Rpdo, CoRpdoCreate_FullRPDOMappingParamRecord) {
  obj1600->EmplaceSub<Obj1600RpdoMapPar::Sub00NumOfMappedObjs>(CO_PDO_NUM_MAPS);
  for (co_unsigned8_t i = 0x01u; i <= CO_PDO_NUM_MAPS; ++i)
    obj1600->EmplaceSub<Obj1600RpdoMapPar::SubNthAppObject>(i, i - 1u);
  CreateRpdo();

  const auto ret = co_rpdo_start(rpdo);

  CHECK_EQUAL(0, ret);
  CHECK_FALSE(co_rpdo_is_stopped(rpdo));

  const auto* const map = co_rpdo_get_map_par(rpdo);
  CHECK_EQUAL(CO_PDO_NUM_MAPS, map->n);
  for (size_t i = 0; i < CO_PDO_NUM_MAPS; ++i) CHECK_EQUAL(i, map->map[i]);
}

/// \Given a pointer to an initialized RPDO service (co_rpdo_t), the object
///        dictionary contains the RPDO communication parameter (0x1400) object
///        with all entires defined and one additional entry (illegal)
///
/// \When co_rpdo_start() is called
///
/// \Then the service is started and configured with all values from the 0x1400
///       object, the illegal entry is omitted
///       \Calls co_dev_find_obj()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_sizeof_val()
///       \Calls co_obj_set_dn_ind()
///       \Calls can_recv_start()
TEST(CO_Rpdo, CoRpdoStart_OversizedRPDOCommParamRecord) {
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub00HighestSubidxSupported>(0x07u);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub02TransmissionType>(0x01u);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub03InhibitTime>(0x0002u);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub04Reserved>(0x03u);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub05EventTimer>(0x0004u);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub06SyncStartValue>(0x05u);
  // illegal sub-object
  obj1400->InsertAndSetSub(0x07u, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t{0});
  CreateRpdo();

  const auto ret = co_rpdo_start(rpdo);

  CHECK_EQUAL(0, ret);
  CHECK_FALSE(co_rpdo_is_stopped(rpdo));

  const auto* const comm = co_rpdo_get_comm_par(rpdo);
  CHECK_EQUAL(0x07u, comm->n);
  CHECK_EQUAL(DEV_ID, comm->cobid);
  CHECK_EQUAL(0x01u, comm->trans);
  CHECK_EQUAL(0x0002u, comm->inhibit);
  CHECK_EQUAL(0x03u, comm->reserved);
  CHECK_EQUAL(0x0004u, comm->event);
  CHECK_EQUAL(0x05u, comm->sync);
}

/// \Given a pointer to a started RPDO service (co_rpdo_t)
///
/// \When co_rpdo_start() is called
///
/// \Then nothing is changed
TEST(CO_Rpdo, CoRpdoStart_AlreadyStarted) {
  CreateRpdo();
  StartRpdo();

  const auto ret = co_rpdo_start(rpdo);

  CHECK_EQUAL(0, ret);
  CHECK_FALSE(co_rpdo_is_stopped(rpdo));
}

///@}

/// @name co_rpdo_stop()
///@{

/// \Given a pointer to a started RPDO service (co_rpdo_t)
///
/// \When co_rpdo_stop() is called with a pointer to the service
///
/// \Then the service is stopped
///       \Calls can_timer_stop()
///       \Calls can_timer_start()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_set_dn_ind()
TEST(CO_Rpdo, CoRpdoStop_Nominal) {
  CreateRpdo();
  StartRpdo();

  co_rpdo_stop(rpdo);

  CHECK(co_rpdo_is_stopped(rpdo));
}

///@}

/// @name co_rpdo_get_ind()
///@{

/// \Given a pointer to an initialized RPDO service (co_rpdo_t)
///
/// \When co_rpdo_get_ind() is called with no addresses to store the indication
///       function and user-specified data pointers at
///
/// \Then nothing is changed
TEST(CO_Rpdo, CoRpdoGetInd_Null) {
  CreateRpdo();

  co_rpdo_get_ind(rpdo, nullptr, nullptr);
}

/// \Given a pointer to an initialized RPDO service (co_rpdo_t)
///
/// \When co_rpdo_get_ind() is called with an address to store the indication
///       function pointer and an address to store user-specified data pointer
///
/// \Then both pointers are set to a null pointer (default values)
TEST(CO_Rpdo, CoRpdoGetInd_Nominal) {
  CreateRpdo();

  co_rpdo_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_rpdo_get_ind(rpdo, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(nullptr, pind);
  POINTERS_EQUAL(nullptr, pdata);
}

///@}

/// @name co_rpdo_set_ind()
///@{

/// \Given a pointer to an initialized RPDO service (co_rpdo_t)
///
/// \When co_rpdo_set_ind() is called with a pointer to an indication
///       function and a pointer to user-specified data
///
/// \Then the indication function and the user-specified data pointers are set
///       in the RPDO service
TEST(CO_Rpdo, CoRpdoSetInd_Nominal) {
  int32_t data = 0;
  CreateRpdo();

  co_rpdo_set_ind(rpdo, CoRpdoInd::Func, &data);

  co_rpdo_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_rpdo_get_ind(rpdo, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(CoRpdoInd::Func, pind);
  POINTERS_EQUAL(&data, pdata);
}

///@}

/// @name co_rpdo_get_err()
///@{

/// \Given a pointer to an initialized RPDO service (co_rpdo_t)
///
/// \When co_rpdo_get_err() is called with no addresses to store the error
///       handling function and user-specified data pointers at
///
/// \Then nothing is changed
TEST(CO_Rpdo, CoRpdoGetErr_Null) {
  CreateRpdo();

  co_rpdo_get_err(rpdo, nullptr, nullptr);
}

/// \Given a pointer to an initialized RPDO service (co_rpdo_t)
///
/// \When co_rpdo_get_err() is called with an address to store the error
///       handling function pointer and an address to store user-specified data
///       pointer
///
/// \Then both pointers are set to a null pointer (default values)
TEST(CO_Rpdo, CoRpdoGetErr_Nominal) {
  CreateRpdo();

  co_rpdo_err_t* perr = nullptr;
  void* pdata = nullptr;
  co_rpdo_get_err(rpdo, &perr, &pdata);
  FUNCTIONPOINTERS_EQUAL(nullptr, perr);
  POINTERS_EQUAL(nullptr, pdata);
}

///@}

/// @name co_rpdo_set_err()
///@{

/// \Given a pointer to an initialized RPDO service (co_rpdo_t)
///
/// \When co_rpdo_set_err() is called with a pointer to an error handling
///       function and a pointer to user-specified data
///
/// \Then the error handling function and the user-specified data pointers are
///       set in the RPDO service
TEST(CO_Rpdo, CoRpdoSetErr_Nominal) {
  int32_t data = 0;
  CreateRpdo();

  co_rpdo_set_err(rpdo, CoRpdoErr::Func, &data);

  co_rpdo_err_t* perr = nullptr;
  void* pdata = nullptr;
  co_rpdo_get_err(rpdo, &perr, &pdata);
  FUNCTIONPOINTERS_EQUAL(CoRpdoErr::Func, perr);
  POINTERS_EQUAL(&data, pdata);
}

///@}

/// @name co_rpdo_rtr()
///@{

/// \Given a pointer to a started RPDO service (co_rpdo_t) configured with the
///        CO_PDO_COBID_VALID bit set in the COB-ID
///
/// \When co_rpdo_rtr() is called
///
/// \Then a PDO RTR message is not transmitted
TEST(CO_Rpdo, CoRpdoRtr_RpdoInvalid) {
  SetupRpdo(CO_PDO_COBID_VALID | DEV_ID);
  CreateRpdo();
  StartRpdo();

  const auto ret = co_rpdo_rtr(rpdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to a started RPDO service (co_rpdo_t) configured with all
///        29-bits set in the COB-ID
///
/// \When co_rpdo_rtr() is called
///
/// \Then a PDO RTR message with the correct 11-bit CAN-ID and the RTR flag is
///       transmitted
///       \Calls can_net_send()
TEST(CO_Rpdo, CoRpdoRtr_Nominal) {
  SetupRpdo(CAN_MASK_EID);  // all 29-bits bits set
  CreateRpdo();
  StartRpdo();

  const auto ret = co_rpdo_rtr(rpdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  BITS_EQUAL(CAN_MASK_BID, CanSend::msg.id, CAN_MASK_BID);  // only 11 bits set
  BITS_EQUAL(CAN_FLAG_RTR, CanSend::msg.flags, CAN_FLAG_RTR);
}

/// \Given a pointer to a started RPDO service (co_rpdo_t) configured with the
///       CO_PDO_COBID_FRAME bit set in the COB-ID
///
/// \When co_rpdo_rtr() is called
///
/// \Then a PDO RTR message with the correct 29-bit CAN-ID, the IDE and the RTR
///       flags is transmitted
///       \Calls can_net_send()
TEST(CO_Rpdo, CoRpdoRtr_ExtendedFrame) {
  SetupRpdo(CAN_MASK_EID | CO_PDO_COBID_FRAME);
  CreateRpdo();
  StartRpdo();

  const auto ret = co_rpdo_rtr(rpdo);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  BITS_EQUAL(CAN_MASK_EID, CanSend::msg.id, CAN_MASK_EID);
  BITS_EQUAL(CAN_FLAG_RTR, CanSend::msg.flags, CAN_FLAG_RTR);
  BITS_EQUAL(CAN_FLAG_IDE, CanSend::msg.flags, CAN_FLAG_IDE);
}

///@}

/// @name co_rpdo_sync()
///@{

/// \Given a pointer to a started RPDO service (co_rpdo_t); there is a CAN
///        frame waiting for a SYNC object
///
/// \When co_rpdo_sync() is called with an invalid counter value
///
/// \Then -1 is returned, the error number is set to ERRNUM_INVAL
///       \Calls set_errnum()
TEST(CO_Rpdo, CoRpdoSync_CounterOverLimit) {
  SetupRpdo(DEV_ID, Obj1400RpdoCommPar::SYNCHRONOUS_TRANSMISSION);
  CreateRpdo();
  StartRpdo();

  const co_unsigned64_t val = 0x1234567890abcdefu;
  const can_msg msg = CreatePdoMsg_U64(val);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  const auto ret = co_rpdo_sync(rpdo, 0xffu);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
  CHECK_EQUAL(0, CoRpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CoRpdoErr::GetNumCalled());
  CHECK_EQUAL(0, co_dev_get_val_u64(dev, PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX));
}

/// \Given a pointer to a started RPDO service (co_rpdo_t) configured with the
///        CO_PDO_COBID_VALID bit set in the COB-ID; there is a CAN frame
///        waiting for a SYNC object
///
/// \When co_rpdo_sync() is called with a counter value
///
/// \Then 0 is returned, nothing is changed
TEST(CO_Rpdo, CoRpdoSync_RpdoNotValid) {
  SetupRpdo(CO_PDO_COBID_VALID | DEV_ID,
            Obj1400RpdoCommPar::SYNCHRONOUS_TRANSMISSION);
  CreateRpdo();
  StartRpdo();

  const co_unsigned64_t val = 0x1234567890abcdefu;
  const can_msg msg = CreatePdoMsg_U64(val);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  const auto ret = co_rpdo_sync(rpdo, 0u);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoRpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CoRpdoErr::GetNumCalled());
  CHECK_EQUAL(0, co_dev_get_val_u64(dev, PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX));
}

/// \Given a pointer to a started RPDO service (co_rpdo_t) configured with
///        a non-synchronous transmission type; there is a CAN frame waiting
///        for a SYNC object
///
/// \When co_rpdo_sync() is called with a counter value
///
/// \Then 0 is returned, nothing is changed
TEST(CO_Rpdo, CoRpdoSync_TransmissionNotSynchronous) {
  SetupRpdo(DEV_ID,
            Obj1400RpdoCommPar::RESERVED_TRANSMISSION);  // not synchronous
  CreateRpdo();
  StartRpdo();

  const co_unsigned64_t val = 0x1234567890abcdefu;
  const can_msg msg = CreatePdoMsg_U64(val);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  const auto ret = co_rpdo_sync(rpdo, 0u);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoRpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CoRpdoErr::GetNumCalled());
  CHECK_EQUAL(0, co_dev_get_val_u64(dev, PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX));
}

/// \Given a pointer to a started RPDO service (co_rpdo_t), there is no CAN
///        frame waiting for a SYNC object
///
/// \When co_rpdo_sync() is called with a counter value
///
/// \Then 0 is returned, nothing is changed
///       \Calls can_timer_stop()
///       \Calls co_dev_get_val_u32()
TEST(CO_Rpdo, CoRpdoSync_NoWaitingFrame) {
  SetupRpdo(DEV_ID, Obj1400RpdoCommPar::SYNCHRONOUS_TRANSMISSION);
  CreateRpdo();
  StartRpdo();

  const auto ret = co_rpdo_sync(rpdo, 0u);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, CoRpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CoRpdoErr::GetNumCalled());
  CHECK_EQUAL(0, co_dev_get_val_u64(dev, PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX));
}

/// \Given a pointer to a started RPDO service (co_rpdo_t) configured with no
///        RPDO indication function and no error handling function set; there
///        is a CAN frame waiting for a SYNC object
///
/// \When co_rpdo_sync() is called with a counter value
///
/// \Then 0 is returned, the object mapped into the PDO is updated
///       \Calls can_timer_stop()
///       \Calls co_dev_get_val_u32()
///       \Calls co_pdo_dn()
TEST(CO_Rpdo, CoRpdoSync_Nominal_NoCallbacks) {
  SetupRpdo(DEV_ID, Obj1400RpdoCommPar::SYNCHRONOUS_TRANSMISSION);
  CreateRpdo();
  StartRpdo();
  co_rpdo_set_ind(rpdo, nullptr, nullptr);
  co_rpdo_set_err(rpdo, nullptr, nullptr);

  const co_unsigned64_t val = 0x1234567890abcdefu;
  const can_msg msg = CreatePdoMsg_U64(val);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  const auto ret = co_rpdo_sync(rpdo, 0u);

  CHECK_EQUAL(0, ret);
  MEMCMP_EQUAL(&val, co_dev_get_val(dev, PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX),
               sizeof(val));
}

/// \Given a pointer to a started RPDO service (co_rpdo_t), there is a CAN
///        frame waiting for a SYNC object
///
/// \When co_rpdo_sync() is called with a counter value
///
/// \Then 0 is returned, the frame is processed and the object mapped into the
///       PDO is updated; the RPDO indication function is called with zero
///       abort code, the frame data and the user-specified data pointer, the
///       error handling function is not called
///       \Calls can_timer_stop()
///       \Calls co_dev_get_val_u32()
///       \Calls co_pdo_dn()
TEST(CO_Rpdo, CoRpdoSync_Nominal) {
  SetupRpdo(DEV_ID, Obj1400RpdoCommPar::SYNCHRONOUS_TRANSMISSION);
  CreateRpdo();
  StartRpdo();

  const co_unsigned64_t val = 0x1234567890abcdefu;
  const can_msg msg = CreatePdoMsg_U64(val);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  const auto ret = co_rpdo_sync(rpdo, 0u);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoRpdoInd::GetNumCalled());
  CoRpdoInd::CheckPtrNotNull(rpdo, 0, msg.len, &ind_data);
  CHECK_EQUAL(0, CoRpdoErr::GetNumCalled());
  MEMCMP_EQUAL(&val, co_dev_get_val(dev, PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX),
               sizeof(val));
}

/// \Given a pointer to a started RPDO service (co_rpdo_t) configured with
///        a mapping on a non-existing object; there is a CAN frame waiting for
///        a SYNC object
///
/// \When co_rpdo_sync() is called with a counter value
///
/// \Then -1 is returned, the waiting frame is processed, but the object mapped
///       into the PDO is not updated; the RPDO indication function is invoked
///       with the CO_SDO_AC_NO_OBJ, the frame data and the user-specified data
///       pointer, the error handling function is not called
///       \Calls can_timer_stop()
///       \Calls co_dev_get_val_u32()
///       \Calls co_pdo_dn()
TEST(CO_Rpdo, CoRpdoSync_NonExistingObjectMapping) {
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub00HighestSubidxSupported>();
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub02TransmissionType>(
      Obj1400RpdoCommPar::SYNCHRONOUS_TRANSMISSION);

  obj1600->EmplaceSub<Obj1600RpdoMapPar::Sub00NumOfMappedObjs>(0x01u);
  obj1600->EmplaceSub<Obj1600RpdoMapPar::SubNthAppObject>(
      0x01u, Obj1600RpdoMapPar::MakeMappingParam(PDO_MAPPED_IDX,
                                                 PDO_MAPPED_SUBIDX, 0x40u));
  CreateRpdo();
  StartRpdo();

  const co_unsigned64_t val = 0x1234567890abcdefu;
  const can_msg msg = CreatePdoMsg_U64(val);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  const auto ret = co_rpdo_sync(rpdo, 0u);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(1u, CoRpdoInd::GetNumCalled());
  CoRpdoInd::CheckPtrNotNull(rpdo, CO_SDO_AC_NO_OBJ, sizeof(val), &ind_data);
  CHECK_EQUAL(0, CoRpdoErr::GetNumCalled());

  CHECK_EQUAL(0, co_dev_get_val_u64(dev, PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX));
}

/// \Given a pointer to a started RPDO service (co_rpdo_t), there is a CAN
///        frame waiting for a SYNC object, but the length of the mapped
///        object(s) exceeds the PDO length
///
/// \When co_rpdo_sync() is called with a counter value
///
/// \Then -1 is returned, the waiting frame is processed, but the object mapped
///       into the PDO is not updated; the RPDO indication function is invoked
///       with the CO_SDO_AC_PDO_LEN, the frame data and the user-specified
///       data pointer; the error handling function is invoked with `0x8210`
///       emergency error code, `0x10` error register and the user-specified
///       data pointer
///       \Calls can_timer_stop()
///       \Calls co_dev_get_val_u32()
///       \Calls co_pdo_dn()
TEST(CO_Rpdo, CoRpdoSync_MappedObjectExceedsPdoLength) {
  SetupRpdo(DEV_ID, Obj1400RpdoCommPar::SYNCHRONOUS_TRANSMISSION);
  CreateRpdo();
  StartRpdo();

  const co_unsigned64_t val = 0x1234567890abcdefu;
  can_msg msg = CreatePdoMsg_U64(val);
  msg.len -= 1u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  const auto ret = co_rpdo_sync(rpdo, 0u);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(1u, CoRpdoInd::GetNumCalled());
  CoRpdoInd::CheckPtrNotNull(rpdo, CO_SDO_AC_PDO_LEN, msg.len, &ind_data);
  CHECK_EQUAL(1u, CoRpdoErr::GetNumCalled());
  CoRpdoErr::Check(rpdo, 0x8210u, 0x10u, &err_data);

  CHECK_EQUAL(0, co_dev_get_val_u64(dev, PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX));
}

/// \Given a pointer to a started RPDO service (co_rpdo_t) and there is a CAN
///        frame waiting for a SYNC object, but the PDO length exceeds the
///        length of the mapped object(s)
///
/// \When co_rpdo_sync() is called with a counter value
///
/// \Then -1 is returned, the waiting frame is processed, but the object mapped
///       into the PDO is not updated; the RPDO indication function is invoked
///       with the CO_SDO_AC_PDO_LEN, the frame data and the user-specified
///       data pointer, the error handling function is invoked with `0x8220`
///       emergency error code, `0x10` error register and the user-specified
///       data pointer
///       \Calls can_timer_stop()
///       \Calls co_dev_get_val_u32()
///       \Calls co_pdo_dn()
TEST(CO_Rpdo, CoRpdoSync_PdoLengthExceedsMappedObject) {
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub00HighestSubidxSupported>();
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub02TransmissionType>(
      Obj1400RpdoCommPar::SYNCHRONOUS_TRANSMISSION);

  obj1600->EmplaceSub<Obj1600RpdoMapPar::Sub00NumOfMappedObjs>(0x01u);
  obj1600->EmplaceSub<Obj1600RpdoMapPar::SubNthAppObject>(
      0x01u, Obj1600RpdoMapPar::MakeMappingParam(PDO_MAPPED_IDX,
                                                 PDO_MAPPED_SUBIDX, 0x01u));

  dev_holder->CreateAndInsertObj(obj2020, PDO_MAPPED_IDX);
  obj2020->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0u});
  co_sub_set_pdo_mapping(obj2020->GetLastSub(), true);

  CreateRpdo();
  StartRpdo();

  const co_unsigned64_t val = 0x1234567890abcdefu;
  const can_msg msg = CreatePdoMsg_U64(val);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  const auto ret = co_rpdo_sync(rpdo, 0u);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CoRpdoInd::GetNumCalled());
  CoRpdoInd::CheckPtrNotNull(rpdo, 0, sizeof(val), &ind_data);
  CHECK_EQUAL(1u, CoRpdoErr::GetNumCalled());
  CoRpdoErr::Check(rpdo, 0x8220u, 0x10u, &err_data);

  CHECK_EQUAL(0, co_dev_get_val_u64(dev, PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX));
}

///@}

/// @name RPDO service: received message processing
///@{

/// \Given a pointer to a started RPDO service (co_rpdo_t) configured with a
///        reserved transmission type
///
/// \When a PDO message is received by the service
///
/// \Then the message is ignored
///       \Calls can_timer_stop()
TEST(CO_Rpdo, CoRpdoRecv_ReservedTransmission) {
  SetupRpdo(DEV_ID, Obj1400RpdoCommPar::RESERVED_TRANSMISSION);
  CreateRpdo();
  StartRpdo();

  const co_unsigned64_t val = 0x1234567890abcdefu;
  const can_msg msg = CreatePdoMsg_U64(val);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CoRpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CoRpdoErr::GetNumCalled());
  CHECK_EQUAL(0, co_dev_get_val_u64(dev, PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX));
}

/// \Given a pointer to a started RPDO service (co_rpdo_t) configured with an
///        event-driven transmission type
///
/// \When a PDO message is received by the service
///
/// \Then the message is processed and the object mapped into the PDO is
///       updated; the RPDO indication function is called with zero abort code,
///       the message data and the user-specified data pointer
///       \Calls can_timer_stop()
///       \Calls co_pdo_dn()
TEST(CO_Rpdo, CoRpdoRecv_EventDrivenTransmission) {
  SetupRpdo(DEV_ID, Obj1400RpdoCommPar::EVENT_DRIVEN_TRANSMISSION);
  CreateRpdo();
  StartRpdo();

  const co_unsigned64_t val = 0x1234567890abcdefu;
  const can_msg msg = CreatePdoMsg_U64(val);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CoRpdoInd::GetNumCalled());
  CoRpdoInd::CheckPtrNotNull(rpdo, 0, msg.len, &ind_data);
  CHECK_EQUAL(0, CoRpdoErr::GetNumCalled());

  MEMCMP_EQUAL(&val, co_dev_get_val(dev, PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX),
               sizeof(val));
}

/// \Given a pointer to a started RPDO service (co_rpdo_t) configured with a
///        synchronous transmission type and a synchronous window length; the
///        SYNC timer has been started
///
/// \When the synchronous window time expires before a PDO message is received
///
/// \Then the next received message is ignored
///       \Calls can_timer_stop()
TEST(CO_Rpdo, CoRpdoRecv_SynchronousTransmission_ExpiredSyncWindow) {
  SetupRpdo(DEV_ID, Obj1400RpdoCommPar::SYNCHRONOUS_TRANSMISSION);
  dev_holder->CreateObjValue<Obj1007SyncWindowLength>(obj1007, 1000u);  // 1 ms
  CreateRpdo();
  StartRpdo();

  // start the synchronous window timer
  CHECK_EQUAL(0, co_rpdo_sync(rpdo, 0u));

  // expire the synchronous window
  AdvanceTimeMs(1u);

  // the next received PDO message
  const co_unsigned64_t val = 0x1234567890abcdefu;
  const can_msg msg = CreatePdoMsg_U64(val);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  // the message is ignored
  CHECK_EQUAL(0, CoRpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CoRpdoErr::GetNumCalled());
  CHECK_EQUAL(0, co_dev_get_val_u64(dev, PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX));
}

/// \Given a pointer to a started RPDO service (co_rpdo_t) configured with a
///        synchronous transmission type and the event timer
///
/// \When a PDO message is received by the service
///
/// \Then the event timer is started and after the timer expires the error
///       handling function is called with `0x8250` emergency error code,
///       `0x10` error register and the user-specified data pointer
///       \Calls can_timer_stop()
///       \Calls can_timer_timeout()
TEST(CO_Rpdo, CoRpdoRecv_SynchronousTransmission_EventTimer) {
  SetupRpdo(DEV_ID, Obj1400RpdoCommPar::SYNCHRONOUS_TRANSMISSION);
  obj1400->SetSub<Obj1400RpdoCommPar::Sub00HighestSubidxSupported>(0x05u);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub03InhibitTime>(0u);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub04Reserved>();
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub05EventTimer>(1u);  // 1ms
  CreateRpdo();
  StartRpdo();

  const co_unsigned64_t val = 0x1234567890abcdefu;
  const can_msg msg = CreatePdoMsg_U64(val);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CoRpdoInd::GetNumCalled());
  CHECK_EQUAL(0, CoRpdoErr::GetNumCalled());

  // expire the event timer
  AdvanceTimeMs(1u);

  CHECK_EQUAL(0, CoRpdoInd::GetNumCalled());
  CHECK_EQUAL(1u, CoRpdoErr::GetNumCalled());
  CoRpdoErr::Check(rpdo, 0x8250u, 0x10u, &err_data);
  CHECK_EQUAL(0, co_dev_get_val_u64(dev, PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX));
}

/// \Given a pointer to a started RPDO service (co_rpdo_t) configured with a
///        synchronous transmission type and the event timer
///
/// \When a PDO message is received by the service
///
/// \Then the event timer is started and after the timer expires nothing is
///       changed
///       \Calls can_timer_stop()
///       \Calls can_timer_timeout()
TEST(CO_Rpdo, CoRpdoRecv_SynchronousTransmission_EventTimer_NoErr) {
  SetupRpdo(DEV_ID, Obj1400RpdoCommPar::SYNCHRONOUS_TRANSMISSION);
  obj1400->SetSub<Obj1400RpdoCommPar::Sub00HighestSubidxSupported>(0x05u);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub03InhibitTime>(0u);
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub04Reserved>();
  obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub05EventTimer>(1u);  // 1ms
  CreateRpdo();
  StartRpdo();
  co_rpdo_set_err(rpdo, nullptr, nullptr);

  const co_unsigned64_t val = 0x1234567890abcdefu;
  const can_msg msg = CreatePdoMsg_U64(val);
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0, CoRpdoInd::GetNumCalled());

  // expire the event timer
  AdvanceTimeMs(1u);

  CHECK_EQUAL(0, CoRpdoInd::GetNumCalled());
  CHECK_EQUAL(0, co_dev_get_val_u64(dev, PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX));
}

///@}

TEST_GROUP_BASE(CO_RpdoAllocation, CO_RpdoBase) {
  Allocators::Limited limitedAllocator;
  co_rpdo_t* rpdo = nullptr;

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(limitedAllocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    dev_holder->CreateObj<Obj1400RpdoCommPar>(obj1400);
    dev_holder->CreateObj<Obj1600RpdoMapPar>(obj1600);
  }

  TEST_TEARDOWN() {
    co_rpdo_destroy(rpdo);
    TEST_BASE_TEARDOWN();
  }
};

/// @name co_rpdo_create()
///@{

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to 0 bytes
///
/// \When co_rpdo_create() is called with pointers to the network and the
///       device, and an RPDO number
///
/// \Then a null pointer is returned, the RPDO service is not created and the
///       error number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_rpdo_alignof()
///       \Calls co_rpdo_sizeof()
///       \Calls get_errc()
///       \Calls set_errc()
TEST(CO_RpdoAllocation, CoRpdoCreate_NoMemory) {
  limitedAllocator.LimitAllocationTo(0u);

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);

  POINTERS_EQUAL(nullptr, rpdo);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the RPDO service instance
///
/// \When co_rpdo_create() is called with pointers to the network and the
///       device, and an RPDO number
///
/// \Then a null pointer is returned, the RPDO service is not created and the
///       error number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_rpdo_alignof()
///       \Calls co_rpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls memset()
///       \Calls can_recv_create()
///       \Calls mem_free()
///       \Calls co_rpdo_get_alloc()
///       \Calls get_errc()
///       \Calls set_errc()
TEST(CO_RpdoAllocation, CoRpdoCreate_NoMemoryForRecv) {
  limitedAllocator.LimitAllocationTo(co_rpdo_sizeof());

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);

  POINTERS_EQUAL(nullptr, rpdo);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the RPDO service instance and the
///        CAN receiver
///
/// \When co_rpdo_create() is called with pointers to the network and the
///       device, and an RPDO number
///
/// \Then a null pointer is returned, the RPDO service is not created and the
///       error number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_rpdo_alignof()
///       \Calls co_rpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls memset()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_recv_destroy()
///       \Calls mem_free()
///       \Calls co_rpdo_get_alloc()
///       \Calls get_errc()
///       \Calls set_errc()
TEST(CO_RpdoAllocation, CoRpdoCreate_NoMemoryForTimer) {
  limitedAllocator.LimitAllocationTo(co_rpdo_sizeof() + can_recv_sizeof());

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);

  POINTERS_EQUAL(nullptr, rpdo);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the RPDO service instance, the
///        CAN receiver and one CAN timer
///
/// \When co_rpdo_create() is called with pointers to the network and the
///       device, and an RPDO number
///
/// \Then a null pointer is returned, the RPDO service is not created and the
///       error number is set to ERRNUM_NOMEM
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_rpdo_alignof()
///       \Calls co_rpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls memset()
///       \Calls can_recv_create()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_destroy()
///       \Calls can_recv_destroy()
///       \Calls mem_free()
///       \Calls co_rpdo_get_alloc()
///       \Calls get_errc()
///       \Calls set_errc()
TEST(CO_RpdoAllocation, CoRpdoCreate_NoMemoryForSecondTimer) {
  limitedAllocator.LimitAllocationTo(co_rpdo_sizeof() + can_recv_sizeof() +
                                     can_timer_sizeof());

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);

  POINTERS_EQUAL(nullptr, rpdo);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

/// \Given initialized device (co_dev_t) and network (can_net_t) with a memory
///        allocator limited to only allocate the RPDO service and all required
///        objects
///
/// \When co_rpdo_create() is called with pointers to the network and the
///       device, and an RPDO number
///
/// \Then a pointer to the created RPDO service is returned
///       \Calls mem_alloc()
///       \Calls can_net_get_alloc()
///       \Calls co_rpdo_alignof()
///       \Calls co_rpdo_sizeof()
///       \Calls co_dev_find_obj()
///       \Calls memset()
///       \Calls can_recv_create()
///       \Calls co_rpdo_get_alloc()
///       \Calls can_recv_set_func()
///       \Calls can_timer_create()
///       \Calls can_timer_set_func()
///       \Calls co_sdo_req_init()
TEST(CO_RpdoAllocation, CoRpdoCreate_ExactMemory) {
  limitedAllocator.LimitAllocationTo(co_rpdo_sizeof() + can_recv_sizeof() +
                                     2u * can_timer_sizeof());

  rpdo = co_rpdo_create(net, dev, RPDO_NUM);

  CHECK(rpdo != nullptr);
  CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
}

///@}
