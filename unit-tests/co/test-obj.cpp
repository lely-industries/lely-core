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

#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/sdo.h>
#include <lely/co/val.h>
#include <lely/util/diag.h>
#include <lely/util/errnum.h>

#include "lely-unit-test.hpp"
#include "override/lelyco-val.hpp"

#include "dev-holder.hpp"
#include "obj-holder.hpp"
#include "sub-holder.hpp"

TEST_GROUP(CO_ObjInit) {
  TEST_SETUP() { LelyUnitTest::DisableDiagnosticMessages(); }

  co_obj_t* AcquireCoObjT() {
#if LELY_NO_MALLOC
    return &object;
#else
    return static_cast<co_obj_t*>(__co_obj_alloc());
#endif
  }

  void ReleaseCoObjT(co_obj_t * obj) {
#if LELY_NO_MALLOC
    POINTERS_EQUAL(&object, obj);
#else
    __co_obj_free(obj);
#endif
  }

  void DestroyCoObjT(co_obj_t * obj) {
    __co_obj_fini(obj);
    ReleaseCoObjT(obj);
  }

#if LELY_NO_MALLOC
  co_obj_t object;
#endif
};

TEST(CO_ObjInit, CoObjInit) {
  auto* const obj = AcquireCoObjT();

  CHECK(obj != nullptr);
  POINTERS_EQUAL(obj, __co_obj_init(obj, 0x1234, NULL, 0));

  POINTERS_EQUAL(nullptr, co_obj_get_dev(obj));
  CHECK_EQUAL(0x1234, co_obj_get_idx(obj));

#ifndef LELY_NO_CO_OBJ_NAME
  POINTERS_EQUAL(nullptr, co_obj_get_name(obj));
#endif

  CHECK_EQUAL(CO_OBJECT_VAR, co_obj_get_code(obj));

  POINTERS_EQUAL(nullptr, co_obj_get_val(obj, 0x00));
  CHECK_EQUAL(0, co_obj_sizeof_val(obj));

  DestroyCoObjT(obj);
}

TEST(CO_ObjInit, CoObjFini) {
  auto* const obj = AcquireCoObjT();
  __co_obj_init(obj, 0x1234, NULL, 0);
  CoDevTHolder dev(0x01);

  co_dev_insert_obj(dev.Get(), obj);

  DestroyCoObjT(obj);
}

TEST_BASE(CO_ObjBase) {
  const co_unsigned16_t OBJ_IDX = 0x1234u;
  const co_unsigned8_t SUB_IDX = 0xabu;
  const co_unsigned16_t SUB_DEFTYPE = CO_DEFTYPE_INTEGER16;
  using sub_type = co_integer16_t;

  const char* const TEST_STR = "testtesttest";
};

TEST_GROUP_BASE(CO_Obj, CO_ObjBase) {
  std::unique_ptr<CoObjTHolder> obj_holder;
  co_obj_t* obj = nullptr;

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();

    obj_holder.reset(new CoObjTHolder(OBJ_IDX));
    obj = obj_holder->Get();

    CHECK(obj != nullptr);
  }

  TEST_TEARDOWN() { obj_holder.reset(); }
};

TEST_GROUP_BASE(CO_Sub, CO_ObjBase) {
  const CO_ObjBase::sub_type SUB_MIN = CO_INTEGER16_MIN;
  const CO_ObjBase::sub_type SUB_MAX = CO_INTEGER16_MAX;
  const CO_ObjBase::sub_type SUB_DEF = 0x0000;

  std::unique_ptr<CoSubTHolder> sub_holder;
  co_sub_t* sub = nullptr;

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();

    sub_holder.reset(new CoSubTHolder(SUB_IDX, SUB_DEFTYPE));
    sub = sub_holder->Get();
    CHECK(sub != nullptr);
  }

  TEST_TEARDOWN() { sub_holder.reset(); }
};

namespace CO_ObjSub_Static {
static unsigned int dn_ind_func_counter = 0;
static unsigned int up_ind_func_counter = 0;
}  // namespace CO_ObjSub_Static

TEST_GROUP_BASE(CO_ObjSub, CO_ObjBase) {
  std::unique_ptr<CoObjTHolder> obj_holder;
  std::unique_ptr<CoSubTHolder> sub_holder;
  co_obj_t* obj = nullptr;
  co_sub_t* sub = nullptr;

  static co_unsigned32_t dn_ind_func(co_sub_t*, struct co_sdo_req*, void*) {
    ++CO_ObjSub_Static::dn_ind_func_counter;
    return 0;
  }
  static co_unsigned32_t up_ind_func(const co_sub_t*, struct co_sdo_req*,
                                     void*) {
    ++CO_ObjSub_Static::up_ind_func_counter;
    return 0;
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();

    obj_holder.reset(new CoObjTHolder(OBJ_IDX));
    obj = obj_holder->Get();
    CHECK(obj != nullptr);

    sub_holder.reset(new CoSubTHolder(SUB_IDX, SUB_DEFTYPE));
    sub = sub_holder->Get();
    CHECK(sub != nullptr);

    CHECK_EQUAL(0, co_obj_insert_sub(obj, sub_holder->Take()));

    CO_ObjSub_Static::dn_ind_func_counter = 0;
    CO_ObjSub_Static::up_ind_func_counter = 0;
  }

  TEST_TEARDOWN() {
    sub_holder.reset();
    obj_holder.reset();
  }
};

TEST_GROUP_BASE(CO_ObjDev, CO_ObjBase) {
  const co_unsigned8_t DEV_ID = 0x01u;

  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj_holder;
  std::unique_ptr<CoSubTHolder> sub_holder;
  co_dev_t* dev = nullptr;
  co_obj_t* obj = nullptr;
  co_sub_t* sub = nullptr;

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    obj_holder.reset(new CoObjTHolder(OBJ_IDX));
    obj = obj_holder->Get();
    CHECK(obj != nullptr);

    sub_holder.reset(new CoSubTHolder(SUB_IDX, SUB_DEFTYPE));
    sub = sub_holder->Get();
    CHECK(sub != nullptr);

    CHECK_EQUAL(0, co_obj_insert_sub(obj, sub_holder->Take()));
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));
  }

  TEST_TEARDOWN() {
    sub_holder.reset();
    obj_holder.reset();
    dev_holder.reset();
  }
};

