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

#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <array>

#include <CppUTest/TestHarness.h>

#include <lely/can/net.h>
#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/val.h>
#include <lely/co/tpdo.h>
#include <lely/util/error.h>

#include <libtest/override/lelyco-val.hpp>
#include <libtest/tools/lely-cpputest-ext.hpp>
#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"
#include "holder/obj.hpp"
#include "holder/sub.hpp"
#include "holder/array-init.hpp"

static void
CheckBuffers(const uint_least8_t* buf1, const uint_least8_t* buf2,
             const size_t n) {
  for (size_t i = 0; i < n; ++i) {
    const std::string check_text = "buf[" + std::to_string(i) + "]";
    CHECK_EQUAL_TEXT(buf2[i], buf1[i], check_text.c_str());
  }
}

TEST_GROUP(CO_DevInit) {
  co_dev_t* AcquireCoDevT() {
#if LELY_NO_MALLOC
    return &device;
#else
    return static_cast<co_dev_t*>(co_dev_alloc());
#endif
  }

  void ReleaseCoDevT(co_dev_t* const dev) {
#if LELY_NO_MALLOC
    POINTERS_EQUAL(&device, dev);
#else
    co_dev_free(dev);
#endif
  }

  void CheckDevAfterInit(co_dev_t* const dev) const {
    CHECK_EQUAL(0, co_dev_get_netid(dev));
    CHECK_EQUAL(0, co_dev_get_idx(dev, 0, nullptr));
    CHECK_EQUAL(0, co_dev_get_vendor_id(dev));
    CHECK_EQUAL(0, co_dev_get_product_code(dev));
    CHECK_EQUAL(0, co_dev_get_revision(dev));
    CHECK_EQUAL(0, co_dev_get_baud(dev));
    CHECK_EQUAL(0, co_dev_get_rate(dev));
    CHECK_EQUAL(0, co_dev_get_lss(dev));
    CHECK_EQUAL(0, co_dev_get_dummy(dev));
  }

#if !LELY_NO_CO_OBJ_NAME
  void CheckDefaultNames(co_dev_t* const dev) const {
    POINTERS_EQUAL(nullptr, co_dev_get_name(dev));
    POINTERS_EQUAL(nullptr, co_dev_get_vendor_name(dev));
    POINTERS_EQUAL(nullptr, co_dev_get_product_name(dev));
    POINTERS_EQUAL(nullptr, co_dev_get_order_code(dev));
  }
#endif

  void DestroyCoDevT(co_dev_t* const dev) {
    co_dev_fini(dev);
    ReleaseCoDevT(dev);
  }

#if LELY_NO_MALLOC
  co_dev_t device;
#endif

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();
    set_errnum(ERRNUM_SUCCESS);
  }
};

#if !LELY_NO_MALLOC
/// @name co_dev_alloc()
///@{

/// \Given N/A
///
/// \When co_dev_alloc() is called
///
/// \Then a pointer to a newly allocated and uninitialized device (co_dev_t)
///       is returned
///       \Calls malloc()
///       \Calls set_errc_from_errno()
TEST(CO_DevInit, CoDevAllocFree_Nominal) {
  void* const ptr = co_dev_alloc();

  CHECK(ptr != nullptr);

  co_dev_free(ptr);
}
///@}
#endif  // !LELY_NO_MALLOC

/// @name co_dev_init()
///@{

/// \Given a pointer to an uninitialized device (co_dev_t)
///
/// \When co_dev_init() is called with a valid node-ID
///
/// \Then the device is initialized with the default values and the requested
///       node-ID and ERRNUM_SUCCESS error number is set
///       \Calls rbtree_init()
TEST(CO_DevInit, CoDevInit_Nominal) {
  auto* const dev = AcquireCoDevT();

  CHECK(dev != nullptr);
  POINTERS_EQUAL(dev, co_dev_init(dev, 0x01u));

  CheckDevAfterInit(dev);
  CHECK_EQUAL(0x01u, co_dev_get_id(dev));
#if !LELY_NO_CO_OBJ_NAME
  CheckDefaultNames(dev);
#endif  // !LELY_NO_CO_OBJ_NAME

  CHECK_EQUAL(ERRNUM_SUCCESS, get_errnum());

  DestroyCoDevT(dev);
}

/// \Given a pointer to an uninitialized device (co_dev_t)
///
/// \When co_dev_init() is called with 0xff
///
/// \Then the device is initialized with the default values and the requested
///       node-ID and ERRNUM_SUCCESS error number is set
///       \Calls rbtree_init()
TEST(CO_DevInit, CoDevInit_UnconfiguredId) {
  auto* const dev = AcquireCoDevT();

  CHECK(dev != nullptr);
  POINTERS_EQUAL(dev, co_dev_init(dev, 0xffu));
  CheckDevAfterInit(dev);
  CHECK_EQUAL(0xffu, co_dev_get_id(dev));
#if !LELY_NO_CO_OBJ_NAME
  CheckDefaultNames(dev);
#endif  // !LELY_NO_CO_OBJ_NAME

  CHECK_EQUAL(ERRNUM_SUCCESS, get_errnum());

  DestroyCoDevT(dev);
}

/// \Given a pointer to an uninitialized device (co_dev_t)
///
/// \When co_dev_init() is called with zero
///
/// \Then a null pointer is returned and ERRNUM_INVAL error number is set
///       \Calls set_errnum()
TEST(CO_DevInit, CoDevInit_ZeroId) {
  auto* const dev = AcquireCoDevT();

  CHECK(dev != nullptr);
  POINTERS_EQUAL(nullptr, co_dev_init(dev, 0x00u));

  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());

  ReleaseCoDevT(dev);
}

/// \Given a pointer to an uninitialized device (co_dev_t)
///
/// \When co_dev_init() is called with an invalid node-ID
///
/// \Then a null pointer is returned and ERRNUM_INVAL error number is set
///       \Calls set_errnum()
TEST(CO_DevInit, CoDevInit_InvalidId) {
  auto* const dev = AcquireCoDevT();
  CHECK(dev != nullptr);

  POINTERS_EQUAL(nullptr, co_dev_init(dev, CO_NUM_NODES + 1));
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());

  POINTERS_EQUAL(nullptr, co_dev_init(dev, 0xff - 1));
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());

  ReleaseCoDevT(dev);
}

///@}

/// @name co_dev_fini()
///@{

/// \Given a pointer to an initialized device (co_dev_t)
///
/// \When co_dev_fini() is called
///
/// \Then the device is finalized
///       \Calls rbtree_foreach()
///       \Calls co_obj_destroy()
///       \IfCalls{!LELY_NO_CO_OBJ_NAME, free()}
TEST(CO_DevInit, CoDevFini_Nominal) {
  auto* const dev = AcquireCoDevT();
  CHECK(dev != nullptr);
  POINTERS_EQUAL(dev, co_dev_init(dev, 0x01u));

  co_dev_fini(dev);

  ReleaseCoDevT(dev);
}

///@}

#if !LELY_NO_MALLOC
/// @name co_dev_destroy()
///@{

/// \Given a null pointer to an initialized device (co_dev_t)
///
/// \When co_dev_destroy() is called
///
/// \Then nothing is changed
TEST(CO_DevInit, CoDevDestroy_Null) {
  co_dev_t* const dev = nullptr;

  co_dev_destroy(dev);
}
///@}
#endif

TEST_GROUP(CO_Dev) {
  co_dev_t* dev = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();

    dev_holder.reset(new CoDevTHolder(0x01u));
    dev = dev_holder->Get();

    CHECK(dev != nullptr);

    set_errnum(ERRNUM_SUCCESS);
  }

  TEST_TEARDOWN() { dev_holder.reset(); }
};

/// @name co_dev_set_net_id()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_netid() is called with a valid network-ID
///
/// \Then 0 is returned, the network-ID is set
TEST(CO_Dev, CoDevSetNetId_Nominal) {
  const auto ret = co_dev_set_netid(dev, 0x3du);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x3du, co_dev_get_netid(dev));
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_netid() is called with an network-ID of 0xff
///
/// \Then 0 is returned, the network-ID is set
TEST(CO_Dev, CoDevSetNetId_Unconfigured) {
  const auto ret = co_dev_set_netid(dev, 0xffu);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0xffu, co_dev_get_netid(dev));
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_netid() is called with an invalid network-ID
///
/// \Then -1 is returned, the network-ID is not set
///       \Calls set_errnum()
TEST(CO_Dev, CoDevSetNetId_InvalidId) {
  const auto ret1 = co_dev_set_netid(dev, CO_NUM_NETWORKS + 1);

  CHECK_EQUAL(-1, ret1);
  CHECK_EQUAL(0, co_dev_get_netid(dev));

  const auto ret2 = co_dev_set_netid(dev, 0xff - 1);

  CHECK_EQUAL(-1, ret2);
  CHECK_EQUAL(0, co_dev_get_netid(dev));
}

///@}

/// @name co_dev_set_id()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_id() is called with a valid node-ID
///
/// \Then 0 is returned, requested node-ID is set
///       \Calls co_obj_set_id()
TEST(CO_Dev, CoDevSetId_Nominal) {
  const auto ret = co_dev_set_id(dev, 0x3du);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x3du, co_dev_get_id(dev));
}

/// \Given a pointer to a device (co_dev_t) containing an entry in the object
///        dictionary
///
/// \When co_dev_set_id() is called with a node-ID
///
/// \Then 0 is returned, requested node-ID is set, the sub-object values of
///       the form `$NODEID { "+" number }` are updated
///       \Calls co_obj_set_id()
TEST(CO_Dev, CoDevSetId_CheckObj) {
  CoObjTHolder obj_holder(0x0000u);
#if !LELY_NO_CO_OBJ_LIMITS
  CoObjTHolder obj1_holder(0x0001u);
  CoObjTHolder obj2_holder(0x1234u);
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
  CoObjTHolder obj3_holder(0xffffu);
#endif
#if !LELY_NO_CO_OBJ_LIMITS
  CoSubTHolder sub_min1_holder(0x00u, CO_DEFTYPE_INTEGER16);
  CoSubTHolder sub_min2_holder(0x01u, CO_DEFTYPE_INTEGER16);
  CoSubTHolder sub_max1_holder(0x00u, CO_DEFTYPE_INTEGER16);
  CoSubTHolder sub_max2_holder(0x01u, CO_DEFTYPE_INTEGER16);
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
  CoSubTHolder sub_def1_holder(0x00u, CO_DEFTYPE_INTEGER16);
  CoSubTHolder sub_def2_holder(0x01u, CO_DEFTYPE_INTEGER16);
#endif

#if !LELY_NO_CO_OBJ_LIMITS
  const co_integer16_t min_val1 = 0x0;
  const co_integer16_t min_val2 = 0x0 + co_dev_get_id(dev);
  CHECK_EQUAL(2, co_sub_set_min(sub_min1_holder.Get(), &min_val1, 2));
  CHECK_EQUAL(2, co_sub_set_min(sub_min2_holder.Get(), &min_val2, 2));
  co_sub_set_flags(sub_min2_holder.Get(), CO_OBJ_FLAGS_MIN_NODEID);

  const co_integer16_t max_val1 = 0x3f00;
  const co_integer16_t max_val2 = 0x3f00 + co_dev_get_id(dev);
  CHECK_EQUAL(2, co_sub_set_max(sub_max1_holder.Get(), &max_val1, 2));
  CHECK_EQUAL(2, co_sub_set_max(sub_max2_holder.Get(), &max_val2, 2));
  co_sub_set_flags(sub_max2_holder.Get(), CO_OBJ_FLAGS_MAX_NODEID);
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
  const co_integer16_t def_val1 = 0x1234;
  const co_integer16_t def_val2 = 0x1234 + co_dev_get_id(dev);
  CHECK_EQUAL(2, co_sub_set_def(sub_def1_holder.Get(), &def_val1, 2));
  CHECK_EQUAL(2, co_sub_set_def(sub_def2_holder.Get(), &def_val2, 2));
  co_sub_set_flags(sub_def2_holder.Get(), CO_OBJ_FLAGS_DEF_NODEID);
#endif

#if !LELY_NO_CO_OBJ_LIMITS
  CHECK(obj1_holder.InsertSub(sub_min1_holder) != nullptr);
  CHECK(obj1_holder.InsertSub(sub_min2_holder) != nullptr);
  CHECK(obj2_holder.InsertSub(sub_max1_holder) != nullptr);
  CHECK(obj2_holder.InsertSub(sub_max2_holder) != nullptr);
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
  CHECK(obj3_holder.InsertSub(sub_def1_holder) != nullptr);
  CHECK(obj3_holder.InsertSub(sub_def2_holder) != nullptr);
#endif

  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder.Take()));
