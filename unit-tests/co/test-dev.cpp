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

#include <cstring>
#include <string>

#include <CppUTest/TestHarness.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lely/can/net.h>
#include <lely/co/dev.h>
#include <lely/co/obj.h>
#include <lely/co/val.h>
#include <lely/co/tpdo.h>
#include <lely/util/errnum.h>

#include "override/lelyco-val.hpp"

static void
CheckBuffers(const uint_least8_t* buf1, const uint_least8_t* buf2,
             const size_t n) {
  for (size_t i = 0; i < n; ++i) {
    const std::string check_text = "buf[" + std::to_string(i) + "]";
    CHECK_EQUAL_TEXT(buf2[i], buf1[i], check_text.c_str());
  }
}

TEST_GROUP(CO_DevInit){};

TEST(CO_DevInit, CODevAllocFree) {
  void* const ptr = __co_dev_alloc();

  CHECK(ptr != nullptr);

  __co_dev_free(ptr);
}

TEST(CO_DevInit, CODevInit) {
  auto* const dev = static_cast<co_dev_t*>(__co_dev_alloc());

  CHECK(dev != nullptr);
  POINTERS_EQUAL(dev, __co_dev_init(dev, 0x01));

  CHECK_EQUAL(0, co_dev_get_netid(dev));
  CHECK_EQUAL(0x01, co_dev_get_id(dev));

  CHECK_EQUAL(0, co_dev_get_idx(dev, 0, nullptr));

  POINTERS_EQUAL(nullptr, co_dev_get_name(dev));

  POINTERS_EQUAL(nullptr, co_dev_get_vendor_name(dev));
  CHECK_EQUAL(0, co_dev_get_vendor_id(dev));
  POINTERS_EQUAL(nullptr, co_dev_get_product_name(dev));
  CHECK_EQUAL(0, co_dev_get_product_code(dev));
  CHECK_EQUAL(0, co_dev_get_revision(dev));
  POINTERS_EQUAL(nullptr, co_dev_get_order_code(dev));

  CHECK_EQUAL(0, co_dev_get_baud(dev));
  CHECK_EQUAL(0, co_dev_get_rate(dev));

  CHECK_EQUAL(0, co_dev_get_lss(dev));

  CHECK_EQUAL(0, co_dev_get_dummy(dev));

  __co_dev_fini(dev);
  __co_dev_free(dev);
}

TEST(CO_DevInit, CODevInit_UnconfiguredId) {
  auto* const dev = static_cast<co_dev_t*>(__co_dev_alloc());

  CHECK(dev != nullptr);
  POINTERS_EQUAL(dev, __co_dev_init(dev, 0xff));

  co_obj_t* const obj1 = co_obj_create(0x0000);
  co_obj_t* const obj2 = co_obj_create(0x0001);
  co_obj_t* const obj3 = co_obj_create(0xffff);
  CHECK(obj1 != nullptr);
  CHECK(obj2 != nullptr);
  CHECK(obj3 != nullptr);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj3));

  CHECK_EQUAL(0, co_dev_set_name(dev, "name"));
  CHECK_EQUAL(0, co_dev_set_vendor_name(dev, "vendor"));
  CHECK_EQUAL(0, co_dev_set_product_name(dev, "product name"));
  CHECK_EQUAL(0, co_dev_set_order_code(dev, "order code"));

  __co_dev_fini(dev);
  __co_dev_free(dev);
}

TEST(CO_DevInit, CODevInit_ZeroId) {
  auto* const dev = static_cast<co_dev_t*>(__co_dev_alloc());

  CHECK(dev != nullptr);
  POINTERS_EQUAL(nullptr, __co_dev_init(dev, 0x00));

  __co_dev_free(dev);
}

TEST(CO_DevInit, CODevInit_InvalidId) {
  auto* const dev = static_cast<co_dev_t*>(__co_dev_alloc());
  CHECK(dev != nullptr);

  POINTERS_EQUAL(nullptr, __co_dev_init(dev, CO_NUM_NODES + 1));
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());

  POINTERS_EQUAL(nullptr, __co_dev_init(dev, 0xff - 1));
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());

  __co_dev_free(dev);
}