TEST(CO_Obj, CoObjPrev_Null) { POINTERS_EQUAL(nullptr, co_obj_prev(obj)); }

TEST(CO_ObjDev, CoObjPrev_OnlyOne) {
  POINTERS_EQUAL(nullptr, co_obj_prev(obj));
}

TEST(CO_ObjDev, CoObjPrev_Removed) {
  CHECK_EQUAL(0, co_dev_remove_obj(dev, obj));

  POINTERS_EQUAL(nullptr, co_obj_prev(obj));

#if !LELY_NO_MALLOC
  co_obj_destroy(obj);
#endif
}

TEST(CO_ObjDev, CoObjPrev) {
  CoObjTHolder obj2_holder(0x0001u);
  co_obj_t* obj2 = obj2_holder.Get();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2_holder.Take()));

  POINTERS_EQUAL(obj2, co_obj_prev(obj));
}

TEST(CO_Obj, CoObjNext_Null) { POINTERS_EQUAL(nullptr, co_obj_next(obj)); }

TEST(CO_ObjDev, CoObjNext_OnlyOne) {
  POINTERS_EQUAL(nullptr, co_obj_next(obj));
}

TEST(CO_ObjDev, CoObjNext_Removed) {
  CHECK_EQUAL(0, co_dev_remove_obj(dev, obj));

  POINTERS_EQUAL(nullptr, co_obj_next(obj));

#if !LELY_NO_MALLOC
  co_obj_destroy(obj);
#endif
}

TEST(CO_ObjDev, CoObjNext) {
  CoObjTHolder obj2_holder(0x2222u);
  co_obj_t* obj2 = obj2_holder.Get();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2_holder.Take()));

  POINTERS_EQUAL(obj2, co_obj_next(obj));
}

TEST(CO_Obj, CoObjGetDev_Null) { POINTERS_EQUAL(nullptr, co_obj_get_dev(obj)); }

TEST(CO_ObjDev, CoObjGetDev) { POINTERS_EQUAL(dev, co_obj_get_dev(obj)); }

TEST(CO_Obj, CoObjGetIdx) { CHECK_EQUAL(OBJ_IDX, co_obj_get_idx(obj)); }

TEST(CO_Obj, CoObjGetSubidx_Null) {
  CHECK_EQUAL(0, co_obj_get_subidx(obj, 0, nullptr));
}

TEST(CO_Obj, CoObjGetSubidx_Empty) {
  co_unsigned8_t sub_list[1] = {0};

  const auto sub_count = co_obj_get_subidx(obj, 1, sub_list);

  CHECK_EQUAL(0, sub_count);
  CHECK_EQUAL(0, sub_list[0]);
}

TEST(CO_ObjDev, CoObjGetSubidx) {
  CoSubTHolder sub2(0x42, CO_DEFTYPE_INTEGER16);
  CHECK(sub != nullptr);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub2.Take()));

  co_unsigned8_t sub_list[1] = {0};
  const auto sub_count = co_obj_get_subidx(obj, 1u, sub_list);

  CHECK_EQUAL(2u, sub_count);
  CHECK_EQUAL(0x42u, sub_list[0]);
}

TEST(CO_ObjSub, CoObjInsertSub_AlreadyAddedToOtherDev) {
  CoObjTHolder obj2(0x0001u);

  CHECK_EQUAL(-1, co_obj_insert_sub(obj2.Get(), sub));
}

TEST(CO_ObjSub, CoObjInsertSub_AlreadyAdded) {
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
}

TEST(CO_ObjSub, CoObjInsertSub_AlreadyAddedAtSubidx) {
  CoSubTHolder sub2(SUB_IDX, CO_DEFTYPE_INTEGER16);

  CHECK_EQUAL(-1, co_obj_insert_sub(obj, sub2.Get()));
}

TEST(CO_Obj, CoObjInsertSub) {
  CoSubTHolder sub2_holder(SUB_IDX, CO_DEFTYPE_INTEGER16);
  const auto sub2 = sub2_holder.Take();
  CHECK(sub2 != nullptr);

  const auto ret = co_obj_insert_sub(obj, sub2);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(sub2, co_obj_find_sub(obj, SUB_IDX));
}

TEST(CO_ObjSub, CoObjRemoveSub_SubInAnotherObj) {
  CoObjTHolder obj2_holder(0x0001u);
  const auto obj2 = obj2_holder.Get();
  CHECK(obj2 != nullptr);

  CHECK_EQUAL(-1, co_obj_remove_sub(obj2, sub));
}

TEST(CO_ObjSub, CoObjRemoveSub) {
  CHECK_EQUAL(0, co_obj_remove_sub(obj, sub));
#if !LELY_NO_MALLOC
  co_sub_destroy(sub);
#endif
}

TEST(CO_ObjSub, CoObjFindSub) {
  POINTERS_EQUAL(sub, co_obj_find_sub(obj, SUB_IDX));
}

TEST(CO_Obj, CoObjFindSub_NotFound) {
  POINTERS_EQUAL(nullptr, co_obj_find_sub(obj, SUB_IDX));
}

TEST(CO_ObjSub, CoObjFirstSub) { POINTERS_EQUAL(sub, co_obj_first_sub(obj)); }

TEST(CO_Obj, CoObjFirstSub_Empty) {
  POINTERS_EQUAL(nullptr, co_obj_first_sub(obj));
}

TEST(CO_ObjSub, CoObjLastSub) { POINTERS_EQUAL(sub, co_obj_last_sub(obj)); }

TEST(CO_Obj, CoObjLastSub_Empty) {
  POINTERS_EQUAL(nullptr, co_obj_last_sub(obj));
}

#ifndef LELY_NO_CO_OBJ_NAME

TEST(CO_Obj, CoObjSetName_Null) {
  CHECK_EQUAL(0, co_obj_set_name(obj, nullptr));

  POINTERS_EQUAL(nullptr, co_obj_get_name(obj));
}

TEST(CO_Obj, CoObjSetName_Empty) {
  const auto ret = co_obj_set_name(obj, "");

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_obj_get_name(obj));
}

