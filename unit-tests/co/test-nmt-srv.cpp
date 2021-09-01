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
#include <utility>
#include <vector>

#include <CppUTest/TestHarness.h>

#include <libtest/allocators/default.hpp>
#include <libtest/allocators/limited.hpp>
#include <libtest/tools/can-send.hpp>
#include <libtest/tools/co-nmt-sync-ind.hpp>
#include <libtest/tools/co-rpdo-ind.hpp>
#include <libtest/tools/co-tpdo-sample-ind.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include <lely/can/msg.h>
#include <lely/can/net.h>

#include <lely/co/csdo.h>
#include <lely/co/dev.h>
#include <lely/co/emcy.h>
#include <lely/co/nmt.h>
#include <lely/co/rpdo.h>
#include <lely/co/ssdo.h>
#include <lely/co/sync.h>
#include <lely/co/tpdo.h>
#include <lely/co/type.h>

#include <lely/util/error.h>

#include "holder/dev.hpp"

#include "obj-init/emcy-error-register.hpp"
#include "obj-init/rpdo-comm-par.hpp"
#include "obj-init/rpdo-map-par.hpp"
#include "obj-init/sdo-client-par.hpp"
#include "obj-init/sdo-server-par.hpp"
#include "obj-init/sync-cobid.hpp"
#include "obj-init/tpdo-comm-par.hpp"
#include "obj-init/tpdo-map-par.hpp"

#include "common/nmt-alloc-sizes.hpp"

TEST_BASE(CO_NmtSrvBase) {
  TEST_BASE_SUPER(CO_NmtSrvBase);

  static const co_unsigned8_t DEV_ID = 0x01u;
  static const co_unsigned16_t RPDO_NUM = 2u;
  static const co_unsigned16_t TPDO_NUM = 2u;
  static const co_unsigned32_t SDO_COBID_REQ = 0x600u + DEV_ID;
  static const co_unsigned32_t SDO_COBID_RES = 0x580u + DEV_ID;
  static const co_unsigned8_t DEFAULT_SSDO_NUM = 1u;
  static const co_unsigned8_t SSDO_NUM = 3u;
  static const co_unsigned8_t CSDO_NUM = 2u;
  static const co_unsigned32_t SYNC_COBID = 0x80u;

  static const co_unsigned16_t PDO_MAPPED_IDX = 0x2020u;
  static const co_unsigned8_t PDO_MAPPED_SUBIDX = 0x00u;
  static const co_unsigned16_t PDO_MAPPED_DEFTYPE = CO_DEFTYPE_UNSIGNED8;
  using pdo_mapped_type = co_unsigned8_t;

  std::unique_ptr<CoDevTHolder> dev_holder;
  std::vector<std::unique_ptr<CoObjTHolder>> objects;

  co_dev_t* dev = nullptr;
  co_nmt_t* nmt = nullptr;

  void CreateObj1001Defaults() {
    std::unique_ptr<CoObjTHolder> obj1001;
    dev_holder->CreateObjValue<Obj1001ErrorRegister>(obj1001);
    objects.push_back(std::move(obj1001));
  }

  void CreateObj1005Defaults() {
    std::unique_ptr<CoObjTHolder> obj1005;
    dev_holder->CreateObjValue<Obj1005CobidSync>(obj1005, SYNC_COBID);
    objects.push_back(std::move(obj1005));
  }

  void CreateObj1201Defaults() {
    std::unique_ptr<CoObjTHolder> obj1200;

    dev_holder->CreateObj<Obj1200SdoServerPar>(
        obj1200, Obj1200SdoServerPar::min_idx + SSDO_NUM - 1u);
    obj1200->EmplaceSub<Obj1200SdoServerPar::Sub00HighestSubidxSupported>(
        0x02u);
    obj1200->EmplaceSub<Obj1200SdoServerPar::Sub01CobIdReq>(SDO_COBID_REQ);
    obj1200->EmplaceSub<Obj1200SdoServerPar::Sub02CobIdRes>(SDO_COBID_RES);

    objects.push_back(std::move(obj1200));
  }

  void CreateObj1280Defaults() {
    std::unique_ptr<CoObjTHolder> obj1280;

    dev_holder->CreateObj<Obj1280SdoClientPar>(
        obj1280, Obj1280SdoClientPar::min_idx + CSDO_NUM - 1u);
    obj1280->EmplaceSub<Obj1280SdoClientPar::Sub00HighestSubidxSupported>(
        0x02u);
    obj1280->EmplaceSub<Obj1280SdoClientPar::Sub01CobIdReq>(SDO_COBID_REQ);
    obj1280->EmplaceSub<Obj1280SdoClientPar::Sub02CobIdRes>(SDO_COBID_RES);

    objects.push_back(std::move(obj1280));
  }

  void CreateObj1400Defaults(const co_unsigned16_t rpdo_num = RPDO_NUM,
                             const co_unsigned8_t tsm = 0xfeu) {
    std::unique_ptr<CoObjTHolder> obj1400;

    dev_holder->CreateObj<Obj1400RpdoCommPar>(
        obj1400, Obj1400RpdoCommPar::min_idx + rpdo_num - 1u);
    obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
    obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub01CobId>(DEV_ID);
    obj1400->EmplaceSub<Obj1400RpdoCommPar::Sub02TransmissionType>(tsm);

    objects.push_back(std::move(obj1400));
  }

  void CreateObj1600Defaults(const co_unsigned16_t rpdo_num = RPDO_NUM) {
    std::unique_ptr<CoObjTHolder> obj1600;

    dev_holder->CreateObj<Obj1600RpdoMapPar>(
        obj1600, Obj1600RpdoMapPar::min_idx + rpdo_num - 1u);
    obj1600->EmplaceSub<Obj1600RpdoMapPar::Sub00NumOfMappedObjs>(0x01u);

    const co_unsigned32_t mapping =
        (static_cast<co_unsigned32_t>(PDO_MAPPED_IDX) << 16u) |
        (static_cast<co_unsigned32_t>(PDO_MAPPED_SUBIDX) << 8u) |
        static_cast<co_unsigned32_t>(co_type_sizeof(PDO_MAPPED_DEFTYPE) * 8u);
    obj1600->EmplaceSub<Obj1600RpdoMapPar::SubNthAppObject>(0x01u, mapping);

    objects.push_back(std::move(obj1600));
  }

  void CreateObj1800Defaults(const co_unsigned16_t tpdo_num = TPDO_NUM,
                             const co_unsigned32_t cobid = DEV_ID,
                             const co_unsigned8_t tsm = 0x00u) {
    std::unique_ptr<CoObjTHolder> obj1800;

    dev_holder->CreateObj<Obj1800TpdoCommPar>(
        obj1800, Obj1800TpdoCommPar::min_idx + tpdo_num - 1u);
    obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub00HighestSubidxSupported>(0x02u);
    obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub01CobId>(cobid);
    obj1800->EmplaceSub<Obj1800TpdoCommPar::Sub02TransmissionType>(tsm);

    objects.push_back(std::move(obj1800));
  }

  void CreateObj1a00Defaults(const co_unsigned16_t tpdo_num = TPDO_NUM) {
    std::unique_ptr<CoObjTHolder> obj1a00;

    dev_holder->CreateObj<Obj1a00TpdoMapPar>(
        obj1a00, Obj1a00TpdoMapPar::min_idx + tpdo_num - 1u);
    obj1a00->EmplaceSub<Obj1a00TpdoMapPar::Sub00NumOfMappedObjs>(0x01u);

    const co_unsigned32_t mapping =
        (static_cast<co_unsigned32_t>(PDO_MAPPED_IDX) << 16u) |
        (static_cast<co_unsigned32_t>(PDO_MAPPED_SUBIDX) << 8u) |
        static_cast<co_unsigned32_t>(co_type_sizeof(PDO_MAPPED_DEFTYPE) * 8u);
    obj1a00->EmplaceSub<Obj1a00TpdoMapPar::SubNthAppObject>(0x01u, mapping);

    objects.push_back(std::move(obj1a00));
  }

  void CreateObj2020PdoMapped() {
    std::unique_ptr<CoObjTHolder> obj2020;
    dev_holder->CreateAndInsertObj(obj2020, PDO_MAPPED_IDX);
    obj2020->InsertAndSetSub(PDO_MAPPED_SUBIDX, PDO_MAPPED_DEFTYPE,
                             pdo_mapped_type{0});
    co_sub_set_pdo_mapping(obj2020->GetLastSub(), true);
    objects.push_back(std::move(obj2020));
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    POINTER_NOT_NULL(dev);
  }

  TEST_TEARDOWN() {
    objects.clear();
    dev_holder.reset();
    set_errnum(ERRNUM_SUCCESS);
  }
};

