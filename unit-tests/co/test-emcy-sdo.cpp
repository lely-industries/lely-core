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

#include <lely/co/csdo.h>
#include <lely/co/emcy.h>
#include <lely/co/obj.h>
#include <lely/co/sdo.h>

#include <libtest/allocators/default.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"

TEST_GROUP(CO_EmcySdo) {
  Allocators::Default allocator;

  const co_unsigned8_t DEV_ID = 0x01u;
  const co_unsigned32_t EMCY_CANID = DEV_ID;
  const co_unsigned32_t EMCY_EID_CANID = EMCY_CANID | (1 << 28u);
  const co_unsigned32_t CONSUMER_COBID = DEV_ID;
  const co_unsigned32_t EXAMPLE_COBID = DEV_ID + 1u;
  const co_unsigned8_t EXCESS_1028_SUBIDX = 0x10u;

  co_dev_t* dev = nullptr;
  can_net_t* net = nullptr;
  co_emcy_t* emcy = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj1001;
  std::unique_ptr<CoObjTHolder> obj1003;
  std::unique_ptr<CoObjTHolder> obj1014;
  std::unique_ptr<CoObjTHolder> obj1028;

  void CreateObjInDev(std::unique_ptr<CoObjTHolder> & obj_holder,
                      co_unsigned16_t idx) {
    obj_holder.reset(new CoObjTHolder(idx));
    CHECK(obj_holder->Get() != nullptr);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));
  }

  void CreateObj1001ErrorRegister() {
    CreateObjInDev(obj1001, 0x1001u);
    obj1001->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0u});
  }

  void CreateObj1003PredefinedErrorField() {
    CreateObjInDev(obj1003, 0x1003u);
    co_obj_set_code(obj1003->Get(), CO_OBJECT_ARRAY);
    obj1003->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{0u});
    obj1003->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32, co_unsigned32_t{0u});
  }

  void CreateObj1014CobIdEmcy() {
    CreateObjInDev(obj1014, 0x1014u);
    obj1014->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED32, EMCY_CANID);
  }

  void CreateObj1028EmcyConsumerObject() {
    CreateObjInDev(obj1028, 0x1028u);
    co_obj_set_code(obj1028->Get(), CO_OBJECT_ARRAY);
    obj1028->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t{1u});
    obj1028->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32, CONSUMER_COBID);
    // NOTE: one test requires additional sub-object and it cannot be added
    //       after starting EMCY
    obj1028->InsertAndSetSub(EXCESS_1028_SUBIDX, CO_DEFTYPE_UNSIGNED32,
                             CONSUMER_COBID + 1);
  }

  void RestartEMCY() {
    co_emcy_stop(emcy);
    CHECK_EQUAL(0, co_emcy_start(emcy));
  }

  void CheckDnConAbortCode(co_unsigned16_t obj_idx, co_unsigned8_t sub_idx,
                           co_unsigned32_t ac) {
    CHECK(CoCsdoDnCon::Called());
    CoCsdoDnCon::Check(nullptr, obj_idx, sub_idx, ac, nullptr);
  }

  void CheckDnConSuccess(co_unsigned16_t obj_idx, co_unsigned8_t sub_idx) {
    CheckDnConAbortCode(obj_idx, sub_idx, 0u);
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    net = can_net_create(allocator.ToAllocT());
    CHECK(net != nullptr);

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    CreateObj1001ErrorRegister();
    CreateObj1003PredefinedErrorField();
    CreateObj1014CobIdEmcy();
    CreateObj1028EmcyConsumerObject();

    emcy = co_emcy_create(net, dev);
    CHECK(emcy != nullptr);

    CoCsdoDnCon::Clear();

    CHECK_EQUAL(0, co_emcy_start(emcy));
  }

  TEST_TEARDOWN() {
    co_emcy_destroy(emcy);

    dev_holder.reset();
    can_net_destroy(net);
  }
};

