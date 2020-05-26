/**@file
 * This is the public header file of the CANopen library.
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

#ifndef LELY_CO_CO_H_
#define LELY_CO_CO_H_

#include <lely/util/util.h>

struct __co_dev;
#if defined(__cplusplus) && !LELY_NO_CXX
// clang-format off
namespace lely { class CODev; }
// clang-format on
typedef lely::CODev co_dev_t;
#else
/// An opaque CANopen device type.
typedef struct __co_dev co_dev_t;
#endif

struct __co_obj;
#if defined(__cplusplus) && !LELY_NO_CXX
// clang-format off
namespace lely { class COObj; }
// clang-format on
typedef lely::COObj co_obj_t;
#else
/// An opaque CANopen object type.
typedef struct __co_obj co_obj_t;
#endif

struct __co_sub;
#if defined(__cplusplus) && !LELY_NO_CXX
// clang-format off
namespace lely { class COSub; }
// clang-format on
typedef lely::COSub co_sub_t;
#else
/// An opaque CANopen sub-object type.
typedef struct __co_sub co_sub_t;
#endif

struct __co_ssdo;
#if defined(__cplusplus) && !LELY_NO_CXX
// clang-format off
namespace lely { class COSSDO; }
// clang-format on
typedef lely::COSSDO co_ssdo_t;
#else
/// An opaque CANopen Server-SDO service type.
typedef struct __co_ssdo co_ssdo_t;
#endif

struct __co_csdo;
#if defined(__cplusplus) && !LELY_NO_CXX
// clang-format off
namespace lely { class COCSDO; }
// clang-format on
typedef lely::COCSDO co_csdo_t;
#else
/// An opaque CANopen Client-SDO service type.
typedef struct __co_csdo co_csdo_t;
#endif

struct __co_rpdo;
#if defined(__cplusplus) && !LELY_NO_CXX
// clang-format off
namespace lely { class CORPDO; }
// clang-format on
typedef lely::CORPDO co_rpdo_t;
#else
/// An opaque CANopen Receive-PDO service type.
typedef struct __co_rpdo co_rpdo_t;
#endif

struct __co_tpdo;
#if defined(__cplusplus) && !LELY_NO_CXX
// clang-format off
namespace lely { class COTPDO; }
// clang-format on
typedef lely::COTPDO co_tpdo_t;
#else
/// An opaque CANopen Transmit-PDO service type.
typedef struct __co_tpdo co_tpdo_t;
#endif

struct __co_sync;
#if defined(__cplusplus) && !LELY_NO_CXX
// clang-format off
namespace lely { class COSync; }
// clang-format on
typedef lely::COSync co_sync_t;
#else
/// An opaque CANopen SYNC producer/consumer service type.
typedef struct __co_sync co_sync_t;
#endif

struct __co_time;
#if defined(__cplusplus) && !LELY_NO_CXX
// clang-format off
namespace lely { class COTime; }
// clang-format on
typedef lely::COTime co_time_t;
#else
/// An opaque CANopen TIME producer/consumer service type.
typedef struct __co_time co_time_t;
#endif

struct __co_emcy;
#if defined(__cplusplus) && !LELY_NO_CXX
// clang-format off
namespace lely { class COEmcy; }
// clang-format on
typedef lely::COEmcy co_emcy_t;
#else
/// An opaque CANopen EMCY producer/consumer service type.
typedef struct __co_emcy co_emcy_t;
#endif

struct __co_nmt;
#if defined(__cplusplus) && !LELY_NO_CXX
// clang-format off
namespace lely { class CONMT; }
// clang-format on
typedef lely::CONMT co_nmt_t;
#else
/// An opaque CANopen NMT master/slave service type.
typedef struct __co_nmt co_nmt_t;
#endif

struct __co_lss;
#if defined(__cplusplus) && !LELY_NO_CXX
// clang-format off
namespace lely { class COLSS; }
// clang-format on
typedef lely::COLSS co_lss_t;
#else
/// An opaque CANopen LSS master/slave service type.
typedef struct __co_lss co_lss_t;
#endif

#endif // !LELY_CO_CO_H_