TEST(CO_Obj, CoObjSetName) {
  const auto ret = co_obj_set_name(obj, TEST_STR);

  CHECK_EQUAL(0, ret);
  STRCMP_EQUAL(TEST_STR, co_obj_get_name(obj));
}

#endif  // !LELY_NO_CO_OBJ_NAME

TEST(CO_Obj, CoObjSetCode) {
  CHECK_EQUAL(0, co_obj_set_code(obj, CO_OBJECT_NULL));
  CHECK_EQUAL(CO_OBJECT_NULL, co_obj_get_code(obj));

  CHECK_EQUAL(0, co_obj_set_code(obj, CO_OBJECT_DOMAIN));
  CHECK_EQUAL(CO_OBJECT_DOMAIN, co_obj_get_code(obj));

  CHECK_EQUAL(0, co_obj_set_code(obj, CO_OBJECT_DEFTYPE));
  CHECK_EQUAL(CO_OBJECT_DEFTYPE, co_obj_get_code(obj));

  CHECK_EQUAL(0, co_obj_set_code(obj, CO_OBJECT_DEFSTRUCT));
  CHECK_EQUAL(CO_OBJECT_DEFSTRUCT, co_obj_get_code(obj));

  CHECK_EQUAL(0, co_obj_set_code(obj, CO_OBJECT_VAR));
  CHECK_EQUAL(CO_OBJECT_VAR, co_obj_get_code(obj));

  CHECK_EQUAL(0, co_obj_set_code(obj, CO_OBJECT_ARRAY));
  CHECK_EQUAL(CO_OBJECT_ARRAY, co_obj_get_code(obj));

  CHECK_EQUAL(0, co_obj_set_code(obj, CO_OBJECT_RECORD));
  CHECK_EQUAL(CO_OBJECT_RECORD, co_obj_get_code(obj));
}

TEST(CO_Obj, CoObjSetCode_Invalid) {
  auto const ret = co_obj_set_code(obj, 0xffu);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_OBJECT_VAR, co_obj_get_code(obj));
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_Obj, CoObjAddressofVal_Null) {
  POINTERS_EQUAL(nullptr, co_obj_addressof_val(nullptr));
}

TEST(CO_Obj, CoObjAddressofVal_NoVal) {
#if LELY_NO_MALLOC
  __co_obj_init(obj, OBJ_IDX, nullptr, 0);
#endif
  POINTERS_EQUAL(nullptr, co_obj_addressof_val(obj));
}

TEST(CO_ObjSub, CoObjAddressofVal) {
  const co_unsigned16_t val = 0x4242u;
  CHECK_EQUAL(sizeof(val), co_obj_set_val(obj, SUB_IDX, &val, sizeof(val)));

  CHECK(co_obj_addressof_val(obj) != nullptr);
}

TEST(CO_Obj, CoObjSizeofVal_Null) {
  CHECK_EQUAL(0, co_obj_sizeof_val(nullptr));
}

TEST(CO_Obj, CoObjSizeofVal_NoVal) {
#if LELY_NO_MALLOC
  CHECK_EQUAL(CoObjTHolder::PREALOCATED_OBJ_SIZE, co_obj_sizeof_val(obj));
#else
  CHECK_EQUAL(0, co_obj_sizeof_val(obj));
#endif
}

TEST(CO_ObjSub, CoObjSizeofVal) {
#if LELY_NO_MALLOC
  CHECK_EQUAL(CoObjTHolder::PREALOCATED_OBJ_SIZE, co_obj_sizeof_val(obj));
#else
  CHECK_EQUAL(co_type_sizeof(SUB_DEFTYPE), co_obj_sizeof_val(obj));
#endif
}

TEST(CO_Obj, CoObjGetVal_Null) {
  POINTERS_EQUAL(nullptr, co_obj_get_val(nullptr, 0x00));
}

TEST(CO_Obj, CoObjGetVal_SubNotFound) {
  POINTERS_EQUAL(nullptr, co_obj_get_val(obj, 0x00));
}

TEST(CO_Obj, CoObjSetVal_SubNotFound) {
  const co_unsigned16_t val = 0x4242u;

  const auto ret = co_obj_set_val(obj, 0x00, &val, sizeof(val));

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_ObjSub, CoObjSetVal) {
  const co_unsigned16_t val = 0x4242u;

  const auto bytes_written = co_obj_set_val(obj, SUB_IDX, &val, sizeof(val));

  CHECK_EQUAL(sizeof(val), bytes_written);
  CHECK_EQUAL(val, co_obj_get_val_i16(obj, SUB_IDX));
}

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Obj, CoObjGetVal_##a##_Null) { \
    CHECK_EQUAL(0x0, co_obj_get_val_##c(nullptr, 0x00)); \
  } \
