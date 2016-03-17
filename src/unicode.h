/*!\file
 * This is the internal header file of the Unicode functions.
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

#ifndef LELY_UTIL_INTERN_UNICODE_H
#define LELY_UTIL_INTERN_UNICODE_H

#include "util.h"
#include <lely/libc/uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Returns the number of bytes in the UTF-8 sequence beginning at \a s, or 0 on
 * error.
 */
static inline size_t utf8_bytes(const char *s);

/*!
 * Returns 1 if the UTF-32 character represents a valid Unicode code point, and
 * 0 if not.
 */
static inline int utf32_valid(char32_t c32);

static inline size_t
utf8_bytes(const char *s)
{
	switch (((unsigned char)*s >> 4) & 0x0f) {
	case 0x0: case 0x1: case 0x2: case 0x3:
	case 0x4: case 0x5: case 0x6: case 0x7:
		return 1;
	case 0xc: case 0xd:
		return 2;
	case 0xe:
		return 3;
	case 0xf:
		return 4;
	default:
		return 0;
	}
}

static inline int
utf32_valid(char32_t c32)
{
	return (c32 <= 0xd7ff || c32 >= 0xe000) && c32 <= 0x10ffff;
}

#ifdef __cplusplus
}
#endif

#endif

