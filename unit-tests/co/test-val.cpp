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
#include <list>

#include <CppUTest/TestHarness.h>

#include <lely/co/val.h>
#include <lely/co/type.h>
#include <lely/co/sdo.h>
#include <lely/util/endian.h>
#include <lely/util/errnum.h>
#include <lely/util/ustring.h>

#include "array-init.hpp"

TEST_GROUP(CO_Val) {
  static const co_unsigned16_t INVALID_TYPE = 0xffffu;
  const char* const TEST_STR = "testtesttest";
  const char16_t* const TEST_STR16 = u"testtesttest";
  static constexpr size_t MAX_VAL_SIZE = 8u;

  CoArrays arrays;

  TEST_TEARDOWN() { arrays.Clear(); }

  /**
   * Returns the read/write size (in bytes) of the value of the specified type.
   * In most cases function returns the same value as co_type_sizeof(), but for
   * a few integer/unsigned types size is different then the size in memory.
   *
   * @see co_type_sizeof()
   */
  static size_t ValGetReadWriteSize(co_unsigned16_t type) {
    switch (type) {
      case CO_DEFTYPE_INTEGER24:
        return 3u;
      case CO_DEFTYPE_INTEGER40:
        return 5u;
      case CO_DEFTYPE_INTEGER48:
        return 6u;
      case CO_DEFTYPE_INTEGER56:
        return 7u;
      case CO_DEFTYPE_UNSIGNED24:
        return 3u;
      case CO_DEFTYPE_UNSIGNED40:
        return 5u;
      case CO_DEFTYPE_UNSIGNED48:
        return 6u;
      case CO_DEFTYPE_UNSIGNED56:
        return 7u;
      case CO_DEFTYPE_TIME_OF_DAY:
      case CO_DEFTYPE_TIME_DIFF:
        return 6u;
      default:
        return co_type_sizeof(type);
    }
  }

  static co_boolean_t ldle_b(const uint_least8_t src[1]) { return (src != 0); }
  static co_unsigned8_t ldle_u8(const uint_least8_t src[1]) { return *src; }
  static co_unsigned24_t ldle_u24(const uint_least8_t src[4]) {
    return ldle_u32(src) & 0x00ffffff;
  }
  static co_unsigned40_t ldle_u40(const uint_least8_t src[8]) {
    return ldle_u64(src) & 0x000000ffffffffff;
  }
  static co_unsigned48_t ldle_u48(const uint_least8_t src[8]) {
    return ldle_u64(src) & 0x0000ffffffffffff;
  }
  static co_unsigned56_t ldle_u56(const uint_least8_t src[8]) {
    return ldle_u64(src) & 0x00ffffffffffffff;
  }
  static co_integer8_t ldle_i8(const uint_least8_t src[1]) { return *src; }
  static co_integer24_t ldle_i24(const uint_least8_t src[4]) {
    co_unsigned24_t u24 = ldle_u24(src);
    if (u24 > CO_INTEGER24_MAX)
      return -(CO_UNSIGNED24_MAX + 1 - u24);
    else
      return u24;
  }
  static co_integer40_t ldle_i40(const uint_least8_t src[8]) {
    co_unsigned40_t u40 = ldle_u40(src);
    if (u40 > CO_INTEGER40_MAX)
      return -(CO_UNSIGNED40_MAX + 1 - u40);
    else
      return u40;
  }
  static co_integer48_t ldle_i48(const uint_least8_t src[8]) {
    co_unsigned48_t u48 = ldle_u48(src);
    if (u48 > CO_INTEGER48_MAX)
      return -(CO_UNSIGNED48_MAX + 1 - u48);
    else
      return u48;
  }
  static co_integer56_t ldle_i56(const uint_least8_t src[8]) {
    co_unsigned56_t u56 = ldle_u56(src);
    if (u56 > CO_INTEGER56_MAX)
      return -(CO_UNSIGNED56_MAX + 1 - u56);
    else
      return u56;
  }
  static co_real32_t ldle_r32(const uint_least8_t src[4]) {
    return ldle_flt32(src);
  }
  static co_real64_t ldle_r64(const uint_least8_t src[8]) {
    return ldle_flt64(src);
  }

  static void stle_b(uint_least8_t dst[1], co_boolean_t val) { *dst = val; }
  static void stle_u8(uint_least8_t dst[1], co_unsigned8_t val) { *dst = val; }
  static void stle_u24(uint_least8_t dst[4], co_unsigned24_t val) {
    stle_u32(dst, val);
  }
  static void stle_u40(uint_least8_t dst[8], co_unsigned40_t val) {
    stle_u64(dst, val);
  }
  static void stle_u48(uint_least8_t dst[8], co_unsigned48_t val) {
    stle_u64(dst, val);
  }
  static void stle_u56(uint_least8_t dst[8], co_unsigned56_t val) {
    stle_u64(dst, val);
  }
  static void stle_i8(uint_least8_t dst[1], co_integer8_t val) { *dst = val; }
  static void stle_i24(uint_least8_t dst[4], co_integer24_t val) {
    if (val < 0) val = CO_UNSIGNED24_MAX + 1 + val;
    stle_u32(dst, val);
  }
  static void stle_i40(uint_least8_t dst[8], co_integer40_t val) {
    if (val < 0) val = CO_UNSIGNED40_MAX + 1 + val;
    stle_u64(dst, val);
  }
  static void stle_i48(uint_least8_t dst[8], co_integer48_t val) {
    if (val < 0) val = CO_UNSIGNED48_MAX + 1 + val;
    stle_u64(dst, val);
  }
  static void stle_i56(uint_least8_t dst[8], co_integer56_t val) {
    if (val < 0) val = CO_UNSIGNED56_MAX + 1 + val;
    stle_u64(dst, val);
  }
  static void stle_r32(uint_least8_t dst[4], co_real32_t val) {
    stle_flt32(dst, val);
  }
  static void stle_r64(uint_least8_t dst[8], co_real64_t val) {
    stle_flt64(dst, val);
  }
};

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Val, CoValInit_##a) { \
    co_##b##_t val; \
