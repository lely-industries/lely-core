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

#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/sdo.h>
#include <lely/co/val.h>
#include <lely/util/diag.h>

#include <libtest/tools/lely-unit-test.hpp>
#include <libtest/override/lelyco-val.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/sub.hpp"

TEST_GROUP(CO_ObjInit) {
  TEST_SETUP() { LelyUnitTest::DisableDiagnosticMessages(); }

  co_obj_t* AcquireCoObjT() {
#if LELY_NO_MALLOC
    return &object;
#else
    return static_cast<co_obj_t*>(co_obj_alloc());
#endif
  }

  void ReleaseCoObjT(co_obj_t * obj) {
#if LELY_NO_MALLOC
    POINTERS_EQUAL(&object, obj);
#else
    co_obj_free(obj);
#endif
  }

  void DestroyCoObjT(co_obj_t * obj) {
    co_obj_fini(obj);
    ReleaseCoObjT(obj);
  }

#if LELY_NO_MALLOC
  co_obj_t object;
#endif
};

/// @name co_obj_init()
///@{

/// \Given a pointer to the uninitialized object (co_obj_t)
///
/// \When co_obj_init() is called with a valid index and a memory area to store
///       object values
///
/// \Then a pointer to the object is returned, the object is initialized with
///       a given index and an object code is set to CO_OBJECT_VAR
///       \Calls rbnode_init()
///       \Calls rbtree_init()
TEST(CO_ObjInit, CoObjInit_Nominal) {
  auto* const obj = AcquireCoObjT();

  CHECK(obj != nullptr);
  POINTERS_EQUAL(obj, co_obj_init(obj, 0x1234u, NULL, 0));

  POINTERS_EQUAL(nullptr, co_obj_get_dev(obj));
  CHECK_EQUAL(0x1234u, co_obj_get_idx(obj));

#if !LELY_NO_CO_OBJ_NAME
  POINTERS_EQUAL(nullptr, co_obj_get_name(obj));
#endif

  CHECK_EQUAL(CO_OBJECT_VAR, co_obj_get_code(obj));

  POINTERS_EQUAL(nullptr, co_obj_get_val(obj, 0x00));
  CHECK_EQUAL(0, co_obj_sizeof_val(obj));

  DestroyCoObjT(obj);
}
///@}

/// @name co_obj_fini()
///@{

/// \Given a pointer to the initialized object (co_obj_t) inserted into
///        a device
///
/// \When co_obj_fini() is called
///
/// \Then memory allocated for this object is freed and the object is removed
///       from a device
///       \Calls co_dev_remove_obj(), which removes the object from the device
///       \IfCalls{!LELY_NO_MALLOC, co_obj_clear()}
TEST(CO_ObjInit, CoObjFini_Nominal) {
  auto* const obj = AcquireCoObjT();
  co_obj_init(obj, 0x1234u, NULL, 0);
  CoDevTHolder dev(0x01);
  co_dev_insert_obj(dev.Get(), obj);

  co_obj_fini(obj);

  POINTERS_EQUAL(nullptr, co_dev_find_obj(dev.Get(), 0x1234u));

  ReleaseCoObjT(obj);
}

///@}

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

  static co_unsigned32_t dn_ind_func(co_sub_t*, struct co_sdo_req*,
                                     co_unsigned32_t ac, void*) {
    if (ac) return ac;
    ++CO_ObjSub_Static::dn_ind_func_counter;
    return 0;
  }
  static co_unsigned32_t up_ind_func(const co_sub_t*, struct co_sdo_req*,
                                     co_unsigned32_t ac, void*) {
    if (ac) return ac;
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

    CHECK(obj_holder->InsertSub(*sub_holder) != nullptr);

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

    CHECK(obj_holder->InsertSub(*sub_holder) != nullptr);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));
  }

  TEST_TEARDOWN() {
    sub_holder.reset();
    obj_holder.reset();
    dev_holder.reset();
  }
};

/// @name co_obj_prev()
///@{

/// \Given a pointer to the object (co_obj_t) not inserted into a device
///
/// \When co_obj_prev() is called
///
/// \Then a null pointer is returned
///       \Calls rbnode_prev()
TEST(CO_Obj, CoObjPrev_ObjNotInDevice) {
  POINTERS_EQUAL(nullptr, co_obj_prev(obj));
}

/// \Given a pointer to the object (co_obj_t) containing a single sub-object,
///        the object is inserted into a device which does not contain any
///        other objects
///
/// \When co_obj_prev() is called
///
/// \Then a null pointer is returned
///       \Calls rbnode_prev()
TEST(CO_ObjDev, CoObjPrev_SingleObjInDev) {
  POINTERS_EQUAL(nullptr, co_obj_prev(obj));
}

/// \Given a pointer to the object (co_obj_t) containing a single sub-object,
///        the object was removed from a device
///
/// \When co_obj_prev() is called
///
/// \Then a null pointer is returned
///       \Calls rbnode_prev()
TEST(CO_ObjDev, CoObjPrev_Removed) {
  CHECK_EQUAL(0, co_dev_remove_obj(dev, obj));

  POINTERS_EQUAL(nullptr, co_obj_prev(obj));

#if !LELY_NO_MALLOC
  co_obj_destroy(obj);
#endif
}

/// \Given a pointer to the object (co_obj_t) containing a single sub-object,
///        the object is inserted into a device with another object added
///        before it
///
/// \When co_obj_prev() is called
///
/// \Then a pointer to the previous object is returned
///       \Calls rbnode_prev()
TEST(CO_ObjDev, CoObjPrev_WithPreviousObject) {
  CoObjTHolder obj2_holder(0x0001u);
  co_obj_t* obj2 = obj2_holder.Get();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2_holder.Take()));

  POINTERS_EQUAL(obj2, co_obj_prev(obj));
}

///@}

/// @name co_obj_next()
///@{

/// \Given a pointer to the object (co_obj_t) not inserted into a device
///
/// \When co_obj_next() is called
///
/// \Then a null pointer is returned
///       \Calls rbnode_next()
TEST(CO_Obj, CoObjNext_ObjNotInDevice) {
  POINTERS_EQUAL(nullptr, co_obj_next(obj));
}

/// \Given a pointer to the object (co_obj_t) containing a single sub-object,
///        the object is inserted into a device, which does not contain any
///        other object
///
/// \When co_obj_next() is called
///
/// \Then a null pointer is returned
///       \Calls rbnode_next()
TEST(CO_ObjDev, CoObjNext_SingleObjInDev) {
  POINTERS_EQUAL(nullptr, co_obj_next(obj));
}

/// \Given a pointer to the object (co_obj_t) containing a single sub-object,
///        the object is removed from a device
///
/// \When co_obj_next() is called
///
/// \Then a null pointer is returned
///       \Calls rbnode_next()
TEST(CO_ObjDev, CoObjNext_Removed) {
  CHECK_EQUAL(0, co_dev_remove_obj(dev, obj));

  POINTERS_EQUAL(nullptr, co_obj_next(obj));

#if !LELY_NO_MALLOC
  co_obj_destroy(obj);
#endif
}

/// \Given a pointer to the object (co_obj_t) containing a single sub-object,
///        the object is inserted into a device with another object after it
///
/// \When co_obj_next() is called
///
/// \Then a pointer to the next object is returned
///       \Calls rbnode_next()
TEST(CO_ObjDev, CoObjNext_WithNextObject) {
  CoObjTHolder obj2_holder(0x2222u);
  co_obj_t* obj2 = obj2_holder.Get();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2_holder.Take()));

  POINTERS_EQUAL(obj2, co_obj_next(obj));
}

///@}

/// @name co_obj_get_dev()
///@{

/// \Given a pointer to the object (co_obj_t) not inserted into a device
///
/// \When co_obj_get_dev() is called
///
/// \Then a null pointer is returned
TEST(CO_Obj, CoObjGetDev_ObjNotInDevice) {
  POINTERS_EQUAL(nullptr, co_obj_get_dev(obj));
}

/// \Given a pointer to the object (co_obj_t) containing a single sub-object,
///        the object is inserted into a device
///
/// \When co_obj_get_dev() is called
///
/// \Then a pointer to the device is returned
TEST(CO_ObjDev, CoObjGetDev_ObjInDevice) {
  POINTERS_EQUAL(dev, co_obj_get_dev(obj));
}

///@}

/// @name co_obj_get_idx()
///@{

/// \Given a pointer to the object (co_obj_t) with a valid index value
///
/// \When co_obj_get_idx() is called
///
/// \Then the object index is returned
TEST(CO_Obj, CoObjGetIdx_Nominal) { CHECK_EQUAL(OBJ_IDX, co_obj_get_idx(obj)); }

///@}

/// @name co_obj_get_subidx()
///@{

/// \Given a pointer to the object (co_obj_t) without any sub-objects
///
/// \When co_obj_get_subidx() is called with no array for returned sub-indices
///
/// \Then 0 is returned
///       \Calls rbtree_size() and returns its result
TEST(CO_Obj, CoObjGetSubidx_SubIdxArrayNull) {
  CHECK_EQUAL(0, co_obj_get_subidx(obj, 0, nullptr));
}

