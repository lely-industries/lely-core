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

#include <CppUTest/TestHarness.h>

#include <lely/co/val.h>

#include "co-rpdo-err.hpp"

size_t CoRpdoErr::num_called_ = 0;

co_rpdo_t* CoRpdoErr::pdo_ = nullptr;
co_unsigned16_t CoRpdoErr::eec_ = CO_UNSIGNED16_MAX;
co_unsigned8_t CoRpdoErr::er_ = CO_UNSIGNED8_MAX;
void* CoRpdoErr::data_ = nullptr;

void
CoRpdoErr::Func(co_rpdo_t* const pdo, const co_unsigned16_t eec,
                const co_unsigned8_t er, void* const data) {
  num_called_++;

  pdo_ = pdo;
  eec_ = eec;
  er_ = er;
  data_ = data;
}

void
CoRpdoErr::Clear() {
  num_called_ = 0u;

  pdo_ = nullptr;
  eec_ = CO_UNSIGNED16_MAX;
  er_ = CO_UNSIGNED8_MAX;
  data_ = nullptr;
}

void
CoRpdoErr::Check(const co_rpdo_t* const pdo, const co_unsigned16_t eec,
                 const co_unsigned8_t er, const void* const data) {
  CHECK_COMPARE(num_called_, >, 0);
  POINTERS_EQUAL(pdo, pdo_);
  CHECK_EQUAL(eec, eec_);
  CHECK_EQUAL(er, er_);
  POINTERS_EQUAL(data, data_);
}