TEST_GROUP_BASE(CO_NmtSrv, CO_NmtSrvBase) {
  Allocators::Default allocator;
  can_net_t* net = nullptr;

  void CreateNmt() {
    nmt = co_nmt_create(net, dev);
    POINTER_NOT_NULL(nmt);
  }

  void CreateNmtAndReset() {
    CreateNmt();
    CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_RESET_NODE));
    CanSend::Clear();
  }

  can_msg CreatePdoMsg(const co_unsigned32_t cobid, const co_unsigned8_t len)
      const {
    can_msg msg = CAN_MSG_INIT;
    msg.id = cobid;
    msg.len = len;

    return msg;
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    net = can_net_create(allocator.ToAllocT(), 0);
    POINTER_NOT_NULL(net);

    can_net_set_send_func(net, CanSend::Func, nullptr);
  }

  TEST_TEARDOWN() {
    CanSend::Clear();
    CoRpdoInd::Clear();
    CoTpdoSampleInd::Clear();
    CoNmtSyncInd::Clear();

    co_nmt_destroy(nmt);
    can_net_destroy(net);

    TEST_BASE_TEARDOWN();
  }
};

#if LELY_NO_MALLOC

  /// @name the NMT service manager services initialization
  ///@{

#if !LELY_NO_CO_RPDO
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t) with RPDO service(s) partially configured - the object
///        dictionary does not contain an RPDO mapping parameter matching the
///        RPDO communication parameter
///
/// \When an NMT service is created
///
/// \Then only the fully configured RPDO services are initialized
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls mem_alloc()
///       \Calls co_rpdo_create()
///       \Calls co_rpdo_set_err()
TEST(CO_NmtSrv, CoNmtSrvInit_RpdoMissingMappingParameterObject) {
  CreateObj1400Defaults(RPDO_NUM);
  CreateObj1400Defaults(RPDO_NUM + 1u);
  CreateObj1600Defaults(RPDO_NUM + 1u);

  CreateNmt();

  POINTERS_EQUAL(nullptr, co_nmt_get_rpdo(nmt, 0u));
  POINTERS_EQUAL(nullptr, co_nmt_get_rpdo(nmt, RPDO_NUM));
  POINTER_NOT_NULL(co_nmt_get_rpdo(nmt, RPDO_NUM + 1u));
  POINTERS_EQUAL(nullptr, co_nmt_get_rpdo(nmt, RPDO_NUM + 2u));
}

/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t) with RPDO service(s) configured
///
/// \When an NMT service is created
///
/// \Then the configured RPDO services are initialized
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls mem_alloc()
///       \Calls co_rpdo_create()
///       \Calls co_rpdo_set_err()
TEST(CO_NmtSrv, CoNmtSrvInit_RpdoNominal) {
  CreateObj1400Defaults();
  CreateObj1600Defaults();

  CreateNmt();

  const co_rpdo_t* const rpdo = co_nmt_get_rpdo(nmt, RPDO_NUM);
  POINTER_NOT_NULL(rpdo);
  POINTERS_EQUAL(net, co_rpdo_get_net(rpdo));
  POINTERS_EQUAL(dev, co_rpdo_get_dev(rpdo));
  CHECK(co_rpdo_is_stopped(rpdo));
  CHECK_EQUAL(RPDO_NUM, co_rpdo_get_num(rpdo));
}
#else
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t)
///
/// \When an NMT service is created
///
/// \Then no RPDO services are initialized
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
TEST(CO_NmtSrv, CoNmtSrvInit_NoRpdoService) {
  CreateNmt();
  POINTERS_EQUAL(nullptr, co_nmt_get_rpdo(nmt, RPDO_NUM));
}
#endif  // !LELY_NO_CO_RPDO

#if !LELY_NO_CO_TPDO
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t) with TPDO service(s) partially configured - the object
///        dictionary does not contain a TPDO mapping parameter matching the
///        TPDO communication parameter
///
/// \When an NMT service is created
///
/// \Then only the fully configured TPDO services are initialized
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls mem_alloc()
///       \Calls co_tpdo_create()
TEST(CO_NmtSrv, CoNmtSrvInit_TpdoMissingMappingParameterObject) {
  CreateObj1800Defaults(TPDO_NUM);
  CreateObj1800Defaults(TPDO_NUM + 1u);
  CreateObj1a00Defaults(TPDO_NUM + 1u);

  CreateNmt();

  POINTERS_EQUAL(nullptr, co_nmt_get_tpdo(nmt, 0u));
  POINTERS_EQUAL(nullptr, co_nmt_get_tpdo(nmt, TPDO_NUM));
  POINTER_NOT_NULL(co_nmt_get_tpdo(nmt, TPDO_NUM + 1u));
  POINTERS_EQUAL(nullptr, co_nmt_get_tpdo(nmt, TPDO_NUM + 2u));
}

/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t) with TPDO service(s) configured
///
/// \When an NMT service is created
///
/// \Then the configured TPDO services are initialized
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls mem_alloc()
///       \Calls co_tpdo_create()
TEST(CO_NmtSrv, CoNmtSrvInit_TpdoNominal) {
  CreateObj1800Defaults();
  CreateObj1a00Defaults();

  CreateNmt();

  const co_tpdo_t* const tpdo = co_nmt_get_tpdo(nmt, TPDO_NUM);
  POINTER_NOT_NULL(tpdo);
  POINTERS_EQUAL(net, co_tpdo_get_net(tpdo));
  POINTERS_EQUAL(dev, co_tpdo_get_dev(tpdo));
  CHECK(co_tpdo_is_stopped(tpdo));
  CHECK_EQUAL(TPDO_NUM, co_tpdo_get_num(tpdo));
}
#else
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t)
///
/// \When an NMT service is created
///
/// \Then no TPDO services are initialized
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
TEST(CO_NmtSrv, CoNmtSrvInit_NoTpdoService) {
  CreateNmt();
  POINTERS_EQUAL(nullptr, co_nmt_get_tpdo(nmt, TPDO_NUM));
}
#endif  // !LELY_NO_CO_TPDO

