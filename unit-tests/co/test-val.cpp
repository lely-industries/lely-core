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
#include <string>

#include <CppUTest/TestHarness.h>

#include <lely/co/val.h>
#include <lely/co/type.h>
#include <lely/co/sdo.h>
#include <lely/util/endian.h>
#include <lely/util/error.h>
#include <lely/util/ustring.h>

#include "holder/array-init.hpp"

TEST_GROUP(CO_Val) {
  static const co_unsigned16_t INVALID_TYPE = 0xffffu;
  static const size_t CO_MAX_VAL_SIZE = 8u;

#if CO_DEFTYPE_TIME_SCET
  static const size_t MAX_VAL_SIZE_SCET = 8u;
#else
  static const size_t MAX_VAL_SIZE_SCET = 0;
#endif

#if CO_DEFTYPE_TIME_SUTC
  static const size_t MAX_VAL_SIZE_SUTC = 12u;
#else
  static const size_t MAX_VAL_SIZE_SUTC = 0;
#endif

  static const size_t MAX_VAL_SIZE =
      MAX(CO_MAX_VAL_SIZE, MAX(MAX_VAL_SIZE_SCET, MAX_VAL_SIZE_SUTC));

  const std::string TEST_STR = "testtesttest";
  const std::u16string TEST_STR16 = u"testtesttest";

  CoArrays arrays;

  TEST_TEARDOWN() {
    set_errnum(0);
    arrays.Clear();
  }

  // Returns the read/write size (in bytes) of the value of the specified type.
  // In most cases function returns the same value as co_type_sizeof(), but for
  // a few integer/unsigned types it is different than the size in the memory.
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
#if CO_DEFTYPE_TIME_SCET
      case CO_DEFTYPE_TIME_SCET:
        return 7u;
#endif
#if CO_DEFTYPE_TIME_SUTC
      case CO_DEFTYPE_TIME_SUTC:
        return 8u;
#endif
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
      return -(static_cast<int32_t>(CO_UNSIGNED24_MAX) + 1 -
               static_cast<int32_t>(u24));
    else
      return u24;
  }
  static co_integer40_t ldle_i40(const uint_least8_t src[8]) {
    co_unsigned40_t u40 = ldle_u40(src);
    if (u40 > CO_INTEGER40_MAX)
      return -(static_cast<int64_t>(CO_UNSIGNED40_MAX) + 1 -
               static_cast<int64_t>(u40));
    else
      return u40;
  }
  static co_integer48_t ldle_i48(const uint_least8_t src[8]) {
    co_unsigned48_t u48 = ldle_u48(src);
    if (u48 > CO_INTEGER48_MAX)
      return -(static_cast<int64_t>(CO_UNSIGNED48_MAX) + 1 -
               static_cast<int64_t>(u48));
    else
      return u48;
  }
  static co_integer56_t ldle_i56(const uint_least8_t src[8]) {
    co_unsigned56_t u56 = ldle_u56(src);
    if (u56 > CO_INTEGER56_MAX)
      return -(static_cast<int64_t>(CO_UNSIGNED56_MAX) + 1 -
               static_cast<int64_t>(u56));
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

/// @name co_val_init()
///@{

/// \Given an uninitialized value of a basic data type
///
/// \When co_val_init() is called with the basic data type and a pointer to
///       the value
///
/// \Then 0 is returned and the value is initialized and zeroed
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
TEST(CO_Val, CoValInit_BasicTypes) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val; \
    POINTERS_EQUAL(&val, memset(&val, 0xff, sizeof(val))); \
\
    const auto ret = co_val_init(CO_DEFTYPE_##NAME, &val); \
\
    CHECK_EQUAL_TEXT(0, ret, "type: " #NAME); \
    const char testbuf[sizeof(co_##name##_t)] = {0}; \
    CHECK_EQUAL_TEXT(0, memcmp(testbuf, &val, sizeof(val)), "type: " #NAME); \
  }
#include <lely/co/def/basic.def>
#undef LELY_CO_DEFINE_TYPE
}

/// \Given an uninitialized value of an array data type
///
/// \When co_val_init() is called with the array data type and a pointer to
///       the value
///
/// \Then 0 is returned and the value is initialized to an empty array
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
///       \IfCalls{LELY_NO_MALLOC, co_array_init()}
TEST(CO_Val, CoValInit_ArrayTypes) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val = arrays.DeadBeef<co_##name##_t>(); \
\
    const auto ret = co_val_init(CO_DEFTYPE_##NAME, &val); \
\
    CHECK_EQUAL_TEXT(0, ret, "type: " #NAME); \
    CHECK_EQUAL_TEXT(0, co_val_sizeof(CO_DEFTYPE_##NAME, &val), \
                     "type: " #NAME); \
    CHECK_TEXT(arrays.IsEmptyInitialized(val), "type: " #NAME); \
  }
#include <lely/co/def/array.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given an uninitialized value of a time data type
///
/// \When co_val_init() is called with the time data type and a pointer to
///       the value
///
/// \Then 0 is returned and all members of the value are initialized and zeroed
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
TEST(CO_Val, CoValInit_TimeTypes) {
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val; \
    POINTERS_EQUAL(&val, memset(&val, 0xff, sizeof(val))); \
\
    const auto ret = co_val_init(CO_DEFTYPE_##NAME, &val); \
\
    CHECK_EQUAL_TEXT(0, ret, "type: " #NAME); \
    CHECK_EQUAL_TEXT(0, val.days, "type: " #NAME); \
    CHECK_EQUAL_TEXT(0, val.ms, "type: " #NAME); \
  }
#include <lely/co/def/time.def>
#undef LELY_CO_DEFINE_TYPE
}

#if CO_DEFTYPE_TIME_SCET
/// \Given an uninitialized value of the SCET (Spacecraft Elapsed Time) data
///        type
///
/// \When co_val_init() is called with the SCET data type and a pointer to
///       the value
///
/// \Then 0 is returned and all members of the value are initialized and zeroed
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
TEST(CO_Val, CoValInit_TIME_SCET) {
  co_time_scet_t val;
  POINTERS_EQUAL(&val, memset(&val, 0xff, sizeof(val)));

  const auto ret = co_val_init(CO_DEFTYPE_TIME_SCET, &val);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, val.seconds);
  CHECK_EQUAL(0, val.subseconds);
}
#endif

#if CO_DEFTYPE_TIME_SUTC
/// \Given an uninitialized value of the SUTC (Spacecraft Universal Time
///        Coordinated) data type
///
/// \When co_val_init() is called with the SUTC data type and a pointer to
///       the value
///
/// \Then 0 is returned and all members of the value are initialized and zeroed
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
TEST(CO_Val, CoValInit_TIME_SUTC) {
  co_time_sutc_t val;
  POINTERS_EQUAL(&val, memset(&val, 0xff, sizeof(val)));

  const auto ret = co_val_init(CO_DEFTYPE_TIME_SUTC, &val);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, val.days);
  CHECK_EQUAL(0, val.ms);
  CHECK_EQUAL(0, val.usec);
}
#endif

/// \Given an uninitialized value of any data type
///
/// \When co_val_init() is called with an invalid data type and a pointer to
///       the value
///
/// \Then -1 is returned, the error number is set to ERRNUM_INVAL, the value
///       is not changed
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
///       \Calls set_errnum()
TEST(CO_Val, CoValInit_Invalid) {
  unsigned char val;
  POINTERS_EQUAL(&val, memset(&val, 0xff, sizeof(val)));

  const auto ret = co_val_init(INVALID_TYPE, &val);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0xffu, val);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

///@}

/// @name co_val_init_min()
///@{

/// \Given an uninitialized value of a basic data type
///
/// \When co_val_init_min() is called with the basic data type and a pointer
///       to the value
///
/// \Then 0 is returned and the value is initialized with the lower limit of
///       the data type
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
TEST(CO_Val, CoValInitMin_BasicTypes) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val; \
    POINTERS_EQUAL(&val, memset(&val, 0xff, sizeof(val))); \
\
    const auto ret = co_val_init_min(CO_DEFTYPE_##NAME, &val); \
\
    CHECK_EQUAL_TEXT(0, ret, "type: " #NAME); \
    CHECK_EQUAL_TEXT(CO_##NAME##_MIN, val, "type: " #NAME); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given an uninitialized value of an array data type
///
/// \When co_val_init_min() is called with the array data type and a pointer
///       to the value
///
/// \Then 0 is returned and the value is initialized to an empty array
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
///       \IfCalls{LELY_NO_MALLOC, co_array_init()}
TEST(CO_Val, CoValInitMin_ArrayTypes) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val = arrays.DeadBeef<co_##name##_t>(); \
\
    const auto ret = co_val_init_min(CO_DEFTYPE_##NAME, &val); \
\
    CHECK_EQUAL_TEXT(0, ret, "type: " #NAME); \
    CHECK_EQUAL_TEXT(0, co_val_sizeof(CO_DEFTYPE_##NAME, &val), \
                     "type: " #NAME); \
    CHECK_TEXT(arrays.IsEmptyInitialized(val), "type: " #NAME); \
  }
#include <lely/co/def/array.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given an uninitialized value of a time data type
///
/// \When co_val_init_min() is called with the time data type and a pointer
///       to the value
///
/// \Then 0 is returned and all value members are initialized with the lower
///       limits of their types
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
TEST(CO_Val, CoValInitMin_TimeTypes) {
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val; \
    POINTERS_EQUAL(&val, memset(&val, 0xff, sizeof(val))); \
\
    const auto ret = co_val_init_min(CO_DEFTYPE_##NAME, &val); \
\
    CHECK_EQUAL(0, ret); \
    CHECK_EQUAL(CO_UNSIGNED16_MIN, val.days); \
    CHECK_EQUAL(CO_UNSIGNED32_MIN, val.ms); \
  }
#include <lely/co/def/time.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

#if CO_DEFTYPE_TIME_SCET
/// \Given an uninitialized value of the SCET (Spacecraft Elapsed Time) data
///        type
///
/// \When co_val_init_min() is called with the SCET data type and a pointer
///       to the value
///
/// \Then 0 is returned and all value members are initialized with the lower
///       limits of their types
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
TEST(CO_Val, CoValInitMin_TIME_SCET) {
  co_time_scet_t val;
  POINTERS_EQUAL(&val, memset(&val, 0xff, sizeof(val)));

  const auto ret = co_val_init_min(CO_DEFTYPE_TIME_SCET, &val);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(CO_UNSIGNED32_MIN, val.seconds);
  CHECK_EQUAL(CO_UNSIGNED24_MIN, val.subseconds);
}
#endif

#if CO_DEFTYPE_TIME_SUTC
/// \Given an uninitialized value of the SUTC (Spacecraft Universal Time
///        Coordinated) data type
///
/// \When co_val_init_min() is called with the SUTC data type and a pointer
///       to the value
///
/// \Then 0 is returned and all value members are initialized with the lower
///       limits of their types
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
TEST(CO_Val, CoValInitMin_TIME_SUTC) {
  co_time_sutc_t val;
  POINTERS_EQUAL(&val, memset(&val, 0xff, sizeof(val)));

  const auto ret = co_val_init_min(CO_DEFTYPE_TIME_SUTC, &val);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(CO_UNSIGNED16_MIN, val.days);
  CHECK_EQUAL(CO_UNSIGNED32_MIN, val.ms);
  CHECK_EQUAL(CO_UNSIGNED16_MIN, val.usec);
}
#endif

/// \Given an uninitialized value of any data type
///
/// \When co_val_init_min() is called with an invalid data type and a pointer
///       to the value
///
/// \Then -1 is returned, the error number is set to ERRNUM_INVAL, the value
///       is not changed
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
///       \Calls set_errnum()
TEST(CO_Val, CoValInitMin_Invalid) {
  unsigned char val;
  POINTERS_EQUAL(&val, memset(&val, 0xff, sizeof(val)));

  const auto ret = co_val_init_min(INVALID_TYPE, &val);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0xffu, val);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

///@}

/// @name co_val_init_max()
///@{

/// \Given an uninitialized value of a basic data type
///
/// \When co_val_init_max() is called with the basic data type and a pointer
///       to the value
///
/// \Then 0 is returned and the value is initialized with the upper limit of
///       the data type
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
TEST(CO_Val, CoValInitMax_BasicTypes) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val; \
    POINTERS_EQUAL(&val, memset(&val, 0xff, sizeof(val))); \
\
    const auto ret = co_val_init_max(CO_DEFTYPE_##NAME, &val); \
\
    CHECK_EQUAL_TEXT(0, ret, "type: " #NAME); \
    CHECK_EQUAL_TEXT(CO_##NAME##_MAX, val, "type: " #NAME); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given an uninitialized value of an array data type
///
/// \When co_val_init_max() is called with the array data type and a pointer
///       to the value
///
/// \Then 0 is returned and the value is initialized to an empty array
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
///       \IfCalls{LELY_NO_MALLOC, co_array_init()}

TEST(CO_Val, CoValInitMax_ArrayTypes) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val = arrays.DeadBeef<co_##name##_t>(); \
\
    const auto ret = co_val_init_max(CO_DEFTYPE_##NAME, &val); \
\
    CHECK_EQUAL_TEXT(0, ret, "type: " #NAME); \
    CHECK_EQUAL_TEXT(0, co_val_sizeof(CO_DEFTYPE_##NAME, &val), \
                     "type: " #NAME); \
    CHECK_TEXT(arrays.IsEmptyInitialized(val), "type: " #NAME); \
  }
#include <lely/co/def/array.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given an uninitialized value of a time data type
///
/// \When co_val_init_max() is called with the time data type and a pointer
///       to the value
///
/// \Then 0 is returned and all value members are initialized with the upper
///       limits of their types
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}

TEST(CO_Val, CoValInitMax_TimeTypes) {
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val; \
    POINTERS_EQUAL(&val, memset(&val, 0xff, sizeof(val))); \
\
    const auto ret = co_val_init_max(CO_DEFTYPE_##NAME, &val); \
\
    CHECK_EQUAL_TEXT(0, ret, "type: " #NAME); \
    CHECK_EQUAL_TEXT(UINT16_MAX, val.days, "type: " #NAME); \
    CHECK_EQUAL_TEXT(UINT32_C(0x0fffffff), val.ms, "type: " #NAME); \
  }
#include <lely/co/def/time.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

#if CO_DEFTYPE_TIME_SCET
/// \Given an uninitialized value of the SCET (Spacecraft Elapsed Time) data
///        type
///
/// \When co_val_init_max() is called with the SCET data type and a pointer
///       to the value
///
/// \Then 0 is returned and all value members are initialized with the upper
///       limits of their types
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
TEST(CO_Val, CoValInitMax_TIME_SCET) {
  co_time_scet_t val;
  POINTERS_EQUAL(&val, memset(&val, 0xff, sizeof(val)));

  const auto ret = co_val_init_max(CO_DEFTYPE_TIME_SCET, &val);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(UINT32_MAX, val.seconds);
  CHECK_EQUAL(UINT32_C(0x00ffffff), val.subseconds);
}
#endif

#if CO_DEFTYPE_TIME_SUTC
/// \Given an uninitialized value of the SUTC (Spacecraft Universal Time
///        Coordinated) data type
///
/// \When co_val_init_max() is called with the SUTC data type and a pointer
///       to the value
///
/// \Then 0 is returned and all value members are initialized with the upper
///       limits of their types
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
TEST(CO_Val, CoValInitMax_TIME_SUTC) {
  co_time_sutc_t val;
  POINTERS_EQUAL(&val, memset(&val, 0xff, sizeof(val)));

  const auto ret = co_val_init_max(CO_DEFTYPE_TIME_SUTC, &val);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(UINT16_MAX, val.days);
  CHECK_EQUAL(UINT32_MAX, val.ms);
  CHECK_EQUAL(UINT16_MAX, val.usec);
}
#endif

/// \Given an uninitialized value of any data type
///
/// \When co_val_init_max() is called with an invalid data type and a pointer
///       to the value
///
/// \Then -1 is returned, the error number is set to ERRNUM_INVAL, the value
///       is not changed
///       \IfCalls{LELY_NO_MALLOC, co_type_is_array()}
///       \Calls set_errnum()
TEST(CO_Val, CoValInitMax_Invalid) {
  unsigned char val;
  POINTERS_EQUAL(&val, memset(&val, 0xff, sizeof(val)));

  const auto ret = co_val_init_max(INVALID_TYPE, &val);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(0xffu, val);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

///@}

/// @name co_val_init_vs()
///@{

/// \Given an uninitialized array of visible characters (co_visible_string_t)
///
/// \When co_val_init_vs() is called with a pointer to the array and a pointer
///       to a null-terminated string
///
/// \Then 0 is returned and the array is initialized with the given string
///       \Calls co_val_init_vs_n()
TEST(CO_Val, CoValInitVs_Nominal) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();

  const auto ret = co_val_init_vs(&val, TEST_STR.c_str());

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(TEST_STR.size(), co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val));
  CHECK_EQUAL(0, memcmp(TEST_STR.c_str(), val, TEST_STR.size() + 1u));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

/// \Given an uninitialized array of visible characters (co_visible_string_t)
///
/// \When co_val_init_vs() is called with a pointer to the array and a null
///       string pointer
///
/// \Then 0 is returned and the array is initialized to an empty string
///       \Calls co_array_fini()
TEST(CO_Val, CoValInitVs_NullString) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();

  const auto ret = co_val_init_vs(&val, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val));
  CHECK(arrays.IsEmptyInitialized(val));
}

///@}

/// @name co_val_init_vs_n()
///@{

/// \Given an uninitialized array of visible characters (co_visible_string_t)
///
/// \When co_val_init_vs_n() is called with a pointer to the array, a pointer
///       to a string and the length of the string
///
/// \Then 0 is returned and the array is initialized with the given string
///       \Calls co_array_alloc()
///       \Calls co_array_init()
///       \Calls memcpy()
TEST(CO_Val, CoValInitVsN_Nominal) {
  const size_t n = 4u;
  co_visible_string_t val = arrays.Init<co_visible_string_t>();

  const auto ret = co_val_init_vs_n(&val, TEST_STR.c_str(), n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val));
  char testbuf[n + 1u] = {0};
  POINTERS_EQUAL(testbuf, memcpy(testbuf, TEST_STR.c_str(), n));
  CHECK_EQUAL(0, memcmp(testbuf, val, n + 1u));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

/// \Given an uninitialized array of visible characters (co_visible_string_t)
///
/// \When co_val_init_vs_n() is called with a pointer to the array, a null
///       string pointer and a non-zero length of a string
///
/// \Then 0 is returned and the array is initialized with a zeroed string of
///       the given length
///       \Calls co_array_alloc()
///       \Calls co_array_init()
TEST(CO_Val, CoValInitVsN_NullString) {
  const size_t n = 7u;
  co_visible_string_t val = arrays.Init<co_visible_string_t>();

  const auto ret = co_val_init_vs_n(&val, nullptr, n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val));
  const char testbuf[n + 1u] = {0};
  CHECK_EQUAL(0, memcmp(testbuf, val, n + 1u));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

#if LELY_NO_MALLOC
/// \Given an array of visible characters (co_visible_string_t) initialized
///        to a null pointer
///
/// \When co_val_init_vs_n() is called with a pointer to the array, a string
///       pointer and a non-zero length of a string
///
/// \Then -1 is returned and nothing is changed, the error number is set to
///       ERRNUM_NOMEM
///       \Calls co_array_alloc()
TEST(CO_Val, CoValInitVsN_NullVal) {
  co_visible_string_t val = nullptr;
  char buf[1u] = {0};

  const auto ret = co_val_init_vs_n(&val, buf, sizeof(buf));

  CHECK_EQUAL(-1, ret);
  POINTERS_EQUAL(nullptr, val);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}
#endif

/// \Given an uninitialized array of visible characters (co_visible_string_t)
///
/// \When co_val_init_vs_n() is called with a pointer to the array, a null
///       string pointer and a length of a string equal to `0`
///
/// \Then 0 is returned and nothing is changed
///       \Calls co_array_fini()
TEST(CO_Val, CoValInitVsN_ZeroLength) {
  co_visible_string_t val = nullptr;

  const auto ret = co_val_init_vs_n(&val, nullptr, 0);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, val);
}

#if LELY_NO_MALLOC
/// \Given an uninitialized array of visible characters (co_visible_string_t)
///
/// \When co_val_init_vs_n() is called with a pointer to the array, a pointer
///       to a string longer than `CO_ARRAY_CAPACITY` and the length of the
///       string
///
/// \Then -1 is returned, the error number is set to ERRNUM_NOMEM
///       \Calls co_array_alloc()
TEST(CO_Val, CoValInitVsN_TooBigString) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();
  const char buf[CO_ARRAY_CAPACITY + 1u] = {0};

  const auto ret = co_val_init_vs_n(&val, buf, sizeof(buf));

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}
#endif

///@}

/// @name co_val_init_os()
///@{

/// \Given an uninitialized array of octets (co_octet_string_t)
///
/// \When co_val_init_os() is called with a pointer to the array, a pointer
///       to an array of octets and the length of the array of octets
///
/// \Then 0 is returned and the array is initialized with the given array of
///       octets
///       \Calls co_array_alloc()
///       \Calls co_array_init()
///       \Calls memcpy()
TEST(CO_Val, CoValInitOs_Nominal) {
  const size_t n = 5u;
  const uint_least8_t os[n] = {0xd3u, 0xe5u, 0x98u, 0xbau, 0x96u};
  co_octet_string_t val = arrays.Init<co_octet_string_t>();

  const auto ret = co_val_init_os(&val, os, n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));
  CHECK_EQUAL(0, memcmp(os, val, n));

  co_val_fini(CO_DEFTYPE_OCTET_STRING, &val);
}

/// \Given an uninitialized array of octets (co_octet_string_t)
///
/// \When co_val_init_os() is called with a pointer to the array, a null
///       array of octets pointer and a non-zero length of the array of octets
///
/// \Then 0 is returned and the array is initialized with a zeroed array of
///       octets
///       \Calls co_array_alloc()
///       \Calls co_array_init()
TEST(CO_Val, CoValInitOs_NullArray) {
  const size_t n = 9u;
  co_octet_string_t val = arrays.Init<co_octet_string_t>();

  const auto ret = co_val_init_os(&val, nullptr, n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));
  const uint_least8_t testbuf[n] = {0x00};
  CHECK_EQUAL(0, memcmp(testbuf, val, n));

  co_val_fini(CO_DEFTYPE_OCTET_STRING, &val);
}

/// \Given an uninitialized array of octets (co_octet_string_t)
///
/// \When co_val_init_os() is called with a pointer to the array, a null
///       array of octets pointer and a length of the array of octets equal to
///       `0`
///
/// \Then 0 is returned and nothing is changed
///       \Calls co_array_fini()
TEST(CO_Val, CoValInitOs_ZeroLength) {
  co_octet_string_t val = nullptr;

  const auto ret = co_val_init_os(&val, nullptr, 0);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, val);
}

#if LELY_NO_MALLOC
/// \Given an uninitialized array of octets (co_octet_string_t)
///
/// \When co_val_init_os() is called with a pointer to the array, a pointer
///       to an array of octets longer than `CO_ARRAY_CAPACITY` and the length
///       of the array of octets
///
/// \Then -1 is returned, the error number is set to ERRNUM_NOMEM
///       \Calls co_array_alloc()
TEST(CO_Val, CoValInitOs_TooBigArray) {
  co_octet_string_t val = arrays.Init<co_octet_string_t>();
  const uint_least8_t buf[CO_ARRAY_CAPACITY + 1u] = {0};

  const auto ret = co_val_init_os(&val, buf, sizeof(buf));

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}
#endif

///@}

/// @name co_val_init_us()
///@{

/// \Given an uninitialized array of 16-bit Unicode characters
///        (co_unicode_string_t)
///
/// \When co_val_init_us() is called with a pointer to the array and
///       a pointer to a null-terminated 16-bit Unicode string
///
/// \Then 0 is returned and the array is initialized with the given string
///       \Calls co_val_init_us_n()
TEST(CO_Val, CoValInitUs_Nominal) {
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();
  const size_t us_val_len = TEST_STR16.size() * sizeof(char16_t);

  const auto ret = co_val_init_us(&val, TEST_STR16.c_str());

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(us_val_len, co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &val));
  CHECK_EQUAL(0, memcmp(TEST_STR16.c_str(), val, us_val_len));

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val);
}

/// \Given an uninitialized array of 16-bit Unicode characters
///        (co_unicode_string_t)
///
/// \When co_val_init_us() is called with a pointer to the array and a null
///       string pointer
///
/// \Then 0 is returned and the array is initialized to an empty string
///       \Calls co_array_fini()
TEST(CO_Val, CoValInitUs_NullString) {
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();

  const auto ret = co_val_init_us(&val, nullptr);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &val));

  CHECK(arrays.IsEmptyInitialized(val));
}

///@}

/// @name co_val_init_us_n()
///@{

/// \Given an uninitialized array of 16-bit Unicode characters
///        (co_unicode_string_t)
///
/// \When co_val_init_us_n() is called with a pointer to the array, a pointer
///       to a 16-bit Unicode string and the length of the string
///
/// \Then 0 is returned and the array is initialized with the given string
///       \Calls co_array_alloc()
///       \Calls co_array_init()
///       \Calls str16cpy()
TEST(CO_Val, CoValInitUsN_Nominal) {
  const size_t n = 6u;
  const size_t us_val_len = n * sizeof(char16_t);
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();

  const auto ret = co_val_init_us_n(&val, TEST_STR16.c_str(), n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(us_val_len, co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &val));
  char16_t testbuf[n + 1u] = {0};
  POINTERS_EQUAL(testbuf, memcpy(testbuf, TEST_STR16.c_str(), us_val_len));
  CHECK_EQUAL(0, memcmp(testbuf, val, us_val_len + sizeof(char16_t)));

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val);
}

/// \Given an uninitialized array of 16-bit Unicode characters
///        (co_unicode_string_t)
///
/// \When co_val_init_us_n() is called with a pointer to the array, a null
///       string pointer and a non-zero length of a string
///
/// \Then 0 is returned and the array is initialized with a zeroed string of
///       the given length
///       \Calls co_array_alloc()
///       \Calls co_array_init()
TEST(CO_Val, CoValInitUsN_NullString) {
  const size_t n = 8u;
  const size_t us_val_len = n * sizeof(char16_t);
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();

  const auto ret = co_val_init_us_n(&val, nullptr, n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(us_val_len, co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &val));
  const char16_t testbuf[n + 1u] = {0};
  CHECK_EQUAL(0, memcmp(testbuf, val, us_val_len + sizeof(char16_t)));

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val);
}

/// \Given an uninitialized array of 16-bit Unicode characters
///        (co_unicode_string_t)
///
/// \When co_val_init_us_n() is called with a pointer to the array, a null
///       string pointer and a length of a string equal to `0`
///
/// \Then 0 is returned and nothing is changed
///       \Calls co_array_fini()
TEST(CO_Val, CoValInitUsN_ZeroLength) {
  co_unicode_string_t val = nullptr;

  const auto ret = co_val_init_us_n(&val, nullptr, 0);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, val);
}

#if LELY_NO_MALLOC
/// \Given an uninitialized array of 16-bit Unicode characters
///        (co_unicode_string_t)
///
/// \When co_val_init_us_n() is called with a pointer to the array, a pointer
///       to a string longer than `CO_ARRAY_CAPACITY` and the length of the
///       string
///
/// \Then -1 is returned, the error number is set to ERRNUM_NOMEM
///       \Calls co_array_alloc()
TEST(CO_Val, CoValInitUsN_TooBigString) {
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();
  const char16_t buf[CO_ARRAY_CAPACITY + 1u] = {0};

  const auto ret = co_val_init_us_n(&val, buf, CO_ARRAY_CAPACITY + 1u);

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}
#endif

///@}

/// @name co_val_init_dom()
///@{

/// \Given an uninitialized block of data (co_domain_t)
///
/// \When co_val_init_dom() is called with a pointer to the block, a pointer
///       to an array of bytes and the length of the array of bytes
///
/// \Then 0 is returned and the block is initialized with the given array of
///       bytes
///       \Calls co_array_alloc()
///       \Calls co_array_init()
///       \Calls memcpy()
TEST(CO_Val, CoValInitDom_Nominal) {
  const size_t n = 4u;
  const unsigned char dom[n] = {0xd3u, 0xe5u, 0x98u, 0xbau};
  co_domain_t val = arrays.Init<co_domain_t>();

  const auto ret = co_val_init_dom(&val, dom, n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));
  CHECK_EQUAL(0, memcmp(dom, val, n));

  co_val_fini(CO_DEFTYPE_DOMAIN, &val);
}

/// \Given an uninitialized block of data (co_domain_t)
///
/// \When co_val_init_dom() is called with a pointer to the block, a null
///       array of bytes pointer and a non-zero length of the array of bytes
///
/// \Then 0 is returned and the block is initialized with a zeroed array of
///       bytes
///       \Calls co_array_alloc()
///       \Calls co_array_init()
TEST(CO_Val, CoValInitDom_NullArray) {
  const size_t n = 7u;
  co_domain_t val = arrays.Init<co_domain_t>();

  const auto ret = co_val_init_dom(&val, nullptr, n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));
  const unsigned char testbuf[n] = {0};
  CHECK_EQUAL(0, memcmp(testbuf, val, n));

  co_val_fini(CO_DEFTYPE_DOMAIN, &val);
}

/// \Given an uninitialized block of data (co_domain_t)
///
/// \When co_val_init_dom() is called with a pointer to the block, a null
///       array of bytes pointer and a length of the array of bytes equal to
///       `0`
///
/// \Then 0 is returned and nothing is changed
///       \Calls co_array_fini()
TEST(CO_Val, CoValInitDom_ZeroLength) {
  co_domain_t val = nullptr;

  const auto ret = co_val_init_dom(&val, nullptr, 0);

  CHECK_EQUAL(0, ret);
  POINTERS_EQUAL(nullptr, val);
}

#if LELY_NO_MALLOC
/// \Given an uninitialized block of data (co_domain_t)
///
/// \When co_val_init_dom() is called with a pointer to the block, a pointer
///       to an array of bytes longer than `CO_ARRAY_CAPACITY` and the length
///       of the array of bytes
///
/// \Then -1 is returned, the error number is set to ERRNUM_NOMEM
///       \Calls co_array_alloc()
TEST(CO_Val, CoValInitDom_TooBigArray) {
  co_domain_t val = arrays.Init<co_domain_t>();
  const uint_least8_t buf[CO_ARRAY_CAPACITY + 1u] = {0};

  const auto ret = co_val_init_dom(&val, buf, sizeof(buf));

  CHECK_EQUAL(-1, ret);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}
#endif

///@}

/// @name co_val_fini()
///@{

/// \Given a value of any non-array data type
///
/// \When co_val_fini() is called with the value data type and a pointer to
///       the value
///
/// \Then nothing is changed
///       \Calls co_type_is_array()
TEST(CO_Val, CoValFini_NonArrayType) {
  co_integer16_t val;
  CHECK_EQUAL(0, co_val_init(CO_DEFTYPE_INTEGER16, &val));

  co_val_fini(CO_DEFTYPE_INTEGER16, &val);
}

/// \Given a value of any array data type
///
/// \When co_val_fini() is called with the array data type and a pointer to
///       the value
///
/// \Then the value is freed and finalized
///       \Calls co_type_is_array()
///       \Calls co_array_free()
///       \Calls co_array_fini()
TEST(CO_Val, CoValFini_ArrayType) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(0, co_val_init_vs(&val, TEST_STR.c_str()));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);

  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val));
#if LELY_NO_MALLOC
  for (size_t i = 0; i < CO_ARRAY_CAPACITY; ++i) CHECK_EQUAL(0, val[i]);
#endif
}

///@}

/// @name co_val_addressof()
///@{

/// \Given N/A
///
/// \When co_val_addressof() is called with any data type and a null value
///       pointer
///
/// \Then a null pointer is returned
TEST(CO_Val, CoValAddressof_Null) {
  POINTERS_EQUAL(nullptr, co_val_addressof(CO_DEFTYPE_INTEGER16, nullptr));
}

/// \Given a value of any array data type
///
/// \When co_val_addressof() is called with the array data type and a pointer
///       to the value
///
/// \Then the address of the first byte in the array is returned
///       \Calls co_type_is_array()
TEST(CO_Val, CoValAddressof_ArrayType) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();
  co_val_init_vs(&val, TEST_STR.c_str());

  const auto* ptr = co_val_addressof(CO_DEFTYPE_VISIBLE_STRING, val);

  POINTERS_EQUAL(*reinterpret_cast<void**>(val), ptr);

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

/// \Given a value of any non-array data type
///
/// \When co_val_addressof() is called with the value data type and a pointer
///       to the value
///
/// \Then the address of the first byte in the value is returned
///       \Calls co_type_is_array()
TEST(CO_Val, CoValAddressof_NonArrayType) {
  co_integer16_t val;
  co_val_init(CO_DEFTYPE_INTEGER16, &val);

  const auto* ptr = co_val_addressof(CO_DEFTYPE_INTEGER16, &val);

  POINTERS_EQUAL(&val, ptr);

  co_val_fini(CO_DEFTYPE_INTEGER16, &val);
}

///@}

/// @name co_val_sizeof()
///@{

/// \Given N/A
///
/// \When co_val_sizeof() is called with any data type and a null value
///       pointer
///
/// \Then 0 is returned
TEST(CO_Val, CoValSizeof_Null) {
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_INTEGER16, nullptr));
}

/// \Given a value of any array data type
///
/// \When co_val_sizeof() is called with the array data type and a pointer
///       to the value
///
/// \Then the array size (in bytes) is returned
///       \Calls co_type_is_array()
///       \Calls co_array_sizeof()
TEST(CO_Val, CoValSizeof_ArrayType) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();
  co_val_init_vs(&val, TEST_STR.c_str());

  const auto ret = co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val);

  CHECK_EQUAL(TEST_STR.size(), ret);

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

/// \Given a value of any non-array data type
///
/// \When co_val_sizeof() is called with the value data type and a pointer
///       to the value
///
/// \Then the value size (in bytes) is returned
///       \Calls co_type_is_array()
///       \Calls co_type_sizeof()
TEST(CO_Val, CoValSizeof_NonArrayType) {
  co_integer16_t val;
  co_val_init(CO_DEFTYPE_INTEGER16, &val);

  const auto ret = co_val_sizeof(CO_DEFTYPE_INTEGER16, &val);

  CHECK_EQUAL(sizeof(val), ret);

  co_val_fini(CO_DEFTYPE_INTEGER16, &val);
}

///@}

/// @name co_val_make()
///@{

/// \Given an uninitialized array of visible characters (co_visible_string_t)
///
/// \When co_val_make() is called with the array data type, a pointer to the
///       array, a pointer to a null-terminated string and any length of the
///       string
///
/// \Then the length of the string is returned and the array is constructed
///       from the given string
///       \Calls strlen()
///       \Calls co_val_init_vs()
TEST(CO_Val, CoValMake_VISIBLE_STRING_Nominal) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();

  const auto ret =
      co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val, TEST_STR.c_str(), 0);

  CHECK_EQUAL(TEST_STR.size(), ret);
  CHECK_EQUAL(TEST_STR.size(), co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val));
  CHECK_EQUAL(0, memcmp(TEST_STR.c_str(), val, TEST_STR.size() + 1u));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

/// \Given an uninitialized array of visible characters (co_visible_string_t)
///
/// \When co_val_make() is called with the array data type, a pointer to the
///       array, a null string pointer and any length of the string
///
/// \Then 0 is returned, nothing is changed
///       \Calls co_val_init_vs()
TEST(CO_Val, CoValMake_VISIBLE_STRING_NullPtr) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();

  const auto ret = co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val, nullptr, 3u);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val));
}