/// @name EMCY service: object 0x1003 modification using SDO
///@{

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the Pre-defined Error Field object (0x1003)
///
/// \When the download indication function for the object 0x1003 is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_EmcySdo, Co1003Dn_NonZeroAbortCode) {
  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret =
      LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1003u, 0x00u, ac);

  CHECK_EQUAL(ac, ret);
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the Pre-defined Error Field object (0x1003)
///
/// \When a value longer than one byte is downloaded to the object 0x1003 using
///       SDO
///
/// \Then CO_SDO_AC_TYPE_LEN_HI abort code is passed to CSDO download
///       confirmation function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
TEST(CO_EmcySdo, Co1003Dn_TypeLenTooHigh) {
  const co_unsigned16_t errors = 0u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1003u, 0x00u, CO_DEFTYPE_UNSIGNED16, &errors,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConAbortCode(0x1003u, 0x00u, CO_SDO_AC_TYPE_LEN_HI);
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the Pre-defined Error Field object (0x1003)
///
/// \When any value is downloaded to a sub-object of the object 0x1003 using SDO
///
/// \Then CO_SDO_AC_NO_WRITE abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_EmcySdo, Co1003Dn_CannotWriteToNonZeroSubidx) {
  const co_unsigned32_t error = 0u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1003u, 0x01u, CO_DEFTYPE_UNSIGNED32, &error,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConAbortCode(0x1003u, 0x01u, CO_SDO_AC_NO_WRITE);
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the Pre-defined Error Field object (0x1003)
///
/// \When a value different than zero is downloaded to the object 0x1003 using
///       SDO
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_EmcySdo, Co1003Dn_NonZeroValuesNotAllowed) {
  const co_unsigned8_t errors = 1u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1003u, 0x00u, CO_DEFTYPE_UNSIGNED8, &errors,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConAbortCode(0x1003u, 0x00u, CO_SDO_AC_PARAM_VAL);
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the Pre-defined Error Field object (0x1003) with multiple
///        errors recorded
///
/// \When a value equal to zero is downloaded to the object 0x1003 using SDO
///
/// \Then a zero abort code is passed to CSDO download confirmation function, 0
///       is set in the 0x1003 object, the error stack of the EMCY service is
///       cleared
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_dn()
///       \Calls co_obj_addressof_val()
///       \Calls co_obj_get_subidx()
TEST(CO_EmcySdo, Co1003Dn_ZeroResetsEmcyMessageStack) {
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x6100u, 0u, nullptr));
  CHECK_EQUAL(0, co_emcy_push(emcy, 0x6200u, 0u, nullptr));

  const co_unsigned8_t errors = 0u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1003u, 0x00u, CO_DEFTYPE_UNSIGNED8, &errors,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConSuccess(0x1003u, 0x00u);

  co_unsigned16_t eec = 0xffffu;
  co_unsigned8_t er = 0xffu;
  co_emcy_peek(emcy, &eec, &er);
  CHECK_EQUAL(0u, eec);
  CHECK_EQUAL(0u, er);
}

///@}