TEST(CO_DevInit, CODevFini) {
  auto* const dev = static_cast<co_dev_t*>(__co_dev_alloc());

  CHECK(dev != nullptr);
  POINTERS_EQUAL(dev, __co_dev_init(dev, 0x01));

  __co_dev_fini(dev);
  __co_dev_free(dev);
}

TEST(CO_DevInit, CODevDestroy_Null) { co_dev_destroy(nullptr); }

TEST_GROUP(CO_Dev) {
  co_dev_t* dev = nullptr;

  TEST_SETUP() {
#ifdef HAVE_LELY_OVERRIDE
    LelyOverride::co_val_read_vc = LelyOverride::AllCallsValid;
    LelyOverride::co_val_write_vc = LelyOverride::AllCallsValid;
#endif
    dev = co_dev_create(0x01);
    CHECK(dev != nullptr);
  }

  TEST_TEARDOWN() {
#ifdef HAVE_LELY_OVERRIDE
    LelyOverride::co_val_read_vc = LelyOverride::AllCallsValid;
    LelyOverride::co_val_write_vc = LelyOverride::AllCallsValid;
#endif
    co_dev_destroy(dev);
  }
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
  co_obj_t* const obj = co_obj_create(0x0000);
#ifndef LELY_NO_CO_OBJ_LIMITS
  co_obj_t* const obj1 = co_obj_create(0x0001);
  co_obj_t* const obj2 = co_obj_create(0x1234);
#endif
#ifndef LELY_NO_CO_OBJ_DEFAULT
  co_obj_t* const obj3 = co_obj_create(0xffff);
#endif
#ifndef LELY_NO_CO_OBJ_LIMITS
  co_sub_t* const sub_min1 = co_sub_create(0x00, CO_DEFTYPE_INTEGER16);
  co_sub_t* const sub_min2 = co_sub_create(0x01, CO_DEFTYPE_INTEGER16);
  co_sub_t* const sub_max1 = co_sub_create(0x00, CO_DEFTYPE_INTEGER16);
  co_sub_t* const sub_max2 = co_sub_create(0x01, CO_DEFTYPE_INTEGER16);
#endif
#ifndef LELY_NO_CO_OBJ_DEFAULT
  co_sub_t* const sub_def1 = co_sub_create(0x00, CO_DEFTYPE_INTEGER16);
  co_sub_t* const sub_def2 = co_sub_create(0x01, CO_DEFTYPE_INTEGER16);
#endif

#ifndef LELY_NO_CO_OBJ_LIMITS
  const co_integer16_t min_val1 = 0x0;
  const co_integer16_t min_val2 = 0x0 + co_dev_get_id(dev);
  CHECK_EQUAL(2, co_sub_set_min(sub_min1, &min_val1, 2));
  CHECK_EQUAL(2, co_sub_set_min(sub_min2, &min_val2, 2));
  co_sub_set_flags(sub_min2, CO_OBJ_FLAGS_MIN_NODEID);

  const co_integer16_t max_val1 = 0x3f00;
  const co_integer16_t max_val2 = 0x3f00 + co_dev_get_id(dev);
  CHECK_EQUAL(2, co_sub_set_max(sub_max1, &max_val1, 2));
  CHECK_EQUAL(2, co_sub_set_max(sub_max2, &max_val2, 2));
  co_sub_set_flags(sub_max2, CO_OBJ_FLAGS_MAX_NODEID);
#endif
#ifndef LELY_NO_CO_OBJ_DEFAULT
  const co_integer16_t def_val1 = 0x1234;
  const co_integer16_t def_val2 = 0x1234 + co_dev_get_id(dev);
  CHECK_EQUAL(2, co_sub_set_def(sub_def1, &def_val1, 2));
  CHECK_EQUAL(2, co_sub_set_def(sub_def2, &def_val2, 2));
  co_sub_set_flags(sub_def2, CO_OBJ_FLAGS_DEF_NODEID);
#endif

#ifndef LELY_NO_CO_OBJ_LIMITS
  CHECK_EQUAL(0, co_obj_insert_sub(obj1, sub_min1));
  CHECK_EQUAL(0, co_obj_insert_sub(obj1, sub_min2));
  CHECK_EQUAL(0, co_obj_insert_sub(obj2, sub_max1));
  CHECK_EQUAL(0, co_obj_insert_sub(obj2, sub_max2));
#endif
#ifndef LELY_NO_CO_OBJ_DEFAULT
  CHECK_EQUAL(0, co_obj_insert_sub(obj3, sub_def1));
  CHECK_EQUAL(0, co_obj_insert_sub(obj3, sub_def2));
#endif

  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));