#if !LELY_NO_CO_OBJ_LIMITS
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1_holder.Take()));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2_holder.Take()));
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj3_holder.Take()));
#endif

  const co_unsigned8_t new_id = 0x3d;

  const auto ret = co_dev_set_id(dev, new_id);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(new_id, co_dev_get_id(dev));

#if !LELY_NO_CO_OBJ_LIMITS || !LELY_NO_CO_OBJ_DEFAULT
  const co_obj_t* out_obj = co_dev_first_obj(dev);
#endif

#if !LELY_NO_CO_OBJ_LIMITS
  out_obj = co_obj_next(out_obj);
  CHECK_EQUAL(0x0, *static_cast<const co_integer16_t*>(
                       co_sub_get_min(co_obj_first_sub(out_obj))));
  CHECK_EQUAL(0x0 + new_id, *static_cast<const co_integer16_t*>(
                                co_sub_get_min(co_obj_last_sub(out_obj))));

  out_obj = co_obj_next(out_obj);
  CHECK_EQUAL(0x3f00u, *static_cast<const co_integer16_t*>(
                           co_sub_get_max(co_obj_first_sub(out_obj))));
  CHECK_EQUAL(0x3f00 + new_id, *static_cast<const co_integer16_t*>(
                                   co_sub_get_max(co_obj_last_sub(out_obj))));
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
  out_obj = co_obj_next(out_obj);
  CHECK_EQUAL(0x1234u, *static_cast<const co_integer16_t*>(
                           co_sub_get_def(co_obj_first_sub(out_obj))));
  CHECK_EQUAL(0x1234 + new_id, *static_cast<const co_integer16_t*>(
                                   co_sub_get_def(co_obj_last_sub(out_obj))));
#endif
}

/// \Given a pointer to a device (co_dev_t) containing an entry in the object
///        dictionary, the entry is of a basic data type
///
/// \When co_dev_set_id() is called with a valid node-ID
///
/// \Then 0 is returned and the requested node-ID is set, the sub-object values
///       of the form `$NODEID { "+" number }` are updated
///       \Calls co_obj_set_id()
TEST(CO_Dev, CoDevSetId_CoType_BasicTypes) {
  (void)0;  // clang-format fix

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  { \
    dev_holder.reset(new CoDevTHolder(0x01u)); \
    dev = dev_holder->Get(); \
\
    CoObjTHolder obj_holder(0x1234u); \
    CoSubTHolder sub_holder(0xabu, CO_DEFTYPE_##a); \
    co_sub_t* const sub = obj_holder.InsertSub(sub_holder); \
    CHECK(sub != nullptr); \
    co_obj_t* const obj = obj_holder.Take(); \
    CHECK_EQUAL(co_type_sizeof(CO_DEFTYPE_##a), \
                co_sub_set_val_##c(sub, 0x42 + co_dev_get_id(dev))); \
    co_sub_set_flags(sub, CO_OBJ_FLAGS_VAL_NODEID); \
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj)); \
    const co_unsigned8_t new_id = 0x14; \
\
    const auto ret = co_dev_set_id(dev, new_id); \
\
    CHECK_EQUAL(0, ret); \
    CHECK_EQUAL(new_id, co_dev_get_id(dev)); \
    const co_obj_t* const out_obj = co_dev_first_obj(dev); \
    CHECK_EQUAL(static_cast<co_##b##_t>(0x42 + new_id), \
                *static_cast<const co_##b##_t*>( \
                    co_sub_get_val(co_obj_first_sub(out_obj)))); \
  }
#include <lely/co/def/basic.def>
#undef LELY_CO_DEFINE_TYPE
}

/// \Given a pointer to a device (co_dev_t) containing an entry in the object
///        dictionary, the entry is of a non-basic data type
///
/// \When co_dev_set_id() is called with a valid node-ID
///
/// \Then 0 is returned and the requested node-ID is set, the sub-object values
///       are not modified
///       \Calls co_obj_set_id()
TEST(CO_Dev, CoDevSetId_CoType_NonBasic) {
  CoObjTHolder obj_holder(0x1234u);
  CoSubTHolder sub_holder(0x01u, CO_DEFTYPE_TIME_OF_DAY);
  co_sub_t* const sub = obj_holder.InsertSub(sub_holder);
  CHECK(sub != nullptr);
  co_obj_t* const obj = obj_holder.Take();
  const co_time_of_day value = {1000u, 2000u};
  CHECK_EQUAL(sizeof(value), co_sub_set_val(sub, &value, sizeof(value)));
  co_sub_set_flags(sub, CO_OBJ_FLAGS_VAL_NODEID);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const co_unsigned8_t new_id = 0x40;
  CHECK_EQUAL(0, co_dev_set_id(dev, new_id));
  CHECK_EQUAL(new_id, co_dev_get_id(dev));

  const void* val = co_sub_get_val(sub);
  POINTER_NOT_NULL(val);
  const auto* const val_ret = static_cast<const co_val*>(val);
  CHECK_EQUAL(value.ms, val_ret->t.ms);
  CHECK_EQUAL(value.days, val_ret->t.days);
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_id() is called with a node-ID of 0xff
///
/// \Then 0 is returned and the required node-ID is set
///       \Calls co_obj_set_id()
TEST(CO_Dev, CoDevSetId_Unconfigured) {
  const auto ret = co_dev_set_id(dev, 0xffu);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0xffu, co_dev_get_id(dev));
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_id() is called with a node-ID equal zero
///
/// \Then -1 is returned and the required node-ID is not set
///       \Calls set_errnum()
TEST(CO_Dev, CoDevSetId_ZeroId) {
  const auto ret = co_dev_set_id(dev, 0x00u);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0x01u, co_dev_get_id(dev));
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_id() is called with an invalid node-ID
///
/// \Then -1 is returned and the required node-ID is not set
///       \Calls set_errnum()
TEST(CO_Dev, CoDevSetId_InvalidId) {
  const auto ret1 = co_dev_set_id(dev, CO_NUM_NODES + 1);

  CHECK_EQUAL(-1, ret1);
  CHECK_EQUAL(0x01u, co_dev_get_id(dev));

  const auto ret2 = co_dev_set_id(dev, 0xff - 1);

  CHECK_EQUAL(-1, ret2);
  CHECK_EQUAL(0x01u, co_dev_get_id(dev));
}

///@}

/// @name co_dev_get_idx()
///@{

/// \Given a pointer to a device (co_dev_t) with an empty object dictionary
///
/// \When co_dev_get_idx() is called with a maximum number of indices to return
///       and a memory area to store the object indices
///
/// \Then 0 is returned and the memory area is not modified
///       \Calls rbtree_first()
///       \Calls co_obj_get_idx()
///       \Calls rbtree_size()
TEST(CO_Dev, CoDevGetIdx_Empty) {
  co_unsigned16_t out_idx = 0x0000;
  const auto ret = co_dev_get_idx(dev, 1, &out_idx);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x0000u, out_idx);
}

/// \Given a pointer to a device (co_dev_t) with an empty object dictionary
///
/// \When co_dev_get_idx() is called with no memory area to store the results
///
/// \Then 0 is returned
///       \Calls rbtree_size()
TEST(CO_Dev, CoDevGetIdx_EmptyNull) {
  const auto ret = co_dev_get_idx(dev, 0, nullptr);

  CHECK_EQUAL(0, ret);
}

/// \Given a pointer to a device (co_dev_t) containing one object in the object
///        dictionary
///
/// \When co_dev_get_idx() is called with no memory area to store the results
///
/// \Then 1 is returned
///       \Calls rbtree_size()
TEST(CO_Dev, CoDevGetIdx_OneObjCheckNumber) {
  CoObjTHolder obj(0x0000u);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const auto ret = co_dev_get_idx(dev, 0, nullptr);

  CHECK_EQUAL(1u, ret);
}

/// \Given a pointer to a device (co_dev_t) containing one object in the object
///        dictionary
///
/// \When co_dev_get_idx() is called with a memory area to store the results
///
/// \Then 1 is returned, the memory area contains the index of the object
///       \Calls rbtree_first()
///       \Calls co_obj_get_idx()
///       \Calls rbnode_next()
///       \Calls rbtree_size()
TEST(CO_Dev, CoDevGetIdx_OneObjCheckIdx) {
  CoObjTHolder obj(0x1234u);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));
  co_unsigned16_t out_idx = 0x0000;
  const auto ret = co_dev_get_idx(dev, 1, &out_idx);

  CHECK_EQUAL(1, ret);
  CHECK_EQUAL(0x1234u, out_idx);
}

/// \Given a pointer to a device (co_dev_t) containing many objects in
///        the object dictionary
///
/// \When co_dev_get_idx() is called with a maximum number of indices to return
///       equal to the number of the objects in the dictionary, a memory area
///       to store the results
///
/// \Then a number of objects in the object dictionary is returned, the memory
///       area contains indices of these objects
///       \Calls rbtree_first()
///       \Calls co_obj_get_idx()
///       \Calls rbnode_next()
///       \Calls rbtree_size()
TEST(CO_Dev, CoDevGetIdx_ManyObj) {
  CoObjTHolder obj1(0x0000u);
  CoObjTHolder obj2(0x1234u);
  CoObjTHolder obj3(0xffffu);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1.Take()));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2.Take()));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj3.Take()));

  co_unsigned16_t out_idx[5] = {0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000};
  const auto ret = co_dev_get_idx(dev, 5, out_idx);

  CHECK_EQUAL(3, ret);
  CHECK_EQUAL(0x0000u, out_idx[0]);
  CHECK_EQUAL(0x1234u, out_idx[1]);
  CHECK_EQUAL(0xffffu, out_idx[2]);
  CHECK_EQUAL(0x0000u, out_idx[3]);
  CHECK_EQUAL(0x0000u, out_idx[4]);
}

/// \Given a pointer to a device (co_dev_t) containing many objects in
///        the object dictionary
///
/// \When co_dev_get_idx() is called with maximum number of indices to return
///       lower than the number of the objects in the dictionary, a memory area
///       to store the results
///
/// \Then a number of the contained objects is returned, requested number of
///       indices in order from lowest to highest are stored in the memory area
///       \Calls rbtree_first()
///       \Calls co_obj_get_idx()
///       \Calls rbnode_next()
///       \Calls rbtree_size()
TEST(CO_Dev, CoDevGetIdx_ManyObj_MaxIdxLessThanArrLen) {
  CoObjTHolder obj1(0x0000u);
  CoObjTHolder obj2(0x1234u);
  CoObjTHolder obj3(0xffffu);
  CoObjTHolder obj4(0xabcdu);
  CoObjTHolder obj5(0x1010u);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1.Take()));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2.Take()));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj3.Take()));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj4.Take()));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj5.Take()));

  co_unsigned16_t out_idx[5] = {0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000};
  const auto ret = co_dev_get_idx(dev, 3, out_idx);

  CHECK_EQUAL(5, ret);
  CHECK_EQUAL(0x0000u, out_idx[0]);
  CHECK_EQUAL(0x1010u, out_idx[1]);
  CHECK_EQUAL(0x1234u, out_idx[2]);
  CHECK_EQUAL(0x0000u, out_idx[3]);
  CHECK_EQUAL(0x0000u, out_idx[4]);
}

///@}

/// @name co_dev_insert_obj()
///@{

/// \Given a pointer to a device (co_dev_t) with an empty object dictionary
///
/// \When co_dev_insert_obj() is called with a pointer to an object (co_obj_t)
///
/// \Then 0 is returned, the object is inserted into the object dictionary
///       \Calls rbtree_find()
///       \Calls rbtree_insert()
TEST(CO_Dev, CoDevInsertObj_Nominal) {
  CoObjTHolder obj_holder(0x1234u);
  co_obj_t* const obj = obj_holder.Take();

  const auto ret = co_dev_insert_obj(dev, obj);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(obj, co_dev_first_obj(dev));
  co_unsigned16_t out_idx = 0x0000;
  CHECK_EQUAL(1, co_dev_get_idx(dev, 1, &out_idx));
  CHECK_EQUAL(0x1234u, out_idx);
  CHECK_EQUAL(dev, co_obj_get_dev(obj));
}

/// \Given a pointer to a device (co_dev_t) with an empty object dictionary
///
/// \When co_dev_insert_obj() is called with a pointer to the object (co_obj_t)
///       which was inserted into other device's object dictionary
///
/// \Then -1 is returned, the object is still inserted into other device's
///       object dictionary and is not inserted into target device's object
///       dictionary
TEST(CO_Dev, CoDevInsertObj_InsertedIntoOtherDev) {
  CoDevTHolder other_dev_holder(0x02u);
  CoObjTHolder obj_holder(0x0001u);
  co_dev_t* const other_dev = other_dev_holder.Get();
  co_obj_t* const obj = obj_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(other_dev, obj));

  const auto ret = co_dev_insert_obj(dev, obj);

  CHECK_EQUAL(-1, ret);
  POINTERS_EQUAL(obj, co_dev_find_obj(other_dev, 0x0001u));
  POINTERS_EQUAL(nullptr, co_dev_find_obj(dev, 0x0001u));
}

