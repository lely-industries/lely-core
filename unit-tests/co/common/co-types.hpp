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

#ifndef LELY_UNIT_TEST_COMMON_CO_TYPES_HPP_
#define LELY_UNIT_TEST_COMMON_CO_TYPES_HPP_

#include <lely/co/type.h>

/// Provides CANopen data type from a given DEFTYPE value.
template <co_unsigned16_t deftype>
struct CoType;

#define LELY_CO_DEFINE_TYPE(NAME, name, tag, c_type) \
  template <> \
  struct CoType<CO_DEFTYPE_##NAME> { \
    using type = co_##name##_t; \
  };
#include <lely/co/def/basic.def>
#undef LELY_CO_DEFINE_TYPE

#endif  // LELY_UNIT_TEST_COMMON_CO_TYPES_HPP_
