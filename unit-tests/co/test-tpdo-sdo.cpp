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
#include <lely/co/tpdo.h>
#include <lely/co/sdo.h>

#include <libtest/allocators/default.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"

TEST_BASE(CO_SdoTpdoBase) {
  TEST_BASE_SUPER(CO_SdoTpdoBase);
  Allocators::Default allocator;

  const co_unsigned8_t DEV_ID = 0x01u;
  const co_unsigned16_t TPDO_NUM = 0x0001u;

  co_dev_t* dev = nullptr;
  can_net_t* net = nullptr;
  co_tpdo_t* tpdo = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1800;
  std::unique_ptr<CoObjTHolder> obj1a00;

  void SetPdoCommCobid(const co_unsigned32_t cobid) {
    co_sub_t* const sub_comm_cobid = co_dev_find_sub(dev, 0x1800u, 0x01u);
    CHECK(sub_comm_cobid != nullptr);
    co_sub_set_val_u32(sub_comm_cobid, cobid);
  }

  void RestartTPDO() {
    co_tpdo_stop(tpdo);
    co_tpdo_start(tpdo);
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(allocator.ToAllocT(), 0);
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    dev_holder->CreateAndInsertObj(obj1800, 0x1800u);
    dev_holder->CreateAndInsertObj(obj1a00, 0x1a00u);

    // 0x00 - highest sub-index supported
    obj1800->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t{0x02u});
    // 0x01 - COB-ID used by TPDO
    obj1800->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32,
                             co_unsigned32_t{DEV_ID});
    // 0x02 - transmission type
    obj1800->InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t{0xfeu});  // event-driven

    tpdo = co_tpdo_create(net, dev, TPDO_NUM);
    CHECK(tpdo != nullptr);

    CoCsdoDnCon::Clear();
  }

  TEST_TEARDOWN() {
    co_tpdo_destroy(tpdo);

    dev_holder.reset();
    can_net_destroy(net);
  }
};

TEST_GROUP_BASE(CO_SdoTpdo1800, CO_SdoTpdoBase) {
  int32_t clang_format_fix = 0;  // unused

  void Insert1800Values() {
    // adjust highest subindex supported
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1800u, 0x00u);
    CHECK(sub != nullptr);
    co_sub_set_val_u8(sub, 0x06u);

    // 0x03 - inhibit time
    obj1800->InsertAndSetSub(0x03u, CO_DEFTYPE_UNSIGNED16,
                             co_unsigned16_t{0x0000u});  // n*100 us
    // 0x04 - reserved (compatibility entry)
    obj1800->InsertAndSetSub(0x04u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t{0x00u});
    // 0x05 - event-timer
    obj1800->InsertAndSetSub(0x05u, CO_DEFTYPE_UNSIGNED16,
                             co_unsigned16_t{0x0000u});  // ms
    // 0x06 - sync value
    obj1800->InsertAndSetSub(0x06u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t{0x00u});
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    Insert1800Values();
    co_tpdo_start(tpdo);
  }

  TEST_TEARDOWN() {
    co_tpdo_stop(tpdo);

    TEST_BASE_TEARDOWN();
  }
};

