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

#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/sub.hpp"

TEST_BASE(CO_PdoBase) {
  co_dev_t* dev = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  const co_unsigned8_t DEV_ID = 0x1fu;
  const co_unsigned16_t DEFAULT_OBJ_IDX = 0x2020u;
  co_sdo_req req;
  char buf[8] = {0};
  membuf buffer;

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();

    CHECK(dev != nullptr);

    membuf_init(&buffer, buf, sizeof(buf));
    co_sdo_req_init(&req, &buffer);
  }

  TEST_TEARDOWN() { dev_holder.reset(); }
};

TEST_GROUP_BASE(CO_Pdo, CO_PdoBase) { const co_unsigned16_t DEFAULT_NUM = 1u; };

TEST(CO_Pdo, CoDevChkRpdo_NoObj) {
  const auto ret = co_dev_chk_rpdo(dev, DEFAULT_OBJ_IDX, 0x19u);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CO_Pdo, CoDevChkRpdo_DataTypeObjDummyEntryDisabled) {
  const co_unsigned16_t obj_idx = CO_DEFTYPE_INTEGER24;

  const auto ret = co_dev_chk_rpdo(dev, obj_idx, 0x00u);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CO_Pdo, CoDevChkRpdo_IllegalDataTypeObj) {
  const co_unsigned16_t obj_idx = CO_DEFTYPE_INTEGER24;

  const auto ret = co_dev_chk_rpdo(dev, obj_idx, 0x01u);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CO_Pdo, CoDevChkRpdo_DataTypeIsDummyEntry) {
  const co_unsigned16_t obj_idx = CO_DEFTYPE_INTEGER24;
  co_dev_set_dummy(dev, 1 << CO_DEFTYPE_INTEGER24);

  const auto ret = co_dev_chk_rpdo(dev, obj_idx, 0x00u);

  CHECK_EQUAL(0u, ret);
}

TEST(CO_Pdo, CoDevChkRpdo_NoWriteAccess) {
  CoObjTHolder obj_default(DEFAULT_OBJ_IDX);
  CHECK(obj_default.Get() != nullptr);
  obj_default.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8,
                              co_unsigned8_t(0x00u));
  co_sub_t* const sub = obj_default.GetLastSub();
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_RO));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_default.Take()));

  const auto ret = co_dev_chk_rpdo(dev, DEFAULT_OBJ_IDX, 0x00u);

  CHECK_EQUAL(CO_SDO_AC_NO_WRITE, ret);
}

TEST(CO_Pdo, CoDevChkRpdo_NoAccessRpdo) {
  CoObjTHolder obj_default(DEFAULT_OBJ_IDX);
  CHECK(obj_default.Get() != nullptr);
  obj_default.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8,
                              co_unsigned8_t(0x00u));
  co_sub_t* const sub = obj_default.GetLastSub();
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_RWR));
  co_sub_set_pdo_mapping(sub, 1);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_default.Take()));

  const auto ret = co_dev_chk_rpdo(dev, DEFAULT_OBJ_IDX, 0x00u);

  CHECK_EQUAL(CO_SDO_AC_NO_PDO, ret);
}

TEST(CO_Pdo, CoDevChkRpdo_PdoMappingFalse) {
  CoObjTHolder obj_default(DEFAULT_OBJ_IDX);
  CHECK(obj_default.Get() != nullptr);
  obj_default.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8,
                              co_unsigned8_t(0x00u));
  co_sub_t* const sub = obj_default.GetLastSub();
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_WO));
  co_sub_set_pdo_mapping(sub, 0);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_default.Take()));

  const auto ret = co_dev_chk_rpdo(dev, DEFAULT_OBJ_IDX, 0x00u);

  CHECK_EQUAL(CO_SDO_AC_NO_PDO, ret);
}

TEST(CO_Pdo, CoDevChkRpdo_NoSub) {
  CoObjTHolder obj_default(DEFAULT_OBJ_IDX);
  CHECK(obj_default.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_default.Take()));

  const auto ret = co_dev_chk_rpdo(dev, DEFAULT_OBJ_IDX, 0x03u);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
}

TEST(CO_Pdo, CoDevChkRpdo) {
  CoObjTHolder obj_default(DEFAULT_OBJ_IDX);
  CHECK(obj_default.Get() != nullptr);
  obj_default.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8,
                              co_unsigned8_t(0x00u));
  co_sub_t* const sub = obj_default.GetLastSub();
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_WO));
  co_sub_set_pdo_mapping(sub, 1);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_default.Take()));

  const auto ret = co_dev_chk_rpdo(dev, DEFAULT_OBJ_IDX, 0x00u);

  CHECK_EQUAL(0u, ret);
}

TEST(CO_Pdo, CoDevCfgRpdo_InvalidPdoNum) {
  const co_unsigned16_t num = CO_NUM_PDOS + 1u;
  const co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  const co_pdo_map_par map = CO_PDO_MAP_PAR_INIT;

  const auto ret = co_dev_cfg_rpdo(dev, num, &comm, &map);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CO_Pdo, CoDevCfgRpdo_NoPdoMappingParamObj) {
  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0;
  const co_pdo_map_par map = CO_PDO_MAP_PAR_INIT;

  CoObjTHolder obj1400(0x1400);
  CHECK(obj1400.Get() != nullptr);
  obj1400.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1400.Take()));

  const auto ret = co_dev_cfg_rpdo(dev, DEFAULT_NUM, &comm, &map);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CO_Pdo, CoDevCfgRpdo_ReenableRpdo) {
  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0;
  comm.cobid = 0x00000000u;
  const co_pdo_map_par map = CO_PDO_MAP_PAR_INIT;

  CoObjTHolder obj1400(0x1400u);
  CHECK(obj1400.Get() != nullptr);
  obj1400.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1400.Take()));

  CoObjTHolder obj1600(0x1600u);
  CHECK(obj1600.Get() != nullptr);
  obj1600.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1600.Take()));

  const auto ret = co_dev_cfg_rpdo(dev, DEFAULT_NUM, &comm, &map);

  CHECK_EQUAL(0u, ret);
}

TEST(CO_Pdo, CoDevCfgRpdo) {
  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0u;
  const co_pdo_map_par map = CO_PDO_MAP_PAR_INIT;

  CoObjTHolder obj1400(0x1400u);
  CHECK(obj1400.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj1400.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1400.Take()));

  CoObjTHolder obj1600(0x1600u);
  CHECK(obj1600.Get() != nullptr);
  // 0x00 - number of mapped application objects in PDO
  obj1600.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1600.Take()));

  const auto ret = co_dev_cfg_rpdo(dev, DEFAULT_NUM, &comm, &map);

  CHECK_EQUAL(0u, ret);
}

