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

#include <lely/can/msg.h>
#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/pdo.h>
#include <lely/co/sdo.h>
#include <lely/co/type.h>
#include <lely/util/diag.h>
#include <lely/util/endian.h>

#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>
#include <libtest/tools/co-sub-dn-ind.hpp>
#include <libtest/tools/co-sub-up-ind.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/sub.hpp"

#include "obj-init/rpdo-comm-par.hpp"
#include "obj-init/rpdo-map-par.hpp"
#include "obj-init/tpdo-comm-par.hpp"
#include "obj-init/tpdo-map-par.hpp"

TEST_BASE(CO_PdoBase) {
  TEST_BASE_SUPER(CO_PdoBase);

  static const co_unsigned8_t DEV_ID = 0x01u;
  static const co_unsigned16_t IDX = 0x2020u;
  static const co_unsigned8_t SUBIDX = 0x00u;
  static const co_unsigned8_t SUB_LEN = 4u;

  co_dev_t* dev = nullptr;

  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj2020;

  void CreateMappableObject() {
    dev_holder->CreateAndInsertObj(obj2020, IDX);
    obj2020->InsertAndSetSub(SUBIDX, CO_DEFTYPE_UNSIGNED32,
                             co_unsigned32_t{0xdeadbeefu});
    co_sub_t* const sub2021 = obj2020->GetLastSub();
    co_sub_set_access(sub2021, CO_ACCESS_RW);
    co_sub_set_pdo_mapping(sub2021, true);
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);
  }

  TEST_TEARDOWN() { dev_holder.reset(); }
};

TEST_GROUP_BASE(CO_PdoRpdo, CO_PdoBase) {
  static const co_unsigned16_t RPDO_NUM = 0x01u;

  using Sub00HighestSubidxSupported =
      Obj1400RpdoCommPar::Sub00HighestSubidxSupported;
  using Sub01CobId = Obj1400RpdoCommPar::Sub01CobId;
  using Sub02TransmissionType = Obj1400RpdoCommPar::Sub02TransmissionType;
  using Sub03InhibitTime = Obj1400RpdoCommPar::Sub03InhibitTime;
  using Sub04Reserved = Obj1400RpdoCommPar::Sub04Reserved;
  using Sub05EventTimer = Obj1400RpdoCommPar::Sub05EventTimer;
  using Sub06SyncStartValue = Obj1400RpdoCommPar::Sub06SyncStartValue;

  using Sub00NumOfMappedObjs = Obj1600RpdoMapPar::Sub00NumOfMappedObjs;
  using SubNthAppObject = Obj1600RpdoMapPar::SubNthAppObject;
  static constexpr auto MakeMappingParam = Obj1600RpdoMapPar::MakeMappingParam;

  std::unique_ptr<CoObjTHolder> obj1400;
  std::unique_ptr<CoObjTHolder> obj1600;

  TEST_SETUP() {
    TEST_BASE_SETUP();

    dev_holder->CreateObj<Obj1400RpdoCommPar>(obj1400);
    dev_holder->CreateObj<Obj1600RpdoMapPar>(obj1600);
  }
};

