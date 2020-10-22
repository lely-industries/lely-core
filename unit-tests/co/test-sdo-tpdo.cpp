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

#include <lely/co/tpdo.h>
#include <lely/co/sdo.h>

#include "holder/dev.hpp"
#include "holder/obj.hpp"

#include "allocators/heap.hpp"

#include "lely-cpputest-ext.hpp"
#include "lely-unit-test.hpp"

TEST_BASE(CO_SdoTpdoBase) {
  TEST_BASE_SUPER(CO_SdoTpdoBase);
  Allocators::HeapAllocator allocator;

  const co_unsigned8_t DEV_ID = 0x01u;
  const co_unsigned16_t TPDO_NUM = 0x0001u;

  can_net_t* net = nullptr;
  co_dev_t* dev = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1800;
  std::unique_ptr<CoObjTHolder> obj1A00;

  co_tpdo_t* tpdo = nullptr;

  void CreateObjInDev(std::unique_ptr<CoObjTHolder> & obj_holder,
                      co_unsigned16_t idx) {
    obj_holder.reset(new CoObjTHolder(idx));
    CHECK(obj_holder->Get() != nullptr);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));
  }

  void InsertPdoCommCobid() {
    // 0x00 - highest sub-index supported
    obj1800->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t(0x01u));
    // 0x01 - COB-ID used by TPDO
    obj1800->InsertAndSetSub(0x00000001u, CO_DEFTYPE_UNSIGNED32,
                             co_unsigned32_t(DEV_ID));
  }

  void SetPdoCommCobid(const co_unsigned32_t cobid) {
    co_sub_t* const sub_comm_cobid = co_dev_find_sub(dev, 0x1800u, 0x01u);
    CHECK(sub_comm_cobid != nullptr);
    co_sub_set_val_u32(sub_comm_cobid, cobid);

    CHECK(tpdo != nullptr);
    co_tpdo_stop(tpdo);
    co_tpdo_start(tpdo);
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(allocator.ToAllocT());
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    CreateObjInDev(obj1A00, 0x1A00u);
    CreateObjInDev(obj1800, 0x1800u);
    InsertPdoCommCobid();

    tpdo = co_tpdo_create(net, dev, TPDO_NUM);
    CHECK(tpdo != nullptr);
  }

  TEST_TEARDOWN() {
    co_tpdo_destroy(tpdo);

    dev_holder.reset();
    can_net_destroy(net);
  }
};