TEST(CO_Pdo, CoDevCfgRpdoComm_NumZero) {
  const co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  const co_unsigned16_t num = 0u;

  const auto ret = co_dev_cfg_rpdo_comm(dev, num, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CO_Pdo, CoDevCfgRpdoComm_NumTooBig) {
  const co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  const co_unsigned16_t num = CO_NUM_PDOS + 1u;

  const auto ret = co_dev_cfg_rpdo_comm(dev, num, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CO_Pdo, CoDevCfgRpdoComm) {
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x01u;
  par.cobid = DEV_ID;

  CoObjTHolder obj(0x1400u);
  CHECK(obj.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x01u));
  // 0x01 - COB-ID
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                      co_unsigned32_t(0x00000000u));

  const auto ret = co_dev_cfg_rpdo_comm(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00));
  CHECK_EQUAL(par.cobid, co_obj_get_val_u32(obj.Get(), 0x01));
}

TEST(CO_Pdo, CoDevCfgRpdoMap_NumZero) {
  const co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  const co_unsigned16_t num = 0u;

  const auto ret = co_dev_cfg_rpdo_map(dev, num, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CO_Pdo, CoDevCfgRpdoMap_NumTooBig) {
  const co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  const co_unsigned16_t num = CO_NUM_PDOS + 1u;

  const auto ret = co_dev_cfg_rpdo_map(dev, num, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CO_Pdo, CoDevCfgRpdoMap) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 0x01u;
  par.map[0] = 0x20000000u;  // idx: 0x2000 subidx: 0x00 len: 0x00

  CoObjTHolder obj(0x1600u);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                      co_unsigned32_t(0x00000000u));

  const auto ret = co_dev_cfg_rpdo_map(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
  CHECK_EQUAL(par.map[0], co_obj_get_val_u32(obj.Get(), 0x01u));
}

TEST(CO_Pdo, CoDevCfgPdoComm_NoObj) {
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;

  CoObjTHolder obj_default(DEFAULT_OBJ_IDX);
  CHECK(obj_default.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_default.Take()));

  const auto ret = co_dev_cfg_rpdo_comm(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CO_Pdo, CoDevCfgPdoComm_NoSubs) {
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x03u;

  CoObjTHolder obj(0x1400u);
  CHECK(obj.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const auto ret = co_dev_cfg_rpdo_comm(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
}

TEST(CO_Pdo, CoDevCfgPdoComm_NoCobid) {
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x01u;

  CoObjTHolder obj(0x1400);
  CHECK(obj.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x01u));

  const auto ret = co_dev_cfg_rpdo_comm(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
}

TEST(CO_Pdo, CoDevCfgPdoComm_ConfigureCobid) {
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x01u;
  par.cobid = DEV_ID;

  CoObjTHolder obj(0x1400);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x01u));
  // 0x01 - COB-ID
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                      co_unsigned32_t(0x00000000u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_rpdo_comm(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
  CHECK_EQUAL(par.cobid, co_obj_get_val_u32(obj.Get(), 0x01u));
}

TEST(CO_Pdo, CoDevCfgPdoComm_ConfigureCobidTypeBroken) {
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x02u;
  par.cobid = DEV_ID;

  CoObjTHolder obj(0x1400u);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x02u));
  // 0x01 - COB-ID
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00000000u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_rpdo_comm(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
  CHECK_EQUAL(0u, co_obj_get_val_u32(obj.Get(), 0x01u));
}

TEST(CO_Pdo, CoDevCfgPdoComm_NoTransmission) {
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x02u;
  par.cobid = DEV_ID;
  par.trans = 0x3du;

  CoObjTHolder obj(0x1400u);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x02u));
  // 0x01 - COB-ID
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                      co_unsigned32_t(0x00000000u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_rpdo_comm(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
  CHECK_EQUAL(par.cobid, co_obj_get_val_u32(obj.Get(), 0x01u));
}

TEST(CO_Pdo, CoDevCfgPdoComm_WithTransmission) {
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x02u;
  par.cobid = DEV_ID;
  par.trans = 0x3du;

  CoObjTHolder obj(0x1400u);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x02u));
  // 0x01 - COB-ID
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                      co_unsigned32_t(0x00000000u));
  // 0x02 - transmission type
  obj.InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_rpdo_comm(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
  CHECK_EQUAL(par.cobid, co_obj_get_val_u32(obj.Get(), 0x01u));
  CHECK_EQUAL(par.trans, co_obj_get_val_u8(obj.Get(), 0x02u));
}

TEST(CO_Pdo, CoDevCfgPdoComm_TransmissionTypeBroken) {
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x03u;
  par.cobid = DEV_ID;
  par.trans = 0x3du;

  CoObjTHolder obj(0x1400u);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x03u));
  // 0x01 - COB-ID
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                      co_unsigned32_t(0x00000000u));
  // 0x02 - transmission type
  obj.InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED48,
                      co_unsigned48_t(0x000000000000u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_rpdo_comm(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
  CHECK_EQUAL(par.cobid, co_obj_get_val_u32(obj.Get(), 0x01u));
}

TEST(CO_Pdo, CoDevCfgPdoComm_NoInhibit) {
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x03u;
  par.cobid = DEV_ID;
  par.trans = 0x3du;

  CoObjTHolder obj(0x1400u);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x03u));
  // 0x01 - COB-ID
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                      co_unsigned32_t(0x00000000u));
  // 0x02 - transmission type
  obj.InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_rpdo_comm(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
  CHECK_EQUAL(par.cobid, co_obj_get_val_u32(obj.Get(), 0x01u));
  CHECK_EQUAL(par.trans, co_obj_get_val_u8(obj.Get(), 0x02u));
}