/// \Given a pointer to a device (co_dev_t), the object dictionary
///        contains the TPDO Communication Parameter object (0x1800)
///
/// \When the download indication function for the object 0x1800 is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_SdoTpdo1800, Co1800DnInd_NonZeroAbortCode) {
  co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret = LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1800u, 0x00u,
                                                        CO_SDO_AC_ERROR);

  CHECK_EQUAL(ac, ret);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: CO_SDO_AC_TYPE_LEN_HI abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_TooLongData) {
  const co_unsigned16_t data = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x00u, CO_DEFTYPE_UNSIGNED16, &data,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_HI, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: CO_SDO_AC_NO_WRITE abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_HighestSubIdxSupported) {
  const co_unsigned8_t num_of_elems = 0x7fu;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_elems, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_NO_WRITE, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_CobidSameAsPrevious) {
  const co_unsigned32_t cobid = DEV_ID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: CO_SDO_AC_PARAM_VAL  abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_CobidValidToValid_NewCanId) {
  SetPdoCommCobid(DEV_ID);
  RestartTPDO();

  const co_unsigned32_t cobid = DEV_ID + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: invalid TPDO
// when: co_1800_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_CobidInvalidToValid_NewCanId) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartTPDO();

  const co_unsigned32_t cobid = DEV_ID + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_CobidValidToValid_FrameBit) {
  const co_unsigned32_t cobid = DEV_ID | CO_PDO_COBID_FRAME;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: COB-ID with frame bit and CO_PDO_COBID_VALID set is downloaded
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_CobidValidToInvalid_ExtendedId_NoFrameBit) {
  co_unsigned32_t cobid = DEV_ID | (1u << 28u) | CO_PDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: COB-ID with CO_PDO_COBID_VALID set is downloaded
// then: 0 abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_CobidValidToInvalid) {
  const co_unsigned32_t cobid = DEV_ID | CO_PDO_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_TransmissionTypeSameAsPrevious) {
  const co_unsigned8_t transmission_type = 0xfeu;
  const auto ret = co_dev_dn_val_req(dev, 0x1800u, 0x02u, CO_DEFTYPE_UNSIGNED8,
                                     &transmission_type, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: CO_SDO_AC_PARAM_VAL  abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_TransmissionTypeReserved) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_RTR);
  RestartTPDO();

  const co_unsigned8_t transmission_type = 0xf1u;
  const auto ret = co_dev_dn_val_req(dev, 0x1800u, 0x02u, CO_DEFTYPE_UNSIGNED8,
                                     &transmission_type, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: CO_SDO_AC_PARAM_VAL  abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_TransmissionType_SynchronousRTR_RTRBitSet) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_RTR);
  RestartTPDO();

  const co_unsigned8_t transmission_type = 0xfcu;
  const auto ret = co_dev_dn_val_req(dev, 0x1800u, 0x02u, CO_DEFTYPE_UNSIGNED8,
                                     &transmission_type, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: CO_SDO_AC_PARAM_VAL  abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_TransmissionType_EventDrivenRTR_RTRBitSet) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_RTR);
  RestartTPDO();

  const co_unsigned8_t transmission_type = 0xfdu;
  const auto ret = co_dev_dn_val_req(dev, 0x1800u, 0x02u, CO_DEFTYPE_UNSIGNED8,
                                     &transmission_type, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: CO_SDO_AC_PARAM_VAL  abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_TransmissionType_RTROnly_RTRBitNotSet) {
  const co_unsigned8_t transmission_type = 0xfdu;
  const auto ret = co_dev_dn_val_req(dev, 0x1800u, 0x02u, CO_DEFTYPE_UNSIGNED8,
                                     &transmission_type, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_TransmissionTypeMax) {
  const co_unsigned8_t transmission_type = 0xffu;
  const auto ret = co_dev_dn_val_req(dev, 0x1800u, 0x02u, CO_DEFTYPE_UNSIGNED8,
                                     &transmission_type, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_TransmissionType) {
  const co_unsigned8_t transmission_type = 0x35u;
  const auto ret = co_dev_dn_val_req(dev, 0x1800u, 0x02u, CO_DEFTYPE_UNSIGNED8,
                                     &transmission_type, nullptr,
                                     CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: invalid TPDO
// when: co_1800_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_InhibitTimeSameAsPrevious) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartTPDO();

  const co_unsigned16_t inhibit_time = 0x0000u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x03u, CO_DEFTYPE_UNSIGNED16,
                        &inhibit_time, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_InhibitTimeValidTPDO) {
  const co_unsigned16_t inhibit_time = 0x0001u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x03u, CO_DEFTYPE_UNSIGNED16,
                        &inhibit_time, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: invalid TPDO
// when: co_1800_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_InhibitTime) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartTPDO();

  const co_unsigned16_t inhibit_time = 0x0003u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x03u, CO_DEFTYPE_UNSIGNED16,
                        &inhibit_time, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: CO_SDO_AC_NO_SUB abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_CompatibilityEntry) {
  const co_unsigned8_t compat = 0x44u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x04u, CO_DEFTYPE_UNSIGNED8, &compat,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_NO_SUB, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_EventTimerSameAsPrevious) {
  const co_unsigned16_t event_timer = 0x0000u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x05u, CO_DEFTYPE_UNSIGNED16,
                        &event_timer, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_EventTimer) {
  const co_unsigned16_t event_timer = 0x3456u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x05u, CO_DEFTYPE_UNSIGNED16,
                        &event_timer, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_SyncSameAsPrevious) {
  const co_unsigned8_t sync = 0x00u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x06u, CO_DEFTYPE_UNSIGNED8, &sync,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1800_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_SyncNewValue_TPDOValid) {
  const co_unsigned8_t sync = 0x01u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x06u, CO_DEFTYPE_UNSIGNED8, &sync,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: invalid TPDO
// when: co_1800_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1800, Co1800DnInd_SyncNewValue_TPDOInvalid) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartTPDO();

  const co_unsigned8_t sync = 0x01u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1800u, 0x06u, CO_DEFTYPE_UNSIGNED8, &sync,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

TEST_GROUP_BASE(CO_SdoTpdo1a00, CO_SdoTpdoBase) {
  std::unique_ptr<CoObjTHolder> obj2021;

  void Insert1a00Values() {
    // 0x00 - number of mapped application objects in PDO
    obj1a00->InsertAndSetSub(0x00, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t{CO_PDO_NUM_MAPS});
    // 0x01-0x40 - application objects
    for (co_unsigned8_t i = 0x01u; i <= CO_PDO_NUM_MAPS; ++i) {
      obj1a00->InsertAndSetSub(i, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t{0});
    }
  }

  void Set1a00Sub1Mapping(co_unsigned32_t mapping) {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1a00u, 0x01u);
    co_sub_set_val_u32(sub, mapping);
  }

  void Insert2021Values() {
    assert(obj2021->Get());

    obj2021->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32,
                             co_unsigned32_t{0xdeadbeefu});
    co_sub_t* sub2021 = obj2021->GetLastSub();
    co_sub_set_access(sub2021, CO_ACCESS_RW);
    co_sub_set_pdo_mapping(sub2021, 1);
  }

  void SetNumOfMappings(co_unsigned8_t mappings_num) {
    co_sub_t* const sub_map_n = co_dev_find_sub(dev, 0x1a00u, 0x00u);
    co_sub_set_val_u8(sub_map_n, mappings_num);
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    Insert1a00Values();
    co_tpdo_start(tpdo);

    CoCsdoDnCon::Clear();
  }

  TEST_TEARDOWN() {
    co_tpdo_stop(tpdo);

    TEST_BASE_TEARDOWN();
  }
};

/// \Given a pointer to a device (co_dev_t), the object dictionary
///        contains the TPDO Mapping Parameter object (0x1a00)
///
/// \When the download indication function for the object 0x1a00 is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_SdoTpdo1a00, Co1a00DnInd_NonZeroAbortCode) {
  co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret = LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1a00u, 0x00u,
                                                        CO_SDO_AC_ERROR);

  CHECK_EQUAL(ac, ret);
}