#ifndef LELY_NO_CO_OBJ_LIMITS
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2));
#endif
#ifndef LELY_NO_CO_OBJ_DEFAULT
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj3));
#endif

  const co_unsigned8_t new_id = 0x3d;

  const auto ret = co_dev_set_id(dev, new_id);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(new_id, co_dev_get_id(dev));

#if !defined(LELY_NO_CO_OBJ_LIMITS) || !defined(LELY_NO_CO_OBJ_DEFAULT)
  const co_obj_t* out_obj = co_dev_first_obj(dev);
#endif

#ifndef LELY_NO_CO_OBJ_LIMITS
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
#ifndef LELY_NO_CO_OBJ_DEFAULT
  out_obj = co_obj_next(out_obj);
  CHECK_EQUAL(0x1234, *static_cast<const co_integer16_t*>(
                          co_sub_get_def(co_obj_first_sub(out_obj))));
  CHECK_EQUAL(0x1234 + new_id, *static_cast<const co_integer16_t*>(
                                   co_sub_get_def(co_obj_last_sub(out_obj))));
#endif
}

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Dev, CoDevSetId_CoType_##a) { \
    co_obj_t* const obj = co_obj_create(0x1234); \
    co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_##a); \
    CHECK_EQUAL(0, co_obj_insert_sub(obj, sub)); \
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
  co_obj_t* const obj = co_obj_create(0x0000);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto ret = co_dev_get_idx(dev, 0xffff, nullptr);

  CHECK_EQUAL(1, ret);
}

TEST(CO_Dev, CoDevGetIdx_OneObjCheckIdx) {
  co_obj_t* const obj = co_obj_create(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  co_unsigned16_t out_idx = 0x0000;
  const auto ret = co_dev_get_idx(dev, 1, &out_idx);

  CHECK_EQUAL(1, ret);
  CHECK_EQUAL(0x1234, out_idx);
}

TEST(CO_Dev, CoDevGetIdx_ManyObj1) {
  co_obj_t* const obj1 = co_obj_create(0x0000);
  co_obj_t* const obj2 = co_obj_create(0x1234);
  co_obj_t* const obj3 = co_obj_create(0xffff);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj3));

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
  co_obj_t* const obj1 = co_obj_create(0x0000);
  co_obj_t* const obj2 = co_obj_create(0x1234);
  co_obj_t* const obj3 = co_obj_create(0xffff);
  co_obj_t* const obj4 = co_obj_create(0xabcd);
  co_obj_t* const obj5 = co_obj_create(0x1010);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj2));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj3));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj4));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj5));

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
  co_obj_t* const obj = co_obj_create(0x1234);

  const auto ret = co_dev_insert_obj(dev, obj);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(obj, co_dev_first_obj(dev));
  co_unsigned16_t out_idx = 0x0000;
  CHECK_EQUAL(1, co_dev_get_idx(dev, 1, &out_idx));
  CHECK_EQUAL(0x1234, out_idx);
  CHECK_EQUAL(dev, co_obj_get_dev(obj));
}

TEST(CO_Dev, CoDevInsertObj_AddedToOtherDev) {
  co_dev_t* const other_dev = co_dev_create(0x02);
  co_obj_t* const obj = co_obj_create(0x0001);
  CHECK_EQUAL(0, co_dev_insert_obj(other_dev, obj));

  const auto ret = co_dev_insert_obj(dev, obj);

  CHECK_EQUAL(-1, ret);

  co_dev_destroy(other_dev);
}

TEST(CO_Dev, CoDevInsertObj_AlreadyAdded) {
  co_obj_t* const obj = co_obj_create(0x0001);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto ret = co_dev_insert_obj(dev, obj);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Dev, CoDevInsertObj_AlreadyAddedAtIdx) {
  co_obj_t* const obj1 = co_obj_create(0x0001);
  co_obj_t* const obj2 = co_obj_create(0x0001);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj1));

  const auto ret = co_dev_insert_obj(dev, obj2);

  CHECK_EQUAL(-1, ret);
  co_obj_destroy(obj2);
}

