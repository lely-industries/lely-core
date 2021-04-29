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

#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

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

  void DestroyCoDevT(co_dev_t* const dev) {
    co_dev_fini(dev);
    ReleaseCoDevT(dev);
  }

#if LELY_NO_MALLOC
  co_dev_t device;
#endif
};

#if !LELY_NO_MALLOC
TEST(CO_DevInit, CODevAllocFree) {
  void* const ptr = co_dev_alloc();

  CHECK(ptr != nullptr);

  co_dev_free(ptr);
}
#endif  // !LELY_NO_MALLOC

TEST(CO_DevInit, CODevInit) {
  auto* const dev = AcquireCoDevT();

  CHECK(dev != nullptr);
  POINTERS_EQUAL(dev, co_dev_init(dev, 0x01));

  CHECK_EQUAL(0, co_dev_get_netid(dev));
  CHECK_EQUAL(0x01, co_dev_get_id(dev));

  CHECK_EQUAL(0, co_dev_get_idx(dev, 0, nullptr));

  CHECK_EQUAL(0, co_dev_get_vendor_id(dev));
  CHECK_EQUAL(0, co_dev_get_product_code(dev));
  CHECK_EQUAL(0, co_dev_get_revision(dev));

#if !LELY_NO_CO_OBJ_NAME
  POINTERS_EQUAL(nullptr, co_dev_get_name(dev));
  POINTERS_EQUAL(nullptr, co_dev_get_vendor_name(dev));
  POINTERS_EQUAL(nullptr, co_dev_get_product_name(dev));
  POINTERS_EQUAL(nullptr, co_dev_get_order_code(dev));
#endif  // !LELY_NO_CO_OBJ_NAME

  CHECK_EQUAL(0, co_dev_get_baud(dev));
  CHECK_EQUAL(0, co_dev_get_rate(dev));

  CHECK_EQUAL(0, co_dev_get_lss(dev));

  CHECK_EQUAL(0, co_dev_get_dummy(dev));

#if !LELY_NO_CO_TPDO
  co_dev_tpdo_event_ind_t* ind_ptr = nullptr;
  void* data_ptr = nullptr;
  co_dev_get_tpdo_event_ind(dev, &ind_ptr, &data_ptr);
  FUNCTIONPOINTERS_EQUAL(nullptr, ind_ptr);
  POINTERS_EQUAL(nullptr, data_ptr);
#endif

  DestroyCoDevT(dev);
}

TEST(CO_DevInit, CODevInit_UnconfiguredId) {
  auto* const dev = AcquireCoDevT();

  CHECK(dev != nullptr);
  POINTERS_EQUAL(dev, co_dev_init(dev, 0xff));

  CoObjTHolder obj1(0x0000);
  CoObjTHolder obj2(0x0001);
  CoObjTHolder obj3(0xffff);
  CHECK(obj1.Get() != nullptr);
  CHECK(obj2.Get() != nullptr);
  CHECK(obj3.Get() != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1.Take()));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2.Take()));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj3.Take()));

#if !LELY_NO_CO_OBJ_NAME
  CHECK_EQUAL(0, co_dev_set_name(dev, "name"));
  CHECK_EQUAL(0, co_dev_set_vendor_name(dev, "vendor"));
  CHECK_EQUAL(0, co_dev_set_product_name(dev, "product name"));
  CHECK_EQUAL(0, co_dev_set_order_code(dev, "order code"));
#endif  // !LELY_NO_CO_OBJ_NAME

  DestroyCoDevT(dev);
}

TEST(CO_DevInit, CODevInit_ZeroId) {
  auto* const dev = AcquireCoDevT();

  CHECK(dev != nullptr);
  POINTERS_EQUAL(nullptr, co_dev_init(dev, 0x00));

  ReleaseCoDevT(dev);
}

TEST(CO_DevInit, CODevInit_InvalidId) {
  auto* const dev = AcquireCoDevT();
  CHECK(dev != nullptr);

  POINTERS_EQUAL(nullptr, co_dev_init(dev, CO_NUM_NODES + 1));
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());

  POINTERS_EQUAL(nullptr, co_dev_init(dev, 0xff - 1));
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());

  ReleaseCoDevT(dev);
}

TEST(CO_DevInit, CODevFini) {
  auto* const dev = AcquireCoDevT();

  CHECK(dev != nullptr);
  POINTERS_EQUAL(dev, co_dev_init(dev, 0x01));

  ReleaseCoDevT(dev);
}

#if !LELY_NO_MALLOC
TEST(CO_DevInit, CODevDestroy_Null) { co_dev_destroy(nullptr); }
#endif

TEST_GROUP(CO_Dev) {
  co_dev_t* dev = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;

  TEST_SETUP() {
    dev_holder.reset(new CoDevTHolder(0x01));
    dev = dev_holder->Get();

    CHECK(dev != nullptr);
  }

  TEST_TEARDOWN() { dev_holder.reset(); }
};

TEST(CO_Dev, CoDevSetNetId) {
  const auto ret = co_dev_set_netid(dev, 0x3d);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x3d, co_dev_get_netid(dev));
}

TEST(CO_Dev, CoDevSetNetId_Unconfigured) {
  const auto ret = co_dev_set_netid(dev, 0xff);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0xff, co_dev_get_netid(dev));
}

TEST(CO_Dev, CoDevSetNetId_InvalidId) {
  const auto ret1 = co_dev_set_netid(dev, CO_NUM_NETWORKS + 1);

  CHECK_EQUAL(-1, ret1);
  CHECK_EQUAL(0, co_dev_get_netid(dev));

  const auto ret2 = co_dev_set_netid(dev, 0xff - 1);

  CHECK_EQUAL(-1, ret2);
  CHECK_EQUAL(0, co_dev_get_netid(dev));
}