/// \Given a pointer to a device (co_dev_t) containing an object in the object
///        dictionary
///
/// \When co_dev_insert_obj() is called with a pointer to the object (co_obj_t)
///       which was already inserted into the object dictionary
///
/// \Then 0 is returned, the object is still inserted into the object dictionary
TEST(CO_Dev, CoDevInsertObj_AlreadyInserted) {
  CoObjTHolder obj_holder(0x00001);
  co_obj_t* const obj = obj_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto ret = co_dev_insert_obj(dev, obj);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(obj, co_dev_find_obj(dev, 0x0001u));
}

/// \Given a pointer to a device (co_dev_t) containing an object in the object
///        dictionary
///
/// \When co_dev_insert_obj() is called with a pointer to the another object
///       (co_obj_t) with an index equal to the index of the object which was
///       already inserted into the other device's object dictionary
///
/// \Then -1 is returned, the initial object is still inserted into the other
///       device's object dictionary
TEST(CO_Dev, CoDevInsertObj_AlreadyInsertedAtIdx) {
  CoObjTHolder obj1_holder(0x0001u);
  CoObjTHolder obj2_holder(0x0001u);
  co_obj_t* const obj1 = obj1_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1));

  const auto ret = co_dev_insert_obj(dev, obj2_holder.Get());

  CHECK_EQUAL(-1, ret);
  POINTERS_EQUAL(obj1, co_dev_find_obj(dev, 0x0001u));
}

///@}

/// @name co_dev_remove_obj()
///@{

/// \Given a pointer to a device (co_dev_t) containing an object in the object
///       dictionary
///
/// \When co_dev_remove_obj() is called with a pointer to the object (co_obj_t)
///
/// \Then 0 is returned, the object is not present in the device
///       \Calls rbtree_remove()
///       \Calls rbnode_init()
TEST(CO_Dev, CoDevRemoveObj_Nominal) {
  CoObjTHolder obj_holder(0x1234u);
  co_obj_t* const obj = obj_holder.Get();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto ret = co_dev_remove_obj(dev, obj);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_dev_get_idx(dev, 0, nullptr));
  POINTERS_EQUAL(nullptr, co_obj_get_dev(obj));
}

/// \Given a pointer to a device (co_dev_t) with an empty object dictionary
///
/// \When co_dev_remove_obj() is called with a pointer to an object not inserted
///       into the dictionary
///
/// \Then -1 is returned, nothing is changed
TEST(CO_Dev, CoDevRemoveObj_NotInserted) {
  CoObjTHolder obj(0x1234u);

  const auto ret = co_dev_remove_obj(dev, obj.Get());

  CHECK_EQUAL(-1, ret);
}

///@}

/// @name co_dev_find_obj()
///@{

/// \Given a pointer to a device (co_dev_t) containing an object in the object
///        dictionary
///
/// \When co_dev_find_obj() is called with the object's index
///
/// \Then a pointer to the object is returned
///       \Calls rbtree_find()
TEST(CO_Dev, CoDevFindObj_Nominal) {
  CoObjTHolder obj_holder(0x1234u);
  co_obj_t* const obj = obj_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto* const ret = co_dev_find_obj(dev, 0x1234u);

  POINTERS_EQUAL(obj, ret);
}

/// \Given a pointer to a device (co_dev_t) containing an object in the object
///        dictionary
///
/// \When co_dev_find_obj() is called with an index of a non-existing object
///
/// \Then a null pointer is returned
///       \Calls rbtree_find()
TEST(CO_Dev, CoDevFindObj_Nonempty_NotFound) {
  CoObjTHolder obj_holder(0x1234u);
  co_obj_t* const obj = obj_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto* const ret = co_dev_find_obj(dev, 0xffffu);

  POINTERS_EQUAL(nullptr, ret);
}

/// \Given a pointer to a device (co_dev_t) with an empty object dictionary
///
/// \When co_dev_find_obj() is called with an index
///
/// \Then a null pointer is returned
///       \Calls rbtree_find()
TEST(CO_Dev, CoDevFindObj_Empty_NotFound) {
  const auto* const ret = co_dev_find_obj(dev, 0x1234u);

  POINTERS_EQUAL(nullptr, ret);
}

///@}

/// @name co_dev_find_sub()
///@{

/// \Given a pointer to a device (co_dev_t) containing an object with
///        a sub-object in the object dictionary
///
/// \When co_dev_find_sub() is called with an index and a sub-index of
///       the sub-object
///
/// \Then a pointer to the sub-object is returned
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
TEST(CO_Dev, CoDevFindSub_Nominal) {
  CoObjTHolder obj_holder(0x1234u);
  CoSubTHolder sub_holder(0xabu, CO_DEFTYPE_INTEGER16);
  co_sub_t* const sub = obj_holder.InsertSub(sub_holder);
  CHECK(sub != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder.Take()));

  const auto* const ret = co_dev_find_sub(dev, 0x1234u, 0xabu);

  POINTERS_EQUAL(sub, ret);
}

/// \Given a pointer to a device (co_dev_t) with an empty object dictionary
///
/// \When co_dev_find_sub() is called with an index and a sub-index
///
/// \Then a null pointer is returned
///       \Calls co_dev_find_obj()
TEST(CO_Dev, CoDevFindSub_NoObj) {
  const auto* const ret = co_dev_find_sub(dev, 0x1234u, 0x00u);

  POINTERS_EQUAL(nullptr, ret);
}

/// \Given a pointer to a device (co_dev_t) containing an empty object in
///        the object dictionary
///
/// \When co_dev_find_sub() is called with an index of the object and
///       a sub-index
///
/// \Then a null pointer is returned
///       \Calls co_dev_find_obj()
///       \Calls co_obj_find_sub()
TEST(CO_Dev, CoDevFindSub_NoSub) {
  CoObjTHolder obj(0x1234u);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const auto* const ret = co_dev_find_sub(dev, 0x1234u, 0x00u);

  POINTERS_EQUAL(nullptr, ret);
}

///@}

/// @name co_dev_first_obj()
///@{

/// \Given a pointer to a device (co_dev_t) containing three objects in
///        the object dictionary, object with the lowest index is inserted first
///
/// \When co_dev_first_obj() is called
///
/// \Then a pointer to the object with a minimal index is returned
///       \Calls rbtree_first()
TEST(CO_Dev, CoDevFirstObj_LowestInsertedFirst) {
  CoObjTHolder obj2_holder(0x1235u);
  const auto obj2 = obj2_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2));
  CoObjTHolder obj3_holder(0x1236u);
  const auto obj3 = obj3_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj3));
  CoObjTHolder obj_holder(0x1234u);
  const auto obj = obj_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto* const ret = co_dev_first_obj(dev);

  POINTERS_EQUAL(obj, ret);
}

/// \Given a pointer to a device (co_dev_t) containing three objects in
///        the object dictionary, object with the lowest index is inserted
///        between objects with higher indices
///
/// \When co_dev_first_obj() is called
///
/// \Then a pointer to the object with a minimal index is returned
///       \Calls rbtree_first()
TEST(CO_Dev, CoDevFirstObj_LowestInsertedInMiddle) {
  CoObjTHolder obj2_holder(0x1235u);
  const auto obj2 = obj2_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2));
  CoObjTHolder obj_holder(0x1234u);
  const auto obj = obj_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));
  CoObjTHolder obj3_holder(0x1236u);
  const auto obj3 = obj3_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj3));

  const auto* const ret = co_dev_first_obj(dev);

  POINTERS_EQUAL(obj, ret);
}

/// \Given a pointer to a device (co_dev_t) containing three objects in
///        the object dictionary, object with the lowest index is inserted last
///
/// \When co_dev_first_obj() is called
///
/// \Then a pointer to the object with a minimal index is returned
///       \Calls rbtree_first()
TEST(CO_Dev, CoDevFirstObj_LowestInsertedLast) {
  CoObjTHolder obj2_holder(0x1235u);
  const auto obj2 = obj2_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2));
  CoObjTHolder obj3_holder(0x1236u);
  const auto obj3 = obj3_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj3));
  CoObjTHolder obj_holder(0x1234u);
  const auto obj = obj_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto* const ret = co_dev_first_obj(dev);

  POINTERS_EQUAL(obj, ret);
}

/// \Given a pointer to a device (co_dev_t) containing one object in
///        the object dictionary
///
/// \When co_dev_first_obj() is called
///
/// \Then a pointer to the object is returned
///       \Calls rbtree_first()
TEST(CO_Dev, CoDevFirstObj_SingleObject) {
  CoObjTHolder obj_holder(0x1234u);
  const auto obj = obj_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto* const ret = co_dev_first_obj(dev);

  POINTERS_EQUAL(obj, ret);
}

/// \Given a pointer to a device (co_dev_t) with an empty object dictionary
///
/// \When co_dev_first_obj() is called
///
/// \Then a null pointer is returned
///       \Calls rbtree_first()
TEST(CO_Dev, CoDevFirstObj_Empty) {
  const auto* const ret = co_dev_first_obj(dev);

  POINTERS_EQUAL(nullptr, ret);
}

///@}

/// @name co_dev_last_obj()
///@{

/// \Given a pointer to a device (co_dev_t) containing three objects in
///        the object dictionary, object with the highest index is
///        inserted first
///
/// \When co_dev_last_obj() is called
///
/// \Then a pointer to the object with a maximum index is returned
///       \Calls rbtree_last()
TEST(CO_Dev, CoDevLastObj_HighestInsertedFirst) {
  CoObjTHolder obj3_holder(0x1236u);
  const auto obj3 = obj3_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj3));
  CoObjTHolder obj2_holder(0x1235u);
  const auto obj2 = obj2_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2));
  CoObjTHolder obj_holder(0x1234u);
  const auto obj = obj_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto* const ret = co_dev_last_obj(dev);

  POINTERS_EQUAL(obj3, ret);
}

/// \Given a pointer to a device (co_dev_t) containing three objects in
///        the object dictionary, object with the highest index is
///        inserted between objects with lower indices
///
/// \When co_dev_last_obj() is called
///
/// \Then a pointer to the object with a maximum index is returned
///       \Calls rbtree_last()
TEST(CO_Dev, CoDevLastObj_HighestInsertedInMiddle) {
  CoObjTHolder obj_holder(0x1234u);
  const auto obj = obj_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));
  CoObjTHolder obj3_holder(0x1236u);
  const auto obj3 = obj3_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj3));
  CoObjTHolder obj2_holder(0x1235u);
  const auto obj2 = obj2_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2));

  const auto* const ret = co_dev_last_obj(dev);

  POINTERS_EQUAL(obj3, ret);
}

/// \Given a pointer to a device (co_dev_t) containing three objects in
///        the object dictionary, object with the highest index is inserted last
///
/// \When co_dev_last_obj() is called
///
/// \Then a pointer to the object with a maximum index is returned
///       \Calls rbtree_last()
TEST(CO_Dev, CoDevLastObj_HighestInsertedLast) {
  CoObjTHolder obj_holder(0x1234u);
  const auto obj = obj_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));
  CoObjTHolder obj3_holder(0x1236u);
  const auto obj3 = obj3_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj3));
  CoObjTHolder obj2_holder(0x1235u);
  const auto obj2 = obj2_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2));

  const auto* const ret = co_dev_last_obj(dev);

  POINTERS_EQUAL(obj3, ret);
}

/// \Given a pointer to a device (co_dev_t) containing one object in
///        the object dictionary
///
/// \When co_dev_last_obj() is called
///
/// \Then a pointer to the object is returned
///       \Calls rbtree_last()
TEST(CO_Dev, CoDevLastObj_SingleObject) {
  CoObjTHolder obj_holder(0x1234u);
  const auto obj = obj_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto* const ret = co_dev_last_obj(dev);

  POINTERS_EQUAL(obj, ret);
}

/// \Given a pointer to a device (co_dev_t) with an empty object dictionary
///
/// \When co_dev_last_obj() is called
///
/// \Then a null pointer is returned
///       \Calls rbtree_last()
TEST(CO_Dev, CoDevLastObj_Empty) {
  const auto* const ret = co_dev_last_obj(dev);

  POINTERS_EQUAL(nullptr, ret);
}

///@}

#if !LELY_NO_CO_OBJ_NAME