TEST_GROUP_BASE(CO_SdoTpdo1800, CO_SdoTpdoBase) {
  union buffer_t {
    co_unsigned8_t num_of_elems;
    co_unsigned32_t cobid;
    co_unsigned8_t transmission_type;
    co_unsigned16_t inhibit_time;
    co_unsigned8_t compatibility_entry;
    co_unsigned16_t event_timer;
    co_unsigned32_t sync_window_length;
  };

  buffer_t buffer;
  char* buffer_num_of_elems = reinterpret_cast<char*>(&buffer.num_of_elems);
  char* buffer_cobid = reinterpret_cast<char*>(&buffer.cobid);
  char* buffer_trans_type = reinterpret_cast<char*>(&buffer.transmission_type);
  char* buffer_inhibit_time = reinterpret_cast<char*>(&buffer.inhibit_time);
  char* buffer_compat = reinterpret_cast<char*>(&buffer.compatibility_entry);
  char* buffer_event_timer = reinterpret_cast<char*>(&buffer.event_timer);
  char* buffer_sync_len = reinterpret_cast<char*>(&buffer.sync_window_length);

  void Insert1800Defaults() {
    co_sub_t* const sub = co_dev_find_sub(dev, 0x1800u, 0x00u);
    CHECK(sub != nullptr);
    co_sub_set_val_u8(sub, 0x05u);

    // 0x02 - transmission type
    obj1800->InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t(0xFEu));  // event-driven
    // 0x03 - inhibit time
    obj1800->InsertAndSetSub(0x03u, CO_DEFTYPE_UNSIGNED16,
                             co_unsigned16_t(0x0000u));  // n*100 us
    // 0x04 - reserved (compatibility entry)
    obj1800->InsertAndSetSub(0x04u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t(0x00u));
    // 0x05 - event-timer
    obj1800->InsertAndSetSub(0x05u, CO_DEFTYPE_UNSIGNED16,
                             co_unsigned16_t(0x0001u));  // ms
  }

  void Check1800Dn(const co_unsigned8_t subidx, const size_t size,
                   const co_unsigned32_t ac_sub_dn_ind, const int rc_req_dn,
                   char* buffer) {
    const size_t bufsize = buffer ? size : 0u;
    size_t nbyte = buffer ? size : 0u;

    co_sub_t* const sub = co_dev_find_sub(dev, 0x1800u, subidx);
    CHECK(sub != nullptr);

    co_sub_dn_ind_t* dn_ind = nullptr;
    co_sub_get_dn_ind(sub, &dn_ind, nullptr);
    CHECK(dn_ind != nullptr);

    membuf buf = {buffer, buffer, buffer + bufsize};
    co_sdo_req req = CO_SDO_REQ_INIT(req);
    co_sdo_req_init(&req, &buf);
    req.size = size;
    req.membuf = &buf;
    req.buf = buffer;
    req.nbyte = nbyte;

    const auto ret_req_dn = co_sdo_req_dn(&req, nullptr, &nbyte, nullptr);

    const auto ret_sub_dn_ind = co_sub_dn_ind(sub, &req);

    CHECK_EQUAL(rc_req_dn, ret_req_dn);
    CHECK_EQUAL(ac_sub_dn_ind, ret_sub_dn_ind);
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    Insert1800Defaults();
    SetPdoCommCobid(0x000000FFu | CO_PDO_COBID_VALID);
  }

  TEST_TEARDOWN() { TEST_BASE_TEARDOWN(); }
};

TEST(CO_SdoTpdo1800, Co1800DnInd_DownloadNumOfElements) {
  buffer.num_of_elems = 0x7Fu;
#if LELY_NO_MALLOC
  Check1800Dn(0x00u, 1u, CO_SDO_AC_NO_MEM, -1, nullptr);
  Check1800Dn(0x00u, 1u, CO_SDO_AC_NO_WRITE, 0, buffer_num_of_elems);
#else
  Check1800Dn(0x00u, 1u, 0u, -1, nullptr);
  Check1800Dn(0x00u, 1u, CO_SDO_AC_NO_WRITE, 0, buffer_num_of_elems);
#endif
}

TEST(CO_SdoTpdo1800, Co1800DnInd_CobidSameAsPrevious) {
  buffer.cobid = 0x000000FFu | CO_PDO_COBID_VALID;
  Check1800Dn(0x01u, 4u, 0u, 0, buffer_cobid);
}

TEST(CO_SdoTpdo1800, Co1800DnInd_CobidOldValidNotSetNewValidNotSetNewCANID) {
  SetPdoCommCobid(0x00000003u);

  buffer.cobid = 0x00000002u;
  Check1800Dn(0x01u, 4u, CO_SDO_AC_PARAM_VAL, 0, buffer_cobid);
}

TEST(CO_SdoTpdo1800, Co1800DnInd_CobidOldValidSetNewValidNotSetNewCANID) {
  SetPdoCommCobid(0x00000003u | CO_PDO_COBID_VALID);

  buffer.cobid = 0x00000002u;
  Check1800Dn(0x01u, 4u, 0u, 0, buffer_cobid);
}

TEST(CO_SdoTpdo1800, Co1800DnInd_CobidOldValidNotSetNewValidNotSetSameCANID) {
  SetPdoCommCobid(0x00000003u);
  buffer.cobid = 0x00000003u | CO_PDO_COBID_FRAME;

  Check1800Dn(0x01u, 4u, 0u, 0, buffer_cobid);
}

TEST(CO_SdoTpdo1800, Co1800DnInd_CobidOldValidSetNewNotSetNewCANID) {
  // CO_PDO_COBID frame not set
  SetPdoCommCobid(0x00000004u | CO_PDO_COBID_VALID);
  buffer.cobid = 0x00000004u;
  buffer.cobid |= CO_PDO_COBID_FRAME;
  Check1800Dn(0x01u, 4u, 0u, 0, buffer_cobid);
}