TEST(CO_Dev, CoDevRemoveObj) {
  co_obj_t* const obj = co_obj_create(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto ret = co_dev_remove_obj(dev, obj);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_dev_get_idx(dev, 0, nullptr));
  POINTERS_EQUAL(nullptr, co_obj_get_dev(obj));

  co_obj_destroy(obj);
}

TEST(CO_Dev, CoDevRemoveObj_NotAdded) {
  co_obj_t* const obj = co_obj_create(0x1234);

  const auto ret = co_dev_remove_obj(dev, obj);

  CHECK_EQUAL(-1, ret);

  co_obj_destroy(obj);
}

TEST(CO_Dev, CoDevFindObj) {
  co_obj_t* const obj = co_obj_create(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto* const ret = co_dev_find_obj(dev, 0x1234);

  POINTERS_EQUAL(obj, ret);
}

TEST(CO_Dev, CoDevFindObj_NotFound) {
  const auto* const ret = co_dev_find_obj(dev, 0x1234);

  POINTERS_EQUAL(nullptr, ret);
}

TEST(CO_Dev, CoDevFindSub) {
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto* const ret = co_dev_find_sub(dev, 0x1234, 0xab);

  POINTERS_EQUAL(sub, ret);
}

TEST(CO_Dev, CoDevFindObj_NoObj) {
  const auto* const ret = co_dev_find_sub(dev, 0x1234, 0x00);

  POINTERS_EQUAL(nullptr, ret);
}

TEST(CO_Dev, CoDevFindObj_NoSub) {
  co_obj_t* const obj = co_obj_create(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto* const ret = co_dev_find_sub(dev, 0x1234, 0x00);

  POINTERS_EQUAL(nullptr, ret);
}

TEST(CO_Dev, CoDevFirstObj) {
  co_obj_t* const obj = co_obj_create(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto* const ret = co_dev_first_obj(dev);

  POINTERS_EQUAL(obj, ret);
}

TEST(CO_Dev, CoDevFirstObj_Empty) {
  const auto* const ret = co_dev_first_obj(dev);

  POINTERS_EQUAL(nullptr, ret);
}

TEST(CO_Dev, CoDevLastObj) {
  co_obj_t* const obj = co_obj_create(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto* const ret = co_dev_last_obj(dev);

  POINTERS_EQUAL(obj, ret);
}

TEST(CO_Dev, CoDevLastObj_Empty) {
  const auto* const ret = co_dev_last_obj(dev);

  POINTERS_EQUAL(nullptr, ret);
}

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

TEST(CO_Dev, CoDevSetVendorId) {
  co_dev_set_vendor_id(dev, 0x12345678);

  CHECK_EQUAL(0x12345678, co_dev_get_vendor_id(dev));
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

TEST(CO_Dev, CoDevSetProductCode) {
  co_dev_set_product_code(dev, 0x12345678);

  CHECK_EQUAL(0x12345678, co_dev_get_product_code(dev));
}

TEST(CO_Dev, CoDevSetRevision) {
  co_dev_set_revision(dev, 0x12345678);

  CHECK_EQUAL(0x12345678, co_dev_get_revision(dev));
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
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(co_type_sizeof(CO_DEFTYPE_INTEGER16),
              co_sub_set_val_i16(sub, 0x0987));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

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
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

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
    co_obj_t* const obj = co_obj_create(0x1234); \
    co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_##a); \
    CHECK_EQUAL(0, co_obj_insert_sub(obj, sub)); \
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj)); \
    const auto set_ret = co_dev_set_val_##c(dev, 0x1234, 0xab, 0x42); \
    CHECK_EQUAL(co_type_sizeof(CO_DEFTYPE_##a), set_ret); \
    const co_##b##_t get_ret = co_dev_get_val_##c(dev, 0x1234, 0xab); \
    CHECK_EQUAL(get_ret, static_cast<co_##b##_t>(0x42)); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE

TEST(CO_Dev, CoDevReadSub) {
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

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
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x02, 0x00,
                                 0x00, 0x00, 0x87, 0x09};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(BUF_SIZE, ret);
  CHECK_EQUAL(0x0987, co_dev_get_val_i16(dev, 0x1234, 0xab));
}

TEST(CO_Dev, CoDevReadSub_NoSub) {
  co_obj_t* const obj = co_obj_create(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x02, 0x00,
                                 0x00, 0x00, 0x87, 0x09};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(BUF_SIZE, ret);
}

TEST(CO_Dev, CoDevReadSub_NoBegin) {
  co_obj_t* const obj = co_obj_create(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x02, 0x00,
                                 0x00, 0x00, 0x87, 0x09};

  const auto ret =
      co_dev_read_sub(dev, nullptr, nullptr, nullptr, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Dev, CoDevReadSub_NoEnd) {
  co_obj_t* const obj = co_obj_create(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x02, 0x00,
                                 0x00, 0x00, 0x87, 0x09};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, nullptr);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Dev, CoDevReadSub_TooSmallBuffer) {
  co_obj_t* const obj = co_obj_create(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const size_t BUF_SIZE = 6;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x01, 0x00, 0x00};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Dev, CoDevReadSub_TooSmallForType) {
  co_obj_t* const obj = co_obj_create(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const size_t BUF_SIZE = 8;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x02,
                                 0x00, 0x00, 0x00, 0x87};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

#ifdef HAVE_LELY_OVERRIDE
TEST(CO_Dev, CoDevReadSub_ReadIdxFailed) {
  co_obj_t* const obj = co_obj_create(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x02, 0x00,
                                 0x00, 0x00, 0x87, 0x09};
  LelyOverride::co_val_read_vc = LelyOverride::NoneCallsValid;

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Dev, CoDevReadSub_ReadSubidxFailed) {
  co_obj_t* const obj = co_obj_create(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x02, 0x00,
                                 0x00, 0x00, 0x87, 0x09};
  LelyOverride::co_val_read_vc = 1;

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Dev, CoDevReadSub_ReadSizeFailed) {
  co_obj_t* const obj = co_obj_create(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x02, 0x00,
                                 0x00, 0x00, 0x87, 0x09};
  LelyOverride::co_val_read_vc = 2;

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}
#endif

TEST(CO_Dev, CoDevReadSub_ValSizeTooBig) {
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234, 0xab, 0x1a1a));

  const size_t BUF_SIZE = 10;
  uint_least8_t buf[BUF_SIZE] = {0x34, 0x12, 0xab, 0x03, 0x00,
                                 0x00, 0x00, 0x87, 0x09, 0x00};

  const auto ret = co_dev_read_sub(dev, nullptr, nullptr, buf, buf + BUF_SIZE);

  CHECK_EQUAL(BUF_SIZE, ret);
  CHECK_EQUAL(0x1a1a, co_dev_get_val_i16(dev, 0x1234, 0xab));
}

TEST(CO_Dev, CoDevWriteSub) {
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));
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
  co_obj_t* const obj = co_obj_create(0x1234);
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0};

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}

#ifdef HAVE_LELY_OVERRIDE
TEST(CO_Dev, CoDevWriteSub_InitWriteFailed) {
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0};
  LelyOverride::co_val_write_vc = LelyOverride::NoneCallsValid;

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
}
#endif

TEST(CO_Dev, CoDevWriteSub_EmptyDomain) {
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_DOMAIN);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const size_t BUF_SIZE = 7;
  uint_least8_t buf[BUF_SIZE] = {0};

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, buf + BUF_SIZE);

  CHECK_EQUAL(7, ret);
  uint_least8_t test_buf[] = {0x34, 0x12, 0xab, 0x00, 0x00, 0x00, 0x00};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