TEST(CO_Dev, CoDevSetId) {
  const auto ret = co_dev_set_id(dev, 0x3d);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x3d, co_dev_get_id(dev));
}

TEST(CO_Dev, CoDevSetId_CheckObj) {
  CoObjTHolder obj_holder(0x0000);
#if !LELY_NO_CO_OBJ_LIMITS
  CoObjTHolder obj1_holder(0x0001);
  CoObjTHolder obj2_holder(0x1234);
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
  CoObjTHolder obj3_holder(0xffff);
#endif
#if !LELY_NO_CO_OBJ_LIMITS
  CoSubTHolder sub_min1_holder(0x00, CO_DEFTYPE_INTEGER16);
  CoSubTHolder sub_min2_holder(0x01, CO_DEFTYPE_INTEGER16);
  CoSubTHolder sub_max1_holder(0x00, CO_DEFTYPE_INTEGER16);
  CoSubTHolder sub_max2_holder(0x01, CO_DEFTYPE_INTEGER16);
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
  CoSubTHolder sub_def1_holder(0x00, CO_DEFTYPE_INTEGER16);
  CoSubTHolder sub_def2_holder(0x01, CO_DEFTYPE_INTEGER16);
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
  CHECK_EQUAL(0x3f00, *static_cast<const co_integer16_t*>(
                          co_sub_get_max(co_obj_first_sub(out_obj))));
  CHECK_EQUAL(0x3f00 + new_id, *static_cast<const co_integer16_t*>(
                                   co_sub_get_max(co_obj_last_sub(out_obj))));
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
  out_obj = co_obj_next(out_obj);
  CHECK_EQUAL(0x1234, *static_cast<const co_integer16_t*>(
                          co_sub_get_def(co_obj_first_sub(out_obj))));
  CHECK_EQUAL(0x1234 + new_id, *static_cast<const co_integer16_t*>(
                                   co_sub_get_def(co_obj_last_sub(out_obj))));
#endif
}

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Dev, CoDevSetId_CoType_##a) { \
    CoObjTHolder obj_holder(0x1234); \
    CoSubTHolder sub_holder(0xab, CO_DEFTYPE_##a); \
    co_sub_t* const sub = obj_holder.InsertSub(sub_holder); \
    CHECK(sub != nullptr); \
    co_obj_t* const obj = obj_holder.Take(); \
    CHECK_EQUAL(co_type_sizeof(CO_DEFTYPE_##a), \
                co_sub_set_val_##c(sub, 0x42 + co_dev_get_id(dev))); \
    co_sub_set_flags(sub, CO_OBJ_FLAGS_VAL_NODEID); \
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj)); \
    const co_unsigned8_t new_id = 0x14; \
    const auto ret = co_dev_set_id(dev, new_id); \
    CHECK_EQUAL(0, ret); \
    CHECK_EQUAL(new_id, co_dev_get_id(dev)); \
    const co_obj_t* const out_obj = co_dev_first_obj(dev); \
    CHECK_EQUAL(static_cast<co_##b##_t>(0x42 + new_id), \
                *static_cast<const co_##b##_t*>( \
                    co_sub_get_val(co_obj_first_sub(out_obj)))); \
  }
#include <lely/co/def/basic.def>
#undef LELY_CO_DEFINE_TYPE

TEST(CO_Dev, CoDevSetId_CoType_NonBasic) {
  CoObjTHolder obj_holder(0x1234);
  CoSubTHolder sub_holder(0x01, CO_DEFTYPE_TIME_OF_DAY);
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
  CHECK(val != nullptr);
  const auto* const val_ret = static_cast<const co_val*>(val);
  CHECK_EQUAL(value.ms, val_ret->t.ms);
  CHECK_EQUAL(value.days, val_ret->t.days);
}

TEST(CO_Dev, CoDevSetId_Unconfigured) {
  const auto ret = co_dev_set_id(dev, 0xff);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0xff, co_dev_get_id(dev));
}

TEST(CO_Dev, CoDevSetId_ZeroId) {
  const auto ret = co_dev_set_id(dev, 0x00);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0x01, co_dev_get_id(dev));
}

TEST(CO_Dev, CoDevSetId_InvalidId) {
  const auto ret1 = co_dev_set_id(dev, CO_NUM_NETWORKS + 1);

  CHECK_EQUAL(-1, ret1);
  CHECK_EQUAL(0x01, co_dev_get_id(dev));

  const auto ret2 = co_dev_set_id(dev, 0xff - 1);

  CHECK_EQUAL(-1, ret2);
  CHECK_EQUAL(0x01, co_dev_get_id(dev));
}

TEST(CO_Dev, CoDevGetIdx_Empty) {
  co_unsigned16_t out_idx = 0x0000;
  const auto ret = co_dev_get_idx(dev, 1, &out_idx);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x0000, out_idx);
}

TEST(CO_Dev, CoDevGetIdx_EmptyNull) {
  const auto ret = co_dev_get_idx(dev, 0, nullptr);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Dev, CoDevGetIdx_OneObjCheckNumber) {
  CoObjTHolder obj(0x0000);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const auto ret = co_dev_get_idx(dev, 0xffff, nullptr);

  CHECK_EQUAL(1, ret);
}