#if LELY_NO_MALLOC
/// \Given an uninitialized array of visible characters (co_visible_string_t)
///
/// \When co_val_make() is called with the array data type, a pointer to the
///       array, a pointer to a string longer than `CO_ARRAY_CAPACITY` and
///       any length of the string
///
/// \Then 0 is returned, the error number is set to ERRNUM_NOMEM
///       \Calls strlen()
///       \Calls co_val_init_vs()
TEST(CO_Val, CoValMake_VISIBLE_STRING_TooBigString) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();
  char buf[CO_ARRAY_CAPACITY + 1u] = {0};
  memset(buf, 'a', CO_ARRAY_CAPACITY);

  const auto ret = co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val, buf, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val));
}
#endif

/// \Given an uninitialized array of octets (co_octet_string_t)
///
/// \When co_val_make() is called with the array data type, a pointer to the
///       array, a pointer to an array of octets and the length of the array of
///       octets
///
/// \Then the length of the array is returned and the array is constructed from
///       the given array of octets
///       \Calls co_val_init_os()
TEST(CO_Val, CoValMake_OCTET_STRING_Nominal) {
  const size_t n = 5u;
  const uint_least8_t os[n] = {0xd3u, 0xe5u, 0x98u, 0xbau, 0x96u};
  co_octet_string_t val = arrays.Init<co_octet_string_t>();

  const auto ret = co_val_make(CO_DEFTYPE_OCTET_STRING, &val, os, n);

  CHECK_EQUAL(n, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));
  CHECK_EQUAL(0, memcmp(os, val, n));

  co_val_fini(CO_DEFTYPE_OCTET_STRING, &val);
}