/// \Given a pointer to the object (co_obj_t) without any sub-objects
///
/// \When co_obj_get_subidx() is called with a memory area to return
///       sub-indices
///
/// \Then 0 is returned and array is not modified
///       \Calls rbtree_first()
///       \Calls rbtree_size() and returns its result
TEST(CO_Obj, CoObjGetSubidx_NoSubObjects) {
  co_unsigned8_t sub_list[1] = {0xffu};

  const auto sub_count = co_obj_get_subidx(obj, 1u, sub_list);

  CHECK_EQUAL(0, sub_count);
  CHECK_EQUAL(0xffu, sub_list[0]);
}

/// \Given a pointer to the object (co_obj_t) containing two sub-objects,
///        the object is inserted into a device
///
/// \When co_obj_get_subidx() is called with a memory area to return
///       one sub-index
///
/// \Then 2 is returned and the array contains the sub-index of the second
///       inserted sub-object
///       \Calls rbtree_first()
///       \Calls rbnode_next()
///       \Calls co_sub_get_subidx()
///       \Calls rbtree_size() and returns its result
TEST(CO_ObjDev, CoObjGetSubidx_WithSubObjects) {
  CoSubTHolder sub2(0x42u, CO_DEFTYPE_INTEGER16);
  CHECK(obj_holder->InsertSub(sub2) != nullptr);
  co_unsigned8_t sub_list[1] = {0};

  const auto sub_count = co_obj_get_subidx(obj, 1u, sub_list);

  CHECK_EQUAL(2u, sub_count);
  CHECK_EQUAL(0x42u, sub_list[0]);
}

///@}

/// @name co_obj_insert_sub()
///@{

/// \Given a pointer to the object (co_obj_t), and another object containing
///        a sub-object
///
/// \When co_obj_insert_sub() is called with a pointer to the sub-object
///       (co_sub_t) from another object
///
/// \Then -1 is returned
TEST(CO_ObjSub, CoObjInsertSub_InsertSubFromOtherObj) {
  CoObjTHolder obj2_holder(0x0001u);
  co_obj_t* const obj2 = obj2_holder.Get();

  CHECK_EQUAL(-1, co_obj_insert_sub(obj2, sub));
}

/// \Given a pointer to the object (co_obj_t) containing a sub-object
///
/// \When co_obj_find_sub() is called with a pointer to the sub-object
///       it already contains
///
/// \Then 0 is returned, the object still contains the sub-object
///       \Calls rbtree_find()
///       \Calls rbtree_insert()
///       \IfCalls{!LELY_NO_MALLOC, co_obj_update()}
TEST(CO_ObjSub, CoObjInsertSub_AlreadyAdded) {
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));

  POINTERS_EQUAL(sub, co_obj_find_sub(obj, SUB_IDX));
}

/// \Given a pointer to the object (co_obj_t) containing a sub-object
///
/// \When co_obj_insert_sub() is called with a pointer to a sub-object
///       (co_sub_t) that has the same sub-index as the sub-object already
///       contained in the object
///
/// \Then -1 is returned and the first sub-object remains in the device
///       \Calls rbtree_find()
TEST(CO_ObjSub, CoObjInsertSub_AlreadyAddedAtSubidx) {
  CoSubTHolder sub2(SUB_IDX, CO_DEFTYPE_INTEGER16);

  CHECK_EQUAL(-1, co_obj_insert_sub(obj, sub2.Get()));

  POINTERS_EQUAL(sub, co_obj_find_sub(obj, SUB_IDX));
}

/// \Given a pointer to the object (co_obj_t)
///
/// \When co_obj_insert_sub() is called with a pointer to a sub-object
///
/// \Then 0 is returned and the sub-object is inserted into the object
///       \Calls rbtree_find()
///       \Calls rbtree_insert()
///       \IfCalls{!LELY_NO_MALLOC, co_obj_update()}
TEST(CO_Obj, CoObjInsertSub_Successful) {
  CoSubTHolder sub2_holder(SUB_IDX, CO_DEFTYPE_INTEGER16);
  const auto sub2 = sub2_holder.Take();
  CHECK(sub2 != nullptr);

  const auto ret = co_obj_insert_sub(obj, sub2);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(sub2, co_obj_find_sub(obj, SUB_IDX));
}

///@}

/// @name co_obj_remove_sub()
///@{

/// \Given a pointer to the object (co_obj_t), and another object containing
///        a sub-object
///
/// \When co_obj_remove_sub() is called with a sub-object from the other object
///
/// \Then -1 is returned and the first object still contains the sub-object
TEST(CO_ObjSub, CoObjRemoveSub_SubInAnotherObj) {
  CoObjTHolder obj2_holder(0x0001u);
  const auto obj2 = obj2_holder.Get();
  CHECK(obj2 != nullptr);

  CHECK_EQUAL(-1, co_obj_remove_sub(obj2, sub));
  POINTERS_EQUAL(sub, co_obj_find_sub(obj, SUB_IDX));
}

/// \Given a pointer to the object (co_obj_t) containing a sub-object
///
/// \When co_obj_remove_sub() is called with a pointer to the sub-object
///       (co_sub_t) contained in the device
///
/// \Then 0 is returned and the sub-object is removed from the object
///       \Calls rbtree_remove()
///       \Calls rbnode_init()
///       \IfCalls{!LELY_NO_MALLOC, co_val_fini()}
///       \IfCalls{!LELY_NO_MALLOC, co_obj_update()}
TEST(CO_ObjSub, CoObjRemoveSub_Successful) {
  CHECK_EQUAL(0, co_obj_remove_sub(obj, sub));

  POINTERS_EQUAL(nullptr, co_obj_find_sub(obj, SUB_IDX));

#if !LELY_NO_MALLOC
  co_sub_destroy(sub);
#endif
}

///@}

/// @name co_obj_find_sub()
///@{

/// \Given a pointer to the object (co_obj_t) containing a sub-object
///
/// \When co_obj_find_sub() is called with a sub-index of the sub-object
///
/// \Then a pointer to the sub-object (co_sub_t) is returned
///       \Calls rbtree_find()
TEST(CO_ObjSub, CoObjFindSub_ObjectContainsSubObject) {
  POINTERS_EQUAL(sub, co_obj_find_sub(obj, SUB_IDX));
}

/// \Given a pointer to the object (co_obj_t) not containing any sub-objects
///
/// \When co_obj_find_sub() is called with a sub-index
///
/// \Then a null pointer is returned
///       \Calls rbtree_find()
TEST(CO_Obj, CoObjFindSub_NotFound) {
  POINTERS_EQUAL(nullptr, co_obj_find_sub(obj, SUB_IDX));
}

///@}

/// @name co_obj_first_sub()
///@{

/// \Given a pointer to the object (co_obj_t) containing a sub-object
///
/// \When co_obj_first_sub() is called
///
/// \Then a pointer to the sub-object is returned
///       \Calls rbtree_find()
TEST(CO_ObjSub, CoObjFirstSub_Nominal) {
  POINTERS_EQUAL(sub, co_obj_first_sub(obj));
}

/// \Given a pointer to the object (co_obj_t) not containing any sub-objects
///
/// \When co_obj_first_sub() is called
///
/// \Then a null pointer is returned
///       \Calls rbtree_find()
TEST(CO_Obj, CoObjFirstSub_Empty) {
  POINTERS_EQUAL(nullptr, co_obj_first_sub(obj));
}

///@}

/// @name co_obj_last_sub()
///@{

/// \Given a pointer to the object (co_obj_t) containing a sub-object
///
/// \When co_obj_last_sub() is called
///
/// \Then a pointer to the sub-object is returned
///       \Calls rbtree_last()
TEST(CO_ObjSub, CoObjLastSub_Nominal) {
  POINTERS_EQUAL(sub, co_obj_last_sub(obj));
}

/// \Given a pointer to the object (co_obj_t) not containing any sub-objects
///
/// \When co_obj_last_sub() is called
///
/// \Then a null pointer is returned
///       \Calls rbtree_last()
TEST(CO_Obj, CoObjLastSub_Empty) {
  POINTERS_EQUAL(nullptr, co_obj_last_sub(obj));
}

///@}

#if !LELY_NO_CO_OBJ_NAME

TEST(CO_Obj, CoObjSetName_Null) {
  CHECK_EQUAL(0, co_obj_set_name(obj, nullptr));

  POINTERS_EQUAL(nullptr, co_obj_get_name(obj));
}

TEST(CO_Obj, CoObjSetName_Empty) {
  const auto ret = co_obj_set_name(obj, "");

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_obj_get_name(obj));
}

TEST(CO_Obj, CoObjSetName_Nominal) {
  const auto ret = co_obj_set_name(obj, TEST_STR);

  CHECK_EQUAL(0, ret);
  STRCMP_EQUAL(TEST_STR, co_obj_get_name(obj));
}

#endif  // !LELY_NO_CO_OBJ_NAME

/// @name co_obj_set_code()
///@{