/// @name EMCY service: object 0x1014 modification using SDO
///@{

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the COB-ID EMCY object (0x1014)
///
/// \When the download indication function for the object 0x1014 is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_EmcySdo, Co1014Dn_NonZeroAbortCode) {
  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret =
      LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1014u, 0x00u, ac);

  CHECK_EQUAL(ac, ret);
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the COB-ID EMCY object (0x1014)
///
/// \When a value shorter than 4 bytes is downloaded to the object 0x1014 using
///       SDO
///
/// \Then CO_SDO_AC_TYPE_LEN_LO abort code is passed to CSDO download
///       confirmation function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
TEST(CO_EmcySdo, Co1014Dn_TypeLenTooLow) {
  const co_unsigned16_t value = 0x1234u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1014u, 0x00u, CO_DEFTYPE_UNSIGNED16, &value,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConAbortCode(0x1014u, 0x00u, CO_SDO_AC_TYPE_LEN_LO);
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the COB-ID EMCY object (0x1014) with an additional
///        sub-object at a non-zero sub-index set to any value
///
/// \When the sub-object is changed using SDO
///
/// \Then CO_SDO_AC_NO_SUB abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_EmcySdo, Co1014Dn_InvalidSubobject) {
  obj1014->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED16,
                           co_unsigned16_t{0x42u});
  RestartEMCY();

  const co_unsigned16_t data = 0;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1014u, 0x01u, CO_DEFTYPE_UNSIGNED16, &data,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConAbortCode(0x1014u, 0x01u, CO_SDO_AC_NO_SUB);
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the COB-ID EMCY object (0x1014)
///
/// \When same value as already set is downloaded to the object 0x1014 using SDO
///
/// \Then a zero abort code is passed to CSDO download confirmation function,
///       nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_EmcySdo, Co1014Dn_SameAsPrevious) {
  const co_unsigned32_t cobid = EMCY_CANID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1014u, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConSuccess(0x1014u, 0x00u);

  CHECK_EQUAL(cobid, co_obj_get_val_u32(obj1014->Get(), 0x00u));
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the COB-ID EMCY object (0x1014) with a valid COB-ID set
///
/// \When a valid COB-ID with a different CAN-ID is downloaded to the object
///       0x1014 using SDO
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_EmcySdo, Co1014Dn_OldValidNewValid_DifferentCanId) {
  const co_unsigned32_t cobid = EMCY_CANID + 20u;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1014u, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConAbortCode(0x1014u, 0x00u, CO_SDO_AC_PARAM_VAL);
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the COB-ID EMCY object (0x1014) with a valid COB-ID set
///
/// \When a valid COB-ID with the same CAN-ID is downloaded to the object 0x1014
///       using SDO
///
/// \Then a zero abort code is passed to CSDO download confirmation function,
///       new COB-ID is set in the object 0x1014
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_sub_dn()
TEST(CO_EmcySdo, Co1014Dn_OldValidNewValid_SameCanId) {
  const co_unsigned32_t cobid = EMCY_CANID | CO_EMCY_COBID_FRAME;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1014u, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConSuccess(0x1014u, 0x00u);

  CHECK_EQUAL(cobid, co_obj_get_val_u32(obj1014->Get(), 0x00u));
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the COB-ID EMCY object (0x1014) with a valid COB-ID set
///
/// \When an invalid COB-ID is downloaded to the object 0x1014 using SDO
///
/// \Then a zero abort code is passed to CSDO download confirmation function,
///       new COB-ID is set in the object 0x1014
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_sub_dn()
TEST(CO_EmcySdo, Co1014Dn_OldValidNewInvalid) {
  const co_unsigned32_t cobid = EMCY_CANID | CO_EMCY_COBID_VALID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1014u, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConSuccess(0x1014u, 0x00u);

  CHECK_EQUAL(cobid, co_obj_get_val_u32(obj1014->Get(), 0x00u));
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the COB-ID EMCY object (0x1014) with an invalid COB-ID set
///
/// \When a valid COB-ID is downloaded to the object 0x1014 using SDO
///
/// \Then a zero abort code is passed to CSDO download confirmation function,
///       new COB-ID is set in the object 0x1014
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_sub_dn()
TEST(CO_EmcySdo, Co1014Dn_OldInvalidNewValid) {
  co_sub_t* const sub = co_dev_find_sub(dev, 0x1014u, 0x00u);
  co_sub_set_val_u32(sub, EMCY_CANID | CO_EMCY_COBID_VALID);
  RestartEMCY();

  const co_unsigned32_t cobid = EMCY_CANID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1014u, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConSuccess(0x1014u, 0x00u);

  CHECK_EQUAL(cobid, co_obj_get_val_u32(obj1014->Get(), 0x00u));
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the COB-ID EMCY object (0x1014) with an invalid COB-ID set
///
/// \When a valid COB-ID with a different CAN-ID using Extended Identifier but
///       without frame bit set is downloaded to the object 0x1014 using SDO
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
TEST(CO_EmcySdo, Co1014Dn_ExtendedIdWithoutFrameBitSet) {
  co_sub_t* const sub = co_dev_find_sub(dev, 0x1014u, 0x00u);
  co_sub_set_val_u32(sub, EMCY_CANID | CO_EMCY_COBID_VALID);
  RestartEMCY();

  const co_unsigned32_t cobid = EMCY_EID_CANID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1014u, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConAbortCode(0x1014u, 0x00u, CO_SDO_AC_PARAM_VAL);
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the COB-ID EMCY object (0x1014) with an invalid COB-ID set
///
/// \When a valid COB-ID with a different CAN-ID using Extended Identifier and
///       with frame bit set is downloaded to the object 0x1014 using SDO
///
/// \Then a zero abort code is passed to CSDO download confirmation function,
///       new COB-ID is set in the object 0x1014
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_val_u32()
///       \Calls co_sub_dn()
TEST(CO_EmcySdo, Co1014Dn_ExtendedIdWithFrameBitSet) {
  co_sub_t* const sub = co_dev_find_sub(dev, 0x1014u, 0x00u);
  co_sub_set_val_u32(sub, EMCY_CANID | CO_EMCY_COBID_VALID);
  RestartEMCY();

  const co_unsigned32_t cobid = EMCY_EID_CANID | CO_EMCY_COBID_FRAME;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1014u, 0x00u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConSuccess(0x1014u, 0x00u);

  CHECK_EQUAL(cobid, co_obj_get_val_u32(obj1014->Get(), 0x00u));
}

///@}

/// @name EMCY service: object 0x1028 modification using SDO
///@{

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the Emergency Consumer Object (0x1028)
///
/// \When the download indication function for the object 0x1028 is called with
///       a non-zero abort code
///
/// \Then the same abort code value is returned, nothing is changed
///       \Calls co_sub_get_type()
TEST(CO_EmcySdo, Co1028Dn_NonZeroAbortCode) {
  const co_unsigned32_t ac = CO_SDO_AC_ERROR;

  const auto ret =
      LelyUnitTest::CallDnIndWithAbortCode(dev, 0x1028u, 0x01u, ac);

  CHECK_EQUAL(ac, ret);
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the Emergency Consumer Object (0x1028) with a consumer
///        COB-ID sub-object
///
/// \When a value shorter than 4 bytes is downloaded to the sub-object using SDO
///
/// \Then CO_SDO_AC_TYPE_LEN_LO abort code is passed to CSDO download
///       confirmation function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
TEST(CO_EmcySdo, Co1028Dn_TypeLenTooLow) {
  const co_unsigned16_t value = 0x1234u;

  const auto ret =
      co_dev_dn_val_req(dev, 0x1028u, 0x01u, CO_DEFTYPE_UNSIGNED16, &value,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConAbortCode(0x1028u, 0x01u, CO_SDO_AC_TYPE_LEN_LO);
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the Emergency Consumer Object (0x1028)
///
/// \When any value is downloaded to the object 0x1028 at sub-index 0x00 using
///       SDO
///
/// \Then CO_SDO_AC_NO_WRITE abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
TEST(CO_EmcySdo, Co1028Dn_HighestSubidxIsConst) {
  const co_unsigned8_t value = 0x05u;

  const auto ret =
      co_dev_dn_val_req(dev, 0x1028u, 0x00u, CO_DEFTYPE_UNSIGNED8, &value,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConAbortCode(0x1028u, 0x00u, CO_SDO_AC_NO_WRITE);
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the Emergency Consumer Object (0x1028) with an additional
///        sub-object at a sub-index greater than the number of declared
///        consumer COB-IDs
///
/// \When the sub-object is changed using SDO
///
/// \Then CO_SDO_AC_NO_SUB abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_obj()
///       \Calls co_obj_get_val_u8()
TEST(CO_EmcySdo, Co1028Dn_SubidxGreaterThanNumConsumers) {
  const co_unsigned32_t cobid = EXAMPLE_COBID;

  const auto ret =
      co_dev_dn_val_req(dev, 0x1028u, EXCESS_1028_SUBIDX, CO_DEFTYPE_UNSIGNED32,
                        &cobid, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConAbortCode(0x1028u, EXCESS_1028_SUBIDX, CO_SDO_AC_NO_SUB);
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the Emergency Consumer Object (0x1028) with more than 127
///        COB-IDs inserted and with an additional sub-object at a sub-index
///        greater than the number of declared consumer COB-IDs
///
/// \When the sub-object is changed using SDO
///
/// \Then CO_SDO_AC_NO_SUB abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_obj()
///       \Calls co_obj_get_val_u8()
TEST(CO_EmcySdo, Co1028Dn_SubidxGreaterThanNumNodes) {
  const co_unsigned8_t subidx = CO_NUM_NODES + 1;
  co_sub_t* const sub = co_dev_find_sub(dev, 0x1028u, 0x00u);
  co_sub_set_val_u8(sub, CO_NUM_NODES + 2u);
  obj1028->InsertAndSetSub(subidx, CO_DEFTYPE_UNSIGNED32, CONSUMER_COBID + 1u);
  RestartEMCY();

  const co_unsigned32_t cobid = EXAMPLE_COBID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1028u, subidx, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConAbortCode(0x1028u, subidx, CO_SDO_AC_NO_SUB);
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the Emergency Consumer Object (0x1028)
///
/// \When same value as already set is downloaded to the object 0x1028 at any
///       non-zero sub-index using SDO
///
/// \Then a zero abort code is passed to CSDO download confirmation function,
///       nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_obj()
///       \Calls co_obj_get_val_u8()
///       \Calls co_sub_get_val_u32()
TEST(CO_EmcySdo, Co1028Dn_SameAsPrevious) {
  const co_unsigned32_t cobid = CONSUMER_COBID;

  const auto ret =
      co_dev_dn_val_req(dev, 0x1028u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConSuccess(0x1028u, 0x01u);

  CHECK_EQUAL(cobid, co_obj_get_val_u32(obj1028->Get(), 0x01u));
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the Emergency Consumer Object (0x1028) with a valid consumer
///        COB-ID sub-object
///
/// \When a valid COB-ID with a different CAN-ID is downloaded to the sub-object
///       using SDO
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_obj()
///       \Calls co_obj_get_val_u8()
///       \Calls co_sub_get_val_u32()
TEST(CO_EmcySdo, Co1028Dn_OldValidNewValid_DifferentCanId) {
  const co_unsigned32_t cobid = CONSUMER_COBID + 1u;

  const auto ret =
      co_dev_dn_val_req(dev, 0x1028u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConAbortCode(0x1028u, 0x01u, CO_SDO_AC_PARAM_VAL);
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the Emergency Consumer Object (0x1028) with a valid consumer
///        COB-ID sub-object
///
/// \When an invalid COB-ID is downloaded to the sub-object using SDO
///
/// \Then a zero abort code is passed to CSDO download confirmation function,
///       new COB-ID is set in the sub-object
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_obj()
///       \Calls co_obj_get_val_u8()
///       \Calls co_sub_get_val_u32()
///       \Calls can_recv_stop()
///       \Calls co_sub_dn()
TEST(CO_EmcySdo, Co1028Dn_OldValidNewInvalid) {
  const co_unsigned32_t cobid = CONSUMER_COBID | CO_EMCY_COBID_VALID;

  const auto ret =
      co_dev_dn_val_req(dev, 0x1028u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConSuccess(0x1028u, 0x01u);

  CHECK_EQUAL(cobid, co_obj_get_val_u32(obj1028->Get(), 0x01u));
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the Emergency Consumer Object (0x1028) with an invalid
///        consumer COB-ID sub-object
///
/// \When a valid COB-ID is downloaded to the sub-object using SDO
///
/// \Then a zero abort code is passed to CSDO download confirmation function,
///       new COB-ID is set in the sub-object
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_obj()
///       \Calls co_obj_get_val_u8()
///       \Calls co_sub_get_val_u32()
///       \Calls can_recv_stop()
///       \Calls co_sub_dn()
TEST(CO_EmcySdo, Co1028Dn_OldInvalidNewValid) {
  co_sub_t* const sub = co_dev_find_sub(dev, 0x1028u, 0x01u);
  co_sub_set_val_u32(sub, CONSUMER_COBID | CO_EMCY_COBID_VALID);
  RestartEMCY();

  const co_unsigned32_t cobid = CONSUMER_COBID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1028u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConSuccess(0x1028u, 0x01u);

  CHECK_EQUAL(cobid, co_obj_get_val_u32(obj1028->Get(), 0x01u));
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the Emergency Consumer Object (0x1028) with a valid consumer
///        COB-ID sub-object
///
/// \When a valid COB-ID with the same CAN-ID is downloaded to the sub-object
///       using SDO
///
/// \Then a zero abort code is passed to CSDO download confirmation function,
///       new COB-ID is set in the sub-object
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_obj()
///       \Calls co_obj_get_val_u8()
///       \Calls co_sub_get_val_u32()
///       \Calls can_recv_stop()
///       \Calls co_sub_dn()
TEST(CO_EmcySdo, Co1028Dn_OldValidNewValid_SameCanId) {
  const co_unsigned32_t cobid = CONSUMER_COBID | CO_EMCY_COBID_FRAME;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1028u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConSuccess(0x1028u, 0x01u);

  CHECK_EQUAL(cobid, co_obj_get_val_u32(obj1028->Get(), 0x01u));
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the Emergency Consumer Object (0x1028) with an invalid
///        consumer COB-ID sub-object
///
/// \When a valid COB-ID with a different CAN-ID using Extended Identifier but
///       without frame bit set is downloaded to the sub-object using SDO
///
/// \Then CO_SDO_AC_PARAM_VAL abort code is passed to CSDO download confirmation
///       function, nothing is changed
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_obj()
///       \Calls co_obj_get_val_u8()
///       \Calls co_sub_get_val_u32()
TEST(CO_EmcySdo, Co1028Dn_ExtendedIdWithoutFrameBitSet) {
  co_sub_t* const sub = co_dev_find_sub(dev, 0x1028u, 0x01u);
  co_sub_set_val_u32(sub, CONSUMER_COBID | CO_EMCY_COBID_VALID);
  RestartEMCY();

  const co_unsigned32_t cobid = EMCY_EID_CANID;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1028u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConAbortCode(0x1028u, 0x01u, CO_SDO_AC_PARAM_VAL);
}

/// \Given a pointer to started EMCY service (co_emcy_t), the object dictionary
///        contains the Emergency Consumer Object (0x1028) with an invalid
///        consumer COB-ID sub-object
///
/// \When a valid COB-ID with a different CAN-ID using Extended Identifier and
///       with frame bit set is downloaded to the sub-object using SDO
///
/// \Then a zero abort code is passed to CSDO download confirmation function,
///       new COB-ID is set in the sub-object
///       \Calls co_sub_get_type()
///       \Calls co_sdo_req_dn_val()
///       \Calls co_sub_get_subidx()
///       \Calls co_sub_get_obj()
///       \Calls co_obj_get_val_u8()
///       \Calls co_sub_get_val_u32()
///       \Calls can_recv_stop()
///       \Calls co_sub_dn()
TEST(CO_EmcySdo, Co1028Dn_ExtendedIdWithFrameBitSet) {
  co_sub_t* const sub = co_dev_find_sub(dev, 0x1028u, 0x01u);
  co_sub_set_val_u32(sub, CONSUMER_COBID | CO_EMCY_COBID_VALID);
  RestartEMCY();

  const co_unsigned32_t cobid = EMCY_EID_CANID | CO_EMCY_COBID_FRAME;
  const auto ret =
      co_dev_dn_val_req(dev, 0x1028u, 0x01u, CO_DEFTYPE_UNSIGNED32, &cobid,
                        nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
  CheckDnConSuccess(0x1028u, 0x01u);

  CHECK_EQUAL(cobid, co_obj_get_val_u32(obj1028->Get(), 0x01u));
}

///@}
