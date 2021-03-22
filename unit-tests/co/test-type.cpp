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

#include <CppUTest/TestHarness.h>

#include <lely/co/type.h>

TEST_GROUP(CO_Type) { static const co_unsigned16_t INVALID_TYPE = 0xffff; };

/// @name co_type_is_basic()
///@{

/// \Given N/A
///
/// \When co_type_is_basic() is called with a basic DEFTYPE
///
/// \Then 1 is returned
TEST(CO_Type, CoTypeIsBasic_True) {
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_BOOLEAN));
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_INTEGER8));
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_INTEGER16));
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_INTEGER32));
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_UNSIGNED8));
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_UNSIGNED16));
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_UNSIGNED32));
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_REAL32));
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_REAL64));
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_INTEGER40));
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_INTEGER48));
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_INTEGER56));
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_INTEGER64));
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_UNSIGNED24));
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_UNSIGNED40));
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_UNSIGNED48));
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_UNSIGNED56));
  CHECK_EQUAL(1, co_type_is_basic(CO_DEFTYPE_UNSIGNED64));
}

/// \Given N/A
///
/// \When co_type_is_basic() is called with a non-basic DEFTYPE
///
/// \Then 0 is returned
TEST(CO_Type, CoTypeIsBasic_False) {
  CHECK_EQUAL(0, co_type_is_basic(CO_DEFTYPE_TIME_OF_DAY));
  CHECK_EQUAL(0, co_type_is_basic(CO_DEFTYPE_TIME_DIFF));
#if CO_DEFTYPE_TIME_SCET
  CHECK_EQUAL(0, co_type_is_basic(CO_DEFTYPE_TIME_SCET));
#endif
#if CO_DEFTYPE_TIME_SUTC
  CHECK_EQUAL(0, co_type_is_basic(CO_DEFTYPE_TIME_SUTC));
#endif
  CHECK_EQUAL(0, co_type_is_basic(CO_DEFTYPE_VISIBLE_STRING));
  CHECK_EQUAL(0, co_type_is_basic(CO_DEFTYPE_OCTET_STRING));
  CHECK_EQUAL(0, co_type_is_basic(CO_DEFTYPE_UNICODE_STRING));
  CHECK_EQUAL(0, co_type_is_basic(CO_DEFTYPE_DOMAIN));
  CHECK_EQUAL(0, co_type_is_basic(INVALID_TYPE));
}

///@}

/// @name co_type_is_array()
///@{

/// \Given N/A
///
/// \When co_type_is_array() is called with an array DEFTYPE
///
/// \Then 1 is returned
TEST(CO_Type, CoTypeIsArray_True) {
  CHECK_EQUAL(1, co_type_is_array(CO_DEFTYPE_VISIBLE_STRING));
  CHECK_EQUAL(1, co_type_is_array(CO_DEFTYPE_OCTET_STRING));
  CHECK_EQUAL(1, co_type_is_array(CO_DEFTYPE_UNICODE_STRING));
  CHECK_EQUAL(1, co_type_is_array(CO_DEFTYPE_DOMAIN));
}

/// \Given N/A
///
/// \When co_type_is_array() is called with a non-array DEFTYPE
///
/// \Then 0 is returned
TEST(CO_Type, CoTypeIsArray_False) {
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_BOOLEAN));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_INTEGER8));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_INTEGER16));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_INTEGER32));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_UNSIGNED8));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_UNSIGNED16));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_UNSIGNED32));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_REAL32));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_REAL64));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_INTEGER40));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_INTEGER48));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_INTEGER56));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_INTEGER64));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_UNSIGNED24));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_UNSIGNED40));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_UNSIGNED48));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_UNSIGNED56));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_UNSIGNED64));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_TIME_OF_DAY));
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_TIME_DIFF));
#if CO_DEFTYPE_TIME_SCET
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_TIME_SCET));
#endif
#if CO_DEFTYPE_TIME_SUTC
  CHECK_EQUAL(0, co_type_is_array(CO_DEFTYPE_TIME_SUTC));
#endif
  CHECK_EQUAL(0, co_type_is_array(INVALID_TYPE));
}

///@}

/// @name co_type_sizeof()
///@{

/// \Given N/A
///
/// \When co_type_sizeof() is called with a DEFTYPE
///
/// \Then the size of the type is returned
TEST(CO_Type, CoTypeSizeof) {
#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  CHECK_EQUAL(sizeof(co_##b##_t), co_type_sizeof(CO_DEFTYPE_##a));
#include <lely/co/def/type.def>
#undef LELY_CO_DEFINE_TYPE
}

/// \Given N/A
///
/// \When co_type_sizeof() is called with an invalid DEFTYPE
///
/// \Then 0 is returned
TEST(CO_Type, CoTypeSizeof_InvalidType) {
  CHECK_EQUAL(0, co_type_sizeof(INVALID_TYPE));
}

///@}

/// @name co_type_alignof()
///@{

/// \Given N/A
///
/// \When co_type_alignof() is called with a DEFTYPE
///
/// \Then alignment of the type is returned
TEST(CO_Type, CoTypeAlignof) {
#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
  CHECK_EQUAL(alignof(co_##b##_t), co_type_alignof(CO_DEFTYPE_##a));
#include <lely/co/def/type.def>  // NOLINT(build/include)
#undef LELY_CO_DEFINE_TYPE
}

/// \Given N/A
///
/// \When co_type_alignof() is called with an invalid DEFTYPE
///
/// \Then 0 is returned
TEST(CO_Type, CoTypeAlignof_InvalidType) {
  CHECK_EQUAL(1, co_type_alignof(INVALID_TYPE));
}

///@}
