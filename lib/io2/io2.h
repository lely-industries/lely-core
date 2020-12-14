/**@file
 * This is the internal header file of the I/O library.
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

#ifndef LELY_IO2_INTERN_IO2_H_
#define LELY_IO2_INTERN_IO2_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lely/io2/io2.h>

#ifdef __cplusplus
// clang-format off
namespace lely {
/// The global namespace for the I/O library.
namespace io {}
}  // namespace lely
// clang-format on
#endif

LELY_INGORE_EMPTY_TRANSLATION_UNIT

#endif // !LELY_IO2_INTERN_IO2_H_