#if !LELY_NO_CO_SDO
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t) with Server-SDO service(s) configured
///
/// \When an NMT service is created
///
/// \Then the default and configured Server-SDO services are initialized
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls mem_alloc()
///       \Calls co_ssdo_create()
TEST(CO_NmtSrv, CoNmtSrvInit_ServerSdo_Init) {
  CreateObj1201Defaults();

  CreateNmt();

  const co_ssdo_t* const default_ssdo = co_nmt_get_ssdo(nmt, DEFAULT_SSDO_NUM);
#if !LELY_NO_MALLOC
  POINTERS_EQUAL(nullptr, default_ssdo);
#else
  POINTER_NOT_NULL(default_ssdo);
  CHECK(co_ssdo_is_stopped(default_ssdo));
  CHECK_EQUAL(DEFAULT_SSDO_NUM, co_ssdo_get_num(default_ssdo));
#endif

  const co_ssdo_t* const ssdo = co_nmt_get_ssdo(nmt, SSDO_NUM);
#if !LELY_NO_MALLOC
  POINTERS_EQUAL(nullptr, ssdo);
#else
  POINTER_NOT_NULL(ssdo);
  CHECK(co_ssdo_is_stopped(ssdo));
  CHECK_EQUAL(SSDO_NUM, co_ssdo_get_num(ssdo));
#endif

  POINTERS_EQUAL(nullptr, co_nmt_get_ssdo(nmt, 0u));
  POINTERS_EQUAL(nullptr, co_nmt_get_ssdo(nmt, SSDO_NUM + 1u));
}

/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t) with Server-SDO service(s) created
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset node' command specifier
///
/// \Then the default and configured Server-SDO services are started
///       \Calls co_ssdo_start()
TEST(CO_NmtSrv, CoNmtSrvInit_ServerSdo_Started) {
  CreateObj1201Defaults();

  CreateNmtAndReset();

  const co_ssdo_t* const default_ssdo = co_nmt_get_ssdo(nmt, DEFAULT_SSDO_NUM);
  POINTER_NOT_NULL(default_ssdo);
  CHECK_FALSE(co_ssdo_is_stopped(default_ssdo));
  CHECK_EQUAL(DEFAULT_SSDO_NUM, co_ssdo_get_num(default_ssdo));

  const co_ssdo_t* const ssdo = co_nmt_get_ssdo(nmt, SSDO_NUM);
  POINTER_NOT_NULL(ssdo);
  CHECK_FALSE(co_ssdo_is_stopped(ssdo));
  CHECK_EQUAL(SSDO_NUM, co_ssdo_get_num(ssdo));

  POINTERS_EQUAL(nullptr, co_nmt_get_ssdo(nmt, 0u));
  POINTERS_EQUAL(nullptr, co_nmt_get_ssdo(nmt, SSDO_NUM + 1u));
}

#if !LELY_NO_CO_CSDO
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t) with Client-SDO service(s) configured
///
/// \When an NMT service is created
///
/// \Then the configured Client-SDO services are initialized
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls mem_alloc()
///       \Calls co_csdo_create()
TEST(CO_NmtSrv, CoNmtSrvInit_ClientSdoNominal) {
  CreateObj1280Defaults();

  CreateNmt();

  const co_csdo_t* const csdo = co_nmt_get_csdo(nmt, CSDO_NUM);
  POINTER_NOT_NULL(csdo);
  CHECK(co_csdo_is_stopped(csdo));
  CHECK_EQUAL(CSDO_NUM, co_csdo_get_num(csdo));

  POINTERS_EQUAL(nullptr, co_nmt_get_csdo(nmt, 0u));
  POINTERS_EQUAL(nullptr, co_nmt_get_csdo(nmt, CSDO_NUM + 1u));
}
#else   // !LELY_NO_CO_CSDO
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t)
///
/// \When an NMT service is created
///
/// \Then no Client-SDO services are initialized
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
TEST(CO_NmtSrv, CoNmtSrvInit_NoClientSdoService) {
  CreateNmt();
  POINTERS_EQUAL(nullptr, co_nmt_get_csdo(nmt, CSDO_NUM));
}
#endif  // !LELY_NO_CO_CSDO
#else   // !LELY_NO_CO_SDO
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t)
///
/// \When an NMT service is created
///
/// \Then no Server-SDO services are initialized
TEST(CO_NmtSrv, CoNmtSrvInit_NoServerSdoService) {
  CreateNmt();
  POINTERS_EQUAL(nullptr, co_nmt_get_ssdo(nmt, DEFAULT_SSDO_NUM));
}
#endif  // !LELY_NO_CO_SDO

#if !LELY_NO_CO_SYNC
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t), the object dictionary does not contain the COB-ID SYNC
///        Message object (0x1005)
///
/// \When an NMT service is created
///
/// \Then the SYNC service is not initialized
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
TEST(CO_NmtSrv, CoNmtSrvInit_NoObj1005) {
  CreateNmt();

  POINTERS_EQUAL(nullptr, co_nmt_get_sync(nmt));
}

/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t), the object dictionary contains the COB-ID SYNC Message
///        object (0x1005)
///
/// \When an NMT service is created
///
/// \Then the SYNC service is initialized
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls co_sync_create()
///       \Calls co_sync_set_ind()
///       \Calls co_sync_set_err()
TEST(CO_NmtSrv, CoNmtSrvInit_SyncNominal) {
  CreateObj1005Defaults();

  CreateNmt();

  const co_sync_t* const sync = co_nmt_get_sync(nmt);
  POINTER_NOT_NULL(sync);
  CHECK(co_sync_is_stopped(sync));
}
#else
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t)
///
/// \When an NMT service is created
///
/// \Then the SYNC service is not initialized
TEST(CO_NmtSrv, CoNmtSrvInit_NoSyncService) {
  CreateNmt();
  POINTERS_EQUAL(nullptr, co_nmt_get_sync(nmt));
}
#endif  // !LELY_NO_CO_SYNC

#if LELY_NO_CO_TIME
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t)
///
/// \When an NMT service is created
///
/// \Then the TIME service is not initialized
TEST(CO_NmtSrv, CoNmtSrvInit_NoTimeService) {
  CreateNmt();
  POINTERS_EQUAL(nullptr, co_nmt_get_time(nmt));
}
#endif  // LELY_NO_CO_TIME

#if !LELY_NO_CO_EMCY
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t), the object dictionary does not contain the Error Register
///        object (0x1001)
///
/// \When an NMT service is created
///
/// \Then the EMCY service is not initialized
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
TEST(CO_NmtSrv, CoNmtSrvInit_NoObj1001) {
  CreateNmt();

  POINTERS_EQUAL(nullptr, co_nmt_get_emcy(nmt));
}

/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t), the object dictionary contains the Error Register object
///        (0x1001)
///
/// \When an NMT service is created
///
/// \Then the EMCY service is initialized
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls co_emcy_create()
TEST(CO_NmtSrv, CoNmtSrvInit_EmcyNominal) {
  CreateObj1001Defaults();

  CreateNmt();

  const co_emcy_t* const emcy = co_nmt_get_emcy(nmt);
  POINTER_NOT_NULL(emcy);
  CHECK(co_emcy_is_stopped(emcy));
}
#else
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t)
///
/// \When an NMT service is created
///
/// \Then the EMCY service is not initialized
TEST(CO_NmtSrv, CoNmtSrvInit_NoEmcyService) {
  CreateNmt();
  POINTERS_EQUAL(nullptr, co_nmt_get_emcy(nmt));
}
#endif  // !LELY_NO_CO_EMCY

#if LELY_NO_CO_LSS
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t)
///
/// \When an NMT service is created
///
/// \Then the LSS service is not initialized
TEST(CO_NmtSrv, CoNmtSrvInit_NoLssService) {
  CreateNmt();
  POINTERS_EQUAL(nullptr, co_nmt_get_lss(nmt));
}
#endif  // LELY_NO_CO_LSS

