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

#include <dlfcn.h>

#include <lely/co/type.h>

/* 1. Initialize valid calls global variable for each function. */
int LelyOverride::co_val_read_vc = LelyOverride::AllCallsValid;
int LelyOverride::co_val_write_vc = LelyOverride::AllCallsValid;
int LelyOverride::co_val_make_vc = LelyOverride::AllCallsValid;

extern "C" {

#if LELY_ENABLE_SHARED
/* 2a. Define function type */
typedef size_t co_val_read_t(co_unsigned16_t, void*, const uint_least8_t*,
                             const uint_least8_t*);
typedef size_t co_val_write_t(co_unsigned16_t, const void*, uint_least8_t*,
                              uint_least8_t*);
typedef size_t co_val_make_t(co_unsigned16_t, void*, const void*, size_t);
#else
/* 2b. Extern "real" function signature. */
extern size_t __real_co_val_read(co_unsigned16_t type, void* val,
                                 const uint_least8_t* begin,
                                 const uint_least8_t* end);
extern size_t __real_co_val_write(co_unsigned16_t type, const void* val,
                                  uint_least8_t* begin, uint_least8_t* end);
extern size_t __real_co_val_make(co_unsigned16_t type, void* val,
                                 const void* ptr, size_t n);
#endif

/* 3. Override function definition with both exact and "wrap" version. */

#if LELY_ENABLE_SHARED
size_t
co_val_read(co_unsigned16_t type, void* val, const uint_least8_t* begin,
            const uint_least8_t* end)
#else
size_t
__wrap_co_val_read(co_unsigned16_t type, void* val, const uint_least8_t* begin,
                   const uint_least8_t* end)
#endif
{
  if (LelyOverride::co_val_read_vc == LelyOverride::NoneCallsValid) return 0;

  if (LelyOverride::co_val_read_vc > LelyOverride::NoneCallsValid)
    --LelyOverride::co_val_read_vc;

#if LELY_ENABLE_SHARED
  co_val_read_t* const orig_co_val_read =
      reinterpret_cast<co_val_read_t*>(dlsym(RTLD_NEXT, "co_val_read"));
  return orig_co_val_read(type, val, begin, end);
#else
  return __real_co_val_read(type, val, begin, end);
#endif
}

#if LELY_ENABLE_SHARED
size_t
co_val_write(co_unsigned16_t type, const void* val, uint_least8_t* begin,
             uint_least8_t* end)
#else
size_t
__wrap_co_val_write(co_unsigned16_t type, const void* val, uint_least8_t* begin,
                    uint_least8_t* end)
#endif
{
  if (LelyOverride::co_val_write_vc == LelyOverride::NoneCallsValid) return 0;

  if (LelyOverride::co_val_write_vc > LelyOverride::NoneCallsValid)
    --LelyOverride::co_val_write_vc;

#if LELY_ENABLE_SHARED
  co_val_write_t* const orig_co_val_write =
      reinterpret_cast<co_val_write_t*>(dlsym(RTLD_NEXT, "co_val_write"));
  return orig_co_val_write(type, val, begin, end);
#else
  return __real_co_val_write(type, val, begin, end);
#endif
}

#if LELY_ENABLE_SHARED
size_t
co_val_make(co_unsigned16_t type, void* val, const void* ptr, size_t n)
#else
size_t
__wrap_co_val_make(co_unsigned16_t type, void* val, const void* ptr, size_t n)
#endif
{
  if (LelyOverride::co_val_make_vc == LelyOverride::NoneCallsValid) return 0;

  if (LelyOverride::co_val_make_vc > LelyOverride::NoneCallsValid)
    --LelyOverride::co_val_make_vc;

#if LELY_ENABLE_SHARED
  co_val_make_t* const orig_co_val_make =
      reinterpret_cast<co_val_make_t*>(dlsym(RTLD_NEXT, "co_val_make"));
  return orig_co_val_make(type, val, ptr, n);
#else
  return __real_co_val_make(type, val, ptr, n);
#endif
}

}  // extern "C"

#endif  // HAVE_LELY_OVERRIDE