\
  TEST(CO_Obj, CoObjGetVal_##a##_SubNotFound) { \
    CHECK_EQUAL(0x0, co_obj_get_val_##c(obj, 0x00)); \
  } \
\
  TEST(CO_Obj, CoObjSetVal_##a##_SubNotFound) { \
    CHECK_EQUAL(0x0, co_obj_set_val_##c(obj, 0x00, 0x42)); \
  } \
\
  TEST(CO_Obj, CoObjSetVal_##a) { \
    const co_##b##_t val = 0x42; \
    CoSubTHolder sub(SUB_IDX, CO_DEFTYPE_##a); \
    CHECK_EQUAL(0, co_obj_insert_sub(obj, sub.Take())); \
\
    const auto ret = co_obj_set_val_##c(obj, SUB_IDX, val); \
\
    CHECK_EQUAL(sizeof(val), ret); \
    CHECK_EQUAL(val, co_obj_get_val_##c(obj, SUB_IDX)); \
  }
#include <lely/co/def/basic.def>
#undef LELY_CO_DEFINE_TYPE

TEST(CO_ObjSub, CoObjSetDnInd) {
  int data = 0;

  co_obj_set_dn_ind(obj, dn_ind_func, &data);

  co_sub_dn_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_dn_ind(sub, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(pind, dn_ind_func);
  POINTERS_EQUAL(pdata, &data);
}

TEST(CO_ObjSub, CoObjSetDnInd_MultipleSubs) {
  CoSubTHolder sub2_holder(0x42u, CO_DEFTYPE_INTEGER16);
  const auto sub2 = sub2_holder.Take();
  CHECK(sub != nullptr);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub2));
  int data = 0;

  co_obj_set_dn_ind(obj, dn_ind_func, &data);

  co_sub_dn_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_dn_ind(sub, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(pind, dn_ind_func);
  POINTERS_EQUAL(pdata, &data);

  pind = nullptr;
  pdata = nullptr;
  co_sub_get_dn_ind(sub2, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(pind, dn_ind_func);
  POINTERS_EQUAL(pdata, &data);
}

TEST(CO_Obj, CoObjSetDnInd_NoSub) { co_obj_set_dn_ind(obj, nullptr, nullptr); }

TEST(CO_ObjSub, CoObjSetUpInd) {
  int data = 0;

  co_obj_set_up_ind(obj, up_ind_func, &data);

  co_sub_up_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_up_ind(sub, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(pind, up_ind_func);
  POINTERS_EQUAL(pdata, &data);
}

TEST(CO_ObjSub, CoObjSetUpInd_MultipleSubs) {
  CoSubTHolder sub2_holder(0x42u, CO_DEFTYPE_INTEGER16);
  const auto sub2 = sub2_holder.Take();
  CHECK(sub != nullptr);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub2));
  int data = 0;

  co_obj_set_up_ind(obj, up_ind_func, &data);

  co_sub_up_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_up_ind(sub, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(pind, up_ind_func);
  POINTERS_EQUAL(pdata, &data);

  pind = nullptr;
  pdata = nullptr;
  co_sub_get_up_ind(sub2, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(pind, up_ind_func);
  POINTERS_EQUAL(pdata, &data);
}

TEST(CO_Obj, CoObjSetUpInd_NoSub) { co_obj_set_up_ind(obj, nullptr, nullptr); }

TEST_GROUP(CO_SubInit) {
  const co_unsigned8_t SUB_IDX = 0xabu;

  TEST_SETUP() { LelyUnitTest::DisableDiagnosticMessages(); }

  co_sub_t* AcquireCoSubT() {
#if LELY_NO_MALLOC
    return &sub;
#else
    return static_cast<co_sub_t*>(__co_sub_alloc());
#endif
  }

  void ReleaseCoSubT(co_sub_t * sub_) {
#if LELY_NO_MALLOC
    POINTERS_EQUAL(&sub, sub_);
#else
    __co_obj_free(sub_);
#endif
  }

  void DestroyCoSubT(co_sub_t * sub) {
    __co_sub_fini(sub);
    ReleaseCoSubT(sub);
  }

#if LELY_NO_MALLOC
  co_sub_t sub;
#endif
};

TEST(CO_SubInit, CoSubInit) {
  auto* const sub = AcquireCoSubT();
  CHECK(sub != nullptr);

  POINTERS_EQUAL(sub, __co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, NULL));

  POINTERS_EQUAL(nullptr, co_sub_get_obj(sub));
  CHECK_EQUAL(SUB_IDX, co_sub_get_subidx(sub));

  CHECK_EQUAL(CO_DEFTYPE_INTEGER16, co_sub_get_type(sub));

  POINTERS_EQUAL(nullptr, co_sub_get_val(sub));

  CHECK_EQUAL(CO_ACCESS_RW, co_sub_get_access(sub));
  CHECK_EQUAL(0, co_sub_get_pdo_mapping(sub));
  CHECK_EQUAL(0, co_sub_get_flags(sub));

  co_sub_dn_ind_t* pind_dn = nullptr;
  void* pdata_dn = nullptr;
  co_sub_get_dn_ind(sub, &pind_dn, &pdata_dn);
  CHECK(pind_dn != nullptr);
  POINTERS_EQUAL(nullptr, pdata_dn);
#ifndef LELY_NO_CO_OBJ_UPLOAD
  co_sub_up_ind_t* pind_up = nullptr;
  void* pdata_up = nullptr;
  co_sub_get_up_ind(sub, &pind_up, &pdata_up);
  CHECK(pind_up != nullptr);
  POINTERS_EQUAL(nullptr, pdata_up);
#endif

  DestroyCoSubT(sub);
}

#ifndef LELY_NO_CO_OBJ_NAME
TEST(CO_SubInit, CoSubInit_Name) {
  auto* const sub = AcquireCoSubT();
  CHECK(sub != nullptr);

  POINTERS_EQUAL(sub, __co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, NULL));

  POINTERS_EQUAL(nullptr, co_sub_get_name(sub));

  DestroyCoSubT(sub);
}
#endif

#ifndef LELY_NO_CO_OBJ_LIMITS
TEST(CO_SubInit, CoSubInit_Limits) {
  auto* const sub = AcquireCoSubT();
  CHECK(sub != nullptr);

  POINTERS_EQUAL(sub, __co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, NULL));

  CHECK_EQUAL(CO_INTEGER16_MIN,
              *static_cast<const CO_ObjBase::sub_type*>(co_sub_get_min(sub)));
  CHECK_EQUAL(CO_INTEGER16_MAX,
              *static_cast<const CO_ObjBase::sub_type*>(co_sub_get_max(sub)));

  DestroyCoSubT(sub);
}

#if HAVE_LELY_OVERRIDE
TEST(CO_SubInit, CoSubInit_InitValMinFails) {
  auto* const sub = AcquireCoSubT();
  LelyOverride::co_val_init_min(Override::NoneCallsValid);

  POINTERS_EQUAL(nullptr,
                 __co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, NULL));

  ReleaseCoSubT(sub);
}

TEST(CO_SubInit, CoSubInit_InitValMaxFails) {
  auto* const sub = AcquireCoSubT();
  LelyOverride::co_val_init_max(Override::NoneCallsValid);

  POINTERS_EQUAL(nullptr,
                 __co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, NULL));

  ReleaseCoSubT(sub);
}
#endif  // HAVE_LELY_OVERRIDE
#endif  // LELY_NO_CO_OBJ_LIMITS

#ifndef LELY_NO_CO_OBJ_DEFAULT
TEST(CO_SubInit, CoSubInit_Default) {
  auto* const sub = AcquireCoSubT();
  CHECK(sub != nullptr);

  POINTERS_EQUAL(sub, __co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, NULL));

  CHECK_EQUAL(0x0000,
              *static_cast<const CO_ObjBase::sub_type*>(co_sub_get_def(sub)));
  DestroyCoSubT(sub);
}