///@}

#endif  // LELY_NO_MALLOC

/// @name the NMT service manager services startup
///@{

#if !LELY_NO_CO_RPDO
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t) with RPDO service(s) configured, a pointer to an
///        initialized NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset node' command specifier
///
/// \Then the configured RPDO services are started
///       \Calls get_errc()
///       \Calls set_errc()
///       \Calls co_rpdo_start()
TEST(CO_NmtSrv, CoNmtSrv_StartRpdo) {
  CreateObj1400Defaults();
  CreateObj1600Defaults();

  CreateNmtAndReset();

  const co_rpdo_t* const rpdo = co_nmt_get_rpdo(nmt, RPDO_NUM);
  POINTER_NOT_NULL(rpdo);
  CHECK_FALSE(co_rpdo_is_stopped(rpdo));
}

#if !LELY_NO_CO_EMCY
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t), with EMCY and RPDO service(s) configured, a pointer to a
///        started NMT service (co_nmt_t)
///
/// \When an RPDO CAN message is received with length not matching configured
///       object mapping
///
/// \Then the EMCY service is notified and the Error Register object (0x1001) is
///       updated
///       \Calls co_nmt_on_err()
TEST(CO_NmtSrv, CoNmtSrv_RpdoErr) {
  CreateObj1001Defaults();
  CreateObj1400Defaults();
  CreateObj1600Defaults();
  CreateNmtAndReset();

  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.len = 0u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0x11u, co_dev_get_val_u8(dev, 0x1001u, 0x00u));
}
#endif  // !LELY_NO_CO_EMCY

#endif  // !LELY_NO_CO_RPDO

#if !LELY_NO_CO_TPDO
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t) with TPDO service(s) configured, a pointer to an
///        initialized NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset node' command specifier
///
/// \Then the configured TPDO services are started
///       \Calls get_errc()
///       \Calls set_errc()
///       \Calls co_tpdo_start()
TEST(CO_NmtSrv, CoNmtSrv_StartTpdo) {
  CreateObj1800Defaults();
  CreateObj1a00Defaults();

  CreateNmtAndReset();

  const co_tpdo_t* const tpdo = co_nmt_get_tpdo(nmt, TPDO_NUM);
  POINTER_NOT_NULL(tpdo);
  CHECK_FALSE(co_tpdo_is_stopped(tpdo));
}
#endif  // !LELY_NO_CO_TPDO

#if !LELY_NO_CO_SDO
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t) with Server-SDO service(s) configured, a pointer to an
///        initialized NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset node' command specifier
///
/// \Then the default and configured Server-SDO services are started
///       \Calls get_errc()
///       \Calls set_errc()
///       \Calls co_ssdo_start()
TEST(CO_NmtSrv, CoNmtSrv_StartServerSdo) {
  CreateObj1201Defaults();

  CreateNmtAndReset();

  const co_ssdo_t* const default_ssdo = co_nmt_get_ssdo(nmt, DEFAULT_SSDO_NUM);
  POINTER_NOT_NULL(default_ssdo);
  CHECK_FALSE(co_ssdo_is_stopped(default_ssdo));

  const co_ssdo_t* const ssdo = co_nmt_get_ssdo(nmt, SSDO_NUM);
  POINTER_NOT_NULL(ssdo);
  CHECK_FALSE(co_ssdo_is_stopped(ssdo));
}

#if !LELY_NO_CO_CSDO
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t) with Client-SDO service(s) configured, a pointer to an
///        initialized NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset node' command specifier
///
/// \Then the configured Client-SDO services are started
///       \Calls get_errc()
///       \Calls set_errc()
///       \Calls co_csdo_start()
TEST(CO_NmtSrv, CoNmtSrv_StartClientSdo) {
  CreateObj1280Defaults();

  CreateNmtAndReset();

  const co_csdo_t* const csdo = co_nmt_get_csdo(nmt, CSDO_NUM);
  POINTER_NOT_NULL(csdo);
  CHECK_FALSE(co_csdo_is_stopped(csdo));
}
#endif  // !LELY_NO_CO_CSDO

#endif  // !LELY_NO_CO_SDO

#if !LELY_NO_CO_SYNC
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t) with the SYNC service configured, a pointer to an
///        initialized NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset node' command specifier
///
/// \Then the SYNC service is started
///       \Calls get_errc()
///       \Calls set_errc()
///       \Calls co_sync_start()
TEST(CO_NmtSrv, CoNmtSrv_StartSync) {
  CreateObj1005Defaults();

  CreateNmtAndReset();

  const co_sync_t* const sync = co_nmt_get_sync(nmt);
  POINTER_NOT_NULL(sync);
  CHECK_FALSE(co_sync_is_stopped(sync));
}

/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t), with the SYNC service configured, a pointer to a started
///        NMT service (co_nmt_t) with a SYNC indication function set
///
/// \When a SYNC message is received
///
/// \Then the SYNC indication function is called
///       \Calls co_nmt_on_sync()
TEST(CO_NmtSrv, CoNmtSrv_SyncInd) {
  CreateObj1005Defaults();
  CreateNmtAndReset();
  co_nmt_set_sync_ind(nmt, &CoNmtSyncInd::Func, nullptr);

  can_msg msg = CAN_MSG_INIT;
  msg.id = SYNC_COBID;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(1u, CoNmtSyncInd::GetNumCalled());
}
#endif  // !LELY_NO_CO_SYNC

#if !LELY_NO_CO_EMCY
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t) with the EMCY service configured, a pointer to an
///        initialized NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset node' command specifier
///
/// \Then the EMCY service is started
///       \Calls get_errc()
///       \Calls set_errc()
///       \Calls co_emcy_start()
TEST(CO_NmtSrv, CoNmtSrv_StartEmcy) {
  CreateObj1001Defaults();

  CreateNmtAndReset();

  const co_emcy_t* const emcy = co_nmt_get_emcy(nmt);
  POINTER_NOT_NULL(emcy);
  CHECK_FALSE(co_emcy_is_stopped(emcy));
}
#endif  // !LELY_NO_CO_EMCY

#if !LELY_NO_CO_EMCY && !LELY_NO_CO_SYNC
/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t), with EMCY and SYNC services configured, a pointer to a
///        started NMT service (co_nmt_t)
///
/// \When a SYNC message with unexpected data length is received
///
/// \Then the EMCY service is notified and the Error Register object (0x1001) is
///       updated
///       \Calls co_nmt_on_err()
TEST(CO_NmtSrv, CoNmtSrv_SyncErr) {
  CreateObj1001Defaults();
  CreateObj1005Defaults();
  CreateNmtAndReset();

  can_msg msg = CAN_MSG_INIT;
  msg.id = SYNC_COBID;
  msg.len = 1u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));

  CHECK_EQUAL(0x11u, co_dev_get_val_u8(dev, 0x1001u, 0x00u));
}
#endif  // !LELY_NO_CO_EMCY && !LELY_NO_CO_SYNC