TEST(CO_Dev, CoDevGetIdx_OneObjCheckIdx) {
  CoObjTHolder obj(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  co_unsigned16_t out_idx = 0x0000;
  const auto ret = co_dev_get_idx(dev, 1, &out_idx);

  CHECK_EQUAL(1, ret);
  CHECK_EQUAL(0x1234, out_idx);
}

TEST(CO_Dev, CoDevGetIdx_ManyObj1) {
  CoObjTHolder obj1(0x0000);
  CoObjTHolder obj2(0x1234);
  CoObjTHolder obj3(0xffff);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1.Take()));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2.Take()));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj3.Take()));

  co_unsigned16_t out_idx[5] = {0x0000, 0x0000, 0x0000, 0x0000, 0x0000};
  const auto ret = co_dev_get_idx(dev, 5, out_idx);

  CHECK_EQUAL(3, ret);
  CHECK_EQUAL(0x0000, out_idx[0]);
  CHECK_EQUAL(0x1234, out_idx[1]);
  CHECK_EQUAL(0xffff, out_idx[2]);
  CHECK_EQUAL(0x0000, out_idx[3]);
  CHECK_EQUAL(0x0000, out_idx[4]);
}

TEST(CO_Dev, CoDevGetIdx_ManyObj2) {
  CoObjTHolder obj1(0x0000);
  CoObjTHolder obj2(0x1234);
  CoObjTHolder obj3(0xffff);
  CoObjTHolder obj4(0xabcd);
  CoObjTHolder obj5(0x1010);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1.Take()));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2.Take()));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj3.Take()));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj4.Take()));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj5.Take()));

  co_unsigned16_t out_idx[5] = {0x0000, 0x0000, 0x0000, 0x0000, 0x0000};
  const auto ret = co_dev_get_idx(dev, 3, out_idx);

  CHECK_EQUAL(5, ret);
  CHECK_EQUAL(0x0000, out_idx[0]);
  CHECK_EQUAL(0x1010, out_idx[1]);
  CHECK_EQUAL(0x1234, out_idx[2]);
  CHECK_EQUAL(0x0000, out_idx[3]);
  CHECK_EQUAL(0x0000, out_idx[4]);
}

TEST(CO_Dev, CoDevInsertObj) {
  CoObjTHolder obj_holder(0x1234);
  co_obj_t* const obj = obj_holder.Take();

  const auto ret = co_dev_insert_obj(dev, obj);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(obj, co_dev_first_obj(dev));
  co_unsigned16_t out_idx = 0x0000;
  CHECK_EQUAL(1, co_dev_get_idx(dev, 1, &out_idx));
  CHECK_EQUAL(0x1234, out_idx);
  CHECK_EQUAL(dev, co_obj_get_dev(obj));
}

TEST(CO_Dev, CoDevInsertObj_AddedToOtherDev) {
  CoDevTHolder other_dev_holder(0x02);
  CoObjTHolder obj_holder(0x0001);
  co_dev_t* const other_dev = other_dev_holder.Get();
  co_obj_t* const obj = obj_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(other_dev, obj));

  const auto ret = co_dev_insert_obj(dev, obj);

  CHECK_EQUAL(-1, ret);
}

TEST(CO_Dev, CoDevInsertObj_AlreadyAdded) {
  CoObjTHolder obj_holder(0x00001);
  co_obj_t* const obj = obj_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto ret = co_dev_insert_obj(dev, obj);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Dev, CoDevInsertObj_AlreadyAddedAtIdx) {
  CoObjTHolder obj1(0x0001);
  CoObjTHolder obj2(0x0001);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1.Take()));

  const auto ret = co_dev_insert_obj(dev, obj2.Get());

  CHECK_EQUAL(-1, ret);
}

TEST(CO_Dev, CoDevRemoveObj) {
  CoObjTHolder obj_holder(0x1234);
  co_obj_t* const obj = obj_holder.Get();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto ret = co_dev_remove_obj(dev, obj);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_dev_get_idx(dev, 0, nullptr));
  POINTERS_EQUAL(nullptr, co_obj_get_dev(obj));
}

TEST(CO_Dev, CoDevRemoveObj_NotAdded) {
  CoObjTHolder obj(0x1234);

  const auto ret = co_dev_remove_obj(dev, obj.Get());

  CHECK_EQUAL(-1, ret);
}

TEST(CO_Dev, CoDevFindObj) {
  CoObjTHolder obj_holder(0x1234);
  co_obj_t* const obj = obj_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto* const ret = co_dev_find_obj(dev, 0x1234);

  POINTERS_EQUAL(obj, ret);
}

TEST(CO_Dev, CoDevFindObj_NotFound) {
  const auto* const ret = co_dev_find_obj(dev, 0x1234);

  POINTERS_EQUAL(nullptr, ret);
}

TEST(CO_Dev, CoDevFindSub) {
  CoObjTHolder obj_holder(0x1234);
  CoSubTHolder sub_holder(0xab, CO_DEFTYPE_INTEGER16);
  co_sub_t* const sub = obj_holder.InsertSub(sub_holder);
  CHECK(sub != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder.Take()));

  const auto* const ret = co_dev_find_sub(dev, 0x1234, 0xab);

  POINTERS_EQUAL(sub, ret);
}

TEST(CO_Dev, CoDevFindSub_NoObj) {
  const auto* const ret = co_dev_find_sub(dev, 0x1234, 0x00);

  POINTERS_EQUAL(nullptr, ret);
}