#if HAVE_LELY_OVERRIDE
TEST(CO_SubInit, CoSubInit_InitValFails) {
  auto* const sub = AcquireCoSubT();
  LelyOverride::co_val_init(Override::NoneCallsValid);

  POINTERS_EQUAL(nullptr,
                 __co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, NULL));

  ReleaseCoSubT(sub);
}
#endif  // HAVE_LELY_OVERRIDE
#endif  // LELY_NO_CO_OBJ_DEFAULT

#ifndef LELY_NO_CO_OBJ_UPLOAD
TEST(CO_SubInit, CoSubInit_Upload) {
  auto* const sub = AcquireCoSubT();
  CHECK(sub != nullptr);

  POINTERS_EQUAL(sub, __co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, NULL));

  co_sub_up_ind_t* pind_up = nullptr;
  void* pdata_up = nullptr;
  co_sub_get_up_ind(sub, &pind_up, &pdata_up);
  CHECK(pind_up != nullptr);
  POINTERS_EQUAL(nullptr, pdata_up);

  DestroyCoSubT(sub);
}
#endif

TEST(CO_SubInit, CoObjFini) {
  auto* const sub = AcquireCoSubT();
  __co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, NULL);
  CoObjTHolder obj(0x1234u);
  CHECK_EQUAL(0, co_obj_insert_sub(obj.Get(), sub));

  DestroyCoSubT(sub);
}

TEST(CO_ObjSub, CoSubPrev) {
  CoSubTHolder sub2_holder(0x42, CO_DEFTYPE_INTEGER16);
  const auto sub2 = sub2_holder.Take();
  CHECK(sub != nullptr);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub2));

  POINTERS_EQUAL(sub2, co_sub_prev(sub));
}

TEST(CO_Sub, CoSubPrev_Null) { POINTERS_EQUAL(nullptr, co_sub_prev(sub)); }

TEST(CO_ObjSub, CoSubPrev_OnlyOne) {
  POINTERS_EQUAL(nullptr, co_sub_prev(sub));
}

TEST(CO_ObjSub, CoSubPrev_Removed) {
  CHECK_EQUAL(0, co_obj_remove_sub(obj, sub));

  POINTERS_EQUAL(nullptr, co_sub_prev(sub));

#if !LELY_NO_MALLOC
  co_sub_destroy(sub);
#endif
}

TEST(CO_ObjSub, CoSubNext) {
  CoSubTHolder sub2_holder(0xcd, CO_DEFTYPE_INTEGER16);
  const auto sub2 = sub2_holder.Take();
  CHECK(sub != nullptr);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub2));

  POINTERS_EQUAL(sub2, co_sub_next(sub));
}

TEST(CO_Sub, CoSubNext_Null) { POINTERS_EQUAL(nullptr, co_sub_next(sub)); }

TEST(CO_ObjSub, CoSubNext_OnlyOne) {
  POINTERS_EQUAL(nullptr, co_sub_next(sub));
}

TEST(CO_ObjSub, CoSubNext_Removed) {
  CHECK_EQUAL(0, co_obj_remove_sub(obj, sub));

  POINTERS_EQUAL(nullptr, co_sub_next(sub));

#if !LELY_NO_MALLOC
  co_sub_destroy(sub);
#endif
}

TEST(CO_ObjSub, CoSubGetObj) { POINTERS_EQUAL(obj, co_sub_get_obj(sub)); }

TEST(CO_ObjSub, CoSubGetSubidx) {
  CHECK_EQUAL(SUB_IDX, co_sub_get_subidx(sub));
}

#ifndef LELY_NO_CO_OBJ_NAME

TEST(CO_Sub, CoSubSetName_Null) {
  CHECK_EQUAL(0, co_sub_set_name(sub, nullptr));

  POINTERS_EQUAL(nullptr, co_sub_get_name(sub));
}

TEST(CO_Sub, CoSubSetName_Empty) {
  const auto ret = co_sub_set_name(sub, "");

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_sub_get_name(sub));
}

TEST(CO_Sub, CoSubSetName) {
  const auto ret = co_sub_set_name(sub, TEST_STR);

  CHECK_EQUAL(0, ret);
  STRCMP_EQUAL(TEST_STR, co_sub_get_name(sub));
}

#endif  // !LELY_NO_CO_OBJ_NAME

TEST(CO_Sub, CoSubGetType) { CHECK_EQUAL(SUB_DEFTYPE, co_sub_get_type(sub)); }

#ifndef LELY_NO_CO_OBJ_LIMITS

TEST(CO_Sub, CoSubAddressofMin_Null) {
  POINTERS_EQUAL(nullptr, co_sub_addressof_min(nullptr));
}

TEST(CO_Sub, CoSubAddressofMin) {
  const auto* const ret = co_sub_addressof_min(sub);

  CHECK(ret != nullptr);
  CHECK_EQUAL(SUB_MIN, *static_cast<const CO_ObjBase::sub_type*>(ret));
}

TEST(CO_Sub, CoSubSizeofMin_Null) {
  CHECK_EQUAL(0, co_sub_sizeof_min(nullptr));
}

TEST(CO_Sub, CoSubSizeofMin) {
  CHECK_EQUAL(co_type_sizeof(SUB_DEFTYPE), co_sub_sizeof_min(sub));
}

TEST(CO_Sub, CoSubGetMin_Null) {
  POINTERS_EQUAL(nullptr, co_sub_get_min(nullptr));
}

TEST(CO_Sub, CoSubSetMin) {
  const CO_ObjBase::sub_type min_val = 0x42;

  const auto ret = co_sub_set_min(sub, &min_val, sizeof(min_val));

  CHECK_EQUAL(sizeof(min_val), ret);
  CHECK_EQUAL(min_val,
              *static_cast<const CO_ObjBase::sub_type*>(co_sub_get_min(sub)));
}

TEST(CO_Sub, CoSubAddressofMax_Null) {
  POINTERS_EQUAL(nullptr, co_sub_addressof_max(nullptr));
}

TEST(CO_Sub, CoSubAddressofMax) {
  const auto* const ret = co_sub_addressof_max(sub);

  CHECK(ret != nullptr);
  CHECK_EQUAL(SUB_MAX, *static_cast<const CO_ObjBase::sub_type*>(ret));
}