/// \Given a pointer to a network (can_net_t), a pointer to a device
///        (co_dev_t) with no services configured, a pointer to an
///        initialized NMT service (co_nmt_t)
///
/// \When co_nmt_cs_ind() is called with the NMT 'reset node' command specifier
///
/// \Then no services are initialized; if SDO support is enabled, the default
///       Server-SDO service is started
///       \Calls get_errc()
///       \Calls set_errc()
///       \IfCalls{!LELY_NO_CO_SDO, co_ssdo_start()}
TEST(CO_NmtSrv, CoNmtSrv_StartServices_NoneConfigured) {
  CreateNmtAndReset();

#if LELY_NO_CO_SDO
  POINTERS_EQUAL(nullptr, co_nmt_get_ssdo(nmt, DEFAULT_SSDO_NUM));
#else
  const co_ssdo_t* const default_ssdo = co_nmt_get_ssdo(nmt, DEFAULT_SSDO_NUM);
  POINTER_NOT_NULL(default_ssdo);
  CHECK_FALSE(co_ssdo_is_stopped(default_ssdo));
#endif

  POINTERS_EQUAL(nullptr, co_nmt_get_sync(nmt));
  POINTERS_EQUAL(nullptr, co_nmt_get_time(nmt));
  POINTERS_EQUAL(nullptr, co_nmt_get_emcy(nmt));
}

///@}

/// @name co_nmt_on_sync()
///@{

/// \Given a pointer to a started NMT service (co_nmt_t) with no PDOs
///        configured; a NMT SYNC indication function is set
///
/// \When co_nmt_on_sync() is called with any counter value
///
/// \Then the NMT SYNC indication function is called with the passed counter
///       value and a user-specified data pointer
TEST(CO_NmtSrv, CoNmtOnSync_NoPDOs) {
  CreateNmtAndReset();
  co_nmt_set_sync_ind(nmt, &CoNmtSyncInd::Func, nullptr);
  const co_unsigned8_t cnt = 5u;

  co_nmt_on_sync(nmt, cnt);

  CHECK_EQUAL(1u, CoNmtSyncInd::GetNumCalled());
  CoNmtSyncInd::Check(nmt, cnt, nullptr);
}

/// \Given a pointer to a started NMT service (co_nmt_t) with no PDOs
///        configured; no NMT SYNC indication function is set
///
/// \When co_nmt_on_sync() is called with any counter value
///
/// \Then nothing is changed
TEST(CO_NmtSrv, CoNmtOnSync_NoSyncInd) {
  CreateNmtAndReset();

  co_nmt_on_sync(nmt, 0);
}

/// \Given a pointer to a started NMT service (co_nmt_t) with a Transmit-PDO
///        (synchronous) and a Receive-PDO configured and started; there is
///        a received PDO message waiting for actuation
///
/// \When co_nmt_on_sync() is called with any counter value
///
/// \Then the NMT SYNC indication function is called with the passed counter
///       value and a user-specified data pointer; the transmission of the
///       synchronous PDO is triggered - the Transmit-PDO sampling indication
///       function is called; the actuation of a received synchronous PDO is
///       triggered - the RPDO indication function is called with zero abort
///       code, a pointer to the received bytes (stored internally), a number
///       of the received bytes and a user-specified data pointer
TEST(CO_NmtSrv, CoNmtOnSync_Nominal) {
  CreateObj2020PdoMapped();
#if !LELY_NO_CO_RPDO
  CreateObj1400Defaults(RPDO_NUM, 0x00u);
  CreateObj1600Defaults(RPDO_NUM);
#endif
#if !LELY_NO_CO_TPDO
  CreateObj1800Defaults(TPDO_NUM, DEV_ID, 0x01u);
  CreateObj1a00Defaults(TPDO_NUM);
#endif
  CreateNmtAndReset();

#if !LELY_NO_CO_RPDO
  co_rpdo_set_ind(co_nmt_get_rpdo(nmt, RPDO_NUM), CoRpdoInd::Func, nullptr);
#endif
#if !LELY_NO_CO_TPDO
  co_tpdo_set_sample_ind(co_nmt_get_tpdo(nmt, TPDO_NUM), CoTpdoSampleInd::Func,
                         nullptr);
#endif

  co_nmt_set_sync_ind(nmt, &CoNmtSyncInd::Func, nullptr);

#if !LELY_NO_CO_RPDO
  can_msg msg = CAN_MSG_INIT;
  msg.id = DEV_ID;
  msg.len = 1u;
  CHECK_EQUAL(1, can_net_recv(net, &msg, 0));
  CHECK_EQUAL(0, CoRpdoInd::GetNumCalled());
#endif

  const co_unsigned8_t cnt = 5u;

  co_nmt_on_sync(nmt, cnt);

  CHECK_EQUAL(1u, CoNmtSyncInd::GetNumCalled());
  CoNmtSyncInd::Check(nmt, cnt, nullptr);
#if !LELY_NO_CO_TPDO
  CHECK_EQUAL(1u, CoTpdoSampleInd::GetNumCalled());
  CoTpdoSampleInd::Check(co_nmt_get_tpdo(nmt, TPDO_NUM), nullptr);
#endif
#if !LELY_NO_CO_RPDO
  CHECK_EQUAL(1u, CoRpdoInd::GetNumCalled());
  CoRpdoInd::Check(co_nmt_get_rpdo(nmt, RPDO_NUM), 0, nullptr,
                   co_type_sizeof(PDO_MAPPED_DEFTYPE), nullptr);
#endif
}

///@}

#if !LELY_NO_CO_TPDO

/// @name co_nmt_on_tpdo_event()
///@{

/// \Given a pointer to a started NMT service (co_nmt_t) with no PDOs
///        configured
///
/// \When co_nmt_on_tpdo_event() is called with any PDO number
///
/// \Then nothing is changed
///       \Calls get_errc()
///       \Calls set_errc()
TEST(CO_NmtSrv, CoNmtOnTpdoEvent_NoPDOs) {
  CreateNmtAndReset();

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  co_nmt_on_tpdo_event(nmt, 0);

  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to a started NMT service (co_nmt_t) with at least one
///        Transmit-PDO (event driven) configured
///
/// \When co_nmt_on_tpdo_event() is called with the TPDO number
///
/// \Then the PDO message is transmitted for the configured Transmit-PDO; the
///       error number is not changed
///       \Calls get_errc()
///       \Calls co_nmt_get_tpdo()
///       \Calls co_tpdo_event()
///       \Calls set_errc()
TEST(CO_NmtSrv, CoNmtOnTpdoEvent_SinglePDO_Transmit) {
  CreateObj2020PdoMapped();
  CreateObj1800Defaults(TPDO_NUM, DEV_ID + TPDO_NUM, 0xfeu);
  CreateObj1a00Defaults(TPDO_NUM);
  CreateObj1800Defaults(TPDO_NUM + 1u, DEV_ID + TPDO_NUM, 0xfeu);
  CreateObj1a00Defaults(TPDO_NUM + 1u);
  CreateNmtAndReset();

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  co_nmt_on_tpdo_event(nmt, TPDO_NUM);

  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CanSend::CheckMsg(
      DEV_ID + TPDO_NUM, 0,
      static_cast<uint_least8_t>(co_type_sizeof(PDO_MAPPED_DEFTYPE)), nullptr);
}

/// \Given a pointer to a started NMT service (co_nmt_t) with at least one
///        Transmit-PDO (event driven) configured
///
/// \When co_nmt_on_tpdo_event() is called with the TPDO number, but
///       co_nmt_on_tpdo_event_lock() was called before
///
/// \Then nothing is changed; after co_nmt_on_tpdo_event_unlock() is called
///       the postponed PDO message is transmitted for the configured
///       Transmit-PDO; the error number is not changed
///       \Calls get_errc()
///       \Calls co_nmt_get_tpdo()
///       \Calls set_errc()
TEST(CO_NmtSrv, CoNmtOnTpdoEvent_SinglePDO_Wait) {
  CreateObj2020PdoMapped();
  CreateObj1800Defaults(TPDO_NUM, DEV_ID + TPDO_NUM, 0xfeu);
  CreateObj1a00Defaults(TPDO_NUM);
  CreateObj1800Defaults(CO_NUM_PDOS, DEV_ID + CO_NUM_PDOS, 0xfeu);
  CreateObj1a00Defaults(CO_NUM_PDOS);
  CreateNmtAndReset();

  co_nmt_on_tpdo_event_lock(nmt);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  co_nmt_on_tpdo_event(nmt, TPDO_NUM);

  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());

  co_nmt_on_tpdo_event_unlock(nmt);

  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CanSend::CheckMsg(
      DEV_ID + TPDO_NUM, 0,
      static_cast<uint_least8_t>(co_type_sizeof(PDO_MAPPED_DEFTYPE)), nullptr);
}