TEST(CO_Dev, CoDevFindSub_NoSub) {
  CoObjTHolder obj(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const auto* const ret = co_dev_find_sub(dev, 0x1234, 0x00);

  POINTERS_EQUAL(nullptr, ret);
}

TEST(CO_Dev, CoDevFirstObj) {
  CoObjTHolder obj_holder(0x1234);
  const auto obj = obj_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto* const ret = co_dev_first_obj(dev);

  POINTERS_EQUAL(obj, ret);
}

TEST(CO_Dev, CoDevFirstObj_Empty) {
  const auto* const ret = co_dev_first_obj(dev);

  POINTERS_EQUAL(nullptr, ret);
}

TEST(CO_Dev, CoDevLastObj) {
  CoObjTHolder obj_holder(0x1234);
  const auto obj = obj_holder.Take();
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto* const ret = co_dev_last_obj(dev);

  POINTERS_EQUAL(obj, ret);
}

TEST(CO_Dev, CoDevLastObj_Empty) {
  const auto* const ret = co_dev_last_obj(dev);

  POINTERS_EQUAL(nullptr, ret);
}

#if !LELY_NO_CO_OBJ_NAME

TEST(CO_Dev, CoDevSetName) {
  const char* name = "DeviceName";
  const auto ret = co_dev_set_name(dev, name);

  CHECK_EQUAL(0, ret);
  STRCMP_EQUAL(name, co_dev_get_name(dev));
}

TEST(CO_Dev, CoDevSetName_Null) {
  const char* name = "DeviceName";
  CHECK_EQUAL(0, co_dev_set_name(dev, name));

  const auto ret = co_dev_set_name(dev, nullptr);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_dev_get_name(dev));
}

TEST(CO_Dev, CoDevSetName_Empty) {
  const char* name = "DeviceName";
  CHECK_EQUAL(0, co_dev_set_name(dev, name));

  const auto ret = co_dev_set_name(dev, "");

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_dev_get_name(dev));
}

TEST(CO_Dev, CoDevSetVendorName) {
  const char* vendor_name = "VendorName";
  const auto ret = co_dev_set_vendor_name(dev, vendor_name);

  CHECK_EQUAL(0, ret);
  STRCMP_EQUAL(vendor_name, co_dev_get_vendor_name(dev));
}

TEST(CO_Dev, CoDevSetVendorName_Null) {
  const char* vendor_name = "VendorName";
  CHECK_EQUAL(0, co_dev_set_vendor_name(dev, vendor_name));

  const auto ret = co_dev_set_vendor_name(dev, nullptr);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_dev_get_vendor_name(dev));
}

TEST(CO_Dev, CoDevSetVendorName_Empty) {
  const char* vendor_name = "VendorName";
  CHECK_EQUAL(0, co_dev_set_vendor_name(dev, vendor_name));

  const auto ret = co_dev_set_vendor_name(dev, "");

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_dev_get_vendor_name(dev));
}

TEST(CO_Dev, CoDevSetProductName) {
  const char* product_name = "ProductName";
  const auto ret = co_dev_set_product_name(dev, product_name);

  CHECK_EQUAL(0, ret);
  STRCMP_EQUAL(product_name, co_dev_get_product_name(dev));
}

TEST(CO_Dev, CoDevSetProductName_Null) {
  const char* product_name = "ProductName";
  CHECK_EQUAL(0, co_dev_set_product_name(dev, product_name));

  const auto ret = co_dev_set_product_name(dev, nullptr);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_dev_get_product_name(dev));
}

TEST(CO_Dev, CoDevSetProductName_Empty) {
  const char* product_name = "ProductName";
  CHECK_EQUAL(0, co_dev_set_product_name(dev, product_name));

  const auto ret = co_dev_set_product_name(dev, "");

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_dev_get_product_name(dev));
}

TEST(CO_Dev, CoDevSetOrderCode) {
  const char* order_code = "OrderCode";
  const auto ret = co_dev_set_order_code(dev, order_code);

  CHECK_EQUAL(0, ret);
  STRCMP_EQUAL(order_code, co_dev_get_order_code(dev));
}

TEST(CO_Dev, CoDevSetOrderCode_Null) {
  const char* order_code = "OrderCode";
  CHECK_EQUAL(0, co_dev_set_order_code(dev, order_code));

  const auto ret = co_dev_set_order_code(dev, nullptr);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_dev_get_order_code(dev));
}

TEST(CO_Dev, CoDevSetOrderCode_Empty) {
  const char* order_code = "OrderCode";
  CHECK_EQUAL(0, co_dev_set_order_code(dev, order_code));

  const auto ret = co_dev_set_order_code(dev, "");

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, co_dev_get_order_code(dev));
}

#endif  // !LELY_NO_CO_OBJ_NAME

TEST(CO_Dev, CoDevSetVendorId) {
  co_dev_set_vendor_id(dev, 0x12345678);

  CHECK_EQUAL(0x12345678, co_dev_get_vendor_id(dev));
}

TEST(CO_Dev, CoDevSetProductCode) {
  co_dev_set_product_code(dev, 0x12345678);

  CHECK_EQUAL(0x12345678, co_dev_get_product_code(dev));
}

TEST(CO_Dev, CoDevSetRevision) {
  co_dev_set_revision(dev, 0x12345678);

  CHECK_EQUAL(0x12345678, co_dev_get_revision(dev));
}

TEST(CO_Dev, CoDevSetBaud) {
  co_dev_set_baud(dev, CO_BAUD_50 | CO_BAUD_1000);

  CHECK_EQUAL(CO_BAUD_50 | CO_BAUD_1000, co_dev_get_baud(dev));
}

TEST(CO_Dev, CoDevSetRate) {
  co_dev_set_rate(dev, 500);

  CHECK_EQUAL(500, co_dev_get_rate(dev));
}

