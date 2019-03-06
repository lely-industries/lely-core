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

#define LELY_COAPP_INTERN 1
#include <lely/coapp/coapp.hpp>

#include <lely/util/errnum.h>

#include <system_error>

namespace lely {

namespace canopen {

namespace {

_Noreturn inline void
throw_errc(const char* what, int errc = get_errc()) {
  throw ::std::system_error(errc, ::std::system_category(), what);
}

}  // namespace

}  // namespace canopen

}  // namespace lely

#endif  // LELY_COAPP_INTERN_COAPP_HPP_
