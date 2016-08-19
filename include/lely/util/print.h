/*!\file
 * This header file is part of the utilities library; it contains the printing
 * function declarations.
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

#ifndef LELY_UTIL_PRINT_H
#define LELY_UTIL_PRINT_H

#include <lely/libc/uchar.h>
#include <lely/util/util.h>

#ifndef LELY_UTIL_PRINT_INLINE
#define LELY_UTIL_PRINT_INLINE	inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

//! Returns the character corresponding to the octal digit \a i. \see ctoo()
LELY_UTIL_PRINT_INLINE int otoc(int i);

/*!
 * Returns the character corresponding to the hexadecimal digit \a i.
 *
 * \see ctox()
 */
LELY_UTIL_PRINT_INLINE int xtoc(int i);

/*!
 * Prints a single character to a memory buffer.
 *
 * \param c      the character to be written.
 * \param pbegin the address of a pointer to the start of the buffer. If
 *               \a pbegin or *\a pbegin is NULL, nothing is written; Otherwise,
 *               on exit, *\a pbegin points to one past the last character
 *               written.
 * \param end    a pointer to the end of the buffer. If \a end is not NULL, at
 *               most `end - *pbegin` characters are written.
 *
 * \returns 1.
 */
LELY_UTIL_PRINT_INLINE size_t print_char(int c, char **pbegin, char *end);

/*!
 * Prints a UTF-8 encoded Unicode character to a memory buffer. Illegal Unicode
 * code points are silently replaced by the Unicode replacement character
 * (U+FFFD). Note that the printed UTF-8 sequence is _not_ null-terminated.
 *
 * \param c32    the Unicode character to be written.
 * \param pbegin the address of a pointer to the start of the buffer. If
 *               \a pbegin or *\a pbegin is NULL, nothing is written; Otherwise,
 *               on exit, *\a pbegin points to one past the last character
 *               written.
 * \param end    a pointer to the end of the buffer. If \a end is not NULL, at
 *               most `end - *pbegin` characters are written, and the output may
 *               be truncated.
 *
 * \returns the number of characters that would have been written had the buffer
 * been sufficiently large.
 *
 * \see print_c99_esc()
 */
LELY_UTIL_EXTERN size_t print_utf8(char32_t c32, char **pbegin, char *end);

/*!
 * Prints a UTF-8 encoded Unicode character to a memory buffer. Non-printable
 * ASCII characters are printed using C99 escape sequences, illegal Unicode code
 * points using hexadecimal escape sequences. Note that the printed UTF-8
 * sequence is _not_ null-terminated.
 *
 * \param c32    the Unicode character to be written.
 * \param pbegin the address of a pointer to the start of the buffer. If
 *               \a pbegin or *\a pbegin is NULL, nothing is written; Otherwise,
 *               on exit, *\a pbegin points to one past the last character
 *               written.
 * \param end    a pointer to the end of the buffer. If \a end is not NULL, at
 *               most `end - *pbegin` characters are written, and the output may
 *               be truncated.
 *
 * \returns the number of characters that would have been written had the buffer
 * been sufficiently large.
 *
 * \see print_utf8()
 */
LELY_UTIL_EXTERN size_t print_c99_esc(char32_t c32, char **pbegin, char *end);

LELY_UTIL_PRINT_INLINE int
otoc(int i)
{
	return '0' + (i & 7);
}

LELY_UTIL_PRINT_INLINE int
xtoc(int i)
{
	i &= 0xf;
	return i < 10 ? '0' + i : 'a' + i - 10;
}

LELY_UTIL_PRINT_INLINE size_t
print_char(int c, char **pbegin, char *end)
{
	if (pbegin && *pbegin && (!end || *pbegin < end))
		*(*pbegin)++ = (unsigned char)c;
	return 1;
}

#ifdef __cplusplus
}
#endif

#endif