/// \Given a pointer to a started NMT service (co_nmt_t) with the last
///        Transmit-PDO (event driven) configured (the TPDO number is equal to
///        CO_NUM_PDOS)
///
/// \When co_nmt_on_tpdo_event() is called with the TPDO number, but
///       co_nmt_on_tpdo_event_lock() was called before
///
/// \Then nothing is changed; after co_nmt_on_tpdo_event_unlock() is called
///       the postponed PDO message is transmitted for the configured last
///       Transmit-PDO; the error number is not changed
///       \Calls get_errc()
///       \Calls co_nmt_get_tpdo()
///       \Calls set_errc()
TEST(CO_NmtSrv, CoNmtOnTpdoEvent_SingleLastPDO_Wait) {
  CreateObj2020PdoMapped();
  CreateObj1800Defaults(TPDO_NUM, DEV_ID + TPDO_NUM, 0xfeu);
  CreateObj1a00Defaults(TPDO_NUM);
  CreateObj1800Defaults(CO_NUM_PDOS, DEV_ID + CO_NUM_PDOS, 0xfeu);
  CreateObj1a00Defaults(CO_NUM_PDOS);
  CreateNmtAndReset();

  co_nmt_on_tpdo_event_lock(nmt);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  co_nmt_on_tpdo_event(nmt, CO_NUM_PDOS);

  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());

  co_nmt_on_tpdo_event_unlock(nmt);

  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CanSend::CheckMsg(
      DEV_ID + CO_NUM_PDOS, 0,
      static_cast<uint_least8_t>(co_type_sizeof(PDO_MAPPED_DEFTYPE)), nullptr);
}

/// \Given a pointer to a started NMT service (co_nmt_t) with many
///        Transmit-PDOs (event driven) configured
///
/// \When co_nmt_on_tpdo_event() is called with a zero PDO number (transmit all
///       PDOs)
///
/// \Then the PDO messages are transmitted for each configured Transmit-PDO,
///       the error number is not changed
///       \Calls get_errc()
///       \Calls co_nmt_get_tpdo()
///       \Calls co_tpdo_event()
///       \Calls set_errc()
TEST(CO_NmtSrv, CoNmtOnTpdoEvent_AllPDOs) {
  CreateObj2020PdoMapped();
  CreateObj1800Defaults(TPDO_NUM, DEV_ID + TPDO_NUM, 0xfeu);
  CreateObj1a00Defaults(TPDO_NUM);
  CreateObj1800Defaults(TPDO_NUM + 1u, DEV_ID + TPDO_NUM + 1u, 0xfeu);
  CreateObj1a00Defaults(TPDO_NUM + 1u);
  CreateNmtAndReset();

  const CanSend::MsgSeq msgs = {
      CreatePdoMsg(DEV_ID + TPDO_NUM, static_cast<uint_least8_t>(
                                          co_type_sizeof(PDO_MAPPED_DEFTYPE))),
      CreatePdoMsg(
          DEV_ID + TPDO_NUM + 1u,
          static_cast<uint_least8_t>(co_type_sizeof(PDO_MAPPED_DEFTYPE))),
  };
  CanSend::SetCheckSeq(msgs);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  co_nmt_on_tpdo_event(nmt, 0);

  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(2u, CanSend::GetNumCalled());
}

/// \Given a pointer to a started NMT service (co_nmt_t) with many
///        Transmit-PDOs (event driven) configured
///
/// \When co_nmt_on_tpdo_event() is called with a zero PDO number (transmit all
///       PDOs), but co_nmt_on_tpdo_event_lock() was called before
///
/// \Then nothing is changed; after co_nmt_on_tpdo_event_unlock() is called
///       the postponed PDO messages are transmitted for each configured
///       Transmit-PDO; the error number is not changed
///       \Calls get_errc()
///       \Calls co_nmt_get_tpdo()
///       \Calls set_errc()
TEST(CO_NmtSrv, CoNmtOnTpdoEvent_AllPDOs_Wait) {
  CreateObj2020PdoMapped();
  CreateObj1800Defaults(TPDO_NUM, DEV_ID + TPDO_NUM, 0xfeu);
  CreateObj1a00Defaults(TPDO_NUM);
  CreateObj1800Defaults(TPDO_NUM + 1u, DEV_ID + TPDO_NUM + 1u, 0xfeu);
  CreateObj1a00Defaults(TPDO_NUM + 1u);
  CreateNmtAndReset();
  co_nmt_on_tpdo_event_lock(nmt);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  co_nmt_on_tpdo_event(nmt, 0);

  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());

  const CanSend::MsgSeq msgs = {
      CreatePdoMsg(DEV_ID + TPDO_NUM, static_cast<uint_least8_t>(
                                          co_type_sizeof(PDO_MAPPED_DEFTYPE))),
      CreatePdoMsg(
          DEV_ID + TPDO_NUM + 1u,
          static_cast<uint_least8_t>(co_type_sizeof(PDO_MAPPED_DEFTYPE))),
  };
  CanSend::SetCheckSeq(msgs);

  co_nmt_on_tpdo_event_unlock(nmt);

  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(2u, CanSend::GetNumCalled());
}

