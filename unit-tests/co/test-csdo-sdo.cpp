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

#include <lely/co/csdo.h>

#include <libtest/tools/lely-unit-test.hpp>

#include "holder/dev.hpp"

TEST_GROUP(CO_CsdoDnInd) {
  static const co_unsigned8_t DEV_ID = 0x01u;

  co_dev_t* dev = nullptr;

  std::unique_ptr<CoDevTHolder> dev_holder;

  TEST_SETUP() {
    dev_holder.reset(new CoDevTHolder(DEV_ID));
    dev = dev_holder->Get();
    CHECK(dev != nullptr);
  }
  TEST_TEARDOWN() { dev_holder.reset(); }
};

/// @name CSDO service: object 0x1280 modification using SDO
///@{

TEST(CO_CsdoDnInd, Co1280DnInd) {
  co_unsigned8_t val = 0u;

  const auto ret = co_dev_dn_val_req(dev, 0x1280u, 0x00u, CO_DEFTYPE_UNSIGNED8,
                                     &val, nullptr, CoCsdoDnCon::func, nullptr);

  CHECK_EQUAL(0, ret);
}

///@}