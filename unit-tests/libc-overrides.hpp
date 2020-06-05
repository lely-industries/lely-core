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

#include <cstdio>

/* C standard library function override setup */

#if defined(__MINGW32__) || __GNUC__ == 7 || __GNUC__ == 8
/* MinGW-w64 tests won't link properly - neither overriding with "strong"
 * symbol nor using --wrap linker option works.
 *
 * Overriding also does not work on GCC 7/8 with -fprintf-return-value
 * optimization enabled (default) when using shared libraries.
 */
#else
#define HAVE_SNPRINTF_OVERRIDE 1
#endif

/* snprintf() override */
#if HAVE_SNPRINTF_OVERRIDE

static int valid_calls_snprintf = -1;  // -1 means no limit

#ifdef __GNUC__
int snprintf(char* __restrict __s, size_t __maxlen,
             const char* __restrict __format, ...) __THROWNL
    __attribute__((__format__(__printf__, 3, 4)));
#endif

int
snprintf(char* s, size_t maxlen, const char* format, ...) __THROWNL {
  if (valid_calls_snprintf == 0) return -1;

  if (valid_calls_snprintf > 0) --valid_calls_snprintf;

  va_list arg;
  va_start(arg, format);
  int ret = vsnprintf(s, maxlen, format, arg);
  va_end(arg);

  return ret;
}
#endif
/* end of snprintf() override */