TEST(CO_Dev, CoDevSetLSS) {
  co_dev_set_lss(dev, 123);

  CHECK_EQUAL(true, co_dev_get_lss(dev));
}

TEST(CO_Dev, CoDevSetDommy) {
  co_dev_set_dummy(dev, 0x00010001);

  CHECK_EQUAL(0x00010001, co_dev_get_dummy(dev));
}

TEST(CO_Dev, CoDevGetVal) {
  CoObjTHolder obj_holder(0x1234);
  CoSubTHolder sub_holder(0xab, CO_DEFTYPE_INTEGER16);
  co_sub_t* const sub = obj_holder.InsertSub(sub_holder);
  CHECK(sub != nullptr);
  CHECK_EQUAL(co_type_sizeof(CO_DEFTYPE_INTEGER16),
              co_sub_set_val_i16(sub, 0x0987));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder.Take()));

  const auto* ret =
      static_cast<const co_integer16_t*>(co_dev_get_val(dev, 0x1234, 0xab));

  CHECK(ret != nullptr);
  CHECK_EQUAL(0x0987, *ret);
}

TEST(CO_Dev, CoDevGetVal_NullDev) {
  const auto ret = co_dev_get_val(nullptr, 0x0000, 0x00);

  POINTERS_EQUAL(nullptr, ret);
}

TEST(CO_Dev, CoDevGetVal_NotFound) {
  const auto ret = co_dev_get_val(dev, 0x0000, 0x00);

  POINTERS_EQUAL(nullptr, ret);
}

TEST(CO_Dev, CoDevSetVal) {
  co_unsigned16_t val = 0x0987;

  CoObjTHolder obj_holder(0x1234);
  CoSubTHolder sub_holder(0xab, CO_DEFTYPE_INTEGER16);
  CHECK(obj_holder.InsertSub(sub_holder) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder.Take()));

  const auto ret = co_dev_set_val(dev, 0x1234, 0xab, &val, 2);

  CHECK_EQUAL(2, ret);
  CHECK_EQUAL(val, co_dev_get_val_i16(dev, 0x1234, 0xab));
}

