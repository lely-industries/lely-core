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

#ifndef LELY_UNIT_TESTS_OBJ_INIT_HPP_
#define LELY_UNIT_TESTS_OBJ_INIT_HPP_

#include <lely/co/type.h>

#include "common/co-types.hpp"

/// CANopen object initialization information template class
template <co_unsigned16_t Idx, co_unsigned16_t Min_idx = Idx,
          co_unsigned16_t Max_idx = Idx>
struct ObjInitT {
  static const co_unsigned16_t idx = Idx;
  static const co_unsigned16_t min_idx = Min_idx;
  static const co_unsigned16_t max_idx = Max_idx;

  /// CANopen sub-object initialization information template class
  template <co_unsigned8_t Subidx, co_unsigned16_t Deftype,
            typename CoType<Deftype>::type Default_val = 0,
            co_unsigned8_t Min_subidx = 0>
  struct SubT {
    static const co_unsigned16_t min_idx = ObjInitT::min_idx;
    static const co_unsigned16_t max_idx = ObjInitT::max_idx;

    using sub_type = typename CoType<Deftype>::type;
    static const co_unsigned8_t subidx = Subidx;
    static const co_unsigned8_t min_subidx = Min_subidx;
    static const co_unsigned16_t deftype = Deftype;
    static const sub_type default_val = Default_val;
  };
};

/// Single-entry CANopen object initialization information template class
template <co_unsigned16_t Idx, co_unsigned16_t Deftype,
          typename CoType<Deftype>::type Default_val = 0>
struct ObjValueInitT
    : public ObjInitT<Idx>,
      ObjInitT<Idx>::template SubT<0x00, Deftype, Default_val> {
  static const co_unsigned16_t min_idx = Idx;
  static const co_unsigned16_t max_idx = Idx;
};

#endif  // LELY_UNIT_TESTS_OBJ_INIT_HPP_
