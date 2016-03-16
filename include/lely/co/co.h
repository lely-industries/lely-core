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

struct __co_obj;
#ifndef __cplusplus
//! An opaque CANopen object type.
typedef struct __co_obj co_obj_t;
#endif

struct __co_sub;
#ifndef __cplusplus
//! An opaque CANopen sub-object type.
typedef struct __co_sub co_sub_t;
#endif

#endif