\
    const auto ret = co_val_init(CO_DEFTYPE_##a, &val); \
\
    CHECK_EQUAL(0, ret); \
    const char testbuf[sizeof(co_##b##_t)] = {0}; \
    CHECK_EQUAL(0, memcmp(testbuf, &val, sizeof(val))); \
  }
#include <lely/co/def/basic.def>
#if !LELY_NO_MALLOC
#include <lely/co/def/array.def>
#endif
#undef LELY_CO_DEFINE_TYPE

#if LELY_NO_MALLOC
#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Val, CoValInit_##a) { \
    co_##b##_t val = arrays.Init<co_##b##_t>(); \
\
    const auto ret = co_val_init(CO_DEFTYPE_##a, &val); \
\
    CHECK_EQUAL(0, ret); \
    const char testbuf[CO_ARRAY_CAPACITY] = {0}; \
    CHECK_EQUAL(0, memcmp(testbuf, val, CO_ARRAY_CAPACITY)); \
  }
#include <lely/co/def/array.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
#endif  // LELY_NO_MALLOC

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Val, CoValInit_##a) { \
    co_##b##_t val; \
\
    const auto ret = co_val_init(CO_DEFTYPE_##a, &val); \
\
    CHECK_EQUAL(0, ret); \
    CHECK_EQUAL(0, val.days); \
    CHECK_EQUAL(0, val.ms); \
  }
#include <lely/co/def/time.def>
#undef LELY_CO_DEFINE_TYPE

TEST(CO_Val, CoValInit_Invalid) {
  char val;

  const auto ret = co_val_init(INVALID_TYPE, &val);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Val, CoValInitMin_##a) { \
    co_##b##_t val; \
\
    const auto ret = co_val_init_min(CO_DEFTYPE_##a, &val); \
\
    CHECK_EQUAL(0, ret); \
    CHECK_EQUAL(CO_##a##_MIN, val); \
  } \
\
  TEST(CO_Val, CoValInitMax_##a) { \
    co_##b##_t val; \
\
    const auto ret = co_val_init_max(CO_DEFTYPE_##a, &val); \
\
    CHECK_EQUAL(0, ret); \
    CHECK_EQUAL(CO_##a##_MAX, val); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Val, CoValInitMin_##a) { \
    co_##b##_t val; \
\
    const auto ret = co_val_init_min(CO_DEFTYPE_##a, &val); \
\
    CHECK_EQUAL(0, ret); \
    CHECK_EQUAL(0, val.days); \
    CHECK_EQUAL(0, val.ms); \
  } \
\
  TEST(CO_Val, CoValInitMax_##a) { \
    co_time_of_day_t val; \
\
    const auto ret = co_val_init_max(CO_DEFTYPE_##a, &val); \
\
    CHECK_EQUAL(0, ret); \
    CHECK_EQUAL(UINT16_MAX, val.days); \
    CHECK_EQUAL(UINT32_C(0x0fffffff), val.ms); \
  }
#include <lely/co/def/time.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE

TEST(CO_Val, CoValInitMin_VISIBLE_STRING) {
  co_visible_string_t val = arrays.DeadBeef<co_visible_string_t>();

  const auto ret = co_val_init_min(CO_DEFTYPE_VISIBLE_STRING, &val);

  CHECK_EQUAL(0, ret);
  CHECK(arrays.IsEmptyInitialized(val));
}

TEST(CO_Val, CoValInitMax_VISIBLE_STRING) {
  co_visible_string_t val = arrays.DeadBeef<co_visible_string_t>();

  const auto ret = co_val_init_max(CO_DEFTYPE_VISIBLE_STRING, &val);

  CHECK_EQUAL(0, ret);
  CHECK(arrays.IsEmptyInitialized(val));
}

TEST(CO_Val, CoValInitMin_OCTET_STRING) {
  co_octet_string_t val = arrays.DeadBeef<co_octet_string_t>();

  const auto ret = co_val_init_min(CO_DEFTYPE_OCTET_STRING, &val);

  CHECK_EQUAL(0, ret);
  CHECK(arrays.IsEmptyInitialized(val));
}

TEST(CO_Val, CoValInitMax_OCTET_STRING) {
  co_octet_string_t val = arrays.DeadBeef<co_octet_string_t>();

  const auto ret = co_val_init_max(CO_DEFTYPE_OCTET_STRING, &val);

  CHECK_EQUAL(0, ret);
  CHECK(arrays.IsEmptyInitialized(val));
}

TEST(CO_Val, CoValInitMin_UNICODE_STRING) {
  co_unicode_string_t val = arrays.DeadBeef<co_unicode_string_t>();

  const auto ret = co_val_init_min(CO_DEFTYPE_UNICODE_STRING, &val);

  CHECK_EQUAL(0, ret);
  CHECK(arrays.IsEmptyInitialized(val));
}

TEST(CO_Val, CoValInitMax_UNICODE_STRING) {
  co_unicode_string_t val = arrays.DeadBeef<co_unicode_string_t>();

  const auto ret = co_val_init_max(CO_DEFTYPE_UNICODE_STRING, &val);

  CHECK_EQUAL(0, ret);
  CHECK(arrays.IsEmptyInitialized(val));
}

TEST(CO_Val, CoValInitMin_DOMAIN) {
  co_domain_t val = arrays.DeadBeef<co_domain_t>();

  const auto ret = co_val_init_min(CO_DEFTYPE_DOMAIN, &val);

  CHECK_EQUAL(0, ret);
  CHECK(arrays.IsEmptyInitialized(val));
}

TEST(CO_Val, CoValInitMax_DOMAIN) {
  co_domain_t val = arrays.DeadBeef<co_domain_t>();

  const auto ret = co_val_init_max(CO_DEFTYPE_DOMAIN, &val);

  CHECK_EQUAL(0, ret);
  CHECK(arrays.IsEmptyInitialized(val));
}

TEST(CO_Val, CoValInitMin_Invalid) {
  char val;
  CHECK_EQUAL(-1, co_val_init_min(INVALID_TYPE, &val));
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_Val, CoValInitMax_Invalid) {
  char val;

  const auto ret = co_val_init_max(INVALID_TYPE, &val);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_Val, CoValInitVs) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();

  const auto ret = co_val_init_vs(&val, TEST_STR);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(strlen(TEST_STR), co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val));
  CHECK_EQUAL(0, memcmp(TEST_STR, val, strlen(TEST_STR) + 1));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

TEST(CO_Val, CoValInitVs_Null) {
  co_visible_string_t val = nullptr;

  const auto ret = co_val_init_vs(&val, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

TEST(CO_Val, CoValInitVsN) {
  const size_t n = 4u;
  co_visible_string_t val = arrays.Init<co_visible_string_t>();

  const auto ret = co_val_init_vs_n(&val, TEST_STR, n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val));
  char testbuf[n + 1] = {0};
  POINTERS_EQUAL(testbuf, memcpy(testbuf, TEST_STR, n));
  CHECK_EQUAL(0, memcmp(testbuf, val, n + 1));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

TEST(CO_Val, CoValInitVsN_Null) {
  const size_t n = 7u;
  co_visible_string_t val = arrays.Init<co_visible_string_t>();

  const auto ret = co_val_init_vs_n(&val, nullptr, n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val));
  const char testbuf[n + 1] = {0};
  CHECK_EQUAL(0, memcmp(testbuf, val, n + 1));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

TEST(CO_Val, CoValInitVsN_Zero) {
  co_visible_string_t val = nullptr;

  const auto ret = co_val_init_vs_n(&val, nullptr, 0);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, val);
}

#if LELY_NO_MALLOC
TEST(CO_Val, CoValInitVsN_TooBigValue) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();
  const char buf[CO_ARRAY_CAPACITY + 1] = {0};

  const auto ret = co_val_init_vs_n(&val, buf, sizeof(buf));

  CHECK_EQUAL(-1, ret);
}
#endif

TEST(CO_Val, CoValInitOs) {
  const size_t n = 5u;
  const uint_least8_t os[n] = {0xd3u, 0xe5u, 0x98u, 0xbau, 0x96u};
  co_octet_string_t val = arrays.Init<co_octet_string_t>();

  const auto ret = co_val_init_os(&val, os, n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));
  CHECK_EQUAL(0, memcmp(os, val, n));

  co_val_fini(CO_DEFTYPE_OCTET_STRING, &val);
}

TEST(CO_Val, CoValInitOs_Null) {
  const size_t n = 9u;
  co_octet_string_t val = arrays.Init<co_octet_string_t>();

  const auto ret = co_val_init_os(&val, nullptr, n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));
  const uint_least8_t testbuf[n] = {0x00};
  CHECK_EQUAL(0, memcmp(testbuf, val, n));

  co_val_fini(CO_DEFTYPE_OCTET_STRING, &val);
}

TEST(CO_Val, CoValInitOs_Zero) {
  co_octet_string_t val = nullptr;

  const auto ret = co_val_init_os(&val, nullptr, 0);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, val);
}

#if LELY_NO_MALLOC
TEST(CO_Val, CoValInitOs_TooBigValue) {
  co_octet_string_t val = arrays.Init<co_octet_string_t>();
  const uint_least8_t buf[CO_ARRAY_CAPACITY + 1] = {0};

  const auto ret = co_val_init_os(&val, buf, sizeof(buf));

  CHECK_EQUAL(-1, ret);
}
#endif

TEST(CO_Val, CoValInitUs) {
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();
  const size_t us_val_len = str16len(TEST_STR16) * sizeof(char16_t);

  const auto ret = co_val_init_us(&val, TEST_STR16);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(us_val_len, co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &val));
  CHECK_EQUAL(0, memcmp(TEST_STR16, val, us_val_len));

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val);
}

TEST(CO_Val, CoValInitUs_Null) {
  co_unicode_string_t val = nullptr;

  const auto ret = co_val_init_us(&val, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &val));

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val);
}

TEST(CO_Val, CoValInitUsN) {
  const size_t n = 6u;
  const size_t us_val_len = n * sizeof(char16_t);
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();

  const auto ret = co_val_init_us_n(&val, TEST_STR16, n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(us_val_len, co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &val));
  char16_t testbuf[n + 1] = {0};
  POINTERS_EQUAL(testbuf, memcpy(testbuf, TEST_STR16, us_val_len));
  CHECK_EQUAL(0, memcmp(testbuf, val, us_val_len + sizeof(char16_t)));

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val);
}

TEST(CO_Val, CoValInitUsN_Null) {
  const size_t n = 8u;
  const size_t us_val_len = n * sizeof(char16_t);
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();

  const auto ret = co_val_init_us_n(&val, nullptr, n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(us_val_len, co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &val));
  const char16_t testbuf[n + 1] = {0};
  CHECK_EQUAL(0, memcmp(testbuf, val, us_val_len + sizeof(char16_t)));

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val);
}

