/**@file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the static device description. See lely/co/sdev.h for the C
 * interface.
 *
 * @copyright 2016-2018 Lely Industries N.V.
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

#ifndef LELY_CO_SDEV_HPP_
#define LELY_CO_SDEV_HPP_

#if !defined(__cplusplus) || LELY_NO_CXX
#error "include <lely/co/sdev.h> for the C interface"
#endif

#include <lely/co/dev.hpp>
#include <lely/co/sdev.h>

namespace lely {

inline typename c_type_traits<__co_dev>::pointer
c_type_traits<__co_dev>::init(pointer p, const co_sdev* sdev) noexcept {
  return __co_dev_init_from_sdev(p, sdev);
}

}  // namespace lely

#endif  // !LELY_CO_SDEV_HPP_