TEST(CO_Pdo, CoDevCfgPdoComm_WithInhibit) {
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x03u;
  par.cobid = DEV_ID;
  par.trans = 0x3du;
  par.inhibit = 0x1111u;

  CoObjTHolder obj(0x1400u);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x03u));
  // 0x01 - COB-ID
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                      co_unsigned32_t(0x00000000u));
  // 0x02 - transmission type
  obj.InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  // 0x03 - inhibit time
  obj.InsertAndSetSub(0x03u, CO_DEFTYPE_UNSIGNED16, co_unsigned16_t(0x0000u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_rpdo_comm(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
  CHECK_EQUAL(par.cobid, co_obj_get_val_u32(obj.Get(), 0x01u));
  CHECK_EQUAL(par.trans, co_obj_get_val_u8(obj.Get(), 0x02u));
  CHECK_EQUAL(par.inhibit, co_obj_get_val_u16(obj.Get(), 0x03u));
}

TEST(CO_Pdo, CoDevCfgPdoComm_InhibitTypeBroken) {
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x05u;
  par.cobid = DEV_ID;
  par.trans = 0x3du;
  par.inhibit = 0x1111u;

  CoObjTHolder obj(0x1400u);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x05u));
  // 0x01 - COB-ID
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                      co_unsigned32_t(0x00000000u));
  // 0x02 - transmission type
  obj.InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  // 0x03 - inhibit time
  obj.InsertAndSetSub(0x03u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_rpdo_comm(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
  CHECK_EQUAL(par.cobid, co_obj_get_val_u32(obj.Get(), 0x01u));
  CHECK_EQUAL(par.trans, co_obj_get_val_u8(obj.Get(), 0x02u));
}

TEST(CO_Pdo, CoDevCfgPdoComm_NoEvent) {
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x05u;
  par.cobid = DEV_ID;
  par.trans = 0x3du;
  par.inhibit = 0x1111u;

  CoObjTHolder obj(0x1400u);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x05u));
  // 0x01 - COB-ID
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                      co_unsigned32_t(0x00000000u));
  // 0x02 - transmission type
  obj.InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  // 0x03 - inhibit time
  obj.InsertAndSetSub(0x03u, CO_DEFTYPE_UNSIGNED16, co_unsigned16_t(0x0000u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_rpdo_comm(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
  CHECK_EQUAL(par.cobid, co_obj_get_val_u32(obj.Get(), 0x01u));
  CHECK_EQUAL(par.trans, co_obj_get_val_u8(obj.Get(), 0x02u));
  CHECK_EQUAL(par.inhibit, co_obj_get_val_u16(obj.Get(), 0x03u));
}

TEST(CO_Pdo, CoDevCfgPdoComm_WithEvent) {
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x05u;
  par.cobid = DEV_ID;
  par.trans = 0x3du;
  par.inhibit = 0x1111u;
  par.event = 0xa213u;

  CoObjTHolder obj(0x1400u);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x05u));
  // 0x01 - COB-ID
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                      co_unsigned32_t(0x00000000u));
  // 0x02 - transmission type
  obj.InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  // 0x03 - inhibit time
  obj.InsertAndSetSub(0x03u, CO_DEFTYPE_UNSIGNED16, co_unsigned16_t(0x0000u));
  // 0x04 - reserved (compatibility entry)
  obj.InsertAndSetSub(0x04u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  // 0x05 - event-timer
  obj.InsertAndSetSub(0x05u, CO_DEFTYPE_UNSIGNED16, co_unsigned16_t(0x0000u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_rpdo_comm(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
  CHECK_EQUAL(par.cobid, co_obj_get_val_u32(obj.Get(), 0x01u));
  CHECK_EQUAL(par.trans, co_obj_get_val_u8(obj.Get(), 0x02u));
  CHECK_EQUAL(par.inhibit, co_obj_get_val_u16(obj.Get(), 0x03u));
  CHECK_EQUAL(0, co_obj_get_val_u8(obj.Get(), 0x04u));
}

TEST(CO_Pdo, CoDevCfgPdoComm_EventTypeBroken) {
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x06u;
  par.cobid = DEV_ID;
  par.trans = 0x3du;
  par.inhibit = 0x1111u;
  par.event = 0xa213u;

  CoObjTHolder obj(0x1400u);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x06u));
  // 0x01 - COB-ID
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                      co_unsigned32_t(0x00000000u));
  // 0x02 - transmission type
  obj.InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  // 0x03 - inhibit time
  obj.InsertAndSetSub(0x03u, CO_DEFTYPE_UNSIGNED16, co_unsigned16_t(0x0000u));
  // 0x04 - reserved (compatibility entry)
  obj.InsertAndSetSub(0x04u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  // 0x05 - event-timer
  obj.InsertAndSetSub(0x05u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_rpdo_comm(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
  CHECK_EQUAL(par.cobid, co_obj_get_val_u32(obj.Get(), 0x01u));
  CHECK_EQUAL(par.trans, co_obj_get_val_u8(obj.Get(), 0x02u));
  CHECK_EQUAL(par.inhibit, co_obj_get_val_u16(obj.Get(), 0x03u));
  CHECK_EQUAL(0, co_obj_get_val_u8(obj.Get(), 0x04u));
}

TEST(CO_Pdo, CoDevCfgPdoComm_NoSync) {
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x06u;
  par.cobid = DEV_ID;
  par.trans = 0x3du;
  par.inhibit = 0x1111u;
  par.event = 0xa213u;

  CoObjTHolder obj(0x1400u);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x06u));
  // 0x01 - COB-ID
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                      co_unsigned32_t(0x00000000u));
  // 0x02 - transmission type
  obj.InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  // 0x03 - inhibit time
  obj.InsertAndSetSub(0x03u, CO_DEFTYPE_UNSIGNED16, co_unsigned16_t(0x0000u));
  // 0x04 - reserved (compatibility entry)
  obj.InsertAndSetSub(0x04u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  // 0x05 - event-timer
  obj.InsertAndSetSub(0x5u, CO_DEFTYPE_UNSIGNED16, co_unsigned16_t(0x0000u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_rpdo_comm(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
  CHECK_EQUAL(par.cobid, co_obj_get_val_u32(obj.Get(), 0x01u));
  CHECK_EQUAL(par.trans, co_obj_get_val_u8(obj.Get(), 0x02u));
  CHECK_EQUAL(par.inhibit, co_obj_get_val_u16(obj.Get(), 0x03u));
  CHECK_EQUAL(0, co_obj_get_val_u8(obj.Get(), 0x04u));
  CHECK_EQUAL(par.event, co_obj_get_val_u16(obj.Get(), 0x05u));
}

TEST(CO_Pdo, CoDevCfgPdoComm) {
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x06u;
  par.cobid = DEV_ID;
  par.trans = 0x01u;
  par.inhibit = 0x2344u;
  par.reserved = 0xffu;
  par.event = 0x0031u;
  par.sync = 0x01u;

  CoObjTHolder obj(0x1400u);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x06u));
  // 0x01 - COB-ID
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                      co_unsigned32_t(0x00000000u));
  // 0x02 - transmission type
  obj.InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  // 0x03 - inhibit time
  obj.InsertAndSetSub(0x03u, CO_DEFTYPE_UNSIGNED16, co_unsigned16_t(0x0000u));
  // 0x04 - reserved (compatibility entry)
  obj.InsertAndSetSub(0x04u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  // 0x05 - event-timer
  obj.InsertAndSetSub(0x05u, CO_DEFTYPE_UNSIGNED16, co_unsigned16_t(0x0000u));
  // 0x06 - SYNC start value
  obj.InsertAndSetSub(0x06u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_rpdo_comm(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
  CHECK_EQUAL(par.cobid, co_obj_get_val_u32(obj.Get(), 0x01u));
  CHECK_EQUAL(par.trans, co_obj_get_val_u8(obj.Get(), 0x02u));
  CHECK_EQUAL(par.inhibit, co_obj_get_val_u16(obj.Get(), 0x03u));
  CHECK_EQUAL(0, co_obj_get_val_u8(obj.Get(), 0x04u));
  CHECK_EQUAL(par.event, co_obj_get_val_u16(obj.Get(), 0x05u));
  CHECK_EQUAL(par.sync, co_obj_get_val_u8(obj.Get(), 0x06u));
}

TEST(CO_Pdo, CoDevCfgPdoMap_NoObj) {
  const co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;

  const auto ret = co_dev_cfg_tpdo_map(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CO_Pdo, CoDevCfgPdoMap_NoSubZero) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 0x06u;

  CoObjTHolder obj(0x1a00u);
  CHECK(obj.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const auto ret = co_dev_cfg_tpdo_map(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
}

TEST(CO_Pdo, CoDevCfgPdoMap_ErrorWhenDisablingMapping) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 0x06u;

  CoObjTHolder obj(0x1a00u);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED16, co_unsigned16_t(0x0000u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_tpdo_map(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN, ret);
  CHECK_EQUAL(0u, co_obj_get_val_u8(obj.Get(), 0x00u));
}

TEST(CO_Pdo, CoDevCfgPdoMap_NoSubOne) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 0x06u;

  CoObjTHolder obj(0x1a00u);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_tpdo_map(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
  CHECK_EQUAL(0u, co_obj_get_val_u8(obj.Get(), 0x00u));
}

TEST(CO_Pdo, CoDevCfgPdoMap_ErrorWhenCopyingMappingParameters) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 0x01u;

  CoObjTHolder obj(0x1a00u);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  // 0x01 - COB-ID
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED16, co_unsigned16_t(0x0000u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_tpdo_map(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN, ret);
  CHECK_EQUAL(0u, co_obj_get_val_u8(obj.Get(), 0x01));
}

TEST(CO_Pdo, CoDevCfgPdoMap_MaxMapped) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = CO_PDO_NUM_MAPS;
  CoObjTHolder obj(0x1600u);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8,
                      co_unsigned8_t(CO_PDO_NUM_MAPS));

  for (size_t i = 1; i <= CO_PDO_NUM_MAPS; i++) {
    par.map[i - 1] = 0x20000000u;  // idx: 0x2000 subidx: 0x00 len: 0x00
    obj.InsertAndSetSub(i, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t(0x00000000u));
  }

  const auto ret = co_dev_cfg_rpdo_map(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
  for (size_t i = 1; i < CO_PDO_NUM_MAPS; i++)
    CHECK_EQUAL(par.map[i - 1], co_obj_get_val_u32(obj.Get(), i));
}

TEST(CO_Pdo, CoDevCfgPdoMap) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 0x01u;
  par.map[0] = 0x20200000u;  // idx: 0x2020 subidx: 0x00 len: 0x00

  CoObjTHolder obj(0x1a00);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  // 0x01 - COB-ID
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                      co_unsigned32_t(0x00000000u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_tpdo_map(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
  CHECK_EQUAL(par.map[0], co_obj_get_val_u32(obj.Get(), 0x01u));
}

TEST(CO_Pdo, CoDevChkTpdo_NoObj) {
  const auto ret = co_dev_chk_tpdo(dev, DEFAULT_OBJ_IDX, 0x00u);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CO_Pdo, CoDevChkTpdo_NoSub) {
  CoObjTHolder obj_default(DEFAULT_OBJ_IDX);
  CHECK(obj_default.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_default.Take()));

  const auto ret = co_dev_chk_tpdo(dev, DEFAULT_OBJ_IDX, 0x00u);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
}

TEST(CO_Pdo, CoDevChkTpdo_NoReadAccess) {
  CoObjTHolder obj_default(DEFAULT_OBJ_IDX);
  CHECK(obj_default.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_default.Take()));
  obj_default.InsertAndSetSub(0x00u, CO_DEFTYPE_INTEGER16,
                              co_integer16_t(0x0000));
  co_sub_t* const sub = obj_default.GetLastSub();
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_WO));

  const auto ret = co_dev_chk_tpdo(dev, DEFAULT_OBJ_IDX, 0x00u);

  CHECK_EQUAL(CO_SDO_AC_NO_READ, ret);
}

TEST(CO_Pdo, CoDevChkTpdo_PdoMappingFalse) {
  CoObjTHolder obj_default(DEFAULT_OBJ_IDX);
  CHECK(obj_default.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_default.Get()));
  obj_default.InsertAndSetSub(0x00u, CO_DEFTYPE_INTEGER16,
                              co_integer16_t(0x0000));

  const auto ret = co_dev_chk_tpdo(dev, DEFAULT_OBJ_IDX, 0x00u);

  CHECK_EQUAL(CO_SDO_AC_NO_PDO, ret);
}

TEST(CO_Pdo, CoDevChkTpdo_NoTPDOAccess) {
  CoObjTHolder obj_default(DEFAULT_OBJ_IDX);
  CHECK(obj_default.Get() != nullptr);
  obj_default.InsertAndSetSub(0x00u, CO_DEFTYPE_INTEGER16,
                              co_integer16_t(0x0000));
  co_sub_t* const sub = obj_default.GetLastSub();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_default.Take()));
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_RWW));
  co_sub_set_pdo_mapping(sub, 1);

  const auto ret = co_dev_chk_tpdo(dev, DEFAULT_OBJ_IDX, 0x00u);

  CHECK_EQUAL(CO_SDO_AC_NO_PDO, ret);
}