TEST(CO_Dev, CoDevSetVal_NotFound) {
  const auto ret = co_dev_set_val(dev, 0x0000, 0x00, nullptr, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Dev, CoDevSetGetVal_CoType_##a) { \
    CoObjTHolder obj(0x1234); \
    CoSubTHolder sub(0xab, CO_DEFTYPE_##a); \
    CHECK(obj.InsertSub(sub) != nullptr); \
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take())); \
    const auto set_ret = co_dev_set_val_##c(dev, 0x1234, 0xab, 0x42); \
    CHECK_EQUAL(co_type_sizeof(CO_DEFTYPE_##a), set_ret); \
    const co_##b##_t get_ret = co_dev_get_val_##c(dev, 0x1234, 0xab); \
    CHECK_EQUAL(get_ret, static_cast<co_##b##_t>(0x42)); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE

TEST(CO_Dev, CoDevReadSub) {
  CoObjTHolder obj(0x1234);
  CoSubTHolder sub(0xab, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x02, 0x00,
                                 0x00, 0x00, 0x87, 0x09};
  co_unsigned16_t idx = 0x0000;
  co_unsigned8_t subidx = 0x00;

  const auto ret = co_dev_read_sub(dev, &idx, &subidx, buf, buf + BUF_SIZE);

  CHECK_EQUAL(BUF_SIZE, ret);
  CHECK_EQUAL(0x1234, idx);
  CHECK_EQUAL(0xab, subidx);
  CHECK_EQUAL(0x0987, co_dev_get_val_i16(dev, idx, subidx));
}

TEST(CO_Dev, CoDevReadSub_NoIdx) {
  CoObjTHolder obj(0x1234);
  CoSubTHolder sub(0xab, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x02, 0x00,
                                 0x00, 0x00, 0x87, 0x09};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(BUF_SIZE, ret);
  CHECK_EQUAL(0x0987, co_dev_get_val_i16(dev, 0x1234, 0xab));
}

TEST(CO_Dev, CoDevReadSub_NoSub) {
  CoObjTHolder obj(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x02, 0x00,
                                 0x00, 0x00, 0x87, 0x09};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(BUF_SIZE, ret);
}

TEST(CO_Dev, CoDevReadSub_NoBegin) {
  CoObjTHolder obj(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x02, 0x00,
                                 0x00, 0x00, 0x87, 0x09};

  const auto ret =
      co_dev_read_sub(dev, nullptr, nullptr, nullptr, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Dev, CoDevReadSub_NoEnd) {
  CoObjTHolder obj(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x02, 0x00,
                                 0x00, 0x00, 0x87, 0x09};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, nullptr);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Dev, CoDevReadSub_TooSmallBuffer) {
  CoObjTHolder obj(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 6;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x01, 0x00, 0x00};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Dev, CoDevReadSub_TooSmallForType) {
  CoObjTHolder obj(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 8;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x02,
                                 0x00, 0x00, 0x00, 0x87};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

#ifdef HAVE_LELY_OVERRIDE
TEST(CO_Dev, CoDevReadSub_ReadIdxFailed) {
  CoObjTHolder obj(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x02, 0x00,
                                 0x00, 0x00, 0x87, 0x09};
  LelyOverride::co_val_read(Override::NoneCallsValid);

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Dev, CoDevReadSub_ReadSubidxFailed) {
  CoObjTHolder obj(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x02, 0x00,
                                 0x00, 0x00, 0x87, 0x09};
  LelyOverride::co_val_read(1);

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Dev, CoDevReadSub_ReadSizeFailed) {
  CoObjTHolder obj(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x02, 0x00,
                                 0x00, 0x00, 0x87, 0x09};
  LelyOverride::co_val_read(2);

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

#if LELY_NO_MALLOC
TEST(CO_Dev, CoDevReadSub_ArrayType) {
  CoObjTHolder obj(0x1234);
  CoSubTHolder sub(0xab, CO_DEFTYPE_OCTET_STRING);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  uint_least8_t buf[] = {0x34, 0x12, 0xab, 0x04, 0x00, 0x00,
                         0x00, 'a',  'b',  'c',  'd'};
  co_unsigned16_t idx = 0x0000;
  co_unsigned8_t subidx = 0x00;

  const auto ret = co_dev_read_sub(dev, &idx, &subidx, buf, buf + sizeof(buf));

  CHECK_EQUAL(sizeof(buf), ret);
  CHECK_EQUAL(0x1234, idx);
  CHECK_EQUAL(0xab, subidx);
  STRCMP_EQUAL("abcd", *reinterpret_cast<char* const*>(
                           co_dev_get_val(dev, idx, subidx)));
}
#endif  // LELY_NO_MALLOC

#endif  // HAVE_LELY_OVERRIDE

TEST(CO_Dev, CoDevReadSub_ValSizeTooBig) {
  CoObjTHolder obj(0x1234);
  CoSubTHolder sub(0xab, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234, 0xab, 0x1a1a));

  const size_t BUF_SIZE = 10;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x03, 0x00,
                                 0x00, 0x00, 0x87, 0x09, 0x00};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(BUF_SIZE, ret);
  CHECK_EQUAL(0x1a1a, co_dev_get_val_i16(dev, 0x1234, 0xab));
}

TEST(CO_Dev, CoDevWriteSub) {
  CoObjTHolder obj(0x1234);
  CoSubTHolder sub(0xab, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234, 0xab, 0x0987));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0};

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, buf + BUF_SIZE);

  CHECK_EQUAL(BUF_SIZE, ret);
  uint_least8_t test_buf[] = {0x34, 0x12, 0xab, 0x02, 0x00,
                              0x00, 0x00, 0x87, 0x09};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

TEST(CO_Dev, CoDevWriteSub_NoSub) {
  CoObjTHolder obj(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0};

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

#ifdef HAVE_LELY_OVERRIDE
TEST(CO_Dev, CoDevWriteSub_InitWriteFailed) {
  CoObjTHolder obj(0x1234);
  CoSubTHolder sub(0xab, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0};
  LelyOverride::co_val_write(Override::NoneCallsValid);

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}
#endif

TEST(CO_Dev, CoDevWriteSub_EmptyDomain) {
  CoObjTHolder obj(0x1234);
  CoSubTHolder sub(0xab, CO_DEFTYPE_DOMAIN);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 7;
  uint_least8_t buf[BUF_SIZE] = {0};

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, buf + BUF_SIZE);

  CHECK_EQUAL(7, ret);
  uint_least8_t test_buf[] = {0x34, 0x12, 0xab, 0x00, 0x00, 0x00, 0x00};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

TEST(CO_Dev, CoDevWriteSub_NoBegin) {
  CoObjTHolder obj(0x1234);
  CoSubTHolder sub(0xab, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, nullptr, nullptr);

  CHECK_EQUAL(9, ret);
}

TEST(CO_Dev, CoDevWriteSub_NoEnd) {
  CoObjTHolder obj(0x1234);
  CoSubTHolder sub(0xab, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234, 0xab, 0x0987));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0};

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, nullptr);

  CHECK_EQUAL(BUF_SIZE, ret);
  uint_least8_t test_buf[] = {0x34, 0x12, 0xab, 0x02, 0x00,
                              0x00, 0x00, 0x87, 0x09};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

TEST(CO_Dev, CoDevWriteSub_TooSmallBuffer) {
  CoObjTHolder obj(0x1234);
  CoSubTHolder sub(0xab, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));

  const size_t BUF_SIZE = 8;
  uint_least8_t buf[BUF_SIZE] = {0};

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, buf + BUF_SIZE);

  CHECK_EQUAL(9, ret);
  uint_least8_t test_buf[BUF_SIZE] = {0};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

#ifdef HAVE_LELY_OVERRIDE
TEST(CO_Dev, CoDevWriteSub_IdxWriteFailed) {
  CoObjTHolder obj(0x1234);
  CoSubTHolder sub(0xab, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234, 0xab, 0x0987));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0};
  LelyOverride::co_val_write(1);

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
  uint_least8_t test_buf[BUF_SIZE] = {0};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

TEST(CO_Dev, CoDevWriteSub_SubidxWriteFailed) {
  CoObjTHolder obj(0x1234);
  CoSubTHolder sub(0xab, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234, 0xab, 0x0987));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0};
  LelyOverride::co_val_write(2);

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
  uint_least8_t test_buf[] = {0x34, 0x12, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

TEST(CO_Dev, CoDevWriteSub_SizeWriteFailed) {
  CoObjTHolder obj(0x1234);
  CoSubTHolder sub(0xab, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234, 0xab, 0x0987));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0};
  LelyOverride::co_val_write(3);

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
  uint_least8_t test_buf[] = {0x34, 0x12, 0xab, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

TEST(CO_Dev, CoDevWriteSub_ValWriteFailed) {
  CoObjTHolder obj(0x1234);
  CoSubTHolder sub(0xab, CO_DEFTYPE_INTEGER16);
  CHECK(obj.InsertSub(sub) != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj.Take()));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234, 0xab, 0x0987));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0};
  LelyOverride::co_val_write(4);

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
  uint_least8_t test_buf[] = {0x34, 0x12, 0xab, 0x02, 0x00,
                              0x00, 0x00, 0x00, 0x00};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}