/// \Given a pointer to the object (co_obj_t)
///
/// \When co_obj_set_code() is called with one of the following:
///         - CO_OBJECT_NULL
///         - CO_OBJECT_DOMAIN
///         - CO_OBJECT_DEFTYPE
///         - CO_OBJECT_DEFSTRUCT
///         - CO_OBJECT_VAR
///         - CO_OBJECT_ARRAY
///         - CO_OBJECT_RECORD
///
/// \Then 0 is returned and the requested object code is set
TEST(CO_Obj, CoObjSetCode_Nominal) {
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

/// \Given a pointer to the object (co_obj_t)
///
/// \When co_obj_set_code() is called with an invalid code
///
/// \Then -1 is returned, ERRNUM_INVAL error number set
TEST(CO_Obj, CoObjSetCode_Invalid) {
  auto const ret = co_obj_set_code(obj, 0xffu);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(CO_OBJECT_VAR, co_obj_get_code(obj));
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

///@}

/// @name co_obj_addressof()
///@{

/// \Given a null pointer to the object (co_obj_t)
///
/// \When co_obj_addressof_val() is called
///
/// \Then a null pointer is returned
TEST(CO_Obj, CoObjAddressofVal_Null) {
  co_obj_t* const obj = nullptr;

  POINTERS_EQUAL(nullptr, co_obj_addressof_val(obj));
}
/// \Given a pointer to the object (co_obj_t) initialized with a null
///        pointer as a memory area to store its values
///
/// \When co_obj_addressof_val() is called
///
/// \Then a null pointer is returned
TEST(CO_Obj, CoObjAddressofVal_NoVal) {
#if LELY_NO_MALLOC
  co_obj_init(obj, OBJ_IDX, nullptr, 0);
#endif

  POINTERS_EQUAL(nullptr, co_obj_addressof_val(obj));
}

/// \Given a pointer to the object (co_obj_t) containing a sub-object with
///        a set value
///
/// \When co_obj_addressof_val() is called
///
/// \Then a non-null pointer is returned
TEST(CO_ObjSub, CoObjAddressofVal_Nominal) {
  const sub_type val = 0x4242u;
  CHECK_EQUAL(sizeof(val), co_obj_set_val(obj, SUB_IDX, &val, sizeof(val)));

  CHECK(co_obj_addressof_val(obj) != nullptr);
}

///@}

/// @name co_obj_sizeof_val()
///@{

/// \Given a null pointer to the object (co_obj_t)
///
/// \When co_obj_sizeof_val() is called
///
/// \Then 0 is returned
TEST(CO_Obj, CoObjSizeofVal_Null) {
  co_obj_t* const obj = nullptr;

  CHECK_EQUAL(0, co_obj_sizeof_val(obj));
}

/// \Given a pointer to the empty object (co_obj_t)
///
/// \When co_obj_sizeof_val() is called
///
/// \Then 0 is returned
TEST(CO_Obj, CoObjSizeofVal_NoVal) { CHECK_EQUAL(0, co_obj_sizeof_val(obj)); }

/// \Given a pointer to the object (co_obj_t) containing the sub-object
///        of a selected type
///
/// \When co_obj_sizeof_val() is called
///
/// \Then a size of the selected type is returned
TEST(CO_ObjSub, CoObjSizeofVal_Nominal) {
  CHECK_EQUAL(co_type_sizeof(SUB_DEFTYPE), co_obj_sizeof_val(obj));
}

///@}

/// @name co_obj_get_val()
///@{

/// \Given a null pointer to the object (co_obj_t)
///
/// \When co_obj_get_val() is called with a sub-index
///
/// \Then a null pointer is returned
///       \Calls co_sub_get_val()
TEST(CO_Obj, CoObjGetVal_Null) {
  co_obj_t* const obj = nullptr;

  POINTERS_EQUAL(nullptr, co_obj_get_val(obj, 0x00));
}

/// \Given a pointer to the object (co_obj_t) without any sub-objects
///
/// \When co_obj_get_val() is called with a sub-index
///
/// \Then a null pointer is returned
///       \Calls co_obj_find_sub()
///       \Calls co_sub_get_val()
TEST(CO_Obj, CoObjGetVal_SubNotFound) {
  POINTERS_EQUAL(nullptr, co_obj_get_val(obj, 0x00u));
}

///@}

/// @name co_obj_set_val()
///@{

/// \Given a pointer to the object (co_obj_t) without any sub-objects
///
/// \When co_obj_set_val() is called with a sub-index, a memory area
///       containing a value
///
/// \Then 0 is returned and the value is not set
///       \Calls co_obj_find_sub()
///       \Calls set_errnum()
TEST(CO_Obj, CoObjSetVal_SubNotFound) {
  const sub_type val = 0x4242u;

  const auto ret = co_obj_set_val(obj, 0x00u, &val, sizeof(val));

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given a pointer to the object (co_obj_t) containing a sub-object
///
/// \When co_obj_set_val() is called with a subindex, a memory area
///       containing a value
///
/// \Then the value size is returned and the requested value is set
///       \Calls co_obj_find_sub()
///       \Calls co_sub_set_val()
TEST(CO_ObjSub, CoObjSetVal_Nominal) {
  const sub_type val = 0x4242u;

  const auto bytes_written = co_obj_set_val(obj, SUB_IDX, &val, sizeof(val));

  CHECK_EQUAL(sizeof(val), bytes_written);
  CHECK_EQUAL(val, co_obj_get_val_i16(obj, SUB_IDX));
}

///@}

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
    CHECK(obj_holder->InsertSub(sub) != nullptr); \
\
    const auto ret = co_obj_set_val_##c(obj, SUB_IDX, val); \
\
    CHECK_EQUAL(sizeof(val), ret); \
    CHECK_EQUAL(val, co_obj_get_val_##c(obj, SUB_IDX)); \
  }
#include <lely/co/def/basic.def>
#undef LELY_CO_DEFINE_TYPE

/// @name co_obj_set_dn_ind()
///@{

/// \Given a pointer to the object (co_obj_t) containing a sub-object
///
/// \When co_obj_set_dn_ind() is called with an indicator function and
///       a pointer to the user-specified data
///
/// \Then the requested indicator and the data pointer are set
///       \Calls co_sub_set_dn_ind()
TEST(CO_ObjSub, CoObjSetDnInd_Nominal) {
  int data = 0;

  co_obj_set_dn_ind(obj, dn_ind_func, &data);

  co_sub_dn_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_dn_ind(sub, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(pind, dn_ind_func);
  POINTERS_EQUAL(pdata, &data);
}

/// \Given a pointer to the object (co_obj_t) containing many sub-objects
///
/// \When co_obj_set_dn_ind() is called with an indicator function and
///       a pointer to the user-specified data
///
/// \Then the requested indicator and the data pointer are set
///       \Calls co_sub_set_dn_ind()
TEST(CO_ObjSub, CoObjSetDnInd_MultipleSubs) {
  CoSubTHolder sub2_holder(0x42u, CO_DEFTYPE_INTEGER16);
  CHECK(sub2_holder.Get() != nullptr);
  const auto* sub2 = obj_holder->InsertSub(sub2_holder);
  CHECK(sub2 != nullptr);
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

/// \Given a pointer to the object (co_obj_t) without any sub-object
///
/// \When co_obj_set_dn_ind() is called with null pointers
///
/// \Then nothing is changed
TEST(CO_Obj, CoObjSetDnInd_NoSub) { co_obj_set_dn_ind(obj, nullptr, nullptr); }

///@}

#if !LELY_NO_CO_OBJ_UPLOAD

/// @name co_obj_set_up_ind()
///@{

/// \Given a pointer to the object (co_obj_t) containing a sub-object
///
/// \When co_obj_set_up_ind() is called with an upload indicator function and
///       a pointer to the user-specified data
///
/// \Then the requested indicator and the data pointer are set
///       \Calls co_sub_set_up_ind()
TEST(CO_ObjSub, CoObjSetUpInd_Nominal) {
  int data = 0;

  co_obj_set_up_ind(obj, up_ind_func, &data);

  co_sub_up_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_up_ind(sub, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(pind, up_ind_func);
  POINTERS_EQUAL(pdata, &data);
}

/// \Given a pointer to the object (co_obj_t) containing many sub-objects
///
/// \When co_obj_set_up_ind() is called with an upload indicator function and
///       a pointer to the user-specified data
///
/// \Then the requested indicator and the data pointer are set
///       \Calls co_sub_set_up_ind()
TEST(CO_ObjSub, CoObjSetUpInd_MultipleSubs) {
  CoSubTHolder sub2_holder(0x42u, CO_DEFTYPE_INTEGER16);
  CHECK(sub2_holder.Get() != nullptr);
  const auto* sub2 = obj_holder->InsertSub(sub2_holder);
  CHECK(sub2 != nullptr);
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

/// \Given a pointer to the object (co_obj_t)
///
/// \When co_obj_set_up_ind() is called with null pointers
///
/// \Then nothing is changed
TEST(CO_Obj, CoObjSetUpInd_NoSub) { co_obj_set_up_ind(obj, nullptr, nullptr); }

///@}

#endif  // !LELY_NO_CO_OBJ_UPLOAD

TEST_GROUP(CO_SubInit) {
  const co_unsigned8_t SUB_IDX = 0xabu;

  TEST_SETUP() { LelyUnitTest::DisableDiagnosticMessages(); }

  co_sub_t* AcquireCoSubT() {
#if LELY_NO_MALLOC
    return &sub;
#else
    return static_cast<co_sub_t*>(co_sub_alloc());
#endif
  }

  void ReleaseCoSubT(co_sub_t * sub_) {
#if LELY_NO_MALLOC
    POINTERS_EQUAL(&sub, sub_);
#else
    co_obj_free(sub_);
#endif
  }

  void DestroyCoSubT(co_sub_t * sub) {
    co_sub_fini(sub);
    ReleaseCoSubT(sub);
  }

#if LELY_NO_MALLOC
  co_sub_t sub;
#endif
};

/// @name co_sub_init()
///@{

/// \Given a pointer to the uninitialized sub-object (co_sub_t)
///
/// \When co_sub_init() is called with a subindex, a data type and no memory
///       area to store values
///
/// \Then a pointer to the sub-object (co_sub_t) is returned, the sub-object
///       is initialized with the requested sub-index, type, value null pointer
///       and all other default values
///       \Calls rbnode_init()
///       \IfCalls{!LELY_NO_CO_OBJ_LIMITS, co_val_init_min()}
///       \IfCalls{!LELY_NO_CO_OBJ_LIMITS, co_val_init_max()}
///       \IfCalls{!LELY_NO_CO_OBJ_DEFAULT, co_val_init()}
TEST(CO_SubInit, CoSubInit_Nominal) {
  auto* const sub = AcquireCoSubT();
  CHECK(sub != nullptr);

  POINTERS_EQUAL(sub, co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, NULL));

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
#if !LELY_NO_CO_OBJ_UPLOAD
  co_sub_up_ind_t* pind_up = nullptr;
  void* pdata_up = nullptr;
  co_sub_get_up_ind(sub, &pind_up, &pdata_up);
  CHECK(pind_up != nullptr);
  POINTERS_EQUAL(nullptr, pdata_up);
#endif

  DestroyCoSubT(sub);
}

#if !LELY_NO_CO_OBJ_NAME
/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_init() is called with a sub-index, a data type and no memory
///       area to store values
///
/// \Then the sub-object name is set to a null pointer
///       \Calls rbnode_init()
///       \IfCalls{!LELY_NO_CO_OBJ_LIMITS, co_val_init_min()}
///       \IfCalls{!LELY_NO_CO_OBJ_LIMITS, co_val_init_max()}
///       \IfCalls{!LELY_NO_CO_OBJ_DEFAULT, co_val_init()}
TEST(CO_SubInit, CoSubInit_Name) {
  auto* const sub = AcquireCoSubT();
  CHECK(sub != nullptr);

  POINTERS_EQUAL(sub, co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, nullptr));

  POINTERS_EQUAL(nullptr, co_sub_get_name(sub));

  DestroyCoSubT(sub);
}
#endif

#if !LELY_NO_CO_OBJ_LIMITS
/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_init() is called with a sub-index, a data type and no memory
///       area to store values
///
/// \Then a pointer to the sub-object is returned, sub-object lower and upper
///       limits are set to default values
///       \Calls rbnode_init()
///       \IfCalls{!LELY_NO_CO_OBJ_LIMITS, co_val_init_min()}
///       \IfCalls{!LELY_NO_CO_OBJ_LIMITS, co_val_init_max()}
///       \IfCalls{!LELY_NO_CO_OBJ_DEFAULT, co_val_init()}
TEST(CO_SubInit, CoSubInit_Limits) {
  auto* const sub = AcquireCoSubT();
  CHECK(sub != nullptr);

  POINTERS_EQUAL(sub, co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, NULL));

  CHECK_EQUAL(CO_INTEGER16_MIN,
              *static_cast<const CO_ObjBase::sub_type*>(co_sub_get_min(sub)));
  CHECK_EQUAL(CO_INTEGER16_MAX,
              *static_cast<const CO_ObjBase::sub_type*>(co_sub_get_max(sub)));

  DestroyCoSubT(sub);
}

#if HAVE_LELY_OVERRIDE
/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_init() is called with a sub-index, a data type and no memory
///       area to store values, but co_val_init_min() fails
///
/// \Then a null pointer is returned
///       \Calls rbnode_init()
///       \IfCalls{!LELY_NO_CO_OBJ_LIMITS, co_val_init_min()}
TEST(CO_SubInit, CoSubInit_InitValMinFails) {
  auto* const sub = AcquireCoSubT();
  LelyOverride::co_val_init_min(Override::NoneCallsValid);

  POINTERS_EQUAL(nullptr,
                 co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, NULL));

  ReleaseCoSubT(sub);
}

/// \Given a pointer to the uninitialized sub-object (co_sub_t)
///
/// \When co_sub_init() is called with a sub-index, a data type and no memory
///       area to store values, but co_val_init_max() fails
///
/// \Then a null pointer is returned
///       \Calls rbnode_init()
///       \IfCalls{!LELY_NO_CO_OBJ_LIMITS, co_val_init_min()}
///       \IfCalls{!LELY_NO_CO_OBJ_LIMITS, co_val_init_max()}
TEST(CO_SubInit, CoSubInit_InitValMaxFails) {
  auto* const sub = AcquireCoSubT();
  LelyOverride::co_val_init_max(Override::NoneCallsValid);

  POINTERS_EQUAL(nullptr,
                 co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, NULL));

  ReleaseCoSubT(sub);
}
#endif  // HAVE_LELY_OVERRIDE
#endif  // LELY_NO_CO_OBJ_LIMITS

#if !LELY_NO_CO_OBJ_DEFAULT
/// \Given a pointer to the uninitialized sub-object (co_sub_t)
///
/// \When co_sub_init() is called with a sub-index, a data type and no memory
///       area to store values
///
/// \Then a pointer to sub-object (co_sub_t) is returned, the default value is
///       set
///       \Calls rbnode_init()
///       \IfCalls{!LELY_NO_CO_OBJ_LIMITS, co_val_init_min()}
///       \IfCalls{!LELY_NO_CO_OBJ_LIMITS, co_val_init_max()}
///       \IfCalls{!LELY_NO_CO_OBJ_DEFAULT, co_val_init()}
TEST(CO_SubInit, CoSubInit_Default) {
  auto* const sub = AcquireCoSubT();
  CHECK(sub != nullptr);

  POINTERS_EQUAL(sub, co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, NULL));

  CHECK_EQUAL(0x0000,
              *static_cast<const CO_ObjBase::sub_type*>(co_sub_get_def(sub)));
  DestroyCoSubT(sub);
}

#if HAVE_LELY_OVERRIDE
/// \Given a pointer to the uninitialized sub-object (co_sub_t)
///
/// \When co_sub_init() is called with a sub-index, a data type and no memory
///       area to store values, but co_val_init() fails
///
/// \Then a null pointer is returned
///       \Calls rbnode_init()
///       \IfCalls{!LELY_NO_CO_OBJ_LIMITS, co_val_init_min()}
///       \IfCalls{!LELY_NO_CO_OBJ_LIMITS, co_val_init_max()}
///       \IfCalls{!LELY_NO_CO_OBJ_DEFAULT, co_val_init()}
TEST(CO_SubInit, CoSubInit_InitValFails) {
  auto* const sub = AcquireCoSubT();
  LelyOverride::co_val_init(Override::NoneCallsValid);

  POINTERS_EQUAL(nullptr,
                 co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, NULL));

  ReleaseCoSubT(sub);
}
#endif  // HAVE_LELY_OVERRIDE
#endif  // LELY_NO_CO_OBJ_DEFAULT

