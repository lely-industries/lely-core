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

#ifndef LELY_UNIT_TEST_HPP_
#define LELY_UNIT_TEST_HPP_

#include <lely/can/msg.h>
#include <lely/co/sdo.h>
#include <lely/co/type.h>

#include "co-csdo-dn-con.hpp"
#include "co-csdo-up-con.hpp"
#include "co-sub-dn-ind.hpp"
#include "co-sub-up-ind.hpp"
#include "can-send.hpp"
#include "sdo-create-message.hpp"
#include "sdo-consts.hpp"
#include "sdo-init-expected-data.hpp"

namespace LelyUnitTest {
/**
 * Sets empty handlers for all diagnostic messages from lely-core library.
 *
 * @see diag_set_handler(), diag_at_set_handler()
 */
void DisableDiagnosticMessages();
}  // namespace LelyUnitTest

#endif  // LELY_UNIT_TEST_HPP_