#endif

TEST_GROUP(CO_DevDCF) {
  co_dev_t* dev = nullptr;
  std::unique_ptr<CoDevTHolder> dev_holder;
  std::unique_ptr<CoObjTHolder> obj_holder;
  std::unique_ptr<CoSubTHolder> sub_holder;
  CoArrays arrays;

  static const size_t BUF_SIZE = 13u;
  const uint_least8_t buf[BUF_SIZE] = {
      0x01, 0x00, 0x00, 0x00,  // number of sub-indexes
      // value 1
      0x34, 0x12,              // index
      0xab,                    // subindex
      0x02, 0x00, 0x00, 0x00,  // size
      0x87, 0x09               // value
  };
  static const size_t MIN_RW_SIZE = 4u;

  TEST_SETUP() {
    dev_holder.reset(new CoDevTHolder(0x01));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);

    obj_holder.reset(new CoObjTHolder(0x1234));
    sub_holder.reset(new CoSubTHolder(0xab, CO_DEFTYPE_INTEGER16));

    obj_holder->InsertSub(*sub_holder);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj_holder->Take()));
  }

  TEST_TEARDOWN() {
    dev_holder.reset();
    obj_holder.reset();
    sub_holder.reset();
    arrays.Clear();
  }
};

TEST(CO_DevDCF, CoDevReadDcf) {
  co_unsigned16_t pmin = 0x0000;
  co_unsigned16_t pmax = 0x0000;

  const auto ret = co_dev_read_dcf(dev, &pmin, &pmax, buf, buf + BUF_SIZE);

  CHECK_EQUAL(BUF_SIZE, ret);
  CHECK_EQUAL(0x0987, co_dev_get_val_i16(dev, 0x1234, 0xab));
  CHECK_EQUAL(0x1234, pmin);
  CHECK_EQUAL(0x1234, pmax);
}

TEST(CO_DevDCF, CoDevReadDcf_NullMinMax) {
  const auto ret = co_dev_read_dcf(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(BUF_SIZE, ret);
  CHECK_EQUAL(0x0987, co_dev_get_val_i16(dev, 0x1234, 0xab));
}

TEST(CO_DevDCF, CoDevReadDcf_InvalidNumberOfSubIndexes) {
  const uint_least8_t empty[BUF_SIZE] = {0};

  const auto ret =
      co_dev_read_dcf(dev, nullptr, nullptr, empty, empty + BUF_SIZE);

  CHECK_EQUAL(MIN_RW_SIZE, ret);
  CHECK_EQUAL(0x0000, co_dev_get_val_i16(dev, 0x1234, 0xab));
}

TEST(CO_DevDCF, CoDevReadDcf_InvaildSubIdx) {
  const auto ret = co_dev_read_dcf(dev, nullptr, nullptr, buf, buf + 7);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x0000, co_dev_get_val_i16(dev, 0x1234, 0xab));
}

#if HAVE_LELY_OVERRIDE
TEST(CO_DevDCF, CoDevReadDcf_FailedToReadNumberOfSubIndexes) {
  LelyOverride::co_val_read(Override::NoneCallsValid);

  const auto ret = co_dev_read_dcf(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}
#endif

TEST(CO_DevDCF, CoDevWriteDcf) {
  co_dev_set_val_i16(dev, 0x1234, 0xab, 0x0987);
  uint_least8_t tmp[BUF_SIZE] = {0};

  const auto ret = co_dev_write_dcf(dev, CO_UNSIGNED16_MIN, CO_UNSIGNED16_MAX,
                                    tmp, tmp + BUF_SIZE);

  CHECK_EQUAL(BUF_SIZE, ret);
  CheckBuffers(tmp, buf, BUF_SIZE);
}

TEST(CO_DevDCF, CoDevWriteDcf_BeforeMin) {
  uint_least8_t tmp[BUF_SIZE] = {0};

  const auto ret =
      co_dev_write_dcf(dev, 0x1235, CO_UNSIGNED16_MAX, tmp, tmp + BUF_SIZE);

  CHECK_EQUAL(MIN_RW_SIZE, ret);
}

TEST(CO_DevDCF, CoDevWriteDcf_AfterMax) {
  uint_least8_t tmp[BUF_SIZE] = {0};

  const auto ret =
      co_dev_write_dcf(dev, CO_UNSIGNED16_MIN, 0x1233, tmp, tmp + BUF_SIZE);

  CHECK_EQUAL(MIN_RW_SIZE, ret);
}

#if LELY_NO_MALLOC
TEST(CO_DevDCF, CoDevWriteDcf_Null) {
  co_dev_set_val_i16(dev, 0x1234, 0xab, 0x0987);

  const auto ret = co_dev_write_dcf(dev, CO_UNSIGNED16_MIN, CO_UNSIGNED16_MAX,
                                    nullptr, nullptr);

  CHECK_EQUAL(MIN_RW_SIZE + sizeof(co_unsigned16_t) + 2 + 1 + 4,
              ret);  // number of bytes that _would have_ been written
}
#endif

#if HAVE_LELY_OVERRIDE
TEST(CO_DevDCF, CoDevWriteDcf_FailedToWriteSubObject) {
  co_dev_set_val_i16(dev, 0x1234, 0xab, 0x0987);
  uint_least8_t buf[BUF_SIZE] = {0};

  LelyOverride::co_val_write(Override::NoneCallsValid);
  const auto ret = co_dev_write_dcf(dev, CO_UNSIGNED16_MIN, CO_UNSIGNED16_MAX,
                                    buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}
#endif

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
  }

  TEST_TEARDOWN() { dev_holder.reset(); }
};

