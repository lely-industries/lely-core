/**@file
 * This is the internal header file of the C11 and POSIX compatibility library.
 *
 * @copyright 2013-2019 Lely Industries N.V.
 *
 * @author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#ifndef LELY_LIBC_INTERN_LIBC_H_
#define LELY_LIBC_INTERN_LIBC_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lely/features.h>

#ifdef __cplusplus
// clang-format off
namespace lely {
/// The global namespace for the compatibility library.
namespace compat {}
}  // namespace lely
// clang-format on
#endif

LELY_INGORE_EMPTY_TRANSLATION_UNIT

#if _WIN32

/**
 * The maximum number of milliseconds to be spent in `SleepEx()` and
 * `SleepConditionVariableCS()`. This value MUST be smaller than `INFINITE`, to
 * prevent an infinite sleep.
 */
#define MAX_SLEEP_MS (ULONG_MAX - 1)

#endif // _WIN32

#endif // !LELY_LIBC_INTERN_LIBC_H_