TEST(CO_Val, CoValInitUsN_Zero) {
  co_unicode_string_t val = nullptr;

  const auto ret = co_val_init_us_n(&val, nullptr, 0);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, val);
}

#if LELY_NO_MALLOC
TEST(CO_Val, CoValInitUsN_TooBigValue) {
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();
  const char16_t buf[CO_ARRAY_CAPACITY + 1] = {0};

  const auto ret = co_val_init_us_n(&val, buf, CO_ARRAY_CAPACITY + 1);

  CHECK_EQUAL(-1, ret);
}
#endif

TEST(CO_Val, CoValInitDom) {
  const size_t n = 4u;
  const unsigned char dom[n] = {0xd3u, 0xe5u, 0x98u, 0xbau};
  co_domain_t val = arrays.Init<co_domain_t>();

  const auto ret = co_val_init_dom(&val, dom, n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));
  CHECK_EQUAL(0, memcmp(dom, val, n));

  co_val_fini(CO_DEFTYPE_DOMAIN, &val);
}

TEST(CO_Val, CoValInitDom_Null) {
  const size_t n = 7u;
  co_domain_t val = arrays.Init<co_domain_t>();

  const auto ret = co_val_init_dom(&val, nullptr, n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));
  const unsigned char testbuf[n] = {0};
  CHECK_EQUAL(0, memcmp(testbuf, val, n));

  co_val_fini(CO_DEFTYPE_DOMAIN, &val);
}

TEST(CO_Val, CoValInitDom_Zero) {
  co_domain_t val = nullptr;

  const auto ret = co_val_init_dom(&val, nullptr, 0);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, val);
}

#if LELY_NO_MALLOC
TEST(CO_Val, CoValInitDom_TooBigValue) {
  co_domain_t val = arrays.Init<co_domain_t>();
  const char buf[CO_ARRAY_CAPACITY + 1] = {0};

  const auto ret = co_val_init_dom(&val, buf, sizeof(buf));

  CHECK_EQUAL(-1, ret);
}
#endif

TEST(CO_Val, CoValFini_BasicType) {
  co_unsigned16_t val;
  CHECK_EQUAL(0, co_val_init(CO_DEFTYPE_INTEGER16, &val));

  co_val_fini(CO_DEFTYPE_INTEGER16, &val);
}

TEST(CO_Val, CoValFini_ArrayType) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(0, co_val_init_vs(&val, TEST_STR));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

TEST(CO_Val, CoValAddressof_Null) {
  POINTERS_EQUAL(nullptr, co_val_addressof(INVALID_TYPE, nullptr));
}

TEST(CO_Val, CoValAddressof_ArrayType) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();
  co_val_init_vs(&val, TEST_STR);

  const auto* ptr = co_val_addressof(CO_DEFTYPE_VISIBLE_STRING, val);

  POINTERS_EQUAL(*reinterpret_cast<void**>(val), ptr);

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

TEST(CO_Val, CoValAddressof_BasicType) {
  co_integer16_t val;
  co_val_init(CO_DEFTYPE_INTEGER16, &val);

  const auto* ptr = co_val_addressof(CO_DEFTYPE_INTEGER16, &val);

  POINTERS_EQUAL(ptr, &val);

  co_val_fini(CO_DEFTYPE_INTEGER16, &val);
}

TEST(CO_Val, CoValSizeof_Null) {
  CHECK_EQUAL(0, co_val_sizeof(INVALID_TYPE, nullptr));
}

TEST(CO_Val, CoValSizeof_ArrayType) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();
  co_val_init_vs(&val, TEST_STR);

  const auto ret = co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val);

  CHECK_EQUAL(strlen(TEST_STR), ret);

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

TEST(CO_Val, CoValSizeof_BasicType) {
  co_integer16_t val;
  co_val_init(CO_DEFTYPE_INTEGER16, &val);

  const auto ret = co_val_sizeof(CO_DEFTYPE_INTEGER16, &val);

  CHECK_EQUAL(sizeof(val), ret);

  co_val_fini(CO_DEFTYPE_INTEGER16, &val);
}

#if LELY_NO_MALLOC
TEST(CO_Val, CoValMake_ArrayType_NullValue) {
  co_visible_string_t val = nullptr;
  char buf[CO_ARRAY_CAPACITY + 1] = {0};
  memset(buf, 'a', CO_ARRAY_CAPACITY);

  const auto ret = co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val, buf, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}
#endif

TEST(CO_Val, CoValMake_VISIBLE_STRING) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();

  const auto ret = co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val, TEST_STR, 0);

  CHECK_EQUAL(strlen(TEST_STR), ret);
  CHECK_EQUAL(strlen(TEST_STR), co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val));
  CHECK_EQUAL(0, memcmp(TEST_STR, val, strlen(TEST_STR) + 1));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

TEST(CO_Val, CoValMake_VISIBLE_STRING_Null) {
  co_visible_string_t val = nullptr;

  const auto ret = co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val, nullptr, 3u);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

#if LELY_NO_MALLOC
TEST(CO_Val, CoValMake_VISIBLE_STRING_TooBigValue) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();
  char buf[CO_ARRAY_CAPACITY + 1] = {0};
  memset(buf, 'a', CO_ARRAY_CAPACITY);

  const auto ret = co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val, buf, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val));
}
#endif

TEST(CO_Val, CoValMake_OCTET_STRING) {
  const size_t n = 5u;
  const uint_least8_t os[n] = {0xd3u, 0xe5u, 0x98u, 0xbau, 0x96u};
  co_octet_string_t val = arrays.Init<co_octet_string_t>();

  const auto ret = co_val_make(CO_DEFTYPE_OCTET_STRING, &val, os, n);

  CHECK_EQUAL(n, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));
  CHECK_EQUAL(0, memcmp(os, val, n));

  co_val_fini(CO_DEFTYPE_OCTET_STRING, &val);
}

TEST(CO_Val, CoValMake_OCTET_STRING_Null) {
  co_octet_string_t val = nullptr;

  const auto ret = co_val_make(CO_DEFTYPE_OCTET_STRING, &val, nullptr, 7u);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));

  co_val_fini(CO_DEFTYPE_OCTET_STRING, &val);
}

#if LELY_NO_MALLOC
TEST(CO_Val, CoValMake_OCTET_STRING_TooBigValue) {
  co_octet_string_t val = arrays.Init<co_octet_string_t>();
  const uint_least8_t buf[CO_ARRAY_CAPACITY + 1] = {0};

  const auto ret = co_val_make(CO_DEFTYPE_OCTET_STRING, &val, buf, sizeof(buf));

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));
}
#endif

TEST(CO_Val, CoValMake_UNICODE_STRING) {
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();
  const size_t us_val_len = str16len(TEST_STR16) * sizeof(char16_t);

  const auto ret = co_val_make(CO_DEFTYPE_UNICODE_STRING, &val, TEST_STR16, 0);

  CHECK_EQUAL(str16len(TEST_STR16), ret);
  CHECK_EQUAL(us_val_len, co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &val));
  CHECK_EQUAL(0, memcmp(TEST_STR16, val, us_val_len));

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val);
}

TEST(CO_Val, CoValMake_UNICODE_STRING_Null) {
  co_unicode_string_t val = nullptr;

  const auto ret = co_val_make(CO_DEFTYPE_UNICODE_STRING, &val, nullptr, 4u);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &val));

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val);
}

#if LELY_NO_MALLOC
TEST(CO_Val, CoValMake_UNICODE_STRING_TooBigValue) {
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();
  char16_t buf[CO_ARRAY_CAPACITY + 1] = {0};
  // incorrect unicode, but good enough for test
  memset(buf, 'a', sizeof(buf) - sizeof(char16_t));

  const auto ret = co_val_make(CO_DEFTYPE_UNICODE_STRING, &val, buf, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &val));
}
#endif

TEST(CO_Val, CoValMake_DOMAIN) {
  const size_t n = 4u;
  const unsigned char dom[n] = {0xd3u, 0xe5u, 0x98u, 0xbau};
  co_domain_t val = arrays.Init<co_domain_t>();

  const auto ret = co_val_make(CO_DEFTYPE_DOMAIN, &val, dom, n);

  CHECK_EQUAL(n, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));
  CHECK_EQUAL(0, memcmp(dom, val, n));

  co_val_fini(CO_DEFTYPE_DOMAIN, &val);
}