/// @name co_dev_chk_rpdo()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_chk_rpdo() is called with an index and a sub-index of a static
///       data type for which the dummy entry is enabled
///
/// \Then 0 is returned
///       \Calls co_type_is_basic()
///       \Calls co_dev_get_dummy()
TEST(CO_PdoRpdo, CoDevChkRpdo_DummyEntryObj_Enabled) {
  const co_unsigned16_t idx = CO_DEFTYPE_INTEGER16;
  co_dev_set_dummy(dev, 1 << CO_DEFTYPE_INTEGER16);

  const auto ret = co_dev_chk_rpdo(dev, idx, 0x00u);

  CHECK_EQUAL(0u, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_chk_rpdo() is called with an index and a sub-index of a static
///       data type for which the dummy entry is disabled
///
/// \Then CO_SDO_AC_NO_OBJ is returned
///       \Calls co_type_is_basic()
///       \Calls co_dev_get_dummy()
TEST(CO_PdoRpdo, CoDevChkRpdo_DummyEntryObj_Disabled) {
  const co_unsigned16_t idx = CO_DEFTYPE_INTEGER16;

  const auto ret = co_dev_chk_rpdo(dev, idx, 0x00u);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_chk_rpdo() is called with an index of a static data type and
///       a non-zero sub-index (illegal entry)
///
/// \Then CO_SDO_AC_NO_OBJ is returned
///       \Calls co_type_is_basic()
///       \Calls co_dev_get_dummy()
///       \Calls co_dev_find_obj()
TEST(CO_PdoRpdo, CoDevChkRpdo_IllegalDummyEntryObj) {
  const co_unsigned16_t obj_idx = CO_DEFTYPE_INTEGER16;
  co_dev_set_dummy(dev, 1 << CO_DEFTYPE_INTEGER16);

  const auto ret = co_dev_chk_rpdo(dev, obj_idx, 0x01u);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_chk_rpdo() is called with an index of a non-existing object
///       and any sub-index
///
/// \Then CO_SDO_AC_NO_OBJ is returned
///       \Calls co_type_is_basic()
///       \Calls co_dev_find_obj()
TEST(CO_PdoRpdo, CoDevChkRpdo_NoObj) {
  const auto ret = co_dev_chk_rpdo(dev, 0xffffu, 0x00u);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_chk_rpdo() is called with an index of an existing object
///       and a sub-index of a non-existing sub-object
///
/// \Then CO_SDO_AC_NO_SUB is returned
///       \Calls co_type_is_basic()
///       \Calls co_dev_find_obj()
///       \Calls co_dev_find_sub()
TEST(CO_PdoRpdo, CoDevChkRpdo_NoSub) {
  CreateMappableObject();

  const auto ret = co_dev_chk_rpdo(dev, IDX, SUBIDX + 1u);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_chk_rpdo() is called with an index and a sub-index of an entry
///       with no write access
///
/// \Then CO_SDO_AC_NO_WRITE is returned
///       \Calls co_type_is_basic()
///       \Calls co_dev_find_obj()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_access()
TEST(CO_PdoRpdo, CoDevChkRpdo_NoWriteAccess) {
  CreateMappableObject();
  co_sub_set_access(obj2020->GetLastSub(), CO_ACCESS_RO);

  const auto ret = co_dev_chk_rpdo(dev, IDX, SUBIDX);

  CHECK_EQUAL(CO_SDO_AC_NO_WRITE, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_chk_rpdo() is called with an index and a sub-index of an entry
///       that has PDO mapping disabled
///
/// \Then CO_SDO_AC_NO_PDO is returned
///       \Calls co_type_is_basic()
///       \Calls co_dev_find_obj()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_access()
///       \Calls co_sub_get_pdo_mapping()
TEST(CO_PdoRpdo, CoDevChkRpdo_NoMappable) {
  CreateMappableObject();
  co_sub_set_pdo_mapping(obj2020->GetLastSub(), false);

  const auto ret = co_dev_chk_rpdo(dev, IDX, SUBIDX);

  CHECK_EQUAL(CO_SDO_AC_NO_PDO, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_chk_rpdo() is called with an index and a sub-index of an entry
///       that cannot be mapped into an RPDO
///
/// \Then CO_SDO_AC_NO_PDO is returned
///       \Calls co_type_is_basic()
///       \Calls co_dev_find_obj()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_access()
///       \Calls co_sub_get_pdo_mapping()
TEST(CO_PdoRpdo, CoDevChkRpdo_NoAccessRpdo) {
  CreateMappableObject();
  co_sub_set_access(obj2020->GetLastSub(), CO_ACCESS_RWR);

  const auto ret = co_dev_chk_rpdo(dev, IDX, SUBIDX);

  CHECK_EQUAL(CO_SDO_AC_NO_PDO, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_chk_rpdo() is called with an index and a sub-index of an entry
///       that can be mapped into a PDO
///
/// \Then 0 is returned
///       \Calls co_type_is_basic()
///       \Calls co_dev_find_obj()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_access()
///       \Calls co_sub_get_pdo_mapping()
TEST(CO_PdoRpdo, CoDevChkRpdo_Nominal) {
  CreateMappableObject();

  const auto ret = co_dev_chk_rpdo(dev, IDX, SUBIDX);

  CHECK_EQUAL(0, ret);
}

///@}

/// @name co_dev_cfg_rpdo()
///@{

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        a malformed RPDO Communication Parameter object (0x1400)
///
/// \When co_dev_cfg_rpdo() is called with an RPDO number, a pointer to the
///       communication parameters (co_pdo_comm_par) and a pointer to the
///       mapping parameters (co_pdo_map_par)
///
/// \Then an error returned by co_dev_cfg_rpdo_comm() is returned, nothing is
///       changed
TEST(CO_PdoRpdo, CoDevCfgRpdo_InvalidRpdoCommParamObj) {
  const co_pdo_comm_par rpdo_comm = CO_PDO_COMM_PAR_INIT;
  const co_pdo_map_par rpdo_map = CO_PDO_MAP_PAR_INIT;

  const auto ret = co_dev_cfg_rpdo(dev, RPDO_NUM + 1u, &rpdo_comm, &rpdo_map);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) and a malformed RPDO
///        Mapping Parameter object (0x1600)
///
/// \When co_dev_cfg_rpdo() is called with an RPDO number, a pointer to the
///       communication parameters (co_pdo_comm_par) and a pointer to the
///       mapping parameters (co_pdo_map_par)
///
/// \Then an error returned by co_dev_cfg_rpdo_map() is returned, nothing is
///       changed
TEST(CO_PdoRpdo, CoDevCfgRpdo_InvalidRpdoMappingParamObj) {
  co_pdo_comm_par rpdo_comm = CO_PDO_COMM_PAR_INIT;
  rpdo_comm.n = 0;
  const co_pdo_map_par rpdo_map = CO_PDO_MAP_PAR_INIT;

  const auto ret = co_dev_cfg_rpdo(dev, RPDO_NUM, &rpdo_comm, &rpdo_map);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with a "COB-ID used
///        by RPDO" entry (0x01) and an RPDO Mapping Parameter object (0x1600)
///        with a "Number of mapped application objects in PDO" entry (0x00);
///        the communication parameters (co_pdo_comm_par) with a COB-ID that
///        does not have the CO_PDO_COBID_VALID bit set; the mapping parameters
///        (co_pdo_map_par) with no application objects
///
/// \When co_dev_cfg_rpdo() is called with an RPDO number, a pointer to the
///       communication parameters and a pointer to the mapping parameters
///
/// \Then 0 is returned, the RPDO Communication Parameters object is configured
///       with the given COB-ID
TEST(CO_PdoRpdo, CoDevCfgRpdo_ReenableRpdo) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x01u);
  obj1400->EmplaceSub<Sub01CobId>(0);
  obj1600->EmplaceSub<Sub00NumOfMappedObjs>(0);

  co_pdo_comm_par rpdo_comm = CO_PDO_COMM_PAR_INIT;
  rpdo_comm.n = 0x01u;
  rpdo_comm.cobid = DEV_ID;
  const co_pdo_map_par rpdo_map = CO_PDO_MAP_PAR_INIT;

  const auto ret = co_dev_cfg_rpdo(dev, RPDO_NUM, &rpdo_comm, &rpdo_map);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(rpdo_comm.n, obj1400->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(rpdo_comm.cobid, obj1400->GetSub<Sub01CobId>());
  CHECK_EQUAL(rpdo_map.n, obj1600->GetSub<Sub00NumOfMappedObjs>());
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with a "COB-ID used
///        by RPDO" entry (0x01) and an RPDO Mapping Parameter object (0x1600)
///        with a "Number of mapped application objects in PDO" entry (0x00);
///        the communication parameters (co_pdo_comm_par) with a COB-ID that
///        has the CO_PDO_COBID_VALID bit set; the mapping parameters
///        (co_pdo_map_par) with no application objects
///
/// \When co_dev_cfg_rpdo() is called with an RPDO number, a pointer to the
///       communication parameters and a pointer to the mapping parameters
///
/// \Then 0 is returned, the RPDO Communication Parameters object is configured
///       with the given COB-ID
TEST(CO_PdoRpdo, CoDevCfgRpdo_DisabledRpdo) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x01u);
  obj1400->EmplaceSub<Sub01CobId>(0);
  obj1600->EmplaceSub<Sub00NumOfMappedObjs>(0);

  co_pdo_comm_par rpdo_comm = CO_PDO_COMM_PAR_INIT;
  rpdo_comm.n = 0x01u;
  rpdo_comm.cobid = DEV_ID | CO_PDO_COBID_VALID;
  const co_pdo_map_par rpdo_map = CO_PDO_MAP_PAR_INIT;

  const auto ret = co_dev_cfg_rpdo(dev, RPDO_NUM, &rpdo_comm, &rpdo_map);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(rpdo_comm.n, obj1400->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(rpdo_comm.cobid, obj1400->GetSub<Sub01CobId>());
  CHECK_EQUAL(rpdo_map.n, obj1600->GetSub<Sub00NumOfMappedObjs>());
}

///@}

/// @name co_dev_cfg_rpdo_comm()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number equal to zero
///       and a pointer to the communication parameters (co_pdo_comm_par)
///
/// \Then CO_SDO_AC_NO_OBJ is returned
TEST(CO_PdoRpdo, CoDevCfgRpdoComm_NumZero) {
  const co_pdo_comm_par rpdo_comm = CO_PDO_COMM_PAR_INIT;

  const auto ret = co_dev_cfg_rpdo_comm(dev, 0u, &rpdo_comm);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number larger than
///       CO_NUM_PDOS and a pointer to the communication parameters
///       (co_pdo_comm_par)
///
/// \Then CO_SDO_AC_NO_OBJ is returned
TEST(CO_PdoRpdo, CoDevCfgRpdoComm_NumOverMax) {
  const co_pdo_comm_par rpdo_comm = CO_PDO_COMM_PAR_INIT;

  const auto ret = co_dev_cfg_rpdo_comm(dev, CO_NUM_PDOS + 1u, &rpdo_comm);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with the "COB-ID
///        used by RPDO" entry (0x01)
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters
///
/// \Then 0 is returned, values from the communication parameters are
///       configured in the object 0x1400
TEST(CO_PdoRpdo, CoDevCfgRpdoComm_Nominal) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x01u);
  obj1400->EmplaceSub<Sub01CobId>(0);

  co_pdo_comm_par rpdo_comm = CO_PDO_COMM_PAR_INIT;
  rpdo_comm.n = 0x01u;
  rpdo_comm.cobid = DEV_ID;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM, &rpdo_comm);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(rpdo_comm.n, obj1400->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(rpdo_comm.cobid, obj1400->GetSub<Sub01CobId>());
}

///@}

/// @name co_dev_cfg_rpdo_map()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_cfg_rpdo_map() is called with an RPDO number equal to zero and
///       a pointer to the mapping parameters (co_pdo_map_par)
///
/// \Then CO_SDO_AC_NO_OBJ is returned
TEST(CO_PdoRpdo, CoDevCfgRpdoMap_NumZero) {
  const co_pdo_map_par rpdo_map = CO_PDO_MAP_PAR_INIT;

  const auto ret = co_dev_cfg_rpdo_map(dev, 0u, &rpdo_map);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_cfg_rpdo_map() is called with an RPDO number larger than
///       CO_NUM_PDOS and a pointer to the mapping parameters (co_pdo_map_par)
///
/// \Then CO_SDO_AC_NO_OBJ is returned
TEST(CO_PdoRpdo, CoDevCfgRpdoMap_NumOverMax) {
  const co_pdo_map_par rpdo_map = CO_PDO_MAP_PAR_INIT;

  const auto ret = co_dev_cfg_rpdo_map(dev, CO_NUM_PDOS + 1u, &rpdo_map);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Mapping Parameter object (0x1600) with some mapping entries
///
/// \When co_dev_cfg_rpdo_map() is called with an RPDO number and a pointer to
///       the mapping parameters (co_pdo_map_par)
///
/// \Then 0 is returned, values from the mapping parameters are configured in
///       the object 0x1600
TEST(CO_PdoRpdo, CoDevCfgRpdoMap_Nominal) {
  obj1600->EmplaceSub<Sub00NumOfMappedObjs>(0);
  obj1600->EmplaceSub<SubNthAppObject>(0x01u, 0);

  co_pdo_map_par rpdo_map = CO_PDO_MAP_PAR_INIT;
  rpdo_map.n = 0x01u;
  rpdo_map.map[0] = MakeMappingParam(0x2000u, 0x00u, 0x00u);

  const auto ret = co_dev_cfg_rpdo_map(dev, RPDO_NUM, &rpdo_map);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(rpdo_map.n, obj1600->GetSub<Sub00NumOfMappedObjs>());
  CHECK_EQUAL(rpdo_map.map[0], obj1600->GetSub<SubNthAppObject>(0x01u));
}

///@}

/// @name co_dev_cfg_rpdo_comm()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters (co_pdo_comm_par)
///
/// \Then CO_SDO_AC_NO_OBJ is returned, nothing is changed
TEST(CO_PdoRpdo, CoDevCfgPdoComm_NoObj) {
  const co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM + 1u, &comm);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with less
///        sub-objects than requested in the communication parameters
///        (co_pdo_comm_par)
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters
///
/// \Then CO_SDO_AC_NO_OBJ is returned, nothing is changed
TEST(CO_PdoRpdo, CoDevCfgPdoComm_NoSubs) {
  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0x03u;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM, &comm);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with no "COB-ID
///        used by RPDO" entry (0x01); the communication parameters
///        (co_pdo_comm_par) that have `cobid` field defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters
///
/// \Then CO_SDO_AC_NO_SUB is returned, nothing is changed
TEST(CO_PdoRpdo, CoDevCfgPdoComm_NoSub01Cobid) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x01u);

  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0x01u;
  comm.cobid = DEV_ID;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM, &comm);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
  CHECK_EQUAL(0x01u, obj1400->GetSub<Sub00HighestSubidxSupported>());
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with the "COB-ID
///        used by RPDO" entry (0x01), but the entry has an invalid data type;
///        the communication parameters (co_pdo_comm_par) that have `cobid`
///        field defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters
///
/// \Then CO_SDO_AC_TYPE_LEN is returned, nothing is changed
TEST(CO_PdoRpdo, CoDevCfgPdoComm_Sub01Cobid_InvalidType) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x02u);
  obj1400->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0u});

  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0x02u;
  comm.cobid = DEV_ID;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM, &comm);

  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN, ret);
  CHECK_EQUAL(0x02u, obj1400->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(0u, co_dev_get_val_u8(dev, 0x1400u, 0x01u));
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with the "COB-ID
///        used by RPDO" entry (0x01); the communication parameters
///        (co_pdo_comm_par) that have `cobid` field defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters
///
/// \Then 0 is returned, all defined values from the communication parameters
///       are configured in the object 0x1400
TEST(CO_PdoRpdo, CoDevCfgPdoComm_Sub01Cobid_Nominal) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x01u);
  obj1400->EmplaceSub<Sub01CobId>(0);

  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0x01u;
  comm.cobid = DEV_ID;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM, &comm);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(comm.n, obj1400->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(comm.cobid, obj1400->GetSub<Sub01CobId>());
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with the "COB-ID
///        used by RPDO" entry (0x01) and no "Transmission type" entry (0x02);
///        the communication parameters (co_pdo_comm_par) that have `cobid` and
///        `trans` fields defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters
///
/// \Then CO_SDO_AC_NO_SUB is returned, the defined values from the
///       communication parameters up to, but without, `trans` are configured
///       in the object 0x1400
TEST(CO_PdoRpdo, CoDevCfgPdoComm_NoSub02TransmissionType) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x02u);
  obj1400->EmplaceSub<Sub01CobId>(0);

  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0x02u;
  comm.cobid = DEV_ID;
  comm.trans = Obj1400RpdoCommPar::EVENT_DRIVEN_TRANSMISSION;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM, &comm);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
  CHECK_EQUAL(0x02u, obj1400->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(comm.cobid, obj1400->GetSub<Sub01CobId>());
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with the "COB-ID
///        used by RPDO" entry (0x01) and the "Transmission type" entry (0x02),
///        but the entry has an invalid data type; the communication parameters
///        (co_pdo_comm_par) that have `cobid` and `trans` fields defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters
///
/// \Then CO_SDO_AC_TYPE_LEN is returned, the defined values from the
///       communication parameters up to, but without, `trans` are configured
///       in the object 0x1400
TEST(CO_PdoRpdo, CoDevCfgPdoComm_Sub02TransmissionType_InvalidType) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x03u);
  obj1400->EmplaceSub<Sub01CobId>(0);
  obj1400->InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t{0u});

  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0x03u;
  comm.cobid = DEV_ID;
  comm.trans = Obj1400RpdoCommPar::EVENT_DRIVEN_TRANSMISSION;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM, &comm);

  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN, ret);
  CHECK_EQUAL(0x03u, obj1400->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(comm.cobid, obj1400->GetSub<Sub01CobId>());
  CHECK_EQUAL(0u, co_dev_get_val_u32(dev, 0x1400u, 0x02u));
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with the "COB-ID
///        used by RPDO" entry (0x01) and the "Transmission type" entry (0x02);
///        the communication parameters (co_pdo_comm_par) that have `cobid`
///        and `trans` fields defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters
///
/// \Then 0 is returned, all defined values from the communication parameters
///       are configured in the object 0x1400
TEST(CO_PdoRpdo, CoDevCfgPdoComm_Sub02TransmissionType_Nominal) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x02u);
  obj1400->EmplaceSub<Sub01CobId>(0);
  obj1400->EmplaceSub<Sub02TransmissionType>(0);

  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0x02u;
  comm.cobid = DEV_ID;
  comm.trans = Obj1400RpdoCommPar::EVENT_DRIVEN_TRANSMISSION;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM, &comm);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(0x02u, obj1400->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(comm.cobid, obj1400->GetSub<Sub01CobId>());
  CHECK_EQUAL(comm.trans, obj1400->GetSub<Sub02TransmissionType>());
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with the "COB-ID
///        used by RPDO" entry (0x01), the "Transmission type" entry (0x02) and
///        no "Inhibit time" entry (0x03); the communication parameters
///        (co_pdo_comm_par) that have `cobid`, `trans` and `inhibit` fields
///        defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters
///
/// \Then CO_SDO_AC_NO_SUB is returned, the defined values from the
///       communication parameters up to, but without, `inhibit` are configured
///       in the object 0x1400
TEST(CO_PdoRpdo, CoDevCfgPdoComm_NoSub03InhibitTime) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x03u);
  obj1400->EmplaceSub<Sub01CobId>(0);
  obj1400->EmplaceSub<Sub02TransmissionType>(0);

  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0x03u;
  comm.cobid = DEV_ID;
  comm.trans = Obj1400RpdoCommPar::EVENT_DRIVEN_TRANSMISSION;
  comm.inhibit = 0x1234u;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM, &comm);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
  CHECK_EQUAL(0x03u, obj1400->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(comm.cobid, obj1400->GetSub<Sub01CobId>());
  CHECK_EQUAL(comm.trans, obj1400->GetSub<Sub02TransmissionType>());
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with the "COB-ID
///        used by RPDO" entry (0x01), the "Transmission type" entry (0x02)
///        and the "Inhibit time" entry (0x03), but the entry has an invalid
///        data type; the communication parameters (co_pdo_comm_par) that have
///        `cobid`, `trans` and `inhibit` fields defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters
///
/// \Then CO_SDO_AC_TYPE_LEN is returned, the defined values from the
///       communication parameters up to, but without, `inhibit` are configured
///       in the object 0x1400
TEST(CO_PdoRpdo, CoDevCfgPdoComm_Sub03InhibitTime_InvalidType) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x05u);
  obj1400->EmplaceSub<Sub01CobId>(0);
  obj1400->EmplaceSub<Sub02TransmissionType>(0);
  obj1400->InsertAndSetSub(0x03u, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t{0u});

  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0x05u;
  comm.cobid = DEV_ID;
  comm.trans = Obj1400RpdoCommPar::EVENT_DRIVEN_TRANSMISSION;
  comm.inhibit = 0x1234u;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM, &comm);

  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN, ret);
  CHECK_EQUAL(0x05u, obj1400->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(comm.cobid, obj1400->GetSub<Sub01CobId>());
  CHECK_EQUAL(comm.trans, obj1400->GetSub<Sub02TransmissionType>());
  CHECK_EQUAL(0u, co_dev_get_val_u32(dev, 0x1400u, 0x03u));
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with the "COB-ID
///        used by RPDO" entry (0x01), the "Transmission type" entry (0x02) and
///        the "Inhibit time" entry (0x03); the communication parameters
///        (co_pdo_comm_par) that have `cobid`, `trans`, `inhibit` fields
///        defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters
///
/// \Then 0 is returned, all defined values from the communication parameters
///       are configured in the object 0x1400
TEST(CO_PdoRpdo, CoDevCfgPdoComm_Sub03InhibitTime_Nominal) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x03u);
  obj1400->EmplaceSub<Sub01CobId>(0);
  obj1400->EmplaceSub<Sub02TransmissionType>(0);
  obj1400->EmplaceSub<Sub03InhibitTime>(0);

  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0x03u;
  comm.cobid = DEV_ID;
  comm.trans = Obj1400RpdoCommPar::EVENT_DRIVEN_TRANSMISSION;
  comm.inhibit = 0x1234u;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM, &comm);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(0x03u, obj1400->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(comm.cobid, obj1400->GetSub<Sub01CobId>());
  CHECK_EQUAL(comm.trans, obj1400->GetSub<Sub02TransmissionType>());
  CHECK_EQUAL(comm.inhibit, obj1400->GetSub<Sub03InhibitTime>());
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with the "COB-ID
///        used by RPDO" entry (0x01), the "Transmission type" entry (0x02),
///        the "Inhibit time" entry (0x03), the "Compatibility entry" entry
///        (0x04) and no "Event timer" entry (0x05); the communication
///        parameters (co_pdo_comm_par) that have `cobid`, `trans`, `inhibit`,
///        `reserved` and `event` fields defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters
///
/// \Then CO_SDO_AC_NO_SUB is returned, the defined values from the
///       communication parameters (except for `reserved`) up to, but without,
///       `event` are configured in the object 0x1400
TEST(CO_PdoRpdo, CoDevCfgPdoComm_NoSub05EventTimer) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x05u);
  obj1400->EmplaceSub<Sub01CobId>(0);
  obj1400->EmplaceSub<Sub02TransmissionType>(0);
  obj1400->EmplaceSub<Sub03InhibitTime>(0);
  obj1400->EmplaceSub<Sub04Reserved>();

  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0x05u;
  comm.cobid = DEV_ID;
  comm.trans = Obj1400RpdoCommPar::EVENT_DRIVEN_TRANSMISSION;
  comm.inhibit = 0x1234u;
  comm.reserved = 0xffu;
  comm.event = 0xabcdu;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM, &comm);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
  CHECK_EQUAL(0x05u, obj1400->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(comm.cobid, obj1400->GetSub<Sub01CobId>());
  CHECK_EQUAL(comm.trans, obj1400->GetSub<Sub02TransmissionType>());
  CHECK_EQUAL(comm.inhibit, obj1400->GetSub<Sub03InhibitTime>());
  CHECK_EQUAL(0, obj1400->GetSub<Sub04Reserved>());
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with the "COB-ID
///        used by RPDO" entry (0x01), the "Transmission type" entry (0x02),
///        the "Inhibit time" entry (0x03), the "Compatibility entry" entry
///        (0x04) and the "Event timer" entry (0x05), but the entry has an
///        invalid data type; the communication parameters (co_pdo_comm_par)
///        that have `cobid`, `trans`, `inhibit`, `reserved` and `event` fields
///        defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters
///
/// \Then CO_SDO_AC_TYPE_LEN is returned, the defined values from the
///       communication parameters (except for `reserved`) up to, but without,
///       `event` are configured in the object 0x1400
TEST(CO_PdoRpdo, CoDevCfgPdoComm_Sub05EventTimer_InvalidType) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x06u);
  obj1400->EmplaceSub<Sub01CobId>(0);
  obj1400->EmplaceSub<Sub02TransmissionType>(0);
  obj1400->EmplaceSub<Sub03InhibitTime>(0);
  obj1400->EmplaceSub<Sub04Reserved>();
  obj1400->InsertAndSetSub(0x05u, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t{0u});

  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0x06u;
  comm.cobid = DEV_ID;
  comm.trans = Obj1400RpdoCommPar::EVENT_DRIVEN_TRANSMISSION;
  comm.inhibit = 0x1234u;
  comm.reserved = 0xffu;
  comm.event = 0xabcdu;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM, &comm);

  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN, ret);
  CHECK_EQUAL(0x06u, obj1400->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(comm.cobid, obj1400->GetSub<Sub01CobId>());
  CHECK_EQUAL(comm.trans, obj1400->GetSub<Sub02TransmissionType>());
  CHECK_EQUAL(comm.inhibit, obj1400->GetSub<Sub03InhibitTime>());
  CHECK_EQUAL(0, obj1400->GetSub<Sub04Reserved>());
  CHECK_EQUAL(0, co_dev_get_val_u32(dev, 0x1400u, 0x05u));
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with the "COB-ID
///        used by RPDO" entry (0x01), the "Transmission type" entry (0x02),
///        the "Inhibit time" entry (0x03), the "Compatibility entry" entry
///        (0x04) and the "Event timer" entry (0x05); the communication
///        parameters (co_pdo_comm_par) that have `cobid`, `trans`, `inhibit`,
///        `reserved` and `event` fields defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters
///
/// \Then 0 is returned, all defined values from the communication parameters
///       (except for `reserved`) are configured in the object 0x1400
TEST(CO_PdoRpdo, CoDevCfgPdoComm_Sub05EventTimer_Nominal) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x05u);
  obj1400->EmplaceSub<Sub01CobId>(0);
  obj1400->EmplaceSub<Sub02TransmissionType>(0);
  obj1400->EmplaceSub<Sub03InhibitTime>(0);
  obj1400->EmplaceSub<Sub04Reserved>();
  obj1400->EmplaceSub<Sub05EventTimer>(0);

  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0x05u;
  comm.cobid = DEV_ID;
  comm.trans = Obj1400RpdoCommPar::EVENT_DRIVEN_TRANSMISSION;
  comm.inhibit = 0x1234u;
  comm.reserved = 0xffu;
  comm.event = 0xabcdu;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM, &comm);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(0x05u, obj1400->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(comm.cobid, obj1400->GetSub<Sub01CobId>());
  CHECK_EQUAL(comm.trans, obj1400->GetSub<Sub02TransmissionType>());
  CHECK_EQUAL(comm.inhibit, obj1400->GetSub<Sub03InhibitTime>());
  CHECK_EQUAL(0, obj1400->GetSub<Sub04Reserved>());
  CHECK_EQUAL(comm.event, obj1400->GetSub<Sub05EventTimer>());
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with the "COB-ID
///        used by RPDO" entry (0x01), the "Transmission type" entry (0x02),
///        the "Inhibit time" entry (0x03), the "Compatibility entry" entry
///        (0x04), the "Event timer" entry (0x05) and no "SYNC start value"
///        entry (0x06); the communication parameters (co_pdo_comm_par) that
///        have `cobid`, `trans`, `inhibit`, `reserved`, `event` and `sync`
///        fields defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters
///
/// \Then CO_SDO_AC_NO_SUB is returned, the defined values from the
///       communication parameters (except for `reserved`) up to, but without,
///       `sync` are configured in the object 0x1400
TEST(CO_PdoRpdo, CoDevCfgPdoComm_NoSub06SyncStartValue) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x06u);
  obj1400->EmplaceSub<Sub01CobId>(0);
  obj1400->EmplaceSub<Sub02TransmissionType>(0);
  obj1400->EmplaceSub<Sub03InhibitTime>(0);
  obj1400->EmplaceSub<Sub04Reserved>();
  obj1400->EmplaceSub<Sub05EventTimer>(0);

  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0x06u;
  comm.cobid = DEV_ID;
  comm.trans = Obj1400RpdoCommPar::EVENT_DRIVEN_TRANSMISSION;
  comm.inhibit = 0x1234u;
  comm.reserved = 0xffu;
  comm.event = 0xabcdu;
  comm.sync = 0x42u;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM, &comm);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
  CHECK_EQUAL(0x06u, obj1400->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(comm.cobid, obj1400->GetSub<Sub01CobId>());
  CHECK_EQUAL(comm.trans, obj1400->GetSub<Sub02TransmissionType>());
  CHECK_EQUAL(comm.inhibit, obj1400->GetSub<Sub03InhibitTime>());
  CHECK_EQUAL(0, obj1400->GetSub<Sub04Reserved>());
  CHECK_EQUAL(comm.event, obj1400->GetSub<Sub05EventTimer>());
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with the "COB-ID
///        used by RPDO" entry (0x01), the "Transmission type" entry (0x02),
///        the "Inhibit time" entry (0x03), the "Compatibility entry" entry
///        (0x04), the "Event timer" entry (0x05) and the "SYNC start value"
///        entry (0x06), but the entry has an invalid data type; the
///        communication parameters (co_pdo_comm_par) that have `cobid`,
///        `trans`, `inhibit`, `reserved`, `event` and `sync` fields defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters
///
/// \Then CO_SDO_AC_TYPE_LEN is returned, the defined values from the
///       communication parameters (except for `reserved`) up to, but without,
///       `sync` are configured in the object 0x1400
TEST(CO_PdoRpdo, CoDevCfgPdoComm_Sub06SyncStartValue_InvalidType) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x07u);
  obj1400->EmplaceSub<Sub01CobId>(0);
  obj1400->EmplaceSub<Sub02TransmissionType>(0);
  obj1400->EmplaceSub<Sub03InhibitTime>(0);
  obj1400->EmplaceSub<Sub04Reserved>();
  obj1400->EmplaceSub<Sub05EventTimer>(0);
  obj1400->InsertAndSetSub(0x06u, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t{0u});

  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0x07u;
  comm.cobid = DEV_ID;
  comm.trans = Obj1400RpdoCommPar::EVENT_DRIVEN_TRANSMISSION;
  comm.inhibit = 0x1234u;
  comm.reserved = 0xffu;
  comm.event = 0xabcdu;
  comm.sync = 0x42u;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM, &comm);

  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN, ret);
  CHECK_EQUAL(0x07u, obj1400->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(comm.cobid, obj1400->GetSub<Sub01CobId>());
  CHECK_EQUAL(comm.trans, obj1400->GetSub<Sub02TransmissionType>());
  CHECK_EQUAL(comm.inhibit, obj1400->GetSub<Sub03InhibitTime>());
  CHECK_EQUAL(0, obj1400->GetSub<Sub04Reserved>());
  CHECK_EQUAL(comm.event, obj1400->GetSub<Sub05EventTimer>());
  CHECK_EQUAL(0, co_dev_get_val_u32(dev, 0x1400u, 0x06u));
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Communication Parameter object (0x1400) with the "COB-ID
///        used by RPDO" entry (0x01), the "Transmission type" entry (0x02),
///        the "Inhibit time" entry (0x03), the "Compatibility entry" entry
///        (0x04), the "Event timer" entry (0x05) and the "SYNC start value"
///        entry (0x06); the communication parameters (co_pdo_comm_par) that
///        have `cobid`, `trans`, `inhibit`, `reserved`, `event` and `sync`
///        fields defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the communication parameters
///
/// \Then 0 is returned, all defined values from the communication parameters
///       (except for `reserved`) are configured in the object 0x1400
TEST(CO_PdoRpdo, CoDevCfgPdoComm_Nominal) {
  obj1400->EmplaceSub<Sub00HighestSubidxSupported>(0x06u);
  obj1400->EmplaceSub<Sub01CobId>(0);
  obj1400->EmplaceSub<Sub02TransmissionType>(0);
  obj1400->EmplaceSub<Sub03InhibitTime>(0);
  obj1400->EmplaceSub<Sub04Reserved>();
  obj1400->EmplaceSub<Sub05EventTimer>(0);
  obj1400->EmplaceSub<Sub06SyncStartValue>(0);

  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0x06u;
  comm.cobid = DEV_ID;
  comm.trans = Obj1400RpdoCommPar::EVENT_DRIVEN_TRANSMISSION;
  comm.inhibit = 0x1234u;
  comm.reserved = 0xffu;
  comm.event = 0xabcdu;
  comm.sync = 0x42u;

  const auto ret = co_dev_cfg_rpdo_comm(dev, RPDO_NUM, &comm);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(0x06u, obj1400->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(comm.cobid, obj1400->GetSub<Sub01CobId>());
  CHECK_EQUAL(comm.trans, obj1400->GetSub<Sub02TransmissionType>());
  CHECK_EQUAL(comm.inhibit, obj1400->GetSub<Sub03InhibitTime>());
  CHECK_EQUAL(0, obj1400->GetSub<Sub04Reserved>());
  CHECK_EQUAL(comm.event, obj1400->GetSub<Sub05EventTimer>());
  CHECK_EQUAL(comm.sync, obj1400->GetSub<Sub06SyncStartValue>());
}

///@}

/// @name co_dev_cfg_rpdo_map()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the mapping parameters (co_pdo_map_par)
///
/// \Then CO_SDO_AC_NO_OBJ is returned, nothing is changed
TEST(CO_PdoRpdo, CoDevCfgPdoMap_NoObj) {
  const co_pdo_map_par map_par = CO_PDO_MAP_PAR_INIT;

  const auto ret = co_dev_cfg_rpdo_map(dev, RPDO_NUM + 1u, &map_par);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Mapping Parameter object (0x1600) with no "Number of mapped
///        application objects in PDO" entry (0x00)
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the mapping parameters (co_pdo_map_par)
///
/// \Then CO_SDO_AC_NO_SUB is returned, nothing is changed
TEST(CO_PdoRpdo, CoDevCfgPdoMap_NoSub00NumOfMappedObjs) {
  const co_pdo_map_par map_par = CO_PDO_MAP_PAR_INIT;

  const auto ret = co_dev_cfg_rpdo_map(dev, RPDO_NUM, &map_par);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Mapping Parameter object (0x1600) with the "Number of mapped
///        application objects in PDO" entry (0x00)
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the mapping parameters (co_pdo_map_par), but fails to disable the
///       mapping
///
/// \Then CO_SDO_AC_NO_WRITE is returned, nothing is changed
TEST(CO_PdoRpdo, CoDevCfgPdoMap_DisableMappingError) {
  obj1600->EmplaceSub<Sub00NumOfMappedObjs>();
  co_sub_set_access(obj1600->GetLastSub(), CO_ACCESS_RO);

  const co_pdo_map_par map_par = CO_PDO_MAP_PAR_INIT;

  const auto ret = co_dev_cfg_rpdo_map(dev, RPDO_NUM, &map_par);

  CHECK_EQUAL(CO_SDO_AC_NO_WRITE, ret);
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Mapping Parameter object (0x1600) with the "Number of mapped
///        application objects in PDO" entry (0x00), but less "Application
///        object" entries than defined in the mapping parameters
///        (co_pdo_map_par)
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the mapping parameters
///
/// \Then CO_SDO_AC_NO_SUB is returned, nothing is changed
TEST(CO_PdoRpdo, CoDevCfgPdoMap_NoSubAppObject) {
  obj1600->EmplaceSub<Sub00NumOfMappedObjs>();

  co_pdo_map_par map_par = CO_PDO_MAP_PAR_INIT;
  map_par.n = 0x01u;
  map_par.map[0] = MakeMappingParam(IDX, SUBIDX, SUB_LEN * CHAR_BIT);

  const auto ret = co_dev_cfg_rpdo_map(dev, RPDO_NUM, &map_par);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
  CHECK_EQUAL(0u, obj1600->GetSub<Sub00NumOfMappedObjs>());
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Mapping Parameter object (0x1600) with the "Number of mapped
///        application objects in PDO" entry (0x00) and a number of
///        "Application object" entries; the mapping parameters
///        (co_pdo_map_par) has equal or less than this number mapping entries
///        defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the mapping parameters, but fails to write a mapping parameter
///
/// \Then CO_SDO_AC_NO_WRITE is returned, nothing is changed
TEST(CO_PdoRpdo, CoDevCfgPdoMap_CopyingMappingParameterError) {
  obj1600->EmplaceSub<Sub00NumOfMappedObjs>(0x00u);
  obj1600->EmplaceSub<SubNthAppObject>(0x01u, 0);
  co_sub_set_access(obj1600->GetLastSub(), CO_ACCESS_RO);

  co_pdo_map_par map_par = CO_PDO_MAP_PAR_INIT;
  map_par.n = 0x01u;
  map_par.map[0] = MakeMappingParam(IDX, SUBIDX, SUB_LEN * CHAR_BIT);

  const auto ret = co_dev_cfg_rpdo_map(dev, RPDO_NUM, &map_par);

  CHECK_EQUAL(CO_SDO_AC_NO_WRITE, ret);
  CHECK_EQUAL(0u, obj1600->GetSub<Sub00NumOfMappedObjs>());
  CHECK_EQUAL(0u, obj1600->GetSub<SubNthAppObject>(0x01u));
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Mapping Parameter object (0x1600) with the "Number of mapped
///        application objects in PDO" entry (0x00) and the maximum number of
///        "Application object" entries; the mapping parameters
///        (co_pdo_map_par) has the maximum number of entries defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the mapping parameters
///
/// \Then 0 is returned, all defined mapping entries defined in the mapping
///       parameters are configured in the object 0x1600
TEST(CO_PdoRpdo, CoDevCfgPdoMap_MaxNumMaps) {
  obj1600->EmplaceSub<Sub00NumOfMappedObjs>(CO_PDO_NUM_MAPS);
  for (co_unsigned8_t i = 1; i <= CO_PDO_NUM_MAPS; i++)
    obj1600->EmplaceSub<SubNthAppObject>(i, 0);

  co_pdo_map_par map_par = CO_PDO_MAP_PAR_INIT;
  map_par.n = CO_PDO_NUM_MAPS;
  for (co_unsigned8_t i = 1; i <= CO_PDO_NUM_MAPS; i++)
    map_par.map[i - 1] = MakeMappingParam(IDX + i, SUBIDX, SUB_LEN * CHAR_BIT);

  const auto ret = co_dev_cfg_rpdo_map(dev, RPDO_NUM, &map_par);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(map_par.n, obj1600->GetSub<Sub00NumOfMappedObjs>());
  for (size_t i = 1; i < CO_PDO_NUM_MAPS; i++)
    CHECK_EQUAL(map_par.map[i - 1u], obj1600->GetSub<SubNthAppObject>(i));
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        an RPDO Mapping Parameter object (0x1600) with the "Number of mapped
///        application objects in PDO" entry (0x00) and a number of
///        "Application object" entries; the mapping parameters
///        (co_pdo_map_par) has equal or less than this number mapping entries
///        defined
///
/// \When co_dev_cfg_rpdo_comm() is called with an RPDO number and a pointer to
///       the mapping parameters
///
/// \Then 0 is returned, all defined mapping entries defined in the mapping
///       parameters are configured in the object 0x1600
TEST(CO_PdoRpdo, CoDevCfgPdoMap_Nominal) {
  obj1600->EmplaceSub<Sub00NumOfMappedObjs>(CO_PDO_NUM_MAPS);
  obj1600->EmplaceSub<SubNthAppObject>(0x01u, 0);

  co_pdo_map_par map_par = CO_PDO_MAP_PAR_INIT;
  map_par.n = 0x01u;
  map_par.map[0] = MakeMappingParam(IDX, SUBIDX, SUB_LEN * CHAR_BIT);

  const auto ret = co_dev_cfg_rpdo_map(dev, RPDO_NUM, &map_par);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(map_par.n, obj1600->GetSub<Sub00NumOfMappedObjs>());
  CHECK_EQUAL(map_par.map[0], obj1600->GetSub<SubNthAppObject>(0x01u));
}

///@}

TEST_GROUP_BASE(CO_PdoTpdo, CO_PdoBase) {
  static const co_unsigned16_t TPDO_NUM = 0x01u;

  using Sub00HighestSubidxSupported =
      Obj1800TpdoCommPar::Sub00HighestSubidxSupported;
  using Sub01CobId = Obj1800TpdoCommPar::Sub01CobId;
  using Sub02TransmissionType = Obj1800TpdoCommPar::Sub02TransmissionType;
  using Sub03InhibitTime = Obj1800TpdoCommPar::Sub03InhibitTime;
  using Sub04Reserved = Obj1800TpdoCommPar::Sub04Reserved;
  using Sub05EventTimer = Obj1800TpdoCommPar::Sub05EventTimer;
  using Sub06SyncStartValue = Obj1800TpdoCommPar::Sub06SyncStartValue;

  using Sub00NumOfMappedObjs = Obj1a00TpdoMapPar::Sub00NumOfMappedObjs;
  using SubNthAppObject = Obj1a00TpdoMapPar::SubNthAppObject;
  static constexpr auto MakeMappingParam = Obj1a00TpdoMapPar::MakeMappingParam;

  std::unique_ptr<CoObjTHolder> obj1800;
  std::unique_ptr<CoObjTHolder> obj1a00;

  TEST_SETUP() {
    TEST_BASE_SETUP();

    dev_holder->CreateObj<Obj1800TpdoCommPar>(obj1800);
    dev_holder->CreateObj<Obj1a00TpdoMapPar>(obj1a00);
  }
};

/// @name co_dev_chk_tpdo()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_chk_tpdo() is called with an index of a static data type and
///       a non-zero sub-index (illegal entry)
///
/// \Then CO_SDO_AC_NO_OBJ is returned
///       \Calls co_type_is_basic()
///       \Calls co_dev_get_dummy()
///       \Calls co_dev_find_obj()
TEST(CO_PdoTpdo, CoDevChkTpdo_IllegalDummyEntryObj) {
  const co_unsigned16_t obj_idx = CO_DEFTYPE_INTEGER16;
  co_dev_set_dummy(dev, 1 << CO_DEFTYPE_INTEGER16);

  const auto ret = co_dev_chk_tpdo(dev, obj_idx, 0x01u);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_chk_tpdo() is called with an index of a non-existing object
///       and any sub-index
///
/// \Then CO_SDO_AC_NO_OBJ is returned
///       \Calls co_type_is_basic()
///       \Calls co_dev_find_obj()
TEST(CO_PdoTpdo, CoDevChkTpdo_NoObj) {
  const auto ret = co_dev_chk_tpdo(dev, 0xffffu, 0x00u);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_chk_tpdo() is called with an index of an existing object
///       and a sub-index of a non-existing sub-object
///
/// \Then CO_SDO_AC_NO_SUB is returned
///       \Calls co_type_is_basic()
///       \Calls co_dev_find_obj()
///       \Calls co_dev_find_sub()
TEST(CO_PdoTpdo, CoDevChkTpdo_NoSub) {
  CreateMappableObject();

  const auto ret = co_dev_chk_tpdo(dev, IDX, SUBIDX + 1u);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_chk_tpdo() is called with an index and a sub-index of an entry
///       with no read access
///
/// \Then CO_SDO_AC_NO_WRITE is returned
///       \Calls co_type_is_basic()
///       \Calls co_dev_find_obj()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_access()
TEST(CO_PdoTpdo, CoDevChkTpdo_NoReadAccess) {
  CreateMappableObject();
  co_sub_set_access(obj2020->GetLastSub(), CO_ACCESS_WO);

  const auto ret = co_dev_chk_tpdo(dev, IDX, SUBIDX);

  CHECK_EQUAL(CO_SDO_AC_NO_READ, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_chk_tpdo() is called with an index and a sub-index of an entry
///       that has PDO mapping disabled
///
/// \Then CO_SDO_AC_NO_PDO is returned
///       \Calls co_type_is_basic()
///       \Calls co_dev_find_obj()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_access()
///       \Calls co_sub_get_pdo_mapping()
TEST(CO_PdoTpdo, CoDevChkTpdo_NoMappable) {
  CreateMappableObject();
  co_sub_set_pdo_mapping(obj2020->GetLastSub(), false);

  const auto ret = co_dev_chk_tpdo(dev, IDX, SUBIDX);

  CHECK_EQUAL(CO_SDO_AC_NO_PDO, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_chk_tpdo() is called with an index and a sub-index of an entry
///       that cannot be mapped into a TPDO
///
/// \Then CO_SDO_AC_NO_PDO is returned
///       \Calls co_type_is_basic()
///       \Calls co_dev_find_obj()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_access()
///       \Calls co_sub_get_pdo_mapping()
TEST(CO_PdoTpdo, CoDevChkTpdo_NoAccessTpdo) {
  CreateMappableObject();
  co_sub_set_access(obj2020->GetLastSub(), CO_ACCESS_RWW);

  const auto ret = co_dev_chk_tpdo(dev, IDX, SUBIDX);

  CHECK_EQUAL(CO_SDO_AC_NO_PDO, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_chk_tpdo() is called with an index and a sub-index of an entry
///       that can be mapped into a PDO
///
/// \Then 0 is returned
///       \Calls co_type_is_basic()
///       \Calls co_dev_find_obj()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_access()
///       \Calls co_sub_get_pdo_mapping()
TEST(CO_PdoTpdo, CoDevChkTpdo_Nominal) {
  CreateMappableObject();

  const auto ret = co_dev_chk_tpdo(dev, IDX, SUBIDX);

  CHECK_EQUAL(0, ret);
}

///@}

/// @name co_dev_cfg_tpdo()
///@{

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        a malformed TPDO Communication Parameter object (0x1800)
///
/// \When co_dev_cfg_tpdo() is called with a TPDO number, a pointer to the
///       communication parameters (co_pdo_comm_par) and a pointer to the
///       mapping parameters (co_pdo_map_par)
///
/// \Then an error returned by co_dev_cfg_tpdo_comm() is returned, nothing is
///       changed
TEST(CO_PdoTpdo, CoDevCfgTpdo_InvalidTpdoCommParamObj) {
  const co_pdo_comm_par tpdo_comm = CO_PDO_COMM_PAR_INIT;
  const co_pdo_map_par tpdo_map = CO_PDO_MAP_PAR_INIT;

  const auto ret = co_dev_cfg_tpdo(dev, TPDO_NUM + 1u, &tpdo_comm, &tpdo_map);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        a TPDO Communication Parameter object (0x1800) and a malformed TPDO
///        Mapping Parameter object (0x1a00)
///
/// \When co_dev_cfg_tpdo() is called with a TPDO number, a pointer to the
///       communication parameters (co_pdo_comm_par) and a pointer to the
///       mapping parameters (co_pdo_map_par)
///
/// \Then an error returned by co_dev_cfg_tpdo_map() is returned, nothing is
///       changed
TEST(CO_PdoTpdo, CoDevCfgTpdo_InvalidTpdoMappingParamObj) {
  co_pdo_comm_par tpdo_comm = CO_PDO_COMM_PAR_INIT;
  tpdo_comm.n = 0;
  const co_pdo_map_par tpdo_map = CO_PDO_MAP_PAR_INIT;

  const auto ret = co_dev_cfg_tpdo(dev, TPDO_NUM, &tpdo_comm, &tpdo_map);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        a TPDO Communication Parameter object (0x1800) with a "COB-ID used
///        by TPDO" entry (0x01) and a TPDO Mapping Parameter object (0x1a00)
///        with a "Number of mapped application objects in PDO" entry (0x00);
///        the communication parameters (co_pdo_comm_par) with a COB-ID that
///        does not have the CO_PDO_COBID_VALID bit set; the mapping parameters
///        (co_pdo_map_par) with no application objects
///
/// \When co_dev_cfg_tpdo() is called with a TPDO number, a pointer to the
///       communication parameters and a pointer to the mapping parameters
///
/// \Then 0 is returned, the TPDO Communication Parameters object is configured
///       with the given COB-ID
TEST(CO_PdoTpdo, CoDevCfgTpdo_ReenableTpdo) {
  obj1800->EmplaceSub<Sub00HighestSubidxSupported>(0x01u);
  obj1800->EmplaceSub<Sub01CobId>(0);
  obj1a00->EmplaceSub<Sub00NumOfMappedObjs>(0);

  co_pdo_comm_par tpdo_comm = CO_PDO_COMM_PAR_INIT;
  tpdo_comm.n = 0x01u;
  tpdo_comm.cobid = DEV_ID;
  const co_pdo_map_par tpdo_map = CO_PDO_MAP_PAR_INIT;

  const auto ret = co_dev_cfg_tpdo(dev, TPDO_NUM, &tpdo_comm, &tpdo_map);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(tpdo_comm.n, obj1800->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(tpdo_comm.cobid, obj1800->GetSub<Sub01CobId>());
  CHECK_EQUAL(tpdo_map.n, obj1a00->GetSub<Sub00NumOfMappedObjs>());
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        a TPDO Communication Parameter object (0x1800) with a "COB-ID used
///        by TPDO" entry (0x01) and a TPDO Mapping Parameter object (0x1a00)
///        with a "Number of mapped application objects in PDO" entry (0x00);
///        the communication parameters (co_pdo_comm_par) with a COB-ID that
///        has the CO_PDO_COBID_VALID bit set; the mapping parameters
///        (co_pdo_map_par) with no application objects
///
/// \When co_dev_cfg_tpdo() is called with a TPDO number, a pointer to the
///       communication parameters and a pointer to the mapping parameters
///
/// \Then 0 is returned, the TPDO Communication Parameters object is configured
///       with the given COB-ID
TEST(CO_PdoTpdo, CoDevCfgTpdo_DisabledTpdo) {
  obj1800->EmplaceSub<Sub00HighestSubidxSupported>(0x01u);
  obj1800->EmplaceSub<Sub01CobId>(0);
  obj1a00->EmplaceSub<Sub00NumOfMappedObjs>(0);

  co_pdo_comm_par tpdo_comm = CO_PDO_COMM_PAR_INIT;
  tpdo_comm.n = 0x01u;
  tpdo_comm.cobid = DEV_ID | CO_PDO_COBID_VALID;
  const co_pdo_map_par tpdo_map = CO_PDO_MAP_PAR_INIT;

  const auto ret = co_dev_cfg_tpdo(dev, TPDO_NUM, &tpdo_comm, &tpdo_map);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(tpdo_comm.n, obj1800->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(tpdo_comm.cobid, obj1800->GetSub<Sub01CobId>());
  CHECK_EQUAL(tpdo_map.n, obj1a00->GetSub<Sub00NumOfMappedObjs>());
}

///@}

/// @name co_dev_cfg_tpdo_comm()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_cfg_tpdo_comm() is called with a TPDO number equal to zero
///       and a pointer to the communication parameters (co_pdo_comm_par)
///
/// \Then CO_SDO_AC_NO_OBJ is returned
TEST(CO_PdoTpdo, CoDevCfgTpdoComm_NumZero) {
  const co_pdo_comm_par tpdo_comm = CO_PDO_COMM_PAR_INIT;

  const auto ret = co_dev_cfg_tpdo_comm(dev, 0u, &tpdo_comm);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_cfg_tpdo_comm() is called with a TPDO number larger than
///       CO_NUM_PDOS and a pointer to the communication parameters
///       (co_pdo_comm_par)
///
/// \Then CO_SDO_AC_NO_OBJ is returned
TEST(CO_PdoTpdo, CoDevCfgTpdoComm_NumOverMax) {
  const co_pdo_comm_par tpdo_comm = CO_PDO_COMM_PAR_INIT;

  const auto ret = co_dev_cfg_tpdo_comm(dev, CO_NUM_PDOS + 1u, &tpdo_comm);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        a TPDO Communication Parameter object (0x1800) with the "COB-ID
///        used by TPDO" entry (0x01)
///
/// \When co_dev_cfg_tpdo_comm() is called with a TPDO number and a pointer to
///       the communication parameters
///
/// \Then 0 is returned, values from the communication parameters are
///       configured in the object 0x1800
TEST(CO_PdoTpdo, CoDevCfgTpdoComm_Nominal) {
  obj1800->EmplaceSub<Sub00HighestSubidxSupported>(0x01u);
  obj1800->EmplaceSub<Sub01CobId>(0);

  co_pdo_comm_par tpdo_comm = CO_PDO_COMM_PAR_INIT;
  tpdo_comm.n = 0x01u;
  tpdo_comm.cobid = DEV_ID;

  const auto ret = co_dev_cfg_tpdo_comm(dev, TPDO_NUM, &tpdo_comm);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(tpdo_comm.n, obj1800->GetSub<Sub00HighestSubidxSupported>());
  CHECK_EQUAL(tpdo_comm.cobid, obj1800->GetSub<Sub01CobId>());
}

///@}

/// @name co_dev_cfg_tpdo_map()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_cfg_tpdo_map() is called with a TPDO number equal to zero and
///       a pointer to the mapping parameters (co_pdo_map_par)
///
/// \Then CO_SDO_AC_NO_OBJ is returned
TEST(CO_PdoTpdo, CoDevCfgTpdoMap_NumZero) {
  const co_pdo_map_par tpdo_map = CO_PDO_MAP_PAR_INIT;

  const auto ret = co_dev_cfg_tpdo_map(dev, 0u, &tpdo_map);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_cfg_tpdo_map() is called with a TPDO number larger than
///       CO_NUM_PDOS and a pointer to the mapping parameters (co_pdo_map_par)
///
/// \Then CO_SDO_AC_NO_OBJ is returned
TEST(CO_PdoTpdo, CoDevCfgTpdoMap_NumOverMax) {
  const co_pdo_map_par tpdo_map = CO_PDO_MAP_PAR_INIT;

  const auto ret = co_dev_cfg_tpdo_map(dev, CO_NUM_PDOS + 1u, &tpdo_map);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a pointer to a device (co_dev_t), the object dictionary contains
///        a TPDO Mapping Parameter object (0x1a00) with some mapping entries
///
/// \When co_dev_cfg_tpdo_map() is called with a TPDO number and a pointer to
///       the mapping parameters (co_pdo_map_par)
///
/// \Then 0 is returned, values from the mapping parameters are configured in
///       the object 0x1a00
TEST(CO_PdoTpdo, CoDevCfgTpdoMap_Nominal) {
  obj1a00->EmplaceSub<Sub00NumOfMappedObjs>(0);
  obj1a00->EmplaceSub<SubNthAppObject>(0x01u, 0);

  co_pdo_map_par tpdo_map = CO_PDO_MAP_PAR_INIT;
  tpdo_map.n = 0x01u;
  tpdo_map.map[0] = MakeMappingParam(0x2000u, 0x00u, 0x00u);

  const auto ret = co_dev_cfg_tpdo_map(dev, TPDO_NUM, &tpdo_map);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(tpdo_map.n, obj1a00->GetSub<Sub00NumOfMappedObjs>());
  CHECK_EQUAL(tpdo_map.map[0], obj1a00->GetSub<SubNthAppObject>(0x01u));
}

///@}

TEST_GROUP_BASE(CO_Pdo, CO_PdoBase) {
  static constexpr auto MakeMappingParam = Obj1600RpdoMapPar::MakeMappingParam;

  static const co_unsigned8_t val_u8_1 = 0x12u;
  static const co_unsigned8_t val_u8_2 = 0x34u;
  static const co_unsigned16_t val_u16 = 0xabcdu;
  static const co_unsigned32_t val_u32 = 0xdeadbeefu;

  co_sdo_req req;
  char buf_[sizeof(co_unsigned64_t)] = {0};
  membuf buffer;

  TEST_SETUP() {
    TEST_BASE_SETUP();

    membuf_init(&buffer, buf_, sizeof(buf_));
    co_sdo_req_init(&req, &buffer);
  }

  TEST_TEARDOWN() {
    CoSubDnInd::Clear();
    CoSubUpInd::Clear();

    TEST_BASE_TEARDOWN();
  }
};

/// @name co_pdo_map()
///@{

/// \Given a PDO mapping parameters (co_pdo_map_par) with more mappings than
///        CO_PDO_NUM_MAPS
///
/// \When co_pdo_map() is called with a pointer to the mapping parameters and
///       any other arguments
///
/// \Then CO_SDO_AC_PDO_LEN is returned, nothing is changed
TEST(CO_Pdo, CoPdoMap_OversizedPdoMap) {
  const co_unsigned8_t n_val = CO_PDO_NUM_MAPS + 1u;
  const co_unsigned64_t val[n_val] = {0u};
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = n_val;

  const auto ret = co_pdo_map(&par, val, n_val, nullptr, nullptr);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
}

/// \Given a PDO mapping parameters (co_pdo_map_par), an array of values to
///        map; the mapping parameters contain the different number of mappings
///        than there are values in the array
///
/// \When co_pdo_map() is called with a pointer to the mapping parameters,
///       a pointer to the array of values, the number of values and any other
///       arguments
///
/// \Then CO_SDO_AC_PDO_LEN is returned, nothing is changed
TEST(CO_Pdo, CoPdoMap_ValuesNumNotEqualMapValuesNum) {
  const co_unsigned8_t n_val = 1u;
  const co_unsigned64_t val[n_val] = {0u};
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = n_val + 1u;

  const auto ret = co_pdo_map(&par, val, n_val, nullptr, nullptr);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
}

/// \Given a PDO mapping parameters (co_pdo_map_par) with no mappings, an empty
///        array of values to map
///
/// \When co_pdo_map() is called with a pointer to the mapping parameters,
///       a pointer to the empty array of values, the zero number of values,
///       a pointer to a buffer, an address of the size of the buffer
///
/// \Then 0 is returned, 0 is stored in the size of the buffer
TEST(CO_Pdo, CoPdoMap_NoValues) {
  const co_unsigned8_t n_val = 0u;
  const co_unsigned64_t val[] = {0u};
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = n_val;

  size_t n_buf = 1u;
  uint_least8_t buf[1u] = {0u};

  const auto ret = co_pdo_map(&par, val, n_val, buf, &n_buf);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(n_val, n_buf);
  CHECK_EQUAL(0u, buf[0u]);
}

/// \Given a PDO mapping parameters (co_pdo_map_par), an array of values to map
///
/// \When co_pdo_map() is called with a pointer to the mapping parameters,
///       a pointer to the array of values, the number of values, a null buffer
///       pointer, an address of the size of the buffer
///
/// \Then 0 is returned, the number of bytes that would have been written if
///       there was a sufficiently large buffer is stored in the size of the
///       buffer
///       \Calls stle_u64()
TEST(CO_Pdo, CoPdoMap_BufNull) {
  const co_unsigned8_t pdo_size = sizeof(val_u16);

  const co_unsigned8_t n_val = 1u;
  const co_unsigned64_t val[n_val] = {0u};

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = n_val;
  par.map[0u] = MakeMappingParam(0x0000u, 0x00u, sizeof(val_u16) * CHAR_BIT);

  size_t n_buf = 0u;

  const auto ret = co_pdo_map(&par, val, n_val, nullptr, &n_buf);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(pdo_size, n_buf);
}

/// \Given a PDO mapping parameters (co_pdo_map_par), an array of values to map
///
/// \When co_pdo_map() is called with a pointer to the mapping parameters,
///       a pointer to the array of values, the number of values, a null buffer
///       pointer, a null size of the buffer pointer
///
/// \Then 0 is returned, nothing is changed
TEST(CO_Pdo, CoPdoMap_BufNull_NBuffNull) {
  const co_unsigned8_t n_val = 1u;
  const co_unsigned64_t val[n_val] = {0u};
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = n_val;
  par.map[0u] = MakeMappingParam(0x0000u, 0x00u, sizeof(val_u16) * CHAR_BIT);

  const auto ret = co_pdo_map(&par, val, n_val, nullptr, nullptr);

  CHECK_EQUAL(0u, ret);
}

/// \Given a PDO mapping parameters (co_pdo_map_par) with mappings that exceed
///        the maximum PDO size, an array of values to map
///
/// \When co_pdo_map() is called with a pointer to the mapping parameters,
///       a pointer to the array of values, the number of values, a pointer to
///       a buffer, an address of the size of the buffer
///
/// \Then CO_SDO_AC_PDO_LEN is returned, the values not exceeding the PDO size
///       are written to the buffer (in little endian), the size of the buffer
///       remains unchanged
///       \Calls stle_u64()
///       \Calls bcpyle()
TEST(CO_Pdo, CoPdoMap_MappingExceedsMaxPdoSize) {
  const co_unsigned8_t n_val = 2u;
  const co_unsigned64_t val[n_val] = {val_u16, CO_UNSIGNED64_MAX};

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = n_val;
  par.map[0u] = MakeMappingParam(0x0000u, 0x00u, sizeof(val_u16) * CHAR_BIT);
  par.map[1u] = MakeMappingParam(0x0000u, 0x00u, 0xffu);

  const size_t buf_size = CAN_MAX_LEN;
  size_t n_buf = buf_size;
  uint_least8_t buf[buf_size] = {0u};

  const auto ret = co_pdo_map(&par, val, n_val, buf, &n_buf);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
  CHECK_EQUAL(buf_size, n_buf);
  CHECK_EQUAL(val_u16, ldle_u16(&buf[0u]));
  for (size_t i = 2u; i < buf_size; ++i) CHECK_EQUAL(0x00u, buf[i]);
}

/// \Given a PDO mapping parameters (co_pdo_map_par) with at least one empty
///        mapping, an array of values to map
///
/// \When co_pdo_map() is called with a pointer to the mapping parameters,
///       a pointer to the array of values, the number of values, a pointer to
///       a buffer, an address of the size of the buffer
///
/// \Then 0 is returned, the values with non-empty mappings are written to the
///       buffer (in little endian)
///       \Calls stle_u64()
///       \Calls bcpyle()
TEST(CO_Pdo, CoPdoMap_EmptyMapping) {
  const co_unsigned8_t pdo_size = sizeof(val_u16) + sizeof(val_u32);

  const co_unsigned8_t n_val = 3u;
  const co_unsigned64_t val[n_val] = {val_u16, CO_UNSIGNED64_MAX, val_u32};

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = n_val;
  par.map[0u] = MakeMappingParam(0u, 0u, sizeof(val_u16) * CHAR_BIT);
  par.map[1u] = 0u;
  par.map[2u] = MakeMappingParam(0u, 0u, sizeof(val_u32) * CHAR_BIT);

  size_t n_buf = pdo_size;
  uint_least8_t buf[pdo_size] = {0};

  const auto ret = co_pdo_map(&par, val, n_val, buf, &n_buf);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(val_u16, ldle_u16(&buf[0u]));
  CHECK_EQUAL(val_u32, ldle_u32(&buf[2u]));
  for (size_t i = 6u; i < pdo_size; ++i) CHECK_EQUAL(0x00u, buf[i]);
}

/// \Given a PDO mapping parameters (co_pdo_map_par), an array of values to map
///
/// \When co_pdo_map() is called with a pointer to the mapping parameters,
///       a pointer to the array of values, the number of values, a pointer to
///       a buffer, a null size of the buffer pointer
///
/// \Then 0 is returned, nothing is changed
///       \Calls stle_u64()
TEST(CO_Pdo, CoPdoMap_NBufNull) {
  const co_unsigned8_t pdo_size = sizeof(val_u16) + sizeof(val_u32);

  const co_unsigned8_t n_val = 2u;
  const co_unsigned64_t val[n_val] = {val_u16, val_u32};

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = n_val;
  par.map[0u] = MakeMappingParam(0u, 0u, sizeof(val_u16) * CHAR_BIT);
  par.map[1u] = MakeMappingParam(0u, 0u, sizeof(val_u32) * CHAR_BIT);

  uint_least8_t buf[pdo_size] = {0};

  const auto ret = co_pdo_map(&par, val, n_val, buf, nullptr);

  CHECK_EQUAL(0u, ret);
  for (size_t i = 0u; i < pdo_size; ++i) CHECK_EQUAL(0x00u, buf[i]);
}

/// \Given a PDO mapping parameters (co_pdo_map_par), an array of values to map
///
/// \When co_pdo_map() is called with a pointer to the mapping parameters,
///       a pointer to the array of values, the number of values, a pointer to
///       a buffer too small for the mapped values, an address of the size of
///       the buffer
///
/// \Then 0 is returned, the values not exceeding the buffer size are written
///       to the buffer (in little endian), the number of bytes that would have
///       been written if there was a sufficiently large buffer is stored in
///       the size of the buffer
///       \Calls stle_u64()
///       \Calls bcpyle()
TEST(CO_Pdo, CoPdoMap_BufferTooSmall) {
  const co_unsigned8_t pdo_size = sizeof(val_u16) + sizeof(val_u32);

  const co_unsigned8_t n_val = 2u;
  const co_unsigned64_t val[n_val] = {val_u16, val_u32};

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = n_val;
  par.map[0u] = MakeMappingParam(0u, 0u, sizeof(val_u16) * CHAR_BIT);
  par.map[1u] = MakeMappingParam(0u, 0u, sizeof(val_u32) * CHAR_BIT);

  size_t n_buf = pdo_size - 1u;
  uint_least8_t buf[pdo_size - 1u] = {0};

  const auto ret = co_pdo_map(&par, val, n_val, buf, &n_buf);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(pdo_size, n_buf);
  CHECK_EQUAL(val_u16, ldle_u16(&buf[0u]));
  for (size_t i = 2u; i < (pdo_size - 1u); ++i) CHECK_EQUAL(0x00u, buf[i]);
}

/// \Given a PDO mapping parameters (co_pdo_map_par), an array of values to map
///
/// \When co_pdo_map() is called with a pointer to the mapping parameters,
///       a pointer to the array of values, the number of values, a pointer to
///       a sufficiently large buffer, an address of the size of the buffer
///
/// \Then 0 is returned, the mapped values are written to the buffer (in little
///       endian), the number of written bytes is stored in the size of the
///       buffer
///       \Calls stle_u64()
///       \Calls bcpyle()
TEST(CO_Pdo, CoPdoMap_Nominal) {
  const co_unsigned8_t pdo_size =
      sizeof(val_u8_1) + sizeof(val_u16) + sizeof(val_u8_2) + sizeof(val_u32);

  const co_unsigned8_t n_val = 4u;
  const co_unsigned64_t val[n_val] = {val_u8_1, val_u16, val_u8_2, val_u32};

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = n_val;
  par.map[0u] = MakeMappingParam(0u, 0u, sizeof(val_u8_1) * CHAR_BIT);
  par.map[1u] = MakeMappingParam(0u, 0u, sizeof(val_u16) * CHAR_BIT);
  par.map[2u] = MakeMappingParam(0u, 0u, sizeof(val_u8_2) * CHAR_BIT);
  par.map[3u] = MakeMappingParam(0u, 0u, sizeof(val_u32) * CHAR_BIT);

  size_t n_buf = pdo_size + 1u;
  uint_least8_t buf[pdo_size + 1u] = {0u};

  const auto ret = co_pdo_map(&par, val, n_val, buf, &n_buf);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(pdo_size, n_buf);
  CHECK_EQUAL(val_u8_1, buf[0u]);
  CHECK_EQUAL(val_u16, ldle_u16(&buf[1u]));
  CHECK_EQUAL(val_u8_2, buf[3u]);
  CHECK_EQUAL(val_u32, ldle_u32(&buf[4u]));
  CHECK_EQUAL(0x00u, buf[8u]);
}

///@}

/// @name co_pdo_unmap()
///@{

/// \Given a PDO mapping parameters (co_pdo_map_par) with more mappings than
///        CO_PDO_NUM_MAPS
///
/// \When co_pdo_unmap() is called with a pointer to the mapping parameters,
///       a pointer to a buffer with the mapped values and any other arguments
///
/// \Then CO_SDO_AC_PDO_LEN is returned, nothing is changed
TEST(CO_Pdo, CoPdoUnmap_OversizedPdoMap) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = CO_PDO_NUM_MAPS + 1u;
  const uint_least8_t buf[] = {0u};

  const auto ret = co_pdo_unmap(&par, buf, 0, nullptr, nullptr);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
}

/// \Given a PDO mapping parameters (co_pdo_map_par) with at least one empty
///        mapping, a buffer with the mapped values
///
/// \When co_pdo_unmap() is called with a pointer to the mapping parameters,
///       a pointer to the buffer, a pointer to the array for unmapped values,
///       an address to the size of the array
///
/// \Then 0 is returned, the mapped values are stored in the array, the empty
///       mapping value is set to `0`, the number of values is stored in the
///       size of the array
///       \Calls bcpyle()
///       \Calls ldle_u64()
TEST(CO_Pdo, CoPdoUnmap_EmptyMapping) {
  const co_unsigned8_t val_num = 3u;

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = val_num;
  par.map[0u] = MakeMappingParam(0u, 0u, sizeof(val_u16) * CHAR_BIT);
  par.map[1u] = 0;
  par.map[2u] = MakeMappingParam(0u, 0u, sizeof(val_u32) * CHAR_BIT);

  const size_t n_buf = sizeof(val_u16) + sizeof(val_u32);
  uint_least8_t buf[n_buf] = {0u};
  stle_u16(buf, val_u16);
  stle_u32(buf + sizeof(val_u16), val_u32);

  co_unsigned8_t n_val = val_num;
  co_unsigned64_t val[val_num] = {0u};

  const auto ret = co_pdo_unmap(&par, buf, n_buf, val, &n_val);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(val_num, n_val);
  CHECK_EQUAL(val_u16, val[0u]);
  CHECK_EQUAL(0, val[1u]);
  CHECK_EQUAL(val_u32, val[2u]);
}

/// \Given a PDO mapping parameters (co_pdo_map_par), a buffer with the mapped
///        values smaller than the mapping
///
/// \When co_pdo_unmap() is called with a pointer to the mapping parameters,
///       a pointer to the buffer, a pointer to the array for unmapped values,
///       an address to the size of the array
///
/// \Then 0 is returned, the mapped values are stored in the array (except for
///       mapping exceeding the buffer), the number of values (from the
///       mappings) is stored in the size of the array
///       \Calls bcpyle()
///       \Calls ldle_u64()
TEST(CO_Pdo, CoPdoUnmap_MappedValuesSizeSmallerThanMapping) {
  const co_unsigned8_t val_num = 2u;

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = val_num;
  par.map[0u] = MakeMappingParam(0u, 0u, sizeof(val_u16) * CHAR_BIT);
  par.map[1u] = MakeMappingParam(0u, 0u, sizeof(val_u32) * CHAR_BIT);

  const size_t n_buf = sizeof(val_u16) + sizeof(val_u32);
  uint_least8_t buf[n_buf] = {0u};
  stle_u16(buf, val_u16);
  stle_u32(buf + sizeof(val_u16), val_u32);

  co_unsigned8_t n_val = val_num;
  co_unsigned64_t val[val_num] = {0u};

  const auto ret = co_pdo_unmap(&par, buf, n_buf - 1u, val, &n_val);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
  CHECK_EQUAL(val_num, n_val);
  CHECK_EQUAL(val_u16, val[0u]);
  CHECK_EQUAL(0, val[1u]);
}

/// \Given a PDO mapping parameters (co_pdo_map_par), a buffer with the mapped
///        values
///
/// \When co_pdo_unmap() is called with a pointer to the mapping parameters,
///       a pointer to the buffer, a null array for unmapped values pointer,
///       an address to the size of the array
///
/// \Then 0 is returned, the number of values is stored in the size of the
///       array
///       \Calls bcpyle()
TEST(CO_Pdo, CoPdoUnmap_ValNull) {
  const co_unsigned8_t val_num = 2u;

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = val_num;
  par.map[0u] = MakeMappingParam(0u, 0u, sizeof(val_u16) * CHAR_BIT);
  par.map[1u] = MakeMappingParam(0u, 0u, sizeof(val_u32) * CHAR_BIT);

  const size_t n_buf = sizeof(val_u16) + sizeof(val_u32);
  uint_least8_t buf[n_buf] = {0u};
  stle_u16(buf, val_u16);
  stle_u32(buf + sizeof(val_u16), val_u32);

  co_unsigned8_t n_val = 0;

  const auto ret = co_pdo_unmap(&par, buf, n_buf, nullptr, &n_val);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(val_num, n_val);
}

/// \Given a PDO mapping parameters (co_pdo_map_par), a buffer with the mapped
///        values
///
/// \When co_pdo_unmap() is called with a pointer to the mapping parameters,
///       a pointer to the buffer, a pointer of an array for unmapped values,
///       a null size of the array pointer
///
/// \Then 0 is returned, nothing is changed
///       \Calls bcpyle()
TEST(CO_Pdo, CoPdoUnmap_NValNull) {
  const co_unsigned8_t val_num = 2u;

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = val_num;
  par.map[0u] = MakeMappingParam(0u, 0u, sizeof(val_u16) * CHAR_BIT);
  par.map[1u] = MakeMappingParam(0u, 0u, sizeof(val_u32) * CHAR_BIT);

  const size_t n_buf = sizeof(val_u16) + sizeof(val_u32);
  uint_least8_t buf[n_buf] = {0u};
  stle_u16(buf, val_u16);
  stle_u32(buf + sizeof(val_u16), val_u32);

  co_unsigned64_t val[val_num] = {0u};

  const auto ret = co_pdo_unmap(&par, buf, n_buf, val, nullptr);

  CHECK_EQUAL(0u, ret);
  for (size_t i = 0u; i < val_num; ++i) CHECK_EQUAL(0x00u, val[i]);
}

/// \Given a PDO mapping parameters (co_pdo_map_par), a buffer with the mapped
///        values
///
/// \When co_pdo_unmap() is called with a pointer to the mapping parameters,
///       a pointer to the buffer, a pointer to the array for unmapped values,
///       an address to the size of the array smaller than the number of mapped
///       values
///
/// \Then 0 is returned, the mapped values not exceeding the array size are
///       stored in the array, the total number of values (from the mappings)
///       is stored in the size of the array
///       \Calls bcpyle()
///       \Calls ldle_u64()
TEST(CO_Pdo, CoPdoUnmap_LessUnmappedValuesThanMapped) {
  const co_unsigned8_t val_num = 2u;

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = val_num;
  par.map[0u] = MakeMappingParam(0u, 0u, sizeof(val_u16) * CHAR_BIT);
  par.map[1u] = MakeMappingParam(0u, 0u, sizeof(val_u32) * CHAR_BIT);

  const size_t n_buf = sizeof(val_u16) + sizeof(val_u32);
  uint_least8_t buf[n_buf] = {0u};
  stle_u16(buf, val_u16);
  stle_u32(buf + sizeof(val_u16), val_u32);

  co_unsigned8_t n_val = val_num - 1u;
  co_unsigned64_t val[val_num] = {0u};

  const auto ret = co_pdo_unmap(&par, buf, n_buf, val, &n_val);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(val_num, n_val);
  CHECK_EQUAL(val_u16, val[0u]);
  CHECK_EQUAL(0u, val[1u]);
}

/// \Given a PDO mapping parameters (co_pdo_map_par), a buffer with the mapped
///        values
///
/// \When co_pdo_unmap() is called with a pointer to the mapping parameters,
///       a pointer to the buffer, a pointer to the array for unmapped values,
///       an address to the size of the array
///
/// \Then 0 is returned, the mapped values are stored in the array, the number
///       of values is stored in the size of the array
///       \Calls bcpyle()
///       \Calls ldle_u64()
TEST(CO_Pdo, CoPdoUnmap_Nominal) {
  const co_unsigned8_t val_num = 4u;
  const co_unsigned8_t pdo_size =
      sizeof(val_u8_1) + sizeof(val_u16) + sizeof(val_u8_2) + sizeof(val_u32);

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = val_num;
  par.map[0u] = MakeMappingParam(0u, 0u, sizeof(val_u8_1) * CHAR_BIT);
  par.map[1u] = MakeMappingParam(0u, 0u, sizeof(val_u16) * CHAR_BIT);
  par.map[2u] = MakeMappingParam(0u, 0u, sizeof(val_u8_2) * CHAR_BIT);
  par.map[3u] = MakeMappingParam(0u, 0u, sizeof(val_u32) * CHAR_BIT);

  uint_least8_t buf[pdo_size] = {0u};
  *buf = val_u8_1;
  stle_u16(buf + sizeof(val_u8_1), val_u16);
  buf[sizeof(val_u8_1) + sizeof(val_u16)] = val_u8_2;
  stle_u32(buf + sizeof(val_u8_1) + sizeof(val_u16) + sizeof(val_u8_2),
           val_u32);

  co_unsigned8_t n_val = val_num;
  co_unsigned64_t val[val_num] = {0u};

  const auto ret = co_pdo_unmap(&par, buf, pdo_size, val, &n_val);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(val_num, n_val);
  CHECK_EQUAL(val_u8_1, val[0u]);
  CHECK_EQUAL(val_u16, val[1u]);
  CHECK_EQUAL(val_u8_2, val[2u]);
  CHECK_EQUAL(val_u32, val[3u]);
}

///@}

/// @name co_pdo_dn()
///@{

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par), a buffer with the mapped values larger than the
///        maximum PDO size, a CANopen SDO download request
///
/// \When co_pdo_dn() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a pointer to the
///       buffer, the number of bytes in the buffer
///
/// \Then CO_SDO_AC_PDO_LEN is returned, nothing is changed
TEST(CO_Pdo, CoPdoDn_BufLargerThanMaxPdoSize) {
  const co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  const size_t n = CAN_MAX_LEN + 1u;
  const uint_least8_t buf[n] = {0u};

  const auto ret = co_pdo_dn(&par, dev, &req, buf, n);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
}

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par), an empty buffer with the mapped values, a CANopen
///        SDO download request
///
/// \When co_pdo_dn() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a pointer to the
///       buffer, a zero number of bytes in the buffer
///
/// \Then 0 is returned, nothing is changed
TEST(CO_Pdo, CoPdoDn_NoMappedValues) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 0u;

  const size_t n = 0u;
  const uint_least8_t buf[] = {0u};

  const auto ret = co_pdo_dn(&par, dev, &req, buf, n);

  CHECK_EQUAL(0u, ret);
}

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par), a buffer with the mapped values that is too small
///        for the mappings, a CANopen SDO download request
///
/// \When co_pdo_dn() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a pointer to the
///       buffer, the number of bytes in the buffer
///
/// \Then CO_SDO_AC_PDO_LEN is returned, nothing is changed
TEST(CO_Pdo, CoPdoDn_BufferTooSmallForMappedValues) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 1u;
  par.map[0] = MakeMappingParam(0u, 0u, sizeof(val_u16) * CHAR_BIT);

  const size_t n = sizeof(val_u16) - 1u;
  const uint_least8_t buf[n] = {0xffu};

  const auto ret = co_pdo_dn(&par, dev, &req, buf, n);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
}

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par) with a mapping into a non-existing object, a buffer
///        with the mapped values, a CANopen SDO download request
///
/// \When co_pdo_dn() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a pointer to the
///       buffer, the number of bytes in the buffer
///
/// \Then CO_SDO_AC_NO_OBJ is returned, nothing is changed
///       \Calls co_dev_chk_rpdo()
TEST(CO_Pdo, CoPdoDn_MappingNonExistingObject) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 1u;
  par.map[0] = MakeMappingParam(0xffffu, 0x00u, 0x00u);

  const size_t n = 0u;
  const uint_least8_t buf[] = {0};

  const auto ret = co_pdo_dn(&par, dev, &req, buf, n);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par) with a mapping into a dummy object, a buffer
///        with the mapped values, a CANopen SDO download request
///
/// \When co_pdo_dn() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a pointer to the
///       buffer, the number of bytes in the buffer
///
/// \Then 0 is returned, nothing is changed
///       \Calls co_dev_chk_rpdo()
///       \Calls co_dev_find_sub()
TEST(CO_Pdo, CoPdoDn_DummyEntryMapping) {
  co_dev_set_dummy(dev, 1u << CO_DEFTYPE_UNSIGNED16);

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 1u;
  par.map[0] =
      MakeMappingParam(CO_DEFTYPE_UNSIGNED16, 0x00u, sizeof(co_unsigned16_t));

  const size_t n = sizeof(co_unsigned16_t);
  uint_least8_t buf[n] = {0u};
  stle_u16(buf, CO_UNSIGNED16_MAX);

  const auto ret = co_pdo_dn(&par, dev, &req, buf, n);

  CHECK_EQUAL(0u, ret);
}

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par), a buffer with the mapped values, a CANopen SDO
///        download request
///
/// \When co_pdo_dn() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a pointer to the
///       buffer, the number of bytes in the buffer; but the download
///       indication function of the mapped object returns an error
///
/// \Then the error from the indication function is returned, nothing is
///       changed
///       \Calls co_dev_chk_rpdo()
///       \Calls co_dev_find_sub()
///       \Calls bcpyle()
///       \Calls co_sdo_req_clear()
///       \Calls co_sub_dn_ind()
TEST(CO_Pdo, CoPdoDn_DnIndError) {
  CreateMappableObject();
  CHECK_EQUAL(SUB_LEN, co_sub_set_val_u32(obj2020->GetLastSub(), 0u));
  co_sub_set_dn_ind(obj2020->GetLastSub(), &CoSubDnInd::FuncDn, nullptr);
  CoSubDnInd::ret = CO_SDO_AC_PARAM_VAL;

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 1u;
  par.map[0] = MakeMappingParam(IDX, SUBIDX, SUB_LEN * CHAR_BIT);
  const size_t n = SUB_LEN;
  uint_least8_t buf[n] = {0u};
  stle_u32(buf, val_u32);

  const auto ret = co_pdo_dn(&par, dev, &req, buf, n);

  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, ret);
  CHECK_EQUAL(1u, CoSubDnInd::GetNumCalled());
  CHECK_EQUAL(0u, co_sub_get_val_u32(obj2020->GetLastSub()));
}

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par) with an empty mapping, a buffer with the mapped
///        values, a CANopen SDO download request
///
/// \When co_pdo_dn() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a pointer to the
///       buffer, the number of bytes in the buffer
///
/// \Then 0 is returned, nothing is changed
TEST(CO_Pdo, CoPdoDn_EmptyMapping) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 1u;
  par.map[0u] = 0u;
  const size_t n = 1u;
  const uint_least8_t buf[n] = {0xffu};

  const auto ret = co_pdo_dn(&par, dev, &req, buf, n);

  CHECK_EQUAL(0u, ret);
}

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par), a buffer with the mapped values, a CANopen SDO
///        download request
///
/// \When co_pdo_dn() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a pointer to the
///       buffer, the number of bytes in the buffer
///
/// \Then 0 is returned, the mapped values are written to the object dictionary
///       \Calls co_dev_chk_rpdo()
///       \Calls co_dev_find_sub()
///       \Calls bcpyle()
///       \Calls co_sdo_req_clear()
///       \Calls co_sub_dn_ind()
TEST(CO_Pdo, CoPdoDn_Nominal) {
  CreateMappableObject();
  CHECK_EQUAL(SUB_LEN, co_sub_set_val_u32(obj2020->GetLastSub(), 0u));
  co_sub_set_dn_ind(obj2020->GetLastSub(), &CoSubDnInd::FuncDn, nullptr);

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 1u;
  par.map[0u] = MakeMappingParam(IDX, SUBIDX, SUB_LEN * CHAR_BIT);
  const size_t n = SUB_LEN;
  uint_least8_t buf[n] = {0u};
  stle_u32(buf, val_u32);

  const auto ret = co_pdo_dn(&par, dev, &req, buf, n);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(1u, CoSubDnInd::GetNumCalled());
  CHECK_EQUAL(val_u32, co_sub_get_val_u32(obj2020->GetLastSub()));
}

///@}

/// @name co_pdo_up()
///@{

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par) with an empty mapping, a CANopen SDO upload request
///
/// \When co_pdo_up() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a pointer to the
///       buffer, an address of the number of bytes in the buffer
///
/// \Then 0 is returned, nothing is changed
TEST(CO_Pdo, CoPdoUp_EmptyMapping) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 1u;
  par.map[0] = 0u;
  size_t n = 0u;
  co_unsigned8_t buf[] = {0u};

  const auto ret = co_pdo_up(&par, dev, &req, buf, &n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0u, n);
}

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par) with a mapping that exceeds the maximum PDO size,
///        a CANopen SDO upload request
///
/// \When co_pdo_up() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a pointer to the
///       buffer, an address of the number of bytes in the buffer
///
/// \Then CO_SDO_AC_PDO_LEN is returned, nothing is changed
TEST(CO_Pdo, CoPdoUp_MappingExceedsMaxPdoSize) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 1u;
  par.map[0] = MakeMappingParam(0u, 0u, 0xffu);
  size_t n = 0u;
  co_unsigned8_t buf[] = {0u};

  const auto ret = co_pdo_up(&par, dev, &req, buf, &n);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
  CHECK_EQUAL(0u, n);
}

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par) with a mapping into a non-existing object,
///        a CANopen SDO upload request
///
/// \When co_pdo_up() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a pointer to the
///       buffer, an address of the number of bytes in the buffer
///
/// \Then CO_SDO_AC_NO_OBJ is returned, nothing is changed
///       \Calls co_dev_chk_tpdo()
TEST(CO_Pdo, CoPdoUp_MappingNonExistingObject) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 1u;
  par.map[0] = MakeMappingParam(0xffffu, 0x00u, 0x00u);
  size_t n = 0u;
  co_unsigned8_t buf[] = {0u};

  const auto ret = co_pdo_up(&par, dev, &req, buf, &n);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
  CHECK_EQUAL(0u, n);
}

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par), a CANopen SDO upload request
///
/// \When co_pdo_up() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a pointer to the
///       buffer, an address of the number of bytes in the buffer; but the
///       upload request of the mapped object does not include the first
///       segment of data
///
/// \Then CO_SDO_AC_PDO_LEN is returned, nothing is changed
///       \Calls co_dev_chk_tpdo()
///       \Calls co_sdo_req_clear()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_up_ind()
///       \Calls co_sdo_req_first()
TEST(CO_Pdo, CoPdoUp_ReqNotFirst) {
  CreateMappableObject();
  co_sub_set_up_ind(obj2020->GetLastSub(), &CoSubUpInd::Func, nullptr);
  co_sdo_req ret_req;
  ret_req.offset = 1u;
  ret_req.nbyte = 1u;
  ret_req.size = SUB_LEN;
  CoSubUpInd::ret_req = &ret_req;

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 1u;
  par.map[0] = MakeMappingParam(IDX, SUBIDX, SUB_LEN * CHAR_BIT);
  size_t n = SUB_LEN;
  co_unsigned8_t buf[SUB_LEN] = {0u};

  const auto ret = co_pdo_up(&par, dev, &req, buf, &n);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
  CHECK_EQUAL(SUB_LEN, n);
  for (size_t i = 0; i < SUB_LEN; ++i) CHECK_EQUAL(0x00u, buf[i]);
}

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par), a CANopen SDO upload request
///
/// \When co_pdo_up() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a pointer to the
///       buffer, an address of the number of bytes in the buffer; but the
///       upload request of the mapped object include the first segment of
///       data, but it is not also the last one
///
/// \Then CO_SDO_AC_PDO_LEN is returned, nothing is changed
///       \Calls co_dev_chk_tpdo()
///       \Calls co_sdo_req_clear()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_up_ind()
///       \Calls co_sdo_req_first()
///       \Calls co_sdo_req_last()
TEST(CO_Pdo, CoPdoUp_ReqFirstButNotLast) {
  CreateMappableObject();
  co_sub_set_up_ind(obj2020->GetLastSub(), &CoSubUpInd::Func, nullptr);
  co_sdo_req ret_req;
  ret_req.offset = 0u;
  ret_req.nbyte = 1u;
  ret_req.size = SUB_LEN;
  CoSubUpInd::ret_req = &ret_req;

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 1u;
  par.map[0] = MakeMappingParam(IDX, SUBIDX, SUB_LEN * CHAR_BIT);
  size_t n = SUB_LEN;
  co_unsigned8_t buf[SUB_LEN] = {0u};

  const auto ret = co_pdo_up(&par, dev, &req, buf, &n);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
  CHECK_EQUAL(SUB_LEN, n);
  for (size_t i = 0; i < SUB_LEN; ++i) CHECK_EQUAL(0x00u, buf[i]);
}

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par), a CANopen SDO upload request
///
/// \When co_pdo_up() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a pointer to the
///       buffer, an address of the number of bytes in the buffer; but the
///       upload indication function of the mapped object returns an error
///
/// \Then the error from the indication function is returned, nothing is
///       changed
///       \Calls co_dev_chk_tpdo()
///       \Calls co_sdo_req_clear()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_up_ind()
TEST(CO_Pdo, CoPdoUp_UpIndError) {
  CreateMappableObject();
  co_sub_set_up_ind(obj2020->GetLastSub(), &CoSubUpInd::Func, nullptr);
  CoSubUpInd::ret = CO_SDO_AC_ERROR;

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 1u;
  par.map[0] = MakeMappingParam(IDX, SUBIDX, SUB_LEN * CHAR_BIT);
  size_t n = SUB_LEN;
  co_unsigned8_t buf[SUB_LEN] = {0u};

  const auto ret = co_pdo_up(&par, dev, &req, buf, &n);

  CHECK_EQUAL(CO_SDO_AC_ERROR, ret);
  CHECK_EQUAL(SUB_LEN, n);
  for (size_t i = 0; i < SUB_LEN; ++i) CHECK_EQUAL(0x00u, buf[i]);
}

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par), a CANopen SDO upload request
///
/// \When co_pdo_up() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a null buffer
///       pointer, an address of the number of bytes in the buffer
///
/// \Then 0 is returned, the number of bytes that would have been written if
///       the buffer had been sufficiently large is stored in the number of
///       bytes in the buffer
///       \Calls co_dev_chk_tpdo()
///       \Calls co_sdo_req_clear()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_up_ind()
///       \Calls co_sdo_req_first()
///       \Calls co_sdo_req_last()
TEST(CO_Pdo, CoPdoUp_BufNull) {
  CreateMappableObject();
  co_sub_set_up_ind(obj2020->GetLastSub(), &CoSubUpInd::Func, nullptr);

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 1u;
  par.map[0] = MakeMappingParam(IDX, SUBIDX, SUB_LEN * CHAR_BIT);
  size_t n = 0u;

  const auto ret = co_pdo_up(&par, dev, &req, nullptr, &n);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(SUB_LEN, n);
}

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par), a CANopen SDO upload request
///
/// \When co_pdo_up() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a pointer to the
///       buffer, a null number of bytes in the buffer pointer
///
/// \Then 0 is returned, nothing is changed
///       \Calls co_dev_chk_tpdo()
///       \Calls co_sdo_req_clear()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_up_ind()
///       \Calls co_sdo_req_first()
///       \Calls co_sdo_req_last()
TEST(CO_Pdo, CoPdoUp_PnNull) {
  CreateMappableObject();
  co_sub_set_up_ind(obj2020->GetLastSub(), &CoSubUpInd::Func, nullptr);

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 1u;
  par.map[0] = MakeMappingParam(IDX, SUBIDX, SUB_LEN * CHAR_BIT);
  co_unsigned8_t buf[SUB_LEN] = {0u};

  const auto ret = co_pdo_up(&par, dev, &req, buf, nullptr);

  CHECK_EQUAL(0u, ret);
  for (size_t i = 0; i < SUB_LEN; ++i) CHECK_EQUAL(0x00u, buf[i]);
}

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par), a CANopen SDO upload request
///
/// \When co_pdo_up() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a pointer to the
///       buffer too small for the mapped values, an address of the number of
///       bytes in the buffer
///
/// \Then 0 is returned, the number of bytes that would have been written if
///       the buffer had been sufficiently large is stored in the number of
///       bytes in the buffer
///       \Calls co_dev_chk_tpdo()
///       \Calls co_sdo_req_clear()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_up_ind()
///       \Calls co_sdo_req_first()
///       \Calls co_sdo_req_last()
TEST(CO_Pdo, CoPdoUp_TooSmallBuffer) {
  CreateMappableObject();
  co_sub_set_up_ind(obj2020->GetLastSub(), &CoSubUpInd::Func, nullptr);

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 1u;
  par.map[0] = MakeMappingParam(IDX, SUBIDX, SUB_LEN * CHAR_BIT);
  size_t n = SUB_LEN - 1u;
  co_unsigned8_t buf[SUB_LEN] = {0u};

  const auto ret = co_pdo_up(&par, dev, &req, buf, &n);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(SUB_LEN, n);
  CHECK_EQUAL(0, ldle_u32(buf));
}

/// \Given a CANopen device (co_dev_t), a PDO mapping parameters
///        (co_pdo_map_par), a CANopen SDO upload request
///
/// \When co_pdo_up() is called with a pointer to the mapping parameters,
///       a pointer to the device, a pointer to the request, a pointer to the
///       buffer, an address of the number of bytes in the buffer
///
/// \Then 0 is returned, the mapped values are written to the buffer (in little
///       endian order), the number of written bytes is stored in the number of
///       bytes in the buffer
///       \Calls co_dev_chk_tpdo()
///       \Calls co_sdo_req_clear()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_up_ind()
///       \Calls co_sdo_req_first()
///       \Calls co_sdo_req_last()
///       \Calls bcpyle()
TEST(CO_Pdo, CoPdoUp_Nominal) {
  CreateMappableObject();
  co_sub_set_up_ind(obj2020->GetLastSub(), &CoSubUpInd::Func, nullptr);

  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 1u;
  par.map[0] = MakeMappingParam(IDX, SUBIDX, SUB_LEN * CHAR_BIT);
  size_t n = SUB_LEN;
  co_unsigned8_t buf[SUB_LEN] = {0u};

  const auto ret = co_pdo_up(&par, dev, &req, buf, &n);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(SUB_LEN, n);
  CHECK_EQUAL(val_u32, ldle_u32(buf));
}

///@}
