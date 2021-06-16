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

#include <lely/co/dev.h>
#include <lely/co/detail/obj.h>
#include <lely/co/obj.h>

#include "co-sub-up-ind.hpp"

unsigned int CoSubUpInd::num_called = 0u;
const co_sub_t* CoSubUpInd::sub = nullptr;
co_sdo_req* CoSubUpInd::req = nullptr;
co_unsigned32_t CoSubUpInd::ac = 0u;
void* CoSubUpInd::data = nullptr;

co_unsigned32_t
CoSubUpInd::Func(const co_sub_t* sub_, co_sdo_req* req_, co_unsigned32_t ac_,
                 void* data_) {
  num_called++;

  sub = sub_;
  req = req_;
  ac = ac_;
  data = data_;

  co_sub_on_up(sub, req, &ac);

  return ac;
}

void
CoSubUpInd::Clear() {
  num_called = 0;

  sub = nullptr;
  req = nullptr;
  ac = 0;
  data = nullptr;
}
