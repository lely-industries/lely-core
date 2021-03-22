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

#ifndef LELY_OVERRIDE_CALL_WRAPPER_H_
#define LELY_OVERRIDE_CALL_WRAPPER_H_

#include <type_traits>
#include <utility>

#if LELY_ENABLE_SHARED
#include <dlfcn.h>
#endif

#include "defs.hpp"

namespace Override {

template <typename F>
class CallWrapper {
 public:
#if LELY_ENABLE_SHARED
  explicit CallWrapper(const char* name)
      : fun_(reinterpret_cast<F>(dlsym(RTLD_NEXT, name))) {}
#else
  explicit CallWrapper(F f) : fun_(f) {}
#endif

  bool
  IsCallValid(int& valid_calls) {
    if (valid_calls == Override::NoneCallsValid) return false;

    if (valid_calls > Override::NoneCallsValid) --valid_calls;

    return true;
  }

  template <typename... Args>
  auto
  call(Args&&... args) -> typename std::result_of<F(Args...)>::type {
    return fun_(std::forward<Args>(args)...);
  }

 private:
  const F fun_;
};
}  // namespace Override

#if LELY_ENABLE_SHARED
#define LELY_WRAP_CALL_TO(F) Override::CallWrapper<decltype(F)*>("" #F)
#define LELY_OVERRIDE(F) F
#else
#define LELY_WRAP_CALL_TO(F) Override::CallWrapper<decltype(F)*>(__real_##F)
#define LELY_OVERRIDE(F) __wrap_##F
#endif

#endif  // LELY_OVERRIDE_CALL_WRAPPER_H_
