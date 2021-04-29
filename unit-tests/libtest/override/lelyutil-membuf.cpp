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

#include "lelyutil-membuf.hpp"

#ifdef HAVE_LELY_OVERRIDE

#include <type_traits>

#include <dlfcn.h>

#include <lely/util/membuf.h>

#include "override-test-plugin.hpp"
#include "call-wrapper.hpp"

using Override::OverridePlugin;

/* 1. Initialize valid calls global variable for the function. */
static int membuf_reserve_vc = Override::AllCallsValid;

void
LelyOverride::membuf_reserve(int valid_calls) {
  OverridePlugin::getCurrent()->setForNextTest(membuf_reserve_vc, valid_calls);
}

extern "C" {

#if !LELY_ENABLE_SHARED
/* 2. Extern "real" function signature. */
extern decltype(membuf_reserve) __real_membuf_reserve;
#endif

/* 3. Override function definition with both exact and "wrap" version. */

size_t
LELY_OVERRIDE(membuf_reserve)(membuf* buf, size_t size) {
  auto fun = LELY_WRAP_CALL_TO(membuf_reserve);
  if (!fun.IsCallValid(membuf_reserve_vc)) return 0;
  return fun.call(buf, size);
}

}  // extern "C"

#endif  // HAVE_LELY_OVERRIDE