/// \Given an uninitialized array of octets (co_octet_string_t)
///
/// \When co_val_make() is called with the array data type, a pointer to the
///       array, a null array of octets pointer and any length of the array of
///       octets
///
/// \Then 0 is returned, nothing is changed
///       \Calls co_val_init_os()
TEST(CO_Val, CoValMake_OCTET_STRING_NullPtr) {
  co_octet_string_t val = arrays.Init<co_octet_string_t>();

  const auto ret = co_val_make(CO_DEFTYPE_OCTET_STRING, &val, nullptr, 7u);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));
}

#if LELY_NO_MALLOC
/// \Given an uninitialized array of octets (co_octet_string_t)
///
/// \When co_val_make() is called with the array data type, a pointer to the
///       array, a pointer to an array of octets longer than
///       `CO_ARRAY_CAPACITY` and the length of the array of octets
///
/// \Then 0 is returned, the error number is set to ERRNUM_NOMEM
///       \Calls co_val_init_os()
TEST(CO_Val, CoValMake_OCTET_STRING_TooBigArray) {
  co_octet_string_t val = arrays.Init<co_octet_string_t>();
  const uint_least8_t buf[CO_ARRAY_CAPACITY + 1u] = {0};

  const auto ret = co_val_make(CO_DEFTYPE_OCTET_STRING, &val, buf, sizeof(buf));

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));
}
#endif

/// \Given an uninitialized array of 16-bit Unicode characters
///        (co_unicode_string_t)
///
/// \When co_val_make() is called with the array data type, a pointer to the
///       array, a pointer to a null-terminated 16-bit Unicode string and any
///       length of the string
///
/// \Then the length of the string is returned and the array is constructed
///       from the given string
///       \Calls str16len()
///       \Calls co_val_init_us()
TEST(CO_Val, CoValMake_UNICODE_STRING_Nominal) {
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();
  const size_t us_val_len = TEST_STR16.size() * sizeof(char16_t);

  const auto ret =
      co_val_make(CO_DEFTYPE_UNICODE_STRING, &val, TEST_STR16.c_str(), 0);

  CHECK_EQUAL(TEST_STR16.size(), ret);
  CHECK_EQUAL(us_val_len, co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &val));
  CHECK_EQUAL(0, memcmp(TEST_STR16.c_str(), val, us_val_len));

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val);
}
/// \Given an uninitialized array of 16-bit Unicode characters
///        (co_unicode_string_t)
///
/// \When co_val_make() is called with the array data type, a pointer to the
///       array, a null string pointer and any length of the string
///
/// \Then 0 is returned, nothing is changed
///       \Calls co_val_init_us()
TEST(CO_Val, CoValMake_UNICODE_STRING_NullPtr) {
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();

  const auto ret = co_val_make(CO_DEFTYPE_UNICODE_STRING, &val, nullptr, 4u);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &val));
}

#if LELY_NO_MALLOC
/// \Given an uninitialized array of 16-bit Unicode characters
///        (co_unicode_string_t)
///
/// \When co_val_make() is called with the array data type, a pointer to the
///       array, a pointer to a 16-bit Unicode string longer than
///       `CO_ARRAY_CAPACITY` and any length of the string
///
/// \Then 0 is returned, the error number is set to ERRNUM_NOMEM
///       \Calls str16len()
///       \Calls co_val_init_us()
TEST(CO_Val, CoValMake_UNICODE_STRING_TooBigString) {
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();
  char16_t buf[(CO_ARRAY_CAPACITY / sizeof(char16_t)) + 1u] = {0};
  // incorrect unicode, but good enough for test
  memset(buf, 'a', sizeof(buf) - sizeof(char16_t));

  const auto ret = co_val_make(CO_DEFTYPE_UNICODE_STRING, &val, buf, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &val));
}
#endif

/// \Given an uninitialized block of data (co_domain_t)
///
/// \When co_val_make() is called with the block data type, a pointer to the
///       block, a pointer to an array of bytes and the length of the array of
///       bytes
///
/// \Then the length of the array is returned and the block is constructed from
///       the given array of bytes
///       \Calls co_val_init_dom()
TEST(CO_Val, CoValMake_DOMAIN_Nominal) {
  const size_t n = 4u;
  const unsigned char dom[n] = {0xd3u, 0xe5u, 0x98u, 0xbau};
  co_domain_t val = arrays.Init<co_domain_t>();

  const auto ret = co_val_make(CO_DEFTYPE_DOMAIN, &val, dom, n);

  CHECK_EQUAL(n, ret);
  CHECK_EQUAL(n, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));
  CHECK_EQUAL(0, memcmp(dom, val, n));

  co_val_fini(CO_DEFTYPE_DOMAIN, &val);
}

/// \Given an uninitialized block of data (co_domain_t)
///
/// \When co_val_make() is called with the block data type, a pointer to the
///       block, a null array of bytes pointer and any length of the array of
///       bytes
///
/// \Then 0 is returned, nothing is changed
///       \Calls co_val_init_dom()
TEST(CO_Val, CoValMake_DOMAIN_NullPtr) {
  const size_t n = 7u;
  co_domain_t val = arrays.Init<co_domain_t>();

  const auto ret = co_val_make(CO_DEFTYPE_DOMAIN, &val, nullptr, n);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));
}

#if LELY_NO_MALLOC
/// \Given an uninitialized block of data (co_domain_t)
///
/// \When co_val_make() is called with the block data type, a pointer to the
///       block, a pointer to an array of bytes longer than `CO_ARRAY_CAPACITY`
///       and the length of the array of bytes
///
/// \Then 0 is returned, the error number is set to ERRNUM_NOMEM
///       \Calls co_val_init_dom()
TEST(CO_Val, CoValMake_DOMAIN_TooBigArray) {
  co_domain_t val = arrays.Init<co_domain_t>();
  const uint_least8_t buf[CO_ARRAY_CAPACITY + 1u] = {0};

  const auto ret = co_val_make(CO_DEFTYPE_DOMAIN, &val, buf, sizeof(buf));

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
  CHECK_EQUAL(0, co_val_sizeof(CO_DEFTYPE_DOMAIN, &val));
}
#endif

/// \Given an uninitialized value of any non-array data type
///
/// \When co_val_make() is called with the value data type, a pointer to the
///       value, a pointer to an array of bytes and the length of the array
///
/// \Then the size of the value is returned and the value is constructed from
///       the given array of bytes
///       \Calls co_type_sizeof()
///       \Calls memcpy()
TEST(CO_Val, CoValMake_NonArrayType_Nominal) {
  co_integer16_t val;
  const uint_least8_t ptr[] = {0x34u, 0x12u};

  const auto ret = co_val_make(CO_DEFTYPE_INTEGER16, &val, ptr, sizeof(val));

  CHECK_EQUAL(sizeof(val), ret);
  CHECK_EQUAL(ldle_i16(ptr), val);

  co_val_fini(CO_DEFTYPE_INTEGER16, &val);
}

/// \Given an uninitialized value of any non-array data type
///
/// \When co_val_make() is called with the value data type, a pointer to the
///       value, a null array of bytes pointer and any length of the array
///
/// \Then 0 is returned, nothing is changed
TEST(CO_Val, CoValMake_NonArrayType_NullPtr) {
  co_integer16_t val;

  const auto ret =
      co_val_make(CO_DEFTYPE_INTEGER16, &val, nullptr, sizeof(val));

  CHECK_EQUAL(0, ret);
}