TEST(CO_Val, CoValMake_DOMAIN_Null) {
  const size_t n = 7u;
  co_domain_t val = nullptr;

  const auto ret = co_val_make(CO_DEFTYPE_DOMAIN, &val, nullptr, n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));

  co_val_fini(CO_DEFTYPE_DOMAIN, &val);
}

#if LELY_NO_MALLOC
TEST(CO_Val, CoValMake_DOMAIN_TooBigValue) {
  co_domain_t val = arrays.Init<co_domain_t>();
  const char buf[CO_ARRAY_CAPACITY + 1] = {0};

  const auto ret = co_val_make(CO_DEFTYPE_DOMAIN, &val, buf, sizeof(buf));

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_DOMAIN, &val));
}
#endif

TEST(CO_Val, CoValMake_BasicType) {
  co_integer16_t val;
  const char ptr[] = {0x42, 0x00};

  const auto ret = co_val_make(CO_DEFTYPE_INTEGER16, &val, ptr, sizeof(val));

  CHECK_EQUAL(sizeof(val), ret);
  CHECK_EQUAL(0x0042, val);
}

TEST(CO_Val, CoValMake_BasicType_Null) {
  co_integer16_t val;

  const auto ret =
      co_val_make(CO_DEFTYPE_INTEGER16, &val, nullptr, sizeof(val));

  CHECK_EQUAL(0, ret);
}

TEST(CO_Val, CoValMake_BasicType_WrongSize) {
  co_integer16_t val;
  const char ptr[sizeof(val) + 1] = {0};

  const auto ret =
      co_val_make(CO_DEFTYPE_INTEGER16, &val, ptr, sizeof(val) + 1);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Val, CoValCopy_VISIBLE_STRING) {
  co_visible_string_t src = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(strlen(TEST_STR),
              co_val_make(CO_DEFTYPE_VISIBLE_STRING, &src, TEST_STR, 0));
  co_visible_string_t dst = arrays.Init<co_visible_string_t>();

  const auto ret = co_val_copy(CO_DEFTYPE_VISIBLE_STRING, &dst, &src);

  CHECK(dst != nullptr);
  CHECK_EQUAL(strlen(TEST_STR), ret);
  CHECK_EQUAL(0, memcmp(TEST_STR, dst, strlen(TEST_STR) + 1));
  CHECK(co_val_addressof(CO_DEFTYPE_VISIBLE_STRING, &src) !=
        co_val_addressof(CO_DEFTYPE_VISIBLE_STRING, &dst));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &src);
  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &dst);
}

#if LELY_NO_MALLOC
TEST(CO_Val, CoValCopy_VISIBLE_STRING_TooSmallDestination) {
  co_visible_string_t src = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(strlen(TEST_STR),
              co_val_make(CO_DEFTYPE_VISIBLE_STRING, &src, TEST_STR, 0));
  co_array dst_array = CO_ARRAY_INIT;
  dst_array.hdr.capacity = strlen(TEST_STR) - 1;
  co_visible_string_t dst;
  co_val_init_array(&dst, &dst_array);

  const auto ret = co_val_copy(CO_DEFTYPE_VISIBLE_STRING, &dst, &src);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, dst_array.hdr.size);
}
#endif

TEST(CO_Val, CoValCopy_OCTET_STRING) {
  const size_t n = 5u;
  const uint_least8_t os[n] = {0xd3u, 0xe5u, 0x98u, 0xbau, 0x96u};
  co_octet_string_t src = arrays.Init<co_octet_string_t>();
  CHECK_EQUAL(n, co_val_make(CO_DEFTYPE_OCTET_STRING, &src, os, n));
  co_octet_string_t dst = arrays.Init<co_octet_string_t>();

  const auto ret = co_val_copy(CO_DEFTYPE_OCTET_STRING, &dst, &src);

  CHECK(dst != nullptr);
  CHECK_EQUAL(n, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &dst));
  CHECK_EQUAL(0, memcmp(os, dst, n));
  CHECK(co_val_addressof(CO_DEFTYPE_OCTET_STRING, &src) !=
        co_val_addressof(CO_DEFTYPE_OCTET_STRING, &dst));

  co_val_fini(CO_DEFTYPE_OCTET_STRING, &src);
  co_val_fini(CO_DEFTYPE_OCTET_STRING, &dst);
}

#if LELY_NO_MALLOC
TEST(CO_Val, CoValCopy_OCTET_STRING_TooSmallDestination) {
  const size_t n = 5u;
  co_octet_string_t src = arrays.Init<co_octet_string_t>();
  const uint_least8_t os[n] = {0xd3u, 0xe5u, 0x98u, 0xbau, 0x96u};
  CHECK_EQUAL(n, co_val_make(CO_DEFTYPE_OCTET_STRING, &src, os, n));
  co_array dst_array = CO_ARRAY_INIT;
  dst_array.hdr.capacity = n - 1;
  co_octet_string_t dst;
  co_val_init_array(&dst, &dst_array);

  const auto ret = co_val_copy(CO_DEFTYPE_OCTET_STRING, &dst, &src);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, dst_array.hdr.size);
}
#endif

TEST(CO_Val, CoValCopy_UNICODE_STRING) {
  co_unicode_string_t src = arrays.Init<co_unicode_string_t>();
  const size_t us_val_len = str16len(TEST_STR16) * sizeof(char16_t);
  CHECK_EQUAL(str16len(TEST_STR16),
              co_val_make(CO_DEFTYPE_UNICODE_STRING, &src, TEST_STR16, 0));
  co_unicode_string_t dst = arrays.Init<co_unicode_string_t>();

  const auto ret = co_val_copy(CO_DEFTYPE_UNICODE_STRING, &dst, &src);

  CHECK(dst != nullptr);
  CHECK_EQUAL(us_val_len, ret);
  CHECK_EQUAL(us_val_len, co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &dst));
  CHECK_EQUAL(0, memcmp(TEST_STR16, dst, us_val_len));
  CHECK(co_val_addressof(CO_DEFTYPE_UNICODE_STRING, &src) !=
        co_val_addressof(CO_DEFTYPE_UNICODE_STRING, &dst));

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &src);
  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &dst);
}

#if LELY_NO_MALLOC
TEST(CO_Val, CoValCopy_UNICODE_STRING_TooSmallDestination) {
  co_unicode_string_t src = arrays.Init<co_unicode_string_t>();
  CHECK_EQUAL(str16len(TEST_STR16),
              co_val_make(CO_DEFTYPE_UNICODE_STRING, &src, TEST_STR16, 0));
  co_array dst_array = CO_ARRAY_INIT;
  dst_array.hdr.capacity = str16len(TEST_STR16) - 1;
  co_unicode_string_t dst;
  co_val_init_array(&dst, &dst_array);

  const auto ret = co_val_copy(CO_DEFTYPE_UNICODE_STRING, &dst, &src);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, dst_array.hdr.size);
}
#endif

TEST(CO_Val, CoValCopy_DOMAIN) {
  const size_t n = 4u;
  const unsigned char dom[n] = {0xd3u, 0xe5u, 0x98u, 0xbau};
  co_domain_t src = arrays.Init<co_domain_t>();
  CHECK_EQUAL(n, co_val_make(CO_DEFTYPE_DOMAIN, &src, dom, n));
  co_domain_t dst = arrays.Init<co_domain_t>();

  const auto ret = co_val_copy(CO_DEFTYPE_DOMAIN, &dst, &src);

  CHECK(dst != nullptr);
  CHECK_EQUAL(n, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_DOMAIN, &dst));
  CHECK_EQUAL(0, memcmp(dom, dst, n));
  CHECK(co_val_addressof(CO_DEFTYPE_DOMAIN, &src) !=
        co_val_addressof(CO_DEFTYPE_DOMAIN, &dst));

  co_val_fini(CO_DEFTYPE_DOMAIN, &src);
  co_val_fini(CO_DEFTYPE_DOMAIN, &dst);
}

#if LELY_NO_MALLOC
TEST(CO_Val, CoValCopy_DOMAIN_TooSmallDestination) {
  const size_t n = 4u;
  const unsigned char dom[n] = {0xd3u, 0xe5u, 0x98u, 0xbau};
  co_domain_t src = arrays.Init<co_domain_t>();
  CHECK_EQUAL(n, co_val_make(CO_DEFTYPE_DOMAIN, &src, dom, n));
  co_array dst_array = CO_ARRAY_INIT;
  dst_array.hdr.capacity = n - 1;
  co_domain_t dst;
  co_val_init_array(&dst, &dst_array);

  const auto ret = co_val_copy(CO_DEFTYPE_DOMAIN, &dst, &src);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, dst_array.hdr.size);
}
#endif