/// @name co_dev_set_name()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_name() is called with a pointer to a name
///
/// \Then 0 is returned and the name is set
///       \Calls realloc()
TEST(CO_Dev, CoDevSetName_Nominal) {
  const char* name = "DeviceName";
  const auto ret = co_dev_set_name(dev, name);

  CHECK_EQUAL(0, ret);
  STRCMP_EQUAL(name, co_dev_get_name(dev));
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_name() is called with a null pointer
///
/// \Then 0 is returned and the device name is a null pointer
///       \Calls free()
TEST(CO_Dev, CoDevSetName_Null) {
  const char* name = "DeviceName";
  CHECK_EQUAL(0, co_dev_set_name(dev, name));

  const auto ret = co_dev_set_name(dev, nullptr);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_dev_get_name(dev));
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_name() is called with a pointer to an empty string
///
/// \Then 0 is returned and the device name is a null pointer
///       \Calls free()
TEST(CO_Dev, CoDevSetName_Empty) {
  const char* name = "DeviceName";
  CHECK_EQUAL(0, co_dev_set_name(dev, name));

  const auto ret = co_dev_set_name(dev, "");

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_dev_get_name(dev));
}

///@}

/// @name co_dev_set_vendor_name()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_vendor_name() is called with a pointer to a new name
///
/// \Then 0 is returned and the vendor name is set
///       \Calls realloc()
///       \Calls strlen()
///       \Calls strcpy()
TEST(CO_Dev, CoDevSetVendorName_Nominal) {
  const char* vendor_name = "VendorName";
  const auto ret = co_dev_set_vendor_name(dev, vendor_name);

  CHECK_EQUAL(0, ret);
  STRCMP_EQUAL(vendor_name, co_dev_get_vendor_name(dev));
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_vendor_name() is called with a null pointer
///
/// \Then 0 is returned and the vendor name is a null pointer
///       \Calls free()
TEST(CO_Dev, CoDevSetVendorName_Null) {
  const char* vendor_name = "VendorName";
  CHECK_EQUAL(0, co_dev_set_vendor_name(dev, vendor_name));

  const auto ret = co_dev_set_vendor_name(dev, nullptr);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_dev_get_vendor_name(dev));
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_vendor_name() is called with a pointer to an empty string
///
/// \Then 0 is returned and the vendor name is a null pointer
///       \Calls free()
TEST(CO_Dev, CoDevSetVendorName_Empty) {
  const char* vendor_name = "VendorName";
  CHECK_EQUAL(0, co_dev_set_vendor_name(dev, vendor_name));

  const auto ret = co_dev_set_vendor_name(dev, "");

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_dev_get_vendor_name(dev));
}

///@}

/// @name co_dev_set_product_name()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_product_name() is called with a pointer to a name
///
/// \Then 0 is returned and the product name is set
///       \Calls realloc()
///       \Calls strlen()
///       \Calls strcpy()
TEST(CO_Dev, CoDevSetProductName_Nominal) {
  const char* product_name = "ProductName";
  const auto ret = co_dev_set_product_name(dev, product_name);

  CHECK_EQUAL(0, ret);
  STRCMP_EQUAL(product_name, co_dev_get_product_name(dev));
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_product_name() is called with a null pointer
///
/// \Then 0 is returned and the product name is a null pointer
///       \Calls free()
TEST(CO_Dev, CoDevSetProductName_Null) {
  const char* product_name = "ProductName";
  CHECK_EQUAL(0, co_dev_set_product_name(dev, product_name));

  const auto ret = co_dev_set_product_name(dev, nullptr);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_dev_get_product_name(dev));
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_product_name() is called with a pointer to an empty string
///
/// \Then 0 is returned and the product name is a null pointer
///       \Calls free()
TEST(CO_Dev, CoDevSetProductName_Empty) {
  const char* product_name = "ProductName";
  CHECK_EQUAL(0, co_dev_set_product_name(dev, product_name));

  const auto ret = co_dev_set_product_name(dev, "");

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_dev_get_product_name(dev));
}

///@}

/// @name co_dev_set_order_code()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_order_code() is called with a pointer to an order code
///
/// \Then 0 is returned and the order code is set
///       \Calls realloc()
///       \Calls strlen()
///       \Calls strcpy()
TEST(CO_Dev, CoDevSetOrderCode_Nominal) {
  const char* order_code = "OrderCode";
  const auto ret = co_dev_set_order_code(dev, order_code);

  CHECK_EQUAL(0, ret);
  STRCMP_EQUAL(order_code, co_dev_get_order_code(dev));
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_order_code() is called with a null pointer
///
/// \Then 0 is returned and the order code is a null pointer
///       \Calls free()
TEST(CO_Dev, CoDevSetOrderCode_Null) {
  const char* order_code = "OrderCode";
  CHECK_EQUAL(0, co_dev_set_order_code(dev, order_code));

  const auto ret = co_dev_set_order_code(dev, nullptr);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_dev_get_order_code(dev));
}

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_order_code() is called with a pointer to an empty string
///
/// \Then 0 is returned and the order code is a null pointer
///       \Calls free()
TEST(CO_Dev, CoDevSetOrderCode_Empty) {
  const char* order_code = "OrderCode";
  CHECK_EQUAL(0, co_dev_set_order_code(dev, order_code));

  const auto ret = co_dev_set_order_code(dev, "");

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_dev_get_order_code(dev));
}

///@}

#endif  // !LELY_NO_CO_OBJ_NAME

/// @name co_dev_set_vendor_id()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_vendor_id() is called with a vendor ID
///
/// \Then the requested vendor ID is set
TEST(CO_Dev, CoDevSetVendorId_Nominal) {
  co_dev_set_vendor_id(dev, 0x12345678u);

  CHECK_EQUAL(0x12345678u, co_dev_get_vendor_id(dev));
}

///@}

/// @name co_dev_set_product_code()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_product_code() is called with a number
///
/// \Then the requested product code is set
TEST(CO_Dev, CoDevSetProductCode_Nominal) {
  co_dev_set_product_code(dev, 0x12345678u);

  CHECK_EQUAL(0x12345678u, co_dev_get_product_code(dev));
}

///@}

/// @name co_dev_set_revision()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_revision() is called with a number
///
/// \Then the requested revision is set
TEST(CO_Dev, CoDevSetRevision_Nominal) {
  co_dev_set_revision(dev, 0x12345678u);

  CHECK_EQUAL(0x12345678u, co_dev_get_revision(dev));
}

///@}

/// @name co_dev_set_baud()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_baud() is called with supported bit rates
///
/// \Then the requested bit rates are set
TEST(CO_Dev, CoDevSetBaud_Nominal) {
  co_dev_set_baud(dev, CO_BAUD_50 | CO_BAUD_1000);

  CHECK_EQUAL(CO_BAUD_50 | CO_BAUD_1000, co_dev_get_baud(dev));
}

///@}

/// @name co_dev_set_rate()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_rate() is called with a baudrate value
///
/// \Then the requested baudrate is set
TEST(CO_Dev, CoDevSetRate_Nominal) {
  co_dev_set_rate(dev, 500u);

  CHECK_EQUAL(500u, co_dev_get_rate(dev));
}

///@}

/// @name co_dev_set_lss()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_lss() is called with a non-zero number
///
/// \Then the LSS support flag is set
TEST(CO_Dev, CoDevSetLSS_Nominal) {
  co_dev_set_lss(dev, 123);

  CHECK_EQUAL(1, co_dev_get_lss(dev));
}

///@}

/// @name co_dev_set_dummy()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_dummy() is called with supported data types for mapping
///       dummy entries in PDOs
///
/// \Then the data types supported for mapping dummy entries in PDOs are set
TEST(CO_Dev, CoDevSetDummy_Nominal) {
  co_dev_set_dummy(dev, 0x00010001u);

  CHECK_EQUAL(0x00010001u, co_dev_get_dummy(dev));
}

///@}

/// @name co_dev_get_val()
///@{

/// \Given a pointer to a device (co_dev_t) containing an entry in
///        the object dictionary
///
/// \When co_dev_get_val() is called with an index and a sub-index of the entry
///
/// \Then a pointer to the entry's value is returned
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_val()
TEST(CO_Dev, CoDevGetVal_Nominal) {
  CoObjTHolder obj_holder(0x1234u);
  CoSubTHolder sub_holder(0xabu, CO_DEFTYPE_INTEGER16);
  co_sub_t* const sub = obj_holder.InsertSub(sub_holder);
  CHECK(sub != nullptr);
  CHECK_EQUAL(co_type_sizeof(CO_DEFTYPE_INTEGER16),
              co_sub_set_val_i16(sub, 0x0987u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder.Take()));

  const auto* ret =
      static_cast<const co_integer16_t*>(co_dev_get_val(dev, 0x1234u, 0xabu));

  POINTER_NOT_NULL(ret);
  CHECK_EQUAL(0x0987u, *ret);
}

/// \Given a null pointer to a device (co_dev_t)
///
/// \When co_dev_get_val() is called with any index and sub-index
///
/// \Then a null pointer is returned
///       \Calls co_sub_get_val()
TEST(CO_Dev, CoDevGetVal_NullDev) {
  co_dev_t* const dev = nullptr;

  const auto ret = co_dev_get_val(dev, 0x0000u, 0x00u);

  POINTERS_EQUAL(nullptr, ret);
}

/// \Given a pointer to a device (co_dev_t) with an empty object dictionary
///
/// \When co_dev_get_val() is called with an index and a sub-index of
///       a non-existing entry
///
/// \Then a null pointer is returned
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_val()
TEST(CO_Dev, CoDevGetVal_NotFound) {
  const auto ret = co_dev_get_val(dev, 0x0000u, 0x00u);

  POINTERS_EQUAL(nullptr, ret);
}

///@}

/// @name co_dev_set_val()
///@{

/// \Given a pointer to a device (co_dev_t) containing an entry in
///        the object dictionary
///
/// \When co_dev_set_val() is called with an index and sub-index of the entry,
///       a pointer to a value to be copied, a number of bytes present at
///       the pointer
///
/// \Then the length of the value is returned, the requested value is set
///       \Calls co_dev_find_sub()
///       \Calls co_sub_set_val()
TEST(CO_Dev, CoDevSetVal_Nominal) {
  co_unsigned16_t val = 0x0987u;

  CoObjTHolder obj_holder(0x1234u);
  CoSubTHolder sub_holder(0xabu, CO_DEFTYPE_INTEGER16);
  CHECK(obj_holder.InsertSub(sub_holder) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder.Take()));

  const auto ret = co_dev_set_val(dev, 0x1234u, 0xabu, &val, sizeof(val));

  CHECK_EQUAL(sizeof(val), ret);
  CHECK_EQUAL(val, co_dev_get_val_i16(dev, 0x1234u, 0xabu));
}

