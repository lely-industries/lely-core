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
#ifndef LELY_UNIT_TEST_HPP_
#define LELY_UNIT_TEST_HPP_

#include <lely/util/diag.h>

namespace LelyUnitTest {
/**
 * Set empty handlers for all diagnostic messages from lely-core library.
 *
 * @see diag_set_handler(), diag_at_set_handler()
 */
inline void
DisableDiagnosticMessages() {
#if LELY_NO_DIAG
  // enforce coverage in NO_DIAG mode
  diag(DIAG_DEBUG, 0, "Message suppressed");
  diag_if(DIAG_DEBUG, 0, nullptr, "Message suppressed");
#else
  diag_set_handler(nullptr, nullptr);
  diag_at_set_handler(nullptr, nullptr);
#endif
}
}  // namespace LelyUnitTest

#endif  // LELY_UNIT_TEST_HPP_