TEST(CO_Dev, CoDevWriteSub_NoBegin) {
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, nullptr, nullptr);

  CHECK_EQUAL(9, ret);
}

TEST(CO_Dev, CoDevWriteSub_NoEnd) {
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));
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
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));

  const size_t BUF_SIZE = 8;
  uint_least8_t buf[BUF_SIZE] = {0};

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, buf + BUF_SIZE);

  CHECK_EQUAL(9, ret);
  uint_least8_t test_buf[BUF_SIZE] = {0};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

#ifdef HAVE_LELY_OVERRIDE
TEST(CO_Dev, CoDevWriteSub_IdxWriteFailed) {
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234, 0xab, 0x0987));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0};
  LelyOverride::co_val_write_vc = 1;

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
  uint_least8_t test_buf[BUF_SIZE] = {0};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

TEST(CO_Dev, CoDevWriteSub_SubidxWriteFailed) {
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234, 0xab, 0x0987));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0};
  LelyOverride::co_val_write_vc = 2;

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
  uint_least8_t test_buf[] = {0x34, 0x12, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

TEST(CO_Dev, CoDevWriteSub_SizeWriteFailed) {
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234, 0xab, 0x0987));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0};
  LelyOverride::co_val_write_vc = 3;

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
  uint_least8_t test_buf[] = {0x34, 0x12, 0xab, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}