TEST(CO_Sub, CoSubSizeofMax_Null) {
  CHECK_EQUAL(0, co_sub_sizeof_max(nullptr));
}

TEST(CO_Sub, CoSubSizeofMax) {
  CHECK_EQUAL(co_type_sizeof(SUB_DEFTYPE), co_sub_sizeof_max(sub));
}

TEST(CO_Sub, CoSubGetMax_Null) {
  POINTERS_EQUAL(nullptr, co_sub_get_max(nullptr));
}

TEST(CO_Sub, CoSubSetMax) {
  const CO_ObjBase::sub_type max_val = 0x42;

  const auto ret = co_sub_set_max(sub, &max_val, sizeof(max_val));

  CHECK_EQUAL(sizeof(max_val), ret);
  CHECK_EQUAL(max_val,
              *static_cast<const CO_ObjBase::sub_type*>(co_sub_get_max(sub)));
}

#endif  // !LELY_NO_CO_OBJ_LIMITS

#ifndef LELY_NO_CO_OBJ_DEFAULT

TEST(CO_Sub, CoSubAddressofDef_Null) {
  POINTERS_EQUAL(nullptr, co_sub_addressof_def(nullptr));
}

TEST(CO_Sub, CoSubAddressofDef) {
  const auto* const ret = co_sub_addressof_def(sub);

  CHECK(ret != nullptr);
  CHECK_EQUAL(SUB_DEF, *static_cast<const CO_ObjBase::sub_type*>(ret));
}

TEST(CO_Sub, CoSubSizeofDef_Null) {
  CHECK_EQUAL(0, co_sub_sizeof_def(nullptr));
}

TEST(CO_Sub, CoSubSizeofDef) {
  CHECK_EQUAL(co_type_sizeof(SUB_DEFTYPE), co_sub_sizeof_def(sub));
}

TEST(CO_Sub, CoSubGetDef_Null) {
  POINTERS_EQUAL(nullptr, co_sub_get_def(nullptr));
}

TEST(CO_Sub, CoSubSetDef) {
  const CO_ObjBase::sub_type def_val = 0x42;

  const auto ret = co_sub_set_def(sub, &def_val, sizeof(def_val));

  CHECK_EQUAL(sizeof(def_val), ret);
  CHECK_EQUAL(def_val,
              *static_cast<const CO_ObjBase::sub_type*>(co_sub_get_def(sub)));
}

#endif  // !LELY_NO_CO_OBJ_DEFAULT

TEST(CO_Sub, CoSubAddressofVal_Null) {
  POINTERS_EQUAL(nullptr, co_sub_addressof_val(nullptr));
}

TEST(CO_ObjSub, CoSubAddressofVal) {
  const auto* const ret = co_sub_addressof_val(sub);

  CHECK(ret != nullptr);
  CHECK_EQUAL(0x0000, *static_cast<const CO_ObjBase::sub_type*>(ret));
}

TEST(CO_Sub, CoSubSizeofVal_Null) {
  CHECK_EQUAL(0, co_sub_sizeof_val(nullptr));
}

TEST(CO_ObjSub, CoSubSizeofVal) {
  CHECK_EQUAL(co_type_sizeof(SUB_DEFTYPE), co_sub_sizeof_val(sub));
}

TEST(CO_Sub, CoSubGetVal_Null) {
  POINTERS_EQUAL(nullptr, co_sub_get_val(nullptr));
}

TEST(CO_ObjSub, CoSubSetVal) {
  const CO_ObjBase::sub_type val = 0x42;

  const auto ret = co_sub_set_val(sub, &val, sizeof(val));

  CHECK_EQUAL(sizeof(val), ret);
  CHECK_EQUAL(val,
              *static_cast<const CO_ObjBase::sub_type*>(co_sub_get_val(sub)));
}

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Obj, CoSubGetVal_##a##_Null) { \
    CHECK_EQUAL(0x0, co_sub_get_val_##c(nullptr)); \
  } \