#if !LELY_NO_CO_OBJ_UPLOAD
/// \Given a pointer to the uninitialized sub-object (co_sub_t)
///
/// \When co_sub_init() is called with a sub-index, a data type and no memory
///       area to store values
///
/// \Then a pointer to sub-object (co_sub_t) is returned, the default upload
///       indicator is set
///       \Calls rbnode_init()
///       \IfCalls{!LELY_NO_CO_OBJ_LIMITS, co_val_init_min()}
///       \IfCalls{!LELY_NO_CO_OBJ_LIMITS, co_val_init_max()}
///       \IfCalls{!LELY_NO_CO_OBJ_DEFAULT, co_val_init()}
TEST(CO_SubInit, CoSubInit_Upload) {
  auto* const sub = AcquireCoSubT();
  CHECK(sub != nullptr);

  POINTERS_EQUAL(sub, co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, NULL));

  co_sub_up_ind_t* pind_up = nullptr;
  void* pdata_up = nullptr;
  co_sub_get_up_ind(sub, &pind_up, &pdata_up);
  CHECK(pind_up != nullptr);
  POINTERS_EQUAL(nullptr, pdata_up);

  DestroyCoSubT(sub);
}
#endif

///@}

/// @name co_sub_fini()
///@{

/// \Given a pointer to the sub-object (co_sub_t) inserted into the object
///
/// \When co_sub_fini() is called
///
/// \Then the sub-object is finalized and removed from the object
///       \Calls co_obj_remove_sub()
///       \IfCalls{!LELY_NO_CO_OBJ_DEFAULT, co_val_fini()}
///       \IfCalls{!LELY_NO_CO_OBJ_LIMITS, co_val_fini()}
TEST(CO_SubInit, CoSubFini_Nominal) {
  auto* const sub = AcquireCoSubT();
  co_sub_init(sub, SUB_IDX, CO_DEFTYPE_INTEGER16, NULL);
  CoObjTHolder obj(0x1234u);
  CHECK_EQUAL(0, co_obj_insert_sub(obj.Get(), sub));

  co_sub_fini(sub);

  POINTERS_EQUAL(nullptr, co_obj_find_sub(obj.Get(), SUB_IDX));

  ReleaseCoSubT(sub);
}

