/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2020 N7 Space Sp. z o.o.
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

#include <lely/co/csdo.h>
#include <lely/co/rpdo.h>
#include <lely/co/sdo.h>

#include "holder/dev.hpp"
#include "holder/obj.hpp"

#include "allocators/heap.hpp"

#include "lely-cpputest-ext.hpp"
#include "lely-unit-test.hpp"
#include "override/lelyco-val.hpp"

TEST_BASE(CO_SdoRpdoBase) {
  TEST_BASE_SUPER(CO_SdoRpdoBase);
  Allocators::HeapAllocator allocator;

  const co_unsigned8_t CO_PDO_MAP_MAX_SUBIDX = 0x40u;
  const co_unsigned8_t DEV_ID = 0x01u;
  const co_unsigned16_t RPDO_NUM = 0x0001u;

  co_dev_t* dev = nullptr;
  can_net_t* net = nullptr;
  co_rpdo_t* rpdo = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1400;
  std::unique_ptr<CoObjTHolder> obj1600;

  void CreateObjInDev(std::unique_ptr<CoObjTHolder> & obj_holder,
                      co_unsigned16_t idx) {
    obj_holder.reset(new CoObjTHolder(idx));
    CHECK(obj_holder->Get() != nullptr);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));
  }

  void SetPdoCommCobid(const co_unsigned32_t cobid) {
    co_sub_t* const sub_comm_cobid = co_dev_find_sub(dev, 0x1400u, 0x01u);
    CHECK(sub_comm_cobid != nullptr);
    co_sub_set_val_u32(sub_comm_cobid, cobid);
  }

  void RestartRPDO() { co_rpdo_start(rpdo); }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(allocator.ToAllocT());
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    CreateObjInDev(obj1400, 0x1400u);
    CreateObjInDev(obj1600, 0x1600u);

    // 0x00 - highest sub-index supported
    obj1400->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t(0x02u));
    // 0x01 - COB-ID used by RPDO
    obj1400->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                             co_unsigned32_t(DEV_ID));
    // 0x02 - transmission type
    obj1400->InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t(0xfeu));  // event-driven

    rpdo = co_rpdo_create(net, dev, RPDO_NUM);
    CHECK(rpdo != nullptr);

    CoCsdoDnCon::Clear();
  }

  TEST_TEARDOWN() {
    co_rpdo_destroy(rpdo);

    dev_holder.reset();
    can_net_destroy(net);
  }
};

TEST_GROUP_BASE(CO_SdoRpdo1400, CO_SdoRpdoBase) {
  int clang_format_fix = 0;  // unused

  void Insert1400Values() {
    // adjust highest subindex supported
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1400u, 0x00u);
    CHECK(sub != nullptr);
    co_sub_set_val_u8(sub, 0x05u);

    // 0x03 - inhibit time
    obj1400->InsertAndSetSub(0x03u, CO_DEFTYPE_UNSIGNED16,
                             co_unsigned16_t(0x0000u));  // n*100 us
    // 0x04 - reserved (compatibility entry)
    obj1400->InsertAndSetSub(0x04u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t(0x00u));
    // 0x05 - event-timer
    obj1400->InsertAndSetSub(0x05u, CO_DEFTYPE_UNSIGNED16,
                             co_unsigned16_t(0x0000u));  // ms
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    Insert1400Values();
    co_rpdo_start(rpdo);
  }

  TEST_TEARDOWN() {
    co_rpdo_stop(rpdo);

    TEST_BASE_TEARDOWN();
  }
};

#if HAVE_LELY_OVERRIDE
// given: valid RPDO
// when: co_1400_dn_ind(), co_val_read() fails
// then: CO_SDO_AC_TYPE_LEN_LO abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_CoValReadZero) {
  LelyOverride::co_val_read(0);

  const int data = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x00u, CO_DEFTYPE_UNSIGNED8, &data,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_LO, CoCsdoDnCon::ac);
}
#endif  // HAVE_LELY_OVERRIDE