/// \Given an uninitialized value of any non-array data type
///
/// \When co_val_make() is called with the value data type, a pointer to the
///       value, a pointer to an array of bytes of a different size than the
///       data type size and the length of the array
///
/// \Then 0 is returned, nothing is changed
///       \Calls co_type_sizeof()
TEST(CO_Val, CoValMake_NonArrayType_WrongSize) {
  co_integer16_t val;
  const uint_least8_t ptr[sizeof(val) + 1u] = {0};

  const auto ret =
      co_val_make(CO_DEFTYPE_INTEGER16, &val, ptr, sizeof(val) + 1u);

  CHECK_EQUAL(0, ret);
}

///@}

/// @name co_val_copy()
///@{

/// \Given a source and a destination arrays of visible characters
///        (co_visible_string_t)
///
/// \When co_val_copy() is called with the array data type, a pointer to the
///       destination array, a pointer to the source array
///
/// \Then the length of the source array is returned, the source array value
///       is deep copied to the destination array
///       \Calls co_type_is_array()
///       \Calls co_val_init_vs()
TEST(CO_Val, CoValCopy_VISIBLE_STRING_Nominal) {
  co_visible_string_t src = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(TEST_STR.size(), co_val_make(CO_DEFTYPE_VISIBLE_STRING, &src,
                                           TEST_STR.c_str(), 0));
  co_visible_string_t dst = arrays.Init<co_visible_string_t>();

  const auto ret = co_val_copy(CO_DEFTYPE_VISIBLE_STRING, &dst, &src);

  CHECK(dst != nullptr);
  CHECK_EQUAL(co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &src), ret);
  CHECK_EQUAL(ret, co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &dst));
  CHECK_EQUAL(0, memcmp(TEST_STR.c_str(), dst, TEST_STR.size() + 1u));
  CHECK(co_val_addressof(CO_DEFTYPE_VISIBLE_STRING, &src) !=
        co_val_addressof(CO_DEFTYPE_VISIBLE_STRING, &dst));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &src);
  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &dst);
}

#if LELY_NO_MALLOC
/// \Given a source and a destination arrays of visible characters
///        (co_visible_string_t), the destination array has smaller capacity
///        than the source
///
/// \When co_val_copy() is called with the array data type, a pointer to the
///       destination array, a pointer to the source array
///
/// \Then 0 is returned, the destination array is not altered, the error
///       number is set to ERRNUM_NOMEM
///       \Calls co_type_is_array()
///       \Calls co_val_init_vs()
TEST(CO_Val, CoValCopy_VISIBLE_STRING_TooSmallDestination) {
  co_visible_string_t src = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(TEST_STR.size(), co_val_make(CO_DEFTYPE_VISIBLE_STRING, &src,
                                           TEST_STR.c_str(), 0));
  co_array dst_array = CO_ARRAY_INIT;
  dst_array.hdr.capacity = TEST_STR.size() - 1u;
  co_visible_string_t dst;
  co_val_init_array(&dst, &dst_array);

  const auto ret = co_val_copy(CO_DEFTYPE_VISIBLE_STRING, &dst, &src);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, dst_array.hdr.size);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}
#endif

/// \Given a source and a destination arrays of octets (co_octet_string_t)
///
/// \When co_val_copy() is called with the array data type, a pointer to the
///       destination array, a pointer to the source array
///
/// \Then the length of the source array is returned, the source array value
///       is deep copied to the destination array
///       \Calls co_type_is_array()
///       \Calls co_val_init_os()
TEST(CO_Val, CoValCopy_OCTET_STRING_Nominal) {
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
/// \Given a source and a destination arrays of octets (co_octet_string_t),
///        the destination array has smaller capacity than the source
///
/// \When co_val_copy() is called with the array data type, a pointer to the
///       destination array, a pointer to the source array
///
/// \Then 0 is returned, the destination array is not altered, the error
///       number is set to ERRNUM_NOMEM
///       \Calls co_type_is_array()
///       \Calls co_val_init_os()
TEST(CO_Val, CoValCopy_OCTET_STRING_TooSmallDestination) {
  const size_t n = 5u;
  co_octet_string_t src = arrays.Init<co_octet_string_t>();
  const uint_least8_t os[n] = {0xd3u, 0xe5u, 0x98u, 0xbau, 0x96u};
  CHECK_EQUAL(n, co_val_make(CO_DEFTYPE_OCTET_STRING, &src, os, n));
  co_array dst_array = CO_ARRAY_INIT;
  dst_array.hdr.capacity = n - 1u;
  co_octet_string_t dst;
  co_val_init_array(&dst, &dst_array);

  const auto ret = co_val_copy(CO_DEFTYPE_OCTET_STRING, &dst, &src);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, dst_array.hdr.size);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}
#endif

/// \Given a source and a destination arrays of 16-bit Unicode characters
///        (co_unicode_string_t)
///
/// \When co_val_copy() is called with the array data type, a pointer to the
///       destination array, a pointer to the source array
///
/// \Then the length (in bytes) of the source array is returned, the source
///       array value is deep copied to the destination array
///       \Calls co_type_is_array()
///       \Calls co_val_init_us()
TEST(CO_Val, CoValCopy_UNICODE_STRING_Nominal) {
  co_unicode_string_t src = arrays.Init<co_unicode_string_t>();
  CHECK_EQUAL(TEST_STR16.size(), co_val_make(CO_DEFTYPE_UNICODE_STRING, &src,
                                             TEST_STR16.c_str(), 0));
  co_unicode_string_t dst = arrays.Init<co_unicode_string_t>();

  const auto ret = co_val_copy(CO_DEFTYPE_UNICODE_STRING, &dst, &src);

  CHECK(dst != nullptr);
  CHECK_EQUAL(co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &src), ret);
  CHECK_EQUAL(ret, co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &dst));
  CHECK_EQUAL(0, memcmp(TEST_STR16.c_str(), dst, ret));
  CHECK(co_val_addressof(CO_DEFTYPE_UNICODE_STRING, &src) !=
        co_val_addressof(CO_DEFTYPE_UNICODE_STRING, &dst));

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &src);
  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &dst);
}

#if LELY_NO_MALLOC
/// \Given a source and a destination arrays of 16-bit Unicode characters
///        (co_unicode_string_t), the destination array has smaller capacity
///        than the source
///
/// \When co_val_copy() is called with the array data type, a pointer to the
///       destination array, a pointer to the source array
///
/// \Then 0 is returned, the destination array is not altered, the error
///       number is set to ERRNUM_NOMEM
///       \Calls co_type_is_array()
///       \Calls co_val_init_us()
TEST(CO_Val, CoValCopy_UNICODE_STRING_TooSmallDestination) {
  co_unicode_string_t src = arrays.Init<co_unicode_string_t>();
  CHECK_EQUAL(TEST_STR16.size(), co_val_make(CO_DEFTYPE_UNICODE_STRING, &src,
                                             TEST_STR16.c_str(), 0));
  co_array dst_array = CO_ARRAY_INIT;
  dst_array.hdr.capacity = TEST_STR16.size() - 1u;
  co_unicode_string_t dst;
  co_val_init_array(&dst, &dst_array);

  const auto ret = co_val_copy(CO_DEFTYPE_UNICODE_STRING, &dst, &src);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, dst_array.hdr.size);
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}
#endif

/// \Given a source and a destination blocks of data (co_domain_t)
///
/// \When co_val_copy() is called with the block data type, a pointer to the
///       destination block, a pointer to the source block
///
/// \Then the length of the source block is returned, the source block data
///       is deep copied to the destination block
///       \Calls co_type_is_array()
///       \Calls co_val_init_dom()
TEST(CO_Val, CoValCopy_DOMAIN_Nominal) {
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
/// \Given a source and a destination blocks of data (co_domain_t),
///        the destination block has smaller capacity than the source
///
/// \When co_val_copy() is called with the block data type, a pointer to the
///       destination block, a pointer to the source block
///
/// \Then 0 is returned, the destination block is not altered, the error
///       number is set to ERRNUM_NOMEM
///       \Calls co_type_is_array()
///       \Calls co_val_init_dom()
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
  CHECK_EQUAL(ERRNUM_NOMEM, get_errnum());
}
#endif

/// \Given a source and a destination values of non-array data type
///
/// \When co_val_copy() is called with the value data type, a pointer to the
///       destination value, a pointer to the source value
///
/// \Then the length of the source value is returned, the source value is
///       copied to the destination
///       \Calls co_type_is_array()
///       \Calls co_type_sizeof()
///       \Calls memcpy()
TEST(CO_Val, CoValCopy_NonArrayType) {
  co_integer16_t src;
  const uint_least8_t ptr[] = {0x34u, 0x12u};
  CHECK_EQUAL(sizeof(src),
              co_val_make(CO_DEFTYPE_INTEGER16, &src, ptr, sizeof(src)));
  co_integer16_t dst;

  const auto ret = co_val_copy(CO_DEFTYPE_INTEGER16, &dst, &src);

  CHECK_EQUAL(sizeof(dst), ret);
  CHECK_EQUAL(ldle_i16(ptr), dst);
}

///@}

/// @name co_val_move()
///@{

/// \Given a source and a destination values of non-array data type
///
/// \When co_val_move() is called with the value data type, a pointer to the
///       destination value, a pointer to the source value
///
/// \Then the length of the source value is returned, the source value is
///       copied to the destination
///       \Calls co_type_sizeof()
///       \Calls memcpy()
///       \Calls co_type_is_array()
TEST(CO_Val, CoValMove_NonArrayType) {
  co_integer16_t src;
  const uint_least8_t ptr[] = {0x34u, 0x12u};
  CHECK_EQUAL(sizeof(src),
              co_val_make(CO_DEFTYPE_INTEGER16, &src, ptr, sizeof(src)));
  co_integer16_t dst;

  const auto ret = co_val_move(CO_DEFTYPE_INTEGER16, &dst, &src);

  CHECK_EQUAL(sizeof(dst), ret);
  CHECK_EQUAL(ldle_i16(ptr), dst);
}

/// \Given a source and a destination values of any array data type
///
/// \When co_val_move() is called with the array data type, a pointer to the
///       destination value, a pointer to the source value
///
/// \Then the length of the source value is returned, the source value is
///       moved to the destination
///       \Calls co_type_sizeof()
///       \Calls memcpy()
///       \Calls co_type_is_array()
TEST(CO_Val, CoValMove_ArrayType) {
  co_visible_string_t src = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(TEST_STR.size(), co_val_make(CO_DEFTYPE_VISIBLE_STRING, &src,
                                           TEST_STR.c_str(), 0));
  const void* src_addr = co_val_addressof(CO_DEFTYPE_VISIBLE_STRING, &src);
  co_visible_string_t dst = arrays.Init<co_visible_string_t>();

  const auto ret = co_val_move(CO_DEFTYPE_VISIBLE_STRING, &dst, &src);

  CHECK_EQUAL(sizeof(src), ret);
  CHECK_EQUAL(0, memcmp(TEST_STR.c_str(), dst, TEST_STR.size() + 1u));
  POINTERS_EQUAL(src_addr, co_val_addressof(CO_DEFTYPE_VISIBLE_STRING, &dst));
  POINTERS_EQUAL(nullptr, co_val_addressof(CO_DEFTYPE_VISIBLE_STRING, &src));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &dst);
}

///@}

/// @name co_val_cmp()
///@{

/// \Given a value of any data type
///
/// \When co_val_cmp() is called with the value data type and pointers to the
///       same value
///
/// \Then 0 is returned
TEST(CO_Val, CoValCmp_PointersEqual) {
  co_integer16_t val;
  CHECK_EQUAL(0, co_val_init(CO_DEFTYPE_INTEGER16, &val));

  CHECK_EQUAL(0, co_val_cmp(CO_DEFTYPE_INTEGER16, &val, &val));
}

/// \Given a value of any data type
///
/// \When co_val_cmp() is called with the value data type, a null value pointer
///       and a pointer to the value
///
/// \Then an integer less than 0 is returned
TEST(CO_Val, CoValCmp_FirstValNull) {
  co_integer16_t val;
  CHECK_EQUAL(0, co_val_init(CO_DEFTYPE_INTEGER16, &val));

  const auto ret = co_val_cmp(CO_DEFTYPE_INTEGER16, nullptr, &val);
  CHECK_COMPARE(ret, <, 0);
}

/// \Given a value of any data type
///
/// \When co_val_cmp() is called with the value data type, a pointer to the
///       value and a null value pointer
///
/// \Then an integer greater than 0 is returned
TEST(CO_Val, CoValCmp_SecondValNull) {
  co_integer16_t val;
  CHECK_EQUAL(0, co_val_init(CO_DEFTYPE_INTEGER16, &val));

  const auto ret = co_val_cmp(CO_DEFTYPE_INTEGER16, &val, nullptr);
  CHECK_COMPARE(ret, >, 0);
}

/// \Given two values of any array data type pointing to the same array address
///
/// \When co_val_cmp() is called with the array data type and pointers to the
///       values
///
/// \Then 0 is returned
///       \Calls co_type_is_array()
TEST(CO_Val, CoValCmp_ArrayType_ArrayAddressesEqual) {
  co_visible_string_t val1 = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(TEST_STR.size(), co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val1,
                                           TEST_STR.c_str(), 0));
  co_visible_string_t val2 = val1;

  CHECK_EQUAL(0, co_val_cmp(CO_DEFTYPE_VISIBLE_STRING, &val1, &val2));

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val1);
  val2 = nullptr;
}

/// \Given two values of any array data type, the first initialized with
///        a string/array and the second with a null string/array pointer
///
/// \When co_val_cmp() is called with the array data type and pointers to the
///       values
///
/// \Then an integer less then 0 is returned
///       \Calls co_type_is_array()
TEST(CO_Val, CoValCmp_ArrayType_FirstValNull) {
  co_visible_string_t val1 = nullptr;
  CHECK_EQUAL(0, co_val_init(CO_DEFTYPE_VISIBLE_STRING, &val1));
  co_visible_string_t val2 = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(TEST_STR.size(), co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val2,
                                           TEST_STR.c_str(), 0));

  const auto ret = co_val_cmp(CO_DEFTYPE_VISIBLE_STRING, &val1, &val2);
  CHECK_COMPARE(ret, <, 0);

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val2);
}

/// \Given two values of any array data type, the first initialized with a null
///        string/array pointer and the second initialized with a string/array
///
/// \When co_val_cmp() is called with the array data type and pointers to the
///       values
///
/// \Then an integer greater than 0 is returned
///       \Calls co_type_is_array()
TEST(CO_Val, CoValCmp_ArrayType_SecondValNull) {
  co_visible_string_t val1 = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(TEST_STR.size(), co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val1,
                                           TEST_STR.c_str(), 0));
  co_visible_string_t val2 = nullptr;
  CHECK_EQUAL(0, co_val_init(CO_DEFTYPE_VISIBLE_STRING, &val2));

  const auto ret = co_val_cmp(CO_DEFTYPE_VISIBLE_STRING, &val1, &val2);
  CHECK_COMPARE(ret, >, 0);

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val1);
}