TEST(CO_Dev, CoDevWriteSub_ValWriteFailed) {
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));
  CHECK_EQUAL(2, co_dev_set_val_i16(dev, 0x1234, 0xab, 0x0987));

  const size_t BUF_SIZE = 9;
  uint_least8_t buf[BUF_SIZE] = {0};
  LelyOverride::co_val_write_vc = 4;

  const auto ret = co_dev_write_sub(dev, 0x1234, 0xab, buf, buf + BUF_SIZE);

  CHECK_EQUAL(0, ret);
  uint_least8_t test_buf[] = {0x34, 0x12, 0xab, 0x02, 0x00,
                              0x00, 0x00, 0x00, 0x00};
  CheckBuffers(buf, test_buf, BUF_SIZE);
}
#endif

TEST_GROUP(CO_DevDCF) {
  co_dev_t* dev = nullptr;

  static const size_t BUF_SIZE = 13;
  const uint_least8_t buf[BUF_SIZE] = {
      0x01, 0x00, 0x00, 0x00,  // number of sub-indexes
      // value 1
      0x34, 0x12,              // index
      0xab,                    // subindex
      0x02, 0x00, 0x00, 0x00,  // size
      0x87, 0x09               // value
  };

  TEST_SETUP() {
    dev = co_dev_create(0x01);
    CHECK(dev != nullptr);

    co_obj_t* const obj = co_obj_create(0x1234);
    co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
    CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
    CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));
  }

  TEST_TEARDOWN() { co_dev_destroy(dev); }
};

TEST(CO_DevDCF, CoDevReadDef) {
  co_unsigned16_t pmin = 0x0000;
  co_unsigned16_t pmax = 0x0000;
  void* ptr = nullptr;
  co_val_init_dom(&ptr, NULL, BUF_SIZE);
  memcpy(ptr, buf, BUF_SIZE);

  const auto ret = co_dev_read_dcf(dev, &pmin, &pmax, &ptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x0987, co_dev_get_val_i16(dev, 0x1234, 0xab));
  CHECK_EQUAL(0x1234, pmin);
  CHECK_EQUAL(0x1234, pmax);

  co_val_fini(CO_DEFTYPE_DOMAIN, &ptr);
}

TEST(CO_DevDCF, CoDevReadDef_NullMinMax) {
  void* ptr = nullptr;
  co_val_init_dom(&ptr, NULL, BUF_SIZE);
  memcpy(ptr, buf, BUF_SIZE);

  const auto ret = co_dev_read_dcf(dev, nullptr, nullptr, &ptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x0987, co_dev_get_val_i16(dev, 0x1234, 0xab));

  co_val_fini(CO_DEFTYPE_DOMAIN, &ptr);
}

TEST(CO_DevDCF, CoDevReadDef_InvalidNumberOfSubIndexes) {
  void* ptr = nullptr;
  co_val_init_dom(&ptr, NULL, 2);

  const auto ret = co_dev_read_dcf(dev, nullptr, nullptr, &ptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x0000, co_dev_get_val_i16(dev, 0x1234, 0xab));

  co_val_fini(CO_DEFTYPE_DOMAIN, &ptr);
}

TEST(CO_DevDCF, CoDevReadDef_InvaildSubIdx) {
  void* ptr = nullptr;
  co_val_init_dom(&ptr, NULL, 7);
  memcpy(ptr, buf, 7);

  const auto ret = co_dev_read_dcf(dev, nullptr, nullptr, &ptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0x0000, co_dev_get_val_i16(dev, 0x1234, 0xab));

  co_val_fini(CO_DEFTYPE_DOMAIN, &ptr);
}