/// \Given a pointer to a started NMT service (co_nmt_t) with a Transmit-PDO
///        (event driven) configured
///
/// \When co_nmt_on_tpdo_event() is called with a PDO number of a non-existing
///       Transmit-PDO, but co_nmt_on_tpdo_event_lock() was called before
///
/// \Then nothing is changed; after co_nmt_on_tpdo_event_unlock() is called
///       no PDO message is transmitted; the error number is not changed
///       \Calls get_errc()
///       \Calls set_errc()
TEST(CO_NmtSrv, CoNmtOnTpdoEvent_NoWaitingPDOs_Unlock) {
  CreateObj2020PdoMapped();
  CreateObj1800Defaults(TPDO_NUM, DEV_ID, 0xfeu);
  CreateObj1a00Defaults();
  CreateNmtAndReset();
  co_nmt_on_tpdo_event_lock(nmt);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  co_nmt_on_tpdo_event(nmt, TPDO_NUM + 1u);

  CHECK_EQUAL(error_num, get_errnum());

  co_nmt_on_tpdo_event_unlock(nmt);

  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to a started NMT service (co_nmt_t) with a Transmit-PDO
///        (event driven) configured
///
/// \When co_nmt_on_tpdo_event() is called with a PDO number of the
///       Transmit-PDO, but co_nmt_on_tpdo_event_lock() was called twice before
///
/// \Then nothing is changed; after co_nmt_on_tpdo_event_unlock() is called
///       no PDO message is transmitted; the error number is not changed
TEST(CO_NmtSrv, CoNmtOnTpdoEvent_DoubleLock_Unlock) {
  CreateObj2020PdoMapped();
  CreateObj1800Defaults(TPDO_NUM, DEV_ID, 0xfeu);
  CreateObj1a00Defaults();
  CreateNmtAndReset();

  co_nmt_on_tpdo_event_lock(nmt);
  co_nmt_on_tpdo_event_lock(nmt);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  co_nmt_on_tpdo_event(nmt, TPDO_NUM + 1u);

  CHECK_EQUAL(error_num, get_errnum());

  co_nmt_on_tpdo_event_unlock(nmt);

  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to a started NMT service (co_nmt_t) with a Transmit-PDO
///        (event driven) configured
///
/// \When co_nmt_on_tpdo_event() is called with a PDO number of the
///       Transmit-PDO, but the NMT service is stopped
///
/// \Then nothing is changed, no PDO message is transmitted; the error number
///       is not changed
///       \Calls get_errc()
///       \Calls co_nmt_get_tpdo()
///       \Calls co_tpdo_event()
///       \Calls set_errc()
TEST(CO_NmtSrv, CoNmtOnTpdoEvent_Stop) {
  CreateObj2020PdoMapped();
  CreateObj1800Defaults(TPDO_NUM, DEV_ID + TPDO_NUM, 0xfeu);
  CreateObj1a00Defaults(TPDO_NUM);
  CreateNmtAndReset();
  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_STOP));

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  co_nmt_on_tpdo_event(nmt, TPDO_NUM);

  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to a started NMT service (co_nmt_t) with a Transmit-PDO
///        (event driven) configured
///
/// \When co_nmt_on_tpdo_event() is called with a PDO number of the
///       Transmit-PDO, but co_nmt_on_tpdo_event_lock() was called before
///
/// \Then nothing is changed; after the service is stopped and
///       co_nmt_on_tpdo_event_unlock() is called no PDO message is
///       transmitted; the error number is not changed
///       \Calls get_errc()
///       \Calls co_nmt_get_tpdo()
///       \Calls co_tpdo_event()
///       \Calls set_errc()
TEST(CO_NmtSrv, CoNmtOnTpdoEvent_Stop_Unlock) {
  CreateObj2020PdoMapped();
  CreateObj1800Defaults(TPDO_NUM, DEV_ID + TPDO_NUM, 0xfeu);
  CreateObj1a00Defaults(TPDO_NUM);
  CreateNmtAndReset();

  co_nmt_on_tpdo_event_lock(nmt);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  co_nmt_on_tpdo_event(nmt, TPDO_NUM);

  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_STOP));

  co_nmt_on_tpdo_event_unlock(nmt);

  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

/// \Given a pointer to a started NMT service (co_nmt_t) with a Transmit-PDO
///        (event driven) configured
///
/// \When co_nmt_on_tpdo_event() is called with a PDO number of the
///       Transmit-PDO, but co_nmt_on_tpdo_event_lock() was called before
///
/// \Then nothing is changed; after the service is stopped, started and then
///       co_nmt_on_tpdo_event_unlock() is called no PDO message is
///       transmitted; the error number is not changed
///       \Calls get_errc()
///       \Calls co_nmt_get_tpdo()
///       \Calls co_tpdo_event()
///       \Calls set_errc()
TEST(CO_NmtSrv, CoNmtOnTpdoEvent_StopStart_Unlock) {
  CreateObj2020PdoMapped();
  CreateObj1800Defaults(TPDO_NUM, DEV_ID + TPDO_NUM, 0xfeu);
  CreateObj1a00Defaults(TPDO_NUM);
  CreateNmtAndReset();

  co_nmt_on_tpdo_event_lock(nmt);

  const errnum_t error_num = ERRNUM_FAULT;
  set_errnum(error_num);

  co_nmt_on_tpdo_event(nmt, TPDO_NUM);

  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_STOP));
  CHECK_EQUAL(0, co_nmt_cs_ind(nmt, CO_NMT_CS_START));

  co_nmt_on_tpdo_event_unlock(nmt);

  CHECK_EQUAL(error_num, get_errnum());
  CHECK_EQUAL(0, CanSend::GetNumCalled());
}

///@}

/// @name the NMT Transmit-PDO event indication function
///@{

/// \Given a pointer to a started NMT service (co_nmt_t) with a Transmit-PDO
///        (event driven) configured
///
/// \When co_dev_tpdo_event() is called for a sub-object mapped into the
///       Transmit-PDO
///
/// \Then the PDO message is transmitted for the configured Transmit-PDO
///       \Calls co_nmt_on_tpdo_event()
TEST(CO_NmtSrv, CoNmtTpdoEventInd_Nominal) {
  CreateObj2020PdoMapped();
  CreateObj1800Defaults(TPDO_NUM, DEV_ID + TPDO_NUM, 0xfeu);
  CreateObj1a00Defaults(TPDO_NUM);
  CreateNmtAndReset();

  co_dev_tpdo_event(dev, PDO_MAPPED_IDX, PDO_MAPPED_SUBIDX);

  CHECK_EQUAL(1u, CanSend::GetNumCalled());
  CanSend::CheckMsg(
      DEV_ID + TPDO_NUM, 0,
      static_cast<uint_least8_t>(co_type_sizeof(PDO_MAPPED_DEFTYPE)), nullptr);
}

///@}

#endif  // !LELY_NO_CO_TPDO

#if LELY_NO_MALLOC

TEST_GROUP_BASE(CO_NmtSrvAllocation, CO_NmtSrvBase) {
  Allocators::Limited limitedAllocator;
  can_net_t* fail_net = nullptr;

  void ExpectNmtCreateFailure() {
    nmt = co_nmt_create(fail_net, dev);

    POINTERS_EQUAL(nullptr, nmt);
    CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
    CHECK_EQUAL(0, limitedAllocator.GetAllocationLimit());
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    fail_net = can_net_create(limitedAllocator.ToAllocT(), 0);
    POINTER_NOT_NULL(fail_net);
  }

  TEST_TEARDOWN() {
    co_nmt_destroy(nmt);
    can_net_destroy(fail_net);

    TEST_BASE_TEARDOWN();
  }
};

  /// @name the NMT service manager services initialization
  ///@{

#if !LELY_NO_CO_RPDO
/// \Given a pointer to a network (can_net_t), a pointer to a device with RPDO
///        service(s) configured; the allocator has not enough memory for an
///        array of pointers to RPDO services
///
/// \When co_nmt_create() is called with a pointer to the device
///
/// \Then a null pointer is returned, the error number it set to ERRNUM_NOMEM
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls mem_alloc()
///       \Calls diag()
///       \Calls mem_free()
TEST(CO_NmtSrvAllocation, CoNmtSrvInit_FailRpdoServiceArrayAllocation) {
  CreateObj1400Defaults();
  CreateObj1600Defaults();

  const size_t dcf_app_par_size = NmtCommon::GetDcfParamsAllocSize(dev);
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() + dcf_app_par_size);

  ExpectNmtCreateFailure();
}

/// \Given a pointer to a network (can_net_t), a pointer to a device with RPDO
///        service(s) configured; the allocator has not enough memory for RPDO
///        services
///
/// \When co_nmt_create() is called with a pointer to the device
///
/// \Then a null pointer is returned, the error number it set to ERRNUM_NOMEM
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls mem_alloc()
///       \Calls co_rpdo_create()
///       \Calls diag()
///       \Calls mem_free()
TEST(CO_NmtSrvAllocation, CoNmtSrvInit_FailRpdoServiceInstanceAllocation) {
  CreateObj1400Defaults();
  CreateObj1600Defaults();

  const size_t dcf_app_par_size = NmtCommon::GetDcfParamsAllocSize(dev);
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() + dcf_app_par_size +
                                     RPDO_NUM * sizeof(co_rpdo_t*));

  ExpectNmtCreateFailure();
}