TEST(CO_Pdo, CoDevChkTpdo) {
  CoObjTHolder obj_default(DEFAULT_OBJ_IDX);
  CHECK(obj_default.Get() != nullptr);
  obj_default.InsertAndSetSub(0x00u, CO_DEFTYPE_INTEGER16,
                              co_integer16_t(0x0000));
  co_sub_t* const sub = obj_default.GetLastSub();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_default.Take()));
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_RWR));
  co_sub_set_pdo_mapping(sub, 1);

  const auto ret = co_dev_chk_tpdo(dev, DEFAULT_OBJ_IDX, 0x00u);

  CHECK_EQUAL(0u, ret);
}

TEST(CO_Pdo, CoDevCfgTpdo_InvalidPdoNum) {
  const co_unsigned16_t num = 0;
  const co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;

  const auto ret = co_dev_cfg_tpdo(dev, num, &comm, nullptr);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CO_Pdo, CoDevCfgTpdo_NoSub) {
  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0u;
  co_pdo_map_par map = CO_PDO_MAP_PAR_INIT;

  CoObjTHolder obj1800(0x1800u);
  CHECK(obj1800.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1800.Take()));

  CoObjTHolder obj1a00(0x1a00u);
  CHECK(obj1a00.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1a00.Take()));

  const auto ret = co_dev_cfg_tpdo(dev, DEFAULT_NUM, &comm, &map);

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, ret);
}