// given: invalid TPDO
// when: co_1a00_dn_ind()
// then: CO_SDO_AC_PDO_LEN  abort code is returned
TEST(CO_SdoTpdo1a00, Co1a00DnInd_NumOfMappingsLenGreaterThanMax) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  Set1a00Sub1Mapping(0x202100ffu);
  RestartTPDO();
  // object which could be mapped
  dev_holder->CreateAndInsertObj(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned8_t num_of_mappings = 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PDO_LEN, CoCsdoDnCon::ac);
}

// given: invalid TPDO
// when: co_1a00_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoTpdo1a00, Co1a00DnInd_EmptyMapping) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  Set1a00Sub1Mapping(0x00000000u);
  RestartTPDO();
  // object which could be mapped
  dev_holder->CreateAndInsertObj(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned8_t num_of_mappings = 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: too long value is downloaded
// then: CO_SDO_AC_TYPE_LEN_HI abort code is returned
TEST(CO_SdoTpdo1a00, Co1a00DnInd_NumOfMappingsRequestFailed) {
  const co_unsigned32_t data = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED32, &data,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN_HI, CoCsdoDnCon::ac);
}

// given: invalid TPDO
// when: co_1a00_dn_ind()
// then: CO_SDO_AC_NO_OBJ abort code is returned
TEST(CO_SdoTpdo1a00, Co1a00DnInd_NumOfMappingsNonExistingObjMapping) {
  Set1a00Sub1Mapping(0xffff0000u);
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartTPDO();

  const co_unsigned8_t num_of_mappings = 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, CoCsdoDnCon::ac);
}