TEST(CO_DevDCF, CoDevWriteDef) {
  co_dev_set_val_i16(dev, 0x1234, 0xab, 0x0987);
  void* ptr = nullptr;

  const auto ret =
      co_dev_write_dcf(dev, CO_UNSIGNED16_MIN, CO_UNSIGNED16_MAX, &ptr);

  CHECK_EQUAL(0, ret);
  CheckBuffers(static_cast<uint_least8_t*>(ptr), buf, BUF_SIZE);

  co_val_fini(CO_DEFTYPE_DOMAIN, &ptr);
}

TEST(CO_DevDCF, CoDevWriteDef_BeforeMin) {
  void* ptr = nullptr;

  const auto ret = co_dev_write_dcf(dev, 0x1235, CO_UNSIGNED16_MAX, &ptr);

  CHECK_EQUAL(0, ret);

  co_val_fini(CO_DEFTYPE_DOMAIN, &ptr);
}

TEST(CO_DevDCF, CoDevWriteDef_AfterMax) {
  void* ptr = nullptr;

  const auto ret = co_dev_write_dcf(dev, CO_UNSIGNED16_MIN, 0x1233, &ptr);

  CHECK_EQUAL(0, ret);

  co_val_fini(CO_DEFTYPE_DOMAIN, &ptr);
}

#ifndef LELY_NO_CO_TPDO
namespace CO_DevTPDO_Static {
static unsigned int tpdo_event_ind_counter = 0;
}  // namespace CO_DevTPDO_Static

TEST_GROUP(CO_DevTPDO) {
  co_dev_t* dev = nullptr;

  static void tpdo_event_ind(co_unsigned16_t, void*) {
    ++CO_DevTPDO_Static::tpdo_event_ind_counter;
  }

  TEST_SETUP() {
    dev = co_dev_create(0x01);
    CHECK(dev != nullptr);

    CO_DevTPDO_Static::tpdo_event_ind_counter = 0;
  }

  TEST_TEARDOWN() { co_dev_destroy(dev); }
};

TEST(CO_DevTPDO, CoDevGetTpdoEventInd_Null) {
  co_dev_get_tpdo_event_ind(dev, nullptr, nullptr);
}

TEST(CO_DevTPDO, CoDevSetTpdoEventInd) {
  int data = 42;
  co_dev_set_tpdo_event_ind(dev, tpdo_event_ind, &data);

  co_dev_tpdo_event_ind_t* ind_ptr = nullptr;
  void* data_ptr = nullptr;
  co_dev_get_tpdo_event_ind(dev, &ind_ptr, &data_ptr);
  FUNCTIONPOINTERS_EQUAL(tpdo_event_ind, ind_ptr);
  POINTERS_EQUAL(&data, data_ptr);
}

TEST(CO_DevTPDO, CoDevTpdoEvent_Empty) { co_dev_tpdo_event(dev, 0x0000, 0x00); }

TEST(CO_DevTPDO, CoDevTpdoEvent_OnlySubNoMapping) {
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));
  co_dev_set_tpdo_event_ind(dev, tpdo_event_ind, nullptr);

  co_dev_tpdo_event(dev, 0x1234, 0xab);
  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

TEST(CO_DevTPDO, CoDevTpdoEvent_MappingPossibleButNoMapping) {
  co_obj_t* const obj = co_obj_create(0x1234);
  co_sub_t* const sub = co_sub_create(0xab, CO_DEFTYPE_INTEGER16);
  CHECK_EQUAL(0, co_obj_insert_sub(obj, sub));
  CHECK_EQUAL(0, co_dev_insert_obj(dev, obj));
  co_sub_set_pdo_mapping(sub, 1);
  co_dev_set_tpdo_event_ind(dev, tpdo_event_ind, nullptr);

  co_dev_tpdo_event(dev, 0x1234, 0xab);
  CHECK_EQUAL(0, CO_DevTPDO_Static::tpdo_event_ind_counter);
}

// TODO(tph): missing co_dev_tpdo_event() tests

#endif  // !LELY_NO_CO_TPDO