TEST(CO_SdoTpdo1800, Co1800DnInd_CobidFrameBitSet) {
  buffer.cobid = (0x000000FFu | CO_PDO_COBID_VALID) | 0x10000000u;
  Check1800Dn(0x01u, 4u, CO_SDO_AC_PARAM_VAL, 0, buffer_cobid);
}

TEST(CO_SdoTpdo1800, Co1800DnInd_CobidFrameBitNotSet) {
  buffer.cobid = 0x000000FFu | CO_PDO_COBID_VALID;
  Check1800Dn(0x01u, 4u, 0u, 0, buffer_cobid);
}

TEST(CO_SdoTpdo1800, Co1800DnInd_TransmissionTypeSameAsPrevious) {
  buffer.transmission_type = 0xFEu;
  Check1800Dn(0x02u, 1u, 0u, 0, buffer_trans_type);
}

// TEST(CO_SdoTpdo1800, Co1800DnInd_TransmissionTypeReserved) {
//   for (co_unsigned8_t i = (0xF1u + 1u); i < 0xFEu; i++) {
//     buffer.transmission_type = i;
//     Check1800Dn(0x02u, 1u, CO_SDO_AC_PARAM_VAL, 0, buffer_trans_type);
//   }
// }

TEST(CO_SdoTpdo1800, Co1800DnInd_TransmissionTypeMax) {
  buffer.transmission_type = 0xFFu;
  Check1800Dn(0x02u, 1u, 0u, 0, buffer_trans_type);
}

TEST(CO_SdoTpdo1800, Co1800DnInd_TransmissionType) {
  buffer.transmission_type = 0x35u;
  Check1800Dn(0x02u, 1u, 0u, 0, buffer_trans_type);
}

TEST(CO_SdoTpdo1800, Co1800DnInd_InhibitTimeInvalidCobid) {
  SetPdoCommCobid(0x000000FFu);

  buffer.inhibit_time = 0x0003u;
  Check1800Dn(0x03u, 2u, CO_SDO_AC_PARAM_VAL, 0, buffer_inhibit_time);
}

TEST(CO_SdoTpdo1800, Co1800DnInd_InhibitTimeSameAsPrevious) {
  Check1800Dn(0x03u, 2u, 0u, 0, buffer_inhibit_time);
}

TEST(CO_SdoTpdo1800, Co1800DnInd_InhibitTime) {
  buffer.inhibit_time = 0x0001u;
  Check1800Dn(0x03u, 2u, 0u, 0, buffer_inhibit_time);
}

TEST(CO_SdoTpdo1800, Co1800DnInd_CompatibilityEntry) {
  Check1800Dn(0x04u, 1u, CO_SDO_AC_NO_SUB, 0, buffer_compat);
}

TEST(CO_SdoTpdo1800, Co1800DnInd_EventTimerSameAsPrevious) {
  buffer.event_timer = 0x0001u;
  Check1800Dn(0x05u, 2u, 0u, 0, buffer_event_timer);
}

TEST(CO_SdoTpdo1800, Co1800DnInd_EventTimer) {
  buffer.event_timer = 0x3456u;
  Check1800Dn(0x05u, 2u, 0u, 0, buffer_event_timer);
}