/// \Given two non-equal values of a basic data type, the first value is less
///        than the second
///
/// \When co_val_cmp() is called with the value data type and pointers to the
///       values
///
/// \Then an integer less than 0 is returned
///       \Calls co_type_is_array()
///       \Calls bool_cmp()
///       \Calls int8_cmp()
///       \Calls int16_cmp()
///       \Calls int32_cmp()
///       \Calls uint8_cmp()
///       \Calls uint16_cmp()
///       \Calls uint32_cmp()
///       \Calls flt_cmp()
///       \Calls int32_cmp()
///       \Calls dbl_cmp()
///       \Calls int64_cmp()
///       \Calls int64_cmp()
///       \Calls int64_cmp()
///       \Calls int64_cmp()
///       \Calls uint32_cmp()
///       \Calls uint64_cmp()
///       \Calls uint64_cmp()
///       \Calls uint64_cmp()
///       \Calls uint64_cmp()
TEST(CO_Val, CoValCmp_BasicTypes) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val1; \
    co_##name##_t val2; \
    CHECK_EQUAL(0, co_val_init_min(CO_DEFTYPE_##NAME, &val1)); \
    CHECK_EQUAL(0, co_val_init_max(CO_DEFTYPE_##NAME, &val2)); \
\
    const auto ret = co_val_cmp(CO_DEFTYPE_##NAME, &val1, &val2); \
\
    CHECK_COMPARE(ret, <, 0); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given two non-equal values of a time data type, the first value is less
///        than the second
///
/// \When co_val_cmp() is called with the time data type and pointers to the
///       values
///
/// \Then an integer less than 0 is returned
///       \Calls co_type_is_array()
///       \Calls uint16_cmp()
TEST(CO_Val, CoValCmp_TimeTypes) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val1; \
    co_##name##_t val2; \
    CHECK_EQUAL(0, co_val_init_min(CO_DEFTYPE_##NAME, &val1)); \
    CHECK_EQUAL(0, co_val_init_max(CO_DEFTYPE_##NAME, &val2)); \
\
    const auto ret = co_val_cmp(CO_DEFTYPE_##NAME, &val1, &val2); \
\
    CHECK_COMPARE(ret, <, 0); \
  }

#include <lely/co/def/time.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given two non-equal values of a time data type, the first value is less
///        than the second, but both have equal `days` values
///
/// \When co_val_cmp() is called with the time data type and pointers to the
///       values
///
/// \Then an integer less than 0 is returned
///       \Calls co_type_is_array()
///       \Calls uint16_cmp()
///       \Calls uint32_cmp()
TEST(CO_Val, CoValCmp_TimeTypes_EqualDays) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val1; \
    co_##name##_t val2; \
    CHECK_EQUAL(0, co_val_init_min(CO_DEFTYPE_##NAME, &val1)); \
    CHECK_EQUAL(0, co_val_init_max(CO_DEFTYPE_##NAME, &val2)); \
    val1.days = val2.days; \
\
    const auto ret = co_val_cmp(CO_DEFTYPE_##NAME, &val1, &val2); \
\
    CHECK_COMPARE(ret, <, 0); \
  }
#include <lely/co/def/time.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

#if CO_DEFTYPE_TIME_SCET

/// \Given two non-equal values of the SCET (Spacecraft Elapsed Time) data
///        type, the first value is less than the second
///
/// \When co_val_cmp() is called with the SCET data type and pointers to the
///       values
///
/// \Then an integer less than 0 is returned
///       \Calls co_type_is_array()
///       \Calls uint32_cmp()
TEST(CO_Val, CoValCmp_TIME_SCET) {
  co_time_scet_t val1;
  co_time_scet_t val2;
  CHECK_EQUAL(0, co_val_init_min(CO_DEFTYPE_TIME_SCET, &val1));
  CHECK_EQUAL(0, co_val_init_max(CO_DEFTYPE_TIME_SCET, &val2));

  const auto ret = co_val_cmp(CO_DEFTYPE_TIME_SCET, &val1, &val2);

  CHECK_COMPARE(ret, <, 0);
}

/// \Given two non-equal values of the SCET (Spacecraft Elapsed Time) data
///        type, the first value is less than the second, but both have equal
///        `seconds` values
///
/// \When co_val_cmp() is called with the SCET data type and pointers to the
///       values
///
/// \Then an integer less than 0 is returned
///       \Calls co_type_is_array()
///       \Calls uint32_cmp()
TEST(CO_Val, CoValCmp_TIME_SCET_EqualSeconds) {
  co_time_scet_t val1;
  co_time_scet_t val2;
  CHECK_EQUAL(0, co_val_init_min(CO_DEFTYPE_TIME_SCET, &val1));
  CHECK_EQUAL(0, co_val_init_max(CO_DEFTYPE_TIME_SCET, &val2));
  val1.seconds = val2.seconds;

  const auto ret = co_val_cmp(CO_DEFTYPE_TIME_SCET, &val1, &val2);

  CHECK_COMPARE(ret, <, 0);
}

#endif  // CO_DEFTYPE_TIME_SCET

#if CO_DEFTYPE_TIME_SUTC

/// \Given two non-equal values of the SUTC (Spacecraft Universal Time
///        Coordinated) data type, the first value is less than the second
///
/// \When co_val_cmp() is called with the SUTC data type and pointers to the
///       values
///
/// \Then an integer less than 0 is returned
///       \Calls co_type_is_array()
///       \Calls uint16_cmp()
TEST(CO_Val, CoValCmp_TIME_SUTC) {
  co_time_sutc_t val1;
  co_time_sutc_t val2;
  CHECK_EQUAL(0, co_val_init_min(CO_DEFTYPE_TIME_SUTC, &val1));
  CHECK_EQUAL(0, co_val_init_max(CO_DEFTYPE_TIME_SUTC, &val2));

  const auto ret = co_val_cmp(CO_DEFTYPE_TIME_SUTC, &val1, &val2);

  CHECK_COMPARE(ret, <, 0);
}

/// \Given two non-equal values of the SUTC (Spacecraft Universal Time
///        Coordinated) data type, the first value is less than the second,
///        but both have equal `days` values
///
/// \When co_val_cmp() is called with the SUTC data type and pointers to the
///       values
///
/// \Then an integer less than 0 is returned
///       \Calls co_type_is_array()
///       \Calls uint16_cmp()
///       \Calls uint32_cmp()
TEST(CO_Val, CoValCmp_TIME_SUTC_EqualDays) {
  co_time_sutc_t val1;
  co_time_sutc_t val2;
  CHECK_EQUAL(0, co_val_init_min(CO_DEFTYPE_TIME_SUTC, &val1));
  CHECK_EQUAL(0, co_val_init_max(CO_DEFTYPE_TIME_SUTC, &val2));
  val1.days = val2.days;

  const auto ret = co_val_cmp(CO_DEFTYPE_TIME_SUTC, &val1, &val2);

  CHECK_COMPARE(ret, <, 0);
}

/// \Given two non-equal values of the SUTC (Spacecraft Universal Time
///        Coordinated) data type, the first value is less than the second,
///        but both have equal `days` and `ms` values
///
/// \When co_val_cmp() is called with the SUTC data type and pointers to the
///       values
///
/// \Then an integer less than 0 is returned
///       \Calls co_type_is_array()
///       \Calls uint16_cmp()
///       \Calls uint32_cmp()
TEST(CO_Val, CoValCmp_TIME_SUTC_EqualDaysMs) {
  co_time_sutc_t val1;
  co_time_sutc_t val2;
  CHECK_EQUAL(0, co_val_init_min(CO_DEFTYPE_TIME_SUTC, &val1));
  CHECK_EQUAL(0, co_val_init_max(CO_DEFTYPE_TIME_SUTC, &val2));
  val1.days = val2.days;
  val1.ms = val2.ms;

  const auto ret = co_val_cmp(CO_DEFTYPE_TIME_SUTC, &val1, &val2);

  CHECK_COMPARE(ret, <, 0);
}

#endif  // CO_DEFTYPE_TIME_SUTC

/// \Given two non-equal arrays of visible characters (co_visible_string_t)
///        with the same length, the first array value is greater than the
///        second
///
/// \When co_val_cmp() is called with the array data type and pointers to the
///       arrays
///
/// \Then an integer greater than 0 is returned
///       \Calls co_type_is_array()
///       \Calls co_val_sizeof()
///       \Calls strncmp()
TEST(CO_Val, CoValCmp_VISIBLE_STRING) {
  const std::string TEST_STR2 = "abcdefg";

  co_visible_string_t val1 = arrays.Init<co_visible_string_t>();
  co_visible_string_t val2 = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(TEST_STR.size(), co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val1,
                                           TEST_STR.c_str(), 0));
  CHECK_EQUAL(TEST_STR2.size(), co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val2,
                                            TEST_STR2.c_str(), 0));

  const auto ret = co_val_cmp(CO_DEFTYPE_VISIBLE_STRING, &val1, &val2);

  CHECK_COMPARE(ret, >, 0);

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val1);
  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val2);
}

/// \Given two non-equal arrays of visible characters (co_visible_string_t),
///        the second array is a shorter substring of the first
///
/// \When co_val_cmp() is called with the array data type and pointers to the
///       arrays
///
/// \Then an integer greater than 0 is returned
///       \Calls co_type_is_array()
///       \Calls co_val_sizeof()
///       \Calls strncmp()
TEST(CO_Val, CoValCmp_VISIBLE_STRING_Substr) {
  co_visible_string_t val1 = arrays.Init<co_visible_string_t>();
  co_visible_string_t val2 = arrays.Init<co_visible_string_t>();
  CHECK_EQUAL(TEST_STR.size(), co_val_make(CO_DEFTYPE_VISIBLE_STRING, &val1,
                                           TEST_STR.c_str(), 0));
  CHECK_EQUAL(0,
              co_val_init_vs_n(&val2, TEST_STR.c_str(), TEST_STR.size() - 5u));

  const auto ret = co_val_cmp(CO_DEFTYPE_VISIBLE_STRING, &val1, &val2);

  CHECK_COMPARE(ret, >, 0);

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val1);
  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val2);
}

/// \Given two non-equal arrays of octets (co_octet_string_t), the first array
///        value is greater than the second
///
/// \When co_val_cmp() is called with the array data type and pointers to the
///       arrays
///
/// \Then an integer greater than 0 is returned
///       \Calls co_type_is_array()
///       \Calls co_val_sizeof()
///       \Calls memcmp()
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

/// \Given two non-equal arrays of 16-bit Unicode characters
///        (co_unicode_string_t), the first array value is greater than the
///        second
///
/// \When co_val_cmp() is called with the array data type and pointers to the
///       arrays
///
/// \Then an integer greater than 0 is returned
///       \Calls co_type_is_array()
///       \Calls co_val_sizeof()
///       \Calls str16ncmp()
TEST(CO_Val, CoValCmp_UNICODE_STRING) {
  const std::u16string TEST_STR16_2 = u"abcdefg";

  co_unicode_string_t val1 = arrays.Init<co_unicode_string_t>();
  co_unicode_string_t val2 = arrays.Init<co_unicode_string_t>();
  CHECK_EQUAL(TEST_STR16.size(), co_val_make(CO_DEFTYPE_UNICODE_STRING, &val1,
                                             TEST_STR16.c_str(), 0));
  CHECK_EQUAL(TEST_STR16_2.size(), co_val_make(CO_DEFTYPE_UNICODE_STRING, &val2,
                                               TEST_STR16_2.c_str(), 0));

  const auto ret = co_val_cmp(CO_DEFTYPE_UNICODE_STRING, &val1, &val2);

  CHECK_COMPARE(ret, >, 0);

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val1);
  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val2);
}

/// \Given two non-equal blocks of data (co_domain_t), the first block value
///        is greater than the second
///
/// \When co_val_cmp() is called with the block data type and pointers to the
///       blocks
///
/// \Then an integer greater than 0 is returned
///       \Calls co_type_is_array()
///       \Calls co_val_sizeof()
///       \Calls memcmp()
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

/// \Given two values of any data type
///
/// \When co_val_cmp() is called with an invalid data type and pointers to the
///       values
///
/// \Then 0 is returned
///       \Calls co_type_is_array()
TEST(CO_Val, CoValCmp_INVALID_TYPE) {
  int val1 = 0;
  int val2 = 0;

  CHECK_EQUAL(0, co_val_cmp(INVALID_TYPE, &val1, &val2));
}

///@}

/// @name co_val_read()
///@{

/// \Given a value of a basic data type, a memory buffer containing bytes
///        to be read
///
/// \When co_val_read() is called with the value data type, a pointer to the
///       value, pointers to the beginning and the end of the buffer
///
/// \Then the value size is returned, the value is set with bytes from the
///       buffer
///       \Calls co_type_is_array()
///       \Calls ldle_i16()
///       \Calls ldle_i32()
///       \Calls ldle_u16()
///       \Calls ldle_u32()
///       \Calls ldle_flt32()
///       \Calls ldle_flt64()
///       \Calls ldle_i64()
///       \Calls ldle_u24()
///       \Calls ldle_u64()
TEST(CO_Val, CoValRead_BasicTypes) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val = CO_##NAME##_INIT; \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##NAME); \
    const uint_least8_t buffer[CO_MAX_VAL_SIZE] = { \
        0x3eu, 0x18u, 0x67u, 0x7bu, 0x34u, 0x15u, 0x09u, 0x27u}; \
    CHECK_COMPARE(val_size, <=, CO_MAX_VAL_SIZE); \
\
    const auto ret = co_val_read(CO_DEFTYPE_##NAME, &val, buffer, \
                                 buffer + CO_MAX_VAL_SIZE); \
\
    CHECK_EQUAL(val_size, ret); \
    CHECK_EQUAL(ldle_##tag(buffer), val); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given a value of a basic data type, a memory buffer containing bytes
///        to be read, all bytes have the most significant bit set to `1`
///
/// \When co_val_read() is called with the value data type, a pointer to the
///       value, pointers to the beginning and the end of the buffer
///
/// \Then the value size is returned, the value is set with bytes from the
///       buffer with no value overflow
///       \Calls co_type_is_array()
///       \Calls ldle_i16()
///       \Calls ldle_i32()
///       \Calls ldle_u16()
///       \Calls ldle_u32()
///       \Calls ldle_flt32()
///       \Calls ldle_flt64()
///       \Calls ldle_i64()
///       \Calls ldle_u24()
///       \Calls ldle_u64()
TEST(CO_Val, CoValRead_BasicTypes_ValueOverflow) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val = CO_##NAME##_INIT; \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##NAME); \
    const uint_least8_t buffer[CO_MAX_VAL_SIZE] = { \
        0xfau, 0x83u, 0xb1, 0xf0u, 0xaau, 0xc4u, 0x88u, 0xe7u}; \
\
    const auto ret = co_val_read(CO_DEFTYPE_##NAME, &val, buffer, \
                                 buffer + CO_MAX_VAL_SIZE); \
\
    CHECK_EQUAL(val_size, ret); \
    CHECK_EQUAL(ldle_##tag(buffer), val); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given a value of the boolean data type, a memory buffer containing `0xff`
///        byte
///
/// \When co_val_read() is called with the boolean data type, a pointer to the
///       value, pointers to the beginning and the end of the buffer
///
/// \Then the boolean value size is returned, the value is set to `0x01`
///       \Calls co_type_is_array()
TEST(CO_Val, CoValRead_BOOLEAN_True) {
  co_boolean_t val = CO_BOOLEAN_MIN;
  const uint_least8_t buffer[] = {0xffu};

  const auto ret = co_val_read(CO_DEFTYPE_BOOLEAN, &val, buffer, buffer + 1u);

  CHECK_EQUAL(ValGetReadWriteSize(CO_DEFTYPE_BOOLEAN), ret);
  CHECK_EQUAL(0x01, val);
}

/// \Given a value of the boolean data type, a memory buffer containing `0x00`
///        byte
///
/// \When co_val_read() is called with the boolean data type, a pointer to the
///       value, pointers to the beginning and the end of the buffer
///
/// \Then the boolean data type size is returned, the value is set to `0x00`
///       \Calls co_type_is_array()
TEST(CO_Val, CoValRead_BOOLEAN_False) {
  co_boolean_t val = CO_BOOLEAN_MAX;
  const uint_least8_t buffer[] = {0x00};

  const auto ret = co_val_read(CO_DEFTYPE_BOOLEAN, &val, buffer, buffer + 1u);

  CHECK_EQUAL(ValGetReadWriteSize(CO_DEFTYPE_BOOLEAN), ret);
  CHECK_EQUAL(0x00, val);
}

/// \Given a value of a time data type, a memory buffer containing bytes to
///        be read
///
/// \When co_val_read() is called with the time data type, a pointer to the
///       value, pointers to the beginning and the end of the buffer
///
/// \Then the time data type size is returned, the value is set with the bytes
///       from the buffer
///       \Calls co_type_is_array()
///       \Calls ldle_u32()
///       \Calls ldle_u16()
TEST(CO_Val, CoValRead_TimeTypes) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val = CO_##NAME##_INIT; \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##NAME); \
    const size_t type_size = co_type_sizeof(CO_DEFTYPE_##NAME); \
    const uint_least8_t buffer[CO_MAX_VAL_SIZE] = { \
        0x3eu, 0x18u, 0x67u, 0x7bu, 0x34u, 0x15u, 0x00u, 0x00u}; \
    CHECK_COMPARE(val_size, <=, CO_MAX_VAL_SIZE); \
    CHECK_COMPARE(type_size, <=, CO_MAX_VAL_SIZE); \
\
    const auto ret = \
        co_val_read(CO_DEFTYPE_##NAME, &val, buffer, buffer + type_size); \
\
    CHECK_EQUAL(val_size, ret); \
    CHECK_EQUAL(ldle_u32(buffer) & UINT32_C(0x0fffffff), val.ms); \
    CHECK_EQUAL(ldle_u16(buffer + 4u), val.days); \
  }
#include <lely/co/def/time.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

#if CO_DEFTYPE_TIME_SCET
/// \Given a value of the SCET (Spacecraft Elapsed Time) data type, a memory
///        buffer containing bytes to be read
///
/// \When co_val_read() is called with the SCET data type, a pointer to the
///       value, pointers to the beginning and the end of the buffer
///
/// \Then the SCET value size is returned, the value is set with the bytes
///       from the buffer
///       \Calls co_type_is_array()
///       \Calls ldle_u24()
///       \Calls ldle_u32()
TEST(CO_Val, CoValRead_TIME_SCET) {
  co_time_scet_t val = CO_TIME_SCET_INIT;
  const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_TIME_SCET);
  const size_t type_size = co_type_sizeof(CO_DEFTYPE_TIME_SCET);
  const uint_least8_t buffer[MAX_VAL_SIZE_SCET] = {0x3eu, 0x18u, 0x67u, 0x7bu,
                                                   0x34u, 0x15u, 0x25u, 0x00u};
  CHECK_COMPARE(val_size, <=, MAX_VAL_SIZE_SCET);
  CHECK_COMPARE(type_size, <=, MAX_VAL_SIZE_SCET);

  const auto ret =
      co_val_read(CO_DEFTYPE_TIME_SCET, &val, buffer, buffer + type_size);

  CHECK_EQUAL(val_size, ret);
  CHECK_EQUAL(ldle_u24(buffer), val.subseconds);
  CHECK_EQUAL(ldle_u32(buffer + 3u), val.seconds);
}
#endif

#if CO_DEFTYPE_TIME_SUTC
/// \Given a value of the SUTC (Spacecraft Universal Time Coordinated) data
///        type, a memory buffer containing bytes to be read
///
/// \When co_val_read() is called with the SUTC data type, a pointer to the
///       value, pointers to the beginning and the end of the buffer
///
/// \Then the SUTC value size is returned, the value is set with the bytes
///       from the buffer
///       \Calls co_type_is_array()
///       \Calls ldle_u16()
///       \Calls ldle_u32()
TEST(CO_Val, CoValRead_TIME_SUTC) {
  co_time_sutc_t val = CO_TIME_SUTC_INIT;
  const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_TIME_SUTC);
  const size_t type_size = co_type_sizeof(CO_DEFTYPE_TIME_SUTC);
  const uint_least8_t buffer[MAX_VAL_SIZE_SUTC] = {
      0x3eu, 0x18u, 0x67u, 0x7bu, 0x34u, 0x15u,
      0x25u, 0xafu, 0x00u, 0x00u, 0x00u, 0x00u,
  };
  CHECK_COMPARE(val_size, <=, MAX_VAL_SIZE_SUTC);
  CHECK_COMPARE(type_size, <=, MAX_VAL_SIZE_SUTC);

  const auto ret =
      co_val_read(CO_DEFTYPE_TIME_SUTC, &val, buffer, buffer + type_size);

  CHECK_EQUAL(val_size, ret);
  CHECK_EQUAL(ldle_u16(buffer), val.usec);
  CHECK_EQUAL(ldle_u32(buffer + 2u), val.ms);
  CHECK_EQUAL(ldle_u16(buffer + 6u), val.days);
}
#endif

/// \Given a value of a non-array data type, a memory buffer smaller than
///        the value size
///
/// \When co_val_read() is called with the value data type, a pointer to the
///       value, pointers to the beginning and the end of the buffer
///
/// \Then 0 is returned, the value is left untouched
///       \Calls co_type_is_array()
TEST(CO_Val, CoValRead_NonArrayTypes_InvalidSize) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val = CO_##NAME##_INIT; \
    const uint_least8_t buffer = 0xff; \
\
    const auto ret = co_val_read(CO_DEFTYPE_##NAME, &val, &buffer, &buffer); \
\
    CHECK_EQUAL(0, ret); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#include <lely/co/def/time.def>   // NOLINT(build/include)
#include <lely/co/def/ecss.def>   // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given a memory buffer containing bytes to be read
///
/// \When co_val_read() is called with any non-array data type, a null value
///       pointer, pointers to the beginning and the end of the buffer
///
/// \Then the size of the data type is returned
///       \Calls co_type_is_array()
TEST(CO_Val, CoValRead_NonArrayTypes_NullValue) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##NAME); \
    const uint_least8_t buffer[MAX_VAL_SIZE] = {0}; \
\
    const auto ret = co_val_read(CO_DEFTYPE_##NAME, nullptr, buffer, \
                                 buffer + MAX_VAL_SIZE); \
\
    CHECK_EQUAL(val_size, ret); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#include <lely/co/def/time.def>   // NOLINT(build/include)
#include <lely/co/def/ecss.def>   // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given a memory buffer containing bytes to be read
///
/// \When co_val_read() is called with any array data type, a null value
///       pointer, pointers to the beginning and the end of the buffer
///
/// \Then the size of the memory buffer is returned
///       \Calls co_type_is_array()
TEST(CO_Val, CoValRead_ArrayTypes_NullValue) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    const size_t buf_size = 2u; \
    const uint_least8_t buffer[buf_size] = {0x00}; \
\
    const auto ret = \
        co_val_read(CO_DEFTYPE_##NAME, nullptr, buffer, buffer + buf_size); \
\
    CHECK_EQUAL(buf_size, ret); \
  }
#include <lely/co/def/array.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given a value of an array data type
///
/// \When co_val_read() is called with the array data type, a pointer to the
///       value and null beginning and end of the buffer pointers
///
/// \Then 0 is returned
///       \Calls co_type_is_array()
///       \Calls co_val_init_vs_n()
///       \Calls co_val_init_os()
///       \Calls co_val_init_us_n()
///       \Calls co_val_init_dom()
TEST(CO_Val, CoValRead_ArrayTypes_NullBuffer) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val = nullptr; \
\
    const auto ret = co_val_read(CO_DEFTYPE_##NAME, &val, nullptr, nullptr); \
\
    CHECK_EQUAL(0, ret); \
  }
#include <lely/co/def/array.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

#if LELY_NO_MALLOC
/// \Given a value of an array data type, a memory buffer containing bytes
///        to be read larger than `CO_ARRAY_CAPACITY`
///
/// \When co_val_read() is called with the array data type, a pointer to the
///       value and pointers to the beginning and the end of the buffer
///
/// \Then 0 is returned, the error number is set to ERRNUM_NOMEM
///       \Calls co_type_is_array()
///       \Calls co_val_init_vs_n()
///       \Calls co_val_init_os()
///       \Calls co_val_init_us_n()
///       \Calls co_val_init_dom()
TEST(CO_Val, CoValRead_ArrayTypes_Overflow) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val = arrays.Init<co_##name##_t>(); \
    const uint_least8_t buffer[CO_ARRAY_CAPACITY + 1u] = {0}; \
\
    const auto ret = \
        co_val_read(CO_DEFTYPE_##NAME, &val, buffer, buffer + sizeof(buffer)); \
\
    CHECK_EQUAL(0, ret); \
    CHECK_EQUAL(ERRNUM_NOMEM, get_errnum()); \
  }
#include <lely/co/def/array.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}
#endif

/// \Given an array of visible characters (co_visible_string_t), a memory
///        buffer containing bytes to be read
///
/// \When co_val_read() is called with the array data type, a pointer to the
///       array and pointers to the beginning and the end of the buffer
///
/// \Then the buffer size is returned, the array is filled with the bytes from
///       the buffer and `\0` at the end
///       \Calls co_type_is_array()
///       \Calls co_val_init_vs_n()
TEST(CO_Val, CoValRead_VISIBLE_STRING_Nominal) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();
  const size_t BUFFER_SIZE = 6u;
  const uint_least8_t buffer[BUFFER_SIZE] = {0x74u, 0x64u, 0x73u,
                                             0x74u, 0x31u, 0x21u};
  const auto ret = co_val_read(CO_DEFTYPE_VISIBLE_STRING, &val, buffer,
                               buffer + BUFFER_SIZE);

  CHECK_EQUAL(BUFFER_SIZE, ret);
  CHECK_EQUAL(BUFFER_SIZE, co_val_sizeof(CO_DEFTYPE_VISIBLE_STRING, &val));
  for (size_t i = 0; i < BUFFER_SIZE; ++i) CHECK_EQUAL(buffer[i], val[i]);
  CHECK_EQUAL('\0', val[BUFFER_SIZE]);

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

/// \Given an array of octets (co_octet_string_t), a memory buffer containing
///        bytes to be read
///
/// \When co_val_read() is called with the array data type, a pointer to the
///       array and pointers to the beginning and the end of the buffer
///
/// \Then the buffer size is returned, the array is filled with the bytes from
///       the buffer
///       \Calls co_type_is_array()
///       \Calls co_val_init_os()
TEST(CO_Val, CoValRead_OCTET_STRING_Nominal) {
  co_octet_string_t val = arrays.Init<co_octet_string_t>();
  const size_t BUFFER_SIZE = 5u;
  const uint_least8_t buffer[BUFFER_SIZE] = {0xd3u, 0xe5u, 0x98u, 0xbau, 0x96u};

  const auto ret =
      co_val_read(CO_DEFTYPE_OCTET_STRING, &val, buffer, buffer + BUFFER_SIZE);

  CHECK_EQUAL(BUFFER_SIZE, ret);
  CHECK_EQUAL(BUFFER_SIZE, co_val_sizeof(CO_DEFTYPE_OCTET_STRING, &val));
  for (size_t i = 0; i < BUFFER_SIZE; ++i) CHECK_EQUAL(buffer[i], val[i]);

  co_val_fini(CO_DEFTYPE_OCTET_STRING, &val);
}

/// \Given an array of 16-bit Unicode characters (co_unicode_string_t),
///        a memory buffer containing bytes to be read
///
/// \When co_val_read() is called with the array data type, a pointer to the
///       array and pointers to the beginning and the end of the buffer
///
/// \Then the buffer size is returned, the array is filled with the bytes from
///       the buffer and `\0` at the end
///       \Calls co_type_is_array()
///       \Calls co_val_init_us_n()
///       \Calls ldle_u16()
TEST(CO_Val, CoValRead_UNICODE_STRING_Nominal) {
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();
  const size_t BUFFER_SIZE = 6u;
  const uint_least8_t buffer[BUFFER_SIZE] = {0x74u, 0x64u, 0x73u,
                                             0x74u, 0x31u, 0x21u};
  const auto ret = co_val_read(CO_DEFTYPE_UNICODE_STRING, &val, buffer,
                               buffer + BUFFER_SIZE);

  CHECK_EQUAL(BUFFER_SIZE, ret);
  CHECK_EQUAL(BUFFER_SIZE, co_val_sizeof(CO_DEFTYPE_UNICODE_STRING, &val));
  size_t i = 0;
  for (size_t j = 0; i < BUFFER_SIZE / 2; ++i, j += 2)
    CHECK_EQUAL(ldle_u16(buffer + j), val[i]);
  CHECK_EQUAL(u'\0', val[i]);

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val);
}

/// \Given a block of data (co_domain_t), a memory buffer containing bytes to
///        be read
///
/// \When co_val_read() is called with the block data type, a pointer to the
///       block and pointers to the beginning and the end of the buffer
///
/// \Then the buffer size is returned, the block is filled with the bytes from
///       the buffer
///       \Calls co_type_is_array()
///       \Calls co_val_init_dom()
TEST(CO_Val, CoValRead_DOMAIN_Nominal) {
  co_domain_t val = arrays.Init<co_domain_t>();
  const size_t BUFFER_SIZE = 4u;
  const uint_least8_t buffer[BUFFER_SIZE] = {0xd3u, 0xe5u, 0x98u, 0xbau};

  const auto ret =
      co_val_read(CO_DEFTYPE_DOMAIN, &val, buffer, buffer + BUFFER_SIZE);

  CHECK_EQUAL(BUFFER_SIZE, ret);
  CHECK_EQUAL(BUFFER_SIZE, co_val_sizeof(CO_DEFTYPE_DOMAIN, &val));
  const auto* vbuf = static_cast<uint_least8_t*>(val);
  for (size_t i = 0; i < BUFFER_SIZE; ++i) CHECK_EQUAL(buffer[i], vbuf[i]);

  co_val_fini(CO_DEFTYPE_DOMAIN, &val);
}

/// \Given a memory buffer containing bytes to be read
///
/// \When co_val_read() is called with an invalid data type, a null value
///       pointer and pointers to the beginning and the end of the buffer
///
/// \Then 0 is returned, the error number is set to ERRNUM_INVAL
///       \Calls co_type_is_array()
TEST(CO_Val, CoValRead_INVALID_TYPE) {
  const uint_least8_t buffer = 0xff;

  const auto ret = co_val_read(INVALID_TYPE, nullptr, &buffer, &buffer);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

///@}

/// @name co_val_read_sdo()
///@{

/// \Given a pointer to a value, an SDO buffer containing a value
///
/// \When co_val_read_sdo() is called with the value data type, the pointer to
///       the value, a pointer to the buffer and a size of the buffer
///
/// \Then 0 is returned, the value is set with the bytes from the buffer
///       \Calls get_errc()
///       \Calls co_val_read()
TEST(CO_Val, CoValReadSdo_Nominal) {
  co_integer16_t val = CO_INTEGER16_INIT;
  const uint_least8_t buffer[] = {0x34u, 0x12u};

  const auto ret =
      co_val_read_sdo(CO_DEFTYPE_INTEGER16, &val, buffer, sizeof(buffer));

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, get_errnum());
  CHECK_EQUAL(ldle_i16(buffer), val);
}

/// \Given a pointer to a value, an SDO buffer containing a value
///
/// \When co_val_read_sdo() is called with the value data type, the pointer to
///       the value, a pointer to the buffer and zero size of the buffer
///
/// \Then 0 is returned, the value is left untouched
///       \Calls get_errc()
///       \Calls co_val_read()
TEST(CO_Val, CoValReadSdo_ZeroSizeBuffer) {
  co_integer16_t val = 0x1234;
  const uint_least8_t buffer[] = {0xcdu, 0xabu};

  const auto ret = co_val_read_sdo(CO_DEFTYPE_INTEGER16, &val, &buffer, 0);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(0, get_errnum());
  CHECK_EQUAL(0x1234, val);
}

/// \Given a pointer to a value, an SDO buffer containing a value
///
/// \When co_val_read_sdo() is called with the value data type, the pointer to
///       the value, a null buffer pointer and any size of the buffer
///
/// \Then CO_SDO_AC_ERROR is returned, the value is left untouched
///       \Calls get_errc()
///       \Calls co_val_read()
///       \Calls get_errnum()
///       \Calls set_errc()
TEST(CO_Val, CoValReadSdo_NullBufferPtr) {
  co_integer16_t val = 0x1234;

  const auto ret = co_val_read_sdo(CO_DEFTYPE_INTEGER16, &val, nullptr, 5u);

  CHECK_EQUAL(CO_SDO_AC_ERROR, ret);
  CHECK_EQUAL(0, get_errnum());
  CHECK_EQUAL(0x1234, val);
}

/// \Given a pointer to a value, an SDO buffer containing a value that is too
///        small
///
/// \When co_val_read_sdo() is called with the value data type, the pointer to
///       the value, a pointer to the buffer and a size of the buffer
///
/// \Then CO_SDO_AC_ERROR is returned, the error number is not changed and
///       the value is left untouched
///       \Calls get_errc()
///       \Calls co_val_read()
///       \Calls get_errnum()
///       \Calls set_errc()
TEST(CO_Val, CoValReadSdo_TooSmallBuffer) {
  co_integer16_t val = 0x1234;
  const uint_least8_t buffer[] = {0xaau};
  set_errnum(ERRNUM_BUSY);  // unrelated error

  const auto ret =
      co_val_read_sdo(CO_DEFTYPE_INTEGER16, &val, buffer, sizeof(buffer));

  CHECK_EQUAL(CO_SDO_AC_ERROR, ret);
  CHECK_EQUAL(ERRNUM_BUSY, get_errnum());
  CHECK_EQUAL(0x1234, val);
}

#if LELY_NO_MALLOC
/// \Given a pointer to a value of any array data type, an SDO buffer
///        containing a value larger than `CO_ARRAY_CAPACITY`
///
/// \When co_val_read_sdo() is called with the value data type, the pointer to
///       the value, a pointer to the buffer and a size of the buffer
///
/// \Then CO_SDO_AC_NO_MEM is returned, the error number is not changed and
///       the value is left untouched
///       \Calls get_errc()
///       \Calls co_val_read()
///       \Calls get_errnum()
///       \Calls set_errc()
TEST(CO_Val, CoValReadSdo_NoMemory) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();
  const uint_least8_t buffer[CO_ARRAY_CAPACITY + 1u] = {0};
  set_errnum(ERRNUM_BUSY);  // unrelated error

  const auto ret =
      co_val_read_sdo(CO_DEFTYPE_VISIBLE_STRING, &val, buffer, sizeof(buffer));

  CHECK_EQUAL(CO_SDO_AC_NO_MEM, ret);
  CHECK_EQUAL(ERRNUM_BUSY, get_errnum());
  CHECK(arrays.IsEmptyInitialized(val));
}
#endif

///@}

/// @name co_val_write()
///@{

/// \Given a value of a basic data type, a memory buffer large enough to
///        store the value
///
/// \When co_val_write() is called with the value data type, a pointer to the
///       value, pointers to the beginning and the end of the buffer
///
/// \Then the value size is returned, the value is written to the buffer
///       \Calls co_type_is_array()
///       \Calls stle_i16()
///       \Calls stle_i32()
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls stle_flt32()
///       \Calls stle_flt64()
///       \Calls stle_i64()
///       \Calls stle_u24()
///       \Calls stle_u64()
TEST(CO_Val, CoValWrite_BasicTypes) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    const co_##name##_t val = CO_##NAME##_MAX; \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##NAME); \
    CHECK_COMPARE(val_size, <=, CO_MAX_VAL_SIZE); \
    uint_least8_t buffer[CO_MAX_VAL_SIZE] = {0x00}; \
\
    const auto ret = co_val_write(CO_DEFTYPE_##NAME, &val, buffer, \
                                  buffer + CO_MAX_VAL_SIZE); \
\
    CHECK_EQUAL(val_size, ret); \
    uint_least8_t vbuf[CO_MAX_VAL_SIZE] = {0x00}; \
    stle_##tag(vbuf, val); \
    for (size_t i = 0; i < CO_MAX_VAL_SIZE; ++i) \
      CHECK_EQUAL(vbuf[i], buffer[i]); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given a value of a basic data type, a memory buffer large enough to
///        store the value
///
/// \When co_val_write() is called with the value data type, a pointer to the
///       value, a pointer to the beginning of the buffer and a null end of
///       the buffer pointer
///
/// \Then the value size is returned, the value is written to the buffer
///       \Calls co_type_is_array()
///       \Calls stle_i16()
///       \Calls stle_i32()
///       \Calls stle_u16()
///       \Calls stle_u32()
///       \Calls stle_flt32()
///       \Calls stle_flt64()
///       \Calls stle_i64()
///       \Calls stle_u24()
///       \Calls stle_u64()
TEST(CO_Val, CoValWrite_BasicTypes_NoEnd) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    const co_##name##_t val = CO_##NAME##_MIN; \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##NAME); \
    CHECK_COMPARE(val_size, <=, CO_MAX_VAL_SIZE); \
    uint_least8_t buffer[CO_MAX_VAL_SIZE] = {0x00}; \
\
    const auto ret = co_val_write(CO_DEFTYPE_##NAME, &val, buffer, nullptr); \
\
    CHECK_EQUAL(val_size, ret); \
    uint_least8_t vbuf[CO_MAX_VAL_SIZE] = {0x00}; \
    stle_##tag(vbuf, val); \
    for (size_t i = 0; i < CO_MAX_VAL_SIZE; ++i) \
      CHECK_EQUAL(vbuf[i], buffer[i]); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given a value of the boolean data type equal to `TRUE`, a memory buffer
///        large enough to store the value
///
/// \When co_val_write() is called with the boolean data type, a pointer to the
///       value, a pointer to the beginning of the buffer and a null end of the
///       buffer pointer
///
/// \Then the boolean value size is returned, `0x01` is written to the buffer
///       \Calls co_type_is_array()
TEST(CO_Val, CoValWrite_BOOLEAN_True) {
  const co_boolean_t val = CO_BOOLEAN_MAX;
  uint_least8_t buffer[] = {0x00};

  const auto ret = co_val_write(CO_DEFTYPE_BOOLEAN, &val, buffer, nullptr);

  CHECK_EQUAL(ValGetReadWriteSize(CO_DEFTYPE_BOOLEAN), ret);
  CHECK_EQUAL(0x01u, buffer[0]);
}

/// \Given a value of the boolean data type equal to `FALSE`, a memory buffer
///        large enough to store the value
///
/// \When co_val_write() is called with the boolean data type, a pointer to the
///       value, a pointer to the beginning of the buffer and a null end of the
///       buffer pointer
///
/// \Then the boolean value size is returned, `0x00` is written to the buffer
///       \Calls co_type_is_array()
TEST(CO_Val, CoValWrite_BOOLEAN_False) {
  const co_boolean_t val = CO_BOOLEAN_MIN;
  uint_least8_t buffer[] = {0xffu};

  const auto ret = co_val_write(CO_DEFTYPE_BOOLEAN, &val, buffer, nullptr);

  CHECK_EQUAL(ValGetReadWriteSize(CO_DEFTYPE_BOOLEAN), ret);
  CHECK_EQUAL(0x00, buffer[0]);
}

/// \Given a value of a time data type, a memory buffer large enough to
///        store the value
///
/// \When co_val_write() is called with the value data type, a pointer to the
///       value, pointers to the beginning and the end of the buffer
///
/// \Then the value size is returned, the value is written to the buffer
///       \Calls co_type_is_array()
///       \Calls stle_u32()
///       \Calls stle_u16()
TEST(CO_Val, CoValWrite_TimeTypes) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val = CO_##NAME##_INIT; \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##NAME); \
    uint_least8_t buffer[CO_MAX_VAL_SIZE] = {0x00}; \
    val.ms = 0x0b67183eu; \
    val.days = 0x1534u; \
\
    CHECK_COMPARE(val_size, <=, CO_MAX_VAL_SIZE); \
\
    const auto ret = co_val_write(CO_DEFTYPE_##NAME, &val, buffer, \
                                  buffer + CO_MAX_VAL_SIZE); \
\
    CHECK_EQUAL(val_size, ret); \
    CHECK_EQUAL(val.ms, ldle_u32(buffer) & UINT32_C(0x0fffffff)); \
    CHECK_EQUAL(val.days, ldle_u16(buffer + 4u)); \
  }
#include <lely/co/def/time.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given a value of a time data type, a memory buffer large enough to
///        store the value
///
/// \When co_val_write() is called with the value data type, a pointer to the
///       value, a pointer to the beginning of the buffer and a null end of the
///       buffer pointer
///
/// \Then the value size is returned, the value is written to the buffer
///       \Calls co_type_is_array()
///       \Calls stle_u32()
///       \Calls stle_u16()
TEST(CO_Val, CoValWrite_TimeTypes_NoEnd) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    co_##name##_t val = CO_##NAME##_INIT; \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##NAME); \
    uint_least8_t buffer[CO_MAX_VAL_SIZE] = {0x00}; \
    val.ms = 0x0b67183eu; \
    val.days = 0x1534u; \
\
    CHECK_COMPARE(val_size, <=, CO_MAX_VAL_SIZE); \
\
    const auto ret = co_val_write(CO_DEFTYPE_##NAME, &val, buffer, nullptr); \
\
    CHECK_EQUAL(val_size, ret); \
    CHECK_EQUAL(val.ms, ldle_u32(buffer) & UINT32_C(0x0fffffff)); \
    CHECK_EQUAL(val.days, ldle_u16(buffer + 4u)); \
  }
#include <lely/co/def/time.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

#if CO_DEFTYPE_TIME_SCET

/// \Given a value of the SCET (Spacecraft Elapsed Time) data type, a memory
///        buffer large enough to store the value
///
/// \When co_val_write() is called with the SCET data type, a pointer to the
///       value, pointers to the beginning and the end of the buffer
///
/// \Then the SCET value size is returned, the value is written to the buffer
///       \Calls co_type_is_array()
///       \Calls stle_u24()
///       \Calls stle_u32()
TEST(CO_Val, CoValWrite_TIME_SCET) {
  co_time_scet_t val = CO_TIME_SCET_INIT;
  const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_TIME_SCET);
  uint_least8_t buffer[MAX_VAL_SIZE_SCET] = {0x00};
  val.subseconds = 0xaf1534u;
  val.seconds = 0x1b67183eu;

  CHECK_COMPARE(val_size, <=, MAX_VAL_SIZE_SCET);

  const auto ret = co_val_write(CO_DEFTYPE_TIME_SCET, &val, buffer,
                                buffer + MAX_VAL_SIZE_SCET);

  CHECK_EQUAL(val_size, ret);
  CHECK_EQUAL(val.subseconds, ldle_u24(buffer));
  CHECK_EQUAL(val.seconds, ldle_u32(buffer + 3u));
}

/// \Given a value of the SCET (Spacecraft Elapsed Time) data type, a memory
///        buffer large enough to store the value
///
/// \When co_val_write() is called with the SCET data type, a pointer to the
///       value, a pointer to the beginning of the buffer and a null end of the
///       buffer pointer
///
/// \Then the SCET value size is returned, the value is written to the buffer
///       \Calls co_type_is_array()
///       \Calls stle_u24()
///       \Calls stle_u32()
TEST(CO_Val, CoValWrite_TIME_SCET_NoEnd) {
  co_time_scet_t val = CO_TIME_SCET_INIT;
  const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_TIME_SCET);
  uint_least8_t buffer[MAX_VAL_SIZE_SCET] = {0x00};
  val.subseconds = 0xaf1534u;
  val.seconds = 0x1b67183eu;

  CHECK_COMPARE(val_size, <=, MAX_VAL_SIZE_SCET);

  const auto ret = co_val_write(CO_DEFTYPE_TIME_SCET, &val, buffer, nullptr);

  CHECK_EQUAL(val_size, ret);
  CHECK_EQUAL(val.subseconds, ldle_u24(buffer));
  CHECK_EQUAL(val.seconds, ldle_u32(buffer + 3u));
}

#endif  // CO_DEFTYPE_TIME_SCET

#if CO_DEFTYPE_TIME_SUTC

/// \Given a value of the SUTC (Spacecraft Universal Time Coordinated) data
///        type, a memory buffer large enough to store the value
///
/// \When co_val_write() is called with the SUTC data type, a pointer to the
///       value, pointers to the beginning and the end of the buffer
///
/// \Then the SUTC value size is returned, the value is written to the buffer
///       \Calls co_type_is_array()
///       \Calls stle_u16()
///       \Calls stle_u32()
TEST(CO_Val, CoValWrite_TIME_SUTC) {
  co_time_sutc_t val = CO_TIME_SUTC_INIT;
  const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_TIME_SUTC);
  uint_least8_t buffer[MAX_VAL_SIZE_SUTC] = {0x00};
  val.usec = 0x5819u;
  val.ms = 0x1b67183eu;
  val.days = 0x1534u;

  CHECK_COMPARE(val_size, <=, MAX_VAL_SIZE_SUTC);

  const auto ret = co_val_write(CO_DEFTYPE_TIME_SUTC, &val, buffer,
                                buffer + MAX_VAL_SIZE_SUTC);

  CHECK_EQUAL(val_size, ret);
  CHECK_EQUAL(val.usec, ldle_u16(buffer));
  CHECK_EQUAL(val.ms, ldle_u32(buffer + 2u));
  CHECK_EQUAL(val.days, ldle_u16(buffer + 6u));
}

/// \Given a value of the SUTC (Spacecraft Universal Time Coordinated) data
///        type, a memory buffer large enough to store the value
///
/// \When co_val_write() is called with the SUTC data type, a pointer to the
///       value, a pointer to the beginning of the buffer and a null end of the
///       buffer pointer
///
/// \Then the SUTC value size is returned, the value is written to the buffer
///       \Calls co_type_is_array()
///       \Calls stle_u16()
///       \Calls stle_u32()
TEST(CO_Val, CoValWrite_TIME_SUTC_NoEnd) {
  co_time_sutc_t val = CO_TIME_SUTC_INIT;
  const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_TIME_SUTC);
  uint_least8_t buffer[MAX_VAL_SIZE_SUTC] = {0x00};
  CHECK_EQUAL(0, co_val_init(CO_DEFTYPE_TIME_SUTC, &val));
  val.usec = 0x5819u;
  val.ms = 0x1b67183eu;
  val.days = 0x1534u;

  CHECK_COMPARE(val_size, <=, MAX_VAL_SIZE_SUTC);

  const auto ret = co_val_write(CO_DEFTYPE_TIME_SUTC, &val, buffer, nullptr);

  CHECK_EQUAL(val_size, ret);
  CHECK_EQUAL(val.usec, ldle_u16(buffer));
  CHECK_EQUAL(val.ms, ldle_u32(buffer + 2u));
  CHECK_EQUAL(val.days, ldle_u16(buffer + 6u));
}

#endif  // CO_DEFTYPE_TIME_SUTC

/// \Given a value of any non-array data type
///
/// \When co_val_write() is called with the value data type, a pointer to the
///       value, null beginning and end of the buffer pointers
///
/// \Then the value size is returned
///       \Calls co_type_is_array()
TEST(CO_Val, CoValWrite_NonArrayTypes_NullBuffer) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    const co_##name##_t val = CO_##NAME##_INIT; \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##NAME); \
\
    const auto ret = co_val_write(CO_DEFTYPE_##NAME, &val, nullptr, nullptr); \
\
    CHECK_EQUAL(val_size, ret); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#include <lely/co/def/time.def>   // NOLINT(build/include)
#include <lely/co/def/ecss.def>   // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given a value of a non-array data type, a memory buffer too small to
///        store the value
///
/// \When co_val_write() is called with the value data type, a pointer to the
///       value, pointers to the beginning and the end of the buffer
///
/// \Then the value size is returned
///       \Calls co_type_is_array()
TEST(CO_Val, CoValWrite_NonArrayTypes_InvalidSize) {
  (void)0;  // clang-format fix
#define LELY_CO_DEFINE_TYPE(NAME, name, tag, type) \
  { \
    const co_##name##_t val = CO_##NAME##_INIT; \
    const size_t val_size = ValGetReadWriteSize(CO_DEFTYPE_##NAME); \
    uint_least8_t buffer[MAX_VAL_SIZE] = {0x00}; \
\
    const auto ret = co_val_write(CO_DEFTYPE_##NAME, &val, buffer, buffer); \
\
    CHECK_EQUAL(val_size, ret); \
    for (size_t i = 0; i < MAX_VAL_SIZE; ++i) CHECK_EQUAL(0x00, buffer[i]); \
  }
#include <lely/co/def/basic.def>  // NOLINT(build/include)
#include <lely/co/def/time.def>   // NOLINT(build/include)
#include <lely/co/def/ecss.def>   // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given a value of any array data type initialized with a null pointer,
///        a memory buffer large enough to store the value
///
/// \When co_val_write() is called with the array data type, a pointer to the
///       value, pointers to the beginning and the end of the buffer
///
/// \Then 0 is returned, nothing is written
///       \Calls co_type_is_array()
///       \Calls co_val_addressof()
///       \Calls co_val_sizeof()
TEST(CO_Val, CoValWrite_NullArray) {
  const size_t ARRAY_SIZE = 5u;
  const co_visible_string_t val = nullptr;
  uint_least8_t buffer[ARRAY_SIZE] = {0xffu, 0xffu, 0xffu, 0xffu, 0xffu};

  const auto ret = co_val_write(CO_DEFTYPE_VISIBLE_STRING, &val, buffer,
                                buffer + ARRAY_SIZE);

  CHECK_EQUAL(0, ret);
  for (size_t i = 0; i < ARRAY_SIZE; ++i) CHECK_EQUAL(0xffu, buffer[i]);
}

/// \Given an array of visible characters (co_visible_string_t), a memory
///        buffer large enough to store the value
///
/// \When co_val_read() is called with the array data type, a pointer to the
///       array, pointers to the beginning and the end of the buffer
///
/// \Then the array size is returned, the array is written to the buffer
///       without the terminating null character
///       \Calls co_type_is_array()
///       \Calls co_val_addressof()
///       \Calls co_val_sizeof()
///       \Calls memcpy()
TEST(CO_Val, CoValWrite_VISIBLE_STRING_Nominal) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();
  const size_t ARRAY_SIZE = 5u;
  const char test_str[ARRAY_SIZE + 1u] = "abcde";
  CHECK_EQUAL(0, co_val_init_vs(&val, test_str));
  uint_least8_t buffer[ARRAY_SIZE + 1u] = {0x00};
  buffer[ARRAY_SIZE] = 0xffu;

  const auto ret = co_val_write(CO_DEFTYPE_VISIBLE_STRING, &val, buffer,
                                buffer + ARRAY_SIZE);

  CHECK_EQUAL(ARRAY_SIZE, ret);
  for (size_t i = 0; i < ARRAY_SIZE; ++i) CHECK_EQUAL(buffer[i], test_str[i]);
  CHECK_EQUAL(0xffu, buffer[ARRAY_SIZE]);

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

/// \Given an array of octets (co_octet_string_t), a memory buffer large enough
///        to store the value
///
/// \When co_val_read() is called with the array data type, a pointer to the
///       array, pointers to the beginning and the end of the buffer
///
/// \Then the array size is returned, the array is written to the buffer
///       \Calls co_type_is_array()
///       \Calls co_val_addressof()
///       \Calls co_val_sizeof()
///       \Calls memcpy()
TEST(CO_Val, CoValWrite_OCTET_STRING_Nominal) {
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

/// \Given an array of 16-bit Unicode characters (co_unicode_string_t),
///        a memory buffer large enough to store the value
///
/// \When co_val_read() is called with the array data type, a pointer to the
///       array, pointers to the beginning and the end of the buffer
///
/// \Then the array size (in bytes) is returned, the array is written to the
///       buffer without the terminating null character
///       \Calls co_type_is_array()
///       \Calls co_val_addressof()
///       \Calls co_val_sizeof()
///       \Calls stle_u16()
TEST(CO_Val, CoValWrite_UNICODE_STRING_Nominal) {
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();
  const size_t ARRAY_SIZE = 6u;
  const char16_t test_str[ARRAY_SIZE / sizeof(char16_t) + 1u] = u"xyz";
  CHECK_EQUAL(0, co_val_init_us(&val, test_str));
  uint_least8_t buffer[ARRAY_SIZE + sizeof(char16_t)] = {0x00};
  memset(buffer + ARRAY_SIZE, 0xff, sizeof(char16_t));

  const auto ret = co_val_write(CO_DEFTYPE_UNICODE_STRING, &val, buffer,
                                buffer + ARRAY_SIZE);

  CHECK_EQUAL(ARRAY_SIZE, ret);
  for (size_t i = 0, j = 0; i < ARRAY_SIZE; i += 2u, ++j)
    CHECK_EQUAL(ldle_u16(buffer + i), test_str[j]);
  CHECK_EQUAL(0xffu, buffer[ARRAY_SIZE]);
  CHECK_EQUAL(0xffu, buffer[ARRAY_SIZE + 1u]);

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val);
}

/// \Given a block of data (co_domain_t), a memory buffer large enough to store
///        the value
///
/// \When co_val_read() is called with the block data type, a pointer to the
///       block, pointers to the beginning and the end of the buffer
///
/// \Then the array size is returned, the block is written to the buffer
///       \Calls co_type_is_array()
///       \Calls co_val_addressof()
///       \Calls co_val_sizeof()
///       \Calls memcpy()
TEST(CO_Val, CoValWrite_DOMAIN_Nominal) {
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

/// \Given an array of visible characters (co_visible_string_t), a memory
///        buffer large enough to store the value
///
/// \When co_val_read() is called with the array data type, a pointer to the
///       array, a pointer to the beginning of the buffer and a null end of the
///       buffer pointer
///
/// \Then the array size is returned, the array is written to the buffer
///       without the terminating null character
///       \Calls co_type_is_array()
///       \Calls co_val_addressof()
///       \Calls co_val_sizeof()
///       \Calls memcpy()
TEST(CO_Val, CoValWrite_VISIBLE_STRING_NoEnd) {
  co_visible_string_t val = arrays.Init<co_visible_string_t>();
  const size_t ARRAY_SIZE = 7u;
  const char test_str[ARRAY_SIZE + 1u] = "qwerty7";
  CHECK_EQUAL(0, co_val_init_vs(&val, test_str));
  uint_least8_t buffer[ARRAY_SIZE + 1u] = {0x00};
  buffer[ARRAY_SIZE] = 0xffu;

  const auto ret =
      co_val_write(CO_DEFTYPE_VISIBLE_STRING, &val, buffer, nullptr);

  CHECK_EQUAL(ARRAY_SIZE, ret);
  for (size_t i = 0; i < ARRAY_SIZE; ++i) CHECK_EQUAL(buffer[i], test_str[i]);
  CHECK_EQUAL(0xffu, buffer[ARRAY_SIZE]);

  co_val_fini(CO_DEFTYPE_VISIBLE_STRING, &val);
}

/// \Given an array of octets (co_octet_string_t)
///
/// \When co_val_read() is called with the array data type, a pointer to the
///       array, null beginning and end of the buffer pointers
///
/// \Then the array size is returned
///       \Calls co_type_is_array()
///       \Calls co_val_addressof()
///       \Calls co_val_sizeof()
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

/// \Given an array of 16-bit Unicode characters (co_unicode_string_t),
///        a memory buffer too small to store the value
///
/// \When co_val_read() is called with the array data type, a pointer to the
///       array, pointers to the beginning and the end of the buffer
///
/// \Then the array size (in bytes) is returned, nothing is written
///       \Calls co_type_is_array()
///       \Calls co_val_addressof()
///       \Calls co_val_sizeof()
TEST(CO_Val, CoValWrite_UNICODE_STRING_TooSmallBuffer) {
  co_unicode_string_t val = arrays.Init<co_unicode_string_t>();
  const size_t ARRAY_SIZE = 6u;
  const char16_t test_str[ARRAY_SIZE / sizeof(char16_t) + 1u] = u"xyz";
  CHECK_EQUAL(0, co_val_init_us(&val, test_str));
  uint_least8_t buffer[ARRAY_SIZE] = {0x00};

  const auto ret = co_val_write(CO_DEFTYPE_UNICODE_STRING, &val, buffer,
                                buffer + ARRAY_SIZE - 1);

  CHECK_EQUAL(ARRAY_SIZE, ret);
  for (size_t i = 0; i < ARRAY_SIZE; ++i) CHECK_EQUAL(0x00, buffer[i]);

  co_val_fini(CO_DEFTYPE_UNICODE_STRING, &val);
}

/// \Given a block of data (co_domain_t) of zero size
///
/// \When co_val_read() is called with the block data type, a pointer to the
///       block, null beginning and end of the buffer pointers
///
/// \Then 0 is returned
///       \Calls co_type_is_array()
///       \Calls co_val_addressof()
///       \Calls co_val_sizeof()
TEST(CO_Val, CoValWrite_DOMAIN_SizeZeroArray) {
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

/// \Given a value of any data type
///
/// \When co_val_write() is called with an invalid data type, a pointer to the
///       value, pointers to the beginning and the end of the buffer
///
/// \Then 0 is returned, the error number is set to ERRNUM_INVAL
///       \Calls co_type_is_array()
///       \Calls set_errnum()
TEST(CO_Val, CoValWrite_INVALID_TYPE) {
  co_integer16_t val = 0x1234;
  uint_least8_t buffer[CO_MAX_VAL_SIZE] = {0x00};

  const auto ret = co_val_write(INVALID_TYPE, &val, buffer, buffer);

  CHECK_EQUAL(0, ret);
  CHECK_EQUAL(ERRNUM_INVAL, get_errnum());
}

///@}

/// @name co_val_init_array()
///@{

#if LELY_NO_MALLOC

/// \Given a value of any array data type, a CANopen array (struct co_array)
///
/// \When co_val_init_array() is called with a pointer to the value and
///       a pointer to the CANopen array
///
/// \Then the value is set to the CANopen array data field
TEST(CO_Val, CoValInitArray_Nominal) {
  co_array array = CO_ARRAY_INIT;
  co_visible_string_t val;

  co_val_init_array(&val, &array);

  POINTERS_EQUAL(array.u.data, val);
}

/// \Given a CANopen array (struct co_array)
///
/// \When co_val_init_array() is called with a null value pointer and a pointer
///       to the CANopen array
///
/// \Then nothing is changed
TEST(CO_Val, CoValInitArray_NullValue) {
  co_array array = CO_ARRAY_INIT;
  co_visible_string_t val = nullptr;

  co_val_init_array(nullptr, &array);

  POINTERS_EQUAL(nullptr, val);
}

/// \Given a value of any array data type
///
/// \When co_val_init_array() is called with the value pointer and a null
///       CANopen array pointer
///
/// \Then the value is set to a null poniter
TEST(CO_Val, CoValInitArray_NullArray) {
  co_visible_string_t val;

  co_val_init_array(&val, nullptr);

  POINTERS_EQUAL(nullptr, val);
}

#endif  // LELY_NO_MALLOC

///@}