TEST(CO_Pdo, CoDevCfgTpdo_ReenableTpdo) {
  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0;
  comm.cobid = DEV_ID;
  co_pdo_map_par map = CO_PDO_MAP_PAR_INIT;
  map.n = 0;

  CoObjTHolder obj1800(0x1800u);
  CHECK(obj1800.Get() != nullptr);
  obj1800.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1800.Take()));

  CoObjTHolder obj1a00(0x1a00u);
  CHECK(obj1a00.Get() != nullptr);
  obj1a00.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1a00.Take()));

  const auto ret = co_dev_cfg_tpdo(dev, DEFAULT_NUM, &comm, &map);

  CHECK_EQUAL(0u, ret);
}

TEST(CO_Pdo, CoDevCfgTpdo) {
  co_pdo_comm_par comm = CO_PDO_COMM_PAR_INIT;
  comm.n = 0;
  co_pdo_map_par map = CO_PDO_MAP_PAR_INIT;

  CoObjTHolder obj1800(0x1800u);
  CHECK(obj1800.Get() != nullptr);
  obj1800.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1800.Take()));

  CoObjTHolder obj1a00(0x1a00u);
  CHECK(obj1a00.Get() != nullptr);
  obj1a00.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1a00.Take()));

  const auto ret = co_dev_cfg_tpdo(dev, DEFAULT_NUM, &comm, &map);

  CHECK_EQUAL(0u, ret);
}

TEST(CO_Pdo, CoDevCfgTpdoComm_NumZero) {
  const co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  const co_unsigned16_t num = 0u;

  const auto ret = co_dev_cfg_tpdo_comm(dev, num, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CO_Pdo, CoDevCfgTpdoComm_NumTooBig) {
  const co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  const co_unsigned16_t num = CO_NUM_PDOS + 1u;

  const auto ret = co_dev_cfg_tpdo_comm(dev, num, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CO_Pdo, CoDevCfgTpdoComm) {
  const co_unsigned16_t num = 511u;
  const co_unsigned16_t obj_idx = 0x19feu;
  co_pdo_comm_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x01u;
  par.cobid = DEV_ID;

  CoObjTHolder obj(obj_idx);
  CHECK(obj.Get() != nullptr);
  // 0x00 - highest sub-index supported
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x01u));
  // 0x01 - COB-ID
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                      co_unsigned32_t(0x00000000u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Get()));

  const auto ret = co_dev_cfg_tpdo_comm(dev, num, &par);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00));
  CHECK_EQUAL(par.cobid, co_obj_get_val_u32(obj.Get(), 0x01));
}

TEST(CO_Pdo, CoDevCfgTpdoMap_NumZero) {
  const co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  const co_unsigned16_t num = 0u;

  const auto ret = co_dev_cfg_tpdo_map(dev, num, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CO_Pdo, CoDevCfgTpdoMap_NumTooBig) {
  const co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  const co_unsigned16_t num = CO_NUM_PDOS + 1u;

  const auto ret = co_dev_cfg_tpdo_map(dev, num, &par);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CO_Pdo, CoDevCfgTpdoMap) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 0x01u;
  par.map[0] = 0x20200000u;  // idx: 0x2020 subidx: 0x00 len: 0x00

  CoObjTHolder obj(0x1a00u);
  CHECK(obj.Get() != nullptr);
  obj.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  obj.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                      co_unsigned32_t(0x00000000u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const auto ret = co_dev_cfg_tpdo_map(dev, DEFAULT_NUM, &par);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(par.n, co_obj_get_val_u8(obj.Get(), 0x00u));
  CHECK_EQUAL(par.map[0], co_obj_get_val_u32(obj.Get(), 0x01u));
}

TEST(CO_Pdo, CoPdoMap_OversizedPdoMap) {
  co_pdo_map_par par = CO_PDO_COMM_PAR_INIT;
  par.n = CO_PDO_NUM_MAPS + 1u;
  const co_unsigned8_t n_vals = 1u;
  const co_unsigned64_t vals[1] = {0u};

  const auto ret = co_pdo_map(&par, vals, n_vals, nullptr, nullptr);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
}

TEST(CO_Pdo, CoPdoMap_RequestedNumNotEqualToGiven) {
  co_pdo_map_par par = CO_PDO_COMM_PAR_INIT;
  par.n = CO_PDO_NUM_MAPS - 1u;
  const co_unsigned8_t n_vals = 1u;
  const co_unsigned64_t vals[1] = {0u};

  const auto ret = co_pdo_map(&par, vals, n_vals, nullptr, nullptr);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
}

TEST(CO_Pdo, CoPdoMap_MappedZeroNBufNull) {
  co_pdo_map_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x00u;
  const co_unsigned8_t n_vals = 0u;
  const co_unsigned64_t vals[1] = {0u};

  const auto ret = co_pdo_map(&par, vals, n_vals, nullptr, nullptr);

  CHECK_EQUAL(0u, ret);
}

TEST(CO_Pdo, CoPdoMap_BufNull) {
  co_pdo_map_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x01u;
  par.map[0] = 0x00000001u;
  const co_unsigned8_t n_vals = 1u;
  const co_unsigned64_t vals[1] = {0u};
  size_t n_buf = 0u;

  const auto ret = co_pdo_map(&par, vals, n_vals, nullptr, &n_buf);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(1u, n_buf);
}

TEST(CO_Pdo, CoPdoMap_BufNullNBuffNullRequestedEqualToGiven) {
  co_pdo_map_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x05u;
  const co_unsigned8_t n_vals = 5u;
  const co_unsigned64_t vals[5] = {0u};

  const auto ret = co_pdo_map(&par, vals, n_vals, nullptr, nullptr);

  CHECK_EQUAL(0u, ret);
}

TEST(CO_Pdo, CoPdoMap_MappingExceedsMaxPDOSize) {
  co_pdo_map_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x02u;
  par.map[0] = 0x00000001u;
  par.map[1] = 0x000000ffu;
  const co_unsigned8_t n_vals = 2u;
  const co_unsigned64_t vals[2] = {0u, 0u};
  size_t n_buf = 5u;
  uint_least8_t buf[5] = {0};

  const auto ret = co_pdo_map(&par, vals, n_vals, buf, &n_buf);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
  CHECK_EQUAL(5u, n_buf);
}

TEST(CO_Pdo, CoPdoMap_MapTooBigNBufNull) {
  co_pdo_map_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x05u;
  par.map[0] = 0x00000001u;
  const co_unsigned8_t n_vals = 5u;
  const co_unsigned64_t vals[5] = {0u};
  uint_least8_t buf[1] = {0};

  const auto ret = co_pdo_map(&par, vals, n_vals, buf, nullptr);

  CHECK_EQUAL(0u, ret);
}

TEST(CO_Pdo, CoPdoMap_BufferTooSmall) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 0x01u;
  par.map[0] = 0x00000009u;
  const co_unsigned8_t n_vals = 1u;
  const co_unsigned64_t vals[1] = {0u};
  size_t n_buf = 1u;
  uint_least8_t buf[1] = {0};

  const auto ret = co_pdo_map(&par, vals, n_vals, buf, &n_buf);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(2u, n_buf);
  CHECK_EQUAL(0x00u, buf[0]);
}