TEST_GROUP_BASE(CO_SdoTpdo1A00, CO_SdoTpdoBase) {
  union buffer_t {
    co_unsigned8_t num_of_elems;
    co_unsigned32_t mapping;
  };

  buffer_t buffer;

  char* buffer_num_of_elems = reinterpret_cast<char*>(&buffer.num_of_elems);
  char* buffer_mapping = reinterpret_cast<char*>(&buffer.mapping);

  std::unique_ptr<CoObjTHolder> obj2021;

  void Insert1A00Defaults() {
    // 0x00 - number of mapped application objects in PDO
    obj1A00->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t(0x40u));
    // 0x01-0x40 - application objects
    for (co_unsigned8_t i = 0x01u; i <= 0x40u; ++i) {
      obj1A00->InsertAndSetSub(i, CO_DEFTYPE_UNSIGNED32,
                               co_unsigned32_t(i - 1));
    }
  }

  void Insert2021Defaults() {
    assert(obj2021->Get());

    obj2021->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t(0x3Fu));
    for (co_unsigned8_t i = 0x01u; i < 0x40u; i++) {
      obj2021->InsertAndSetSub(i, CO_DEFTYPE_UNSIGNED16,
                               co_unsigned16_t(0x2345u));
      co_sub_t* sub = obj2021->GetLastSub();
      co_sub_set_access(sub, CO_ACCESS_RW);
      co_sub_set_pdo_mapping(sub, 1);
    }
  }

  void SetNumOfMappings(co_unsigned8_t mappings_num) {
    co_sub_t* const sub_map_n = co_dev_find_sub(dev, 0x1A00u, 0x00u);
    co_sub_set_val_u8(sub_map_n, mappings_num);
    co_tpdo_stop(tpdo);
    co_tpdo_start(tpdo);
  }

  void Check1A00Dn(const co_unsigned8_t subidx, const size_t size,
                   const co_unsigned32_t ac_sub_dn_ind, const int rc_req_dn,
                   char* buffer) {
    const size_t bufsize = buffer ? size : 0u;
    size_t nbyte = buffer ? size : 0u;

    co_sub_t* sub = co_dev_find_sub(dev, 0x1A00u, subidx);
    CHECK(sub != nullptr);

    co_sub_dn_ind_t* dn_ind = nullptr;
    co_sub_get_dn_ind(sub, &dn_ind, nullptr);
    CHECK(dn_ind != nullptr);

    membuf buf = {buffer, buffer, buffer + bufsize};
    co_sdo_req req = CO_SDO_REQ_INIT(req);
    co_sdo_req_init(&req, &buf);
    req.size = size;
    req.membuf = &buf;
    req.buf = buffer;
    req.nbyte = nbyte;

    const auto ret_req_dn = co_sdo_req_dn(&req, nullptr, &nbyte, nullptr);

    const auto ret_sub_dn_ind = co_sub_dn_ind(sub, &req);

    CHECK_EQUAL(rc_req_dn, ret_req_dn);
    CHECK_EQUAL(ac_sub_dn_ind, ret_sub_dn_ind);
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    Insert1A00Defaults();
    SetPdoCommCobid(0x000000FFu | CO_PDO_COBID_VALID);
  }

  TEST_TEARDOWN() { TEST_BASE_TEARDOWN(); }
};

TEST(CO_SdoTpdo1A00, Co1A00DnInd_NumOfMappingLenGreaterThanMax) {
  SetNumOfMappings(0x00);

  // object which could be mapped
  CreateObjInDev(obj2021, 0x2021u);
  Insert2021Defaults();

  // mapping values
  buffer.mapping = 0x20210120u;
  Check1A00Dn(0x01u, 4u, 0u, 0, buffer_mapping);
  buffer.mapping = 0x202101FFu;
  Check1A00Dn(0x02u, 4u, 0u, 0, buffer_mapping);

  buffer.num_of_elems = 2u;
  Check1A00Dn(0x00u, 1u, CO_SDO_AC_PDO_LEN, 0, buffer_num_of_elems);
}

TEST(CO_SdoTpdo1A00, Co1A00DnInd_NumOfMappingsNoDataToDn) {
  Check1A00Dn(0x00u, 0u, CO_SDO_AC_TYPE_LEN_LO, 0, nullptr);
}

TEST(CO_SdoTpdo1A00, Co1A00DnInd_NumOfMappingsNoObjToMap) {
  buffer.num_of_elems = 2u;
  Check1A00Dn(0x00u, 1u, CO_SDO_AC_NO_OBJ, 0, buffer_num_of_elems);
}

TEST(CO_SdoTpdo1A00, Co1A00DnInd_NumOfMappingsDownloadSameAsPrevious) {
  buffer.num_of_elems = 64u;
  Check1A00Dn(0x00u, 1u, 0u, 0, buffer_num_of_elems);
}

TEST(CO_SdoTpdo1A00, Co1A00DnInd_NumOfMappingsButValidBitNotSet) {
  buffer.num_of_elems = 2u;
  SetPdoCommCobid(0x000000FFu);
  Check1A00Dn(0x00u, 1u, CO_SDO_AC_PARAM_VAL, 0, buffer_num_of_elems);
}

