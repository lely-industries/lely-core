/*!\file
 * This is the internal header file of the CANopen library.
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

#ifndef LELY_CO_INTERN_CO_H
#define LELY_CO_INTERN_CO_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define LELY_CO_INTERN	1
#include <lely/co/co.h>

#ifndef LELY_CO_EXPORT
#ifdef DLL_EXPORT
#define LELY_CO_EXPORT	__dllexport
#else
#define LELY_CO_EXPORT
#endif
#endif

#ifndef ALIGN
/*!
 * Rounds \a x up to the nearest multiple of \a a.
 *
 * Since the rounding is performed with a bitmask, \a a MUST be a power of two.
 */
#ifdef __GNUC__
#define ALIGN(x, a)	__ALIGN_MASK((x), (__typeof__(x))(a) - 1)
#else
#define ALIGN(x, a)	__ALIGN_MASK((x), (a) - 1)
#endif
#endif

#ifndef __ALIGN_MASK
#define __ALIGN_MASK(x, mask)	(((x) + (mask)) & ~(mask))
#endif

//! The maximum number of nodes in a CANopen network.
#define CO_NUM_NODES	127

#endif