TEST(CO_Pdo, CoPdoMap) {
  co_pdo_map_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x02u;
  par.map[0] =
      0x00000020u;  // each element of val has 32 bits that we want to map
  par.map[1] = 0x00000020u;
  const co_unsigned8_t n_vals = 2u;
  const co_unsigned64_t vals[2] = {0x00000000ddddddddu, 0x00000000ccccccccu};
  size_t n_buf = 9u;
  uint_least8_t buf[9] = {0x00u};

  const auto ret = co_pdo_map(&par, vals, n_vals, buf, &n_buf);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(8u, n_buf);
  CHECK_EQUAL(0xddu, buf[0]);
  CHECK_EQUAL(0xddu, buf[1]);
  CHECK_EQUAL(0xddu, buf[2]);
  CHECK_EQUAL(0xddu, buf[3]);
  CHECK_EQUAL(0xccu, buf[4]);
  CHECK_EQUAL(0xccu, buf[5]);
  CHECK_EQUAL(0xccu, buf[6]);
  CHECK_EQUAL(0xccu, buf[7]);
  CHECK_EQUAL(0x00u, buf[8]);
}

TEST(CO_Pdo, CoPdoUnmap_OversizedPdoMap) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = CO_PDO_NUM_MAPS + 1u;
  const size_t n_buf = 0u;
  const uint_least8_t buf[1] = {0x00u};

  const auto ret = co_pdo_unmap(&par, buf, n_buf, nullptr, nullptr);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
}

TEST(CO_Pdo, CoPdoUnmap_DeclaredMorePdosThanGiven) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 0x02u;
  par.map[0] = 0x00000001u;
  const size_t n_buf = 1u;
  const uint_least8_t buf[1] = {0x00u};
  co_unsigned8_t n_vals = 1u;

  const auto ret = co_pdo_unmap(&par, buf, n_buf, nullptr, &n_vals);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(2u, n_vals);
}

TEST(CO_Pdo, CoPdoUnmap_NValsNull) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 0x02u;
  par.map[0] = 0x00000001u;
  const size_t n_buf = 1u;
  const uint_least8_t buf[1] = {0x00u};
  co_unsigned64_t vals[1] = {0u};

  const auto ret = co_pdo_unmap(&par, buf, n_buf, vals, nullptr);

  CHECK_EQUAL(0u, ret);
}

TEST(CO_Pdo, CoPdoUnmap_MapNonzero) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 0x03u;
  par.map[0] = 0x00000001u;
  par.map[1] = 0x00000002u;
  par.map[2] = 0x00000001u;
  const size_t n_buf = 1u;
  const uint_least8_t buf[1] = {0x00u};
  co_unsigned8_t n_vals = 1;
  co_unsigned64_t vals[1] = {0u};

  const auto ret = co_pdo_unmap(&par, buf, n_buf, vals, &n_vals);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(3u, n_vals);
}

TEST(CO_Pdo, CoPdoUnmap_DeclaredNumOfPdosNonzeroButGivenZero) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  par.n = 0x02u;
  par.map[0] = 0x00000001u;
  const size_t n_buf = 0u;
  const uint_least8_t buf[1] = {0x00u};

  const auto ret = co_pdo_unmap(&par, buf, n_buf, nullptr, nullptr);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
}

TEST(CO_Pdo, CoPdoUnmap) {
  co_pdo_map_par par = CO_PDO_COMM_PAR_INIT;
  par.n = 0x02u;
  par.map[0] = 0x00000020u;  // in this test case, each element of vals should
                             // have 32 bits
  par.map[1] = 0x00000020u;
  const size_t n_buf = 8u;
  const uint_least8_t buf[8] = {0xddu, 0xddu, 0xddu, 0xddu,
                                0xccu, 0xccu, 0xccu, 0xccu};
  co_unsigned8_t n_vals = 3u;
  co_unsigned64_t vals[3] = {0x0000000000000000u, 0x0000000000000000u,
                             0x0000000000000000u};

  const auto ret = co_pdo_unmap(&par, buf, n_buf, vals, &n_vals);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(2u, n_vals);
  CHECK_EQUAL(0x00000000ddddddddu, vals[0]);
  CHECK_EQUAL(0x00000000ccccccccu, vals[1]);
}

