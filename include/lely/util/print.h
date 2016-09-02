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

/*!
 * Prints the Base64 representation of binary data to a memory buffer. This
 * function implements the MIME variant of Base64 as specified in
 * <a href="https://tools.ietf.org/html/rfc2045">RFC 2045</a>.
 *
 * \param ptr    a pointer to the binary data to be encoded and written.
 * \param n      the number of bytes at \a ptr.
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
 */
LELY_UTIL_EXTERN size_t print_base64(const void *ptr, size_t n, char **pbegin,
		char *end);

LELY_UTIL_PRINT_INLINE int
otoc(int i)
{
#if __STDC_ISO_10646__ && !__STDC_MB_MIGHT_NEQ_WC__
	return '0' + (i & 7);
#else
	switch (i & 7) {
	case 0: return '0';
	case 1: return '1';
	case 2: return '2';
	case 3: return '3';
	case 4: return '4';
	case 5: return '5';
	case 6: return '6';
	case 7: return '7';
	default: return -1;
	}
#endif
}

LELY_UTIL_PRINT_INLINE int
xtoc(int i)
{
#if __STDC_ISO_10646__ && !__STDC_MB_MIGHT_NEQ_WC__
	i &= 0xf;
	return i < 10 ? '0' + i : 'a' + i - 10;
#else
	switch (i & 0xf) {
	case 0x0: return '0';
	case 0x1: return '1';
	case 0x2: return '2';
	case 0x3: return '3';
	case 0x4: return '4';
	case 0x5: return '5';
	case 0x6: return '6';
	case 0x7: return '7';
	case 0x8: return '8';
	case 0x9: return '9';
	case 0xa: return 'a';
	case 0xb: return 'b';
	case 0xc: return 'c';
	case 0xd: return 'd';
	case 0xe: return 'e';
	case 0xf: return 'f';
	default: return -1;
	}
#endif
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

