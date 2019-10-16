/**@file
 * This is the internal header file of the fiber implementation.
 *
 * @see lely/util/fiber.h
 *
 * @copyright 2018-2019 Lely Industries N.V.
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

#ifndef LELY_UTIL_INTERN_FIBER_H_
#define LELY_UTIL_INTERN_FIBER_H_

#include "util.h"

#ifndef LELY_FIBER_MINSTKSZ
/// The minimum size (in bytes) of a fiber stack frame.
#ifdef MINSIGSTKSZ
#define LELY_FIBER_MINSTKSZ MINSIGSTKSZ
#elif __WORDSIZE == 64
#define LELY_FIBER_MINSTKSZ 8192
#else
#define LELY_FIBER_MINSTKSZ 4096
#endif
#endif

#ifndef LELY_FIBER_STKSZ
/// The default size (in bytes) of a fiber stack frame.
#define LELY_FIBER_STKSZ 131072
#endif

#endif // !LELY_UTIL_INTERN_FIBER_H_
