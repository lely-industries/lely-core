/**@file
 * This header file is part of the CANopen library; it contains the C++
 * interface of the Server-SDO declarations. See lely/co/ssdo.h for the C
 * interface.
 *
 * @copyright 2016-2020 Lely Industries N.V.
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

#ifndef LELY_CO_SSDO_HPP_
#define LELY_CO_SSDO_HPP_

#if !defined(__cplusplus) || LELY_NO_CXX
#error "include <lely/co/ssdo.h> for the C interface"
#endif

#include <lely/can/net.hpp>
#include <lely/co/ssdo.h>

namespace lely {

/// The attributes of #co_ssdo_t required by #lely::COSSDO.
template <>
struct c_type_traits<__co_ssdo> {
  typedef __co_ssdo value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  static void*
  alloc() noexcept {
    return __co_ssdo_alloc();
  }

  static void
  free(void* ptr) noexcept {
    __co_ssdo_free(ptr);
  }

  static pointer
  init(pointer p, CANNet* net, CODev* dev, co_unsigned8_t num) noexcept {
    return __co_ssdo_init(p, net, dev, num);
  }

  static void
  fini(pointer p) noexcept {
    __co_ssdo_fini(p);
  }
};

/// An opaque CANopen Server-SDO service type.
class COSSDO : public incomplete_c_type<__co_ssdo> {
  typedef incomplete_c_type<__co_ssdo> c_base;

 public:
  COSSDO(CANNet* net, CODev* dev, co_unsigned8_t num) : c_base(net, dev, num) {}

  int
  start() noexcept {
    return co_ssdo_start(this);
  }

  void
  stop() noexcept {
    co_ssdo_stop(this);
  }

  CANNet*
  getNet() const noexcept {
    return co_ssdo_get_net(this);
  }

  CODev*
  getDev() const noexcept {
    return co_ssdo_get_dev(this);
  }

  co_unsigned8_t
  getNum() const noexcept {
    return co_ssdo_get_num(this);
  }

  const co_sdo_par&
  getPar() const noexcept {
    return *co_ssdo_get_par(this);
  }

  int
  getTimeout() const noexcept {
    return co_ssdo_get_timeout(this);
  }

  void
  setTimeout(int timeout) noexcept {
    co_ssdo_set_timeout(this, timeout);
  }

 protected:
  ~COSSDO() = default;
};

}  // namespace lely

#endif  // !LELY_CO_SSDO_HPP_