\
  TEST(CO_Obj, CoSubGetVal_##a##_BadDefType) { \
    CoSubTHolder sub(SUB_IDX, CO_DEFTYPE_##a); \
\
    if (CO_DEFTYPE_##a != CO_DEFTYPE_BOOLEAN) \
      CHECK_EQUAL(0x0, co_sub_get_val_b(sub.Get())); \
    else /* NOLINT(whitespace/newline) */ \
      CHECK_EQUAL(0x0, co_sub_get_val_i16(sub.Get())); \
  } \
\
  TEST(CO_Sub, CoSubGetVal_##a##_SubValNull) { \
    CHECK_EQUAL(0x0, co_sub_get_val_##c(sub)); \
  } \
\
  TEST(CO_Obj, CoSubSetVal_##a##_BadDefType) { \
    const co_##b##_t val = 0x42; \
    CoSubTHolder sub(SUB_IDX, CO_DEFTYPE_##a); \
\
    if (CO_DEFTYPE_##a != CO_DEFTYPE_BOOLEAN) \
      CHECK_EQUAL(0x0, co_sub_set_val_b(sub.Get(), val)); \
    else /* NOLINT(whitespace/newline) */ \
      CHECK_EQUAL(0x0, co_sub_set_val_i16(sub.Get(), val)); \
\
    CHECK_EQUAL(ERRNUM_INVAL, get_errnum()); \
  } \
\
  TEST(CO_Obj, CoSubSetVal_##a) { \
    const co_##b##_t val = 0x42; \
    CoSubTHolder sub_holder(SUB_IDX, CO_DEFTYPE_##a); \
    const auto sub = sub_holder.Take(); \
    CHECK(sub != nullptr); \
    CHECK_EQUAL(0, co_obj_insert_sub(obj, sub)); \
\
    const auto ret = co_sub_set_val_##c(sub, val); \
\
    CHECK_EQUAL(sizeof(val), ret); \
    CHECK_EQUAL(val, co_sub_get_val_##c(sub)); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE

TEST(CO_Obj, CoSubChkVal_NotBasic) {
  CoSubTHolder sub(SUB_IDX, CO_DEFTYPE_DOMAIN);

  CHECK_EQUAL(0, co_sub_chk_val(sub.Get(), CO_DEFTYPE_DOMAIN, nullptr));
}

TEST(CO_Sub, CoSubChkVal_BadDefType) {
  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN,
              co_sub_chk_val(sub, CO_DEFTYPE_BOOLEAN, nullptr));
}

TEST(CO_Sub, CoSubChkVal_BadRange) {
  const CO_ObjBase::sub_type val = 0x0000;
  const CO_ObjBase::sub_type min_val = 0x4242;
  const CO_ObjBase::sub_type max_val = min_val - 1;
  co_sub_set_min(sub, &min_val, sizeof(min_val));
  co_sub_set_max(sub, &max_val, sizeof(max_val));

  CHECK_EQUAL(CO_SDO_AC_PARAM_RANGE, co_sub_chk_val(sub, SUB_DEFTYPE, &val));
}

TEST(CO_Sub, CoSubChkVal_OverMax) {
  const CO_ObjBase::sub_type max_val = 0x0042;
  const CO_ObjBase::sub_type val = max_val + 1;
  co_sub_set_max(sub, &max_val, sizeof(max_val));

  CHECK_EQUAL(CO_SDO_AC_PARAM_HI, co_sub_chk_val(sub, SUB_DEFTYPE, &val));
}

TEST(CO_Sub, CoSubChkVal_UnderMin) {
  const CO_ObjBase::sub_type min_val = 0x0042;
  const CO_ObjBase::sub_type val = min_val - 1;
  co_sub_set_min(sub, &min_val, sizeof(min_val));

  CHECK_EQUAL(CO_SDO_AC_PARAM_LO, co_sub_chk_val(sub, SUB_DEFTYPE, &val));
}

TEST(CO_Sub, CoSubChkVal) {
  const CO_ObjBase::sub_type val = 0x0042;
  const CO_ObjBase::sub_type min_val = 0x0000;
  const CO_ObjBase::sub_type max_val = 0x4242;
  co_sub_set_min(sub, &min_val, sizeof(min_val));
  co_sub_set_max(sub, &max_val, sizeof(max_val));

  CHECK_EQUAL(0, co_sub_chk_val(sub, SUB_DEFTYPE, &val));
}

TEST(CO_Sub, CoSubSetAccess) {
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_RO));
  CHECK_EQUAL(CO_ACCESS_RO, co_sub_get_access(sub));

  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_WO));
  CHECK_EQUAL(CO_ACCESS_WO, co_sub_get_access(sub));

  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_RW));
  CHECK_EQUAL(CO_ACCESS_RW, co_sub_get_access(sub));

  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_RWR));
  CHECK_EQUAL(CO_ACCESS_RWR, co_sub_get_access(sub));

  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_RWW));
  CHECK_EQUAL(CO_ACCESS_RWW, co_sub_get_access(sub));

  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_CONST));
  CHECK_EQUAL(CO_ACCESS_CONST, co_sub_get_access(sub));
}

TEST(CO_Sub, CoSubSetAccess_Invalid) {
  CHECK_EQUAL(-1, co_sub_set_access(sub, 0xff));
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_Sub, CoSubSetPdoMapping) {
  co_sub_set_pdo_mapping(sub, 0xffff);
  CHECK_EQUAL(true, co_sub_get_pdo_mapping(sub));
}

TEST(CO_Sub, CoSubSetFlags) {
  co_sub_set_flags(sub, CO_OBJ_FLAGS_READ | CO_OBJ_FLAGS_MAX_NODEID);
  CHECK_EQUAL(CO_OBJ_FLAGS_READ | CO_OBJ_FLAGS_MAX_NODEID,
              co_sub_get_flags(sub));
}

#ifndef LELY_NO_CO_OBJ_FILE

TEST(CO_Sub, CoSubGetUploadFile_NoFlag) {
  POINTERS_EQUAL(nullptr, co_sub_get_upload_file(sub));
}

TEST(CO_Obj, CoSubGetUploadFile) {
  CoSubTHolder sub_holder(SUB_IDX, CO_DEFTYPE_DOMAIN);
  const auto sub = sub_holder.Take();
  CHECK(sub != nullptr);
  co_sub_set_flags(sub, CO_OBJ_FLAGS_UPLOAD_FILE);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));

#if LELY_NO_MALLOC
  STRCMP_EQUAL("", co_sub_get_upload_file(sub));
#else
  POINTERS_EQUAL(nullptr, co_sub_get_upload_file(sub));
#endif
}

TEST(CO_Sub, CoSubSetUploadFile_NoFlag) {
  CHECK_EQUAL(-1, co_sub_set_upload_file(sub, TEST_STR));
}

#if HAVE_LELY_OVERRIDE
TEST(CO_Obj, CoSubSetUploadFile_SetValFailed) {
  CoSubTHolder sub_holder(SUB_IDX, CO_DEFTYPE_DOMAIN);
  const auto sub = sub_holder.Take();
  CHECK(sub != nullptr);
  co_sub_set_flags(sub, CO_OBJ_FLAGS_UPLOAD_FILE);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  LelyOverride::co_val_make(Override::NoneCallsValid);

  const auto ret = co_sub_set_upload_file(sub, TEST_STR);

  CHECK_EQUAL(-1, ret);
}
#endif

TEST(CO_Obj, CoSubSetUploadFile) {
  CoSubTHolder sub_holder(SUB_IDX, CO_DEFTYPE_DOMAIN);
  const auto sub = sub_holder.Take();
  CHECK(sub != nullptr);
  co_sub_set_flags(sub, CO_OBJ_FLAGS_UPLOAD_FILE);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));

  const auto ret = co_sub_set_upload_file(sub, TEST_STR);

  CHECK_EQUAL(0, ret);
  STRCMP_EQUAL(TEST_STR, co_sub_get_upload_file(sub));
}

TEST(CO_Sub, CoSubGetDownloadFile_NoFlag) {
  POINTERS_EQUAL(nullptr, co_sub_get_download_file(sub));
}

TEST(CO_Obj, CoSubGetDownloadFile) {
  CoSubTHolder sub_holder(SUB_IDX, CO_DEFTYPE_DOMAIN);
  const auto sub = sub_holder.Take();
  CHECK(sub != nullptr);
  co_sub_set_flags(sub, CO_OBJ_FLAGS_DOWNLOAD_FILE);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));