///@}

/// @name co_sub_prev()
///@{

/// \Given a pointer to the sub-object (co_sub_t) inserted into the object with
///        another sub-object before it
///
/// \When co_sub_prev() is called
///
/// \Then a pointer to the other sub-object (co_sub_t) is returned
///       \Calls rbnode_prev()
TEST(CO_ObjSub, CoSubPrev_Nominal) {
  CoSubTHolder sub2_holder(0x42, CO_DEFTYPE_INTEGER16);
  const auto sub2 = sub2_holder.Take();
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub2));

  POINTERS_EQUAL(sub2, co_sub_prev(sub));
}

/// \Given a pointer to the sub-object (co_sub_t) not inserted into any object
///
/// \When co_sub_prev() is called
///
/// \Then a null pointer is returned
///       \Calls rbnode_prev()
TEST(CO_Sub, CoSubPrev_ObjNull) { POINTERS_EQUAL(nullptr, co_sub_prev(sub)); }

/// \Given a pointer to the only sub-object (co_sub_t) inserted into the object
///
/// \When co_sub_prev() is called
///
/// \Then a null pointer is returned
///       \Calls rbnode_prev()
TEST(CO_ObjSub, CoSubPrev_SingleSubInObj) {
  POINTERS_EQUAL(nullptr, co_sub_prev(sub));
}

/// \Given a pointer to the sub-object (co_sub_t) which was removed from
///        the object
///
/// \When co_sub_prev() is called
///
/// \Then a null pointer is returned
///       \Calls rbnode_prev()
TEST(CO_ObjSub, CoSubPrev_Removed) {
  CHECK_EQUAL(0, co_obj_remove_sub(obj, sub));

  POINTERS_EQUAL(nullptr, co_sub_prev(sub));

#if !LELY_NO_MALLOC
  co_sub_destroy(sub);
#endif
}

///@}

/// @name co_sub_next()
///@{

/// \Given a pointer to the sub-object (co_sub_t) inserted into the object with
///        another sub-object after it
///
/// \When co_sub_next() is called
///
/// \Then a pointer to the other sub-object (co_sub_t) is returned
///       \Calls rbnode_next()
TEST(CO_ObjSub, CoSubNext_Nominal) {
  CoSubTHolder sub2_holder(0xcd, CO_DEFTYPE_INTEGER16);
  CHECK(sub2_holder.Get() != nullptr);
  const auto* sub2 = obj_holder->InsertSub(sub2_holder);
  CHECK(sub2 != nullptr);

  POINTERS_EQUAL(sub2, co_sub_next(sub));
}

/// \Given a pointer to the sub-object (co_sub_t) not inserted into any object
///
/// \When co_sub_next() is called
///
/// \Then a null pointer is returned
///       \Calls rbnode_next()
TEST(CO_Sub, CoSubNext_ObjNull) { POINTERS_EQUAL(nullptr, co_sub_next(sub)); }

/// \Given a pointer to the only sub-object (co_sub_t) inserted into the object
///
/// \When co_sub_next() is called
///
/// \Then a null pointer is returned
///       \Calls rbnode_next()
TEST(CO_ObjSub, CoSubNext_SingleSubInObj) {
  POINTERS_EQUAL(nullptr, co_sub_next(sub));
}

/// \Given a pointer to the sub-object (co_sub_t) which was removed from
///        the object
///
/// \When co_sub_next() is called
///
/// \Then a null pointer is returned
///       \Calls rbnode_next()
TEST(CO_ObjSub, CoSubNext_Removed) {
  CHECK_EQUAL(0, co_obj_remove_sub(obj, sub));

  POINTERS_EQUAL(nullptr, co_sub_next(sub));

#if !LELY_NO_MALLOC
  co_sub_destroy(sub);
#endif
}

///@}

/// @name co_sub_get_obj()
///@{

/// \Given a pointer to the sub-object (co_sub_t) inserted into object
///
/// \When co_sub_get_obj() is called
///
/// \Then a pointer to the object (co_obj_t) is returned
TEST(CO_ObjSub, CoSubGetObj_Nominal) {
  POINTERS_EQUAL(obj, co_sub_get_obj(sub));
}

///@}

/// @name co_sub_get_subidx()
///@{

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_get_subidx() is called
///
/// \Then a sub-index is returned
TEST(CO_ObjSub, CoSubGetSubidx_Nominal) {
  CHECK_EQUAL(SUB_IDX, co_sub_get_subidx(sub));
}

///@}

/// @name co_sub_set_name()
///@{

#if !LELY_NO_CO_OBJ_NAME

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_set_name() is called with a null pointer
///
/// \Then 0 is returned and the sub-object name is set to a null pointer
TEST(CO_Sub, CoSubSetName_Null) {
  const auto ret = co_sub_set_name(sub, nullptr);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_sub_get_name(sub));
}

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_set_name() is called with an empty string
///
/// \Then 0 is returned and the sub-object name is set to a null pointer
///       \Calls set_errc()
TEST(CO_Sub, CoSubSetName_Empty) {
  const auto ret = co_sub_set_name(sub, "");

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_sub_get_name(sub));
}

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_set_name() is called with a string
///
/// \Then 0 is returned and the sub-object name is set to the string
///       \Calls set_errc()
TEST(CO_Sub, CoSubSetName_Nominal) {
  const auto ret = co_sub_set_name(sub, TEST_STR);

  CHECK_EQUAL(0, ret);
  STRCMP_EQUAL(TEST_STR, co_sub_get_name(sub));
}

#endif  // !LELY_NO_CO_OBJ_NAME
///@}

/// @name co_sub_get_type()
///@{

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_get_type() is called
///
/// \Then the sub-object data type is returned
TEST(CO_Sub, CoSubGetType_Nominal) {
  CHECK_EQUAL(SUB_DEFTYPE, co_sub_get_type(sub));
}

///@}

/// @name co_sub_addressof_min()
///@{
#if !LELY_NO_CO_OBJ_LIMITS

/// \Given a null pointer to the sub-object (co_sub_t)
///
/// \When co_sub_addressof_min() is called
///
/// \Then a null pointer is returned
///       \Calls co_val_addressof()
TEST(CO_Sub, CoSubAddressofMin_Null) {
  co_sub_t* const sub = nullptr;

  POINTERS_EQUAL(nullptr, co_sub_addressof_min(sub));
}

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_addressof_min() is called
///
/// \Then a pointer to the value of lower limit of the sub-object's value is
///       returned
///       \Calls co_val_addressof()
TEST(CO_Sub, CoSubAddressofMin_Nominal) {
  const auto* const ret = co_sub_addressof_min(sub);

  CHECK(ret != nullptr);
  CHECK_EQUAL(SUB_MIN, *static_cast<const CO_ObjBase::sub_type*>(ret));
}

///@}

/// @name co_sub_sizeof_min()
///@{

/// \Given a null pointer to the sub-object (co_sub_t)
///
/// \When co_sub_sizeof_min() is called
///
/// \Then 0 is returned
///       \Calls co_val_sizeof()
TEST(CO_Sub, CoSubSizeofMin_Null) {
  co_sub_t* const sub = nullptr;

  CHECK_EQUAL(0, co_sub_sizeof_min(sub));
}

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_sizeof_min()) is called
///
/// \Then the size of the value of the lower limit of the sub-object value is
///       returned
///       \Calls co_val_sizeof()
TEST(CO_Sub, CoSubSizeofMin_Nominal) {
  CHECK_EQUAL(co_type_sizeof(SUB_DEFTYPE), co_sub_sizeof_min(sub));
}

///@}

/// @name co_sub_get_min()
///@{

/// \Given a null pointer to the sub-object (co_sub_t)
///
/// \When co_sub_get_min() is called
///
/// \Then a null pointer is returned
TEST(CO_Sub, CoSubGetMin_Null) {
  co_sub_t* const sub = nullptr;

  POINTERS_EQUAL(nullptr, co_sub_get_min(sub));
}

///@}

/// @name co_sub_set_min()
///@{

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_set_min() is called with a memory area containing the lower
///       limit value
///
/// \Then the size of the lower limit value is returned and the limit is set
///       \Calls co_val_fini()
///       \Calls co_val_make()
TEST(CO_Sub, CoSubSetMin_Nominal) {
  const CO_ObjBase::sub_type min_val = 0x42;

  const auto ret = co_sub_set_min(sub, &min_val, sizeof(min_val));

  CHECK_EQUAL(sizeof(min_val), ret);
  CHECK_EQUAL(min_val,
              *static_cast<const CO_ObjBase::sub_type*>(co_sub_get_min(sub)));
}

///@}

/// @name co_sub_addressof_max()
///@{

/// \Given a null pointer to the sub-object (co_sub_t)
///
/// \When co_sub_addressof_max() is called
///
/// \Then a null pointer is returned
TEST(CO_Sub, CoSubAddressofMax_Null) {
  co_sub_t* const sub = nullptr;

  POINTERS_EQUAL(nullptr, co_sub_addressof_max(sub));
}

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_addressof_max() is called
///
/// \Then a pointer to the value of the upper limit of the sub-object value is
///       returned
///       \Calls co_val_addressof()
TEST(CO_Sub, CoSubAddressofMax_Nominal) {
  const auto* const ret = co_sub_addressof_max(sub);

  CHECK(ret != nullptr);
  CHECK_EQUAL(SUB_MAX, *static_cast<const CO_ObjBase::sub_type*>(ret));
}