TEST_GROUP_BASE(CO_DevTpdoEventInd, CO_DevTpdoBase){};

TEST(CO_DevTpdoEventInd, CoDevGetTpdoEventInd_Null) {
  co_dev_get_tpdo_event_ind(dev, nullptr, nullptr);
}

TEST(CO_DevTpdoEventInd, CoDevSetTpdoEventInd) {
  int data = 42;
  co_dev_set_tpdo_event_ind(dev, tpdo_event_ind, &data);

  co_dev_tpdo_event_ind_t* ind_ptr = nullptr;
  void* data_ptr = nullptr;
  co_dev_get_tpdo_event_ind(dev, &ind_ptr, &data_ptr);
  FUNCTIONPOINTERS_EQUAL(tpdo_event_ind, ind_ptr);
  POINTERS_EQUAL(&data, data_ptr);
}

TEST_GROUP_BASE(CO_DevTpdoEvent, CO_DevTpdoBase) {
  const co_unsigned16_t OBJ_IDX = 0x1234u;
  const co_unsigned16_t SUB_IDX = 0xabu;
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
                             co_unsigned8_t(0x02u));
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
                             co_unsigned8_t(0x01u));
    obj1a00->InsertAndSetSub(0x01u, CO_DEFTYPE_UNSIGNED32, mapping);
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1a00->Take()));

    tpdo_mappings.push_back(std::move(obj1a00));
  }

  co_unsigned32_t EncodeMapping(co_unsigned16_t obj_idx, co_unsigned8_t sub_idx,
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
  }

  TEST_TEARDOWN() {
    tpdo_objects.clear();
    tpdo_mappings.clear();
    sub_holder.reset();
    obj_holder.reset();

    TEST_BASE_TEARDOWN();
  }
};

TEST(CO_DevTpdoEvent, CoDevTpdoEvent_InvalidIndices) {
  co_dev_tpdo_event(dev, 0x0000, 0x00);
}

TEST(CO_DevTpdoEvent, CoDevTpdoEvent_OnlySubNoMapping) {
  co_sub_set_pdo_mapping(sub, 0);

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);
  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

TEST(CO_DevTpdoEvent, CoDevTpdoEvent_MappingPossibleButNoMapping) {
  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);
  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

TEST(CO_DevTpdoEvent, CoDevTpdoEvent_InvalidTpdoMaxSubIndex) {
  CoObjTHolder obj1800(0x1800u);
  obj1800.InsertAndSetSub(0x00u, CO_DEFTYPE_UNSIGNED8, co_unsigned8_t(0x00u));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1800.Take()));
  CreateSingleEntryMapping(EncodeMapping(OBJ_IDX, SUB_IDX, SUB_SIZE));

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

TEST(CO_DevTpdoEvent, CoDevTpdoEvent_InvalidTpdoCobId) {
  CreateTpdoCommObject(DEV_ID | CO_PDO_COBID_VALID, 0x00u);
  CreateSingleEntryMapping(EncodeMapping(OBJ_IDX, SUB_IDX, SUB_SIZE));

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

TEST(CO_DevTpdoEvent, CoDevTpdoEvent_ReservedTransmissionType) {
  CreateTpdoCommObject(DEV_ID, 0xf1u);
  CreateSingleEntryMapping(EncodeMapping(OBJ_IDX, SUB_IDX, SUB_SIZE));

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

TEST(CO_DevTpdoEvent, CoDevTpdoEvent_NoTpdoMapping) {
  CreateAcyclicTpdoCommObject();

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

TEST(CO_DevTpdoEvent, CoDevTpdoEvent_DifferentObjectIndexInMapping) {
  CreateAcyclicTpdoCommObject();
  CreateSingleEntryMapping(EncodeMapping(OBJ_IDX - 0x100, SUB_IDX, SUB_SIZE));

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

TEST(CO_DevTpdoEvent, CoDevTpdoEvent_DifferentSubIndexInMapping) {
  CreateAcyclicTpdoCommObject();
  CreateSingleEntryMapping(EncodeMapping(OBJ_IDX, SUB_IDX + 10, SUB_SIZE));

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

TEST(CO_DevTpdoEvent, CoDevTpdoEvent_NoIndicationFunction) {
  CreateAcyclicTpdoCommObject();
  CreateSingleEntryMapping(EncodeMapping(OBJ_IDX, SUB_IDX, SUB_SIZE));
  co_dev_set_tpdo_event_ind(dev, nullptr, nullptr);

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

TEST(CO_DevTpdoEvent, CoDevTpdoEvent_ValidAcyclicTpdo) {
  CreateAcyclicTpdoCommObject();
  CreateSingleEntryMapping(EncodeMapping(OBJ_IDX, SUB_IDX, SUB_SIZE));

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(1, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

TEST(CO_DevTpdoEvent, CoDevTpdoEvent_ValidEventDrivenTpdo) {
  CreateTpdoCommObject(DEV_ID, 0xfeu);
  CreateSingleEntryMapping(EncodeMapping(OBJ_IDX, SUB_IDX, SUB_SIZE));

  co_dev_tpdo_event(dev, OBJ_IDX, SUB_IDX);

  CHECK_EQUAL(1, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

TEST(CO_DevTpdoEvent, CoDevTpdEvent_CallsIndicationFunction_ForMatchedTpdos) {
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

#endif  // !LELY_NO_CO_TPDO