#if LELY_NO_MALLOC
  STRCMP_EQUAL("", co_sub_get_download_file(sub));
#else
  POINTERS_EQUAL(nullptr, co_sub_get_download_file(sub));
#endif
}

TEST(CO_Sub, CoSubSetDownloadFile_NoFlag) {
  CHECK_EQUAL(-1, co_sub_set_download_file(sub, TEST_STR));
}

#if HAVE_LELY_OVERRIDE
TEST(CO_Obj, CoSubSetDownloadFile_SetValFailed) {
  CoSubTHolder sub_holder(SUB_IDX, CO_DEFTYPE_DOMAIN);
  const auto sub = sub_holder.Take();
  CHECK(sub != nullptr);
  co_sub_set_flags(sub, CO_OBJ_FLAGS_DOWNLOAD_FILE);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  LelyOverride::co_val_make(Override::NoneCallsValid);

  const auto ret = co_sub_set_download_file(sub, TEST_STR);

  CHECK_EQUAL(-1, ret);
}
#endif

TEST(CO_Obj, CoSubSetDownloadFile) {
  CoSubTHolder sub_holder(SUB_IDX, CO_DEFTYPE_DOMAIN);
  const auto sub = sub_holder.Take();
  CHECK(sub != nullptr);
  co_sub_set_flags(sub, CO_OBJ_FLAGS_DOWNLOAD_FILE);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));

  const auto ret = co_sub_set_download_file(sub, TEST_STR);

  CHECK_EQUAL(0, ret);
  STRCMP_EQUAL(TEST_STR, co_sub_get_download_file(sub));
}

#endif  // !LELY_NO_CO_OBJ_FILE

TEST(CO_Sub, CoSubGetDnInd_Null) { co_sub_get_dn_ind(sub, nullptr, nullptr); }

TEST(CO_Sub, CoSubSetDnInd_Null) {
  co_sub_set_dn_ind(sub, nullptr, nullptr);

  co_sub_dn_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_dn_ind(sub, &pind, &pdata);
  CHECK(pind != nullptr);
  POINTERS_EQUAL(nullptr, pdata);
}

TEST(CO_Sub, CoSubSetDnInd_NullInd) {
  int data = 0x0;

  co_sub_set_dn_ind(sub, nullptr, &data);

  co_sub_dn_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_dn_ind(sub, &pind, &pdata);
  CHECK(pind != nullptr);
  POINTERS_EQUAL(nullptr, pdata);
}

TEST(CO_ObjSub, CoSubSetDnInd) {
  int data = 0x0;

  co_sub_set_dn_ind(sub, dn_ind_func, &data);

  co_sub_dn_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_dn_ind(sub, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(dn_ind_func, pind);
  POINTERS_EQUAL(&data, pdata);
}

// TODO(sdo): co_sub_on_dn() tests

TEST(CO_Sub, CoSubDnInd_NoSub) {
  CHECK_EQUAL(CO_SDO_AC_NO_SUB, co_sub_dn_ind(nullptr, nullptr));
}

TEST(CO_Sub, CoSubDnInd_NoWriteAccess) {
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_RO));
  CHECK_EQUAL(CO_SDO_AC_NO_WRITE, co_sub_dn_ind(sub, nullptr));
}

TEST(CO_Sub, CoSubDnInd_NoReq) {
  CHECK_EQUAL(CO_SDO_AC_ERROR, co_sub_dn_ind(sub, nullptr));
}

TEST(CO_ObjSub, CoSubDnInd) {
  co_sdo_req req = CO_SDO_REQ_INIT;
  co_sub_set_dn_ind(sub, dn_ind_func, nullptr);

  const auto ret = co_sub_dn_ind(sub, &req);
  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1, CO_ObjSub_Static::dn_ind_func_counter);
}

// TODO(sdo): co_sub_dn_ind_val() tests

// TODO(sdo): co_sub_dn() tests

#ifndef LELY_NO_CO_OBJ_UPLOAD

TEST(CO_Sub, CoSubGetUpInd_Null) { co_sub_get_up_ind(sub, nullptr, nullptr); }

TEST(CO_Sub, CoSubSetUpInd_Null) {
  co_sub_set_up_ind(sub, nullptr, nullptr);

  co_sub_up_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_up_ind(sub, &pind, &pdata);
  CHECK(pind != nullptr);
  POINTERS_EQUAL(nullptr, pdata);
}

TEST(CO_Sub, CoSubSetUpInd_NullInd) {
  int data = 0x0;

  co_sub_set_up_ind(sub, nullptr, &data);

  co_sub_up_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_up_ind(sub, &pind, &pdata);
  CHECK(pind != nullptr);
  POINTERS_EQUAL(nullptr, pdata);
}

TEST(CO_ObjSub, CoSubSetUpInd) {
  int data = 0x0;

  co_sub_set_up_ind(sub, up_ind_func, &data);

  co_sub_up_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_up_ind(sub, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(up_ind_func, pind);
  POINTERS_EQUAL(&data, pdata);
}

#endif  // !LELY_NO_CO_OBJ_UPLOAD

// TODO(sdo): co_sub_on_up() tests

TEST(CO_Sub, CoSubUpInd_NoSub) {
  CHECK_EQUAL(CO_SDO_AC_NO_SUB, co_sub_up_ind(nullptr, nullptr));
}

TEST(CO_Sub, CoSubUpInd_NoReadAccess) {
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_WO));

  CHECK_EQUAL(CO_SDO_AC_NO_READ, co_sub_up_ind(sub, nullptr));
}

TEST(CO_Sub, CoSubUpInd_NoReq) {
  CHECK_EQUAL(CO_SDO_AC_ERROR, co_sub_up_ind(sub, nullptr));
}

TEST(CO_ObjSub, CoSubUpInd) {
  co_sdo_req req = CO_SDO_REQ_INIT;
  co_sub_set_up_ind(sub, up_ind_func, nullptr);

  const auto ret = co_sub_up_ind(sub, &req);
  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1, CO_ObjSub_Static::up_ind_func_counter);
}

// TODO(sdo): co_sub_up_ind_val() tests

// TODO(sdo): co_sub_up() tests