///@}

/// @name co_sub_sizeof_max()
///@{

/// \Given a null pointer to the sub-object (co_sub_t)
///
/// \When co_sub_sizeof_max() is called
///
/// \Then 0 is returned
TEST(CO_Sub, CoSubSizeofMax_Null) {
  co_sub_t* const sub = nullptr;

  CHECK_EQUAL(0, co_sub_sizeof_max(sub));
}

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_sizeof_max() is called
///
/// \Then the size of the value of the upper limit of the sub-object value is
///       returned
///       /Calls co_val_sizeof()
TEST(CO_Sub, CoSubSizeofMax_Nominal) {
  CHECK_EQUAL(co_type_sizeof(SUB_DEFTYPE), co_sub_sizeof_max(sub));
}

///@}

/// @name co_sub_get_max()
///@{

/// \Given a null pointer to the sub-object (co_sub_t)
///
/// \When co_sub_get_max() is called
///
/// \Then a null pointer is returned
TEST(CO_Sub, CoSubGetMax_Null) {
  co_sub_t* const sub = nullptr;

  POINTERS_EQUAL(nullptr, co_sub_get_max(sub));
}

///@}

/// @name co_sub_set_max()
///@{

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_set_max() is called with a memory area containing the upper
///       limit value
///
/// \Then the size of the upper limit value is returned and the limit is set
///       /Calls co_val_fini()
///       /Calls co_val_make()
TEST(CO_Sub, CoSubSetMax_Nominal) {
  const CO_ObjBase::sub_type max_val = 0x42;

  const auto ret = co_sub_set_max(sub, &max_val, sizeof(max_val));

  CHECK_EQUAL(sizeof(max_val), ret);
  CHECK_EQUAL(max_val,
              *static_cast<const CO_ObjBase::sub_type*>(co_sub_get_max(sub)));
}

///@}

#endif  // !LELY_NO_CO_OBJ_LIMITS

#if !LELY_NO_CO_OBJ_DEFAULT

/// @name co_sub_addressof_def()
///@{

/// \Given a null pointer to the sub-object (co_sub_t)
///
/// \When co_sub_addressof_def() is called
///
/// \Then a null pointer is returned
TEST(CO_Sub, CoSubAddressofDef_Null) {
  co_sub_t* const sub = nullptr;

  POINTERS_EQUAL(nullptr, co_sub_addressof_def(sub));
}

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_addressof_def() is called
///
/// \Then a pointer to the default value of the sub-object's value is returned
///       \Calls co_val_addressof()
TEST(CO_Sub, CoSubAddressofDef_Nominal) {
  const auto* const ret = co_sub_addressof_def(sub);

  CHECK(ret != nullptr);
  CHECK_EQUAL(SUB_DEF, *static_cast<const CO_ObjBase::sub_type*>(ret));
}

///@}

/// @name co_sub_sizeof_def()
///@{

/// \Given a null pointer to the sub-object (co_sub_t)
///
/// \When co_sub_sizeof_def() is called
///
/// \Then 0 is returned
TEST(CO_Sub, CoSubSizeofDef_Null) {
  co_sub_t* const sub = nullptr;

  CHECK_EQUAL(0, co_sub_sizeof_def(sub));
}

///@}

/// @name co_type_sizeof_def()
///@{

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_type_sizeof_def() is called
///
/// \Then the size of the default value of the sub-object value is returned
TEST(CO_Sub, CoSubSizeofDef_Nominal) {
  CHECK_EQUAL(co_type_sizeof(SUB_DEFTYPE), co_sub_sizeof_def(sub));
}

///@}

/// @name co_sub_get_def()
///@{

/// \Given a null pointer to the sub-object (co_sub_t)
///
/// \When co_sub_get_def() is called
///
/// \Then a null pointer is returned
TEST(CO_Sub, CoSubGetDef_Null) {
  co_sub_t* const sub = nullptr;

  POINTERS_EQUAL(nullptr, co_sub_get_def(sub));
}

///@}

/// @name co_sub_set_def()
///@{

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_set_def() is called with a memory area containing the default
///       value
///
/// \Then the size of the default value is returned and the default value is
///       set
///       \Calls co_val_fini()
///       \Calls co_val_make()
TEST(CO_Sub, CoSubSetDef_Nominal) {
  const CO_ObjBase::sub_type def_val = 0x42;

  const auto ret = co_sub_set_def(sub, &def_val, sizeof(def_val));

  CHECK_EQUAL(sizeof(def_val), ret);
  CHECK_EQUAL(def_val,
              *static_cast<const CO_ObjBase::sub_type*>(co_sub_get_def(sub)));
}

///@}

#endif  // !LELY_NO_CO_OBJ_DEFAULT

/// @name co_sub_addressof_val()
///@{

/// \Given a null pointer to the sub-object (co_sub_t)
///
/// \When co_sub_addressof_val() is called
///
/// \Then a null pointer is returned
TEST(CO_Sub, CoSubAddressofVal_Null) {
  co_sub_t* const sub = nullptr;

  POINTERS_EQUAL(nullptr, co_sub_addressof_val(sub));
}

/// \Given a pointer to the sub-object (co_sub_t) inserted into the object
///
/// \When co_sub_addressof_val() is called
///
/// \Then a pointer to the sub-object value is returned
///       \Calls co_val_addressof()
TEST(CO_ObjSub, CoSubAddressofVal_Nominal) {
  const auto* const ret = co_sub_addressof_val(sub);

  CHECK(ret != nullptr);
  CHECK_EQUAL(0x0000, *static_cast<const CO_ObjBase::sub_type*>(ret));
}

///@}

/// @name co_sub_sizeof_val()
///@{

/// \Given a null pointer to the sub-object (co_sub_t)
///
/// \When co_sub_sizeof_val() is called
///
/// \Then 0 is returned
TEST(CO_Sub, CoSubSizeofVal_Null) {
  co_sub_t* const sub = nullptr;

  CHECK_EQUAL(0, co_sub_sizeof_val(sub));
}

/// \Given a pointer to the sub-object (co_sub_t) inserted into the object
///
/// \When co_sub_sizeof_val() is called
///
/// \Then the size of the sub-object value is returned
TEST(CO_ObjSub, CoSubSizeofVal_Nominal) {
  CHECK_EQUAL(co_type_sizeof(SUB_DEFTYPE), co_sub_sizeof_val(sub));
}

///@}

/// @name co_sub_get_val()
///@{

/// \Given a null pointer to the sub-object (co_sub_t)
///
/// \When co_sub_get_val() is called
///
/// \Then a null pointer is returned
TEST(CO_Sub, CoSubGetVal_Null) {
  co_sub_t* const sub = nullptr;

  POINTERS_EQUAL(nullptr, co_sub_get_val(sub));
}

///@}

/// @name co_sub_set_val()
///@{

/// \Given a pointer to the sub-object (co_sub_t) inserted into the object
///
/// \When co_sub_set_val() is called with a memory area containing a value
///
/// \Then the size of the sub-object value is returned and the value is set
///       \Calls co_val_fini()
///       \Calls co_val_make()
TEST(CO_ObjSub, CoSubSetVal_Nominal) {
  const CO_ObjBase::sub_type val = 0x42;

  const auto ret = co_sub_set_val(sub, &val, sizeof(val));

  CHECK_EQUAL(sizeof(val), ret);
  CHECK_EQUAL(val,
              *static_cast<const CO_ObjBase::sub_type*>(co_sub_get_val(sub)));
}

///@}

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
  } \