// given: valid RPDO
// when: co_1400_dn_ind()
// then: CO_SDO_AC_NO_WRITE abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_DownloadNumOfElements) {
  const co_unsigned8_t num_of_elems = 0x7fu;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_elems, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_NO_WRITE, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_CobidSameAsPrevious) {
  const co_unsigned32_t cobid = DEV_ID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: CO_SDO_AC_PARAM_VAL  abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_CobidValidToValid_NewCanId) {
  SetPdoCommCobid(DEV_ID);
  RestartRPDO();

  const co_unsigned32_t cobid = DEV_ID + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_CobidInvalidToValid_NewCanId) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartRPDO();

  const co_unsigned32_t cobid = DEV_ID + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_CobidValidToValid_FrameBit) {
  const co_unsigned32_t cobid = DEV_ID | CO_PDO_COBID_FRAME;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: COB-ID with frame bit and CO_PDO_COBID_VALID set is downloaded
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_CobidValidToInvalid_ExtendedId_NoFrameBit) {
  co_unsigned32_t cobid = DEV_ID | (1u << 28u) | CO_PDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: COB-ID with CO_PDO_COBID_VALID set is downloaded
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_CobidValidToInvalid) {
  const co_unsigned32_t cobid = DEV_ID | CO_PDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_TransmissionTypeSameAsPrevious) {
  const co_unsigned8_t transmission_type = 0xfeu;
  const auto ret = co_dev_dn_val_req(dev, 0x1400u, 0x02u, CO_DEFTYPE_UNSIGNED8,
                                     &transmission_type, nullptr,
                                     CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: CO_SDO_AC_PARAM_VAL  abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_TransmissionTypeReserved) {
  for (co_unsigned8_t i = 0xf1u; i < 0xfeu; i++) {
    CoCsdoDnCon::Clear();

    const co_unsigned8_t transmission_type = i;
    const auto ret = co_dev_dn_val_req(dev, 0x1400u, 0x02u,
                                       CO_DEFTYPE_UNSIGNED8, &transmission_type,
                                       nullptr, CoCsdoDnCon::func, nullptr);

    CHECK_EQUAL(0, ret);
    CHECK(CoCsdoDnCon::called);
    CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
  }
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_TransmissionTypeMax) {
  const co_unsigned8_t transmission_type = 0xffu;
  const auto ret = co_dev_dn_val_req(dev, 0x1400u, 0x02u, CO_DEFTYPE_UNSIGNED8,
                                     &transmission_type, nullptr,
                                     CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_TransmissionType) {
  const co_unsigned8_t transmission_type = 0x35u;
  const auto ret = co_dev_dn_val_req(dev, 0x1400u, 0x02u, CO_DEFTYPE_UNSIGNED8,
                                     &transmission_type, nullptr,
                                     CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_InhibitTimeSameAsPrevious) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartRPDO();

  const co_unsigned16_t inhibit_time = 0x0000u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x03u, CO_DEFTYPE_UNSIGNED16,
                        &inhibit_time, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_InhibitTimeValidRPDO) {
  const co_unsigned16_t inhibit_time = 0x0001u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x03u, CO_DEFTYPE_UNSIGNED16,
                        &inhibit_time, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_InhibitTime) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartRPDO();

  const co_unsigned16_t inhibit_time = 0x0003u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x03u, CO_DEFTYPE_UNSIGNED16,
                        &inhibit_time, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: CO_SDO_AC_NO_SUB abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_CompatibilityEntry) {
  const co_unsigned8_t compat = 0x44u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x04u, CO_DEFTYPE_UNSIGNED8, &compat,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_NO_SUB, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_EventTimerSameAsPrevious) {
  const co_unsigned16_t event_timer = 0x0000u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x05u, CO_DEFTYPE_UNSIGNED16,
                        &event_timer, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1400_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1400, Co1400DnInd_EventTimer) {
  const co_unsigned16_t event_timer = 0x3456u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1400u, 0x05u, CO_DEFTYPE_UNSIGNED16,
                        &event_timer, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

TEST_GROUP_BASE(CO_SdoRpdo1600, CO_SdoRpdoBase) {
  std::unique_ptr<CoObjTHolder> obj2021;

  void Insert1600Values() {
    // 0x00 - number of mapped application objects in PDO
    obj1600->InsertAndSetSub(0x00, CO_DEFTYPE_UNSIGNED8, CO_PDO_MAP_MAX_SUBIDX);
    // 0x01-0x40 - application objects
    for (co_unsigned8_t i = 0x01u; i <= CO_PDO_MAP_MAX_SUBIDX; ++i) {
      obj1600->InsertAndSetSub(i, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t(0));
    }
  }

  void Set1600Sub1Mapping(co_unsigned32_t mapping) {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1600u, 0x01u);
    co_sub_set_val_u32(sub, mapping);
  }

  void Insert2021Values() {
    assert(obj2021->Get());

    obj2021->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32,
                             co_unsigned32_t(0xdeadbeefu));
    co_sub_t* sub2021 = obj2021->GetLastSub();
    co_sub_set_access(sub2021, CO_ACCESS_RW);
    co_sub_set_pdo_mapping(sub2021, 1);
  }

  void SetNumOfMappings(co_unsigned8_t mappings_num) {
    co_sub_t* const sub_map_n = co_dev_find_sub(dev, 0x1600u, 0x00u);
    co_sub_set_val_u8(sub_map_n, mappings_num);
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    Insert1600Values();
    co_rpdo_start(rpdo);

    CoCsdoDnCon::Clear();
  }

  TEST_TEARDOWN() {
    co_rpdo_stop(rpdo);

    TEST_BASE_TEARDOWN();
  }
};

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: CO_SDO_AC_PDO_LEN  abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_NumOfMappingsLenGreaterThanMax) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  Set1600Sub1Mapping(0x202100ffu);
  RestartRPDO();
  // object which could be mapped
  CreateObjInDev(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned8_t num_of_mappings = 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_EmptyMapping) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  Set1600Sub1Mapping(0x00000000u);
  RestartRPDO();
  // object which could be mapped
  CreateObjInDev(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned8_t num_of_mappings = 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: too long value is downloaded
// then: CO_SDO_AC_TYPE_LEN_HI abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_NumOfMappingsRequestFailed) {
  const co_unsigned32_t data = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED32, &data,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_HI, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: CO_SDO_AC_NO_OBJ abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_NumOfMappingsNonExistingObjMapping) {
  Set1600Sub1Mapping(0xffff0000u);
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartRPDO();

  const co_unsigned8_t num_of_mappings = 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_NumOfMappingsSameAsPrevious) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartRPDO();

  const co_unsigned8_t num_of_mappings = CO_PDO_MAP_MAX_SUBIDX;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1600_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_NumOfMappingsButValidBitNotSet) {
  const co_unsigned8_t num_of_mappings = 2u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_NumOfMappingsTooManyObjsToMap) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartRPDO();

  const co_unsigned8_t num_of_mappings = CO_PDO_MAP_MAX_SUBIDX + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1600_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_NumOfMappingsNoMappings) {
  const co_unsigned8_t num_of_mappings = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_NumOfMappings) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  Set1600Sub1Mapping(0x20210020u);
  RestartRPDO();
  // object which could be mapped
  CreateObjInDev(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned8_t num_of_mappings = 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: CO_SDO_AC_NO_OBJ abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_MappingNonexisting) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  SetNumOfMappings(0x00);
  RestartRPDO();

  const co_unsigned32_t mapping = 0xffff0000u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_MappingSameAsPrevious) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  Set1600Sub1Mapping(0x20210020u);
  RestartRPDO();
  // object which could be mapped
  CreateObjInDev(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned32_t mapping = 0x20210020u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_MappingNumOfMappingsNonzero) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  SetNumOfMappings(0x01u);
  RestartRPDO();
  // object which could be mapped
  CreateObjInDev(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned32_t mapping = 0x20210110u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: valid RPDO
// when: co_1600_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_MappingValidBitNotSet) {
  SetPdoCommCobid(DEV_ID);
  SetNumOfMappings(0x01u);
  RestartRPDO();
  // object which could be mapped
  CreateObjInDev(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned32_t mapping = 0x20210020u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_Mapping) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  SetNumOfMappings(0x00);
  RestartRPDO();
  // object which could be mapped
  CreateObjInDev(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned32_t mapping = 0x20210020u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600u, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: invalid RPDO
// when: co_1600_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoRpdo1600, Co1600DnInd_MappingZeroes) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  SetNumOfMappings(0x00);
  Set1600Sub1Mapping(0x20210020u);
  RestartRPDO();
  // object which could be mapped
  CreateObjInDev(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned32_t mapping = 0x00000000u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1600, 0x01, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::called);
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}
