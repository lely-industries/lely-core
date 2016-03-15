/*!\file
 * This is the public header file of the utilities library.
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

#ifndef LELY_UTIL_UTIL_H
#define LELY_UTIL_UTIL_H

#include <lely/libc/libc.h>

#ifndef LELY_UTIL_EXTERN
#ifdef DLL_EXPORT
#ifdef LELY_UTIL_INTERN
#define LELY_UTIL_EXTERN	extern __dllexport
#else
#define LELY_UTIL_EXTERN	extern __dllimport
#endif
#else
#define LELY_UTIL_EXTERN	extern
#endif
#endif

#ifndef MIN
/*!
 * Returns the minimum of \a a and \a b. Guaranteed to return the opposite of
 * MAX(), i.e., if MAX() returns \a a then MIN() returns \a b and vice versa.
 */
#define MIN(a, b)	((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
/*!
 * Returns the maximum of \a a and \a b. Guaranteed to return the opposite of
 * MIN().
 */
#define MAX(a, b)	((a) < (b) ? (b) : (a))
#endif

#endif

