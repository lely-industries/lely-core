/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2021 N7 Space Sp. z o.o.
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

#ifndef CO_NMT_COMMON_ALLOC_SIZES_HPP_
#define CO_NMT_COMMON_ALLOC_SIZES_HPP_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lely/can/net.h>
#include <lely/co/ssdo.h>
#include <lib/co/nmt_hb.h>
#include <lib/co/nmt_rdn.h>

#include "holder/dev.hpp"

namespace NmtCommon {
size_t
GetDcfParamsAllocSize(const co_dev_t* const dev) {
  size_t dcfs_size = 0;
  dcfs_size += co_dev_write_dcf(dev, 0x1000u, 0x1fffu, nullptr, nullptr);
#if !LELY_NO_CO_DCF_RESTORE
  dcfs_size += co_dev_write_dcf(dev, 0x2000u, 0x9fffu, nullptr, nullptr);
#endif

  return dcfs_size;
}

size_t
GetSlavesAllocSize() {
  size_t size = 0;
#if !LELY_NO_CO_MASTER
  size += can_recv_sizeof();
#if !LELY_NO_CO_NG
  size += can_timer_sizeof();
#endif
#endif  // !LELY_NO_CO_MASTER
  return CO_NUM_NODES * size;
}

size_t
GetHbConsumersAllocSize(const size_t hb_num) {
  return hb_num * (co_nmt_hb_sizeof() + can_recv_sizeof() + can_timer_sizeof());
}

size_t
GetSsdoAllocSize(const size_t ssdo_num = 1u) {
  return ssdo_num * (sizeof(co_ssdo_t*) + co_ssdo_sizeof() + can_recv_sizeof() +
                     can_timer_sizeof());
}

size_t
GetServicesAllocSize() {
  size_t size = 0;
#if LELY_NO_MALLOC && !LELY_NO_CO_SDO
  size += GetSsdoAllocSize();
#endif
  return size;
}

size_t
GetNmtTimersAllocSize() {
  size_t size = can_timer_sizeof();
#if !LELY_NO_CO_MASTER
  size += can_timer_sizeof();
#endif
  return size;
}

size_t
GetNmtRecvsAllocSize() {
  return 2u * can_recv_sizeof();
}

size_t
GetNmtRedundancyAllocSize() {
  size_t size = 0;
#if !LELY_NO_CO_ECSS_REDUNDANCY && LELY_NO_MALLOC
  size += co_nmt_rdn_sizeof();
  size += can_timer_sizeof();
#endif
  return size;
}

}  // namespace NmtCommon

#endif  // CO_NMT_COMMON_ALLOC_SIZES_HPP_