/// \Given a pointer to a device (co_dev_t) with an empty object dictionary
///
/// \When co_dev_set_val() is called with an index and a sub-index of
///       a non-existing entry, a pointer to a value to be copied, a number of
///       bytes present at the pointer
///
/// \Then 0 is returned and ERRNUM_INVAL error number is set
///       \Calls co_dev_find_sub()
///       \Calls set_errnum()
TEST(CO_Dev, CoDevSetVal_NotFound) {
  const auto ret = co_dev_set_val(dev, 0x0000u, 0x00u, nullptr, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

/// \Given a pointer to a device (co_dev_t) containing an entry in
///       the object dictionary
///
/// \When co_dev_set_val_<typename>() is called with an index, a sub-index of
///       the entry and a value
///
/// \Then the requested value is set and can be obtained using
///       co_dev_get_val_<typename>()
TEST(CO_Dev, CoDevSetGetVal_BasicTypes) {
  (void)0;  // clang-format fix

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  { \
    dev_holder.reset(new CoDevTHolder(0x01u)); \
    dev = dev_holder->Get(); \
\
    CoObjTHolder obj(0x1234u); \
    CoSubTHolder sub(0xabu, CO_DEFTYPE_##a); \
    CHECK(obj.InsertSub(sub) != nullptr); \
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take())); \
\
    const auto set_ret = co_dev_set_val_##c(dev, 0x1234u, 0xabu, 0x42u); \
\
    CHECK_EQUAL(co_type_sizeof(CO_DEFTYPE_##a), set_ret); \
\
    const co_##b##_t get_ret = co_dev_get_val_##c(dev, 0x1234u, 0xabu); \
\
    CHECK_EQUAL(get_ret, static_cast<co_##b##_t>(0x42u)); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

///@}

/// @name co_dev_read_sub()
///@{

/// \Given a pointer to a device (co_dev_t) containing an entry in
///        the object dictionary
///
/// \When co_dev_read_sub() is called with pointers to memory area to store
///       an index and a sub-index, a pointer to a start of a buffer with
///       a value in the concise DCF format and a pointer to one past the last
///       byte in the buffer
///
/// \Then a number of the bytes read from the buffer is returned, the index and
///       the sub-index are stored in the memory and the value read from
///       the buffer is set as the entry value
///       \Calls co_val_read()
///       \Calls co_dev_find_sub()
///       \Calls co_get_type()
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
///       \Calls co_val_init()
///       \Calls co_sub_set_val()
///       \Calls co_val_addressof()
///       \Calls co_val_sizeof()
///       \IfCalls{!LELY_NO_MALLOC, co_val_fini()}
TEST(CO_Dev, CoDevReadSub_Nominal) {
  CoObjTHolder obj(0x1234u);
  CoSubTHolder sub(0xabu, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9u;
  uint_least8_t buf[BUF_SIZE] = {0x34u, 0x12u, 0xabu, 0x02u, 0x00u,
                                 0x00u, 0x00u, 0x87u, 0x09};
  co_unsigned16_t idx = 0x0000;
  co_unsigned8_t subidx = 0x00;

  const auto ret = co_dev_read_sub(dev, &idx, &subidx, buf, buf + BUF_SIZE);

  const size_t bytes_read = 9u;
  CHECK_EQUAL(bytes_read, ret);
  CHECK_EQUAL(0x1234u, idx);
  CHECK_EQUAL(0xabu, subidx);
  CHECK_EQUAL(0x0987u, co_dev_get_val_i16(dev, idx, subidx));
}

/// \Given a pointer to a device (co_dev_t) containing an entry in
///        the object dictionary
///
/// \When co_dev_read_sub() is called with no memory area to store an index
///       and a sub-index, a pointer to a start of a buffer with a value in
///       the concise DCF format and a pointer to one past the last byte in
///       the buffer
///
/// \Then the number of bytes read from the buffer is returned, the value read
///       from the buffer is set as the entry value
///       \Calls co_val_read()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_type()
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
///       \Calls co_val_init()
///       \Calls co_val_read()
///       \Calls co_sub_set_val()
///       \Calls co_val_addressof()
///       \Calls co_val_sizeof()
///       \IfCalls{!LELY_NO_MALLOC, co_val_fini()}
TEST(CO_Dev, CoDevReadSub_NoIdx) {
  CoObjTHolder obj(0x1234u);
  CoSubTHolder sub(0xabu, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9u;
  uint_least8_t buf[BUF_SIZE] = {0x34u, 0x12u, 0xabu, 0x02u, 0x00u,
                                 0x00u, 0x00u, 0x87u, 0x09};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(BUF_SIZE, ret);
  CHECK_EQUAL(0x0987u, co_dev_get_val_i16(dev, 0x1234u, 0xabu));
}

/// \Given a pointer to a device (co_dev_t) containing an empty object in
///        the object dictionary
///
/// \When co_dev_read_sub() is called with null pointers to the memory area to
///       store an index and a sub-index, a pointer to a start of a buffer with
///       a value in the concise DCF format and a pointer to one past the last
///       byte in the buffer
///
/// \Then a number of the bytes read from the buffer is returned, nothing is
///       changed
///       \Calls co_val_read()
///       \Calls co_dev_find_sub()
TEST(CO_Dev, CoDevReadSub_NoSub) {
  CoObjTHolder obj(0x1234u);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9u;
  uint_least8_t buf[BUF_SIZE] = {0x34u, 0x12u, 0xabu, 0x02u, 0x00u,
                                 0x00u, 0x00u, 0x87u, 0x09};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  const size_t bytes_read = 9u;
  CHECK_EQUAL(bytes_read, ret);
}

/// \Given a pointer to a device (co_dev_t) containing an empty object in
///       the object dictionary
///
/// \When co_dev_read_sub() is called with null pointers to the memory area to
///       store an index and a sub-index, a null pointer to a start of a buffer
///       with a value in the concise DCF format and a pointer to one past
///       the last byte in the buffer
///
/// \Then 0 is returned, nothing is changed
TEST(CO_Dev, CoDevReadSub_NoBegin) {
  CoObjTHolder obj(0x1234u);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9u;
  uint_least8_t buf[BUF_SIZE] = {0x34u, 0x12u, 0xabu, 0x02u, 0x00u,
                                 0x00u, 0x00u, 0x87u, 0x09};

  const auto ret =
      co_dev_read_sub(dev, nullptr, nullptr, nullptr, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

/// \Given a pointer to a device (co_dev_t) containing an empty object in
///        the object dictionary
///
/// \When co_dev_read_sub() is called with null pointers to the memory area to
///       store an index and a sub-index, a pointer to a start of a buffer with
///       a value in the concise DCF format and a null pointer to one past
///       the last byte in the buffer
///
/// \Then 0 is returned, nothing is changed
TEST(CO_Dev, CoDevReadSub_NoEnd) {
  CoObjTHolder obj(0x1234u);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9u;
  uint_least8_t buf[BUF_SIZE] = {0x34u, 0x12u, 0xabu, 0x02u, 0x00u,
                                 0x00u, 0x00u, 0x87u, 0x09};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, nullptr);

  CHECK_EQUAL(0, ret);
}

/// \Given a pointer to a device (co_dev_t) containing an empty object in
///        the object dictionary
///
/// \When co_dev_read_sub() is called with null pointers to the memory area to
///       store an index and a sub-index, a pointer to a start of a buffer with
///       a value in the concise DCF format and a pointer to one past the last
///       byte in the buffer, the buffer is too small
///
/// \Then 0 is returned, nothing is changed
TEST(CO_Dev, CoDevReadSub_TooSmallBuffer) {
  CoObjTHolder obj(0x1234u);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 6;
  uint_least8_t buf[BUF_SIZE] = {0x34u, 0x12u, 0xabu, 0x01u, 0x00u, 0x00};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

/// \Given a pointer to a device (co_dev_t) containing an empty object in
///        the object dictionary
///
/// \When co_dev_read_sub() is called with null pointers to the memory area to
///       store an index and a sub-index, a pointer to a start of a buffer with
///       a value in the concise DCF format and a pointer to one past the last
///       byte in the buffer, the buffer does not contain the whole value
///
/// \Then 0 is returned, nothing is changed
///       \Calls co_val_read()
TEST(CO_Dev, CoDevReadSub_TooSmallForType) {
  CoObjTHolder obj(0x1234u);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 8;
  uint_least8_t buf[BUF_SIZE] = {0x34u, 0x12u, 0xabu, 0x02u,
                                 0x00u, 0x00u, 0x00u, 0x87};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

#ifdef HAVE_LELY_OVERRIDE
/// \Given a pointer to a device (co_dev_t) containing an empty object in
///        the object dictionary
///
/// \When co_dev_read_sub() is called with null pointers to the memory area to
///       store an index and a sub-index, a pointer to a start of a buffer with
///       a value in the concise DCF format and a pointer to one past the last
///       byte in the buffer, but co_val_read() fails
///
/// \Then 0 is returned, nothing is changed
///       \Calls co_val_read()
TEST(CO_Dev, CoDevReadSub_ReadIdxFailed) {
  CoObjTHolder obj(0x1234u);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9u;
  uint_least8_t buf[BUF_SIZE] = {0x34u, 0x12u, 0xabu, 0x02u, 0x00u,
                                 0x00u, 0x00u, 0x87u, 0x09};

  LelyOverride::co_val_read(Override::NoneCallsValid);
  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

/// \Given a pointer to a device (co_dev_t) containing an empty object in
///       the object dictionary
///
/// \When co_dev_read_sub() is called with null pointers to the memory area to
///       store an index and a sub-index, a pointer to a start of a buffer with
///       a value in the concise DCF format and a pointer to one past the last
///       byte in the buffer, but co_val_read() fails on the second call
///
/// \Then 0 is returned, nothing is changed
///       \Calls co_val_read()
TEST(CO_Dev, CoDevReadSub_ReadSubidxFailed) {
  CoObjTHolder obj(0x1234u);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9u;
  uint_least8_t buf[BUF_SIZE] = {0x34u, 0x12u, 0xabu, 0x02u, 0x00u,
                                 0x00u, 0x00u, 0x87u, 0x09};

  LelyOverride::co_val_read(1);
  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

/// \Given a pointer to a device (co_dev_t) containing an empty object in
///        the object dictionary
///
/// \When co_dev_read_sub() is called with null pointers to the memory area to
///       store an index and a sub-index, a pointer to a start of a buffer with
///       a value in the concise DCF format and a pointer to one past the last
///       byte in the buffer, but co_val_read() fails on the third call
///
/// \Then 0 is returned, nothing is changed
///       \Calls co_val_read()
TEST(CO_Dev, CoDevReadSub_ReadSizeFailed) {
  CoObjTHolder obj(0x1234u);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9u;
  uint_least8_t buf[BUF_SIZE] = {0x34u, 0x12u, 0xabu, 0x02u, 0x00u,
                                 0x00u, 0x00u, 0x87u, 0x09};

  LelyOverride::co_val_read(2);
  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

#endif  // HAVE_LELY_OVERRIDE

#if LELY_NO_MALLOC
/// \Given a pointer to a device (co_dev_t) containing an object of the array
///        type in the object dictionary
///
/// \When co_dev_read_sub() is called with a memory area to store an index and
///       a sub-index, a pointer to a start of a buffer with a value in
///       the concise DCF format and a pointer to one past the last byte in
///       the buffer
///
/// \Then a number of the bytes read from the buffer is returned, the index and
///       the sub-index are stored in the memory area, the value read from
///       the buffer is set as the entry value
///       \Calls co_val_read()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_type()
///       \Calls co_type_is_array()
///       \Calls co_type_is_array()
///       \Calls co_val_init_array()
///       \Calls co_sub_set_val()
///       \Calls co_val_addressof()
///       \Calls co_val_sizeof()
TEST(CO_Dev, CoDevReadSub_ArrayType) {
  CoObjTHolder obj(0x1234u);
  CoSubTHolder sub(0xabu, CO_DEFTYPE_OCTET_STRING);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 11u;
  uint_least8_t buf[BUF_SIZE] = {0x34u, 0x12u, 0xabu, 0x04u, 0x00u, 0x00u,
                                 0x00u, 'a',   'b',   'c',   'd'};
  co_unsigned16_t idx = 0x0000;
  co_unsigned8_t subidx = 0x00;

  const auto ret = co_dev_read_sub(dev, &idx, &subidx, buf, buf + BUF_SIZE);

  const size_t bytes_read = 11u;
  CHECK_EQUAL(bytes_read, ret);
  CHECK_EQUAL(0x1234u, idx);
  CHECK_EQUAL(0xabu, subidx);
  STRCMP_EQUAL("abcd", *reinterpret_cast<char* const*>(
                           co_dev_get_val(dev, idx, subidx)));
}
#endif  // LELY_NO_MALLOC

/// \Given a pointer to a device (co_dev_t) containing an entry in
///        the object dictionary
///
/// \When co_dev_read_sub() is called with null pointers to the memory area to
///       store an index and a sub-index, a pointer to a start of a buffer with
///       a value in the concise DCF format and a pointer to one past the last
///       byte in the buffer, the buffer contains additional byte for a value
///
/// \Then a number of the bytes read from the buffer is returned, the value is
///       not changed
///       \Calls co_val_read()
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_type()
///       \Calls co_type_is_array()
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
///       \Calls co_val_init()
TEST(CO_Dev, CoDevReadSub_ValSizeTooBig) {
  CoObjTHolder obj(0x1234u);
  CoSubTHolder sub(0xabu, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234u, 0xabu, 0x1a1au));

  const size_t BUF_SIZE = 10u;
  uint_least8_t buf[BUF_SIZE] = {0x34u, 0x12u, 0xabu, 0x03u, 0x00u,
                                 0x00u, 0x00u, 0x87u, 0x09u, 0x00};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(BUF_SIZE, ret);
  CHECK_EQUAL(0x1a1au, co_dev_get_val_i16(dev, 0x1234u, 0xabu));
}

///@}

/// @name co_dev_write_sub()
///@{

/// \Given a pointer to a device (co_dev_t) containing an entry in
///        the object dictionary
///
/// \When co_dev_write_sub() is called with an index and a sub-index, a pointer
///       to a start of an empty buffer and a pointer to one past the last byte
///       in the buffer
///
/// \Then a number of the bytes written to the buffer is returned, the buffer
///       contains the expected data in the concise DCF format
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_type()
///       \Calls co_sub_get_val()
///       \Calls co_val_write()
TEST(CO_Dev, CoDevWriteSub_Nominal) {
  CoObjTHolder obj(0x1234u);
  CoSubTHolder sub(0xabu, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234u, 0xabu, 0x0987u));

  const size_t BUF_SIZE = 9u;
  uint_least8_t buf[BUF_SIZE] = {0};

  const auto ret = co_dev_write_sub(dev, 0x1234u, 0xabu, buf, buf + BUF_SIZE);

  CHECK_EQUAL(BUF_SIZE, ret);
  uint_least8_t test_buf[] = {0x34u, 0x12u, 0xabu, 0x02u, 0x00u,
                              0x00u, 0x00u, 0x87u, 0x09};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

/// \Given a pointer to a device (co_dev_t) containing an empty object in
///        the object dictionary
///
/// \When co_dev_write_sub() is called with the index and a sub-index, a pointer
///       to a start of an empty buffer and a pointer to one past the last byte
///       in the buffer
///
/// \Then a number of the bytes that would have been written to the buffer had
///       the buffer been sufficiently large is returned, the buffer remains
///       empty
///       \Calls co_dev_find_sub()
TEST(CO_Dev, CoDevWriteSub_NoSub) {
  CoObjTHolder obj(0x1234u);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9u;
  uint_least8_t buf[BUF_SIZE] = {0};

  const auto ret = co_dev_write_sub(dev, 0x1234u, 0xabu, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
  const uint_least8_t expected[BUF_SIZE] = {0};
  CheckBuffers(expected, buf, BUF_SIZE);
}

#ifdef HAVE_LELY_OVERRIDE
/// \Given a pointer to a device (co_dev_t) containing an entry in the object
///        dictionary
///
/// \When co_dev_write_sub() is called with an index and a sub-index, a pointer
///       to a start of an empty buffer and a pointer to one past the last byte
///       in the buffer but co_val_write() fails
///
/// \Then a number of the bytes that would have been written to the buffer had
///       the buffer been sufficiently large is returned, the buffer remains
///       empty
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_type()
///       \Calls co_sub_get_val()
///       \Calls co_val_write()
///       \Calls co_val_sizeof()
TEST(CO_Dev, CoDevWriteSub_InitWriteFailed) {
  CoObjTHolder obj(0x1234u);
  CoSubTHolder sub(0xabu, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9u;
  uint_least8_t buf[BUF_SIZE] = {0};

  LelyOverride::co_val_write(Override::NoneCallsValid);
  const auto ret = co_dev_write_sub(dev, 0x1234u, 0xabu, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
  const uint_least8_t expected[BUF_SIZE] = {0};
  CheckBuffers(expected, buf, BUF_SIZE);
}
#endif

/// \Given a pointer to a device (co_dev_t) containing an entry in
///        the object dictionary, the entry is of a domain type
///
/// \When co_dev_write_sub() is called with an index and a sub-index, a pointer
///       to a start of an empty buffer and a pointer to one past the last byte
///       in the buffer
///
/// \Then a number of the bytes written to the buffer is returned, the buffer
///       contains the expected data in the concise DCF format
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_type()
///       \Calls co_sub_get_val()
///       \Calls co_val_write()
TEST(CO_Dev, CoDevWriteSub_EmptyDomain) {
  CoObjTHolder obj(0x1234u);
  CoSubTHolder sub(0xabu, CO_DEFTYPE_DOMAIN);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 7;
  uint_least8_t buf[BUF_SIZE] = {0};

  const auto ret = co_dev_write_sub(dev, 0x1234u, 0xabu, buf, buf + BUF_SIZE);

  const size_t bytes_read = 7u;
  CHECK_EQUAL(bytes_read, ret);
  uint_least8_t test_buf[] = {0x34u, 0x12u, 0xabu, 0x00u, 0x00u, 0x00u, 0x00};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

/// \Given a pointer to a device (co_dev_t) containing an entry in
///       the object dictionary
///
/// \When co_dev_write_sub() is called with an index and a sub-index and null
///       buffer pointers
///
/// \Then the number of bytes that would have been written had the buffer been
///       provided is returned, nothing is changed
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_type()
///       \Calls co_sub_get_val()
///       \Calls co_val_write()
TEST(CO_Dev, CoDevWriteSub_NoBegin) {
  CoObjTHolder obj(0x1234u);
  CoSubTHolder sub(0xabu, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const auto ret = co_dev_write_sub(dev, 0x1234u, 0xabu, nullptr, nullptr);

  CHECK_EQUAL(9, ret);
}

/// \Given a pointer to a device (co_dev_t) containing an entry in
///        the object dictionary
///
/// \When co_dev_write_sub() is called with an index and a sub-index, a pointer
///       to a start of a buffer and a null pointer
///
/// \Then a number of the bytes written to the buffer is returned, the buffer
///       contains the expected data in the concise DCF format
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_type()
///       \Calls co_sub_get_val()
///       \Calls co_val_write()
TEST(CO_Dev, CoDevWriteSub_NoEnd) {
  CoObjTHolder obj(0x1234u);
  CoSubTHolder sub(0xabu, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234u, 0xabu, 0x0987u));

  const size_t BUF_SIZE = 9u;
  uint_least8_t buf[BUF_SIZE] = {0};

  const auto ret = co_dev_write_sub(dev, 0x1234u, 0xabu, buf, nullptr);

  const size_t bytes_read = 9u;
  CHECK_EQUAL(bytes_read, ret);
  uint_least8_t test_buf[] = {0x34u, 0x12u, 0xabu, 0x02u, 0x00u,
                              0x00u, 0x00u, 0x87u, 0x09};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

/// \Given a pointer to a device (co_dev_t) containing an entry in
///        the object dictionary
///
/// \When co_dev_write_sub() is called with an index and a sub-index, a pointer
///       to a start of an empty buffer and a pointer to one past the last byte
///       in the buffer, the buffer is too small
///
/// \Then a number of the bytes that would have been written to the buffer had
///       the buffer been sufficiently large is returned, the buffer remains
///       empty
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_type()
///       \Calls co_sub_get_val()
///       \Calls co_val_write()
TEST(CO_Dev, CoDevWriteSub_TooSmallBuffer) {
  CoObjTHolder obj(0x1234u);
  CoSubTHolder sub(0xabu, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 8;
  uint_least8_t buf[BUF_SIZE] = {0};

  const auto ret = co_dev_write_sub(dev, 0x1234u, 0xabu, buf, buf + BUF_SIZE);

  const size_t bytes_read = 9u;
  CHECK_EQUAL(bytes_read, ret);
  uint_least8_t test_buf[BUF_SIZE] = {0};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

#ifdef HAVE_LELY_OVERRIDE
/// \Given a pointer to a device (co_dev_t) containing an entry in
///       the object dictionary
///
/// \When co_dev_write_sub() is called with an index and a sub-index, a pointer
///       to a start of an empty buffer and a pointer to one past the last byte
///       in the buffer but co_val_write() fails on the second call
///
/// \Then 0 is returned, the buffer remains empty
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_type()
///       \Calls co_sub_get_val()
///       \Calls co_val_write()
TEST(CO_Dev, CoDevWriteSub_IdxWriteFailed) {
  CoObjTHolder obj(0x1234u);
  CoSubTHolder sub(0xabu, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234u, 0xabu, 0x0987u));

  const size_t BUF_SIZE = 9u;
  uint_least8_t buf[BUF_SIZE] = {0};
  LelyOverride::co_val_write(1);

  const auto ret = co_dev_write_sub(dev, 0x1234u, 0xabu, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
  uint_least8_t test_buf[BUF_SIZE] = {0};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

/// \Given a pointer to a device (co_dev_t) containing an entry in
///       the object dictionary
///
/// \When co_dev_write_sub() is called with an index and a sub-index, a pointer
///       to a start of an empty buffer and a pointer to one past the last byte
///       in the buffer but co_val_write() fails on the third call
///
/// \Then 0 is returned, the buffer was partially overwritten
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_type()
///       \Calls co_sub_get_val()
///       \Calls co_val_write()
TEST(CO_Dev, CoDevWriteSub_SubidxWriteFailed) {
  CoObjTHolder obj(0x1234u);
  CoSubTHolder sub(0xabu, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234u, 0xabu, 0x0987u));

  const size_t BUF_SIZE = 9u;
  uint_least8_t buf[BUF_SIZE] = {0};
  LelyOverride::co_val_write(2);

  const auto ret = co_dev_write_sub(dev, 0x1234u, 0xabu, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
  uint_least8_t test_buf[] = {0x34u, 0x12u, 0x00u, 0x00u, 0x00u,
                              0x00u, 0x00u, 0x00u, 0x00};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

/// \Given a pointer to a device (co_dev_t) containing an entry in
///       the object dictionary
///
/// \When co_dev_write_sub() is called with an index and a sub-index, a pointer
///       to a start of an empty buffer and a pointer to one past the last byte
///       in the buffer but co_val_write() fails on the fourth call
///
/// \Then 0 is returned, the buffer was partially overwritten
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_type()
///       \Calls co_sub_get_val()
///       \Calls co_val_write()
TEST(CO_Dev, CoDevWriteSub_SizeWriteFailed) {
  CoObjTHolder obj(0x1234u);
  CoSubTHolder sub(0xabu, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234u, 0xabu, 0x0987u));

  const size_t BUF_SIZE = 9u;
  uint_least8_t buf[BUF_SIZE] = {0};
  LelyOverride::co_val_write(3);

  const auto ret = co_dev_write_sub(dev, 0x1234u, 0xabu, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
  uint_least8_t test_buf[] = {0x34u, 0x12u, 0xabu, 0x00u, 0x00u,
                              0x00u, 0x00u, 0x00u, 0x00};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

/// \Given a pointer to a device (co_dev_t) containing an entry in
///       the object dictionary
///
/// \When co_dev_write_sub() is called with an index and a sub-index, a pointer
///       to a start of an empty buffer and a pointer to one past the last byte
///       in the buffer but co_val_write() fails on the fifth call
///
/// \Then 0 is returned, the buffer was partially overwritten
///       \Calls co_dev_find_sub()
///       \Calls co_sub_get_type()
///       \Calls co_sub_get_val()
///       \Calls co_val_write()
///       \Calls co_val_sizeof()
TEST(CO_Dev, CoDevWriteSub_ValWriteFailed) {
  CoObjTHolder obj(0x1234u);
  CoSubTHolder sub(0xabu, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234u, 0xabu, 0x0987u));

  const size_t BUF_SIZE = 9u;
  uint_least8_t buf[BUF_SIZE] = {0};
  LelyOverride::co_val_write(4);

  const auto ret = co_dev_write_sub(dev, 0x1234u, 0xabu, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
  uint_least8_t test_buf[] = {0x34u, 0x12u, 0xabu, 0x02u, 0x00u,
                              0x00u, 0x00u, 0x00u, 0x00};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}
#endif

///@}

TEST_GROUP(CO_DevDCF) {
  co_dev_t* dev = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj_holder;
  std::unique_ptr<CoSubTHolder> sub_holder;
  CoArrays arrays;

  static const size_t BUF_SIZE = 13u;
  const uint_least8_t buf[BUF_SIZE] = {
      0x01u, 0x00u, 0x00u, 0x00u,  // number of sub-indices
      // value 1
      0x34u, 0x12u,                // index
      0xabu,                       // sub-index
      0x02u, 0x00u, 0x00u, 0x00u,  // size
      0x87u, 0x09                  // value
  };
  static const size_t MIN_RW_SIZE = 4u;

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();

    dev_holder.reset(new CoDevTHolder(0x01u));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    obj_holder.reset(new CoObjTHolder(0x1234u));
    sub_holder.reset(new CoSubTHolder(0xabu, CO_DEFTYPE_INTEGER16));

    obj_holder->InsertSub(*sub_holder);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));

    set_errnum(ERRNUM_SUCCESS);
  }

  TEST_TEARDOWN() {
    dev_holder.reset();
    obj_holder.reset();
    sub_holder.reset();
    arrays.Clear();
  }
};

/// @name co_dev_read_dcf()
///@{

/// \Given a pointer to a device (co_dev_t), a buffer containing a value in DCF
///        format
///
/// \When co_dev_read_dcf() is called with pointers to the memory area to store
///       the minimum object index and the maximum object index, a pointer to
///       a start of the buffer and a pointer to one past the last byte in
///       the buffer
///
/// \Then a number of bytes read from the buffer is returned, the requested
///       entry, the minimum and the maximum indices are set to the requested
///       values
///       \Calls co_val_read()
///       \Calls co_dev_read_sub()
TEST(CO_DevDCF, CoDevReadDcf_Nominal) {
  co_unsigned16_t pmin = 0x0000;
  co_unsigned16_t pmax = 0x0000;

  const auto ret = co_dev_read_dcf(dev, &pmin, &pmax, buf, buf + BUF_SIZE);

  CHECK_EQUAL(BUF_SIZE, ret);
  CHECK_EQUAL(0x0987u, co_dev_get_val_i16(dev, 0x1234u, 0xabu));
  CHECK_EQUAL(0x1234u, pmin);
  CHECK_EQUAL(0x1234u, pmax);
}

/// \Given a pointer to a device (co_dev_t), a buffer containing a value in DCF
///        format
///
/// \When co_dev_read_dcf() is called with no memory area to store the minimum
///       object index and the maximum object index, a pointer to a start of
///       the buffer and a pointer to one past the last byte in the buffer
///
/// \Then a number of bytes read from the buffer is returned, the requested
///       entry is set to the requested value
///       \Calls co_val_read()
///       \Calls co_dev_read_sub()
TEST(CO_DevDCF, CoDevReadDcf_NullMinMax) {
  const auto ret = co_dev_read_dcf(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(BUF_SIZE, ret);
  CHECK_EQUAL(0x0987u, co_dev_get_val_i16(dev, 0x1234u, 0xabu));
}

/// \Given a pointer to a device (co_dev_t), a zeroed buffer
///
/// \When co_dev_read_dcf() is called with no memory area to store the minimum
///       object index and the maximum object index, a pointer to the zeroed
///       buffer and a pointer to one past the last byte in the buffer
///
/// \Then a number of bytes read from the buffer is returned, nothing is changed
///       \Calls co_val_read()
TEST(CO_DevDCF, CoDevReadDcf_InvalidNumberOfSubIndexes) {
  const uint_least8_t empty[BUF_SIZE] = {0};

  const auto ret =
      co_dev_read_dcf(dev, nullptr, nullptr, empty, empty + BUF_SIZE);

  CHECK_EQUAL(MIN_RW_SIZE, ret);
  CHECK_EQUAL(0x0000u, co_dev_get_val_i16(dev, 0x1234u, 0xabu));
}

/// \Given a pointer to a device (co_dev_t), a buffer containing an incomplete
///        value in DCF format
///
/// \When co_dev_read_dcf() is called with no memory area to store the minimum
///       object index and the maximum object index, a pointer to a start of
///       the buffer and a pointer to one past the last byte in the buffer
///
/// \Then 0 is returned, nothing is changed
///       \Calls co_val_read()
///       \Calls co_dev_read_sub()
TEST(CO_DevDCF, CoDevReadDcf_NoSub) {
  const auto ret = co_dev_read_dcf(dev, nullptr, nullptr, buf, buf + 7u);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x0000u, co_dev_get_val_i16(dev, 0x1234u, 0xabu));
}

#if HAVE_LELY_OVERRIDE
/// \Given a pointer to a device (co_dev_t), a buffer containing a value in DCF
///        format
///
/// \When co_dev_read_dcf() is called with no memory area to store the minimum
///       object index and the maximum object index, a pointer to a start of
///       the buffer and a pointer to one past the last byte in the buffer but
///       co_val_read() fails
///
/// \Then 0 is returned, nothing is changed
///       \Calls co_val_read()
TEST(CO_DevDCF, CoDevReadDcf_FailedToReadNumberOfSubIndexes) {
  LelyOverride::co_val_read(Override::NoneCallsValid);

  const auto ret = co_dev_read_dcf(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x0000u, co_dev_get_val_i16(dev, 0x1234u, 0xabu));
}
#endif

///@}

/// @name co_dev_write_dcf()
///@{

/// \Given a pointer to a device (co_dev_t) containing an entry in
///       the object dictionary
///
/// \When co_dev_write_dcf() is called with a minimum and a maximum object
///       index, a pointer to a start of an empty buffer and a pointer to one
///       past the last byte in the buffer
///
/// \Then a number of bytes written to the buffer is returned, the buffer
///       contains the expected data in the concise DCF format
///       \Calls co_dev_first_obj()
///       \Calls co_obj_next()
///       \Calls co_obj_get_idx()
///       \Calls co_obj_first_sub()
///       \Calls co_sub_get_subidx()
///       \Calls co_dev_write_sub()
///       \Calls co_sub_next()
///       \Calls co_val_write()
TEST(CO_DevDCF, CoDevWriteDcf_Nominal) {
  co_dev_set_val_i16(dev, 0x1234u, 0xabu, 0x0987u);
  uint_least8_t tmp[BUF_SIZE] = {0};

  const auto ret = co_dev_write_dcf(dev, CO_UNSIGNED16_MIN, CO_UNSIGNED16_MAX,
                                    tmp, tmp + BUF_SIZE);

  CHECK_EQUAL(BUF_SIZE, ret);
  CheckBuffers(tmp, buf, BUF_SIZE);
}

/// \Given a pointer to a device (co_dev_t) containing an entry in
///       the object dictionary
///
/// \When co_dev_write_dcf() is called with a minimum and a maximum object
///       index, a pointer to a start of an empty buffer and a pointer to one
///       past the last byte in the buffer, minimum object index is greater than
///       the index of the contained object
///
/// \Then a number of bytes that would have been written to the buffer had
///       the buffer been sufficiently large is returned, the buffer remains
///       empty
///       \Calls co_dev_first_obj()
///       \Calls co_obj_get_idx()
///       \Calls co_val_write()
TEST(CO_DevDCF, CoDevWriteDcf_BeforeMin) {
  std::array<uint_least8_t, BUF_SIZE> tmp{0x00};

  const auto ret = co_dev_write_dcf(dev, 0x1235u, CO_UNSIGNED16_MAX, tmp.data(),
                                    tmp.data() + tmp.size());

  CHECK_EQUAL(MIN_RW_SIZE, ret);
  for (const auto& item : tmp) CHECK_EQUAL(0, item);
}

/// \Given a pointer to a device (co_dev_t) containing an entry in
///       the object dictionary
///
/// \When co_dev_write_dcf() is called with a minimum and a maximum object
///       index, a pointer to a start of an empty buffer and a pointer to one
///       past the last byte in the buffer, maximum object index is lower than
///       the index of the contained object
///
/// \Then a number of bytes that would have been written to the buffer had
///       the buffer been sufficiently large is returned, the buffer remains
///       empty
///       \Calls co_dev_first_obj()
///       \Calls co_obj_get_idx()
///       \Calls co_val_write()
TEST(CO_DevDCF, CoDevWriteDcf_AfterMax) {
  std::array<uint_least8_t, BUF_SIZE> tmp{0x00};

  const auto ret = co_dev_write_dcf(dev, CO_UNSIGNED16_MIN, 0x1233u, tmp.data(),
                                    tmp.data() + tmp.size());

  CHECK_EQUAL(MIN_RW_SIZE, ret);
  for (const auto& item : tmp) CHECK_EQUAL(0, item);
}

#if LELY_NO_MALLOC
/// \Given a pointer to a device (co_dev_t) containing an entry in
///        the object dictionary
///
/// \When co_dev_write_dcf() is called with a minimum and a maximum object index
///       and null buffer pointers
///
/// \Then a number of bytes that would have been written had the buffer been
///       sufficiently large is returned, nothing is changed
///       \Calls co_dev_first_obj()
///       \Calls co_obj_get_idx()
///       \Calls co_obj_first_sub()
///       \Calls co_sub_get_subidx()
///       \Calls co_dev_write_sub()
///       \Calls co_obj_next()
TEST(CO_DevDCF, CoDevWriteDcf_Null) {
  co_dev_set_val_i16(dev, 0x1234u, 0xabu, 0x0987u);

  const auto ret = co_dev_write_dcf(dev, CO_UNSIGNED16_MIN, CO_UNSIGNED16_MAX,
                                    nullptr, nullptr);

  CHECK_EQUAL(MIN_RW_SIZE + sizeof(co_unsigned16_t) + 2 + 1 + 4, ret);
}
#endif

#if HAVE_LELY_OVERRIDE
/// \Given a pointer to a device (co_dev_t) containing an entry in
///        the object dictionary
///
/// \When co_dev_write_dcf() is called with a minimum and a maximum object
///       index, a pointer to a start of an empty buffer and a pointer to one
///       past the last byte in the buffer but co_val_write() fails
///
/// \Then 0 is returned, the buffer remains empty
///       \Calls co_dev_first_obj()
///       \Calls co_obj_get_idx()
///       \Calls co_obj_first_sub()
///       \Calls co_sub_get_subidx()
///       \Calls co_dev_write_sub()
TEST(CO_DevDCF, CoDevWriteDcf_FailedToWriteSubObject) {
  co_dev_set_val_i16(dev, 0x1234u, 0xabu, 0x0987u);
  std::array<uint_least8_t, BUF_SIZE> buf{0x00};

  LelyOverride::co_val_write(Override::NoneCallsValid);
  const auto ret = co_dev_write_dcf(dev, CO_UNSIGNED16_MIN, CO_UNSIGNED16_MAX,
                                    buf.data(), buf.data() + buf.size());

  CHECK_EQUAL(0, ret);
  for (const auto& item : buf) CHECK_EQUAL(0, item);
}
#endif

///@}

#if !LELY_NO_CO_TPDO
namespace CO_DevTPDO_Static {
static unsigned int tpdo_event_ind_counter = 0;
static co_unsigned16_t tpdo_event_ind_last_pdo_num = 0;
}  // namespace CO_DevTPDO_Static

TEST_BASE(CO_DevTpdoBase) {
  TEST_BASE_SUPER(CO_DevTpdoBase);

  const co_unsigned8_t DEV_ID = 0x01u;
  std::unique_ptr<CoDevTHolder> dev_holder;
  co_dev_t* dev = nullptr;

  static void tpdo_event_ind(co_unsigned16_t pdo_num, void*) {
    ++CO_DevTPDO_Static::tpdo_event_ind_counter;
    CO_DevTPDO_Static::tpdo_event_ind_last_pdo_num = pdo_num;
  }

  TEST_SETUP() {
    LelyUnitTest::DisableDiagnosticMessages();

    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    CO_DevTPDO_Static::tpdo_event_ind_counter = 0;
    CO_DevTPDO_Static::tpdo_event_ind_last_pdo_num = 0;

    set_errnum(ERRNUM_SUCCESS);
  }

  TEST_TEARDOWN() { dev_holder.reset(); }
};

TEST_GROUP_BASE(CO_DevTpdoEventInd, CO_DevTpdoBase){};

/// @name co_dev_get_tpdo_event_ind()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_get_tpdo_event_ind() is called with no memory area to store
///       the results
///
/// \Then nothing is changed
TEST(CO_DevTpdoEventInd, CoDevGetTpdoEventInd_Null) {
  co_dev_get_tpdo_event_ind(dev, nullptr, nullptr);
}

///@}

/// @name co_dev_set_tpdo_event_ind()
///@{

/// \Given a pointer to a device (co_dev_t)
///
/// \When co_dev_set_tpdo_event_ind() is called with a pointer to a TPDO event
///       indication function and a pointer to a user-specified data
///
/// \Then the requested TPDO event indication function is set
TEST(CO_DevTpdoEventInd, CoDevSetTpdoEventInd_Nominal) {
  int data = 42;
  co_dev_set_tpdo_event_ind(dev, tpdo_event_ind, &data);

  co_dev_tpdo_event_ind_t* ind_ptr = nullptr;
  void* data_ptr = nullptr;
  co_dev_get_tpdo_event_ind(dev, &ind_ptr, &data_ptr);
  FUNCTIONPOINTERS_EQUAL(tpdo_event_ind, ind_ptr);
  POINTERS_EQUAL(&data, data_ptr);
}

///@}

TEST_GROUP_BASE(CO_DevTpdoEvent, CO_DevTpdoBase) {
  const co_unsigned16_t OBJ_IDX = 0x1234u;
  const co_unsigned8_t SUB_IDX = 0xabu;
  const co_unsigned8_t SUB_SIZE = 16u;

  std::unique_ptr<CoObjTHolder> obj_holder;
  std::unique_ptr<CoSubTHolder> sub_holder;
  co_sub_t* sub = nullptr;

  std::vector<std::unique_ptr<CoObjTHolder>> tpdo_objects;
  std::vector<std::unique_ptr<CoObjTHolder>> tpdo_mappings;

  void CreateTpdoCommObject(co_unsigned32_t cobid, co_unsigned8_t transmission,
                            co_unsigned16_t tpdo_num = 1) {
    std::unique_ptr<CoObjTHolder> obj1800(
        new CoObjTHolder(0x1800u + tpdo_num - 1));
    obj1800->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t{0x02u});
    obj1800->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32, cobid);
    obj1800->InsertAndSetSub(0x02u, CO_DEFTYPE_UNSIGNED8, transmission);

    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1800->Take()));

    tpdo_objects.push_back(std::move(obj1800));
  }

  void CreateAcyclicTpdoCommObject(co_unsigned16_t tpdo_num = 1) {
    CreateTpdoCommObject(DEV_ID, 0x00u, tpdo_num);
  }

  void CreateSingleEntryMapping(co_unsigned32_t mapping,
                                co_unsigned16_t tpdo_num = 1) {
    std::unique_ptr<CoObjTHolder> obj1a00(
        new CoObjTHolder(0x1a00u + tpdo_num - 1));
    obj1a00->InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8,
                             co_unsigned8_t{0x01u});
    obj1a00->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32, mapping);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1a00->Take()));

    tpdo_mappings.push_back(std::move(obj1a00));
  }

  static co_unsigned32_t EncodeMapping(co_unsigned16_t obj_idx,
                                       co_unsigned8_t sub_idx,
                                       co_unsigned8_t num_bits) {
    co_unsigned32_t encoding = 0;

    encoding |= obj_idx << 16;
    encoding |= sub_idx << 8;
    encoding |= num_bits;

    return encoding;
  }

  void CreateTestObject() {
    obj_holder.reset(new CoObjTHolder(OBJ_IDX));
    sub_holder.reset(new CoSubTHolder(SUB_IDX, CO_DEFTYPE_INTEGER16));

    sub = obj_holder->InsertSub(*sub_holder);
    CHECK(sub != nullptr);

    co_sub_set_pdo_mapping(sub, 1);

    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));
  }

  TEST_SETUP() {
    TEST_BASE_SETUP();

    CreateTestObject();
    co_dev_set_tpdo_event_ind(dev, tpdo_event_ind, nullptr);

    set_errnum(ERRNUM_SUCCESS);
  }

  TEST_TEARDOWN() {
    tpdo_objects.clear();
    tpdo_mappings.clear();
    sub_holder.reset();
    obj_holder.reset();

    TEST_BASE_TEARDOWN();
  }
};

/// @name co_dev_tpdo_event()
///@{

/// \Given a pointer to a device (co_dev_t) with a custom TPDO event indication
///        function set
///
/// \When co_dev_tpdo_event() is called with an index and a sub-index of a
///       non-existing sub-object
///
/// \Then the indication function was not called
///       \Calls co_dev_chk_tpdo()
TEST(CO_DevTpdoEvent, CoDevTpdoEvent_InvalidIndices) {
  co_dev_tpdo_event(dev, 0x0000u, 0x00u);

  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

/// \Given a pointer to a device (co_dev_t) with a custom TPDO event indication
///        function set, the object dictionary contains an entry with disabled
///        PDO mapping
///
/// \When co_dev_tpdo_event() is called with the index and sub-index of the
///       entry
///
/// \Then the indication function was not called
///       \Calls co_dev_chk_tpdo()
TEST(CO_DevTpdoEvent, CoDevTpdoEvent_OnlySubNoMapping) {
  co_sub_set_pdo_mapping(sub, 0);

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

/// \Given a pointer to a device (co_dev_t) with a custom TPDO event indication
///        function set, the object dictionary contains an entry with enabled
///        PDO mapping and does not contain any TPDO Communication Parameter
///        objects (0x1800-0x19ff)
///
/// \When co_dev_tpdo_event() is called with the index and sub-index of the
///       entry
///
/// \Then the indication function was not called
///       \Calls co_dev_chk_tpdo()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_next()
///       \Calls co_obj_get_idx()
///       \Calls co_obj_addressof_val()
TEST(CO_DevTpdoEvent, CoDevTpdoEvent_MappingPossibleButNoMapping) {
  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

/// \Given a pointer to a device (co_dev_t) with a custom TPDO event indication
///        function set, the object dictionary contains an entry with enabled
///        PDO mapping and a TPDO Communication Parameter object (0x1800) with
///        the Highest Sub-index Supported sub-object (0x00) outside allowed
///        value range
///
/// \When co_dev_tpdo_event() is called with the index and sub-index of the
///       entry
///
/// \Then the indication function was not called
///       \Calls co_dev_chk_tpdo()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_next()
///       \Calls co_obj_get_idx()
///       \Calls co_obj_addressof_val()
TEST(CO_DevTpdoEvent, CoDevTpdoEvent_InvalidTpdoMaxSubIndex) {
  CoObjTHolder obj1800(0x1800u);
  obj1800.InsertAndSetSub(
      0x00u, CO_DEFTYPE_UNSIGNED8,
      co_unsigned8_t{0x00u});  // highest sub-index supported
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1800.Take()));

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

/// \Given a pointer to a device (co_dev_t) with a custom TPDO event indication
///        function set, the object dictionary contains an entry with enabled
///        PDO mapping and a TPDO Communication Parameter object (0x1800) with
///        an invalid COB-ID of the TPDO
///
/// \When co_dev_tpdo_event() is called with the index and sub-index of the
///       entry
///
/// \Then the indication function was not called
///       \Calls co_dev_chk_tpdo()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_next()
///       \Calls co_obj_get_idx()
///       \Calls co_obj_addressof_val()
TEST(CO_DevTpdoEvent, CoDevTpdoEvent_InvalidTpdoCobId) {
  CreateTpdoCommObject(DEV_ID | CO_PDO_COBID_VALID, 0x00u);

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

/// \Given a pointer to a device (co_dev_t) with a custom TPDO event indication
///        function set, the object dictionary contains an entry with enabled
///        PDO mapping and a TPDO Communication Parameter object (0x1800) with a
///        reserved TPDO transmission type
///
/// \When co_dev_tpdo_event() is called with the index and sub-index of the
///       entry
///
/// \Then the indication function was not called
///       \Calls co_dev_chk_tpdo()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_next()
///       \Calls co_obj_get_idx()
///       \Calls co_obj_addressof_val()
TEST(CO_DevTpdoEvent, CoDevTpdoEvent_ReservedTransmissionType) {
  CreateTpdoCommObject(DEV_ID, 0xf1u);

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

/// \Given a pointer to a device (co_dev_t) with a custom TPDO event indication
///        function set, the object dictionary contains an entry with enabled
///        PDO mapping and a TPDO Communication Parameter object (0x1800) with
///        acyclic TPDO transmission type but does not contain the matching TPDO
///        Mapping Parameter object (0x1a00)
///
/// \When co_dev_tpdo_event() is called with the index and sub-index of the
///       entry
///
/// \Then the indication function was not called
///       \Calls co_dev_chk_tpdo()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_next()
///       \Calls co_obj_get_idx()
///       \Calls co_obj_addressof_val()
TEST(CO_DevTpdoEvent, CoDevTpdoEvent_NoTpdoMapping) {
  CreateAcyclicTpdoCommObject();

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

/// \Given a pointer to a device (co_dev_t) with a custom TPDO event indication
///        function set, the object dictionary contains an entry with enabled
///        PDO mapping, a TPDO Communication Parameter object (0x1800) with
///        acyclic TPDO transmission type and the matching TPDO Mapping
///        Parameter object (0x1a00) with a mapping using a different object
///        index
///
/// \When co_dev_tpdo_event() is called with the index and sub-index of the
///       entry
///
/// \Then the indication function was not called
///       \Calls co_dev_chk_tpdo()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_next()
///       \Calls co_obj_get_idx()
///       \Calls co_obj_addressof_val()
TEST(CO_DevTpdoEvent, CoDevTpdoEvent_DifferentObjectIndexInMapping) {
  CreateAcyclicTpdoCommObject();
  CreateSingleEntryMapping(EncodeMapping(OBJ_IDX - 0x100, SUB_IDX, SUB_SIZE));

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

/// \Given a pointer to a device (co_dev_t) with a custom TPDO event indication
///        function set, the object dictionary contains an entry with enabled
///        PDO mapping, a TPDO Communication Parameter object (0x1800) with
///        acyclic TPDO transmission type and the matching TPDO Mapping
///        Parameter object (0x1a00) with a mapping using a different sub-index
///
/// \When co_dev_tpdo_event() is called with the index and sub-index of the
///       entry
///
/// \Then the indication function was not called
///       \Calls co_dev_chk_tpdo()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_next()
///       \Calls co_obj_get_idx()
///       \Calls co_obj_addressof_val()
TEST(CO_DevTpdoEvent, CoDevTpdoEvent_DifferentSubIndexInMapping) {
  CreateAcyclicTpdoCommObject();
  CreateSingleEntryMapping(EncodeMapping(OBJ_IDX, SUB_IDX + 10, SUB_SIZE));

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

/// \Given a pointer to a device (co_dev_t) with no custom TPDO event indication
///        function set, the object dictionary contains an entry with enabled
///        PDO mapping, a TPDO Communication Parameter object (0x1800) with
///        acyclic TPDO transmission type and the matching TPDO Mapping
///        Parameter object (0x1a00) with the entry mapped
///
/// \When co_dev_tpdo_event() is called with the index and sub-index of the
///       entry
///
/// \Then nothing is changed
///       \Calls co_dev_chk_tpdo()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_next()
///       \Calls co_obj_get_idx()
///       \Calls co_obj_addressof_val()
TEST(CO_DevTpdoEvent, CoDevTpdoEvent_NoIndicationFunction) {
  CreateAcyclicTpdoCommObject();
  CreateSingleEntryMapping(EncodeMapping(OBJ_IDX, SUB_IDX, SUB_SIZE));
  co_dev_set_tpdo_event_ind(dev, nullptr, nullptr);

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

/// \Given a pointer to a device (co_dev_t) with a custom TPDO event indication
///        function set, the object dictionary contains an entry with enabled
///        PDO mapping, a TPDO Communication Parameter object (0x1800) with
///        acyclic TPDO transmission type and the matching TPDO Mapping
///        Parameter object (0x1a00) with the entry mapped
///
/// \When co_dev_tpdo_event() is called with the index and sub-index of the
///       entry
///
/// \Then the indication function was called once
///       \Calls co_dev_chk_tpdo()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_next()
///       \Calls co_obj_get_idx()
///       \Calls co_obj_addressof_val()
TEST(CO_DevTpdoEvent, CoDevTpdoEvent_ValidAcyclicTpdo) {
  CreateAcyclicTpdoCommObject();
  CreateSingleEntryMapping(EncodeMapping(OBJ_IDX, SUB_IDX, SUB_SIZE));

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(1, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

/// \Given a pointer to a device (co_dev_t) with a custom TPDO event indication
///        function set, the object dictionary contains an entry with enabled
///        PDO mapping, a TPDO Communication Parameter object (0x1800) with an
///        event-driven TPDO transmission type and the matching TPDO Mapping
///        Parameter object (0x1a00) with the entry mapped
///
/// \When co_dev_tpdo_event() is called with the index and sub-index of the
///       entry
///
/// \Then the indication function was called once
///       \Calls co_dev_chk_tpdo()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_next()
///       \Calls co_obj_get_idx()
///       \Calls co_obj_addressof_val()
TEST(CO_DevTpdoEvent, CoDevTpdoEvent_ValidEventDrivenTpdo) {
  CreateTpdoCommObject(DEV_ID, 0xfeu);
  CreateSingleEntryMapping(EncodeMapping(OBJ_IDX, SUB_IDX, SUB_SIZE));

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(1, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

/// \Given a pointer to a device (co_dev_t) with a custom TPDO event indication
///        function set, the object dictionary contains an entry with enabled
///        PDO mapping, multiple TPDO Communication Parameter objects
///        (0x1800-0x19ff) with acyclic TPDO transmission types and matching
///        TPDO Mapping Parameter objects (0x1a00-0x1bff), the entry is mapped
///        in at least one of them
///
/// \When co_dev_tpdo_event() is called with the index and sub-index of the
///       entry
///
/// \Then the indication function was called once for every matched TPDO in
///       order of increasing PDO numbers
///       \Calls co_dev_chk_tpdo()
///       \Calls co_dev_find_obj()
///       \Calls co_obj_next()
///       \Calls co_obj_get_idx()
///       \Calls co_obj_addressof_val()
TEST(CO_DevTpdoEvent, CoDevTpdoEvent_CallsIndicationFunction_ForMatchedTpdos) {
  CreateAcyclicTpdoCommObject(10);
  CreateAcyclicTpdoCommObject(20);
  CreateAcyclicTpdoCommObject(30);
  CreateAcyclicTpdoCommObject(40);
  CreateSingleEntryMapping(EncodeMapping(OBJ_IDX, SUB_IDX - 10, SUB_SIZE), 10);
  CreateSingleEntryMapping(EncodeMapping(OBJ_IDX, SUB_IDX, SUB_SIZE), 20);
  CreateSingleEntryMapping(EncodeMapping(OBJ_IDX, SUB_IDX, SUB_SIZE), 30);
  CreateSingleEntryMapping(EncodeMapping(OBJ_IDX, SUB_IDX + 10, SUB_SIZE), 40);

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(2, CO_DevTPDO_Static::tpdo_event_ind_counter);
  CHECK_EQUAL(30, CO_DevTPDO_Static::tpdo_event_ind_last_pdo_num);
}

///@}

#endif  // !LELY_NO_CO_TPDO
