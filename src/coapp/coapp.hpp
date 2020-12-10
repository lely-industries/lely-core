/**@file
 * This is the internal header file of the C++ CANopen application library.
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

#ifndef LELY_COAPP_INTERN_COAPP_HPP_
#define LELY_COAPP_INTERN_COAPP_HPP_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lely/features.h>

// Force the use of the C interface of the CANopen library.
#undef LELY_NO_CXX
#define LELY_NO_CXX 1

#if LELY_NO_ERRNO || LELY_NO_MALLOC
#error This file requires errno and/or dynamic memory allocation.
#endif

namespace lely {
/// The namespace for the C++ CANopen application library.
namespace canopen {
/**
 * The namespace for implementation details of the C++ CANopen application
 * library.
 */
namespace detail {}
}  // namespace canopen
}  // namespace lely

LELY_INGORE_EMPTY_TRANSLATION_UNIT

#endif  // LELY_COAPP_INTERN_COAPP_HPP_