TEST(CO_SdoTpdo1A00, Co1A00DnInd_NumOfMappingsTooManyObjsToMap) {
  // PDO supports up to 64 mappings in a single object
  buffer.num_of_elems = 65u;
  SetPdoCommCobid(0x000000FFu | CO_PDO_COBID_VALID);
  Check1A00Dn(0x00u, 1u, CO_SDO_AC_PARAM_VAL, 0, buffer_num_of_elems);
}

TEST(CO_SdoTpdo1A00, Co1A00DnInd_NumOfMappingsNoObjsToMap) {
  buffer.num_of_elems = 0u;
  Check1A00Dn(0x00u, 1u, 0u, 0, buffer_num_of_elems);
}

TEST(CO_SdoTpdo1A00, Co1A00DnInd_NumOfMapping) {
  // object which could be mapped
  SetNumOfMappings(0x00u);
  CreateObjInDev(obj2021, 0x2021u);
  Insert2021Defaults();

  // mapping value
  buffer.mapping = 0x20210120u;
  Check1A00Dn(0x01u, 4u, 0u, 0, buffer_mapping);

  buffer.num_of_elems = 1u;
  Check1A00Dn(0x00u, 1u, 0u, 0, buffer_num_of_elems);
}

TEST(CO_SdoTpdo1A00, Co1A00DnInd_MappingEmpty) {
  SetPdoCommCobid(0x000000FF | CO_PDO_COBID_VALID);
  SetNumOfMappings(0x00);

  CreateObjInDev(obj2021, 0x2021u);
  Insert2021Defaults();

  buffer.mapping = 0x20210110u;
  Check1A00Dn(0x01u, 4u, 0u, 0, buffer_mapping);

  buffer.mapping = 0x00000000u;
  Check1A00Dn(0x01u, 4u, 0u, 0, buffer_mapping);
}

TEST(CO_SdoTpdo1A00, Co1A00DnInd_MappingNonexisting) {
  SetNumOfMappings(0x00);

  buffer.mapping = 0xDEADBEEFu;
  Check1A00Dn(0x3Eu, 4u, CO_SDO_AC_NO_OBJ, 0, buffer_mapping);
}

TEST(CO_SdoTpdo1A00, Co1A00DnInd_MappingDoubles) {
  SetNumOfMappings(0x00u);

  CreateObjInDev(obj2021, 0x2021u);
  Insert2021Defaults();

  buffer.mapping = 0x20210110u;
  Check1A00Dn(0x01u, 4u, 0u, 0, buffer_mapping);

  buffer.mapping = 0x20210110u;
  Check1A00Dn(0x01u, 4u, 0u, 0, buffer_mapping);
}

TEST(CO_SdoTpdo1A00, Co1A00DnInd_MappingNumNonzero) {
  SetNumOfMappings(0x01u);

  CreateObjInDev(obj2021, 0x2021u);
  Insert2021Defaults();

  buffer.mapping = 0x20210110u;
  Check1A00Dn(0x01u, 4u, CO_SDO_AC_PARAM_VAL, 0, buffer_mapping);
}

TEST(CO_SdoTpdo1A00, Co1A00DnInd_MappingValidBitNotSet) {
  SetPdoCommCobid(0x000000FFu);
  SetNumOfMappings(0x01u);

  CreateObjInDev(obj2021, 0x2021u);
  Insert2021Defaults();

  buffer.mapping = 0x20210110u;
  Check1A00Dn(0x01u, 4u, CO_SDO_AC_PARAM_VAL, 0, buffer_mapping);
}

TEST(CO_SdoTpdo1A00, Co1A00DnInd_Mapping) {
  SetNumOfMappings(0x00);

  CreateObjInDev(obj2021, 0x2021u);
  Insert2021Defaults();

  for (co_unsigned8_t i = 1u; i < 0x40u; i++) {
    buffer.mapping = 0x20210010u | (i << 8u);
    Check1A00Dn(i, 4u, 0u, 0, buffer_mapping);
  }
}
