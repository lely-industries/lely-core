/*!\file
 * This is the public header file of the CANopen library.
 *
 * \copyright 2016 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#ifndef LELY_CO_CO_H
#define LELY_CO_CO_H

#include <lely/libc/libc.h>
#include <lely/util/util.h>
#include <lely/can/can.h>

#ifndef LELY_CO_EXTERN
#ifdef DLL_EXPORT
#ifdef LELY_CO_INTERN
#define LELY_CO_EXTERN	extern __dllexport
#else
#define LELY_CO_EXTERN	extern __dllimport
#endif
#else
#define LELY_CO_EXTERN	extern
#endif
#endif

struct __co_dev;
#ifdef __cplusplus
namespace lely { class CODev; }
typedef lely::CODev co_dev_t;
#else
//! An opaque CANopen device type.
typedef struct __co_dev co_dev_t;
#endif

struct __co_obj;
#ifdef __cplusplus
namespace lely { class COObj; }
typedef lely::COObj co_obj_t;
#else
//! An opaque CANopen object type.
typedef struct __co_obj co_obj_t;
#endif

struct __co_sub;
#ifdef __cplusplus
namespace lely { class COSub; }
typedef lely::COSub co_sub_t;
#else
//! An opaque CANopen sub-object type.
typedef struct __co_sub co_sub_t;
#endif

struct __co_ssdo;
#ifdef __cplusplus
namespace lely { class COSSDO; }
typedef lely::COSSDO co_ssdo_t;
#else
//! An opaque CANopen Server-SDO service type.
typedef struct __co_ssdo co_ssdo_t;
#endif

struct __co_csdo;
#ifdef __cplusplus
namespace lely { class COCSDO; }
typedef lely::COCSDO co_csdo_t;
#else
//! An opaque CANopen Client-SDO service type.
typedef struct __co_csdo co_csdo_t;
#endif

struct __co_rpdo;
#ifdef __cplusplus
namespace lely { class CORPDO; }
typedef lely::CORPDO co_rpdo_t;
#else
//! An opaque CANopen Receive-PDO service type.
typedef struct __co_rpdo co_rpdo_t;
#endif

struct __co_tpdo;
#ifdef __cplusplus
namespace lely { class COTPDO; }
typedef lely::COTPDO co_tpdo_t;
#else
//! An opaque CANopen Transmit-PDO service type.
typedef struct __co_tpdo co_tpdo_t;
#endif

struct __co_sync;
#ifdef __cplusplus
namespace lely { class COSync; }
typedef lely::COSync co_sync_t;
#else
//! An opaque CANopen SYNC producer/consumer service type.
typedef struct __co_sync co_sync_t;
#endif

struct __co_time;
#ifdef __cplusplus
namespace lely { class COTime; }
typedef lely::COTime co_time_t;
#else
//! An opaque CANopen TIME producer/consumer service type.
typedef struct __co_time co_time_t;
#endif

struct __co_emcy;
#ifndef __cplusplus
//! An opaque CANopen EMCY producer/consumer service type.
typedef struct __co_emcy co_emcy_t;
#endif

struct __co_nmt;
#ifndef __cplusplus
//! An opaque CANopen NMT master/slave service type.
typedef struct __co_nmt co_nmt_t;
#endif

struct __co_lss;
#ifndef __cplusplus
//! An opaque CANopen LSS master/slave service type.
typedef struct __co_lss co_lss_t;
#endif

#endif