\
  TEST(CO_Obj, CoSubSetVal_##a) { \
    const co_##b##_t val = 0x42; \
    CoSubTHolder sub_holder(SUB_IDX, CO_DEFTYPE_##a); \
    auto* const sub = obj_holder->InsertSub(sub_holder); \
    CHECK(sub != nullptr); \
\
    const auto ret = co_sub_set_val_##c(sub, val); \
\
    CHECK_EQUAL(sizeof(val), ret); \
    CHECK_EQUAL(val, co_sub_get_val_##c(sub)); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE

/// @name co_sub_chk_val()
///@{

/// \Given a pointer to the sub-object (co_sub_t) of a non-basic type
///
/// \When co_sub_chk_val() is called with the sub-object type and a null pointer
///
/// \Then 0 is returned
///       \Calls co_type_is_basic()
TEST(CO_Obj, CoSubChkVal_NotBasicType) {
  CoSubTHolder sub_holder(SUB_IDX, CO_DEFTYPE_DOMAIN);
  co_sub_t* const sub = sub_holder.Get();

  const auto ret = co_sub_chk_val(sub, CO_DEFTYPE_DOMAIN, nullptr);

  CHECK_EQUAL(0, ret);
}

/// \Given a pointer to the sub-object (co_sub_t) of a basic type
///
/// \When co_sub_chk_val() is called with an incorrect type and a null pointer
///
/// \Then CO_SDO_AC_TYPE_LEN abort code is returned
///       \Calls co_type_is_basic()
TEST(CO_Sub, CoSubChkVal_BadDefType) {
  const auto ret = co_sub_chk_val(sub, CO_DEFTYPE_BOOLEAN, nullptr);

  CHECK_EQUAL(CO_SDO_AC_TYPE_LEN, ret);
}

/// \Given a pointer to the sub-object (co_sub_t) with limits set incorrectly
///        (the upper limit below the lower limit
///
/// \When co_sub_chk_val() is called with the sub-object type and a memory area
///       containing a value
///
/// \Then CO_SDO_AC_PARAM_RANGE abort code is returned
///       \Calls co_type_is_basic()
///       \Calls co_val_cmp()
TEST(CO_Sub, CoSubChkVal_BadRange) {
  const CO_ObjBase::sub_type val = 0x0000;
  const CO_ObjBase::sub_type min_val = 0x4242;
  const CO_ObjBase::sub_type max_val = min_val - 1;
  co_sub_set_min(sub, &min_val, sizeof(min_val));
  co_sub_set_max(sub, &max_val, sizeof(max_val));

  const auto ret = co_sub_chk_val(sub, SUB_DEFTYPE, &val);

  CHECK_EQUAL(CO_SDO_AC_PARAM_RANGE, ret);
}

/// \Given a pointer to the sub-object (co_sub_t) with the upper limit set
///
/// \When co_sub_chk_val() is called with the sub-object type and a memory area
///       containing a value above the upper limit
///
/// \Then CO_SDO_AC_PARAM_HI abort code is returned
///       \Calls co_type_is_basic()
///       \Calls co_val_cmp()
TEST(CO_Sub, CoSubChkVal_OverMax) {
  const CO_ObjBase::sub_type max_val = 0x0042;
  const CO_ObjBase::sub_type val = max_val + 1;
  co_sub_set_max(sub, &max_val, sizeof(max_val));

  const auto ret = co_sub_chk_val(sub, SUB_DEFTYPE, &val);

  CHECK_EQUAL(CO_SDO_AC_PARAM_HI, ret);
}

/// \Given a pointer to the sub-object (co_sub_t) with the lower limit set
///
/// \When co_sub_chk_val() is called with the sub-object type and a memory area
///       containing a value below the lower limit
///
/// \Then CO_SDO_AC_PARAM_LO abort code is returned
///       \Calls co_type_is_basic()
///       \Calls co_val_cmp()
TEST(CO_Sub, CoSubChkVal_UnderMin) {
  const CO_ObjBase::sub_type min_val = 0x0042;
  const CO_ObjBase::sub_type val = min_val - 1;
  co_sub_set_min(sub, &min_val, sizeof(min_val));

  const auto ret = co_sub_chk_val(sub, SUB_DEFTYPE, &val);

  CHECK_EQUAL(CO_SDO_AC_PARAM_LO, ret);
}

/// \Given a pointer to the sub-object (co_sub_t) with the upper and the lower
///        limits set
///
/// \When co_sub_chk_val() is called with the sub-object type and a memory area
///       containing a value within limits
///
/// \Then 0 is returned
///       \Calls co_type_is_basic()
///       \Calls co_val_cmp()
TEST(CO_Sub, CoSubChkVal_Nominal) {
  const CO_ObjBase::sub_type val = 0x0042;
  const CO_ObjBase::sub_type min_val = 0x0000;
  const CO_ObjBase::sub_type max_val = 0x4242;
  co_sub_set_min(sub, &min_val, sizeof(min_val));
  co_sub_set_max(sub, &max_val, sizeof(max_val));

  const auto ret = co_sub_chk_val(sub, SUB_DEFTYPE, &val);

  CHECK_EQUAL(0, ret);
}

///@}

/// @name co_sub_set_access()
///@{

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_set_access() is called with one of the following:
///         - CO_ACCESS_RO
///         - CO_ACCESS_WO
///         - CO_ACCESS_RW
///         - CO_ACCESS_RWR
///         - CO_ACCESS_RWW
///         - CO_ACCESS_CONST
///
/// \Then 0 is returned and requested access code is set
TEST(CO_Sub, CoSubSetAccess_Nominal) {
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

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_set_access() is called with an invalid access type
///
/// \Then -1 is returned, ERRNUM_INVAL error number is set
///       \Calls set_errnum()
TEST(CO_Sub, CoSubSetAccess_Invalid) {
  const auto ret = co_sub_set_access(sub, 0xff);

  CHECK_EQUAL(-1, ret);

  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

///@}

/// @name co_sub_set_pdo_mapping()
///@{

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_set_pdo_mapping() is called with a non-zero integer
///
/// \Then PDO mapping of the sub-object is enabled
TEST(CO_Sub, CoSubSetPdoMapping_Nominal) {
  co_sub_set_pdo_mapping(sub, 1u);

  CHECK_EQUAL(true, co_sub_get_pdo_mapping(sub));
}

///@}

/// @name co_sub_set_flags()
///@{

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_set_flags() is called with a set of object flags
///
/// \Then the requested flags are set
TEST(CO_Sub, CoSubSetFlags_Nominal) {
  co_sub_set_flags(sub, CO_OBJ_FLAGS_READ | CO_OBJ_FLAGS_MAX_NODEID);

  CHECK_EQUAL(CO_OBJ_FLAGS_READ | CO_OBJ_FLAGS_MAX_NODEID,
              co_sub_get_flags(sub));
}

///@}

#if !LELY_NO_CO_OBJ_FILE

TEST(CO_Sub, CoSubGetUploadFile_NoFlag) {
  POINTERS_EQUAL(nullptr, co_sub_get_upload_file(sub));
}

TEST(CO_Obj, CoSubGetUploadFile_Nominal) {
  CoSubTHolder sub_holder(SUB_IDX, CO_DEFTYPE_DOMAIN);
  auto* const sub = sub_holder.Get();
  CHECK(sub != nullptr);
  co_sub_set_flags(sub, CO_OBJ_FLAGS_UPLOAD_FILE);
  CHECK(obj_holder->InsertSub(sub_holder) != nullptr);

  const auto ret = co_sub_get_upload_file(sub);

#if LELY_NO_MALLOC
  STRCMP_EQUAL("", ret);
#else
  POINTERS_EQUAL(nullptr, ret);
#endif
}

TEST(CO_Sub, CoSubSetUploadFile_NoFlag) {
  CHECK_EQUAL(-1, co_sub_set_upload_file(sub, TEST_STR));
}

#if HAVE_LELY_OVERRIDE
TEST(CO_Obj, CoSubSetUploadFile_SetValFailed) {
  CoSubTHolder sub_holder(SUB_IDX, CO_DEFTYPE_DOMAIN);
  auto* const sub = sub_holder.Get();
  CHECK(sub != nullptr);
  co_sub_set_flags(sub, CO_OBJ_FLAGS_UPLOAD_FILE);
  CHECK(obj_holder->InsertSub(sub_holder) != nullptr);
  LelyOverride::co_val_make(Override::NoneCallsValid);

  const auto ret = co_sub_set_upload_file(sub, TEST_STR);

  CHECK_EQUAL(-1, ret);
}
#endif

TEST(CO_Obj, CoSubSetUploadFile_Nominal) {
  CoSubTHolder sub_holder(SUB_IDX, CO_DEFTYPE_DOMAIN);
  auto* const sub = sub_holder.Get();
  CHECK(sub != nullptr);
  co_sub_set_flags(sub, CO_OBJ_FLAGS_UPLOAD_FILE);
  CHECK(obj_holder->InsertSub(sub_holder) != nullptr);

  const auto ret = co_sub_set_upload_file(sub, TEST_STR);

  CHECK_EQUAL(0, ret);
  STRCMP_EQUAL(TEST_STR, co_sub_get_upload_file(sub));
}

TEST(CO_Sub, CoSubGetDownloadFile_NoFlag) {
  POINTERS_EQUAL(nullptr, co_sub_get_download_file(sub));
}

TEST(CO_Obj, CoSubGetDownloadFile_Nominal) {
  CoSubTHolder sub_holder(SUB_IDX, CO_DEFTYPE_DOMAIN);
  auto* const sub = sub_holder.Get();
  CHECK(sub != nullptr);
  co_sub_set_flags(sub, CO_OBJ_FLAGS_DOWNLOAD_FILE);
  CHECK(obj_holder->InsertSub(sub_holder) != nullptr);

  const auto ret = co_sub_get_download_file(sub);

#if LELY_NO_MALLOC
  STRCMP_EQUAL("", ret);
#else
  POINTERS_EQUAL(nullptr, ret);
#endif
}

TEST(CO_Sub, CoSubSetDownloadFile_NoFlag) {
  CHECK_EQUAL(-1, co_sub_set_download_file(sub, TEST_STR));
}

#if HAVE_LELY_OVERRIDE
TEST(CO_Obj, CoSubSetDownloadFile_SetValFailed) {
  CoSubTHolder sub_holder(SUB_IDX, CO_DEFTYPE_DOMAIN);
  auto* const sub = sub_holder.Get();
  CHECK(sub != nullptr);
  co_sub_set_flags(sub, CO_OBJ_FLAGS_DOWNLOAD_FILE);
  CHECK(obj_holder->InsertSub(sub_holder) != nullptr);
  LelyOverride::co_val_make(Override::NoneCallsValid);

  const auto ret = co_sub_set_download_file(sub, TEST_STR);

  CHECK_EQUAL(-1, ret);
}
#endif

TEST(CO_Obj, CoSubSetDownloadFile_Nominal) {
  CoSubTHolder sub_holder(SUB_IDX, CO_DEFTYPE_DOMAIN);
  auto* const sub = sub_holder.Get();
  CHECK(sub != nullptr);
  co_sub_set_flags(sub, CO_OBJ_FLAGS_DOWNLOAD_FILE);
  CHECK(obj_holder->InsertSub(sub_holder) != nullptr);

  const auto ret = co_sub_set_download_file(sub, TEST_STR);

  CHECK_EQUAL(0, ret);
  STRCMP_EQUAL(TEST_STR, co_sub_get_download_file(sub));
}

#endif  // !LELY_NO_CO_OBJ_FILE

/// @name co_sub_get_dn_ind()
///@{

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_get_dn_ind() is called with null pointers
///
/// \Then nothing is changed
TEST(CO_Sub, CoSubGetDnInd_Null) { co_sub_get_dn_ind(sub, nullptr, nullptr); }

///@}

/// @name co_sub_set_dn_ind()
///@{

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_set_dn_ind() is called with null pointers
///
/// \Then the sub-object download indication functions is set to the default
///       indication function, the custom data is set to a null pointer
TEST(CO_Sub, CoSubSetDnInd_Null) {
  co_sub_set_dn_ind(sub, nullptr, nullptr);

  co_sub_dn_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_dn_ind(sub, &pind, &pdata);
  CHECK(pind != nullptr);
  POINTERS_EQUAL(nullptr, pdata);
}

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_set_dn_ind() is called with a null pointer and a non-null
///       pointer to the user-specified data
///
/// \Then the sub-object download indication function is set to the default
///       indication function, the custom data is set to a null pointer
TEST(CO_Sub, CoSubSetDnInd_NullInd) {
  int data = 0x0;

  co_sub_set_dn_ind(sub, nullptr, &data);

  co_sub_dn_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_dn_ind(sub, &pind, &pdata);
  CHECK(pind != nullptr);
  POINTERS_EQUAL(nullptr, pdata);
}

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_set_dn_ind() is called with a pointer to the download
///       indication function and a non-null pointer to the user-specified data
///
/// \Then the sub-object download indication and user-specified data are set to
///       requested values
TEST(CO_ObjSub, CoSubSetDnInd_Nominal) {
  int data = 0x0;

  co_sub_set_dn_ind(sub, dn_ind_func, &data);

  co_sub_dn_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_dn_ind(sub, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(dn_ind_func, pind);
  POINTERS_EQUAL(&data, pdata);
}

///@}

/// @name co_sub_on_dn()
///@{

// TODO(sdo): co_sub_on_dn() tests

///@}

/// @name co_sub_dn_ind()
///@{

/// \Given a null pointer to the sub-object (co_sub_t)
///
/// \When co_sub_dn_ind() is called with no SDO download request
///
/// \Then CO_SDO_AC_NO_SUB is returned
TEST(CO_Sub, CoSubDnInd_NoSub) {
  co_sub_t* const sub = nullptr;
  co_sdo_req* const req = nullptr;

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, co_sub_dn_ind(sub, req, 0));
}

/// \Given a pointer to the sub-object (co_sub_t) with read-only access
///
/// \When co_sub_dn_ind() is called with no SDO download request
///
/// \Then CO_SDO_AC_NO_WRITE abort code is returned
TEST(CO_Sub, CoSubDnInd_NoWriteAccess) {
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_RO));

  CHECK_EQUAL(CO_SDO_AC_NO_WRITE, co_sub_dn_ind(sub, nullptr, 0));
}

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_dn_ind() is called with no SDO download request
///
/// \Then CO_SDO_AC_ERROR abort code is returned
TEST(CO_Sub, CoSubDnInd_NoReq) {
  co_sdo_req* const req = nullptr;

  CHECK_EQUAL(CO_SDO_AC_ERROR, co_sub_dn_ind(sub, req, 0));
}

/// \Given a pointer to sub-object (co_sub_t) with a custom download indication
///        function set
///
/// \When co_sub_dn_ind() is called with an SDO download request
///
/// \Then 0 is returned and the indication function is called
///       \Calls co_sub_default_dn_ind()
TEST(CO_ObjSub, CoSubDnInd_Nominal) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  co_sub_set_dn_ind(sub, dn_ind_func, nullptr);

  const auto ret = co_sub_dn_ind(sub, &req, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1, CO_ObjSub_Static::dn_ind_func_counter);
}

///@}

/// @name co_sub_dn_ind_val()
///@{

// TODO(sdo): co_sub_dn_ind_val() tests

///@}

/// @name co_sub_dn()
///@{

// TODO(sdo): co_sub_dn() tests

///@}

/// @name co_sub_get_up_ind()
///@{

#if !LELY_NO_CO_OBJ_UPLOAD

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_get_up_ind() is called with null pointers
///
/// \Then nothing is changed
TEST(CO_Sub, CoSubGetUpInd_Null) { co_sub_get_up_ind(sub, nullptr, nullptr); }

///@}

/// @name co_sub_set_up_ind()
///@{

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_set_up_ind() is called with null pointers
///
/// \Then the sub-object upload indication functions is set to the default
///       indication function, the user-specified data is set to a null pointer
TEST(CO_Sub, CoSubSetUpInd_Null) {
  co_sub_set_up_ind(sub, nullptr, nullptr);

  co_sub_up_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_up_ind(sub, &pind, &pdata);
  CHECK(pind != nullptr);
  POINTERS_EQUAL(nullptr, pdata);
}

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_set_up_ind() is called with a null pointer and a non-null
///       pointer to the user-specified data
///
/// \Then the sub-object upload indication functions is set to the default
///       indication function, the user-specified data is set to a null
///       pointer
TEST(CO_Sub, CoSubSetUpInd_NullInd) {
  int data = 0x0;

  co_sub_set_up_ind(sub, nullptr, &data);

  co_sub_up_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_up_ind(sub, &pind, &pdata);
  CHECK(pind != nullptr);
  POINTERS_EQUAL(nullptr, pdata);
}

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_set_up_ind() is called with a pointer to the upload indication
///       function and a non-null pointer to the user-specified data
///
/// \Then the sub-object upload indication and the user-specified data are set
///       to requested values
TEST(CO_ObjSub, CoSubSetUpInd_Nominal) {
  int data = 0;

  co_sub_set_up_ind(sub, up_ind_func, &data);

  co_sub_up_ind_t* pind = nullptr;
  void* pdata = nullptr;
  co_sub_get_up_ind(sub, &pind, &pdata);
  FUNCTIONPOINTERS_EQUAL(up_ind_func, pind);
  POINTERS_EQUAL(&data, pdata);
}

///@}

#endif  // !LELY_NO_CO_OBJ_UPLOAD

/// @name co_sub_on_up()
///@{

// TODO(sdo): co_sub_on_up() tests

///@}

/// @name co_sub_up_ind()
///@{

/// \Given a null pointer to the sub-object (co_sub_t)
///
/// \When co_sub_up_ind() function is called with no SDO upload request
///
/// \Then CO_SDO_AC_NO_SUB abort code is returned
TEST(CO_Sub, CoSubUpInd_NoSub) {
  co_sub_t* const sub = nullptr;
  co_sdo_req* const req = nullptr;

  CHECK_EQUAL(CO_SDO_AC_NO_SUB, co_sub_up_ind(sub, req, 0));
}

/// \Given a pointer to the sub-object (co_sub_t) with write-only access
///
/// \When co_sub_up_ind() is called with no SDO upload request
///
/// \Then CO_SDO_AC_NO_READ abort code is returned
TEST(CO_Sub, CoSubUpInd_NoReadAccess) {
  CHECK_EQUAL(0, co_sub_set_access(sub, CO_ACCESS_WO));

  CHECK_EQUAL(CO_SDO_AC_NO_READ, co_sub_up_ind(sub, nullptr, 0));
}

/// \Given a pointer to the sub-object (co_sub_t)
///
/// \When co_sub_up_ind() is called with no SDO upload request
///
/// \Then CO_SDO_AC_ERROR abort code is returned
TEST(CO_Sub, CoSubUpInd_NoReq) {
  CHECK_EQUAL(CO_SDO_AC_ERROR, co_sub_up_ind(sub, nullptr, 0));
}

/// \Given a pointer to sub-object (co_sub_t) with a custom upload indication
///        function set
///
/// \When co_sub_up_ind() is called with an SDO upload request
///
/// \Then 0 is returned and the indication function is called
///       \Calls co_sub_default_up_ind()
TEST(CO_ObjSub, CoSubUpInd_Nominal) {
  co_sdo_req req = CO_SDO_REQ_INIT(req);
  co_sub_set_up_ind(sub, up_ind_func, nullptr);

  const auto ret = co_sub_up_ind(sub, &req, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(1u, CO_ObjSub_Static::up_ind_func_counter);
}

///@}

// TODO(sdo): co_sub_up_ind_val() tests

// TODO(sdo): co_sub_up() tests
