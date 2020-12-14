/**@file
 * This is the internal header file of the utilities library.
 *
 * @copyright 2013-2020 Lely Industries N.V.
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

#ifndef LELY_UTIL_INTERN_UTIL_H_
#define LELY_UTIL_INTERN_UTIL_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lely/util/util.h>

#if _WIN32
#include <windows.h>
#elif defined(_POSIX_C_SOURCE)
#include <unistd.h>
#endif

#ifndef ALIGNED_ALLOC_BITS
/**
 * The minimum alignment (in bits) of heap-allocated objects. This corresponds
 * to the number of least-significant bits in a pointer guaranteed to be zero.
 */
#if __WORDSIZE == 64
#define ALIGNED_ALLOC_BITS 4
#else
#define ALIGNED_ALLOC_BITS 3
#endif
#endif

#ifndef ALIGNED_ALLOC_SIZE
/// The minimum alignment (in bytes) of heap-allocated objects.
#define ALIGNED_ALLOC_SIZE ((size_t)(1 << (ALIGNED_ALLOC_BITS)))
#endif

#ifdef __cplusplus
// clang-format off
namespace lely {
/// The global namespace for the utilities library.
namespace util {}
}  // namespace lely
// clang-format on
#endif

#endif // !LELY_UTIL_INTERN_UTIL_H_