TEST(CO_Val, CoValCopy_BasicType) {
  co_integer16_t src;
  const char ptr[] = {0x42, 0x00};
  CHECK_EQUAL(sizeof(src),
              co_val_make(CO_DEFTYPE_INTEGER16, &src, ptr, sizeof(src)));
  co_integer16_t dst;

  const auto ret = co_val_copy(CO_DEFTYPE_INTEGER16, &dst, &src);

  CHECK_EQUAL(sizeof(dst), ret);
  CHECK_EQUAL(0x0042, dst);
}

TEST(CO_Val, CoValMove_BasicType) {
  co_integer16_t src;
  const char ptr[] = {0x42, 0x00};
  CHECK_EQUAL(sizeof(src),
              co_val_make(CO_DEFTYPE_INTEGER16, &src, ptr, sizeof(src)));
  co_integer16_t dst;

  const auto ret = co_val_move(CO_DEFTYPE_INTEGER16, &dst, &src);

  CHECK_EQUAL(sizeof(dst), ret);
  CHECK_EQUAL(0x0042, dst);
}

TEST(CO_Val, CoValMove_ArrayType) {
  co_visible_string_t src = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(strlen(TEST_STR),
              co_val_make(CO_DEFTYPE_VISIBLE_STRING, &src, TEST_STR, 0));
  const void* src_addr = co_val_addressof(CO_DEFTYPE_VISIBLE_STRING, &src);
  co_visible_string_t dst = arrays.Init<co_visible_string_t>();

  const auto ret = co_val_move(CO_DEFTYPE_VISIBLE_STRING, &dst, &src);

  CHECK_EQUAL(sizeof(src), ret);
  CHECK_EQUAL(0, memcmp(TEST_STR, dst, strlen(TEST_STR) + 1));
  POINTERS_EQUAL(src_addr, co_val_addressof(CO_DEFTYPE_VISIBLE_STRING, &dst));
  POINTERS_EQUAL(nullptr, co_val_addressof(CO_DEFTYPE_VISIBLE_STRING, &src));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &dst);
}

TEST(CO_Val, CoValCmp_PointersEqual) {
  co_integer16_t val;
  CHECK_EQUAL(0, co_val_init(CO_DEFTYPE_INTEGER16, &val));

  CHECK_EQUAL(0, co_val_cmp(CO_DEFTYPE_INTEGER16, &val, &val));
}

TEST(CO_Val, CoValCmp_FirstValNull) {
  co_integer16_t val;
  CHECK_EQUAL(0, co_val_init(CO_DEFTYPE_INTEGER16, &val));

  CHECK_EQUAL(-1, co_val_cmp(CO_DEFTYPE_INTEGER16, nullptr, &val));
}

TEST(CO_Val, CoValCmp_SecondValNull) {
  co_integer16_t val;
  CHECK_EQUAL(0, co_val_init(CO_DEFTYPE_INTEGER16, &val));

  CHECK_EQUAL(1, co_val_cmp(CO_DEFTYPE_INTEGER16, &val, nullptr));
}

TEST(CO_Val, CoValCmp_ArrayType_PointersEqual) {
  co_visible_string_t val1 = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(strlen(TEST_STR),
              co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val1, TEST_STR, 0));
  co_visible_string_t val2 = val1;

  CHECK_EQUAL(0, co_val_cmp(CO_DEFTYPE_VISIBLE_STRING, &val1, &val2));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val1);
}

TEST(CO_Val, CoValCmp_ArrayType_FirstValNull) {
  co_visible_string_t val1 = nullptr;
  CHECK_EQUAL(0, co_val_init(CO_DEFTYPE_VISIBLE_STRING, &val1));
  co_visible_string_t val2 = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(strlen(TEST_STR),
              co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val2, TEST_STR, 0));

  CHECK_EQUAL(-1, co_val_cmp(CO_DEFTYPE_VISIBLE_STRING, &val1, &val2));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val1);
  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val2);
}