#endif  // !LELY_NO_CO_RPDO

#if !LELY_NO_CO_TPDO
/// \Given a pointer to a network (can_net_t), a pointer to a device with TPDO
///        service(s) configured; the allocator has not enough memory for an
///        array of pointers to TPDO services
///
/// \When co_nmt_create() is called with a pointer to the device
///
/// \Then a null pointer is returned, the error number it set to ERRNUM_NOMEM
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls mem_alloc()
///       \Calls diag()
///       \Calls mem_free()
TEST(CO_NmtSrvAllocation, CoNmtSrvInit_FailTpdoServiceArrayAllocation) {
  CreateObj1800Defaults();
  CreateObj1a00Defaults();

  const size_t dcf_app_par_size = NmtCommon::GetDcfParamsAllocSize(dev);
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() + dcf_app_par_size);

  ExpectNmtCreateFailure();
}

/// \Given a pointer to a network (can_net_t), a pointer to a device with TPDO
///        service(s) configured; the allocator has not enough memory for TPDO
///        services
///
/// \When co_nmt_create() is called with a pointer to the device
///
/// \Then a null pointer is returned, the error number it set to ERRNUM_NOMEM
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls mem_alloc()
///       \Calls co_tpdo_create()
///       \Calls diag()
///       \Calls mem_free()
TEST(CO_NmtSrvAllocation, CoNmtSrvInit_FailTpdoServiceInstanceAllocation) {
  CreateObj1800Defaults();
  CreateObj1a00Defaults();

  const size_t dcf_app_par_size = NmtCommon::GetDcfParamsAllocSize(dev);
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() + dcf_app_par_size +
                                     TPDO_NUM * sizeof(co_tpdo_t*));

  ExpectNmtCreateFailure();
}

#endif  // !LELY_NO_CO_TPDO

#if !LELY_NO_CO_SDO

/// \Given a pointer to a network (can_net_t), a pointer to a device with
///        Server-SDO service(s) configured; the allocator has not enough memory
///        for an array of pointers to Server-SDO services
///
/// \When co_nmt_create() is called with a pointer to the device
///
/// \Then a null pointer is returned, the error number it set to ERRNUM_NOMEM
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls mem_alloc()
///       \Calls diag()
///       \Calls mem_free()
TEST(CO_NmtSrvAllocation, CoNmtSrvInit_FailServerSdoServiceArrayAllocation) {
  const size_t dcf_app_par_size = NmtCommon::GetDcfParamsAllocSize(dev);
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() + dcf_app_par_size);

  ExpectNmtCreateFailure();
}

/// \Given a pointer to a network (can_net_t), a pointer to a device with
///        Server-SDO service(s) configured; the allocator has not enough memory
///        for Server-SDO services
///
/// \When co_nmt_create() is called with a pointer to the device
///
/// \Then a null pointer is returned, the error number it set to ERRNUM_NOMEM
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls mem_alloc()
///       \Calls co_ssdo_create()
///       \Calls diag()
///       \Calls mem_free()
TEST(CO_NmtSrvAllocation, CoNmtSrvInit_FailServerSdoServiceInstanceAllocation) {
  const size_t dcf_app_par_size = NmtCommon::GetDcfParamsAllocSize(dev);
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() + dcf_app_par_size +
                                     sizeof(co_ssdo_t*));

  ExpectNmtCreateFailure();
}

#if !LELY_NO_CO_CSDO

/// \Given a pointer to a network (can_net_t), a pointer to a device with
///        Client-SDO service(s) configured; the allocator has not enough memory
///        for an array of pointers to Client-SDO services
///
/// \When co_nmt_create() is called with a pointer to the device
///
/// \Then a null pointer is returned, the error number it set to ERRNUM_NOMEM
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls mem_alloc()
///       \Calls diag()
///       \Calls mem_free()
TEST(CO_NmtSrvAllocation, CoNmtSrvInit_FailClientSdoServiceArrayAllocation) {
  CreateObj1280Defaults();

  const size_t dcf_app_par_size = NmtCommon::GetDcfParamsAllocSize(dev);
  const size_t default_ssdo_size = NmtCommon::GetSsdoAllocSize();
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() + dcf_app_par_size +
                                     default_ssdo_size);

  ExpectNmtCreateFailure();
}

/// \Given a pointer to a network (can_net_t), a pointer to a device with
///        Client-SDO service(s) configured; the allocator has not enough memory
///        for Client-SDO services
///
/// \When co_nmt_create() is called with a pointer to the device
///
/// \Then a null pointer is returned, the error number it set to ERRNUM_NOMEM
///       \Calls co_nmt_get_alloc()
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls mem_alloc()
///       \Calls co_ssdo_create()
///       \Calls co_csdo_create()
///       \Calls diag()
///       \Calls mem_free()
TEST(CO_NmtSrvAllocation, CoNmtSrvInit_FailClientSdoServiceInstanceAllocation) {
  CreateObj1280Defaults();

  const size_t dcf_app_par_size = NmtCommon::GetDcfParamsAllocSize(dev);
  const size_t default_ssdo_size = NmtCommon::GetSsdoAllocSize();
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() + dcf_app_par_size +
                                     default_ssdo_size +
                                     CSDO_NUM * sizeof(co_csdo_t*));

  ExpectNmtCreateFailure();
}

#endif  // !LELY_NO_CO_CSDO

#endif  // !LELY_NO_CO_SDO

#if !LELY_NO_CO_SYNC
/// \Given a pointer to a network (can_net_t), a pointer to a device with the
///        COB-ID SYNC Message object (0x1005); the allocator has not enough
///        memory to create the SYNC service
///
/// \When co_nmt_create() is called with a pointer to the device
///
/// \Then a null pointer is returned, the error number it set to ERRNUM_NOMEM
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls co_sync_create()
///       \Calls diag()
TEST(CO_NmtSrvAllocation, CoNmtSrvInit_FailSyncServiceCreation) {
  CreateObj1005Defaults();

  const size_t dcf_app_par_size = NmtCommon::GetDcfParamsAllocSize(dev);
  const size_t default_ssdo_size = NmtCommon::GetSsdoAllocSize();
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() + dcf_app_par_size +
                                     default_ssdo_size);

  ExpectNmtCreateFailure();
}
#endif  // !LELY_NO_CO_SYNC

#if !LELY_NO_CO_EMCY
/// \Given a pointer to a network (can_net_t), a pointer to a device with the
///        Error Register object (0x1001); the allocator has not enough memory
///        to create the EMCY service
///
/// \When co_nmt_create() is called with a pointer to the device
///
/// \Then a null pointer is returned, the error number it set to ERRNUM_NOMEM
///       \Calls co_nmt_get_net()
///       \Calls co_nmt_get_dev()
///       \Calls co_dev_find_obj()
///       \Calls co_emcy_create()
///       \Calls diag()
TEST(CO_NmtSrvAllocation, CoNmtSrvInit_FailEmcyServiceCreation) {
  CreateObj1001Defaults();

  const size_t dcf_app_par_size = NmtCommon::GetDcfParamsAllocSize(dev);
  const size_t default_ssdo_size = NmtCommon::GetSsdoAllocSize();
  limitedAllocator.LimitAllocationTo(co_nmt_sizeof() + dcf_app_par_size +
                                     default_ssdo_size);

  ExpectNmtCreateFailure();
}
#endif  // !LELY_NO_CO_EMCY

///@}

#endif  // LELY_NO_MALLOC
