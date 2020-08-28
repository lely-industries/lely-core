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

#include "lelyco-val.hpp"

#ifdef HAVE_LELY_OVERRIDE

#include <type_traits>

#include <dlfcn.h>

#include <lely/co/type.h>
#include <lely/co/val.h>

#include "override-test-plugin.hpp"
#include "call-wrapper.hpp"

using Override::OverridePlugin;

/* 1. Initialize valid calls global variable for each function. */
static int co_val_read_vc = Override::AllCallsValid;
static int co_val_write_vc = Override::AllCallsValid;
static int co_val_make_vc = Override::AllCallsValid;
static int co_val_init_min_vc = Override::AllCallsValid;
static int co_val_init_max_vc = Override::AllCallsValid;
static int co_val_init_vc = Override::AllCallsValid;

void
LelyOverride::co_val_read(int valid_calls) {
  OverridePlugin::getCurrent()->setForNextTest(co_val_read_vc, valid_calls);
}

void
LelyOverride::co_val_write(int valid_calls) {
  OverridePlugin::getCurrent()->setForNextTest(co_val_write_vc, valid_calls);
}

void
LelyOverride::co_val_make(int valid_calls) {
  OverridePlugin::getCurrent()->setForNextTest(co_val_make_vc, valid_calls);
}

void
LelyOverride::co_val_init_min(int valid_calls) {
  OverridePlugin::getCurrent()->setForNextTest(co_val_init_min_vc, valid_calls);
}

void
LelyOverride::co_val_init_max(int valid_calls) {
  OverridePlugin::getCurrent()->setForNextTest(co_val_init_max_vc, valid_calls);
}

void
LelyOverride::co_val_init(int valid_calls) {
  OverridePlugin::getCurrent()->setForNextTest(co_val_init_vc, valid_calls);
}

extern "C" {

#if !LELY_ENABLE_SHARED
/* 2. Extern "real" function signature. */
extern decltype(co_val_read) __real_co_val_read;
extern decltype(co_val_write) __real_co_val_write;
extern decltype(co_val_make) __real_co_val_make;
extern decltype(co_val_init_min) __real_co_val_init_min;
extern decltype(co_val_init_max) __real_co_val_init_max;
extern decltype(co_val_init) __real_co_val_init;
#endif

/* 3. Override function definition with both exact and "wrap" version. */

size_t
LELY_OVERRIDE(co_val_read)(co_unsigned16_t type, void* val,
                           const uint_least8_t* begin,
                           const uint_least8_t* end) {
  auto fun = LELY_WRAP_CALL_TO(co_val_read);
  if (!fun.IsCallValid(co_val_read_vc)) return 0;
  return fun.call(type, val, begin, end);
}

size_t
LELY_OVERRIDE(co_val_write)(co_unsigned16_t type, const void* val,
                            uint_least8_t* begin, uint_least8_t* end) {
  auto fun = LELY_WRAP_CALL_TO(co_val_write);
  if (!fun.IsCallValid(co_val_write_vc)) return 0;
  return fun.call(type, val, begin, end);
}

size_t
LELY_OVERRIDE(co_val_make)(co_unsigned16_t type, void* val, const void* ptr,
                           size_t n) {
  auto fun = LELY_WRAP_CALL_TO(co_val_make);
  if (!fun.IsCallValid(co_val_make_vc)) return 0;
  return fun.call(type, val, ptr, n);
}

int
LELY_OVERRIDE(co_val_init_min)(co_unsigned16_t type, void* val) {
  auto fun = LELY_WRAP_CALL_TO(co_val_init_min);
  if (!fun.IsCallValid(co_val_init_min_vc)) return -1;
  return fun.call(type, val);
}

int
LELY_OVERRIDE(co_val_init_max)(co_unsigned16_t type, void* val) {
  auto fun = LELY_WRAP_CALL_TO(co_val_init_max);
  if (!fun.IsCallValid(co_val_init_max_vc)) return -1;
  return fun.call(type, val);
}

int
LELY_OVERRIDE(co_val_init)(co_unsigned16_t type, void* val) {
  auto fun = LELY_WRAP_CALL_TO(co_val_init);
  if (!fun.IsCallValid(co_val_init_vc)) return -1;
  return fun.call(type, val);
}

}  // extern "C"

#endif  // HAVE_LELY_OVERRIDE