TEST(CO_Val, CoValCmp_ArrayType_SecondValNull) {
  co_visible_string_t val1 = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(strlen(TEST_STR),
              co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val1, TEST_STR, 0));
  co_visible_string_t val2 = nullptr;
  CHECK_EQUAL(0, co_val_init(CO_DEFTYPE_VISIBLE_STRING, &val2));

  CHECK_EQUAL(1, co_val_cmp(CO_DEFTYPE_VISIBLE_STRING, &val1, &val2));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val1);
  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val2);
}

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Val, CoValCmp_##a) { \
    co_##b##_t val1; \
    co_##b##_t val2; \
    CHECK_EQUAL(0, co_val_init_min(CO_DEFTYPE_##a, &val1)); \
    CHECK_EQUAL(0, co_val_init_max(CO_DEFTYPE_##a, &val2)); \
\
    const auto ret = co_val_cmp(CO_DEFTYPE_##a, &val1, &val2); \
\
    CHECK_COMPARE(ret, <, 0); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Val, CoValCmp_##a) { \
    co_##b##_t val1; \
    co_##b##_t val2; \
    CHECK_EQUAL(0, co_val_init_min(CO_DEFTYPE_##a, &val1)); \
    CHECK_EQUAL(0, co_val_init_max(CO_DEFTYPE_##a, &val2)); \
\
    const auto ret = co_val_cmp(CO_DEFTYPE_##a, &val1, &val2); \
\
    CHECK_COMPARE(ret, <, 0); \
  } \
\
  TEST(CO_Val, CoValCmp_##a##_EqualMs) { \
    co_##b##_t val1; \
    co_##b##_t val2; \
    CHECK_EQUAL(0, co_val_init_min(CO_DEFTYPE_##a, &val1)); \
    CHECK_EQUAL(0, co_val_init_max(CO_DEFTYPE_##a, &val2)); \
    val1.ms = val2.ms; \
\
    const auto ret = co_val_cmp(CO_DEFTYPE_##a, &val1, &val2); \
\
    CHECK_COMPARE(ret, <, 0); \
  }
#include <lely/co/def/time.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE

TEST(CO_Val, CoValCmp_VISIBLE_STRING) {
  const char TEST_STR2[] = "abcdefg";

  co_visible_string_t val1 = arrays.Init<co_visible_string_t>();
  co_visible_string_t val2 = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(strlen(TEST_STR),
              co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val1, TEST_STR, 0));
  CHECK_EQUAL(strlen(TEST_STR2),
              co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val2, TEST_STR2, 0));

  const auto ret = co_val_cmp(CO_DEFTYPE_VISIBLE_STRING, &val1, &val2);

  CHECK_COMPARE(ret, >, 0);

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val1);
  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val2);
}

TEST(CO_Val, CoValCmp_VISIBLE_STRING_Substr) {
  co_visible_string_t val1 = arrays.Init<co_visible_string_t>();
  co_visible_string_t val2 = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(strlen(TEST_STR),
              co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val1, TEST_STR, 0));
  CHECK_EQUAL(0, co_val_init_vs_n(&val2, TEST_STR, strlen(TEST_STR) - 5u));

  const auto ret = co_val_cmp(CO_DEFTYPE_VISIBLE_STRING, &val1, &val2);

  CHECK_COMPARE(ret, >, 0);

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val1);
  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val2);
}

TEST(CO_Val, CoValCmp_OCTET_STRING) {
  const size_t n1 = 5u;
  const size_t n2 = 3u;
  const uint_least8_t os1[n1] = {0xd3u, 0xe5u, 0x98u, 0xbau, 0x96u};
  const uint_least8_t os2[n2] = {0x56u, 0x02u, 0x2cu};
  co_octet_string_t val1 = arrays.Init<co_octet_string_t>();
  co_octet_string_t val2 = arrays.Init<co_octet_string_t>();
  CHECK_EQUAL(n1, co_val_make(CO_DEFTYPE_OCTET_STRING, &val1, os1, n1));
  CHECK_EQUAL(n2, co_val_make(CO_DEFTYPE_OCTET_STRING, &val2, os2, n2));

  const auto ret = co_val_cmp(CO_DEFTYPE_OCTET_STRING, &val1, &val2);

  CHECK_COMPARE(ret, >, 0);

  co_val_fini(CO_DEFTYPE_OCTET_STRING, &val1);
  co_val_fini(CO_DEFTYPE_OCTET_STRING, &val2);
}

TEST(CO_Val, CoValCmp_UNICODE_STRING) {
  const char16_t TEST_STR16_2[] = u"abcdefg";

  co_unicode_string_t val1 = arrays.Init<co_unicode_string_t>();
  co_unicode_string_t val2 = arrays.Init<co_unicode_string_t>();
  CHECK_EQUAL(str16len(TEST_STR16),
              co_val_make(CO_DEFTYPE_UNICODE_STRING, &val1, TEST_STR16, 0));
  CHECK_EQUAL(str16len(TEST_STR16_2),
              co_val_make(CO_DEFTYPE_UNICODE_STRING, &val2, TEST_STR16_2, 0));

  const auto ret = co_val_cmp(CO_DEFTYPE_UNICODE_STRING, &val1, &val2);

  CHECK_COMPARE(ret, >, 0);

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val1);
  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val2);
}

TEST(CO_Val, CoValCmp_DOMAIN) {
  const size_t n1 = 4u;
  const size_t n2 = 2u;
  const unsigned char dom1[n1] = {0xd3u, 0xe5u, 0x98u, 0xbau};
  const unsigned char dom2[n2] = {0x24u, 0x30u};
  co_domain_t val1 = arrays.Init<co_domain_t>();
  co_domain_t val2 = arrays.Init<co_domain_t>();
  CHECK_EQUAL(n1, co_val_make(CO_DEFTYPE_DOMAIN, &val1, dom1, n1));
  CHECK_EQUAL(n2, co_val_make(CO_DEFTYPE_DOMAIN, &val2, dom2, n2));

  const auto ret = co_val_cmp(CO_DEFTYPE_DOMAIN, &val1, &val2);

  CHECK_COMPARE(ret, >, 0);

  co_val_fini(CO_DEFTYPE_DOMAIN, &val1);
  co_val_fini(CO_DEFTYPE_DOMAIN, &val2);
}

TEST(CO_Val, CoValCmp_INVALID_TYPE) {
  int val1 = 0;
  int val2 = 0;

  CHECK_EQUAL(0, co_val_cmp(INVALID_TYPE, &val1, &val2));
}

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Val, CoValRead_##a) { \
    co_##b##_t val; \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##a); \
    const uint_least8_t buffer[MAX_VAL_SIZE] = {0x3eu, 0x18u, 0x67u, 0x7bu, \
                                                0x34u, 0x15u, 0x09u, 0x27u}; \
    CHECK_COMPARE(val_size, <=, MAX_VAL_SIZE); \
\
    const auto ret = \
        co_val_read(CO_DEFTYPE_##a, &val, buffer, buffer + MAX_VAL_SIZE); \
\
    CHECK_EQUAL(val_size, ret); \
    CHECK_EQUAL(ldle_##c(buffer), val); \
  } \
\
  TEST(CO_Val, CoValRead_##a##_Overflow) { \
    co_##b##_t val; \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##a); \
    const uint_least8_t buffer[MAX_VAL_SIZE] = {0xfau, 0x83u, 0xb1,  0xf0u, \
                                                0xaau, 0xc4u, 0x88u, 0xe7u}; \
    const auto ret = \
        co_val_read(CO_DEFTYPE_##a, &val, buffer, buffer + MAX_VAL_SIZE); \
\
    CHECK_EQUAL(val_size, ret); \
    CHECK_EQUAL(ldle_##c(buffer), val); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE

TEST(CO_Val, CoValRead_BOOLEAN_True) {
  co_boolean_t val;
  const uint_least8_t buffer[] = {0xffu};

  const auto ret = co_val_read(CO_DEFTYPE_BOOLEAN, &val, buffer, nullptr);

  CHECK_EQUAL(ValGetReadWriteSize(CO_DEFTYPE_BOOLEAN), ret);
  CHECK_EQUAL(0x01, val);
}

TEST(CO_Val, CoValRead_BOOLEAN_False) {
  co_boolean_t val;
  const uint_least8_t buffer[] = {0x00};

  const auto ret = co_val_read(CO_DEFTYPE_BOOLEAN, &val, buffer, nullptr);

  CHECK_EQUAL(ValGetReadWriteSize(CO_DEFTYPE_BOOLEAN), ret);
  CHECK_EQUAL(0x00, val);
}

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Val, CoValRead_##a) { \
    co_##b##_t val; \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##a); \
    const size_t type_size = co_type_sizeof(CO_DEFTYPE_##a); \
    const uint_least8_t buffer[MAX_VAL_SIZE] = {0x3eu, 0x18u, 0x67u, 0x7bu, \
                                                0x34u, 0x15u, 0x00u, 0x00u}; \
    CHECK_COMPARE(val_size, <=, MAX_VAL_SIZE); \
    CHECK_COMPARE(type_size, <=, MAX_VAL_SIZE); \
\
    const auto ret = \
        co_val_read(CO_DEFTYPE_##a, &val, buffer, buffer + type_size); \
\
    CHECK_EQUAL(val_size, ret); \
    CHECK_EQUAL(ldle_u32(buffer) & UINT32_C(0x0fffffff), val.ms); \
    CHECK_EQUAL(ldle_u16(buffer + 4), val.days); \
  }
#include <lely/co/def/time.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Val, CoValRead_##a##_InvalidSize) { \
    co_##b##_t val; \
    const uint_least8_t buffer = 0x00; \
\
    const auto ret = co_val_read(CO_DEFTYPE_##a, &val, &buffer, &buffer); \
\
    CHECK_EQUAL(0, ret); \
  } \
\
  TEST(CO_Val, CoValRead_##a##_NullVal) { \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##a); \
    const uint_least8_t buffer[MAX_VAL_SIZE] = {0x00}; \
\
    const auto ret = \
        co_val_read(CO_DEFTYPE_##a, nullptr, buffer, buffer + MAX_VAL_SIZE); \
\
    CHECK_EQUAL(val_size, ret); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#include <lely/co/def/time.def>   // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Val, CoValRead_##a##_NullVal) { \
    const size_t buf_size = 2u; \
    const uint_least8_t buffer[buf_size] = {0x00}; \
\
    const auto ret = \
        co_val_read(CO_DEFTYPE_##a, nullptr, buffer, buffer + buf_size); \
\
    CHECK_EQUAL(buf_size, ret); \
  } \
\
  TEST(CO_Val, CoValRead_##a##_ZeroBuffer) { \
    co_##b##_t val = nullptr; \
\
    const auto ret = co_val_read(CO_DEFTYPE_##a, &val, nullptr, nullptr); \
\
    CHECK_EQUAL(0, ret); \
\
    co_val_fini(CO_DEFTYPE_##a, &val); \
  }
#include <lely/co/def/array.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE

#if LELY_NO_MALLOC
#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Val, CoValRead_##a##_Overflow) { \
    co_##b##_t val = arrays.Init<co_##b##_t>(); \
    const uint_least8_t buffer[CO_ARRAY_CAPACITY + 1] = {0}; \
\
    const auto ret = \
        co_val_read(CO_DEFTYPE_##a, &val, buffer, buffer + sizeof(buffer)); \
\
    CHECK_EQUAL(0, ret); \
  }
#include <lely/co/def/array.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
#endif  // LELY_NO_MALLOC

TEST(CO_Val, CoValRead_VISIBLE_STRING) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();
  const size_t ARRAY_SIZE = 6u;
  const uint_least8_t buffer[ARRAY_SIZE] = {0x74u, 0x64u, 0x73u,
                                            0x74u, 0x31u, 0x21u};
  const auto ret =
      co_val_read(CO_DEFTYPE_VISIBLE_STRING, &val, buffer, buffer + ARRAY_SIZE);

  CHECK_EQUAL(ARRAY_SIZE, ret);
  for (size_t i = 0; i < ARRAY_SIZE; ++i) CHECK_EQUAL(buffer[i], val[i]);
  CHECK_EQUAL('\0', val[ARRAY_SIZE]);

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

TEST(CO_Val, CoValRead_OCTET_STRING) {
  co_octet_string_t val = arrays.Init<co_octet_string_t>();
  const size_t ARRAY_SIZE = 5u;
  const uint_least8_t buffer[ARRAY_SIZE] = {0xd3u, 0xe5u, 0x98u, 0xbau, 0x96u};

  const auto ret =
      co_val_read(CO_DEFTYPE_OCTET_STRING, &val, buffer, buffer + ARRAY_SIZE);

  CHECK_EQUAL(ARRAY_SIZE, ret);
  for (size_t i = 0; i < ARRAY_SIZE; ++i) CHECK_EQUAL(buffer[i], val[i]);
  CHECK_EQUAL(0, val[ARRAY_SIZE]);

  co_val_fini(CO_DEFTYPE_OCTET_STRING, &val);
}

TEST(CO_Val, CoValRead_UNICODE_STRING) {
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();
  const size_t ARRAY_SIZE = 6u;
  const uint_least8_t buffer[ARRAY_SIZE] = {0x74u, 0x64u, 0x73u,
                                            0x74u, 0x31u, 0x21u};
  const auto ret =
      co_val_read(CO_DEFTYPE_UNICODE_STRING, &val, buffer, buffer + ARRAY_SIZE);

  CHECK_EQUAL(ARRAY_SIZE, ret);
  size_t i = 0;
  for (size_t j = 0; i < ARRAY_SIZE / 2; ++i, j += 2)
    CHECK_EQUAL(ldle_u16(buffer + j), val[i]);
  CHECK_EQUAL(u'\0', val[i]);

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val);
}

TEST(CO_Val, CoValRead_DOMAIN) {
  co_domain_t val = arrays.Init<co_domain_t>();
  const size_t ARRAY_SIZE = 4u;
  const uint_least8_t buffer[ARRAY_SIZE] = {0xd3u, 0xe5u, 0x98u, 0xbau};

  const auto ret =
      co_val_read(CO_DEFTYPE_DOMAIN, &val, buffer, buffer + ARRAY_SIZE);

  CHECK_EQUAL(ARRAY_SIZE, ret);
  const auto* vbuf = static_cast<uint_least8_t*>(val);
  for (size_t i = 0; i < ARRAY_SIZE; ++i) CHECK_EQUAL(buffer[i], vbuf[i]);

  co_val_fini(CO_DEFTYPE_DOMAIN, &val);
}

TEST(CO_Val, CoValRead_INVALID_TYPE) {
  const uint_least8_t buffer = 0x00;

  const auto ret = co_val_read(INVALID_TYPE, nullptr, &buffer, &buffer);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

TEST(CO_Val, CoValReadSdo) {
  co_unsigned16_t val = 0xbeefu;
  const uint8_t buffer[] = {0xaau, 0xbbu};
  set_errnum(0);

  const auto ret =
      co_val_read_sdo(CO_DEFTYPE_UNSIGNED16, &val, buffer, sizeof(buffer));

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, get_errnum());
  CHECK_EQUAL(0xbbaau, val);
}

TEST(CO_Val, CoValReadSdo_FromNull) {
  co_unsigned16_t val = 0xbeefu;
  set_errnum(0);

  const auto ret = co_val_read_sdo(CO_DEFTYPE_UNSIGNED16, &val, nullptr, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, get_errnum());
  CHECK_EQUAL(0xbeefu, val);
}

TEST(CO_Val, CoValReadSdo_FromTooSmall) {
  const uint8_t buffer[] = {0xaau};
  co_unsigned16_t val = 0xbeefu;
  set_errnum(0);

  const auto ret =
      co_val_read_sdo(CO_DEFTYPE_UNSIGNED16, &val, buffer, sizeof(buffer));

  CHECK_EQUAL(CO_SDO_AC_ERROR, ret);
  CHECK_EQUAL(0, get_errnum());
  CHECK_EQUAL(0xbeefu, val);
}

#if LELY_NO_MALLOC
TEST(CO_Val, CoValReadSdo_ToTooSmall) {
  const char buffer[] = "too long string";
  co_array array = CO_ARRAY_INIT;
  array.hdr.capacity = 1;
  co_visible_string_t val;
  co_val_init_array(&val, &array);
  set_errnum(42);

  const auto ret =
      co_val_read_sdo(CO_DEFTYPE_VISIBLE_STRING, &val, buffer, sizeof(buffer));

  CHECK_EQUAL(CO_SDO_AC_NO_MEM, ret);
  CHECK_EQUAL(42, get_errnum());
}
#endif

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Val, CoValWrite_##a) { \
    co_##b##_t val; \
    CHECK_EQUAL(0, co_val_init_max(CO_DEFTYPE_##a, &val)); \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##a); \
    CHECK_COMPARE(val_size, <=, MAX_VAL_SIZE); \
    uint_least8_t buffer[MAX_VAL_SIZE] = {0x00}; \
\
    const auto ret = \
        co_val_write(CO_DEFTYPE_##a, &val, buffer, buffer + MAX_VAL_SIZE); \
\
    CHECK_EQUAL(val_size, ret); \
    uint_least8_t vbuf[MAX_VAL_SIZE] = {0x00}; \
    stle_##c(vbuf, val); \
    for (size_t i = 0; i < MAX_VAL_SIZE; ++i) CHECK_EQUAL(vbuf[i], buffer[i]); \
  } \
\
  TEST(CO_Val, CoValWrite_##a##_NoEnd) { \
    co_##b##_t val; \
    CHECK_EQUAL(0, co_val_init_min(CO_DEFTYPE_##a, &val)); \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##a); \
    CHECK_COMPARE(val_size, <=, MAX_VAL_SIZE); \
    uint_least8_t buffer[MAX_VAL_SIZE] = {0x00}; \
\
    const auto ret = co_val_write(CO_DEFTYPE_##a, &val, buffer, nullptr); \
\
    CHECK_EQUAL(val_size, ret); \
    uint_least8_t vbuf[MAX_VAL_SIZE] = {0x00}; \
    stle_##c(vbuf, val); \
    for (size_t i = 0; i < MAX_VAL_SIZE; ++i) CHECK_EQUAL(vbuf[i], buffer[i]); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE

TEST(CO_Val, CoValWrite_BOOLEAN_True) {
  co_boolean_t val;
  const uint_least8_t ptr[] = {0xffu};
  co_val_make(CO_DEFTYPE_BOOLEAN, &val, ptr, 1);
  uint_least8_t buffer[] = {0x00};

  const auto ret = co_val_write(CO_DEFTYPE_BOOLEAN, &val, buffer, nullptr);

  CHECK_EQUAL(ValGetReadWriteSize(CO_DEFTYPE_BOOLEAN), ret);
  CHECK_EQUAL(0x01u, *buffer);
}

TEST(CO_Val, CoValWrite_BOOLEAN_False) {
  co_boolean_t val;
  const uint_least8_t ptr[] = {0x00};
  co_val_make(CO_DEFTYPE_BOOLEAN, &val, ptr, 1);
  uint_least8_t buffer[] = {0xffu};

  const auto ret = co_val_write(CO_DEFTYPE_BOOLEAN, &val, buffer, nullptr);

  CHECK_EQUAL(ValGetReadWriteSize(CO_DEFTYPE_BOOLEAN), ret);
  CHECK_EQUAL(0x00, *buffer);
}

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Val, CoValWrite_##a) { \
    co_##b##_t val; \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##a); \
    uint_least8_t buffer[MAX_VAL_SIZE] = {0x00}; \
    CHECK_EQUAL(0, co_val_init(CO_DEFTYPE_##a, &val)); \
    val.ms = 0x0b67183eu; \
    val.days = 0x1534u; \
\
    CHECK_COMPARE(val_size, <=, MAX_VAL_SIZE); \
\
    const auto ret = \
        co_val_write(CO_DEFTYPE_##a, &val, buffer, buffer + MAX_VAL_SIZE); \
\
    CHECK_EQUAL(val_size, ret); \
    CHECK_EQUAL(val.ms, ldle_u32(buffer) & UINT32_C(0x0fffffff)); \
    CHECK_EQUAL(val.days, ldle_u16(buffer + 4)); \
  } \
\
  TEST(CO_Val, CoValWrite_##a##_NoEnd) { \
    co_##b##_t val; \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##a); \
    uint_least8_t buffer[MAX_VAL_SIZE] = {0x00}; \
    CHECK_EQUAL(0, co_val_init(CO_DEFTYPE_##a, &val)); \
    val.ms = 0x0b67183eu; \
    val.days = 0x1534u; \
\
    CHECK_COMPARE(val_size, <=, MAX_VAL_SIZE); \
\
    const auto ret = co_val_write(CO_DEFTYPE_##a, &val, buffer, nullptr); \
\
    CHECK_EQUAL(val_size, ret); \
    CHECK_EQUAL(val.ms, ldle_u32(buffer) & UINT32_C(0x0fffffff)); \
    CHECK_EQUAL(val.days, ldle_u16(buffer + 4)); \
  }
#include <lely/co/def/time.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  TEST(CO_Val, CoValWrite_##a##_NullBuffer) { \
    co_##b##_t val; \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##a); \
\
    const auto ret = co_val_write(CO_DEFTYPE_##a, &val, nullptr, nullptr); \
\
    CHECK_EQUAL(val_size, ret); \
  } \
\
  TEST(CO_Val, CoValWrite_##a##_InvalidSize) { \
    co_##b##_t val; \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##a); \
    uint_least8_t buffer[MAX_VAL_SIZE] = {0x00}; \
\
    const auto ret = co_val_write(CO_DEFTYPE_##a, &val, buffer, buffer); \
\
    CHECK_EQUAL(val_size, ret); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#include <lely/co/def/time.def>   // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE

TEST(CO_Val, CoValWrite_NullArray) {
  const size_t ARRAY_SIZE = 5u;
  co_visible_string_t val = nullptr;
  uint_least8_t buffer[ARRAY_SIZE] = {0x00};

  const auto ret = co_val_write(CO_DEFTYPE_VISIBLE_STRING, &val, buffer,
                                buffer + ARRAY_SIZE);

  CHECK_EQUAL(0, ret);
}

TEST(CO_Val, CoValWrite_VISIBLE_STRING) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();
  const size_t ARRAY_SIZE = 5u;
  const char test_str[ARRAY_SIZE + 1] = "abcde";
  CHECK_EQUAL(0, co_val_init_vs(&val, test_str));
  uint_least8_t buffer[ARRAY_SIZE] = {0x00};

  const auto ret = co_val_write(CO_DEFTYPE_VISIBLE_STRING, &val, buffer,
                                buffer + ARRAY_SIZE);

  CHECK_EQUAL(ARRAY_SIZE, ret);
  for (size_t i = 0; i < ARRAY_SIZE; ++i) CHECK_EQUAL(buffer[i], test_str[i]);

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

TEST(CO_Val, CoValWrite_OCTET_STRING) {
  co_octet_string_t val = arrays.Init<co_octet_string_t>();
  const size_t ARRAY_SIZE = 5u;
  const uint_least8_t test_str[ARRAY_SIZE] = {0xd3u, 0xe5u, 0x98u, 0xbau,
                                              0x96u};
  CHECK_EQUAL(0, co_val_init_os(&val, test_str, ARRAY_SIZE));
  uint_least8_t buffer[ARRAY_SIZE] = {0x00};

  const auto ret =
      co_val_write(CO_DEFTYPE_OCTET_STRING, &val, buffer, buffer + ARRAY_SIZE);

  CHECK_EQUAL(ARRAY_SIZE, ret);
  for (size_t i = 0; i < ARRAY_SIZE; ++i) CHECK_EQUAL(buffer[i], test_str[i]);

  co_val_fini(CO_DEFTYPE_OCTET_STRING, &val);
}

TEST(CO_Val, CoValWrite_UNICODE_STRING) {
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();
  const size_t ARRAY_SIZE = 6u;
  const char16_t test_str[ARRAY_SIZE / 2 + 1] = u"xyz";
  CHECK_EQUAL(0, co_val_init_us(&val, test_str));
  uint_least8_t buffer[ARRAY_SIZE] = {0x00};

  const auto ret = co_val_write(CO_DEFTYPE_UNICODE_STRING, &val, buffer,
                                buffer + ARRAY_SIZE);

  CHECK_EQUAL(ARRAY_SIZE, ret);
  for (size_t i = 0, j = 0; i < ARRAY_SIZE; i += 2, ++j)
    CHECK_EQUAL(ldle_u16(buffer + i), test_str[j]);

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val);
}

TEST(CO_Val, CoValWrite_DOMAIN) {
  co_domain_t val = arrays.Init<co_domain_t>();
  const size_t ARRAY_SIZE = 4u;
  const uint_least8_t test_buf[ARRAY_SIZE] = {0xd3u, 0xe5u, 0x98u, 0xbau};
  CHECK_EQUAL(0, co_val_init_dom(&val, test_buf, ARRAY_SIZE));
  uint_least8_t buffer[ARRAY_SIZE] = {0x00};

  const auto ret =
      co_val_write(CO_DEFTYPE_DOMAIN, &val, buffer, buffer + ARRAY_SIZE);

  CHECK_EQUAL(ARRAY_SIZE, ret);
  for (size_t i = 0; i < ARRAY_SIZE; ++i) CHECK_EQUAL(buffer[i], test_buf[i]);

  co_val_fini(CO_DEFTYPE_DOMAIN, &val);
}

TEST(CO_Val, CoValWrite_VISIBLE_STRING_NoEnd) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();
  const size_t ARRAY_SIZE = 7u;
  const char test_str[ARRAY_SIZE + 1] = "qwerty7";
  CHECK_EQUAL(0, co_val_init_vs(&val, test_str));
  uint_least8_t buffer[ARRAY_SIZE] = {0x00};

  const auto ret =
      co_val_write(CO_DEFTYPE_VISIBLE_STRING, &val, buffer, nullptr);

  CHECK_EQUAL(ARRAY_SIZE, ret);
  for (size_t i = 0; i < ARRAY_SIZE; ++i) CHECK_EQUAL(buffer[i], test_str[i]);

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

TEST(CO_Val, CoValWrite_OCTET_STRING_NullBuffer) {
  co_octet_string_t val = arrays.Init<co_octet_string_t>();
  const size_t ARRAY_SIZE = 5u;
  const uint_least8_t test_str[ARRAY_SIZE] = {0xd3u, 0xe5u, 0x98u, 0xbau,
                                              0x96u};
  CHECK_EQUAL(0, co_val_init_os(&val, test_str, ARRAY_SIZE));

  const auto ret =
      co_val_write(CO_DEFTYPE_OCTET_STRING, &val, nullptr, nullptr);

  CHECK_EQUAL(ARRAY_SIZE, ret);

  co_val_fini(CO_DEFTYPE_OCTET_STRING, &val);
}

TEST(CO_Val, CoValWrite_UNICODE_STRING_TooSmallBuffer) {
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();
  const size_t ARRAY_SIZE = 6u;
  const char16_t test_str[ARRAY_SIZE / 2 + 1] = u"xyz";
  CHECK_EQUAL(0, co_val_init_us(&val, test_str));
  uint_least8_t buffer[ARRAY_SIZE - 1] = {0x00};

  const auto ret = co_val_write(CO_DEFTYPE_UNICODE_STRING, &val, buffer,
                                buffer + ARRAY_SIZE - 1);

  CHECK_EQUAL(ARRAY_SIZE, ret);

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val);
}

TEST(CO_Val, CoValWrite_DOMAIN_SizeZero) {
  co_domain_t val = arrays.Init<co_domain_t>();
  const size_t ARRAY_SIZE = 4u;
  const uint_least8_t test_buf[ARRAY_SIZE] = {0xd3u, 0xe5u, 0x98u, 0xbau};
  CHECK_EQUAL(0, co_val_init_dom(&val, test_buf, ARRAY_SIZE));

  const size_t co_array_offset =
      ALIGN(sizeof(co_array_hdr), _Alignof(union co_val));
  const auto hdr = reinterpret_cast<co_array_hdr*>(
      reinterpret_cast<char*>(val) - co_array_offset);
  hdr->size = 0;

  const auto ret = co_val_write(CO_DEFTYPE_DOMAIN, &val, nullptr, nullptr);

  CHECK_EQUAL(0, ret);

  co_val_fini(CO_DEFTYPE_DOMAIN, &val);
}

TEST(CO_Val, CoValWrite_INVALID_TYPE) {
  co_integer16_t val;
  uint_least8_t buffer[MAX_VAL_SIZE] = {0x00};

  const auto ret = co_val_write(INVALID_TYPE, &val, buffer, buffer);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

#if LELY_NO_MALLOC
TEST(CO_Val, CoValInitArray) {
  co_array array = CO_ARRAY_INIT;
  co_visible_string_t val;

  co_val_init_array(&val, &array);

  POINTERS_EQUAL(array.u.data, val);
}

TEST(CO_Val, CoValInitArray_NullValue) {
  co_array array = CO_ARRAY_INIT;
  co_visible_string_t val = nullptr;

  co_val_init_array(nullptr, &array);

  POINTERS_EQUAL(nullptr, val);
}

TEST(CO_Val, CoValInitArray_NullArray) {
  co_visible_string_t val;

  co_val_init_array(&val, nullptr);

  POINTERS_EQUAL(nullptr, val);
}
#endif  // LELY_NO_MALLOC