TEST_GROUP_BASE(CoPdo_CoPdoDn, CO_PdoBase) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;

  static bool co_sub_dn_ind_called;

  static co_unsigned32_t co_sub_dn_ind_error(co_sub_t*, struct co_sdo_req*,
                                             co_unsigned32_t ac, void*) {
    if (ac) return ac;
    return CO_SDO_AC_PARAM_VAL;
  }
  static co_unsigned32_t co_sub_dn_ind_ok(co_sub_t*, co_sdo_req*,
                                          co_unsigned32_t ac, void*) {
    if (ac) return ac;
    co_sub_dn_ind_called = true;
    return 0u;
  }
};
bool TEST_GROUP_CppUTestGroupCoPdo_CoPdoDn::co_sub_dn_ind_called = false;

TEST(CoPdo_CoPdoDn, BufBiggerThanCanMaxLen) {
  uint_least8_t buf[1] = {0};
  const size_t n = CAN_MAX_LEN + 1u;

  const auto ret = co_pdo_dn(&par, dev, &req, buf, n);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
}

TEST(CoPdo_CoPdoDn, NoParameters) {
  uint_least8_t buf[1] = {0};
  const size_t n = 6u;

  const auto ret = co_pdo_dn(&par, dev, &req, buf, n);

  CHECK_EQUAL(0u, ret);
}

TEST(CoPdo_CoPdoDn, ObjectDoesNotExist) {
  par.n = 0x02u;
  par.map[0] = 0x00000001u;
  uint_least8_t buf[1] = {0};
  const size_t n = 6u;

  const auto ret = co_pdo_dn(&par, dev, &req, buf, n);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
}

TEST(CoPdo_CoPdoDn, BufferTooSmall) {
  par.n = 0x03u;
  par.map[0] = 0x00000001u;
  uint_least8_t buf[1] = {0};
  const size_t n = 0;

  const auto ret = co_pdo_dn(&par, dev, &req, buf, n);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
}

TEST(CoPdo_CoPdoDn, DownloadIndicatorReturnsError) {
  par.n = 0x01u;
  par.map[0] = 0x00000001u;
  uint_least8_t buf[1] = {0};
  const size_t n = 2u;

  CoObjTHolder obj_default(DEFAULT_OBJ_IDX);
  CHECK(obj_default.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_default.Take()));
  obj_default.InsertAndSetSub(0x19u, CO_DEFTYPE_UNSIGNED8,
                              co_unsigned8_t(0x00u));

  CoObjTHolder obj0000(0x0000u);
  CHECK(obj0000.Get() != nullptr);
  obj0000.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  co_sub_t* const sub0000_0 = obj0000.GetLastSub();
  CHECK_EQUAL(0, co_sub_set_access(sub0000_0, CO_ACCESS_RW));
  co_sub_set_pdo_mapping(sub0000_0, 1);
  co_sub_set_dn_ind(sub0000_0, &co_sub_dn_ind_error, nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj0000.Take()));

  const auto ret = co_pdo_dn(&par, dev, &req, buf, n);

  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, ret);
}

TEST(CoPdo_CoPdoDn, AllTypesAreDummyEntries) {
  par.n = 0x02u;
  par.map[0] = 0x00010000u;
  uint_least8_t buf[1] = {0};
  const size_t n = 2u;
  co_dev_set_dummy(dev, 0xffffffffu);

  const auto ret = co_pdo_dn(&par, dev, &req, buf, n);

  CHECK_EQUAL(0u, ret);
}

TEST(CoPdo_CoPdoDn, Nominal) {
  par.n = 0x03u;
  par.map[0] = 0x00000001u;
  uint_least8_t buf[1] = {0};
  const size_t n = 2u;

  CoObjTHolder obj_default(DEFAULT_OBJ_IDX);
  CHECK(obj_default.Get() != nullptr);
  obj_default.InsertAndSetSub(0x19u, CO_DEFTYPE_INTEGER8, co_integer8_t(0x00u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_default.Take()));

  CoObjTHolder obj0000(0x0000u);
  CHECK(obj0000.Get() != nullptr);
  obj0000.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  co_sub_t* const sub0000_0 = obj0000.GetLastSub();
  CHECK_EQUAL(0, co_sub_set_access(sub0000_0, CO_ACCESS_RW));
  co_sub_set_pdo_mapping(sub0000_0, 1);
  co_sub_set_dn_ind(sub0000_0, &co_sub_dn_ind_ok, nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj0000.Take()));

  const auto ret = co_pdo_dn(&par, dev, &req, buf, n);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(true, co_sub_dn_ind_called);
}

TEST_GROUP_BASE(CoPdo_CoPdoUp, CO_PdoBase) {
  co_pdo_map_par par = CO_PDO_MAP_PAR_INIT;
  co_unsigned8_t buf[12] = {0x00u};
  static bool sub_up_ind_called;
  static co_unsigned8_t reqbuf[];

  static co_unsigned32_t sub_ind_not_req_first(const co_sub_t*, co_sdo_req* req,
                                               co_unsigned32_t ac, void*) {
    if (ac) return ac;
    req->offset = 1u;
    return 0u;
  }
  static co_unsigned32_t sub_ind_not_req_last(const co_sub_t*, co_sdo_req* req,
                                              co_unsigned32_t ac, void*) {
    if (ac) return ac;
    req->offset = 1u;
    req->nbyte = 1u;
    req->size = 4u;
    return 0u;
  }
  static co_unsigned32_t sub_ind_req_last(const co_sub_t*, co_sdo_req* req,
                                          co_unsigned32_t ac, void*) {
    if (ac) return ac;
    req->offset = 0u;
    req->nbyte = 1u;
    req->size = 4u;
    return 0u;
  }
  static co_unsigned32_t sub_ind_req_error(const co_sub_t*, co_sdo_req*,
                                           co_unsigned32_t ac, void*) {
    if (ac) return ac;
    return CO_SDO_AC_ERROR;
  }
  static co_unsigned32_t sub_up_ind(const co_sub_t*, co_sdo_req* req,
                                    co_unsigned32_t ac, void*) {
    if (ac) return ac;
    sub_up_ind_called = true;
    req->buf = reqbuf;
    return 0u;
  }
};
bool TEST_GROUP_CppUTestGroupCoPdo_CoPdoUp::sub_up_ind_called = false;
co_unsigned8_t TEST_GROUP_CppUTestGroupCoPdo_CoPdoUp::reqbuf[12] = {
    0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu,
    0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu};