// given: invalid TPDO
// when: co_1a00_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1a00, Co1a00DnInd_NumOfMappingsSameAsPrevious) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartTPDO();

  const co_unsigned8_t num_of_mappings = CO_PDO_NUM_MAPS;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1a00_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoTpdo1a00, Co1a00DnInd_NumOfMappingsValidTPDO) {
  const co_unsigned8_t num_of_mappings = 2u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: invalid TPDO
// when: co_1a00_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoTpdo1a00, Co1a00DnInd_NumOfMappingsTooManyObjsToMap) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  RestartTPDO();

  const co_unsigned8_t num_of_mappings = CO_PDO_NUM_MAPS + 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1a00_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1a00, Co1a00DnInd_NumOfMappingsNoMappings) {
  const co_unsigned8_t num_of_mappings = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: invalid TPDO
// when: co_1a00_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1a00, Co1a00DnInd_NumOfMappings) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  Set1a00Sub1Mapping(0x20210020u);
  RestartTPDO();
  // object which could be mapped
  dev_holder->CreateAndInsertObj(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned8_t num_of_mappings = 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                        &num_of_mappings, nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: invalid TPDO
// when: co_1a00_dn_ind()
// then: CO_SDO_AC_NO_OBJ abort code is returned
TEST(CO_SdoTpdo1a00, Co1a00DnInd_MappingNonexisting) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  SetNumOfMappings(0x00);
  RestartTPDO();

  const co_unsigned32_t mapping = 0xffff0000u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_NO_OBJ, CoCsdoDnCon::ac);
}

// given: invalid TPDO
// when: co_1a00_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1a00, Co1a00DnInd_MappingSameAsPrevious) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  Set1a00Sub1Mapping(0x20210020u);
  RestartTPDO();
  // object which could be mapped
  dev_holder->CreateAndInsertObj(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned32_t mapping = 0x20210020u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: invalid TPDO
// when: co_1a00_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoTpdo1a00, Co1a00DnInd_MappingNumOfMappingsNonzero) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  SetNumOfMappings(0x01u);
  RestartTPDO();
  // object which could be mapped
  dev_holder->CreateAndInsertObj(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned32_t mapping = 0x20210020u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: valid TPDO
// when: co_1a00_dn_ind()
// then: CO_SDO_AC_PARAM_VAL abort code is returned
TEST(CO_SdoTpdo1a00, Co1a00DnInd_MappingValidTPDO) {
  SetPdoCommCobid(DEV_ID);
  SetNumOfMappings(0x01u);
  RestartTPDO();
  // object which could be mapped
  dev_holder->CreateAndInsertObj(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned32_t mapping = 0x20210020u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(CO_SDO_AC_PARAM_VAL, CoCsdoDnCon::ac);
}

// given: invalid TPDO
// when: co_1a00_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1a00, Co1a00DnInd_Mapping) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  SetNumOfMappings(0x00);
  RestartTPDO();
  // object which could be mapped
  dev_holder->CreateAndInsertObj(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned32_t mapping = 0x20210020u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x01u, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}

// given: invalid TPDO
// when: co_1a00_dn_ind()
// then: 0 abort code is returned
TEST(CO_SdoTpdo1a00, Co1a00DnInd_MappingZeros) {
  SetPdoCommCobid(DEV_ID | CO_PDO_COBID_VALID);
  SetNumOfMappings(0x00);
  Set1a00Sub1Mapping(0x20210020u);
  RestartTPDO();
  // object which could be mapped
  dev_holder->CreateAndInsertObj(obj2021, 0x2021u);
  Insert2021Values();

  const co_unsigned32_t mapping = 0x00000000u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1a00u, 0x01, CO_DEFTYPE_UNSIGNED32, &mapping,
                        nullptr, CoCsdoDnCon::Func, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK(CoCsdoDnCon::Called());
  CHECK_EQUAL(0, CoCsdoDnCon::ac);
}
