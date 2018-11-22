/**@file
 * This header file is part of the utilities library; it contains the IEEE 754
 * floating-point format type definitions.
 *
 * @copyright 2018 Lely Industries N.V.
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

#ifndef LELY_UTIL_FLOAT_H_
#define LELY_UTIL_FLOAT_H_

#include <lely/features.h>

#include <float.h>

#ifndef LELY_FLT16_TYPE
#if FLT_MANT_DIG == 11 && FLT_MAX_EXP == 16
#define LELY_FLT16_TYPE float
#elif DBL_MANT_DIG == 11 && DBL_MAX_EXP == 16
#define LELY_FLT16_TYPE double
#elif LDBL_MANT_DIG == 11 && LDBL_MAX_EXP == 16
#define LELY_FLT16_TYPE long double
#endif
#endif

#ifdef LELY_FLT16_TYPE
/// The IEEE 754 half-precision binary floating-point format type (binary16).
typedef LELY_FLT16_TYPE flt16_t;
#endif

#ifndef LELY_FLT32_TYPE
#if FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128
#define LELY_FLT32_TYPE float
#elif DBL_MANT_DIG == 24 && DBL_MAX_EXP == 128
#define LELY_FLT32_TYPE double
#elif LDBL_MANT_DIG == 24 && LDBL_MAX_EXP == 128
#define LELY_FLT32_TYPE long double
#endif
#endif

#ifdef LELY_FLT32_TYPE
/// The IEEE 754 single-precision binary floating-point format type (binary32).
typedef LELY_FLT32_TYPE flt32_t;
#endif

#ifndef LELY_FLT64_TYPE
#if FLT_MANT_DIG == 53 && FLT_MAX_EXP == 1024
#define LELY_FLT64_TYPE float
#elif DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024
#define LELY_FLT64_TYPE double
#elif LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
#define LELY_FLT64_TYPE long double
#endif
#endif

#ifdef LELY_FLT64_TYPE
/// The IEEE 754 double-precision binary floating-point format type (binary64).
typedef LELY_FLT64_TYPE flt64_t;
#endif

#endif // !LELY_UTIL_FLOAT_H_