TEST(CoPdo_CoPdoUp, MappingExceedsMaxPDOSize) {
  par.n = 0x01u;
  par.map[0] = 0x000000feu;
  size_t n = 0u;

  const auto ret = co_pdo_up(&par, dev, &req, buf, &n);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
  CHECK_EQUAL(0u, n);
}

TEST(CoPdo_CoPdoUp, MapIsOneObjNotFound) {
  par.n = 0x01u;
  par.map[0] = 0x00000001u;
  size_t n = 0u;

  const auto ret = co_pdo_up(&par, dev, &req, buf, &n);

  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, ret);
  CHECK_EQUAL(0u, n);
}

TEST(CoPdo_CoPdoUp, NotReqFirst) {
  par.n = 0x01u;
  par.map[0] = 0x00000001u;
  size_t n = 0u;

  CoObjTHolder obj0000(0x0000u);
  CHECK(obj0000.Get() != nullptr);
  obj0000.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  co_sub_t* const sub = obj0000.GetLastSub();
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_RO));
  co_sub_set_pdo_mapping(sub, 1);
  co_sub_set_up_ind(sub, &sub_ind_not_req_first, nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj0000.Take()));

  const auto ret = co_pdo_up(&par, dev, &req, buf, &n);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
  CHECK_EQUAL(0u, n);
}

TEST(CoPdo_CoPdoUp, NotReqLast) {
  par.n = 0x01u;
  par.map[0] = 0x00000001u;
  size_t n = 0u;

  CoObjTHolder obj0000(0x0000u);
  CHECK(obj0000.Get() != nullptr);
  obj0000.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  co_sub_t* const sub = obj0000.GetLastSub();
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_RO));
  co_sub_set_pdo_mapping(sub, 1);
  co_sub_set_up_ind(sub, &sub_ind_not_req_last, nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj0000.Take()));

  const auto ret = co_pdo_up(&par, dev, &req, buf, &n);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
  CHECK_EQUAL(0u, n);
}

TEST(CoPdo_CoPdoUp, ReqLast) {
  par.n = 0x01u;
  par.map[0] = 0x00000001u;
  size_t n = 0u;

  CoObjTHolder obj0000(0x0000u);
  CHECK(obj0000.Get() != nullptr);
  obj0000.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  co_sub_t* const sub = obj0000.GetLastSub();
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_RO));
  co_sub_set_pdo_mapping(sub, 1);
  co_sub_set_up_ind(sub, &sub_ind_req_last, nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj0000.Take()));

  const auto ret = co_pdo_up(&par, dev, &req, buf, &n);

  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, ret);
  CHECK_EQUAL(0u, n);
}

TEST(CoPdo_CoPdoUp, IndError) {
  par.n = 0x01u;
  par.map[0] = 0x00000001u;
  size_t n = 0u;

  CoObjTHolder obj0000(0x0000u);
  CHECK(obj0000.Get() != nullptr);
  obj0000.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  co_sub_t* const sub = obj0000.GetLastSub();
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_RO));
  co_sub_set_pdo_mapping(sub, 1);
  co_sub_set_up_ind(sub, &sub_ind_req_error, nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj0000.Take()));

  const auto ret = co_pdo_up(&par, dev, &req, buf, &n);

  CHECK_EQUAL(CO_SDO_AC_ERROR, ret);
  CHECK_EQUAL(0u, n);
}

TEST(CoPdo_CoPdoUp, BufferNotNullButCapacityZero) {
  par.n = 0x01u;
  size_t n = 0u;

  const auto ret = co_pdo_up(&par, dev, &req, buf, &n);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(0u, n);
}

TEST(CoPdo_CoPdoUp, MapIsOneObjAndSubAsPDO) {
  par.n = 0x01u;
  par.map[0] = 0x00000001u;
  size_t n = 0u;

  CoObjTHolder obj0000(0x0000u);
  CHECK(obj0000.Get() != nullptr);
  obj0000.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  co_sub_t* const sub = obj0000.GetLastSub();
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_RO));
  co_sub_set_pdo_mapping(sub, 1);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj0000.Take()));

  const auto ret = co_pdo_up(&par, dev, &req, buf, &n);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(1u, n);
}

TEST(CoPdo_CoPdoUp, BufNull) {
  par.n = 0x01u;
  par.map[0] = 0x00000001u;
  size_t n = 0;

  CoObjTHolder obj0000(0x0000u);
  CHECK(obj0000.Get() != nullptr);
  obj0000.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  co_sub_t* const sub = obj0000.GetLastSub();
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_RO));
  co_sub_set_pdo_mapping(sub, 1);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj0000.Take()));

  const auto ret = co_pdo_up(&par, dev, &req, nullptr, &n);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(1u, n);
}

TEST(CoPdo_CoPdoUp, PnNull) {
  par.n = 0x01u;
  par.map[0] = 0x00000001u;

  CoObjTHolder obj0000(0x0000u);
  CHECK(obj0000.Get() != nullptr);
  obj0000.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  co_sub_t* const sub = obj0000.GetLastSub();
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_RO));
  co_sub_set_pdo_mapping(sub, 1);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj0000.Take()));

  const auto ret = co_pdo_up(&par, dev, &req, buf, nullptr);

  CHECK_EQUAL(0u, ret);
}

TEST(CoPdo_CoPdoUp, Nominal) {
  par.n = 0x02u;
  par.map[0] = 0x00000010u;
  par.map[1] = 0x00000010u;
  size_t n = 12u;

  CoObjTHolder obj0000(0x0000u);
  CHECK(obj0000.Get() != nullptr);
  obj0000.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x03u));
  co_sub_t* const sub = obj0000.GetLastSub();
  co_sub_set_pdo_mapping(sub, 1);
  obj0000.InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED16,
                          co_unsigned16_t(0xbbddu));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj0000.Take()));
  co_sub_set_up_ind(sub, &sub_up_ind, nullptr);

  const auto ret = co_pdo_up(&par, dev, &req, buf, &n);

  CHECK_EQUAL(0u, ret);
  CHECK_EQUAL(4u, n);
  CHECK(sub_up_ind_called);
  CHECK_EQUAL(0xffu, buf[0]);
  CHECK_EQUAL(0xffu, buf[1]);
  CHECK_EQUAL(0xffu, buf[2]);
  CHECK_EQUAL(0xffu, buf[3]);
  CHECK_EQUAL(0x00u, buf[4]);
  CHECK_EQUAL(0x00u, buf[5]);
}

// TODO(sdo): check if buffers have correct values after the download/upload
